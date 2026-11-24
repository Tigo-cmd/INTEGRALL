import asyncio
import json

from sqlalchemy import select

from app.bridge import Bridge
from app.config import get_settings
from app.db import SessionLocal
from app.models import Command, Device, Event, Telemetry, User

DEV = "INT_A1B2C3D4E5F6"


class CaptureHub:
    def __init__(self): self.msgs = []
    async def send(self, user_id, message): self.msgs.append((user_id, message))


async def setup_device(online=False):
    async with SessionLocal() as db:
        u = User(email="ada@example.com", password_hash="x")
        db.add(u)
        await db.flush()
        db.add(Device(id=DEV, owner_id=u.id, name="pump", online=online))
        await db.commit()
        return u.id


def bridge(hub):
    return Bridge(get_settings(), SessionLocal, hub)


async def test_status_meta_health_and_telemetry_are_stored_and_pushed():
    uid = await setup_device()
    hub = CaptureHub(); b = bridge(hub)
    await b.handle(f"integrall/{DEV}/status", b"online")
    await b.handle(f"integrall/{DEV}/meta", json.dumps({"fw": "1.2.0", "relays": [{"index": 0}]}).encode())
    await b.handle(f"integrall/{DEV}/health", json.dumps({"parts": {"lcd": False}}).encode())
    await b.handle(f"integrall/{DEV}/telemetry", json.dumps({"temperature": 24.5}).encode())
    async with SessionLocal() as db:
        d = await db.get(Device, DEV)
        assert d.online and d.meta["fw"] == "1.2.0" and d.health["parts"]["lcd"] is False
        assert d.last_telemetry == {"temperature": 24.5}
        rows = (await db.scalars(select(Telemetry))).all()
        assert len(rows) == 1 and rows[0].data == {"temperature": 24.5}
    kinds = [m["type"] for _, m in hub.msgs]
    assert kinds == ["status", "meta", "health", "telemetry"]
    assert all(u == uid and m["device_id"] == DEV for u, m in hub.msgs)


async def test_lwt_offline():
    await setup_device(online=True)
    b = bridge(CaptureHub())
    await b.handle(f"integrall/{DEV}/status", b"offline")
    async with SessionLocal() as db:
        assert (await db.get(Device, DEV)).online is False


async def test_garbage_unknown_and_oversized_messages_are_ignored():
    await setup_device()
    hub = CaptureHub(); b = bridge(hub)
    await b.handle(f"integrall/{DEV}/telemetry", b"not json")
    await b.handle(f"integrall/{DEV}/telemetry", b"[1,2,3]")
    await b.handle("integrall/INT_FFFFFFFFFFFF/telemetry", b'{"t":1}')
    await b.handle(f"integrall/{DEV}/telemetry", b'{"x":"' + b"a" * 5000 + b'"}')
    await b.handle(f"integrall/{DEV}/cmd", b'{"id":"1","action":"x"}')     # our own echo
    async with SessionLocal() as db:
        assert (await db.scalars(select(Telemetry))).all() == []
    assert hub.msgs == []


async def test_implausible_device_timestamp_replaced_by_receive_time():
    await setup_device()
    b = bridge(CaptureHub())
    await b.handle(f"integrall/{DEV}/telemetry", b'{"ts": 12, "v": 1}')     # clock not synced
    async with SessionLocal() as db:
        t = (await db.scalars(select(Telemetry))).one()
        assert t.ts.year >= 2024 and "ts" not in t.data


async def test_event_stored():
    await setup_device()
    b = bridge(CaptureHub())
    await b.handle(f"integrall/{DEV}/event", b'{"type":"lock","detail":"unlocked"}')
    async with SessionLocal() as db:
        e = (await db.scalars(select(Event))).one()
        assert (e.type, e.detail) == ("lock", "unlocked")


async def test_command_ack_and_expiry():
    await setup_device(online=True)
    hub = CaptureHub(); b = bridge(hub)

    class FakeClient:
        def __init__(self): self.published = []
        async def publish(self, topic, payload, qos=0): self.published.append((topic, json.loads(payload)))
    b._client = FakeClient()

    async with SessionLocal() as db:
        ok_cmd = Command(device_id=DEV, action="relay.set", args={"index": 0, "on": True})
        lost_cmd = Command(device_id=DEV, action="ping", args={})
        db.add_all([ok_cmd, lost_cmd]); await db.commit()
    await b.send_command(ok_cmd)
    await b.send_command(lost_cmd)
    assert b._client.published[0] == (f"integrall/{DEV}/cmd",
                                      {"id": ok_cmd.id, "action": "relay.set", "args": {"index": 0, "on": True}})
    await b.handle(f"integrall/{DEV}/cmd/ack", json.dumps({"id": ok_cmd.id, "ok": True, "result": {"on": True}}).encode())
    await asyncio.sleep(1.3)                                   # timeout is 1 s in tests
    async with SessionLocal() as db:
        assert (await db.get(Command, ok_cmd.id)).status == "acked"
        assert (await db.get(Command, ok_cmd.id)).result == {"on": True}
        assert (await db.get(Command, lost_cmd.id)).status == "expired"
    # a late ack must not resurrect an expired command
    await b.handle(f"integrall/{DEV}/cmd/ack", json.dumps({"id": lost_cmd.id, "ok": True}).encode())
    async with SessionLocal() as db:
        assert (await db.get(Command, lost_cmd.id)).status == "expired"


async def test_device_cannot_ack_another_devices_command():
    await setup_device(online=True)
    b = bridge(CaptureHub())
    async with SessionLocal() as db:
        u = (await db.scalars(select(User))).one()
        db.add(Device(id="INT_000000000001", owner_id=u.id, name="other"))
        cmd = Command(device_id=DEV, action="ping", args={})
        db.add(cmd); await db.commit()
    await b.handle("integrall/INT_000000000001/cmd/ack", json.dumps({"id": cmd.id, "ok": True}).encode())
    async with SessionLocal() as db:
        assert (await db.get(Command, cmd.id)).status == "sent"
