"""Experimental fixed attachments from a near-first-cover band, for every K.

This is NOT an amendment to the sealed A0..A6 benchmark. No statistical or
perturbation guarantee is claimed. In particular the K2 pair-margin theorem
does not apply to moving q3/q4 centers or a changing critical catalogue.

Context protocol: n, forest, cover_level(site), witnesses(). witnesses must
be the COMPLETE proper-K strong universe: population >= K, p+q_min <= K,
ALL closed-ball point incidences, and (ball_id, exact squared radius, node)
triples. FULL fusion nodes, including p+q_min=K+1, remain in the forest.
The catalogue and universe are borrowed and never changed by this module.
"""
from dataclasses import dataclass
from fractions import Fraction


class BandError(ValueError):
    """Explicit invalid input, never a truncated band or silently rounded level."""


def exact(value):
    if isinstance(value, bool) or not isinstance(value, (int, Fraction)):
        raise BandError('levels and eta must be exact integers or Fractions')
    return Fraction(value)


@dataclass(frozen=True)
class BandAttachments:
    dates: tuple
    nodes: tuple
    eta: Fraction
    examined: int
    selected: int
    per_site_selected: tuple


def cover_band_lca(ctx, eta=Fraction(1, 8)):
    """Join ALL strong witnesses with beta <= (1+eta)^2 alpha_K(x)^2.

    Attach once at max(first-cover date, birth of their LCA), to
    its ancestor alive at that exact date (closed cut). Singletons complete
    cuts before entry; after entry only ascendance is allowed. Root branches
    may be prolonged beyond their last fusion, as in the native framework.
    This uses local look-ahead: selected later witnesses need not be active
    at the entry date. Coverage there follows from the selected first-cover
    witness and ascendance, NOT from activation of all selected witnesses.

    One streaming pass over each site's existing witness list. No pairs of
    witnesses, no new geometry, no leaf-only restriction, no dense n*n table.
    Work: D scanned incidences and at most D LCA/ancestor queries, plus n
    outputs. This does NOT bound D, universe construction, or FULL work.
    """
    eta = exact(eta)
    if eta < 0:
        raise BandError('negative eta')
    universe = ctx.witnesses()
    if len(universe) != ctx.n:
        raise BandError('witness list count differs from site count')
    forest = ctx.forest
    dates, nodes, counts = [], [], []
    examined = selected = 0
    scale = (1 + eta) ** 2
    for s, witnesses in enumerate(universe):
        alpha = exact(ctx.cover_level(s))
        if alpha < 0:
            raise BandError('negative first-cover level')
        threshold = scale * alpha
        joint, count, at_alpha = None, 0, False
        for _ball, raw_level, node in witnesses:
            examined += 1
            level = exact(raw_level)
            if level < alpha:
                raise BandError('witness preceding first-cover level')
            if level > threshold:
                continue
            if isinstance(node, bool) or not isinstance(node, int) or not 0 <= node < len(forest):
                raise BandError('invalid witness node')
            owner = forest.ancestor(node, level, True)
            if owner is None or owner != node:
                raise BandError('witness node must be alive at its own level')
            joint = owner if joint is None else forest.lca(joint, owner)
            if joint is None:
                raise BandError('no common ancestor for the selected witnesses')
            count += 1
            at_alpha |= level == alpha
        if not at_alpha:
            raise BandError('missing first-cover witness')
        date = max(exact(forest.level(joint)), alpha)
        owner = forest.ancestor(joint, date, True)
        if owner is None:
            raise BandError('attachment ancestor not born')
        dates.append(date)
        nodes.append(owner)
        counts.append(count)
        selected += count
    return BandAttachments(tuple(dates), tuple(nodes), eta, examined, selected, tuple(counts))
