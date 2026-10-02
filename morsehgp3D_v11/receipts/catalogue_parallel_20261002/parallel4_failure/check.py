#!/usr/bin/env python3
"""Lecture autonome de l'echec clos parallel4 ; aucun produit, reseau ou binaire execute."""
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent
COMMIT = 'a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e'
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-c',
              instance='ehgp-v7-3b1d496aed430749ea7e049f')
NATIVE = ('gcc_release', 'gcc_asan_ubsan', 'gcc_tsan', 'bits21', 'bits24', 'poison')
GATES = {'mhgp11_catalogue_parallel_'+name for name in (
    'equivalence', 'frontier_overlap', 'frontier_edges', 'frontier_deep',
    'global_limits', 'memory_and_refusals', 'timings', 'inventaire')}


def need(condition, message):
    if not condition:
        raise ValueError(message)


def read():
    provenance = json.loads((ROOT/'provenance.json').read_text())
    need(provenance['commit'] == COMMIT and provenance['source_package_copied'] is False and
         provenance['archive_copied'] is False, 'provenance capsule')
    for name, pin in provenance['copied_or_derived'].items():
        need(Path(name).name == name, 'nom capsule simple')
        need(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == pin['sha256'], 'empreinte '+name)
    result = {name: json.loads((ROOT/(name+'.json')).read_text())
              for name in ('receipt', 'matrix', 'asan18', 'stop', 'commands', 'build_errors')}
    result.update({name: (ROOT/(name+'.txt')).read_text() for name in ('mutants_catalogue','mutants_tower','stop')})
    result['stop_record'] = json.loads((ROOT/'stop.json').read_text())
    result['provenance'] = provenance
    return result


def judge(data):
    receipt = data['receipt']
    need(receipt['commit'] == COMMIT and receipt['target'] == TARGET, 'source et cible')
    need(receipt['status'] == 'failed_remote' and receipt['closure'] == 'stopped' and
         receipt['worker_exit_code'] == 1 and receipt['worker_outcome'] == 'exited', 'echec conserve')
    for key in ('targeted_shutdown_certified', 'results_verified', 'oslogin_key_removed',
                'private_key_deleted', 'reserve_released', 'start_certified', 'guest_guard_intact'):
        need(receipt[key] is True, 'fermeture '+key)
    need(receipt['stop_exit_code'] == 0 and not receipt['errors'] and not receipt['results_skipped_members'],
         'fermeture sans erreur ni archive ignoree')
    need(receipt['overflow'] == dict(evicted=[], truncated_streams=[]), 'capture tronquee')
    observed = receipt['observed_after']
    need(observed['name'] == TARGET['instance'] and observed['status'] == 'TERMINATED' and
         observed['lastStartTimestamp'] == receipt['generation'] == receipt['closing_generation'],
         'generation arretee exacte')
    stop = data['stop_record']
    need(stop['exit_code'] == 0 and '--expected-last-start-timestamp' in stop['argv'] and
         receipt['generation'] in stop['argv'], 'garde de fermeture')
    need(TARGET['instance']+' arrêtée et vérifiée (état GCE TERMINATED)' in data['stop'], 'sortie arret cible')
    need(receipt['results_sha256'] == data['provenance']['archive']['sha256'], 'archive epinglee')
    matrix = data['matrix']
    need(matrix['complete'] is True and matrix['conforming'] is False and matrix['exit_code'] == 1,
         'matrice non conforme conservee')
    configs = {c['name']: c for c in matrix['configurations']}
    need(set(configs) == set(NATIVE+('mutants','style','clang_release')), 'inventaire configurations')
    for name in NATIVE:
        config = configs[name]
        need(config['status'] == 'build_failed' and config['conforming'] is False, 'construction refusee '+name)
        need(config['tests']['failed'] == 8 and {f['test'] for f in config['failures']} == GATES,
             'huit lancements impossibles '+name)
        for failure in config['failures']:
            need('lancement_impossible' in '\n'.join(failure['excerpt']), 'echec de lancement, non geometrie')
        errors = '\n'.join(data['build_errors'][name])
        need("has no member named 'hex'" in errors and "reference to 'detail' is ambiguous" in errors,
             'deux causes C++ '+name)
        need(all('/tests/catalogue/parallel.cpp:' in line for line in data['build_errors'][name]),
             'aucune erreur produit dans les diagnostics compiles')
    totals = {key: sum(c.get('tests',{}).get(key,0) for c in configs.values())
              for key in ('passed','failed','selected')}
    need(totals == dict(passed=1729,failed=50,selected=1779), 'comptes matrice')
    need(configs['clang_release']['status'] == 'absent' and configs['clang_release']['optional'] is True,
         'clang facultatif absent')
    need(configs['style']['status'] == 'ok' and configs['style']['tests']['passed'] == 2, 'style seul distinct')
    mutants = configs['mutants']
    need({f['test'] for f in mutants['failures']} == {'mhgp11_mutants_catalogue','mhgp11_mutants_tower'},
         'deux portes mutants')
    need('TEMOIN ROUGE module=catalogue : aucun mutant juge' in data['mutants_catalogue'],
         'catalogue sans mutant juge')
    tower = data['mutants_tower']
    need(len(re.findall(r'^\S+\s+TUE\s+code\s*$', tower, re.M)) == 36 and
         'INVALIDES module=tower : 1 sur 37' in tower and 'cells_deux_passes_non_comparees' in tower and
         "unused parameter 'first'" in tower and "unused parameter 'second'" in tower,
         'mutant tower invalide, non mort causale')
    supplement = data['asan18']
    need(supplement['complete'] is True and supplement['conforming'] is True and supplement['exit_code'] == 0,
         'supplement ASan18 distinct')
    need(len(supplement['configurations']) == 1 and supplement['configurations'][0]['status'] == 'ok' and
         supplement['configurations'][0]['tests'] == dict(ctest_failed=0,ctest_total=107,failed=0,
                                                         not_run=0,passed=107,selected=107), 'ASan18 107/107')
    for name, code in (('000_matrice','1'),('001_asan18','0'),('002_parallel','2')):
        row = data['commands'][name]
        need(row['exit_code'] == code and row['group_closed'] == '1' and row['residual_group_killed'] == '0'
             and row['streams_truncated'] == '0', 'commande fermee '+name)
    benchmark = data['commands']['002_parallel']
    need(benchmark['stdout'].strip() == 'catalogue_parallel_refused: ValueError' and benchmark['stderr'] == ''
         and all(not name.startswith('files/') for name in benchmark['files']), 'aucun resultat benchmark')
    return dict(verdict='conforme_echec_preserve', native=0, cloud=0, matrix=totals, asan18=107,
                tower_killed=36, tower_invalid=1, catalogue_mutants_judged=0, benchmarks=0,
                closure='stopped_keys_removed_reserve_released')


