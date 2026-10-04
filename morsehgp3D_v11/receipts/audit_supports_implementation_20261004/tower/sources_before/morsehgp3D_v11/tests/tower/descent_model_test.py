"""Sensibilite du juge de descente et faits Gamma ferme, sans executable natif."""
import copy
import itertools as it
import json
import sys

import descent_oracle as oracle


def blank_ledger():
    result = dict.fromkeys(oracle.COUNTS, 0)
    result.update(part_meb=dict.fromkeys(oracle.MEB, 0), trace_meb=dict.fromkeys(oracle.MEB, 0),
                  census=dict.fromkeys(oracle.CENSUS, 0), memo=dict.fromkeys(oracle.MEMO, 0))
    return result


def add_ledger(a, b):
    return {key: add_ledger(value, b[key]) if isinstance(value, dict) else value+b[key]
            for key, value in a.items()}


def answer(req, bits, last=False):
    reason = oracle.refusal(req, bits)
    row = dict(status='ok', reason='none', coord_bits=bits, kmax=req['kmax'], order=req['order'],
               query_memory=dict(after=0, peak=0), owner_after=0, sites=[], site_ids=[], balls=None,
               steps=[], result=None)
    if reason:
        row.update(reason=reason, status='resource_exhausted' if reason == 'memory_budget' else
                   'unsupported_degeneracy' if reason == 'multiplicity_unsupported' else 'invalid_input')
        return row
    points, identifiers, balls, metadata = oracle.geometry(tuple(req['records']), req['kmax'])
    row.update(sites=[list(p) for p in points], site_ids=[list(ids) for ids in identifiers], balls=copy.deepcopy(metadata))
    current = tuple(sorted(req['part'])); k = req['order']; sum_work = blank_ledger()
    initial = oracle.meb(points, current)[0]
    while True:
        value, center, inner, shell, _ = oracle.meb(points, current)
        choices = oracle.transitions(points, current)
        nxt = choices[-1 if last else 0] if choices else ()
        work = blank_ledger(); work['steps'] = 1
        # Temoin de comptage minimal admissible : aucune revendication de compteurs natifs.
        work['part_meb'].update(presentations=1, nondegenerate=1, positive=1, containing=1, point_tests=k,
                                diameter_pairs=oracle.math.comb(k, 2))
        if k == 1:
            work['singleton_hits'] = 1
        elif any(b.center == center and b.level == value for b in balls):
            work['catalogue_hits'] = 1
        else:
            work['census_calls'] = 1
            work['census'].update(nodes=1, bounds=1, point_tests=len(points), passes=2)
            row['query_memory']['peak'] = max(row['query_memory']['peak'], 4*(k if len(inner) >= k else len(inner)+len(shell)))
        if nxt:
            work['interior_steps'] = int(len(inner) >= k)
            work['trace_steps'] = int(len(inner) < k)
            work['candidate_traces'] = work['trace_steps']
        seed = None if nxt else oracle.seed_for(points, balls, current, k)
        row['steps'].append(dict(part=list(current), level=oracle.encoded(value), next=list(nxt), seed=seed, ledger=work))
        sum_work = add_ledger(sum_work, work)
        if not nxt:
            row['result'] = dict(initial_level=oracle.encoded(initial), terminal_level=oracle.encoded(value),
                                 seed=seed, ledger=sum_work)
            return row
        current = tuple(nxt)


