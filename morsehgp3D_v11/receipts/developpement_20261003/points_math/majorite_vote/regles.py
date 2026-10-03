#!/usr/bin/env python3
"""Regles de pendaison FULL_k -> points, ecrites ici depuis leurs definitions ECRITES (aucun code de regle importe) :

  H_m    : docs/HIERARCHIE_POINTS.md, section 3 (developpeur v11) ;
  ER0h   : VERDICT_FINAL.md v10, section 2.2 (credits explicites, pas la forme close) ;
  ER0    : la meme sans heritage (chaque vote garde son poids, compte des son debut) ;
  VOTE   : vote de la these (section 9.1) rendu hierarchique : partition de l'unite S_tau / T_x sur les faces de x,
           majorite stricte a denominateur fige (une seule fois), avec ou sans cone de marge ;
  ARGMAX : le meme vote applique a chaque niveau (argmax par coupe) : sert a exhiber la non-laminarite.

Chaque regle rend, par site, (t, o) : t = date d'entree en RAYON (Surd exact), o = noeud vivant a t^2 (coupe fermee).
Aucun assert ; tout est Fraction ou Surd.
"""
from fractions import Fraction
from itertools import combinations

from surd import Surd, smax


class RuleError(RuntimeError):
    pass


def need(c, m):
    if not c:
        raise RuleError(m)


# ------------------------------------------------------------------------------------------------ H_m

def qualification(tree, m):
    """q(v) : premier niveau de la vie de v ou son amas discret compte au moins m sites ; None sinon."""
    per = {}
    for x in range(tree.n):
        for v, c in tree.cov[x].items():
            per.setdefault(v, []).append(c)
    out = {}
    for v in range(len(tree)):
        cs = sorted(per.get(v, []))
        q = None
        if len(cs) >= m:
            q = max(cs[m - 1], tree.birth[v])
            if tree.death[v] is not None and q >= tree.death[v]:
                q = None
        out[v] = q
    return out


def hm_point(tree, x, qual):
    starts = []
    for v, c in tree.cov[x].items():
        q = qual[v]
        if q is None:
            continue
        s = max(c, q)
        if tree.death[v] is not None and s >= tree.death[v]:
            continue
        starts.append((s, v))
    need(starts, 'aucun point qualifie (site %d)' % x)
    t = min(s for s, _v in starts)
    p1 = min(v for s, v in starts if s == t)
    D = max(tree.meet(p1, t, v, s) - s for s, v in starts)
    e = t + D
    return e, tree.anc(p1, e)


def rule_hm(tree, m):
    qual = qualification(tree, m)
    out = []
    for x in range(tree.n):
        e, o = hm_point(tree, x, qual)
        out.append((Surd.sqrt(e), o, {'e': e}))
    return out


# ------------------------------------------------------------------------------------------------ ER0h / ER0

def er_votes(tree, x, eta):
    cov = tree.cov[x]
    A = min(cov.values())
    E2 = (1 + eta) * A
    votes = {}
    for v, c in cov.items():
        if c >= E2:
            continue
        d = tree.death[v]
        hi = E2 if d is None else min(d, E2)
        w = hi - c
        if w > 0:
            votes[v] = (c, w)
    return votes, A, E2


def er_credits(tree, votes, heritage, theta=None):
    """Credits [(debut, noeud, credit)] ; heritage=True : ER0h (prorata des masses de sous-arbre).
    theta (variante ER-hv de la v10) : l'enfant c ne recoit que g_c = min(1, S(c) / (theta W)) de sa part, le parent
    garde le reste comme credit propre, compte des son debut."""
    if not heritage:
        return [(c, v, w) for v, (c, w) in votes.items()]
    vparent = {}
    for v in votes:
        u = tree.parent[v]
        while u >= 0 and u not in votes:
            u = tree.parent[u]
        vparent[v] = u if u >= 0 else None
        # definition ecrite : le parent de vote est le parent FULL s'il vote ; on controle l'equivalence
        p = tree.parent[v]
        need(vparent[v] == (p if p >= 0 and p in votes else None), 'parent de vote different du parent FULL')
    kids = dict((v, []) for v in votes)
    for v, p in vparent.items():
        if p is not None:
            kids[p].append(v)
    order = sorted(votes, key=lambda v: tree.birth[v])          # enfants avant parents
    S = {}
    for v in order:
        S[v] = votes[v][1] + sum((S[c] for c in kids[v]), Fraction(0))
    recv = dict((v, Fraction(0)) for v in votes)
    W = sum((w for _c, w in votes.values()), Fraction(0))
    out = []
    for v in reversed(order):                                     # parents avant enfants
        total = votes[v][1] + recv[v]
        if not kids[v]:
            out.append((votes[v][0], v, total))
            continue
        base = S[v] - votes[v][1]
        keep = Fraction(0)
        for c in kids[v]:
            part = total * S[c] / base
            g = Fraction(1) if theta is None else min(Fraction(1), S[c] / (theta * W))
            recv[c] += part * g
            keep += part * (1 - g)
        if keep > 0:
            out.append((votes[v][0], v, keep))
    need(sum((w for _o, _v, w in out), Fraction(0)) == W, 'conservation de W')
    return out


