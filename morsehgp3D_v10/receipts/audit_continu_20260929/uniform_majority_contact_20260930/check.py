"""Four-site affine-3D Fraction proof, full Gamma2, no native/GCP calls."""
from fractions import Fraction as F
from itertools import combinations
import json


def require(ok, message):
    if not ok: raise ValueError(message)


def dot(a,b): return sum((x*y for x,y in zip(a,b)), F(0))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def d2(a,b): return dot(sub(a,b),sub(a,b))


def solve(matrix, rhs):
    n=len(rhs)
    m=[list(row)+[value] for row,value in zip(matrix,rhs)]
    for j in range(n):
        i=next((i for i in range(j,n) if m[i][j]),None)
        if i is None: return None
        m[j],m[i]=m[i],m[j]
        pivot=m[j][j]; m[j]=[x/pivot for x in m[j]]
        for i in range(n):
            if i!=j:
                q=m[i][j]; m[i]=[x-q*y for x,y in zip(m[i],m[j])]
    return [row[-1] for row in m]


def sphere(P, support):
    a=P[support[0]]
    ds=[sub(P[i],a) for i in support[1:]]
    if not ds: return a,F(0)
    t=solve([[dot(x,y) for y in ds] for x in ds],[dot(x,x)/2 for x in ds])
    if t is None or min([1-sum(t),*t])<=0: return None
    center=tuple(a[j]+sum((u*x[j] for u,x in zip(t,ds)),F(0)) for j in range(3))
    beta=d2(center,a)
    require(all(d2(P[i],center)==beta for i in support),'support sphere')
    return center,beta


def catalogue(P):
    balls={}
    for q in range(1,5):
        for support in combinations(range(4),q):
            key=sphere(P,support)
            if key is not None: balls.setdefault(key,[]).append(support)
    out=[]
    for (center,beta),supports in balls.items():
        interior=tuple(i for i in range(4) if d2(P[i],center)<beta)
        shell=tuple(i for i in range(4) if d2(P[i],center)==beta)
        qmin=min(map(len,supports))
        out.append(dict(center=center,beta=beta,supports=supports,
                        interior=interior,shell=shell,qmin=qmin,
                        live=len(interior)<2<=len(interior)+len(shell),
                        strong=len(interior)+qmin<=2<=len(interior)+len(shell)))
    return out


def gamma(P, drop_abz=False):
    # Full Gamma2: ALL six pair vertices and ALL four three-part hyperedges.
    faces=list(combinations(range(4),2))
    birth={face:d2(P[face[0]],P[face[1]])/4 for face in faces}
    events={}
    for tri in combinations(range(4),3):
        sides=sorted(d2(P[a],P[b]) for a,b in combinations(tri,2))
        # Every triangle of this proved family is right/obtuse.
        require(sides[2]>=sides[0]+sides[1],'fixture triangle is acute')
        events[tri]=sides[2]/4
    levels=sorted(set(birth.values())|set(events.values()))
    def cut(beta, closed=True):
        active=[v for v in faces if birth[v]<beta or (closed and birth[v]==beta)]
        parent={v:v for v in active}
        def find(v):
            while parent[v]!=v: v=parent[v]
            return v
        for tri,value in events.items():
            if drop_abz and tri==(0,1,2): continue
            if value<beta or (closed and value==beta):
                vs=list(combinations(tri,2))
                require(all(v in parent for v in vs),'unborn Gamma endpoint')
                for v in vs[1:]: parent[find(v)]=find(vs[0])
        comps={}
        for v in active: comps.setdefault(find(v),[]).append(v)
        return tuple(sorted(tuple(sorted(vs)) for vs in comps.values()))
    return faces,birth,events,levels,cut


def owner(cut, ball):
    population=ball['interior']+ball['shell']
    facets=list(combinations(sorted(population),2))
    components=cut(ball['beta'])
    occupied=[comp for comp in components if any(face in comp for face in facets)]
    require(len(occupied)==1 and all(face in occupied[0] for face in facets),'nonunique live ball owner')
    return facets[0]


