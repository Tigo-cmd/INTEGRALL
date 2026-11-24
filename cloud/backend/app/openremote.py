"""
OpenRemote sync: mirrors every Integrall device as an OpenRemote asset and
turns OpenRemote attribute writes back into Integrall commands.

    device --MQTT--> Mosquitto --> Bridge --listener--> OpenRemoteSync --> OpenRemote
    OpenRemote UI write --> OR MQTT API --> OpenRemoteSync --> Command --> device

The firmware is unchanged: devices keep talking to our Mosquitto with the
frozen contract. OpenRemote only sees assets.

Per device, one ThingAsset named after the device, with attributes:
  integrallId, online, firmware, chip, board, ip, mac, modules   (from status/meta)
  health (JSON), partsMissing, rssi, heap, uptime                 (from health)
  <telemetry key> ...                                             (created on first sight)
  lastEvent                                                       (text, history kept)
  relay<N>      boolean, WRITE to switch relay N                  (from meta.relays)
  <switch>      boolean, WRITE -> switch.set {name,on}            (from meta.switches, addCloudSwitch())
  command       JSON {"action": "...", "args": {...}}, WRITE to send any command
  lastCommand   JSON result of the last command sent from OpenRemote

Transport:
  - REST (client-credentials token) only to create assets / add attributes.
  - OpenRemote's MQTT API for value writes ({realm}/{cid}/writeattribute/...)
    and for receiving user writes ({realm}/{cid}/attributevalue/<name>/#).
Nothing is polled; updates are pushed both ways. Device updates are queued so a
slow OpenRemote can never stall the device bridge.
"""
from __future__ import annotations

import asyncio
import json
import logging
import re
import ssl
import time
import uuid
from datetime import datetime
from typing import Any, Protocol
from urllib.parse import urlparse

import aiomqtt
import httpx
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker

from .config import Settings
from .models import Command, Device, utcnow

log = logging.getLogger("integrall.openremote")

MAX_RELAYS = 8                  # matches INTEGRALL_MAX_RELAYS on ESP
QUEUE_MAX = 2000
META_TEXT = ("firmware", "chip", "board", "ip", "mac", "modules")
HEALTH_NUM = {"rssi": "rssi", "heap": "heap", "uptime_s": "uptime"}
RESERVED = {"integrallId", "online", "health", "partsMissing", "lastEvent", "command", "lastCommand",
            "location", "notes", *META_TEXT, *HEALTH_NUM.values()}
_NAME_OK = re.compile(r"[^A-Za-z0-9_]")
_RELAY = re.compile(r"^relay(\d)$")


# ----------------------------------------------------------- attribute model --
def attribute(name: str, vtype: str, value: Any = None, *, label: str | None = None,
              store: bool = False) -> dict:
    meta: dict[str, Any] = {}
    if label:
        meta["label"] = label
    if store:
        meta["storeDataPoints"] = True      # keep history -> charts in the OR UI
        meta["ruleState"] = True            # usable in OR rules/flows
    return {"name": name, "type": vtype, "value": value, "meta": meta}


def telemetry_name(key: str) -> str:
    """Telemetry key -> safe OpenRemote attribute name that can't shadow ours."""
    n = _NAME_OK.sub("_", key)[:60] or "value"
    if n[0].isdigit() or n in RESERVED or _RELAY.match(n):
        n = "t_" + n
    return n


def switch_attr(name: Any) -> str:
    """Device cloud-switch name -> OpenRemote attribute name (e.g. "buzzer")."""
    n = _NAME_OK.sub("_", str(name))[:60] or "switch"
    if n[0].isdigit() or n in RESERVED or _RELAY.match(n):
        n = "switch_" + n
    return n


def switches_of(meta: dict) -> dict[str, str]:
    """{attribute name: device switch name} from meta.switches (addCloudSwitch on the device)."""
    out: dict[str, str] = {}
    for sw in meta.get("switches") or []:
        if isinstance(sw, dict) and isinstance(sw.get("name"), str) and sw["name"]:
            out.setdefault(switch_attr(sw["name"]), sw["name"])
    return out


