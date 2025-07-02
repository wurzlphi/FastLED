import argparse
import os
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path

MINIMUM_REPORT_SEVERTIY = "medium"
MINIMUM_FAIL_SEVERTIY = "high"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run cppcheck on the project")
    parser.add_argument("board", nargs="?", help="Board to check, optional")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    here = Path(__file__).parent
    project_root = here.parent
    
    # Get the FastLED source directory
    fastled_src_path = project_root / "src"
    if not fastled_src_path.exists():
        print(f"FastLED source directory not found: {fastled_src_path}")
        return 1

    # Create a temporary directory for the check project
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        
        # Create a simple platformio.ini for checking
        platformio_ini_content = f"""[env:check]
platform = atmelavr
board = uno
framework = arduino

[env]
check_tool = cppcheck
check_skip_packages = yes
check_severity = {MINIMUM_REPORT_SEVERTIY}
check_fail_on_defect = {MINIMUM_FAIL_SEVERTIY}
check_flags = --inline-suppr
"""
        
        platformio_ini_path = temp_path / "platformio.ini"
        platformio_ini_path.write_text(platformio_ini_content)
        
        # Create lib directory and symlink FastLED source
        lib_dir = temp_path / "lib"
        lib_dir.mkdir()
        fastled_lib_dir = lib_dir / "FastLED"
        
        # Copy the library.json to make it a proper library
        shutil.copytree(fastled_src_path, fastled_lib_dir / "src")
        shutil.copy2(project_root / "library.json", fastled_lib_dir / "library.json")
        
        # Create a dummy src directory with a minimal file for the check project
        src_dir = temp_path / "src"
        src_dir.mkdir()
        dummy_cpp = src_dir / "main.cpp"
        dummy_cpp.write_text("""
#include <Arduino.h>
#include <FastLED.h>

void setup() {}
void loop() {}
""")
        
        print(f"Running pio check on FastLED library in temporary project: {temp_path}")
        os.chdir(str(temp_path))
        
        # Run pio check command
        cp = subprocess.run(
            [
                "pio",
                "check",
                "--skip-packages",
                f"--severity={MINIMUM_REPORT_SEVERTIY}",
                f"--fail-on-defect={MINIMUM_FAIL_SEVERTIY}",
                "--flags",
                "--inline-suppr",
            ],
        )
        return cp.returncode


if __name__ == "__main__":
    sys.exit(main())
