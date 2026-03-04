@echo off
REM start_backend.bat
REM Quick start script for Integrall Backend (Windows)

echo === Integrall Backend Startup ===

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python is not installed
    exit /b 1
)

REM Check if virtual environment exists
if not exist "venv" (
    echo Creating virtual environment...
    python -m venv venv
)

REM Activate virtual environment
echo Activating virtual environment...
call venv\Scripts\activate.bat

REM Install dependencies
echo Installing dependencies...
pip install -q -r requirements.txt

REM Check if .env exists
if not exist ".env" (
    echo Creating .env from template...
    copy .env.example .env
    echo WARNING: Please edit .env and set your API keys before production use!
)

REM Start the server
echo Starting FastAPI server...
echo API documentation will be available at:
echo   - Swagger UI: http://localhost:8000/docs
echo   - ReDoc: http://localhost:8000/redoc
echo.

uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
