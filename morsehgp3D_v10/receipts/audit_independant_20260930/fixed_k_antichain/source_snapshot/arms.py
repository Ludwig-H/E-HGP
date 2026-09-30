#!/usr/bin/env python3
"""Bras d'affectation des points de l'experience frontiere v10 (au-dessus des fondations A et B).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=experience_frontiere_dev, public_status=not_claimed. GCP non utilise. Aucune source du moteur modifiee.

Trois objets restent separes : l'arbre de densite (foret FULL d'ordre K de l'export natif, fixe), l'affectation des
points (les BRAS, ici) et la selection d'une partition (EOM, hors de ce module).

Contrat commun (preenregistrement, anchor_contract) : un bras donne a chaque point x une attache (t(x), v(x)) :
  - t(x) : date = rayon CARRE exact (int ou Fraction), jamais un flottant ;
  - v(x) : noeud de la foret FULL d'ordre K, VIVANT a t(x) en coupe fermee (niveau(v) <= t < niveau(parent(v)), ou v
    racine ; la racine reste vivante au-dela de la derniere fusion : branche racine prolongee) ;
  - l'attache est fixee une fois, avant toute coupe, jamais revoquee ; apres t(x) le point suit les seuls ancetres de
    v(x) ; avant t(x) il est un singleton de completion (jamais un bloc collectif bruit) ;
  - partition a beta : blocs = ancetre vivant a beta du noeud d'attache des points entres ; hauteur de reunion
    u(x, y) = max(t(x), t(y), niveau(LCA(v(x), v(y)))). Les partitions sont emboitees par construction.

Bras (identifiant du preenregistrement entre parentheses) :
  core                    (A0) t = D_K(x) = d_K(x)^2 (x compris), v = composante contenant x (attache core native) ;
  cover                   (A1) t = alpha_K(x)^2, v = composante du centre de la premiere boule couvrante (attache
                               cover native, ordre canonique (niveau, S*) aux egalites) ;
  anchor_eta              (A2) K = 2 : contrat d'ancrage adopte mot pour mot ; S_eta(x) = {{x, y} :
                               |x - y|^2 <= (1 + eta)^2 min_y |x - y|^2} sur TOUTES les paires, egalites comprises ;
                               milieu de chaque paire resolu a son propre niveau |x - y|^2 / 4 (resolveur exact K2 de
                               frontier_core, descente sans ball_node) ; J = LCA ; e(x) = max(niveau(J), max des
                               niveaux des paires) ; cible = ancetre de J vivant a e(x) ;
  maj_uniform             (A3) majorite stricte a masse fixe, poids 1 ;
  maj_invbeta             (A4) majorite stricte a masse fixe, poids 1/beta (beta = niveau, rayon carre) ;
  unique_else_lca         (A5) a alpha_K(x)^2 : une seule composante couvrante -> attache immediate ; sinon LCA des
                               composantes couvrantes, a la premiere date ou il existe ;
  unique_else_maj_invbeta (A6) meme test d'unicite ; vrais conflits : premiere majorite fixe en 1/beta (A4).

Majorite a masse fixe (A3, A4, partie conflit de A6) : univers des temoins propre a K (frontier_core.witness_universe,
mode renforce : boules du catalogue de population fermee >= K et p + q_min <= K, TOUTES leurs incidences I u U ;
temoin = (boule, niveau beta_b, ball_node[b] vivant a beta_b)). W_x = somme des poids de TOUS les temoins de x, fixee
une fois (temoins inactifs compris). M_x(C, beta) = somme des poids des temoins actifs (beta_b <= beta) dont l'ancetre
vivant a beta est C. x est attache la premiere fois qu'un C verifie M_x(C, beta) > W_x / 2 (strict). Les dates
candidates sont les dates des temoins ET les niveaux de fusion des ancetres de leurs noeuds : on les parcourt par
l'arbre virtuel (noeuds temoins et LCA de voisins en ordre DFS), plateau par plateau (niveaux exacts), avec une
union-find ponderee ; O(D_x log D_x) par point (D_x incidences), requetes d'ancetre et de LCA par sauts.

K = 1 (hors campagne) : les temoins sont les sites au niveau nul ; masse finie 1 declaree pour ce temoin de rayon nul
(pas 1/0), dans les deux modes de poids : l'attache est alors (0, naissance du site).

Aucune verification ne repose sur assert (comportement identique sous python3 -O) ; toute incoherence leve
frontier_core.FrontierError ; tout plafond de ressources leve ArmRefusal (jamais de troncature silencieuse).

Usage en ligne de commande :
  python3 arms.py EXPORT.json --arm=NOM [--eta=1/4] [--eps2=3,48] [--max-candidates=N] --out=RESULTAT.json
Codes : 0 conforme ; 1 incoherence (FrontierError) ; 2 refus (usage, plafond, domaine).
"""
from fractions import Fraction
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import frontier_core as fc  # noqa: E402

SCHEMA_RESULT = 'mhgp10_frontier_arm_v1'
NONE = fc.NONE

