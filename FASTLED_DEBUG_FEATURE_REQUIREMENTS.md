# FastLED Debug Feature Requirements

## Overview

This document outlines the requirements for enhancing the `fastled --debug` feature to automatically enable app mode when Playwright cache is available, and to integrate with the `fastled --install` VSCode task generation.

## Current State

Based on the conversation summary and codebase analysis, the current FastLED ecosystem includes:

1. **FastLED C++ Library** (this repository) - Core LED control library
2. **FastLED Python CLI** (external PyPI package) - Web compiler and development tools
3. **VSCode Integration** - Tasks and debugging configurations

## Feature Requirements

### 1. Playwright Cache Detection for `fastled --debug`

**Requirement**: When `fastled --debug` is used, check for Playwright cache and automatically enable app mode.

**Behavior**:
- **If `~/.fastled/playwright` exists**: Automatically add `--app` flag without prompting
- **If `~/.fastled/playwright` does NOT exist**: Prompt user "Would you like to install the FastLED debugger? [y/n]"

**Implementation Location**: External FastLED Python CLI package (not this repository)

### 2. VSCode Tasks Integration

**Requirement**: Update VSCode tasks generation to always include `--app` flag for debug mode.

**Current Tasks** (from conversation summary):
- "Run FastLED (Debug)" - Should use `--debug --app` flags
- "Run FastLED (Quick)" - Should use `--background-update` flag

**Implementation**: When `fastled --install` generates `.vscode/tasks.json`, ensure debug task includes:
```json
{
    "label": "Run FastLED (Debug)",
    "command": "fastled",
    "args": ["${file}", "--debug", "--app"],
    // ... other task configuration
}
```

### 3. Auto-App Mode Logic

**Condition for Automatic `--app`**:
```
IF (Playwright cache exists at ~/.fastled/playwright) THEN
    Add --app flag automatically
ELSE
    Prompt user: "Would you like to install the FastLED debugger? [y/n]"
    IF user says yes THEN
        Install playwright and add --app flag
    ELSE
        Run without --app flag
    END IF
END IF
```

### 4. Integration with Existing Install Process

**From conversation summary**, the `fastled --install` feature already:
- Checks for `.vscode/` directory existence
- Searches parent directories for existing VSCode projects
- Prompts for auto-debug extension installation
- Generates VSCode configuration files

**New Requirement**: Integrate Playwright cache detection into this flow.

## Technical Implementation Notes

### Playwright Cache Location
- **Path**: `~/.fastled/playwright`
- **Detection**: Check if directory exists and contains browser installation
- **Creation**: Installed via `pip install playwright && python -m playwright install`

### VSCode Tasks Template
The generated tasks should include:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "shell",
            "label": "Run FastLED (Debug)",
            "command": "fastled",
            "args": ["${file}", "--debug", "--app"],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": true,
                "panel": "new",
                "showReuseMessage": false,
                "clear": true
            },
            "detail": "Run FastLED with debug mode and app visualization",
            "problemMatcher": []
        },
        {
            "type": "shell",
            "label": "Run FastLED (Quick)",
            "command": "fastled", 
            "args": ["${file}", "--background-update"],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always", 
                "focus": true,
                "panel": "new",
                "showReuseMessage": false,
                "clear": true
            },
            "detail": "Run FastLED with quick background update mode",
            "problemMatcher": []
        }
    ]
}
```

## Dependencies

### External Packages Required
- `fastled` (PyPI package) - Contains the CLI implementation
- `playwright` (PyPI package) - Browser automation for app mode
- VSCode or Cursor - IDE for task execution

### File System Requirements
- Write access to `~/.fastled/` directory
- Write access to project `.vscode/` directory
- Read access to check Playwright cache existence

## Backward Compatibility

### Existing Behavior Preservation
- If user manually specifies `--debug` without `--app`, respect that choice
- Existing `fastled --install` functionality should continue to work
- Non-interactive environments should not prompt

### Graceful Degradation
- If Playwright is not available, continue without app mode
- If VSCode is not available, skip task generation
- If write permissions are lacking, warn but continue

## User Experience Flow

### First-Time User (No Playwright)
1. User runs `fastled --debug sketch.ino`
2. System detects no Playwright cache
3. Prompt: "Would you like to install the FastLED debugger? [y/n]"
4. If yes: Install Playwright, add `--app` flag, continue
5. If no: Run without `--app` flag

### Returning User (Has Playwright)
1. User runs `fastled --debug sketch.ino`
2. System detects Playwright cache at `~/.fastled/playwright`
3. Automatically add `--app` flag
4. Run with debug app mode enabled

### VSCode Task Usage
1. User opens `.ino` file in VSCode
2. Run "Run FastLED (Debug)" task
3. Task executes `fastled sketch.ino --debug --app`
4. Debug session starts with web app visualization

## Testing Requirements

### Unit Tests Needed
- Playwright cache detection logic
- Task generation with correct flags
- User prompt handling (interactive/non-interactive modes)
- Error handling for missing dependencies

### Integration Tests Needed
- End-to-end `fastled --debug` with and without Playwright
- VSCode task execution in various environments
- Cross-platform behavior (Windows, macOS, Linux)

## Implementation Priority

1. **High Priority**: Playwright cache detection for `fastled --debug`
2. **High Priority**: VSCode tasks generation with `--app` flag
3. **Medium Priority**: User prompting for debugger installation
4. **Low Priority**: Advanced error handling and edge cases

## Notes

- This feature enhancement is for the external FastLED Python CLI package
- The FastLED C++ library (this repository) does not need modification
- Implementation should be backward compatible with existing workflows
- Consider adding a `--no-app` flag to explicitly disable app mode if needed