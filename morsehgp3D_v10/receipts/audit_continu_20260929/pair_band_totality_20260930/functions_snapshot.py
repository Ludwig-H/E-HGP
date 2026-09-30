def poids_bande(beta, A, eta):
    """Poids de bande souple en niveaux : max(0, 1 + 1/eta - beta / (eta A)) ; eta > 0 rationnel ; A = alpha^2."""
    eta = Fraction(eta)
    w = 1 + 1 / eta - Fraction(beta) / (eta * Fraction(A))
    return w if w > 0 else Fraction(0)

def votes_paires(gf, x, eta):
    """Votes de PAIRES d'ordre K : pour chaque site y != x, milieu m = (x + y)/2, niveau d'entree
    l^2 = max(|xy|^2 / 4, D_K(m)^2) (D_K(m) : distance de m a son K-ieme plus proche site), composante = celle de
    L_K qui contient m (celle du sommet des K plus proches sites de m : m est dans le convexe
    {p : F dans B(p, s)}, inclus dans L_K(s)). Echelle A = min_y l^2 (plus petit niveau de vote, >= alpha^2) ;
    poids de bande souple relatifs a A. A K = 2, l = |xy|/2 et les votes sont ceux de Gamma_2 : MMp = MMg."""
    P, K = gf.P, gf.K
    brut = []
    for y in range(gf.n):
        if y == x:
            continue
        m = tuple(Fraction(a + b, 2) for a, b in zip(P[x], P[y]))
        half2 = Fraction(sum((a - b) ** 2 for a, b in zip(P[x], P[y])), 4)
        dist = sorted((sum((mi - pi) ** 2 for mi, pi in zip(m, P[p])), p) for p in range(gf.n))
        DK2 = dist[K - 1][0]
        F = tuple(sorted(p for _d, p in dist[:K]))
        exiger(gf.beta[F] <= max(half2, DK2), 'paires : sommet K-PPV du milieu non ne a son niveau')
        brut.append((max(half2, DK2), gf.born[F], y))
    A = min(l2 for l2, _v, _y in brut)
    exiger(A >= gf.alpha2(x), 'paires : niveau de vote sous la premiere couverture')
    out = []
    for l2, v, y in sorted(brut, key=lambda t: (t[0], t[2])):
        w = poids_bande(l2, A, eta)
        if w > 0:
            out.append((l2, v, w, y))
    return A, out
