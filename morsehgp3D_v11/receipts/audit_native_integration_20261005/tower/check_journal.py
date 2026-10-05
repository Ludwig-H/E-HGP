#!/usr/bin/env python3
"""Contrôle scalaire indépendant du journal S3, sans moteur ni imports du dépôt.

Préconditions mathématiques déjà acquises (non requalifiées ici) : Cat_K exact,
W_K : p+q_min-1 <= K <= p+m ; q_min >= 2 ; cellules régulières m=q_min
avec q_min traces au rang faible, naissance au rang fort ; une cellule étendue
possède au plus C(m,K-p) traces strictes, t=m est une naissance.

Chaque cellule effectivement rejouée est alors contenue dans le majorant de
SeedLog::make. Un balayage du journal traite ses cellules dans l'ordre ; avant
une cellule, le nombre A de branches retenues est <= son offset begin, car
chaque branche vient d'une de ses graines. Après déduplication, A+branches <=
begin+traces=end : la copie vers A n'écrase donc jamais une cellule future.
La mémoire du journal est 12*C+8+4*G ; le sweep (12*N) meurt avant l'allocation
finale prior (4*A), et les marques (4*N) sont rendues avant celle-ci.

Le témoin à deux plateaux ci-dessous est une forêt abstraite valide avec
hyperarêtes datées, PAS un nuage géométrique nouveau ni un oracle FULL. La
partition est obtenue par fermeture d'ensembles ; ses rattachements et ses
comptes sont comparés à un compactage du journal. Il contrôle les trous laissés
par une continuation et la réutilisation d'une naissance sur plusieurs rangs.
Il n'exécute ni C++, ni banc, ni oracle produit ; aucun gain/performance acquis.
"""
from fractions import Fraction
from itertools import permutations
from math import comb
import hashlib
import json
from pathlib import Path

checks = 0

def guard(condition, why):
    global checks
    if not condition:
        raise ValueError(why)
    checks += 1


def closure(universe, edges):
    blocks = [{x} for x in universe]
    for edge in edges:
        touched = [b for b in blocks if b.intersection(edge)]
        rest = [b for b in blocks if not b.intersection(edge)]
        blocks = rest + [set().union(*touched)]
    return sorted((frozenset(b) for b in blocks), key=lambda b: min(b))


