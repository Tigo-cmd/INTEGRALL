# app/api/routes/telemetry.py
# Telemetry data API routes

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List

from app.core.database import get_db
from app.services.crud import TelemetryCRUD, DeviceCRUD
from app.schemas.device import TelemetrySubmit, TelemetryResponse

router = APIRouter()


@router.post("/telemetry", response_model=TelemetryResponse, status_code=201)
async def submit_telemetry(
    telemetry_data: TelemetrySubmit,
    db: AsyncSession = Depends(get_db)
):
    """
    Submit telemetry data from device.
    
    Called by ESP32 to report sensor readings, relay states, etc.
    """
    # Verify device exists
    device = await DeviceCRUD.get_by_device_id(db, telemetry_data.device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {telemetry_data.device_id} not found"
        )
    
    telemetry = await TelemetryCRUD.create(db, telemetry_data)
    return telemetry


@router.get("/devices/{device_id}/telemetry", response_model=List[TelemetryResponse])
async def get_device_telemetry(
    device_id: str,
    limit: int = 100,
    db: AsyncSession = Depends(get_db)
):
    """
    Get telemetry history for a device.
    """
    device = await DeviceCRUD.get_by_device_id(db, device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )
    
    telemetry = await TelemetryCRUD.get_for_device(db, device_id, limit)
    return telemetry
