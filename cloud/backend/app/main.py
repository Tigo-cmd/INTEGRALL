"""Integrall cloud backend: device registry, MQTT bridge, REST + WebSocket API."""
import asyncio
import contextlib
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .bridge import Bridge
from .config import get_settings
from .db import SessionLocal
from .hub import hub
from .openremote import OpenRemoteSync, RestApi
from .routes import auth, devices, live

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")


@asynccontextmanager
async def lifespan(app: FastAPI):
    settings = get_settings()
    bridge = Bridge(settings, SessionLocal, hub)
    app.state.bridge = bridge
    app.state.openremote = None
    tasks = []
    if app.state.start_bridge:
        tasks.append(asyncio.create_task(bridge.run()))
        if settings.or_enabled:
            sync = OpenRemoteSync(settings, SessionLocal, bridge, RestApi(settings))
            bridge.listeners.append(sync.notify)
            app.state.openremote = sync
            tasks.append(asyncio.create_task(sync.run()))
    yield
    for task in tasks:
        task.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            await task
    if app.state.openremote:
        await app.state.openremote.api.aclose()


def create_app(start_bridge: bool = True) -> FastAPI:
    app = FastAPI(title="Integrall Cloud", version="1.2.0", lifespan=lifespan)
    app.state.start_bridge = start_bridge
    app.add_middleware(CORSMiddleware, allow_origins=get_settings().cors_list, allow_credentials=False,
                       allow_methods=["*"], allow_headers=["Authorization", "Content-Type"])
    app.include_router(auth.router)
    app.include_router(devices.router)
    app.include_router(live.router)

    @app.get("/api/health", tags=["ops"])
    async def health() -> dict:
        sync = app.state.openremote
        return {"ok": True, "mqtt": app.state.bridge.connected, "ws_clients": hub.connections(),
                "openremote": None if sync is None else sync.connected}

    return app


app = create_app()
