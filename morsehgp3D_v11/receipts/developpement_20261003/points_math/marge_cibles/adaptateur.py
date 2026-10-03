#!/usr/bin/env python3
"""Adaptateur : regles de pendaison v11 (oracle exact bench/points_reference.py) -> juge condense v10.

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
public_status=not_claimed. GCP non utilise. Lecture seule sur le depot, le worktree v11 et les dossiers v10 :
aucun bytecode ecrit (sys.dont_write_bytecode avant tout import), aucune ecriture hors de ce dossier.

Trois routes, aucune decision flottante :
  route A (oracle v11)  : hgp11_ref.Definition(P).order(K), puis points_reference.reference_rules(res, n, m) ;
                          ultrametrique exacte u(i, j) en niveaux carres (Fraction) par reference_ultrametric ;
  route B (ici, v10)    : H_m reecrite depuis sa definition (docs/HIERARCHIE_POINTS.md, section 3) sur l'arbre
                          Gamma_K de la v10 (vfull.full_gamma, couverture T.cov[x] = {v : c_x(v)}), en niveau carre
                          ou en rayon (variante « marge en rayon ») ;
  juge (v10, inchange)  : cellules.juger_cellule / ver.juger (semantique condensee de CIBLES_REVISEES.md 1.1) sur
                          un objet UHier qui expose partition(r) et rayons_changement() comme ver.Hier.
Le juge ne lit que les partitions aux coupes fermees : deux sites actifs i, j sont dans le meme bloc au rayon r
si et seulement si u(i, j) <= r^2 ; un site non entre (u(i, i) > r^2) est un singleton (comme ver.Hier).
"""
import os
import sys

sys.dont_write_bytecode = True
os.environ.setdefault('MHGP10_FIXTURES_SCRATCH', '/workspaces/E-HGP/build/v11-points-math/marge_cibles/scratch')

import importlib.util  # noqa: E402
from fractions import Fraction  # noqa: E402

V10_VERDICT = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict'
V10_VER = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'
V11_BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
for _p in (V10_VER, V11_BENCH):
    if _p not in sys.path:
        sys.path.insert(0, _p)

_spec = importlib.util.spec_from_file_location('cellules_v10', os.path.join(V10_VERDICT, 'cellules.py'))
CL = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(CL)
ver = CL.ver
R = CL.R

import points_reference as PR  # noqa: E402  (route A)
from hgp11_ref import Definition  # noqa: E402


class AdaptErreur(RuntimeError):
    pass


def exiger(c, m):
    if not c:
        raise AdaptErreur(m)


# ------------------------------------------------------------------ hierarchie lue sur une ultrametrique

