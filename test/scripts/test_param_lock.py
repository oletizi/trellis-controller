#!/usr/bin/env python3

import subprocess
import time
import os
import signal
import sys

def test_parameter_lock():
    print("Testing parameter lock functionality...")
    
    # Start simulation process
    proc = subprocess.Popen(
        ['./build-simulation/trellis_simulation'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        cwd='/Users/orion/work/trellis-controller'
    )
    
    try:
        # Wait for initialization
        time.sleep(1)
        
        print("Step 1: Sending uppercase 'A' to start hold...")
        proc.stdin.write('A')
        proc.stdin.flush()
        
        # Wait longer than hold threshold (500ms)
        print("Step 2: Waiting 700ms (longer than 500ms threshold)...")
        time.sleep(0.7)
        
        print("Step 3: Sending lowercase 'a' to release and trigger parameter lock...")
        proc.stdin.write('a')
        proc.stdin.flush()
        
        # Wait a moment
        time.sleep(0.5)
        
        print("Step 4: Sending lowercase 'b' to trigger parameter adjustment...")
        proc.stdin.write('b')
        proc.stdin.flush()
        
        # Wait to see results
        time.sleep(1)
        
        print("Step 5: Exiting simulation...")
        proc.stdin.write('\033')  # ESC key
        proc.stdin.flush()
        
        # Wait for exit
        proc.wait(timeout=5)
        
    except subprocess.TimeoutExpired:
        print("Simulation didn't exit, killing...")
        proc.kill()
        proc.wait()
    
    except KeyboardInterrupt:
        print("Test interrupted, killing simulation...")
        proc.kill()
        proc.wait()
    
    print("Test complete")

if __name__ == "__main__":
    test_parameter_lock()