def value_type(v: Any) -> str | None:
    if isinstance(v, bool):
        return "boolean"
    if isinstance(v, (int, float)):
        return "number"
    if isinstance(v, str):
        return "text"
    if isinstance(v, dict):
        return "JSONObject"
    return None


def device_attributes(device: Device) -> dict[str, dict]:
    """Full attribute set for a new asset, pre-filled with what we already know."""
    attrs = [
        attribute("integrallId", "text", device.id, label="Integrall ID"),
        attribute("online", "boolean", bool(device.online), label="Online", store=True),
        attribute("health", "JSONObject", device.health, label="Health report"),
        attribute("partsMissing", "number", (device.health or {}).get("missing"), label="Parts missing"),
        attribute("lastEvent", "text", None, label="Last event", store=True),
        # "JSON" (any JSON) rather than "JSONObject": accepts "ping" and the null we clear it with.
        attribute("command", "JSON", None, label="Send command"),
        attribute("lastCommand", "JSONObject", None, label="Last command result"),
        attribute("notes", "text", None),            # required on every ThingAsset
        attribute("location", "GEO_JSONPoint", None),  # required; set it in the OR UI to show the device on the map
    ]
    attrs += [attribute(k, "text", None, label=k.capitalize()) for k in META_TEXT]
    attrs += [attribute(n, "number", None, label=n.capitalize(), store=True) for n in HEALTH_NUM.values()]
    out = {a["name"]: a for a in attrs}
    for upd in meta_values(device.meta or {}):
        _merge(out, upd)
    for upd in health_values(device.health or {}):
        _merge(out, upd)
    for upd in telemetry_values(device.last_telemetry or {}):
        _merge(out, upd)
    return out


def _merge(attrs: dict[str, dict], upd: tuple[str, str, Any, dict]) -> None:
    name, vtype, value, extra = upd
    if name not in attrs:
        attrs[name] = attribute(name, vtype, **extra)
    attrs[name]["value"] = value


# Each *_values() yields (name, valueType, value, attribute() kwargs if new).
def meta_values(meta: dict) -> list[tuple[str, str, Any, dict]]:
    out = []
    for src, dst in (("fw", "firmware"), ("chip", "chip"), ("board", "board"), ("ip", "ip"), ("mac", "mac")):
        if isinstance(meta.get(src), str):
            out.append((dst, "text", meta[src], {}))
    if isinstance(meta.get("modules"), list):
        out.append(("modules", "text", ", ".join(map(str, meta["modules"])), {}))
    for r in meta.get("relays") or []:
        if isinstance(r, dict) and isinstance(r.get("index"), int) and 0 <= r["index"] < MAX_RELAYS:
            label = f"Relay {r['index']}" + (f" ({r['name']})" if r.get("name") else "")
            out.append((f"relay{r['index']}", "boolean", bool(r.get("on")), {"label": label, "store": True}))
    for sw in meta.get("switches") or []:
        if isinstance(sw, dict) and isinstance(sw.get("name"), str) and sw["name"]:
            out.append((switch_attr(sw["name"]), "boolean", bool(sw.get("on")),
                        {"label": f"{sw['name']} (switch)", "store": True}))
    return out


def health_values(h: dict) -> list[tuple[str, str, Any, dict]]:
    out: list[tuple[str, str, Any, dict]] = [("health", "JSONObject", h, {})]
    if isinstance(h.get("missing"), (int, float)):
        out.append(("partsMissing", "number", h["missing"], {}))
    for src, dst in HEALTH_NUM.items():
        if isinstance(h.get(src), (int, float)) and not isinstance(h.get(src), bool):
            out.append((dst, "number", h[src], {}))
    return out


def telemetry_values(data: dict, avoid: set[str] | frozenset = frozenset()) -> list[tuple[str, str, Any, dict]]:
    """`avoid`: attribute names taken by this device's switches (telemetry gets a t_ prefix)."""
    out = []
    for k, v in data.items():
        vt = value_type(v)
        if vt:
            n = telemetry_name(k)
            if n in avoid:
                n = "t_" + n
            out.append((n, vt, v, {"label": k, "store": vt in ("number", "boolean")}))
    return out


# ------------------------------------------------------------------- REST ----
class AssetApi(Protocol):
    async def create(self, asset: dict) -> dict: ...
    async def get(self, asset_id: str) -> dict | None: ...
    async def update(self, asset: dict) -> dict: ...
    async def delete(self, asset_id: str) -> None: ...


