"""
Optimized compilation module that supports incremental builds by preserving
build artifacts between examples while ensuring correct builds.
"""
import hashlib
import json
import os
import shutil
import subprocess
import time
from pathlib import Path
from threading import Lock
from typing import Dict, List, Optional, Set, Tuple

from ci.boards import Board  # type: ignore
from ci.locked_print import locked_print

ERROR_HAPPENED = False
IS_GITHUB = "GITHUB_ACTIONS" in os.environ
FIRST_BUILD_LOCK = Lock()
USE_FIRST_BUILD_LOCK = IS_GITHUB

# Cache for tracking file modifications and build state
BUILD_CACHE: Dict[str, Dict[str, str]] = {}


def errors_happened() -> bool:
    """Return whether any errors happened during the build."""
    return ERROR_HAPPENED


def _fastled_js_is_parent_directory(p: Path) -> bool:
    """Check if fastled_js is a parent directory of the given path."""
    return "fastled_js" in str(p.absolute())


def get_file_hash(file_path: Path) -> str:
    """Get SHA256 hash of a file's contents."""
    try:
        with open(file_path, 'rb') as f:
            return hashlib.sha256(f.read()).hexdigest()
    except Exception as e:
        locked_print(f"Warning: Could not hash file {file_path}: {e}")
        return str(file_path.stat().st_mtime) if file_path.exists() else ""


def load_build_cache(cache_file: Path) -> Dict[str, Dict[str, str]]:
    """Load build cache from JSON file."""
    try:
        if cache_file.exists():
            with open(cache_file, 'r') as f:
                return json.load(f)
    except Exception as e:
        locked_print(f"Warning: Could not load build cache: {e}")
    return {}


def save_build_cache(cache_file: Path, cache_data: Dict[str, Dict[str, str]]) -> None:
    """Save build cache to JSON file."""
    try:
        cache_file.parent.mkdir(parents=True, exist_ok=True)
        with open(cache_file, 'w') as f:
            json.dump(cache_data, f, indent=2)
    except Exception as e:
        locked_print(f"Warning: Could not save build cache: {e}")


def get_example_files_with_hashes(example_path: Path) -> Dict[str, str]:
    """Get all files in example with their hashes."""
    files_with_hashes = {}
    
    for src_file in example_path.rglob("*"):
        if src_file.is_file() and not _fastled_js_is_parent_directory(src_file):
            rel_path = str(src_file.relative_to(example_path))
            files_with_hashes[rel_path] = get_file_hash(src_file)
    
    return files_with_hashes


def needs_rebuild(example_path: Path, builddir: Path, board_name: str) -> Tuple[bool, Set[str]]:
    """
    Determine if rebuild is needed and which files changed.
    
    Returns:
        (needs_rebuild, changed_files)
    """
    cache_file = builddir / "build_cache.json"
    cache_key = f"{board_name}_{example_path.name}"
    
    cache_data = load_build_cache(cache_file)
    
    # Get current example files and their hashes
    current_files = get_example_files_with_hashes(example_path)
    
    # Check if we have cached info for this example
    if cache_key not in cache_data:
        locked_print(f"No cache entry for {cache_key}, full rebuild needed")
        return True, set(current_files.keys())
    
    cached_files = cache_data[cache_key]
    
    # Find changed files
    changed_files = set()
    
    # Check for new or modified files
    for file_path, current_hash in current_files.items():
        if file_path not in cached_files or cached_files[file_path] != current_hash:
            changed_files.add(file_path)
    
    # Check for deleted files
    for file_path in cached_files:
        if file_path not in current_files:
            changed_files.add(file_path)
    
    # Check if source libs have changed (FastLED itself)
    fastled_src = Path("src")
    if fastled_src.exists():
        # Simple check: if any .cpp/.h file in src has newer mtime than cache file
        if cache_file.exists():
            cache_mtime = cache_file.stat().st_mtime
            for src_file in fastled_src.rglob("*.[ch]pp"):
                if src_file.stat().st_mtime > cache_mtime:
                    locked_print(f"FastLED source file {src_file} modified, rebuild needed")
                    return True, set(current_files.keys())
    
    needs_rebuild = len(changed_files) > 0
    
    if needs_rebuild:
        locked_print(f"Rebuild needed for {cache_key}, changed files: {changed_files}")
    else:
        locked_print(f"No rebuild needed for {cache_key}")
    
    return needs_rebuild, changed_files


