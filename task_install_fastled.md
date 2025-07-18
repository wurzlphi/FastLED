# Task: Implement `fastled --install` Feature

## Overview

Implement the `fastled --install` command that installs the Auto Debug tool used in the FastLED repository. This command provides conditional installation based on project type and ensures proper VSCode debugging configuration for Arduino (.ino) files.

## Requirements

### 1. Directory Validation
- **Primary**: Must be executed in a VSCode project (`.vscode/` directory must exist in current directory)
- **Secondary**: Search up to 5 parent directories for existing `.vscode/` projects
- **Alternative**: If no `.vscode/` directory found anywhere, prompt user to generate VSCode project
- **IDE Check**: Only offer VSCode project generation if `code` OR `cursor` commands are available
- **Error Message**: If no IDEs available: "No supported IDE found (VSCode or Cursor). Please install VSCode or Cursor first."

### 2. Arduino File Support
- Add Auto Debug entries for `*.ino` files **anywhere** in the project (not just examples/)
- Modify `.vscode/launch.json` to support debugging any `.ino` file
- Remove examples-specific path restrictions

### 3. Conditional Installation Logic
- **Basic Install**: For projects WITHOUT FastLED in `library.json`
  - Install Auto Debug extension only
  - Add Arduino debugging configuration for `.ino` files
- **Full Install**: For projects WITH "FastLED" in `library.json` 
  - Everything from basic install PLUS
  - Complete FastLED development environment setup
  - clangd configuration and compile_commands.json generation
  - JavaScript linting setup

### 4. File Exclusions
- Do NOT install support for surrounding files (`.h`, `*.cpp`) UNLESS `library.json` contains "FastLED"
- Only `.ino` files get debugging support in non-FastLED projects

## Implementation Details

### Core Components

#### 1. Pre-Installation Validation

