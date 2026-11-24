"""Fan-out of live updates to connected dashboard WebSockets (push, never polled)."""
import asyncio
import json
import logging
from collections import defaultdict

from fastapi import WebSocket

log = logging.getLogger("integrall.hub")


class Hub:
    def __init__(self) -> None:
        self._by_user: dict[int, set[WebSocket]] = defaultdict(set)
        self._lock = asyncio.Lock()

    async def add(self, user_id: int, ws: WebSocket) -> None:
        async with self._lock:
            self._by_user[user_id].add(ws)

    async def remove(self, user_id: int, ws: WebSocket) -> None:
        async with self._lock:
            self._by_user[user_id].discard(ws)
            if not self._by_user[user_id]:
                del self._by_user[user_id]

    async def send(self, user_id: int, message: dict) -> None:
        async with self._lock:
            targets = list(self._by_user.get(user_id, ()))
        if not targets:
            return
        text = json.dumps(message, default=str)
        dead = []
        for ws in targets:
            try:
                await ws.send_text(text)
            except Exception:  # client vanished mid-send
                dead.append(ws)
        for ws in dead:
            await self.remove(user_id, ws)

    def connections(self) -> int:
        return sum(len(v) for v in self._by_user.values())


hub = Hub()
