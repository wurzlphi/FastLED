import unittest

from ci.boards import get_board
from ci.concurrent_run import ConcurrentRunArgs, concurrent_run
from ci.paths import PROJECT_ROOT


class TestPioCiIntegration(unittest.TestCase):
    """Integration test for the concurrent PlatformIO CI pipeline.

    This test validates that the full cycle (initialize build directory → compile
    multiple examples) works using the high-level `concurrent_run` helper.  We
    deliberately choose a minimal workload (UNO board; Blink, Audio, Noise
    examples) to keep runtime reasonable while still exercising the complete
    code-path:  build-dir creation, project initialisation, example copy, and
    compilation.
    """

    EXAMPLES = ["Blink", "Audio", "Noise"]

    def test_full_cycle_uno(self) -> None:  # noqa: D401
        """Run the full concurrent build cycle for the Uno board."""
        # Board under test
        board = get_board("uno")

        # Resolve example directories
        example_paths = [PROJECT_ROOT / "examples" / ex for ex in self.EXAMPLES]

        # Construct arguments for the concurrent runner
        run_args = ConcurrentRunArgs(
            projects=[board],
            examples=example_paths,
            skip_init=False,
            defines=[],
            customsdk=None,
            extra_packages=[],
            libs=["src"],
            build_dir=None,  # default .build directory
            extra_scripts=None,  # use default FastLED CI scripts
            cwd=str(PROJECT_ROOT),
            board_dir=(PROJECT_ROOT / "ci" / "boards").as_posix(),
            build_flags=[
                "-Wl,-Map,firmware.map",
                "-fopt-info-all=optimization_report.txt",
            ],
            verbose=False,
        )

        # Execute build and assert success
        result = concurrent_run(run_args)
        self.assertEqual(
            result,
            0,
            "Concurrent build cycle failed – see test log for details.",
        )


if __name__ == "__main__":
    unittest.main()