def update_build_cache(
    example_path: Path, 
    builddir: Path, 
    board_name: str,
    current_files: Dict[str, str]
) -> None:
    """Update build cache with current file hashes."""
    cache_file = builddir / "build_cache.json"
    cache_key = f"{board_name}_{example_path.name}"
    
    cache_data = load_build_cache(cache_file)
    cache_data[cache_key] = current_files
    save_build_cache(cache_file, cache_data)


def sync_example_files(
    example_path: Path, 
    srcdir: Path, 
    changed_files: Optional[Set[str]] = None,
    clean_existing: bool = False
) -> None:
    """
    Sync example files to src directory.
    If changed_files is provided, only sync those files.
    If clean_existing is True, clean the src directory first (for example switching).
    """
    if clean_existing and srcdir.exists():
        # Clean sync - remove and recreate src directory for new examples
        locked_print(f"Cleaning src directory for new example: {srcdir}")
        shutil.rmtree(srcdir, ignore_errors=False)
        # Ensure directory is recreated immediately
        srcdir.mkdir(parents=True, exist_ok=True)
    
    if changed_files is None:
        # Full sync - ensure src directory exists and copy all files
        srcdir.mkdir(parents=True, exist_ok=True)
        
        # Copy all files
        for src_file in example_path.rglob("*"):
            if src_file.is_file() and not _fastled_js_is_parent_directory(src_file):
                rel_path = src_file.relative_to(example_path)
                dst_file = srcdir / rel_path
                dst_file.parent.mkdir(parents=True, exist_ok=True)
                locked_print(f"Copying {src_file} to {dst_file}")
                shutil.copy2(src_file, dst_file)
    else:
        # Incremental sync - only update changed files
        srcdir.mkdir(parents=True, exist_ok=True)
        
        for rel_path in changed_files:
            src_file = example_path / rel_path
            dst_file = srcdir / rel_path
            
            if src_file.exists():
                # File was added or modified
                dst_file.parent.mkdir(parents=True, exist_ok=True)
                locked_print(f"Updating {src_file} to {dst_file}")
                shutil.copy2(src_file, dst_file)
            elif dst_file.exists():
                # File was deleted
                locked_print(f"Removing deleted file {dst_file}")
                dst_file.unlink()


