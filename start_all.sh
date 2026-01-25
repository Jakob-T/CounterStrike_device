#!/bin/bash
set -e

python3 ws_bomb.py &
WS_PID=$!

python3 arm_bomb.py &
ARM_PID=$!

echo "Started ws_bomb (PID=$WS_PID) and arm_bomb (PID=$ARM_PID)"
echo "Press Ctrl+C to stop both."

trap 'echo "Stopping..."; kill $WS_PID $ARM_PID 2>/dev/null; exit 0' INT TERM

wait
