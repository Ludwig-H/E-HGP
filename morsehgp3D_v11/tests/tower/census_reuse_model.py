"""Voie une passe explicite du juge Fraction ; routage physique sans simulation du moteur natif."""
import copy
import json
import descent_model_test as model
import descent_oracle as oracle


def borrowed_answer(req, bits):
    row = model.answer(req, bits)
    if row['status'] == 'ok':
        row['query_memory']['peak'] = 4*len(row['sites'])
        for step in row['steps']:
            step['ledger']['census']['passes'] = step['ledger']['census_calls']
        row['result']['ledger']['census']['passes'] = row['result']['ledger']['census_calls']
    return row


def saturation_case():
    records = oracle.fixtures.records(tuple((x, 0, 0) for x in range(0, 11, 2)))
    req = dict(name='saturated_k4_six_sites', records=records, order=2, part=[0, 5], kmax=4, budget=1 << 24)
    points, _, balls, _ = oracle.geometry(tuple(records), 4)
    value, center, inner, shell, _ = oracle.meb(points, (0, 5))
    facts = (
        points == tuple((x, 0, 0) for x in range(0, 11, 2)),
        value == oracle.F(25), center == (oracle.F(5), oracle.F(0), oracle.F(0)),
        inner == (1, 2, 3, 4), shell == (0, 5), len(inner)+2 > req['kmax']+1,
        not any(b.center == center and b.level == value for b in balls),
        oracle.transitions(points, (0, 5))[0] == (1, 2), oracle.meb(points, (1, 2))[0] == oracle.F(1),
        not oracle.transitions(points, (1, 2)),
        any(b.center == (oracle.F(3), oracle.F(0), oracle.F(0)) and b.level == 1 for b in balls),
    )
    oracle.require(all(facts), 'témoin saturé analytique hors catalogue K4')
    return req, len(facts)


def main():
    positives = checks = corruptions = routing = saturation_facts = 0
    for bits in (18, 21, 24):
        reqs = oracle.requests(bits)
        saturated, facts = saturation_case(); saturation_facts += facts; reqs.append(saturated)
        for req in reqs:
            row = borrowed_answer(req, bits)
            checks += oracle.judge(row, req, bits, 1)
            positives += 1
        reference = model.answer(saturated, bits); one_pass = borrowed_answer(saturated, bits)
        checks += oracle.judge(reference, saturated, bits, 2); positives += 1
        for row, passes in ((reference, 2), (one_pass, 1)):
            first = row['steps'][0]
            oracle.require(first['next'] == [1, 2] and first['ledger']['interior_steps'] == 1 and
                           first['ledger']['census_calls'] == 1 and first['ledger']['catalogue_hits'] == 0 and
                           first['ledger']['census']['passes'] == passes and len(row['steps']) == 2 and
                           oracle.rational(row['result']['initial_level']) == 25 and
                           oracle.rational(row['result']['terminal_level']) == 1, 'route saturée et dates exactes')
            saturation_facts += 8
        for mutation in (
            lambda r: r['steps'][0]['ledger']['census'].__setitem__('passes', 2),
            lambda r: r['steps'][0]['ledger'].__setitem__('interior_steps', 0),
            lambda r: r['steps'][0].__setitem__('next', [0, 5]),
            lambda r: r['result'].__setitem__('terminal_level', ['4', '1']),
        ):
            bad = copy.deepcopy(one_pass); mutation(bad)
            try:
                oracle.judge(bad, saturated, bits, 1)
            except (ValueError, TypeError, KeyError, IndexError):
                corruptions += 1
            else:
                raise ValueError('corruption du témoin saturé acceptée')
        req = next(r for r in reqs if r['name'] == 'interior_miss')
        row = borrowed_answer(req, bits)
        for mutation in (
            lambda r: r['query_memory'].__setitem__('peak', 8),
            lambda r: r['query_memory'].__setitem__('after', 4),
            lambda r: r['steps'][0]['ledger']['census'].__setitem__('passes', 2),
            lambda r: r['steps'][0]['ledger']['census'].__setitem__('passes', 0),
            lambda r: r['steps'][0]['ledger'].__setitem__('census_calls', 0),
            lambda r: r['result']['ledger']['census'].__setitem__('passes', 2),
            lambda r: r['result'].__setitem__('initial_level', r['result']['terminal_level']),
            lambda r: r['steps'][0].__setitem__('next', [0, 3]),
        ):
            bad = copy.deepcopy(row); mutation(bad)
            try:
                oracle.judge(bad, req, bits, 1)
            except (ValueError, TypeError, KeyError, IndexError):
                corruptions += 1
            else:
                raise ValueError('corruption empruntee acceptee')
        for actual, passes in ((row, 2), (model.answer(req, bits), 1), (row, True)):
            try:
                oracle.judge(actual, req, bits, passes)
            except (ValueError, TypeError, KeyError, IndexError):
                corruptions += 1
            else:
                raise ValueError('confusion des voies census')
    # Une tranche active par worker ; quand J<W, le worker peut avoir un ID>=C.
    # Sinon le slot worker est injectif parmi les appels simultanes. Pas de memo indexe par worker.
    for workers in (1, 2, 4, 48):
        for lanes in (1, 2, 4, 48, 256):
            for capacity in (1, 2, 7, 64, 4096):
                count = min(workers, lanes, capacity)
                for jobs in range(1, min(lanes, capacity)+1):
                    active = min(workers, jobs)
                    for offset in (0, workers-1):
                        slots = [ordinal if jobs < workers else (ordinal+offset) % workers
                                 for ordinal in range(active)]
                        oracle.require(len(set(slots)) == active and all(s < count for s in slots),
                                       'alias ou slot hors enveloppe')
                        routing += 1
    print(json.dumps(dict(verdict='conforme', positives=positives, checks=checks,
                          corruptions=corruptions, routing=routing,
                          saturation_facts=saturation_facts, native=0), sort_keys=True))


if __name__ == '__main__':
    main()