```python
import os
import json
import shutil
import subprocess
from pathlib import Path

def validate_vscode_project():
    """Validate that we're in a VSCode project directory or offer to create one"""
    if os.path.exists(".vscode"):
        return True
    
    # Search for .vscode directory up to 5 levels up
    vscode_project_path = find_vscode_project_upward()
    if vscode_project_path:
        print(f"Found a .vscode project in {vscode_project_path}")
        print("Install there? [y/n]")
        
        response = input().strip().lower()
        if response in ['y', 'yes']:
            # Change to the parent directory and continue installation there
            os.chdir(vscode_project_path)
            print(f"✅ Changed to VSCode project directory: {vscode_project_path}")
            return True
        else:
            print("Continuing with current directory...")
            # Fall through to generate new project logic
    
    # Check if any supported IDE is available
    has_vscode = shutil.which("code") is not None
    has_cursor = shutil.which("cursor") is not None
    
    if not has_vscode and not has_cursor:
        raise Exception("No supported IDE found (VSCode or Cursor). Please install VSCode or Cursor first.")
    
    # Prompt user to generate VSCode project
    if vscode_project_path:
        print("Would you like to generate a new VSCode project with FastLED configuration in the current directory? [y/n]")
    else:
        print("No .vscode directory found in current directory or parent directories.")
        print("Would you like to generate a VSCode project with FastLED configuration? [y/n]")
    
    response = input().strip().lower()
    if response in ['y', 'yes']:
        generate_vscode_project()
        print("✅ VSCode project generated successfully!")
        return True
    else:
        raise Exception("VSCode project generation declined. Cannot proceed without .vscode directory.")
    
    return False

def find_vscode_project_upward(max_levels=5):
    """Search for .vscode directory up to max_levels parent directories"""
    current_path = Path.cwd()
    
    for level in range(max_levels):
        # Go up one level (start with parent for level 0)
        parent_path = current_path.parent
        if parent_path == current_path:  # Reached filesystem root
            break
        current_path = parent_path
        
        vscode_path = current_path / ".vscode"
        if vscode_path.exists() and vscode_path.is_dir():
            return str(current_path.absolute())
    
    return None

def detect_fastled_project():
    """Check if this is a FastLED library project"""
    if not os.path.exists("library.json"):
        return False
    
    try:
        with open("library.json", 'r') as f:
            library_data = json.load(f)
            return "FastLED" in library_data.get("name", "")
    except (json.JSONDecodeError, KeyError):
        return False

def generate_vscode_project():
    """Generate a complete VSCode project with FastLED configuration"""
    print("🔧 Generating VSCode project with FastLED configuration...")
    
    # Create .vscode directory
    os.makedirs(".vscode", exist_ok=True)
    
    # Generate launch.json
    launch_config = {
        "version": "0.2.0",
        "configurations": [
            {
                "name": "🎯 Auto Debug (Smart File Detection)",
                "type": "auto-debug",
                "request": "launch",
                "map": {
                    "*.py": "Python: Current File (UV)",
                    "*.ino": "Arduino: Run .ino with FastLED"
                }
            },
            {
                "name": "Python: Current File (UV)",
                "type": "node-terminal",
                "request": "launch",
                "command": "uv run python \"${file}\"",
                "cwd": "${workspaceFolder}",
                "console": "integratedTerminal",
                "internalConsoleOptions": "neverOpen"
            },
            {
                "name": "Arduino: Run .ino with FastLED",
                "type": "python",
                "request": "launch",
                "module": "fastled",
                "args": ["${file}", "--background-update", "--app", "--debug"],
                "console": "integratedTerminal",
                "cwd": "${workspaceFolder}",
                "justMyCode": False
            }
        ]
    }
    
    with open(".vscode/launch.json", 'w') as f:
        json.dump(launch_config, f, indent=4)
    
    # Generate tasks.json
    tasks_config = {
        "version": "2.0.0",
        "tasks": [
            {
                "type": "shell",
                "label": "Run FastLED Web Compiler",
                "command": "uv",
                "args": ["run", "fastled", "${file}", "--no-auto-updates"],
                "options": {"cwd": "${workspaceFolder}"},
                "group": {"kind": "build", "isDefault": True},
                "presentation": {
                    "echo": True,
                    "reveal": "always",
                    "focus": True,
                    "panel": "new",
                    "showReuseMessage": False,
                    "clear": True
                },
                "detail": "Run FastLED web compiler on the current .ino file",
                "problemMatcher": []
            },
            {
                "type": "shell",
                "label": "Install FastLED Package",
                "command": "pip",
                "args": ["install", "fastled"],
                "options": {"cwd": "${workspaceFolder}"},
                "group": "build",
                "presentation": {
                    "echo": True,
                    "reveal": "always",
                    "focus": False,
                    "panel": "shared",
                    "showReuseMessage": True,
                    "clear": False
                },
                "detail": "Install or upgrade the FastLED Python package"
            }
        ]
    }
    
    with open(".vscode/tasks.json", 'w') as f:
        json.dump(tasks_config, f, indent=4)
    
    # Generate settings.json
    settings_config = {
        "files.eol": "\n",
        "files.autoDetectEol": False,
        "files.insertFinalNewline": True,
        "files.trimFinalNewlines": True,
        "editor.tabSize": 4,
        "editor.insertSpaces": True,
        "editor.detectIndentation": True,
        "editor.formatOnSave": False,
        "python.defaultInterpreterPath": "uv",
        "files.associations": {
            "*.ino": "cpp"
        }
    }
    
    with open(".vscode/settings.json", 'w') as f:
        json.dump(settings_config, f, indent=4)
    
    # Generate extensions.json
    extensions_config = {
        "recommendations": [
            "ms-python.python",
            "ms-vscode.cpptools",
            "ms-vscode.cmake-tools"
        ]
    }
    
    with open(".vscode/extensions.json", 'w') as f:
        json.dump(extensions_config, f, indent=4)
    
    print("📁 Generated .vscode/launch.json - Arduino debugging configuration")
    print("📁 Generated .vscode/tasks.json - FastLED build tasks")
    print("📁 Generated .vscode/settings.json - Basic project settings")
    print("📁 Generated .vscode/extensions.json - Recommended extensions")
```

#### 2. Auto Debug Extension Management

**Extension Details:**
- **File**: `DarrenLevine.auto-debug-1.0.2.vsix`
- **Size**: 93KB
- **GitHub URL**: `https://raw.githubusercontent.com/fastled/fastled/main/.vscode/DarrenLevine.auto-debug-1.0.2.vsix`

