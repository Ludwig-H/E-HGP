#!/usr/bin/env python3
"""Contre-audit du correctif M6 null : sources epinglees et JSON seulement."""
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
RECEIPTS = ROOT / 'morsehgp3D_v12/receipts'
PRIOR = HERE.parent / 'm6_proposition'
INTEGRATION = HERE.parent / 'm6_integration'
RELATIVE = 'morsehgp3D_v12/microbancs/mes_m6_session/run_m6.py'


def need(condition, detail):
    if not condition:
        raise RuntimeError(detail)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    meta = json.loads((HERE / 'sources.json').read_text())
    old_meta = json.loads((PRIOR / 'sources.json').read_text())
    integration_meta = json.loads((INTEGRATION / 'sources.json').read_text())
    dependencies = [HERE / 'check.py', HERE / 'sources.json', PRIOR / 'check.py', PRIOR / 'sources.json',
                    INTEGRATION / 'sources.json',
                    RECEIPTS / 'audit_session_t1_20261007/m6/result.json']
    historical = RECEIPTS / 'g4_t0a_20261007/resultats/cmd/002_m6/files/m6'
    dependencies += sorted(p for p in historical.rglob('*') if p.is_file())
    if (HERE / 'capture.patch').exists():
        dependencies.append(HERE / 'capture.patch')
    before = {str(p.relative_to(RECEIPTS)): sha(p.read_bytes()) for p in dependencies}
    factory_previous = load(PRIOR / 'check.py', 'm6_prior_witnesses')
    with tempfile.TemporaryDirectory(prefix='audit-m6-null-') as tmp:
        root = Path(tmp)
        current_root = root / 'current'
        pin = meta.get('pin', meta['base_pin'])
        for path in meta['sources_sha256']:
            data = subprocess.check_output(['git', 'show', pin + ':' + path], cwd=ROOT)
            dest = current_root / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
        if not meta.get('pin'):
            delta = (HERE / 'capture.patch').read_bytes()
            need(sha(delta) == meta['capture_patch_sha256'], 'delta de capture modifie')
            subprocess.run(['git', 'apply', str(HERE / 'capture.patch')], cwd=current_root, check=True)
        actual = {p: sha((current_root / p).read_bytes()) for p in meta['sources_sha256']}
        need(actual == meta['sources_sha256'], 'sources hors capture')
        current = current_root / RELATIVE
        original = root / 'before_all_guards.py'
        original.write_bytes(subprocess.check_output(
            ['git', 'show', old_meta['base_pin'] + ':' + RELATIVE], cwd=ROOT))
        need(sha(original.read_bytes()) == old_meta['driver_sha256'], 'ancien juge incorrect')
        before_null = root / 'before_null.py'
        before_null.write_bytes(subprocess.check_output(
            ['git', 'show', integration_meta['pin'] + ':' + RELATIVE], cwd=ROOT))
        need(sha(before_null.read_bytes()) == integration_meta['sources_sha256'][RELATIVE], 'juge e501 incorrect')
        need(sha(current.read_bytes()) == integration_meta['patched_driver_sha256'],
             'le correctif ne correspond plus au garde null contre-juge')
        factory = load(current.parent / 'tests/test_juge_m6.py', 'm6_null_official_gate')
        factory.RECUS = RECEIPTS
        factory.RECU_G4 = historical
        factory.AUDITEUR = RECEIPTS / 'audit_session_t1_20261007/m6/result.json'
        # Les fabriques officielles remplacent leurs seules frontieres externes.
        # Tout appel natif accidentel via subprocess.run est interdit durant les jugements.
        with patch.object(subprocess, 'run', side_effect=RuntimeError('commande externe interdite')):
            observations = factory_previous.cases(factory, original, current, root)
            for observation in observations:
                observation['integrated'] = observation.pop('proposed')
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = factory.main(['test_juge_m6.py'])
            rows = [json.loads(line) for line in out.getvalue().splitlines()]
            total = rows[-1]
            need(code == 0 and total['cas'] == meta['expected_gate_cases'] and not total['ecarts'],
                 (code, total, err.getvalue()))
            history = next(row for row in rows if row.get('cas') == 'rejuge_session_g4_a')
            need(history['prises'] == 9 and history['lignes'] == 585 and history['non_rejouable'] == 3, history)
            folder = root / 'only_null'
            folder.mkdir()
            (folder / 'm6_report.json').write_text('null\n')
            null = {name: factory_previous.judged(factory, driver, folder)
                    for name, driver in [('published_e501', before_null), ('integrated', current)]}
            need(null['published_e501']['code'] == 0 and null['integrated']['code'] == 3, null)
            need(null['integrated']['refus'] == ['rapport hors schema'], null)
        need(actual == {p: sha((current_root / p).read_bytes()) for p in meta['sources_sha256']},
             'copie source modifiee pendant les jugements')
    after = {str(p.relative_to(RECEIPTS)): sha(p.read_bytes()) for p in dependencies}
    need(before == after, 'dependance modifiee pendant les jugements')
    result = dict(sources=meta, dependencies_sha256=before, sources_before_after_equal=True,
                  observations=observations, rapport_null=null,
                  official_gate=dict(code=code, cas=total['cas'], ecarts=total['ecarts'], historical_v1=history),
                  native_execution=False, mutant_matrix_executed=False,
                  scope='residu null corrige; quinze anciens temoins et compatibilite historique preserves')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