ARMS = ('core', 'cover', 'anchor_eta', 'maj_uniform', 'maj_invbeta', 'unique_else_lca', 'unique_else_maj_invbeta')
PREREG_ID = {
    'core': 'A0_core',
    'cover': 'A1_cover_first',
    'anchor_eta': 'A2_anchor_eta',
    'maj_uniform': 'A3_majority_uniform',
    'maj_invbeta': 'A4_majority_inv_beta',
    'unique_else_lca': 'A5_unique_then_lca',
    'unique_else_maj_invbeta': 'A6_unique_then_majority_inv_beta',
}
# valeurs de eta : union du preenregistrement {0, 1/4, 1} et de la tache {0, 1/8, 1/4, 1/2}
DEFAULT_ETAS = (Fraction(0), Fraction(1, 8), Fraction(1, 4), Fraction(1, 2), Fraction(1))
DEFAULT_EPS2 = (3, 48)  # perturb.json des scenes dev : delta = 1 et delta = 4


class ArmRefusal(fc.FrontierError):
    """Refus explicite d'un bras (plafond de ressources, K hors domaine, point sans temoin) ; jamais une troncature."""


def require(condition, message):
    if not condition:
        raise fc.FrontierError(message)


# ------------------------------------------------------------------ points d'injection (valeurs de production)
# La porte de conception remplace ces fonctions et constantes par des mutants causaux pour prouver qu'elle n'est
# pas vide ; le code de production ne les modifie jamais.

_CLOSED_CUT = True        # convention de coupe des ancetres (fermee)
_STRONG_UNIVERSE = True   # univers renforce p + q_min <= K


def _weight(mode, level):
    """Poids d'un temoin de niveau `level` (rayon carre exact)."""
    if mode == 'uniform':
        return Fraction(1)
    if mode == 'inverse_beta':
        level = fc.as_fraction(level)
        if level == 0:
            return Fraction(1)  # convention declaree : temoin site de rayon nul (K = 1), masse finie 1
        return 1 / level
    raise ArmRefusal('mode de poids inconnu : %r' % (mode,))


def _majority(mass, total, active_total):
    """Majorite stricte a denominateur FIXE : M > W_x / 2 (active_total ne sert qu'aux mutants)."""
    return 2 * mass > total


def _plateau_is_candidate(kinds):
    """Toute date de temoin ET tout niveau de fusion est une date candidate."""
    return True


def _covering_components(ctx, s, alpha2):
    """Composantes vivantes a alpha2 des temoins de x de date <= alpha2, dedupliquees et triees."""
    return sorted(set(_covering_list(ctx, s, alpha2)))


def _band_members(ctx, s, eta):
    """(m2, seuil, membres) : m2 = min |x - y|^2, seuil = (1 + eta)^2 m2, membres = TOUS les y != x avec
    |x - y|^2 <= seuil (egalites comprises), tries par indice de site."""
    grid = ctx.grid()
    P = ctx.sites[s]
    near = grid.k_nearest(P, 2)
    ctx.cnt['requetes_voisins'] += 1
    m2 = near[1][0]
    thr = (1 + eta) ** 2 * m2
    members = [y for y in grid.within(P, thr) if y != s]
    ctx.cnt['requetes_voisins'] += 1
    return m2, thr, members


def _anchor_join(ctx, nodes):
    """Ancetre commun J des composantes des milieux."""
    J = nodes[0]
    for v in nodes[1:]:
        J = ctx.lca(J, v)
    return J


def _anchor_date(level_J, beta_max):
    """e(x) = max(naissance(J), max des niveaux des candidats)."""
    return max(level_J, beta_max)


def _unique_conflict_attach(ctx, alpha2, comps):
    """Vrai conflit (au moins deux composantes) : J = LCA, date max(alpha2, niveau(J)), cible anc(J, date)."""
    J = comps[0]
    for v in comps[1:]:
        J = ctx.lca(J, v)
    t = max(alpha2, ctx.level(J))
    return t, ctx.anc(J, t)


# ------------------------------------------------------------------ contexte commun a un export

