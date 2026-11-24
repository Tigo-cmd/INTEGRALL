"""Live updates for the dashboard: one WebSocket per browser tab, server pushes."""
from fastapi import APIRouter, WebSocket, WebSocketDisconnect, status

from ..db import SessionLocal
from ..hub import hub
from ..models import User
from ..security import decode_access_token

router = APIRouter()


@router.websocket("/api/ws")
async def live(ws: WebSocket) -> None:
    # Browsers can't set headers on WebSocket: the JWT comes as the first message,
    # never in the URL (URLs end up in proxy logs).
    await ws.accept()
    try:
        first = await ws.receive_json()
    except (WebSocketDisconnect, ValueError):
        return
    user_id = decode_access_token(str(first.get("token", ""))) if isinstance(first, dict) else None
    async with SessionLocal() as db:
        user = await db.get(User, user_id) if user_id is not None else None
    if user is None:
        await ws.close(code=status.WS_1008_POLICY_VIOLATION)
        return
    await hub.add(user.id, ws)
    await ws.send_json({"type": "hello", "user": user.email})
    try:
        while True:
            await ws.receive_text()        # client pings keep proxies happy; content ignored
    except WebSocketDisconnect:
        pass
    finally:
        await hub.remove(user.id, ws)
