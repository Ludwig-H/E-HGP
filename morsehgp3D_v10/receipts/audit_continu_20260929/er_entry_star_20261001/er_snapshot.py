#!/usr/bin/env python3
"""ER : appartenance relative a l'echelle, sur FULL_K et la relation de couverture, avec admissibilite par
min_cluster_size (mcs), votes de noeuds ponderes par leur temps de couverture dans la bande, majorite stricte a
denominateur fige et cone de marge. Regle ancree : date t(x), proprietaire o(x) vivant a t(x).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=juge_final_echelle_relative, public_status=not_claimed. GCP non utilise. Aucun moteur modifie. La bibliotheque
partagee fixtures_cibles/lib et le worktree partage sont IMPORTES en lecture seule (pas de bytecode).

Donnees (intrinseques : FULL_K et relation de couverture seulement) :
  T arbre de fusion (naissance b_v, mort d_v, parent) ; cov[x] = {v : c_x(v)} (premier niveau ou v couvre x pendant
  sa vie ; clos vers le haut) ; alpha2[x] = min_v c_x(v) ; inc[v] = [(c_y(v), y)] tries (amas discret de v).

Definition (niveaux = rayons carres rationnels ; dates = rayons, sommes exactes de racines) :
  1. Entrees de x : noeuds minimaux de V_x (aucun enfant ne couvre x pendant sa vie).
  2. Echelle d'une entree u : Z = {y != x : c_y(u) <= c_x(u)} (co-couverts par u au plus tard avec x) ;
     sigma = max_{y in Z} alpha2[y] ; rapport rho = c_x(u) / sigma >= 1. Entree NATURELLE ssi rho <= Lambda
     (Lambda = lambda^2, lambda rationnel en rayon).
  3. nat_x(v) = 1 ssi une entree naturelle de x est sous v (v compris).
  4. Admissibilite : a(v) = mcs-ieme plus petit c_y(v) (l'amas discret de v atteint mcs points), s'il est < d_v.
  5. Votes : v de V_x, nat_x(v) = 1, a(v) defini ; debut o_v = max(c_x(v), a(v)) < d_v. Ancre A = min o_v ;
     bande E2 = (1 + eta) A ; poids omega_v = min(d_v, E2) - o_v (temps de couverture dans la bande, > 0).
  6. Masse a la coupe fermee s : m(C, s) = somme des omega_v des votes debutes (o_v <= s) dont l'ancetre vivant a s
     est C. W = somme de tous les omega (figee). T_1/2 = premier s ou une composante porte 2 m > W ; O = elle.
  7. Date t = max( sqrt(T_1/2), max_e ( sqrt(e) - kappa sqrt(A) (2 G(e-)/W - 1) ) ), e parcourant les niveaux
     > T_1/2 ou la masse G de la lignee de O saute, avec G(e-) < W ; proprietaire = ancetre de O vivant a t.
  8. Repli : aucun vote naturel admissible -> meme regle sans le filtre d'echelle ; aucun noeud admissible (n < mcs)
     -> date = premiere couverture par la racine, proprietaire la racine.

Aucun assert : comportement identique sous python3 -O. Tout est entier, Fraction ou QS.
"""
from bisect import bisect_right
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
SCR = '/tmp/mhgp10-juge-final/echelle_relative'
os.environ.setdefault('MHGP10_FIXTURES_SCRATCH', os.path.join(SCR, 'lib_scratch'))
LIB = '/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib'
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import regles as RG  # noqa: E402  (bibliotheque partagee, lecture seule)

QS = RG.QS
Rayon = RG.Rayon

CADRE = ('phase=exploration_v10_hors_registre backend=cpu_reference profile=quantized_u18_input_only '
         'mode=juge_final_echelle_relative public_status=not_claimed ; GCP non utilise')

VARIANTES = ('sans_echelle', 'sans_admissibilite', 'sans_cone', 'votes_unitaires', 'majorite_large',
             'proprietaire_O', 'ancre_alpha', 'sigma_min', 'repli_racine')


class ERErreur(RuntimeError):
    """Invariant viole (jamais un assert)."""


def exiger(cond, msg):
    if not cond:
        raise ERErreur(msg)


def fr(v):
    if isinstance(v, Fraction):
        return v
    if isinstance(v, int) and not isinstance(v, bool):
        return Fraction(v)
    if isinstance(v, str):
        return Fraction(v)
    raise ERErreur('rationnel exact attendu : %r' % (v,))


