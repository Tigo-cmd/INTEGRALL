# app/api/routes/commands.py
# Command management API routes

from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List

from app.core.database import get_db
from app.services.crud import CommandCRUD, DeviceCRUD
from app.schemas.device import (
    CommandCreate, CommandResponse, PendingCommandsResponse, CommandResult
)

router = APIRouter()


@router.post("/commands", response_model=CommandResponse, status_code=201)
async def create_command(
    command_data: CommandCreate,
    db: AsyncSession = Depends(get_db)
):
    """
    Create a new command for a device.
    
    Example:
    {
        "device_id": "INT_A1B2C3D4E5F6",
        "command_type": "relay_on",
        "parameters": {"pin": 5, "safety_timeout_ms": 30000}
    }
    """
    # Verify device exists
    device = await DeviceCRUD.get_by_device_id(db, command_data.device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {command_data.device_id} not found"
        )
    
    command = await CommandCRUD.create(db, command_data)
    return command


@router.get("/devices/{device_id}/commands", response_model=PendingCommandsResponse)
async def get_pending_commands(
    device_id: str,
    db: AsyncSession = Depends(get_db)
):
    """
    Get pending commands for a device (polled by ESP32).
    
    This endpoint is called by the device every few seconds to check
    for new commands from the backend.
    """
    # Verify device exists
    device = await DeviceCRUD.get_by_device_id(db, device_id)
    if not device:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Device {device_id} not found"
        )
    
    # Get pending commands
    commands = await CommandCRUD.get_pending_for_device(db, device_id)
    
    # Mark commands as sent (they're being delivered to device)
    for cmd in commands:
        await CommandCRUD.mark_sent(db, cmd.command_id)
    
    return PendingCommandsResponse(commands=commands)


@router.post("/commands/{command_id}/response")
async def command_response(
    command_id: str,
    result: CommandResult,
    db: AsyncSession = Depends(get_db)
):
    """
    Submit device response to a command.
    
    Called by ESP32 after executing a command to report success/failure.
    """
    success = await CommandCRUD.mark_completed(db, command_id, result)
    if not success:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Command {command_id} not found"
        )
    
    return {"status": "recorded", "command_id": command_id}


@router.get("/commands", response_model=List[CommandResponse])
async def list_commands(
    device_id: str = None,
    status: str = None,
    limit: int = 100,
    db: AsyncSession = Depends(get_db)
):
    """
    List commands with optional filtering.
    """
    # TODO: Add filtering logic
    from sqlalchemy import select
    from app.models.device import Command
    
    query = select(Command).limit(limit).order_by(Command.created_at.desc())
    
    if device_id:
        query = query.where(Command.device_id == device_id)
    if status:
        query = query.where(Command.status == status)
    
    result = await db.execute(query)
    commands = result.scalars().all()
    
    return commands
