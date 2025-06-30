"""
Optimized concurrent compilation module that supports incremental builds
and better path handling.
"""
import os
import subprocess
import time
from concurrent.futures import Future, ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

from ci.boards import Board  # type: ignore
from ci.compile_for_board_v2 import compile_examples, errors_happened, clean_build_cache
from ci.cpu_count import cpu_count
from ci.create_build_dir_v2 import create_build_dir
from ci.locked_print import locked_print

# Board initialization doesn't take a lot of memory or cpu so it's safe to run in parallel
PARALLEL_PROJECT_INITIALIZATION = (
    os.environ.get("PARALLEL_PROJECT_INITIALIZATION", "0") == "1"
)


def _banner_print(msg: str) -> None:
    """Print a banner message."""
    lines = msg.splitlines()
    for line in lines:
        print("#" * (len(line) + 4))
        print(f"# {line} #")
        print("#" * (len(line) + 4))


@dataclass
class ConcurrentRunArgsV2:
    """Enhanced arguments for concurrent compilation with incremental build support."""
    projects: List[Board]
    examples: List[Path]
    skip_init: bool
    defines: List[str]
    customsdk: Optional[str]
    extra_packages: List[str]
    libs: Optional[List[str]]
    build_dir: Optional[str]
    extra_scripts: Optional[str]
    cwd: Optional[str]
    board_dir: Optional[str]
    build_flags: Optional[List[str]]
    verbose: bool = False
    extra_examples: Optional[Dict[Board, List[Path]]] = None
    symbols: bool = False
    force_rebuild: bool = False  # New option for forcing rebuilds
    clean_cache: bool = False    # New option for cleaning build cache


