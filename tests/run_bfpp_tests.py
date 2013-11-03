import re
import sys
import math
import argparse
import subprocess

from pathlib import Path

def decode_bytes(value):
    result = bytearray()
    escapes = {ord("n"): 10, ord("r"): 13, ord("t"): 9, ord("\\"): 92}
    i = 0
    while i < len(value):
        if value[i] != ord("\\"):
            result.append(value[i])
            i += 1
            continue
        
        i += 1
        if i == len(value):
            raise ValueError("unfinished escape")
        
        if value[i] in escapes:
            result.append(escapes[value[i]])
            i += 1
        elif value[i] == ord("x"):
            digits = value[i + 1:i + 3]
            if not re.fullmatch(rb"[0-9a-fA-F]{2}", digits):
                raise ValueError("\\x requires exactly two hexadecimal digits")
            result.append(int(digits, 16))
            i += 3
        else:
            raise ValueError(f"unknown escape: {value[i:i + 1]!r}")
        
    return bytes(result)

def read_expectations(source):
    comments = list(re.finditer(rb"/([^/]*)/", source))
    if source.count(b"/") % 2:
        raise ValueError("unterminated BFPP comment")
    
    values = {}
    output_end = None
    for comment in comments:
        body = comment.group(1)
        for key in (b"INPUT", b"OUTPUT"):
            if not body.lstrip().startswith(key + b":"):
                continue
            prefix = b" " + key + b": "
            if (not body.startswith(prefix) or not body.endswith(b" ")
                    or len(body) < len(prefix) + 1):
                raise ValueError(f"use / {key.decode()}: value / with framing spaces")
            
            if key in values:
                raise ValueError(f"duplicate {key.decode()} section")
            
            values[key] = decode_bytes(body[len(prefix):-1])
            if key == b"OUTPUT":
                output_end = comment.end()

    if output_end is None:
        raise ValueError("missing final / OUTPUT: value / comment")
    
    if source[output_end:].strip():
        raise ValueError("OUTPUT must be the final section")
    
    return values.get(b"INPUT", b""), values[b"OUTPUT"]

def run_case(path, command, timeout):
    input_bytes, expected = read_expectations(path.read_bytes())
    completed = subprocess.run(
        [*command, str(path.resolve())], input=input_bytes,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout,
    )
    
    if completed.returncode != 0:
        raise ValueError(
            f"interpreter exited with {completed.returncode}; stderr={completed.stderr!r}"
        )
    
    if completed.stdout != expected:
        raise ValueError(f"expected {expected!r}, got {completed.stdout!r}")

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--cases", type=Path, default=Path(__file__).resolve().parent / "cases",
        help="directory containing .bfpp fixtures",
    )
    parser.add_argument("--file", type=Path, help="run a single .bfpp fixture")
    parser.add_argument("--timeout", type=float, default=5.0, help="seconds per fixture")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- interpreter [arguments]")
    args = parser.parse_args(argv)
    command = args.command
    
    if command[:1] == ["--"]:
        command = command[1:]
    
    if not command:
        parser.error("supply an interpreter command after --")
    
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--timeout must be finite and positive")
    
    paths = [args.file] if args.file else sorted(args.cases.glob("*.bfpp"))
    if not paths:
        parser.error(f"no .bfpp fixtures found in {args.cases}")
    
    failures = 0
    for path in paths:
        try:
            run_case(path, command, args.timeout)
        except (OSError, ValueError, subprocess.TimeoutExpired) as error:
            print(f"[FAIL] {path.name}: {error}", file=sys.stderr)
            failures += 1
        else:
            print(f"[ OK ] {path.name}")
    
    print(f"{len(paths) - failures}/{len(paths)} BFPP tests passed")
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
