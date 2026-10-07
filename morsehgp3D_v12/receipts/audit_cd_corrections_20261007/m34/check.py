#!/usr/bin/env python3
"""Contrelecture locale des corrections 320db4a12, au pin sources.json.

Aucune mesure de performance. Un seul binaire natif M4 normal est execute ;
les injections de journaux et de processus sont des scripts explicitement simules.
Pas d'assert : controles actifs aussi sous python -O. Sortie JSON deterministe.
"""
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SRC = ROOT / 'morsehgp3D_v12/microbancs/mes_m3_m4_tour'
BINARY = Path(sys.argv[1]).resolve()
MANIFEST = json.loads((HERE / 'sources.json').read_text())


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def verify_sources():
    require(subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT,
                                    text=True).strip() == MANIFEST['pin'], 'pin changed')
    for relative, expected in MANIFEST['files'].items():
        require(sha(ROOT / relative) == expected, 'source changed: ' + relative)


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def native_gate():
    flags = ['-S'] + (['-O'] if sys.flags.optimize else [])
    command = [sys.executable] + flags + [str(SRC / 'tests/test_m4_preuves.py'),
                                         '--binaire', str(BINARY)]
    proc = subprocess.run(command, text=True, capture_output=True, timeout=60)
    require(proc.returncode == 0, 'native gate failed: ' + proc.stdout + proc.stderr)
    rows = [json.loads(s) for s in proc.stdout.splitlines()]
    require(len(rows) == 10 and rows[-1]['cas'] == 9 and not rows[-1]['ecarts'],
            'nine native cases missing')
    rows[-1].pop('optimise')
    return rows


def fresh_processes(test, pilot):
    original = test.FAUX
    instrument = '''
if os.environ.get("AUDIT_PID_LOG"):
    with open(os.environ["AUDIT_PID_LOG"], "a") as audit_file:
        audit_file.write(json.dumps({"pid": os.getpid(), "name": nom, "args": sys.argv[1:]}) + "\\n")
'''
    test.FAUX = original.replace('nom = os.path.basename(sys.argv[0])',
                                 'nom = os.path.basename(sys.argv[0])' + instrument)
    try:
        with tempfile.TemporaryDirectory(prefix='audit-m34-processes-') as tmp:
            log = Path(tmp) / 'pids.jsonl'
            os.environ['AUDIT_PID_LOG'] = str(log)
            banc = test.Banc(pilot, tmp, {'rapport': 0.1})
            require(banc.etape('vider') == 0, 'simulated dump refused')
            require(banc.etape('resolution') == 0, 'first resolution refused')
            old = {str(p.relative_to(banc.args.sortie)): sha(p)
                   for p in Path(banc.args.sortie).rglob('*.jsonl')}
            banc.nouvelle_campagne()
            banc.scenario_courant({'rapport': 0.2})
            require(banc.etape('resolution') == 0, 'second resolution refused')
            require(all(sha(Path(banc.args.sortie) / name) == h for name, h in old.items()),
                    'old journals changed')
            launches = [json.loads(s) for s in log.read_text().splitlines()]
            launches = [r for r in launches if '--chrono-resolution' in r['args'] and
                        int(r['args'][r['args'].index('--chrono-resolution') + 1]) > 0]
            campaigns = banc.rapport['resolution']['audit_square_k4']['campagnes']
            require(len(launches) == 10 and len({r['pid'] for r in launches}) == 10,
                    'resolution did not use ten distinct processes')
            require(len(campaigns) == 2 and all(c['prises_valides'] == 5 for c in campaigns.values()),
                    'campaign/take loss')
            all_journals = [p['journal'] for c in campaigns.values() for p in c['prises']]
            require(len(set(all_journals)) == 10, 'journal name reused')
            duplicate_refused = False
            try:
                banc.etape('resolution')
            except SystemExit:
                duplicate_refused = True
            require(duplicate_refused, 'duplicate campaign identifier accepted')
            return {'scenario': 'simulated_resolution_processes', 'campaigns': 2,
                    'takes_per_campaign': [5, 5], 'distinct_pids': 10, 'distinct_journals': 10,
                    'previous_journals_unchanged': True, 'duplicate_campaign_refused': True,
                    'performance_claim': False}
    finally:
        test.FAUX = original
        os.environ.pop('AUDIT_PID_LOG', None)


