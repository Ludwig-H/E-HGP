"""Fixtures de CLUSTER_v2 12.2 rejouees par l'oracle independant (elles ne sont gravees dans aucun test du depot)."""
import oracle_l03 as O
from fractions import Fraction as Fr

def blocs(T, a, entry, mcs=1):
    return [b for b in T.partition(a, entry) if len(b) >= mcs]

# cx_firstcov_k3_n6 (OP12 : premiere couverture puis ancetre vivant != C n X)
P = [(2, 4, 4), (2, 8, 5), (2, 9, 1), (3, 7, 0), (6, 9, 6), (8, 10, 7)]
T = O.Tour(P, 3)
core = O.core_projection(T)
print('cx_firstcov_k3_n6 : D_3(x4) =', core[4][0], ' N_3(x4) =', core[4][1], ' beta(N_3(x4)) =', T.V[core[4][1]])
a4, F4 = T.cover_entries(4)
print('  premiere couverture de x4 : alpha^2 =', a4, ' K-parties :', F4)
print('  coupe a = 18, C n X (core), blocs de points entres :', blocs(T, Fr(18), core))
cs = T.comps(Fr(18))
print('  a = 18 : composante de N_3(x4) =', sorted(cs[core[4][1]]), ' ; composante de la premiere couverture =', sorted(cs[F4[0]]))
print('  memes composantes ?', cs[core[4][1]] == cs[F4[0]])
u = T.hauteurs(core)
print('  u_core(1,2) =', u.get((1, 2)), ' u_core(1,4) =', u.get((1, 4)), ' u_core(2,4) =', u.get((2, 4)))

# cx_E5
P = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
for K in (2, 3):
    T = O.Tour(P, K)
    core = O.core_projection(T)
    print('cx_E5 K=%d (core) :' % K, [(str(a), p) for a, p in T.suite(core, 2)])
# cx_four_L11F1
P = [(0, 9, 0), (24, 9, 0), (12, 27, 0), (12, 0, 0)]
T = O.Tour(P, 2)
print('cx_four_L11F1 K=2 (core) :', [(str(a), p) for a, p in T.suite(O.core_projection(T), 2)])
# cx_square_K2
P = [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)]
T = O.Tour(P, 2)
print('cx_square_K2 (core) :', [(str(a), p) for a, p in T.suite(O.core_projection(T), 2)])
# line5 / entry_equal_level
P = [(0, 0, 0), (1, 0, 0), (5, 0, 0), (9, 0, 0), (10, 0, 0)]
T = O.Tour(P, 2)
print('line5 K=2 (core) :', [(str(a), p) for a, p in T.suite(O.core_projection(T), 2)])
# cx_fold_k2_n6 : {0,1,2} u {4,5} a 190/7 en FULL
P = [(1, 8, 9), (1, 8, 11), (4, 10, 6), (6, 2, 12), (11, 5, 10), (12, 8, 10)]
T = O.Tour(P, 2)
print('cx_fold_k2_n6 K=2 (core) :', [(str(a), p) for a, p in T.suite(O.core_projection(T), 2)])
# cx_fold_k3_n5 : FULL 01234@54
P = [(2, 6, 6), (2, 9, 6), (5, 7, 2), (9, 4, 5), (9, 10, 8)]
T = O.Tour(P, 3)
print('cx_fold_k3_n5 K=3 (core) :', [(str(a), p) for a, p in T.suite(O.core_projection(T), 2)])
