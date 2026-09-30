#!/usr/bin/env python3
"""MMt_kappa : majorite a marge continue sur le TEMPS DE COUVERTURE des branches (regle intrinseque, TI).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=revision_cible_majorites_continues, public_status=not_claimed. GCP non utilise. Aucun moteur modifie.
Bibliotheque partagee (fixtures_cibles/lib) et worktree partage lus seulement.

Donnees (FULL_K natif et relation de couverture seulement) : pour le point x, V_x = noeuds qui couvrent x pendant
leur vie, c_x(v) = premier niveau ou v couvre x, d_v = niveau de mort (naissance du parent ; infini a la racine),
A = alpha(x)^2 = min c_x(v). Bande de niveaux [A, E2], E2 = (1 + eta) A.

  masse d'une composante C vivante au niveau s : m(C, s) = somme, sur les v de V_x descendants de C (C compris),
      de la longueur de [c_x(v), min(d_v, s, E2)] (niveaux ; zero si vide) ;
  W = masse totale (tous les v, s infini) ; G(s) = max_C m(C, s) ; mu(s) = 2 G(s) / W - 1 ;
  T(theta) = min{s : G(s) >= theta W} ; T_1/2 = inf{s : G(s) > W/2} ; O = composante majoritaire juste apres ;
  date t = sup_{theta in ]1/2, 1]} ( sqrt(T(theta)) - kappa a (2 theta - 1) ), a = alpha(x) (rayon) ;
  proprietaire o = ancetre de O vivant a t.

Forme finie (prouvee au memo) : t = max( sqrt(T_1/2), sqrt(e) - kappa a mu(e-) aux niveaux d'evenement e > T_1/2
avec G(e-) < W, et D(s*) aux points critiques s* = (W / (4 kappa a))^2 interieurs a un segment de croissance ).

Entre deux evenements, une composante vivante est un seul noeud : sa masse croit a la pente 1 (en niveau) si elle
couvre x et si s < E2, sinon elle est constante ; les fusions ajoutent les masses (sauts de G). Tout est exact :
niveaux, masses, W et marges rationnels ; dates en sommes de racines (QS) ; aucun assert (identique sous -O).
"""
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
if ICI not in sys.path:
    sys.path.insert(0, ICI)
import mmc  # noqa: E402  (fixe le cache de la bibliotheque sous /tmp/mhgp10-revision-cible/majorites_continues)

RG = mmc.RG
QS = mmc.QS
Rayon = mmc.Rayon
NONE = mmc.NONE
exiger = mmc.exiger
qmax = mmc.qmax
qmin = mmc.qmin


def _mul(a, b):
    """Produit exact de deux QS (ou rationnels)."""
    return RG._qs_mul(a, b)


# ------------------------------------------------------------------ structure couvrante intrinseque

def structure(sc):
    """(T, cover_par_point) : T arbre des masses (memes indices que sc.foret) ; cover_par_point[p] = {v: c_x(v)}."""
    T, cover, _info = sc.arbre_masses()
    par_point = [None] * sc.n
    for s, cv in enumerate(cover):
        par_point[sc.ctx.point_id[s]] = dict(cv)
    for p in range(sc.n):
        exiger(par_point[p], 'structure couvrante vide (point %s)' % sc.noms[p])
    return T, par_point


def signature_ti(sc):
    """Forme canonique recursive de (FULL_K, relation de couverture) : (naissance, [(point, c_p(v))], enfants tries).
    Deux scenes de meme signature ont des arbres isomorphes a niveaux egaux et les memes couvertures : toute regle
    intrinseque (TI) y donne les memes dates et les memes partitions."""
    T, cov = structure(sc)
    par_noeud = {}
    for p in range(sc.n):
        for v, c in cov[p].items():
            par_noeud.setdefault(v, []).append((sc.noms[p], str(c)))

    def rec(v):
        return (str(T.birth[v]), tuple(sorted(par_noeud.get(v, []))), tuple(sorted(rec(c) for c in T.children[v])))
    return rec(T.root)


# ------------------------------------------------------------------ noyau exact (un point)

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


# ------------------------------------------------------------------ regle complete

