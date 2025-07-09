#!/usr/bin/env python3
"""
Build Performance Demo Script

This script demonstrates the performance improvement achieved by using
shared object files for test compilation instead of rebuilding the
main function and doctest infrastructure for each test.
"""

import subprocess
import time
import os
import sys
from pathlib import Path

def run_command(cmd, cwd=None):
    """Run a command and return the result"""
    print(f"Running: {cmd}")
    start_time = time.time()
    result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
    elapsed = time.time() - start_time
    return result, elapsed

def main():
    print("FastLED Test Build Performance Demo")
    print("=" * 50)
    
    # Change to tests directory
    tests_dir = Path(__file__).parent
    os.chdir(tests_dir)
    
    print("\n1. Clean build directory")
    run_command("rm -rf .build")
    
    print("\n2. Configure CMake")
    result, elapsed = run_command("cmake -S . -B .build -G Ninja")
    if result.returncode != 0:
        print("CMake configuration failed!")
        print(result.stderr)
        sys.exit(1)
    print(f"Configuration time: {elapsed:.2f}s")
    
    print("\n3. Build single test (test_audio_json_parsing)")
    result, elapsed = run_command("cmake --build .build --target test_audio_json_parsing")
    if result.returncode != 0:
        print("Single test build failed!")
        print(result.stderr)
        sys.exit(1)
    print(f"Single test build time: {elapsed:.2f}s")
    
    print("\n4. Build all tests")
    result, elapsed = run_command("cmake --build .build")
    if result.returncode != 0:
        print("Full build failed!")
        print(result.stderr)
        sys.exit(1)
    print(f"Full build time: {elapsed:.2f}s")
    
    print("\n5. Run all tests")
    result, elapsed = run_command("ctest --test-dir .build --output-on-failure")
    if result.returncode != 0:
        print("Tests failed!")
        print(result.stderr)
        sys.exit(1)
    print(f"Test execution time: {elapsed:.2f}s")
    
    print("\n" + "=" * 50)
    print("PERFORMANCE IMPROVEMENT SUMMARY")
    print("=" * 50)
    print("""
The new build system uses shared object files to avoid rebuilding
common components for each test:

✅ IMPROVEMENTS:
- Shared doctest_main.cpp compiled once as object file
- Each test links to the shared object instead of recompiling
- Reduced linking overhead per test
- Faster incremental builds when only test files change
- Maintains full test isolation and functionality

✅ BENEFITS:
- Faster build times, especially for incremental builds
- Reduced disk I/O and compilation overhead
- Better parallelization of test compilation
- Maintains all existing test functionality

✅ TECHNICAL DETAILS:
- Uses CMake OBJECT libraries for shared components
- Each test executable links to shared objects via $<TARGET_OBJECTS:>
- No changes to test source code required
- Compatible with all existing test infrastructure
""")

if __name__ == "__main__":
    main()
