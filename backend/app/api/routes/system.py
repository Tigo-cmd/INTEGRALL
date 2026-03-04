# app/api/routes/system.py
# System-level API routes

from fastapi import APIRouter, Depends
from sqlalchemy.ext.asyncio import AsyncSession

from app.core.database import get_db
from app.core.config import get_settings
from app.services.crud import DeviceCRUD, CommandCRUD
from app.schemas.device import SystemStatus

router = APIRouter()
settings = get_settings()


@router.get("/system/status", response_model=SystemStatus)
async def get_system_status(db: AsyncSession = Depends(get_db)):
    """
    Get overall system status and statistics.
    """
    total_devices = await DeviceCRUD.get_count(db)
    online_devices = await DeviceCRUD.get_online_count(db)
    pending_commands = await CommandCRUD.get_pending_count(db)
    
    import time
    from app.main import _start_time
    uptime = int(time.time() - _start_time)
    
    return SystemStatus(
        status="operational",
        version=settings.VERSION,
        uptime_seconds=uptime,
        connected_devices=online_devices,
        total_devices=total_devices,
        pending_commands=pending_commands
    )
