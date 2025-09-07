#!/bin/bash
# Test script to verify parameter lock hold detection is working

echo "Testing parameter lock hold detection..."
echo "Starting simulation - will test the hold system..."

cd build-simulation

# Run simulation with test input
echo "Testing hold sequence:"
echo "1. Press 'Q' (UPPERCASE) to PRESS&HOLD step button (1,0)"
echo "2. Wait 500ms for parameter lock to activate"
echo "3. Press 'q' (lowercase) to RELEASE step button"

# Start simulation and send test sequence
(
    sleep 1
    echo "Q" # Press button (1,0)
    sleep 0.6  # Wait 600ms (longer than 500ms threshold)
    echo "q" # Release button (1,0)
    sleep 1
    echo -e "\x1b" # ESC to quit
) | timeout 5 ./trellis_simulation 2>param_test_debug.log

echo "Simulation completed. Check param_test_debug.log for debug output:"
if [ -f param_test_debug.log ]; then
    cat param_test_debug.log
else
    echo "No debug log found"
fi