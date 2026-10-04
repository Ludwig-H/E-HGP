"""Oracle Gram/Fraction catalogue et controles independants des diagnostics de la coupe adaptative."""
import argparse
import copy
import hashlib
import json
import subprocess

from fraction_model import require
from fraction_oracle import requests
from judge import canonical_answer, check_response, parse


def uint(value, bound=2**64):
    return type(value) is int and 0 <= value < bound


def check_plan(answer, bits):
    p, tasks, total = answer['planning'], answer['tasks'], answer['ledger']
    require(type(p) is dict and p.get('adaptive') is True and type(p.get('memory_fallback')) is bool,
            'planning adaptatif requis')
    for key in ('plan_nodes', 'plan_leaves', 'empty_leaves', 'rounds', 'priority_tests', 'replay_bytes'):
        require(uint(p.get(key)), 'entier du planning : ' + key)
    require(type(tasks) is list and 1 <= p['plan_leaves'] <= 1024, 'plafond de planning')
    require(p['plan_nodes'] == 2*p['plan_leaves']-1 and len(tasks)+p['empty_leaves'] == p['plan_leaves'],
            'fantomes et arbre binaire du plan')
    require(p['rounds'] <= 3*bits and p['plan_nodes'] <= total['nodes'], 'rondes et noeuds visites')
    require(p['replay_bytes'] >= 8*len(answer['sites']), 'racine et source du rejeu simultanees')
    paths = []
    sums = dict.fromkeys(total, 0)
    for t in tasks:
        require(type(t) is dict and t.get('path_known') is True and t.get('inside_known') is True,
                'descripteur geometrique complet')
        for key in ('depth', 'count', 'capacity', 'inside', 'count_ns', 'fill_ns'):
            require(uint(t.get(key)), 'entier du descripteur : ' + key)
        require(t['depth'] <= 3*bits and 0 < t['count'] <= t['capacity'] <= len(answer['sites']) and
                t['inside'] <= t['count'], 'bornes locales')
        require(type(t['path']) is list and len(t['path']) == 2 and all(uint(v) for v in t['path']), 'chemin 128 bits')
        raw = (t['path'][0] << 64) | t['path'][1]
        require(raw % (1 << (128-t['depth'])) == 0, 'bits hors chemin nuls')
        paths.append(format(raw, '0128b')[:t['depth']])
        for key in ('lo', 'hi'):
            require(type(t.get(key)) is list and len(t[key]) == 3 and all(uint(v, 2**bits+1) for v in t[key]),
                    'boite entiere T0')
        require(all(a < b for a, b in zip(t['lo'], t['hi'])), 'boite non vide')
        # G1 ne peut retirer aucun point de Q. La population interieure se compte donc sur TOUT le Cloud,
        # sans reprendre les reservoirs ni les listes du produit.
        inside = sum(all(lo <= x < hi for lo, x, hi in zip(t['lo'], site, t['hi'])) for site in answer['sites'])
        require(t['inside'] == inside, 'population demi-ouverte comparee au nuage global')
        require(type(t.get('ledger')) is dict and set(t['ledger']) == set(total), 'inventaire travail par job')
        for key, value in t['ledger'].items():
            require(uint(value), 'compteur de job exact')
            sums[key] = max(sums[key], value) if key.startswith('max_') else sums[key] + value
    require(paths == sorted(set(paths)) and all(not b.startswith(a) for a, b in zip(paths, paths[1:])),
            'jobs en antichaine et ordre DFS')
    for key, value in sums.items():
        if key == 'nodes':
            require(value + p['plan_nodes'] == total[key], 'noeuds prefixe+suffixes sans doublon')
        elif key == 'filter_tests':
            require(value <= total[key], 'filtre du preambule separe')
        elif key == 'max_depth':
            require(value <= total[key], 'profondeur logique du preambule separee')
        else:
            require(value == total[key], 'travail complet des suffixes : ' + key)
    return len(tasks)


def stable_plan(answer):
    return dict(planning=answer['planning'], tasks=[{k: v for k, v in t.items() if k not in ('count_ns', 'fill_ns')}
                                                   for t in answer['tasks']])


def compare(req, actual, baseline, bits, workers, previous=None):
    checks = check_response(req, actual, bits)
    require(actual.get('adaptive_frontier') is True and type(actual.get('workers')) is int and
            actual['workers'] == workers, 'voie adaptative/W observes')
    require(canonical_answer(actual) == canonical_answer(baseline), 'sortie exacte differente de la reference')
    require(actual['ledger'] == baseline['ledger'], 'travail geometrique change par le planning')
    if req.refusal:
        require('planning' not in actual and 'tasks' not in actual, 'diagnostics partiels sur refus')
    else:
        check_plan(actual, bits)
        if previous is not None:
            require(stable_plan(actual) == stable_plan(previous), 'plan/ledgers dependent de W')
    return checks + 5


