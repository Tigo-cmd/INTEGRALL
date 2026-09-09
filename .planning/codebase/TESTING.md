# Testing Patterns

**Analysis Date:** 2026-09-09

## Test Framework

**Runner:**
- Not configured - No Jest, Vitest, or similar test framework detected
- TypeScript compilation available via `tsc --noEmit -p tsconfig.app.json`

**Assertion Library:**
- Not detected

**Run Commands:**
```bash
npm run typecheck          # Type checking without emit (weather_station and website/IntegrallV1)
npm run lint              # Linting with ESLint
npm run build             # Build for production
npm run dev               # Development server
```

## Test File Organization

**Location:**
- Not applicable - No test files detected
- Search for `*.test.ts`, `*.test.tsx`, `*.spec.ts`, `*.spec.tsx` yields no results

**Naming:**
- Follows conventional naming: would be `[Component].test.tsx` or `[Module].spec.ts` if tests existed

**Structure:**
- Not applicable - No test infrastructure present

## Test Structure

**Current Approach:**
The codebase uses comparison-style test files that demonstrate old patterns vs. new patterns:

**Example files (non-traditional testing):**
- `test_relay.cpp` - Shows ❌ old unsafe relay control vs. ✅ Integrall framework approach
- `test_buzzer.cpp` - Shows ❌ blocking audio patterns vs. ✅ non-blocking Integrall patterns
- `test.py` - Motion detection demonstration script (integrates with ESP32 camera)

**Pattern example from `test_relay.cpp`:**
```cpp
// ❌ THE OLD WAY (Dangerous & Complex)
int pumpPin = 4;
bool pumpRunning = false;
// ... manual state management ...

// ✅ THE INTEGRALL WAY (Industrial Safety, 0 Boilerplate)
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  integrall.begin();
  integrall.relay.addRelay(0, 4, false);
  integrall.relay.setSafetyTimeout(0, 10000);
  integrall.relay.attachTrigger(0, 12, true);
}
```

## Mocking

**Framework:** Not configured

**Current Patterns:**
- No mocking framework detected
- C++ firmware tests show full integration approach (no mocking)
- Python test uses real stream from actual ESP32 device

## Fixtures and Factories

**Test Data:**
- No dedicated test fixtures found
- Firmware framework provides example sketches comparing patterns

**Location:**
- Top-level test files: `test_relay.cpp`, `test_buzzer.cpp`, `test_lcd.cpp`, `test_edge_cases.cpp`
- Python script: `test.py`, `weather_backend.py`
- Demonstration files use inline data

## Coverage

**Requirements:** Not enforced - No coverage configuration detected

**View Coverage:** Not applicable

## Type Checking

**TypeScript Type Checking:**
```bash
npm run typecheck
```

**What it checks:**
- All `*.ts` and `*.tsx` files in `src/` directory
- Strict type checking enabled globally
- Catches unused variables and parameters
- Detects switch fallthrough cases

**Configuration:**
- `tsconfig.app.json` - Main configuration for app code
- `tsconfig.node.json` - Configuration for Vite/build tooling
- Strict mode enabled for all projects

## Test Types

**Unit Tests:**
- Not implemented in current codebase
- Recommendation: Add with Vitest for consistency with Vite build tool

**Integration Tests:**
- Firmware testing via demonstration files (`test_relay.cpp`, `test_buzzer.cpp`)
- Backend testing via Python scripts (`test.py` for motion detection, `weather_backend.py` for weather system)

**E2E Tests:**
- Not implemented
- Would require test runner like Playwright or Cypress

**Manual Testing Patterns:**
The codebase demonstrates a before/after approach:
- `test_relay.cpp` - Manual verification that framework handles relay safety
- `test_buzzer.cpp` - Manual verification that non-blocking patterns work
- `test.py` - OpenCV motion detection on live ESP32 camera stream
- `simulate_esp32.py` - Python script that simulates ESP32 responses for testing

## Quality Assurance

**Linting:**
- ESLint enforces code quality
- Rules configured in `eslint.config.js` for both `weather_station` and `website/IntegrallV1`
- Run: `npm run lint`

**Type Safety:**
- TypeScript strict mode enabled
- Type checking via `npm run typecheck`
- No implicit any allowed
- Unused variables/parameters cause errors

**Build Validation:**
- TypeScript compilation during build: `tsc -b` in `website/IntegrallV1`
- Vite build process catches syntax errors
- Run: `npm run build`

## Common Patterns

**Async Testing (if implemented):**
- Would use React Testing Library for component testing
- Example pattern (not currently used):
```typescript
// Hypothetical pattern based on current async code structure
const fetchWeatherData = async () => {
  try {
    const response = await fetch('http://localhost:5000/api/live');
    const result = await response.json();
    // Process data...
  } catch (error) {
    console.error('Failed to fetch:', error);
  }
};
```

**Error Testing:**
- Currently no error testing framework
- Code handles errors with try-catch and fallback states
- Example from `src/App.tsx`:
```typescript
try {
  const response = await fetch('http://localhost:5000/api/live');
  // Process response
} catch (error) {
  console.error('Failed to fetch:', error);
  setSystemStatus((prev) => ({ ...prev, online: false }));
}
```

## Testing Roadmap

**Currently Missing:**
1. Unit test framework (Jest or Vitest)
2. Component testing library (React Testing Library)
3. Test coverage tracking
4. E2E testing framework
5. Automated test CI/CD pipeline

**Recommended First Steps:**
1. Add Vitest to `package.json` (pairs well with Vite)
2. Add React Testing Library for component tests
3. Add test files in co-located `__tests__/` or `.test.tsx` pattern
4. Configure coverage reporting
5. Add test script to `package.json`

---

*Testing analysis: 2026-09-09*
