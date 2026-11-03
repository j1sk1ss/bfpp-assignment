import subprocess
import tempfile
import unittest

from pathlib import Path
from unittest.mock import patch
from run_bfpp_tests import decode_bytes, read_expectations, run_case

class FixtureFormatTests(unittest.TestCase):
    def test_binary_escapes(self):
        self.assertEqual(decode_bytes(rb"A\n\r\t\\\x00\xff\x2F"), b"A\n\r\t\\\x00\xff/")

    def test_preserves_payload_whitespace(self):
        self.assertEqual(read_expectations(b"/ OUTPUT:  A  /\n"), (b"", b" A "))

    def test_input_and_empty_output(self):
        self.assertEqual(read_expectations(b"/ INPUT: Hi\n /\n!\n/ OUTPUT:  /"), (b"Hi\n", b""))

    def test_ordinary_comments_do_not_supply_expectations(self):
        source = b"/ prose mentioning OUTPUT: wrong /\n.\n/ OUTPUT: \\x00 /"
        self.assertEqual(read_expectations(source), (b"", b"\x00"))

    def test_rejects_bad_metadata(self):
        for source in (
            b"+.", b"/ OUTPUT: A /+", b"/ OUTPUT: A / / another comment /",
            b"/ OUTPUT: A / / OUTPUT: B /", b"/ INPUT: A / / INPUT: B / / OUTPUT:  /",
            b"/ OUTPUT:A/", b"/ OUTPUT: /", b"/ OUTPUT: A / / unfinished", b"/ OUTPUT: \\q /",
            b"/ OUTPUT: \\x1 /", b"/ OUTPUT: \\xGG /", b"/ OUTPUT: \\ /",
        ):
            with self.subTest(source=source), self.assertRaises(ValueError):
                read_expectations(source)

class ExecutionTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "case.bfpp"
        self.path.write_bytes(b"/ INPUT: Q /\n,.\n/ OUTPUT: Q /\n")

    @patch("run_bfpp_tests.subprocess.run")
    def test_forwards_input_and_command(self, run):
        run.return_value = subprocess.CompletedProcess([], 0, b"Q", b"")
        run_case(self.path, ["bfpp", "--tape=fixed"], 1)
        self.assertEqual(run.call_args.args[0], ["bfpp", "--tape=fixed", str(self.path)])
        self.assertEqual(run.call_args.kwargs["input"], b"Q")
        self.assertEqual(run.call_args.kwargs["timeout"], 1)

    @patch("run_bfpp_tests.subprocess.run")
    def test_rejects_extra_newline(self, run):
        run.return_value = subprocess.CompletedProcess([], 0, b"Q\n", b"")
        with self.assertRaisesRegex(ValueError, "expected"):
            run_case(self.path, ["bfpp"], 1)

    @patch("run_bfpp_tests.subprocess.run")
    def test_rejects_failure_even_with_expected_output(self, run):
        run.return_value = subprocess.CompletedProcess([], 1, b"Q", b"runtime error")
        with self.assertRaisesRegex(ValueError, "runtime error"):
            run_case(self.path, ["bfpp"], 1)

    @patch("run_bfpp_tests.subprocess.run")
    def test_timeout_is_propagated(self, run):
        run.side_effect = subprocess.TimeoutExpired(["bfpp"], 1)
        with self.assertRaises(subprocess.TimeoutExpired):
            run_case(self.path, ["bfpp"], 1)

if __name__ == "__main__":
    unittest.main()
