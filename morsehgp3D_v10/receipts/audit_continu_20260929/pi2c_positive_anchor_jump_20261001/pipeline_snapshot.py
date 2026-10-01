#!/usr/bin/env python3
"""Pipeline FULL_K -> hierarchie laminaire de points -> condensation par min_cluster_size (mcs).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=juge_final_parametres, public_status=not_claimed. GCP non utilise. Aucun moteur modifie. La bibliotheque partagee
(fixtures_cibles/lib), les modules des approches (revision_cible/...) et le worktree partage sont IMPORTES en lecture
seule (pas de bytecode, aucune ecriture).

Ce module ajoute deux choses :

1. Le FILTRE D'ADMISSIBILITE Pi2 (projection qui prend mcs en parametre). Pour un noeud v de FULL_K, son amas discret
   au niveau s est Cl(v, s) = { p : v couvre p au niveau s } (lemme de couverture : v couvre p des c_p(v)).
   s_mcs(v) = mcs-ieme plus petit c_p(v) ; v est un NOEUD-CLUSTER s'il atteint mcs points pendant sa vie
   (s_mcs(v) < d_v, d_v = naissance du parent, infini a la racine). Un noeud qui n'est pas un noeud-cluster est
   - ADMISSIBLE si sa lignee devient un cluster par elle-meme : son premier ancetre-cluster u atteint mcs points
     pendant sa vie (croissance), ou nait d'une fusion dont AUCUN enfant n'est un noeud-cluster ;
   - DISSOUS sinon : sa lignee n'atteint mcs points qu'en rejoignant un cluster deja forme (absorption, ou fusion de
     clusters existants). Une structure dissoute ne reclame aucun point (principe Pi2).
   Les noeuds-clusters sont admissibles. L'ensemble admissible est clos vers le haut. Pour mcs <= K, tout noeud est
   un noeud-cluster des sa naissance (une composante de L_K couvre au moins K sites) : le filtre est vide.

2. Les familles de regles, chacune sur la structure couvrante FILTREE (mcs = None : pas de filtre) :
   - MMt_{kappa,eta} (temps de couverture, mmt.mmt_point) ;
   - MMg_{kappa,eta'} (K-parties, Gamma_K) et MMp_{kappa,eta'} (paires d'ordre K) (mmc.mm_point) ;
   - maj_paires[eta] (majorite stricte de paires, bande dure, entree immediate) ;
   - majorite progressive z (unites de branche, phi = r^-z).
   Chaque regle rend une regles.Hierarchie (dates en Rayon, proprietaires vivants a leur date).

Toutes les decisions sont exactes (Fraction, sommes de racines QS, filtre flottant certifie de la bibliotheque).
Aucun assert : comportement identique sous python3 -O.
"""
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
SCR = '/tmp/mhgp10-juge-final/parametres'
os.environ.setdefault('MHGP10_FIXTURES_SCRATCH', os.path.join(SCR, 'lib_scratch'))
LIB = '/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib'
MMC = '/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues'
CIBLES = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles'
for _p in (LIB, MMC, CIBLES):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import regles as RG  # noqa: E402
import mmc  # noqa: E402
import mmt  # noqa: E402
import condense as CD  # noqa: E402

QS = RG.QS
Rayon = RG.Rayon
NONE = RG.NONE
PA = RG.PA

CADRE = ('phase=exploration_v10_hors_registre backend=cpu_reference profile=quantized_u18_input_only '
         'mode=juge_final_parametres public_status=not_claimed ; GCP non utilise ; aucun moteur modifie')


class PipeErreur(RuntimeError):
    """Invariant viole (jamais un assert)."""


def exiger(cond, msg):
    if not cond:
        raise PipeErreur(msg)


# ------------------------------------------------------------------ arbres : interface commune

class Arbre:
    """Arbre de fusion minimal : birth, death (None = infini), parent (-1 = racine), children, depth."""

    def __init__(self, birth, death, parent, children):
        self.birth, self.death, self.parent, self.children = birth, death, parent, children
        n = len(birth)
        roots = [v for v in range(n) if parent[v] < 0]
        exiger(len(roots) == 1, 'arbre a %d racines' % len(roots))
        self.root = roots[0]
        depth = [None] * n
        depth[self.root] = 0
        pile = [self.root]
        while pile:
            v = pile.pop()
            for c in children[v]:
                depth[c] = depth[v] + 1
                pile.append(c)
        exiger(all(d is not None for d in depth), 'noeud hors arbre')
        self.depth = depth

    def __len__(self):
        return len(self.birth)