def projected(P, balls, eta, mode='strong', drop_abz=False, dedup_components=False):
    faces,birth,events,levels,cut=gamma(P,drop_abz)
    alpha=[min(value for face,value in birth.items() if x in face) for x in range(4)]
    selected=[]; times=[]; seeds=[]; geometric=[]; gap=None
    for x in range(4):
        threshold=(1+eta)**2*alpha[x]
        pairs=tuple(face for face in faces if d2(P[x],tuple((u+v)/2 for u,v in zip(P[face[0]],P[face[1]])))
                    <=birth[face] and birth[face]<=threshold)
        geometric.append(pairs)
        for face in faces:
            midpoint=tuple((u+v)/2 for u,v in zip(P[face[0]],P[face[1]]))
            if d2(P[x],midpoint)<=birth[face]:
                distance=abs(birth[face]-threshold)
                gap=distance if gap is None else min(gap,distance)
        if mode=='parts':
            rows=[(birth[face],face) for face in faces if x in face and birth[face]<=threshold]
        else:
            rows=[]
            for ball in balls:
                if ball['strong'] and x in ball['interior']+ball['shell'] and ball['beta']<=threshold:
                    rows.append((ball['beta'],owner(cut,ball)))
        rows.sort()
        require(rows,'empty vote set')
        if dedup_components:
            seen=set(); dedup=[]
            for c,v in rows:
                comp=next(comp for comp in cut(threshold) if v in comp)
                if comp not in seen: dedup.append((c,v)); seen.add(comp)
            rows=dedup
        selected.append(rows)
        for beta in levels:
            mass={}
            for c,v in rows:
                if c<=beta:
                    comp=next(comp for comp in cut(beta) if v in comp)
                    mass[comp]=mass.get(comp,0)+1
            wins=[comp for comp,value in mass.items() if 2*value>len(rows)]
            require(len(wins)<=1,'two strict majorities')
            if wins:
                times.append(beta); seeds.append(wins[0][0]); break
        else: raise ValueError('no majority')
    def height(x,y):
        for beta in levels:
            if beta>=times[x] and beta>=times[y]:
                components=cut(beta)
                if any(seeds[x] in comp and seeds[y] in comp for comp in components): return beta
        raise ValueError('points never join')
    return dict(times=times,selected=selected,geometric=geometric,gap=gap,
                height_ab=height(0,1),height_ay=height(0,3)),(faces,birth,events,levels,cut)


def fixture(delta, scale=F(1), M=None):
    if M is None:
        P=[(0,0,0),(10,0,0),(9-delta,3,0),(0,0,9)]
        eta=F(1,8); first=F(81,4); ab=F(25)
        root=(171-18*delta+delta*delta)/4
    else:
        T=M*M+1
        P=[(0,0,0),(2*T,0,0),(2*M*M-delta,2*M,0),(0,0,2*T)]
        eta=F(1,8) if M==3 else F(1,1000)
        first=M*M*T-M*M*delta+delta*delta/4
        ab=F(T*T); root=T*(2*M*M+1)-M*M*delta+delta*delta/4
    P=[tuple(F(c)*scale for c in point) for point in P]
    first*=scale*scale; ab*=scale*scale; root*=scale*scale
    determinant=P[1][0]*P[2][1]*P[3][2]
    require(determinant>0,'not affine dimension3')
    return P,eta,first,ab,root