class ArmContext:
    """Donnees partagees par les bras d'un meme export : foret FULL d'ordre K, attaches natives, univers des temoins,
    grille exacte, resolveur de paires K = 2, ordre DFS. Les compteurs `cnt` sont remis a zero par `begin`."""

    def __init__(self, export, k=None, verify_qmin=True):
        self.export = export
        self.verify_qmin = verify_qmin
        self.qmin_checked = 0
        self._all_cover = None
        self.order = export.order(k)
        self.k = self.order.k
        self.forest = self.order.forest
        self.sites = export.sites
        self.n = len(export.sites)
        self.point_id = export.point_id
        self.cnt = {}
        self._witnesses = {}
        self._grid = None
        self._tin = None
        self._tout = None

    def begin(self):
        self.cnt = {'requetes_ancetre': 0, 'requetes_lca': 0, 'requetes_voisins': 0}
        return self.cnt

    # -- foret
    def level(self, v):
        return self.forest.level(v)

    def anc(self, v, beta):
        """Ancetre de v vivant a beta (coupe fermee) ; v doit etre ne a beta."""
        self.cnt['requetes_ancetre'] = self.cnt.get('requetes_ancetre', 0) + 1
        a = self.forest.ancestor(v, beta, _CLOSED_CUT)
        require(a is not None, 'noeud %d non ne a %s' % (v, beta))
        return a

    def lca(self, u, v):
        self.cnt['requetes_lca'] = self.cnt.get('requetes_lca', 0) + 1
        return self.forest.lca(u, v)

    def euler(self):
        """Ordre DFS (entree/sortie) de la foret, iteratif."""
        if self._tin is None:
            f = self.forest
            tin = [0] * len(f)
            tout = [0] * len(f)
            clock = 0
            stack = [(f.root, 0)]
            while stack:
                v, state = stack.pop()
                if state == 0:
                    tin[v] = clock
                    clock += 1
                    stack.append((v, 1))
                    for c in sorted(f.children[v], reverse=True):
                        stack.append((c, 0))
                else:
                    tout[v] = clock
                    clock += 1
            self._tin, self._tout = tin, tout
        return self._tin, self._tout

    def is_ancestor(self, a, v):
        tin, tout = self.euler()
        return tin[a] <= tin[v] and tout[v] <= tout[a]

    # -- donnees natives
    def core_level(self, s):
        return Fraction(self.order.core_level[s])

    def cover_level(self, s):
        return self.order.cover_level(s)

    def witnesses(self):
        """Univers des temoins propre a K (renforce) : par site, liste de (boule, niveau, noeud). A la premiere
        construction (K >= 2), la decision du filtre p + q_min <= K est recalculee exactement (frontier_core.qmin_exact,
        arithmetique entiere) pour toute boule de population >= K et p <= K - 2 : q_min exact = q_min natif, sinon
        FrontierError."""
        key = bool(_STRONG_UNIVERSE)
        if key not in self._witnesses:
            if self.verify_qmin and self.k >= 2 and not self.qmin_checked:
                for ball in self.export.balls:
                    if ball.population >= self.k and ball.p <= self.k - 2:
                        q = fc.qmin_exact(self.sites, ball.U, ball.num, ball.den)
                        require(q == ball.q, 'boule %d : q_min exact %s != q_min natif %d' % (ball.index, q, ball.q))
                        self.qmin_checked += 1
            self._witnesses[key] = fc.witness_universe(self.export, self.k, strong=key)
        return self._witnesses[key]

    def all_covering_at_alpha(self, s):
        """Composantes, vivantes a alpha_K(x)^2, de TOUTES les boules de population >= K qui contiennent x a ce niveau
        exact (sans filtre p + q_min) ; par le lemme de couverture, egales a celles de l'univers renforce."""
        if self._all_cover is None:
            f = self.forest
            alpha = [self.cover_level(t) for t in range(self.n)]
            out = [set() for _ in range(self.n)]
            if self.k == 1:
                for t in range(self.n):
                    out[t].add(f.birth_node[t])
            else:
                for ball in self.export.balls:
                    if ball.population < self.k:
                        continue
                    v = self.order.ball_node[ball.index]
                    for t in ball.members():
                        if ball.level == alpha[t]:
                            out[t].add(f.ancestor(v, alpha[t], True))
            self._all_cover = [sorted(c) for c in out]
        return self._all_cover[s]

    def grid(self):
        if self._grid is None:
            self._grid = fc.SiteGrid(self.sites)
        return self._grid

    def new_resolver(self):
        """Resolveur de paires K = 2 NEUF (memo vide) : chaque bras d'ancrage compte son propre cout."""
        if self.k != 2:
            raise ArmRefusal('resolveur de paires : K = 2 seulement (K = %d)' % self.k)
        return fc.PairResolverK2(self.export, self.grid())


def _covering_list(ctx, s, alpha2):
    """Composantes (avec repetitions) des temoins de x de date <= alpha2 ; controle que alpha2 est la premiere date
    de couverture (aucun temoin anterieur, au moins un temoin a alpha2)."""
    wl = ctx.witnesses()[s]
    at = [v for (_b, lvl, v) in wl if lvl <= alpha2]
    require(at, 'site %d : aucun temoin a alpha_K^2 = %s' % (s, alpha2))
    require(all(lvl >= alpha2 for (_b, lvl, _v) in wl), 'site %d : temoin anterieur a alpha_K^2' % s)
    ctx.cnt['incidences_a_alpha'] = ctx.cnt.get('incidences_a_alpha', 0) + len(at)
    return [ctx.anc(v, alpha2) for v in at]


# ------------------------------------------------------------------ resultat d'un bras

