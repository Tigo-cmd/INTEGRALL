from flask import Flask, request, jsonify
from flask_cors import CORS
import json
import time

app = Flask(__name__)
CORS(app)  # Enable CORS for the frontend

# In-memory storage for telemetry data
# In a production app, use a database like SQLite or TimescaleDB
telemetry_history = []
MAX_HISTORY = 50

@app.route('/telemetry', methods=['POST'])
def receive_telemetry():
    """
    Endpoint for the ESP32 to send sensor data.
    The Integrall framework sends data as JSON via POST.
    """
    data = request.get_json()
    if not data:
        return jsonify({"status": "error", "message": "No JSON data received"}), 400

    # Add server-side timestamp
    data['timestamp'] = time.time()
    
    # Store in history
    telemetry_history.append(data)
    
    # Keep only the last MAX_HISTORY readings
    if len(telemetry_history) > MAX_HISTORY:
        telemetry_history.pop(0)

    print(f"DEBUG: Received Telemetry - T: {data.get('temp_bme', 'N/A')} H: {data.get('hum_bme', 'N/A')} - Total Points: {len(telemetry_history)}")
    
    return jsonify({"status": "success", "received": data}), 200

@app.route('/api/live', methods=['GET'])
def get_live_data():
    """
    Endpoint for the Frontend to poll for current sensor data.
    """
    if not telemetry_history:
        return jsonify({"status": "waiting", "data": None}), 200
        
    return jsonify({
        "status": "online",
        "current": telemetry_history[-1],
        "history": telemetry_history
    }), 200

if __name__ == '__main__':
    # Run on all interfaces so the ESP32 can connect via IP
    # Make sure your computer firewall allows port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)
