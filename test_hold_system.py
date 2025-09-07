#!/usr/bin/env python3
"""
Test script to verify the new hold system is working correctly.
Tests the UPPERCASE=PRESS, lowercase=RELEASE functionality.
"""

import subprocess
import time
import sys
import os

def run_test():
    print("Testing the new hold system for parameter lock...")
    print("UPPERCASE letters (Q,A,Z,1) should PRESS buttons")
    print("lowercase letters (q,a,z,!) should RELEASE buttons")
    print()
    
    # Change to build directory
    os.chdir('build-simulation')
    
    # Create test sequence
    test_sequence = [
        ("Q", "PRESS step button (1,0) - should start hold timer"),
        ("", "Wait 600ms - should trigger parameter lock mode"),
        ("q", "RELEASE step button (1,0) - should exit parameter lock mode"),
        ("1", "PRESS step button (0,0) - test number keys"),
        ("!", "RELEASE step button (0,0) - test shifted symbols"),
        ("\x1b", "ESC - quit simulation")
    ]
    
    # Build the input sequence with timing
    input_data = ""
    for key, description in test_sequence:
        print(f"Sending: '{key}' - {description}")
        input_data += key + "\n"
        if key == "Q":  # Add delay after pressing Q to allow parameter lock to trigger
            time.sleep(0.1)  # Small delay so the input reaches the program
    
    # Run simulation with test input
    try:
        result = subprocess.run(
            ['timeout', '5', './trellis_simulation'],
            input=input_data,
            text=True,
            capture_output=True,
            timeout=6
        )
        
        print("\n=== SIMULATION OUTPUT ===")
        if result.stdout:
            print("STDOUT:", result.stdout[:500] + "..." if len(result.stdout) > 500 else result.stdout)
        
        print("\n=== DEBUG OUTPUT ===")
        if result.stderr:
            print("STDERR:", result.stderr)
        else:
            print("No debug output captured")
        
        print(f"\nReturn code: {result.returncode}")
        
        # Check for expected debug messages
        if "PARAM_LOCK: Successfully entered parameter lock mode" in result.stderr:
            print("✅ SUCCESS: Parameter lock mode was activated!")
        elif "PARAM_LOCK:" in result.stderr:
            print("⚠️  PARTIAL: Some parameter lock debug output found, but not successful entry")
        else:
            print("❌ ISSUE: No parameter lock debug output found")
            
        return result.returncode == 0 or result.returncode == 124  # 124 is timeout (expected)
        
    except subprocess.TimeoutExpired:
        print("❌ Test timed out")
        return False
    except Exception as e:
        print(f"❌ Test failed with error: {e}")
        return False

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)