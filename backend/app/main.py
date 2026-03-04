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
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"] if settings.DEBUG else [],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

from fastapi.staticfiles import StaticFiles
import os

# Mount static files for the dashboard
static_dir = os.path.join(os.path.dirname(__file__), "static")
if os.path.exists(static_dir):
    app.mount("/static", StaticFiles(directory=static_dir), name="static")
    
    # Root redirect to dashboard
    @app.get("/", tags=["UI"])
    async def redirect_to_dashboard():
        from fastapi.responses import RedirectResponse
        return RedirectResponse(url="/static/index.html")

# Public health check (no auth required)
@app.get("/health", tags=["System"])
async def health_check():
    """Public health check endpoint"""
    return {
        "status": "healthy",
        "timestamp": datetime.utcnow().isoformat(),
        "version": settings.VERSION
    }

# Include API routers (protected with API key)
app.include_router(devices.router, prefix="/api", tags=["Devices"], dependencies=[Depends(verify_api_key)])
app.include_router(commands.router, prefix="/api", tags=["Commands"], dependencies=[Depends(verify_api_key)])
app.include_router(telemetry.router, prefix="/api", tags=["Telemetry"], dependencies=[Depends(verify_api_key)])
app.include_router(system.router, prefix="/api", tags=["System"], dependencies=[Depends(verify_api_key)])


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=settings.DEBUG
    )