```python
def download_auto_debug_extension():
    """Download the Auto Debug extension from GitHub"""
    import requests
    
    url = "https://raw.githubusercontent.com/fastled/fastled/main/.vscode/DarrenLevine.auto-debug-1.0.2.vsix"
    local_path = ".vscode/DarrenLevine.auto-debug-1.0.2.vsix"
    
    print("Downloading Auto Debug extension...")
    response = requests.get(url)
    response.raise_for_status()
    
    with open(local_path, 'wb') as f:
        f.write(response.content)
    
    print(f"✅ Downloaded extension to {local_path}")
    return local_path

def install_vscode_extensions(extension_path):
    """Install extension in available editors"""
    installed_count = 0
    
    # Try VSCode
    if shutil.which("code"):
        try:
            subprocess.run(
                ["code", "--install-extension", extension_path],
                check=True, capture_output=True
            )
            print("✅ Auto Debug extension installed successfully on VSCode!")
            installed_count += 1
        except subprocess.CalledProcessError:
            print("⚠️  Warning: Auto Debug extension installation failed on VSCode.")
    else:
        print("ℹ️  VSCode not found (code command not available).")
    
    # Try Cursor
    if shutil.which("cursor"):
        try:
            subprocess.run(
                ["cursor", "--install-extension", extension_path],
                check=True, capture_output=True
            )
            print("✅ Auto Debug extension installed successfully on Cursor!")
            installed_count += 1
        except subprocess.CalledProcessError:
            print("⚠️  Warning: Auto Debug extension installation failed on Cursor.")
    else:
        print("ℹ️  Cursor not found (cursor command not available).")
    
    return installed_count
```

#### 3. Arduino Debug Configuration Updates

**Target File**: `.vscode/launch.json`

**Key Changes Required:**
- Update Auto Debug mapping from `"examples/**/*.ino"` to `"*.ino"` 
- Ensure Arduino debug configuration exists
- Support debugging any `.ino` file anywhere in the project

```python
def update_launch_json_for_arduino():
    """Update launch.json to add Arduino debugging for .ino files"""
    launch_json_path = ".vscode/launch.json"
    
    # Read existing configuration
    with open(launch_json_path, 'r') as f:
        launch_config = json.load(f)
    
    # Find the Auto Debug configuration
    auto_debug_config = None
    for config in launch_config["configurations"]:
        if config.get("name") == "🎯 Auto Debug (Smart File Detection)":
            auto_debug_config = config
            break
    
    if auto_debug_config:
        # Update the mapping to support .ino files anywhere (not just examples/)
        auto_debug_config["map"]["*.ino"] = "Arduino: Run .ino with FastLED"
        # Remove the examples-specific mapping if it exists
        if "examples/**/*.ino" in auto_debug_config["map"]:
            del auto_debug_config["map"]["examples/**/*.ino"]
    else:
        # Add the Auto Debug configuration if it doesn't exist
        auto_debug_config = {
            "name": "🎯 Auto Debug (Smart File Detection)",
            "type": "auto-debug", 
            "request": "launch",
            "map": {
                "*.ino": "Arduino: Run .ino with FastLED"
            }
        }
        launch_config["configurations"].insert(0, auto_debug_config)
    
    # Ensure Arduino debug configuration exists
    arduino_config_exists = any(
        config.get("name") == "Arduino: Run .ino with FastLED" or 
        config.get("name") == "Arduino: Run .ino with FastLED (Examples)"
        for config in launch_config["configurations"]
    )
    
    if not arduino_config_exists:
        arduino_config = {
            "name": "Arduino: Run .ino with FastLED",
            "type": "python",
            "request": "launch", 
            "module": "fastled",
            "args": ["${file}", "--background-update", "--app", "--debug"],
            "console": "integratedTerminal",
            "cwd": "${workspaceFolder}",
            "justMyCode": False
        }
        launch_config["configurations"].append(arduino_config)
    
    # Write back the updated configuration
    with open(launch_json_path, 'w') as f:
        json.dump(launch_config, f, indent=4)
```

#### 4. Full FastLED Development Setup

**Only executed when `library.json` contains "FastLED"**

This replicates the functionality from the `./install` script:

