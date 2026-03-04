
# Let's create the project structure for Integrall
# First, let's set up the directory structure

import os
import json

# Base project directory
base_dir = "./"

def main():
    # Create directory structure
    dirs = [
        "firmware/Integrall/src",
        "firmware/Integrall/src/core",
        "firmware/Integrall/src/modules",
        "firmware/Integrall/src/config",
        "firmware/Integrall/examples/BasicRelay",
        "firmware/Integrall/examples/KeypadLock",
        "firmware/Integrall/examples/FullDemo",
        "backend/app/api",
        "backend/app/models",
        "backend/app/services",
        "backend/app/core",
        "backend/migrations",
        "docs",
        "tests"
    ]

    for d in dirs:
        os.makedirs(f"{base_dir}/{d}", exist_ok=True)

    print("✅ Project directory structure created")
    print(f"Location: {base_dir}")

    # Create a project overview file
    overview = """# Integrall IoT Framework - Development Roadmap

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
"""

    with open(f"{base_dir}/ROADMAP.md", "w") as f:
        f.write(overview)

    print("✅ Roadmap created")

if __name__ == "__main__":
    main()