def majority_and_cone(tree, credits, W, A, kappa, relative=False):
    """Majorite stricte a denominateur fige sur des credits [(debut, noeud, credit)], puis cone de marge.
    relative=False : terme sqrt(e) - kappa sqrt(A) mu(e-) (cone de la v10) ;
    relative=True  : terme sqrt(e) - kappa (sqrt(e) - sqrt(A)) mu(e-) (cone RELATIF, propose ici : la remise est
                     proportionnelle a l'attente depuis la premiere couverture, pas a l'echelle sqrt(A)).
    Rend (date Surd, proprietaire, details)."""
    levels = set(o for o, _v, _w in credits)
    for _o, v, _w in credits:
        for u in tree.chain(v):
            levels.add(tree.birth[u])
    levels = sorted(levels)

    def masses(s):
        m = {}
        for o, v, w in credits:
            if o <= s:
                C = tree.anc(v, s)
                m[C] = m.get(C, Fraction(0)) + w
        return m
    T_half = O = None
    for s in levels:
        m = masses(s)
        win = [C for C, mm in m.items() if 2 * mm > W]
        if win:
            need(len(win) == 1, 'deux majorites')
            T_half, O = s, win[0]
            break
    need(T_half is not None, 'aucune majorite')
    date = Surd.sqrt(T_half)
    terms = []
    if kappa is not None:
        a = Surd.sqrt(A)
        G_prev = masses(T_half)[O]
        for s in levels:
            if s <= T_half:
                continue
            Gs = masses(s).get(tree.anc(O, s), Fraction(0))
            need(Gs >= G_prev, 'masse de lignee decroissante')
            if Gs > G_prev and G_prev < W:
                mu = 2 * G_prev / W - 1
                if relative:
                    val = Surd.sqrt(s) - (Surd.sqrt(s) - a).scale(kappa * mu)
                else:
                    val = Surd.sqrt(s) - a.scale(kappa * mu)
                terms.append((s, G_prev, mu))
                if val.cmp(date) > 0:
                    date = val
            G_prev = Gs
    owner = tree.anc_surd(O, date)
    return date, owner, {'T_half': T_half, 'O': O, 'terms': terms}


def rule_er(tree, eta, kappa, heritage=True, theta=None, relative=False):
    eta, kappa = Fraction(eta), Fraction(kappa)
    out = []
    for x in range(tree.n):
        votes, A, E2 = er_votes(tree, x, eta)
        need(votes, 'aucun vote')
        W = sum((w for _c, w in votes.values()), Fraction(0))
        cr = er_credits(tree, votes, heritage, None if theta is None else Fraction(theta))
        date, owner, det = majority_and_cone(tree, cr, W, A, kappa, relative)
        det.update({'A': A, 'E2': E2, 'W': W, 'votes': votes, 'credits': cr})
        out.append((date, owner, det))
    return out


def rule_er0h(tree, eta=1, kappa=12):
    return rule_er(tree, eta, kappa, True)


def rule_er0(tree, eta=1, kappa=12):
    return rule_er(tree, eta, kappa, False)


def rule_er0hv(tree, eta=1, kappa=12, theta=Fraction(1, 20)):
    return rule_er(tree, eta, kappa, True, theta)


def rule_er0hr(tree, eta=1, kappa=10):
    """ER0h a cone relatif (votes, poids, heritage et majorite de ER0h ; remise kappa (sqrt(e) - sqrt(A)) mu)."""
    return rule_er(tree, eta, kappa, True, None, True)


# ------------------------------------------------------------------------------------------------ vote de la these

