from fractions import Fraction as F
a=(0,0,0); b=(20,0,0); c=(12,30,0)
def d2(p,q): return sum((x-y)**2 for x,y in zip(p,q))
E={'ab':d2(a,b),'ac':d2(a,c),'bc':d2(b,c)}
print('edges d2',E)
# acute?
s=sorted(E.values()); print('acute', s[2] < s[0]+s[1])
# Gabriel emptiness of each diametral ball (strict interior)
def diam_empty(p,q,o):
    m=tuple(F(x+y,2) for x,y in zip(p,q)); r2=F(d2(p,q),4)
    return sum((F(x)-y)**2 for x,y in zip(o,m)) >= r2
print('gabriel ab',diam_empty(a,b,c),'ac',diam_empty(a,c,b),'bc',diam_empty(b,c,a))
# circumradius^2 of triangle
A,B,C=E['bc'],E['ac'],E['ab']
import math
# R^2 = a^2 b^2 c^2 / (16 * area^2); 16 area^2 = 2(A B + B C + C A) - (A^2+B^2+C^2)
R2=F(A*B*C, 2*(A*B+B*C+C*A)-(A*A+B*B+C*C))
print('circumR2',R2,'> ab level',F(E['ab'],4), R2>F(E['ab'],4))
# K1 single linkage levels (r^2 = d^2/4): full vs ab omitted
def kruskal(edges,n=3):
    par=list(range(n))
    def f(x):
        while par[x]!=x: x=par[x]
        return x
    out=[]
    for w,u,v in sorted(edges):
        ru,rv=f(u),f(v)
        if ru!=rv: par[ru]=rv; out.append(F(w,4))
    return out
full=[(E['ab'],0,1),(E['ac'],0,2),(E['bc'],1,2)]
print('K1 fusion levels full', kruskal(full), 'ab omitted', kruskal(full[1:]))
# Euler E_1 = n - #q2(p=0) + #q3(p=0) - #q4(p=0)
print('E1 full', 3-3+1, 'ab omitted', 3-2+1)
