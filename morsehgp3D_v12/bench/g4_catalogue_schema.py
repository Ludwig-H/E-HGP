"""Validation de preuve du pilote T1-b ; schema emis par catalogue_probe.cpp.

Une preuve absente/incoherente est refusee avant tout verdict de performance.
Les sorties d'echec ne sont pas prises pour des sorties de succes incompletes.
"""
import re

LEDGER = ('nodes leaves filter_tests max_depth max_leaf dominance_tests prefixes judged census_tests emitted '
          'incidences q4_candidates q4_levels region_pair_tests region_pair_rejects region_line_tests '
          'region_line_rejects region_line_evaluations region_line_cache_hits region_line_fallbacks').split()
STAGES = ('traversal_ns count_ns fill_ns levels_ns sort_ns assemble_ns table_ns transfer_ns publish_ns').split()
DIAGNOSTICS = ('levels tasks candidates leaves_narrow leaves_medium leaves_wide leaves_exact leaves_virtual_warp '
               'leaves_rewritten max_leaf_span traversal_ns count_ns fill_ns levels_ns sort_ns assemble_ns table_ns '
               'peak_bytes chains_repaired chain_elements').split()
DEVICE = ('batches replayed_leaves replayed_balls replayed_wide replayed_span rewritten_device rewritten_host '
          'device_bytes pinned_bytes allocations arena_bytes transfer_ns transfer_h2d_bytes transfer_d2h_bytes '
          'transfer_ops publish_ns').split()
MUTANTS = {'feuille_non_resolue_admise_sans_rejeu': 'identite', 'fin_sans_departage_exact': 'identite',
           'un_fil_par_feuille': 'temps'}


def uint(value):
    return type(value) is int and 0 <= value < 2**64


def digest(value):
    return isinstance(value, str) and re.fullmatch('[0-9a-f]{64}', value) is not None


def numbers(record, fields):
    return isinstance(record, dict) and all(uint(record.get(field)) for field in fields)


def okay(record):
    return isinstance(record, dict) and record.get('status') == 'ok' and record.get('reason') == 'none'


def valid_run(run, passes, path, k, leaf, threads, digests):
    """Sortie de succes complete ; les comptes d'erreur sont traites par le juge appelant."""
    if not isinstance(run, dict) or type(run.get('unreadable')) is not int or run['unreadable'] != 0:
        return False
    rows, hashes = run.get('passes'), run.get('digests')
    if not isinstance(rows, list) or len(rows) != passes or not isinstance(hashes, list):
        return False
    if len(hashes) != (passes if digests else 0) or not all(digest(value) for value in hashes):
        return False
    if not okay(run.get('exit')):
        return False
    if path == 'device':
        if not okay(run.get('open')) or not uint(run['open'].get('open_ns')):
            return False
    elif run.get('open') is not None:
        return False
    sites = None
    for index, row in enumerate(rows):
        if not isinstance(row, dict) or row.get('phase') != 'catalogue' or row.get('path') != path or not okay(row):
            return False
        expected = {'pass': index, 'coord_bits': 21, 'kmax': k, 'leaf': leaf, 'threads': threads}
        if any(type(row.get(field)) is not int or row[field] != value for field, value in expected.items()):
            return False
        if not numbers(row, ('sites', 'wall_ns', 'balls', 'incidences', 'levels')) or not 0 < row['sites'] < 2**32:
            return False
        if sites is not None and row['sites'] != sites:
            return False
        sites = row['sites']
        if not numbers(row.get('ledger'), LEDGER) or not numbers(row.get('diagnostics'), DIAGNOSTICS) \
                or not numbers(row.get('device'), DEVICE) or not numbers(row.get('stages'), STAGES):
            return False
        if row['balls'] != row['ledger']['emitted'] or row['incidences'] != row['ledger']['incidences']:
            return False
        if path == 'cpu' and any(row['device'][field] != 0 for field in DEVICE):
            return False
        for field in STAGES:
            source = row['device'] if field in ('transfer_ns', 'publish_ns') else row['diagnostics']
            if row['stages'][field] != source[field]:
                return False
        if sum(row['stages'][field] for field in STAGES) > row['wall_ns']:
            return False
    return True


def entries(steps, name, keys, expected):
    rows = steps.get(name)
    if not isinstance(rows, list) or any(not isinstance(row, dict) for row in rows):
        raise ValueError(name + ' : liste absente ou invalide')
    seen = []
    for row in rows:
        key = tuple(row.get(field) for field in keys)
        for field, value in zip(keys, key):
            if field in ('k', 'leaf', 'process') and not uint(value):
                raise ValueError(name + ' : identifiant non entier')
        if key not in expected or key in seen:
            raise ValueError(name + ' : cas absent du contrat ou identifiant duplique')
        seen.append(key)
    if len(seen) != len(expected):
        raise ValueError(name + ' : cohorte incomplete')
    return rows


