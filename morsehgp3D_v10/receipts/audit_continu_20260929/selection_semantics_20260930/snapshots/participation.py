"""Masse de participation conservee et hierarchies dures derivees.

Unite de masse (par point x et noeud v de V_x) : w_x(v) = phi(c_x(v)) - phi(d_v), la persistance en echelle phi de
la couverture de x par la branche v (d_racine = infini, phi = 0). W_x = somme des w_x(v) = longueur-phi de l'arbre
de couverture T_x (telescopage : une chaine couverte de c a l'infini pese phi(c)).

Deux activations de la meme masse fixe :
  - PROGRESSIVE (gradual) : a l'instant beta, la masse de x dans v vivant vaut
        A_x(v, beta) = somme_{u descendant strict de v} w_x(u) + (phi(c_x(v)) - phi(beta))_+ ;
  - PAR MARCHES (step) : S_x(v, beta) = somme_{u descendant ou egal, c_x(u) <= beta} w_x(u).
Majorite : t(x) = premier beta ou une branche vivante porte >= W_x/2 (progressive) ou > W_x/2 (marches) ;
proprietaire o(x) = cette branche ; ensuite ascendance seule. Les dates sont representees dans l'espace phi
(phi_date = phi(t(x)), decroissante) : exact pour q entier.
"""
from fractions import Fraction

from fullk import MathError, require
from scale import cmp


def units(T, cover, phi):
    """Par point : dict v -> w_x(v), et W_x."""
    out = []
    for x, cv in enumerate(cover):
        w = {}
        for v, c in cv.items():
            val = phi(c) - phi(T.death[v])
            require(cmp(val, phi.zero()) > 0, 'poids non positif (c_x(v) doit preceder d_v)')
            w[v] = val
        total = phi.zero()
        for val in w.values():
            total += val
        out.append((w, total))
    return out


def bases(T, cover_x, w):
    """base(v) = somme des w(u) sur les descendants stricts u de v dans V_x (V_x est clos vers le haut)."""
    acc = {}
    for v in sorted(cover_x, key=lambda u: -T.depth[u]):
        s = w[v] - w[v]  # zero du regime de calcul
        for c in T.children[v]:
            if c in cover_x:
                s = s + acc[c] + w[c]
        acc[v] = s
    return acc


class Attach:
    """Attache d'un point : phi_date (phi(t(x))), date exacte si disponible, proprietaire, mode."""
    __slots__ = ('phi_date', 'beta', 'owner', 'how')

    def __init__(self, phi_date, beta, owner, how):
        self.phi_date, self.beta, self.owner, self.how = phi_date, beta, owner, how


def majority_gradual(T, cover, phi, unit=None):
    """Majorite a activation progressive (>= W/2 ; exclusif car le futur de T_x est > 0)."""
    unit = unit if unit is not None else units(T, cover, phi)
    res = []
    for x, cv in enumerate(cover):
        w, W = unit[x]
        half = W / 2
        base = bases(T, cv, w)
        best = None  # (phi_date, owner, how, beta_exact)
        for v in cv:
            s = cv[v]
            ps, pd = phi(s), phi(T.death[v])
            if cmp(base[v], half, soft=True) >= 0:
                cand = (ps, v, 'jump', s)       # atteint des l'entree (saut a une fusion)
            else:
                target = base[v] + ps - half    # phi(t) = target
                # egalite a la mort de v : la date est d_v dans les deux lectures ; on laisse le parent la prendre
                if cmp(target, pd, soft=True) <= 0:
                    continue                    # pas de franchissement strictement avant d_v
                exact = None
                if isinstance(target, Fraction) and phi.q == 1:
                    exact = 1 / target
                cand = (target, v, 'slide', exact)
            c = 1 if best is None else cmp(cand[0], best[0], soft=True)
            if c > 0:
                best = cand
            elif c == 0 and T.depth[cand[1]] < T.depth[best[1]]:
                # meme instant : garder le noeud vivant a cet instant (le plus haut, coupe fermee)
                best = cand
        require(best is not None, 'point %d jamais attache' % x)
        res.append(Attach(best[0], best[3], best[1], best[2]))
    return res


def majority_step(T, cover, phi, unit=None):
    """Majorite stricte a activation par marches (unites de branche entieres a leur premiere couverture)."""
    unit = unit if unit is not None else units(T, cover, phi)
    res = []
    for x, cv in enumerate(cover):
        w, W = unit[x]
        base = bases(T, cv, w)
        best = None
        for v in cv:
            if cmp(2 * (base[v] + w[v]), W, soft=True) > 0:
                s = cv[v]
                if best is None or s < best[1] or (s == best[1] and T.depth[v] < T.depth[best[0]]):
                    best = (v, s)
        require(best is not None, 'point %d jamais attache (marches)' % x)
        res.append(Attach(phi(best[1]), best[1], best[0], 'step'))
    return res


