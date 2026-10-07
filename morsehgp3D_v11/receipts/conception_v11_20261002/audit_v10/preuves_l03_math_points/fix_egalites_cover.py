"""Departage des egalites exactes de premiere couverture dans le binaire v10 (entree cover) : non-equivariance."""
import math
import natif as N
import oracle_l03 as O

def show(nom, P, K, names):
    res = N.run_cluster(P, K, 'cover', mcs=2)
    print(nom, ': code', res['code'])
    for a, p in N.suite(res['tree'], 2):
        print('    r = %10.4f : %s' % (math.sqrt(a), ' | '.join('{' + ','.join(names[i] for i in b) + '}' for b in p) or '(aucun)'))
    T = O.Tour(P, K)
    for x in range(T.n):
        a, comps = T.cover_tie_components(x)
        if len(comps) > 1:
            print('    oracle : %s a %d composantes couvrantes distinctes a alpha = %.4f (egalite exacte)' % (names[x], len(comps), O.r(a)))

# 1. trois points alignes equidistants : le milieu est couvert au meme niveau par deux composantes
show('{0,2,4} K=2', [(0, 0, 0), (2, 0, 0), (4, 0, 0)], 2, ['p0', 'p2', 'p4'])
# la meme figure, translatee et tournee de 90 degres (axe y) : meme objet a isometrie pres
show('{0,2,4} sur l axe y', [(7, 0, 0), (7, 2, 0), (7, 4, 0)], 2, ['q0', 'q2', 'q4'])
# 2. carre : chaque sommet est couvert au meme niveau par deux aretes
show('carre K=2', [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)], 2, ['P', 'Q', 'R', 'S'])
# 3. fixture Q4 de l'utilisateur (centralement symetrique par rapport a m)
show('Q4 K=3', [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600), (2200, 2200, 2200),
                (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)], 3, ['C', 'P', 'Q', 'R', 'm', 'D', 'P2', 'Q2', 'R2'])
# 4. Q4 image par la symetrie centrale p -> 5200 - p (meme nuage, sites renommes) : on rend les noms images
show('Q4 K=3, image par symetrie centrale (noms transportes)', [tuple(5200 - c for c in p) for p in
     [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600), (2200, 2200, 2200),
      (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)]], 3, ['C', 'P', 'Q', 'R', 'm', 'D', 'P2', 'Q2', 'R2'])
