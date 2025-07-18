# Task: Implement `fastled --install` Feature

## Overview

Implement the `fastled --install` command that provides a comprehensive setup for FastLED development environments. The command adapts its behavior based on the project context and user choices, ensuring safe installation while protecting existing development environments.

## Core Requirements

### 1. Project Detection and Validation

#### Directory Validation Flow
1. **Current Directory**: Check if `.vscode/` exists in current directory
2. **Parent Search**: If not found, search up to 5 parent directories for `.vscode/`
3. **IDE Availability**: Verify `code` (VSCode) OR `cursor` (Cursor) commands are available
4. **Project Generation**: Offer to create new VSCode project if none found and IDE available

#### Repository Type Detection
```python
def detect_fastled_project():
    """Check if library.json contains FastLED"""
    # Returns True if library.json has "name": "FastLED"

def is_fastled_repository():
    """🚨 CRITICAL: Detect actual FastLED repository"""
    # Strict verification of multiple markers:
    # - src/FastLED.h, examples/Blink/Blink.ino, ci/ci-compile.py
    # - src/platforms/, tests/test_*.cpp pattern
    # - library.json with correct name and repository URL
```

### 2. User Interaction Flow

#### Auto Debug Extension Prompt
```
Would you like to install the plugin for FastLED (auto-debug)? [y/n]
```
- **Always prompt** before installing extension
- **Dry-run mode**: Skip prompt, simulate installation
- **Installation**: Support both VSCode and Cursor

#### Project Generation Prompt
```
No .vscode directory found in current directory or parent directories.
Would you like to generate a VSCode project with FastLED configuration? [y/n]
```

#### Parent Directory Prompt
```
Found a .vscode project in /path/to/parent/
Install there? [y/n]
```

#### Examples Installation Prompt
```
No existing Arduino content found.
Would you like to install FastLED examples? [y/n]
```
- **Only prompt** if NO `.ino` files AND NO `examples/` folder exist
- **New projects**: Auto-install examples without prompting

### 3. Installation Modes

| Project Type | Detection Criteria | Installation Behavior |
|--------------|-------------------|----------------------|
| **Basic Project** | `.vscode/` exists, not FastLED | Arduino debugging + tasks |
| **External FastLED** | `library.json` has FastLED, not repository | Arduino debugging + tasks (NO clangd) |
| **FastLED Repository** | All repository markers present | Full development environment |
| **New Project** | No `.vscode/`, IDE available | Generate project + examples + tasks |

## 🚨 CRITICAL SAFETY REQUIREMENTS

### clangd Environment Protection
- **MANDATORY**: Only install clangd settings in actual FastLED repository
- **Detection**: Must pass strict `is_fastled_repository()` verification
- **Protection**: Prevents corruption of user's C++ development environment
- **NO EXCEPTIONS**: This rule cannot be bypassed under any circumstances

### Error Messages
- Missing `.vscode/` + No IDE: `"No supported IDE found (VSCode or Cursor). Please install VSCode or Cursor first."`
- clangd Protection: `"⚠️ Skipping clangd settings - not in FastLED repository (protects your environment)"`

## Implementation Specification

### Core Functions

#### 1. Main Installation Function
```python
def fastled_install(dry_run=False):
    """Main installation function with dry-run support"""
    try:
        # 1. Validate VSCode project or offer alternatives
        validate_vscode_project()
        
        # 2. Detect project type
        is_fastled_project = detect_fastled_project()
        is_repository = is_fastled_repository()
        
        # 3. Auto Debug extension (with prompt)
        if not dry_run:
            response = input("Would you like to install the plugin for FastLED (auto-debug)? [y/n]")
        else:
            response = 'yes'
            
        if response in ['y', 'yes']:
            if dry_run:
                print("[DRY-RUN]: NO PLUGIN INSTALLED")
            else:
                install_auto_debug_extension()
        
        # 4. Configure VSCode files
        update_launch_json_for_arduino()
        generate_fastled_tasks()
        
        # 5. Examples installation (conditional)
        if not check_existing_arduino_content():
            install_fastled_examples()
        
        # 6. Full development setup (repository only)
        if is_fastled_project:
            if is_repository:
                setup_fastled_development_environment()
                update_vscode_settings_for_fastled()
            else:
                print("⚠️ Skipping clangd settings - not in FastLED repository")
        
        # 7. Post-installation auto-execution
        if not dry_run:
            auto_execute_fastled()
        
        return True
    except Exception as e:
        print(f"❌ Installation failed: {e}")
        return False
```

#### 2. VSCode Configuration Generation

**Launch Configuration (`launch.json`)**:
```json
{
    "name": "🎯 Auto Debug (Smart File Detection)",
    "type": "auto-debug",
    "request": "launch",
    "map": {
        "*.ino": "Arduino: Run .ino with FastLED",
        "*.py": "Python: Current File (UV)"
    }
}
```

**Build Tasks (`tasks.json`)**:
```json
{
    "label": "Run FastLED (Debug)",
    "command": "fastled",
    "args": ["${file}", "--debug"],
    "group": {"kind": "build", "isDefault": true}
},
{
    "label": "Run FastLED (Quick)", 
    "command": "fastled",
    "args": ["${file}", "--background-update"]
}
```

**Settings (`settings.json`)**:
- Basic projects: File associations and basic formatting
- FastLED repository: clangd configuration + IntelliSense overrides

#### 3. Examples Installation System

