"""Oracles scalaires de l'admission census par workerID, sans imports produit."""
from itertools import combinations
import json


def require(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    cases = underestimates = 0
    for workers in range(1, 11):
        for scratch in range(workers + 1):
            for chunks in range(1, workers + 3):
                active = min(workers, chunks)
                # Max des allocations possedees sur tous les ensembles de workers
                # actifs autorises par une reclamation dynamique non ordonnee.
                exact = max(sum(w >= scratch for w in subset)
                            for subset in combinations(range(workers), active))
                old = active - min(active, scratch)
                corrected = min(active, workers - scratch)
                require(exact == corrected, 'majorant ferme exact')
                underestimates += old < exact
                cases += 1
    worker_ids = [30, 31]
    workers, chunks, scratch, sites = 48, 2, 4, 100
    active = min(workers, chunks)
    old = active - min(active, scratch)
    actual = sum(w >= scratch for w in worker_ids)
    corrected = min(active, workers - scratch)
    require(old == 0 and actual == corrected == 2, 'contre-modele')
    print(json.dumps({'cases': cases, 'underestimated_bounds': underestimates,
        'fixture': {'W': workers, 'chunks': chunks, 'scratch_slots': scratch,
                    'claiming_worker_ids': worker_ids, 'old_owned_bound': old,
                    'actual_owned_workers': actual, 'corrected_owned_bound': corrected,
                    'n': sites, 'omitted_worst_case_bytes': 4 * sites * actual},
        'scope': 'routing and admission only; no proof that a particular cloud allocates this maximum',
        'native_executions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
