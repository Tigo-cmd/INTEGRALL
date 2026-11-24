"""
Simulated Integrall device — test the cloud + dashboard with no hardware.

Speaks the same MQTT contract as the firmware (MqttModule.h): status/LWT,
meta (with 2 relays), health, telemetry every few seconds, and answers the
built-in commands (relay.set, relay.toggle, ping, health, reboot, buzzer.beep).

    python fake_device.py --id INT_AABBCCDDEEFF --token <token from dashboard> \
                          --host localhost --port 1883
"""
import argparse
import asyncio
import json
import math
import random
import time

import aiomqtt


async def run(a):
    base = f"integrall/{a.id}"
    relays = [{"index": 0, "name": "pump", "on": False}, {"index": 1, "name": "fan", "on": False}]
    started = time.time()

    def health():
        return {"parts": {"lcd": True, "dht22": True, "bme280": False, "wifi": True, "cloud": True},
                "i2c": ["0x27"], "heap": random.randint(180000, 200000), "rssi": random.randint(-70, -50),
                "uptime_s": int(time.time() - started), "missing": 1, "fw": "1.2.0-sim"}

    will = aiomqtt.Will(f"{base}/status", "offline", qos=1, retain=True)
    async with aiomqtt.Client(a.host, a.port, identifier=a.id, username=a.id, password=a.token,
                              will=will, keepalive=15) as c:
        await c.subscribe(f"{base}/cmd", qos=1)
        await c.publish(f"{base}/status", "online", qos=1, retain=True)
        meta = {"fw": "1.2.0-sim", "chip": "ESP32 (simulated)", "ip": "192.168.1.77",
                "modules": ["relay", "sensors", "lcd", "buzzer"], "relays": relays}
        await c.publish(f"{base}/meta", json.dumps(meta), qos=1, retain=True)
        await c.publish(f"{base}/health", json.dumps(health()), qos=1, retain=True)
        print(f"{a.id} online at {a.host}:{a.port}")

        async def telemetry():
            t = 0
            while True:
                data = {"temperature": round(24 + 3 * math.sin(t / 10) + random.uniform(-0.2, 0.2), 2),
                        "humidity": round(55 + 8 * math.cos(t / 14) + random.uniform(-0.5, 0.5), 1),
                        "relay0": relays[0]["on"], "rssi": random.randint(-70, -50)}
                await c.publish(f"{base}/telemetry", json.dumps(data))
                t += 1
                await asyncio.sleep(a.interval)

        async def commands():
            async for m in c.messages:
                cmd = json.loads(m.payload)
                action, args = cmd.get("action"), cmd.get("args") or {}
                ack = {"id": cmd.get("id"), "ok": True}
                if action in ("relay.set", "relay.toggle"):
                    i = args.get("index", -1)
                    if not 0 <= i < len(relays):
                        ack.update(ok=False, msg="bad relay index")
                    else:
                        relays[i]["on"] = (not relays[i]["on"]) if action == "relay.toggle" else bool(args.get("on"))
                        ack["result"] = {"on": relays[i]["on"]}
                        await c.publish(f"{base}/meta", json.dumps(meta), qos=1, retain=True)  # like the firmware
                        await c.publish(f"{base}/event", json.dumps({"type": "relay", "detail": f"{relays[i]['name']} {'on' if relays[i]['on'] else 'off'}"}), qos=1)
                elif action == "ping":
                    ack["msg"] = "pong"
                elif action == "health":
                    await c.publish(f"{base}/health", json.dumps(health()), qos=1, retain=True)
                elif action in ("reboot", "buzzer.beep"):
                    ack["msg"] = "simulated"
                else:
                    ack.update(ok=False, msg="unknown action")
                await c.publish(f"{base}/cmd/ack", json.dumps(ack), qos=1)
                print(f"cmd {action} {args} -> {ack}")

        await asyncio.gather(telemetry(), commands())


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--id", required=True)
    p.add_argument("--token", required=True)
    p.add_argument("--host", default="localhost")
    p.add_argument("--port", type=int, default=1883)
    p.add_argument("--interval", type=float, default=3.0)
    asyncio.run(run(p.parse_args()))
