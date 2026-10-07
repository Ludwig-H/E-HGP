"""Discontinuite de cover (auditeur continu, 2.1) et borne 2 eps de core : oracle independant + binaire v10."""
import math
import natif as N
import oracle_l03 as O
for P in ([(0, 0, 0), (999, 0, 0), (2000, 0, 0)], [(0, 0, 0), (1001, 0, 0), (2000, 0, 0)]):
    T = O.Tour(P, 2)
    core = O.core_projection(T)
    cov = O.cover_projection(T)
    uc = T.hauteurs(core)
    uv = T.hauteurs(cov)
    print('nuage', [p[0] for p in P])
    print('  oracle : u_core(gauche, milieu) = %.3f ; u_cover(gauche, milieu) = %.3f ; u_cover(milieu, droite) = %.3f' % (
        O.r(uc[(0, 1)]), O.r(uv[(0, 1)]), O.r(uv[(1, 2)])))
    for entry in ('core', 'cover'):
        res = N.run_cluster(P, 2, entry, mcs=2)
        s = N.suite(res['tree'], 2)
        print('  binaire --entry=%s :' % entry, [(round(math.sqrt(a), 3), p) for a, p in s])
