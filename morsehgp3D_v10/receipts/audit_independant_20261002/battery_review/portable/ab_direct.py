"""Batterie des niveaux A et B (1er octobre 2026) : comparaison DIRECTE de deux hierarchies de points, sans verite, et
formes RESIDUELLES. Aucun z, aucune selection, aucune coupe plate : seulement les arbres.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, mode=benchmark_only, public_status=not_claimed.

Objets. Un arbre adapte E (tvp_core.FlatTree.E : evenements, parent d'indice plus grand, racine en dernier, un point
attache a son evenement d'entree) ; ses BLOCS sont les ensembles de points des sous-arbres.
  - clusters condenses a la taille s : un enfant ch d'un evenement est un cluster ssi sa masse est >= s et qu'un AUTRE
    enfant du meme evenement a une masse >= s (c'est la regle de condense_pr.condense : au moins deux grandes
    branches). Le seuil tau(ch) = min(masse(ch), plus grande masse d'un frere) resume toutes les tailles : ch est un
    cluster a la taille s ssi tau(ch) >= s. Les clusters a la taille s' >= s sont donc un sous-ensemble de ceux a la
    taille s. Les masses comptent tous les points (comme la condensation), l'evaluation ignore les points de verite -2.
  - own_s(c) : points du cluster c qui ne sont dans aucun de ses clusters enfants a la taille s (la racine compte
    comme un cluster : son residu est ce qui n'entre dans aucun cluster).
  - meilleur bloc d'un arbre B pour une cible t (ensemble de points) : max de l'IoU sur les blocs de B. Les seuls
    candidats utiles sont l'evenement d'entree de chaque point de t et le plus petit bloc contenant deux points de t
    consecutifs dans l'ordre de parcours de B (tout bloc qui coupe t contient une plage de cet ordre ; le plus petit
    bloc contenant la meme plage a la meme intersection et une taille au plus egale).
Aucune decision ne repose sur un assert (identique sous python3 -O).
"""
import numpy as np


def need(ok, why):
    if not ok:
        raise ValueError(why)


class Layout:
    """Disposition des points d'un arbre adapte : chaque evenement v couvre les positions [lo[v], hi[v][ (ses points
    propres d'abord, puis ses enfants). weight : 1 si le point est evalue (verite != -2), 0 sinon."""

    def __init__(self, E, weight):
        nE = len(E['rank'])
        parent = np.array([-1 if p is None else p for p in E['parent']], dtype=np.int64)
        need(nE >= 1 and parent[-1] == -1 and int((parent < 0).sum()) == 1, 'une seule racine, la derniere')
        need(bool(np.all(parent[:-1] > np.arange(nE - 1))), 'parent d indice plus grand que l enfant')
        target = np.asarray(E['target'], dtype=np.int64)
        n = len(target)
        weight = np.asarray(weight, dtype=np.int64)
        need(len(weight) == n and bool(((weight == 0) | (weight == 1)).all()), 'poids 0 ou 1 par point')
        own = np.bincount(target, minlength=nE).astype(np.int64)
        mass = own.copy()
        pl = parent.tolist()
        ml = mass.tolist()
        for v in range(nE - 1):  # enfants avant parents
            ml[pl[v]] += ml[v]
        mass = np.asarray(ml, dtype=np.int64)
        need(int(mass[-1]) == n, 'masse de la racine')
        lo = [0] * nE
        cur = [0] * nE
        depth = [0] * nE
        ol = own.tolist()
        cur[nE - 1] = ol[nE - 1]
        for v in range(nE - 2, -1, -1):  # parents avant enfants
            p = pl[v]
            lo[v] = cur[p]
            cur[p] += ml[v]
            cur[v] = lo[v] + ol[v]
            depth[v] = depth[p] + 1
        lo = np.asarray(lo, dtype=np.int64)
        hi = lo + mass
        depth = np.asarray(depth, dtype=np.int64)
        order = np.argsort(target, kind='stable')
        start = np.zeros(nE + 1, dtype=np.int64)
        start[1:] = np.cumsum(own)
        pos = np.empty(n, dtype=np.int64)
        pos[order] = lo[target[order]] + (np.arange(n) - start[target[order]])
        need(n == 0 or bool(np.array_equal(np.sort(pos), np.arange(n))), 'positions : une permutation')
        point_at = np.empty(n, dtype=np.int64)
        point_at[pos] = np.arange(n)
        gap = np.full(max(n - 1, 0), -1, dtype=np.int64)  # plus petit bloc contenant les positions j et j + 1
        if n > 1:
            ch = np.arange(nE - 1)
            ok = (lo[ch] > lo[parent[ch]]) & (mass[ch] > 0)
            gap[lo[ch][ok] - 1] = parent[ch][ok]
            inner = pos > lo[target]
            gap[pos[inner] - 1] = target[inner]
            need(bool((gap >= 0).all()), 'frontiere sans bloc')
        W = np.zeros(n + 1, dtype=np.int64)
        W[1:] = np.cumsum(weight[point_at])
        self.nE, self.n, self.parent, self.target, self.mass = nE, n, parent, target, mass
        self.lo, self.hi, self.depth, self.pos, self.point_at, self.W = lo, hi, depth, pos, point_at, W
        self.weight = weight
        self.size = W[hi] - W[lo]  # taille evaluee de chaque bloc
        self.gap = gap
        self._table = None

    def _sparse(self):
        """Table creuse : indice de la frontiere de profondeur minimale sur une plage de frontieres."""
        if self._table is None:
            d = self.depth[self.gap]
            tab = [np.arange(len(d), dtype=np.int64)]
            span = 1
            while 2 * span <= len(d):
                a, b = tab[-1][:-span], tab[-1][span:]
                tab.append(np.where(d[a] <= d[b], a, b))
                span *= 2
            self._table = (d, tab)
        return self._table

    def lca(self, p, q):
        """Plus petit bloc contenant les positions p < q (tableaux)."""
        d, tab = self._sparse()
        l, r = p, q - 1
        k = np.floor(np.log2((r - l + 1).astype(np.float64))).astype(np.int64)
        k = np.where((1 << (k + 1)) <= r - l + 1, k + 1, k)
        k = np.where((1 << k) > r - l + 1, k - 1, k)
        out = np.empty(len(p), dtype=np.int64)
        for kk in np.unique(k).tolist():
            sel = np.flatnonzero(k == kk)
            a = tab[kk][l[sel]]
            b = tab[kk][r[sel] - (1 << kk) + 1]
            out[sel] = np.where(d[a] <= d[b], a, b)
        return self.gap[out]

    def points_of(self, v):
        return self.point_at[self.lo[v]:self.hi[v]]


