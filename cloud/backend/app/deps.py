"""Shared FastAPI dependencies."""
from fastapi import Depends, HTTPException, Request, status
from fastapi.security import HTTPAuthorizationCredentials, HTTPBearer
from sqlalchemy.ext.asyncio import AsyncSession

from .bridge import Bridge
from .db import get_session
from .models import Device, User
from .security import decode_access_token

bearer = HTTPBearer(auto_error=False)


async def current_user(
    creds: HTTPAuthorizationCredentials | None = Depends(bearer),
    db: AsyncSession = Depends(get_session),
) -> User:
    user_id = decode_access_token(creds.credentials) if creds else None
    user = await db.get(User, user_id) if user_id is not None else None
    if user is None:
        raise HTTPException(status.HTTP_401_UNAUTHORIZED, "not authenticated",
                            headers={"WWW-Authenticate": "Bearer"})
    return user


def get_bridge(request: Request) -> Bridge:
    return request.app.state.bridge


async def owned_device(device_id: str, user: User, db: AsyncSession) -> Device:
    device = await db.get(Device, device_id.upper())
    if device is None or device.owner_id != user.id:
        raise HTTPException(status.HTTP_404_NOT_FOUND, "device not found")
    return device
