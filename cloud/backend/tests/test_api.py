from tests.conftest import login

DEV = "INT_A1B2C3D4E5F6"


async def test_auth_flow(app_client):
    c = app_client
    assert (await c.get("/api/devices")).status_code == 401
    await login(c)
    me = await c.get("/api/auth/me")
    assert me.json()["email"] == "ada@example.com"
    bad = await c.post("/api/auth/login", json={"email": "ada@example.com", "password": "wrong password"})
    assert bad.status_code == 401
    dup = await c.post("/api/auth/register", json={"email": "ADA@example.com", "password": "another one!"})
    assert dup.status_code == 409


async def test_device_lifecycle_provisions_broker(app_client):
    c = await login(app_client)
    r = await c.post("/api/devices", json={"id": DEV.lower(), "name": "Pump room"})
    assert r.status_code == 201, r.text
    body = r.json()
    assert body["id"] == DEV and len(body["token"]) >= 24
    assert c.fake.provisioned == [(DEV, body["token"])]
    listed = (await c.get("/api/devices")).json()
    assert [d["id"] for d in listed] == [DEV] and "token" not in listed[0]   # token shown once only
    assert (await c.post("/api/devices", json={"id": DEV, "name": "x"})).status_code == 409
    rot = await c.post(f"/api/devices/{DEV}/rotate-token")
    assert rot.status_code == 200 and c.fake.passwords[0][1] == rot.json()["token"]
    assert (await c.delete(f"/api/devices/{DEV}")).status_code == 204
    assert c.fake.deprovisioned == [DEV]


async def test_bad_device_id_rejected(app_client):
    c = await login(app_client)
    r = await c.post("/api/devices", json={"id": "INT_OFFLINE", "name": "uno"})
    assert r.status_code == 422


async def test_users_cannot_see_each_others_devices(app_client):
    c = await login(app_client)
    await c.post("/api/devices", json={"id": DEV, "name": "mine"})
    await login(c, email="eve@example.com")
    assert (await c.get(f"/api/devices/{DEV}")).status_code == 404
    assert (await c.post(f"/api/devices/{DEV}/commands", json={"action": "relay.set"})).status_code == 404
    assert (await c.get("/api/devices")).json() == []


async def test_commands_refused_while_offline_and_validated(app_client):
    c = await login(app_client)
    await c.post("/api/devices", json={"id": DEV, "name": "pump"})
    r = await c.post(f"/api/devices/{DEV}/commands", json={"action": "relay.set", "args": {"index": 0, "on": True}})
    assert r.status_code == 409          # offline: never queued for later
    assert c.fake.sent == []
    bad = await c.post(f"/api/devices/{DEV}/commands", json={"action": "DROP TABLE"})
    assert bad.status_code == 422
