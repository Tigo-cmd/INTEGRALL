"""
End-to-end check of the Integrall cloud against a REAL broker + backend.

Plays the role of an Integrall device using the exact MQTT topic contract the
firmware implements (MqttModule.h), and of the dashboard (REST + WebSocket).

    python e2e_test.py --api http://localhost:8000 --mqtt-host localhost --mqtt-port 1883
"""
import argparse
import asyncio
import json
import sys
import uuid

import aiomqtt
import httpx
import websockets

DEV = "INT_AABBCCDDEEFF"
OTHER = "INT_112233445566"
ok_count = 0


def check(cond, what):
    global ok_count
    if not cond:
        print(f"  FAIL  {what}")
        sys.exit(1)
    ok_count += 1
    print(f"  ok    {what}")


async def ws_expect(ws, kind, timeout=5.0, **match):
    async def _wait():
        while True:
            m = json.loads(await ws.recv())
            if m.get("type") == kind and all(m.get(k) == v for k, v in match.items()):
                return m
    return await asyncio.wait_for(_wait(), timeout)


async def main(a):
    api = httpx.AsyncClient(base_url=a.api, timeout=10)
    email = f"e2e-{uuid.uuid4().hex[:6]}@example.com"
    r = await api.post("/api/auth/register", json={"email": email, "password": "e2e-password-123"})
    check(r.status_code == 201, "register user")
    jwt = r.json()["access_token"]
    api.headers["Authorization"] = f"Bearer {jwt}"

    ws_url = a.api.replace("http", "ws", 1) + "/api/ws"
    async with websockets.connect(ws_url) as ws:
        await ws.send(json.dumps({"token": jwt}))
        check((await ws_expect(ws, "hello"))["user"] == email, "dashboard WebSocket authenticated")

        for dev_id in (DEV, OTHER):
            await api.delete(f"/api/devices/{dev_id}")        # leftovers from an earlier run
        r = await api.post("/api/devices", json={"id": DEV, "name": "E2E pump"})
        check(r.status_code == 201, "create device (broker login provisioned)")
        token = r.json()["token"]

        # wrong password must be refused by the broker
        try:
            async with aiomqtt.Client(a.mqtt_host, a.mqtt_port, identifier=DEV, username=DEV, password="wrong"):
                check(False, "broker rejects wrong device password")
        except aiomqtt.MqttError:
            check(True, "broker rejects wrong device password")

        will = aiomqtt.Will(f"integrall/{DEV}/status", "offline", qos=1, retain=True)
        device = aiomqtt.Client(a.mqtt_host, a.mqtt_port, identifier=DEV, username=DEV, password=token,
                                will=will, keepalive=5, clean_session=True)
        await device.__aenter__()
        await device.subscribe(f"integrall/{DEV}/cmd", qos=1)
        await device.publish(f"integrall/{DEV}/status", "online", qos=1, retain=True)
        check((await ws_expect(ws, "status", device_id=DEV))["online"] is True, "device online pushed to dashboard")

        meta = {"fw": "1.2.0", "chip": "ESP32", "modules": ["relay"], "relays": [{"index": 0, "name": "pump", "on": False}]}
        await device.publish(f"integrall/{DEV}/meta", json.dumps(meta), qos=1, retain=True)
        check((await ws_expect(ws, "meta", device_id=DEV))["meta"]["fw"] == "1.2.0", "meta pushed")

        await device.publish(f"integrall/{DEV}/telemetry", json.dumps({"temperature": 24.5, "relay0": False}))
        m = await ws_expect(ws, "telemetry", device_id=DEV)
        check(m["data"]["temperature"] == 24.5, "telemetry pushed live (no polling)")
        r = await api.get(f"/api/devices/{DEV}/telemetry")
        check(r.status_code == 200 and r.json()[-1]["data"]["temperature"] == 24.5, "telemetry stored in DB")

        await device.publish(f"integrall/{DEV}/event", json.dumps({"type": "lock", "detail": "unlocked"}), qos=1)
        check((await ws_expect(ws, "event", device_id=DEV))["detail"] == "unlocked", "event pushed")

        # --- command round trip
        r = await api.post(f"/api/devices/{DEV}/commands", json={"action": "relay.set", "args": {"index": 0, "on": True}})
        check(r.status_code == 202, "command accepted")
        cmd_id = r.json()["id"]
        msg = await asyncio.wait_for(anext(aiter(device.messages)), 5)
        cmd = json.loads(msg.payload)
        check(cmd == {"id": cmd_id, "action": "relay.set", "args": {"index": 0, "on": True}}, "device received command instantly")
        await device.publish(f"integrall/{DEV}/cmd/ack", json.dumps({"id": cmd_id, "ok": True, "result": {"on": True}}), qos=1)
        m = await ws_expect(ws, "command", device_id=DEV)
        check(m["command"]["status"] == "acked" and m["command"]["result"] == {"on": True}, "ack pushed to dashboard")

        # --- isolation: a device can't impersonate or spy on another device
        await api.post("/api/devices", json={"id": OTHER, "name": "victim"})
        await device.publish(f"integrall/{OTHER}/telemetry", json.dumps({"spoofed": 1}))
        await asyncio.sleep(1)
        r = await api.get(f"/api/devices/{OTHER}/telemetry")
        check(r.json() == [], "broker ACL blocks publishing as another device")
        try:
            await device.subscribe("integrall/#", qos=1)
            spied = False
            try:
                await device.publish(f"integrall/{DEV}/telemetry", "{}")
                await asyncio.wait_for(anext(aiter(device.messages)), 1.5)
                spied = True
            except asyncio.TimeoutError:
                pass
            check(not spied, "broker ACL blocks subscribing to other devices")
        except aiomqtt.MqttError:
            check(True, "broker ACL blocks subscribing to other devices")

        # --- ungraceful drop -> LWT -> offline
        device._client.socket().close()            # simulate power loss (no DISCONNECT packet)
        m = await ws_expect(ws, "status", device_id=DEV, timeout=20)
        check(m["online"] is False, "power loss detected via LWT (offline pushed)")
        r = await api.post(f"/api/devices/{DEV}/commands", json={"action": "ping"})
        check(r.status_code == 409, "commands refused while offline (never replayed later)")

        for dev_id in (DEV, OTHER):
            await api.delete(f"/api/devices/{dev_id}")
    print(f"\nE2E PASSED: {ok_count} checks")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--api", default="http://localhost:8000")
    p.add_argument("--mqtt-host", default="localhost")
    p.add_argument("--mqtt-port", type=int, default=1883)
    asyncio.run(main(p.parse_args()))