class OpenRemoteError(RuntimeError):
    pass


def _check(r: httpx.Response) -> None:
    if r.is_error:
        raise OpenRemoteError(f"OpenRemote {r.request.method} {r.request.url.path} -> {r.status_code}: {r.text[:500]}")


class RestApi:
    """OpenRemote REST, authenticated as a service user (client credentials)."""

    def __init__(self, s: Settings) -> None:
        self.s = s
        self.http = httpx.AsyncClient(base_url=s.or_url.rstrip("/"), verify=s.or_verify_tls, timeout=15)
        self._token: str | None = None
        self._token_exp = 0.0

    async def _auth(self) -> dict:
        if not self._token or time.monotonic() > self._token_exp:
            r = await self.http.post(f"/auth/realms/{self.s.or_realm}/protocol/openid-connect/token", data={
                "grant_type": "client_credentials", "client_id": self.s.or_client_id,
                "client_secret": self.s.or_client_secret})
            _check(r)
            doc = r.json()
            self._token = doc["access_token"]
            self._token_exp = time.monotonic() + max(30, int(doc.get("expires_in", 300)) - 30)
        return {"Authorization": f"Bearer {self._token}"}

    async def create(self, asset: dict) -> dict:
        r = await self.http.post(f"/api/{self.s.or_realm}/asset", json=asset, headers=await self._auth())
        _check(r)
        return r.json()

    async def get(self, asset_id: str) -> dict | None:
        r = await self.http.get(f"/api/{self.s.or_realm}/asset/{asset_id}", headers=await self._auth())
        if r.status_code == 404:
            return None
        _check(r)
        return r.json()

    async def update(self, asset: dict) -> dict:
        r = await self.http.put(f"/api/{self.s.or_realm}/asset/{asset['id']}", json=asset,
                                headers=await self._auth())
        _check(r)
        return r.json() if r.content else asset

    async def delete(self, asset_id: str) -> None:
        r = await self.http.delete(f"/api/{self.s.or_realm}/asset", params={"assetId": asset_id},
                                   headers=await self._auth())
        if r.status_code != 404:
            _check(r)

    async def aclose(self) -> None:
        await self.http.aclose()