def arbre_de_foret(f):
    """Arbre depuis une frontier_core.Forest (native ou Gamma)."""
    n = len(f)
    birth = [Fraction(f.levels[f.lv[v]]) for v in range(n)]
    parent = [-1 if f.parent[v] == NONE else f.parent[v] for v in range(n)]
    death = [None if parent[v] < 0 else birth[parent[v]] for v in range(n)]
    return Arbre(birth, death, parent, [list(f.children[v]) for v in range(n)])


# ------------------------------------------------------------------ filtre d'admissibilite Pi2

class Admissibilite:
    """Verdict Pi2 pour un mcs donne. `cover` : liste par point de {noeud : premier niveau de couverture}, close vers
    le haut (lemme de couverture). mcs = None : tout est admissible.

    Noeud-cluster : s_mcs(v) < d_v (l'amas discret de v atteint mcs points pendant sa vie).
    Trois lectures de Pi2 pour un noeud v non-cluster qui couvre x (seule la premiere est retenue) :
      - 'point' (retenue) : on remonte la lignee de v jusqu'a son premier ancetre-cluster ; v est DISSOUS pour x si,
        a une fusion du chemin (ancetre-cluster compris), un AUTRE enfant est un noeud-cluster qui couvre x pendant sa
        vie : la lignee de v ne devient un cluster qu'en rejoignant un cluster qui reclame deja x. Sinon v est
        admissible pour x (credite au cluster qu'il rejoint ou qu'il forme) ;
      - 'stricte' (ecartee) : v est dissous des que son premier ancetre-cluster nait d'une fusion ayant un enfant
        noeud-cluster (absorption), qu'il couvre x ou non ;
      - 'souple' (ecartee) : v est dissous seulement si cette fusion reunit au moins deux noeuds-clusters.
    Les noeuds-clusters sont toujours admissibles. Pour mcs <= K tout noeud est un noeud-cluster (filtre vide).

    Lecture CONTINUE 'continu' (Pi2c, retenue apres la fixture T1_1700 D+1 : la lecture 'point' bascule quand une
    variante +-1 scinde le plateau de la fusion globale) : chaque noeud v qui couvre x recoit un poids
    omega(x, v) = 1 si v est un cluster quand il commence a couvrir x ; sinon, avec l1 = niveau ou la lignee de v
    devient (partie d') un cluster et l2 = premiere fusion du chemin qui reunit la lignee de v a une autre lignee
    couvrant x alors que l'une des deux est deja un noeud-cluster (fin de la concurrence ; une fusion de morceaux
    sous mcs qui FORME un cluster n'en est pas une) ; infini s'il n'y en a pas :
    omega = 1 si l2 est infini, omega = (l2 - l1) / (l2 - c_x(v)) sinon.
    C'est la part de la periode de concurrence [c_x(v), l2] pendant laquelle la lignee de v est un cluster :
    0 quand elle ne devient cluster qu'en rencontrant une option de x (dissolution de la lecture 'point'),
    continue quand une variante ecarte les deux fusions d'un plateau."""

    MODES = ('continu', 'point', 'stricte', 'souple')

    def __init__(self, T, cover, mcs, mode='point'):
        self.mcs = mcs
        self.mode = mode
        exiger(mode in self.MODES, 'mode Pi2 inconnu : %r' % (mode,))
        n = len(T.birth)
        self.T = T
        self._cache = {}
        if mcs is None:
            self.cluster = [True] * n
            self.s_mcs = [None] * n
            return
        exiger(isinstance(mcs, int) and mcs >= 1, 'mcs entier >= 1')
        niveaux = [[] for _ in range(n)]
        for cv in cover:
            for v, c in cv.items():
                niveaux[v].append(c)
        s_mcs = [None] * n
        for v in range(n):
            lv = sorted(niveaux[v])
            s_mcs[v] = lv[mcs - 1] if len(lv) >= mcs else None
        self.s_mcs = s_mcs
        self.cluster = [s_mcs[v] is not None and (T.death[v] is None or s_mcs[v] < T.death[v]) for v in range(n)]

    def vide(self):
        return all(self.cluster)

    def admissible(self, v, cv):
        """Verdict de v pour le point de couverture cv (dict noeud -> niveau)."""
        cl = self.cluster
        if cl[v]:
            return True
        T = self.T
        if self.mode == 'point':
            u = v
            while True:
                p = T.parent[u]
                if p < 0:
                    return False
                for c in T.children[p]:
                    if c != u and cl[c] and c in cv:
                        return False
                if cl[p]:
                    return True
                u = p
        # lectures globales (ne dependent pas du point)
        if v in self._cache:
            return self._cache[v]
        u = v
        while u >= 0 and not cl[u]:
            u = T.parent[u]
        if u < 0:
            out = False
        elif self.s_mcs[u] > T.birth[u]:
            out = True
        else:
            k = sum(1 for c in T.children[u] if cl[c])
            out = (k == 0) if self.mode == 'stricte' else (k <= 1)
        self._cache[v] = out
        return out

    def omega(self, v, cv):
        """Poids continu Pi2c (Fraction dans [0, 1]) du noeud v pour le point de couverture cv."""
        cl, T, sm = self.cluster, self.T, self.s_mcs
        c = cv[v]
        if cl[v] and sm[v] <= c:
            return Fraction(1)
        if cl[v]:
            l1 = sm[v] if sm[v] > T.birth[v] else T.birth[v]
        else:
            u = v
            while u >= 0 and not cl[u]:
                u = T.parent[u]
            if u < 0:
                return Fraction(0)
            l1 = sm[u] if sm[u] > T.birth[u] else T.birth[u]
        l2 = None
        u = v
        while T.parent[u] >= 0:
            p = T.parent[u]
            autres = [ch for ch in T.children[p] if ch != u and ch in cv]
            if autres and (cl[u] or any(cl[ch] for ch in autres)):
                l2 = T.birth[p]
                break
            u = p
        if l2 is None:
            return Fraction(1)
        exiger(c < l1 <= l2, 'Pi2c : ordre c < l1 <= l2 viole')
        return (l2 - l1) / (l2 - c)

    def ponderer(self, cv):
        """{noeud : (niveau de couverture, poids)} pour les poids > 0 (lectures binaires : poids 1)."""
        if self.mcs is None:
            return {v: (c, Fraction(1)) for v, c in cv.items()}
        if self.mode == 'continu':
            out = {}
            for v, c in cv.items():
                w = self.omega(v, cv)
                if w > 0:
                    out[v] = (c, w)
            return out
        return {v: (c, Fraction(1)) for v, c in self.filtrer(cv).items()}

    def ponderer_bande(self, cv, eta):
        """Comme ponderer, restreint aux noeuds de la bande [A, (1 + eta) A[ (A = premier niveau de poids > 0) :
        c'est tout ce que lit MMt."""
        if self.mcs is None or self.mode != 'continu':
            pw = self.ponderer(cv)
            A = min(c for c, _w in pw.values())
            E2 = (1 + Fraction(eta)) * A
            return {v: cw for v, cw in pw.items() if cw[0] < E2}
        ordre = sorted(cv.items(), key=lambda t: t[1])
        out = {}
        E2 = None
        for v, c in ordre:
            if E2 is not None and c >= E2:
                break
            w = self.omega(v, cv)
            if w > 0:
                if E2 is None:
                    E2 = (1 + Fraction(eta)) * c
                out[v] = (c, w)
        return out

    def filtrer(self, cv):
        if self.mcs is None:
            return dict(cv)
        if self.mode == 'continu':
            return {v: c for v, (c, _w) in self.ponderer(cv).items()}
        out = {v: c for v, c in cv.items() if self.admissible(v, cv)}
        for v in out:
            p = self.T.parent[v]
            if p >= 0:
                exiger(p in out, 'admissibilite non close vers le haut (noeud %d)' % v)
        return out

    def signature(self, covers):
        """Poids de tous les (point, noeud) : deux mcs de meme signature donnent la meme hierarchie."""
        if self.mcs is None:
            return ()
        out = []
        for p, cv in enumerate(covers):
            pw = self.ponderer(cv)
            for v in sorted(cv):
                w = pw[v][1] if v in pw else Fraction(0)
                if w != 1:
                    out.append((p, v, w))
        return tuple(out)


