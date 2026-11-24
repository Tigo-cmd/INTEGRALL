from datetime import timedelta

from fastapi import APIRouter, Depends, HTTPException, Query, Request, Response, status
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..bridge import Bridge, BrokerUnavailable
from ..db import get_session
from ..deps import current_user, get_bridge, owned_device
from ..models import Command, Device, Event, Telemetry, User, utcnow
from ..schemas import (CommandIn, CommandOut, DeviceCreate, DeviceCreated, DeviceOut, DeviceRename,
                       EventOut, TelemetryOut)
from ..security import new_device_token

router = APIRouter(prefix="/api/devices", tags=["devices"])


def _notify_openremote(request: Request, device_id: str, update: dict) -> None:
    sync = getattr(request.app.state, "openremote", None)
    if sync is not None:
        sync.notify(device_id, update)


@router.get("", response_model=list[DeviceOut])
async def list_devices(user: User = Depends(current_user), db: AsyncSession = Depends(get_session)):
    rows = await db.scalars(select(Device).where(Device.owner_id == user.id).order_by(Device.name))
    return list(rows)


@router.post("", response_model=DeviceCreated, status_code=status.HTTP_201_CREATED)
async def create_device(body: DeviceCreate, request: Request, user: User = Depends(current_user),
                        db: AsyncSession = Depends(get_session), bridge: Bridge = Depends(get_bridge)):
    if await db.get(Device, body.id):
        raise HTTPException(status.HTTP_409_CONFLICT, "device already registered")
    token = new_device_token()
    try:
        await bridge.provision_device(body.id, token)    # broker login first: no orphan DB rows
    except BrokerUnavailable as e:
        raise HTTPException(status.HTTP_503_SERVICE_UNAVAILABLE, str(e)) from e
    device = Device(id=body.id, name=body.name, owner_id=user.id)
    db.add(device)
    await db.commit()
    await db.refresh(device)
    _notify_openremote(request, device.id, {"type": "registered"})
    return DeviceCreated(**DeviceOut.model_validate(device).model_dump(), token=token)


@router.get("/{device_id}", response_model=DeviceOut)
async def get_device(device_id: str, user: User = Depends(current_user), db: AsyncSession = Depends(get_session)):
    return await owned_device(device_id, user, db)


@router.patch("/{device_id}", response_model=DeviceOut)
async def rename_device(device_id: str, body: DeviceRename, user: User = Depends(current_user),
                        db: AsyncSession = Depends(get_session)):
    device = await owned_device(device_id, user, db)
    device.name = body.name
    await db.commit()
    return device


@router.post("/{device_id}/rotate-token")
async def rotate_token(device_id: str, user: User = Depends(current_user),
                       db: AsyncSession = Depends(get_session), bridge: Bridge = Depends(get_bridge)) -> dict:
    device = await owned_device(device_id, user, db)
    token = new_device_token()
    try:
        await bridge.set_device_password(device.id, token)
    except BrokerUnavailable as e:
        raise HTTPException(status.HTTP_503_SERVICE_UNAVAILABLE, str(e)) from e
    return {"token": token}


@router.delete("/{device_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_device(device_id: str, request: Request, user: User = Depends(current_user),
                        db: AsyncSession = Depends(get_session), bridge: Bridge = Depends(get_bridge)):
    device = await owned_device(device_id, user, db)
    try:
        await bridge.deprovision_device(device.id)
    except BrokerUnavailable as e:
        raise HTTPException(status.HTTP_503_SERVICE_UNAVAILABLE, str(e)) from e
    asset_id = device.or_asset_id
    await db.delete(device)
    await db.commit()
    _notify_openremote(request, device_id.upper(), {"type": "removed", "asset_id": asset_id})
    return Response(status_code=status.HTTP_204_NO_CONTENT)


@router.get("/{device_id}/telemetry", response_model=list[TelemetryOut])
async def telemetry(device_id: str, minutes: int = Query(60, ge=1, le=60 * 24 * 31),
                    limit: int = Query(1000, ge=1, le=10000),
                    user: User = Depends(current_user), db: AsyncSession = Depends(get_session)):
    await owned_device(device_id, user, db)
    since = utcnow() - timedelta(minutes=minutes)
    rows = await db.scalars(select(Telemetry).where(Telemetry.device_id == device_id.upper(), Telemetry.ts >= since)
                            .order_by(Telemetry.ts.desc()).limit(limit))
    return list(reversed(list(rows)))


@router.get("/{device_id}/events", response_model=list[EventOut])
async def events(device_id: str, limit: int = Query(100, ge=1, le=1000),
                 user: User = Depends(current_user), db: AsyncSession = Depends(get_session)):
    await owned_device(device_id, user, db)
    rows = await db.scalars(select(Event).where(Event.device_id == device_id.upper())
                            .order_by(Event.ts.desc()).limit(limit))
    return list(rows)


@router.post("/{device_id}/commands", response_model=CommandOut, status_code=status.HTTP_202_ACCEPTED)
async def send_command(device_id: str, body: CommandIn, user: User = Depends(current_user),
                       db: AsyncSession = Depends(get_session), bridge: Bridge = Depends(get_bridge)):
    """Publish a command. The result arrives later on the WebSocket ("command" message)."""
    device = await owned_device(device_id, user, db)
    if not device.online:
        raise HTTPException(status.HTTP_409_CONFLICT, "device is offline; commands are not queued")
    cmd = Command(device_id=device.id, action=body.action, args=body.args)
    db.add(cmd)
    await db.commit()
    await db.refresh(cmd)
    try:
        await bridge.send_command(cmd)
    except BrokerUnavailable as e:
        cmd.status, cmd.message, cmd.completed_at = "failed", str(e), utcnow()
        await db.commit()
        raise HTTPException(status.HTTP_503_SERVICE_UNAVAILABLE, str(e)) from e
    return cmd


@router.get("/{device_id}/commands", response_model=list[CommandOut])
async def list_commands(device_id: str, limit: int = Query(50, ge=1, le=500),
                        user: User = Depends(current_user), db: AsyncSession = Depends(get_session)):
    await owned_device(device_id, user, db)
    rows = await db.scalars(select(Command).where(Command.device_id == device_id.upper())
                            .order_by(Command.created_at.desc()).limit(limit))
    return list(rows)