# ------------------------------------------------------------------ structure intrinseque d'une scene

class StructureER:
    """FULL_K natif (meme foret que la scene) et relation de couverture, indexes par POINT (identifiant de scene)."""

    def __init__(self, sc, agregats=('max', 'min')):
        self.sc = sc
        T, cover, _info = sc.arbre_masses()
        self.T = T
        n = sc.n
        cov = [None] * n
        for s, cv in enumerate(cover):
            cov[sc.ctx.point_id[s]] = dict(cv)
        for p in range(n):
            exiger(cov[p], 'point %s jamais couvert' % sc.noms[p])
        self.cov = cov
        self.alpha2 = [min(cov[p].values()) for p in range(n)]
        inc = {}
        for p in range(n):
            for v, c in cov[p].items():
                inc.setdefault(v, []).append((c, p))
        for v in inc:
            inc[v].sort()
        self.inc = inc
        # par noeud : niveaux de couverture tries et deux meilleurs alpha2 de chaque prefixe (max et min), pour
        # l'echelle des entrees en O(log) sans parcourir l'amas discret
        self._cs, self._top = {}, {agr: {} for agr in agregats}
        for v, lst in inc.items():
            self._cs[v] = [c for c, _y in lst]
            for agr in agregats:
                b1 = b2 = None
                tops = []
                for _c, y in lst:
                    cand = (self.alpha2[y], y)
                    if b1 is None or (cand[0] > b1[0] if agr == 'max' else cand[0] < b1[0]):
                        b1, b2 = cand, b1
                    elif b2 is None or (cand[0] > b2[0] if agr == 'max' else cand[0] < b2[0]):
                        b2 = cand
                    tops.append((b1, b2))
                self._top[agr][v] = tops
        self.entrees = []
        for p in range(n):
            cv = cov[p]
            self.entrees.append(sorted((v for v in cv if not any(ch in cv for ch in T.children[v])),
                                       key=lambda v: (cv[v], v)))
        # ordre d'Euler (prefixe) pour la selection mediane
        tin = [0] * len(T)
        horloge = 0
        pile = [T.root]
        while pile:
            v = pile.pop()
            tin[v] = horloge
            horloge += 1
            for c in sorted(T.children[v], reverse=True):
                pile.append(c)
        self.tin = tin
        self._adm = {}
        self._sig = {}
        self.n = n

    # -- echelle des entrees
    def sigma2(self, p, u, agregat='max'):
        """Echelle de l'entree u de p : max (ou min) des alpha2 des points co-couverts par u au plus tard avec p
        (Z = {y != p : c_y(u) <= c_p(u)}). Rend (sigma2, |Z|). Prefixe trie des niveaux de u, deux meilleurs."""
        cle = (p, u, agregat)
        if cle not in self._sig:
            c = self.cov[p][u]
            k = bisect_right(self._cs[u], c) - 1
            exiger(k >= 0, 'entree %d du point %s : prefixe vide' % (u, self.sc.noms[p]))
            b1, b2 = self._top[agregat][u][k]
            best = b1 if b1[1] != p else b2
            exiger(best is not None, 'entree %d du point %s sans co-couvert' % (u, self.sc.noms[p]))
            self._sig[cle] = (best[0], k)
        return self._sig[cle]

    def co_couverts(self, p, u):
        """Liste explicite Z (lecture, controle) : points y != p avec c_y(u) <= c_p(u)."""
        c = self.cov[p][u]
        return [y for cy, y in self.inc[u] if cy <= c and y != p]

    def rapports(self, p, agregat='max'):
        """[(entree, c, sigma2, rho = c / sigma2, |Z|)] pour les entrees de p (rho en niveaux)."""
        out = []
        for u in self.entrees[p]:
            s2, nz = self.sigma2(p, u, agregat)
            out.append((u, self.cov[p][u], s2, self.cov[p][u] / s2, nz))
        return out

    # -- admissibilite
    def onset_adm(self, mcs):
        """{v : a(v)} : niveau ou l'amas discret de v atteint mcs points (dans sa vie), ou None."""
        if mcs not in self._adm:
            T = self.T
            out = {}
            for v in range(len(T)):
                lst = self.inc.get(v, [])
                a = None
                if len(lst) >= mcs:
                    c = lst[mcs - 1][0]
                    if T.death[v] is None or c < T.death[v]:
                        a = c
                out[v] = a
            self._adm[mcs] = out
        return self._adm[mcs]

    # -- arbre
    def anc_rayon(self, v, t):
        """Ancetre de v vivant au rayon exact t (Rayon) ; v doit etre ne a t."""
        T = self.T
        exiger(Rayon(b2=T.birth[v]).cmp(t) <= 0, 'noeud non ne a la date')
        while T.parent[v] >= 0 and Rayon(b2=T.birth[T.parent[v]]).cmp(t) <= 0:
            v = T.parent[v]
        return v


