"""Selection sans hierarchie de points : condensation N-aire et EOM sur l'arbre FULL avec masses fractionnaires,
puis vote sur l'antichaine selectionnee (proposition 7 du manuscrit, transposee aux unites de branche).

Masses (lambda = phi = beta^(-z/2), meme echelle que la tete EOM) :
  - fractionnaire, par marches ('step')   : l'unite (x, u) entre entiere a c_x(u) ;
  - fractionnaire, progressive ('gradual'): l'unite (x, u) s'accumule de c_x(u) a d_u ;
  - dure ('hard', controle)               : masse 1 par point, entree a t(x) dans o(x) (hierarchie dure donnee).
A la mort d'un noeud, les deux masses fractionnaires coincident (toutes les unites du sous-arbre sont acquises) :
la condensation (gros/petits enfants) est donc la meme ; seules les stabilites different.
Condensation : enfants gros si masse >= mcs ; >= 2 gros : scission ; 1 gros : continuation ; 0 : fin.
EOM : R(C) = max(S(C), somme R(enfants)) ; egalite -> parent ; racine exclue (allow_single_cluster = faux).
"""
from fractions import Fraction

from fullk import require
from scale import cmp
import participation as PA


class NodeStats:
    """end_mass[v] (masse apportee au parent), stab[v] (integrale de la masse sur la vie de v, en lambda)."""

    def __init__(self, end_mass, stab):
        self.end_mass, self.stab = end_mass, stab


def fractional_stats(T, cover, phi, mode):
    unit = PA.units(T, cover, phi)
    zero = phi.zero()
    end_mass = [zero] * len(T)
    stab = [zero] * len(T)
    for x, cv in enumerate(cover):
        w, W = unit[x]
        base = PA.bases(T, cv, w)
        for v, c in cv.items():
            lb, ld, lc = phi(T.birth[v]), phi(T.death[v]), phi(c)
            end_mass[v] += (base[v] + w[v]) / W
            part = base[v] * (lb - ld)
            if mode == 'step':
                part += w[v] * (lc - ld)
            elif mode == 'gradual':
                part += (lc - ld) * (lc - ld) / 2
            else:
                raise ValueError(mode)
            stab[v] += part / W
    return NodeStats(end_mass, stab)


def hard_stats(T, atts, phi):
    """Masse 1 par point attache : x est present dans o(x) des t(x), puis dans chaque ancetre des sa naissance."""
    zero = phi.zero()
    end_mass = [zero] * len(T)
    stab = [zero] * len(T)
    for a in atts:
        if a.owner is None:
            continue
        v, start = a.owner, a.phi_date
        while True:
            end_mass[v] += 1
            stab[v] += start - phi(T.death[v])
            p = T.parent[v]
            if p < 0:
                break
            v, start = p, phi(T.birth[p])
    return NodeStats(end_mass, stab)


def condense(T, stats, mcs):
    """Arbre condense : liste de clusters {'top', 'chain' (noeuds du haut vers le bas), 'children'}."""
    clusters = []

    def new_cluster(top, parent):
        clusters.append({'top': top, 'chain': [], 'children': [], 'parent': parent})
        if parent is not None:
            clusters[parent]['children'].append(len(clusters) - 1)
        return len(clusters) - 1

    stack = [(T.root, new_cluster(T.root, None))]
    while stack:
        v, cid = stack.pop()
        clusters[cid]['chain'].append(v)
        big = [c for c in T.children[v] if cmp(stats.end_mass[c], Fraction(mcs), soft=True) >= 0]
        if len(big) >= 2:
            for c in sorted(big):
                stack.append((c, new_cluster(c, cid)))
        elif len(big) == 1:
            stack.append((big[0], cid))
    return clusters


def eom(T, stats, clusters):
    """Selection EOM ; rend la liste des indices de clusters selectionnes (antichaine), racine exclue."""
    S = []
    for cl in clusters:
        s = stats.stab[cl['chain'][0]] - stats.stab[cl['chain'][0]]
        for v in cl['chain']:
            s += stats.stab[v]
        S.append(s)
    R = [None] * len(clusters)
    selected = [False] * len(clusters)
    order = sorted(range(len(clusters)), key=lambda i: -depth_of(clusters, i))
    for i in order:
        ch = clusters[i]['children']
        if not ch:
            R[i] = S[i]
            selected[i] = True
            continue
        tot = R[ch[0]]
        for j in ch[1:]:
            tot = tot + R[j]
        if i != 0 and cmp(S[i], tot, soft=True) >= 0:
            R[i] = S[i]
            selected[i] = True
            unselect(clusters, i, selected)
        else:
            R[i] = tot
    selected[0] = False
    chosen = [i for i in range(len(clusters)) if selected[i] and not ancestor_selected(clusters, i, selected)]
    return chosen, S


def depth_of(clusters, i):
    d = 0
    while clusters[i]['parent'] is not None:
        i = clusters[i]['parent']
        d += 1
    return d


def unselect(clusters, i, selected):
    stack = list(clusters[i]['children'])
    while stack:
        j = stack.pop()
        selected[j] = False
        stack.extend(clusters[j]['children'])


def ancestor_selected(clusters, i, selected):
    j = clusters[i]['parent']
    while j is not None:
        if selected[j] and j != 0:
            return True
        j = clusters[j]['parent']
    return False


