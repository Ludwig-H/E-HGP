"""Majorite de bande dans l'univers des PAIRES a K = 2, a l'echelle et exacte.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=revision_cible_statistique, public_status=not_claimed. GCP non utilise.
Convention de niveau : rayon r des boules (niveau r^2), these.

Univers (definition de `majorites_continues/MEMO.md` § 3.1) : un vote par site y != x, rayon
l(x, y) = max(|xy|/2, D_2(milieu)) = |xy|/2 a K = 2 (x et y sont a |xy|/2 du milieu) ; composante du vote au rayon
s >= l : celle de L_2(s) qui contient le milieu. Cette composante est lue EXACTEMENT par `PairResolverK2` des
fondations frontiere (descente vers une paire vide du catalogue, niveaux strictement decroissants, decisions
entieres), puis remontee aux ancetres. Bande : l <= (1 + eta) alpha_2(x), soit |xy| <= (1 + eta) d_2(x).
Regle : majorite stricte, poids uniformes, denominateur W fige (nombre de votes de bande) ; x entre au premier niveau
ou une composante vivante porte 2 m > W, puis suit ses ancetres (ancrage : laminaire par construction).

Calcul : arbre virtuel des noeuds votants (meme preuve que `majorites_rapides.py` : un vote situe strictement sous un
noeud u a un niveau < naissance(u)). Controles : auto-controle du resolveur (deux descentes, extremite gardee a ou b,
meme noeud) sur un echantillon de paires ; juge direct d'echantillon (masses recalculees a chaque niveau de vote et a
chaque LCA de deux noeuds votants) ; recoupe avec une reconstruction Gamma_2 exhaustive sur petits nuages (hauteurs
de reunion identiques). Aucune decision en flottant ; aucun assert.
"""
from fractions import Fraction
from math import isqrt
import random

import stat_commun as SC
import regles as RG
import harnais as H

fc = RG.fc


class ErreurPaires(RuntimeError):
    pass


def _exiger(c, m):
    if not c:
        raise ErreurPaires(m)


class PairesK2:
    """Votes de paires de bande pour chaque site d'une SceneStat a K = 2."""

    def __init__(self, sc, eta, controle_resolveur=300):
        _exiger(sc.K == 2, 'univers des paires implemente a K = 2 seulement')
        self.sc = sc
        self.eta = Fraction(eta)
        self.res = fc.PairResolverK2(sc.export)
        _exiger(self.res.forest is sc.foret, 'foret du resolveur differente de celle de la scene')
        self.controles = {'resolveur_deux_descentes': 0}
        rng = random.Random('paires|%s' % sc.sha_nuage)
        n = sc.ctx.n
        for _ in range(min(controle_resolveur, n)):
            a = rng.randrange(n)
            b = rng.randrange(n)
            if a == b:
                continue
            _exiger(self.res.resolve(a, b, 0) == self.res.resolve(a, b, 1),
                    'resolveur : deux descentes differentes pour {%d, %d}' % (a, b))
            self.controles['resolveur_deux_descentes'] += 1

    def lignes(self, s):
        """[(niveau Fraction, noeud, site y)] des votes de bande du site s (indices natifs)."""
        ctx = self.sc.ctx
        X = ctx.sites[s]
        seuil4 = 4 * (1 + self.eta) ** 2 * ctx.alpha2(s)      # |xy|^2 <= 4 (1+eta)^2 alpha^2
        h = isqrt(int(seuil4) + 1) + 1
        lo = [X[i] - h for i in range(3)]
        hi = [X[i] + h for i in range(3)]
        out = []
        for y in self.res.grid.candidates(lo, hi):
            if y == s:
                continue
            d2 = fc.sq_dist(X, ctx.sites[y])
            if d2 <= seuil4:
                out.append((Fraction(d2, 4), self.res.resolve(s, y), y))
        _exiger(out, 'site %d sans vote de paire dans sa bande' % s)
        return out


