"""API request/response models."""
import re
from datetime import datetime
from typing import Any

from pydantic import BaseModel, EmailStr, Field, field_validator

DEVICE_ID_RE = re.compile(r"^INT_[0-9A-F]{12}$")
ACTION_RE = re.compile(r"^[a-z][a-z0-9_]*(\.[a-z0-9_]+)*$")


class Credentials(BaseModel):
    email: EmailStr
    password: str = Field(min_length=8, max_length=128)


class TokenOut(BaseModel):
    access_token: str
    token_type: str = "bearer"


class DeviceCreate(BaseModel):
    id: str = Field(description="Device ID printed by the firmware, e.g. INT_A1B2C3D4E5F6")
    name: str = Field(min_length=1, max_length=80)

    @field_validator("id")
    @classmethod
    def _id(cls, v: str) -> str:
        v = v.strip().upper()
        if not DEVICE_ID_RE.match(v):
            raise ValueError("device id must look like INT_A1B2C3D4E5F6 (see the device's serial output)")
        return v


class DeviceOut(BaseModel):
    id: str
    name: str
    online: bool
    last_seen: datetime | None
    meta: dict | None
    health: dict | None
    last_telemetry: dict | None
    created_at: datetime

    model_config = {"from_attributes": True}


class DeviceCreated(DeviceOut):
    token: str = Field(description="MQTT password for the device. Shown only once.")


class DeviceRename(BaseModel):
    name: str = Field(min_length=1, max_length=80)


class TelemetryOut(BaseModel):
    ts: datetime
    data: dict
    model_config = {"from_attributes": True}


class EventOut(BaseModel):
    ts: datetime
    type: str
    detail: str
    model_config = {"from_attributes": True}


class CommandIn(BaseModel):
    action: str = Field(max_length=64)
    args: dict[str, Any] = Field(default_factory=dict)

    @field_validator("action")
    @classmethod
    def _action(cls, v: str) -> str:
        if not ACTION_RE.match(v):
            raise ValueError("action must be dotted lowercase, e.g. relay.set")
        return v


class CommandOut(BaseModel):
    id: str
    device_id: str
    action: str
    args: dict
    status: str
    message: str | None
    result: dict | None
    created_at: datetime
    completed_at: datetime | None
    model_config = {"from_attributes": True}
