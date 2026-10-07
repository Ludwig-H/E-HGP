"""Injections supplementaires et regressions du vrai main MES-M2, sans GPU ni donnees reelles.

La fabrique de fichiers et les doubles d'outils du test officiel sont importes et epingles.
Les mutations, attendus et controles ci-dessous sont independants. Aucun appel a son
main(), qui relirait le recu G4 ; seules ses fabriques synthetiques sont utilisees.
"""
import importlib.util
import json
from pathlib import Path
import subprocess
from unittest.mock import patch


def check(value, message):
    if not value:
        raise RuntimeError(message)


def run(root):
    path = root / 'morsehgp3D_v12/microbancs/mes_m2_feuille/tests/test_juge_m2.py'
    spec = importlib.util.spec_from_file_location('audit_m2_factory', path)
    factory = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(factory)
    cases = [
        ('baseline', {}, [], 'adopte'),
        ('host_identity_error', {'identite_hote_fausse': True}, [], 'refuse'),
        ('host_exit_2', {'commandes': {'identity_host': 2}}, [], 'refuse'),
        ('stale_file', {'prises_perimees': True, 'banc_sans_ecrire': True}, [], 'refuse'),
        ('wrong_nonce', {'jeton_banc': 'older-session'}, [], 'refuse'),
        ('binary_changed', {'modifier_binaire': True}, [], 'refuse'),
        ('missing_grid_frame', {}, ['--frames', 'ng00,ng01'], 'refuse'),
        ('admission_failed', {'refus_admission': ('ng01_k10_l24.bin',)}, [], 'refuse'),
        ('nonfinite_times', {'ms_non_finies': True}, [], 'refuse'),
        ('raw_times_override_median', {}, [], 'rejete'),
        ('sanitizer_empty_forms', {}, [], 'adopte'),
        ('discarded_exit_1', {'commandes': {'bench_discarded': 1}}, [], 'adopte'),
        ('contradictory_mismatch', {}, [], 'adopte'),
    ]
    output = {}
    original = factory.charge_banc
    for name, scenario, options, expected in cases:
        injected = []
        def charge(m, path, ident, forms, reps, warmup, nonce, scenario, leaves=None):
            result = original(m, path, ident, forms, reps, warmup, nonce, scenario, leaves)
            case = result['cases'][0]
            if name == 'sanitizer_empty_forms' and leaves is not None:
                case['forms'] = []
                case['leaves'] = 0
                injected.append({'stage': 'sanitizer', 'forms': 0, 'leaves': 0})
            if name == 'contradictory_mismatch':
                for row in case['forms']:
                    row['mismatched_counts'] = 1
                    row['overflow'] = True
                injected.append({'identity': result['identity'], 'mismatched_counts': 1, 'overflow': True})
            if name == 'raw_times_override_median':
                for row in case['forms']:
                    if row['form'] == 'j3':
                        row['ms'] = [60.0] * reps
                        row['median_ms'] = 6.0
                injected.append({'j3_raw_median': 60, 'j3_declared_median': 6})
            return result
        # Le double officiel couvre toutes les commandes externes. Toute fuite hors
        # de cette frontiere est bloquee explicitement, y compris git/nvidia-smi.
        with patch.object(factory, 'charge_banc', side_effect=charge), \
                patch.object(subprocess, 'run', side_effect=RuntimeError('external process forbidden during judge probe')):
            code, report, journal = factory.jouer(dict(scenario, nom=name), options)
        actual = report['verdicts']['j3']['verdict']
        check(code == 0 and actual == expected, name + ': unexpected verdict ' + actual)
        check(report['choice'] == ('j3' if expected == 'adopte' else None), name + ': unexpected choice')
        if expected == 'refuse':
            check(report['refusals'], name + ': refusal reason missing')
        if name == 'admission_failed':
            check(not any(s.startswith('bench_') or s == 'identity_host' for s in journal),
                  'native work reached after admission refusal')
        if name == 'stale_file':
            check('fichier de prise absent' in ' '.join(report['refusals']), 'stale file survived unlink')
        if name == 'sanitizer_empty_forms':
            check(len(injected) == 3 and all(v['proof'] for v in report['sanitizer'].values()),
                  'three empty sanitizer cases not admitted')
        if name == 'discarded_exit_1':
            check(report['bench_discarded']['code'] == 1 and report['bench_discarded']['proof'] is True,
                  'contradictory discarded code not admitted')
        ratio = report['verdicts']['j3'].get('ratio_gm_all_cases')
        if name == 'raw_times_override_median':
            check(abs(ratio - 0.6) < 1e-12, 'judge must derive ratio from raw times')
        output[name] = {'main_code': code, 'verdict': actual, 'choice': report['choice'],
                        'refusals_count': len(report['refusals']), 'refusals_first': report['refusals'][:2],
                        'valid_takes': len(report['evidence']['runs']), 'dumps': len(report['evidence']['dumps']),
                        'ratio_gm': ratio, 'injected_outputs': len(injected),
                        'injection_example': injected[0] if injected else None}
    return output
