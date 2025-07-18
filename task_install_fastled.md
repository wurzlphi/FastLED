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

### 5. Project Initialization with Examples
- **New Projects**: Automatically copy all FastLED examples when generating new VSCode project
- **Existing Projects**: Check for existing `.ino` files or `examples/` folder
  - If NO `.ino` files AND NO `examples/` folder found: Prompt user to install examples [y/n]
  - If existing content found: Skip example installation to avoid conflicts

### 6. FastLED Build Tasks Configuration
- **Generate/Update `.vscode/tasks.json`**: Add FastLED-specific build tasks
- **Run FastLED (Debug)**: Task with `--debug` flag for debugging mode
- **Run FastLED (Quick)**: Task with `--background-update` flag for quick execution
- **Project Root Execution**: Tasks configured to run `fastled` from project root directory

### 7. Post-Installation Auto-Execution
- **Condition Check**: After successful installation, check for Arduino content
- **Content Detection**: Look for `.ino` files OR installed examples
- **Automatic Launch**: If content found, remove `--install` argument and run `main()`
- **Execution Behavior**: Equivalent to running `fastled .` from command line
- **Dry-Run Skip**: Do NOT auto-execute in dry-run mode

### 8. Testing Requirements
- **Dry Run Mode**: Support `--dry-run` flag for testing without actual installations
- **Temporary Directory Testing**: Unit tests must run in isolated temporary folders
- **Plugin Installation**: In dry-run mode, output `[DRY-RUN]: NO PLUGIN INSTALLED` instead of installing extension
- **File Creation**: Still create `.vscode/*.json` files in dry-run mode for validation
- **Validation**: Verify generated JSON files are valid and contain expected configurations
- **Auto-Execution Testing**: Verify that auto-execution is triggered when appropriate

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
    
    # Generate FastLED tasks using the dedicated function
    generate_fastled_tasks()
    
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
    print("📁 Generated .vscode/settings.json - Basic project settings")
    print("📁 Generated .vscode/extensions.json - Recommended extensions")
    
    # For new projects, automatically install examples
    install_fastled_examples(force=True)

def check_existing_arduino_content():
    """Check if project already has .ino files or examples folder"""
    # Check for any .ino files in current directory and subdirectories
    current_dir = Path.cwd()
    ino_files = list(current_dir.rglob("*.ino"))
    
    # Check for examples folder
    examples_folder = current_dir / "examples"
    has_examples_folder = examples_folder.exists() and examples_folder.is_dir()
    
    return len(ino_files) > 0 or has_examples_folder

def install_fastled_examples(force=False):
    """Install FastLED examples into the current project"""
    if not force and check_existing_arduino_content():
        print("Found existing .ino files or examples/ folder.")
        print("Would you like to install FastLED examples anyway? [y/n]")
        
        response = input().strip().lower()
        if response not in ['y', 'yes']:
            print("Skipping FastLED examples installation.")
            return False
    elif not force:
        print("No existing Arduino content found.")
        print("Would you like to install FastLED examples? [y/n]")
        
        response = input().strip().lower()
        if response not in ['y', 'yes']:
            print("Skipping FastLED examples installation.")
            return False
    
    print("📦 Installing FastLED examples...")
    
    # Download examples from FastLED repository
    examples_url = "https://api.github.com/repos/fastled/fastled/contents/examples"
    
    try:
        import requests
        response = requests.get(examples_url)
        response.raise_for_status()
        examples_data = response.json()
        
        # Create examples directory
        examples_dir = Path("examples")
        examples_dir.mkdir(exist_ok=True)
        
        installed_count = 0
        for item in examples_data:
            if item["type"] == "dir":
                example_name = item["name"]
                download_example_folder(example_name, examples_dir / example_name)
                installed_count += 1
                print(f"✅ Installed example: {example_name}")
        
        # Also create a simple Blink example in root for quick testing
        create_simple_blink_example()
        
        print(f"🎉 Successfully installed {installed_count} FastLED examples!")
        print("📁 Examples available in ./examples/ directory")
        print("🚀 Quick start: Open Blink.ino and press F5 to debug")
        
        return True
        
    except Exception as e:
        print(f"⚠️  Warning: Failed to install examples: {e}")
        print("You can manually download examples from: https://github.com/FastLED/FastLED/tree/main/examples")
        return False

