"""Juge natif du classificateur : catalogue Gram puis separation affine exhaustive.

Les compteurs MEB du seul prefixe visite sont modelises separement ; ils ne
decident pas le kind. Aucun code produit n'est importe. --selftest reste Python.
"""
import copy
from functools import lru_cache
from itertools import combinations, islice
import json
import subprocess
import sys

import cells_oracle as cells
import classification_model as classification

require = classification.require


def requests(bits):
    rows = []
    for name, shell in classification.fixtures(bits):
        # La coquille large exerce le raccourci sans fabriquer des milliers de MEB de grandes parties.
        # t=m13 est deja juge analytiquement par classification_model, et refuse par la fenetre native.
        for k in ((1,) if name == 'shell13' else (5, 12)):
            rows.append(dict(name=name+'_K'+str(k), records=cells.fixtures.records(shell.points), kmax=k))
    for name, points in (
        ('square_center', ((0,0,0),(4,0,0),(0,4,0),(4,4,0),(2,2,0))),
        ('octa_center', ((2,2,0),(2,0,2),(0,2,2),(4,2,2),(2,4,2),(2,2,4),(2,2,2))),
        ('line13', tuple((i,0,0) for i in range(13)))):
        rows.append(dict(name=name, records=cells.fixtures.records(points), kmax=12))
    for name in ('global_qmin_K12','extended_q4_K12','high_tetra_K12'):
        row = next(r for r in rows if r['name'] == name)
        rows.append(dict(row, name=name+'_reverse', records=row['records'][::-1]))
    pair = ((0,0,0,7),(2,0,0,9))
    for name, records, k in (
        ('empty', (), 5), ('coordinate', ((1 << bits,0,0,7),), 5),
        ('duplicate_id', ((0,0,0,7),(1,0,0,7)), 5), ('weight', ((0,0,0,7),(0,0,0,8)), 5),
        ('zero', pair, 0), ('large', pair, 13)):
        rows.append(dict(name=name, records=records, kmax=k))
    return rows


@lru_cache(maxsize=4096)
def certified(points, center, qmin):
    return classification.certify(points, center, qmin)


def cell_of(points, ball, k):
    shell = certified(tuple(points[i] for i in ball.shell), ball.center, ball.qmin)
    t = classification.native_window(shell, ball.p, k, 12)
    expected = classification.expected(shell, t)  # Univers exhaustif, pas simulation firststrict.
    work = dict.fromkeys(cells.FIELDS, 0)
    for subset in islice(combinations(ball.shell, t), expected['examined']):
        ledger = cells.meb.local_meb(tuple(points[i] for i in subset))['ledger']
        for key in work:
            work[key] += ledger[key]
    return dict(order=k, qmin=ball.qmin, kind=expected['kind'], ledger=dict(
        combinations=expected['combinations'], examined=expected['examined'],
        meb_calls=expected['expected_meb_calls'], meb=work))


@lru_cache(maxsize=128)
def answer(records, k, bits):
    reason = 'none'
    if not records:
        reason = 'empty_input'
    elif any(any(v < 0 or v >= 1 << bits for v in row[:3]) for row in records):
        reason = 'coordinate_out_of_domain'
    elif len({row[3] for row in records}) != len(records):
        reason = 'duplicate_point_id'
    elif k < 1 or k > 12:
        reason = 'kmax_out_of_range'
    elif len({row[:3] for row in records}) != len(records):
        reason = 'multiplicity_unsupported'
    status = 'ok' if reason == 'none' else (
        'unsupported_degeneracy' if reason == 'multiplicity_unsupported' else 'invalid_input')
    result = dict(status=status, reason=reason, coord_bits=bits, kmax=k, owner_after=0, classifications=None)
    if reason != 'none':
        return result
    points, _ = cells.model.prepared(records)
    result['classifications'] = [dict(ball=i, **cell_of(points, ball, order))
        for i, ball in enumerate(cells.model.catalogue(points, k))
        for order in range(ball.p+ball.qmin-1, min(ball.p+len(ball.shell), k)+1)]
    return result


def judge(row, request, bits):
    return classification.equal(row, answer(tuple(request['records']), request['kmax'], bits))


def selftest():
    checks, corruptions, count = 0, 0, 0
    for bits in (18,21,24):
        reqs = requests(bits)
        for r in reqs:
            row = answer(tuple(r['records']),r['kmax'],bits)
            checks += judge(copy.deepcopy(row),r,bits)
            count += len(row['classifications'] or ())
        request = next(r for r in reqs if r['name'] == 'global_qmin_K12')
        row = answer(tuple(request['records']),request['kmax'],bits)
        at = next(i for i,c in enumerate(row['classifications']) if c['ledger']['examined'] == 3)
        def corrupt(fn):
            nonlocal corruptions
            bad = copy.deepcopy(row); fn(bad)
            try:
                judge(bad,request,bits)
            except ValueError:
                corruptions += 1
                return
            raise ValueError('corruption non detectee')
        corrupt(lambda v: v['classifications'].pop())
        corrupt(lambda v: v['classifications'].reverse())
        corrupt(lambda v: v['classifications'][at].__setitem__('kind','birth'))
        corrupt(lambda v: v['classifications'][at].__setitem__('qmin',4))
        corrupt(lambda v: v['classifications'][at]['ledger'].__setitem__('examined',1))
        corrupt(lambda v: v['classifications'][at]['ledger'].__setitem__('meb_calls',False))
        corrupt(lambda v: v['classifications'][at]['ledger']['meb'].__setitem__('containing',1))
        corrupt(lambda v: v.__setitem__('owner_after',1))
    require(corruptions == 24 and count >= 1000 and checks >= 20000, 'planchers')
    print(json.dumps(dict(verdict='conforme', native=0, cells=count, checks=checks, corruptions=corruptions),sort_keys=True))


def run(executable):
    profile = subprocess.run([executable,'--profile'], capture_output=True, text=True, timeout=10, check=True)
    require(profile.stderr == '', 'stderr profile')
    bits = cells.parse(profile.stdout)['coord_bits']
    require(type(bits) is int and bits in (18,21,24), 'profil')
    reqs = requests(bits)
    encoded = ''.join('%d %d\n' % (r['kmax'],len(r['records']))+
                      ''.join(' '.join(map(str,p))+'\n' for p in r['records']) for r in reqs)
    result = subprocess.run([executable], input=encoded, capture_output=True, text=True, timeout=180)
    require(result.returncode == 0 and result.stderr == '', 'processus natif')
    lines = result.stdout.splitlines()
    require(len(lines) == len(reqs), 'inventaire reponses')
    checks = sum(judge(cells.parse(line),r,bits) for line,r in zip(lines,reqs))
    count = sum(len(answer(tuple(r['records']),r['kmax'],bits)['classifications'] or ()) for r in reqs)
    require(len(reqs) == 31 and count >= 350 and checks >= 7000, 'planchers')
    print(json.dumps(dict(verdict='conforme', bits=bits, requests=len(reqs), cells=count, checks=checks),sort_keys=True))


if __name__ == '__main__':
    try:
        require(len(sys.argv) == 2, 'usage classification_oracle.py executable|--selftest')
        selftest() if sys.argv[1] == '--selftest' else run(sys.argv[1])
    except (ValueError, TypeError, KeyError, IndexError, subprocess.SubprocessError) as error:
        print('REFUS '+str(error),file=sys.stderr)
        raise SystemExit(1)