class RegleT:
    def __init__(self, nom, h, details, params):
        self.nom, self.h, self.details, self.params = nom, h, details, params
        self.univers = 'couverture'


def mm_temps(sc, kappa, eta, nom=None, marge=True):
    """MMt_kappa sur la structure couvrante native (TI) ; hierarchie sur la foret native."""
    if nom is None:
        nom = ('MMt[k=%s,e=%s]' % (Fraction(kappa), Fraction(eta)) if marge
               else 'MMt_evt[e=%s]' % Fraction(eta))
    T, cov = structure(sc)
    res = []
    for p in range(sc.n):
        r = mmt_point(T, cov[p], eta, kappa, marge)
        r['dK2'] = Fraction(sc.dK2(p))
        r['cv'] = cov[p]
        res.append(r)
    dates = [Rayon.somme(r['date']) for r in res]
    owners = [r['owner'] for r in res]
    h = RG.Hierarchie(sc, nom, 'majorite_continue_TI', sc.foret, dates, owners)
    return RegleT(nom, h, res, {'kappa': Fraction(kappa), 'eta': Fraction(eta), 'marge': marge})


def controler_mmt(sc, regle):
    """Invariants exacts : Hierarchie.valider ; NP ancree (le proprietaire couvre x a sa date : il a un descendant
    ou lui-meme dans V_x, de premiere couverture <= date) ; chaine alpha <= T_1/2 <= t <= T1 <= sqrt(E2) + d_K/2."""
    n_ctrl = regle.h.valider()
    T, cov = structure(sc)
    for p in range(sc.n):
        d = regle.details[p]
        t = regle.h.dates[p]
        o = regle.h.proprietaires[p]
        ok = False
        for v, c in cov[p].items():
            if Rayon(b2=c).cmp(t) <= 0 and T.birth[v] <= T.birth[o]:
                u = v
                while u != o and T.parent[u] >= 0 and T.birth[T.parent[u]] <= T.birth[o]:
                    u = T.parent[u]
                if u == o:
                    ok = True
                    break
        exiger(ok, '%s : proprietaire du point %s ne le couvre pas a sa date' % (regle.nom, sc.noms[p]))
        a = QS.sqrt(d['A'])
        exiger(a.cmp(QS.sqrt(d['T_half'])) <= 0, '%s : T_1/2 < alpha' % regle.nom)
        exiger(QS.sqrt(d['T_half']).cmp(d['date']) <= 0, '%s : date < T_1/2' % regle.nom)
        exiger(d['date'].cmp(QS.sqrt(d['T1'])) <= 0, '%s : date > T1' % regle.nom)
        borne = QS.sqrt(d['E2']) + QS.sqrt(d['dK2']) * Fraction(1, 2)
        exiger(QS.sqrt(d['T1']).cmp(borne) <= 0, '%s : T1 > sqrt(E2) + d_K/2 (point %s)' % (regle.nom, sc.noms[p]))
        n_ctrl += 5
    return n_ctrl


# ------------------------------------------------------------------ borne prouvee (theoreme S_t)

def _compteurs(T, cv, A, E, eps2):
    """(M, n_max) du lemme de transfert : M = somme, sur les fusions f de rayon dans ]alpha, E + 2 eps], de
    (nombre d'enfants couvrants de premiere couverture < b_f) - 1 ; n_max = nombre maximal de composantes couvrantes
    simultanees sur la bande [A, E^2[ (niveaux). E et eps2 = 2 eps sont des QS (rayons)."""
    lim = E + eps2
    M = 0
    for f in set(cv):
        kids = [u for u in T.children[f] if u in cv and cv[u] < T.birth[f]]
        if len(kids) >= 2:
            rf = QS.sqrt(T.birth[f])
            if rf.cmp(QS.sqrt(A)) > 0 and rf.cmp(lim) <= 0:
                M += len(kids) - 1
    niveaux = sorted({c for c in cv.values()} | {T.birth[v] for v in cv} | {A})
    nmax = 0
    E2 = _mul(E, E).as_rational()
    for s in niveaux:
        if E2 is not None and s >= E2:
            continue
        k = sum(1 for v, c in cv.items() if c <= s and T.birth[v] <= s and (T.death[v] is None or T.death[v] > s))
        nmax = max(nmax, k)
    return M, nmax


