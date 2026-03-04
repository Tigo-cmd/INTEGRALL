# app/schemas/device.py
# Pydantic schemas for request/response validation

from pydantic import BaseModel, Field
from typing import Optional, Dict, Any, List
from datetime import datetime
from enum import Enum


# ============================================================================
# Device Schemas
# ============================================================================

class DeviceRegistration(BaseModel):
    """Device registration request from ESP32"""
    device_id: str = Field(..., min_length=10, max_length=32, description="Unique device ID")
    ip_address: Optional[str] = Field(None, description="Device IP address")
    mac_address: Optional[str] = Field(None, description="MAC address")
    rssi: Optional[int] = Field(None, description="WiFi signal strength (dBm)")
    firmware_version: Optional[str] = Field(None, description="Firmware version")
    sdk_version: Optional[str] = Field(None, description="ESP32 SDK version")
    
    class Config:
        json_schema_extra = {
            "example": {
                "device_id": "INT_A1B2C3D4E5F6",
                "ip_address": "192.168.1.105",
                "mac_address": "A1:B2:C3:D4:E5:F6",
                "rssi": -65,
                "firmware_version": "0.1.0",
                "sdk_version": "v4.4.4"
            }
        }


class DeviceUpdate(BaseModel):
    """Update device information"""
    name: Optional[str] = Field(None, max_length=100)
    location: Optional[str] = Field(None, max_length=200)
    description: Optional[str] = None
    capabilities: Optional[Dict[str, Any]] = None
    is_online: Optional[bool] = None


class DeviceResponse(BaseModel):
    """Device information response"""
    id: int
    device_id: str
    name: Optional[str]
    ip_address: Optional[str]
    mac_address: Optional[str]
    is_online: bool
    last_seen: Optional[datetime]
    firmware_version: Optional[str]
    rssi: Optional[int]
    capabilities: Optional[str]
    
    class Config:
        from_attributes = True


class DeviceListResponse(BaseModel):
    """List of devices with metadata"""
    devices: List[DeviceResponse]
    total: int
    online_count: int


# ============================================================================
# Command Schemas
# ============================================================================

class CommandType(str, Enum):
    RELAY_ON = "relay_on"
    RELAY_OFF = "relay_off"
    RELAY_TOGGLE = "relay_toggle"
    RELAY_SET_TIMEOUT = "relay_set_timeout"
    # Future: KEYPAD_UNLOCK, DISPLAY_TEXT, etc.


class CommandCreate(BaseModel):
    """Create a new command for a device"""
    command_type: CommandType = Field(..., description="Type of command")
    device_id: str = Field(..., description="Target device ID")
    parameters: Optional[Dict[str, Any]] = Field(
        default={},
        description="Command parameters (e.g., {'pin': 5, 'duration': 30000})"
    )
    
    class Config:
        json_schema_extra = {
            "example": {
                "command_type": "relay_on",
                "device_id": "INT_A1B2C3D4E5F6",
                "parameters": {"pin": 5, "safety_timeout_ms": 30000}
            }
        }


class CommandResponse(BaseModel):
    """Command details response"""
    command_id: str
    device_id: str
    command_type: str
    parameters: Optional[str]  # JSON string
    status: str
    created_at: Optional[datetime]
    sent_at: Optional[datetime]
    completed_at: Optional[datetime]
    
    class Config:
        from_attributes = True


class CommandResult(BaseModel):
    """Device response to a command"""
    success: bool = Field(..., description="Whether command executed successfully")
    message: Optional[str] = Field(None, description="Result message or error")
    data: Optional[Dict[str, Any]] = Field(None, description="Additional response data")


class PendingCommandsResponse(BaseModel):
    """Commands waiting to be sent to device"""
    commands: List[CommandResponse]


# ============================================================================
# Telemetry Schemas
# ============================================================================

class TelemetrySubmit(BaseModel):
    """Telemetry data from device"""
    device_id: str
    data: Dict[str, Any] = Field(..., description="Telemetry payload")
    
    class Config:
        json_schema_extra = {
            "example": {
                "device_id": "INT_A1B2C3D4E5F6",
                "data": {
                    "relay_states": [True, False, False],
                    "uptime_seconds": 3600,
                    "free_heap": 150000
                }
            }
        }


class TelemetryResponse(BaseModel):
    """Telemetry record response"""
    id: int
    device_id: str
    data: str  # JSON string
    received_at: datetime
    
    class Config:
        from_attributes = True


# ============================================================================
# System Schemas
# ============================================================================

class SystemStatus(BaseModel):
    """Backend system status"""
    status: str = "operational"
    version: str
    uptime_seconds: float
    connected_devices: int
    total_devices: int
    pending_commands: int


class HealthCheck(BaseModel):
    """Simple health check response"""
    status: str
    timestamp: datetime
