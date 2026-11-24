"""
MQTT bridge between devices and the backend.

Topic contract (must match firmware/Integrall/src/modules/MqttModule.h):
  {prefix}/{id}/status    retained "online" | "offline" (LWT)
  {prefix}/{id}/meta      retained JSON
  {prefix}/{id}/health    retained JSON
  {prefix}/{id}/telemetry JSON object
  {prefix}/{id}/event     {"type","detail","ts"?}
  {prefix}/{id}/cmd       backend -> device {"id","action","args"}
  {prefix}/{id}/cmd/ack   device -> backend {"id","ok","msg"?,"result"?}

The broker's ACLs (see provision_device) guarantee a device can only publish
under its own {id}, so the id in the topic is trusted.
"""
import asyncio
import contextlib
import json
import logging
import ssl
import uuid
from collections.abc import Callable
from datetime import datetime, timedelta, timezone
from typing import Any

import aiomqtt
from sqlalchemy.ext.asyncio import AsyncSession, async_sessionmaker

from .config import Settings
from .hub import Hub
from .models import Command, Device, Event, Telemetry, utcnow

log = logging.getLogger("integrall.bridge")

DYNSEC_TOPIC = "$CONTROL/dynamic-security/v1"


class BrokerUnavailable(RuntimeError):
    pass