def main():
    checks, corruptions, facts, positives, totals = 0, 0, 0, 0, []
    for bits in (18,21,24):
        reqs = oracle.requests(bits)
        rows = [answer(req, bits) for req in reqs]
        checks += sum(oracle.judge(row, req, bits) for row, req in zip(rows, reqs))
        positives += len(rows)
        def find(name):
            i = next(i for i, req in enumerate(reqs) if req['name'] == name)
            return reqs[i], rows[i]
        req, row = find('closed_line')
        other = answer(req, bits, last=True)
        checks += oracle.judge(other, req, bits); positives += 1
        oracle.require(row['result']['seed'] != other['result']['seed'], 'deux terminaux valides')
        points = oracle.geometry(tuple(req['records']), req['kmax'])[0]
        closed = oracle.components(points, 2, oracle.F(4))
        strict = oracle.components(points, 2, oracle.F(4), closed=False)
        oracle.require(closed[(0,1)] == closed[(1,2)], 'connexion a egalite')
        oracle.require(strict[(0,1)] != strict[(1,2)] and (0,2) not in strict, 'graphe strict distinct')
        facts += 3
        global_req, global_row = find('local_q4_global_q2')
        cloud, _, global_balls, _ = oracle.geometry(tuple(global_req['records']), global_req['kmax'])
        part = tuple(sorted(global_req['part'])); initial_sphere = oracle.meb(cloud, part)
        local_strict = []
        for q in range(1,5):
            for support in it.combinations(part,q):
                sphere = oracle.model.circumsphere([cloud[i] for i in support])
                if sphere and sphere[:2] == (initial_sphere[1],initial_sphere[0]) and all(w > 0 for w in sphere[2]):
                    local_strict.append(q)
        oracle.require(min(local_strict) == 4 and any(b.qmin == 2 and b.center == initial_sphere[1]
                       and b.level == initial_sphere[0] for b in global_balls), 'support local q4 et global q2')
        shell_req, shell_row = find('shell14_k12')
        shell_birth = shell_row['balls'][shell_row['result']['seed']['ball']]
        oracle.require(len(shell_birth['shell']) == 14 and shell_row['result']['seed']['order'] == 12,
                       'coquille plus large que plafond des parties')
        facts += 2
        _, chain = find('three_steps')
        oracle.require(len(chain['steps']) == 3, 'chaine de trois pas')
        facts += 1
        # Contre-modele direct : enumerer chaque hyperarete (k+1)-part plutot que les populations.
        for name in ('closed_line', 'extended_birth', 'trace_outside_catalogue', 'regular_tetra_k4'):
            r, a = find(name); cloud = oracle.geometry(tuple(r['records']), r['kmax'])[0]
            threshold = oracle.rational(a['result']['initial_level'])
            for k in range(1, min(4, len(cloud))+1):
                for is_closed in (False, True):
                    oracle.require(oracle.components(cloud,k,threshold,is_closed) ==
                                   oracle.components(cloud,k,threshold,is_closed,direct=True), 'Gamma deux constructions')
                    facts += 1
        for r, a in zip(reqs, rows):
            if a['status'] != 'ok':
                continue
            cloud = oracle.geometry(tuple(r['records']), r['kmax'])[0]
            terminal = tuple(a['steps'][-1]['part']); sphere = oracle.meb(cloud, terminal)
            oracle.require(all(oracle.meb(cloud, p)[0] == sphere[0]
                               for p in it.combinations(sorted(sphere[4]), r['order'])), 'naissance universelle')
            facts += 1
        interior_req, interior = find('interior_miss')
        oracle.require(interior['steps'][0]['ledger']['interior_steps'] == 1 and
                       interior['steps'][0]['ledger']['census_calls'] == 1, 'interieur hors CatK')
        trace_req, trace = find('trace_outside_catalogue')
        oracle.require(trace['steps'][0]['ledger']['trace_steps'] == 1 and
                       trace['steps'][0]['ledger']['census_calls'] == 1, 'trace hors fenetre CatK')
        birth_req, birth = find('extended_birth')
        ball = birth['balls'][birth['result']['seed']['ball']]
        oracle.require(len(ball['shell']) == 4 and len(ball['inner']) == 1 and ball['qmin'] == 2,
                       'naissance etendue et non simplexe regulier')
        facts += 3
        def corrupt(request, good, mutation):
            nonlocal corruptions
            bad = copy.deepcopy(good); mutation(bad)
            try:
                oracle.judge(bad, request, bits)
            except (ValueError, TypeError, KeyError, IndexError):
                corruptions += 1
                return
            raise ValueError('corruption acceptee')
        one_req, one = find('site_among_three')
        cloud = oracle.geometry(tuple(one_req['records']), one_req['kmax'])[0]
        chosen = tuple(one_req['part'])
        single = oracle.meb(cloud, chosen)
        oracle.require(single[:4] == (oracle.F(0), tuple(map(oracle.F, cloud[chosen[0]])), (), chosen),
                       'singleton : centre, rayon et population calcules par Fraction')
        oracle.require(len(one['steps']) == 1 and one['result']['seed'] == dict(site=chosen[0], ball=None, order=1),
                       'singleton non premier : terminal propre')
        oracle.require(one['query_memory'] == dict(after=0, peak=0) and
                       one['result']['ledger']['part_meb']['containing'] == 1, 'MEB payee sans census')
        empty_budget = copy.deepcopy(one_req); empty_budget['budget'] = 0
        checks += oracle.judge(one, empty_budget, bits); positives += 1
        facts += 4
        def work_change(value, **fields):
            for work in (value['steps'][0]['ledger'], value['result']['ledger']):
                work.update(fields)
        for mutation in (
            lambda v: work_change(v, singleton_hits=True),
            lambda v: work_change(v, singleton_hits=0, catalogue_hits=1),
            lambda v: work_change(v, singleton_hits=0, census_calls=1),
            lambda v: work_change(v, candidate_traces=1),
            lambda v: work_change(v, census=dict.fromkeys(oracle.CENSUS, 1)),
            lambda v: work_change(v, part_meb=dict.fromkeys(oracle.MEB, 0)),
            lambda v: v['query_memory'].__setitem__('peak', 4),
            lambda v: [work.pop('singleton_hits') for work in (v['steps'][0]['ledger'], v['result']['ledger'])],
            lambda v: [seed.__setitem__('site', 0) for seed in (v['steps'][0]['seed'], v['result']['seed'])],
        ):
            corrupt(one_req, one, mutation)
        pair_req, pair = find('pair_birth')
        corrupt(pair_req, pair, lambda v: work_change(v, singleton_hits=1, catalogue_hits=0, census_calls=0))
        for mutate in (
            lambda v: v.__setitem__('status', 'resource_exhausted'),
            lambda v: v.__setitem__('reason', 'memory_budget'),
            lambda v: v.__setitem__('coord_bits', 17),
            lambda v: v.__setitem__('owner_after', False),
            lambda v: v['query_memory'].__setitem__('after', 1),
            lambda v: v['steps'][0].__setitem__('level', ['0','1']),
            lambda v: v['steps'][0].__setitem__('level', ['4','0']),
            lambda v: v['steps'][0].__setitem__('next', [0,2]),
            lambda v: v['steps'][0].__setitem__('next', [1,1]),
            lambda v: v['steps'][0].__setitem__('next', [1,0]),
            lambda v: v['steps'][1].__setitem__('part', [1,2]),
            lambda v: v['steps'][0].__setitem__('seed', v['steps'][-1]['seed']),
            lambda v: v['steps'][-1]['seed'].__setitem__('ball', None),
            lambda v: v['steps'][-1]['seed'].__setitem__('site', 0),
            lambda v: v['steps'][-1]['seed'].__setitem__('order', 1),
            lambda v: v['result'].__setitem__('initial_level', v['result']['terminal_level']),
            lambda v: v['result'].__setitem__('terminal_level', ['0','1']),
            lambda v: v['result']['ledger'].__setitem__('steps', 1),
            lambda v: v['steps'][0]['ledger'].__setitem__('steps', True),
            lambda v: v['steps'][0]['ledger'].__setitem__('trace_steps', 0),
            lambda v: v['steps'][0]['ledger']['part_meb'].__setitem__('containing', 0),
            lambda v: v['steps'][0]['ledger']['part_meb'].__setitem__('diameter_pairs', 0),
            lambda v: v['steps'][0]['ledger']['trace_meb'].__setitem__('diameter_pairs', 1),
            lambda v: v['steps'][0]['ledger']['census'].__setitem__('passes', 1),
            lambda v: v['steps'].clear(),
            lambda v: v['balls'][0].__setitem__('qmin', 4),
            lambda v: v['balls'][0]['shell'].pop(),
            lambda v: v['site_ids'][0].__setitem__(0, 7),
            lambda v: v['sites'][0].__setitem__(0, 9),
            lambda v: v.__setitem__('unexpected', 0),
        ):
            corrupt(req, row, mutate)
        corrupt(interior_req, interior, lambda v: v['query_memory'].__setitem__('peak', 0))
        corrupt(trace_req, trace, lambda v: v['steps'][0]['ledger'].__setitem__('candidate_traces', 0))
        no_req, no = find('no_query_memory')
        corrupt(no_req, no, lambda v: v.__setitem__('steps', [copy.deepcopy(row['steps'][0])]))
        corrupt(no_req, no, lambda v: v.__setitem__('balls', []))
        # Pretendre que le premier MEB est deja une naissance doit etre refuse meme avec ledgers coherents.
        fake = copy.deepcopy(row); fake['steps'] = fake['steps'][:1]
        fake['steps'][0].update(next=[], seed=dict(site=None, ball=2, order=2))
        fake['steps'][0]['ledger'].update(trace_steps=0, interior_steps=0)
        fake['result'].update(terminal_level=['4','1'], seed=fake['steps'][0]['seed'], ledger=fake['steps'][0]['ledger'])
        corrupt(req, fake, lambda _: None)
        totals.append(dict(bits=bits, requests=len(reqs), steps=sum(len(a['steps']) for a in rows),
                           refusals=sum(a['status'] != 'ok' for a in rows)))
    malformed = 0
    for line in ('{', '{"x":1,"x":2}', '{"x":NaN}'):
        try:
            oracle.parse(line)
        except ValueError:
            malformed += 1
    oracle.require(malformed == 3 and corruptions == 135 and facts >= 212 and checks >= 12000, 'planchers')
    print(json.dumps(dict(verdict='conforme', native=0, checks=checks, positives=positives, facts=facts,
                         corruptions=corruptions, malformed=malformed, profiles=totals), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError, IndexError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        raise SystemExit(1)