def atom_majority(T, atoms, phi, strict=True):
    """Majorite a denominateur fixe sur des atomes (niveau, noeud, poids) actives par marches.

    atoms[x] = liste de (level, node, weight) ; node vivant a level. Rend des Attach (phi_date = phi(t))."""
    res = []
    for x, rows in enumerate(atoms):
        if not rows:
            res.append(Attach(None, None, None, 'no_atom'))  # convention 1/T_x = 0 : aucune masse, jamais attache
            continue
        W = sum((wt for _l, _v, wt in rows), rows[0][2] - rows[0][2])
        events = sorted({l for l, _v, _w in rows} | {T.birth[v] for v in range(len(T))})
        owner = None
        for beta in events:
            mass = {}
            for l, v, wt in rows:
                if l <= beta:
                    a = T.anc(v, beta)
                    mass[a] = mass.get(a, wt - wt) + wt
            win = [a for a, m in mass.items() if (cmp(2 * m, W, soft=True) > 0 if strict else cmp(2 * m, W, soft=True) >= 0)]
            require(len(win) <= 1, 'deux majorites')
            if win:
                owner = (beta, win[0])
                break
        if owner is None:
            res.append(Attach(None, None, None, 'never'))
        else:
            res.append(Attach(phi(owner[0]), owner[0], owner[1], 'atom_step'))
    return res


def fixed_attach(T, dates, nodes, phi):
    """Attaches donnees (date exacte, noeud vivant a cette date) : core, cover, bandes..."""
    res = []
    for d, v in zip(dates, nodes):
        a = T.anc(v, d)
        require(a is not None, 'attache anterieure a la naissance')
        res.append(Attach(phi(d), d, a, 'fixed'))
    return res


# ------------------------------------------------------------------ partitions, hauteurs, laminarite

def attached_at(att, phi, beta):
    """x attache a la coupe fermee beta ssi t(x) <= beta ssi phi(t(x)) >= phi(beta)."""
    if att.owner is None:
        return False
    if att.beta is not None:
        return att.beta <= beta
    return cmp(att.phi_date, phi(beta), soft=True) >= 0


def blocks(T, atts, phi, beta):
    groups = {}
    r = T.cut_rank(beta)
    for x, a in enumerate(atts):
        key = ('c', T.anc_rank(a.owner, r)) if attached_at(a, phi, beta) else ('s', x)
        groups.setdefault(key, []).append(x)
    return sorted(sorted(g) for g in groups.values())


def nested(before, after):
    where = {}
    for i, b in enumerate(after):
        for x in b:
            where[x] = i
    return all(len({where[x] for x in b}) == 1 for b in before)


def cut_levels(T, atts):
    """Coupes de controle : tous les niveaux d'evenements et les dates exactes d'attache."""
    lv = set(T.birth)
    for a in atts:
        if a.beta is not None:
            lv.add(a.beta)
    lv = sorted(lv)
    mids = [(a + b) / 2 for a, b in zip(lv, lv[1:])]
    return sorted(set(lv) | set(mids) | {lv[-1] + 1})


def check_laminar(T, atts, phi, max_cuts=None):
    """Nombre de transitions verifiees ; leve MathError si un bloc se scinde. max_cuts : juge d'echantillon
    (sous-suite deterministe et ordonnee des coupes, extremites comprises) ; None = toutes les coupes."""
    prev = None
    count = 0
    cuts = cut_levels(T, atts)
    if max_cuts is not None and len(cuts) > max_cuts:
        step = (len(cuts) - 1) / (max_cuts - 1)
        cuts = sorted({cuts[round(i * step)] for i in range(max_cuts)})
    for beta in cuts:
        cur = blocks(T, atts, phi, beta)
        if prev is not None:
            require(nested(prev, cur), 'bloc scinde a beta=%s' % beta)
            count += 1
        prev = cur
    return count


def merge_phi(T, atts, x, y, phi):
    """Hauteur de reunion de x et y dans l'espace phi : min(phi(t_x), phi(t_y), phi(niveau LCA))."""
    ax, ay = atts[x], atts[y]
    if ax.owner is None or ay.owner is None:
        return None
    l = T.lca(ax.owner, ay.owner)
    vals = [ax.phi_date, ay.phi_date, phi(T.birth[l])]
    m = vals[0]
    for v in vals[1:]:
        if cmp(v, m, soft=True) < 0:
            m = v
    return m