def selected_top_map(T, clusters, chosen):
    """sel[v] = indice k du cluster choisi dont le top est un ancetre-ou-egal de v, sinon None."""
    tops = {clusters[i]['top']: k for k, i in enumerate(chosen)}
    sel = [None] * len(T)
    for v in T.topdown:
        if v in tops:
            sel[v] = tops[v]
        elif T.parent[v] >= 0:
            sel[v] = sel[T.parent[v]]
    return sel


def vote_labels(T, cover, phi, clusters, chosen):
    """Vote fractionnaire : V_x(c) = part de x dans le sous-arbre de top(c) ; argmax unique > 0, sinon -1."""
    unit = PA.units(T, cover, phi)
    sel = selected_top_map(T, clusters, chosen)
    labels, margins = [], []
    for x, cv in enumerate(cover):
        w, W = unit[x]
        V = {}
        for u in cv:
            k = sel[u]
            if k is not None:
                V[k] = V.get(k, w[u] - w[u]) + w[u]
        if not V:
            labels.append(-1)
            margins.append(None)
            continue
        keys = sorted(V)
        best = keys[0]
        for k in keys[1:]:
            if cmp(V[k], V[best], soft=True) > 0:
                best = k
        ties = [k for k in keys if cmp(V[k], V[best], soft=True) == 0]
        if len(ties) > 1:
            labels.append(-1)
            margins.append(0.0)
        else:
            labels.append(best)
            second = max((float(V[k]) for k in keys if k != best), default=0.0)
            margins.append((float(V[best]) - second) / float(W))
    return labels, margins


def hard_labels(T, atts, clusters, chosen):
    """Etiquette dure : cluster choisi dont le top est un ancetre-ou-egal du proprietaire (le proprietaire est
    vivant a t(x), donc t(x) < mort du top automatiquement) ; sinon -1."""
    sel = selected_top_map(T, clusters, chosen)
    return [-1 if a.owner is None or sel[a.owner] is None else sel[a.owner] for a in atts]


def ari(a, b):
    """Indice de Rand ajuste exact (Fraction) ; le bruit (-1) est une classe comme les autres."""
    from collections import Counter
    n = len(a)
    require(n == len(b) and n >= 2, 'ARI : tailles')

    def c2(k):
        return k * (k - 1) // 2
    cont = Counter(zip(a, b))
    sa = Counter(a)
    sb = Counter(b)
    index = sum(c2(v) for v in cont.values())
    ea = sum(c2(v) for v in sa.values())
    eb = sum(c2(v) for v in sb.values())
    expected = Fraction(ea * eb, c2(n))
    maxi = Fraction(ea + eb, 2)
    if maxi == expected:
        return Fraction(1)
    return (index - expected) / (maxi - expected)


def fractional_stats_fast(T, cover, phi, mode):
    """Meme resultat que fractional_stats, par segments du squelette (lambda = phi)."""
    zero = phi.zero()
    end_mass = [zero] * len(T)
    stab = [zero] * len(T)
    for x, cv in enumerate(cover):
        segs, W = PA.segments(T, cv, phi)
        for sg in segs:
            ps = phi(sg.s)
            v = sg.a
            while True:
                lb, ld = phi(T.birth[v]), phi(T.death[v])
                end_mass[v] += (sg.base + ps - ld) / W
                if v == sg.a:
                    part = sg.base * (lb - ld)
                    if mode == 'step':
                        part += (ps - ld) * (ps - ld)
                    else:
                        part += (ps - ld) * (ps - ld) / 2
                else:
                    if mode == 'step':
                        part = (sg.base + ps - ld) * (lb - ld)
                    else:
                        part = sg.base * (lb - ld) + ps * (lb - ld) - (lb * lb - ld * ld) / 2
                stab[v] += part / W
                if v == sg.top_node:
                    break
                v = T.parent[v]
    return NodeStats(end_mass, stab)


def vote_labels_fast(T, cover, phi, clusters, chosen):
    """Meme vote que vote_labels, par segments : la part de x dans Sub(top(c)) est la somme des portions de ses
    chaines contenues dans ce sous-arbre."""
    sel = selected_top_map(T, clusters, chosen)
    labels, margins = [], []
    for x, cv in enumerate(cover):
        segs, W = PA.segments(T, cv, phi)
        V = {}
        for sg in segs:
            ps = phi(sg.s)
            v = sg.a
            # portion de la chaine sous chaque top choisi : du debut jusqu'a la mort du dernier noeud sous ce top
            while True:
                k = sel[v]
                if k is not None:
                    # remonter tant que l'on reste sous le meme top choisi
                    u = v
                    while u != sg.top_node and sel[T.parent[u]] == k:
                        u = T.parent[u]
                    start = ps if v == sg.a else phi(T.birth[v])
                    V[k] = V.get(k, W - W) + start - phi(T.death[u])
                    if u == sg.top_node:
                        break
                    v = T.parent[u]
                    continue
                if v == sg.top_node:
                    break
                v = T.parent[v]
        if not V:
            labels.append(-1)
            margins.append(None)
            continue
        keys = sorted(V)
        best = keys[0]
        for k in keys[1:]:
            if cmp(V[k], V[best], soft=True) > 0:
                best = k
        ties = [k for k in keys if cmp(V[k], V[best], soft=True) == 0]
        if len(ties) > 1:
            labels.append(-1)
            margins.append(0.0)
        else:
            labels.append(best)
            second = max((float(V[k]) for k in keys if k != best), default=0.0)
            margins.append((float(V[best]) - second) / float(W))
    return labels, margins