def borne_t(dX, dY, eps, kappa, eta, TX, TY):
    """Borne prouvee pour un point (theoreme S_t du memo) : |dt| <= (1 + kappa) eps + 2 kappa a_max nu,
    nu = (Delta_XY + Delta_YX) / W_min, Delta_XY = 2 eps E^X (2 M^X + (lambda + 1) n_max^X), lambda = sqrt(1 + eta),
    E = lambda alpha. Terme des proprietaires : tau = min(max(2 eps, 3 kappa a_max Delta / W_min), d_K/2) si
    3 Delta < W_min (Delta = max des deux transferts), tau = d_K/2 sinon. Rend (borne QS, tau QS, details)."""
    kappa, eta = Fraction(kappa), Fraction(eta)
    lam = QS.sqrt(1 + eta)
    out = {}
    D = {}
    for nom, d, T in (('X', dX, TX), ('Y', dY, TY)):
        E = QS.sqrt((1 + eta) * d['A'])
        M, nmax = _compteurs(T, d['cv'], d['A'], E, eps * 2)
        # Delta = 2 eps E (2 M + (lambda + 1) n_max)
        D[nom] = _mul(_mul(eps * 2, E), QS.rat(2 * M) + (lam + 1) * nmax)
        out[nom] = {'M': M, 'n_max': nmax}
    Wmin = min(dX['W'], dY['W'])
    amax = qmax(dX['alpha'], dY['alpha'])
    nu_num = D['X'] + D['Y']
    bt = eps * (1 + kappa) + _mul(amax, nu_num) * (2 * kappa / Wmin)
    Dm = qmax(D['X'], D['Y'])
    demi_dK = qmax(QS.sqrt(dX['dK2']), QS.sqrt(dY['dK2'])) * Fraction(1, 2)
    if (Dm * 3).cmp(QS.rat(Wmin)) < 0:
        tau = qmin(qmax(eps * 2, _mul(amax, Dm) * (3 * kappa / Wmin)), demi_dK)
    else:
        tau = demi_dK
    out['Delta_XY'] = float(D['X'])
    out['Delta_YX'] = float(D['Y'])
    return bt, tau, out


def verifier_borne_t(r1, r2, eps, sc1, sc2):
    """Controle exact du theoreme S_t (dates et hauteurs) sur deux nuages apparies, memes noms de points."""
    kappa, eta = r1.params['kappa'], r1.params['eta']
    T1, _c1 = structure(sc1)
    T2, _c2 = structure(sc2)
    n = sc1.n
    dt = {}
    bt, tau = {}, {}
    ok = True
    n_ctrl = 0
    for p in range(n):
        q = sc2.index[sc1.noms[p]]
        dt[p] = mmc.qabs(mmc.rayon_qs(r1.h.dates[p]) - mmc.rayon_qs(r2.h.dates[q]))
        b, ta, _o = borne_t(r1.details[p], r2.details[q], eps, kappa, eta, T1, T2)
        bt[p], tau[p] = b, ta
        if dt[p].cmp(b) > 0:
            ok = False
        n_ctrl += 1
    rmax_u = 0.0
    for p in range(n):
        for p2 in range(p + 1, n):
            q, q2 = sc2.index[sc1.noms[p]], sc2.index[sc1.noms[p2]]
            du = mmc.qabs(mmc.rayon_qs(r1.h.hauteur(p, p2)) - mmc.rayon_qs(r2.h.hauteur(q, q2)))
            b = qmax(bt[p], bt[p2]) + qmax(tau[p], tau[p2]) + eps
            if du.cmp(b) > 0:
                ok = False
            n_ctrl += 1
            rmax_u = max(rmax_u, float(du))
    e = float(eps)
    return ok, {'max_dt_sur_eps': max(float(v) for v in dt.values()) / e if e else 0.0,
                'max_du_sur_eps': rmax_u / e if e else 0.0,
                'borne_dates_max_sur_eps': max(float(v) for v in bt.values()) / e if e else 0.0}, n_ctrl


