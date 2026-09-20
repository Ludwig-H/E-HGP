#!/usr/bin/env python3
"""Independent integer design gate for a FUTURE q3/q4 witness-inheritance port.

No product import or binary: this judges the mathematical transport contract,
not the current q2 implementation or a q3/q4 support/tower producer. The tiny
front below reproduces the documented midpoint tree and WSPD split rule to
establish that the three counterexamples are reachable, not arbitrary boxes.
Run normally and with python -O; all guards are explicit exceptions.
"""

import itertools
import json
from dataclasses import dataclass


class RejectedDesign(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def bounds(points):
    return tuple((min(p[d] for p in points), max(p[d] for p in points)) for d in range(3))


def mul(a, b):
    values = [x * y for x in a for y in b]
    return min(values), max(values)


def sub(a, b):
    return a[0] - b[1], a[1] - b[0]


def squared(a):
    return (0 if a[0] <= 0 <= a[1] else min(x * x for x in a), max(x * x for x in a))


def certificate(a, b, z):
    """Separable exact H minimum and outward interval Xi upper bound."""
    u = [(z[d] - a[d][1], z[d] - a[d][0]) for d in range(3)]
    v = [(b[d][0] - z[d], b[d][1] - z[d]) for d in range(3)]
    h = sum(mul(x, y)[0] for x, y in zip(u, v))
    xi = sum(squared(sub(mul(u[(d + 1) % 3], v[(d + 2) % 3]),
                         mul(u[(d + 2) % 3], v[(d + 1) % 3])))[1] for d in range(3))
    level = 0 if h <= 0 else 4 if 2 * h * h > xi else 3 if 3 * h * h > xi else 2
    return h, xi, level


def scalar(a, b, z):
    u = [z[d] - a[d] for d in range(3)]
    v = [b[d] - z[d] for d in range(3)]
    h = sum(x * y for x, y in zip(u, v))
    xi = sum((u[(d + 1) % 3] * v[(d + 2) % 3] -
              u[(d + 2) % 3] * v[(d + 1) % 3]) ** 2 for d in range(3))
    return h, xi


def check_corners(a, b, z):
    h, xi, level = certificate(a, b, z)
    values = [scalar(x, y, z) for x, y in itertools.product(itertools.product(*a), itertools.product(*b))]
    require(h == min(x for x, _ in values), "H bound differs from scalar corner minimum")
    require(xi >= max(y for _, y in values), "Xi bound misses a scalar corner")
    return h, xi, level


@dataclass
class Node:
    ids: tuple
    box: tuple
    left: int | None = None
    right: int | None = None


def tree(points):
    nodes, order = [], []

    def build(ids):
        bb = bounds([points[i] for i in ids])
        here = len(nodes)
        nodes.append(Node(tuple(ids), bb))
        if len(ids) == 1:
            order.extend(ids)
        else:
            axis = max(range(3), key=lambda d: bb[d][1] - bb[d][0])
            middle = sum(bb[axis]) // 2
            left = [i for i in ids if points[i][axis] <= middle]
            right = [i for i in ids if points[i][axis] > middle]
            require(left and right, "midpoint split failed")
            nodes[here].left = build(left)
            nodes[here].right = build(right)
        return here

    build(list(range(len(points))))
    return nodes, order


def diameter2(box):
    return sum((hi - lo) ** 2 for lo, hi in box)


def gap2(a, b):
    return sum(max(0, a[d][0] - b[d][1], b[d][0] - a[d][1]) ** 2 for d in range(3))


def window(pivot, count, n):
    first = max(0, min(pivot - count // 2, n - count))
    return tuple(range(first, first + count))


def front(points, k, mask, s, inherit=True, factor=1, mutant=None):
    """Reference DESIGN only: per-rank levels may be promoted on descendants."""
    nodes, order = tree(points)
    rank_of = {point_id: rank for rank, point_id in enumerate(order)}
    events, outputs = [], []
    tasks = [(0, 0, mask, {})]

    def filter_product(ai, bi, active, received):
        a, b = nodes[ai], nodes[bi]
        initial = active
        exterior = len(points) - len(a.ids) - len(b.ids)
        needed = min(k + 2 - q for q in (2, 3, 4) if active & (1 << (q - 2)))
        event = dict(a=a.ids, b=b.ids, mask_before=active, exterior=exterior,
                     needed=needed, received=dict(received))
        if exterior < needed:
            if received and mutant == "reject_skipped_nonempty":
                raise RejectedDesign("a skipped multi-lane search can receive witnesses")
            event.update(skipped=True, mask_after=active, final=dict(received))
            events.append(event)
            return active, dict(received)
        credits = {q: sum(level >= q for level in received.values()) for q in (2, 3, 4)}
        known = dict(received)
        center4 = [sum(a.box[d]) + sum(b.box[d]) for d in range(3)]

        def distance(node):
            return sum(max(0, 4 * node.box[d][0] - center4[d],
                           center4[d] - 4 * node.box[d][1]) ** 2 for d in range(3))

        cursor = 0
        while nodes[cursor].left is not None:
            left, right = nodes[cursor].left, nodes[cursor].right
            cursor = left if distance(nodes[left]) <= distance(nodes[right]) else right
        pivot = rank_of[nodes[cursor].ids[0]]
        historical = window(pivot, min(k, len(points)), len(points))
        wider = window(pivot, min(factor * k, len(points)), len(points))
        extension = tuple(rank for rank in wider if rank not in historical)
        # The rank order is historical first, then left/right complementary intervals.
        for phase, ranks in enumerate((historical, extension)):
            if not active:
                break
            if phase:
                if factor == 1:
                    break
                if not extension:
                    if mutant == "reject_empty_extension":
                        raise RejectedDesign("a multi-lane widened window can legitimately be empty")
                    event["empty_extension"] = True
            for rank in ranks:
                if not active:
                    break
                site = order[rank]
                if site in a.ids or site in b.ids:
                    continue
                previous_level = known.get(rank, 0)
                if rank in received:
                    if mutant == "skip_all_received":
                        continue
                    if all(previous_level >= q for q in (2, 3, 4) if active & (1 << (q - 2))):
                        continue
                _, _, level = certificate(a.box, b.box, points[site])
                for q in (2, 3, 4):
                    bit = 1 << (q - 2)
                    if active & bit and previous_level < q <= level:
                        credits[q] += 1
                        if credits[q] == k + 2 - q:
                            active &= ~bit
                if inherit and level:
                    known[rank] = max(level, previous_level)
        if active:
            lowest = min(q for q in (2, 3, 4) if active & (1 << (q - 2)))
            known = {rank: level for rank, level in known.items() if level >= lowest}
            require(len(known) < k + 2 - lowest, "transported list already rejects an active lane")
        event.update(skipped=False, mask_after=active, final=dict(known),
                     historical=historical, extension=extension, rejected=initial & ~active)
        events.append(event)
        return active, known

    while tasks:
        ai, bi, active, received = tasks.pop()
        a, b = nodes[ai], nodes[bi]
        if ai == bi:
            require(not received, "diagonal product received a list")
            if a.left is not None:
                tasks.extend([(a.right, a.right, active, {}), (a.left, a.right, active, {}),
                              (a.left, a.left, active, {})])
            continue
        active, known = filter_product(ai, bi, active, received)
        if not active:
            continue
        da, db = diameter2(a.box), diameter2(b.box)
        if gap2(a.box, b.box) >= s * s * max(da, db):
            outputs.append((a.ids, b.ids, active))
        else:
            split_a = a.left is not None and (b.left is None or da >= db)
            split = a if split_a else b
            require(split.left is not None, "nonseparated singleton product")
            tasks.extend([(split.right if split_a else ai, bi if split_a else split.right, active, dict(known)),
                          (split.left if split_a else ai, bi if split_a else split.left, active, dict(known))])
    return events, outputs


def event_for(events, a, b):
    found = [event for event in events if set(event["a"]) == set(a) and set(event["b"]) == set(b)]
    require(len(found) == 1, "named product is not reached exactly once")
    return found[0]


def expect_rejected(points, k, mask, s, factor, mutant):
    try:
        front(points, k, mask, s, factor=factor, mutant=mutant)
    except RejectedDesign:
        return
    raise RuntimeError("design mutant survived: " + mutant)


def main():
    promotion = ((0, 0, 0), (0, 4, 0), (10, 0, 0), (10, 4, 0), (5, 4, 0))
    a, b, z = bounds(promotion[:2]), bounds(promotion[2:4]), promotion[4]
    require(check_corners(a, b, z) == (9, 1600, 2), "parent promotion fixture changed")
    require(check_corners(bounds((promotion[1],)), b, z) == (25, 400, 4), "child promotion fixture changed")
    reference, _ = front(promotion, 2, 3, 8)
    plain, _ = front(promotion, 2, 3, 8, inherit=False)
    mutant, _ = front(promotion, 2, 3, 8, mutant="skip_all_received")
    parent = event_for(reference, (0, 1), (2, 3))
    child = event_for(reference, (1,), (2, 3))
    plain_child = event_for(plain, (1,), (2, 3))
    wrong = event_for(mutant, (1,), (2, 3))
    require(parent["final"] and set(parent["final"].values()) == {2}, "parent did not transport its q2 rank")
    require(child["mask_after"] == plain_child["mask_after"] == 1 and wrong["mask_after"] == 3,
            "global deduplication mutant did not lose the q3 rejection")

    skip = tuple((x, 0, 0) for x in (0, 1, 5, 10, 11))
    skip_events, _ = front(skip, 3, 5, 12)
    parent = event_for(skip_events, (0, 1), (3, 4))
    child = event_for(skip_events, (0,), (3, 4))
    require(parent["mask_before"] == 5 and parent["mask_after"] == 1 and parent["final"],
            "parent did not reject q4 and keep its q2 witness")
    require(child["skipped"] and child["received"] and child["exterior"] == 2 and child["needed"] == 3,
            "skipped search with a received rank is not reached")
    expect_rejected(skip, 3, 5, 12, 1, "reject_skipped_nonempty")

    empty = tuple((x, 0, 0) for x in (0, 1, 10, 11))
    empty_events, _ = front(empty, 4, 4, 8, factor=2)
    require(any(event.get("empty_extension") for event in empty_events), "empty extension was not reached")
    expect_rejected(empty, 4, 4, 8, 2, "reject_empty_extension")

    # Prove the current q2 nonempty-window argument on a complete small domain.
    arithmetic_cases = 0
    for n in range(2, 25):
        for k in range(1, 11):
            for na in range(1, n):
                for nb in range(1, n - na + 1):
                    if n - na - nb < k:
                        continue
                    require(n >= k + 2, "q2 exterior-population implication failed")
                    for factor in (2, 4):
                        for pivot in range(n):
                            historical = set(window(pivot, min(k, n), n))
                            wider = set(window(pivot, min(factor * k, n), n))
                            require(historical < wider, "q2 window did not strictly expand")
                            arithmetic_cases += 1

    print(json.dumps({"schema": "mhgp8_audit_front_inheritance_math_v1", "status": "PASS",
                      "scope": "independent mathematical design; no product qualification",
                      "reachable_fixtures": 3, "design_mutants_killed": 3,
                      "q2_window_arithmetic_cases": arithmetic_cases,
                      "promotion_parent": {"h_min": 9, "xi_high": 1600, "level": 2},
                      "promotion_child": {"h_min": 25, "xi_high": 400, "level": 4},
                      "skipped_received_search": {"K": 3, "mask_before_parent": 5,
                                                   "mask_after_parent": 1, "exterior_child": 2},
                      "empty_extension": {"n": 4, "K": 4, "mask": 4, "factor": 2}},
                     sort_keys=True))


if __name__ == "__main__":
    main()
