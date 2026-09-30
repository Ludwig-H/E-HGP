def _anc(T, v, s):
    """Ancetre de v vivant au niveau s (coupe fermee) ; v doit etre ne (birth <= s)."""
    exiger(T.birth[v] <= s, 'noeud non ne')
    while T.parent[v] >= 0 and T.birth[T.parent[v]] <= s:
        v = T.parent[v]
    return v

def mmt_point(T, cv, eta, kappa, marge=True):
    """Date (QS, rayon), proprietaire (noeud) et details de MMt pour un point de structure couvrante cv.
    marge=False : mutant DATE PAR EVENEMENT (t = sqrt(T_1/2), sans cone) ; il sert a montrer la necessite du cone."""
    eta, kappa = Fraction(eta), Fraction(kappa)
    exiger(eta > 0 and kappa > 0, 'parametres non positifs')
    A = min(cv.values())
    E2 = (1 + eta) * A
    massifs = [v for v, c in cv.items() if c < E2]

    def fin(v):
        d = T.death[v]
        return E2 if d is None or d > E2 else d

    W = sum((fin(v) - cv[v] for v in massifs if fin(v) > cv[v]), Fraction(0))
    exiger(W > 0, 'masse totale nulle')
    evs = {A, E2}
    for v in massifs:
        evs.add(cv[v])
        u = v
        while T.parent[u] >= 0:
            u = T.parent[u]
            evs.add(T.birth[u])
    evs = sorted(evs)

    def masses(s):
        """{composante vivante a s : (masse a s, pente sur [s, evenement suivant))}."""
        out = {}
        for v in massifs:
            if T.birth[v] > s:
                continue
            C = _anc(T, v, s)
            m = min(fin(v), s) - cv[v]
            m = m if m > 0 else Fraction(0)
            old = out.get(C, (Fraction(0), 0))
            out[C] = (old[0] + m, old[1])
        for C in list(out):
            pente = 1 if (C in cv and cv[C] <= s and s < E2) else 0
            out[C] = (out[C][0], pente)
        return out

    demi = W / 2
    T_half = O = None
    termes = []
    crit = []
    T1 = None
    trace = []
    prec = None           # (niveau, masses) de l'evenement precedent
    for i, e in enumerate(evs):
        # segment [prec, e) : franchissement continu de W/2 par la composante de tete
        if prec is not None:
            s0, ms = prec
            gauche = {C: m + p * (e - s0) for C, (m, p) in ms.items()}
            if T_half is None:
                for C, (m, p) in ms.items():
                    if p == 1 and m <= demi < m + (e - s0):
                        exiger(T_half is None, 'franchissement non exclusif')
                        T_half, O = s0 + (demi - m), C
                if T_half is not None:
                    Gm = gauche[O]
                    if Gm < W:
                        termes.append((e, 2 * Gm / W - 1))
                    # point critique interieur a ]T_half, e[
                    crit.append((T_half, e, O, s0, ms[O][0]))
            else:
                OC = _anc(T, O, s0)
                Gm = gauche[OC]
                if Gm < W:
                    termes.append((e, 2 * Gm / W - 1))
                if ms[OC][1] == 1:
                    crit.append((s0, e, OC, s0, ms[OC][0]))
        ms = masses(e)
        G = max(m for m, _p in ms.values()) if ms else Fraction(0)
        trace.append((e, G))
        if T_half is None and 2 * G > W:
            gagnants = [C for C, (m, _p) in ms.items() if 2 * m > W]
            exiger(len(gagnants) == 1, 'majorite non exclusive')
            T_half, O = e, gagnants[0]
        if T1 is None and G == W:
            T1 = e
        prec = (e, ms)
    exiger(T_half is not None and T1 is not None, 'majorite ou unanimite jamais atteinte')
    a = QS.sqrt(A)
    date = QS.sqrt(T_half)
    arg = ('T_half', T_half)
    for e, mu in (termes if marge else []):
        if e <= T_half:
            continue
        exiger(mu > 0, 'marge non positive apres T_1/2')
        val = QS.sqrt(e) - a * (kappa * mu)
        if val.cmp(date) > 0:
            date, arg = val, ('saut', e)
    # points critiques : D(s) = sqrt(s) - kappa a (2 G(s)/W - 1) concave sur un segment de pente 1 ;
    # maximum interieur en sqrt(s*) = W / (4 kappa a), soit s* = W^2 / (16 kappa^2 A)
    s_star = W * W / (16 * kappa * kappa * A)
    for lo, hi, _C, s0, m0 in (crit if marge else []):
        if lo < s_star < hi:
            G_star = m0 + (s_star - s0)
            val = QS.sqrt(s_star) - a * (kappa * (2 * G_star / W - 1))
            if val.cmp(date) > 0:
                date, arg = val, ('critique', s_star)
    rd = Rayon.somme(date)
    o = _anc(T, O, T_half)
    # proprietaire : ancetre vivant a la date (rayon exact)
    while T.parent[o] >= 0 and Rayon(b2=T.birth[T.parent[o]]).cmp(rd) <= 0:
        o = T.parent[o]
    return {'date': date, 'owner': o, 'T_half': T_half, 'O': O, 'W': W, 'A': A, 'E2': E2, 'T1': T1,
            'termes': termes, 'argmax': arg, 'N': len(massifs), 'trace': trace, 'alpha': a}
