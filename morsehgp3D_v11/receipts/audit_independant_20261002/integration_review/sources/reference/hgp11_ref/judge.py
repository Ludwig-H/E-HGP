"""Juge : l'etage B (constructive.Reference) doit EGALER l'etage A (definition.Definition).

Comparaison champ par champ du modele canonique, par ordre : arbre de fusion (niveaux, arites, boules de
naissance), application verticale, entrees core, entrees cover (niveau et ensemble de TOUS les noeuds couvrants),
coupes ouvertes et fermees a chaque niveau d'evenement de l'un ou l'autre etage (par noeud vivant : points couverts
et points de coeur). Avant la comparaison, chaque resultat passe un controle de coherence interne, ecrit ici une
troisieme fois (noeuds vivants recalcules par niveaux).

Tout ecart est rendu comme texte : aucune instruction d'assertion du langage.
"""
from .constructive import Reference
from .definition import Definition
from .model import InvariantError, validate_tree

COUNTERS = ('clouds', 'orders', 'cuts', 'levels', 'nodes', 'births', 'merges', 'nary_merges', 'plateau_levels',
            'mixed_levels', 'verticals', 'core_entries', 'core_at_node_level', 'cover_entries', 'cover_ties',
            'balls', 'extended_balls', 'weighted_balls', 'general_births', 'general_joins', 'general_inert',
            'jumps', 'descents', 'steps', 'adhoc_spheres', 'weighted_clouds')


def new_counters():
    return dict((name, 0) for name in COUNTERS)


def _alive(res, level):
    """Noeuds vivants a la coupe fermee, relus sur les niveaux : nes au plus tard a la coupe, parent absent ou ne
    apres."""
    return set(v for v, node in enumerate(res.nodes)
               if node.level <= level and (res.parent[v] < 0 or res.nodes[res.parent[v]].level > level))


def coherence(res, n, prev=None):
    """Coherence interne d'un OrderResult sur n points (prev : resultat de l'ordre k - 1). None, ou la faute."""
    fault = validate_tree(res.nodes)
    if fault:
        return fault
    full = (1 << n) - 1
    # rangs entiers des niveaux de coupe ; chaque noeud nait a un niveau de coupe
    rank = dict((cut.level, i) for i, cut in enumerate(res.cuts))
    if len(rank) != len(res.cuts) or any(res.cuts[i - 1].level >= res.cuts[i].level for i in range(1, len(res.cuts))):
        return 'niveaux de coupe non strictement croissants'
    born = [[] for _ in res.cuts]
    for v, node in enumerate(res.nodes):
        if node.level not in rank:
            return 'noeud %d : niveau %s absent des coupes' % (v, node.level)
        born[rank[node.level]].append(v)
    alive = set()
    last = ()
    for i, cut in enumerate(res.cuts):
        if cut.opened != last:
            return 'coupe ouverte a %s differente de la coupe fermee precedente' % cut.level
        last = cut.closed
        for v in born[i]:  # coupe fermee : les noeuds de ce niveau naissent, leurs enfants meurent
            alive.add(v)
            alive.difference_update(res.nodes[v].children)
        if [e[0] for e in cut.closed] != sorted(alive):
            return 'coupe fermee a %s : noeuds listes differents des noeuds vivants' % cut.level
        seen = 0
        for _v, cov, cor in cut.closed:
            if cov & ~full or cor & ~cov or cov == 0:
                return 'coupe a %s : coeur hors de la couverture, couverture vide ou point inconnu' % cut.level
            if cor & seen:
                return 'coupe a %s : un point dans le coeur de deux composantes' % cut.level
            seen |= cor
    if not res.cuts or len(res.cuts[-1].closed) != 1 or res.cuts[-1].closed[0][1:] != (full, full):
        return 'derniere coupe : la racine ne porte pas tous les points'
    if len(res.core) != n or len(res.cover) != n:
        return 'entrees : %d core et %d cover pour %d points' % (len(res.core), len(res.cover), n)
    for x in range(n):
        bit = 1 << x
        core, cover = res.core[x], res.cover[x]
        opened, shut = res.cut(core.level)
        if any(cor & bit for _v, _cov, cor in opened) or [v for v, _cov, cor in shut if cor & bit] != [core.nodes]:
            return 'point %d : entree core (%s, noeud %s) contredite par les coupes' % (x, core.level, core.nodes)
        opened, shut = res.cut(cover.level)
        if any(cov & bit for _v, cov, _cor in opened) or set(v for v, cov, _cor in shut if cov & bit) != cover.nodes:
            return 'point %d : entree cover (%s, %s) contredite par les coupes' % (x, cover.level, sorted(cover.nodes))
        if not cover.nodes or not cover.level <= core.level <= 4 * cover.level:
            return 'point %d : alpha^2 = %s et D_k = %s hors de alpha^2 <= D_k <= 4 alpha^2' % (x, cover.level, core.level)
    if (res.lower is None) != (res.k == 1) or (prev is None) != (res.k == 1):
        return 'application verticale absente ou en trop'
    if res.lower is not None:
        if len(res.lower) != len(res.nodes):
            return 'application verticale : %d images pour %d noeuds' % (len(res.lower), len(res.nodes))
        for v, w in enumerate(res.lower):
            if w is None or not 0 <= w < len(prev.nodes) or w not in _alive(prev, res.nodes[v].level):
                return 'noeud %d : image verticale %s non vivante a la coupe fermee %s' % (v, w, res.nodes[v].level)
    return None


