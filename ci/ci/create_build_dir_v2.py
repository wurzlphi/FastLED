"""
Optimized build directory creation with better path handling and incremental build support.
"""
import json
import os
import shutil
import subprocess
import time
import warnings
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from ci.boards import Board  # type: ignore
from ci.locked_print import locked_print


def _install_global_package(package: str) -> None:
    """Install a global PlatformIO package."""
    locked_print(f"*** Installing global package: {package} ***")
    cmd_list = [
        "pio", "pkg", "install", "-g", "-p", package,
    ]
    cmd_str = subprocess.list2cmdline(cmd_list)
    locked_print(f"Running command:\n\n{cmd_str}\n\n")
    
    result = subprocess.run(
        cmd_str,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=True,
    )
    locked_print(result.stdout)
    locked_print(f"*** Finished installing {package} ***")


def insert_tool_aliases(meta_json: Dict[str, Dict]) -> None:
    """Insert toolchain aliases into the metadata JSON."""
    for board in meta_json.keys():
        aliases: Dict[str, Optional[str]] = {}
        cc_path = meta_json[board].get("cc_path")
        cc_path = Path(cc_path) if cc_path else None
        
        if cc_path and cc_path.exists():
            # Get the prefix of the base name of the compiler
            cc_base = cc_path.name
            parent = cc_path.parent
            prefix = cc_base.split("gcc")[0] if "gcc" in cc_base else cc_base.split("cc")[0]
            suffix = cc_path.suffix
            
            # Create the aliases for common toolchain tools
            for tool in [
                "gcc", "g++", "ar", "objcopy", "objdump", "size", "nm", 
                "ld", "as", "ranlib", "strip", "c++filt", "readelf", "addr2line",
            ]:
                name = f"{prefix}{tool}{suffix}"
                tool_path = parent / name
                if tool_path.exists():
                    aliases[tool] = str(tool_path)
                else:
                    aliases[tool] = None
        
        meta_json[board]["aliases"] = aliases


def remove_readonly(func, path, _):
    """Clear the readonly bit and reattempt the removal."""
    if os.name == "nt":
        os.system(f"attrib -r {path}")
    else:
        try:
            os.chmod(path, 0o777)
        except Exception:
            locked_print(f"Error removing readonly attribute from {path}")
    func(path)


def robust_rmtree(path: Path, max_retries: int = 5, delay: float = 0.1) -> bool:
    """
    Robustly remove a directory tree, handling race conditions and concurrent access.
    
    Args:
        path: Path to the directory to remove
        max_retries: Maximum number of retry attempts
        delay: Delay between retries in seconds
    
    Returns:
        True if removal was successful, False otherwise
    """
    if not path.exists():
        locked_print(f"Directory {path} doesn't exist, skipping removal")
        return True
    
    for attempt in range(max_retries):
        try:
            locked_print(f"Attempting to remove directory {path} (attempt {attempt + 1}/{max_retries})")
            shutil.rmtree(path, onerror=remove_readonly)
            locked_print(f"Successfully removed directory {path}")
            return True
        except OSError as e:
            if attempt == max_retries - 1:
                locked_print(f"Failed to remove directory {path} after {max_retries} attempts: {e}")
                return False
            
            locked_print(f"Failed to remove directory {path} on attempt {attempt + 1}: {e}")
            
            # Check if another process removed it
            if not path.exists():
                locked_print(f"Directory {path} was removed by another process")
                return True
            
            # Wait before retrying with exponential backoff
            time.sleep(delay * (2**attempt))
        
        except Exception as e:
            locked_print(f"Unexpected error removing directory {path}: {e}")
            return False
    
    return False


def safe_file_removal(file_path: Path, max_retries: int = 3) -> bool:
    """
    Safely remove a file with retry logic.
    
    Args:
        file_path: Path to the file to remove
        max_retries: Maximum number of retry attempts
    
    Returns:
        True if removal was successful, False otherwise
    """
    if not file_path.exists():
        return True
    
    for attempt in range(max_retries):
        try:
            file_path.unlink()
            locked_print(f"Successfully removed file {file_path}")
            return True
        except OSError as e:
            if attempt == max_retries - 1:
                locked_print(f"Failed to remove file {file_path} after {max_retries} attempts: {e}")
                return False
            
            locked_print(f"Failed to remove file {file_path} on attempt {attempt + 1}: {e}")
            
            # Check if another process removed it
            if not file_path.exists():
                locked_print(f"File {file_path} was removed by another process")
                return True
            
            time.sleep(0.1 * (attempt + 1))
        
        except Exception as e:
            locked_print(f"Unexpected error removing file {file_path}: {e}")
            return False
    
    return False