def compile_for_board_and_example(
    board: Board,
    example: Path,
    build_dir: Optional[str],
    verbose_on_failure: bool,
    libs: Optional[List[str]],
    force_rebuild: bool = False,
    clean_src_only: bool = False,
) -> Tuple[bool, str]:
    """Compile the given example for the given board with incremental build support."""
    global ERROR_HAPPENED
    
    if board.board_name == "web":
        locked_print(f"Skipping web target for example {example}")
        return True, ""
    
    board_name = board.board_name
    use_pio_run = board.use_pio_run
    real_board_name = board.get_real_board_name()
    libs = libs or []
    
    # Normalize paths
    builddir = (
        Path(build_dir).resolve() / board_name 
        if build_dir 
        else Path(".build").resolve() / board_name
    )
    builddir.mkdir(parents=True, exist_ok=True)
    srcdir = builddir / "src"
    
    locked_print(f"*** Building example {example.name} for board {board_name} ***")
    
    # Determine if rebuild is needed
    if not force_rebuild:
        needs_rebuild_flag, changed_files = needs_rebuild(example, builddir, board_name)
    else:
        needs_rebuild_flag = True
        changed_files = set()
        locked_print(f"Force rebuild requested for {example.name}")
    
    # Sync example files to src directory
    if needs_rebuild_flag or force_rebuild or clean_src_only:
        # Determine sync mode
        clean_mode = force_rebuild or clean_src_only
        sync_mode = "full clean sync" if force_rebuild else ("clean src sync" if clean_src_only else "full sync")
        
        locked_print(f"{'Force rebuild' if force_rebuild else ('Clean src only' if clean_src_only else 'Rebuild needed')} for {example.name}, performing {sync_mode}")
        sync_example_files(example, srcdir, clean_existing=clean_mode)
    else:
        # Incremental build - only sync changed files if any
        if changed_files:
            locked_print(f"Incremental sync for {example.name}, updating {len(changed_files)} files")
            sync_example_files(example, srcdir, changed_files)
        else:
            # No rebuild needed, but ensure src directory exists
            if not srcdir.exists():
                locked_print(f"Warning: src directory missing, forcing full sync")
                sync_example_files(example, srcdir)
    
    # Setup library directories if using pio run
    if use_pio_run:
        for lib in libs:
            project_libdir = Path(lib)
            if not project_libdir.exists():
                locked_print(f"Warning: Library directory {project_libdir} not found")
                continue
                
            build_lib = builddir / "lib" / lib
            
            # Only copy if library doesn't exist or source is newer
            if not build_lib.exists() or needs_rebuild_flag:
                if build_lib.exists():
                    shutil.rmtree(build_lib, ignore_errors=True)
                locked_print(f"Copying library {project_libdir} to {build_lib}")
                shutil.copytree(project_libdir, build_lib)
    
    # Build command
    cwd: Optional[str] = None
    if use_pio_run:
        cwd = str(builddir)
        cmd_list = ["pio", "run"]
    else:
        cmd_list = [
            "pio", "ci",
            "--board", real_board_name,
            *[f"--lib={lib}" for lib in libs],
            "--keep-build-dir",
            f"--build-dir={builddir.as_posix()}",
            f"{example.as_posix()}/*.ino"
        ]
    
    # Execute build
    success, stdout = _execute_build_command(
        cmd_list, 
        cwd, 
        example, 
        board_name, 
        verbose_on_failure
    )
    
    if success:
        # Update build cache on successful build
        current_files = get_example_files_with_hashes(example)
        update_build_cache(example, builddir, board_name, current_files)
    
    return success, stdout