def binary_changes_during_execution(test, pilot):
    original = test.FAUX
    # Only a simulated Python executable modifies its own temporary file.
    test.FAUX = original.replace('nom = os.path.basename(sys.argv[0])', '''
nom = os.path.basename(sys.argv[0])
with open(sys.argv[0], "a") as audit_file:
    audit_file.write("# changed during this invocation\\n")
''')
    try:
        with tempfile.TemporaryDirectory(prefix='audit-m34-hash-') as tmp:
            banc = test.Banc(pilot, tmp)
            code = banc.etape('vider')
            block = banc.rapport['vidages']['audit_square_k4']
            require(code == 3 and not block['conforme'], 'changed binary admitted')
            require(block['binaire']['construit'] and block['binaire']['inchange'] is False and
                    pilot.problemes_provenance(block['binaire']), 'missing provenance refusal')
            return {'scenario': 'simulated_binary_changed_during_execution',
                    'code': code, 'conforme': block['conforme'], 'provenance_refusal': True}
    finally:
        test.FAUX = original


def incomplete_mutant(test, pilot):
    # The normal executable is the real native M4. Only the mutant is a fake
    # process, correctly hash-bound to its recorded build, with an incomplete
    # response for a different frame/order range. It is not a geometric mutant.
    rows = [{'phase': 'entree', 'K': 1, 'trame': 'autre_trame', 'mutant_sans_contraction': True},
            {'phase': 'ordre', 'k': 1, 'identite': {'identiques': False}},
            {'phase': 'fin', 'code': 1}]
    require(pilot.valider_mutant_m4(1, rows) == (True, True, True),
            'residual acceptance no longer reproduced; review expected outcome')
    with tempfile.TemporaryDirectory(prefix='audit-m34-incomplete-') as tmp:
        banc = test.Banc(pilot, tmp, reels=BINARY.parent)
        banc.nouvelle_campagne(processus=1)
        target = banc.construction / 'mhgp12_mes_m4_mutant_sans_contraction'
        output = '\n'.join(json.dumps(r, sort_keys=True) for r in rows) + '\n'
        target.write_text('#!/usr/bin/env python3\nimport sys\nsys.stdout.write(' + repr(output) + ')\nsys.exit(1)\n')
        banc.rapport['construction'] = pilot.enregistrer_binaires(banc.args, {'code': 0, 'etapes': []})
        require(banc.etape('vider') == 0, 'simulated fixture inventory refused')
        code = banc.etape('m4')
        normal = banc.rapport['mes_m4']['audit_square_k4']
        mutant = banc.rapport['mes_m4']['mutant_sans_contraction_audit_square_k4']
        require(code == 0 and normal['conforme'] and mutant['tue'] and mutant['sortie_complete'],
                'residual whole-step acceptance no longer reproduced')
        require(mutant['binaire']['construit'] and mutant['binaire']['inchange'],
                'injection not hash-bound')
        return {'scenario': 'incomplete_wrong_frame_simulated_mutant', 'status': 'known_residual_gap',
                'finding': 'CST-0018', 'pilot_step_code': code, 'native_normal_conforme': True,
                'mutant_declared_killed': mutant['tue'], 'mutant_declared_complete': mutant['sortie_complete'],
                'expected_frame': 'audit_square', 'expected_orders': [1, 2, 3, 4],
                'injected_rows': rows, 'mutant_binary_hash_bound': True,
                'normal_births_checked': sum(r.get('lem_t6', {}).get('naissances_jugees', 0)
                    for r in normal['processus'][0]['lignes'] if r.get('phase') == 'ordre')}


def main():
    verify_sources()
    before = sha(BINARY)
    require(before == json.loads((HERE / 'build.json').read_text())['binary_sha256'], 'binary pin mismatch')
    developer = load(SRC / 'tests/test_pilote.py', 'developer_test_m34')
    pilot = developer.charger('pilot_under_audit')
    native = native_gate()
    with contextlib.redirect_stdout(io.StringIO()):
        injections = developer.partie_b(pilot)
    independent = [fresh_processes(developer, pilot), binary_changes_during_execution(developer, pilot),
                   incomplete_mutant(developer, pilot)]
    verify_sources()
    require(before == sha(BINARY), 'native binary changed')
    print(json.dumps({'pin': MANIFEST['pin'], 'sources_unchanged': True, 'native_binary_unchanged': True,
                      'native_binary_sha256': before, 'native_gate': native,
                      'developer_part_b_count': len(injections), 'developer_part_b': injections,
                      'independent': independent, 'status': 'checks_passed_with_CST0018_residual_reproduced'},
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