class ArmResult:
    """Attaches (date, noeud) par SITE (indice moteur, rang de Morton) ; `point_id[s]` donne l'identifiant du point
    (rang dans points.u32le). Les methodes *_points travaillent en identifiants de points."""

    def __init__(self, ctx, arm, params, dates, nodes, counters, info):
        self.arm = arm
        self.params = dict(params)
        self.k = ctx.k
        self.forest = ctx.forest
        self.point_id = list(ctx.point_id)
        self.site_of_point = {p: s for s, p in enumerate(self.point_id)}
        self.dates = dates
        self.nodes = nodes
        self.counters = dict(counters)
        self.info = info
        self._att = None

    @property
    def prereg_id(self):
        return PREREG_ID[self.arm]

    @property
    def key(self):
        return arm_key(self.arm, self.params)

    def attachments(self):
        if self._att is None:
            self._att = fc.Attachments(self.forest, self.dates, self.nodes)
        return self._att

    def validate(self):
        """Contrat d'attache : date exacte >= 0, noeud valide, niveau(v) <= t, v VIVANT a t (normalisation neutre)."""
        f = self.forest
        require(len(self.dates) == len(self.nodes) == len(self.point_id), 'tailles des attaches')
        att = self.attachments()
        for s, (t, v) in enumerate(zip(self.dates, self.nodes)):
            require(isinstance(t, Fraction) and t >= 0, 'site %d : date non exacte ou negative' % s)
            require(isinstance(v, int) and 0 <= v < len(f), 'site %d : noeud invalide' % s)
            require(f.level(v) <= t, 'site %d : noeud posterieur a la date' % s)
            require(f.is_alive(v, t, True), 'site %d : noeud %d mort a sa date %s' % (s, v, t))
            require(att.nodes[s] == v and att.dates[s] == t, 'site %d : normalisation non neutre' % s)
        return len(self.dates)

    def events(self, extra=()):
        """{0} u niveaux FULL u dates d'attache u extra (dates des temoins pour les majorites) : la partition est
        constante entre deux evenements consecutifs."""
        f = self.forest
        ev = {Fraction(0)} | {f.level(v) for v in range(len(f))} | set(self.dates)
        ev |= {fc.as_fraction(b) for b in extra}
        return sorted(ev)

    def blocks(self, beta, closed=True):
        return self.attachments().blocks(beta, closed)

    def point_blocks(self, beta, closed=True):
        return sorted(sorted(self.point_id[s] for s in blk) for blk in self.blocks(beta, closed))

    def height(self, s, t):
        return self.attachments().merge_height(s, t)

    def height_points(self, x, y):
        return self.height(self.site_of_point[x], self.site_of_point[y])

    def date_point(self, x):
        return self.dates[self.site_of_point[x]]

    def node_point(self, x):
        return self.nodes[self.site_of_point[x]]

    def info_by_point(self):
        """Informations du bras en identifiants de points : listes par site reordonnees par point, indices de sites
        des candidats convertis en identifiants de points."""
        n = len(self.point_id)
        order = [self.site_of_point[p] for p in range(n)]
        out = {}
        for key, val in self.info.items():
            if key == 'candidats':
                out[key] = [sorted(self.point_id[y] for y in val[s]) for s in order]
            elif key == 'certifiable_par_site':
                out['certifiable_par_point'] = {e: [val[e][s] for s in order] for e in val}
            elif isinstance(val, list) and len(val) == n:
                out[key] = [val[s] for s in order]
            else:
                out[key] = val
        return out

    def to_json(self, export_sha256=None):
        rows = [None] * len(self.point_id)
        for s, pid in enumerate(self.point_id):
            t = self.dates[s]
            rows[pid] = [pid, '%d/%d' % (t.numerator, t.denominator), self.nodes[s]]
        return {
            'schema': SCHEMA_RESULT,
            'arm': self.arm,
            'prereg_id': self.prereg_id,
            'params': {k: str(v) for k, v in sorted(self.params.items())},
            'K': self.k,
            'export_sha256': export_sha256,
            'convention': 'dates = rayons CARRES exacts num/den ; noeud = indice de la foret FULL d ordre K de l '
                          'export, vivant a la date (coupe fermee) ; lignes [identifiant de point, date, noeud]',
            'attaches': rows,
            'counters': self.counters,
            'info': _jsonable(self.info_by_point()),
        }


def _jsonable(obj):
    if isinstance(obj, Fraction):
        return '%d/%d' % (obj.numerator, obj.denominator)
    if isinstance(obj, dict):
        return {str(k): _jsonable(v) for k, v in sorted(obj.items(), key=lambda kv: str(kv[0]))}
    if isinstance(obj, (list, tuple)):
        return [_jsonable(v) for v in obj]
    return obj


def arm_key(arm, params):
    if arm == 'anchor_eta':
        eta = Fraction(params['eta'])
        return 'anchor_eta[%s]' % eta
    return arm


# ------------------------------------------------------------------ A0, A1 : attaches natives

def arm_core(ctx):
    """A0 : t = D_K(x) (entier exact), v = composante de L_K(D_K(x)) contenant x (export core_node)."""
    cnt = ctx.begin()
    dates = [ctx.core_level(s) for s in range(ctx.n)]
    nodes = [int(v) for v in ctx.order.core_node]
    cnt['lectures_natives'] = ctx.n
    return ArmResult(ctx, 'core', {}, dates, nodes, cnt, {})


def arm_cover(ctx):
    """A1 : t = alpha_K(x)^2, v = composante du centre de la premiere boule couvrante (export cover_node)."""
    cnt = ctx.begin()
    dates = [ctx.cover_level(s) for s in range(ctx.n)]
    nodes = [int(v) for v in ctx.order.cover_node]
    cnt['lectures_natives'] = ctx.n
    return ArmResult(ctx, 'cover', {}, dates, nodes, cnt, {})


