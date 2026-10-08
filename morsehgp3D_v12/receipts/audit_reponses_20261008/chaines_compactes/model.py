#!/usr/bin/env python3
"""Modele de selection/scans seulement, aucun niveau exact ni moteur GPU."""
import math
import struct

U32_NONE = (1 << 32) - 1
U64_MAX = (1 << 64) - 1


def need(ok, why):
    if not ok:
        raise ValueError(why)


def natural(x, high=U64_MAX):
    return type(x) is int and 0 <= x <= high


def key_order(a_bits, b_bits):
    # Reprise de la classification F4 pour produire les aretes du modele.
    # Ne constitue pas une nouvelle preuve du certificat flottant.
    a, b = (struct.unpack('d', struct.pack('Q', x))[0] for x in (a_bits, b_bits))
    if a == 0 or b == 0:
        return (a > b) - (a < b)
    c = 1.0 - 2.0 ** -40
    return -1 if a < c * b else 1 if b < c * a else 0


def validate(verdict, order, keys, fault):
    n = len(keys)
    need(n < U32_NONE, 'index_overflow_u32')
    need(len(verdict) == len(order) == n, 'length')
    need(type(fault) is int and fault in (0, 8), 'fault')
    need(all(type(x) is int and x in (0, 1) for x in verdict), 'verdict')
    need(not n or verdict[0] == 0, 'first_verdict')
    need((fault == 8) == any(verdict), 'fault_consistency')
    need(all(natural(x, n - 1) for x in order) and len(set(order)) == n, 'order_permutation')
    for x in keys:
        need(natural(x), 'key_word')
        value = struct.unpack('d', struct.pack('Q', x))[0]
        need(math.isfinite(value) and value > 0, 'positive_finite_key')
    need(all(a <= b for a, b in zip(keys, keys[1:])), 'initial_F3_order')
    edge = [False] + [key_order(keys[i - 1], keys[i]) == 0 for i in range(1, n)] if n else []
    need(all(not verdict[i] or edge[i] for i in range(n)), 'inversion_on_certain_boundary')
    return edge


def collect_original(verdict, order, keys):
    """Traduction du parcours collect_chains/chain_around epingle, sans ses allocations."""
    bounds, balls, packed_keys = [], [], []
    n, covered = len(keys), 0
    for i in range(1, n):
        if verdict[i] != 1 or i < covered:
            continue
        s, e = i - 1, i + 1
        while s > 0 and key_order(keys[s - 1], keys[s]) == 0:
            s -= 1
        while e < n and key_order(keys[e - 1], keys[e]) == 0:
            e += 1
        bounds.append((s, e))
        balls.extend(order[s:e])
        packed_keys.extend(keys[s:e])
        covered = e
    return bounds, balls, packed_keys


def graph_oracle(verdict, order, keys):
    """Oracle independant : composantes du graphe chemin, parcours explicite."""
    n, seen = len(keys), set()
    components = []
    for start in range(n):
        if start in seen:
            continue
        todo, vertices = [start], []
        seen.add(start)
        while todo:
            v = todo.pop()
            vertices.append(v)
            for w in (v - 1, v + 1):
                if 0 <= w < n and w not in seen and key_order(keys[min(v, w)], keys[max(v, w)]) == 0:
                    seen.add(w)
                    todo.append(w)
        component = sorted(vertices)
        if any(verdict[i] == 1 for i in component[1:]):
            components.append(component)
    return ([(c[0], c[-1] + 1) for c in components],
            [order[i] for c in components for i in c], [keys[i] for c in components for i in c])


def segmented(a, b):
    """Monoi'de ordre-sensible : un debut a droite coupe le cumul de gauche."""
    return (a[0] or b[0], b[1] if b[0] else a[1] or b[1])


def inclusive_scan(values, op, identity):
    """Blelloch (arbre), travail lineaire ; modele de primitive GPU, pas une implementation GPU."""
    if not values:
        return [], 0
    n = len(values)
    p = 1 << (n - 1).bit_length()
    tree = list(values) + [identity] * (p - n)
    work, step = 0, 1
    while step < p:
        for end in range(2 * step - 1, p, 2 * step):
            tree[end] = op(tree[end - step], tree[end]); work += 1
        step *= 2
    tree[-1] = identity
    step = p // 2
    while step:
        for end in range(2 * step - 1, p, 2 * step):
            left = tree[end - step]
            tree[end - step] = tree[end]
            tree[end] = op(tree[end], left); work += 1
        step //= 2
    return [op(tree[i], values[i]) for i in range(n)], work + n


def compact(verdict, order, keys, fault=None, max_elements=None, max_chains=None, payload_budget=U64_MAX):
    if fault is None:
        fault = 8 if any(verdict) else 0
    edge = validate(verdict, order, keys, fault)
    n = len(keys)
    heads = [not x for x in edge]
    tails = [not edge[i + 1] if i + 1 < n else True for i in range(n)]
    forward, a = inclusive_scan(list(zip(heads, map(bool, verdict))), segmented, (False, False))
    reverse, b = inclusive_scan(list(zip(reversed(tails), reversed(list(map(bool, verdict))))),
                                segmented, (False, False))
    backward = list(reversed(reverse))
    selected = [forward[i][1] or backward[i][1] for i in range(n)]
    ranks, c = inclusive_scan([int(x) for x in selected], lambda x, y: x + y, 0)
    chain_ranks, d = inclusive_scan([int(selected[i] and heads[i]) for i in range(n)], lambda x, y: x + y, 0)
    r, q = (ranks[-1], chain_ranks[-1]) if n else (0, 0)
    check_counts(n, r, q, n if max_elements is None else max_elements,
                 n if max_chains is None else max_chains, payload_budget)
    bounds, balls, packed_keys = [[None, None] for _ in range(q)], [None] * r, [None] * r
    for i in range(n):
        if not selected[i]:
            continue
        balls[ranks[i] - 1], packed_keys[ranks[i] - 1] = order[i], keys[i]
        if heads[i]:
            bounds[chain_ranks[i] - 1][0] = i
        if tails[i]:
            bounds[chain_ranks[i] - 1][1] = i + 1
    need(a + b + c + d <= 20 * n, 'scan_work_bound')
    return ([tuple(x) for x in bounds], balls, packed_keys), a + b + c + d


def check_counts(n, r, q, max_elements, max_chains, payload_budget):
    need(all(natural(x) for x in [n, r, q, max_elements, max_chains, payload_budget]), 'count_type_or_overflow')
    need(n < U32_NONE, 'index_overflow_u32')
    need(r <= n and 2 * q <= r and ((r == 0) == (q == 0)), 'counts')
    need(r <= max_elements and q <= max_chains, 'capacity')
    need(12 * r + 16 * q + 16 <= payload_budget, 'memory_budget')
    return 12 * r + 16 * q + 16