# ------------------------------------------------------------------ verification independante de la forme finie

def _masse_directe(T, cv, E2, C, s):
    """Masse de la composante C (vivante a s) par la definition : somme sur les v de V_x descendants de C (C compris),
    nes avant s, de |[c_x(v), min(d_v, s, E2)]|. Calcul independant de mmt_point (remontee explicite des parents)."""
    tot = Fraction(0)
    for v, c in cv.items():
        if T.birth[v] > s or c >= E2:
            continue
        u = v
        while u != C and T.parent[u] >= 0:
            u = T.parent[u]
        if u != C:
            continue
        d = T.death[v]
        hi = s if d is None else min(d, s)
        hi = min(hi, E2)
        if hi > c:
            tot += hi - c
    return tot


def _vivants(T, s):
    return [v for v in range(len(T.birth)) if T.birth[v] <= s and (T.death[v] is None or T.death[v] > s)]


def verifier_forme(T, cv, eta, kappa, r):
    """Controle independant de la date de MMt par echantillonnage exact :
    (i) pas de majorite stricte avant T_1/2, majorite stricte apres, tenue par la lignee de O ;
    (ii) propriete de sup : D(s) = sqrt(s) - kappa a (2 G(s)/W - 1) <= t en chaque niveau echantillonne > T_1/2
         (evenements, limites a gauche, milieux, tiers, point critique) ;
    (iii) atteinte : t vaut sqrt(T_1/2), ou D(e-) a un evenement e, ou D(s*) au point critique.
    Rend le nombre de controles ; leve MMErreur sinon."""
    kappa = Fraction(kappa)
    A, E2, W = r['A'], r['E2'], r['W']
    a = QS.sqrt(A)
    t = r['date']
    Th = r['T_half']
    evs = sorted({e for e, _G in r['trace']})
    pts = set(evs)
    gauches = []
    for e0, e1 in zip(evs, evs[1:]):
        pts.add(e0 + (e1 - e0) / 2)
        pts.add(e0 + (e1 - e0) / 3)
        gauches.append((e1, e0 + (e1 - e0) * Fraction(999, 1000)))
    s_star = W * W / (16 * kappa * kappa * A)
    pts.add(s_star)
    pts.add(Th)
    n = 0

    def G_de(s):
        best, arg = Fraction(-1), None
        for C in _vivants(T, s):
            m = _masse_directe(T, cv, E2, C, s)
            if m > best:
                best, arg = m, C
        return best, arg

    def D(s, G):
        return QS.sqrt(s) - a * (kappa * (2 * G / W - 1))

    for s in sorted(pts):
        G, C = G_de(s)
        if s < Th:
            exiger(2 * G <= W, 'forme : majorite stricte avant T_1/2')
        elif s > Th:
            exiger(2 * G > W, 'forme : pas de majorite apres T_1/2')
            exiger(_anc(T, r['O'], s) == C, 'forme : la tete n est pas la lignee de O')
            if G < W:
                exiger(D(s, G).cmp(t) <= 0, 'forme : sup viole en s = %s' % s)
        n += 1
    atteint = QS.sqrt(Th).cmp(t) == 0
    for e, s_g in gauches:
        if s_g <= Th:
            continue
        # limite a gauche de G en e : masse de la lignee de O juste avant e, prolongee jusqu'a e (continuite)
        C = _anc(T, r['O'], s_g)
        Ge = _masse_directe(T, cv, E2, C, e) if (T.death[C] is None or T.death[C] >= e) else None
        if Ge is None:
            continue
        exiger(2 * Ge > W, 'forme : limite a gauche sans majorite')
        if Ge < W:
            v = D(e, Ge)
            exiger(v.cmp(t) <= 0, 'forme : sup viole a la limite gauche de %s' % e)
            if v.cmp(t) == 0:
                atteint = True
        n += 1
    if s_star > Th:
        G, _C = G_de(s_star)
        if G < W and D(s_star, G).cmp(t) == 0:
            atteint = True
    exiger(atteint, 'forme : date non atteinte par un terme')
    return n + 1
