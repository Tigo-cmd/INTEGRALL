"""OpenRemote sync against an in-memory fake OpenRemote (REST + MQTT API)."""
import json

from app.bridge import Bridge
from app.config import get_settings
from app.db import SessionLocal
from app.models import Command, Device, User
from app.openremote import OpenRemoteSync, telemetry_name

DEV = "INT_A1B2C3D4E5F6"
REALM = "master"


class FakeOR:
    """Asset store with the REST surface the sync uses."""
    def __init__(self):
        self.assets, self.deleted, self.n = {}, [], 0

    async def create(self, asset):
        self.n += 1
        a = json.loads(json.dumps(asset))
        a["id"], a["version"] = f"{self.n:022d}", 0
        self.assets[a["id"]] = a
        return a

    async def get(self, asset_id):
        a = self.assets.get(asset_id)
        return json.loads(json.dumps(a)) if a else None

    async def update(self, asset):
        cur = self.assets[asset["id"]]
        assert asset["version"] == cur["version"], "stale version"
        asset["version"] += 1
        self.assets[asset["id"]] = json.loads(json.dumps(asset))
        return asset

    async def delete(self, asset_id):
        self.deleted.append(asset_id)
        self.assets.pop(asset_id, None)


class FakeBridge:
    def __init__(self): self.sent = []
    async def send_command(self, cmd): self.sent.append(cmd)


async def make(online=True, meta=None):
    async with SessionLocal() as db:
        u = User(email="ada@example.com", password_hash="x")
        db.add(u); await db.flush()
        db.add(Device(id=DEV, owner_id=u.id, name="pump", online=online, meta=meta))
        await db.commit()
    api, bridge = FakeOR(), FakeBridge()
    sync = OpenRemoteSync(get_settings(), SessionLocal, bridge, api)
    writes = []

    async def publish(topic, payload):
        writes.append((topic, json.loads(payload)))
    sync._publish = publish
    sync.client_id = "cid"
    return sync, api, bridge, writes


def written(writes):
    """{attribute: value} of every writeattribute publish."""
    out = {}
    for topic, body in writes:
        realm, cid, kind, name, asset = topic.split("/")
        assert (realm, cid, kind) == (REALM, "cid", "writeattribute") and len(asset) == 22
        assert isinstance(body["timestamp"], int) and body["timestamp"] > 1_600_000_000_000
        out[name] = body["value"]
    return out


async def drain(sync):
    while not sync.queue.empty():
        await sync.process(*sync.queue.get_nowait())


async def test_asset_created_with_known_state_and_id_stored():
    sync, api, _, _ = await make(meta={"fw": "1.2.0", "relays": [{"index": 0, "name": "pump", "on": True}]})
    asset_id = await sync.ensure_asset(DEV)
    a = api.assets[asset_id]
    assert a["type"] == "ThingAsset" and a["realm"] == REALM and a["name"] == "pump"
    at = a["attributes"]
    assert at["integrallId"]["value"] == DEV and at["firmware"]["value"] == "1.2.0"
    assert at["relay0"]["value"] is True and at["relay0"]["type"] == "boolean"
    assert at["relay0"]["meta"]["label"] == "Relay 0 (pump)"
    assert at["command"]["type"] == "JSON"
    assert at["notes"]["type"] == "text" and at["location"]["type"] == "GEO_JSONPoint"   # required by ThingAsset
    async with SessionLocal() as db:
        assert (await db.get(Device, DEV)).or_asset_id == asset_id
    assert await sync.ensure_asset(DEV) == asset_id and len(api.assets) == 1     # idempotent