# ------------------------------------------------------------------ noyau : un point

def _majorite(T, votes, W, large=False):
    """Premiere majorite stricte (2 m > W ; 2 m >= W si large) en coupe fermee. votes : [(o, v, w)].
    Rend (T_1/2, O, masse de O a T_1/2) ou (None, None, None)."""
    niveaux = set(o for o, _v, _w in votes)
    for _o, v, _w in votes:
        u = v
        while T.parent[u] >= 0:
            u = T.parent[u]
            niveaux.add(T.birth[u])
    for s in sorted(niveaux):
        masses = {}
        for o, v, w in votes:
            if o <= s:
                C = T.anc(v, s)
                masses[C] = masses.get(C, Fraction(0)) + w
        gagnants = [C for C, m in masses.items() if (2 * m >= W if large else 2 * m > W)]
        if gagnants:
            if len(gagnants) > 1:
                # seulement possible avec la majorite large (mutant) : egalite exacte entre deux composantes
                gagnants.sort(key=lambda C: (-masses[C], C))
            return s, gagnants[0], masses[gagnants[0]]
    return None, None, None


def er_point(st, p, Lam, eta, kappa, mcs, variante=None):
    """Date (QS, rayon), proprietaire et details de ER pour le point p de la structure st.
    Lam : seuil sur rho (niveaux), Fraction ou None (pas de filtre) ; eta, kappa : Fraction ; mcs : entier >= 1."""
    T = st.T
    cv = st.cov[p]
    agr = 'min' if variante == 'sigma_min' else 'max'
    if variante == 'sans_echelle':
        Lam = None
    if variante == 'sans_admissibilite':
        mcs = 1
    adm = st.onset_adm(mcs)
    rap = st.rapports(p, agr)
    nat_entrees = [u for u, _c, _s2, rho, _Z in rap if Lam is None or rho <= Lam]

    def votes_de(naturels):
        nat = set()
        for u in naturels:
            w = u
            while w >= 0 and w not in nat:
                nat.add(w)
                w = T.parent[w]
        cands = []
        for v in nat:
            a = adm.get(v)
            if a is None:
                continue
            o = max(cv[v], a)
            if T.death[v] is not None and o >= T.death[v]:
                continue
            cands.append((o, v))
        return cands

    mode = 'naturel'
    cands = votes_de(nat_entrees)
    if not cands:
        if variante == 'repli_racine':
            cands = []
            mode = 'repli_racine'
        else:
            cands = votes_de([u for u, _c, _s2, _r, _Z in rap])
            mode = 'repli_sans_echelle'
    racine = T.root
    if not cands:
        c_r = cv[racine]
        a_r = adm.get(racine)
        o = c_r if a_r is None else max(c_r, a_r)
        date = QS.sqrt(o)
        return {'date': date, 'owner': racine, 'mode': mode if mode == 'repli_racine' else 'aucun_admissible',
                'A': o, 'E2': o, 'W': Fraction(0), 'T_half': o, 'O': racine, 'votes': [], 'termes': [],
                'arg': ('racine', o), 'rapports': rap, 'T1': o, 'h': []}
    A = min(o for o, _v in cands)
    if variante == 'ancre_alpha':
        A = st.alpha2[p]
    E2 = (1 + eta) * A
    votes = []
    for o, v in cands:
        hi = E2 if T.death[v] is None else min(T.death[v], E2)
        w = hi - o
        if w > 0:
            votes.append((o, v, Fraction(1) if variante == 'votes_unitaires' else w))
    if not votes:
        # seulement avec l'ancre alpha (mutant) : aucun vote dans la bande -> repli racine
        c_r = cv[racine]
        return {'date': QS.sqrt(c_r), 'owner': racine, 'mode': 'bande_vide', 'A': A, 'E2': E2, 'W': Fraction(0),
                'T_half': c_r, 'O': racine, 'votes': [], 'termes': [], 'arg': ('racine', c_r), 'rapports': rap,
                'T1': c_r, 'h': []}
    W = sum((w for _o, _v, w in votes), Fraction(0))
    if variante == 'majorite_large':
        T_half, O, mO = _majorite(T, votes, W, large=True)
        exiger(T_half is not None, 'aucune majorite (point %s)' % st.sc.noms[p])
        h = sorted(((max(o, T.birth[T.lca(v, O)]), w, v) for o, v, w in votes), key=lambda t3: t3[0])
    else:
        # deux selections ponderees (auditeur continu) : m = atome median en ordre d'Euler (cumul > W/2 strict) ;
        # h_v = max(o_v, b(LCA(v, m))) ; T_1/2 = premier quantile des h dont le cumul depasse W/2 ; O = anc(m).
        # Pour s >= T_1/2, anc_s(m) = anc_s(O) : les h servent aussi au cone.
        m = None
        cum = Fraction(0)
        for _o, v, w in sorted(votes, key=lambda t3: st.tin[t3[1]]):
            cum += w
            if 2 * cum > W:
                m = v
                break
        h = sorted(((max(o, T.birth[T.lca(v, m)]), w, v) for o, v, w in votes), key=lambda t3: t3[0])
        T_half = None
        cum = Fraction(0)
        i = 0
        while i < len(h):
            e = h[i][0]
            while i < len(h) and h[i][0] == e:
                cum += h[i][1]
                i += 1
            if 2 * cum > W:
                T_half = e
                break
        exiger(T_half is not None and m is not None, 'aucune majorite (point %s)' % st.sc.noms[p])
        O = T.anc(m, T_half)
    a = QS.sqrt(A)
    date = QS.sqrt(T_half)
    arg = ('T_half', T_half)
    termes = []
    T1 = None
    cum = Fraction(0)
    i = 0
    while i < len(h):
        e = h[i][0]
        G_moins = cum
        while i < len(h) and h[i][0] == e:
            cum += h[i][1]
            i += 1
        if T1 is None and cum == W:
            T1 = e
        if e <= T_half or G_moins >= W:
            continue
        mu = 2 * G_moins / W - 1
        exiger(mu > 0 or variante == 'majorite_large', 'marge non positive apres T_1/2')
        val = QS.sqrt(e) - a * (kappa * mu)
        termes.append((e, mu))
        if variante != 'sans_cone' and val.cmp(date) > 0:
            date, arg = val, ('saut', e)
    exiger(T1 is not None, 'unanimite jamais atteinte (point %s)' % st.sc.noms[p])
    rd = Rayon.somme(date)
    owner = O if variante == 'proprietaire_O' else st.anc_rayon(O, rd)
    return {'date': date, 'owner': owner, 'mode': mode, 'A': A, 'E2': E2, 'W': W, 'T_half': T_half, 'O': O,
            'votes': votes, 'termes': termes, 'arg': arg, 'rapports': rap, 'T1': T1, 'h': h}


