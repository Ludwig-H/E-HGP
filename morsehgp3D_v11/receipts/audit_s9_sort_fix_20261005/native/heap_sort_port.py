"""Port stdlib borne du heap sort WIP internal.hpp:95-116.

Chaque lecture et ecriture de tableau est controlee. Les Outcome sont
propages au meme endroit que MHGP11_TRY, sans changer la relation.
Aucun C++, build, test natif, budget natif ou publication n'est execute.
"""
from itertools import permutations
from pathlib import Path
import hashlib
import json

SOURCE = 'sources/morsehgp3D_v11/src/points/internal.hpp'
SHA = 'cfb4de6a99309262d37c7819a32b93fc14a0f89d4b9021a88e9770a0e170356e'
ERROR = ('resource_exhausted', 'num', 'radical_sign_budget', 'original_outcome_unchanged')


def port(values, keys, refuse_at=0):
    values = list(values)
    size = len(values)
    calls = []

    def read(index):
        if not 0 <= index < size:
            raise SystemExit('lecture hors borne')
        return values[index]

    def exchange(a, b):
        av, bv = read(a), read(b)
        values[a], values[b] = bv, av

    def less(a, b):
        calls.append((a, b))
        if refuse_at and len(calls) == refuse_at:
            return ERROR, None
        return None, (keys[a], a) < (keys[b], b)

    def sift(root, end):
        while 2 * root + 1 < end:
            child = 2 * root + 1
            if child + 1 < end:
                outcome, smaller = less(read(child), read(child + 1))
                if outcome is not None:
                    return outcome
                if smaller:
                    child += 1
            outcome, smaller = less(read(root), read(child))
            if outcome is not None:
                return outcome
            if not smaller:
                return None
            exchange(root, child)
            root = child
        return None

    for root in reversed(range(size // 2)):
        outcome = sift(root, size)
        if outcome is not None:
            return values, calls, outcome
    for end in range(size, 1, -1):
        exchange(0, end - 1)
        outcome = sift(0, end - 1)
        if outcome is not None:
            return values, calls, outcome
    return values, calls, None


def exercise(values, keys):
    ordered, sequence, outcome = port(values, keys)
    expected = sorted(values, key=lambda x: (keys[x], x))
    if outcome is not None or ordered != expected:
        raise SystemExit('ordre de succes incorrect')
    for k in range(1, len(sequence) + 1):
        partial, seen, got = port(values, keys, k)
        if got is not ERROR or len(seen) != k or seen != sequence[:k]:
            raise SystemExit('refus non immediat ou Outcome modifie')
        if sorted(partial) != sorted(values):
            raise SystemExit('permutation perdue sur refus')
    return len(sequence)


def main():
    source = Path(__file__).parent / SOURCE
    if hashlib.sha256(source.read_bytes()).hexdigest() != SHA:
        raise SystemExit('source capturee modifiee')
    patterns = []
    for n in range(8):
        for p in permutations(range(n)):
            patterns.append((list(p), list(range(n))))
    # Clefs de la porte native : egalites exactes departagees par SiteIdx.
    for n in range(17, 65):
        values = list(range(n))
        keys = [(i * 7919) % 13 for i in values]
        patterns.append((values, keys))
    # Autres ordres, ties, et le temoin17 de la capsule initiale.
    for n in (0,1,2,8,16,17,32,64):
        values = list(range(n))
        for keys in (list(reversed(values)), [0]*n, [(i*7)%5 for i in values]):
            patterns.append((values, keys))
            patterns.append((list(reversed(values)), keys))
    rank17 = [0,1,3,4,5,6,7,8,16,9,10,11,12,13,14,15,2]
    patterns.append((list(range(17)), rank17))
    refusals = sum(exercise(values, keys) for values, keys in patterns)
    witness = []
    for fail_at in (4,9,30):
        partial, calls, outcome = port(range(17), rank17, fail_at)
        witness.append({'fail_at':fail_at,'calls':len(calls),'reason':outcome[2],
                        'permutation_preserved':sorted(partial)==list(range(17))})
    nmax = 0xFFFFFFFE
    if 2*(nmax-1)+1 >= 1<<64:
        raise SystemExit('borne u64 incorrecte')
    print(json.dumps({'schema':'ehgp.audit.heap_sort_port.v1','source':SOURCE,
                      'source_sha256':SHA,'patterns':len(patterns),
                      'success_orders_checked':len(patterns),'first_refusal_positions_checked':refusals,
                      'all_indices_checked':True,'refusal_calls_stop_exactly':True,
                      'outcome_original_preserved':True,'tie_order':'date/key puis SiteIdx',
                      'witness17':witness,'production_n_max':nmax,
                      'production_child_expression_max':2*(nmax-1)+1,
                      'scope':'port Python et borne de lecture, aucune qualification native/ASan/API/CLI'},
                     sort_keys=True,indent=2))

if __name__ == '__main__':
    main()
