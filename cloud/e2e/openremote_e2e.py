"""
End-to-end check: Integrall device <-> backend <-> a REAL OpenRemote.

Plays an Integrall device (exact firmware MQTT contract) and an OpenRemote UI
user (REST attribute writes as the service user), and checks both directions.

    OR_CLIENT_SECRET=... python openremote_e2e.py --api http://localhost:8000 \
        --mqtt-host localhost --mqtt-port 1883 --or-url https://localhost --or-client-id integrall

The backend must run with the INTEGRALL_OR_* settings pointing at the same OpenRemote.
"""
import argparse
import asyncio
import json
import os
import sys
import time
import uuid

import aiomqtt
import httpx

DEV = "INT_0E2E" + uuid.uuid4().hex[:8].upper()   # fresh per run
ok_count = 0


def check(cond, what):
    global ok_count
    if not cond:
        print(f"  FAIL  {what}")
        sys.exit(1)
    ok_count += 1
    print(f"  ok    {what}")


class OR:
    def __init__(self, a):
        self.a = a
        self.http = httpx.AsyncClient(base_url=a.or_url, verify=not a.insecure, timeout=15)
        self._tok, self._exp = None, 0.0

    async def h(self):
        if time.monotonic() > self._exp:
            r = await self.http.post(f"/auth/realms/{self.a.realm}/protocol/openid-connect/token", data={
                "grant_type": "client_credentials", "client_id": self.a.or_client_id,
                "client_secret": os.environ["OR_CLIENT_SECRET"]})
            r.raise_for_status()
            self._tok, self._exp = r.json()["access_token"], time.monotonic() + 60
        return {"Authorization": f"Bearer {self._tok}"}

    async def find(self, device_id):
        r = await self.http.post(f"/api/{self.a.realm}/asset/query", headers=await self.h(),
                                 json={"realm": {"name": self.a.realm}, "types": ["ThingAsset"]})
        for asset in r.json():
            if (asset["attributes"].get("integrallId") or {}).get("value") == device_id:
                return asset
        return None

    async def attrs(self, asset_id):
        r = await self.http.get(f"/api/{self.a.realm}/asset/{asset_id}", headers=await self.h())
        return {} if r.status_code == 404 else {k: v.get("value") for k, v in r.json()["attributes"].items()}

    async def write(self, asset_id, name, value):
        r = await self.http.put(f"/api/{self.a.realm}/asset/{asset_id}/attribute/{name}",
                                headers=await self.h(), json=value)
        return r.status_code