# ------------------------------------------------------------------ regle complete

class RegleER:
    def __init__(self, nom, h, details, params):
        self.nom, self.h, self.details, self.params = nom, h, details, params


def nom_regle(lam, eta, kappa, mcs, variante=None):
    base = 'ER[l=%s,e=%s,k=%s,mcs=%s]' % (fr(lam) if lam is not None else 'inf', fr(eta), fr(kappa), mcs)
    return base if not variante else base + '~' + variante


def regle_er(st, lam, eta, kappa, mcs, variante=None, valider=True):
    """ER sur la structure st (StructureER). lam : rationnel en rayon (Lambda = lam^2) ou None (sans filtre)."""
    exiger(variante is None or variante in VARIANTES, 'variante inconnue : %s' % variante)
    eta, kappa = fr(eta), fr(kappa)
    exiger(eta > 0 and kappa > 0, 'parametres non positifs')
    exiger(isinstance(mcs, int) and not isinstance(mcs, bool) and mcs >= 1, 'mcs entier >= 1 attendu')
    Lam = None if lam is None else fr(lam) ** 2
    exiger(Lam is None or Lam >= 1, 'lambda >= 1 attendu')
    sc = st.sc
    res = [er_point(st, p, Lam, eta, kappa, mcs, variante) for p in range(sc.n)]
    dates = [Rayon.somme(r['date']) for r in res]
    owners = [r['owner'] for r in res]
    nom = nom_regle(lam, eta, kappa, mcs, variante)
    h = RG.Hierarchie(sc, nom, 'echelle_relative', sc.foret, dates, owners,
                      {'lambda': None if lam is None else str(fr(lam)), 'eta': str(eta), 'kappa': str(kappa),
                       'mcs': mcs, 'variante': variante})
    if valider:
        h.valider()
    return RegleER(nom, h, res, {'lam': lam, 'Lam': Lam, 'eta': eta, 'kappa': kappa, 'mcs': mcs,
                                 'variante': variante})


