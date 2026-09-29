"""DEV : condensation HDBSCAN d'un arbre de points exporte par mhgp10_cluster --tree, en Python, pour inspecter
les stabilites (outil de diagnostic, jamais un juge). Niveaux exportes = rayons carres ; lambda = level^(-z/2).

  python3 condense_dev.py TREE --mcs M --z Z [--top 25]
"""
import argparse
import collections
import math


def load(path):
    with open(path) as f:
        tok = f.read().split('\n')
    i = 0
    nl = int(tok[i].split()[1]); i += 1
    levels = [float(x) for x in tok[i:i + nl]]; i += nl
    nn = int(tok[i].split()[1]); i += 1
    rank, parent = [], []
    for line in tok[i:i + nn]:
        a, b = line.split()
        rank.append(int(a)); parent.append(int(b))
    i += nn
    npnt = int(tok[i].split()[1]); i += 1
    pts = []
    for line in tok[i:i + npnt]:
        idx, node, prank, _ = line.split()
        pts.append((int(idx), int(node), int(prank)))
    return levels, rank, parent, pts


def condense(levels, rank, parent, pts, mcs, z):
    lam = lambda lv: math.inf if lv <= 0 else lv ** (-z / 2.0)
    nn = len(rank)
    children = collections.defaultdict(list)
    roots = []
    for v, p in enumerate(parent):
        (roots if p < 0 else children[p]).append(v)
    attached = collections.defaultdict(list)
    for idx, node, prank in pts:
        attached[node].append((idx, prank))
    size = [0] * nn
    order = sorted(range(nn), key=lambda v: rank[v])  # enfants avant parents (rang croissant)
    for v in order:
        size[v] = len(attached[v]) + sum(size[c] for c in children[v])
    clusters = []  # dict(parent, birth_lam, size, stab, children)

    def new_cluster(par, blam, sz):
        clusters.append(dict(parent=par, birth=blam, size=sz, stab=0.0, kids=[], death=None))
        if par is not None:
            clusters[par]['kids'].append(len(clusters) - 1)
        return len(clusters) - 1

    def drop_subtree(v, c, lam_leave):
        stack = [v]
        while stack:
            u = stack.pop()
            clusters[c]['stab'] += (len(attached[u])) * (lam_leave - clusters[c]['birth'])
            stack.extend(children[u])

    root_c = new_cluster(None, 0.0, sum(size[r] for r in roots))
    work = [(r, root_c) for r in roots]
    while work:
        v, c = work.pop()
        b = clusters[c]['birth']
        for idx, prank in attached[v]:
            clusters[c]['stab'] += min(lam(levels[prank]), 1e300) - b
        lv = lam(levels[rank[v]])
        big = [ch for ch in children[v] if size[ch] >= mcs]
        small = [ch for ch in children[v] if size[ch] < mcs]
        for ch in small:
            drop_subtree(ch, c, lv)
        if len(big) >= 2:
            clusters[c]['death'] = lv
            for ch in big:
                clusters[c]['stab'] += size[ch] * (lv - b)
                work.append((ch, new_cluster(c, lv, size[ch])))
        elif len(big) == 1:
            work.append((big[0], c))
        else:
            clusters[c]['death'] = lv if clusters[c]['death'] is None else clusters[c]['death']
    # EOM, racine exclue
    sel = [False] * len(clusters)
    best = [0.0] * len(clusters)
    for c in reversed(range(len(clusters))):
        kids = clusters[c]['kids']
        s_kids = sum(best[k] for k in kids)
        if c != 0 and (not kids or clusters[c]['stab'] >= s_kids):
            best[c] = clusters[c]['stab']
            sel[c] = True
        else:
            best[c] = s_kids
    # ne garder que les plus hauts selectionnes
    chosen = []
    stack = [0]
    while stack:
        c = stack.pop()
        if sel[c] and c != 0:
            chosen.append(c)
        else:
            stack.extend(clusters[c]['kids'])
    return clusters, sel, best, chosen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('tree')
    ap.add_argument('--mcs', type=int, required=True)
    ap.add_argument('--z', type=float, required=True)
    ap.add_argument('--top', type=int, default=25)
    a = ap.parse_args()
    levels, rank, parent, pts = load(a.tree)
    clusters, sel, best, chosen = condense(levels, rank, parent, pts, a.mcs, a.z)
    r = lambda l: math.inf if l == 0 else (l ** (-1.0 / a.z) if l != math.inf else 0.0)
    print('clusters %d, choisis %d : tailles %s' % (len(clusters), len(chosen),
                                                   sorted((clusters[c]['size'] for c in chosen), reverse=True)))
    depth = {0: 0}
    for c in range(1, len(clusters)):
        depth[c] = depth[clusters[c]['parent']] + 1
    shown = sorted(range(len(clusters)), key=lambda c: -clusters[c]['size'])[:a.top]
    for c in sorted(shown):
        cl = clusters[c]
        print('%s#%d taille=%d r_naissance=%.1f r_scission=%s stab=%.4g enfants_best=%.4g %s' % (
            '  ' * depth[c], c, cl['size'], r(cl['birth']) if cl['birth'] > 0 else float('inf'),
            '%.1f' % r(cl['death']) if cl['death'] else '-', cl['stab'],
            sum(best[k] for k in cl['kids']), 'CHOISI' if c in chosen else ''))


if __name__ == '__main__':
    main()
