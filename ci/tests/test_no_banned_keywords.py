import os
import unittest
from typing import Callable, List

from ci.check_files import (
    EXCLUDED_FILES,
    FileContent,
    FileContentChecker,
    MultiCheckerFileProcessor,
    collect_files_to_check,
)
from ci.paths import PROJECT_ROOT


SRC_ROOT = PROJECT_ROOT / "src"

# Banned keywords that are macros on some platforms
BANNED_KEYWORDS = [
    "PSTR",  # Program memory string macro on AVR
    "F",     # Flash memory macro on AVR
    "min",   # min macro on some platforms
    "max",   # max macro on some platforms
]


class BannedKeywordsChecker(FileContentChecker):
    """Checker class for banned keywords."""

    def __init__(self, banned_keywords_list: List[str]):
        """Initialize with the list of banned keywords to check for."""
        self.banned_keywords_list = banned_keywords_list

    def should_process_file(self, file_path: str) -> bool:
        """Check if file should be processed for banned keywords."""
        # Check file extension
        if not file_path.endswith((".cpp", ".h", ".hpp", ".ino")):
            return False

        # Check if file is in excluded list
        if any(file_path.endswith(excluded) for excluded in EXCLUDED_FILES):
            return False

        return True

    def check_file_content(self, file_content: FileContent) -> List[str]:
        """Check file content for banned keywords."""
        failings = []

        if len(self.banned_keywords_list) == 0:
            return failings

        # Check each line for banned keywords
        for line_number, line in enumerate(file_content.lines, 1):
            # Skip comments
            stripped_line = line.strip()
            if stripped_line.startswith("//") or stripped_line.startswith("/*"):
                continue

            # Skip preprocessor directives (except for specific cases)
            if stripped_line.startswith("#"):
                continue

            # Check for banned keywords in the line
            for keyword in self.banned_keywords_list:
                # Look for the keyword as a standalone word (not part of another word)
                # This regex-like approach checks for word boundaries
                import re
                pattern = r'\b' + re.escape(keyword) + r'\b'
                if re.search(pattern, line) and "// ok keyword" not in line:
                    failings.append(
                        f"Found banned keyword '{keyword}' in {file_content.path}:{line_number}"
                    )

        return failings


def _test_no_banned_keywords(
    test_directories: list[str],
    banned_keywords_list: List[str],
    on_fail: Callable[[str], None],
) -> None:
    """Searches through the program files to check for banned keywords."""
    # Collect files to check
    files_to_check = collect_files_to_check(test_directories)

    # Create processor and checker
    processor = MultiCheckerFileProcessor()
    checker = BannedKeywordsChecker(banned_keywords_list)

    # Process files
    results = processor.process_files_with_checkers(files_to_check, [checker])

    # Get results for banned keywords checker
    all_failings = results.get("BannedKeywordsChecker", []) or []

    if all_failings:
        msg = f"Found {len(all_failings)} banned keyword(s): \n" + "\n".join(
            all_failings
        )
        for failing in all_failings:
            print(failing)

        on_fail(msg)
    else:
        print("No banned keywords found.")


class TestNoBannedKeywords(unittest.TestCase):
    def test_no_banned_keywords_src(self) -> None:
        """Searches through the source files to check for banned keywords."""

        def on_fail(msg: str) -> None:
            self.fail(
                msg + "\n"
                "You can add '// ok keyword' at the end of the line to silence this error for specific usages."
            )

        # Test directories as requested
        test_directories = [
            os.path.join(SRC_ROOT, "fl"),
            os.path.join(SRC_ROOT, "fx"),
            os.path.join(SRC_ROOT, "sensors"),
        ]
        _test_no_banned_keywords(
            test_directories=test_directories,
            banned_keywords_list=BANNED_KEYWORDS,
            on_fail=on_fail,
        )

    def test_no_banned_keywords_examples(self) -> None:
        """Searches through the examples to check for banned keywords."""

        def on_fail(msg: str) -> None:
            self.fail(
                msg + "\n"
                "You can add '// ok keyword' at the end of the line to silence this error for specific usages."
            )

        test_directories = ["examples"]

        _test_no_banned_keywords(
            test_directories=test_directories,
            banned_keywords_list=BANNED_KEYWORDS,
            on_fail=on_fail,
        )

    def test_no_banned_keywords_platforms(self) -> None:
        """Searches through the platforms directory to check for banned keywords."""

        def on_fail(msg: str) -> None:
            self.fail(
                msg + "\n"
                "You can add '// ok keyword' at the end of the line to silence this error for specific usages."
            )

        # Test the platforms directory
        test_directories = [
            os.path.join(SRC_ROOT, "platforms"),
        ]
        _test_no_banned_keywords(
            test_directories=test_directories,
            banned_keywords_list=BANNED_KEYWORDS,
            on_fail=on_fail,
        )


if __name__ == "__main__":
    unittest.main()
