#!/usr/bin/env python3
"""Modele borne de l'empreinte additive ; aucun moteur, chrono ou nuage reel."""
import itertools
import json
import random

U64 = (1 << 64) - 1
MASKS = (0, 3, U64, 0xAAAAAAAAAAAAAAAA, 1 << 63)


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def site_key(site):
    z = (site + 0x9E3779B97F4A7C15) & U64
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & U64
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & U64
    return z ^ (z >> 31)


def direct(inner, selected, mask):
    # Population complete triee, comme key_of ; arithmetique non bornee
    # puis reduction, independamment du groupement par cellule.
    return (sum(site_key(x) for x in sorted(inner + selected)) & U64) & mask


def shared(inner, shell, bits, mask):
    base = 0
    for x in inner:
        base = (base + site_key(x)) & U64
    keys = [site_key(x) for x in shell]
    result = base
    while bits:
        low = bits & -bits
        result = (result + keys[low.bit_length() - 1]) & U64
        bits ^= low
    return result & mask


def main():
    rng = random.Random(20261007)
    traces = wraps = high_bit = 0
    for case in range(1000):
        p = case % 12
        t = 1 + case % (12 - p)
        m = max(t, 64 if case % 7 == 0 else 1 + case % 64)
        ids = rng.sample(range((1 << 32) - 1), p + m)
        inner, shell = sorted(ids[:p]), sorted(ids[p:])
        chosen = sorted(rng.sample(range(m), t))
        if m == 64:
            chosen = sorted(set(chosen[:-1] + [63]))
        bits = sum(1 << j for j in chosen)
        selected = [shell[j] for j in chosen]
        wraps += sum(site_key(x) for x in inner + selected) > U64
        high_bit += bool(bits & (1 << 63))
        for mask in MASKS:
            need(direct(inner, selected, mask) == shared(inner, shell, bits, mask),
                 'groupement non equivalent')
        traces += 1
    # Collision forcee : une population egale est retrouvee par sa cle complete,
    # toutes les autres restent absentes meme si toutes les empreintes valent 0.
    rows = [tuple(x) for x in itertools.combinations(range(8), 3)]
    keyed = {(direct([], list(row), 0), row) for row in rows}
    need(len(keyed) == len(rows), 'collision confondue avec identite')
    need((0, (0, 1, 8)) not in keyed, 'fausse presence')
    # Omettre I peut manquer une naissance. Le rejeu a masque nul garde sa
    # presence exacte et voit un nombre de premieres sondes reussies different.
    inner, shell = [1], [2]
    correct = direct(inner, shell, U64)
    omitted = shared([], shell, 1, U64)
    need(correct != omitted, 'temoin trop faible')
    need(direct(inner, shell, 0) == shared([], shell, 1, 0) == 0,
         'masque nul incorrect')
    print(json.dumps(dict(traces=traces, masks=len(MASKS), checked=traces * len(MASKS),
                          traces_with_wrap=wraps, traces_with_bit63=high_bit,
                          distinct_full_keys_with_hash_zero=len(keyed),
                          omitted_interior=dict(inner=inner, selected=shell,
                                                correct_key=correct, wrong_key=omitted,
                                                first_hit_correct=1, first_hit_wrong=0,
                                                first_hit_zero_mask_reference=1),
                          native_executed=False), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
