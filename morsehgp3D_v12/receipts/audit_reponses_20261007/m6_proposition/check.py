#!/usr/bin/env python3
"""Applique la proposition dans /tmp, rejoue les contre-temoins et la porte M6 officielle, sans GPU.

python3 -B -S check.py ; python3 -B -S -O check.py. Sortie JSON deterministe.
Les sources produit sont lues au pin ; aucun fichier du produit n'est modifie.
"""
import contextlib
import copy
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
MICRO = ROOT / 'morsehgp3D_v12/microbancs/mes_m6_session'
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


def judged(factory, driver, folder):
    with patch.object(factory, 'charger', side_effect=lambda: load(driver, 'audit_m6_driver')):
        code, answer = factory.rejuger(folder)
    return dict(code=code, verdict=answer['verdict'], prises=answer['prises'], lignes=answer['lignes'],
                refus=answer['refus'], limites_declarees=len(answer['non_rejouable']))


def cases(factory, original, proposed, temporary):
    observations = []
    code, _, base, folder, _ = factory.passage(temporary, 'base')
    need(code == 0 and len(base['runs']) == 9, 'base complete refusee')
    payloads = {p.name: p.read_bytes() for p in folder.glob('m6_*.jsonl')}
    specifications = [
        ('temoin_complet', lambda r: None, False, 0),
        ('indice_false', lambda r: r['runs'][0].update(process=False), False, 0),
        ('indice_float', lambda r: r['runs'][0].update(process=0.0), False, 0),
        ('indices_float_sans_fichiers', lambda r: (
            [row.update(process=float(row['process'])) for row in r['runs']],
            r.update(summary_median_us={m: {} for m in ('spin', 'yield', 'blocking')})), True, 0),
        ('schema_inconnu_refus_explicite', lambda r: r.update(
            schema='ehgp.v12.mes_m6.v999', refusals=['binaire modifie pendant les prises']), False, 0),
        ('code_compilation_false', lambda r: r['compile'].update(code=False), False, 0),
        ('code_prise_false', lambda r: r['runs'][0].update(code=False), False, 0),
        ('code_compilation_float', lambda r: r['compile'].update(code=0.0), False, 0),
        ('code_prise_float', lambda r: r['runs'][0].update(code=0.0), False, 0),
        ('indice_negatif', lambda r: r['runs'][0].update(process=-1), False, 3),
        ('indice_hors_plage', lambda r: r['runs'][0].update(process=3), False, 3),
        ('schema_absent', lambda r: r.pop('schema'), False, 0),
        ('schema_null', lambda r: r.update(schema=None), False, 0),
        ('schema_liste', lambda r: r.update(schema=[]), False, 0),
        ('v1_refus_explicite', lambda r: r.update(schema='ehgp.v12.mes_m6.v1',
            refusals=['binaire modifie pendant les prises']), False, 0),
    ]
    for name, mutate, remove, old_code in specifications:
        report = copy.deepcopy(base)
        mutate(report)
        for filename, content in payloads.items():
            path = folder / filename
            path.write_bytes(content)
            if remove:
                path.unlink()
        (folder/'m6_report.json').write_text(json.dumps(report))
        before = judged(factory, original, folder)
        after = judged(factory, proposed, folder)
        need(before['code'] == old_code, (name, before))
        need(after['code'] == (0 if name == 'temoin_complet' else 3), (name, after))
        # Refus de fichier absent : le chemin temporaire ne fait pas partie de la preuve.
        for result in (before, after):
            result['refus'] = [s.replace(str(folder), '<dossier>') for s in result['refus']]
        observations.append(dict(cas=name, before=before, proposed=after))
    return observations


def official_gate(factory, proposed):
    def charger():
        module = load(proposed, 'audit_m6_proposed')
        module.SOURCE = MICRO / 'mes_m6_session_cost.cu'
        return module
    stdout, stderr = io.StringIO(), io.StringIO()
    with patch.object(factory, 'charger', side_effect=charger), contextlib.redirect_stdout(stdout), \
            contextlib.redirect_stderr(stderr):
        code = factory.main(['test_juge_m6.py'])
    rows = [json.loads(line) for line in stdout.getvalue().splitlines()]
    need(code == 0 and rows[-1]['ecarts'] == [] and rows[-1]['cas'] == 54, (code, rows[-1], stderr.getvalue()))
    historical = next(r for r in rows if r.get('cas') == 'rejuge_session_g4_a')
    need(historical['prises'] == 9 and historical['lignes'] == 585 and historical['non_rejouable'] == 3,
         historical)
    return dict(code=code, cas=rows[-1]['cas'], ecarts=rows[-1]['ecarts'], historique_v1=historical)


def main():
    meta = json.loads((HERE/'sources.json').read_text())
    source = subprocess.check_output(['git', 'show', meta['base_pin']+':'+RELATIVE], cwd=ROOT)
    need(sha(source) == meta['driver_sha256'], 'base differente')
    delta = (HERE/'proposition.patch').read_bytes()
    need(sha(delta) == meta['patch_sha256'], 'patch different')
    dependencies = [MICRO/'tests/test_juge_m6.py', MICRO/'mes_m6_session_cost.cu',
                    MICRO/'run_m6.py']
    dependencies += sorted(factory_path for factory_path in
                           (ROOT/'morsehgp3D_v12/receipts/g4_t0a_20261007/resultats/cmd/002_m6').rglob('*')
                           if factory_path.is_file())
    hashes = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in dependencies}
    factory = load(MICRO/'tests/test_juge_m6.py', 'audit_m6_official')
    with tempfile.TemporaryDirectory(prefix='audit-proposition-m6-') as tmp:
        temporary = Path(tmp)
        original = temporary/'original.py'
        original.write_bytes(source)
        proposed = temporary/RELATIVE
        proposed.parent.mkdir(parents=True)
        proposed.write_bytes(source)
        subprocess.run(['git', 'apply', '--check', str(HERE/'proposition.patch')], cwd=temporary, check=True)
        subprocess.run(['git', 'apply', str(HERE/'proposition.patch')], cwd=temporary, check=True)
        need(sha(proposed.read_bytes()) == meta['proposed_driver_sha256'], 'application incorrecte')
        # Les appels natifs deviennent impossibles pendant les fabriques et la porte officielle.
        with patch.object(subprocess, 'run', side_effect=RuntimeError('commande externe interdite')):
            observations = cases(factory, original, proposed, temporary)
            official = official_gate(factory, proposed)
    need(hashes == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in dependencies}, 'dependance modifiee')
    print(json.dumps(dict(proposition=meta, sources_sha256=hashes, observations=observations,
                         official_gate=official, scope='JSON synthetiques et relecture historique, aucun GPU'),
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
