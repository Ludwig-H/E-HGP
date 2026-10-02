"""A/B exact des options : oracle Gram/Fraction existant, encodages canoniques et travail J2 explicite."""
import argparse
import copy
import hashlib
import json
import subprocess

from fraction_oracle import requests
from fraction_model import require
from judge import canonical_answer, check_response, parse

CACHE_FIELDS = ('region_line_evaluations', 'region_line_cache_hits', 'region_line_fallbacks')


def compare(req, answer, reference, bits, cache, sort, workers, serial=None):
    checks = check_response(req, answer, bits)
    for key, wanted in (('cache_center_lines', cache), ('indirect_sort', sort)):
        require(type(answer.get(key)) is bool and answer[key] == wanted, 'option non observee : ' + key)
    require(type(answer.get('workers')) is int and answer['workers'] == workers, 'Pool W non observe')
    require(canonical_answer(answer) == canonical_answer(reference), 'geometrie/encodages A/B differents')
    left, right = answer['ledger'], reference['ledger']
    require({k: v for k, v in left.items() if k not in CACHE_FIELDS} ==
            {k: v for k, v in right.items() if k not in CACHE_FIELDS}, 'travail logique A/B different')
    if not req.refusal:
        if not cache:
            require(left == right and left['region_line_cache_hits'] == left['region_line_fallbacks'] == 0,
                    'cache desactive exerce')
        if left['max_leaf'] <= 32:
            require(left['region_line_fallbacks'] == 0, 'repli dans une petite feuille')
    if serial is not None:
        require(left == serial['ledger'], 'travail optionnel depend de W')
    return checks + 7


def selftest():
    from fixtures import records
    from judge import Request
    from model_test import truthful_response
    positives, corruptions = 0, 0
    for bits in (18, 21, 24):
        req = Request('cache-model', records(((0, 0, 0), (4, 0, 0))), 1)
        reference = truthful_response(req, bits)
        reference.update(workers=0, cache_center_lines=False, indirect_sort=False)
        reference['ledger'].update(region_line_tests=7, region_line_evaluations=7)
        actual = copy.deepcopy(reference)
        actual.update(workers=4, cache_center_lines=True, indirect_sort=True)
        actual['ledger'].update(region_line_evaluations=4, region_line_cache_hits=3)
        compare(req, actual, reference, bits, True, True, 4, actual)
        positives += 1
        for change in (
                lambda v: v.__setitem__('cache_center_lines', False),
                lambda v: v.__setitem__('indirect_sort', 1),
                lambda v: v.__setitem__('workers', 8),
                lambda v: v['ledger'].__setitem__('region_line_evaluations', 7),
                lambda v: v['ledger'].__setitem__('region_line_cache_hits', True),
                lambda v: v['ledger'].__setitem__('region_line_fallbacks', 5),
                lambda v: v['ledger'].__setitem__('region_line_fallbacks', 1),
                lambda v: v['ledger'].__setitem__('prefixes', 1),
                lambda v: v['levels'].__setitem__(0, ['0', '2'])):
            bad = copy.deepcopy(actual)
            change(bad)
            try:
                compare(req, bad, reference, bits, True, True, 4, actual)
            except (ValueError, KeyError, TypeError):
                corruptions += 1
            else:
                raise ValueError('corruption optionnelle non detectee')
        fallback = copy.deepcopy(actual)
        fallback['ledger'].update(max_leaf=33, region_line_evaluations=7, region_line_cache_hits=0,
                                  region_line_fallbacks=7)
        fallback_reference = copy.deepcopy(reference)
        fallback_reference['ledger']['max_leaf'] = 33
        compare(req, fallback, fallback_reference, bits, True, True, 4)
        positives += 1
    require(positives == 6 and corruptions == 27, 'planchers modele options')
    print('catalogue_optimization_model conforme positives6 corruptions27 native0')


def run(probe, cache, sort):
    options = dict(capture_output=True, text=True, encoding='utf-8', errors='backslashreplace')
    profile = subprocess.run([probe, '--profile'], timeout=15, **options)
    require(profile.returncode == 0 and not profile.stderr, 'profil refuse')
    bits = parse(profile.stdout).get('coord_bits')
    require(type(bits) is int and bits in (18, 21, 24), 'profil invalide')
    batch, _, _ = requests(bits)
    payload = ''.join(req.encode() for req in batch)

    def execute(flags):
        result = subprocess.run([probe] + flags, input=payload, timeout=120, **options)
        require(result.returncode == 0 and not result.stderr, 'pilote refuse : ' + result.stderr)
        lines = result.stdout.splitlines()
        require(len(lines) == len(batch), 'inventaire des reponses')
        return [parse(line) for line in lines]

    reference = execute([])
    checks = 0
    for req, answer in zip(batch, reference):
        checks += compare(req, answer, answer, bits, False, False, 0)
    flags = (['--cache-center-lines'] if cache else []) + (['--indirect-sort'] if sort else [])
    serial = execute(flags)
    parallel = execute(flags + ['--workers', '4'])
    for req, base, one, many in zip(batch, reference, serial, parallel):
        checks += compare(req, one, base, bits, cache, sort, 0)
        checks += compare(req, many, base, bits, cache, sort, 4, one)
    hits = sum(a['ledger'].get('region_line_cache_hits', 0) for a in serial)
    if cache:
        require(hits > 0, 'aucune reponse reutilisee par le vrai cache')
    refused = 3 * sum(bool(req.refusal) for req in batch)
    accepted = 3 * len(batch) - refused
    require(len(batch) == 378 and (accepted, refused) == (1074, 60) and checks > 45000, 'planchers lot options')
    print(json.dumps(dict(bits=bits, cache_center_lines=cache, indirect_sort=sort, requests=3*len(batch),
                         accepted=accepted, refused=refused, paired=2*len(batch), workers=[0, 4], checks=checks,
                         cache_hits=hits, input_sha256=hashlib.sha256(payload.encode()).hexdigest()), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('probe', nargs='?')
    parser.add_argument('--cache', action='store_true')
    parser.add_argument('--sort', action='store_true')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest and args.probe is None and not args.cache and not args.sort:
        selftest()
    elif args.probe and not args.selftest and (args.cache or args.sort):
        run(args.probe, args.cache, args.sort)
    else:
        parser.error('pilote avec option(s), ou --selftest seul')