# ------------------------------------------------------------------ A3, A4 : majorite a masse fixe

def _majority_attach(ctx, s, mode):
    """Premiere majorite stricte a masse fixe du site s : (date, noeud vivant, W_x, masse gagnante)."""
    wl = ctx.witnesses()[s]
    if not wl:
        raise ArmRefusal('site %d sans temoin (univers incomplet)' % s)
    weights = [_weight(mode, lvl) for (_b, lvl, _v) in wl]
    total = sum(weights, Fraction(0))
    if total <= 0:
        raise ArmRefusal('site %d : masse totale nulle' % s)
    cnt = ctx.cnt
    cnt['incidences'] = cnt.get('incidences', 0) + len(wl)
    tin, _tout = ctx.euler()
    wnodes = sorted({v for (_b, _l, v) in wl}, key=lambda v: tin[v])
    cnt['noeuds_temoins'] = cnt.get('noeuds_temoins', 0) + len(wnodes)
    V = set(wnodes)
    for i in range(len(wnodes) - 1):
        V.add(ctx.lca(wnodes[i], wnodes[i + 1]))
    V = sorted(V, key=lambda v: tin[v])
    cnt['noeuds_virtuels'] = cnt.get('noeuds_virtuels', 0) + len(V)
    vchildren = {}
    stack = []
    for u in V:
        while stack and not ctx.is_ancestor(stack[-1], u):
            stack.pop()
        if stack:
            vchildren.setdefault(stack[-1], []).append(u)
        stack.append(u)
    events = {}
    for u in vchildren:
        events.setdefault(ctx.level(u), ([], []))[0].append(u)
    for (_b, lvl, v), w in zip(wl, weights):
        events.setdefault(fc.as_fraction(lvl), ([], []))[1].append((v, w))
    top = {u: u for u in V}
    mass = {u: Fraction(0) for u in V}

    def find(u):
        root = u
        while top[root] != root:
            root = top[root]
        while top[u] != root:
            top[u], u = root, top[u]
        return root

    active_total = Fraction(0)
    found = None
    for beta in sorted(events):
        fusions, acts = events[beta]
        touched = set()
        kinds = set()
        for u in fusions:
            ru = find(u)
            for c in vchildren[u]:
                rc = find(c)
                if rc != ru:
                    top[rc] = ru
                    mass[ru] += mass[rc]
            touched.add(ru)
            kinds.add('fusion')
        for v, w in acts:
            r = find(v)
            mass[r] += w
            active_total += w
            touched.add(r)
            kinds.add('activation')
        cnt['plateaux'] = cnt.get('plateaux', 0) + 1
        if not _plateau_is_candidate(kinds):
            continue
        winners = sorted({find(r) for r in touched if _majority(mass[find(r)], total, active_total)})
        if len(winners) > 1:
            raise fc.FrontierError('site %d : deux majorites strictes a %s (exclusivite violee)' % (s, beta))
        if found is None:
            if winners:
                r = winners[0]
                found = (beta, ctx.anc(r, beta), total, mass[r], r)
            continue
        # apres l'attache : la majorite acquise ne se perd pas, aucune autre composante n'en acquiert, et la
        # composante majoritaire est l'ancetre vivant du proprietaire (ascendance, preuve de l'audit continu, 8)
        r0 = find(found[4])
        require(_majority(mass[r0], total, active_total) and winners in ([], [r0]),
                'site %d : majorite perdue ou concurrente a %s (ascendance violee)' % (s, beta))
        require(ctx.anc(found[1], beta) == ctx.anc(r0, beta),
                'site %d : composante majoritaire hors de la lignee du proprietaire a %s' % (s, beta))
        cnt['controles_ascendance'] = cnt.get('controles_ascendance', 0) + 1
    if found is None:
        raise fc.FrontierError('site %d : aucune majorite apres tous les evenements' % s)
    return found[:4]


def _arm_majority(ctx, arm, mode):
    cnt = ctx.begin()
    dates, nodes, W, won = [], [], [], []
    for s in range(ctx.n):
        t, v, total, m = _majority_attach(ctx, s, mode)
        dates.append(t)
        nodes.append(v)
        W.append(total)
        won.append(m)
    info = {'W': W, 'masse_gagnante': won, 'poids': mode, 'theta': '1/2 strict',
            'univers': 'renforce' if _STRONG_UNIVERSE else 'complet'}
    return ArmResult(ctx, arm, {}, dates, nodes, cnt, info)


def arm_maj_uniform(ctx):
    """A3 : majorite stricte a masse fixe, poids uniformes (controle attendu en echec)."""
    return _arm_majority(ctx, 'maj_uniform', 'uniform')


def arm_maj_invbeta(ctx):
    """A4 : majorite stricte a masse fixe, poids 1/beta."""
    return _arm_majority(ctx, 'maj_invbeta', 'inverse_beta')


# ------------------------------------------------------------------ A5, A6 : couverture unique