# ------------------------------------------------------------------ controles independants

def _anc_marche(T, v, s):
    """Ancetre vivant a s par marche explicite (naissance <= s < mort), independant de Tree.anc."""
    while True:
        d = T.death[v]
        if d is None or s < d:
            return v
        v = T.parent[v]


def _lca_marche(T, u, v):
    au = []
    w = u
    while w >= 0:
        au.append(w)
        w = T.parent[w]
    s = set(au)
    w = v
    while w not in s:
        w = T.parent[w]
    return w


def verifier_point(st, p, r, kappa):
    """Controle independant d'un point par la definition : balayage de tous les niveaux utiles (debuts, naissances
    des ancetres, milieux, bord de bande) ; aucune majorite stricte avant T_1/2, majorite tenue ensuite par la lignee
    de O ; date = sup des termes du cone (forme sup), atteinte. Rend le nombre de controles."""
    T = st.T
    votes = r['votes']
    if not votes:
        return 0
    W = r['W']
    A = r['A']
    a = QS.sqrt(A)
    niveaux = set(o for o, _v, _w in votes) | {A, r['E2'], r['T_half']}
    for _o, v, _w in votes:
        u = v
        while u >= 0:
            niveaux.add(T.birth[u])
            u = T.parent[u]
    niveaux = sorted(niveaux)
    pts = list(niveaux) + [(x + y) / 2 for x, y in zip(niveaux, niveaux[1:])]
    n = 0

    def G_de(s):
        masses = {}
        for o, v, w in votes:
            if o <= s:
                C = _anc_marche(T, v, s)
                masses[C] = masses.get(C, Fraction(0)) + w
        if not masses:
            return Fraction(0), None
        C = max(masses, key=lambda c: (masses[c], -c))
        return masses[C], C

    for s in sorted(pts):
        G, C = G_de(s)
        if s < r['T_half']:
            exiger(2 * G <= W, 'controle : majorite stricte avant T_1/2 (point %s)' % st.sc.noms[p])
        else:
            exiger(2 * G > W, 'controle : pas de majorite apres T_1/2 (point %s)' % st.sc.noms[p])
            exiger(_anc_marche(T, r['O'], s) == C, 'controle : tete hors de la lignee de O (point %s)'
                   % st.sc.noms[p])
        n += 1
    # forme sup : termes aux sauts de G apres T_1/2, G(e-) = G au niveau precedent (constante entre evenements)
    t = r['date']
    atteint = QS.sqrt(r['T_half']).cmp(t) == 0
    prec = None
    for s in niveaux:
        if s > r['T_half'] and prec is not None:
            Gm, _ = G_de(prec)
            Gs, _ = G_de(s)
            if Gs > Gm and Gm < W:
                val = QS.sqrt(s) - a * (kappa * (2 * Gm / W - 1))
                exiger(val.cmp(t) <= 0, 'controle : sup viole en %s (point %s)' % (s, st.sc.noms[p]))
                if val.cmp(t) == 0:
                    atteint = True
                n += 1
        prec = s
    exiger(atteint, 'controle : date non atteinte (point %s)' % st.sc.noms[p])
    return n + 1


def deux_medianes(st, r):
    """Recoupe de (T_1/2, O) par l'algorithme de l'auditeur continu (deux selections ponderees) : m = noeud de
    l'atome mediane en ordre d'Euler (cumul > W/2 strict) ; h_i = max(o_i, b(LCA(v_i, m))) ; T_1/2 = premier quantile
    des h dont le cumul depasse W/2 ; O = ancetre de m vivant a T_1/2. Implementation independante (Euler propre,
    LCA par marche)."""
    T = st.T
    votes = r['votes']
    W = r['W']
    tin = {}
    horloge = 0
    pile = [T.root]
    while pile:
        v = pile.pop()
        tin[v] = horloge
        horloge += 1
        for c in sorted(T.children[v], reverse=True):
            pile.append(c)
    at = sorted(votes, key=lambda t3: (tin[t3[1]], t3[0]))
    cum = Fraction(0)
    m = None
    for _o, v, w in at:
        cum += w
        if 2 * cum > W:
            m = v
            break
    hs = sorted((max(o, T.birth[_lca_marche(T, v, m)]), w) for o, v, w in votes)
    cum = Fraction(0)
    i = 0
    while i < len(hs):
        e = hs[i][0]
        while i < len(hs) and hs[i][0] == e:
            cum += hs[i][1]
            i += 1
        if 2 * cum > W:
            return e, _anc_marche(T, m, e)
    return None, None


