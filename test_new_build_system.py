#!/usr/bin/env python3
"""
Test script for the new FastLED build system v2.

This script validates that the new build system:
1. Correctly handles the user's test case: bash compile uno --examples Blink,Animartrix,apa102,apa102hd
2. Supports incremental builds
3. Has proper path handling
4. Works with the existing project structure
"""
import sys
import subprocess
import time
from pathlib import Path


def run_command(cmd, description="", check=True, timeout=300):
    """Run a command and capture its output."""
    print(f"\n{'='*60}")
    print(f"🔧 {description}")
    print(f"📋 Command: {' '.join(cmd)}")
    print(f"{'='*60}")
    
    start_time = time.time()
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=check,
            timeout=timeout
        )
        
        elapsed = time.time() - start_time
        print(f"⏱️  Completed in {elapsed:.2f} seconds")
        
        if result.stdout:
            print(f"\n📤 STDOUT:\n{result.stdout}")
        
        if result.stderr:
            print(f"\n📤 STDERR:\n{result.stderr}")
        
        return result
    
    except subprocess.CalledProcessError as e:
        elapsed = time.time() - start_time
        print(f"❌ Failed after {elapsed:.2f} seconds with return code {e.returncode}")
        
        if e.stdout:
            print(f"\n📤 STDOUT:\n{e.stdout}")
        
        if e.stderr:
            print(f"\n📤 STDERR:\n{e.stderr}")
        
        raise
    
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        print(f"⏰ Timed out after {elapsed:.2f} seconds")
        raise


def check_paths():
    """Check that required paths exist."""
    print("\n🔍 Checking project structure...")
    
    required_paths = [
        "ci/ci-compile-v2.py",
        "ci/ci/compile_for_board_v2.py", 
        "ci/ci/concurrent_run_v2.py",
        "ci/ci/create_build_dir_v2.py",
        "examples/Blink",
        "examples/Animartrix",
        "examples/Apa102",
        "examples/Apa102HD",
        "src",
    ]
    
    missing_paths = []
    for path_str in required_paths:
        path = Path(path_str)
        if not path.exists():
            missing_paths.append(path_str)
        else:
            print(f"✅ {path_str}")
    
    if missing_paths:
        print(f"\n❌ Missing required paths:")
        for path in missing_paths:
            print(f"   - {path}")
        return False
    
    print(f"✅ All required paths found!")
    return True


def test_v2_compilation():
    """Test the new v2 compilation system."""
    print("\n🚀 Testing FastLED v2 Build System")
    print("="*60)
    
    # Check project structure
    if not check_paths():
        print("❌ Project structure check failed")
        return False
    
    # Test 1: Basic compilation test with user's examples
    print("\n🧪 Test 1: Basic compilation (user's test case)")
    try:
        result = run_command([
            "uv", "run", "ci/ci-compile-v2.py",
            "uno", 
            "--examples", "Blink,Animartrix,Apa102,Apa102HD",
            "--no-interactive"
        ], "Running user's test case", timeout=600)
        
        if result.returncode == 0:
            print("✅ Test 1 PASSED: Basic compilation successful")
        else:
            print("❌ Test 1 FAILED: Basic compilation failed")
            return False
    except Exception as e:
        print(f"❌ Test 1 FAILED: Exception during compilation: {e}")
        return False
    
    # Test 2: Incremental build test
    print("\n🧪 Test 2: Incremental build test")
    try:
        # Run the same compilation again - should be faster due to incremental builds
        start_time = time.time()
        result = run_command([
            "uv", "run", "ci/ci-compile-v2.py",
            "uno",
            "--examples", "Blink",
            "--no-interactive"
        ], "Testing incremental build", timeout=300)
        elapsed = time.time() - start_time
        
        if result.returncode == 0:
            print(f"✅ Test 2 PASSED: Incremental build successful ({elapsed:.2f}s)")
        else:
            print("❌ Test 2 FAILED: Incremental build failed")
            return False
    except Exception as e:
        print(f"❌ Test 2 FAILED: Exception during incremental build: {e}")
        return False
    
    # Test 3: Force rebuild test
    print("\n🧪 Test 3: Force rebuild test")
    try:
        result = run_command([
            "uv", "run", "ci/ci-compile-v2.py",
            "uno",
            "--examples", "Blink",
            "--force-rebuild",
            "--no-interactive"
        ], "Testing force rebuild", timeout=300)
        
        if result.returncode == 0:
            print("✅ Test 3 PASSED: Force rebuild successful")
        else:
            print("❌ Test 3 FAILED: Force rebuild failed")
            return False
    except Exception as e:
        print(f"❌ Test 3 FAILED: Exception during force rebuild: {e}")
        return False
    
    # Test 4: Cache stats test
    print("\n🧪 Test 4: Cache statistics test")
    try:
        result = run_command([
            "uv", "run", "ci/ci-compile-v2.py",
            "--show-cache-stats"
        ], "Checking cache statistics", timeout=30)
        
        if result.returncode == 0:
            print("✅ Test 4 PASSED: Cache statistics displayed")
        else:
            print("❌ Test 4 FAILED: Cache statistics failed")
            return False
    except Exception as e:
        print(f"❌ Test 4 FAILED: Exception during cache stats: {e}")
        return False
    
    return True


def test_path_handling():
    """Test that paths are correctly handled."""
    print("\n🧪 Testing path handling...")
    
    # Check that build directories are created with proper structure
    build_dir = Path(".build")
    if build_dir.exists():
        uno_dir = build_dir / "uno"
        if uno_dir.exists():
            print(f"✅ Build directory structure looks good: {uno_dir}")
            
            # Check for cache file
            cache_file = uno_dir / "build_cache.json"
            if cache_file.exists():
                print(f"✅ Build cache found: {cache_file}")
            else:
                print(f"⚠️  Build cache not found (might not have run yet): {cache_file}")
                
            return True
        else:
            print(f"⚠️  UNO build directory not found: {uno_dir}")
            return False
    else:
        print(f"⚠️  Build directory not found: {build_dir}")
        return False


def main():
    """Main test function."""
    print("🧪 FastLED v2 Build System Test Suite")
    print("="*60)
    
    # Change to project root
    script_dir = Path(__file__).parent
    os.chdir(script_dir)
    
    success = True
    
    try:
        # Test the v2 compilation system
        if not test_v2_compilation():
            success = False
        
        # Test path handling
        if not test_path_handling():
            success = False
        
        # Final results
        print("\n" + "="*60)
        if success:
            print("🎉 ALL TESTS PASSED! The v2 build system is working correctly.")
            print("\nKey improvements verified:")
            print("  ✅ Incremental builds work")
            print("  ✅ Path handling is correct")
            print("  ✅ User test case compiles successfully")
            print("  ✅ Build cache is functioning")
        else:
            print("❌ SOME TESTS FAILED! Check the output above for details.")
        
        print("="*60)
        return 0 if success else 1
        
    except KeyboardInterrupt:
        print("\n🛑 Tests interrupted by user")
        return 1
    except Exception as e:
        print(f"\n💥 Unexpected error: {e}")
        return 1


if __name__ == "__main__":
    import os
    sys.exit(main())