```python
def setup_fastled_development_environment():
    """Full FastLED development environment setup"""
    print("🔧 Setting up FastLED development environment...")
    
    # Build tests to generate compile_commands.json for clangd
    print("Building C++ tests to generate compile_commands.json...")
    try:
        subprocess.run([
            "uv", "run", "ci/cpp_test_run.py", 
            "--compile-only", "--clang", "--test", "test_function"
        ], check=True, cwd=".")
        
        # Copy compile_commands.json to project root for clangd IntelliSense
        if os.path.exists("tests/.build/compile_commands.json"):
            shutil.copy("tests/.build/compile_commands.json", "compile_commands.json")
            print("✅ clangd support enabled - IntelliSense should work in VSCode")
        else:
            print("⚠️  Warning: compile_commands.json not found. Tests may not have built successfully.")
    except subprocess.CalledProcessError as e:
        print(f"⚠️  Warning: Failed to build tests for clangd setup: {e}")
    
    # Setup JavaScript linting
    print("Installing fast JavaScript linter (Node.js + ESLint)...")
    try:
        subprocess.run(["uv", "run", "ci/setup-js-linting-fast.py"], check=True, cwd=".")
        print("✅ Fast JavaScript linting enabled - 53x faster than Deno!")
    except subprocess.CalledProcessError:
        print("⚠️  Warning: JavaScript linter setup failed. You can retry with: uv run ci/setup-js-linting-fast.py")

def update_vscode_settings_for_fastled():
    """Update VSCode settings for FastLED development"""
    settings_path = ".vscode/settings.json"
    
    # Essential clangd settings for FastLED development
    fastled_settings = {
        "clangd.arguments": [
            "--compile-commands-dir=${workspaceFolder}",
            "--clang-tidy",
            "--header-insertion=never", 
            "--completion-style=detailed",
            "--function-arg-placeholders=false",
            "--background-index",
            "--pch-storage=memory"
        ],
        "clangd.fallbackFlags": [
            "-std=c++17",
            "-I${workspaceFolder}/src",
            "-I${workspaceFolder}/tests",
            "-Wno-global-constructors"
        ],
        "C_Cpp.intelliSenseEngine": "disabled",
        "C_Cpp.autocomplete": "disabled",
        "C_Cpp.errorSquiggles": "disabled"
    }
    
    if os.path.exists(settings_path):
        with open(settings_path, 'r') as f:
            settings = json.load(f)
    else:
        settings = {}
    
    # Merge FastLED-specific settings
    settings.update(fastled_settings)
    
    with open(settings_path, 'w') as f:
        json.dump(settings, f, indent=4)
    
    print("✅ VSCode settings updated for FastLED development")
```

#### 5. Main Installation Function

```python
def fastled_install():
    """Main installation function for fastled --install"""
    try:
        # Validate we're in a VSCode project
        validate_vscode_project()
        print("✅ VSCode project detected")
        
        # Check if this is a FastLED library project
        is_fastled_project = detect_fastled_project()
        
        if is_fastled_project:
            print("🚀 FastLED library project detected - performing full installation")
        else:
            print("📦 General project detected - installing Arduino debugging support only")
        
        # Download and install Auto Debug extension
        extension_path = download_auto_debug_extension()
        installed_count = install_vscode_extensions(extension_path)
        
        # Update launch.json for Arduino debugging
        update_launch_json_for_arduino()
        print("✅ Arduino debugging configuration added to .vscode/launch.json")
        
        # Full FastLED setup only if this is a FastLED project
        if is_fastled_project:
            setup_fastled_development_environment()
            update_vscode_settings_for_fastled()
            
            print("\n🎉 Full FastLED development environment installation complete!")
            print("\nTo use:")
            print("  - Run tests: bash test")
            print("  - Run linting: bash lint (Python, C++, and JavaScript)")
            print("  - Debug in VSCode: Open test file and press F5")
            print("  - Auto Debug: Use '🎯 Auto Debug (Smart File Detection)' configuration")
            print("  - clangd IntelliSense: Should work automatically in VSCode")
        else:
            print("\n🎉 Arduino debugging installation complete!")
            print("\nTo use:")
            print("  - Debug .ino files: Open any .ino file and press F5")
            print("  - Auto Debug: Use '🎯 Auto Debug (Smart File Detection)' configuration")
        
        # Installation summary
        if installed_count == 0:
            print("\n⚠️  Warning: Auto Debug extension could not be installed automatically.")
            print("      Please install manually:")
            print("      - Open VSCode/Cursor")
            print("      - Go to Extensions (Ctrl+Shift+X)")
            print("      - Click ... menu → Install from VSIX")
            print(f"      - Select: {extension_path}")
        
        return True
        
    except Exception as e:
        print(f"❌ Installation failed: {e}")
        return False
```

## Reference URLs

### Auto Debug Extension Binary
```
https://raw.githubusercontent.com/fastled/fastled/main/.vscode/DarrenLevine.auto-debug-1.0.2.vsix
```

