#!/usr/bin/env bash
# Build the EFMesh ambassador firmware for Heltec V3 / V4.
#
# What this does:
#   1. Clones the upstream Meshtastic firmware ambassador branch
#      (ascendr/firmware @ ambassador).
#   2. Overlays the EFMesh customizations from src/ on top of it
#      (canned message + owner short/long name).
#   3. Builds the .uf2 / .bin via PlatformIO for the requested board.
#
# Requirements:
#   - git
#   - Python 3.10+
#   - PlatformIO Core (pip install platformio)
#
# Usage:
#   scripts/build.sh heltec-v3
#   scripts/build.sh heltec-v4
#
# Output:
#   ./build-out/<board>/firmware.bin

set -euo pipefail

BOARD="${1:-heltec-v3}"
case "$BOARD" in
    heltec-v3) PIO_ENV="heltec-v3" ;;
    heltec-v4) PIO_ENV="heltec-v4" ;;
    *) echo "Unsupported board: $BOARD. Use heltec-v3 or heltec-v4." >&2 ; exit 1 ;;
esac

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$REPO_ROOT/.build-workdir"
OUT="$REPO_ROOT/build-out/$BOARD"

mkdir -p "$OUT"
rm -rf "$WORK"
mkdir -p "$WORK"

echo "==> Cloning upstream Meshtastic firmware (ambassador branch)..."
git clone --depth 1 --branch ambassador https://github.com/ascendr/firmware.git "$WORK/firmware"

echo "==> Overlaying EFMesh customizations..."
# 1. CannedMessageModule.cpp default message
python3 - <<PY
import re, pathlib
p = pathlib.Path("$WORK/firmware/src/modules/CannedMessageModule.cpp")
text = p.read_text()
old = '"I\\'d like to invite you to join the Arizona Meshtastic Community (azmsh.net) over on MediumFast Slot 18"'
new = '"Hello! I\\'d like to invite you to join the Electric Forest Meshtastic Community (efmesh.com) over on MediumFast preset."'
if old not in text:
    raise SystemExit("Upstream string not found — cannot apply EFMesh canned message patch.")
p.write_text(text.replace(old, new))
print("  - patched CannedMessageModule.cpp")
PY

# 2. userPrefs.jsonc owner identity
cp "$REPO_ROOT/src/userPrefs.jsonc" "$WORK/firmware/userPrefs.jsonc"
echo "  - replaced userPrefs.jsonc"

# 3. AmbassadorModule.cpp / .h are already in the upstream ambassador branch.
#    We keep our copy in src/modules/ as a stable reference, but use upstream's
#    in-place files for the build (they are identical).

echo "==> Building with PlatformIO (env: $PIO_ENV)..."
cd "$WORK/firmware"
pio run -e "$PIO_ENV"

BIN_PATH="$WORK/firmware/.pio/build/$PIO_ENV/firmware.bin"
if [[ ! -f "$BIN_PATH" ]]; then
    echo "Build did not produce firmware.bin at $BIN_PATH" >&2
    ls "$WORK/firmware/.pio/build/$PIO_ENV/" >&2 || true
    exit 1
fi

cp "$BIN_PATH" "$OUT/firmware.bin"
echo "==> Done. Output: $OUT/firmware.bin"
