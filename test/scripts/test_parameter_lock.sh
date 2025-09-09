#!/bin/bash

# Test script for parameter lock functionality
echo "Testing parameter lock functionality..."

# Start simulation in background
./build-simulation/trellis_simulation &
SIM_PID=$!

# Wait for startup
sleep 2

# Test sequence: Hold uppercase 'A', wait 600ms, then press lowercase 'b'
# Using expect to send keystrokes
expect << EOF
set timeout 10
spawn ./build-simulation/trellis_simulation

# Wait for initialization
sleep 2

# Send uppercase A (should start hold)
send "A"

# Wait longer than hold threshold (500ms)
sleep 1

# Send lowercase a (should release and trigger parameter lock)
send "a"

# Wait a moment
sleep 0.5

# Send lowercase b (should trigger parameter adjustment)
send "b"

# Wait to see results
sleep 2

# Exit simulation
send "\033"
expect eof
EOF

# Clean up
kill $SIM_PID 2>/dev/null

echo "Test complete"