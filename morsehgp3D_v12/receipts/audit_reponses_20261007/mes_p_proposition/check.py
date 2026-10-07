#!/usr/bin/env python3
"""Proposition de cohorte MES-P : patch temporaire, cinq contre-temoins et porte officielle legere.

python3 -B -S check.py ; python3 -B -S -O check.py. Aucun HGP, build ou GPU.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
RECEIPTS = HERE.parents[1]
AUDIT = RECEIPTS/'audit_mes_p_corrections_20261007'
PREFIX = 'morsehgp3D_v12/microbancs/mes_p_petits/'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    dependencies = [HERE/'check.py', HERE/'sources.json', HERE/'proposition.patch',
                    HERE/'test_pilote_p.snapshot.py', AUDIT/'check.py',
                    AUDIT/'snapshot/analyse_p.py', AUDIT/'snapshot/pilote_p.py']
    hashes = {str(p.relative_to(RECEIPTS)): sha(p.read_bytes()) for p in dependencies}
    metadata = json.loads((HERE/'sources.json').read_text())
    original = {name: (AUDIT/'snapshot'/name).read_bytes() for name in ('analyse_p.py', 'pilote_p.py')}
    original['test_pilote_p.py'] = (HERE/'test_pilote_p.snapshot.py').read_bytes()
    for name, data in original.items():
        need(sha(data) == metadata['sources_sha256'][PREFIX+name], 'source differente : '+name)
        published = subprocess.check_output(['git', 'show', metadata['published_base_pin']+':'+PREFIX+name],
                                            cwd=HERE)
        need(published == data, 'source differente du pin publie : '+name)
    need(sha((HERE/'proposition.patch').read_bytes()) == metadata['patch_sha256'], 'patch different')
    python = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
    before = subprocess.run(python+[str(AUDIT/'check.py'), '--require-common'], capture_output=True,
                            text=True, timeout=15)
    need(before.returncode == 1, ('controle negatif', before.returncode, before.stderr))
    with tempfile.TemporaryDirectory(prefix='audit-proposition-mesp-') as tmp:
        folder = Path(tmp)/PREFIX
        folder.mkdir(parents=True)
        for name, data in original.items():
            (folder/name).write_bytes(data)
        subprocess.run(['git', 'apply', '--check', str(HERE/'proposition.patch')], cwd=tmp, check=True)
        subprocess.run(['git', 'apply', str(HERE/'proposition.patch')], cwd=tmp, check=True)
        need(sha((folder/'analyse_p.py').read_bytes()) == metadata['proposed_analyser_sha256'],
             'application incorrecte')
        after = subprocess.run(python+[str(AUDIT/'check.py'), '--source-dir', str(folder), '--require-common'],
                               capture_output=True, text=True, timeout=15)
        need(after.returncode == 0, ('contre-temoins', after.returncode, after.stderr))
        official = subprocess.run(python+[str(folder/'test_pilote_p.py')], capture_output=True, text=True, timeout=15)
        need(official.returncode == 0, ('porte officielle', official.returncode, official.stderr))
        official_result = json.loads(official.stdout)
        need(official_result == dict(porte='mes_p', cas=4, ecarts=0), official_result)
    before_result, after_result = json.loads(before.stdout), json.loads(after.stdout)
    need(not before_result['cst_0238_common_cohort_contract_pass'], 'controle negatif non causal')
    need(after_result['cst_0238_common_cohort_contract_pass'], 'contrat non acquis par la proposition')
    need(hashes == {str(p.relative_to(RECEIPTS)): sha(p.read_bytes()) for p in dependencies},
         'dependance modifiee pendant le rejeu')
    print(json.dumps(dict(sources=metadata, audit_check_sha256=sha((AUDIT/'check.py').read_bytes()),
                         dependencies_sha256=hashes, dependencies_unchanged=True,
                         before_code=before.returncode, before_stdout_sha256=sha(before.stdout.encode()),
                         proposed_code=after.returncode, proposed_checks=after_result,
                         official_gate=official_result, official_gate_code=official.returncode,
                         scope='patch temporaire ; aucun temps HGP ni livraison produit'),
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
