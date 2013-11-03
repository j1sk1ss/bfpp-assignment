import sys
import json
import unittest
import subprocess

from pathlib import Path

FIXTURES: Path = Path(__file__).resolve().parent / "fixtures"
COMMAND = None

class CliTests(unittest.TestCase):
    def test_cases(self):
        self.assertIsNotNone(COMMAND, "pass the bfpp executable as the first argument")
        for case in json.loads((FIXTURES / "cli/cases.json").read_text()):
            with self.subTest(name=case["name"]):
                args = [arg.replace("{fixtures}", str(FIXTURES)) for arg in case["args"]]
                result = subprocess.run(
                    [str(COMMAND), *args],
                    input=(FIXTURES / case["input"]).read_bytes(),
                    capture_output=True, timeout=5,
                )
                self.assertEqual(result.returncode, case["status"])
                self.assertEqual(result.stdout, (FIXTURES / case["output"]).read_bytes())
                if case["error"]:
                    self.assertIn(case["error"].encode(), result.stderr)
                else:
                    self.assertEqual(result.stderr, b"")


if __name__ == "__main__":
    COMMAND = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
    unittest.main()