def reserve(T, cover, phi, unit, x, beta):
    """Reserve de x a beta (masse non encore active), normalisee."""
    w, W = unit[x]
    base = bases(T, cover[x], w)
    active = phi.zero()
    for v, c in cover[x].items():
        if T.alive(v, beta) and c <= beta:
            active += base[v] + phi(c) - phi(beta)
    return 1 - active / W


# ------------------------------------------------------------------ representation compressee (squelette de T_x)
# Un noeud de V_x est une CONTINUATION s'il a exactement un enfant dans V_x : le long d'une chaine de continuations,
# les unites telescopent. Le squelette (noeuds de V_x ayant 0 ou >= 2 enfants couverts) porte toute l'information
# des majorites ; son nombre de segments est au plus 2 x (nombre d'entrees de x), donc au plus 2 x (temoins de x).

class Segment:
    __slots__ = ('a', 's', 'top_node', 'top_level', 'next', 'base', 'w')

    def __init__(self, a, s, top_node, top_level, nxt):
        self.a, self.s, self.top_node, self.top_level, self.next = a, s, top_node, top_level, nxt
        self.base = None
        self.w = None


def segments(T, cv, phi):
    """Segments du squelette de T_x (du bas vers le haut), bases et W_x."""
    count = {}
    for v in cv:
        p = T.parent[v]
        if p >= 0:
            count[p] = count.get(p, 0) + 1
    skel = [v for v in cv if count.get(v, 0) != 1]
    segs = {}
    for a in skel:
        u = a
        while T.parent[u] >= 0 and count.get(T.parent[u], 0) == 1:
            u = T.parent[u]
        nxt = T.parent[u] if T.parent[u] >= 0 else None
        segs[a] = Segment(a, cv[a], u, T.death[u], nxt)
    order = sorted(segs, key=lambda a: -T.depth[a])
    zero = phi.zero()
    for a in order:
        sg = segs[a]
        if sg.base is None:
            sg.base = zero
        sg.w = phi(sg.s) - phi(sg.top_level)
        require(cmp(sg.w, zero) > 0, 'segment de poids non positif')
        if sg.next is not None:
            up = segs[sg.next]
            up.base = (up.base if up.base is not None else zero) + sg.base + sg.w
    W = zero
    for sg in segs.values():
        W = W + sg.w
    return [segs[a] for a in order], W


def chain_owner(T, a, target, phi):
    """Noeud de la chaine issue de a vivant a l'instant t tel que phi(t) = target (coupe fermee)."""
    v = a
    while T.death[v] is not None and cmp(phi(T.death[v]), target, soft=True) >= 0:
        v = T.parent[v]
    return v


def majority_gradual_fast(T, cover, phi):
    res = []
    for x, cv in enumerate(cover):
        segs, W = segments(T, cv, phi)
        half = W / 2
        best = None
        for sg in segs:
            ps = phi(sg.s)
            if cmp(sg.base, half, soft=True) >= 0:
                cand = (ps, sg.a, 'jump', sg.s)
            else:
                target = sg.base + ps - half
                if cmp(target, phi(sg.top_level), soft=True) <= 0:
                    continue
                exact = 1 / target if isinstance(target, Fraction) and phi.q == 1 else None
                cand = (target, chain_owner(T, sg.a, target, phi), 'slide', exact)
            c = 1 if best is None else cmp(cand[0], best[0], soft=True)
            if c > 0 or (c == 0 and T.depth[cand[1]] < T.depth[best[1]]):
                best = cand
        require(best is not None, 'point %d jamais attache' % x)
        res.append(Attach(best[0], best[3], best[1], best[2]))
    return res


def majority_step_fast(T, cover, phi):
    res = []
    for x, cv in enumerate(cover):
        segs, W = segments(T, cv, phi)
        best = None
        for sg in segs:
            acc0 = sg.base + phi(sg.s)
            v = sg.a
            while True:
                if cmp(2 * (acc0 - phi(T.death[v])), W, soft=True) > 0:
                    t = sg.s if v == sg.a else T.birth[v]
                    if best is None or t < best[1] or (t == best[1] and T.depth[v] < T.depth[best[0]]):
                        best = (v, t)
                    break
                if v == sg.top_node:
                    break
                v = T.parent[v]
        require(best is not None, 'point %d jamais attache (marches)' % x)
        res.append(Attach(phi(best[1]), best[1], best[0], 'step'))
    return res