def majorite_virtuelle(ctx, rows):
    """Premiere majorite stricte (niveau, noeud) pour des votes uniformes [(niveau, noeud, _)] ; arbre virtuel."""
    f = ctx.forest
    tin = ctx.tin
    W = len(rows)
    own = {}
    for l, v, _y in rows:
        own.setdefault(v, []).append(l)
    noeuds = sorted(own, key=lambda v: tin[v])
    virt = set(noeuds)
    for a, b in zip(noeuds, noeuds[1:]):
        virt.add(f.lca(a, b))
    virt = sorted(virt, key=lambda v: tin[v])
    parent_v = {}
    pile = []
    for v in virt:
        while pile and not f.is_ancestor(pile[-1], v):
            pile.pop()
        parent_v[v] = pile[-1] if pile else None
        pile.append(v)
    below = {v: 0 for v in virt}
    for v in sorted(virt, key=lambda u: -f.depth[u]):
        p = parent_v[v]
        if p is not None:
            below[p] += below[v] + len(own.get(v, ()))
    best = None
    for u in virt:
        bu = f.level(u)
        items = sorted(own.get(u, ()))
        m = below[u]
        i = 0
        while i < len(items) and items[i] <= bu:
            m += 1
            i += 1
        t = None
        if 2 * m > W:
            t = bu
        else:
            while i < len(items):
                l = items[i]
                while i < len(items) and items[i] == l:
                    m += 1
                    i += 1
                if 2 * m > W:
                    t = l
                    break
        if t is None:
            continue
        if best is None or t < best[0]:
            best = (t, u)
        elif t == best[0] and u != best[1]:
            lo, hi = (u, best[1]) if f.depth[u] > f.depth[best[1]] else (best[1], u)
            _exiger(f.is_ancestor(hi, lo), 'deux majorites disjointes au meme niveau')
            best = (t, hi) if f.level(hi) <= t else (t, lo)
    _exiger(best is not None, 'point sans majorite')
    return best[0], best[1], W


def construire(sc, eta, juge=300):
    """Hierarchie de la bibliotheque pour la majorite de paires de bande eta a K = 2 ; controles dans `info`."""
    pk = PairesK2(sc, eta)
    ctx = sc.ctx
    f = ctx.forest
    dates = [None] * sc.n
    owners = [None] * sc.n
    nb = []
    lignes = {}
    for s in range(ctx.n):
        p = ctx.point_id[s]
        rows = pk.lignes(s)
        lignes[s] = rows
        t, u, W = majorite_virtuelle(ctx, rows)
        _exiger(f.ancestor(u, t, True) == u, 'proprietaire non vivant a sa date (site %d)' % s)
        dates[p] = RG.Rayon(b2=t)
        owners[p] = u
        nb.append(W)
    nom = 'maj_paires[%s]' % Fraction(eta)
    info = {'eta': str(Fraction(eta)), 'votes_min': min(nb), 'votes_max': max(nb),
            'votes_moyen': round(sum(nb) / len(nb), 3), 'W_egal_1': sum(1 for k in nb if k == 1),
            'W_egal_2': sum(1 for k in nb if k == 2), 'resolveur_descentes': pk.res.steps,
            'resolveur_naissances': pk.res.births_reached}
    h = RG.Hierarchie(sc, nom, 'majorite', f, dates, owners, info)
    info['juge_direct'] = juge_direct(sc, h, lignes, juge)
    info['controles_resolveur'] = pk.controles['resolveur_deux_descentes']
    return h