def _unique_pass(ctx, arm, conflict_rule):
    cnt = ctx.begin()
    dates, nodes, n_comp, decided = [], [], [], []
    for s in range(ctx.n):
        alpha2 = ctx.cover_level(s)
        comps = _covering_components(ctx, s, alpha2)
        # lemme de couverture : memes composantes que TOUTES les boules couvrantes de population >= K a ce niveau, et
        # la composante de la premiere boule couvrante native (attache cover) en fait partie
        require(set(comps) == set(ctx.all_covering_at_alpha(s)),
                'site %d : composantes couvrantes (univers renforce) != toutes les boules a alpha_K^2' % s)
        native = ctx.anc(int(ctx.order.cover_node[s]), alpha2)
        require(native in comps, 'site %d : composante cover native hors des composantes couvrantes' % s)
        n_comp.append(len(comps))
        if len(comps) == 1:
            dates.append(alpha2)
            nodes.append(comps[0])
            decided.append('unique')
            continue
        cnt['conflits'] = cnt.get('conflits', 0) + 1
        t, v, how = conflict_rule(ctx, s, alpha2, comps)
        dates.append(t)
        nodes.append(v)
        decided.append(how)
    info = {'composantes_couvrantes_a_alpha': n_comp, 'decision': decided,
            'conflits': sum(1 for c in n_comp if c >= 2)}
    return ArmResult(ctx, arm, {}, dates, nodes, cnt, info)


def arm_unique_else_lca(ctx):
    """A5 : composante couvrante unique a alpha_K(x)^2 -> attache immediate jamais revoquee ; sinon LCA."""
    def rule(ctx, s, alpha2, comps):
        t, v = _unique_conflict_attach(ctx, alpha2, comps)
        return t, v, 'lca'
    return _unique_pass(ctx, 'unique_else_lca', rule)


def arm_unique_else_maj_invbeta(ctx):
    """A6 : composante couvrante unique -> attache immediate ; vrais conflits -> majorite fixe 1/beta (A4)."""
    def rule(ctx, s, alpha2, comps):
        t, v, _W, _m = _majority_attach(ctx, s, 'inverse_beta')
        require(t >= alpha2, 'site %d : majorite avant alpha_K^2' % s)
        return t, v, 'majorite'
    return _unique_pass(ctx, 'unique_else_maj_invbeta', rule)


# ------------------------------------------------------------------ A2 : ancrage K2 (contrat adopte)

def abs_diff_sqrt_gt(a, c, b, k, e):
    """Vrai ssi |sqrt(a) - c sqrt(b)| > k sqrt(e), exactement ; a, b, e >= 0 et c, k >= 0 rationnels.

    |sqrt a - c sqrt b|^2 = a + c^2 b - 2 c sqrt(a b) ; la condition equivaut a A > 2 c sqrt(a b) avec
    A = a + c^2 b - k^2 e, soit A > 0 et A^2 > 4 c^2 a b."""
    a, b, e = fc.as_fraction(a), fc.as_fraction(b), fc.as_fraction(e)
    c, k = Fraction(c), Fraction(k)
    require(a >= 0 and b >= 0 and e >= 0 and c >= 0 and k >= 0, 'certificat : arguments negatifs')
    A = a + c * c * b - k * k * e
    return A > 0 and A * A > 4 * c * c * a * b


def _second_distance(ctx, s):
    """Deuxieme plus petite distance carree de x aux autres sites (egalites comptees), None si n = 2."""
    if ctx.n < 3:
        return None
    ctx.cnt['requetes_voisins'] += 1
    return ctx.grid().k_nearest(ctx.sites[s], 3)[2][0]


def _outer_distance(ctx, s, n_members):
    """Plus petite distance carree strictement hors bande (None si tous les sites sont dans la bande)."""
    if n_members + 1 >= ctx.n:
        return None
    ctx.cnt['requetes_voisins'] += 1
    return ctx.grid().k_nearest(ctx.sites[s], n_members + 2)[n_members + 1][0]


