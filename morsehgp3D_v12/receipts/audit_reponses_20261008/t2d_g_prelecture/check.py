#!/usr/bin/env python3
"""Pins et schémas JSON seuls ; aucune sonde ni campagne."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).parent
    cap = json.loads((here / 'capture.json').read_bytes())
    sources = {}
    for path, digest in cap['source_sha256'].items():
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['commit'] + ':' + path])
        need(hashlib.sha256(body).hexdigest() == digest, 'source différente')
        sources[path] = body
    for path, digest in cap['receipt_artifacts_sha256'].items():
        need(hashlib.sha256((here / path).read_bytes()).hexdigest() == digest, 'reçu différent')
    module = {'__name__': 'audit_import_only', '__file__': 'pilote_t2c.py'}
    body = sources['morsehgp3D_v12/microbancs/mes_t2c_g/pilote_t2c.py']
    exec(compile(body, 'pilote_t2c.py@' + cap['commit'], 'exec'), module)
    old = {key: 0 for key in module['DIAG_AVANT']}
    old['order_ns'] = [0] * 5
    new = {key: 0 for key in module['DIAG_APRES']}
    new.update(order_ns=[0] * 5, pass_ns=[0] * 5, table_ns=[0] * 4, join_ns=[0] * 4)
    validate = module['diagnostics_valides']
    validate(old, 5, 'avant')
    validate(new, 5, 'apres')
    refused = 0
    for data, name in [(old, 'apres'), (new, 'avant')]:
        try:
            validate(data, 5, name)
        except ValueError:
            refused += 1
    need(refused == 2, 'schémas indûment confondus')
    need(len(module['DIAG_AVANT']) == 7 and len(module['DIAG_APRES']) == 13, 'cardinalités')
    need(module['schema_bras']('avant') == module['schema_bras']('avant_bis') == 'avant', 'choix du bras')
    need(module['REGLE_T2C']['passes_min'] == 6 and module['REGLE_T2C']['processus_min'] == 10, 'règle T2c')
    print(json.dumps(dict(ok=True, pinned_sources=len(sources), accepted_schemas=2,
                         cross_schema_refusals=refused, native_executed=False), sort_keys=True))


if __name__ == '__main__':
    main()