class UHier:
    """Hierarchie de points donnee par son ultrametrique exacte.

    kind 'sq' : U[i][j] Fraction, niveau carre ; kind 'r' : U[i][j] objet R (rayon, somme de racines)."""

    def __init__(self, U, kind, nom=''):
        self.U, self.kind, self.nom = U, kind, nom
        self.n = len(U)
        if kind == 'sq':
            self.dates = [R.rac(U[i][i]) for i in range(self.n)]
        else:
            self.dates = [U[i][i] for i in range(self.n)]
        self._chg = None

    def _le(self, r):
        if self.kind == 'sq':
            q = r.carre_si_racine()
            if q is not None:
                return lambda u: u <= q
            return lambda u: R.rac(u).cmp(r) <= 0
        return lambda u: u.cmp(r) <= 0

    def partition(self, r):
        le = self._le(r)
        n = self.n
        actif = [le(self.U[i][i]) for i in range(n)]
        parent = list(range(n))

        def trouver(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        for i in range(n):
            if not actif[i]:
                continue
            for j in range(i + 1, n):
                if actif[j] and le(self.U[i][j]):
                    a, b = trouver(i), trouver(j)
                    if a != b:
                        parent[b] = a
        groupes = {}
        for i in range(n):
            cle = ('c', trouver(i)) if actif[i] else ('s', i)
            groupes.setdefault(cle, []).append(i)
        return tuple(sorted(tuple(sorted(g)) for g in groupes.values()))

    def _valeur_r(self, u):
        return R.rac(u) if self.kind == 'sq' else u

    def rayons_changement(self):
        if self._chg is None:
            if self.kind == 'sq':
                vals = sorted(set(self.U[i][j] for i in range(self.n) for j in range(i, self.n)))
                self._chg = [R.rac(v) for v in vals]
            else:
                evs = [self.U[i][j] for i in range(self.n) for j in range(i, self.n)]
                evs.sort(key=float)
                out = []
                for e in evs:
                    i = len(out)
                    while i > 0 and out[i - 1].cmp(e) > 0:
                        i -= 1
                    if i > 0 and out[i - 1].cmp(e) == 0:
                        continue
                    if i < len(out) and out[i].cmp(e) == 0:
                        continue
                    out.insert(i, e)
                self._chg = out
        return self._chg

    def valider(self):
        """Ultrametrique : u(i, j) >= max(u(i, i), u(j, j)) et u(i, j) <= max(u(i, l), u(l, j)) (laminarite)."""
        n, U = self.n, self.U
        cmp = (lambda a, b: (a > b) - (a < b)) if self.kind == 'sq' else (lambda a, b: a.cmp(b))

        def mx(a, b):
            return a if cmp(a, b) >= 0 else b
        c = 0
        for i in range(n):
            for j in range(n):
                exiger(cmp(U[i][j], U[j][i]) == 0, 'u non symetrique')
                exiger(cmp(U[i][j], mx(U[i][i], U[j][j])) >= 0, 'u(i,j) < date')
                for l in range(n):
                    exiger(cmp(U[i][j], mx(U[i][l], U[l][j])) <= 0, 'inegalite ultrametrique violee')
                    c += 1
        return c


# ------------------------------------------------------------------ route A : oracle v11

_DEF = {}


def definition(P):
    cle = tuple(P)
    if cle not in _DEF:
        _DEF[cle] = Definition(list(P))
    return _DEF[cle]


def route_a(P, K, m):
    """{regle: (entrees, U)} de l'oracle v11 pour un m donne ; regles core, cover, first, margin1, margin."""
    d = definition(P)
    res = d.order(K)
    ref, tree = PR.reference_rules(res, len(P), m)
    out = {}
    for rule in PR.RULES:
        out[rule] = (ref[rule], PR.reference_ultrametric(ref[rule], tree))
    return out, res, tree


# ------------------------------------------------------------------ route B : H_m reecrite sur l'arbre Gamma_K v10

def _lca(T, a, b):
    anc = set(T.ancetres(a))
    for w in T.ancetres(b):
        if w in anc:
            return w
    raise AdaptErreur('foret')


def route_b(T, m, mode='sq', kappa=1):
    """H_m sur vfull.Arbre (famille H4 de pente kappa >= 1 ; kappa = 1 : H_m). mode 'sq' : marge en niveau carre ;
    mode 'r' : meme regle, niveaux remplaces par les rayons (marge en rayon).
    Rend (dates, owners, U) ; U en Fraction (sq) ou en R (r)."""
    n = T.n
    adm = ver.admissibilite(T, m)  # a(v) = m-ieme plus petit c_y(v) s'il est < d_v (qualification)
    entrees = []
    for x in range(n):
        s = {}
        for v, c in T.cov[x].items():
            a = adm[v]
            if a is None:
                continue
            o = max(c, a)
            if T.death[v] is not None and o >= T.death[v]:
                continue
            s[v] = o
        exiger(s, 'site jamais couvert par un noeud qualifie')
        t = min(s.values())
        o = min(v for v in s if s[v] == t)  # tout choix convient (H1, recoupe par la route A)
        anc_o = set(T.ancetres(o))
        if mode == 'sq':
            D = Fraction(0)
            for v, sv in s.items():
                if v in anc_o:
                    continue
                w = _lca(T, o, v)
                if w in (o, v):  # meme lignee : terme max(t, s_v) - s_v = 0
                    continue
                term = T.birth[w] - kappa * (sv - t) - t  # famille H4 : e = sup_q m(p, q) - kappa (h(q) - t)
                if term > D:
                    D = term
            e = t + D
            own = o
            while T.parent[own] >= 0 and T.birth[T.parent[own]] <= e:
                own = T.parent[own]
            entrees.append((e, own))
        else:
            D = R()
            for v, sv in s.items():
                if v in anc_o:
                    continue
                w = _lca(T, o, v)
                if w in (o, v):
                    continue
                term = R.rac(T.birth[w]) - (R.rac(sv) - R.rac(t)).mul(kappa) - R.rac(t)
                if term.cmp(D) > 0:
                    D = term
            e = R.rac(t) + D
            own = o
            while T.parent[own] >= 0 and R.rac(T.birth[T.parent[own]]).cmp(e) <= 0:
                own = T.parent[own]
            entrees.append((e, own))
    U = ultrametrique_arbre(T, entrees, mode)
    return entrees, U


def ultrametrique_arbre(T, entrees, mode):
    n = len(entrees)
    U = [[None] * n for _ in range(n)]
    for i in range(n):
        ei, oi = entrees[i]
        U[i][i] = ei
        for j in range(i + 1, n):
            ej, oj = entrees[j]
            w = _lca(T, oi, oj)
            if mode == 'sq':
                val = max(ei, ej)
                if w not in (oi, oj):
                    val = max(val, T.birth[w])
            else:
                val = ei if ei.cmp(ej) >= 0 else ej
                if w not in (oi, oj):
                    b = R.rac(T.birth[w])
                    if b.cmp(val) > 0:
                        val = b
            U[i][j] = U[j][i] = val
    return U


def u_de_hier(h):
    """Ultrametrique (R) d'une ver.Hier (dates R, proprietaires de T) : sert a valider l'adaptateur sur ER0h."""
    T = h.T
    ent = [(h.dates[x], h.owners[x]) for x in range(h.n)]
    return ultrametrique_arbre(T, ent, 'r')


def egal_u(U1, k1, U2, k2):
    """Egalite exacte de deux ultrametriques (Fraction carree ou R)."""
    n = len(U1)
    for i in range(n):
        for j in range(n):
            a, b = U1[i][j], U2[i][j]
            if k1 == k2 == 'sq':
                if a != b:
                    return False
            else:
                ra = R.rac(a) if k1 == 'sq' else a
                rb = R.rac(b) if k2 == 'sq' else b
                if ra.cmp(rb) != 0:
                    return False
    return True
