#!/usr/bin/env bash
# Compile every Integrall example + CI sketch against every compatible board.
#
# Usage:
#   firmware/ci/build_all.sh                 # full matrix
#   firmware/ci/build_all.sh BasicRelay      # only sketches whose name matches
#   ENVS="esp32dev" firmware/ci/build_all.sh # restrict boards
#
# Requires PlatformIO (`pio`). Set PIO=/path/to/pio if it is not on PATH.
# Exit code is non-zero if any build fails. Logs land in firmware/ci/logs/.
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB="${LIB:-$HERE/../Integrall}"
PIO="${PIO:-$(command -v pio || echo "$HOME/.platformio-venv/bin/pio")}"
FILTER="${1:-}"
LOGS="${LOGS:-$HERE/logs}"
WORK="${WORK:-$HERE/.work}"
mkdir -p "$LOGS" "$WORK"
export INTEGRALL_CI_SKETCHES="$HERE/sketches"

# Which boards each sketch must build on. Default: esp32dev + nodemcuv2.
boards_for() {
  case "$1" in
    SmartCamera|CameraOnly)                   echo "esp32cam" ;;
    SmartFallDevice)                          echo "esp32dev" ;;   # BLE/IMU/cellular combo, ESP32-only
    AllModulesESP32)                          echo "esp32dev esp32dev_core3" ;;
    AllModulesESP8266)                        echo "nodemcuv2" ;;
    AllModulesAVR|HardwareCheckGroupB)        echo "uno" ;;
    ArduinoOfflineTest|HardwareCheck|BasicRelay|BlinkNonBlocking|SerialOnlyTest|KeypadLock|ServoKeypadControl|ServoMultiControl|ParkingSensor|AlarmSystem|SmartSwitch|WeatherStation|DistanceAlerts)
                                              echo "esp32dev esp32dev_core3 nodemcuv2 uno" ;;
    *)                                        echo "esp32dev esp32dev_core3 nodemcuv2" ;;
  esac
}

sketches=()
for d in "$LIB"/examples/*/ "$HERE"/sketches/*/; do
  [ -d "$d" ] || continue
  sketches+=("${d%/}")
done

pass=0; fail=0; failed=()
for dir in "${sketches[@]}"; do
  name="$(basename "$dir")"
  [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]] && continue
  for env in $(boards_for "$name"); do
    [ -n "${ENVS:-}" ] && [[ " $ENVS " != *" $env "* ]] && continue
    log="$LOGS/${name}__${env}.log"
    bdir="$WORK/$env"
    printf '%-28s %-10s ' "$name" "$env"
    # pio ci --keep-build-dir does NOT refresh its copy of --lib / src, so
    # stale library code would be compiled. Remove the copies every time
    # (toolchain + framework objects stay cached).
    rm -rf "$bdir/lib" "$bdir/src"
    # Every file in the sketch folder (e.g. Dashboard.h) must travel with it.
    if "$PIO" ci "$dir" --lib "$LIB" --project-conf "$HERE/platformio.ini" \
         -e "$env" --build-dir "$bdir" --keep-build-dir >"$log" 2>&1; then
      warn=$(grep -c 'Integrall/src.*warning:' "$log" || true)
      ram=$(grep -oE 'RAM: .*\(used' "$log" | grep -oE '[0-9.]+%' | tr -d '%' | head -1)
      flash=$(grep -oE 'Flash: .*\(used' "$log" | grep -oE '[0-9.]+%' | tr -d '%' | head -1)
      # PlatformIO reports SUCCESS even when static RAM exceeds the chip. Treat
      # >100 % as a failure and >85 % on AVR as a warning (stack needs room).
      if [ -n "$ram" ] && awk "BEGIN{exit !($ram > 100)}"; then
        echo "FAIL (RAM ${ram}% > 100%) -> $log"; fail=$((fail+1)); failed+=("$name/$env (RAM ${ram}%)"); continue
      fi
      note=""
      [ "$env" = "uno" ] && [ -n "$ram" ] && awk "BEGIN{exit !($ram > 85)}" && note="  <-- RAM tight"
      echo "OK   (lib warnings: $warn, RAM ${ram:-?}%, flash ${flash:-?}%)$note"; pass=$((pass+1))
    else
      echo "FAIL -> $log"; fail=$((fail+1)); failed+=("$name/$env")
    fi
  done
done

echo
echo "passed: $pass  failed: $fail"
for f in "${failed[@]:-}"; do [ -n "$f" ] && echo "  FAILED: $f"; done
[ "$fail" -eq 0 ]
