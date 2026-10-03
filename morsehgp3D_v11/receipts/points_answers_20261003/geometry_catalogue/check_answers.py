#!/usr/bin/env python3
"""Independent bounded Q6/Q7 verification: exact Fraction geometry and radicals.

Runs only the frozen Python definition oracle. No native executable or fit.
Assertions intentionally avoided: normal and -O have identical guards.
"""
from fractions import Fraction as F
from math import isqrt
from decimal import Decimal, localcontext
from pathlib import Path
import json
import sys

BASE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(BASE / 'reference'))
from hgp11_ref import Definition  # noqa: E402

CHECKS = 0


def need(ok, label):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(label)


def classes(terms):
    groups = []
    for q, c in terms:
        q, c = F(q), F(c)
        if not c or not q:
            continue
        need(q > 0, 'positive radicand')
        for g in groups:
            ratio = q / g[0]
            a, b = isqrt(ratio.numerator), isqrt(ratio.denominator)
            if a*a == ratio.numerator and b*b == ratio.denominator:
                g[1] += c * F(a, b)
                break
        else:
            groups.append([q, c])
    return [(q, c) for q, c in groups if c]


def sign(terms):
    terms = classes(terms)
    if not terms:
        return 0
    for bits in (96, 192, 384, 768, 1536, 3072, 6144):
        lo = hi = F(0)
        unit = 1 << bits
        for q, c in terms:
            a = isqrt((q.numerator << (2*bits)) // q.denominator)
            lower = F(a, unit)
            upper = lower if a*a*q.denominator == q.numerator << (2*bits) else F(a+1, unit)
            lo += c * (lower if c > 0 else upper)
            hi += c * (upper if c > 0 else lower)
        if lo > 0:
            return 1
        if hi < 0:
            return -1
    raise RuntimeError('radical comparison resource refusal')


def diff(a, b):
    return a + [(q, -c) for q, c in b]


def mx(a, b):
    return a if sign(diff(a, b)) >= 0 else b


def dec(terms):
    with localcontext() as ctx:
        ctx.prec = 100
        return sum((Decimal(c.numerator)/Decimal(c.denominator) *
                    (Decimal(q.numerator)/Decimal(q.denominator)).sqrt()
                    for q, c in terms), Decimal(0))


def serial(terms):
    return [[str(q), str(c)] for q, c in classes(terms)]


def d2(a, b):
    return sum((F(x)-y)**2 for x, y in zip(a, b))


def triangle(points):
    a, b, c = [d2(points[i], points[j]) for i, j in ((1, 2), (0, 2), (0, 1))]
    area16 = 2*(a*b + a*c + b*c) - a*a - b*b - c*c
    need(area16 > 0, 'independent triangle')
    need(2*max(a, b, c) < a+b+c, 'acute triangle')
    return a*b*c / area16


def er0h_three(points, kappa):
    """Derive the three-node covering profile from acute triangle geometry.

    x=points[0], the short pair (1,2) does not cover x until the root.
    Own root vote is inherited proportionally by the two x-covering leaves.
    The majority margin is therefore (s1-s2)/(s1+s2), independently of W.
    """
    c1, c2 = sorted([d2(points[0], points[j])/4 for j in (1, 2)])
    root = triangle(points)
    need(d2(points[1], points[2])/4 < c1 < root, 'three pair births before root')
    need(root < 2*c1, 'eta1 root vote inside window')
    s1, s2 = root-c1, root-c2
    need(s1 >= s2 > 0, 'positive leaf votes')
    if s1 == s2:
        return [(root, F(1))], F(0), root
    mu = (s1-s2)/(s1+s2)
    cone = [(root, F(1)), (c1, -kappa*mu)]
    need(sign(diff(cone, [(c1, F(1))])) > 0, 'cone active')
    return cone, mu, root


def signature(res):
    memo = {}
    def one(v):
        if v not in memo:
            memo[v] = tuple(sorted(one(c) for c in res.nodes[v].children)) if res.nodes[v].children else ('birth', str(res.nodes[v].center))
        return repr(memo[v])
    return sorted(one(v) for v in range(len(res.nodes)))


def decorated(res, n):
    """Canonical labels by the sites covered at a node's birth, for this three-site family only."""
    labels={}
    for v,node in enumerate(res.nodes):
        cut=next(c for c in res.cuts if c.level==node.level)
        mask=next(coverage for w,coverage,_ in cut.closed if w==v)
        need(mask not in labels.values(),'unique labelled birth')
        labels[v]=mask
    birth={labels[v]:node.level for v,node in enumerate(res.nodes)}
    children={labels[v]:sorted(labels[c] for c in node.children) for v,node in enumerate(res.nodes)}
    cover={}
    for cut in res.cuts:
        for v,coverage,_ in cut.closed:
            for i in range(n):
                if coverage>>i&1:cover.setdefault((i,labels[v]),cut.level)
    return birth,children,cover


def check_s():
    rows = []
    fixed_parameter_checks = []
    for rho in range(2, 41):
        L, h = 8*rho*rho, 8*rho
        x = [(0,0,0), (L,h,0), (L,-h,0)]
        y = [x[0], x[1], (L,-h,1)]
        o = F(L*L+h*h,4)
        for kappa in [F(1,100),F(1,2),F(1),F(12),F(31)]:
            _, _, b = er0h_three(x,kappa)
            _, mu, bp = er0h_three(y,kappa)
            difference = [(F(1), b-bp-kappa*kappa*mu*mu*o), (o*bp,2*kappa*mu)]
            lower = F(16,129)*kappa*rho*rho-F(1,4)
            need(sign(diff(difference,[(F(1),lower)]))>=0,'S fixed-parameter bound')
            fixed_parameter_checks.append({'rho':rho,'kappa':str(kappa),'exact_lower_verified':True})
    for rho in [2, 3, 5, 10, 20, 40, 80, 160]:
        L, h = 8*rho*rho, 8*rho
        x = [(0,0,0), (L,h,0), (L,-h,0)]
        y = [x[0], x[1], (L,-h,1)]
        o = F(16*rho*rho*(rho*rho+1))
        b = F(16*(rho*rho+1)**2)
        eps = F((rho*rho+1)*(192*rho*rho-63), 4*(256*rho**4+rho*rho+1))
        tx, _, bx = er0h_three(x, F(12))
        ty, mu, by = er0h_three(y, F(12))
        need(bx == b and by == b+eps, 'Heron closed form S')
        need(0 < eps <= F(1,4), 'epsilon prime bounds')
        need(mu == F(1,4)/(2*(b-o)+2*eps-F(1,4)), 'margin closed form S')
        # X squared date - Y squared date = -eps - kappa^2*mu^2*o + 2*kappa*mu*sqrt(o*bprime).
        delta_squared = [(F(1), -eps-144*mu*mu*o), (o*by, 24*mu)]
        lower = F(16*12,129)*rho*rho - F(1,4)
        need(sign(diff(delta_squared, [(F(1), lower)])) >= 0, 'S lower bound')
        need(12*mu <= F(12,124*(rho*rho+1)), 'mu upper bound')
        dx, dy = Definition(x).order(2), Definition(y).order(2)
        need(sorted(n.level for n in dx.nodes) == sorted([o,o,F(h*h),b]), 'Definition X levels')
        need(sorted(n.level for n in dy.nodes) == sorted([o,o+F(1,4),F(h*h)+F(1,4),b+eps]), 'Definition Y levels')
        born_x,children_x,cover_x=decorated(dx,3)
        born_y,children_y,cover_y=decorated(dy,3)
        need(children_x==children_y and born_x.keys()==born_y.keys() and cover_x.keys()==cover_y.keys(),'matched decorated FULL')
        need(max([abs(born_x[v]-born_y[v]) for v in born_x]+[abs(cover_x[t]-cover_y[t]) for t in cover_x])==F(1,4),'decorated profile delta exactly one quarter')
        rows.append({'rho':rho, 'L':L, 'profile_delta_squared':'1/4', 'root_increment':str(eps),
                     'mu':str(mu), 'ratio_squared':str(dec(delta_squared)/Decimal('0.25')),
                     'claimed_lower_ratio':str(4*lower), 'date_X':serial(tx), 'date_Y':serial(ty)})
    # A different exact family disproves a geometry-Lipschitz radius bound.
    # For each fixed kappa>0 choose integer A>=max(48,4*kappa), then L=A*rho^4,h=A*rho^3.
    radial = []
    for kappa in [F(1,2),F(12),F(40)]:
        A = max(48, (4*kappa.numerator+kappa.denominator-1)//kappa.denominator)
        for rho in [2,5,10,20,40]:
            L, h = A*rho**4, A*rho**3
            x = [(0,0,0),(L,h,0),(L,-h,0)]
            y = [x[0],x[1],(L+1,-h,0)]
            tx, _, b = er0h_three(x,kappa)
            ty, mu, bp = er0h_three(y,kappa)
            o, d, s = F(L*L+h*h,4),F(2*L+1,4),b-F(L*L+h*h,4)
            need(bp >= b, 'radial positive root increment')
            need(sign([(bp,F(1)),(b,F(-1)),(F(1),F(-1))]) <= 0, 'MEB radius change at most displacement1')
            need(bp-b <= 3*d and 5*d <= s, 'radial margin denominator upper bound')
            need(mu >= F(8*rho*rho,15*L), 'radial mu lower bound')
            gap = diff(tx,ty)
            lower = kappa*F(rho*rho,4)-1
            need(sign(diff(gap,[(F(1),lower)])) >= 0,'radial geometric lower bound')
            radial.append({'kappa':str(kappa),'A':A,'rho':rho,'displacement_epsilon':'1',
                           'radius_jump':str(dec(gap)),'proved_lower':str(lower),
                           'inside_u21_after_common_translation':L+1 < 1<<21 and 2*h < 1<<21})
    return {'level_family':rows,'fixed_parameter_checks':fixed_parameter_checks,'radius_family':radial}


def projection(res, n, m, kappa):
    nodes = res.nodes
    parent = [-1]*len(nodes)
    for v,node in enumerate(nodes):
        for c in node.children:
            parent[c]=v
    def chain(v):
        out=[]
        while v>=0:
            out.append(v);v=parent[v]
        return out
    def lca(a,b):
        ancestors=set(chain(a))
        return next(v for v in chain(b) if v in ancestors)
    def up(v,date):
        while parent[v]>=0 and sign(diff(date,[(nodes[parent[v]].level,F(1))]))>=0:
            v=parent[v]
        return v
    profile=[{} for _ in range(n)]
    for cut in res.cuts:
        for v,coverage,_ in cut.closed:
            if coverage.bit_count()>=m:
                for i in range(n):
                    if coverage>>i&1:
                        profile[i].setdefault(v,cut.level)
    result=[]
    for p in profile:
        need(bool(p),'qualified profile nonempty')
        t=min(p.values());v=min(v for v,b in p.items() if b==t)
        e=[(t,F(1))]
        for rival,q in p.items():
            ancestor=lca(v,rival)
            meet=max(t,q) if ancestor in (v,rival) else max(t,q,nodes[ancestor].level)
            e=mx(e,[(meet,F(1)),(q,-F(kappa)),(t,F(kappa))])
        result.append((e,up(v,e)))
    def blocks(r, squared=False):
        groups={}
        for i,(date,owner) in enumerate(result):
            query = [(F(r),F(1))] if squared else [(F(1),F(r))]
            if sign(diff(query,date))>=0:
                alive=owner
                threshold = F(r) if squared else F(r)**2
                while parent[alive]>=0 and nodes[parent[alive]].level<=threshold:
                    alive=parent[alive]
                groups.setdefault(alive,[]).append(i)
        return sorted(groups.values())
    return result,blocks,parent


def check_q6():
    cases=[
        ('T0_equilateral_exact',[(1,1,2),(1,2,1),(2,2,2),(3,3,2),(4,4,2),(4,3,3)],'ABCDEF',3,1,[F(1)],[[0,1,2],[3,4,5]]),
        ('Q1bis_T1_1700',[(268,3000,0),(268,1000,0),(2000,2000,0),(3700,2000,0),(5432,3000,0),(5432,1000,0)],'ABCDEF',3,1,[F(1155),F(1400),F(1600),F(1787)],[[0,1,2],[3,4,5]]),
        ('Q2_S17',[(1000,1000,1000),(1100,1000,1000),(1010,1120,1000),(1010,1119,1016)],['x','a','b1','b2'],1,2,[F(61),F(65),F(70),F(75)],[[0,1],[2,3]])]
    rows=[]
    for name,points,names,m,kappa,cuts,wanted in cases:
        res=Definition(points).order(2)
        entries,blocks,parent=projection(res,len(points),m,kappa)
        got=[]
        for r in cuts:
            b=blocks(r);need(b==wanted,name+' target '+str(r));got.append({'radius':str(r),'blocks':b})
        # A site chosen by the projection really belongs to its owner's covering set.
        for i,(date,owner) in enumerate(entries):
            last=[cut for cut in res.cuts if sign(diff(date,[(cut.level,F(1))]))>=0][-1]
            masks={v:coverage for v,coverage,_ in last.closed}
            need(owner in masks and (masks[owner]>>i)&1,name+' faithful owner')
        # Radius dates are exact radical combinations; intervals below are only presentation.
        rows.append({'case':name,'sites':len(points),'qualification_m':m,'kappa':kappa,
                     'entry_dates':[serial(e) for e,_ in entries],
                     'entry_radii':[str(dec(e)) for e,_ in entries],
                     'owner_nodes':[o for _,o in entries],'cuts':got,
                     'nodes':[{'level':str(node.level),'children':list(node.children)} for node in res.nodes]})
        # Exact covariance under an integral homothety + translation and reversal of input order.
        transformed=[tuple(2*c+10000 for c in p) for p in reversed(points)]
        rt=Definition(transformed).order(2)
        et,bt,_=projection(rt,len(points),m,kappa)
        for i,(e,_) in enumerate(entries):
            need(sign(diff(et[len(points)-1-i][0],[(q,2*c) for q,c in e]))==0,name+' scale/permutation date')
        for r in cuts:
            need(sorted(sorted(len(points)-1-i for i in b) for b in bt(2*r))==wanted,name+' scale/permutation partition')
    return rows


def check_q2_variants():
    source=json.loads((BASE/'primary_sources/Q2_fixture_extract.json').read_text())['fixture']
    rows=[]
    for item in [{'name':'base','points':source['points'],'target':source['target']}]+source['variants']:
        names=list(item['points']);points=[tuple(item['points'][n]) for n in names]
        need(len(points)==4,'Q2 primary cardinality')
        res=Definition(points).order(2)
        entries,blocks,parent=projection(res,4,1,2)
        targets=[]
        for target in item['target']:
            lo,hi=[F(s) for s in target['r2']]
            wanted=sorted(sorted(names.index(n) for n in b) for b in target['blocks'])
            need(blocks(lo,squared=True)==wanted,'Q2 primary lower partition')
            # Every site is already active. Check all possible FULL changes inside the target interval.
            need(sum(map(len,wanted))==4,'Q2 target contains all sites')
            queried=[lo]+[n.level for n in res.nodes if lo<n.level<hi]
            if target['bounds'][1]==']':queried.append(hi)
            for level in queried:
                need(blocks(level,squared=True)==wanted,'Q2 primary partition throughout interval')
            targets.append({'r2':[str(lo),str(hi)],'bounds':target['bounds'],'blocks':wanted,'checked_FULL_cuts':len(queried)})
        rows.append({'variant':item['name'],'sites':4,'targets':targets,'entry_dates':[serial(e) for e,_ in entries]})
    return rows


def main():
    s=check_s();q6=check_q6();q2=check_q2_variants()
    result={'status':'PASS','checks':CHECKS,'scope':'frozen pure Python definition oracle, Fraction/radical comparisons; no native/GCP',
            'S':s,'Q6_cardinality_counterexample':q6,'Q2_primary_five_variants':q2,
            'limits':['No global geometric impossibility follows from S or fixed-threshold failures.',
                      'The Q6 construction uses only N to choose a rule: not natural, not insertion/deletion stable, not A5glob on the raw profile.',
                      'S is unbounded only across scales/profiles; a finite fixed u21 domain cannot contain the entire asymptotic sequence.',
                      'Upper stability bound 5 epsilon is a mathematical interleaving proof, not empirically established by this script.']}
    print(json.dumps(result,indent=2,sort_keys=True))


if __name__=='__main__':
    main()