def arm_anchor_eta(ctx, eta, eps2_values=DEFAULT_EPS2, max_candidates=None):
    """A2 (K = 2) : ancrage des ambiguites, contrat adopte mot pour mot. Publie aussi la marge a la frontiere de
    bande et, pour chaque eps2, le nombre de sites certifiables : eta > 0 : |g_xy| > (2 + eta) eps pour toutes les
    paires incidentes (g_xy = |x - y|/2 - (1 + eta) alpha_min(x)) ; eta = 0 : ecart entre deuxieme et premiere
    demi-distance > 2 eps ; egalites exactes comptees a part. Un plafond `max_candidates` depasse leve ArmRefusal."""
    if ctx.k != 2:
        raise ArmRefusal('anchor_eta : K = 2 seulement (K = %d)' % ctx.k)
    eta = Fraction(eta)
    if eta < 0:
        raise ArmRefusal('anchor_eta : eta negatif')
    eps2_values = tuple(fc.as_fraction(e) for e in eps2_values)
    cnt = ctx.begin()
    res = ctx.new_resolver()
    dates, nodes, cands = [], [], []
    n_members, inner_d2, outer_d2, m2s, second_d2 = [], [], [], [], []
    tie_frontier, tie_minimum = [], []
    certifiable = {str(e): [] for e in eps2_values}
    for s in range(ctx.n):
        m2, thr, members = _band_members(ctx, s, eta)
        require(members, 'site %d : bande vide' % s)
        if max_candidates is not None and len(members) > max_candidates:
            raise ArmRefusal('site %d : %d paires candidates > plafond %d (refus, pas de troncature)'
                             % (s, len(members), max_candidates))
        P = ctx.sites[s]
        d2 = [fc.sq_dist(P, ctx.sites[y]) for y in members]
        cnt['paires_candidates'] = cnt.get('paires_candidates', 0) + len(members)
        cnt['max_candidats'] = max(cnt.get('max_candidats', 0), len(members))
        cands.append(list(members))
        comp = [res.resolve(s, y) for y in members]
        cnt['resolutions'] = cnt.get('resolutions', 0) + len(members)
        J = _anchor_join(ctx, comp)
        beta_max = Fraction(max(d2), 4)
        e = _anchor_date(ctx.level(J), beta_max)
        dates.append(e)
        nodes.append(ctx.anc(J, e))
        # marges et certificats (exacts ; affichage seulement en flottant)
        inner = max(d2)
        outer = _outer_distance(ctx, s, len(members))
        second = _second_distance(ctx, s)
        m2s.append(m2)
        n_members.append(len(members))
        inner_d2.append(inner)
        outer_d2.append(outer)
        second_d2.append(second)
        tie_frontier.append(eta > 0 and inner == thr)
        tie_minimum.append(second is not None and second == m2)
        for e2 in eps2_values:
            if eta == 0:
                ok = second is None or abs_diff_sqrt_gt(second, 1, m2, 4, e2)
            else:
                kk = 2 * (2 + eta)
                ok = abs_diff_sqrt_gt(inner, 1 + eta, m2, kk, e2)
                if outer is not None:
                    ok = ok and abs_diff_sqrt_gt(outer, 1 + eta, m2, kk, e2)
            certifiable[str(e2)].append(bool(ok))
    cnt['pas_de_descente'] = res.steps
    cnt['naissances_atteintes'] = res.births_reached
    cnt['recensements'] = res.census_calls
    cnt['memo_paires'] = len(res.memo)
    margins = []
    for s in range(ctx.n):
        a = m2s[s] ** 0.5 / 2
        if eta == 0:
            margins.append(None if second_d2[s] is None else second_d2[s] ** 0.5 / 2 - a)
        else:
            g_in = abs(inner_d2[s] ** 0.5 / 2 - float(1 + eta) * a)
            g_out = None if outer_d2[s] is None else abs(outer_d2[s] ** 0.5 / 2 - float(1 + eta) * a)
            margins.append(g_in if g_out is None else min(g_in, g_out))
    finite = [m for m in margins if m is not None]
    info = {
        'eta': eta,
        'candidats': cands,
        'n_candidats': n_members,
        'm2': m2s,
        'd2_interieur_max': inner_d2,
        'd2_exterieur_min': outer_d2,
        'd2_deuxieme': second_d2,
        'egalite_frontiere_bande': tie_frontier,
        'egalite_minimum': tie_minimum,
        'n_egalites_frontiere_bande': sum(1 for t in tie_frontier if t),
        'n_egalites_minimum': sum(1 for t in tie_minimum if t),
        'marge_affichage_min': None if not finite else float('%.9g' % min(finite)),
        'marge_definition': ('eta = 0 : (sqrt(d2_deuxieme) - sqrt(m2))/2 ; eta > 0 : min |g_xy| sur les paires '
                             'extremes de part et d autre du seuil (rayons, affichage flottant)'),
        'certifiables': {k: sum(1 for b in v if b) for k, v in certifiable.items()},
        'certifiable_par_site': certifiable,
        'n_sites': ctx.n,
    }
    return ArmResult(ctx, 'anchor_eta', {'eta': eta}, dates, nodes, cnt, info)


# ------------------------------------------------------------------ execution groupee

def run_arm(ctx, arm, **params):
    if arm == 'core':
        return arm_core(ctx)
    if arm == 'cover':
        return arm_cover(ctx)
    if arm == 'anchor_eta':
        return arm_anchor_eta(ctx, params.get('eta', 0), params.get('eps2_values', DEFAULT_EPS2),
                              params.get('max_candidates'))
    if arm == 'maj_uniform':
        return arm_maj_uniform(ctx)
    if arm == 'maj_invbeta':
        return arm_maj_invbeta(ctx)
    if arm == 'unique_else_lca':
        return arm_unique_else_lca(ctx)
    if arm == 'unique_else_maj_invbeta':
        return arm_unique_else_maj_invbeta(ctx)
    raise ArmRefusal('bras inconnu : %r' % (arm,))


def run_all(ctx, etas=DEFAULT_ETAS, eps2_values=DEFAULT_EPS2):
    """Tous les bras applicables a l'ordre de l'export ; anchor_eta pour chaque eta si K = 2. Rend {cle: resultat}."""
    out = {}
    for arm in ARMS:
        if arm == 'anchor_eta':
            if ctx.k != 2:
                continue
            for eta in etas:
                r = arm_anchor_eta(ctx, eta, eps2_values)
                out[r.key] = r
            continue
        r = run_arm(ctx, arm)
        out[r.key] = r
    return out