async def test_updates_flow_through_bridge_listener_to_openremote():
    sync, api, _, writes = await make(online=False)
    hub_msgs = []

    class Hub:
        async def send(self, uid, m): hub_msgs.append(m)
    b = Bridge(get_settings(), SessionLocal, Hub())
    b.listeners.append(sync.notify)
    await b.handle(f"integrall/{DEV}/status", b"online")
    await b.handle(f"integrall/{DEV}/meta", json.dumps({"fw": "1.2.0", "relays": [{"index": 1, "on": False}]}).encode())
    await b.handle(f"integrall/{DEV}/health", json.dumps({"missing": 1, "rssi": -60, "parts": {"lcd": False}}).encode())
    await b.handle(f"integrall/{DEV}/telemetry", json.dumps({"temperature": 24.5, "door": True, "mode": "eco"}).encode())
    await b.handle(f"integrall/{DEV}/event", json.dumps({"type": "lock", "detail": "opened"}).encode())
    await drain(sync)
    asset = next(iter(api.assets.values()))
    w = {n: a["value"] for n, a in asset["attributes"].items()} | written(writes)   # what OR now holds
    assert w["online"] is True and w["partsMissing"] == 1 and w["rssi"] == -60
    assert w["temperature"] == 24.5 and w["door"] is True and w["mode"] == "eco"
    assert w["lastEvent"] == "lock: opened"
    at = asset["attributes"]
    # new attributes are added via REST carrying their first value (no separate, older-stamped write)
    assert at["temperature"]["value"] == 24.5 and "temperature" not in written(writes)
    assert at["temperature"]["type"] == "number" and at["temperature"]["meta"]["storeDataPoints"] is True
    assert at["mode"]["type"] == "text" and at["relay1"]["type"] == "boolean"
    assert at["health"]["type"] == "JSONObject"


async def test_unchanged_values_are_not_rewritten():
    sync, _, _, writes = await make()
    await sync.ensure_asset(DEV)
    topic = f"{REALM}/cid/writeattribute/t/{sync._asset_of[DEV]}"
    for v in (1, 1, 1, 2, 2):
        await sync.process(DEV, {"type": "telemetry", "data": {"t": v}})
    assert [t for t, _ in writes].count(topic) == 1           # 1 went in with the new attribute; only 1->2 written


async def test_relay_write_in_openremote_becomes_command_and_echo_is_ignored():
    sync, _, bridge, writes = await make(meta={"relays": [{"index": 0, "on": False}]})
    aid = await sync.ensure_asset(DEV)
    topic = f"{REALM}/cid/attributevalue/relay0/{aid}"
    await sync.on_or_message(topic, b"true")                    # user flips the switch
    assert [(c.action, c.args) for c in bridge.sent] == [("relay.set", {"index": 0, "on": True})]
    await sync.on_or_message(topic, b"true")                    # duplicate delivery
    await sync.on_or_message(topic, b"false")                   # user flips back before the device answered
    assert [c.args["on"] for c in bridge.sent] == [True, False]
    # device reports its state; our write's echo must not become a command
    await sync.process(DEV, {"type": "meta", "meta": {"relays": [{"index": 0, "on": True}]}})
    assert written(writes)["relay0"] is True
    await sync.on_or_message(topic, b"true")
    assert len(bridge.sent) == 2


async def test_command_attribute_sends_and_is_cleared_and_result_reported():
    sync, _, bridge, writes = await make()
    aid = await sync.ensure_asset(DEV)
    await sync.on_or_message(f"{REALM}/cid/attributevalue/command/{aid}",
                             json.dumps({"action": "buzzer.beep", "args": {"ms": 200}}).encode())
    [cmd] = bridge.sent
    assert (cmd.action, cmd.args) == ("buzzer.beep", {"ms": 200})
    assert written(writes)["command"] is None                   # cleared so the same command can be resent
    await sync.on_or_message(f"{REALM}/cid/attributevalue/command/{aid}", b"null")   # echo of the clear
    await sync.on_or_message(f"{REALM}/cid/attributevalue/command/{aid}", b'"ping"')  # bare action string
    assert [c.action for c in bridge.sent] == ["buzzer.beep", "ping"]
    await sync.process(DEV, {"type": "command", "command": {"id": cmd.id, "action": "buzzer.beep",
                                                           "status": "acked", "result": {"ok": 1}}})
    assert written(writes)["lastCommand"]["status"] == "acked"


async def test_offline_device_command_fails_and_relay_switch_is_reverted():
    sync, _, bridge, writes = await make(online=False, meta={"relays": [{"index": 0, "on": False}]})
    aid = await sync.ensure_asset(DEV)
    await sync.on_or_message(f"{REALM}/cid/attributevalue/relay0/{aid}", b"true")
    assert bridge.sent == []
    async with SessionLocal() as db:
        from sqlalchemy import select
        c = (await db.scalars(select(Command))).one()
        assert (c.status, c.message) == ("failed", "device is offline")
    await drain(sync)
    w = written(writes)
    assert w["lastCommand"]["status"] == "failed" and w["relay0"] is False      # UI switch snaps back