def concurrent_run_v2(args: ConcurrentRunArgsV2) -> int:
    """
    Enhanced concurrent compilation with incremental build support.
    
    Key improvements:
    - Incremental builds by default (unless force_rebuild=True)
    - Better path normalization and handling
    - Build cache management
    - Improved error handling and logging
    """
    projects = args.projects
    examples = args.examples
    skip_init = args.skip_init
    defines = args.defines
    customsdk = args.customsdk
    extra_packages = args.extra_packages
    build_dir = args.build_dir
    extra_scripts = args.extra_scripts
    cwd = args.cwd
    board_dir = args.board_dir
    libs = args.libs
    extra_examples = args.extra_examples or {}
    force_rebuild = args.force_rebuild
    clean_cache = args.clean_cache
    
    prev_cwd: Optional[str] = None
    
    # Change to project directory if specified
    if cwd:
        prev_cwd = os.getcwd()
        locked_print(f"Changing to directory {cwd}")
        os.chdir(cwd)
    
    # Clean build cache if requested
    if clean_cache:
        locked_print("Cleaning build cache for all boards...")
        clean_build_cache(build_dir)
        locked_print("Build cache cleaned.")
    
    # Initialize build directories
    start_time = time.time()
    first_project = projects[0]
    
    # Create initial build directory
    locked_print(f"Creating build directory for first project: {first_project.board_name}")
    success, msg = create_build_dir(
        board=first_project,
        defines=defines,
        customsdk=customsdk,
        no_install_deps=skip_init,
        extra_packages=extra_packages,
        build_dir=build_dir,
        board_dir=board_dir,
        build_flags=args.build_flags,
        extra_scripts=extra_scripts,
    )
    
    if not success:
        locked_print(f"ERROR: Failed to create initial build directory: {msg}")
        return 1
    
    init_time = time.time() - start_time
    locked_print(f"Initial build directory created in {init_time:.2f} seconds")
    
    # Initialize build directories for all boards in parallel
    parallel_workers = 1 if not PARALLEL_PROJECT_INITIALIZATION else min(len(projects), 4)
    locked_print(f"Initializing build directories for {len(projects)} boards with {parallel_workers} workers")
    
    with ThreadPoolExecutor(max_workers=parallel_workers) as executor:
        future_to_board: Dict[Future, Board] = {}
        
        for board in projects:
            locked_print(f"Submitting build directory initialization for board: {board.board_name}")
            future = executor.submit(
                create_build_dir,
                board,
                defines,
                customsdk,
                skip_init,
                extra_packages,
                build_dir,
                board_dir,
                args.build_flags,
                extra_scripts,
            )
            future_to_board[future] = board
        
        completed_boards = 0
        failed_boards = 0
        
        for future in as_completed(future_to_board):
            board = future_to_board[future]
            try:
                success, msg = future.result()
                if not success:
                    locked_print(f"ERROR: Failed to initialize build_dir for board {board.board_name}:\n{msg}")
                    failed_boards += 1
                    # Cancel remaining tasks
                    for f in future_to_board:
                        if not f.done():
                            f.cancel()
                    return 1
                else:
                    completed_boards += 1
                    locked_print(f"SUCCESS: Initialized build_dir for board {board.board_name} ({completed_boards}/{len(projects)})")
            except Exception as e:
                locked_print(f"EXCEPTION: Build directory initialization failed for board {board.board_name}: {e}")
                failed_boards += 1
                # Cancel remaining tasks
                for f in future_to_board:
                    if not f.done():
                        f.cancel()
                return 1
    
    init_end_time = time.time()
    total_init_time = (init_end_time - start_time) / 60
    locked_print(f"\nAll build directories initialized in {total_init_time:.2f} minutes.")
    
    # Run compilation process with incremental build support
    compilation_errors: List[str] = []
    
    # Determine optimal number of workers for compilation
    # Use fewer workers than CPUs to avoid overwhelming the system
    num_cpus = max(1, min(cpu_count() // 2, len(projects)))
    locked_print(f"Starting compilation with {num_cpus} parallel workers")
    
    compilation_start = time.time()
    
    with ThreadPoolExecutor(max_workers=num_cpus) as executor:
        future_to_board = {}
        
        for board in projects:
            board_examples = examples + extra_examples.get(board, [])
            future = executor.submit(
                compile_examples,
                board,
                board_examples,
                build_dir,
                args.verbose,  # verbose_on_failure
                libs,
                force_rebuild,
            )
            future_to_board[future] = board
        
        for future in as_completed(future_to_board):
            board = future_to_board[future]
            try:
                success, msg = future.result()
                if not success:
                    error_msg = f"Compilation failed for board {board.board_name}: {msg}"
                    compilation_errors.append(error_msg)
                    locked_print(f"ERROR: {error_msg}")
                    
                    # Cancel remaining compilations on first failure
                    for f in future_to_board:
                        if not f.done():
                            f.cancel()
                    break
                else:
                    locked_print(f"SUCCESS: Compilation completed for board {board.board_name}")
            except Exception as e:
                error_msg = f"Exception during compilation for board {board.board_name}: {e}"
                compilation_errors.append(error_msg)
                locked_print(f"ERROR: {error_msg}")
                
                # Cancel remaining compilations on exception
                for f in future_to_board:
                    if not f.done():
                        f.cancel()
                break
    
    compilation_end = time.time()
    compilation_time = (compilation_end - compilation_start) / 60
    
    # Change back to original directory
    if prev_cwd:
        locked_print(f"Changing back to directory {prev_cwd}")
        os.chdir(prev_cwd)
    
    # Check for compilation errors
    if errors_happened() or compilation_errors:
        locked_print(f"\nCompilation completed with errors in {compilation_time:.2f} minutes.")
        if compilation_errors:
            locked_print("Compilation errors:")
            for error in compilation_errors:
                locked_print(f"  - {error}")
        return 1
    
    locked_print(f"\nAll compilations completed successfully in {compilation_time:.2f} minutes.")
    
    # Run symbol analysis if requested
    if args.symbols:
        symbol_analysis_start = time.time()
        locked_print("\nRunning symbol analysis on compiled outputs...")
        
        symbol_errors = _run_symbol_analysis(projects, cwd or os.getcwd())
        
        symbol_analysis_end = time.time()
        symbol_analysis_time = (symbol_analysis_end - symbol_analysis_start) / 60
        
        if symbol_errors:
            locked_print(f"\nSymbol analysis completed with {len(symbol_errors)} error(s) in {symbol_analysis_time:.2f} minutes:")
            for error in symbol_errors:
                locked_print(f"  - {error}")
        else:
            locked_print(f"\nSymbol analysis completed successfully for all {len(projects)} board(s) in {symbol_analysis_time:.2f} minutes.")
    
    total_time = (time.time() - start_time) / 60
    locked_print(f"\n🎉 All operations completed successfully in {total_time:.2f} minutes total! 🎉")
    
    return 0


def _run_symbol_analysis(projects: List[Board], cwd: str) -> List[str]:
    """Run symbol analysis for all projects and return any errors."""
    symbol_analysis_errors = []
    
    for board in projects:
        try:
            locked_print(f"Running symbol analysis for board: {board.board_name}")
            
            cmd = [
                "uv", "run",
                "ci/ci/symbol_analysis.py",
                "--board", board.board_name,
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=cwd,
                timeout=300,  # 5 minute timeout for symbol analysis
            )
            
            if result.returncode != 0:
                error_msg = f"Symbol analysis failed for board {board.board_name}: {result.stderr}"
                symbol_analysis_errors.append(error_msg)
                locked_print(f"ERROR: {error_msg}")
            else:
                locked_print(f"Symbol analysis completed for board: {board.board_name}")
                # Print condensed symbol analysis output
                if result.stdout:
                    # Extract just the summary lines
                    lines = result.stdout.strip().split('\n')
                    summary_lines = [line for line in lines if 
                                   'SUMMARY:' in line or 
                                   'Total symbols:' in line or 
                                   'Total symbol size:' in line or
                                   'LARGEST SYMBOLS' in line]
                    if summary_lines:
                        locked_print(f"Symbol analysis summary for {board.board_name}:")
                        for line in summary_lines[:5]:  # Show first 5 lines
                            locked_print(f"  {line}")
        
        except subprocess.TimeoutExpired:
            error_msg = f"Symbol analysis timed out for board {board.board_name}"
            symbol_analysis_errors.append(error_msg)
            locked_print(f"ERROR: {error_msg}")
        except Exception as e:
            error_msg = f"Exception during symbol analysis for board {board.board_name}: {e}"
            symbol_analysis_errors.append(error_msg)
            locked_print(f"ERROR: {error_msg}")
    
    return symbol_analysis_errors


def print_build_summary(
    projects: List[Board], 
    examples: List[Path], 
    build_time: float,
    force_rebuild: bool = False
) -> None:
    """Print a summary of the build configuration and results."""
    locked_print("\n" + "="*80)
    locked_print("BUILD SUMMARY")
    locked_print("="*80)
    locked_print(f"Boards compiled: {len(projects)}")
    locked_print(f"Examples compiled: {len(examples)}")
    locked_print(f"Total build combinations: {len(projects) * len(examples)}")
    locked_print(f"Build mode: {'Full rebuild' if force_rebuild else 'Incremental'}")
    locked_print(f"Total build time: {build_time:.2f} minutes")
    locked_print(f"Average time per combination: {(build_time * 60) / (len(projects) * len(examples)):.1f} seconds")
    
    locked_print("\nBoards:")
    for project in projects:
        locked_print(f"  - {project.board_name}")
    
    locked_print("\nExamples:")
    for example in examples:
        locked_print(f"  - {example.name}")
    
    locked_print("="*80)
