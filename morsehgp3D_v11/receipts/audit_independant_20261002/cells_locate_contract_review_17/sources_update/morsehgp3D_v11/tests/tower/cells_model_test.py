"""Sensibilite du juge T2, sans executable natif ; tests effectifs aussi sous python -O."""
import copy
import json
import sys

import cells_oracle as oracle


def main():
    checks, corruptions, totals = 0, 0, []
    for bits in (18, 21, 24):
        reqs = oracle.requests(bits)
        rows = [oracle.native_model(req, bits) for req in reqs]
        checks += sum(oracle.judge(row, req, bits) for row, req in zip(rows, reqs))
        pairs = [(r, a) for r, a in zip(reqs, rows) if r['name'] == 'square_center_K5']
        req, row = pairs[0]
        chosen = next(i for i, b in enumerate(row['balls']) if len(b['inner']) == 1 and len(b['shell']) == 4)
        cell_index = next(i for i, c in enumerate(row['balls'][chosen]['cells']) if c['order'] == 3)
        def cell(v): return v['balls'][chosen]['cells'][cell_index]
        def corrupt(fn):
            nonlocal corruptions
            bad = copy.deepcopy(row); fn(bad)
            try:
                oracle.judge(bad, req, bits)
            except (ValueError, KeyError, TypeError, IndexError):
                corruptions += 1
                return
            raise ValueError('corruption non detectee')
        corrupt(lambda v: cell(v)['traces'].pop())
        corrupt(lambda v: cell(v)['traces'].append(copy.deepcopy(cell(v)['traces'][0])))
        corrupt(lambda v: cell(v)['traces'].reverse())
        corrupt(lambda v: cell(v)['traces'][0]['sites'].__setitem__(11, 0))
        corrupt(lambda v: cell(v)['traces'][0]['sites'].__setitem__(0, oracle.NONE))
        corrupt(lambda v: cell(v)['traces'][0].__setitem__('arity', 2))
        corrupt(lambda v: cell(v).__setitem__('kind', 'birth'))
        corrupt(lambda v: cell(v).__setitem__('regular', True))
        corrupt(lambda v: cell(v)['ledger'].__setitem__('passes', 1))
        corrupt(lambda v: cell(v)['ledger'].__setitem__('meb_calls', 0))
        corrupt(lambda v: cell(v)['ledger']['meb'].__setitem__('point_tests', 0))
        corrupt(lambda v: v['cell_memory'].__setitem__('after', 1))
        corrupt(lambda v: v['cell_memory'].__setitem__('peak', 0))
        corrupt(lambda v: v.__setitem__('coord_bits', 19))
        corrupt(lambda v: v.__setitem__('owner_after', False))
        corrupt(lambda v: v['balls'][chosen]['inner'].clear())
        # Faits graves : le carre+centre a quatre traces a k3, aucune a k4 ; la coquille est plus grande que k.
        oracle.require(len(cell(row)['traces']) == 4, 'quatre aretes')
        birth = next(c for c in row['balls'][chosen]['cells'] if c['order'] == 4)
        oracle.require(birth['kind'] == 'birth' and not birth['traces'], 'naissance etendue')
        extended_req = next(r for r in reqs if r['name'] == 'extended_q4_K5')
        extended = oracle.expected(extended_req, bits)
        ball = next(b for b in extended['balls'] if b['qmin'] == 4 and len(b['shell']) == 5)
        triples, quads = (next(c for c in ball['cells'] if c['order'] == k) for k in (3, 4))
        oracle.require(len(triples['traces']) == 10 and 0 < len(quads['traces']) < 5, 'q4 etendue')
        wide_req = next(r for r in reqs if r['name'] == 'wide_shell')
        wide = oracle.expected(wide_req, bits)
        shell14 = next(b for b in wide['balls'] if len(b['shell']) == 14)
        oracle.require(len(shell14['cells']) == 1 and len(shell14['cells'][0]['traces']) == 14, 'coquille sans plafond12')
        line_req = next(r for r in reqs if r['name'] == 'line13_K12')
        line = oracle.expected(line_req, bits)
        longest = next(b for b in line['balls'] if len(b['inner']) == 11)
        oracle.require(len(longest['cells'][0]['traces']) == 2 and longest['cells'][0]['traces'][0]['arity'] == 12,
                       'trace stricte de cardinal12 sans padding')
        for reverse_name in ('square_center', 'extended_q4', 'maximum_tetra'):
            a = next(r for r in reqs if r['name'] == reverse_name+'_K12')
            b = next(r for r in reqs if r['name'] == reverse_name+'_reverse')
            oracle.equal_exact(oracle.expected(a,bits), oracle.expected(b,bits))
        totals.append(dict(bits=bits, requests=len(reqs), cells=sum(len(b['cells']) for r in rows for b in r['balls'] or ()),
                           traces=sum(len(c['traces']) for r in rows for b in r['balls'] or () for c in b['cells'])))
    malformed = 0
    for line in ('{', '{"x":1,"x":2}', '{"x":NaN}'):
        try:
            oracle.parse(line)
        except ValueError:
            malformed += 1
    oracle.require(corruptions == 48 and malformed == 3, 'planchers corruptions')
    oracle.require(checks >= 201000 and all(r['requests'] >= 50 and r['cells'] >= 1300 and r['traces'] >= 2200
                                         for r in totals), 'planchers du modele')
    print(json.dumps(dict(verdict='conforme', native=0, checks=checks, corruptions=corruptions,
                         malformed=malformed, profiles=totals), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError, IndexError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