def selftest(data):
    mutations = (
        lambda d: d['receipt'].__setitem__('status','completed'),
        lambda d: d['receipt'].__setitem__('targeted_shutdown_certified',False),
        lambda d: d['receipt'].__setitem__('reserve_released',False),
        lambda d: d['receipt'].__setitem__('oslogin_key_removed',False),
        lambda d: d['receipt'].__setitem__('private_key_deleted',False),
        lambda d: d['receipt']['observed_after'].__setitem__('name','another-instance'),
        lambda d: d['receipt'].__setitem__('closing_generation','another-generation'),
        lambda d: d['matrix'].__setitem__('conforming',True),
        lambda d: d['matrix']['configurations'][0]['tests'].__setitem__('passed',348),
        lambda d: d.__setitem__('mutants_catalogue','all killed'),
        lambda d: d.__setitem__('mutants_tower',d['mutants_tower'].replace('1 sur 37','0 sur 37')),
        lambda d: d['asan18']['configurations'][0]['tests'].__setitem__('passed',106),
        lambda d: d['commands']['002_parallel']['files'].append('files/parallel.json'),
        lambda d: d['commands']['002_parallel'].__setitem__('group_closed','0'),
    )
    for mutate in mutations:
        bad = copy.deepcopy(data); mutate(bad)
        try:
            judge(bad)
        except (ValueError,KeyError,TypeError):
            continue
        raise ValueError('corruption non detectee')
    return dict(verdict='conforme', corruptions=len(mutations), native=0, cloud=0)


if __name__ == '__main__':
    try:
        need(sys.argv[1:] in ([],['--selftest']), 'usage check.py [--selftest]')
        data = read(); result = judge(data)
        print(json.dumps(selftest(data) if sys.argv[1:] else result, sort_keys=True))
    except (ValueError,KeyError,TypeError,OSError) as error:
        print('REFUS '+str(error),file=sys.stderr)
        raise SystemExit(1)
