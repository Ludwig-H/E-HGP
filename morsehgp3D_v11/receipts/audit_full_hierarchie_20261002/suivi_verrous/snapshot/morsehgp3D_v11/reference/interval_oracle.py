"""Attendu independant sur une droite, pour juger les deux etages de la reference sans leur code.

Pour des points alignes (multiplicites comprises), la region { y : D_k(y) <= r^2 } coupe la droite selon la reunion
des intervalles [x_(j+k-1) - r, x_j + r] des fenetres de k points consecutifs (points tries) ; a la coupe ouverte les
inegalites sont strictes. Deux regions temoins se rencontrent si et seulement si leurs traces sur la droite se
rencontrent (la boule minimale de points alignes a son centre sur la droite) : les composantes de la tour sont donc
celles de cette reunion d'intervalles. Par composante [lo, hi] : points de coeur = points de [lo, hi] ; points
couverts = points de [lo - r, hi + r] ; image verticale = la composante de l'ordre k - 1 qui la contient.

Ce fichier n'importe rien de hgp11_ref : ni boule minimale, ni graphe, ni catalogue, ni numerotation, ni lecture de
coupe partagee. Il relit un OrderResult sur ses seuls enregistrements (nodes, lower, cuts).
"""
from fractions import Fraction


def interval_regions(xs, k, radius, closed):
    """Composantes de { y : D_k(y) <= r^2 } (ou < a la coupe ouverte) pour des points tries sur une droite."""
    out = []
    for j in range(len(xs) - k + 1):
        lo, hi = xs[j + k - 1] - radius, xs[j] + radius
        if not (lo <= hi if closed else lo < hi):
            continue
        if out and (lo <= out[-1][1] if closed else lo < out[-1][1]):
            out[-1] = (out[-1][0], max(out[-1][1], hi))
        else:
            out.append((lo, hi))
    return out


def interval_masks(xs, lo, hi, closed):
    """Masque des points de [lo, hi] (bornes exclues a la coupe ouverte)."""
    return sum(1 << i for i, x in enumerate(xs) if (lo <= x <= hi if closed else lo < x < hi))


def interval_cut(res, level, closed):
    """Coupe d'un etage a un niveau quelconque, relue sur ses enregistrements : {noeud vivant: (couverture, coeur)}
    et, par noeud vivant, l'abscisse du centre de sa premiere naissance."""
    last = None
    for cut in res.cuts:
        if cut.level <= level:
            last = cut
    if last is None:
        return {}, {}
    entries = (last.closed if closed else last.opened) if last.level == level else last.closed
    anchors = {}
    for node, _cov, _cor in entries:
        leaf = node
        while res.nodes[leaf].children:
            leaf = res.nodes[leaf].children[0]
        anchors[node] = res.nodes[leaf].center[0]
    return dict((node, (cov, cor)) for node, cov, cor in entries), anchors


def interval_check(positions, results, counts):
    """Juge les resultats (ordres 1 .. n) d'un etage sur des points alignes. Rend None, ou la premiere faute."""
    xs = [Fraction(x) for x in positions]
    gaps = set(abs(a - b) for a in xs for b in xs)
    radii = sorted(set([Fraction(0)]) | gaps | set(gap / 2 for gap in gaps))
    probes = sorted(set(radii) | set((a + b) / 2 for a, b in zip(radii, radii[1:])) | set([radii[-1] + 1]))
    for k, res in enumerate(results, 1):
        up = dict((c, v) for v, node in enumerate(results[k - 2].nodes) for c in node.children) if k > 1 else None
        for radius in probes:
            for closed in (False, True):
                regions = interval_regions(xs, k, radius, closed)
                found, anchors = interval_cut(res, radius * radius, closed)
                if len(found) != len(regions):
                    return 'k=%d r=%s ferme=%s : %d composantes, %d attendues' % (
                        k, radius, closed, len(found), len(regions))
                below = interval_cut(results[k - 2], radius * radius, closed)[0] if k > 1 else None
                for node, masks in sorted(found.items()):
                    inside = [(lo, hi) for lo, hi in regions
                              if (lo <= anchors[node] <= hi if closed else lo < anchors[node] < hi)]
                    if len(inside) != 1:
                        return 'k=%d r=%s : naissance du noeud %d dans %d composantes' % (k, radius, node, len(inside))
                    lo, hi = inside[0]
                    want = (interval_masks(xs, lo - radius, hi + radius, closed), interval_masks(xs, lo, hi, closed))
                    if masks != want:
                        return 'k=%d r=%s ferme=%s noeud %d : (couverture, coeur) %r, attendu %r' % (
                            k, radius, closed, node, masks, want)
                    counts['components'] += 1
                    if k == 1:
                        continue
                    # image verticale : l'intervalle de l'ordre k - 1 qui contient celui-ci, au meme rayon
                    outer = [(a, b) for a, b in interval_regions(xs, k - 1, radius, closed) if a <= lo and hi <= b]
                    if len(outer) != 1:
                        return 'k=%d r=%s : inclusion verticale ambigue' % (k, radius)
                    image = res.lower[node]
                    while image not in below:  # remontee par les enfants de l'ordre k - 1, jusqu'a un noeud vivant
                        if image not in up:
                            return 'k=%d r=%s noeud %d : image verticale sans ancetre vivant' % (k, radius, node)
                        image = up[image]
                    want = (interval_masks(xs, outer[0][0] - radius, outer[0][1] + radius, closed),
                            interval_masks(xs, outer[0][0], outer[0][1], closed))
                    if below[image] != want:
                        return 'k=%d r=%s noeud %d : image verticale %d fausse' % (k, radius, node, image)
                    counts['verticals'] += 1
                counts['cuts'] += 1
    return None