# ------------------------------------------------------------------ structures couvrantes

class Contexte:
    """Scene de la bibliotheque + structure couvrante native (T, cover par point) + Gamma_K a la demande."""

    def __init__(self, sc):
        self.sc = sc
        Tm, cov = mmt.structure(sc)
        self.Tm = Tm                          # fullk.Tree, memes indices que sc.foret
        self.T = Arbre(list(Tm.birth), list(Tm.death), list(Tm.parent), [list(c) for c in Tm.children])
        self.cover = cov                      # par identifiant de point
        self._gf = None
        self._gcover = None
        self._adm = {}
        self._gadm = {}
        self.mode = 'continu'

    def adm(self, mcs):
        cle = (mcs, self.mode)
        if cle not in self._adm:
            self._adm[cle] = Admissibilite(self.T, self.cover, mcs, self.mode)
        return self._adm[cle]

    def gamma(self):
        if self._gf is None:
            sc = self.sc
            gf = mmc.GammaForest(sc.P, sc.K)
            mmc.recouper_gamma(gf, sc)
            self._gf = gf
            self._gT = arbre_de_foret(gf.forest)
            cov = [dict() for _ in range(sc.n)]
            T = self._gT
            for F, b in gf.beta.items():
                v0 = gf.born[F]
                for p in F:
                    v = v0
                    lvl = b
                    while True:
                        c = lvl if lvl > T.birth[v] else T.birth[v]
                        old = cov[p].get(v)
                        if old is not None and old <= c:
                            break
                        cov[p][v] = c
                        if T.parent[v] < 0:
                            break
                        v = T.parent[v]
                        lvl = T.birth[v]
            self._gcover = cov
        return self._gf

    def gadm(self, mcs):
        self.gamma()
        cle = (mcs, self.mode)
        if cle not in self._gadm:
            self._gadm[cle] = Admissibilite(self._gT, self._gcover, mcs, self.mode)
        return self._gadm[cle]