def compare_orders(a, b):
    """Ecarts entre le resultat de l'etage A et celui de l'etage B pour un meme ordre (liste de textes)."""
    out = []
    if a.nodes != b.nodes:
        i = next((i for i, (x, y) in enumerate(zip(a.nodes, b.nodes)) if x != y), min(len(a.nodes), len(b.nodes)))
        out.append('arbre : %d noeuds contre %d, premier ecart au noeud %d : %s contre %s' % (
            len(a.nodes), len(b.nodes), i, a.nodes[i] if i < len(a.nodes) else None,
            b.nodes[i] if i < len(b.nodes) else None))
        return out  # les identifiants ne sont plus comparables
    if a.lower != b.lower:
        i = next(i for i, (x, y) in enumerate(zip(a.lower, b.lower)) if x != y)
        out.append('verticale du noeud %d : %s contre %s' % (i, a.lower[i], b.lower[i]))
    for name, ea, eb in (('core', a.core, b.core), ('cover', a.cover, b.cover)):
        if ea != eb:
            x = next(x for x, (u, v) in enumerate(zip(ea, eb)) if u != v)
            out.append('entree %s du point %d : %s contre %s' % (name, x, tuple(ea[x]), tuple(eb[x])))
    for level in sorted(set(c.level for c in a.cuts) | set(c.level for c in b.cuts)):
        ca, cb = a.cut(level), b.cut(level)
        for which, ua, ub in (('ouverte', ca[0], cb[0]), ('fermee', ca[1], cb[1])):
            if ua != ub:
                out.append('coupe %s a %s : %s contre %s' % (which, level, ua, ub))
                return out
    return out


def count_order(a, cnt):
    """Compteurs de non-vacuite lus sur le resultat de l'etage A d'un ordre."""
    cnt['orders'] += 1
    cnt['levels'] += len(a.cuts)
    cnt['nodes'] += len(a.nodes)
    merge_levels = {}
    birth_levels = set()
    for node in a.nodes:
        if node.center is None:
            cnt['merges'] += 1
            cnt['nary_merges'] += len(node.children) >= 3
            merge_levels[node.level] = merge_levels.get(node.level, 0) + 1
        else:
            cnt['births'] += 1
            birth_levels.add(node.level)
    cnt['plateau_levels'] += sum(1 for c in merge_levels.values() if c >= 2)
    cnt['mixed_levels'] += sum(1 for lv in merge_levels if lv in birth_levels)
    if a.lower is not None:
        cnt['verticals'] += len(a.lower)
    node_levels = set(node.level for node in a.nodes)
    cnt['core_entries'] += len(a.core)
    cnt['core_at_node_level'] += sum(1 for e in a.core if e.level in node_levels)
    cnt['cover_entries'] += len(a.cover)
    cnt['cover_ties'] += sum(1 for e in a.cover if len(e.nodes) >= 2)


def compare_cloud(points, kmax, cnt=None):
    """Confronte B a A sur un nuage, ordres 1 .. min(kmax, n). Rend (ecarts, A, B) ; ecarts vide si conforme."""
    cnt = cnt if cnt is not None else new_counters()
    errors = []
    truth = Definition(points)
    try:
        ref = Reference(points, kmax)
    except InvariantError as exc:
        return ['etage B, catalogue : invariant viole : %s' % exc], truth, None
    n = truth.n
    prev_a = prev_b = None
    for k in range(1, min(kmax, n) + 1):
        a = truth.order(k)
        fault = coherence(a, n, prev_a)
        if fault:
            errors.append('k=%d etage A incoherent : %s' % (k, fault))
            break
        try:
            b = ref.order(k)
        except InvariantError as exc:
            errors.append('k=%d etage B : invariant viole : %s' % (k, exc))
            break
        fault = coherence(b, n, prev_b)
        if fault:
            errors.append('k=%d etage B incoherent : %s' % (k, fault))
            break
        diffs = compare_orders(a, b)
        errors.extend('k=%d %s' % (k, d) for d in diffs)
        if diffs:
            break
        # regle de depart de la v10 pour cover : la composante retenue est l'une des composantes couvrantes
        stray = [x for x, (_ball, node) in enumerate(ref.cover_choice[k]) if node not in a.cover[x].nodes]
        if stray:
            errors.append('k=%d premiere boule couvrante du point %d hors des composantes couvrantes' % (k, stray[0]))
            break
        cnt['cuts'] += 2 * len(set(c.level for c in a.cuts) | set(c.level for c in b.cuts))
        count_order(a, cnt)
        prev_a, prev_b = a, b
    cnt['clouds'] += 1
    cnt['weighted_clouds'] += len(ref.sites) < n
    cnt['balls'] += len(ref.balls)
    cnt['extended_balls'] += sum(1 for ball in ref.balls if ball.extended)
    cnt['weighted_balls'] += sum(1 for ball in ref.balls if ball.weighted)
    for name, key in (('general_births', 'ext_births'), ('general_joins', 'ext_joins'), ('general_inert', 'ext_inert'),
                      ('jumps', 'jumps'), ('descents', 'descents'), ('steps', 'steps'), ('adhoc_spheres', 'adhoc')):
        cnt[name] += ref.stats[key]
    return errors, truth, ref