def download_example_folder(example_name, target_dir):
    """Download a specific example folder from GitHub"""
    import requests
    
    folder_url = f"https://api.github.com/repos/fastled/fastled/contents/examples/{example_name}"
    
    try:
        response = requests.get(folder_url)
        response.raise_for_status()
        folder_data = response.json()
        
        # Create target directory
        target_dir.mkdir(parents=True, exist_ok=True)
        
        for item in folder_data:
            if item["type"] == "file":
                file_response = requests.get(item["download_url"])
                file_response.raise_for_status()
                
                file_path = target_dir / item["name"]
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(file_response.text)
            elif item["type"] == "dir":
                # Recursively download subdirectories
                download_example_folder(f"{example_name}/{item['name']}", target_dir / item["name"])
    
    except Exception as e:
        print(f"⚠️  Warning: Failed to download {example_name}: {e}")

def create_simple_blink_example():
    """Create a simple Blink.ino example in the project root"""
    blink_content = """// FastLED Blink Example
// A simple example to get started with FastLED

#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    50
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

void setup() {
    delay(3000); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
    // Turn the LED on, then pause
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    
    // Now turn the LED off, then pause
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(500);
}
"""
    
    with open("Blink.ino", 'w') as f:
        f.write(blink_content)
    
    print("📝 Created Blink.ino in project root for quick testing")

