#!/usr/bin/env python3
"""Porte d'invariance du recu au nombre de workers.

Le parallelisme decoupe l'arbre en sous-arbres independants a une profondeur de
recolte. La propriete half-open garantit qu'un support n'est possede que par une
seule cellule, donc par une seule tache; les compteurs cumulatifs s'additionnent
et les compteurs de pic prennent le maximum. Aucun n'est donc sensible a
l'ordonnancement.

Cette porte l'exige litteralement : le recu doit etre IDENTIQUE ligne a ligne
entre un run sequentiel et un run a plusieurs workers, la seule ligne exclue
etant celle qui annonce le nombre de workers. C'est la propriete qu'un portage
device devra conserver.

Codes : 0 identique, 1 divergence, 2 refus avant calcul.
"""

import argparse
import subprocess
import sys
from pathlib import Path


SEMANTIC_PREFIXES = (
    "supports_q",
    "supports_total",
    "supports_per_point",
    "accepted_closed_rank",
    "K1Receipt",
    "IDENTITES",
    "ID S=",
    "ACCORD",
)


def receipt(binary: str, args: list[str], threads: int,
            semantic_only: bool = False) -> list[str] | None:
    cmd = [binary] + args + [f"--threads={threads}"]
    done = subprocess.run(cmd, capture_output=True, text=True)
    if done.returncode != 0:
        print(f"REFUS : {threads} workers rendent {done.returncode}", file=sys.stderr)
        print(done.stderr, file=sys.stderr)
        return None
    # La ligne `cloud=` porte le nombre de workers : c'est la seule difference
    # legitime entre deux runs.
    lines = [l for l in done.stdout.splitlines() if not l.startswith("cloud=")]
    if semantic_only:
        lines = [l for l in lines if l.startswith(SEMANTIC_PREFIXES)]
    return lines


def main() -> int:
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--points", type=int, required=True)
    parser.add_argument("--smax", type=int, default=11)
    parser.add_argument("--family", required=True)
    parser.add_argument("--seed", type=int, default=11)
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 2, 4])
    # Une chaine unique : argparse prendrait un `--drapeau` isole pour une option.
    parser.add_argument("--extra", default="")
    # Les compteurs du lift differe dependent des frontieres de lots, donc de
    # l'ordre d'arrivee : seule sa SEMANTIQUE est invariante. Ce mode compare
    # alors les lignes de sortie et non le ledger de travail.
    parser.add_argument("--semantic-only", action="store_true")
    args = parser.parse_args()

    if not Path(args.binary).is_file():
        print(f"REFUS : binaire absent {args.binary}", file=sys.stderr)
        return 2

    base = [
        f"--points={args.points}",
        f"--smax={args.smax}",
        f"--family={args.family}",
        f"--seed={args.seed}",
    ] + [a for a in args.extra.split() if a]

    reference = None
    for threads in args.threads:
        current = receipt(args.binary, base, threads, args.semantic_only)
        if current is None:
            return 2
        if reference is None:
            reference = current
            continue
        if current != reference:
            print("DIVERGENCE entre nombres de workers", file=sys.stderr)
            for a, b in zip(reference, current):
                if a != b:
                    print(f"  sequentiel : {a}", file=sys.stderr)
                    print(f"  {threads} workers : {b}", file=sys.stderr)
            return 1

    kind = "SEMANTIQUE IDENTIQUE" if args.semantic_only else "RECU IDENTIQUE"
    print(f"{kind} pour les workers {args.threads}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
