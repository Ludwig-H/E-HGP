from fractions import Fraction as F
import json

def need(ok, label):
    if not ok:
        raise RuntimeError(label)

def det(a):
    return (a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1])
            - a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0])
            + a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]))

P = [(31072,31072,31072), (231073,231074,31075),
     (231076,31077,231078), (31079,231080,231081)]
need(all(0 <= c < 2**18 for p in P for c in p), 'u18 domain')
u = [[F(P[i][j]-P[0][j]) for j in range(3)] for i in range(1,4)]
D = det(u)
need(D != 0, 'nondegenerate tetrahedron')
rhs = [sum(v*v for v in row)/2 for row in u]
center = []
for j in range(3):
    matrix = [row.copy() for row in u]
    for i in range(3):
        matrix[i][j] = rhs[i]
    center.append(det(matrix)/D)
beta = sum(c*c for c in center)
transposed = [list(row) for row in zip(*u)]
weights = []
for j in range(3):
    matrix = [row.copy() for row in transposed]
    for i in range(3):
        matrix[i][j] = center[i]
    weights.append(det(matrix)/D)
weights = [1-sum(weights)]+weights
need(all(w > 0 for w in weights) and sum(weights) == 1, 'strictly positive support')
for p in P:
    need(sum((F(p[j]-P[0][j])-center[j])**2 for j in range(3)) == beta, 'common sphere')
need(beta == F(3840384017632571220362572603308033439019683,128009600122397840006480000000000), 'exact q4 level')
need((4*beta).denominator != 1, 'not quarter-integer')
fifth = (131077,131077,131077)
need(all(0 <= q < 2**18 for q in fifth), 'fifth u18 domain')
fifth_distance2 = sum((F(fifth[j]-P[0][j])-center[j])**2 for j in range(3))
need(fifth_distance2 < beta, 'fifth site strictly inside identical MEB')
# K=5 on five sites: one Gamma_5 vertex, no edge. The original q4
# positive support forces the same MEB; the added site is interior.
# The single FULL_5 root covers all five points from A=beta to infinity.
eta, kappa = F(2,3), F(4)
A = beta
E2 = (1+eta)*A
W = E2-A
T_half = A+W/2
s_star = W*W/(16*kappa*kappa*A)
need(s_star < A < T_half < E2, 'single-branch MMt events')
need(W == F(3840384017632571220362572603308033439019683,192014400183596760009720000000000), 'true branch mass')
need(beta.numerator > 2**127-1 and W.numerator > 2**127-1, 'signed i128 overflow')
print(json.dumps({'status':'PASS','scope':'exact_geometric_MEB_and_one_branch_MMt_not_native_execution',
    'K':5,'n':5,'points':P+[fifth],'fifth_distance2':str(fifth_distance2),'beta':str(beta),'beta_num_bits':beta.numerator.bit_length(),
    'beta_den_bits':beta.denominator.bit_length(),'barycentric_weights':list(map(str,weights)),
    'eta':str(eta),'kappa':str(kappa),'W':str(W),'W_num_bits':W.numerator.bit_length(),
    'W_den_bits':W.denominator.bit_length(),'T_half':str(T_half),'critical_inside_band':False},sort_keys=True))
