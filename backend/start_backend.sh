#!/bin/bash
# start_backend.sh
# Quick start script for Integrall Backend

echo "=== Integrall Backend Startup ==="

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "Installing dependencies..."
pip install -q -r requirements.txt

# Check if .env exists
if [ ! -f ".env" ]; then
    echo "Creating .env from template..."
    cp .env.example .env
    echo "WARNING: Please edit .env and set your API keys before production use!"
fi

# Initialize database (if using SQLite, this happens automatically)
echo "Initializing database..."

# Start the server
echo "Starting FastAPI server..."
echo "API documentation will be available at:"
echo "  - Swagger UI: http://localhost:8000/docs"
echo "  - ReDoc: http://localhost:8000/redoc"
echo ""

uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
