# Integrall IoT Framework - Development Roadmap

## Current Phase: Foundation (Week 1-2)
- [x] Project structure
- [ ] Core configuration system (compile-time flags)
- [ ] DeviceManager (WiFi, backend registration)
- [ ] HTTP Client with retry logic
- [ ] Logger utility
- [ ] First module: RelayModule

## Next Phases:
- Phase 2: Input modules (Keypad) + Python backend API
- Phase 3: Display modules (OLED/LCD)
- Phase 4: Polish, examples, documentation

## Architecture Decisions Made:
1. PlatformIO primary, Arduino IDE compatible
2. Header-only configuration pattern (user defines flags in sketch)
3. Non-blocking event loop design
4. REST API backend (FastAPI) with device polling