**Content Detection**:
```python
def check_existing_arduino_content():
    """Check for .ino files OR examples/ folder"""
    ino_files = list(Path.cwd().rglob("*.ino"))
    examples_folder = Path("examples").exists()
    return len(ino_files) > 0 or examples_folder
```

**Installation Process**:
1. Download examples from FastLED GitHub repository
2. Create `examples/` directory structure
3. Generate `Blink.ino` in project root for quick testing

#### 4. Post-Installation Auto-Execution

**Trigger Conditions**:
- Arduino content detected (`.ino` files OR examples)
- Not in dry-run mode

**Execution Process**:
```python
def auto_execute_fastled():
    """Auto-launch fastled after successful installation"""
    if check_existing_arduino_content() or os.path.exists("Blink.ino"):
        # Filter arguments: remove --install, --dry-run
        # Add current directory if no target specified
        # Call main() directly: equivalent to 'fastled .'
```

## Testing Requirements

### Dry-Run Mode Support
**Command**: `fastled --install --dry-run`

**Behavior**:
- Skip actual extension installation: Output `[DRY-RUN]: NO PLUGIN INSTALLED`
- Create all `.vscode/*.json` files for validation
- Skip auto-execution at end
- Default to 'yes' for all prompts

### Unit Test Suite

#### Required Test Functions
```python
def test_fastled_install_dry_run():
    """Comprehensive dry-run validation in temporary directory"""
    # Verify all .vscode files created and valid
    # Check Auto Debug configuration mapping
    # Validate FastLED tasks presence and arguments
    # Confirm Blink.ino example creation

def test_fastled_install_existing_vscode_project():
    """Test configuration merging with existing projects"""
    # Preserve existing launch.json configurations
    # Merge tasks without duplicates

def test_fastled_install_auto_execution():
    """Verify auto-execution triggers correctly"""
    # Mock main() function call
    # Verify argument filtering

def test_fastled_repository_detection_safety():
    """🚨 CRITICAL: Test clangd protection"""
    # Create fake FastLED project
    # Verify clangd settings NOT applied
    # Confirm safety messages displayed
```

### Validation Criteria
1. **JSON Validity**: All generated `.vscode/*.json` files must be valid JSON
2. **Configuration Completeness**: Required configurations present
3. **Task Functionality**: Correct commands and arguments
4. **Safety Protection**: clangd settings only in repository
5. **Auto-Execution**: Proper argument filtering and main() call

## Installation Behavior Reference

### Complete Decision Matrix

| Current Dir | Parent Search | IDE Available | FastLED Repo | External FastLED | Has Content | Result |
|-------------|---------------|---------------|--------------|------------------|-------------|---------|
| ✅ .vscode | N/A | ✅ | ❌ | ❌ | Any | **Basic**: Arduino debugging |
| ✅ .vscode | N/A | ✅ | ❌ | ✅ | Any | **Limited**: Arduino debugging (NO clangd) |
| ✅ .vscode | N/A | ✅ | ✅ | ✅ | Any | **Full Dev**: Arduino + clangd + dev env |
| ❌ | ✅ Found | ✅ | Any | Any | Any | **Prompt**: Install in parent? |
| ❌ | ❌ None | ✅ | Any | Any | Any | **Generate**: New project + examples |
| ❌ | ❌ None | ❌ | Any | Any | Any | **Error**: No IDE found |

### Post-Installation Actions
- **Examples**: Installed if no existing Arduino content
- **Auto-Execution**: Runs `fastled .` if content present (skip in dry-run)
- **Protection Messages**: Clear explanations when skipping clangd

## Success Criteria

### Core Functionality
1. ✅ Validates VSCode project directory or searches parent directories
2. ✅ Offers to install in found parent VSCode project with directory change
3. ✅ Checks for available IDE before offering project generation
4. ✅ Generates complete VSCode project when requested
5. ✅ Prompts user before installing Auto Debug extension
6. ✅ Downloads and installs extension only with user consent

### Configuration Management
7. ✅ Updates `.vscode/launch.json` for Arduino debugging
8. ✅ Generates FastLED build tasks in `.vscode/tasks.json`
9. ✅ Merges with existing configurations without conflicts
10. ✅ Supports `.ino` files anywhere in project

### Content Management
11. ✅ Detects existing Arduino content to avoid conflicts
12. ✅ Prompts for examples only when no existing content found
13. ✅ Downloads complete FastLED examples from GitHub
14. ✅ Creates quick-start Blink.ino example

### Safety and Environment Protection
15. ✅ 🚨 **CRITICAL**: Only applies clangd settings in actual FastLED repository
16. ✅ 🚨 **CRITICAL**: Protects user environments from FastLED configurations
17. ✅ Detects FastLED repository with multiple verification markers
18. ✅ Provides clear protection messages

### Advanced Features
19. ✅ Auto-executes FastLED when Arduino content present
20. ✅ Skips auto-execution in dry-run mode
21. ✅ Properly filters arguments for auto-execution
22. ✅ Handles all error conditions gracefully

### Testing and Quality
23. ✅ Supports comprehensive dry-run mode for testing
24. ✅ Validates all generated JSON configurations
25. ✅ Provides clear feedback and manual fallback instructions

## Integration Notes

- **CLI Interface**: Add to existing fastled-wasm command structure
- **Command Pattern**: Follow existing argument parsing and error handling
- **Compatibility**: Maintain compatibility with existing fastled commands
- **Workflow Consistency**: Align with current FastLED development patterns
- **Test Integration**: Include in automated test suite with proper isolation