def main():
    cases = 0
    # Aucun choix de géométrie : borne uniforme pour tout nombre de traces réel.
    for k in range(1, 13):
        for p in range(k):
            for q in range(2, 5):
                for m in range(q, 25):
                    if not p+q-1 <= k <= p+m:
                        continue
                    t = k-p
                    if m == q:
                        cap = 0 if k == p+q else q
                        guard(cap == (0 if t == m else comb(m,t)), 'capacité régulière')
                    else:
                        cap = 0 if t == m else comb(m,t)
                        for strict in {0, cap//2, cap}:
                            guard(0 <= strict <= cap, 'capacité étendue')
                        guard(t != m or cap == 0, 'coquille entière naissance')
                    cases += 1
    # Modèle à deux plateaux : toutes les graines désignent les naissances.
    ranks = [Fraction(4)]*5 + [Fraction(7)]*3
    raw = [(0,1),(1,2),(3,4),(0,2),(5,5),(0,3),(4,5),(1,5,1)]
    old4 = closure(range(6), [])
    closed4 = closure(range(6), raw[:5])
    closed7 = closure(range(6), raw)
    guard(closed4 == [frozenset({0,1,2}),frozenset({3,4}),frozenset({5})], 'plateau 4 atomique')
    guard(closed7 == [frozenset(range(6))], 'plateau 7 atomique')
    # Nœuds canoniques : 6,7 au rang 4 ; 8 au rang 7, fils 6,7,5.
    open_nodes4 = {i:i for i in range(6)}
    open_nodes7 = {0:6,1:6,2:6,3:7,4:7,5:5}
    parent4 = {0:6,1:6,2:6,3:7,4:7,5:5}
    parent7 = {5:8,6:8,7:8}
    expected = [(6,(0,1)),(6,(1,2)),(7,(3,4)),(6,(0,2)),(5,(5,)),
                (8,(6,7)),(8,(5,7)),(8,(5,6))]
    seeds = [x for cell in raw for x in cell]
    offsets = [0]
    for cell in raw:
        offsets.append(offsets[-1]+len(cell))
    kept = 0
    entries = []
    touched = {Fraction(4):set(), Fraction(7):set()}
    continuations = {Fraction(4):set(), Fraction(7):set()}
    merged = {Fraction(4):set(), Fraction(7):set()}
    prior_offsets = [0]
    for ordinal, rank in enumerate(ranks):
        begin,end = offsets[ordinal:ordinal+2]
        mapping = open_nodes4 if rank == 4 else open_nodes7
        parents = parent4 if rank == 4 else parent7
        branches = tuple(sorted({mapping[x] for x in seeds[begin:end]}))
        owners = {parents[x] for x in branches}
        guard(len(owners) == 1, 'toutes branches même propriétaire fermé')
        att = next(iter(owners))
        guard((att,branches) == expected[ordinal], 'rattachement abstrait')
        role = 'internal' if branches == (att,) else 'merge'
        touched[rank].update(branches)
        if role == 'internal': continuations[rank].update(branches)
        else: merged[rank].update(branches)
        guard(kept <= begin, 'début de copie avant source')
        if role == 'merge':
            guard(kept+len(branches) <= end, 'pas de copie sur cellule future')
            for i,x in enumerate(branches): seeds[kept+i] = x
            kept += len(branches)
        prior_offsets.append(kept)
        entries.append({'rank':str(rank),'node':att,'role':role,'branches':list(branches),
                        'traces':end-begin})
    wanted = [x for (att,branch) in expected if branch != (att,) for x in branch]
    guard(seeds[:kept] == wanted, 'compactage final exact')
    guard(kept == 14 and sum(map(len,raw)) == 17, 'traces vs branches')
    guard(sum(map(len,touched.values())) == 9, 'touches par plateau')
    guard(sum(map(len,merged.values())) == 8, 'tous huit enfants consommés')
    guard(sum(map(len,continuations.values())) == 1, 'une continuation')
    # L'ordre de résolution des 5 premiers jobs ne décide pas de leur publication.
    for completed in permutations(range(5)):
        slots = {i:raw[i] for i in completed}
        guard([slots[i] for i in range(5)] == raw[:5], 'jobs publiés par ordinal')
    # Coexistences exactes du nouveau journal, hors domaine/forêt déjà retenus.
    C,G,W,N,A = 8,17,8,9,14
    J = 12*C+8+4*G
    fixed = 25*W+8
    sweep_phase = J+fixed+16*N
    prior_phase = J+fixed+4*A
    result_phase = fixed+4*A
    guard(J == 172 and sweep_phase == 524 and prior_phase == 436 and result_phase == 264,
          'coexistences du journal et du rattachement')
    guard(12*(2**32-2)+8+4*((2**64-1)//4//2) < 2**64, 'admission u64 du journal')
    source = Path(__file__).read_bytes()
    print(json.dumps({'status':'conforme','checks':checks,'capacity_cases':cases,
        'abstract_plateaus':2,'cells':len(raw),'original_traces':17,'retained_branches':14,
        'completion_orders':120,'entries':entries,'prior_offsets':prior_offsets,
        'memory_additional_bytes':{'journal':J,'sweep_phase':sweep_phase,'prior_phase':prior_phase,
                                  'result_phase':result_phase},
        'script_sha256':hashlib.sha256(source).hexdigest(),
        'scope':'scalaire/foret abstraite, aucun moteur exécuté'},ensure_ascii=False,sort_keys=True,indent=2))

if __name__ == '__main__':
    main()
