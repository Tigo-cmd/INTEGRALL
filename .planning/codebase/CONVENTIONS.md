# Coding Conventions

**Analysis Date:** 2026-09-09

## Naming Patterns

**Files:**
- React components: PascalCase (e.g., `SystemHealthHeader.tsx`, `SensorMatrix.tsx`) - located in `src/components/`
- Utilities: camelCase (e.g., `utils.ts`) - located in `src/lib/`
- Types: camelCase (e.g., `types.ts`) - located in `src/`
- CSS/styling: lowercase with hyphens (e.g., `index.css`, `progressive-blur.tsx` for component files)
- Data/config fields: snake_case in interfaces (e.g., `temp_bme`, `hum_dht`, `light_level`, `rain_level`) - see `src/types.ts`
- C++ firmware files: PascalCase for modules (e.g., `CameraModule.h`, `RelayModule.h`) - located in `firmware/Integrall/src/modules/`

**Functions:**
- camelCase for all function names (both TS/JS and C++)
- React component functions: PascalCase (e.g., `App()`, `SystemHealthHeader()`) - they are component exports
- Helper functions: camelCase (e.g., `calculateRateOfChange()`, `formatUptime()`, `formatTime()`)
- Example: `src/App.tsx` uses `fetchWeatherData()`, `calculateRateOfChange()`

**Variables:**
- Local variables: camelCase (e.g., `historicalData`, `systemStatus`, `telemetryLogs`, `startTime`)
- Constants: UPPER_SNAKE_CASE (e.g., `POLLING_INTERVAL`, `MAX_HISTORY`, `MAX_LOGS`)
- State variables (React hooks): camelCase (e.g., `const [systemStatus, setSystemStatus]`)
- Data field names: snake_case (e.g., `temp_bme`, `temp_dht`, `hum_bme`, `hum_dht`, `light_level`, `rain_level`, `dew_point`, `heat_index`)

**Types:**
- Interface names: PascalCase (e.g., `WeatherData`, `SystemStatus`, `SensorMetrics`, `TelemetryLog`, `ButtonProps`)
- Example from `src/types.ts`: `interface WeatherData`, `interface SystemStatus`, `interface SensorMetrics`
- Component props interfaces: `[ComponentName]Props` (e.g., `SystemHealthHeaderProps`)

## Code Style

**Formatting:**
- ESLint with TypeScript support (no Prettier config detected)
- 2-space indentation (inferred from source files)
- Semicolons used consistently
- Files in `weather_station` and `website/IntegrallV1` both use same ESLint setup

**Linting:**
- Framework: ESLint with `@eslint/js` and `typescript-eslint`
- Config files:
  - `weather_station/eslint.config.js`
  - `website/IntegrallV1/eslint.config.js`
- Key rules:
  - React hooks rules enabled (`eslint-plugin-react-hooks`)
  - React refresh rules enabled (`eslint-plugin-react-refresh`) - warns when exporting non-component values
  - TypeScript strict mode enabled
- Run command: `npm run lint`

**TypeScript Compiler Settings:**
- `strict: true` - All strict type checking enabled
- `noUnusedLocals: true` - Error on unused variables
- `noUnusedParameters: true` - Error on unused function parameters
- `noFallthroughCasesInSwitch: true` - Error on switch fallthrough
- `target: ES2020` - Target modern JavaScript
- `moduleResolution: bundler` - Use bundler module resolution
- Config location: `tsconfig.app.json` in both projects

## Import Organization

**Order:**
1. React and external npm packages (e.g., `import { useEffect, useState } from 'react'`)
2. Third-party components/libraries (e.g., `import { LineChart, Line, ... } from 'recharts'`)
3. Local components (e.g., `import SystemHealthHeader from './components/SystemHealthHeader'`)
4. Local utilities and types (e.g., `import { WeatherData, SystemStatus } from './types'`)
5. Styles (e.g., `import './index.css'`)

**Example from `src/App.tsx`:**
```typescript
import { useEffect, useState } from 'react';
import SystemHealthHeader from './components/SystemHealthHeader';
import SensorMatrix from './components/SensorMatrix';
import Charts from './components/Charts';
import TelemetryStream from './components/TelemetryStream';
import { WeatherData, SystemStatus, TelemetryLog } from './types';
```

**Path Aliases:**
- `@` alias points to `src/` directory (configured in `tsconfig.app.json`)
- Used in `website/IntegrallV1` for imports like `import { cn } from "@/lib/utils"`

## Error Handling

**Patterns:**
- Try-catch blocks used for async operations (e.g., `fetchWeatherData()` in `src/App.tsx`)
- Fallback to safe defaults on error
- Console.error() used for error logging: `console.error('Failed to fetch:', error)`
- No specific error classes; uses generic Error objects
- Example: When fetch fails, sets `online: false` as fallback state

**C++ firmware patterns (in `firmware/Integrall/src/`)**:
- Uses Logger macros: `INTEGRALL_LOG_INFO()`, `INTEGRALL_LOG_ERROR()`, `INTEGRALL_LOG_WARNING()`
- Return early on initialization failures (e.g., `if (!_initialized) return nullptr`)
- Boolean return values for status checks (e.g., `bool begin()`, `bool isReady()`)

## Logging

**Framework:** `console` object (browser) and custom Logger macros (firmware)

**Patterns:**
- Web: `console.error()` for errors, implicit console output for status
- Firmware: Macro-based logging through `INTEGRALL_LOG_*` family
- No structured logging framework detected

**Examples:**
- `console.error('Failed to fetch:', error)` in `src/App.tsx`
- `INTEGRALL_LOG_INFO("Camera: Initializing hardware...")` in firmware

## Comments

**When to Comment:**
- Explain non-obvious logic (e.g., rate-of-change calculation logic)
- Document configuration sections (e.g., `// 🏷️ 1. Configurations` in `test.py`)
- Mark deprecated patterns (e.g., `// ❌ THE OLD WAY` vs `// ✅ THE INTEGRALL WAY` in test files)
- C++ firmware uses extensive header comments for module documentation

**JSDoc/TSDoc:**
- Minimal usage detected
- C++ firmware uses block comments for method documentation (see `CameraModule.h`)
- Example: Multi-line comments for method purposes and parameter descriptions

## Function Design

**Size:** Functions kept relatively compact, typically 20-50 lines for data fetching/processing

**Parameters:**
- Destructuring used for component props (e.g., `({ status }: SystemHealthHeaderProps)`)
- No default parameters extensively used
- Pass minimal data to functions

**Return Values:**
- React components return JSX
- Utility functions return typed values (e.g., `number`, `string`, `JSX.Element`)
- Async functions return Promises
- C++ methods return `bool` for success/failure, pointers for resource allocation

## Module Design

**Exports:**
- Components: Default exports (e.g., `export default function SystemHealthHeader()`)
- Utilities and helpers: Named exports (e.g., `export function cn(...)`)
- Types: Named exports from `types.ts`
- Example from `src/components/ui/button.tsx`: `export { Button, buttonVariants }`

**Barrel Files:**
- Not extensively used; imports typically reference specific files
- Type exports centralized in `types.ts` files

**File Organization:**
- Components in `src/components/` (organized by feature in website project)
- UI components in `src/components/ui/` for shared components
- Layout components in `src/components/layout/`
- Utilities in `src/lib/`
- Type definitions in `src/types.ts`
- Entry point: `src/main.tsx`

**Namespace Usage:**
- C++ firmware uses `namespace Integrall` to avoid naming collisions (e.g., `class Integrall::CameraModule`)

---

*Convention analysis: 2026-09-09*