def juge_direct(sc, h, lignes, echantillon=300):
    """Masses recalculees par ancetre a chaque niveau de vote et a chaque LCA de deux noeuds votants, sur des sites
    tires : aucune majorite stricte avant la date, puis celle de l'ancetre du proprietaire."""
    ctx = sc.ctx
    f = ctx.forest
    rng = random.Random('juge_paires|%s|%s' % (sc.sha_nuage, h.nom))
    n_ctrl = 0
    for s in rng.sample(range(ctx.n), min(echantillon, ctx.n)):
        p = ctx.point_id[s]
        rows = lignes[s]
        W = len(rows)
        lv = set(l for l, _v, _y in rows)
        nds = sorted(set(v for _l, v, _y in rows))
        for i in range(len(nds)):
            for j in range(i + 1, len(nds)):
                lv.add(f.level(f.lca(nds[i], nds[j])))
        date = h.dates[p].b2
        for beta in sorted(lv):
            t = f.threshold(beta, True)
            masse = {}
            for l, v, _y in rows:
                if l <= beta:
                    a = f.ancestor_lv(v, t)
                    masse[a] = masse.get(a, 0) + 1
            gagnants = [a for a, m in masse.items() if 2 * m > W]
            _exiger(len(gagnants) <= 1, 'majorite non exclusive (site %d)' % s)
            if date <= beta:
                _exiger(gagnants == [f.ancestor_lv(h.proprietaires[p], t)], 'majorite perdue (site %d)' % s)
            else:
                _exiger(not gagnants, 'majorite avant la date (site %d)' % s)
            n_ctrl += 1
    return n_ctrl


class SceneStatPaires(H.SceneStat):
    """SceneStat du harnais avec, en plus, les regles `maj_paires[eta]` (K = 2)."""

    def regle_legere(self, nom, coupes=24, juge_maj=300, recouper_bibliotheque=False):
        if not nom.startswith('maj_paires['):
            return super().regle_legere(nom, coupes, juge_maj, recouper_bibliotheque)
        if nom in self._regles:
            return self._regles[nom]
        h = construire(self, Fraction(nom[len('maj_paires['):-1]), juge_maj)
        self.controles['juge_direct_' + nom] = h.info['juge_direct']
        self.controles['leger_' + nom] = H.valider_leger(self, h, coupes)
        self._regles[nom] = h
        return h


def recoupe_gamma(points, eta):
    """Petit nuage : hauteurs de reunion de maj_paires[eta] (foret native) egales a celles d'une reconstruction
    Gamma_2 exhaustive (GammaForest de l'approche soeur, votes de paires, majorite stricte recalculee par balayage
    de tous les niveaux). Rend le nombre de hauteurs comparees."""
    MMC = SC.charger_module('mmc_soeur_lu',
                            '/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/mmc.py')
    sc = SceneStatPaires(points, 2, nom='recoupe_paires')
    h = sc.regle_legere('maj_paires[%s]' % Fraction(eta))
    P = sc.P
    gf = MMC.GammaForest(P, 2)
    g = gf.forest
    dates, owners = [], []
    for x in range(sc.n):
        rows = []
        a2 = gf.alpha2(x)
        for y in range(sc.n):
            if y == x:
                continue
            l2 = Fraction(sum((u - v) ** 2 for u, v in zip(P[x], P[y])), 4)
            if l2 <= (1 + Fraction(eta)) ** 2 * a2:
                rows.append((l2, gf.born[tuple(sorted((x, y)))]))
        W = len(rows)
        trouve = None
        for beta in sorted(set(l for l, _v in rows) | set(g.levels)):
            t = g.threshold(beta, True)
            masse = {}
            for l, v in rows:
                if l <= beta:
                    a = g.ancestor_lv(v, t)
                    masse[a] = masse.get(a, 0) + 1
            gg = [a for a, m in masse.items() if 2 * m > W]
            if gg:
                trouve = (beta, gg[0])
                break
        _exiger(trouve is not None, 'Gamma : pas de majorite')
        dates.append(trouve[0])
        owners.append(trouve[1])
    n_cmp = 0
    for x in range(sc.n):
        for y in range(x + 1, sc.n):
            ug = max(dates[x], dates[y], g.levels[g.lv[g.lca(owners[x], owners[y])]])
            un = h.hauteur(x, y)
            _exiger(un.cmp(RG.Rayon(b2=ug)) == 0, 'recoupe Gamma : hauteur (%d, %d) differente' % (x, y))
            n_cmp += 1
    return n_cmp