# ------------------------------------------------------------------ regles

def _hier(sc, nom, famille, foret, dates, owners, info=None):
    h = RG.Hierarchie(sc, nom, famille, foret, dates, owners, info or {})
    return h


def regle_mmt(ctx, kappa, eta, mcs=None):
    """MMt_{kappa,eta} sur la structure couvrante ponderee par Pi2 (mcs = None : MMt de reference, mmt.mmt_point).
    Lectures binaires : MMt de reference sur la structure filtree ; lecture continue : MMt pondere (mmt_pond).
    Cache par point : le resultat ne depend que des noeuds de la bande (c < (1 + eta) A) et de leurs poids."""
    import mmt_rapide
    sc = ctx.sc
    ad = ctx.adm(mcs)
    eta, kappa = Fraction(eta), Fraction(kappa)
    cache = ctx.__dict__.setdefault('_cache_mmt', {})
    dates, owners, det = [], [], []
    for p in range(sc.n):
        cvw = ad.ponderer_bande(ctx.cover[p], eta)
        exiger(cvw, 'MMt : point %s sans structure admissible' % sc.noms[p])
        bande = tuple(sorted((v, c, w) for v, (c, w) in cvw.items()))
        cle = (p, kappa, eta, bande)
        r = cache.get(cle)
        if r is None:
            # la bande suffit : les noeuds au-dela de (1 + eta) A ne portent aucune masse ; les ancetres sont lus
            # dans l'arbre (naissances), pas dans la couverture. Noyau par balayage (mmt_rapide), egal au noyau de
            # reference (recoupe mmt_rapide.recouper dans les recus).
            r = mmt_rapide.mmt_point_rapide(ctx.Tm, cvw, eta, kappa)
            if len(cache) > 200000:
                cache.clear()
            cache[cle] = r
        dates.append(Rayon.somme(r['date']))
        owners.append(r['owner'])
        det.append(r)
    nom = 'MMt[k=%s,e=%s%s]' % (kappa, eta, '' if mcs is None else ',mcs=%d' % mcs)
    h = _hier(sc, nom, 'MMt', sc.foret, dates, owners, {'mcs': mcs})
    h.details = det
    return h


def _poids_gamma(ctx, x, ad):
    """{noeud Gamma : poids Pi2 (Fraction > 0)} pour le point x (1 partout sans filtre)."""
    ctx.gamma()
    gcv = ctx._gcover[x]
    return {v: w for v, (_c, w) in ad.ponderer(gcv).items()}


def _votes_gamma(ctx, x, eta1, ad):
    gf = ctx.gamma()
    pw = _poids_gamma(ctx, x, ad)
    out = []
    for F, b, v in gf.votes(x):
        if v in pw:
            out.append((b, v, pw[v]))
    exiger(out, 'MMg : point sans vote admissible')
    A = min(b for b, _v, _w in out)
    vs = []
    for b, v, om in out:
        w = mmc.poids_bande(b, A, eta1) * om
        if w > 0:
            vs.append((b, v, w))
    return A, vs