def validate_report(report, contract):
    """Rend une raison de refus, ou None ; n'accepte jamais une exception comme preuve."""
    try:
        if not isinstance(report, dict) or not isinstance(report.get('steps'), dict):
            raise ValueError('rapport ou etapes absents')
        if report.get('schema') != 'mhgp12_g4_catalogue_device_v1' or report.get('budget_ns') != 45_000_000 \
                or type(report.get('budget_ns')) is not int or report.get('contract') != contract:
            raise ValueError('schema ou contrat du rapport incoherent')
        steps, opts = report['steps'], report.get('options')
        if not isinstance(opts, dict):
            raise ValueError('options absentes')
        for key in ('processes', 'passes', 'threads'):
            if type(opts.get(key)) is not int or (opts[key] != contract[key] if key == 'threads' else opts[key] < contract[key]):
                raise ValueError('contrat de mesure : ' + key)
        if type(opts.get('skip_mutants')) is not bool:
            raise ValueError('option skip_mutants invalide')
        for key in ('environment', 'build', 'gates'):
            if not isinstance(steps.get(key), dict):
                raise ValueError(key + ' absent ou invalide')
        for key in ('nvcc', 'gpu'):
            if not isinstance(steps['environment'].get(key), str) or not steps['environment'][key].strip():
                raise ValueError('outil absent ou invalide : ' + key)
        if not isinstance(steps['gates'].get('device_open_stdout', ''), str):
            raise ValueError('sortie de porte appareil invalide')
        if type(steps['gates'].get('timeout')) is not bool:
            raise ValueError('etat de delai des portes absent')
        built = steps['build']
        if type(built.get('ok')) is not bool:
            raise ValueError('etat de construction invalide')
        if built['ok']:
            if any(type(built.get(key)) is not int or built[key] != 0 for key in ('configure', 'build')):
                raise ValueError('construction declaree conforme sans codes conformes')
            binaries = built.get('binaries')
            if not isinstance(binaries, dict) or not all(digest(binaries.get(name)) for name in
                                                        ('mhgp12_catalogue_probe', 'mhgp12_catalogue_device_unit')):
                raise ValueError('empreintes des binaires absentes')
        for name in ('code', 'device_open_code'):
            if steps['gates'].get(name) is not None and type(steps['gates'][name]) is not int:
                raise ValueError('code de porte invalide')
        for name in ('gpu_quiet_before', 'gpu_quiet_after'):
            if steps.get(name) is not True:
                raise ValueError('isolation GPU non prouvee : ' + name)
        for entry in entries(steps, 'identity', ('case', 'k', 'leaf'),
                             [tuple(case) for case in contract['identity_cases']]):
            for path, count in (('cpu', 1), ('device', contract['identity_passes'])):
                code = entry.get(path + '_code')
                if code is not None and type(code) is not int:
                    raise ValueError('code de sonde invalide')
                if code == 0 and not valid_run(entry.get(path), count, path, entry['k'], entry['leaf'],
                                               contract['threads'], True):
                    raise ValueError('identite : sortie incomplete ou incoherente')
        for entry in entries(steps, 'cpu_timing', ('case', 'k', 'leaf'),
                             [tuple(case) for case in contract['f2_cases']]):
            if type(entry.get('code')) is not int or entry['code'] != 0 or not valid_run(
                    entry.get('run'), contract['cpu_passes'], 'cpu', entry['k'], entry['leaf'], contract['threads'], True):
                raise ValueError('temps CPU : sortie incomplete ou incoherente')
        for k, processes, passes in ((5, opts['processes'], opts['passes']),
                                     (10, contract['k10_processes'], contract['k10_passes'])):
            expected = [(frame, process) for frame in contract['frames'] for process in range(processes)]
            for entry in entries(steps, 'device_timing_k%d' % k, ('frame', 'process'), expected):
                if type(entry.get('code')) is not int or entry['code'] != 0 or not valid_run(
                        entry.get('run'), passes, 'device', k, 24, contract['threads'], False):
                    raise ValueError('temps appareil : sortie incomplete ou incoherente')
        for entry in entries(steps, 'mutants', ('id',), [(name,) for name in contract['mutants']]):
            if entry.get('critere') != MUTANTS[entry['id']]:
                raise ValueError('critere mutant non conforme au manifeste')
            if type(entry.get('applied')) is not bool or not isinstance(entry.get('build'), dict) \
                    or type(entry['build'].get('ok')) is not bool:
                raise ValueError('application ou construction mutant non prouvee')
            if entry['applied'] and entry['build']['ok']:
                if entry['critere'] == 'temps':
                    if type(entry.get('timeout')) is not bool:
                        raise ValueError('delai du mutant non prouve')
                    if entry['timeout']:
                        if entry.get('code') is not None:
                            raise ValueError('delai et code mutant incoherents')
                    elif type(entry.get('code')) is not int or entry['code'] != 0 or not valid_run(
                            entry.get('run'), 4, 'device', 5, 24, contract['threads'], True):
                        raise ValueError('mesure du mutant incomplete')
                else:
                    if type(entry.get('unit_timeout')) is not bool:
                        raise ValueError('delai de porte mutant non prouve')
                    if entry['unit_timeout'] and entry.get('unit_code') is not None:
                        raise ValueError('delai et code de porte mutant incoherents')
                    for name in ('unit_code', 'ng00_code'):
                        if entry.get(name) is not None and type(entry[name]) is not int:
                            raise ValueError('code mutant invalide')
                    if entry.get('ng00_code') == 0 and not valid_run(
                            entry.get('ng00'), 1, 'device', 5, 24, contract['threads'], True):
                        raise ValueError('sonde mutant incomplete')
                    if entry.get('ng00_code') == 0 and entry.get('ng00_digest') != entry['ng00']['digests'][0]:
                        raise ValueError('empreinte mutant incoherente')
        return None
    except (ValueError, TypeError, KeyError, AttributeError) as error:
        return str(error)
