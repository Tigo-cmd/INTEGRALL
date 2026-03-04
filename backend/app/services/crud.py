# app/services/crud.py
# Database CRUD operations for devices and commands

from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, update, delete, desc
from sqlalchemy.sql import func
from typing import List, Optional, Dict, Any
from datetime import datetime
import uuid
import json

from app.models.device import Device, Command, Telemetry
from app.schemas.device import (
    DeviceRegistration, DeviceUpdate, CommandCreate, 
    TelemetrySubmit, CommandResult
)


class DeviceCRUD:
    @staticmethod
    async def get_by_device_id(db: AsyncSession, device_id: str) -> Optional[Device]:
        result = await db.execute(
            select(Device).where(Device.device_id == device_id)
        )
        return result.scalar_one_or_none()
    
    @staticmethod
    async def get_multi(db: AsyncSession, skip: int = 0, limit: int = 100) -> List[Device]:
        result = await db.execute(
            select(Device).offset(skip).limit(limit)
        )
        return result.scalars().all()
    
    @staticmethod
    async def get_count(db: AsyncSession) -> int:
        result = await db.execute(select(func.count(Device.id)))
        return result.scalar()
    
    @staticmethod
    async def get_online_count(db: AsyncSession) -> int:
        result = await db.execute(
            select(func.count(Device.id)).where(Device.is_online == True)
        )
        return result.scalar()
    
    @staticmethod
    async def create_or_update(
        db: AsyncSession, 
        device_data: DeviceRegistration
    ) -> Device:
        device = await DeviceCRUD.get_by_device_id(db, device_data.device_id)
        
        if device:
            device.ip_address = device_data.ip_address
            device.mac_address = device_data.mac_address
            device.rssi = device_data.rssi
            device.firmware_version = device_data.firmware_version
            device.sdk_version = device_data.sdk_version
            device.is_online = True
            device.last_seen = datetime.utcnow()
        else:
            device = Device(
                device_id=device_data.device_id,
                ip_address=device_data.ip_address,
                mac_address=device_data.mac_address,
                rssi=device_data.rssi,
                firmware_version=device_data.firmware_version,
                sdk_version=device_data.sdk_version,
                is_online=True,
                last_seen=datetime.utcnow()
            )
            db.add(device)
        
        await db.commit()
        await db.refresh(device)
        return device
    
    @staticmethod
    async def update(
        db: AsyncSession, 
        device_id: str, 
        update_data: DeviceUpdate
    ) -> Optional[Device]:
        device = await DeviceCRUD.get_by_device_id(db, device_id)
        if not device:
            return None
        
        update_dict = update_data.model_dump(exclude_unset=True)
        for field, value in update_dict.items():
            setattr(device, field, value)
        
        await db.commit()
        await db.refresh(device)
        return device
    
    @staticmethod
    async def mark_offline(db: AsyncSession, device_id: str) -> bool:
        device = await DeviceCRUD.get_by_device_id(db, device_id)
        if device:
            device.is_online = False
            await db.commit()
            return True
        return False


class CommandCRUD:
    @staticmethod
    async def create(
        db: AsyncSession, 
        command_data: CommandCreate
    ) -> Command:
        command = Command(
            command_id=str(uuid.uuid4()),
            device_id=command_data.device_id,
            command_type=command_data.command_type.value,
            parameters=json.dumps(command_data.parameters) if command_data.parameters else None,
            status="pending"
        )
        db.add(command)
        await db.commit()
        await db.refresh(command)
        return command
    
    @staticmethod
    async def get_pending_for_device(
        db: AsyncSession, 
        device_id: str
    ) -> List[Command]:
        result = await db.execute(
            select(Command)
            .where(Command.device_id == device_id)
            .where(Command.status == "pending")
            .order_by(Command.created_at)
        )
        return result.scalars().all()
    
    @staticmethod
    async def mark_sent(db: AsyncSession, command_id: str) -> bool:
        result = await db.execute(
            update(Command)
            .where(Command.command_id == command_id)
            .values(status="sent", sent_at=datetime.utcnow())
        )
        await db.commit()
        return result.rowcount > 0
    
    @staticmethod
    async def mark_completed(
        db: AsyncSession, 
        command_id: str, 
        result_data: CommandResult
    ) -> bool:
        result = await db.execute(
            update(Command)
            .where(Command.command_id == command_id)
            .values(
                status="completed" if result_data.success else "failed",
                completed_at=datetime.utcnow(),
                response_data=json.dumps(result_data.model_dump()) if result_data else None,
                error_message=result_data.message if not result_data.success else None
            )
        )
        await db.commit()
        return result.rowcount > 0
    
    @staticmethod
    async def get_pending_count(db: AsyncSession) -> int:
        result = await db.execute(
            select(func.count(Command.id)).where(Command.status == "pending")
        )
        return result.scalar()


class TelemetryCRUD:
    @staticmethod
    async def create(
        db: AsyncSession, 
        telemetry_data: TelemetrySubmit
    ) -> Telemetry:
        data = telemetry_data.data
        telemetry = Telemetry(
            device_id=telemetry_data.device_id,
            data=json.dumps(data),
            temperature=data.get("temperature"),
            humidity=data.get("humidity"),
            uptime_seconds=data.get("uptime_seconds")
        )
        db.add(telemetry)
        await db.commit()
        await db.refresh(telemetry)
        return telemetry
    
    @staticmethod
    async def get_for_device(
        db: AsyncSession, 
        device_id: str, 
        limit: int = 100
    ) -> List[Telemetry]:
        result = await db.execute(
            select(Telemetry)
            .where(Telemetry.device_id == device_id)
            .order_by(desc(Telemetry.received_at))
            .limit(limit)
        )
        return result.scalars().all()
