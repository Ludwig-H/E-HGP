#!/usr/bin/env python3
"""G-L5 : modele abstrait exact, sans geometrie ni chrono ; Python standard.

Une portee = (proprietaire immuable avec generation, ordre). Les lignes de naissance
sont (population triee, cible, rang), les requetes (population, representant, rang
de jonction). Le modele de reference scanne les naissances ; la proposition trie
les mots complets par radix et fusionne. Aucun hash ne certifie une egalite.
"""
import copy
import hashlib
import json
import random
from pathlib import Path


class Refusal(Exception):
    pass


def need(value, message):
    if not value:
        raise RuntimeError(message)


def valid_population(population, k):
    return (len(population) == k and all(type(x) is int and 0 <= x < 2**32 for x in population)
            and all(a < b for a, b in zip(population, population[1:])))


def prepare(births, queries, scopes):
    for scope in scopes:
        if len(scope) != 3 or not 2 <= scope[2] <= 12:
            raise Refusal('scope')
    seen_queries = set()
    for scope, population, representative, rank in queries:
        if scope not in scopes or not valid_population(population, scope[2]):
            raise Refusal('query population or owner')
        key = scope, representative
        if key in seen_queries:
            raise Refusal('duplicate representative slot')
        seen_queries.add(key)
        if type(representative) is not int or representative < 0 or type(rank) is not int or rank < 0:
            raise Refusal('query payload')
    for scope, population, target, rank in births:
        if scope not in scopes or not valid_population(population, scope[2]):
            raise Refusal('birth population or owner')
        if type(target) is not int or target < 0 or type(rank) is not int or rank < 0:
            raise Refusal('birth payload')


def state(scope, query, birth):
    population, representative, before = query
    if birth is None:
        # Le premier probe a deja ete consomme ; reprendre juste avant locate,
        # avec chain=1, Previous.rank=before et F inchanges.
        return (scope, representative, 'miss', None, 1, 0, 0, 1, before, population)
    _population, target, rank = birth
    if rank >= before:
        raise Refusal('nondecreasing birth rank')
    # Un probe, un first_probe_hit, un controle, chain_histogram[0] += 1.
    return (scope, representative, 'hit', target, 1, 1, 1, 0, before, population)


def reference(births, queries, scopes):
    prepare(births, queries, scopes)
    seen = set()
    for scope, population, _target, _rank in births:
        if (scope, population) in seen:
            raise Refusal('duplicate complete birth population')
        seen.add((scope, population))
    result = []
    for scope, population, representative, rank in queries:
        found = [row[1:] for row in births if row[0] == scope and row[1] == population]
        result.append(state(scope, (population, representative, rank), found[0] if found else None))
    return sorted(result)


def radix(rows, fingerprint, work):
    if not rows:
        return []
    # Optional hash precede la population exacte ; il peut etre constant.
    # Les clefs sont calculees depuis F, jamais fournies par le producteur.
    records = []
    for row in rows:
        h = fingerprint(row[0])
        words = row[0] if h is None else (h >> 32, h & 0xffffffff) + row[0]
        records.append((words, row))
    # Tri LSD stable : mot de poids faible d'abord, octets petit-boutistes par mot.
    for word in reversed(range(len(records[0][0]))):
        for shift in (0, 8, 16, 24):
            counts = [0] * 256
            for key, _row in records:
                counts[(key[word] >> shift) & 255] += 1
            total = 0
            for digit, count in enumerate(counts):
                counts[digit], total = total, total + count
            dest = [None] * len(records)
            for item in records:
                digit = (item[0][word] >> shift) & 255
                dest[counts[digit]] = item
                counts[digit] += 1
            records = dest
            work['radix_record_visits'] += 2 * len(records)
            work['radix_bucket_visits'] += 256
    return records


def proposed(births, queries, scopes, fingerprint):
    prepare(births, queries, scopes)
    work = dict(radix_record_visits=0, radix_bucket_visits=0, join_advances=0, query_rows=0)
    result = []
    # En produit, la portee est celle de l'appel immuable, pas une cle hachee.
    for scope in sorted(scopes):
        table = radix([row[1:] for row in births if row[0] == scope], fingerprint, work)
        requests = radix([row[1:] for row in queries if row[0] == scope], fingerprint, work)
        for left, right in zip(table, table[1:]):
            if left[0] == right[0]:
                raise Refusal('duplicate complete birth population')
        at = 0
        for key, query in requests:
            while at < len(table) and table[at][0] < key:
                at += 1
                work['join_advances'] += 1
            birth = table[at][1] if at < len(table) and table[at][0] == key else None
            result.append(state(scope, query, birth))
            work['query_rows'] += 1
    # Le curseur de table n'avance jamais en arriere ; aucune paire B x Q visitee.
    need(work['join_advances'] <= len(births), 'join cursor bound')
    need(work['query_rows'] == len(queries), 'representative loss')
    return sorted(result), work


