#!/usr/bin/env python3
"""Contre-audit M6 e50114adf : quinze cas anterieurs, porte 68 cas, et null contre correctif minimal.

Sources publiees chargees au pin, copies temporaires ; aucune compilation ni commande GPU.
"""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PRIOR = HERE.parent/'m6_proposition'
RECEIPTS = ROOT/'morsehgp3D_v12/receipts'
RELATIVE = 'morsehgp3D_v12/microbancs/mes_m6_session/run_m6.py'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    dependencies = [HERE/'check.py', HERE/'sources.json', HERE/'refus_null.patch',
                    PRIOR/'check.py', PRIOR/'sources.json', PRIOR/'proposition.patch']
    hashes = {str(p.relative_to(RECEIPTS)): sha(p.read_bytes()) for p in dependencies}
    meta = json.loads((HERE/'sources.json').read_text())
    prior = json.loads((PRIOR/'sources.json').read_text())
    sources = {name: subprocess.check_output(['git','show',meta['pin']+':'+name],cwd=ROOT)
               for name in meta['sources_sha256']}
    need({p:sha(b) for p,b in sources.items()} == meta['sources_sha256'], 'sources hors pin')
    old_source = subprocess.check_output(['git','show',prior['base_pin']+':'+RELATIVE],cwd=ROOT)
    need(sha(old_source) == prior['driver_sha256'], 'ancien juge incorrect')
    previous = load(PRIOR/'check.py', 'previous_counter_witnesses')
    with tempfile.TemporaryDirectory(prefix='audit-m6-integration-') as tmp:
        root = Path(tmp)
        for path, data in sources.items():
            dest = root/'current'/path
            dest.parent.mkdir(parents=True,exist_ok=True)
            dest.write_bytes(data)
        original = root/'old.py'
        original.write_bytes(old_source)
        current = root/'current'/RELATIVE
        factory = load(current.parent/'tests/test_juge_m6.py', 'm6_current_official_gate')
        factory.RECUS = RECEIPTS
        factory.RECU_G4 = RECEIPTS/'g4_t0a_20261007/resultats/cmd/002_m6/files/m6'
        factory.AUDITEUR = RECEIPTS/'audit_session_t1_20261007/m6/result.json'
        proposed = root/'proposed'/RELATIVE
        proposed.parent.mkdir(parents=True)
        proposed.write_bytes(old_source)
        subprocess.run(['git','apply',str(PRIOR/'proposition.patch')],cwd=root/'proposed',check=True)
        need(sha(proposed.read_bytes()) == meta['proposition_previous_sha256'], 'proposition ancienne incorrecte')
        fixed = root/'fixed'/RELATIVE
        fixed.parent.mkdir(parents=True)
        fixed.write_bytes(current.read_bytes())
        subprocess.run(['git','apply','--check',str(HERE/'refus_null.patch')],cwd=root/'fixed',check=True)
        subprocess.run(['git','apply',str(HERE/'refus_null.patch')],cwd=root/'fixed',check=True)
        need(sha(fixed.read_bytes()) == meta['patched_driver_sha256'], 'patch null incorrect')
        with patch.object(subprocess,'run',side_effect=RuntimeError('commande externe interdite')):
            observations = previous.cases(factory,original,current,root)
            for observation in observations:
                observation['published_e501'] = observation.pop('proposed')
            # Le main officiel tourne sans changement ; seules ses frontieres externes sont ses doubles habituels.
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                gate_code = factory.main(['test_juge_m6.py'])
            rows = [json.loads(s) for s in out.getvalue().splitlines()]
            need(gate_code == 0 and rows[-1]['cas'] == 68 and not rows[-1]['ecarts'],
                 (gate_code,rows[-1],err.getvalue()))
            historical = next(r for r in rows if r.get('cas') == 'rejuge_session_g4_a')
            folder = root/'null_report'
            folder.mkdir()
            (folder/'m6_report.json').write_text('null\n')
            null = {name:previous.judged(factory,driver,folder) for name,driver in
                    [('before',original),('proposition_03ccabaf',proposed),('published_e501',current),
                     ('proposition_refus_null',fixed)]}
            need([null[x]['code'] for x in ('before','proposition_03ccabaf','published_e501',
                                           'proposition_refus_null')] == [0,3,0,3], null)
    need(hashes == {str(p.relative_to(RECEIPTS)):sha(p.read_bytes()) for p in dependencies}, 'dependance modifiee')
    print(json.dumps(dict(sources=meta,dependencies_sha256=hashes,observations=observations,
                         official_gate=dict(code=gate_code,cas=68,ecarts=[],historical_v1=historical),
                         rapport_null=null,native_execution=False,mutant_matrix_executed=False,
                         scope='anciens residus corriges; null accepte sans prise par e50114adf'),
                     ensure_ascii=False,sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
