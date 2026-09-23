#!/usr/bin/env python3
"""Auditeur C, lentille 7 : mesures, recus et ecart au contrat (v9).

Relit les sorties BRUTES des recus G4 R1..R7b (vm/probe_N.stdout, GNU time
vm/probe_N.stderr, vm/probe_N.summary.json) -- jamais les README ni les
SUMMARY.json -- et imprime : table chronologique, ventilation par phase,
planchers CPU/48, ecart au contrat, W24->W48, tailles par site, temps systeme,
et le controle des SUMMARY.json. Optionnellement, avec --scaling DIR (le recu
lidar_scaling_local_20260923), les densites de sortie par site et les
exposants locaux. Lecture seule ; sans assert (valide sous python3 -O).

  python3 lentille7_mesures_g4.py --root <racine du depot> [--scaling DIR]
"""
import argparse
import runpy
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--scaling')
    ap.add_argument('--rows', default=str(HERE / 'rows.json'))
    a = ap.parse_args()
    sys.argv = ['table_g4.py', a.root, a.rows]
    runpy.run_path(str(HERE / 'table_g4.py'), run_name='__main__')
    sys.argv = ['analyse.py', a.rows]
    runpy.run_path(str(HERE / 'analyse.py'), run_name='__main__')
    if a.scaling:
        sys.argv = ['scaling_laws.py', a.scaling]
        runpy.run_path(str(HERE / 'scaling_laws.py'), run_name='__main__')
    return 0


if __name__ == '__main__':
    sys.exit(main())
