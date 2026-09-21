#!/usr/bin/env python3
"""Indexed q3/q4 LiDAR matrices; explicit modes, no default large campaign.

Uses the tranche32 qualification collector. Tranche31 input metadata/hash
validation is reused; no earlier capture or algorithm qualification is inherited.
"""
import argparse
import json
from pathlib import Path

import run_q34_indexed_checks as checks


def make_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--build", type=Path, required=True)
    run.add_argument("--output", type=Path, required=True)
    run.add_argument("--qualification", choices=("preflight", "candidate"), default="preflight")
    run.add_argument("--scans", type=int, nargs="+", default=[0])
    run.add_argument("--sizes", type=int, nargs="+", required=True)
    run.add_argument("--kmax", type=int, nargs="+", default=[5, 10])
    run.add_argument("--s", type=int, nargs="+", default=[8, 10, 12])
    run.add_argument("--workers", type=int, nargs="+", default=[1, 4])
    run.add_argument("--backends", type=int, nargs="+", default=[28])
    run.add_argument("--witness-modes", nargs="+", choices=checks.MODES, default=["rectangle-pair"])
    run.add_argument("--q3-census-mode", choices=("scalar", "boxes"), default="boxes")
    run.add_argument("--payload", choices=("digest", "records"), default="digest")
    checks.add_read_parsers(sub)
    return parser


def configuration(args):
    matrix = {name: list(getattr(args, name)) for name in
              ("scans", "sizes", "kmax", "s", "workers", "backends", "witness_modes")}
    matrix["payload"] = args.payload
    matrix["q3_census_mode"] = args.q3_census_mode
    checks.validate_matrix(matrix)
    return dict(campaign="lidar", qualification=args.qualification, matrix=matrix)


def main():
    args = make_parser().parse_args()
    if args.operation == "run":
        checks.run(args, configuration(args))
    else:
        result = checks.read(args.path, args.check_live) if args.operation == "read" else checks.selftest(args.path)
        if getattr(args, "compact", False):
            result = {key: value for key, value in result.items() if key != "growth"}
        print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
