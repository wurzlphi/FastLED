"""
Enhanced compilation script with incremental build support and optimized path handling.

This is the next-generation FastLED compilation system that:
- Supports incremental builds by preserving build artifacts
- Has better path normalization and handling
- Provides faster compilation through smart caching
- Maintains compatibility with the original system
"""
import argparse
import os
import sys
import time
import warnings
from pathlib import Path

from ci.boards import Board, get_board  # type: ignore
from ci.concurrent_run_v2 import ConcurrentRunArgsV2, concurrent_run_v2, print_build_summary
from ci.locked_print import locked_print

HERE = Path(__file__).parent.resolve()

LIBS = ["src"]
EXTRA_LIBS = [
    "https://github.com/me-no-dev/ESPAsyncWebServer.git",
    "ArduinoOTA",
    "SD",
    "FS", 
    "ESPmDNS",
    "WiFi",
    "WebSockets",
]
BUILD_FLAGS = ["-Wl,-Map,firmware.map", "-fopt-info-all=optimization_report.txt"]

# Default boards to compile for
DEFAULT_BOARDS_NAMES = [
    "apollo3_red",
    "apollo3_thing_explorable",
    "web",
    "uno",  # Build is faster if this is first
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

OTHER_BOARDS_NAMES = [
    "nano_every",
    "esp32-c2-devkitm-1",
]

# Examples to compile
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
    """Parse command line arguments with enhanced options for the v2 build system."""
    parser = argparse.ArgumentParser(
        description="FastLED Enhanced Compilation System with Incremental Builds",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s uno --examples Blink,Animartrix,apa102,apa102hd
  %(prog)s esp32dev --examples Blink --force-rebuild
  %(prog)s teensy31 --examples Fire2012 --clean-cache
  %(prog)s uno,esp32dev --examples Blink --symbols
        """,
    )
    
    # Positional board argument
    parser.add_argument(
        "boards",
        type=str,
        help="Comma-separated list of boards to compile for",
        nargs="?",
    )
    
    # Positional examples (for backwards compatibility)
    parser.add_argument(
        "positional_examples",
        type=str,
        help="Examples to compile (positional arguments after board name)",
        nargs="*",
    )
    
    # Named arguments
    parser.add_argument(
        "--examples", 
        type=str, 
        help="Comma-separated list of examples to compile"
    )
    parser.add_argument(
        "--exclude-examples", 
        type=str, 
        help="Examples that should be excluded"
    )
    parser.add_argument(
        "--skip-init", 
        action="store_true", 
        help="Skip the initialization step"
    )
    parser.add_argument(
        "--defines", 
        type=str, 
        help="Comma-separated list of compiler definitions"
    )
    parser.add_argument(
        "--customsdk", 
        type=str, 
        help="custom_sdkconfig project option"
    )
    parser.add_argument(
        "--extra-packages",
        type=str,
        help="Comma-separated list of extra packages to install",
    )
    parser.add_argument(
        "--add-extra-esp32-libs",
        action="store_true",
        help="Add extra libraries to the libraries list to check against compiler errors.",
    )
    parser.add_argument(
        "--build-dir", 
        type=str, 
        help="Override the default build directory"
    )
    parser.add_argument(
        "--no-project-options",
        action="store_true",
        help="Don't use custom project options",
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Enable interactive mode to choose a board",
    )
    parser.add_argument(
        "--no-interactive", 
        action="store_true", 
        help="Disable interactive mode"
    )
    parser.add_argument(
        "-v", "--verbose", 
        action="store_true", 
        help="Enable verbose output"
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
    
    # Enhanced v2 build system options
    parser.add_argument(
        "--force-rebuild",
        action="store_true",
        help="Force full rebuild (disable incremental compilation)",
    )
    parser.add_argument(
        "--clean-cache",
        action="store_true",
        help="Clean build cache before compilation",
    )
    parser.add_argument(
        "--show-cache-stats",
        action="store_true",
        help="Show build cache statistics",
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
    
    # Check for environment variable overrides
    if os.environ.get("FASTLED_CI_NO_INTERACTIVE") == "true":
        args.interactive = False
        args.no_interactive = True
    
    # Handle conflicting arguments
    if args.interactive and args.no_interactive:
        warnings.warn(
            "Both --interactive and --no-interactive were passed, --no-interactive takes precedence."
        )
        args.interactive = False
    
    if args.allsrc and args.no_allsrc:
        warnings.warn(
            "Both --allsrc and --no-allsrc were passed, this is contradictory. Please specify only one."
        )
        sys.exit(1)
    
    return args


def remove_duplicates(items: list[str]) -> list[str]:
    """Remove duplicates while preserving order."""
    seen = set()
    out = []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out


def choose_board_interactively(boards: list[str]) -> list[str]:
    """Interactive board selection."""
    print("Available boards:")
    boards = remove_duplicates(sorted(boards))
    for i, board in enumerate(boards):
        print(f"[{i}]: {board}")
    print("[all]: All boards")
    
    out: list[str] = []
    while True:
        try:
            input_str = input(
                "Enter the number of the board(s) you want to compile to, or their name(s): "
            )
            if "all" in input_str:
                return boards
            
            for board in input_str.split(","):
                board = board.strip()
                if board == "":
                    continue
                if not board.isdigit():
                    out.append(board)  # Assume it's a board name
                else:
                    index = int(board)
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
    """Resolve example name to full path."""
    # Normalize example name - remove examples/ prefix if present
    if example.startswith("examples/"):
        example = example[len("examples/"):]
    
    example_path = HERE.parent / "examples" / example
    if not example_path.exists():
        raise FileNotFoundError(f"Example '{example}' not found at '{example_path}'")
    return example_path


def create_concurrent_run_args_v2(args: argparse.Namespace) -> ConcurrentRunArgsV2:
    """Create arguments for the enhanced concurrent run system."""
    skip_init = args.skip_init
    
    # Handle board selection
    if args.interactive:
        boards = choose_board_interactively(DEFAULT_BOARDS_NAMES + OTHER_BOARDS_NAMES)
    else:
        boards = args.boards.split(",") if args.boards else DEFAULT_BOARDS_NAMES
    
    # Create board objects
    projects: list[Board] = []
    for board in boards:
        projects.append(get_board(board, no_project_options=args.no_project_options))
    
    # Handle example selection
    extra_examples: dict[Board, list[Path]] = {}
    
    if args.positional_examples:
        # Convert positional examples, handling both "examples/Blink" and "Blink" formats
        examples = []
        for example in args.positional_examples:
            if example.startswith("examples/"):
                example = example[len("examples/"):]
            examples.append(example)
    elif args.examples:
        examples = args.examples.split(",")
    else:
        examples = DEFAULT_EXAMPLES
        # Only add extra examples when using defaults
        for b, _examples in EXTRA_EXAMPLES.items():
            resolved_examples = [resolve_example_path(example) for example in _examples]
            extra_examples[b] = resolved_examples
    
    # Resolve example paths
    examples_paths = [resolve_example_path(example) for example in examples]
    
    # Process example exclusions
    if args.exclude_examples:
        exclude_examples = args.exclude_examples.split(",")
        examples_paths = [
            example
            for example in examples_paths
            if example.name not in exclude_examples
        ]
        for exclude in exclude_examples:
            if exclude in examples:
                examples.remove(exclude)
    
    # Handle defines
    defines: list[str] = []
    if args.defines:
        defines.extend(args.defines.split(","))
    
    # Add FASTLED_ALL_SRC define when --allsrc or --no-allsrc flag is specified
    if args.allsrc:
        defines.append("FASTLED_ALL_SRC=1")
    elif args.no_allsrc:
        defines.append("FASTLED_ALL_SRC=0")
    
    # Handle extra packages
    extra_packages: list[str] = []
    if args.extra_packages:
        extra_packages.extend(args.extra_packages.split(","))
    
    # Build configuration
    customsdk = args.customsdk
    build_dir = args.build_dir
    extra_scripts = "pre:lib/ci/ci-flags.py"  # This path might be wrong, but keeping for compatibility
    verbose = args.verbose
    
    # Create the enhanced concurrent run arguments
    out = ConcurrentRunArgsV2(
        projects=projects,
        examples=examples_paths,
        skip_init=skip_init,
        defines=defines,
        customsdk=customsdk,
        extra_packages=extra_packages,
        libs=LIBS,
        build_dir=build_dir,
        extra_scripts=extra_scripts,
        cwd=str(HERE.parent),
        board_dir=(HERE / "boards").absolute().as_posix(),
        build_flags=BUILD_FLAGS,
        verbose=verbose,
        extra_examples=extra_examples,
        symbols=args.symbols,
        force_rebuild=args.force_rebuild,
        clean_cache=args.clean_cache,
    )
    
    return out


def show_cache_stats(build_dir: str | None = None) -> None:
    """Show build cache statistics."""
    build_root = Path(build_dir) if build_dir else Path(".build")
    
    if not build_root.exists():
        locked_print("No build directory found.")
        return
    
    locked_print("\n" + "="*60)
    locked_print("BUILD CACHE STATISTICS")
    locked_print("="*60)
    
    total_cache_files = 0
    total_boards = 0
    
    for board_dir in build_root.glob("*/"):
        if not board_dir.is_dir():
            continue
            
        total_boards += 1
        cache_file = board_dir / "build_cache.json"
        
        if cache_file.exists():
            total_cache_files += 1
            try:
                import json
                with open(cache_file, 'r') as f:
                    cache_data = json.load(f)
                
                locked_print(f"\n📁 {board_dir.name}:")
                locked_print(f"   Cache entries: {len(cache_data)}")
                locked_print(f"   Cache file size: {cache_file.stat().st_size} bytes")
                locked_print(f"   Last modified: {time.ctime(cache_file.stat().st_mtime)}")
                
                # Show example breakdown
                if cache_data:
                    locked_print("   Examples cached:")
                    for cache_key in sorted(cache_data.keys()):
                        example_name = cache_key.split('_', 1)[1] if '_' in cache_key else cache_key
                        file_count = len(cache_data[cache_key])
                        locked_print(f"     - {example_name}: {file_count} files")
                        
            except Exception as e:
                locked_print(f"\n📁 {board_dir.name}: Error reading cache - {e}")
        else:
            locked_print(f"\n📁 {board_dir.name}: No cache file")
    
    locked_print(f"\nSummary:")
    locked_print(f"  Total boards: {total_boards}")
    locked_print(f"  Boards with cache: {total_cache_files}")
    locked_print(f"  Cache coverage: {(total_cache_files/max(total_boards, 1)*100):.1f}%")
    locked_print("="*60)


def main() -> int:
    """Main function for the enhanced compilation system."""
    args = parse_args()
    
    if args.supported_boards:
        print(",".join(DEFAULT_BOARDS_NAMES))
        return 0
    
    if args.show_cache_stats:
        show_cache_stats(args.build_dir)
        return 0
    
    if args.add_extra_esp32_libs:
        LIBS.extend(EXTRA_LIBS)
    
    # Create run arguments and start compilation
    run_args = create_concurrent_run_args_v2(args)
    
    locked_print("🚀 Starting FastLED Enhanced Compilation System v2")
    locked_print(f"📋 Configuration:")
    locked_print(f"   Boards: {[p.board_name for p in run_args.projects]}")
    locked_print(f"   Examples: {[e.name for e in run_args.examples]}")
    locked_print(f"   Build mode: {'Full rebuild' if run_args.force_rebuild else 'Incremental'}")
    locked_print(f"   Clean cache: {run_args.clean_cache}")
    locked_print(f"   Symbol analysis: {run_args.symbols}")
    
    start_time = time.time()
    rtn = concurrent_run_v2(args=run_args)
    build_time = (time.time() - start_time) / 60
    
    # Print build summary
    print_build_summary(
        run_args.projects, 
        run_args.examples,
        build_time,
        run_args.force_rebuild
    )
    
    if rtn == 0:
        locked_print("✅ All compilations completed successfully!")
    else:
        locked_print("❌ Compilation failed. Check the output above for details.")
    
    return rtn


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        locked_print("\n🛑 Compilation interrupted by user.")
        sys.exit(1)
    except Exception as e:
        locked_print(f"\n💥 Unexpected error: {e}")
        sys.exit(1)