def regle_mmg(ctx, kappa, eta1, mcs=None):
    """MMg_{kappa,eta'} (univers Gamma_K) ; filtre Pi2 sur l'arbre Gamma ; echelle = premier vote admissible."""
    sc = ctx.sc
    gf = ctx.gamma()
    ad = ctx.gadm(mcs)
    dates, owners, det = [], [], []
    for p in range(sc.n):
        A, vs = _votes_gamma(ctx, p, eta1, ad)
        r = mmc.mm_point(gf.forest, vs, A, kappa, True)
        dates.append(Rayon.somme(r['date']))
        owners.append(r['owner'])
        det.append(r)
    nom = 'MMg[k=%s,e=%s%s]' % (Fraction(kappa), Fraction(eta1), '' if mcs is None else ',mcs=%d' % mcs)
    h = _hier(sc, nom, 'MMg', gf.forest, dates, owners, {'mcs': mcs})
    h.details = det
    return h


def votes_paires_bruts(ctx, x):
    """Votes de paires d'ordre K de x : [(l^2, noeud Gamma du sommet K-PPV du milieu, y)] (mmc.votes_paires)."""
    gf = ctx.gamma()
    P, K = gf.P, gf.K
    brut = []
    for y in range(gf.n):
        if y == x:
            continue
        m = tuple(Fraction(a + b, 2) for a, b in zip(P[x], P[y]))
        half2 = Fraction(sum((a - b) ** 2 for a, b in zip(P[x], P[y])), 4)
        dist = sorted((sum((mi - pi) ** 2 for mi, pi in zip(m, P[q])), q) for q in range(gf.n))
        DK2 = dist[K - 1][0]
        F = tuple(sorted(q for _d, q in dist[:K]))
        l2 = max(half2, DK2)
        exiger(gf.beta[F] <= l2, 'paires : sommet K-PPV du milieu non ne a son niveau')
        brut.append((l2, gf.born[F], y))
    return brut


def regle_mmp(ctx, kappa, eta1, mcs=None):
    """MMp_{kappa,eta'} (votes de paires d'ordre K) ; filtre Pi2 sur l'arbre Gamma."""
    sc = ctx.sc
    gf = ctx.gamma()
    ad = ctx.gadm(mcs)
    dates, owners, det = [], [], []
    for p in range(sc.n):
        pw = _poids_gamma(ctx, p, ad)
        brut = [(l2, v, y) for l2, v, y in votes_paires_bruts(ctx, p) if v in pw]
        exiger(brut, 'MMp : point sans vote admissible')
        A = min(l2 for l2, _v, _y in brut)
        vs = []
        for l2, v, _y in brut:
            w = mmc.poids_bande(l2, A, eta1) * pw[v]
            if w > 0:
                vs.append((l2, v, w))
        r = mmc.mm_point(gf.forest, vs, A, kappa, True)
        dates.append(Rayon.somme(r['date']))
        owners.append(r['owner'])
        det.append(r)
    nom = 'MMp[k=%s,e=%s%s]' % (Fraction(kappa), Fraction(eta1), '' if mcs is None else ',mcs=%d' % mcs)
    h = _hier(sc, nom, 'MMp', gf.forest, dates, owners, {'mcs': mcs})
    h.details = det
    return h


def regle_maj_paires(ctx, eta, mcs=None):
    """maj_paires[eta] : majorite stricte a denominateur fige, votes de paires d'ordre K, bande dure (1 + eta) a,
    a = plus petit niveau de vote admissible (= alpha a K = 2 sans filtre), poids 1, entree immediate."""
    sc = ctx.sc
    gf = ctx.gamma()
    ad = ctx.gadm(mcs)
    eta = Fraction(eta)
    dates, owners, det = [], [], []
    for p in range(sc.n):
        pw = _poids_gamma(ctx, p, ad)
        brut = [(l2, v) for l2, v, _y in votes_paires_bruts(ctx, p) if v in pw]
        exiger(brut, 'maj_paires : point sans vote admissible')
        A = min(l2 for l2, _v in brut)
        R2 = (1 + eta) ** 2 * A
        votes = [(l2, v, pw[v]) for l2, v in brut if l2 <= R2]
        r = mmc.mm_point(gf.forest, votes, A, 1, marge=False)
        dates.append(Rayon.somme(r['date']))
        owners.append(r['owner'])
        det.append(r)
    nom = 'maj_paires[%s%s]' % (eta, '' if mcs is None else ',mcs=%d' % mcs)
    h = _hier(sc, nom, 'maj_paires', gf.forest, dates, owners, {'mcs': mcs})
    h.details = det
    return h