def psi(beta, p):
    """psi(rho) = rho^(-p) avec rho = sqrt(beta) ; p pair pour rester rationnel."""
    need(p % 2 == 0 and p >= 0, 'exposant pair attendu')
    if p == 0:
        return Fraction(1)
    need(beta > 0, 'rayon nul')
    return 1 / beta ** (p // 2)


def gabriel(d, sigma):
    """Def. 28 de la these : l'interieur de la plus petite boule de sigma ne contient aucun point hors de sigma."""
    level, center, _closed = d.meb(tuple(sorted(sigma)))
    for i, pt in enumerate(d.points):
        if i in sigma:
            continue
        if sum((Fraction(pt[j]) - center[j]) ** 2 for j in range(3)) < level:
            return False
    return True


def face_data(d, k, p, faces):
    """S_tau pour les faces de F_K. faces='all' : toutes les k-parties et toutes les (k+1)-parties ;
    faces='gabriel' : facettes des k-simplexes de Gabriel (Def. 29) et cofaces de Gabriel seulement."""
    n = d.n
    d.order(k)
    vnode = d._vertex_node[k]
    parts = list(combinations(range(n), k))
    beta = dict((t, d.meb(t)[0]) for t in parts)
    S = dict((t, Fraction(0)) for t in parts)
    inF = dict((t, faces == 'all') for t in parts)
    if k < n:
        for sigma in combinations(range(n), k + 1):
            if faces == 'gabriel' and not gabriel(d, set(sigma)):
                continue
            w = psi(d.meb(sigma)[0], p)
            for j in range(k + 1):
                t = sigma[:j] + sigma[j + 1:]
                S[t] += w
                inF[t] = True
    if k == 1:
        for t in parts:
            inF[t] = True
    return parts, beta, S, inF, vnode


def vote_credits(d, k, x, data):
    parts, beta, S, inF, vnode = data
    mine = [t for t in parts if x in t and inF[t]]
    T = sum((S[t] for t in mine), Fraction(0))
    if T == 0:
        # convention de la these 1/T_x = 0 : aucune masse ; on vote alors uniformement (repli declare)
        return [(beta[t], vnode[t], Fraction(1, len(mine))) for t in mine], True
    return [(beta[t], vnode[t], S[t] / T) for t in mine if S[t] > 0], False


def rule_vote(d, tree, k, p=0, faces='all', kappa=None):
    data = face_data(d, k, p, faces)
    out = []
    for x in range(tree.n):
        cr, repli = vote_credits(d, k, x, data)
        need(cr, 'aucune face pour le site %d' % x)
        A = min(o for o, _v, _w in cr)
        date, owner, det = majority_and_cone(tree, cr, Fraction(1), A, kappa)
        det.update({'credits': cr, 'repli': repli, 'A': A})
        out.append((date, owner, det))
    return out


def argmax_trace(d, tree, k, x, data):
    """Argmax du vote a chaque niveau d'evenement (version 'a chaque niveau') : [(niveau, gagnants)]."""
    cr, _repli = vote_credits(d, k, x, data)
    levels = set(o for o, _v, _w in cr)
    for _o, v, _w in cr:
        for u in tree.chain(v):
            levels.add(tree.birth[u])
    trace = []
    for s in sorted(levels):
        m = {}
        for o, v, w in cr:
            if o <= s:
                C = tree.anc(v, s)
                m[C] = m.get(C, Fraction(0)) + w
        if not m:
            continue
        best = max(m.values())
        trace.append((s, sorted(C for C, mm in m.items() if mm == best), m))
    return trace


def argmax_violation(tree, trace):
    """Premiere paire de niveaux s1 < s2 ou le gagnant unique a s2 n'est pas l'ancetre vivant du gagnant a s1."""
    for i in range(len(trace)):
        s1, g1, _m1 = trace[i]
        if len(g1) != 1:
            continue
        for j in range(i + 1, len(trace)):
            s2, g2, _m2 = trace[j]
            if len(g2) != 1:
                continue
            if tree.anc(g1[0], s2) != g2[0]:
                return (s1, g1[0], s2, g2[0])
    return None


# ------------------------------------------------------------------------------------------------ hierarchies

def ultrametric(tree, hang):
    """u(i, j) en RAYON (Surd) : max(t_i, t_j, sqrt(niveau du LCA) si le LCA n'est ni o_i ni o_j)."""
    n = len(hang)
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        ti, oi = hang[i][0], hang[i][1]
        u[i][i] = ti
        for j in range(i + 1, n):
            tj, oj = hang[j][0], hang[j][1]
            w = tree.lca(oi, oj)
            cands = [ti, tj]
            if w not in (oi, oj):
                cands.append(Surd.sqrt(tree.birth[w]))
            u[i][j] = u[j][i] = smax(cands)
    return u


def blocks_at(u, r):
    """Blocs (sites entres a r, reunis a u <= r) au rayon r (Surd)."""
    n = len(u)
    active = [i for i in range(n) if u[i][i].cmp(r) <= 0]
    out, seen = [], set()
    for i in active:
        if i in seen:
            continue
        b = frozenset(j for j in active if u[i][j].cmp(r) <= 0)
        seen |= b
        out.append(b)
    return sorted((sorted(b) for b in out))


def change_radii(u):
    vals = []
    n = len(u)
    for i in range(n):
        for j in range(i, n):
            vals.append(u[i][j])
    vals.sort(key=float)
    out = []
    for v in vals:
        if out and out[-1].cmp(v) == 0:
            continue
        out.append(v)
    out.sort(key=float)
    return out


def hierarchy_text(u, names, mcs=2):
    """Suite des blocs d'au moins mcs sites, par rayon de changement : [(rayon flottant, 'AB|CD')]."""
    out = []
    prev = None
    for r in change_radii(u):
        bl = [b for b in blocks_at(u, r) if len(b) >= mcs]
        cur = '|'.join(sorted(''.join(names[i] for i in b) if all(len(nm) == 1 for nm in names)
                              else '{' + ','.join(names[i] for i in b) + '}' for b in bl))
        if cur != prev:
            out.append((round(float(r), 3), cur))
            prev = cur
    return out