def controler_np(st, regle):
    """Controles toujours faits : Hierarchie.valider (fait a la construction) et NP ancree. Rend le nombre."""
    n_ctrl = 0
    for p in range(st.n):
        t = regle.h.dates[p]
        o = regle.h.proprietaires[p]
        cvo = st.cov[p].get(o)
        exiger(cvo is not None and Rayon(b2=cvo).cmp(t) <= 0,
               '%s : proprietaire du point %s ne le couvre pas a sa date' % (regle.nom, st.sc.noms[p]))
        n_ctrl += 1
    return n_ctrl


def controler_er(st, regle):
    """Invariants exacts : Hierarchie.valider ; NP ancree (le proprietaire couvre x a sa date) ; chaine
    sqrt(A) <= sqrt(T_1/2) <= t <= sqrt(T(1)) et T(1) <= 4 E2 (borne prouvee) ; recoupes independantes (balayage par
    la definition, deux medianes de l'auditeur). Rend le nombre de controles ; leve ERErreur sinon."""
    n_ctrl = regle.h.valider()
    var = regle.params['variante']
    for p in range(st.n):
        r = regle.details[p]
        t = regle.h.dates[p]
        o = regle.h.proprietaires[p]
        cvo = st.cov[p].get(o)
        exiger(cvo is not None and Rayon(b2=cvo).cmp(t) <= 0,
               '%s : proprietaire du point %s ne le couvre pas a sa date' % (regle.nom, st.sc.noms[p]))
        n_ctrl += 1
        if not r['votes']:
            continue
        exiger(QS.sqrt(r['A']).cmp(QS.sqrt(r['T_half'])) <= 0, '%s : T_1/2 < A' % regle.nom)
        exiger(QS.sqrt(r['T_half']).cmp(r['date']) <= 0, '%s : date < T_1/2' % regle.nom)
        exiger(r['date'].cmp(QS.sqrt(r['T1'])) <= 0, '%s : date > T(1)' % regle.nom)
        exiger(r['T1'] <= 4 * r['E2'], '%s : T(1) > 4 E2 (point %s)' % (regle.nom, st.sc.noms[p]))
        n_ctrl += 4
        if var in (None, 'sans_echelle', 'sans_admissibilite', 'sigma_min', 'repli_racine'):
            n_ctrl += verifier_point(st, p, r, regle.params['kappa'])
            th, O2 = deux_medianes(st, r)
            exiger(th == r['T_half'] and O2 == r['O'], '%s : deux medianes differentes (point %s)'
                   % (regle.nom, st.sc.noms[p]))
            th3, O3, _m3 = _majorite(st.T, r['votes'], r['W'])
            exiger(th3 == r['T_half'] and O3 == r['O'], '%s : balayage et selection mediane differents (point %s)'
                   % (regle.nom, st.sc.noms[p]))
            n_ctrl += 2
    return n_ctrl


# ------------------------------------------------------------------ aides de lecture

def resume_point(st, regle, p):
    r = regle.details[p]
    sc = st.sc
    return {'point': sc.noms[p], 'date': float(r['date']), 'date_exacte': r['date'].text(), 'mode': r['mode'],
            'proprietaire': regle.h.proprietaires[p], 'A_r': float(QS.sqrt(r['A'])) if r['A'] else 0.0,
            'T_half_r': float(QS.sqrt(r['T_half'])), 'W': str(r['W']), 'votes': len(r['votes']),
            'arg': [r['arg'][0], float(QS.sqrt(r['arg'][1]))],
            'entrees': [{'noeud': u, 'c_r': float(QS.sqrt(c)), 'rho_r': float(QS.sqrt(rho)),
                         'co': [sc.noms[y] for y in st.co_couverts(p, u)]} for u, c, _s2, rho, _nz in r['rapports']]}