def _execute_build_command(
    cmd_list: List[str],
    cwd: Optional[str],
    example: Path,
    board_name: str,
    verbose_on_failure: bool
) -> Tuple[bool, str]:
    """Execute the build command with proper error handling and timing."""
    global ERROR_HAPPENED
    
    cmd_str = subprocess.list2cmdline(cmd_list)
    msg_list = [
        "\n\n******************************",
        f"* Running command in cwd: {cwd if cwd else os.getcwd()}",
        f"*     {cmd_str}",
        "******************************\n",
    ]
    msg = "\n".join(msg_list)
    locked_print(msg)

    # Start timing for the process
    start_time = time.time()

    # Run the process with real-time output capture and timing
    result = subprocess.Popen(
        cmd_list,
        cwd=cwd,
        shell=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    # Capture output lines in real-time with timing
    stdout_lines = []
    if result.stdout:
        for line in iter(result.stdout.readline, ""):
            if line:
                elapsed = time.time() - start_time
                timing_prefix = f"{elapsed:5.2f} "
                timed_line = timing_prefix + line.rstrip()
                stdout_lines.append(line.rstrip())
                locked_print(timed_line)

    # Wait for process to complete
    result.wait()

    # Join all stdout lines for the return value
    stdout = "\n".join(stdout_lines)
    stdout = stdout.replace("lib/src", "src").replace("lib\\src", "src")
    
    if result.returncode != 0:
        if not verbose_on_failure:
            ERROR_HAPPENED = True
            return False, stdout
            
        if ERROR_HAPPENED:
            return False, ""
            
        ERROR_HAPPENED = True
        locked_print(f"*** Error compiling example {example} for board {board_name} ***")
        
        # Re-run with verbose output
        cmd_list.append("-v")
        verbose_stdout = _run_verbose_build(cmd_list, cwd)
        stdout = verbose_stdout + "\n\nThis is a second attempt with verbose output, look above for compiler errors.\n"
        return False, stdout
    
    locked_print(f"*** Finished building example {example} for board {board_name} ***")
    return True, stdout


def _run_verbose_build(cmd_list: List[str], cwd: Optional[str]) -> str:
    """Run build command with verbose output for debugging."""
    cmd_str = subprocess.list2cmdline(cmd_list)
    msg_list = [
        "\n\n******************************",
        "* Re-running failed command but with verbose output:",
        f"*     {cmd_str}",
        "******************************\n",
    ]
    msg = "\n".join(msg_list)
    locked_print(msg)

    start_time_verbose = time.time()
    
    result = subprocess.Popen(
        cmd_list,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    stdout_lines_verbose = []
    if result.stdout:
        for line in iter(result.stdout.readline, ""):
            if line:
                elapsed = time.time() - start_time_verbose
                timing_prefix = f"{elapsed:5.2f} "
                timed_line = timing_prefix + line.rstrip()
                stdout_lines_verbose.append(line.rstrip())
                locked_print(timed_line)

    result.wait()
    return "\n".join(stdout_lines_verbose)


def compile_examples(
    board: Board,
    examples: List[Path],
    build_dir: Optional[str],
    verbose_on_failure: bool,
    libs: Optional[List[str]],
    force_rebuild: bool = False,
) -> Tuple[bool, str]:
    """
    Process the task queue for the given board with incremental build support.
    
    Incremental build behavior:
    - First example: Uses incremental build if cache exists
    - Subsequent examples: Forces clean rebuild to avoid function conflicts
    - Same example re-run: Uses incremental build based on file changes
    
    This ensures no conflicts between examples while still providing speed benefits
    when re-running the same example multiple times.
    """
    global ERROR_HAPPENED
    
    board_name = board.board_name
    is_first = True
    
    for example in examples:
        if ERROR_HAPPENED:
            return True, ""
            
        # Normalize example path
        example = example.resolve().relative_to(Path(".").resolve())
        locked_print(f"\n*** Building {example} for board {board_name} ***")
        
        if is_first:
            locked_print(f"*** Building first example {example} for board {board_name} ***")
        
        # OPTIMIZED FIX: Clean src only when switching between examples to avoid conflicts
        # This preserves the FastLED library build while avoiding function conflicts
        clean_src_for_new_example = not is_first and not force_rebuild
        
        # Use locking for first build on GitHub to manage memory usage
        if is_first and USE_FIRST_BUILD_LOCK:
            with FIRST_BUILD_LOCK:
                success, message = compile_for_board_and_example(
                    board=board,
                    example=example,
                    build_dir=build_dir,
                    verbose_on_failure=verbose_on_failure,
                    libs=libs,
                    force_rebuild=force_rebuild,
                    clean_src_only=clean_src_for_new_example,
                )
        else:
            success, message = compile_for_board_and_example(
                board=board,
                example=example,
                build_dir=build_dir,
                verbose_on_failure=verbose_on_failure,
                libs=libs,
                force_rebuild=force_rebuild,
                clean_src_only=clean_src_for_new_example,
            )
        
        is_first = False
        
        if not success:
            ERROR_HAPPENED = True
            return (
                False,
                f"Error building {example} for board {board_name}. stdout:\n{message}",
            )
    
    return True, ""


def clean_build_cache(build_dir: Optional[str], board_name: Optional[str] = None) -> None:
    """Clean build cache for specified board or all boards."""
    build_root = Path(build_dir) if build_dir else Path(".build")
    
    if board_name:
        # Clean specific board
        board_dir = build_root / board_name
        cache_file = board_dir / "build_cache.json"
        if cache_file.exists():
            cache_file.unlink()
            locked_print(f"Cleaned build cache for board {board_name}")
    else:
        # Clean all boards
        for board_dir in build_root.glob("*/"):
            if board_dir.is_dir():
                cache_file = board_dir / "build_cache.json"
                if cache_file.exists():
                    cache_file.unlink()
                    locked_print(f"Cleaned build cache for board {board_dir.name}")
