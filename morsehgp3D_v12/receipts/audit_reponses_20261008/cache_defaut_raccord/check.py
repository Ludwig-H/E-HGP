#!/usr/bin/env python3
"""Liaison statique des deux commits ; aucune sonde ni allocation native."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import argparse


def need(ok, why):
    if not ok:
        raise ValueError(why)


def tokens(text):
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|\w+|[^\s]'
    return [t for t in re.findall(pattern, text) if not t.startswith(('//', '/*'))]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    a = ap.parse_args()
    c = json.loads(Path(__file__).with_name('capture.json').read_text())
    files = {}
    for name, versions in c['files'].items():
        files[name] = {}
        for label, expected in versions.items():
            body = subprocess.check_output(['git', '-C', str(a.repo), 'show',
                    c['pins'][label]+':morsehgp3D_v12/'+name])
            need(len(body)==expected['bytes'] and hashlib.sha256(body).hexdigest()==expected['sha256'], name)
            files[name][label] = body.decode()
    old, new = files['bench/full_probe.cpp']['measured'], files['bench/full_probe.cpp']['delivered']
    need(new.count('cache = u64{8} << 30')==1, 'défaut absent/multiple')
    normalized = new.replace('cache = u64{8} << 30', 'cache = 0')
    need(tokens(old)==tokens(normalized), 'autre changement exécutable de la sonde')
    changed = subprocess.check_output(['git', '-C', str(a.repo), 'diff', '--name-only',
        c['pins']['measured'], c['pins']['delivered'], '--', 'morsehgp3D_v12/src', 'morsehgp3D_v12/bench'], text=True)
    need(set(changed.splitlines())=={'morsehgp3D_v12/bench/full_probe.cpp',
                                    'morsehgp3D_v12/src/tower/pipeline_run.cpp'}, 'autre moteur modifié')
    for name in ('src/core/buffer.hpp','src/core/buffer.cpp','tests/tower/full_probe_check.py',
                 'microbancs/mes_full/pilote_full.py','microbancs/mes_apparie/pilote_apparie.py',
                 'microbancs/outils/lecteur_full.py'):
        need(files[name]['measured']==files[name]['delivered'], 'source commune modifiée')
    need('MemoryBudget budget(o.budget, o.cache);' in new and 'if (!o.device) return passes(' in new,
         'raccord CPU/GPU')
    x=c['numeric_policy_example'];room=x['limit']-x['used'];amount=x['admitted_bytes']
    need((amount<=room)==x['without_cache'] and
         (amount<=room and amount//8<=room-amount)==x['with_cache'], 'exemple admission')
    print('verified: default8GiB only executable probe delta; same cache and readers; CPU/GPU option; '
          'finite-budget policy example; no native test or timing')


if __name__=='__main__':
    main()