def one_case(name,delta,scale=F(1),M=None):
    P,eta,first,ab,root=fixture(delta,scale,M)
    balls=catalogue(P)
    # Cross-check every MEB(pair/triple) against independent critical-support enumeration.
    strong,(faces,birth,events,levels,cut)=projected(P,balls,eta)
    for part,value in {**birth,**events}.items():
        candidates=[ball for ball in balls if any(set(s)<=set(part) for s in ball['supports'])
                    and all(d2(P[i],ball['center'])<=ball['beta'] for i in part)]
        require(candidates and min(ball['beta'] for ball in candidates)==value,'independent MEB mismatch')
    require(len(faces)==6 and len(events)==4 and len(balls)==10,'complete inventory')
    require(min(value for face,value in birth.items() if 0 in face)==first,'first cover alpha')
    require(strong['geometric']==[((0,1),(0,2),(0,3)),((1,2),),((1,2),),((0,3),)],
            'geometric band changed')
    require(strong['gap']>0,'band-edge coincidence')
    abball=next(ball for ball in balls if (0,1) in ball['supports'])
    require(abball['qmin']==2 and abball['live'],'AB qmin/live')
    require(abball['interior']==(() if delta==0 else (2,)),'AB interior census')
    require(abball['shell']==((0,1,2) if delta==0 else (0,1)),'AB shell census')
    require(abball['strong']==(delta==0),'AB strong transition')
    require(len(strong['selected'][0])==(3 if delta==0 else 2),'vote cardinality')
    expected=ab if delta==0 else root
    require(strong['times'][0]==expected and strong['height_ab']==expected,'strict-majority discontinuity')
    parts,_g=projected(P,balls,eta,mode='parts')
    require(len(parts['selected'][0])==3 and parts['times'][0]==ab and parts['height_ab']==ab,'K-parts control')
    # Full exact cuts/coverages are recorded on both sides of every critical plateau.
    traces=[]
    for beta in levels:
        for closed in (False,True):
            comps=cut(beta,closed)
            traces.append(dict(beta=str(beta),closed=closed,
                               components=[[list(face) for face in comp] for comp in comps],
                               covers=[sorted(set(i for face in comp for i in face)) for comp in comps]))
    if scale.denominator==1 and delta*scale==1:
        require(all(c.denominator==1 and 0<=c<2**18 for point in P for c in point),'u18 jitter fixture')
    return dict(name=name,delta=str(delta),scale=str(scale),M=M,eta=str(eta),
                points=[[str(c) for c in point] for point in P],determinant=str(P[1][0]*P[2][1]*P[3][2]),
                alpha_a2=str(first),threshold_a=str((1+eta)**2*first),minimum_squared_band_gap=str(strong['gap']),
                vote_count_a=len(strong['selected'][0]),entry_a=str(strong['times'][0]),
                height_ab=str(strong['height_ab']),height_ay=str(strong['height_ay']),
                parts_entry_a=str(parts['times'][0]),parts_height_ab=str(parts['height_ab']),
                ab_p=len(abball['interior']),ab_qmin=abball['qmin'],ab_strong=abball['strong'],
                full_gamma=traces),P,balls,eta


def main():
    specs=[('base_shell',F(0),F(1),None)]
    specs += [('base_inward_'+str(d),d,F(1),None) for d in (F(1,16),F(1,256),F(1,4096),F(1,65536))]
    specs += [('u18_shell_1024',F(0),F(1024),None),('u18_jitter1_1024',F(1,1024),F(1024),None)]
    for M in (3,32):
        specs += [('family_M%d_shell'%M,F(0),F(1),M),
                  ('family_M%d_inward'%M,F(1,65536),F(1),M)]
    reports=[]; fixtures={}
    for name,delta,scale,M in specs:
        report,P,balls,eta=one_case(name,delta,scale,M)
        reports.append(report); fixtures[name]=(P,balls,eta)
    mutants=[]
    for kind,name in (('drop_weak_abz_event','base_inward_1/16'),('component_dedup','base_shell')):
        P,balls,eta=fixtures[name]
        truth,_g=projected(P,balls,eta)
        altered,_g=projected(P,balls,eta,drop_abz=(kind=='drop_weak_abz_event'),
                              dedup_components=(kind=='component_dedup'))
        require(altered['height_ab']!=truth['height_ab'],'mutant survived')
        mutants.append(dict(kind=kind,case=name,correct_height=str(truth['height_ab']),
                            wrong_height=str(altered['height_ab'])))
    print(json.dumps(dict(status='UNIFORM_BAND_CONTACT_DISCONTINUITY_PROVED',K=2,cases=len(reports),
                         catalogue_balls_per_case=10,gamma_vertices_per_case=6,gamma_hyperedges_per_case=4,
                         reports=reports,wrong_value_mutants=mutants,
                         native_calls=0,GCP_used=False,
                         scope='affine3D exact Fraction Gamma2; strong-vote removal, not sphere splitting, native or benchmark'),
                     sort_keys=True,indent=2))


if __name__=='__main__': main()
