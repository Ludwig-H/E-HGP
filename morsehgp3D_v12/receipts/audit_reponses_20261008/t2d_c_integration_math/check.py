#!/usr/bin/env python3
"""Modèles bornés de fenêtres et de tranches, sans moteur ni CUDA."""
import argparse
from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def oracle(edges, markers):
    """Composantes d'un graphe chemin, par exploration indépendante."""
    n = len(edges) + 1
    unseen = set(range(n)); result = []
    while unseen:
        todo = [min(unseen)]; component = set()
        while todo:
            v = todo.pop()
            if v not in unseen:
                continue
            unseen.remove(v); component.add(v)
            for u in (v - 1, v + 1):
                if 0 <= u < n and edges[min(u, v)]:
                    todo.append(u)
        if any(i - 1 in component and i in component for i in markers):
            result.append((min(component), max(component) + 1))
    return result


def windows(edges, markers, initial=64, mutant=None):
    """Traduction du contrôle chain_bounds/covered ; pas du calcul F3/F4."""
    n = len(edges) + 1
    need(all(1 <= i < n and edges[i - 1] for i in markers), 'précondition marqueur')
    need(markers == sorted(set(markers)), 'précondition positions')
    result = []; covered = 0; read = 0
    for i in markers:
        if i < covered and mutant != 'sans_dedup':
            continue
        w = min(initial, n)
        while True:
            lo, hi = max(0, i - w), min(n, i + w)
            read += hi - lo
            s, e = i - 1, i + 1
            while s > lo and edges[s - 1]:
                s -= 1
            while e < hi and edges[e - 1]:
                e += 1
            if mutant == 'fenetre_unique' or ((s > lo or lo == 0) and (e < hi or hi == n)):
                break
            w = min(2 * w, n)
        result.append((s, e)); covered = e
    return result, read


def export(records):
    """Champs logiques de la sortie, y compris la forme du premier niveau de rang."""
    balls = []; offsets = [0]; values = []; levels = [(0, 1)]; old = None
    for rec in records:
        ident, num, den, support, population, key = rec
        value = Fraction(num, den)
        if value != old:
            levels.append((num, den)); old = value
        balls.append((ident, support, len(levels) - 1, key))
        values.extend(population); offsets.append(len(values))
    table = sorted(range(len(records)), key=lambda i: records[i][3])
    return balls, offsets, values, levels, table


def repair(records, edges, mutant=None):
    canonical = lambda r: (Fraction(r[1], r[2]), r[3])
    marks = [i for i in range(1, len(records)) if canonical(records[i - 1]) > canonical(records[i])]
    ranges, _ = windows(edges, marks)
    out = list(records)
    for s, e in ranges:
        part = sorted(records[s:e], key=canonical)
        if mutant == 'cles_desolidarisees':
            part = [r[:-1] + (records[s + j][-1],) for j, r in enumerate(part)]
        out[s:e] = part
    return out


def staged(segments, slots, capacity, mutant=None):
    """DMA différé jusqu'à wait ; slots empoisonnés, libérés après consommation."""
    need(1 <= slots <= 8, 'nombre de cases')
    for width, values in segments:
        need(not values or 0 < width <= capacity, 'largeur de tranche')
    jobs = []
    for segment, (width, values) in enumerate(segments):
        if not values:
            continue
        per = capacity // width
        for first in range(0, len(values), per):
            jobs.append((segment, first, min(per, len(values) - first)))
    pending = [None] * slots; data = [None] * slots; state = ['free'] * slots
    out = [[None] * len(v) for _, v in segments]; writes = [[0] * len(v) for _, v in segments]
    issued = consumed = 0
    def issue(j):
        c = j % slots
        need(state[c] == 'free', 'case réutilisée avant consommation')
        pending[c] = jobs[j]; data[c] = None; state[c] = 'issued'
    while issued < min(slots, len(jobs)):
        issue(issued); issued += 1
    while consumed < issued:
        c = consumed % slots
        segment, first, count = pending[c]
        if mutant != 'sans_attente':
            data[c] = tuple(segments[segment][1][first:first + count]); state[c] = 'ready'
        if mutant == 'reemploi_precoce' and issued < len(jobs):
            issue(issued)
        need(state[c] == 'ready', 'consommation avant attente')
        for k, value in enumerate(data[c]):
            out[segment][first + k] = value; writes[segment][first + k] += 1
        state[c] = 'free'; data[c] = None
        if issued < len(jobs):
            issue(issued); issued += 1
        consumed += 1
    need(all(all(v == 1 for v in row) for row in writes), 'couverture exacte')
    need(all(v == 'free' for v in state), 'fin sans emprunt')
    return out, len(jobs)