def normalize_build_paths(build_dir: Optional[str], board_name: str) -> Tuple[Path, Path]:
    """
    Normalize and create build directory paths.
    
    Returns:
        (builddir, srcdir) - Absolute paths to build and source directories
    """
    build_root = Path(build_dir).resolve() if build_dir else Path(".build").resolve()
    builddir = build_root / board_name
    
    # Ensure paths are absolute and normalized
    builddir = builddir.resolve()
    
    return builddir, builddir / "src"


def create_build_dir(
    board: Board,
    defines: List[str],
    customsdk: Optional[str],
    no_install_deps: bool,
    extra_packages: List[str],
    build_dir: Optional[str],
    board_dir: Optional[str],
    build_flags: Optional[List[str]],
    extra_scripts: Optional[str],
) -> Tuple[bool, str]:
    """Create optimized build directory for the given board."""
    import threading
    
    # Skip web board as it's not a real compilation target
    if board.board_name == "web":
        locked_print(f"Skipping web target for board {board.board_name}")
        return True, ""
    
    # Add board-specific defines
    if board.defines:
        defines.extend(board.defines)
        defines = list(set(defines))  # Remove duplicates
    
    board_name = board.board_name
    real_board_name = board.get_real_board_name()
    thread_id = threading.current_thread().ident
    
    locked_print(f"*** [Thread {thread_id}] Initializing environment for {board_name} ***")
    
    # Normalize paths
    builddir, _ = normalize_build_paths(build_dir, board_name)
    
    locked_print(f"[Thread {thread_id}] Creating build directory: {builddir}")
    try:
        builddir.mkdir(parents=True, exist_ok=True)
        locked_print(f"[Thread {thread_id}] Successfully created build directory: {builddir}")
    except Exception as e:
        locked_print(f"[Thread {thread_id}] Error creating build directory {builddir}: {e}")
        return False, f"Failed to create build directory: {e}"
    
    # Clean lib directory (FastLED source) for updates - this is necessary for incremental builds
    libdir = builddir / "lib"
    if libdir.exists():
        locked_print(f"[Thread {thread_id}] Removing existing lib directory: {libdir}")
        if not robust_rmtree(libdir):
            locked_print(f"[Thread {thread_id}] Warning: Failed to remove lib directory {libdir}, continuing anyway")
    
    # Remove existing platformio.ini to ensure clean configuration
    platformio_ini = builddir / "platformio.ini"
    if platformio_ini.exists():
        locked_print(f"[Thread {thread_id}] Removing existing platformio.ini: {platformio_ini}")
        if not safe_file_removal(platformio_ini):
            locked_print(f"[Thread {thread_id}] Warning: Failed to remove {platformio_ini}, continuing anyway")
    
    # Handle board directory copying
    if board_dir:
        dst_dir = builddir / "boards"
        locked_print(f"[Thread {thread_id}] Processing board directory: {board_dir} -> {dst_dir}")
        
        if dst_dir.exists():
            locked_print(f"[Thread {thread_id}] Removing existing boards directory: {dst_dir}")
            if not robust_rmtree(dst_dir):
                locked_print(f"[Thread {thread_id}] Error: Failed to remove boards directory {dst_dir}")
                return False, f"Failed to remove existing boards directory {dst_dir}"
        
        try:
            locked_print(f"[Thread {thread_id}] Copying board directory: {board_dir} -> {dst_dir}")
            shutil.copytree(str(board_dir), str(dst_dir))
            locked_print(f"[Thread {thread_id}] Successfully copied board directory to {dst_dir}")
        except Exception as e:
            locked_print(f"[Thread {thread_id}] Error copying board directory: {e}")
            return False, f"Failed to copy board directory: {e}"
    
    # Install platform if needed
    if board.platform_needs_install and board.platform:
        try:
            _install_global_package(board.platform)
        except subprocess.CalledProcessError as e:
            return False, str(e.stdout) if e.stdout else str(e)
    elif board.platform_needs_install:
        warnings.warn("Platform install was specified but no platform was given.")
    
    # Build PlatformIO project initialization command
    cmd_list = [
        "pio", "project", "init",
        "--project-dir", str(builddir),
        "--board", real_board_name,
    ]
    
    # Add platform-specific options
    if board.platform:
        cmd_list.append(f"--project-option=platform={board.platform}")
    if board.platform_packages:
        cmd_list.append(f"--project-option=platform_packages={board.platform_packages}")
    if board.framework:
        cmd_list.append(f"--project-option=framework={board.framework}")
    if board.board_build_core:
        cmd_list.append(f"--project-option=board_build.core={board.board_build_core}")
    if board.board_build_filesystem_size:
        cmd_list.append(f"--project-option=board_build.filesystem_size={board.board_build_filesystem_size}")
    
    # Add build flags
    if build_flags:
        for build_flag in build_flags:
            cmd_list.append(f"--project-option=build_flags={build_flag}")
    
    # Add defines as build flags
    if defines:
        build_flags_str = " ".join(f"-D{define}" for define in defines)
        cmd_list.append(f"--project-option=build_flags={build_flags_str}")
    
    # Add SDK configuration
    if board.customsdk and customsdk:
        cmd_list.append(f"--project-option=custom_sdkconfig={customsdk}")
    
    # Add extra packages
    if extra_packages:
        cmd_list.append(f'--project-option=lib_deps={",".join(extra_packages)}')
    
    # Skip dependency installation if requested
    if no_install_deps:
        cmd_list.append("--no-install-dependencies")
    
    # Create enhanced CCACHE configuration script
    ccache_script = builddir / "ccache_config.py"
    _create_ccache_script(ccache_script, thread_id)
    
    # Setup build scripts
    project_root = Path.cwd()
    ci_flags_script = (project_root / "ci" / "ci-flags.py").resolve()
    ccache_script_abs = ccache_script.resolve()
    
    script_list = [
        f"pre:{ci_flags_script.as_posix()}",
        f"pre:{ccache_script_abs.as_posix()}",
    ]
    
    if extra_scripts:
        extra_scripts_path = Path(extra_scripts).resolve()
        if not str(extra_scripts_path).startswith("pre:"):
            extra_scripts_path_str = f"pre:{extra_scripts_path.as_posix()}"
        else:
            extra_scripts_path_str = extra_scripts
        script_list.append(extra_scripts_path_str)
    
    cmd_list.append(f"--project-option=extra_scripts=[{','.join(script_list)}]")
    
    # Execute PlatformIO project initialization
    cmd_str = subprocess.list2cmdline(cmd_list)
    locked_print(f"\n[Thread {thread_id}] Running command:\n  {cmd_str}\n")
    
    result = subprocess.run(
        cmd_str,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    
    stdout = result.stdout or ""
    locked_print(stdout)
    
    if result.returncode != 0:
        locked_print(f"*** [Thread {thread_id}] Error setting up board {board_name} ***")
        return False, stdout
    
    locked_print(f"*** [Thread {thread_id}] Finished initializing environment for board {board_name} ***")
    
    # Print platformio.ini contents for debugging
    _debug_print_platformio_ini(builddir / "platformio.ini", thread_id)
    
    # Generate build metadata
    success, metadata_output = _generate_build_metadata(builddir, thread_id)
    if not success:
        locked_print(f"[Thread {thread_id}] Warning: Failed to generate build metadata: {metadata_output}")
    
    return True, stdout


def _create_ccache_script(ccache_script: Path, thread_id: Optional[int] = None) -> None:
    """Create enhanced CCACHE configuration script."""
    if ccache_script.exists():
        return
    
    thread_str = f"[Thread {thread_id}] " if thread_id else ""
    locked_print(f"{thread_str}Creating CCACHE configuration script at {ccache_script}")
    
    ccache_content = '''"""Enhanced CCACHE configuration for PlatformIO builds."""

import os
import platform
import subprocess
from pathlib import Path

Import("env")

def is_ccache_available():
    """Check if ccache is available in the system."""
    try:
        subprocess.run(["ccache", "--version"], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def get_ccache_path():
    """Get the full path to ccache executable."""
    if platform.system() == "Windows":
        # Windows paths for ccache
        ccache_paths = [
            "C:\\\\ProgramData\\\\chocolatey\\\\bin\\\\ccache.exe",
            os.path.expanduser("~\\\\scoop\\\\shims\\\\ccache.exe"),
            "C:\\\\tools\\\\ccache\\\\ccache.exe",
        ]
        for path in ccache_paths:
            if os.path.exists(path):
                return path
    else:
        # Unix-like systems
        try:
            return subprocess.check_output(["which", "ccache"]).decode().strip()
        except subprocess.CalledProcessError:
            pass
    return None

def configure_ccache(env):
    """Configure CCACHE for the build environment."""
    if not is_ccache_available():
        print("CCACHE is not available. Skipping CCACHE configuration.")
        return

    ccache_path = get_ccache_path()
    if not ccache_path:
        print("Could not find CCACHE executable. Skipping CCACHE configuration.")
        return

    print(f"Found CCACHE at: {ccache_path}")

    # Set up CCACHE environment variables
    project_dir = env.get("PROJECT_DIR", os.getcwd())
    if "CCACHE_DIR" not in os.environ:
        ccache_dir = os.path.join(project_dir, ".ccache")
        os.environ["CCACHE_DIR"] = ccache_dir
        Path(ccache_dir).mkdir(parents=True, exist_ok=True)

    # Enhanced CCACHE configuration
    os.environ["CCACHE_BASEDIR"] = project_dir
    os.environ["CCACHE_COMPRESS"] = "true"
    os.environ["CCACHE_COMPRESSLEVEL"] = "6"
    os.environ["CCACHE_MAXSIZE"] = "1G"  # Increased cache size
    os.environ["CCACHE_SLOPPINESS"] = "time_macros,include_file_mtime"
    os.environ["CCACHE_NOHASHDIR"] = "true"

    # Wrap compiler commands with ccache
    original_cc = env.get("CC", "gcc")
    original_cxx = env.get("CXX", "g++")

    # Don't wrap if already wrapped
    if "ccache" not in str(original_cc):
        env.Replace(
            CC=f'"{ccache_path}" {original_cc}',
            CXX=f'"{ccache_path}" {original_cxx}',
        )
        print(f"Wrapped CC: {env.get('CC')}")
        print(f"Wrapped CXX: {env.get('CXX')}")

    # Show CCACHE stats (non-blocking)
    try:
        subprocess.run([ccache_path, "--show-stats"], check=False, timeout=5)
    except (subprocess.TimeoutExpired, Exception):
        pass

# Apply CCACHE configuration
configure_ccache(env)'''
    
    with open(ccache_script, 'w', encoding='utf-8') as f:
        f.write(ccache_content)


def _debug_print_platformio_ini(platformio_ini: Path, thread_id: Optional[int] = None) -> None:
    """Print platformio.ini contents for debugging."""
    thread_str = f"[Thread {thread_id}] " if thread_id else ""
    
    if platformio_ini.exists():
        locked_print(f"\n{thread_str}*** Contents of {platformio_ini} after initialization ***")
        try:
            with open(platformio_ini, 'r', encoding='utf-8') as f:
                ini_contents = f.read()
                locked_print(f"\n{ini_contents}\n")
        except Exception as e:
            locked_print(f"{thread_str}Error reading {platformio_ini}: {e}")
        locked_print(f"{thread_str}*** End of {platformio_ini} contents ***\n")
    else:
        locked_print(f"{thread_str}Warning: {platformio_ini} was not found after initialization")


def _generate_build_metadata(builddir: Path, thread_id: Optional[int] = None) -> Tuple[bool, str]:
    """Generate build metadata JSON file."""
    thread_str = f"[Thread {thread_id}] " if thread_id else ""
    
    cmd_list = ["pio", "project", "metadata", "--json-output"]
    cmd_str = subprocess.list2cmdline(cmd_list)
    
    try:
        result = subprocess.run(
            cmd_list,
            cwd=str(builddir),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
            timeout=60,  # 1 minute timeout
        )
        
        stdout = result.stdout or ""
        
        if result.returncode == 0:
            try:
                data = json.loads(stdout)
                insert_tool_aliases(data)
                
                metadata_json = builddir / "build_info.json"
                formatted = json.dumps(data, indent=4, sort_keys=True)
                with open(metadata_json, 'w', encoding='utf-8') as f:
                    f.write(formatted)
                
                locked_print(f"{thread_str}Generated build metadata: {metadata_json}")
                return True, "Metadata generated successfully"
                
            except json.JSONDecodeError as e:
                # Fallback: save raw stdout
                metadata_json = builddir / "build_info.json"
                with open(metadata_json, 'w', encoding='utf-8') as f:
                    f.write(stdout)
                
                locked_print(f"{thread_str}Warning: JSON decode error, saved raw metadata: {e}")
                return True, f"Raw metadata saved due to JSON error: {e}"
        else:
            locked_print(f"{thread_str}Warning: Metadata generation failed with return code {result.returncode}")
            return False, stdout
            
    except subprocess.TimeoutExpired:
        locked_print(f"{thread_str}Warning: Metadata generation timed out")
        return False, "Metadata generation timed out"
    except Exception as e:
        locked_print(f"{thread_str}Warning: Exception during metadata generation: {e}")
        return False, f"Exception: {e}"