def cluster_thresholds(L):
    """tau[v] = min(masse(v), plus grande masse d'un frere) ; 0 pour la racine et pour un enfant unique."""
    nE = L.nE
    tau = np.zeros(nE, dtype=np.int64)
    if nE < 2:
        return tau
    ch = np.arange(nE - 1)
    par = L.parent[ch]
    m = L.mass[ch]
    order = np.lexsort((-m, par))
    ps, ms, cs = par[order], m[order], ch[order]
    first = np.ones(len(ps), dtype=bool)
    first[1:] = ps[1:] != ps[:-1]
    idx_first = np.flatnonzero(first)
    grp = np.cumsum(first) - 1
    top1 = ms[idx_first]
    second = np.zeros(len(idx_first), dtype=np.int64)
    has2 = np.zeros(len(idx_first), dtype=bool)
    nxt = idx_first + 1
    ok = nxt < len(ps)
    ok[ok] = ~first[nxt[ok]]
    has2[ok] = True
    second[ok] = ms[nxt[ok]]
    other = np.where(first, second[grp], top1[grp])
    tau[cs] = np.minimum(ms, other)
    return tau


def target_forest(L, tau, s):
    """Clusters a la taille s (tau >= s), dans l'ordre des evenements decroissants (parents avant enfants).
    Rend (evenements, parent dans la famille (-1 : aucun), classe par point (-1 : aucune))."""
    nE = L.nE
    is_t = tau >= s
    is_t[nE - 1] = False
    ev = np.flatnonzero(is_t)[::-1].copy()
    index = np.full(nE, -1, dtype=np.int64)
    index[ev] = np.arange(len(ev))
    anc = [-1] * nE  # cluster le plus profond contenant l'evenement (lui compris)
    pl = L.parent.tolist()
    il = index.tolist()
    for v in range(nE - 2, -1, -1):
        anc[v] = il[v] if il[v] >= 0 else anc[pl[v]]
    anc = np.asarray(anc, dtype=np.int64)
    tparent = np.where(L.parent[ev] >= 0, anc[np.maximum(L.parent[ev], 0)], -1)
    need(len(ev) == 0 or bool((tparent < np.arange(len(ev))).all()), 'famille : parent avant enfant')
    return ev, tparent, anc[L.target]


