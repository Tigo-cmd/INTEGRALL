# Codebase Concerns

**Analysis Date:** 2026-09-09

## Tech Debt

**In-Memory Data Storage Without Persistence:**
- Issue: Both `weather_backend.py` and `IOT-Smart-Waste-Sorting/serverside/app.py` use in-memory Python lists/dicts for all data storage
- Files: 
  - `weather_backend.py` (lines 11-12, 28-32)
  - `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 30-45, 158, 162)
- Impact: All data is lost on application restart; no historical data persistence; scaling to multiple devices impossible
- Fix approach: Migrate to SQLite for local development or PostgreSQL/TimescaleDB for production; implement data migration scripts

**Hardcoded Configuration Values:**
- Issue: Backend URLs, ports, IP addresses hardcoded in source code instead of environment variables
- Files:
  - `weather_station/src/App.tsx` (line 33): `'http://localhost:5000/api/live'`
  - `simulate_esp32.py` (line 6): `"http://localhost:5000/telemetry"`
  - `IOT-Smart-Waste-Sorting/serverside/test_inference.py` (line 4): `"http://localhost:5000/api/detect"`
  - `test.py` (line 7): `"192.168.1.168"`
- Impact: Cannot deploy to different environments without code changes; secrets risk if credentials ever added
- Fix approach: Use `.env` files with environment variable loading across all frontend/backend projects; update fetch URLs to use config objects

**Monolithic Firmware Header File:**
- Issue: `firmware/Integrall/src/Integrall.h` is 1707 lines with all module includes and core logic in one header
- Files: `firmware/Integrall/src/Integrall.h`
- Impact: Difficult to maintain, long compilation times, circular dependency risk, hard to reason about module dependencies
- Fix approach: Split into smaller headers per module; use forward declarations; move implementation to .cpp files

**Disabled MJPEG Server Due to Memory Constraints:**
- Issue: Camera MJPEG server is commented out to save RAM
- Files: `firmware/Integrall/src/modules/CameraModule.h` (lines 80-87)
- Impact: Camera module cannot stream video in real-time; feature essentially incomplete; indicates resource exhaustion
- Fix approach: Profile memory usage; consider frame buffering strategy or reducing resolution; document RAM requirements

## Known Bugs

**Database Method Call Error (Critical):**
- Symptoms: Application will crash when trying to save detection or alert records
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 158, 162)
- Trigger: After any waste detection via POST to `/api/detect`
- Issue: Code calls `db["detections"].insert(0, new_detection)` but Python lists don't have `.insert()` with prepending semantic; should be `.append()` or use `deque`
- Workaround: Manually prepend to list or use reverse order retrieval

**Groq API Model Name Invalid:**
- Symptoms: Vision analysis will fail with model not found error
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (line 61)
- Issue: Model string `"meta-llama/llama-4-scout-17b-16e-instruct"` doesn't match official Groq model IDs; should verify against Groq API docs
- Current: Likely defaults to wrong model or fails with 404

**Flask Debug Mode in Production:**
- Symptoms: Detailed error pages expose code, debugger enabled, auto-reload active
- Files: 
  - `weather_backend.py` (line 55)
  - `IOT-Smart-Waste-Sorting/serverside/app.py` (line 225)
- Trigger: Always active when running `app.run(..., debug=True)`
- Impact: Security vulnerability in any production deployment

**Weather Station Rate of Change Division by Zero:**
- Symptoms: Can produce Infinity or NaN values in UI
- Files: `weather_station/src/App.tsx` (lines 24-29)
- Trigger: When `oneHourAgo === 0`, division produces Infinity; no type coercion or validation
- Workaround: Check for zero before division

**Missing .insert() Method on Python List:**
- Symptoms: TypeError at runtime on first detection save
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (line 158)
- Issue: Should be `.append()` to add to end, or use `collections.deque` with `.appendleft()` for prepending
- Impact: Waste sorting API will crash on first image upload

## Security Considerations

**Missing API Authentication:**
- Risk: Anyone with access to network can send arbitrary sensor data or trigger waste detection
- Files:
  - `weather_backend.py` (all endpoints)
  - `IOT-Smart-Waste-Sorting/serverside/app.py` (all endpoints)
- Current mitigation: None
- Recommendations: 
  - Add Bearer token validation to all routes
  - Use API key header validation
  - Implement device registration with unique tokens
  - Add rate limiting per device

**CORS Enabled for All Origins:**
- Risk: Frontend can be compromised on any domain and make requests to backend
- Files:
  - `weather_backend.py` (line 7): `CORS(app)`
  - `IOT-Smart-Waste-Sorting/serverside/app.py` (line 15): `CORS(app)`
- Current mitigation: Implicit allow-all
- Recommendations: 
  - Restrict to specific origin: `CORS(app, origins=["http://localhost:3000"])`
  - Use environment variables for allowed origins
  - Validate Origin header

**No Input Validation or Sanitization:**
- Risk: Malformed data can crash backend or cause unexpected behavior
- Files:
  - `weather_backend.py` (lines 20-21): Only checks if data exists, no schema validation
  - `IOT-Smart-Waste-Sorting/serverside/app.py` (line 120): device_id taken from request without validation
- Current mitigation: None
- Recommendations:
  - Use Pydantic (Python) or Zod (TypeScript) for schema validation
  - Validate file types in uploads
  - Sanitize string fields

**File Upload Without Type Validation:**
- Risk: ESP32 could upload non-JPEG files; potential for code injection or resource exhaustion
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 124-133)
- Current mitigation: None
- Recommendations:
  - Check MIME type against whitelist
  - Validate file signatures (magic bytes)
  - Add max file size limit
  - Use virus scanner on uploaded files

**Missing Environment Variable Protection:**
- Risk: API keys visible in `.env` files; `.env` file can be accidentally committed
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (line 24): GROQ_API_KEY loaded unsafely
- Current mitigation: None
- Recommendations:
  - Add `.env` to `.gitignore` (verify it's there)
  - Use environment-specific configs
  - Rotate API keys regularly
  - Use secrets management system (AWS Secrets Manager, HashiCorp Vault)

## Performance Bottlenecks

**Unbounded In-Memory Data Growth:**
- Problem: `weather_station/src/App.tsx` stores unlimited telemetry logs (MAX_LOGS=100) but continuously appends to arrays
- Files: `weather_station/src/App.tsx` (lines 69)
- Cause: No memory management; arrays grow indefinitely if polling runs for days
- Improvement path: 
  - Use circular buffer or sliding window (already coded as slice(-MAX_LOGS) but redundant allocation)
  - Implement IndexedDB/localStorage for browser persistence
  - Clear old logs on client restart

**Polling Instead of WebSocket:**
- Problem: Client polls every 4 seconds for updates (POLLING_INTERVAL=4000)
- Files: `weather_station/src/App.tsx` (line 8)
- Cause: Wasted bandwidth on 4-second HTTP requests; creates latency for real-time updates
- Improvement path: 
  - Implement WebSocket or Server-Sent Events (SSE)
  - Reduce polling to 10-30 seconds for non-critical data
  - Add exponential backoff on repeated failures

**Inefficient Rate of Change Calculation:**
- Problem: Recalculates all historical data on every render
- Files: `weather_station/src/App.tsx` (lines 95-99)
- Cause: No memoization; called on every state update
- Improvement path: Use `useMemo()` hook to cache calculations; precalculate on data arrival

## Fragile Areas

**Weather Station Frontend Network Resilience:**
- Files: `weather_station/src/App.tsx` (lines 31-85)
- Why fragile: 
  - No error boundary component
  - Fetch errors silently logged to console
  - No retry logic on connection failure
  - Silent online/offline state toggle without user feedback
  - Assumes backend always returns expected JSON shape
- Safe modification: 
  - Add try-catch with error toast notifications
  - Implement exponential backoff retry logic
  - Validate response structure before using
  - Show loading and error states to user
- Test coverage gaps: No tests for network failure scenarios

**Groq Vision API Integration:**
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 51-102)
- Why fragile:
  - No timeout on API call
  - Assumes response can be parsed as JSON
  - Model name may be incorrect
  - No fallback if API key missing
  - Regex cleanup of markdown is fragile (line 96)
  - Zero confidence threshold not validated (line 141)
- Safe modification:
  - Add timeout parameter to completions call
  - Wrap json.loads() in try-except
  - Validate model name against Groq API list
  - Test with various response formats
  - Add integration tests with mock Groq responses
- Test coverage gaps: No tests for API failures; no tests for malformed responses

**ESP32 WiFi Reconnection Logic:**
- Files: `firmware/Integrall/src/core/DeviceManager.cpp` (lines 126-150)
- Why fragile:
  - Hard-coded WiFi timeout (config.wifi_connect_timeout_ms)
  - No exponential backoff between reconnect attempts
  - No handling of WiFi AP mode transition
  - State machine transitions not fully covered
- Safe modification:
  - Implement exponential backoff with jitter
  - Add state validation before transitions
  - Test all state transition paths
- Test coverage gaps: No unit tests; only firmware-level integration

**In-Memory Database Operations:**
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 186-192, 204-209)
- Why fragile:
  - Linear search through all records (O(n))
  - No indexing on device_id or category
  - Concurrent requests could cause race conditions
  - No transaction support
- Safe modification:
  - Add locking with threading.Lock()
  - Move to actual database with proper indexing
  - Implement proper query API
- Test coverage gaps: No concurrency tests

## Scaling Limits

**Single Device Assumption in Waste Sorting Backend:**
- Current capacity: Hardcoded device info for "dev-001" only
- Limit: Only one device can be managed
- Files: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 31-41, 154-156)
- Scaling path: 
  - Move device registry to database
  - Implement device provisioning API
  - Support multiple simultaneous uploads

**Fixed Buffer Sizes:**
- Current capacity: Weather station stores MAX_HISTORY=50 readings
- Limit: After 50 readings (~200 seconds at 4s intervals), oldest data lost
- Files: `weather_station/src/App.tsx` (line 9)
- Scaling path: Implement time-based retention instead of count-based

**ESP32 Memory Constraints:**
- Current capacity: Camera module disabled MJPEG server; frame buffer set to 1 with QVGA (320x240)
- Limit: Cannot stream video; low resolution for AI inference
- Files: `firmware/Integrall/src/modules/CameraModule.h` (lines 49-67)
- Scaling path: 
  - Use external PSRAM if available
  - Implement frame compression
  - Stream to backend for AI processing instead of on-device

## Dependencies at Risk

**React Version Mismatch:**
- Risk: Weather station and waste sorting use React 18.3.1, but IntegrallV1 uses 19.2.0
- Impact: Cannot share components; build complexity if merged into monorepo
- Migration plan: Upgrade all projects to React 19 consistently

**Pre-Release ESLint Plugin:**
- Risk: `eslint-plugin-react-hooks@^5.1.0-rc.0` is release candidate
- Impact: Could introduce breaking changes; not recommended for production
- Migration plan: Update to latest stable version when available

**Missing Type Definitions:**
- Risk: Python backends have no type hints; TypeScript projects don't validate Python types
- Impact: Runtime errors not caught at development time
- Migration plan: Add Pydantic models to all Python endpoints; generate OpenAPI schema

## Missing Critical Features

**No Database Schema:**
- Problem: No data persistence strategy documented or implemented
- Blocks: Long-term data analysis, multi-device support, data recovery
- Impact: All telemetry lost on restart

**No API Documentation:**
- Problem: No OpenAPI/Swagger specs for backend endpoints
- Blocks: Frontend/firmware developers don't know exact request/response format
- Impact: Client-server integration difficult; assumptions lead to bugs

**No Authentication/Authorization System:**
- Problem: No user accounts, device registration, or role-based access control
- Blocks: Multi-tenant deployments; security auditing
- Impact: Backend accessible by anyone with network access

**No Monitoring or Alerting:**
- Problem: No logging aggregation, error tracking, or health checks
- Blocks: Production debugging; detecting issues before users report them
- Impact: Silent failures; extended downtime without visibility

## Test Coverage Gaps

**Zero Backend Tests:**
- What's not tested: 
  - Flask route handlers for telemetry endpoints
  - Groq API integration and error handling
  - File upload validation and storage
  - Device/detection/alert CRUD operations
- Files: All Python files
- Risk: Any change breaks functionality without detection
- Priority: High

**Zero Frontend Integration Tests:**
- What's not tested:
  - Fetch to backend and error scenarios
  - Real-time data polling and state updates
  - Network disconnection handling
  - Data transformation and validation
- Files: `weather_station/src/App.tsx`, `IOT-Smart-Waste-Sorting/client/project/src/*`
- Risk: UI silently breaks on backend changes
- Priority: High

**No Firmware Unit Tests:**
- What's not tested:
  - WiFi reconnection state machine
  - Module initialization order
  - Relay safety interlocks
  - Memory management under load
- Files: All `.cpp` and `.h` firmware files
- Risk: Silent failures in production devices
- Priority: Medium

**No End-to-End Tests:**
- What's not tested:
  - Full flow from ESP32 → backend → frontend visualization
  - Multi-device scenarios
  - Network failure recovery
  - Concurrent uploads
- Risk: Integration issues only discovered in production
- Priority: Medium

---

*Concerns audit: 2026-09-09*