# ------------------------------------------------------------------ controles et diagnostic de masse

def majority_owners_bruteforce(ctx, mode, beta):
    """Proprietaire majoritaire recalcule coupe par coupe (D1.3), par site : noeud vivant a beta ou None.
    Leve FrontierError si deux composantes ont une majorite stricte (exclusivite)."""
    beta = fc.as_fraction(beta)
    out = []
    for s, wl in enumerate(ctx.witnesses()):
        weights = [_weight(mode, lvl) for (_b, lvl, _v) in wl]
        total = sum(weights, Fraction(0))
        masses = {}
        for (_b, lvl, v), w in zip(wl, weights):
            if lvl <= beta:
                c = ctx.forest.ancestor(v, beta, True)
                masses[c] = masses.get(c, Fraction(0)) + w
        owners = [c for c, m in masses.items() if 2 * m > total]
        require(len(owners) <= 1, 'site %d : deux majorites strictes a %s' % (s, beta))
        out.append(owners[0] if owners else None)
    return out


def covering_components_at(ctx, s, beta):
    """Toutes les composantes vivantes a beta qui couvrent le site s (univers renforce, dedupliquees)."""
    return fc.covering_components(ctx.forest, ctx.witnesses()[s], beta)


def fractional_masses(ctx, mode, beta):
    """Diagnostic D2bis (pas un bras) : chaque point repartit une masse unite sur ses temoins au prorata des poids,
    W_x fixe (temoins inactifs compris) ; une part est active a partir de la date de son temoin et suit les
    ancetres de son noeud. Rend (masses {noeud vivant a beta : Fraction}, reserve inactive) avec l'identite exacte
    somme des masses + reserve = n (controlee). W_x = 0 : refus."""
    beta = fc.as_fraction(beta)
    masses = {}
    reserve = Fraction(0)
    for s, wl in enumerate(ctx.witnesses()):
        weights = [_weight(mode, lvl) for (_b, lvl, _v) in wl]
        total = sum(weights, Fraction(0))
        if total <= 0:
            raise ArmRefusal('D2bis : site %d sans temoin (W_x = 0)' % s)
        active = Fraction(0)
        for (_b, lvl, v), w in zip(wl, weights):
            if lvl <= beta:
                c = ctx.forest.ancestor(v, beta, True)
                share = w / total
                masses[c] = masses.get(c, Fraction(0)) + share
                active += share
        reserve += 1 - active
    require(sum(masses.values(), Fraction(0)) + reserve == ctx.n, 'D2bis : identite de masse violee')
    return masses, reserve


def check_nesting(result, cuts):
    """D1 : partitions emboitees de coupe en coupe (liste croissante de coupes fermees), plus un seul bloc au-dela
    du dernier evenement. Rend le nombre d'inclusions de blocs controlees."""
    prev = None
    checked = 0
    for beta in cuts:
        blocks = result.blocks(beta)
        if prev is not None:
            require(fc.nested(prev, blocks), '%s : bloc scinde entre deux coupes (%s)' % (result.key, beta))
            checked += len(prev)
        prev = blocks
    last = max(result.events()) + 1
    require(len(result.blocks(last)) == 1, '%s : plus d un bloc au-dela du dernier evenement' % result.key)
    return checked


# ------------------------------------------------------------------ ligne de commande

def _parse_fraction(text):
    try:
        v = Fraction(text)
    except (ValueError, ZeroDivisionError):
        raise ArmRefusal('rationnel invalide : %r' % (text,))
    return v


def main(argv):
    ap = argparse.ArgumentParser(description='bras d affectation (experience frontiere v10)')
    ap.add_argument('export')
    ap.add_argument('--arm', required=True, choices=ARMS)
    ap.add_argument('--eta', default='0')
    ap.add_argument('--eps2', default=','.join(str(e) for e in DEFAULT_EPS2))
    ap.add_argument('--max-candidates', type=int, default=None)
    ap.add_argument('--out', required=True)
    try:
        args = ap.parse_args(argv)
    except SystemExit:
        return 2
    try:
        if os.path.exists(args.out):
            raise ArmRefusal('sortie existante : %s' % args.out)
        e = fc.load_export(args.export)
        ctx = ArmContext(e)
        params = {}
        if args.arm == 'anchor_eta':
            params = dict(eta=_parse_fraction(args.eta),
                          eps2_values=tuple(_parse_fraction(t) for t in args.eps2.split(',') if t),
                          max_candidates=args.max_candidates)
        r = run_arm(ctx, args.arm, **params)
        r.validate()
        blob = json.dumps(r.to_json(e.sha256), sort_keys=True, indent=1) + '\n'
        with open(args.out, 'w') as f:
            f.write(blob)
    except ArmRefusal as err:
        print('REFUS : %s' % err, file=sys.stderr)
        return 2
    except fc.FrontierError as err:
        print('INCOHERENCE : %s' % err, file=sys.stderr)
        return 1
    print(json.dumps({'status': 'ok', 'arm': r.key, 'K': r.k, 'n': len(r.dates), 'counters': r.counters},
                     sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