def materialize(words, num_words, mutant=None):
    result = [(0, 1)]
    for row in words:
        ns, ds = row[:num_words], row[num_words:]
        if mutant == 'mots_hauts_perdus':
            ns, ds = ns[:1], ds[:1]
        n = sum(w << (64 * i) for i, w in enumerate(ns))
        d = sum(w << (64 * i) for i, w in enumerate(ds))
        need(d > 0, 'dénominateur')
        result.append((n, d))
    if mutant == 'rang_zero_ecrase':
        return result[1:]
    return result


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--repo', type=Path, required=True); args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    for version, files in cap['sources'].items():
        for path, digest in files.items():
            raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['pins'][version] + ':' + path])
            need(hashlib.sha256(raw).hexdigest() == digest, 'source différente : ' + path)
    exhaustive = 0; killed = set()
    for n in range(1, 9):
        for states in itertools.product(range(3), repeat=n - 1):
            edges = [s != 0 for s in states]
            markers = [i + 1 for i, s in enumerate(states) if s == 2]
            want = oracle(edges, markers)
            for initial in (1, 2, 64):
                got, read = windows(edges, markers, initial)
                need(got == want, 'composantes différentes')
                repaired = sum(e - s for s, e in got)
                need(read <= 8 * (repaired + initial * len(got)), 'borne de lectures')
                exhaustive += 1
            for mutant in ('sans_dedup', 'fenetre_unique'):
                if mutant not in killed and windows(edges, markers, 1, mutant)[0] != want:
                    killed.add(mutant)
    halos = 0
    for n in (63, 64, 65, 127, 128, 129, 255, 256, 257, 1023, 1024, 1025, 2049):
        for breaks in ((), (n // 2,), (1, n - 2)):
            edges = [i + 1 not in breaks for i in range(n - 1)]
            for mode in ('tous', 'extremites'):
                candidates = range(1, n) if mode == 'tous' else (1, n // 2, n - 1)
                markers = sorted(set(i for i in candidates if edges[i - 1]))
                got, read = windows(edges, markers)
                need(got == oracle(edges, markers), 'halo')
                need(read <= 8 * (sum(e - s for s, e in got) + 64 * len(got)), 'borne halo')
                halos += 1
    # Deux groupes séparés par une arête certaine ; égalités de valeur et formes différentes.
    base = [(0, 2, 4, (1, 2), (0, 1), 10), (1, 3, 6, (0, 3), (1,), 11),
            (2, 3, 4, (2, 3), (2, 3, 4), 12), (3, 1, 4, (0, 1), (), 13)]
    canonical = lambda r: (Fraction(r[1], r[2]), r[3])
    payload = 0
    for perm in itertools.permutations(base):
        # La clé est attachée au record ; les clés croissent dans l'ordre avant réparation.
        first = [r[:-1] + (10 + j,) for j, r in enumerate(perm)]
        last = [(i + 4, n + 10 * d, d, tuple(x + 10 for x in s), p, k + 100)
                for i, n, d, s, p, k in first]
        records = first + last; edges = [True] * 3 + [False] + [True] * 3
        want = export(sorted(records, key=canonical))
        need(export(repair(records, edges)) == want, 'objet canonique')
        if export(repair(records, edges, 'cles_desolidarisees')) != want:
            killed.add('cles_desolidarisees')
        payload += 1
    need(export([]) == ([], [0], [], [(0, 1)], []), 'catalogue vide')
    streams = levels = 0
    for num_words, den_words in ((3, 3), (4, 3), (5, 4)):
        rows = [tuple(1 + (i + j) % 3 for j in range(num_words + den_words)) for i in range(13)]
        exact = materialize(rows, num_words)
        for slots, capacity in itertools.product((1, 2, 3, 8), (72, 73, 127, 256, 1024)):
            segments = [(4, list(range(131))), (8, []), (8 * (num_words + den_words), rows),
                        (8, list(range(17)))]
            got, _ = staged(segments, slots, capacity)
            need(got == [v for _, v in segments], 'tranches')
            need(materialize(got[2], num_words) == exact, 'formes exactes')
            streams += 1; levels += len(rows)
        for mutant in ('mots_hauts_perdus', 'rang_zero_ecrase'):
            if materialize(rows, num_words, mutant) != exact:
                killed.add(mutant)
    for mutant in ('sans_attente', 'reemploi_precoce'):
        try:
            staged([(4, list(range(11)))], 2, 8, mutant)
        except ValueError:
            killed.add(mutant)
    refusals = 0
    for slots, capacity, segments in ((0, 8, [(4, [1])]), (9, 8, [(4, [1])]),
                                       (2, 8, [(9, [1])]), (2, 8, [(0, [1])])):
        try:
            staged(segments, slots, capacity)
        except ValueError:
            refusals += 1
    empty, chunks = staged([(0, []), (72, [])], 2, 72)
    need(empty == [[], []] and chunks == 0, 'flux vide')
    need(len(killed) == 7 and refusals == 4, 'témoins incomplets')
    print(json.dumps(dict(exhaustive_graph_windows=exhaustive, halo_cases=halos, canonical_payloads=payload,
                         streams=streams, level_rows=levels, model_mutants=sorted(killed), invalid_streams=refusals,
                         native_execution=False, gpu_execution=False), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