async def test_foreign_assets_and_bad_topics_ignored():
    sync, _, bridge, _ = await make()
    await sync.ensure_asset(DEV)
    await sync.on_or_message(f"{REALM}/cid/attributevalue/relay0/{'9' * 22}", b"true")
    await sync.on_or_message(f"{REALM}/cid/attribute/relay0", b"true")
    assert bridge.sent == []


async def test_old_command_attribute_type_is_upgraded():
    sync, api, _, _ = await make()
    aid = await sync.ensure_asset(DEV)
    api.assets[aid]["attributes"]["command"]["type"] = "JSONObject"
    sync._attrs.clear()
    await sync.ensure_asset(DEV)
    assert api.assets[aid]["attributes"]["command"]["type"] == "JSON"


async def test_deleted_asset_is_recreated_and_removed_device_deletes_asset():
    sync, api, _, _ = await make()
    aid = await sync.ensure_asset(DEV)
    del api.assets[aid]
    await sync.sync_all()                                       # reconnect re-checks assets
    new = sync._asset_of[DEV]
    assert new != aid and new in api.assets
    await sync.process(DEV, {"type": "removed", "asset_id": new})
    assert api.deleted == [new] and DEV not in sync._asset_of


def test_telemetry_names_cannot_shadow_control_attributes():
    assert telemetry_name("temperature") == "temperature"
    assert telemetry_name("relay0") == "t_relay0"
    assert telemetry_name("command") == "t_command"
    assert telemetry_name("air quality") == "air_quality"
    assert telemetry_name("2nd") == "t_2nd"


async def test_cloud_switch_becomes_writable_attribute_and_command():
    sync, api, bridge, writes = await make(meta={"switches": [{"name": "buzzer", "on": False}]})
    subs = []

    async def subscribe(topic):
        subs.append(topic)
    sync._subscribe = subscribe
    aid = await sync.ensure_asset(DEV)
    at = api.assets[aid]["attributes"]
    assert at["buzzer"]["type"] == "boolean" and at["buzzer"]["value"] is False
    assert subs == [f"{REALM}/cid/attributevalue/buzzer/{aid}"]
    await sync.on_or_message(f"{REALM}/cid/attributevalue/buzzer/{aid}", b"true")      # UI switch
    [cmd] = bridge.sent
    assert (cmd.action, cmd.args) == ("switch.set", {"name": "buzzer", "on": True})
    # the device (button) turns it off: mirrored, and our own echo is not a command
    await sync.process(DEV, {"type": "meta", "meta": {"switches": [{"name": "buzzer", "on": False}]}})
    assert written(writes)["buzzer"] is False
    await sync.on_or_message(f"{REALM}/cid/attributevalue/buzzer/{aid}", b"false")
    assert len(bridge.sent) == 1
    # telemetry with the same name can't hijack the switch attribute
    await sync.process(DEV, {"type": "telemetry", "data": {"buzzer": True}})
    assert "t_buzzer" in api.assets[aid]["attributes"] and written(writes)["buzzer"] is False


async def test_switch_learned_from_later_meta_and_failed_switch_reverts():
    sync, api, bridge, writes = await make(online=True)
    subs = []

    async def subscribe(topic):
        subs.append(topic)
    sync._subscribe = subscribe
    aid = await sync.ensure_asset(DEV)
    assert subs == []
    meta = {"switches": [{"name": "relay0", "on": True}]}       # clashes with a relay name
    async with SessionLocal() as db:
        (await db.get(Device, DEV)).meta = meta
        await db.commit()
    await sync.process(DEV, {"type": "meta", "meta": meta})
    assert subs == [f"{REALM}/cid/attributevalue/switch_relay0/{aid}"]
    await sync.on_or_message(f"{REALM}/cid/attributevalue/switch_relay0/{aid}", b"false")
    [cmd] = bridge.sent
    await sync.process(DEV, {"type": "command", "command": {"id": cmd.id, "action": "switch.set",
                                                           "args": {"name": "relay0", "on": False},
                                                           "status": "expired"}})
    assert written(writes)["switch_relay0"] is True             # snaps back to the device's real state
