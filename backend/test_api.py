#!/usr/bin/env python3
"""
Integrall Backend API Test Script

Tests all major API endpoints to verify backend functionality.
Run this after starting the backend server.

Usage:
    python test_api.py

Requirements:
    - Backend running on http://localhost:8000
    - httpx installed (pip install httpx)
"""

import httpx
import asyncio
import sys
from datetime import datetime

# Configuration
BASE_URL = "http://localhost:8000"
API_KEY = "your-iot-device-api-key"  # Change to match your .env

# Test device ID
TEST_DEVICE_ID = "INT_TEST_DEVICE_001"


async def test_health():
    """Test public health endpoint"""
    print("\n🧪 Testing Health Endpoint...")
    async with httpx.AsyncClient() as client:
        response = await client.get(f"{BASE_URL}/health")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "healthy"
        print(f"✅ Health check passed: {data['version']}")


async def test_auth_failure():
    """Test that missing API key returns 401"""
    print("\n🧪 Testing Authentication...")
    async with httpx.AsyncClient() as client:
        response = await client.get(f"{BASE_URL}/api/devices")
        assert response.status_code == 401
        print("✅ Authentication working (401 for missing key)")


async def test_device_registration():
    """Test device registration"""
    print("\n🧪 Testing Device Registration...")
    
    headers = {"X-API-Key": API_KEY}
    device_data = {
        "device_id": TEST_DEVICE_ID,
        "ip_address": "192.168.1.100",
        "mac_address": "AA:BB:CC:DD:EE:FF",
        "rssi": -65,
        "firmware_version": "0.1.0",
        "sdk_version": "v4.4.4"
    }
    
    async with httpx.AsyncClient() as client:
        # Register device
        response = await client.post(
            f"{BASE_URL}/api/devices/register",
            json=device_data,
            headers=headers
        )
        assert response.status_code == 201
        data = response.json()
        assert data["device_id"] == TEST_DEVICE_ID
        print(f"✅ Device registered: {data['device_id']}")
        
        # List devices
        response = await client.get(
            f"{BASE_URL}/api/devices",
            headers=headers
        )
        assert response.status_code == 200
        data = response.json()
        assert len(data["devices"]) > 0
        print(f"✅ Device list retrieved: {data['total']} devices")


async def test_commands():
    """Test command creation and retrieval"""
    print("\n🧪 Testing Command System...")
    
    headers = {"X-API-Key": API_KEY}
    
    async with httpx.AsyncClient() as client:
        # Create command
        command_data = {
            "device_id": TEST_DEVICE_ID,
            "command_type": "relay_on",
            "parameters": {"pin": 5, "safety_timeout_ms": 30000}
        }
        
        response = await client.post(
            f"{BASE_URL}/api/commands",
            json=command_data,
            headers=headers
        )
        assert response.status_code == 201
        command = response.json()
        print(f"✅ Command created: {command['command_id']}")
        
        # Poll commands (as device would)
        response = await client.get(
            f"{BASE_URL}/api/devices/{TEST_DEVICE_ID}/commands",
            headers=headers
        )
        assert response.status_code == 200
        data = response.json()
        print(f"✅ Commands polled: {len(data['commands'])} pending")
        
        # Submit command response (as device would)
        if data['commands']:
            cmd_id = data['commands'][0]['command_id']
            response_data = {
                "success": True,
                "message": "Relay activated successfully"
            }
            response = await client.post(
                f"{BASE_URL}/api/commands/{cmd_id}/response",
                json=response_data,
                headers=headers
            )
            assert response.status_code == 200
            print(f"✅ Command response submitted")


async def test_telemetry():
    """Test telemetry submission"""
    print("\n🧪 Testing Telemetry...")
    
    headers = {"X-API-Key": API_KEY}
    telemetry_data = {
        "device_id": TEST_DEVICE_ID,
        "data": {
            "temperature": 24.5,
            "humidity": 60,
            "relay_states": [True, False, False, False],
            "free_heap": 150000,
            "uptime_seconds": 3600
        }
    }
    
    async with httpx.AsyncClient() as client:
        response = await client.post(
            f"{BASE_URL}/api/telemetry",
            json=telemetry_data,
            headers=headers
        )
        assert response.status_code == 201
        print(f"✅ Telemetry submitted")
        
        # Retrieve telemetry
        response = await client.get(
            f"{BASE_URL}/api/devices/{TEST_DEVICE_ID}/telemetry",
            headers=headers
        )
        assert response.status_code == 200
        data = response.json()
        print(f"✅ Telemetry history retrieved: {len(data)} records")


async def test_system_status():
    """Test system status endpoint"""
    print("\n🧪 Testing System Status...")
    
    headers = {"X-API-Key": API_KEY}
    
    async with httpx.AsyncClient() as client:
        response = await client.get(
            f"{BASE_URL}/api/system/status",
            headers=headers
        )
        assert response.status_code == 200
        data = response.json()
        assert "version" in data
        assert "connected_devices" in data
        print(f"✅ System status: {data['status']}")
        print(f"   Version: {data['version']}")
        print(f"   Devices: {data['connected_devices']}/{data['total_devices']}")
        print(f"   Pending: {data['pending_commands']}")


async def run_all_tests():
    """Run all tests in sequence"""
    print("=" * 60)
    print("INTEGRALL BACKEND API TEST SUITE")
    print("=" * 60)
    print(f"Base URL: {BASE_URL}")
    print(f"Time: {datetime.now().isoformat()}")
    
    try:
        await test_health()
        await test_auth_failure()
        await test_device_registration()
        await test_commands()
        await test_telemetry()
        await test_system_status()
        
        print("\n" + "=" * 60)
        print("✅ ALL TESTS PASSED!")
        print("=" * 60)
        return 0
        
    except AssertionError as e:
        print(f"\n❌ TEST FAILED: {e}")
        return 1
    except Exception as e:
        print(f"\n❌ ERROR: {e}")
        return 1


if __name__ == "__main__":
    result = asyncio.run(run_all_tests())
    sys.exit(result)
