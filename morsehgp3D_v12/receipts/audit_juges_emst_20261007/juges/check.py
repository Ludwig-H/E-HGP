#!/usr/bin/env python3
"""Petits témoins causaux des vrais pilotes ; frontières externes simulées, aucun GPU."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
MICRO = ROOT/'morsehgp3D_v12/microbancs'
PIN = '1f7642e105aebd76632c58c63fdfd5b5c0824779'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def m2_tests():
    f = load(MICRO/'mes_m2_feuille/tests/test_juge_m2.py', 'm2_audit_factory')
    original = f.charge_banc
    rows = []
    for name, scenario, expected in [('baseline', {}, 'adopte'), ('sanitizer_empty', {}, 'refuse'),
                                    ('discarded_code_1', {'commandes': {'bench_discarded': 1}}, 'refuse'),
                                    ('contradictory_counts', {}, 'refuse')]:
        injected = []
        def charge(m, path, ident, forms, reps, warmup, nonce, sc, leaves=None):
            result = original(m, path, ident, forms, reps, warmup, nonce, sc, leaves)
            if name == 'sanitizer_empty' and leaves is not None:
                result['cases'][0].update(forms=[], leaves=0)
                injected.append('forms=[], leaves=0')
            if name == 'contradictory_counts':
                for row in result['cases'][0]['forms']:
                    row.update(mismatched_counts=1, overflow=True)
                injected.append('identity=true, mismatched_counts=1, overflow=true')
            return result
        with patch.object(f, 'charge_banc', side_effect=charge), patch.object(
                subprocess, 'run', side_effect=RuntimeError('exécution externe interdite')):
            code, report, _ = f.jouer(dict(scenario, nom=name), [])
        verdict = report['verdicts']['j3']['verdict']
        require(code == 0 and verdict == expected, (name, code, verdict, expected))
        rows.append({'case': name, 'main_code': code, 'verdict': verdict,
                     'refusals': len(report['refusals']), 'injected': len(injected)})
    return rows


def m5_tests():
    f = load(MICRO/'mes_m5_parcours/tests/test_juge_m5.py', 'm5_audit_factory')
    original_take, original_sim = f.prise_banc, f.simulation
    # Les neuf premiers cas reprennent les défaillances historiques et leurs contrôles positifs.
    scenarios = [(name, sc, opts, expected) for name, sc, opts, expected in f.INJECTIONS[:9]]
    scenarios += [(name, {}, [], 'adopte') for name in
                  ('gpu_missing_but_identical', 'gpu_wrong_digest_but_identical', 'host_no_ledgers',
                   'sanitizer_no_measurements')]
    rows = []
    for name, scenario, options, expected in scenarios:
        injected = []
        def take(m, paths, nonce, reps, warmup, sc, stage):
            result = original_take(m, paths, nonce, reps, warmup, sc, stage)
            # Compléter le contrôle positif par les champs du vrai producteur CUDA (lignes 504..511).
            for c, path in zip(result['cases'], paths):
                ident = m.dump_identity(Path(path))
                c.update(status=ident['status'], ledger={key: ident[key] for key in
                         ('nodes', 'leaves', 'filter_tests', 'max_depth')},
                         leaves=ident['n_leaves'], reference_leaves=ident['n_leaves'],
                         digest='1234', reference_digest='1234', missing=0, extra=0,
                         list_mismatch=0, meta_mismatch=0, first='')
                c['ledger']['max_leaf'] = ident['max_leaf_seen']
                if stage.startswith('gpu_ng00_k5_l24') and name == 'gpu_missing_but_identical':
                    c['missing'] = 1
                    injected.append({'stage': stage, 'identity': c['identity'], 'missing': c['missing']})
                if stage.startswith('gpu_ng00_k5_l24') and name == 'gpu_wrong_digest_but_identical':
                    c['digest'] = '5678'
                    injected.append({'stage': stage, 'identity': c['identity'],
                                     'digest': c['digest'], 'reference_digest': c['reference_digest']})
                if stage.startswith('sanitizer_') and name == 'sanitizer_no_measurements':
                    c.update(total_ms=[], resident_ms=[])
                    injected.append({'stage': stage, 'command_reps': reps, 'total_samples': 0,
                                     'resident_samples': 0})
            return result
        def simulation(m, oracle, sc, journal, bdir, work):
            base = original_sim(m, oracle, sc, journal, bdir, work)
            def run(session, stage, command, timeout, **kwargs):
                result = base(session, stage, command, timeout, **kwargs)
                if stage == 'identity_host' and name == 'host_no_ledgers':
                    args = list(map(str, command))
                    target = Path(args[args.index('--json')+1])
                    values = [json.loads(line) for line in target.read_text().splitlines()]
                    for row in values:
                        if row.get('phase') == 'identity':
                            row.pop('ledger')
                            row.pop('reference_ledger')
                            injected.append({'stage': stage, 'identity': row['identity'],
                                             'ledger_present': False, 'reference_ledger_present': False})
                    target.write_text('\n'.join(json.dumps(row) for row in values)+'\n')
                return result
            return run
        with patch.object(f, 'prise_banc', side_effect=take), patch.object(f, 'simulation', side_effect=simulation), \
                patch.object(subprocess, 'run', side_effect=RuntimeError('exécution externe interdite')):
            code, report, _ = f.jouer(dict(scenario, nom=name), options)
        require(code == 0 and report['verdict'] == expected,
                (name, code, report['verdict'], expected, report['refused'][:3]))
        if name.startswith(('gpu_', 'host_', 'sanitizer_')):
            require(injected, 'injection non exercée : '+name)
        rows.append({'case': name, 'main_code': code, 'verdict': report['verdict'],
                     'refusals': len(report['refused']), 'rejections': len(report['rejected']),
                     'retained_rounds': sum(len(v) for v in report['rounds'].values()),
                     'injected': len(injected), 'example': injected[:1]})
    return rows


def m4_tests():
    # Le pilote lance uniquement les scripts synthétiques jetables de cette fabrique.
    f = load(MICRO/'mes_m3_m4_tour/tests/test_pilote.py', 'm4_audit_factory')
    return f.partie_b_mutant_m4(f.charger('m4_audit_product'))


def m6_tests():
    f = load(MICRO/'mes_m6_session/tests/test_juge_m6.py', 'm6_audit_factory')
    rows = []
    with tempfile.TemporaryDirectory(prefix='ehgp_m6_judge_') as temporary:
        folder = Path(temporary)
        with patch.object(subprocess, 'run', side_effect=RuntimeError('exécution externe interdite')):
            rows.extend(f.cas_auditeur(folder))
            code, _, report, out, _ = f.passage(folder, 'baseline')
            require(code == 0 and report['verdict'] == 'mes_m6_ok', 'M6 base refusée')
            code, answer = f.rejuger(out)
            require(code == 0, 'M6 relecture de base refusée')
            rows.append({'case': 'baseline_rejudge', 'code': code, 'verdict': answer['verdict'],
                         'takes': answer['prises']})
            mutations = ['bad_take_hash', 'empty_provenance', 'isolation_contradiction', 'explicit_refusal']
            for name in mutations:
                altered = copy.deepcopy(report)
                if name == 'bad_take_hash':
                    altered['runs'][0]['sha256'] = '0'*64
                elif name == 'empty_provenance':
                    altered['binary_sha256'] = ''
                    altered['sources_sha256'] = {}
                elif name == 'isolation_contradiction':
                    altered['runs'][0]['isolation_before'].update(code=9, processes='synthetic-process', quiet=True)
                elif name == 'explicit_refusal':
                    altered['refusals'] = ['binaire modifie ou retire pendant les prises']
                (out/'m6_report.json').write_text(json.dumps(altered))
                code, answer = f.rejuger(out)
                require(code == (3 if name == 'bad_take_hash' else 0), (name, code, answer['refus']))
                rows.append({'case': name, 'code': code, 'verdict': answer['verdict'],
                             'takes': answer['prises'], 'refusals': len(answer['refus']),
                             'not_replayable': answer['non_rejouable']})
    return rows


def main():
    files = []
    for name in ('mes_m2_feuille', 'mes_m3_m4_tour', 'mes_m5_parcours', 'mes_m6_session'):
        files += sorted((MICRO/name).rglob('*.py'))
        if (MICRO/name/'README.md').is_file():
            files.append(MICRO/name/'README.md')
    files += [MICRO/'mes_m5_parcours/cuda/traversal_bench.cu',
              MICRO/'mes_m6_session/mes_m6_session_cost.cu']
    before = {str(path.relative_to(ROOT)): digest(path) for path in files}
    for relative, hashed in before.items():
        data = subprocess.check_output(['git', 'show', PIN+':'+relative], cwd=ROOT)
        require(hashlib.sha256(data).hexdigest() == hashed, 'source différente du pin : '+relative)
    results = {'m2': m2_tests(), 'm4': m4_tests(), 'm5': m5_tests(), 'm6': m6_tests()}
    after = {str(path.relative_to(ROOT)): digest(path) for path in files}
    require(before == after, 'sources modifiées pendant le test')
    print(json.dumps({'pin': PIN, 'sources_git_pin_and_before_after_equal': True, 'source_sha256': before,
                      'script_sha256': digest(Path(__file__)), 'results': results,
                      'scope': 'synthetic judge evidence only; no GCP/native benchmarks/real data'},
                     indent=1, sort_keys=True))


if __name__ == '__main__':
    main()