def generate_fastled_tasks():
    """Generate or update .vscode/tasks.json with FastLED build tasks"""
    tasks_config = {
        "version": "2.0.0",
        "tasks": [
            {
                "type": "shell",
                "label": "Run FastLED (Debug)",
                "command": "fastled",
                "args": ["${file}", "--debug"],
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
                "detail": "Run FastLED web compiler with debug mode on the current .ino file",
                "problemMatcher": []
            },
            {
                "type": "shell",
                "label": "Run FastLED (Quick)",
                "command": "fastled",
                "args": ["${file}", "--background-update"],
                "options": {"cwd": "${workspaceFolder}"},
                "group": "build",
                "presentation": {
                    "echo": True,
                    "reveal": "always",
                    "focus": True,
                    "panel": "new",
                    "showReuseMessage": False,
                    "clear": True
                },
                "detail": "Run FastLED web compiler with background updates on the current .ino file",
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
    
    # Check if tasks.json already exists
    tasks_path = ".vscode/tasks.json"
    if os.path.exists(tasks_path):
        # Load existing tasks and merge
        try:
            with open(tasks_path, 'r') as f:
                existing_tasks = json.load(f)
            
            # Remove any existing FastLED tasks to avoid duplicates
            if "tasks" in existing_tasks:
                existing_tasks["tasks"] = [
                    task for task in existing_tasks["tasks"]
                    if not (task.get("label", "").startswith("Run FastLED") or 
                           task.get("label") == "Install FastLED Package")
                ]
                # Add new FastLED tasks
                existing_tasks["tasks"].extend(tasks_config["tasks"])
                tasks_config = existing_tasks
            
        except (json.JSONDecodeError, KeyError):
            # If existing file is malformed, use new config
            pass
    
    with open(tasks_path, 'w') as f:
        json.dump(tasks_config, f, indent=4)
    
    print("📁 Generated/Updated .vscode/tasks.json - FastLED build tasks")
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
def fastled_install(dry_run=False):
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
        
        # Prompt for Auto Debug extension installation
        if not dry_run:
            print("Would you like to install the plugin for FastLED (auto-debug)? [y/n]")
            response = input().strip().lower()
        else:
            response = 'yes'  # Default to yes for dry-run testing
        
        installed_count = 0
        if response in ['y', 'yes']:
            if dry_run:
                print("[DRY-RUN]: NO PLUGIN INSTALLED")
                installed_count = 1  # Simulate successful installation for testing
            else:
                # Download and install Auto Debug extension
                extension_path = download_auto_debug_extension()
                installed_count = install_vscode_extensions(extension_path)
        else:
            print("Skipping Auto Debug extension installation.")
        
        # Update launch.json for Arduino debugging
        update_launch_json_for_arduino()
        print("✅ Arduino debugging configuration added to .vscode/launch.json")
        
        # Generate FastLED build tasks
        generate_fastled_tasks()
        
        # Check if we should install examples (for existing projects)
        if not check_existing_arduino_content():
            install_fastled_examples()
        
        # Full FastLED setup only if this is a FastLED project
        if is_fastled_project:
            setup_fastled_development_environment()
            update_vscode_settings_for_fastled()
            
            print("\n🎉 Full FastLED development environment installation complete!")
            print("\nTo use:")
            print("  - Run tests: bash test")
            print("  - Run linting: bash lint (Python, C++, and JavaScript)")
            print("  - Debug in VSCode: Open test file and press F5")
            if response in ['y', 'yes'] and installed_count > 0:
                print("  - Auto Debug: Use '🎯 Auto Debug (Smart File Detection)' configuration")
            print("  - clangd IntelliSense: Should work automatically in VSCode")
        else:
            print("\n🎉 Arduino debugging installation complete!")
            print("\nTo use:")
            print("  - Debug .ino files: Open any .ino file and press F5")
            if response in ['y', 'yes'] and installed_count > 0:
                print("  - Auto Debug: Use '🎯 Auto Debug (Smart File Detection)' configuration")
        
        # Installation summary
        if response in ['y', 'yes']:
            if installed_count == 0:
                print("\n⚠️  Warning: Auto Debug extension could not be installed automatically.")
                print("      Please install manually:")
                print("      - Open VSCode/Cursor")
                print("      - Go to Extensions (Ctrl+Shift+X)")
                print("      - Click ... menu → Install from VSIX")
                print(f"      - Select: {extension_path}")
            elif installed_count == 1:
                print(f"\n✅ Auto Debug extension installed on 1 editor.")
            else:
                print(f"\n✅ Auto Debug extension installed on {installed_count} editors!")
        else:
            print("\nℹ️  Auto Debug extension was not installed. You can install it later by running fastled --install again.")
        
        # Post-installation auto-execution
        if not dry_run:
            auto_execute_fastled()
        
        return True

def auto_execute_fastled():
    """Auto-execute fastled if Arduino content is present after installation"""
    import sys
    
    # Check if we have Arduino content to work with
    has_arduino_content = check_existing_arduino_content()
    
    # Also check if we just installed examples (Blink.ino should exist)
    blink_exists = os.path.exists("Blink.ino")
    
    if has_arduino_content or blink_exists:
        print("\n🚀 Arduino content detected - launching FastLED...")
        print("=" * 60)
        
        # Remove --install and other install-specific arguments from sys.argv
        filtered_argv = []
        skip_next = False
        
        for i, arg in enumerate(sys.argv):
            if skip_next:
                skip_next = False
                continue
                
            if arg in ['--install', '--dry-run']:
                continue
            elif arg.startswith('--install') or arg.startswith('--dry-run'):
                continue
            else:
                filtered_argv.append(arg)
        
        # If no file argument provided, use current directory
        if len(filtered_argv) == 1:  # Only script name
            filtered_argv.append('.')
        
        # Update sys.argv for main() call
        original_argv = sys.argv[:]
        sys.argv = filtered_argv
        
        try:
            # Import and call the main FastLED function
            # This is equivalent to running: fastled .
            from fastled import main
            main()
        except ImportError:
            print("⚠️  Warning: FastLED package not found. Please install it first:")
            print("    pip install fastled")
        except Exception as e:
            print(f"⚠️  Warning: Failed to auto-launch FastLED: {e}")
            print("You can manually run: fastled .")
        finally:
            # Restore original argv
            sys.argv = original_argv
    else:
        print("\nℹ️  No Arduino content found. Create a .ino file and run 'fastled <filename>' to get started!")
        
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

| Project Type | `.vscode/` here | `.vscode/` found up | IDE Available | `library.json` has "FastLED" | Has .ino/examples | Installation Behavior |
|--------------|-----------------|-------------------|---------------|------------------------------|------------------|----------------------|
| VSCode Project | ✅ | N/A | ✅ | ❌ | ✅ | **Basic**: Prompt for auto-debug → Arduino debugging → Auto-launch FastLED |
| VSCode Project | ✅ | N/A | ✅ | ❌ | ❌ | **Basic + Examples**: Prompt for auto-debug + examples → Auto-launch FastLED |
| FastLED Project | ✅ | N/A | ✅ | ✅ | ✅ | **Full**: Prompt for auto-debug → Full dev environment → Auto-launch FastLED |
| FastLED Project | ✅ | N/A | ✅ | ✅ | ❌ | **Full + Examples**: Prompt for auto-debug + examples + dev env → Auto-launch FastLED |
| Parent VSCode | ❌ | ✅ | ✅ | N/A | N/A | **Prompt**: "Found .vscode in `<path>`, install there?" → cd + install → Auto-launch if content |
| New Project | ❌ | ❌ | ✅ | N/A | N/A | **Generate**: VSCode project + auto-install examples + prompt auto-debug → Auto-launch FastLED |
| No IDE | ❌ | ❌ | ❌ | N/A | N/A | **Error**: "No supported IDE found" |

**Search Range**: Up to 5 parent directories  
**IDE Available**: Either `code` (VSCode) or `cursor` (Cursor) command exists  
**Auto-launch FastLED**: Automatically runs `fastled .` if `.ino` files or examples are present (skipped in dry-run mode)

## Configuration Changes Summary

### All Projects (Basic Install)
- **Auto Debug Extension** (prompted):
  - Prompt: "Would you like to install the plugin for FastLED (auto-debug)? [y/n]"
  - If yes: Download and install `DarrenLevine.auto-debug-1.0.2.vsix`
  - Support both VSCode (`code`) and Cursor (`cursor`) commands
- **`.vscode/launch.json`**: 
  - Add/update Auto Debug configuration with `"*.ino": "Arduino: Run .ino with FastLED"`
  - Remove examples-specific paths (`"examples/**/*.ino"`)
  - Add Arduino debug configuration if missing
- **`.vscode/tasks.json`**: 
  - Add "Run FastLED (Debug)" task with `--debug` flag
  - Add "Run FastLED (Quick)" task with `--background-update` flag
  - Merge with existing tasks (replace duplicates)
  - Set Debug task as default build task
- **Examples Installation** (conditional):
  - If NO `.ino` files AND NO `examples/` folder: Prompt to install FastLED examples
  - Downloads all examples from FastLED repository into `./examples/` directory
  - Creates `Blink.ino` in project root for quick testing

### New Projects (Generate VSCode Project)
- **Complete VSCode Setup**:
  - Generate `.vscode/launch.json`, `.vscode/tasks.json`, `.vscode/settings.json`, `.vscode/extensions.json`
  - Full Arduino debugging configuration
- **Automatic Examples Installation**:
  - Downloads ALL FastLED examples automatically (no prompt)
  - Creates complete `./examples/` directory structure
  - Creates `Blink.ino` in project root for immediate testing

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
6. ✅ Automatically installs FastLED examples for new projects
7. ✅ Detects existing Arduino content (.ino files or examples/ folder)
8. ✅ Prompts for examples installation only when no existing content found
9. ✅ Downloads complete FastLED examples from GitHub repository
10. ✅ Creates quick-start Blink.ino example in project root
11. ✅ Correctly detects FastLED vs non-FastLED projects
12. ✅ Prompts user before installing Auto Debug extension
13. ✅ Downloads and installs Auto Debug extension only if user consents
14. ✅ Updates `.vscode/launch.json` for Arduino debugging
15. ✅ Supports `.ino` files anywhere in project (not just examples/)
16. ✅ Conditional full setup for FastLED projects only
17. ✅ Auto-executes FastLED when Arduino content is present after installation
18. ✅ Skips auto-execution in dry-run mode for testing
19. ✅ Properly handles argument filtering for auto-execution
20. ✅ Provides clear feedback and manual fallback instructions
21. ✅ Handles all error conditions gracefully

## Testing Requirements

### Unit Test Implementation

**Test Function**: `test_fastled_install_dry_run()`

```python
import tempfile
import os
import json
from pathlib import Path

def test_fastled_install_dry_run():
    """Test fastled --install in dry-run mode"""
    with tempfile.TemporaryDirectory() as temp_dir:
        # Change to temporary directory
        original_cwd = os.getcwd()
        os.chdir(temp_dir)
        
        try:
            # Run installation in dry-run mode
            result = fastled_install(dry_run=True)
            
            # Validate successful execution
            assert result == True, "Installation should succeed in dry-run mode"
            
            # Validate .vscode directory was created
            vscode_dir = Path(".vscode")
            assert vscode_dir.exists(), ".vscode directory should be created"
            assert vscode_dir.is_dir(), ".vscode should be a directory"
            
            # Validate launch.json
            launch_json = vscode_dir / "launch.json"
            assert launch_json.exists(), "launch.json should be created"
            
            with open(launch_json, 'r') as f:
                launch_config = json.load(f)
            
            assert "configurations" in launch_config, "launch.json should have configurations"
            assert len(launch_config["configurations"]) > 0, "Should have at least one configuration"
            
            # Find Auto Debug configuration
            auto_debug_config = None
            for config in launch_config["configurations"]:
                if config.get("name") == "🎯 Auto Debug (Smart File Detection)":
                    auto_debug_config = config
                    break
            
            assert auto_debug_config is not None, "Should have Auto Debug configuration"
            assert "*.ino" in auto_debug_config.get("map", {}), "Should map *.ino files"
            assert auto_debug_config["map"]["*.ino"] == "Arduino: Run .ino with FastLED", "Should map to Arduino config"
            
            # Validate Arduino debug configuration exists
            arduino_config_exists = any(
                config.get("name") == "Arduino: Run .ino with FastLED" 
                for config in launch_config["configurations"]
            )
            assert arduino_config_exists, "Should have Arduino debug configuration"
            
            # Validate tasks.json
            tasks_json = vscode_dir / "tasks.json"
            assert tasks_json.exists(), "tasks.json should be created"
            
            with open(tasks_json, 'r') as f:
                tasks_config = json.load(f)
            
            assert "tasks" in tasks_config, "tasks.json should have tasks array"
            
            task_labels = [task.get("label", "") for task in tasks_config["tasks"]]
            assert "Run FastLED (Debug)" in task_labels, "Should have Debug task"
            assert "Run FastLED (Quick)" in task_labels, "Should have Quick task"
            
            # Validate Debug task configuration
            debug_task = next(task for task in tasks_config["tasks"] if task.get("label") == "Run FastLED (Debug)")
            assert "--debug" in debug_task.get("args", []), "Debug task should have --debug flag"
            
            # Validate Quick task configuration
            quick_task = next(task for task in tasks_config["tasks"] if task.get("label") == "Run FastLED (Quick)")
            assert "--background-update" in quick_task.get("args", []), "Quick task should have --background-update flag"
            
            # Validate settings.json
            settings_json = vscode_dir / "settings.json"
            assert settings_json.exists(), "settings.json should be created"
            
            with open(settings_json, 'r') as f:
                settings_config = json.load(f)
            
            assert "files.associations" in settings_config, "Should have file associations"
            assert settings_config["files.associations"].get("*.ino") == "cpp", "Should associate .ino with cpp"
            
            # Validate extensions.json
            extensions_json = vscode_dir / "extensions.json"
            assert extensions_json.exists(), "extensions.json should be created"
            
            with open(extensions_json, 'r') as f:
                extensions_config = json.load(f)
            
            assert "recommendations" in extensions_config, "Should have extension recommendations"
            assert "ms-python.python" in extensions_config["recommendations"], "Should recommend Python extension"
            
            # Validate Blink.ino example was created
            blink_ino = Path("Blink.ino")
            assert blink_ino.exists(), "Blink.ino should be created"
            
            with open(blink_ino, 'r') as f:
                blink_content = f.read()
            
            assert "#include <FastLED.h>" in blink_content, "Blink.ino should include FastLED"
            assert "CRGB leds" in blink_content, "Blink.ino should declare LED array"
            
            print("✅ All dry-run validations passed!")
            
        finally:
            # Restore original directory
            os.chdir(original_cwd)

def test_fastled_install_auto_execution():
    """Test auto-execution after installation"""
    import unittest.mock
    
    with tempfile.TemporaryDirectory() as temp_dir:
        original_cwd = os.getcwd()
        os.chdir(temp_dir)
        
        try:
            # Mock the main function to verify it gets called
            with unittest.mock.patch('fastled.main') as mock_main:
                with unittest.mock.patch('sys.argv', ['fastled', '--install']):
                    # Run installation (not dry-run)
                    result = fastled_install(dry_run=False)
                    assert result == True, "Installation should succeed"
                    
                    # Verify auto-execution was attempted
                    mock_main.assert_called_once(), "FastLED main() should be called after installation"
            
            print("✅ Auto-execution validation passed!")
            
        finally:
            os.chdir(original_cwd)

def test_fastled_install_no_auto_execution_dry_run():
    """Test that auto-execution is skipped in dry-run mode"""
    import unittest.mock
    
    with tempfile.TemporaryDirectory() as temp_dir:
        original_cwd = os.getcwd()
        os.chdir(temp_dir)
        
        try:
            # Mock the main function to verify it does NOT get called
            with unittest.mock.patch('fastled.main') as mock_main:
                with unittest.mock.patch('sys.argv', ['fastled', '--install', '--dry-run']):
                    # Run installation in dry-run mode
                    result = fastled_install(dry_run=True)
                    assert result == True, "Installation should succeed in dry-run"
                    
                    # Verify auto-execution was NOT attempted
                    mock_main.assert_not_called(), "FastLED main() should NOT be called in dry-run mode"
            
            print("✅ Dry-run auto-execution skip validation passed!")
            
        finally:
            os.chdir(original_cwd)

def test_fastled_install_existing_vscode_project():
    """Test installation in existing VSCode project"""
    with tempfile.TemporaryDirectory() as temp_dir:
        original_cwd = os.getcwd()
        os.chdir(temp_dir)
        
        try:
            # Create existing .vscode directory with basic launch.json
            vscode_dir = Path(".vscode")
            vscode_dir.mkdir()
            
            existing_launch = {
                "version": "0.2.0",
                "configurations": [
                    {
                        "name": "Existing Config",
                        "type": "python",
                        "request": "launch",
                        "program": "${file}"
                    }
                ]
            }
            
            with open(vscode_dir / "launch.json", 'w') as f:
                json.dump(existing_launch, f, indent=4)
            
            # Run installation in dry-run mode
            result = fastled_install(dry_run=True)
            assert result == True, "Installation should succeed"
            
            # Validate existing configuration was preserved
            with open(vscode_dir / "launch.json", 'r') as f:
                updated_launch = json.load(f)
            
            config_names = [config.get("name") for config in updated_launch["configurations"]]
            assert "Existing Config" in config_names, "Should preserve existing configurations"
            assert "🎯 Auto Debug (Smart File Detection)" in config_names, "Should add Auto Debug config"
            
            print("✅ Existing project validation passed!")
            
        finally:
            os.chdir(original_cwd)

def test_fastled_install_tasks_merging():
    """Test tasks.json merging with existing tasks"""
    with tempfile.TemporaryDirectory() as temp_dir:
        original_cwd = os.getcwd()
        os.chdir(temp_dir)
        
        try:
            # Create .vscode directory with existing tasks
            vscode_dir = Path(".vscode")
            vscode_dir.mkdir()
            
            existing_tasks = {
                "version": "2.0.0",
                "tasks": [
                    {
                        "label": "Existing Task",
                        "type": "shell",
                        "command": "echo",
                        "args": ["hello"]
                    },
                    {
                        "label": "Run FastLED (Debug)",  # This should be replaced
                        "type": "shell",
                        "command": "old-command"
                    }
                ]
            }
            
            with open(vscode_dir / "tasks.json", 'w') as f:
                json.dump(existing_tasks, f, indent=4)
            
            # Generate FastLED tasks
            generate_fastled_tasks()
            
            # Validate tasks were merged correctly
            with open(vscode_dir / "tasks.json", 'r') as f:
                updated_tasks = json.load(f)
            
            task_labels = [task.get("label") for task in updated_tasks["tasks"]]
            assert "Existing Task" in task_labels, "Should preserve existing tasks"
            assert "Run FastLED (Debug)" in task_labels, "Should have FastLED Debug task"
            assert "Run FastLED (Quick)" in task_labels, "Should have FastLED Quick task"
            
            # Ensure old FastLED tasks were replaced
            debug_tasks = [task for task in updated_tasks["tasks"] if task.get("label") == "Run FastLED (Debug)"]
            assert len(debug_tasks) == 1, "Should have only one FastLED Debug task"
            assert debug_tasks[0].get("command") == "fastled", "Should use new fastled command"
            
            print("✅ Tasks merging validation passed!")
            
        finally:
            os.chdir(original_cwd)
```

### Dry-Run Mode Implementation

**Required Command Line Interface:**
```bash
fastled --install --dry-run
```

**Expected Output:**
```
✅ VSCode project detected
📦 General project detected - installing Arduino debugging support only
[DRY-RUN]: NO PLUGIN INSTALLED
✅ Arduino debugging configuration added to .vscode/launch.json
📁 Generated/Updated .vscode/tasks.json - FastLED build tasks
📦 Installing FastLED examples...
✅ Installed example: Blink
...
🎉 Arduino debugging installation complete!
```

### Validation Requirements

1. **JSON Validity**: All generated `.vscode/*.json` files must be valid JSON
2. **Configuration Completeness**: All required configurations must be present
3. **Task Functionality**: Tasks must have correct commands and arguments
4. **File Associations**: Proper file type associations must be set
5. **Extension Recommendations**: Required extensions must be listed
6. **Example Content**: Generated example files must be syntactically correct

## Integration Notes

- Add this functionality to the existing fastled-wasm CLI interface
- Follow existing command patterns and error handling
- Ensure compatibility with existing fastled commands
- Maintain consistency with current FastLED development workflow
- Implement comprehensive unit testing with dry-run mode
- Validate all generated configurations in isolated test environments