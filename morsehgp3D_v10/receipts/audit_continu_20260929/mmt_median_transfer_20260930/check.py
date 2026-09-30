#!/usr/bin/env python3
"""Bounded rational tree proof; AST reference functions only, no native import."""
import ast
import json
import math
from fractions import Fraction as F
from pathlib import Path


def require(c, message):
    if not c:
        raise ValueError(message)


def rational_sqrt(x):
    x = F(x)
    a, b = math.isqrt(x.numerator), math.isqrt(x.denominator)
    return F(a, b) if a*a == x.numerator and b*b == x.denominator else None


class QS:
    def __init__(self, terms=()):
        self.terms = {}
        for r, a in terms:
            r, a = F(r), F(a)
            if not a or not r:
                continue
            for old in self.terms:
                q = rational_sqrt(r/old)
                if q is not None:
                    self.terms[old] += a*q
                    break
            else:
                self.terms[r] = a
        self.terms = {r:a for r,a in self.terms.items() if a}

    @staticmethod
    def sqrt(r):
        return QS([(F(r), F(1))])

    @staticmethod
    def rat(a):
        return QS([(F(1), F(a))])

    def __add__(self, other):
        other = other if isinstance(other, QS) else QS.rat(other)
        return QS(list(self.terms.items()) + list(other.terms.items()))

    __radd__ = __add__

    def __mul__(self, a):
        return QS([(r, q*F(a)) for r, q in self.terms.items()])

    def __sub__(self, other):
        return self + (other if isinstance(other, QS) else QS.rat(other)) * -1

    def cmp(self, other):
        d = self-other
        if not d.terms:
            return 0
        for bits in (16, 32, 64, 128, 256, 512, 1024, 4096):
            den = 1 << bits
            lo = hi = F(0)
            for r, a in d.terms.items():
                floor = math.isqrt((r.numerator << (2*bits)) // r.denominator)
                l, h = F(floor, den), F(floor+1, den)
                if l*l == r:
                    h = l
                lo += a*(l if a > 0 else h)
                hi += a*(h if a > 0 else l)
            if lo > 0:
                return 1
            if hi < 0:
                return -1
        raise ValueError('bounded exact comparison unresolved')

    def serial(self):
        return sorted([[str(r), str(a)] for r,a in self.terms.items()])


class Rayon:
    def __init__(self, b2=None, q=None):
        self.q = q if q is not None else QS.sqrt(b2)

    @staticmethod
    def somme(q):
        return Rayon(q=q)

    def cmp(self, other):
        return self.q.cmp(other.q)


class Tree:
    def __init__(self, births, parents):
        self.birth = list(map(F, births))
        self.parent = parents
        self.root = parents.index(-1)
        self.children = [[] for _ in births]
        self.death = [None if p < 0 else self.birth[p] for p in parents]
        for v,p in enumerate(parents):
            if p >= 0:
                self.children[p].append(v)
                require(self.birth[p] > self.birth[v], 'strict toy lives')

    def ancestor(self, v, s):
        while self.parent[v] >= 0 and self.birth[self.parent[v]] <= s:
            v = self.parent[v]
        return v

    def lca(self, a, b):
        seen = set()
        while a >= 0:
            seen.add(a)
            a = self.parent[a]
        while b not in seen:
            b = self.parent[b]
        return b

    def preorder(self):
        out = []
        def visit(v):
            out.append(v)
            for c in self.children[v]:
                visit(c)
        visit(self.root)
        return out


def atoms(T, cv, E):
    out = []
    for v,c in cv.items():
        e = min(E,T.death[v]) if T.death[v] is not None else E
        if e > c:
            out.append((v,c,e))
    return out


def naive(T, aa, s):
    out = {}
    for v,c,e in aa:
        if T.birth[v] <= s:
            C = T.ancestor(v,s)
            out[C] = out.get(C,F(0)) + max(F(0), min(e,s)-c)
    return out


def median_stream(T, aa):
    ws = {v:e-c for v,c,e in aa}
    W = sum(ws.values(),F(0))
    acc = F(0)
    m = None
    for v in T.preorder():
        acc += ws.get(v, F(0))
        if 2*acc >= W and v in ws:
            m = v
            break
    require(m is not None, 'median absent')
    events = {}
    for v,c,e in aa:
        h = T.birth[T.lca(v,m)]
        a = max(c,h)
        jump = min(a,e)-c
        j,p = events.get(a,(F(0),0))
        events[a] = (j+jump, p+(1 if a < e else 0))
        if a < e:
            j,p = events.get(e,(F(0),0))
            events[e] = (j,p-1)
    require(len(events) <= 2*len(aa), 'event bound')
    trace = []
    prev = G = F(0)
    slope = 0
    Th = T1 = None
    for s,(jump, ds) in sorted(events.items()):
        left = G + slope*(s-prev)
        if Th is None and slope and G <= W/2 < left:
            Th = prev+(W/2-G)
        G = left+jump
        slope += ds
        require(slope in (0,1), 'lineage slope not 0/1')
        if Th is None and G > W/2:
            Th = s
        if T1 is None and G == W:
            T1 = s
        trace.append((s,left,G,slope))
        prev = s
    require(Th is not None and T1 is not None, 'stream thresholds absent')
    return m,W,Th,T1,trace


def stream_mass(trace,s):
    G = F(0)
    for e,_left,g,p in trace:
        if e > s:
            break
        G = g+p*(s-e)
    return G


def date_from_stream(T, m, W, Th, T1, trace, kappa, A):
    date = QS.sqrt(Th)
    star = W*W/(16*kappa*kappa*A)
    def candidate(s,g):
        return QS.sqrt(s)-QS.sqrt(A)*(kappa*(2*g/W-1))
    for i,(e,left,g,p) in enumerate(trace):
        if Th < e <= T1:
            v = candidate(e,left)
            if v.cmp(date) > 0:
                date = v
        if i+1 < len(trace) and p:
            hi = trace[i+1][0]
            if max(e,Th) < star < min(hi,T1):
                v = candidate(star,g+star-e)
                if v.cmp(date) > 0:
                    date = v
    owner = T.ancestor(m,Th)
    while T.parent[owner] >= 0 and QS.sqrt(T.birth[T.parent[owner]]).cmp(date) <= 0:
        owner = T.parent[owner]
    return date,owner


def run():
    p = Path(__file__).with_name('reference_functions.py')
    t = ast.parse(p.read_text())
    require(all(isinstance(n,ast.FunctionDef) for n in t.body), 'function-only AST')
    require({n.name for n in t.body} == {'_anc','mmt_point'}, 'AST whitelist')
    env = {'Fraction':F,'QS':QS,'Rayon':Rayon,'exiger':require}
    exec(compile(t,str(p),'exec'),env)
    cases = [
        ('single',Tree([1],[-1])),
        ('half_plateau',Tree([1,1,9],[2,2,-1])),
        ('nary_three',Tree([1,1,1,9],[3,3,3,-1])),
        ('mixed',Tree([1,1,1,2,9],[3,3,4,4,-1])),
        ('critical',Tree([1]*10+[F(3,2),F(3,2),4],[10]*10+[12,12,-1])),
    ]
    output = []
    for name,T in cases:
        cv = dict(enumerate(T.birth))
        aa = atoms(T,cv,F(4))
        m,W,Th,T1,trace = median_stream(T,aa)
        ref = env['mmt_point'](T,cv,F(3),F(2))
        require((W,Th,T1) == (ref['W'],ref['T_half'],ref['T1']), name+' thresholds')
        date,owner = date_from_stream(T,m,W,Th,T1,trace,F(2),F(1))
        require(date.cmp(ref['date']) == 0 and owner == ref['owner'], name+' date/owner')
        ev = sorted(set(T.birth + [F(1),F(4),Th] + [s for s,_,_,_ in trace]))
        pts = set(ev)
        for a,b in zip(ev,ev[1:]):
            pts.update([a+(b-a)/2,a+(b-a)/3,a+(b-a)*F(999,1000)])
        for s in sorted(pts):
            masses = naive(T,aa,s)
            G = max(masses.values(),default=F(0))
            gs = stream_mass(trace,s)
            require((2*G > W) == (2*gs > W), name+' strict majority')
            if s >= T.birth[m]:
                require(gs == masses.get(T.ancestor(m,s),F(0)), name+' exact lineage mass')
            if 2*G > W:
                require(G == gs, name+' majority is median lineage')
        output.append({'name':name,'D':len(aa),'events':len(trace),'samples':len(pts),
                       'median':m,'W':str(W),'T_half':str(Th),'T1':str(T1),
                       'date':date.serial(),'owner':owner,'reference_argmax':ref['argmax'][0]})
    require(output[1]['T_half'] == '9', 'strict half plateau must wait until fusion')
    require(output[4]['reference_argmax'] == 'critique', 'critical point exercised')
    # Actual two-site scale family: exact published B_t, not a violation of real dates.
    eta,kappa,lam,eps = F(3),F(2),F(2),F(1)
    aX,aY = F(1),F(2)
    WX,WY = eta*aX*aX,eta*aY*aY
    DX,DY = 2*eps*lam*aX*(lam+1),2*eps*lam*aY*(lam+1)
    exact_bt = (1+kappa)*eps+2*kappa*max(aX,aY)*(DX+DY)/min(WX,WY)
    simplified = (1+kappa)*eps+8*kappa*lam/eta*(lam+1)*eps
    require((exact_bt,simplified) == (99,35), 'finite-bound algebra')
    return {'scope':'abstract tree Fraction/AST, no engine; five toys; no runtime scalability claim',
            'median_cases':output,'finite_bound':{'exact_Bt':str(exact_bt),
            'announced_simplified':str(simplified),'actual_date_gap':'sqrt(5/2)',
            'not_a_stability_counterexample':True}}


if __name__ == '__main__':
    print(json.dumps(run(),sort_keys=True,separators=(',',':')))
