"""Exact audit prototype: keep descendant-minimal witness nodes before LCA.

Finite FULL trees only. No geometry, weights, statistics or product mutation.
Arbitrary witness order is accepted. Full I/U universe is supplied by context.
"""
from dataclasses import dataclass
from fractions import Fraction as F


class ReductionError(ValueError):
    pass


def exact(value):
    if isinstance(value, bool) or not isinstance(value, (int, F)):
        raise ReductionError('exact finite integer/Fraction required')
    return F(value)


def euler(forest):
    """Preorder intervals, also for forests; does not fabricate a common root."""
    roots = [v for v, p in enumerate(forest.parent) if p is None or p == -1]
    tin, tout, seen, clock = [None]*len(forest), [None]*len(forest), set(), 0
    for root in roots:
        stack = [(root, False)]
        while stack:
            v, closing = stack.pop()
            if closing:
                tout[v] = clock
                continue
            if v in seen:
                raise ReductionError('forest is not a rooted acyclic forest')
            seen.add(v)
            tin[v] = clock
            clock += 1
            stack.append((v, True))
            for child in reversed(forest.children[v]):
                if forest.parent[child] != v:
                    raise ReductionError('inconsistent parent/child')
                stack.append((child, False))
    if len(seen) != len(forest):
        raise ReductionError('unreachable forest nodes')
    return tin, tout


def minimal_nodes(nodes, tin, tout):
    """A selected ancestor is redundant iff next selected preorder node is below it."""
    selected = sorted(set(nodes), key=lambda v: tin[v])
    return tuple(v for i, v in enumerate(selected)
                 if i+1 == len(selected) or not (tin[v] < tin[selected[i+1]] < tout[v]))


@dataclass(frozen=True)
class Reduced:
    dates: tuple
    nodes: tuple
    minima: tuple
    selected: int
    removed_distinct_nodes: int


def cover_band_antichain(ctx, eta=F(1, 8)):
    eta = exact(eta)
    if eta < 0:
        raise ReductionError('negative eta')
    witnesses = ctx.witnesses()
    if len(witnesses) != ctx.n:
        raise ReductionError('incorrect witness list count')
    f = ctx.forest
    tin, tout = euler(f)
    dates, nodes, minima = [], [], []
    total = removed = 0
    for s, entries in enumerate(witnesses):
        alpha = exact(ctx.cover_level(s))
        if alpha < 0:
            raise ReductionError('negative first-cover level')
        threshold = (1+eta)**2 * alpha
        selected, at_alpha = [], False
        for _ball, raw_level, v in entries:
            beta = exact(raw_level)
            if beta < alpha:
                raise ReductionError('witness precedes first cover')
            if beta > threshold:
                continue
            if isinstance(v, bool) or not isinstance(v, int) or not 0 <= v < len(f):
                raise ReductionError('invalid node')
            if f.ancestor(v, beta, True) != v:
                raise ReductionError('witness is not alive at its own closed date')
            selected.append(v)
            at_alpha |= beta == alpha
        if not at_alpha:
            raise ReductionError('missing first-cover witness')
        keep = minimal_nodes(selected, tin, tout)
        joint = keep[0]
        for v in keep[1:]:
            joint = f.lca(joint, v)
            if joint is None:
                raise ReductionError('different roots: no finite common ancestor')
        date = max(alpha, exact(f.level(joint)))
        owner = f.ancestor(joint, date, True)
        if owner is None:
            raise ReductionError('attachment not born')
        total += len(selected)
        removed += len(set(selected))-len(keep)
        dates.append(date)
        nodes.append(owner)
        minima.append(keep)
    return Reduced(tuple(dates), tuple(nodes), tuple(minima), total, removed)
