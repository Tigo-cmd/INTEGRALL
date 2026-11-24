import os
import tempfile

_db = tempfile.NamedTemporaryFile(suffix=".db", delete=False)
os.environ["INTEGRALL_DATABASE_URL"] = f"sqlite+aiosqlite:///{_db.name}"
os.environ["INTEGRALL_JWT_SECRET"] = "test-secret-not-for-production-0123456789"
os.environ["INTEGRALL_COMMAND_TIMEOUT_S"] = "1"

import pytest  # noqa: E402
from httpx import ASGITransport, AsyncClient  # noqa: E402

from app.db import Base, engine  # noqa: E402
from app.main import create_app  # noqa: E402
from app import models  # noqa: E402,F401


class FakeBridge:
    """Stands in for the MQTT bridge: records what would be sent to the broker."""
    def __init__(self):
        self.provisioned, self.deprovisioned, self.sent, self.passwords = [], [], [], []
        self.connected = True

    async def provision_device(self, device_id, token): self.provisioned.append((device_id, token))
    async def deprovision_device(self, device_id): self.deprovisioned.append(device_id)
    async def set_device_password(self, device_id, token): self.passwords.append((device_id, token))
    async def send_command(self, cmd): self.sent.append(cmd)


@pytest.fixture(autouse=True)
async def fresh_db():
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.drop_all)
        await conn.run_sync(Base.metadata.create_all)
    yield


@pytest.fixture
async def app_client():
    app = create_app(start_bridge=False)
    fake = FakeBridge()
    async with app.router.lifespan_context(app):
        app.state.bridge = fake
        async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as c:
            c.fake = fake
            yield c


async def login(client, email="ada@example.com", password="correct horse battery"):
    r = await client.post("/api/auth/register", json={"email": email, "password": password})
    assert r.status_code == 201, r.text
    client.headers["Authorization"] = f"Bearer {r.json()['access_token']}"
    return client
