#!/usr/bin/env python3
"""Lecteur du recu pipeline_g4 : empreintes, verdicts des sessions G4 et medianes appariees.

    python3 -B morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/check.py

Code 0 si les empreintes concordent, si chaque session a un arret cible certifie et si la qualification
`claudeab7` a le verdict `conforme` ; 1 sinon. Python 3.10 nu, aucun assert.
"""
import hashlib
import io
import json
from pathlib import Path
import statistics
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SESSIONS = ('claudeprof1', 'claudeab1', 'claudeab4', 'claudeab7')
PREEMPTED = ('claudeab5', 'claudeab6')  # preemption Spot au demarrage, arret cible certifie, aucune commande


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def member(archive, suffix):
    with tarfile.open(archive) as t:
        for m in t.getmembers():
            if m.name.endswith(suffix):
                return json.load(io.TextIOWrapper(t.extractfile(m), encoding='utf-8'))
    return None


def table(report):
    rows = {}
    for t in report['timings']:
        rows.setdefault((t['frame'], t['workers'], t['variant']), []).append(t['summary'])
    out = []
    for key in sorted(rows):
        v = rows[key]
        med = lambda f: statistics.median(f(x) for x in v)  # noqa: E731
        out.append('%s W%s %-4s n=%d  FULL %7.1f  domaine %6.1f  forets %6.1f  CPU %5.2f s' % (
            key[0], key[1], key[2], len(v), med(lambda x: x['wall_ns'] / 1e6), med(lambda x: x['domain_ns'] / 1e6),
            med(lambda x: x['forest_ns'] / 1e6), med(lambda x: x['cpu_seconds'])))
    return out


def main():
    problems = []
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        path = HERE / name
        if not path.is_file() or sha256(path) != digest:
            problems.append('empreinte : ' + name)
    for name in SESSIONS:
        base = HERE / 'sessions' / name
        receipt = json.loads((base / 'receipt.json').read_text())
        after = receipt.get('observed_after') or {}
        if receipt.get('targeted_shutdown_certified') is not True or after.get('status') != 'TERMINATED':
            problems.append('arret cible non certifie : ' + name)
        report = member(base / 'results.tar.gz', 'ab_report.json') or member(base / 'results.tar.gz',
                                                                              'profile_report.json')
        print('==', name, 'schema', report.get('schema'), 'verdict', report.get('verdict'))
        if report.get('timings') and 'summary' in report['timings'][0]:
            for row in table(report):
                print('  ' + row)
        if name == 'claudeab7':
            if report.get('verdict') != 'conforme':
                problems.append('qualification non conforme : %s' % report.get('refusals'))
            print('  portes', report.get('tests'), 'tsan', report.get('tsan'))
            for module, m in report.get('mutants', {}).items():
                print('  mutants', module, len(m.get('killed', [])), 'tues sur', len(m.get('ids', [])))
    for name in PREEMPTED:
        receipt = json.loads((HERE / 'sessions' / name / 'receipt.json').read_text())
        if receipt.get('targeted_shutdown_certified') is not True or receipt.get('status') != 'failed_before_start':
            problems.append('session preemptee mal close : ' + name)
        print('==', name, receipt.get('status'), 'arret cible certifie', receipt.get('targeted_shutdown_certified'))
    print('pipeline_g4_verdict', 'conforme' if not problems else 'refus', problems)
    return 0 if not problems else 1


if __name__ == '__main__':
    raise SystemExit(main())
