#!/usr/bin/env python3
"""
New FastLED compilation system using root platformio.ini with pio ci.

This script replaces the previous ci-compile.py approach by using the enhanced
root platformio.ini file with all board configurations and pio ci for building
examples with proper symlink handling.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
import warnings
from pathlib import Path
from typing import List, Optional, Tuple

from ci.boards import Board, get_board  # type: ignore
from ci.locked_print import locked_print

HERE = Path(__file__).parent.resolve()
ROOT_DIR = HERE.parent

# Default boards to compile for
DEFAULT_BOARDS_NAMES = [
    "apollo3_red",
    "apollo3_thing_explorable", 
    "uno",  # Build is faster if this is first, because it's used for global init.
    "esp32dev",
    "esp01",  # ESP8266
    "esp32c3",
    "attiny85",
    "ATtiny1616",
    "esp32c6",
    "esp32s3",
    "esp32p4",
    "yun",
    "digix",
    "teensylc",
    "teensy30",
    "teensy31",
    "teensy41",
    "adafruit_feather_nrf52840_sense",
    "xiaoblesense_adafruit",
    "rpipico",
    "rpipico2",
    "uno_r4_wifi",
    "esp32rmt_51",
    "esp32dev_idf44",
    "bluepill",
    "giga_r1",
    "sparkfun_xrp_controller",
]

# Examples to compile by default
DEFAULT_EXAMPLES = [
    "Animartrix",
    "Apa102",
    "Apa102HD",
    "Audio",
    "Blink",
    "Blur",
    "Chromancer",
    "ColorPalette",
    "ColorTemperature",
    "Corkscrew",
    "Cylon",
    "DemoReel100",
    "Downscale",
    "FestivalStick",
    "FirstLight",
    "Fire2012",
    "Multiple/MultipleStripsInOneArray",
    "Multiple/ArrayOfLedArrays",
    "Noise",
    "NoisePlayground",
    "NoisePlusPalette",
    "LuminescentGrand",
    "Pacifica",
    "Pride2015",
    "RGBCalibrate",
    "RGBSetDemo",
    "RGBW",
    "Overclock",
    "RGBWEmulated",
    "TwinkleFox",
    "XYMatrix",
    "FireMatrix",
    "FireCylinder",
    "FxGfx2Video",
    "FxSdCard",
    "FxCylon",
    "FxDemoReel100",
    "FxTwinkleFox",
    "FxFire2012",
    "FxNoisePlusPalette",
    "FxPacifica",
    "FxEngine",
    "WS2816",
]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compile FastLED examples using root platformio.ini with pio ci"
    )
    parser.add_argument(
        "boards",
        type=str,
        help="Comma-separated list of boards to compile for",
        nargs="?",
    )
    parser.add_argument(
        "positional_examples",
        type=str,
        help="Examples to compile (positional arguments after board name)",
        nargs="*",
    )
    parser.add_argument(
        "--examples", type=str, help="Comma-separated list of examples to compile"
    )
    parser.add_argument(
        "--exclude-examples", type=str, help="Examples that should be excluded"
    )
    parser.add_argument(
        "--defines", type=str, help="Comma-separated list of compiler definitions"
    )
    parser.add_argument(
        "--build-dir", type=str, help="Override the default build directory"
    )
    parser.add_argument(
        "--optimization-report", 
        action="store_true", 
        help="Enable optimization report generation"
    )
    parser.add_argument(
        "--build-info", 
        action="store_true",
        help="Generate build_info.json files"
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Enable interactive mode to choose a board",
    )
    parser.add_argument(
        "--no-interactive", action="store_true", help="Disable interactive mode"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose output"
    )
    parser.add_argument(
        "--supported-boards",
        action="store_true",
        help="Print the list of supported boards and exit",
    )
    parser.add_argument(
        "--symbols",
        action="store_true",
        help="Run symbol analysis on compiled output",
    )
    parser.add_argument(
        "--allsrc",
        action="store_true",
        help="Enable all-source build (adds FASTLED_ALL_SRC=1 define)",
    )
    parser.add_argument(
        "--no-allsrc",
        action="store_true",
        help="Disable all-source build (adds FASTLED_ALL_SRC=0 define)",
    )

    try:
        args = parser.parse_intermixed_args()
        unknown = []
    except SystemExit:
        # If parse_intermixed_args fails, fall back to parse_known_args
        args, unknown = parser.parse_known_args()

    # Handle unknown arguments intelligently
    unknown_examples = []
    unknown_flags = []
    for arg in unknown:
        if arg.startswith("-"):
            unknown_flags.append(arg)
        else:
            unknown_examples.append(arg)

    # Add unknown examples to positional_examples
    if unknown_examples:
        if not hasattr(args, "positional_examples") or args.positional_examples is None:
            args.positional_examples = []
        args.positional_examples.extend(unknown_examples)

    # Only warn about actual unknown flags, not examples
    if unknown_flags:
        warnings.warn(f"Unknown arguments: {unknown_flags}")

    # Check for environment variables
    if os.environ.get("FASTLED_CI_NO_INTERACTIVE") == "true":
        args.interactive = False
        args.no_interactive = True

    # if --interactive and --no-interactive are both passed, --no-interactive takes precedence.
    if args.interactive and args.no_interactive:
        warnings.warn(
            "Both --interactive and --no-interactive were passed, --no-interactive takes precedence."
        )
        args.interactive = False

    # Validate that --allsrc and --no-allsrc are not both specified
    if args.allsrc and args.no_allsrc:
        warnings.warn(
            "Both --allsrc and --no-allsrc were passed, this is contradictory. Please specify only one."
        )
        sys.exit(1)

    return args


def choose_board_interactively(boards: List[str]) -> List[str]:
    """Interactive board selection."""
    print("Available boards:")
    boards = sorted(list(set(boards)))  # Remove duplicates and sort
    for i, board in enumerate(boards):
        print(f"[{i}]: {board}")
    print("[all]: All boards")
    
    out: List[str] = []
    while True:
        try:
            input_str = input(
                "Enter the number of the board(s) you want to compile to, or its name(s): "
            )
            if "all" in input_str:
                return boards
            for board in input_str.split(","):
                if board == "":
                    continue
                if not board.isdigit():
                    out.append(board)  # Assume it's a board name.
                else:
                    index = int(board)  # Find the board from the index.
                    if 0 <= index < len(boards):
                        out.append(boards[index])
                    else:
                        warnings.warn(f"invalid board index: {index}, skipping")
            if not out:
                print("Please try again.")
                continue
            return out
        except ValueError:
            print("Invalid input. Please enter a number.")


def resolve_example_path(example: str) -> Path:
    """Resolve example name to its full path."""
    example_path = ROOT_DIR / "examples" / example
    if not example_path.exists():
        raise FileNotFoundError(f"Example '{example}' not found at '{example_path}'")
    return example_path


def setup_environment_variables(
    build_dir: Optional[str],
    optimization_report: bool,
    build_info: bool,
    defines: List[str]
) -> None:
    """Set up environment variables for the platformio.ini file."""
    
    # Set up build directory
    if build_dir:
        os.environ["FASTLED_BUILD_DIR"] = build_dir
    
    # Set up optimization report
    if optimization_report:
        os.environ["FASTLED_OPTIMIZATION_REPORT"] = "1"
    
    # Set up build info generation
    if build_info:
        os.environ["FASTLED_GENERATE_BUILD_INFO"] = "1"
    
    # Set up custom defines
    if defines:
        os.environ["FASTLED_CUSTOM_DEFINES"] = ",".join(defines)
    
    # Import and run the build support setup
    sys.path.insert(0, str(HERE / "ci"))
    from fastled_build_support import setup_build_environment
    setup_build_environment()


def compile_example_with_pio_ci(
    board_name: str,
    example_paths: List[Path],
    build_dir: Optional[str],
    verbose: bool
) -> Tuple[bool, str]:
    """Compile examples for a board using pio ci with root platformio.ini."""
    
    # Skip web boards - they're handled separately
    if board_name == "web":
        locked_print(f"Skipping web target for board {board_name}")
        return True, ""
    
    locked_print(f"*** Compiling examples for board {board_name} using pio ci ***")
    
    errors = []
    
    for example_path in example_paths:
        locked_print(f"*** Building example {example_path.name} for board {board_name} ***")
        
        # Find the .ino file in the example directory
        ino_files = list(example_path.glob("*.ino"))
        if not ino_files:
            error_msg = f"No .ino file found in {example_path}"
            locked_print(f"ERROR: {error_msg}")
            errors.append(error_msg)
            continue
        
        # Use the first .ino file found
        ino_file = ino_files[0]
        
        # Set up the build directory for this specific example
        if build_dir:
            example_build_dir = Path(build_dir) / board_name / example_path.name
        else:
            example_build_dir = ROOT_DIR / ".build" / board_name / example_path.name
        
        # Get absolute path to FastLED library for symlink
        fastled_path = str(ROOT_DIR.absolute())
        lib_option = f"lib_deps=symlink://{fastled_path}"
        
        # Build pio ci command - use --board instead of --project-conf/--environment for better compatibility
        cmd_list = [
            "pio",
            "ci",
            str(ino_file),
            "--board", board_name,
            "--keep-build-dir",
            "--build-dir", str(example_build_dir),
            "--project-option", lib_option,
        ]
        
        # Add additional source directories if they exist in the example
        for subdir in example_path.iterdir():
            if subdir.is_dir() and subdir.name not in [".git", "__pycache__", ".pio", ".vscode"]:
                # Check if this directory contains source files
                source_files = [
                    f for f in subdir.rglob("*")
                    if f.is_file() and f.suffix in [".cpp", ".c"]
                ]
                if source_files:
                    cmd_list.extend(["--lib", str(subdir)])
                    if verbose:
                        locked_print(f"  Added source directory: {subdir.name}")
        
        # Always add verbose flag to pio ci for better output
        cmd_list.append("--verbose")
        
        # Execute the command
        cmd_str = subprocess.list2cmdline(cmd_list)
        if verbose:
            locked_print(f"Command: {cmd_str}")
        
        start_time = time.time()
        
        try:
            result = subprocess.run(
                cmd_list,
                capture_output=True,
                text=True,
                cwd=str(ROOT_DIR),
                timeout=60,  # 1 minute timeout for testing
            )
            
            elapsed_time = time.time() - start_time
            
            if result.returncode == 0:
                locked_print(
                    f"*** Successfully built {example_path.name} for {board_name} in {elapsed_time:.2f}s ***"
                )
                if verbose and result.stdout:
                    locked_print(f"Build output:\n{result.stdout}")
            else:
                error_msg = f"Failed to build {example_path.name} for {board_name}"
                locked_print(f"ERROR: {error_msg}")
                if verbose:
                    locked_print(f"Command: {cmd_str}")
                if result.stdout:
                    locked_print(f"STDOUT:\n{result.stdout}")
                if result.stderr:
                    locked_print(f"STDERR:\n{result.stderr}")
                errors.append(f"{error_msg}: {result.stderr}")
                
        except subprocess.TimeoutExpired:
            error_msg = f"Timeout building {example_path.name} for {board_name}"
            locked_print(f"ERROR: {error_msg}")
            errors.append(error_msg)
        except Exception as e:
            error_msg = f"Exception building {example_path.name} for {board_name}: {e}"
            locked_print(f"ERROR: {error_msg}")
            errors.append(error_msg)
    
    if errors:
        return False, "\n".join(errors)
    
    return True, f"Successfully compiled all examples for {board_name}"


def run_symbol_analysis(board_names: List[str]) -> None:
    """Run symbol analysis on compiled outputs if requested."""
    locked_print("\nRunning symbol analysis on compiled outputs...")
    
    for board_name in board_names:
        if board_name == "web":
            continue
        
        try:
            locked_print(f"Running symbol analysis for board: {board_name}")
            
            cmd = [
                "uv",
                "run",
                "ci/ci/symbol_analysis.py",
                "--board",
                board_name,
            ]
            
            result = subprocess.run(
                cmd, capture_output=True, text=True, cwd=str(ROOT_DIR)
            )
            
            if result.returncode != 0:
                locked_print(
                    f"ERROR: Symbol analysis failed for board {board_name}: {result.stderr}"
                )
            else:
                locked_print(f"Symbol analysis completed for board: {board_name}")
                if result.stdout:
                    print(result.stdout)
                    
        except Exception as e:
            locked_print(
                f"ERROR: Exception during symbol analysis for board {board_name}: {e}"
            )


def main() -> int:
    """Main function."""
    args = parse_args()
    
    if args.supported_boards:
        print(",".join(DEFAULT_BOARDS_NAMES))
        return 0
    
    # Determine which boards to compile for
    if args.interactive:
        board_names = choose_board_interactively(DEFAULT_BOARDS_NAMES)
    else:
        board_names = args.boards.split(",") if args.boards else DEFAULT_BOARDS_NAMES
    
    # Determine which examples to compile
    if args.positional_examples:
        # Convert positional examples, handling both "examples/Blink" and "Blink" formats
        examples = []
        for example in args.positional_examples:
            # Remove "examples/" prefix if present
            if example.startswith("examples/"):
                example = example[len("examples/"):]
            examples.append(example)
    elif args.examples:
        examples = args.examples.split(",")
    else:
        examples = DEFAULT_EXAMPLES
    
    # Process example exclusions
    if args.exclude_examples:
        exclude_examples = args.exclude_examples.split(",")
        examples = [ex for ex in examples if ex not in exclude_examples]
    
    # Resolve example paths
    example_paths: List[Path] = []
    for example in examples:
        try:
            example_path = resolve_example_path(example)
            example_paths.append(example_path)
        except FileNotFoundError as e:
            locked_print(f"ERROR: {e}")
            return 1
    
    # Set up defines
    defines: List[str] = []
    if args.defines:
        defines.extend(args.defines.split(","))
    
    # Add FASTLED_ALL_SRC define when --allsrc or --no-allsrc flag is specified
    if args.allsrc:
        defines.append("FASTLED_ALL_SRC=1")
    elif args.no_allsrc:
        defines.append("FASTLED_ALL_SRC=0")
    
    # Set up environment variables for platformio.ini
    setup_environment_variables(
        build_dir=args.build_dir,
        optimization_report=args.optimization_report,
        build_info=args.build_info,
        defines=defines
    )
    
    # Start compilation
    start_time = time.time()
    locked_print(
        f"Starting compilation for {len(board_names)} boards with {len(example_paths)} examples"
    )
    
    compilation_errors = []
    
    # Compile for each board
    for board_name in board_names:
        success, message = compile_example_with_pio_ci(
            board_name=board_name,
            example_paths=example_paths,
            build_dir=args.build_dir,
            verbose=args.verbose,
        )
        
        if not success:
            compilation_errors.append(f"Board {board_name}: {message}")
            locked_print(f"ERROR: Compilation failed for board {board_name}")
            # Continue with other boards instead of stopping
    
    # Run symbol analysis if requested
    if args.symbols:
        run_symbol_analysis(board_names)
    
    # Report results
    elapsed_time = time.time() - start_time
    time_str = time.strftime("%Mm:%Ss", time.gmtime(elapsed_time))
    
    if compilation_errors:
        locked_print(
            f"\nCompilation finished in {time_str} with {len(compilation_errors)} error(s):"
        )
        for error in compilation_errors:
            locked_print(f"  - {error}")
        return 1
    else:
        locked_print(f"\nAll compilations completed successfully in {time_str}")
        return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        locked_print("\nInterrupted by user")
        sys.exit(1)
