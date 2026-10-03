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


def main():
    positives = checks = corruptions = routing = 0
    for bits in (18, 21, 24):
        reqs = oracle.requests(bits)
        for req in reqs:
            row = borrowed_answer(req, bits)
            checks += oracle.judge(row, req, bits, 1)
            positives += 1
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
                          corruptions=corruptions, routing=routing, native=0), sort_keys=True))


if __name__ == '__main__':
    main()
