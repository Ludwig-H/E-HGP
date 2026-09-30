"""Sonde légère et indépendante : refus de campagnes incomplètes et couverture du reçu C.

Usage : python3 evidence_bench_completeness.py <v10 snapshot> <v10 checkout>
Aucune scène générée, aucune VM, aucun binaire produit exécuté. Les seules écritures
temporaires sont dans un TemporaryDirectory propre à la sonde.
"""
import csv
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    snapshot, checkout = map(Path, sys.argv[1:])
    prereg_path = snapshot / 'bench/synthetic/prereg/PREREG_V10_COVER_C_20260929.json'
    prereg = json.loads(prereg_path.read_text())
    sys.path.insert(0, str(snapshot / 'bench/synthetic'))
    import run_test

    specs, digest = run_test.plan_manifest(prereg)
    names = {m['name'] for m in prereg['methods']}
    expected = {(run_test.unit_name(s), name) for s in specs for name in names}
    receipt = checkout / 'receipts/test_cover_C_20260929'
    with gzip.open(receipt / 'results.csv.gz', 'rt') as f:
        rows = list(csv.DictReader(f))
    got = [(r['unit'], r['method']) for r in rows]
    print(json.dumps(dict(receipt='lot_C', expected_pairs=len(expected), rows=len(rows),
                          missing=len(expected - set(got)), extra=len(set(got) - expected),
                          duplicates=len(got) - len(set(got)), manifest_sha256=digest,
                          refused=sum(int(r['refused']) for r in rows)), sort_keys=True))

    # Le plan et ses 960 scènes restent intacts ; seules les tailles Monte-Carlo
    # sont réduites afin de tester le garde de complétude sans calcul long.
    prereg['decision']['permutations'] = 8
    prereg['decision']['bootstrap'] = 8
    with tempfile.TemporaryDirectory(prefix='mhgp10-audit-evidence-') as temp:
        temp = Path(temp)
        preg = temp / 'PREREG.json'
        preg.write_text(json.dumps(prereg))
        psha = hashlib.sha256(preg.read_bytes()).hexdigest()
        run = temp / 'partial'
        run.mkdir()
        unit_rows = [r for r in rows if r['unit'] == rows[0]['unit']]
        with (run / 'results.csv').open('w', newline='') as f:
            w = csv.DictWriter(f, fieldnames=unit_rows[0].keys())
            w.writeheader()
            w.writerows(unit_rows)
        (run / 'run.json').write_text(json.dumps(dict(prereg_sha256=psha, plan_sha256=digest,
                                                    scenes=len(specs), computed=1, complete=False)))
        r = subprocess.run([sys.executable, str(snapshot / 'bench/synthetic/decide.py'),
                            '--prereg', str(preg), '--run', str(run)], capture_output=True, text=True)
        decision = json.loads((run / 'DECISION.json').read_text()) if (run / 'DECISION.json').exists() else None
        print(json.dumps(dict(probe='partial_decide', expected_scenes=len(specs), present_scenes=1,
                              declared_complete=False, exit_code=r.returncode,
                              written_decision_scenes=None if decision is None else decision['scenes']),
                         sort_keys=True))

        # merge_sessions sait refuser les doublons, mais ne compare pas les noms
        # présents au manifeste : remplacer un nom par un nom hors plan suffit.
        session = temp / 'unknown'
        session.mkdir()
        (session / 'run.json').write_text(json.dumps(dict(prereg_sha256=psha, plan_sha256=digest,
                                                        scenes=len(specs))))
        with (session / 'results.csv').open('w', newline='') as f:
            w = csv.DictWriter(f, fieldnames=unit_rows[0].keys())
            w.writeheader()
            w.writerows(dict(r, unit='outside_the_preregistered_plan') for r in unit_rows)
        merged = temp / 'merged'
        r = subprocess.run([sys.executable, str(snapshot / 'bench/g4/merge_sessions.py'),
                            '--prereg', str(preg), '--out', str(merged), str(session)],
                           capture_output=True, text=True)
        print(json.dumps(dict(probe='merge_unknown_unit', exit_code=r.returncode,
                              accepted_unit='outside_the_preregistered_plan'), sort_keys=True))


if __name__ == '__main__':
    main()
