# app/main.py
# FastAPI application entry point with lifespan events

from contextlib import asynccontextmanager
from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.security import APIKeyHeader
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import time

from app.core.config import get_settings
from app.core.database import init_db, close_db
from app.api.routes import devices, commands, telemetry, system

settings = get_settings()

# API Key security scheme
api_key_header = APIKeyHeader(name="X-API-Key", auto_error=False)

# Global start time for uptime calculation
_start_time = time.time()


async def verify_api_key(api_key: str = Depends(api_key_header)):
    """Verify API key for device endpoints"""
    if api_key is None:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="API key is required in X-API-Key header"
        )
    if api_key != settings.API_KEY:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid API key"
        )
    return api_key


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan events - startup and shutdown"""
    # Startup: Initialize database
    await init_db()
    print(f"=== Integrall Backend v{settings.VERSION} Started ===")
    print(f"Database: {settings.DATABASE_URL}")
    print(f"Debug mode: {settings.DEBUG}")
    
    yield  # Application runs here
    
    # Shutdown: Cleanup
    await close_db()
    print("=== Integrall Backend Shutdown ===")


# Create FastAPI app with lifespan
app = FastAPI(
    title=settings.APP_NAME,
    version=settings.VERSION,
    description="Unified IoT firmware framework backend - manages ESP32 devices, commands, and telemetry",
    lifespan=lifespan,
    # Add API key auth to all routes by default
    dependencies=[Depends(verify_api_key)]
)

# CORS middleware (configure for production)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"] if settings.DEBUG else [],  # Restrict in production
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# Public health check (no auth required)
@app.get("/health", tags=["System"])
async def health_check():
    """Public health check endpoint"""
    return {
        "status": "healthy",
        "timestamp": datetime.utcnow().isoformat(),
        "version": settings.VERSION
    }


# System status (protected)
@app.get("/api/status", tags=["System"])
async def system_status():
    """Protected system status"""
    uptime = time.time() - _start_time
    return {
        "status": "operational",
        "version": settings.VERSION,
        "uptime_seconds": int(uptime),
        "timestamp": datetime.utcnow().isoformat()
    }


# Include API routers
app.include_router(devices.router, prefix="/api", tags=["Devices"])
app.include_router(commands.router, prefix="/api", tags=["Commands"])
app.include_router(telemetry.router, prefix="/api", tags=["Telemetry"])
app.include_router(system.router, prefix="/api", tags=["System"])


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=settings.DEBUG
    )
