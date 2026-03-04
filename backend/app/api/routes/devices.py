# app/api/routes/devices.py
# Device management API routes

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List, Optional

from app.core.database import get_db
from app.core.config import get_settings
from app.services.crud import DeviceCRUD
from app.schemas.device import (
    DeviceRegistration, DeviceResponse, DeviceListResponse, DeviceUpdate
)

router = APIRouter()
settings = get_settings()


@router.post("/devices/register", response_model=DeviceResponse, status_code=201)
async def register_device(
    device_data: DeviceRegistration,
    db: AsyncSession = Depends(get_db)
):
    """
    Register or update a device connection.
    
    Called by ESP32 on startup to register itself with the backend.
    Creates new device record if not exists, updates existing if found.
    """
    device = await DeviceCRUD.create_or_update(db, device_data)
    return device


@router.get("/devices", response_model=DeviceListResponse)
async def list_devices(
    skip: int = 0,
    limit: int = 100,
    db: AsyncSession = Depends(get_db)
):
    """
    List all registered devices with their status.
    """
    devices = await DeviceCRUD.get_multi(db, skip=skip, limit=limit)
    total = await DeviceCRUD.get_count(db)
    online = await DeviceCRUD.get_online_count(db)
    
    return DeviceListResponse(
        devices=devices,
        total=total,
        online_count=online
    )


@router.get("/devices/{device_id}", response_model=DeviceResponse)
async def get_device(
    device_id: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Get specific device details by device ID.
    """
    device = await DeviceCRUD.get_by_device_id(db, device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )
    return device


@router.patch("/devices/{device_id}", response_model=DeviceResponse)
async def update_device(
    device_id: str,
    update_data: DeviceUpdate,
    db: AsyncSession = Depends(get_db)
):
    """
    Update device information (name, location, etc.).
    """
    device = await DeviceCRUD.update(db, device_id, update_data)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )
    return device


@router.post("/devices/{device_id}/heartbeat")
async def device_heartbeat(
    device_id: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Device heartbeat endpoint - updates last_seen timestamp.
    Can be used by devices to maintain online status without full registration.
    """
    device = await DeviceCRUD.get_by_device_id(db, device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )
    
    # Update last_seen
    from datetime import datetime
    device.last_seen = datetime.utcnow()
    device.is_online = True
    await db.commit()
    
    return {"status": "ok", "timestamp": device.last_seen.isoformat()}