def selftest():
    from fixtures import records
    from judge import Request
    from model_test import truthful_response
    positives = corruptions = 0
    for bits in (18, 21, 24):
        req = Request('adaptive-model', records(((0, 0, 0), (4, 0, 0))), 1)
        base = truthful_response(req, bits)
        base['ledger'].update(nodes=1, leaves=1, filter_tests=4, max_leaf=2)
        answer = copy.deepcopy(base)
        answer.update(adaptive_frontier=True, workers=4)
        answer['planning'] = dict(adaptive=True, memory_fallback=False, plan_nodes=1, plan_leaves=1,
                                  empty_leaves=0, rounds=0, priority_tests=0, replay_bytes=16)
        task_ledger = copy.deepcopy(base['ledger']); task_ledger.update(nodes=0, filter_tests=0)
        answer['tasks'] = [dict(path=[0, 0], lo=[0, 0, 0], hi=[5, 1, 1], path_known=True, inside_known=True,
                               depth=0, count=2, capacity=2, inside=2, count_ns=1, fill_ns=1, ledger=task_ledger)]
        compare(req, answer, base, bits, 4, answer); positives += 1
        for change in (
                lambda a: a.__setitem__('adaptive_frontier', False),
                lambda a: a.__setitem__('workers', True),
                lambda a: a['planning'].__setitem__('plan_nodes', 2),
                lambda a: a['planning'].__setitem__('empty_leaves', 1),
                lambda a: a['planning'].__setitem__('priority_tests', True),
                lambda a: a['planning'].__setitem__('replay_bytes', 15),
                lambda a: a['tasks'][0].__setitem__('inside', 1),
                lambda a: a['tasks'][0].__setitem__('path', [0, 1]),
                lambda a: a['tasks'][0].__setitem__('capacity', 1),
                lambda a: a['tasks'][0]['ledger'].__setitem__('leaves', 0),
                lambda a: a['tasks'][0]['ledger'].__setitem__('nodes', 1),
                lambda a: a['tasks'][0].__setitem__('path_known', False)):
            bad = copy.deepcopy(answer); change(bad)
            try:
                compare(req, bad, base, bits, 4, answer)
            except (ValueError, TypeError, KeyError):
                corruptions += 1
            else:
                raise ValueError('corruption adaptative survivante')
    require((positives, corruptions) == (3, 36), 'planchers modele diagnostics')
    print('adaptive_oracle_model conforme positives3 corruptions36 native0')


def run(probe, combined):
    options = dict(capture_output=True, text=True, encoding='utf-8', errors='backslashreplace')
    profile = subprocess.run([probe, '--profile'], timeout=15, **options)
    require(profile.returncode == 0 and not profile.stderr, 'profil refuse')
    bits = parse(profile.stdout)['coord_bits']
    require(type(bits) is int and bits in (18, 21, 24), 'profil non qualifie')
    batch, _, _ = requests(bits)
    payload = ''.join(req.encode() for req in batch)
    flags = ['--cache-center-lines', '--indirect-sort'] if combined else []

    def execute(extra):
        result = subprocess.run([probe]+flags+extra, input=payload, timeout=120, **options)
        require(result.returncode == 0 and not result.stderr, 'pilote refuse : '+result.stderr)
        lines = result.stdout.splitlines()
        require(len(lines) == len(batch), 'inventaire du lot')
        return [parse(line) for line in lines]

    baseline, one, many = execute([]), execute(['--adaptive-frontier', '--workers', '1']), execute([
        '--adaptive-frontier', '--workers', '4'])
    checks = 0
    for req, reference, a, b in zip(batch, baseline, one, many):
        checks += check_response(req, reference, bits)
        checks += compare(req, a, reference, bits, 1)
        checks += compare(req, b, reference, bits, 4, a)
    require(len(batch) == 378 and checks > 45000, 'planchers Fraction adaptatif')
    print(json.dumps(dict(bits=bits, combined=combined, requests=3*len(batch), accepted=1074, refused=60,
                          paired=2*len(batch), workers=[0, 1, 4], checks=checks,
                          input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe', nargs='?')
    parser.add_argument('--combined', action='store_true')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest and args.probe is None and not args.combined:
        selftest()
    elif args.probe and not args.selftest:
        run(args.probe, args.combined)
    else:
        parser.error('pilote [--combined], ou --selftest seul')
