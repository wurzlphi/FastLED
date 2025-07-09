# Windows Stack Trace Improvements

## Overview

This document describes the improvements made to Windows stack trace functionality in the FastLED project. The original stack traces showed raw memory addresses, making debugging difficult. The improvements provide meaningful function names, line numbers, and detailed crash information.

## What Was Improved

### Before (Original Stack Trace)
```
Error: signal 11:
Stack trace (Windows):
#0  0x00007ff6988acd67 [test_rbtree.exe]
#1  0x00007ff6988ad066 [test_rbtree.exe]
#2  0x00007ff6988d95b2 [test_rbtree.exe]
#3  0x00007ff92b3fce46 _C_specific_handler+0x96
#4  0x00007ff92b4128bf _chkstk+0x11f
#5  0x00007ff92b3c2554 RtlRaiseException+0x484
```

### After (Improved Stack Trace)
```
=== WINDOWS EXCEPTION HANDLER ===
Exception caught: 0xc0000005 at address 0x00007ff6988acd67
Exception type: Access Violation
Attempted to read at address 0x0000000000000000

Stack trace (Windows):
Captured 15 frames:

#0  0x00007ff6988acd67 [test_rbtree.exe] test_function_1+0x22 [test_stack_trace_windows.cpp:12]
#1  0x00007ff6988ad066 [test_rbtree.exe] test_function_2+0x17 [test_stack_trace_windows.cpp:18]
#2  0x00007ff6988d95b2 [test_rbtree.exe] test_function_3+0x17 [test_stack_trace_windows.cpp:23]
#3  0x00007ff6988d95b2 [test_rbtree.exe] DOCTEST_ANON_FUNC_2+0x6e [test_stack_trace_windows.cpp:35]

Debug Information:
- Symbol handler initialized: Yes
- Process ID: 1234
- Thread ID: 5678

Loaded modules:
  test_rbtree.exe
  kernel32.dll
  ntdll.dll
```

## Key Improvements

### 1. Enhanced Symbol Resolution
- **Function Names**: Shows actual C++ function names instead of raw addresses
- **Line Numbers**: Displays source file and line number information
- **Module Names**: Identifies which executable/library each frame belongs to
- **Displacement**: Shows offset within functions for precise location

### 2. Detailed Exception Information
- **Exception Types**: Specific identification of crash types (Access Violation, Stack Overflow, etc.)
- **Memory Access Details**: For access violations, shows whether it was a read or write operation
- **Target Address**: Shows the specific memory address that caused the crash

### 3. Better Error Handling
- **Symbol Initialization**: Proper error reporting if debug symbols can't be loaded
- **Fallback Information**: Module names even when symbols aren't available
- **Debug Context**: Process ID, thread ID, and loaded modules for context

### 4. Improved Build Configuration
- **Debug Symbols**: Automatic generation of debug symbols for all test executables
- **Optimization Disabled**: Debug builds with `-O0` to preserve stack information
- **Frame Pointers**: `-fno-omit-frame-pointer` for better stack unwinding
- **Linker Options**: Proper debug linking with `/DEBUG:FULL` for MSVC

## Technical Implementation

### Crash Handler Architecture
The improved Windows crash handler uses a multi-layered approach:

1. **Windows Structured Exception Handling (SEH)**: Catches native Windows exceptions
2. **Signal Handling**: Handles POSIX signals (SIGSEGV, SIGABRT, etc.)
3. **Symbol Resolution**: Uses Windows Debug Help Library (dbghelp.dll)
4. **Stack Unwinding**: Uses `CaptureStackBackTrace` for reliable stack capture

### Key Files Modified
- `tests/crash_handler_win.h`: Enhanced Windows-specific crash handling
- `tests/CMakeLists.txt`: Improved debug symbol generation and linking
- `tests/test_stack_trace_windows.cpp`: Test file to demonstrate improvements

### Build Configuration Changes
```cmake
# MSVC (Visual Studio)
set_target_properties(${TEST_NAME} PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:CONSOLE /DEBUG:FULL /OPT:NOREF /OPT:NOICF"
    COMPILE_FLAGS "/Zi /Od /RTC1")

# MinGW/Clang
set_target_properties(${TEST_NAME} PROPERTIES
    LINK_FLAGS "-Xlinker /subsystem:console -g -O0")
target_compile_options(${TEST_NAME} PRIVATE -g -O0 -fno-omit-frame-pointer)
```

## Usage

### Running Tests with Improved Stack Traces
```bash
# Standard test run (includes stack trace improvements)
bash test

# Windows-specific test
bash test test_stack_trace_windows
```

### Manual Testing
```cpp
#include "crash_handler.h"

int main() {
    setup_crash_handler();  // Enable crash handling
    
    // Your code here...
    // If a crash occurs, you'll get detailed stack traces
    
    return 0;
}
```

### Testing the Improvements
```bash
# On Windows, run the test script
tests/test_windows_stack_trace.bat

# Or manually build and run
cd tests
mkdir .build && cd .build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --target test_stack_trace_windows
bin\test_stack_trace_windows.exe
```

## Platform Support

### Windows
- **Full Support**: Enhanced stack traces with symbol resolution
- **Requirements**: Debug symbols must be available
- **Fallback**: Module names when symbols aren't available

### Linux/macOS
- **LibUnwind**: Enhanced stack traces with symbol resolution
- **Execinfo**: Basic stack traces as fallback
- **No-op**: Minimal handling when no stack trace library is available

## Troubleshooting

### Common Issues

1. **"Symbol handler not available"**
   - **Cause**: Debug symbols not generated or not found
   - **Solution**: Ensure debug build with `-g` or `/Zi` flags

2. **"SymInitialize failed"**
   - **Cause**: Insufficient permissions or missing debug symbols
   - **Solution**: Run as administrator or check symbol path

3. **Raw addresses still showing**
   - **Cause**: Debug symbols stripped or not linked properly
   - **Solution**: Check linker flags and ensure debug build

### Debug Symbol Requirements
- **PDB files**: For MSVC builds, ensure `.pdb` files are generated
- **DWARF info**: For GCC/Clang builds, ensure `-g` flag is used
- **Symbol path**: Ensure debug symbols are in the expected location

## Future Enhancements

### Planned Improvements
1. **C++ Demangling**: Better demangling of C++ symbol names
2. **Source Context**: Show source code lines around crash location
3. **Variable Inspection**: Display local variable values at crash time
4. **Minidump Generation**: Create Windows minidump files for external analysis

### Integration Opportunities
1. **CI/CD Integration**: Automated crash analysis in continuous integration
2. **Remote Debugging**: Stack trace analysis for remote crash reports
3. **Performance Profiling**: Integration with performance analysis tools

## Conclusion

The Windows stack trace improvements significantly enhance the debugging experience for FastLED developers. Instead of cryptic memory addresses, developers now get meaningful function names, line numbers, and detailed crash context, making it much easier to identify and fix issues.

The improvements are backward-compatible and automatically fall back to basic information when debug symbols aren't available, ensuring robust operation across different build configurations.