async def until(fn, timeout=10.0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        v = fn()
        if asyncio.iscoroutine(v):
            v = await v
        if v:
            return v
        await asyncio.sleep(0.3)
    return None


async def main(a):
    api = httpx.AsyncClient(base_url=a.api, timeout=10)
    r = await api.post("/api/auth/register", json={"email": f"or-e2e-{uuid.uuid4().hex[:6]}@example.com",
                                                    "password": "e2e-password-123"})
    check(r.status_code == 201, "register user")
    api.headers["Authorization"] = f"Bearer {r.json()['access_token']}"
    h = (await api.get("/api/health")).json()
    check(h.get("openremote") is True, "backend reports OpenRemote connected")

    orc = OR(a)
    r = await api.post("/api/devices", json={"id": DEV, "name": "OR e2e pump"})
    check(r.status_code == 201, "register device")
    token = r.json()["token"]
    asset = await until(lambda: orc.find(DEV))
    check(asset is not None and asset["name"] == "OR e2e pump", "asset created in OpenRemote on registration")
    aid = asset["id"]

    base = f"integrall/{DEV}"
    relay = {"on": False}
    buzzer = {"on": False}

    def meta():
        return json.dumps({"fw": "e2e", "chip": "ESP32", "relays": [{"index": 0, "name": "pump", "on": relay["on"]}],
                           "switches": [{"name": "buzzer", "on": buzzer["on"]}]})
    will = aiomqtt.Will(f"{base}/status", "offline", qos=1, retain=True)
    async with aiomqtt.Client(a.mqtt_host, a.mqtt_port, identifier=DEV, username=DEV, password=token,
                              will=will, keepalive=5) as dev:
        await dev.subscribe(f"{base}/cmd", qos=1)
        seen = []

        async def device_loop():
            async for m in dev.messages:
                c = json.loads(m.payload)
                seen.append(c)
                if c["action"] == "relay.set":
                    relay["on"] = bool(c["args"]["on"])
                    await dev.publish(f"{base}/cmd/ack", json.dumps({"id": c["id"], "ok": True,
                                                                      "result": {"on": relay["on"]}}), qos=1)
                    await dev.publish(f"{base}/meta", meta(), qos=1, retain=True)
                elif c["action"] == "switch.set" and c["args"].get("name") == "buzzer":
                    buzzer["on"] = bool(c["args"]["on"])            # what addCloudSwitch() does on the ESP32
                    await dev.publish(f"{base}/cmd/ack", json.dumps({"id": c["id"], "ok": True,
                                                                      "result": {"on": buzzer["on"]}}), qos=1)
                    await dev.publish(f"{base}/meta", meta(), qos=1, retain=True)
                else:
                    await dev.publish(f"{base}/cmd/ack", json.dumps({"id": c["id"], "ok": True, "msg": "done"}), qos=1)
        task = asyncio.create_task(device_loop())

        await dev.publish(f"{base}/status", "online", qos=1, retain=True)
        await dev.publish(f"{base}/meta", meta(), qos=1, retain=True)
        await dev.publish(f"{base}/health", json.dumps({"missing": 2, "rssi": -58, "parts": {"lcd": False}}),
                          qos=1, retain=True)
        await dev.publish(f"{base}/telemetry", json.dumps({"temperature": 21.5, "door": True}))
        await dev.publish(f"{base}/event", json.dumps({"type": "lock", "detail": "opened"}), qos=1)

        v = await until(lambda: _match(orc, aid, online=True, firmware="e2e", partsMissing=2,
                                       temperature=21.5, door=True, relay0=False, lastEvent="lock: opened"))
        check(v, "status, meta, health, telemetry and event mirrored into the asset")
        await dev.publish(f"{base}/telemetry", json.dumps({"temperature": 22.75}))
        check(await until(lambda: _match(orc, aid, temperature=22.75)), "live telemetry update pushed")

        check(await orc.write(aid, "relay0", True) == 200, "OpenRemote user switches relay0 on")
        check(await until(lambda: _match(orc, aid, relay0=True, lastCommand=lambda c: c and c["status"] == "acked")),
              "device got relay.set, acked; asset shows relay0=true and acked result")
        check(relay["on"] is True and [c["action"] for c in seen] == ["relay.set"], "exactly one command, no echo loop")

        # cloud switch (addCloudSwitch("buzzer", ...)): UI -> device, then device button -> UI
        check(await until(lambda: _match(orc, aid, buzzer=False)), "buzzer switch attribute created from meta")
        check(await orc.write(aid, "buzzer", True) == 200, "OpenRemote user switches buzzer on")
        check(await until(lambda: buzzer["on"] is True and _match(orc, aid, buzzer=True)),
              "device got switch.set, buzzer on, asset shows it")
        buzzer["on"] = False                          # someone presses the button on the device
        await dev.publish(f"{base}/meta", meta(), qos=1, retain=True)
        check(await until(lambda: _match(orc, aid, buzzer=False)), "button press on device mirrored to OpenRemote")
        await asyncio.sleep(1)
        check(sum(c["action"] == "switch.set" for c in seen) == 1, "no echo command after the device-side change")

        for i in range(2):                           # same command twice must run twice
            check(await orc.write(aid, "command", {"action": "buzzer.beep", "args": {"ms": 50}}) == 200,
                  f"write command attribute (#{i + 1})")
            check(await until(lambda: _match(orc, aid, command=None)) and
                  await until(lambda: len([c for c in seen if c["action"] == "buzzer.beep"]) == i + 1),
                  f"command delivered and attribute cleared (#{i + 1})")
        check(await orc.write(aid, "command", "ping") == 200, "bare action string accepted")
        check(await until(lambda: any(c["action"] == "ping" for c in seen)), "ping delivered")

        task.cancel()
    # leaving without disconnect handshake isn't possible with aiomqtt; publish offline as the LWT would
    async with aiomqtt.Client(a.mqtt_host, a.mqtt_port, identifier=DEV, username=DEV, password=token) as dev:
        await dev.publish(f"{base}/status", "offline", qos=1, retain=True)
    check(await until(lambda: _match(orc, aid, online=False)), "offline mirrored")
    check(await orc.write(aid, "relay0", False) == 200, "switch relay0 while device offline")
    check(await until(lambda: _match(orc, aid, relay0=True,
                                     lastCommand=lambda c: c and c["status"] == "failed")),
          "command fails and the switch snaps back to the real state")

    r = await api.delete(f"/api/devices/{DEV}")
    check(r.status_code == 204, "remove device")
    check(await until(lambda: _gone(orc, aid)), "asset deleted from OpenRemote")
    print(f"\n{ok_count} checks passed")


async def _match(orc, aid, **want):
    have = await orc.attrs(aid)
    for k, v in want.items():
        if callable(v):
            if not v(have.get(k)):
                return False
        elif have.get(k) != v:
            return False
    return True


async def _gone(orc, aid):
    return await orc.attrs(aid) == {}


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--api", default="http://localhost:8000")
    p.add_argument("--mqtt-host", default="localhost")
    p.add_argument("--mqtt-port", type=int, default=1883)
    p.add_argument("--or-url", default="https://localhost")
    p.add_argument("--or-client-id", default="integrall")
    p.add_argument("--realm", default="master")
    p.add_argument("--insecure", action="store_true", help="accept the quick-start self-signed certificate")
    asyncio.run(main(p.parse_args()))
