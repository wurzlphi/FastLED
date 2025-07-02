"""
Runs the compilation process for examples on boards using pio ci command.
This replaces the previous concurrent build system with a simpler pio ci approach.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
import warnings
from pathlib import Path

from ci.boards import Board, get_board  # type: ignore
from ci.locked_print import locked_print

HERE = Path(__file__).parent.resolve()

# Default boards to compile for
DEFAULT_BOARDS_NAMES = [
    "apollo3_red",
    "apollo3_thing_explorable",
    "web",  # work in progress
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
    "esp32rmt_51",
    "giga_r1",
    "sparkfun_xrp_controller",
]

OTHER_BOARDS_NAMES = [
    "nano_every",
    "esp32-c2-devkitm-1",
]

# Examples to compile.
DEFAULT_EXAMPLES = [
    "Animartrix",
    "Apa102",
    "Apa102HD",
    "Apa102HDOverride",
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

EXTRA_EXAMPLES: dict[Board, list[str]] = {
    # ESP32DEV: ["EspI2SDemo"],
    # ESP32_S3_DEVKITC_1: ["EspS3I2SDemo"],
}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compile FastLED examples for various boards using pio ci."
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
    parser.add_argument("--customsdk", type=str, help="custom_sdkconfig project option")
    parser.add_argument(
        "--extra-packages",
        type=str,
        help="Comma-separated list of extra packages to install",
    )
    parser.add_argument(
        "--build-dir", type=str, help="Override the default build directory"
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
    parser.add_argument(
        "--optimize-build",
        action="store_true",
        help="Enable two-stage build optimization to reduce repeated library compilation",
    )
    try:
        args = parser.parse_intermixed_args()
        unknown = []
    except SystemExit:
        # If parse_intermixed_args fails, fall back to parse_known_args
        args, unknown = parser.parse_known_args()

    # Handle unknown arguments intelligently - treat non-flag arguments as examples
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

    # Check for FASTLED_CI_NO_INTERACTIVE environment variable
    if os.environ.get("FASTLED_CI_NO_INTERACTIVE") == "true":
        args.interactive = False
        args.no_interactive = True

    # if --interactive and --no-interative are both passed, --no-interactive takes precedence.
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


def remove_duplicates(items: list[str]) -> list[str]:
    seen = set()
    out = []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out


def choose_board_interactively(boards: list[str]) -> list[str]:
    print("Available boards:")
    boards = remove_duplicates(sorted(boards))
    for i, board in enumerate(boards):
        print(f"[{i}]: {board}")
    print("[all]: All boards")
    out: list[str] = []
    while True:
        try:
            input_str = input(
                "Enter the number of the board(s) you want to compile to, or it's name(s): "
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
    example_path = HERE.parent / "examples" / example
    if not example_path.exists():
        raise FileNotFoundError(f"Example '{example}' not found at '{example_path}'")
    return example_path


def generate_build_info(
    board: Board, board_build_dir: Path, defines: list[str]
) -> bool:
    """Generate build_info.json file for the board using pio project metadata."""
    import json
    import tempfile

    board_name = board.board_name
    real_board_name = board.get_real_board_name()

    # Create a temporary project to get metadata
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_project = Path(temp_dir) / "temp_project"
        temp_project.mkdir(parents=True, exist_ok=True)

        # Initialize a temporary project
        cmd_list = [
            "pio",
            "project",
            "init",
            "--project-dir",
            str(temp_project),
            "--board",
            real_board_name,
        ]

        # Add platform-specific options
        if board.platform:
            cmd_list.append(f"--project-option=platform={board.platform}")
        if board.platform_packages:
            cmd_list.append(
                f"--project-option=platform_packages={board.platform_packages}"
            )
        if board.framework:
            cmd_list.append(f"--project-option=framework={board.framework}")
        if board.board_build_core:
            cmd_list.append(
                f"--project-option=board_build.core={board.board_build_core}"
            )
        if board.board_build_filesystem_size:
            cmd_list.append(
                f"--project-option=board_build.filesystem_size={board.board_build_filesystem_size}"
            )

        # Add defines
        all_defines = defines.copy()
        if board.defines:
            all_defines.extend(board.defines)
        if all_defines:
            build_flags_str = " ".join(f"-D{define}" for define in all_defines)
            cmd_list.append(f"--project-option=build_flags={build_flags_str}")

        if board.customsdk:
            cmd_list.append(f"--project-option=custom_sdkconfig={board.customsdk}")

        try:
            # Initialize the project
            result = subprocess.run(
                cmd_list,
                capture_output=True,
                text=True,
                cwd=str(temp_project),
                timeout=60,
            )

            if result.returncode != 0:
                locked_print(
                    f"Warning: Failed to initialize temp project for {board_name}: {result.stderr}"
                )
                return False

            # Get metadata
            metadata_cmd = ["pio", "project", "metadata", "--json-output"]
            metadata_result = subprocess.run(
                metadata_cmd,
                capture_output=True,
                text=True,
                cwd=str(temp_project),
                timeout=60,
            )

            if metadata_result.returncode != 0:
                locked_print(
                    f"Warning: Failed to get metadata for {board_name}: {metadata_result.stderr}"
                )
                return False

            # Parse and save the metadata
            try:
                data = json.loads(metadata_result.stdout)

                # Add tool aliases (from create_build_dir.py)
                sys.path.insert(0, str(HERE))
                from ci.create_build_dir import insert_tool_aliases

                insert_tool_aliases(data)

                # Save to build_info.json
                build_info_path = board_build_dir / "build_info.json"
                with open(build_info_path, "w") as f:
                    json.dump(data, f, indent=4, sort_keys=True)

                locked_print(f"Generated build_info.json for {board_name}")
                return True

            except json.JSONDecodeError as e:
                locked_print(
                    f"Warning: Failed to parse metadata JSON for {board_name}: {e}"
                )
                return False

        except subprocess.TimeoutExpired:
            locked_print(
                f"Warning: Timeout generating build_info.json for {board_name}"
            )
            return False
        except Exception as e:
            locked_print(
                f"Warning: Exception generating build_info.json for {board_name}: {e}"
            )
            return False


def setup_library_for_windows(board_build_dir: Path, example_path: Path) -> Path:
    """Set up the FastLED library for Windows builds by copying files."""
    lib_dir = (
        board_build_dir / example_path.name / ".pio" / "libdeps" / "uno" / "FastLED"
    )
    lib_dir.mkdir(parents=True, exist_ok=True)

    # Copy the library files
    src_dir = HERE.parent
    if lib_dir.exists():
        shutil.rmtree(lib_dir)
    shutil.copytree(
        src_dir,
        lib_dir,
        ignore=shutil.ignore_patterns(".git", ".pio", "__pycache__", ".vscode"),
    )

    return lib_dir


def compile_with_pio_ci_optimized(
    board: Board,
    example_paths: list[Path],
    build_dir: str | None,
    defines: list[str],
    verbose: bool,
) -> tuple[bool, str]:
    """Two-stage optimized compilation for examples using pio ci command.
    
    Stage 1: Initialize build folder with --disable-auto-clean to preserve libraries
    Stage 2: Batch building using LDF mode chain to reuse compiled libraries
    """
    
    # Skip web boards
    if board.board_name == "web":
        locked_print(f"Skipping web target for board {board.board_name}")
        return True, ""

    board_name = board.board_name
    real_board_name = board.get_real_board_name()

    # Set up build directory
    if build_dir:
        board_build_dir = Path(build_dir) / board_name
    else:
        board_build_dir = Path(".build") / board_name

    board_build_dir.mkdir(parents=True, exist_ok=True)

    # Generate build_info.json for this board
    generate_build_info(board, board_build_dir, defines)

    locked_print(f"*** Two-Stage Optimized Compilation for board {board_name} ***")
    locked_print(f"*** Stage 1: Initializing build environment with library caching ***")

    # Get absolute path to FastLED library using platform's natural path format
    fastled_path = str(HERE.parent.absolute())
    lib_option = f"lib_deps=symlink://{fastled_path}"

    # Set up board-specific build cache directory with absolute path
    cache_dir = HERE.parent / ".pio_cache" / board.board_name
    absolute_cache_dir = str(cache_dir.absolute())
    cache_option = f"build_cache_dir={absolute_cache_dir}"

    errors = []
    stage1_time = 0
    stage2_time = 0

    # STAGE 1: Initialize build environment with first example
    if example_paths:
        first_example = example_paths[0]
        
        locked_print(f"*** Stage 1: Building {first_example.name} to initialize libraries ***")
        
        # Find the .ino file in the example directory
        ino_files = list(first_example.glob("*.ino"))
        if not ino_files:
            error_msg = f"No .ino file found in {first_example}"
            locked_print(f"ERROR: {error_msg}")
            return False, error_msg

        # Use the first .ino file found
        ino_file = ino_files[0]

        # Build Stage 1 pio ci command with --disable-auto-clean
        stage1_cmd_list = [
            "pio",
            "ci",
            str(ino_file),
            "--board",
            real_board_name,
            "--keep-build-dir",
            "--disable-auto-clean",  # Key optimization: preserve compiled libraries
            "--build-dir",
            str(board_build_dir / first_example.name),
            "--project-option",
            lib_option,
            "--project-option",
            cache_option,
        ]

        # Add platform-specific options for Stage 1
        if board.platform:
            stage1_cmd_list.extend(["--project-option", f"platform={board.platform}"])
        if board.platform_packages:
            stage1_cmd_list.extend(
                ["--project-option", f"platform_packages={board.platform_packages}"]
            )
        if board.framework:
            stage1_cmd_list.extend(["--project-option", f"framework={board.framework}"])
        if board.board_build_core:
            stage1_cmd_list.extend(
                ["--project-option", f"board_build.core={board.board_build_core}"]
            )
        if board.board_build_filesystem_size:
            stage1_cmd_list.extend(
                [
                    "--project-option",
                    f"board_build.filesystem_size={board.board_build_filesystem_size}",
                ]
            )

        # Add defines and build flags for Stage 1
        all_defines = defines.copy()
        if board.defines:
            all_defines.extend(board.defines)

        build_flags_list = []
        build_flags_list.append("-fopt-info-all=optimization_report.txt")
        
        if all_defines:
            build_flags_list.extend(f"-D{define}" for define in all_defines)

        if build_flags_list:
            build_flags_str = " ".join(build_flags_list)
            stage1_cmd_list.extend(["--project-option", f"build_flags={build_flags_str}"])

        if board.customsdk:
            stage1_cmd_list.extend(["--project-option", f"custom_sdkconfig={board.customsdk}"])

        if verbose:
            stage1_cmd_list.append("--verbose")

        # Execute Stage 1
        stage1_start = time.time()
        stage1_success, stage1_error = _execute_pio_command(
            stage1_cmd_list, first_example.name, board_name, verbose, "Stage 1"
        )
        stage1_time = time.time() - stage1_start

        if not stage1_success:
            return False, f"Stage 1 failed: {stage1_error}"

        locked_print(f"*** Stage 1 completed in {stage1_time:.2f}s - Libraries cached ***")

    # STAGE 2: Batch compilation of remaining examples using cached libraries
    if len(example_paths) > 1:
        locked_print(f"*** Stage 2: Batch building remaining {len(example_paths)-1} examples ***")
        
        stage2_start = time.time()
        
        for example_path in example_paths[1:]:
            locked_print(f"*** Stage 2: Building {example_path.name} using cached libraries ***")
            
            # Find the .ino file
            ino_files = list(example_path.glob("*.ino"))
            if not ino_files:
                error_msg = f"No .ino file found in {example_path}"
                locked_print(f"ERROR: {error_msg}")
                errors.append(error_msg)
                continue

            ino_file = ino_files[0]

            # Build Stage 2 pio ci command with LDF mode chain for faster dependency resolution
            stage2_cmd_list = [
                "pio",
                "ci",
                str(ino_file),
                "--board",
                real_board_name,
                "--keep-build-dir",
                "--build-dir",
                str(board_build_dir / example_path.name),
                "--project-option",
                lib_option,
                "--project-option",
                cache_option,
                "--project-option",
                "lib_ldf_mode=chain",  # Optimization: use chain mode for faster resolution
            ]

            # Add platform-specific options for Stage 2
            if board.platform:
                stage2_cmd_list.extend(["--project-option", f"platform={board.platform}"])
            if board.platform_packages:
                stage2_cmd_list.extend(
                    ["--project-option", f"platform_packages={board.platform_packages}"]
                )
            if board.framework:
                stage2_cmd_list.extend(["--project-option", f"framework={board.framework}"])
            if board.board_build_core:
                stage2_cmd_list.extend(
                    ["--project-option", f"board_build.core={board.board_build_core}"]
                )
            if board.board_build_filesystem_size:
                stage2_cmd_list.extend(
                    [
                        "--project-option",
                        f"board_build.filesystem_size={board.board_build_filesystem_size}",
                    ]
                )

            # Add defines and build flags for Stage 2
            if build_flags_list:
                build_flags_str = " ".join(build_flags_list)
                stage2_cmd_list.extend(["--project-option", f"build_flags={build_flags_str}"])

            if board.customsdk:
                stage2_cmd_list.extend(["--project-option", f"custom_sdkconfig={board.customsdk}"])

            if verbose:
                stage2_cmd_list.append("--verbose")

            # Execute Stage 2 build
            stage2_success, stage2_error = _execute_pio_command(
                stage2_cmd_list, example_path.name, board_name, verbose, "Stage 2"
            )

            if not stage2_success:
                errors.append(f"Stage 2 failed for {example_path.name}: {stage2_error}")

        stage2_time = time.time() - stage2_start
        locked_print(f"*** Stage 2 completed in {stage2_time:.2f}s ***")

    total_time = stage1_time + stage2_time
    locked_print(f"*** Two-Stage Build Summary ***")
    locked_print(f"Stage 1 (Library Init): {stage1_time:.2f}s")
    locked_print(f"Stage 2 (Batch Build): {stage2_time:.2f}s")
    locked_print(f"Total Time: {total_time:.2f}s")
    locked_print(f"Examples Built: {len(example_paths)}")

    if errors:
        return False, "\n".join(errors)

    return True, f"Successfully compiled all {len(example_paths)} examples for {board_name} in {total_time:.2f}s"


def _execute_pio_command(
    cmd_list: list[str],
    example_name: str,
    board_name: str,
    verbose: bool,
    stage_name: str = ""
) -> tuple[bool, str]:
    """Execute a PlatformIO command and return success status and error message."""
    
    cmd_str = subprocess.list2cmdline(cmd_list)
    stage_prefix = f"{stage_name}: " if stage_name else ""
    locked_print(f"{stage_prefix}Building {example_name} for {board_name}...")
    
    if verbose:
        locked_print(f"Command: {cmd_str}")

    start_time = time.time()

    try:
        # Launch subprocess.Popen and capture output line by line with timestamps
        result = subprocess.Popen(
            cmd_list,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(HERE.parent),
        )

        # Capture output lines in real-time with timestamp buffer
        stdout_lines = []
        timestamped_lines = []

        if result.stdout:
            for line in iter(result.stdout.readline, ""):
                if line:
                    line_stripped = line.rstrip()
                    stdout_lines.append(line_stripped)

                    # Add elapsed time since build started with 2 decimal places
                    elapsed_time = time.time() - start_time
                    timestamp = f"{elapsed_time:.2f}"
                    timestamped_line = f"{timestamp} {line_stripped}"
                    timestamped_lines.append(timestamped_line)

                    if verbose:
                        # In verbose mode, show each line immediately with timestamp
                        locked_print(timestamped_line)
                    else:
                        # In normal mode, show only essential build steps
                        line_lower = line_stripped.lower()
                        show_line = False

                        # Show actual source file compilation (but not compiler commands)
                        if "compiling .pio" in line_lower:
                            show_line = True
                        # Show linking step
                        elif (
                            line_stripped.startswith("Linking")
                            or "linking" in line_lower
                        ):
                            show_line = True
                        # Show memory usage
                        elif line_stripped.startswith(
                            "RAM:"
                        ) or line_stripped.startswith("Flash:"):
                            show_line = True
                        # Show build results
                        elif any(
                            result in line_stripped
                            for result in ["SUCCESS", "FAILED"]
                        ):
                            show_line = True
                        # Show errors and warnings (but avoid long command lines)
                        elif (
                            "error:" in line_lower or "warning:" in line_lower
                        ) and not line_stripped.startswith("avr-"):
                            show_line = True
                        # Show "Building in release mode" but not compiler commands
                        elif (
                            line_stripped == "Building in release mode"
                            or line_stripped == "Building in debug mode"
                        ):
                            show_line = True

                        if show_line:
                            locked_print(f"{stage_prefix}{timestamped_line}")

        # Wait for process to complete
        result.wait()

        stdout = "\n".join(stdout_lines)
        returncode = result.returncode
        elapsed_time = time.time() - start_time

        if returncode == 0:
            locked_print(
                f"{stage_prefix}Successfully built {example_name} for {board_name} in {elapsed_time:.2f}s"
            )
            return True, ""
        else:
            error_msg = f"Failed to build {example_name} for {board_name}"
            locked_print(f"ERROR: {stage_prefix}{error_msg}")
            if verbose:
                locked_print(f"Command: {cmd_str}")
                if stdout:
                    locked_print(f"STDOUT:\n{stdout}")
            return False, error_msg

    except subprocess.TimeoutExpired:
        error_msg = f"Timeout building {example_name} for {board_name}"
        locked_print(f"ERROR: {stage_prefix}{error_msg}")
        return False, error_msg
    except Exception as e:
        error_msg = f"Exception building {example_name} for {board_name}: {e}"
        locked_print(f"ERROR: {stage_prefix}{error_msg}")
        return False, error_msg


def run_symbol_analysis(boards: list[Board]) -> None:
    """Run symbol analysis on compiled outputs if requested."""
    locked_print("\nRunning symbol analysis on compiled outputs...")

    for board in boards:
        if board.board_name == "web":
            continue

        try:
            locked_print(f"Running symbol analysis for board: {board.board_name}")

            cmd = [
                "uv",
                "run",
                "ci/ci/symbol_analysis.py",
                "--board",
                board.board_name,
            ]

            result = subprocess.run(
                cmd, capture_output=True, text=True, cwd=str(HERE.parent)
            )

            if result.returncode != 0:
                locked_print(
                    f"ERROR: Symbol analysis failed for board {board.board_name}: {result.stderr}"
                )
            else:
                locked_print(f"Symbol analysis completed for board: {board.board_name}")
                if result.stdout:
                    print(result.stdout)

        except Exception as e:
            locked_print(
                f"ERROR: Exception during symbol analysis for board {board.board_name}: {e}"
            )


def compile_with_pio_ci(
    board: Board,
    example_paths: list[Path],
    build_dir: str | None,
    defines: list[str],
    verbose: bool,
) -> tuple[bool, str]:
    """Compile examples for a board using pio ci command (original implementation)."""

    # Skip web boards
    if board.board_name == "web":
        locked_print(f"Skipping web target for board {board.board_name}")
        return True, ""

    board_name = board.board_name
    real_board_name = board.get_real_board_name()

    # Set up build directory
    if build_dir:
        board_build_dir = Path(build_dir) / board_name
    else:
        board_build_dir = Path(".build") / board_name

    board_build_dir.mkdir(parents=True, exist_ok=True)

    # Generate build_info.json for this board
    generate_build_info(board, board_build_dir, defines)

    locked_print(f"*** Compiling examples for board {board_name} using pio ci ***")

    errors = []

    for example_path in example_paths:
        locked_print(
            f"*** Building example {example_path.name} for board {board_name} ***"
        )

        # Find the .ino file in the example directory
        ino_files = list(example_path.glob("*.ino"))
        if not ino_files:
            error_msg = f"No .ino file found in {example_path}"
            locked_print(f"ERROR: {error_msg}")
            errors.append(error_msg)
            continue

        # Use the first .ino file found
        ino_file = ino_files[0]

        # Get absolute path to FastLED library using platform's natural path format
        fastled_path = str(HERE.parent.absolute())
        lib_option = f"lib_deps=symlink://{fastled_path}"

        # Set up board-specific build cache directory with absolute path
        cache_dir = HERE.parent / ".pio_cache" / board.board_name
        absolute_cache_dir = str(cache_dir.absolute())
        cache_option = f"build_cache_dir={absolute_cache_dir}"

        # Build pio ci command
        cmd_list = [
            "pio",
            "ci",
            str(ino_file),
            "--board",
            real_board_name,
            "--keep-build-dir",
            "--build-dir",
            str(board_build_dir / example_path.name),
            "--project-option",
            lib_option,
            "--project-option",
            cache_option,
        ]

        # Add platform-specific options
        if board.platform:
            cmd_list.extend(["--project-option", f"platform={board.platform}"])

        if board.platform_packages:
            cmd_list.extend(
                ["--project-option", f"platform_packages={board.platform_packages}"]
            )

        if board.framework:
            cmd_list.extend(["--project-option", f"framework={board.framework}"])

        if board.board_build_core:
            cmd_list.extend(
                ["--project-option", f"board_build.core={board.board_build_core}"]
            )

        if board.board_build_filesystem_size:
            cmd_list.extend(
                [
                    "--project-option",
                    f"board_build.filesystem_size={board.board_build_filesystem_size}",
                ]
            )

        # Add defines and include paths
        all_defines = defines.copy()
        if board.defines:
            all_defines.extend(board.defines)

        build_flags_list = []

        # Add optimization report flag for all builds (generates optimization_report.txt)
        # The report will be created in the build directory where GCC runs
        build_flags_list.append("-fopt-info-all=optimization_report.txt")

        # Add defines as build flags
        if all_defines:
            build_flags_list.extend(f"-D{define}" for define in all_defines)

        # Add build flags directly using project options
        if build_flags_list:
            build_flags_str = " ".join(build_flags_list)
            cmd_list.extend(["--project-option", f"build_flags={build_flags_str}"])

            # Show custom defines (excluding the optimization report flag)
            custom_defines = [
                flag
                for flag in build_flags_list
                if flag.startswith("-D") and "FASTLED" in flag
            ]
            if custom_defines or verbose:
                if custom_defines:
                    locked_print(f"Custom defines: {' '.join(custom_defines)}")
                if verbose:
                    locked_print(f"All build flags: {build_flags_str}")

        # Add custom SDK config if specified
        if board.customsdk:
            cmd_list.extend(["--project-option", f"custom_sdkconfig={board.customsdk}"])
            locked_print(f"Using custom SDK config: {board.customsdk}")

        # Only add verbose flag to pio ci when explicitly requested
        if verbose:
            cmd_list.append("--verbose")

        # Execute the command using the helper function
        success, error = _execute_pio_command(
            cmd_list, example_path.name, board_name, verbose
        )

        if not success:
            errors.append(f"{example_path.name}: {error}")

    if errors:
        return False, "\n".join(errors)

    return True, f"Successfully compiled all examples for {board_name}"


def main() -> int:
    """Main function."""
    args = parse_args()

    if args.supported_boards:
        print(",".join(DEFAULT_BOARDS_NAMES))
        return 0

    # Determine which boards to compile for
    if args.interactive:
        boards_names = choose_board_interactively(
            DEFAULT_BOARDS_NAMES + OTHER_BOARDS_NAMES
        )
    else:
        boards_names = args.boards.split(",") if args.boards else DEFAULT_BOARDS_NAMES

    # Get board objects
    boards: list[Board] = []
    for board_name in boards_names:
        try:
            board = get_board(board_name, no_project_options=False)
            boards.append(board)
        except Exception as e:
            locked_print(f"ERROR: Failed to get board '{board_name}': {e}")
            return 1

    # Determine which examples to compile
    if args.positional_examples:
        # Convert positional examples, handling both "examples/Blink" and "Blink" formats
        examples = []
        for example in args.positional_examples:
            # Remove "examples/" prefix if present
            if example.startswith("examples/"):
                example = example[len("examples/") :]
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
    example_paths: list[Path] = []
    for example in examples:
        try:
            example_path = resolve_example_path(example)
            example_paths.append(example_path)
        except FileNotFoundError as e:
            locked_print(f"ERROR: {e}")
            return 1

    # Add extra examples for specific boards
    extra_examples: dict[Board, list[Path]] = {}
    for board in boards:
        if board in EXTRA_EXAMPLES:
            board_examples = []
            for example in EXTRA_EXAMPLES[board]:
                try:
                    board_examples.append(resolve_example_path(example))
                except FileNotFoundError as e:
                    locked_print(f"WARNING: {e}")
            if board_examples:
                extra_examples[board] = board_examples

    # Set up defines
    defines: list[str] = []
    if args.defines:
        defines.extend(args.defines.split(","))

    # Add FASTLED_ALL_SRC define when --allsrc or --no-allsrc flag is specified
    if args.allsrc:
        defines.append("FASTLED_ALL_SRC=1")
    elif args.no_allsrc:
        defines.append("FASTLED_ALL_SRC=0")

    # Start compilation
    start_time = time.time()
    locked_print(
        f"Starting compilation for {len(boards)} boards with {len(example_paths)} examples"
    )

    compilation_errors = []

    # Choose compilation approach based on --optimize-build flag
    if args.optimize_build:
        locked_print("*** Using Two-Stage Build Optimization ***")
        compile_function = compile_with_pio_ci_optimized
    else:
        locked_print("*** Using Standard Build Process ***")
        compile_function = compile_with_pio_ci

    # Compile for each board
    for board in boards:
        board_examples = example_paths.copy()
        if board in extra_examples:
            board_examples.extend(extra_examples[board])

        success, message = compile_function(
            board=board,
            example_paths=board_examples,
            build_dir=args.build_dir,
            defines=defines,
            verbose=args.verbose,
        )

        if not success:
            compilation_errors.append(f"Board {board.board_name}: {message}")
            locked_print(f"ERROR: Compilation failed for board {board.board_name}")
            # Continue with other boards instead of stopping

    # Run symbol analysis if requested
    if args.symbols:
        run_symbol_analysis(boards)

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