def regle_progressive(ctx, z, mcs=None):
    """Majorite progressive (memo des masses, definitions 2 a 4) sur la structure couvrante filtree :
    w_x(v) = phi(c_x(v)) - phi(d_v), A_x(v, beta) = base_x(v) + (phi(c_x(v)) - phi(beta))_+, entree au premier beta
    ou une branche vivante porte A >= W/2 ; phi = r^-z exact. Meme noyau que Scene._progressive (bibliotheque)."""
    sc = ctx.sc
    T = ctx.Tm
    ad = ctx.adm(mcs)
    phis = {}

    def ph(beta):
        if beta is None:
            return Fraction(0)
        if beta not in phis:
            phis[beta] = RG.SR.depuis(RG.phi(beta, z))
        return phis[beta]

    dates, owners = [], []
    for p in range(sc.n):
        cvw = ad.ponderer(ctx.cover[p])
        cv = {v: c for v, (c, _w) in cvw.items()}
        exiger(cv, 'progressive : point sans structure admissible')
        w = {}
        for v, c in cv.items():
            val = ph(c) - ph(T.death[v]) if T.death[v] is not None else ph(c)
            exiger(RG.signe(val, Fraction(0)) > 0, 'progressive : unite non positive')
            om = cvw[v][1]
            w[v] = val if om == 1 else val * om
        W = Fraction(0)
        for val in w.values():
            W = W + val
        half = RG._moitie(W)
        base = PA.bases(T, cv, w)
        best = None
        for v in cv:
            sv = cv[v]
            ps = ph(sv)
            pd = ph(T.death[v])
            om = cvw[v][1]
            if RG.signe(base[v], half) >= 0:
                cand = (ps, v, 'saut', sv)
            else:
                # base + omega (phi(c) - phi(t)) = W/2  =>  phi(t) = phi(c) - (W/2 - base) / omega
                target = base[v] + ps - half if om == 1 else ps - (half - base[v]) * (1 / om)
                if RG.signe(target, pd) <= 0:
                    continue
                cand = (target, v, 'glissement', None)
            if best is None:
                best = cand
            else:
                k = RG.signe(cand[0], best[0])
                if k > 0 or (k == 0 and T.depth[cand[1]] < T.depth[best[1]]):
                    best = cand
        exiger(best is not None, 'progressive : point jamais attache')
        if best[2] == 'saut':
            dates.append(Rayon(b2=best[3]))
        else:
            dates.append(Rayon.echelle(best[0], z))
        owners.append(best[1])
    nom = 'prog[z=%d%s]' % (z, '' if mcs is None else ',mcs=%d' % mcs)
    return _hier(sc, nom, 'progressive', sc.foret, dates, owners, {'mcs': mcs})


# ------------------------------------------------------------------ fabrique

def construire(ctx, regle, mcs=None):
    """regle : tuple (famille, parametres...). Rend une Hierarchie validee (Hierarchie.valider)."""
    fam = regle[0]
    if fam == 'MMt':
        h = regle_mmt(ctx, regle[1], regle[2], mcs)
    elif fam == 'MMg':
        h = regle_mmg(ctx, regle[1], regle[2], mcs)
    elif fam == 'MMp':
        h = regle_mmp(ctx, regle[1], regle[2], mcs)
    elif fam == 'maj_paires':
        h = regle_maj_paires(ctx, regle[1], mcs)
    elif fam == 'prog':
        h = regle_progressive(ctx, regle[1], mcs)
    elif fam == 'lib':
        exiger(mcs is None, 'regle de bibliotheque : pas de filtre mcs')
        h = ctx.sc.regle(regle[1])
    else:
        raise PipeErreur('famille inconnue : %r' % (fam,))
    h.valider()
    return h


def nom_regle(regle, mcs_aware):
    fam = regle[0]
    if fam in ('MMt', 'MMg', 'MMp'):
        s = '%s(%s, %s)' % (fam, regle[1], regle[2])
    elif fam == 'maj_paires':
        s = 'maj_paires[%s]' % regle[1]
    elif fam == 'prog':
        s = 'prog[z=%d]' % regle[1]
    else:
        s = regle[1]
    return s + (' + Pi2' if mcs_aware else '')


