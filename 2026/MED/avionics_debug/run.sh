# g++ -std=c++17 -I. decode_mux.cpp -o decode_mux
# g++ -std=c++17 -I. decode_simple.cpp -o decode_simple

# sudo ./decode_simple /dev/ttyUSB0 115200  
# sudo ./decode_mux /dev/ttyUSB0 115200

#!/usr/bin/env bash
#
# run_decode.sh – compile (if needed) and run one of the two decoders
#
#   Usage:
#     ./run_decode.sh simple 0 [115200]
#     ./run_decode.sh mux    1 [9600]
#
#   Positional args
#     1. MODE    – “simple” or “mux”
#     2. USBIDX  – number after /dev/ttyUSB (e.g. 0 → /dev/ttyUSB0)
#   Optional arg
#     3. BAUD    – defaults to 115200 if omitted
#
set -euo pipefail

if (( $# < 2 || $# > 3 )); then
  echo "Usage: $0 <simple|mux> <USB index> [baudrate]" >&2
  exit 1
fi

MODE="$1"                # simple | mux
USBIDX="$2"              # e.g. 0 → /dev/ttyUSB0
BAUDRATE="${3:-115200}"  # default if not supplied

case "$MODE" in
  simple) SRC=decode_simple.cpp ; BIN=decode_simple ;;
  mux)    SRC=decode_mux.cpp    ; BIN=decode_mux    ;;
  *) echo "Error: first arg must be 'simple' or 'mux'" >&2; exit 1 ;;
esac

# Re-compile only if binary is missing or source is newer
if [[ ! -x "./$BIN" || "./$BIN" -ot "$SRC" ]]; then
  echo "Compiling $SRC → $BIN …"
  g++ -std=c++17 -I. "$SRC" -o "$BIN"
fi

DEV="/dev/ttyUSB${USBIDX}"
if [[ ! -e "$DEV" ]]; then
  echo "Error: device $DEV not found" >&2
  exit 1
fi

echo "Running $BIN on $DEV @ $BAUDRATE baud …"
sudo "./$BIN" "$DEV" "$BAUDRATE"
