"""
FastLED Build Support Script for PlatformIO

This script provides build_info.json generation and environment variable handling
for the enhanced platformio.ini file system.
"""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, Protocol

# ruff: noqa: F821
# pyright: reportUndefinedVariable=false
try:
    Import("env")  # type: ignore # Import is provided by PlatformIO
    PLATFORMIO_ENV_AVAILABLE = True
except:
    PLATFORMIO_ENV_AVAILABLE = False


class PlatformIOEnv(Protocol):
    """Type information for PlatformIO environment."""

    def get(self, key: str, default: str | None = None) -> str | None:
        """Get a value from the environment."""
        ...

    def Append(self, **kwargs: Any) -> None:
        """Append to environment variables."""
        ...

    def Replace(self, **kwargs: Any) -> None:
        """Replace environment variables."""
        ...


def setup_build_environment() -> None:
    """Set up environment variables for PlatformIO builds based on external settings."""
    
    # Set up optimization report flag
    optimization_report = os.getenv("FASTLED_OPTIMIZATION_REPORT", "").lower()
    if optimization_report in ["1", "true", "yes", "on"]:
        os.environ["FASTLED_OPTIMIZATION_REPORT_FLAG"] = "-fopt-info-all=optimization_report.txt"
    else:
        os.environ["FASTLED_OPTIMIZATION_REPORT_FLAG"] = ""
    
    # Set up custom defines
    custom_defines = os.getenv("FASTLED_CUSTOM_DEFINES", "")
    if custom_defines:
        # Convert comma-separated defines to build flags
        defines_list = [f"-D{define.strip()}" for define in custom_defines.split(",") if define.strip()]
        os.environ["FASTLED_CUSTOM_DEFINES_FLAGS"] = " ".join(defines_list)
    else:
        os.environ["FASTLED_CUSTOM_DEFINES_FLAGS"] = ""
    
    # Set up build directory
    build_dir = os.getenv("FASTLED_BUILD_DIR", "")
    if build_dir:
        os.environ["FASTLED_BUILD_DIR"] = build_dir
    else:
        os.environ["FASTLED_BUILD_DIR"] = ".pio/build"
    
    # Set up build info script
    generate_build_info = os.getenv("FASTLED_GENERATE_BUILD_INFO", "").lower()
    if generate_build_info in ["1", "true", "yes", "on"]:
        # Point to this script's build info generation function
        script_path = Path(__file__).absolute()
        os.environ["FASTLED_BUILD_INFO_SCRIPT"] = f"post:{script_path}"
    else:
        os.environ["FASTLED_BUILD_INFO_SCRIPT"] = ""


def insert_tool_aliases(data: Dict[str, Any]) -> None:
    """Insert tool aliases into build metadata, similar to create_build_dir.py"""
    
    # Find gcc tools in the build environment
    for env_name, env_data in data.items():
        if not isinstance(env_data, dict):
            continue
            
        # Get compiler paths from PlatformIO metadata
        cc_path = env_data.get("cc_path", "")
        cxx_path = env_data.get("cxx_path", "")
        
        if cc_path or cxx_path:
            # Extract toolchain prefix (e.g., "xtensa-esp32-elf-" from "xtensa-esp32-elf-gcc")
            toolchain_prefix = ""
            if cc_path:
                cc_name = Path(cc_path).name
                if cc_name.endswith("-gcc"):
                    toolchain_prefix = cc_name[:-3]  # Remove "gcc"
                elif cc_name == "gcc":
                    toolchain_prefix = ""
            
            # Create tool aliases
            tool_aliases = {}
            if toolchain_prefix:
                tool_aliases.update({
                    "gcc": f"{toolchain_prefix}gcc",
                    "g++": f"{toolchain_prefix}g++", 
                    "ar": f"{toolchain_prefix}ar",
                    "objcopy": f"{toolchain_prefix}objcopy",
                    "objdump": f"{toolchain_prefix}objdump", 
                    "nm": f"{toolchain_prefix}nm",
                    "size": f"{toolchain_prefix}size",
                    "strip": f"{toolchain_prefix}strip",
                    "gdb": f"{toolchain_prefix}gdb",
                })
            else:
                # Standard tools without prefix
                tool_aliases.update({
                    "gcc": "gcc",
                    "g++": "g++",
                    "ar": "ar", 
                    "objcopy": "objcopy",
                    "objdump": "objdump",
                    "nm": "nm",
                    "size": "size",
                    "strip": "strip",
                    "gdb": "gdb",
                })
            
            # Add tool aliases to environment data
            env_data["tool_aliases"] = tool_aliases


def generate_build_info_json(env: PlatformIOEnv) -> None:  # type: ignore
    """Generate build_info.json file for the current build environment."""
    
    if not PLATFORMIO_ENV_AVAILABLE:
        print("PlatformIO environment not available, skipping build_info.json generation")
        return
    
    try:
        # Get build information from PlatformIO environment
        project_dir = env.get("PROJECT_DIR", os.getcwd())
        board = env.get("BOARD", "unknown")
        platform = env.get("PIOPLATFORM", "unknown")
        framework = env.get("PIOFRAMEWORK", "unknown")
        
        # Get build directory
        build_dir = env.get("BUILD_DIR", os.path.join(project_dir, ".pio", "build", board))
        
        # Create a temporary project to get detailed metadata
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_project = Path(temp_dir) / "temp_project"
            temp_project.mkdir(parents=True, exist_ok=True)
            
            # Get platform metadata using pio project metadata
            metadata_cmd = ["pio", "project", "metadata", "--json-output"]
            
            try:
                result = subprocess.run(
                    metadata_cmd,
                    capture_output=True,
                    text=True,
                    cwd=project_dir,
                    timeout=60,
                )
                
                if result.returncode == 0:
                    data = json.loads(result.stdout)
                    
                    # Add tool aliases
                    insert_tool_aliases(data)
                    
                    # Save build_info.json in the build directory
                    build_info_path = Path(build_dir) / "build_info.json"
                    build_info_path.parent.mkdir(parents=True, exist_ok=True)
                    
                    with open(build_info_path, "w") as f:
                        json.dump(data, f, indent=4, sort_keys=True)
                    
                    print(f"Generated build_info.json at: {build_info_path}")
                    
                else:
                    print(f"Failed to get project metadata: {result.stderr}")
                    
            except subprocess.TimeoutExpired:
                print("Timeout getting project metadata for build_info.json")
            except json.JSONDecodeError as e:
                print(f"Failed to parse project metadata JSON: {e}")
            except Exception as e:
                print(f"Exception generating build_info.json: {e}")
                
    except Exception as e:
        print(f"Error in build_info.json generation: {e}")


def main() -> None:
    """Main entry point for command-line usage."""
    
    if len(sys.argv) > 1 and sys.argv[1] == "setup_env":
        # Set up environment variables
        setup_build_environment()
        print("FastLED build environment variables set up")
        
    elif len(sys.argv) > 1 and sys.argv[1] == "generate_build_info":
        # Generate build_info.json (requires PlatformIO environment)
        if PLATFORMIO_ENV_AVAILABLE:
            generate_build_info_json(env)  # type: ignore
        else:
            print("PlatformIO environment not available")
            
    else:
        print("Usage: python fastled_build_support.py [setup_env|generate_build_info]")


# Set up environment variables when imported
setup_build_environment()

# If this script is called as a PlatformIO extra_script, generate build_info.json
if PLATFORMIO_ENV_AVAILABLE:
    # This runs when the script is called as a PlatformIO extra_script
    generate_build_info_json(env)  # type: ignore


if __name__ == "__main__":
    main()
