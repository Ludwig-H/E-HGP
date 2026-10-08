#!/usr/bin/env python3
"""Contrôle documentaire des octets Git ; aucun moteur ni modèle de CUDA."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, default=HERE.parents[3])
    args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    src = {}
    for name, expected in cap['sources'].items():
        need(not name.startswith('/') and '..' not in Path(name).parts, 'chemin relatif')
        data = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['commit'] + ':' + name])
        need(hashlib.sha256(data).hexdigest() == expected, 'source différente : ' + name)
        src[name] = data.decode('utf-8')
    for anchor in cap['anchors']:
        need(anchor['text'] in src[anchor['source']], 'ancrage absent : ' + anchor['name'])
    probe = src['morsehgp3D_v12/bench/catalogue_probe.cpp']
    device = src['morsehgp3D_v12/src/catalogue/device_cuda.cu']
    need('restart_peak(' not in probe and 'restart_peak(' not in device, 'pic remis à zéro')
    slices = src['morsehgp3D_v12/src/catalogue/finish_slices.hpp']
    close = slices[slices.index('inline Outcome slices_close('):slices.index('// Fin d\'etage complete par tranches')]
    need(close.index('materialize_levels(') < close.index('const Stopwatch table_watch'), 'frontière chrono')
    need('stats.table_ns' in close and 'publish_ns' not in close and 'stats.take_ns' not in close, 'imputation changée')
    result = {'status': 'source_anchors_verified', 'commit': cap['commit'],
              'source_files': len(src), 'anchors': len(cap['anchors']),
              'native_executed': False, 'gpu_qualified': False,
              'findings': ['variable_metadata_outside_budget', 'sliced_device_host_work_missing_from_stage_detail'],
              'device_peak': 'cumulative_separate_budget_or_zero_if_shared'}
    expected = json.loads((HERE / 'results.json').read_text())
    need(result == expected, 'résultat documentaire différent')
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
