# app/models/device.py
# Database models for Integrall IoT system

from sqlalchemy import Column, Integer, String, Boolean, DateTime, Float, Text, Index
from sqlalchemy.sql import func
from app.core.database import Base
from datetime import datetime
from typing import Optional


class Device(Base):
    """Represents an ESP32 device connected to the system"""
    __tablename__ = "devices"
    
    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(String(32), unique=True, index=True, nullable=False)
    name = Column(String(100), nullable=True)
    
    # Network info
    ip_address = Column(String(15), nullable=True)
    mac_address = Column(String(17), nullable=True)
    wifi_ssid = Column(String(100), nullable=True)
    rssi = Column(Integer, nullable=True)  # WiFi signal strength
    
    # Device info
    firmware_version = Column(String(20), nullable=True)
    sdk_version = Column(String(30), nullable=True)
    
    # Status
    is_online = Column(Boolean, default=False)
    last_seen = Column(DateTime(timezone=True), server_default=func.now())
    registered_at = Column(DateTime(timezone=True), server_default=func.now())
    
    # Capabilities (JSON-like storage for simplicity in v0.1)
    capabilities = Column(Text, nullable=True)  # JSON: {"relay": 4, "keypad": true}
    
    # Metadata
    location = Column(String(200), nullable=True)
    description = Column(Text, nullable=True)
    
    def __repr__(self):
        return f"<Device(device_id={self.device_id}, online={self.is_online})>"
    
    def to_dict(self):
        return {
            "id": self.id,
            "device_id": self.device_id,
            "name": self.name,
            "ip_address": self.ip_address,
            "mac_address": self.mac_address,
            "is_online": self.is_online,
            "last_seen": self.last_seen.isoformat() if self.last_seen else None,
            "firmware_version": self.firmware_version,
            "rssi": self.rssi,
            "capabilities": self.capabilities,
        }


class Command(Base):
    """Commands sent to devices (relay on/off, etc.)"""
    __tablename__ = "commands"
    
    id = Column(Integer, primary_key=True, index=True)
    command_id = Column(String(36), unique=True, index=True, nullable=False)
    device_id = Column(String(32), index=True, nullable=False)
    
    # Command details
    command_type = Column(String(50), nullable=False)  # "relay_on", "relay_off", etc.
    parameters = Column(Text, nullable=True)  # JSON: {"pin": 5, "duration": 30000}
    
    # Status tracking
    status = Column(String(20), default="pending")  # pending, sent, completed, failed
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    sent_at = Column(DateTime(timezone=True), nullable=True)
    completed_at = Column(DateTime(timezone=True), nullable=True)
    
    # Response from device
    response_data = Column(Text, nullable=True)
    error_message = Column(Text, nullable=True)
    
    def __repr__(self):
        return f"<Command(id={self.command_id}, type={self.command_type}, status={self.status})>"
    
    def to_dict(self):
        return {
            "command_id": self.command_id,
            "device_id": self.device_id,
            "command_type": self.command_type,
            "parameters": self.parameters,
            "status": self.status,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "sent_at": self.sent_at.isoformat() if self.sent_at else None,
            "completed_at": self.completed_at.isoformat() if self.completed_at else None,
        }


class Telemetry(Base):
    """Telemetry data received from devices"""
    __tablename__ = "telemetry"
    
    id = Column(Integer, primary_key=True, index=True)
    device_id = Column(String(32), index=True, nullable=False)
    
    # Telemetry data (JSON)
    data = Column(Text, nullable=False)
    received_at = Column(DateTime(timezone=True), server_default=func.now())
    
    # Optional: parsed fields for common metrics
    temperature = Column(Float, nullable=True)
    humidity = Column(Float, nullable=True)
    uptime_seconds = Column(Integer, nullable=True)
    
    def __repr__(self):
        return f"<Telemetry(device={self.device_id}, received={self.received_at})>"
