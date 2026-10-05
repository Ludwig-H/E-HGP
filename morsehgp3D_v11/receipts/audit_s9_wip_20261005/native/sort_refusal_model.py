"""Modele borne d'une partition GCC 11.4, sans execution du produit natif.

Le groupe strict est initialement ordonne par SiteIdx, comme entry_order.
Des dates distinctes dans (1,2), donc toutes de plancher 1, ordonnent les
SiteIdx 1 < 16 < 8. Les premieres trois comparaisons choisissent 16 comme
pivot. Le refus injecte a la quatrieme comparaison suit exactement le
basculement global date -> SiteIdx du WIP point_tree.cpp:80-90.
Ce modele ne prouve pas un nuage geometrique causant radical_sign_budget.
Il ne modele que la premiere partition introsort, suffisante pour l'OOB.
GCC source : releases/gcc-11.4.0/libstdc++-v3/include/bits/stl_algo.h,
__move_median_to_first et __unguarded_partition, __introsort_loop seuil 16.
"""
import json
from fractions import Fraction

DATE_RANK = [0, 1, 3, 4, 5, 6, 7, 8, 16, 9, 10, 11, 12, 13, 14, 15, 2]

class ReadBeyond(Exception):
    def __init__(self, index):
        self.index = index

class Refused(Exception):
    pass

def one_partition(mode):
    data = list(range(17))
    trace = []
    failed = False
    phase = 'median'
    calls = 0

    def value(index):
        if not 0 <= index < len(data):
            raise ReadBeyond(index)
        return data[index]

    def less(left_index, right_index):
        nonlocal calls, failed
        left, right = value(left_index), value(right_index)
        calls += 1
        if mode != 'no_refusal' and calls == 4:
            failed = True
            if mode == 'immediate_refusal':
                trace.append({'call': calls, 'left': left, 'right': right,
                              'relation': 'Outcome_refused_immediatement'})
                raise Refused()
        result = left < right if failed else (DATE_RANK[left], left) < (DATE_RANK[right], right)
        trace.append({'call': calls, 'left': left, 'right': right,
                      'relation': 'SiteIdx' if failed else 'date_exacte', 'result': result})
        return result

    try:
        # Arbre de decision median-of-three de GCC, avec positions 1,8,16.
        if less(1, 8):
            chosen = 8 if less(8, 16) else (16 if less(1, 16) else 1)
        else:
            chosen = 1 if less(1, 16) else (16 if less(8, 16) else 8)
        data[0], data[chosen] = data[chosen], data[0]
        pivot = data[0]
        phase = 'unguarded_partition'
        left, right = 1, len(data)
        while True:
            while less(left, 0):
                left += 1
            right -= 1
            while less(0, right):
                right -= 1
            if left >= right:
                outcome = {'status': 'partition_complete', 'cut': left}
                break
            data[left], data[right] = data[right], data[left]
            left += 1
    except ReadBeyond as error:
        outcome = {'status': 'read_out_of_range', 'index': error.index,
                   'valid_indices': [0, len(data)-1]}
    except Refused:
        outcome = {'status': 'refusal_propagated_before_read_out_of_range'}
    return {'mode': mode, 'phase': phase, 'calls': calls, 'injected_refusal': failed,
            'pivot_SiteIdx': pivot, 'result': outcome, 'comparisons': trace}

def date_of_rank(rank):
    special = {0: Fraction(21,20), 1: Fraction(11,10), 2: Fraction(3,2), 16: Fraction(19,10)}
    return special[rank] if rank in special else Fraction(3,2) + Fraction(rank-2,50)

dates = [date_of_rank(rank) for rank in DATE_RANK]
levels = [Fraction(0),Fraction(1),Fraction(4)] + sorted((d+1)**2 for d in dates)
blueprint = []
for site,d in enumerate(dates):
    m = (d+1)**2
    if not (1 < d < 2 and m > 4 and levels[1] == 1 and levels[2] == 4):
        raise SystemExit('Domaines rationnels incorrects')
    blueprint.append({'site':site,'date':[d.numerator,d.denominator],
                      't_rank':1,'M_rank':levels.index(m),'Q_rank':2,
                      'floor_rank':1,'strict':1})

results = [one_partition(m) for m in ('no_refusal','fallback_SiteIdx','immediate_refusal')]
expected = ['partition_complete','read_out_of_range','refusal_propagated_before_read_out_of_range']
for result, status in zip(results, expected):
    if result['result']['status'] != status:
        raise SystemExit('Modele divergent : '+result['mode'])
if results[1]['result']['index'] != 17 or results[2]['calls'] != 4:
    raise SystemExit('Borne causale incorrecte')
print(json.dumps({'schema':'ehgp.audit.checked_sort_model.v1','sites':17,
                  'initial_SiteIdx':list(range(17)),'date_rank':DATE_RANK,
                  'rational_blueprint':blueprint,
                  'levels':[[l.numerator,l.denominator] for l in levels],
                  'date_identity':'sqrt(1)+sqrt((date+1)^2)-sqrt(4)=date; toutes 1<date<2',
                  'scope':'Refus injecte du comparateur ; blueprint rationnel, pas catalogue geometrique, nuage u21 ni execution C++',
                  'results':results},sort_keys=True,indent=2))
