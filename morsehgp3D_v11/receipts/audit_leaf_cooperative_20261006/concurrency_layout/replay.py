"""Bornes combinatoires et layout conditionnel, aucun sizeof ni CUDA execute."""
import argparse
import itertools
import json
from math import comb
from pathlib import Path


def check(v, m):
    if not v:
        raise ValueError(m)


def layout(fields, alignment):
    at = 0
    for size, align in fields:
        at = ((at + align - 1) // align) * align + size
    return ((at + alignment - 1) // alignment) * alignment


def proof():
    ranks = [k*(k-1)*(k-2)//6 + j*(j-1)//2 + i for i,j,k in itertools.combinations(range(32),3)]
    check(sorted(ranks) == list(range(4960)), 'rangs J2 denses')
    check(max(ranks)>>5 == 154 and (max(ranks)&31) == 31, 'dernier bit J2')
    requests = [0,1,0,31,32,31]
    outcomes = set()
    for order in set(itertools.permutations(requests)):
        seen = set(); hits = evaluations = 0
        for rank in order:
            if rank in seen: hits += 1
            else: evaluations += 1
            seen.add(rank)
        outcomes.add((hits,evaluations))
    check(outcomes == {(2,4)}, 'atomic test-set ideal, ordre independant')
    table = layout([(4,4),(32*3*4,4),(3*32*8,8),(3*32*8,8),(155*4,4)],8)
    pair = layout([(4,4),(4,4),(8,8),(8,8)],8)
    counters = layout([(8,8),(8,8)],8)
    prefix = sum(comb(32,q) for q in range(1,5)); bound=32*3*prefix
    check((table,pair,counters,prefix,bound) == (2552,24,16,41448,3979008),'calculs de layout/bornes')
    check(bound < 1<<22 < 1<<32, 'comptes/préfixes de feuille compactables en u32')
    return dict(kind='preuve stdlib combinatoire/ABI conditionnelle ; native0 cuda0',
                abi_assumptions='u32=4/align4,u64=8/align8,align struct8 ; aucun sizeof execute',
                pair_bound=comb(32,2), triple_bound=len(ranks), seen_words32=155,
                rank_max=max(ranks),cache_atomic_model=dict(requests=len(requests), distinct=4,hits=2,evaluations=4),
                current_wip_layout=dict(tables=table,pair_task=pair,pair_tasks=pair*496,
                                        counts_per_pair=counters*496,total_before_metadata=table+pair*496+counters*496),
                compact_design_example=dict(tables=table,pairs_u32=496*4,
                                           counts_or_prefixes_two_u32=496*8, masks_per_site_two_u64=32*16,
                                           total_before_metadata=table+496*4+496*8+32*16),
                prefix_bound=prefix, count_bound=bound,
                preparation_call_counts=[dict(m=m, reference_pair_relation=m*(m-1)//2, wip_fill_row_pair_relation=m*(m-1), reported_dominance_tests=m*(m-1)//2) for m in (1,2,31,32)],
                scope='nombres/rangs et tailles sous hypotheses seulement ; synchronization, allocation shared et exactitude native non qualifiees')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',type=Path);a=p.parse_args()
    result=proof()
    if a.check:
        check(json.loads(a.check.read_text())==result,'JSON fige different')
        print('coop Q3/Q4 borne conforme :496 paires,4960 rangs,155 mots32 ; layout conditionnel22392/9016 ; native0 cuda0')
    else:print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
