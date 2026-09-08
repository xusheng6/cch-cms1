#!/usr/bin/env python3
"""Compare the portable engine with a JSON trace from emulate_cms1_search.py."""

from __future__ import annotations

import argparse
import hashlib
import os
import json
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


class NotSearchOracle(ValueError):
    pass


def has_no_best_move(trace: dict) -> bool:
    """DOS can exhaust every root on its first pass without setting a PV."""
    if trace.get("pv_words"):
        return False
    if trace.get("root_move_bytes") == 0:
        return True
    roots = trace.get("root_moves", [])
    return (bool(roots) and len(roots) * 6 == trace.get("root_move_bytes")
            and all(move.get("source") == 0xffff for move in roots)
            and trace.get("root_score", 0) <= -6500)


def dos_square_to_ucci(square: int) -> str:
    rank, file_word = divmod(square, 0x40)
    file = file_word // 2
    if not (0 <= file < 9 and 0 <= rank < 10 and file_word % 2 == 0):
        raise ValueError(f"invalid CMS1 square {square:#x}")
    return f"{chr(ord('a') + file)}{9-rank}"


def compare(path: Path, engine: Path, overrides: dict | None = None) -> dict:
    trace = json.loads(path.read_text())
    return compare_trace(trace, str(path), engine, overrides)


def compare_trace(trace: dict, label: str, engine: Path,
                  overrides: dict | None = None) -> dict:
    if not isinstance(trace, dict) or "pv_words" not in trace:
        raise NotSearchOracle("metadata or non-search diagnostic")
    pv = trace["pv_words"]
    if trace.get("forced_move"):
        raise NotSearchOracle("forced-move diagnostic")
    no_moves = has_no_best_move(trace)
    if (len(pv) < 2 and not no_moves) or not trace.get("returned", True):
        raise ValueError("not a completed unrestricted search oracle")
    expected_move = "0000" if no_moves else dos_square_to_ucci(pv[0]) + dos_square_to_ucci(pv[1])
    depth = trace["fixed_ply"]
    position_moves = trace.get("position_moves", [])
    if trace.get("fen"):
        position_command = "position fen " + trace["fen"]
        if position_moves:
            position_command += " moves " + " ".join(position_moves)
    elif position_moves:
        position_command = "position startpos moves " + " ".join(position_moves)
    elif trace["side_selector"] == 0x40:
        position_command = "position startpos"
    elif trace["side_selector"] == 0x20:
        position_command = ("position fen "
            "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR b")
    else:
        raise SystemExit(f"unknown CMS1 side selector {trace['side_selector']:#x}")
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith("CCH_")}
    environment["CCH_BOOK"] = "/dev/null"
    environment.update(overrides or {})
    completed = subprocess.run(
        [str(engine.resolve())],
        input=f"{position_command}\ngo depth {depth}\nquit\n",
        text=True, capture_output=True, check=True, cwd="/tmp", env=environment,
        timeout=120)
    move_match = re.search(r"^bestmove (\S+)$", completed.stdout, re.MULTILINE)
    info_match = re.search(r"^info depth (\d+) score cp (-?\d+) nodes (\d+)(?: pv(?: [a-i][0-9][a-i][0-9])*)?$",
                           completed.stdout, re.MULTILINE)
    if not move_match or not info_match:
        raise SystemExit(f"unexpected engine response:\n{completed.stdout}")
    actual_move = move_match.group(1)
    actual_score = int(info_match.group(2))
    report = {
        "trace": label,
        "depth": depth,
        "score_kind": "no-search sentinel" if trace.get("completed_depth") == 0
                      and trace["root_score"] == -32767 else "searched",
        "original": {"bestmove": expected_move, "score": trace["root_score"],
                     "search_calls": trace.get("search_call_count") or
                                     len(trace.get("search_entries", [])) or
                                     trace["search_calls_low_word"],
                     "search_calls_low_word": trace["search_calls_low_word"]},
        "portable": {"bestmove": actual_move, "score": actual_score,
                     "nodes": int(info_match.group(3))},
        "move_matches": actual_move == expected_move,
        "score_matches": actual_score == trace["root_score"],
    }
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path, help="one trace, a directory, or compact .jsonl fixtures")
    parser.add_argument("engine", type=Path)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--engine-env", action="append", default=[], metavar="NAME=VALUE")
    args = parser.parse_args()
    engine_sha256 = hashlib.sha256(args.engine.read_bytes()).hexdigest()
    overrides = dict(item.split("=", 1) for item in args.engine_env)
    compact = args.trace.suffix == ".jsonl"
    if compact:
        paths = [(f"{args.trace}:{i}", json.loads(line))
                 for i, line in enumerate(args.trace.read_text().splitlines(), 1) if line.strip()]
    else:
        paths = sorted(args.trace.glob("*.json")) if args.trace.is_dir() else [args.trace]
    def run(path):
        try:
            if compact:
                return compare_trace(path[1], path[0], args.engine, overrides)
            return compare(path, args.engine, overrides)
        except NotSearchOracle as error:
            return {"trace": str(path), "skipped": str(error)}
        except Exception as error:
            return {"trace": str(path), "error": str(error)}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        reports = list(pool.map(run, paths))
    if hashlib.sha256(args.engine.read_bytes()).hexdigest() != engine_sha256:
        raise SystemExit("Engine binary changed during comparison; discard and rerun this batch")
    skipped = [r for r in reports if "skipped" in r]
    reports = [r for r in reports if "skipped" not in r]
    matches = sum(r.get("move_matches", False) and r.get("score_matches", False)
                  for r in reports)
    output = {"engine_sha256": engine_sha256,
              "fixture_sha256": hashlib.sha256(args.trace.read_bytes()).hexdigest() if compact else None,
              "total": len(reports), "matches": matches, "skipped": skipped,
              "failures": [r for r in reports if not
                           (r.get("move_matches") and r.get("score_matches"))]}
    if args.report:
        args.report.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(output if args.trace.is_dir() or compact else (reports or skipped)[0], indent=2))
    return int(not reports or matches != len(reports))


if __name__ == "__main__":
    raise SystemExit(main())