def best_blocks_for_targets(Lb, Lt, events, chunk=2000000):
    """Pour chaque cible (evenement de Lt), meilleur bloc de Lb. Rend (m, inter, bloc, evenement) en int64 ; les
    cibles sans point evalue ont m = 0 et inter = 0. Les deux dispositions portent les memes points et poids."""
    need(Lb.n == Lt.n and bool(np.array_equal(Lb.weight, Lt.weight)), 'memes points et memes poids')
    T = len(events)
    m = np.zeros(T, dtype=np.int64)
    inter = np.zeros(T, dtype=np.int64)
    block = np.zeros(T, dtype=np.int64)
    bev = np.full(T, -1, dtype=np.int64)
    n = Lb.n
    sizes = (Lt.hi[events] - Lt.lo[events]).tolist()
    i = 0
    while i < T:
        j, tot = i, 0
        while j < T and (j == i or tot + sizes[j] <= chunk):
            tot += sizes[j]
            j += 1
        tid = np.repeat(np.arange(i, j), sizes[i:j])
        pts = np.concatenate([Lt.points_of(int(v)) for v in events[i:j]]) if tot else np.zeros(0, dtype=np.int64)
        keep = Lb.weight[pts] == 1
        tid, pts = tid[keep], pts[keep]
        if len(pts):
            pb = Lb.pos[pts]
            o = np.lexsort((pb, tid))
            tid, pb, pts = tid[o], pb[o], pts[o]
            key = tid * n + pb
            cnt_t = np.bincount(tid - i, minlength=j - i)
            m[i:j] = cnt_t
            same = tid[1:] == tid[:-1]
            ca_t, ca_e = tid, Lb.target[pts]
            if same.any():
                l, r = pb[:-1][same], pb[1:][same]
                cb_e = Lb.lca(l, r)
                ca_t = np.concatenate([ca_t, tid[:-1][same]])
                ca_e = np.concatenate([ca_e, cb_e])
            c = np.searchsorted(key, ca_t * n + Lb.hi[ca_e]) - np.searchsorted(key, ca_t * n + Lb.lo[ca_e])
            sz = Lb.size[ca_e]
            mm = m[ca_t]
            need(bool((c >= 1).all()) and bool((c <= sz).all()) and bool((c <= mm).all()), 'intersection incoherente')
            iou = c / (sz + mm - c)
            o = np.lexsort((ca_e, sz, -iou, ca_t))
            ft = ca_t[o]
            first = np.ones(len(o), dtype=bool)
            first[1:] = ft[1:] != ft[:-1]
            sel = o[first]
            inter[ca_t[sel]] = c[sel]
            block[ca_t[sel]] = sz[sel]
            bev[ca_t[sel]] = ca_e[sel]
        i = j
    return m, inter, block, bev


def brute_best_blocks(Eb, Et, events, weight):
    """Reference par force brute (tests, petits arbres) : ensembles de points explicites."""
    def sets(E):
        nE = len(E['rank'])
        s = [set() for _ in range(nE)]
        for x, v in enumerate(E['target']):
            s[v].add(x)
        for v in range(nE - 1):
            s[E['parent'][v]] |= s[v]
        return s
    sb, st = sets(Eb), sets(Et)
    ev_ok = {x for x in range(len(weight)) if weight[x] == 1}
    out = []
    for v in events:
        t = st[int(v)] & ev_ok
        best = (0, 1, 0, 0)
        for b in sb:
            be = b & ev_ok
            c = len(be & t)
            if c == 0:
                continue
            den = len(be) + len(t) - c
            if c * best[1] > best[0] * den:
                best = (c, den, len(be), len(t))
        out.append((len(t), best[0], best[2]))
    return out


BINS = 100


def summarize(m, inter, block):
    """Distribution des IoU d'une famille de cibles (m > 0) : comptes et sommes, simples et ponderes par la taille,
    et histogrammes a 100 classes (classe i : [i/100, (i+1)/100[ ; la classe 100 est l'IoU exactement 1)."""
    m = np.asarray(m, dtype=np.int64)
    inter = np.asarray(inter, dtype=np.int64)
    block = np.asarray(block, dtype=np.int64)
    keep = m > 0
    m, inter, block = m[keep], inter[keep], block[keep]
    den = block + m - inter
    iou = np.where(inter > 0, inter / np.maximum(den, 1), 0.0)
    prec = np.where(inter > 0, inter / np.maximum(block, 1), 0.0)
    rec = inter / np.maximum(m, 1)
    b = np.minimum((iou * BINS).astype(np.int64), BINS - 1)
    b = np.where(inter == den, BINS, b)  # exactement 1
    hist = np.bincount(b, minlength=BINS + 1)
    histw = np.bincount(b, weights=m.astype(np.float64), minlength=BINS + 1).astype(np.int64)
    ge90 = 10 * inter >= 9 * den
    ge99 = 100 * inter >= 99 * den
    ge50 = 2 * inter > den
    return dict(targets=int(len(m)), points=int(m.sum()), sum_iou=float(iou.sum()),
                sum_iou_w=float((iou * m).sum()), sum_precision=float(prec.sum()),
                sum_precision_w=float((prec * m).sum()), sum_recall=float(rec.sum()),
                sum_recall_w=float((rec * m).sum()), n_gt50=int(ge50.sum()), w_gt50=int(m[ge50].sum()),
                n_ge90=int(ge90.sum()), w_ge90=int(m[ge90].sum()), n_ge99=int(ge99.sum()), w_ge99=int(m[ge99].sum()),
                n_eq1=int((inter == den).sum()), w_eq1=int(m[inter == den].sum()),
                hist=';'.join(map(str, hist.tolist())), hist_w=';'.join(map(str, histw.tolist())))