# ------------------------------------------------------------------- sync ----
class OpenRemoteSync:
    def __init__(self, settings: Settings, sessions: async_sessionmaker[AsyncSession], bridge, api: AssetApi) -> None:
        self.s = settings
        self.sessions = sessions
        self.bridge = bridge
        self.api = api
        self.client_id = f"integrall-{uuid.uuid4().hex[:8]}"
        self.queue: asyncio.Queue[tuple[str, dict]] = asyncio.Queue(QUEUE_MAX)
        self._publish = None                     # set while the OR MQTT session is up (tests inject one)
        self._asset_of: dict[str, str] = {}      # device id -> asset id
        self._device_of: dict[str, str] = {}     # asset id -> device id
        self._attrs: dict[str, set[str]] = {}    # asset id -> attribute names known to exist
        self._or_value: dict[tuple[str, str], Any] = {}   # last value we know OR holds (echo filter)
        self._switches: dict[str, dict[str, str]] = {}      # asset id -> {attribute: device switch name}
        self._subscribe = None                   # set while connected: subscribe(topic)
        self._subscribed: set[str] = set()
        self._dropped = 0
        self.connected = False

    # ---- called by the device bridge (sync, must not block) ----------------
    def notify(self, device_id: str, update: dict) -> None:
        try:
            self.queue.put_nowait((device_id, update))
        except asyncio.QueueFull:
            self._dropped += 1
            if self._dropped % 100 == 1:
                log.warning("OpenRemote queue full; dropped %d updates so far", self._dropped)

    # ---- lifecycle ----------------------------------------------------------
    async def run(self) -> None:
        """Keep an MQTT session to OpenRemote alive; process queued updates while connected."""
        host = self.s.or_mqtt_host or urlparse(self.s.or_url).hostname or "localhost"
        tls = None
        if self.s.or_mqtt_tls:
            tls = ssl.create_default_context()
            if not self.s.or_verify_tls:
                tls.check_hostname = False
                tls.verify_mode = ssl.CERT_NONE
        await self._load_links()
        backoff = 1.0
        while True:
            try:
                async with aiomqtt.Client(hostname=host, port=self.s.or_mqtt_port, identifier=self.client_id,
                                          username=f"{self.s.or_realm}:{self.s.or_client_id}",
                                          password=self.s.or_client_secret, tls_context=tls,
                                          keepalive=30) as client:
                    async def publish(topic: str, payload: str) -> None:
                        await client.publish(topic, payload, qos=1)
                    async def subscribe(topic: str) -> None:
                        await client.subscribe(topic, qos=0)
                    self._publish, self._subscribe = publish, subscribe
                    self._subscribed.clear()
                    base = f"{self.s.or_realm}/{self.client_id}/attributevalue"
                    for name in ["command", *(f"relay{i}" for i in range(MAX_RELAYS))]:
                        await client.subscribe(f"{base}/{name}/#", qos=0)
                    for asset_id, sws in self._switches.items():        # switches known from earlier
                        for attr in sws:
                            await self._sub(asset_id, attr)
                    self.connected, backoff = True, 1.0
                    log.info("OpenRemote connected (%s:%s, realm %s)", host, self.s.or_mqtt_port, self.s.or_realm)
                    await self.sync_all()
                    worker = asyncio.create_task(self._worker())
                    try:
                        async for msg in client.messages:
                            try:
                                await self.on_or_message(str(msg.topic), bytes(msg.payload or b""))
                            except Exception:
                                log.exception("failed to handle OpenRemote message on %s", msg.topic)
                    finally:
                        worker.cancel()
            except aiomqtt.MqttError as e:
                log.warning("OpenRemote MQTT disconnected (%s); retrying in %.0fs", e, backoff)
            finally:
                self._publish, self._subscribe, self.connected = None, None, False
            await asyncio.sleep(backoff)
            backoff = min(backoff * 2, 60.0)

    async def _worker(self) -> None:
        while True:
            device_id, update = await self.queue.get()
            try:
                await self.process(device_id, update)
            except Exception:
                log.exception("OpenRemote sync of %s %s failed", device_id, update.get("type"))

    async def _load_links(self) -> None:
        async with self.sessions() as db:
            for d in await db.scalars(select(Device).where(Device.or_asset_id.is_not(None))):
                self._link(d.id, d.or_asset_id)

    def _link(self, device_id: str, asset_id: str) -> None:
        self._asset_of[device_id] = asset_id
        self._device_of[asset_id] = device_id

    async def _learn_switches(self, asset_id: str, meta: dict) -> None:
        """Remember the device's cloud switches and listen for UI writes to them."""
        sws = switches_of(meta)
        if not sws and asset_id not in self._switches:
            return
        self._switches[asset_id] = sws
        for attr in sws:
            await self._sub(asset_id, attr)

    async def _sub(self, asset_id: str, attr: str) -> None:
        topic = f"{self.s.or_realm}/{self.client_id}/attributevalue/{attr}/{asset_id}"
        if self._subscribe is not None and topic not in self._subscribed:
            await self._subscribe(topic)
            self._subscribed.add(topic)

    async def sync_all(self) -> None:
        """Make sure every registered device has an asset (run on each OR connect)."""
        self._attrs.clear()                      # re-check: assets may have been deleted in OR meanwhile
        async with self.sessions() as db:
            ids = list(await db.scalars(select(Device.id)))
        for device_id in ids:
            try:
                await self.ensure_asset(device_id)
            except Exception:
                log.exception("could not create OpenRemote asset for %s", device_id)

    # ---- assets -------------------------------------------------------------
    async def ensure_asset(self, device_id: str) -> str | None:
        asset_id = self._asset_of.get(device_id)
        if asset_id and asset_id in self._attrs:
            return asset_id
        async with self.sessions() as db:
            device = await db.get(Device, device_id)
            if device is None:
                return None
            if asset_id:
                asset = await self.api.get(asset_id)
                if asset is not None:
                    attrs = asset.setdefault("attributes", {})
                    if (attrs.get("command") or {}).get("type") != "JSON":     # assets made by 1.2.0-dev
                        attrs["command"] = attribute("command", "JSON", None, label="Send command")
                        await self.api.update(asset)
                    self._attrs[asset_id] = set(attrs)
                    await self._learn_switches(asset_id, device.meta or {})
                    return asset_id
                log.info("OpenRemote asset of %s was deleted; recreating", device_id)
            body = {"name": device.name or device.id, "type": "ThingAsset", "realm": self.s.or_realm,
                    "attributes": device_attributes(device)}
            if self.s.or_parent_id:
                body["parentId"] = self.s.or_parent_id
            asset = await self.api.create(body)
            asset_id = asset["id"]
            device.or_asset_id = asset_id
            await db.commit()
        for name, a in body["attributes"].items():
            self._or_value[(asset_id, name)] = a.get("value")
        self._attrs[asset_id] = set(body["attributes"])
        self._link(device_id, asset_id)
        await self._learn_switches(asset_id, device.meta or {})
        log.info("created OpenRemote asset %s for %s", asset_id, device_id)
        return asset_id

    async def _add_attributes(self, asset_id: str, new: list[dict]) -> None:
        asset = await self.api.get(asset_id)
        if asset is None:
            self._attrs.pop(asset_id, None)
            raise LookupError(f"asset {asset_id} vanished")
        attrs = asset.setdefault("attributes", {})
        for a in new:
            attrs.setdefault(a["name"], a)
        await self.api.update(asset)            # carries the current "version" (optimistic lock)
        self._attrs[asset_id] = set(attrs)

    async def remove_device(self, device_id: str, asset_id: str | None) -> None:
        asset_id = asset_id or self._asset_of.get(device_id)
        self._asset_of.pop(device_id, None)
        if asset_id:
            self._device_of.pop(asset_id, None)
            self._attrs.pop(asset_id, None)
            await self.api.delete(asset_id)

    # ---- device -> OpenRemote ---------------------------------------------
    async def process(self, device_id: str, update: dict) -> None:
        kind = update.get("type")
        if kind == "removed":
            await self.remove_device(device_id, update.get("asset_id"))
            return
        asset_id = await self.ensure_asset(device_id)
        if not asset_id or kind == "registered":
            return
        ts = update.get("ts")
        values: list[tuple[str, str, Any, dict]] = []
        if kind == "status":
            values = [("online", "boolean", bool(update.get("online")), {})]
        elif kind == "meta":
            await self._learn_switches(asset_id, update.get("meta") or {})
            values = meta_values(update.get("meta") or {})
        elif kind == "health":
            values = health_values(update.get("health") or {})
        elif kind == "telemetry":
            values = telemetry_values(update.get("data") or {}, set(self._switches.get(asset_id, {})))
        elif kind == "event":
            detail = update.get("detail") or ""
            values = [("lastEvent", "text", f"{update.get('event')}: {detail}" if detail else update.get("event"), {})]
        elif kind == "command":
            values = await self._command_result(device_id, update.get("command") or {})
        await self.write_values(asset_id, values, ts if isinstance(ts, datetime) else None)

    async def write_values(self, asset_id: str, values: list[tuple[str, str, Any, dict]],
                           ts: datetime | None = None) -> None:
        stamp = int((ts or utcnow()).timestamp() * 1000)
        known = self._attrs.get(asset_id, set())
        missing = []
        for n, t, v, extra in values:
            if n not in known:
                # Created *with* its value: a separate write stamped earlier than the
                # creation would be discarded by OpenRemote as outdated.
                missing.append({**attribute(n, t, v, **extra), "timestamp": stamp})
        if missing:
            await self._add_attributes(asset_id, missing)
            for a in missing:
                self._or_value[(asset_id, a["name"])] = a["value"]
        added = {a["name"] for a in missing}
        for name, _, value, _ in values:
            if name in added:
                continue
            key = (asset_id, name)
            if self._or_value.get(key, object()) == value and name != "lastEvent":
                continue                                    # unchanged: OR would drop it anyway
            self._or_value[key] = value                     # set first: the echo must look "known"
            await self._write(asset_id, name, value, stamp)

    async def _write(self, asset_id: str, name: str, value: Any, stamp: int) -> None:
        if self._publish is None:
            raise ConnectionError("OpenRemote MQTT not connected")
        topic = f"{self.s.or_realm}/{self.client_id}/writeattribute/{name}/{asset_id}"
        await self._publish(topic, json.dumps({"value": value, "timestamp": stamp}, default=str))

    async def _command_result(self, device_id: str, c: dict) -> list[tuple[str, str, Any, dict]]:
        if c.get("status") == "sent":
            return []
        out: list[tuple[str, str, Any, dict]] = [("lastCommand", "JSONObject", {
            "id": c.get("id"), "action": c.get("action"), "status": c.get("status"),
            "message": c.get("message"), "result": c.get("result")}, {})]
        if c.get("action") in ("relay.set", "switch.set") and c.get("status") in ("failed", "expired"):
            out += await self._actual_state(device_id, c["action"], c.get("args") or {})
        return out

    async def _actual_state(self, device_id: str, action: str, args: dict) -> list[tuple[str, str, Any, dict]]:
        """Undo an optimistic switch in the OR UI after a command didn't go through."""
        async with self.sessions() as db:
            device = await db.get(Device, device_id)
        meta = (device.meta if device else None) or {}
        if action == "relay.set":
            for r in meta.get("relays") or []:
                if isinstance(r, dict) and r.get("index") == args.get("index"):
                    return [(f"relay{args['index']}", "boolean", bool(r.get("on")), {})]
        else:
            for sw in meta.get("switches") or []:
                if isinstance(sw, dict) and sw.get("name") == args.get("name"):
                    return [(switch_attr(sw["name"]), "boolean", bool(sw.get("on")), {})]
        return []

    # ---- OpenRemote -> device ---------------------------------------------
    async def on_or_message(self, topic: str, payload: bytes) -> None:
        parts = topic.split("/")        # {realm}/{cid}/attributevalue/{attribute}/{assetId}
        if len(parts) != 5 or parts[2] != "attributevalue":
            return
        name, asset_id = parts[3], parts[4]
        device_id = self._device_of.get(asset_id)
        if device_id is None:
            return                                          # not one of ours
        try:
            value = json.loads(payload) if payload else None
        except ValueError:
            value = payload.decode(errors="replace")
        key = (asset_id, name)
        if key in self._or_value and self._or_value[key] == value:
            return                                          # echo of our own write
        self._or_value[key] = value

        if m := _RELAY.match(name):
            if isinstance(value, bool):
                await self.send(device_id, asset_id, "relay.set", {"index": int(m.group(1)), "on": value})
        elif name in self._switches.get(asset_id, {}):
            if isinstance(value, bool):
                await self.send(device_id, asset_id, "switch.set",
                                {"name": self._switches[asset_id][name], "on": value})
        elif name == "command" and value is not None:
            if isinstance(value, str):
                try:
                    value = json.loads(value)
                except ValueError:
                    value = {"action": value}
            if isinstance(value, dict) and isinstance(value.get("action"), str) and value["action"]:
                args = value.get("args") if isinstance(value.get("args"), dict) else {}
                await self.send(device_id, asset_id, value["action"][:64], args)
            # Clear it, so writing the same command again is a change OR will deliver.
            await self.write_values(asset_id, [("command", "JSON", None, {})])

    async def send(self, device_id: str, asset_id: str, action: str, args: dict) -> None:
        async with self.sessions() as db:
            device = await db.get(Device, device_id)
            if device is None:
                return
            cmd = Command(device_id=device_id, action=action, args=args)
            if not device.online:
                cmd.status, cmd.message, cmd.completed_at = "failed", "device is offline", utcnow()
            db.add(cmd)
            await db.commit()
            await db.refresh(cmd)
        if cmd.status == "sent":
            try:
                await self.bridge.send_command(cmd)
                log.info("OpenRemote -> %s: %s %s", device_id, action, args)
                return
            except Exception as e:                           # broker down
                async with self.sessions() as db:
                    row = await db.get(Command, cmd.id)
                    row.status, row.message, row.completed_at = "failed", str(e)[:500], utcnow()
                    await db.commit()
                    cmd = row
        res = {"id": cmd.id, "action": action, "args": args, "status": cmd.status, "message": cmd.message}
        self.notify(device_id, {"type": "command", "command": res})
