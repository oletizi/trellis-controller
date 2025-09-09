#!/bin/bash

# Test parameter lock timing
echo "Testing parameter lock with proper timing..."

# Create a named pipe for controlled input
PIPE=$(mktemp -u)
mkfifo "$PIPE"

# Start simulation with the pipe as input
./build-simulation/trellis_simulation < "$PIPE" > test_timing.log 2>&1 &
SIM_PID=$!

# Function to send keys with timing
send_key_sequence() {
    exec 3>"$PIPE"
    
    echo "Sending uppercase A..."
    echo -n "A" >&3
    
    echo "Waiting 700ms..."
    sleep 0.7
    
    echo "Sending lowercase a..."
    echo -n "a" >&3
    
    sleep 0.2
    
    echo "Sending lowercase b..."
    echo -n "b" >&3
    
    sleep 0.5
    
    echo "Sending ESC to quit..."
    printf "\033" >&3
    
    exec 3>&-
}

# Send the key sequence
send_key_sequence &
SENDER_PID=$!

# Wait for completion
wait $SENDER_PID
wait $SIM_PID

# Clean up
rm -f "$PIPE"

echo "Test complete. Check test_timing.log for results."