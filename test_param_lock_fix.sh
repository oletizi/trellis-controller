#!/bin/bash
# Test script to verify parameter lock bug fixes

echo "=== Testing Parameter Lock Bug Fixes ==="
echo ""
echo "This test verifies:"
echo "1. Parameter lock mode doesn't exit when control buttons are released"  
echo "2. Steps don't toggle when released after parameter lock usage"
echo "3. Note increment/decrement controls work properly"
echo ""

# Build the simulator  
echo "Building simulator..."
make simulation-build || exit 1

echo ""
echo "=== Test Instructions ==="
echo ""
echo "1. Start the simulator and activate a step (e.g., press '1')"
echo "2. Hold the step key for ~0.5 seconds to enter parameter lock mode"
echo "3. Press and release control keys (like '5' for note up, '2' for note down)"
echo "4. Verify parameter lock mode persists after control key releases"
echo "5. Release the held step key to exit parameter lock mode"
echo "6. Verify the step remains active (doesn't toggle off)"
echo ""
echo "The fixes should make these behaviors work correctly."
echo "Press Enter to start the simulator..."
read

# Run the simulator
echo "Starting simulator..."
./build-simulation/trellis_simulation

echo ""
echo "Test complete!"