def verdict(call):
    try:
        result = call()
        return 'ok', result
    except Refusal as error:
        return 'refused', str(error)


def main():
    rng = random.Random(7102026)
    fingerprints = [('no_hash', lambda _p: None), ('constant', lambda _p: 0),
                    ('sum_mod_u64', lambda p: sum(p) % 2**64)]
    runs, query_rows, max_births, max_queries = 0, 0, 0, 0
    for case in range(20):
        k = (2, 3, 5, 10, 12)[case % 5]
        # Meme numerotation de sites dans deux proprietaires et deux generations.
        scopes = {(1, 7, k), (1, 8, k), (2, 7, k)}
        births, queries = [], []
        for scope in sorted(scopes):
            populations = set()
            while len(populations) < 6:
                populations.add(tuple(sorted(rng.sample(range(32), k))))
            populations = sorted(populations)
            for target, population in enumerate(populations):
                births.append((scope, population, 100 * scope[0] + 10 * scope[1] + target, target + 1))
            for representative in range(14):
                population = (populations[representative % 6] if representative < 10 else
                              tuple(sorted(rng.sample(range(32), k))))
                queries.append((scope, population, representative, 20 + representative))
        wanted = reference(births, queries, scopes)
        for name, fingerprint in fingerprints:
            for schedule in range(3):
                b, q = list(births), list(queries)
                if schedule == 1:
                    b.reverse(); q.reverse()
                elif schedule == 2:
                    rng.shuffle(b); rng.shuffle(q)
                got, work = proposed(b, q, scopes, fingerprint)
                need(got == wanted, ('differential', case, name, schedule))
                runs += 1
                query_rows += work['query_rows']
                max_births, max_queries = max(max_births, len(b)), max(max_queries, len(q))

    scope = (1, 7, 2)
    b = [(scope, (0, 3), 7, 1)]
    q = [(scope, (0, 3), 0, 2), (scope, (0, 3), 1, 5), (scope, (1, 2), 2, 4)]
    valid, valid_work = proposed(b, q, {scope}, lambda p: sum(p))
    # Contre-exemple au hash pris pour certificat : meme k et meme somme, F distinctes.
    collision = dict(birth_population=[0, 3], query_population=[1, 2], sum=3,
                     wrong_hash_only_target=7, exact_result=valid[-1][2])
    need(collision['exact_result'] == 'miss', collision)
    scoped_births = [(scope, (0, 3), 7, 1), ((1, 8, 2), (0, 3), 11, 1),
                     ((2, 7, 2), (0, 3), 13, 1)]
    scoped_queries = [(row[0], (0, 3), 0, 2) for row in scoped_births]
    scoped_results, _ = proposed(scoped_births, scoped_queries, {row[0] for row in scoped_births}, lambda _p: 0)
    need([row[3] for row in scoped_results] == [7, 11, 13], 'owner/generation alias')
    # La meme partie ne dispense jamais du controle de niveau de chaque origine.
    bad_rank = copy.deepcopy(q)
    bad_rank[1] = (scope, (0, 3), 1, 1)
    refused_cases = {
        'equal_rank_for_one_duplicate_query': (b, bad_rank, {scope}),
        'duplicate_birth_same_payload': (b + b, q, {scope}),
        'duplicate_birth_other_target': (b + [(scope, (0, 3), 9, 1)], q, {scope}),
        'duplicate_representative_slot': (b, q + [q[0]], {scope}),
        'owner_absent': (b, [((3, 7, 2), (0, 3), 0, 2)], {scope}),
        'population_not_strict': ([(scope, (0, 0), 7, 1)], q, {scope}),
        'wrong_order_length': (b, [(scope, (0, 3, 5), 0, 2)], {scope}),
        'k1_keeps_special_path': ([], [], {(1, 7, 1)}),
    }
    refusals = {}
    for name, (bb, qq, scopes) in refused_cases.items():
        expected = verdict(lambda: reference(bb, qq, scopes))
        got = verdict(lambda: proposed(bb, qq, scopes, lambda _p: 0))
        need(got == expected and got[0] == 'refused', (name, expected, got))
        refusals[name] = got[1]
    result = dict(schema='mhgp12.audit.g_l5_model.v1', model_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  abstract_differential_runs=runs, query_rows_checked=query_rows, max_births=max_births,
                  max_queries=max_queries, hash_modes=[x[0] for x in fingerprints],
                  schedules='original, inverse et permutation; aucune execution concurrente native',
                  forced_collision_witness=collision, valid_duplicate_query_states=valid,
                  owner_generation_distinct_targets=[row[3] for row in scoped_results],
                  linear_join_work=valid_work, refusals=refusals,
                  geometry_computed=False, native_execution=False, elapsed_times_published=False)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