class Bridge:
    def __init__(self, settings: Settings, sessions: async_sessionmaker[AsyncSession], hub: Hub) -> None:
        self.s = settings
        self.sessions = sessions
        self.hub = hub
        self._client: aiomqtt.Client | None = None
        self._pending_dynsec: dict[str, asyncio.Future] = {}
        self._tasks: set[asyncio.Task] = set()
        # Extra consumers of every device update (e.g. the OpenRemote sync).
        # Called synchronously; they must only enqueue, never block.
        self.listeners: list[Callable[[str, dict], None]] = []

    # ------------------------------------------------------------------ run --
    @property
    def connected(self) -> bool:
        return self._client is not None

    async def run(self) -> None:
        """Keep a broker session alive forever (exponential back-off on failure)."""
        backoff = 1.0
        tls = ssl.create_default_context() if self.s.mqtt_tls else None
        while True:
            try:
                async with aiomqtt.Client(
                    hostname=self.s.mqtt_host,
                    port=self.s.mqtt_port,
                    username=self.s.mqtt_username,
                    password=self.s.mqtt_password,
                    identifier=f"integrall-backend-{uuid.uuid4().hex[:8]}",
                    tls_context=tls,
                    keepalive=30,
                ) as client:
                    self._client = client
                    if self.s.dynsec_enabled:
                        await client.subscribe(f"{DYNSEC_TOPIC}/response", qos=1)
                        reader = asyncio.create_task(self._pump(client))   # responses arrive via messages
                        try:
                            await self._ensure_backend_role()
                        finally:
                            reader.cancel()
                            with contextlib.suppress(asyncio.CancelledError):
                                await reader
                    await client.subscribe(f"{self.s.mqtt_prefix}/+/#", qos=1)
                    backoff = 1.0
                    log.info("MQTT bridge connected to %s:%s", self.s.mqtt_host, self.s.mqtt_port)
                    async for msg in client.messages:
                        try:
                            await self.handle(str(msg.topic), bytes(msg.payload or b""))
                        except Exception:  # one bad message must never kill the bridge
                            log.exception("failed to handle message on %s", msg.topic)
            except aiomqtt.MqttError as e:
                log.warning("MQTT bridge disconnected (%s); retrying in %.0fs", e, backoff)
            finally:
                self._client = None
            await asyncio.sleep(backoff)
            backoff = min(backoff * 2, 60.0)

    async def _pump(self, client: aiomqtt.Client) -> None:
        async for msg in client.messages:
            await self.handle(str(msg.topic), bytes(msg.payload or b""))

    async def _ensure_backend_role(self) -> None:
        """Give the backend's own broker account read/write on all device topics (idempotent)."""
        role = "integrall-backend"
        [me] = await self._dynsec([{"command": "getClient", "username": self.s.mqtt_username}])
        roles = {r.get("rolename") for r in (me.get("data") or {}).get("client", {}).get("roles", [])}
        if role in roles:
            return
        await self._dynsec([
            {"command": "createRole", "rolename": role, "acls": [
                {"acltype": "publishClientSend", "topic": f"{self.s.mqtt_prefix}/#", "allow": True},
                {"acltype": "publishClientReceive", "topic": f"{self.s.mqtt_prefix}/#", "allow": True},
                {"acltype": "subscribePattern", "topic": f"{self.s.mqtt_prefix}/#", "allow": True},
            ]},
        ], tolerate_errors=True)                         # role may exist from an earlier run
        log.info("granting broker role %s to %s (broker will reconnect us once)", role, self.s.mqtt_username)
        await self._dynsec([{"command": "addClientRole", "username": self.s.mqtt_username, "rolename": role}],
                           tolerate_errors=True)

    # --------------------------------------------------------------- inbound --
    async def handle(self, topic: str, payload: bytes) -> None:
        if topic == f"{DYNSEC_TOPIC}/response":
            self._on_dynsec_response(payload)
            return
        parts = topic.split("/")
        if len(parts) < 3 or parts[0] != self.s.mqtt_prefix:
            return
        device_id, kind = parts[1], "/".join(parts[2:])
        if kind == "cmd":                       # our own outgoing commands echoed back
            return
        if len(payload) > self.s.max_payload_bytes:
            log.warning("dropping oversized %s from %s (%d bytes)", kind, device_id, len(payload))
            return

        async with self.sessions() as db:
            device = await db.get(Device, device_id)
            if device is None:
                return                           # unknown device (not registered here)
            owner = device.owner_id
            now = utcnow()
            device.last_seen = now
            out: dict[str, Any] | None = None

            if kind == "status":
                device.online = payload.decode(errors="replace").strip() == "online"
                out = {"type": "status", "online": device.online}
            elif kind in ("meta", "health"):
                doc = _json_object(payload)
                if doc is None:
                    return
                setattr(device, kind, doc)
                device.online = True
                out = {"type": kind, kind: doc}
            elif kind == "telemetry":
                doc = _json_object(payload)
                if doc is None:
                    return
                ts = _device_ts(doc.pop("ts", None), now)
                db.add(Telemetry(device_id=device_id, ts=ts, data=doc))
                device.last_telemetry = doc
                device.online = True
                out = {"type": "telemetry", "ts": ts, "data": doc}
            elif kind == "event":
                doc = _json_object(payload)
                if doc is None:
                    return
                ts = _device_ts(doc.get("ts"), now)
                ev = Event(device_id=device_id, ts=ts, type=str(doc.get("type", "event"))[:64],
                           detail=str(doc.get("detail", ""))[:1000])
                db.add(ev)
                out = {"type": "event", "ts": ts, "event": ev.type, "detail": ev.detail}
            elif kind == "cmd/ack":
                doc = _json_object(payload)
                if doc is None or not doc.get("id"):
                    return
                cmd = await db.get(Command, str(doc["id"]))
                if cmd is None or cmd.device_id != device_id or cmd.status != "sent":
                    return                       # unknown, foreign or already expired
                cmd.status = "acked" if doc.get("ok") else "failed"
                cmd.message = (str(doc["msg"])[:500]) if doc.get("msg") else None
                cmd.result = doc.get("result") if isinstance(doc.get("result"), dict) else None
                cmd.completed_at = now
                out = {"type": "command", "command": _command_dict(cmd)}
            else:
                return
            await db.commit()

        if out is not None:
            await self._emit(owner, device_id, out)

    async def _emit(self, owner: int, device_id: str, out: dict) -> None:
        out["device_id"] = device_id
        await self.hub.send(owner, out)
        for listener in self.listeners:
            try:
                listener(device_id, out)
            except Exception:
                log.exception("update listener failed")

    # -------------------------------------------------------------- commands --
    async def send_command(self, cmd: Command) -> None:
        if not self._client:
            raise BrokerUnavailable("MQTT broker not connected")
        payload = json.dumps({"id": cmd.id, "action": cmd.action, "args": cmd.args})
        await self._client.publish(f"{self.s.mqtt_prefix}/{cmd.device_id}/cmd", payload, qos=1)
        task = asyncio.create_task(self._expire_later(cmd.id))
        self._tasks.add(task)
        task.add_done_callback(self._tasks.discard)

    async def _expire_later(self, command_id: str) -> None:
        await asyncio.sleep(self.s.command_timeout_s)
        async with self.sessions() as db:
            cmd = await db.get(Command, command_id)
            if cmd is None or cmd.status != "sent":
                return
            cmd.status = "expired"
            cmd.message = "device did not answer in time"
            cmd.completed_at = utcnow()
            device = await db.get(Device, cmd.device_id)
            await db.commit()
            if device:
                await self._emit(device.owner_id, cmd.device_id, {"type": "command", "command": _command_dict(cmd)})

    # ------------------------------------------------------- broker accounts --
    async def provision_device(self, device_id: str, token: str) -> None:
        """Create the device's broker login, restricted to its own topics."""
        if not self.s.dynsec_enabled:
            return
        base = f"{self.s.mqtt_prefix}/{device_id}"
        role = f"device-{device_id}"
        await self._dynsec([
            {"command": "createRole", "rolename": role, "acls": [
                {"acltype": "publishClientSend", "topic": f"{base}/#", "allow": True},
                {"acltype": "subscribePattern", "topic": f"{base}/cmd", "allow": True},
            ]},
            {"command": "createClient", "username": device_id, "password": token,
             "roles": [{"rolename": role}]},
        ])

    async def set_device_password(self, device_id: str, token: str) -> None:
        if self.s.dynsec_enabled:
            await self._dynsec([{"command": "setClientPassword", "username": device_id, "password": token},
                                {"command": "disableClient", "username": device_id},
                                {"command": "enableClient", "username": device_id}])  # kick old session

    async def deprovision_device(self, device_id: str) -> None:
        if self.s.dynsec_enabled:
            await self._dynsec([{"command": "deleteClient", "username": device_id},
                                {"command": "deleteRole", "rolename": f"device-{device_id}"}],
                               tolerate_errors=True)

    async def _dynsec(self, commands: list[dict], tolerate_errors: bool = False) -> list[dict]:
        """Run dynamic-security commands; returns each command's response dict."""
        if not self._client:
            raise BrokerUnavailable("MQTT broker not connected")
        loop = asyncio.get_running_loop()
        futures = []
        for c in commands:
            corr = uuid.uuid4().hex
            c["correlationData"] = corr
            fut = loop.create_future()
            self._pending_dynsec[corr] = fut
            futures.append((c["command"], fut))
        await self._client.publish(DYNSEC_TOPIC, json.dumps({"commands": commands}), qos=1)
        responses: list[dict] = []
        try:
            for name, fut in futures:
                resp = await asyncio.wait_for(fut, timeout=5)
                if resp.get("error") and not tolerate_errors:
                    raise BrokerUnavailable(f"broker refused {name}: {resp['error']}")
                responses.append(resp)
            return responses
        except asyncio.TimeoutError as e:
            raise BrokerUnavailable("broker did not confirm account change (is dynamic-security enabled?)") from e
        finally:
            for c in commands:
                self._pending_dynsec.pop(c["correlationData"], None)

    def _on_dynsec_response(self, payload: bytes) -> None:
        doc = _json_object(payload) or {}
        for r in doc.get("responses", []):
            fut = self._pending_dynsec.get(r.get("correlationData", ""))
            if fut and not fut.done():
                fut.set_result(r)


# ------------------------------------------------------------------ helpers --
def _json_object(payload: bytes) -> dict | None:
    try:
        doc = json.loads(payload)
    except (ValueError, UnicodeDecodeError):
        return None
    return doc if isinstance(doc, dict) else None


def _device_ts(value: Any, now: datetime) -> datetime:
    """Use the device's NTP timestamp only if it is plausible; else receive time."""
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        try:
            ts = datetime.fromtimestamp(value, tz=timezone.utc)
        except (OverflowError, OSError, ValueError):
            return now
        if abs(ts - now) < timedelta(days=1):
            return ts
    return now


def _command_dict(c: Command) -> dict:
    return {"id": c.id, "action": c.action, "args": c.args, "status": c.status, "message": c.message,
            "result": c.result, "created_at": c.created_at, "completed_at": c.completed_at}