### Install Script Reference
```
https://raw.githubusercontent.com/fastled/fastled/main/install
```

**Relevant Logic**: Lines 43-87 contain the Auto Debug extension installation logic from the original install script.

## Installation Behavior Matrix

| Project Type | `.vscode/` here | `.vscode/` found up | IDE Available | `library.json` has "FastLED" | Installation Behavior |
|--------------|-----------------|-------------------|---------------|------------------------------|----------------------|
| VSCode Project | ✅ | N/A | ✅ | ❌ | **Basic**: Arduino debugging only |
| FastLED Project | ✅ | N/A | ✅ | ✅ | **Full**: Arduino debugging + FastLED dev environment |
| Parent VSCode | ❌ | ✅ | ✅ | N/A | **Prompt**: "Found .vscode in `<path>`, install there?" → cd + install |
| No VSCode (with IDE) | ❌ | ❌ | ✅ | N/A | **Prompt**: Generate VSCode project → Basic/Full install |
| No VSCode (no IDE) | ❌ | ❌ | ❌ | N/A | **Error**: "No supported IDE found" |

**Search Range**: Up to 5 parent directories  
**IDE Available**: Either `code` (VSCode) or `cursor` (Cursor) command exists

## Configuration Changes Summary

### All Projects (Basic Install)
- **`.vscode/launch.json`**: 
  - Add/update Auto Debug configuration with `"*.ino": "Arduino: Run .ino with FastLED"`
  - Remove examples-specific paths (`"examples/**/*.ino"`)
  - Add Arduino debug configuration if missing
- **Extension Installation**: 
  - Download and install `DarrenLevine.auto-debug-1.0.2.vsix`
  - Support both VSCode (`code`) and Cursor (`cursor`) commands

### FastLED Projects Only (Full Install)
- **All basic install features** PLUS:
- **`.vscode/settings.json`**: 
  - Add clangd configuration with FastLED-specific paths
  - Disable conflicting C++ IntelliSense
  - Set fallback flags for C++17 and FastLED includes
- **`compile_commands.json`**: 
  - Generate via `ci/cpp_test_run.py --compile-only --clang --test test_function`
  - Copy from `tests/.build/compile_commands.json` to project root
- **JavaScript Environment**: 
  - Setup fast Node.js-based linting via `ci/setup-js-linting-fast.py`

## Error Handling Requirements

1. **Missing `.vscode/` Directory**: 
   - **Parent Directory Found**: Prompt "Found a .vscode project in `<path>`, install there? [y/n]"
     - If yes: Change to parent directory and continue installation
     - If no: Fall through to generation prompt
   - **No Parent Found + IDE Available**: Prompt user to generate VSCode project with [y/n] choice
   - **No IDE Available**: Error with exact message: "No supported IDE found (VSCode or Cursor). Please install VSCode or Cursor first."
   - **User Declines Generation**: "VSCode project generation declined. Cannot proceed without .vscode directory."

2. **Network Failures**: 
   - Graceful fallback with manual installation instructions
   - Show exact path to downloaded extension file

3. **Editor Not Found**: 
   - Informational messages for missing `code` or `cursor` commands
   - Continue with available editors

4. **Build Failures**: 
   - Warning messages for clangd setup failures
   - Continue with partial setup, don't abort entire installation

5. **JSON Parsing Errors**: 
   - Handle malformed `.vscode/*.json` files gracefully
   - Backup original files before modification

## Success Criteria

1. ✅ Validates VSCode project directory requirement or searches parent directories
2. ✅ Searches up to 5 parent directories for existing `.vscode/` projects
3. ✅ Offers to install in found parent VSCode project with directory change
4. ✅ Checks for available IDE (VSCode/Cursor) before offering project generation
5. ✅ Generates complete VSCode project with FastLED configuration when requested
6. ✅ Correctly detects FastLED vs non-FastLED projects
7. ✅ Downloads and installs Auto Debug extension
8. ✅ Updates `.vscode/launch.json` for Arduino debugging
9. ✅ Supports `.ino` files anywhere in project (not just examples/)
10. ✅ Conditional full setup for FastLED projects only
11. ✅ Provides clear feedback and manual fallback instructions
12. ✅ Handles all error conditions gracefully

## Integration Notes

- Add this functionality to the existing fastled-wasm CLI interface
- Follow existing command patterns and error handling
- Ensure compatibility with existing fastled commands
- Maintain consistency with current FastLED development workflow