def residuals(L, tau, s, size, cnt, m):
    """Formes residuelles a la taille s. size[v], cnt[v, g] : comptes par sous-arbre (tvp_core.subtree_counts) ;
    m[g] : taille des groupes. Rend un dict par groupe (ordre des colonnes) : meilleur BLOC (cluster condense, racine
    exclue) et meilleur RESIDU own_s(c) (racine comprise), en entiers exacts (intersection, taille)."""
    nE = L.nE
    is_c = tau >= s
    is_c[nE - 1] = True
    anc = [nE - 1] * nE
    pl = L.parent.tolist()
    cl = is_c.tolist()
    for v in range(nE - 2, -1, -1):
        anc[v] = v if cl[v] else anc[pl[v]]
    anc = np.asarray(anc, dtype=np.int64)
    cl_ev = np.flatnonzero(is_c)
    inner = cl_ev[cl_ev != nE - 1]
    child_size = np.zeros(nE, dtype=np.int64)
    child_cnt = np.zeros(cnt.shape, dtype=np.int64)
    if len(inner):
        pa = anc[L.parent[inner]]
        np.add.at(child_size, pa, size[inner])
        np.add.at(child_cnt, pa, cnt[inner])
    own_size = size[cl_ev] - child_size[cl_ev]
    own_cnt = cnt[cl_ev] - child_cnt[cl_ev]
    need(bool((own_size >= 0).all()) and bool((own_cnt >= 0).all()), 'residu negatif')
    need(int(own_size.sum()) == int(size[nE - 1]), 'les residus ne partitionnent pas les points evalues')
    out = []
    for g in range(cnt.shape[1]):
        rec = dict(m=int(m[g]), clusters=int(len(inner)))
        for name, ev, sz, cc in (('blk', inner, size[inner], cnt[inner][:, g] if len(inner) else np.zeros(0, int)),
                                 ('res', cl_ev, own_size, own_cnt[:, g])):
            if len(ev) == 0 or int(cc.max()) == 0:
                rec.update({name + '_inter': 0, name + '_size': 0, name + '_event': -1})
                continue
            den = sz + m[g] - cc
            iou = cc / den
            o = np.lexsort((ev, sz, -iou))
            b = int(o[0])
            rec.update({name + '_inter': int(cc[b]), name + '_size': int(sz[b]), name + '_event': int(ev[b])})
        rec['res_is_root'] = int(rec['res_event'] == nE - 1)
        out.append(rec)
    return out


def write_targets_file(path, tparent, cls):
    """Fichier des cibles de tour_plafond_ab : T, parents (0xFFFFFFFF : racine), classe par site (ordre d'entree)."""
    T = len(tparent)
    a = np.empty(1 + T + len(cls), dtype='<u4')
    a[0] = T
    a[1:1 + T] = np.where(tparent < 0, 0xFFFFFFFF, tparent).astype(np.uint32)
    a[1 + T:] = np.where(cls < 0, 0xFFFFFFFF, cls).astype(np.uint32)
    a.tofile(path)


def read_targets_out(path, T):
    """Sortie de tour_plafond_ab : (m, inter, evald, niveau approche, noeud) par cible."""
    raw = np.loadtxt(path, dtype=np.float64, ndmin=2) if T else np.zeros((0, 6))
    need(raw.shape == (T, 6) and bool(np.array_equal(raw[:, 0], np.arange(T))), 'sortie des cibles : T lignes')
    m, inter, evald = (raw[:, c].astype(np.int64) for c in (1, 2, 3))
    return m, inter, evald, raw[:, 4], raw[:, 5].astype(np.int64)
