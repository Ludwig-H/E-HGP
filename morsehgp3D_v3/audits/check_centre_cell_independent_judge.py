#!/usr/bin/env python3
"""Porte du juge arithmetiquement independant de la source par cellules.

Le sujet et le juge ne partagent que le generateur de nuages et une ligne de
texte. Le sujet decide par determinants entiers `i128`; le juge resout le centre
par elimination de Gauss en rationnels multiprecision, teste la positivite par
les coordonnees barycentriques et fait son census par comparaisons de
rationnels. Deux fautes communes ne peuvent donc pas se compenser.

Ce pilote enchaine les deux binaires et propage le code du juge. Il refuse
d'annoncer un accord si le sujet n'a pas ferme sa liste d'identites.

Codes : 0 accord, 1 desaccord, 2 refus avant calcul, 3 plancher viole.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument("--subject-binary", required=True)
    parser.add_argument("--judge-binary", required=True)
    parser.add_argument("--points", type=int, required=True)
    parser.add_argument("--smax", type=int, required=True)
    parser.add_argument("--family", required=True)
    parser.add_argument("--seed", type=int, default=11)
    parser.add_argument("--min-supports", type=int, default=1)
    parser.add_argument("--inject", default="")
    parser.add_argument("--k1", action="store_true")
    args = parser.parse_args()

    for binary in (args.subject_binary, args.judge_binary):
        if not Path(binary).is_file():
            print(f"REFUS : binaire absent {binary}", file=sys.stderr)
            return 2

    subject_cmd = [
        args.subject_binary,
        f"--points={args.points}",
        f"--smax={args.smax}",
        f"--family={args.family}",
        f"--seed={args.seed}",
        "--emit-identities",
    ]
    if args.k1:
        subject_cmd.append("--k1")
    if args.inject:
        subject_cmd.append(f"--inject={args.inject}")

    with tempfile.NamedTemporaryFile("w+", suffix=".txt", delete=False) as handle:
        path = handle.name
    try:
        completed = subprocess.run(subject_cmd, capture_output=True, text=True)
        if completed.returncode != 0:
            print(f"REFUS : le sujet rend {completed.returncode}", file=sys.stderr)
            print(completed.stderr, file=sys.stderr)
            return 2
        Path(path).write_text(completed.stdout, encoding="utf-8")

        judge_cmd = [
            args.judge_binary,
            f"--points={args.points}",
            f"--smax={args.smax}",
            f"--family={args.family}",
            f"--seed={args.seed}",
        ]
        if args.k1:
            judge_cmd.append(f"--k1-subject={path}")
        else:
            judge_cmd.append(f"--min-supports={args.min_supports}")
            judge_cmd.append(f"--subject={path}")
        verdict = subprocess.run(judge_cmd, capture_output=True, text=True)
        sys.stdout.write(verdict.stdout)
        sys.stderr.write(verdict.stderr)
        return verdict.returncode
    finally:
        Path(path).unlink(missing_ok=True)


if __name__ == "__main__":
    raise SystemExit(main())
