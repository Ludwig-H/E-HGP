#!/usr/bin/env python3
"""Modele borne du scan propose, pas une qualification du code natif ni un chrono."""
from bisect import bisect_left
from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path
import random

PIN = "58d384721678d11ef8ccd86c76cca182f41a716c"
NONE = 2**32 - 1


def require(condition, message):
    if not condition:
        raise ValueError(message)


def valid(rows):
    for i, (form, support, population) in enumerate(rows):
        value = Fraction(*form)
        require(value > 0, "niveau non positif")
        if i:
            previous = Fraction(*rows[i - 1][0])
            require(previous <= value, "niveau decroissant")
            require(previous != value or rows[i - 1][1] < support, "departage ou doublon")
        require(len(population) >= 2, "population incomplete")


def oracle(rows):
    valid(rows)
    values = sorted({Fraction(*row[0]) for row in rows})
    ranks = [bisect_left(values, Fraction(*row[0])) + 1 for row in rows]
    forms = [(0, 1)]
    for rank in range(1, len(values) + 1):
        forms.append(rows[ranks.index(rank)][0])
    offsets = [0]
    flat = []
    for _, _, population in rows:
        flat.extend(population)
        offsets.append(len(flat))
    return dict(ranks=ranks, forms=forms, offsets=offsets, flat=flat)


def blocked(rows, grain, seed, mutant=""):
    valid(rows)
    require(grain > 0, "grain nul")
    n = len(rows)
    local_rank = [0] * n
    local_offset = [0] * n
    starts = [False] * n
    blocks = []
    for first in range(0, n, grain):
        last = min(n, first + grain)
        rank = size = 0
        for i in range(first, last):
            start = i == 0 or Fraction(*rows[i - 1][0]) < Fraction(*rows[i][0])
            if mutant == "sans_halo" and i == first:
                start = True
            if mutant == "formes_non_valeurs":
                start = i == 0 or rows[i - 1][0] != rows[i][0]
            starts[i] = start
            rank += int(start)
            local_rank[i], local_offset[i] = rank, size
            size += len(rows[i][2])
        blocks.append([first, last, rank, size, 0, 0])
    rank_base = size_base = 0
    for block in blocks:
        block[4], block[5] = rank_base, size_base
        rank_base += block[2]
        size_base += block[3]
        if mutant == "prefixe_inclusif":
            block[4], block[5] = rank_base, size_base
    result = dict(ranks=[None] * n, forms=[None] * (rank_base + 1),
                  offsets=[None] * (n + 1), flat=[None] * size_base)
    result["forms"][0], result["offsets"][0] = (0, 1), 0
    # Toute permutation de completion des blocs doit ecrire des plages disjointes.
    random.Random(seed).shuffle(blocks)
    for first, last, _, _, rb, sb in blocks:
        for i in range(first, last):
            rank, offset = rb + local_rank[i], sb + local_offset[i]
            result["ranks"][i] = rank
            if starts[i]:
                require(rank < len(result["forms"]), "rang hors tableau")
                require(result["forms"][rank] is None, "double ecriture niveau")
                result["forms"][rank] = rows[i][0]
            elif mutant == "dernier_representant":
                result["forms"][rank] = rows[i][0]
            for j, value in enumerate(rows[i][2]):
                require(offset + j < len(result["flat"]), "population hors tableau")
                require(result["flat"][offset + j] is None, "double ecriture population")
                result["flat"][offset + j] = value
            result["offsets"][i + 1] = offset + len(rows[i][2])
    return result


def support_table(supports, sites):
    """Alternative : permutation globale triee des quatre SiteIdx puis CSR."""
    keys = [tuple(s) + (NONE,) * (4 - len(s)) for s in supports]
    for support in supports:
        require(2 <= len(support) <= 4 and list(support) == sorted(set(support)), "support invalide")
        require(support[-1] < sites, "site hors domaine")
    require(len(set(keys)) == len(keys), "support repete")
    order = sorted(range(len(keys)), key=keys.__getitem__)
    firsts = [keys[b][0] for b in order]
    offsets = [bisect_left(firsts, s) for s in range(sites + 1)]
    return keys, order, offsets


def main():
    rng = random.Random(20261007)
    checks = invalid = lookups = 0
    samples = [[], [((100, 4), (1, 2), (1, 2))]]
    # Egalites rationnelles de representations differentes, plateaux traversant les blocs.
    witness = [((100, 4), (1, 2), (1, 2)),
               ((409600, 16384), (3, 4, 5), (3, 4, 5)),
               ((25, 1), (6, 7), (6, 7)),
               ((26, 1), (8, 9), (8, 9))]
    samples.append(witness)
    for n in (2, 3, 4, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 257):
        for family in range(8):
            values = sorted(rng.randrange(1, max(2, n // 3)) for _ in range(n))
            if family == 0:
                values = [25] * n
            rows = []
            for i, value in enumerate(values):
                multiplier = rng.choice((1, 4, 16384, 2**80 + 1))
                rows.append(((value * multiplier, multiplier), (2*i, 2*i + 1),
                             tuple(range(i * 16, i * 16 + rng.randrange(2, 13)))))
            samples.append(rows)
    for rows in samples:
        expected = oracle(rows)
        for grain in (1, 2, 3, 7, 16, 32, 64, 128, 512):
            for seed in (0, 1, 7):
                require(blocked(rows, grain, seed) == expected, "scan different")
                checks += 1
    for rows in ([((0, 1), (1, 2), (1, 2))], witness[::-1], witness[:1] * 2):
        for fn in (oracle, lambda x: blocked(x, 2, 0)):
            try:
                fn(rows)
            except ValueError:
                invalid += 1
            else:
                raise ValueError("temoin invalide accepte")
    killed = {}
    for mutant in ("sans_halo", "formes_non_valeurs", "prefixe_inclusif", "dernier_representant"):
        try:
            got = blocked(witness, 2, 0, mutant)
            killed[mutant] = got != oracle(witness)
        except ValueError:
            killed[mutant] = True
        require(killed[mutant], "mutant vivant")
    for sites in range(2, 14):
        universe = [s for q in (2, 3, 4) for s in itertools.combinations(range(sites), q)]
        for _ in range(4):
            supports = rng.sample(universe, rng.randrange(len(universe) + 1))
            keys, order, offsets = support_table(supports, sites)
            expected = {s: b for b, s in enumerate(supports)}
            for support in universe:
                key = support + (NONE,) * (4 - len(support))
                row = order[offsets[support[0]]:offsets[support[0] + 1]]
                rowkeys = [keys[b] for b in row]
                at = bisect_left(rowkeys, key)
                found = row[at] if at < len(row) and rowkeys[at] == key else None
                require(found == expected.get(support), "table differente")
                lookups += 1
    root = Path(__file__).resolve().parents[3]
    paths = ("src/catalogue/assemble.cpp", "src/catalogue/table.cpp", "src/catalogue/sort.cpp")
    hashes = {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}
    print(json.dumps(dict(pin=PIN, modele_seulement=True, nuages=0, suites=len(samples),
                         comparaisons_scan=checks, refus=invalid, mutants=killed,
                         requetes_table=lookups, source_sha256=hashes), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
