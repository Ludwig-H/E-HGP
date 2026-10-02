"""Two bounded analytic flow fixtures, not a product implementation or arithmetic benchmark."""
from fractions import Fraction as F


def fixture(name,points,center,weights):
    if sum(weights)!=1:raise RuntimeError('weights not normalized')
    if [sum(w*p[j] for w,p in zip(weights,points)) for j in range(3)]!=list(center):raise RuntimeError('center barycentric mismatch')
    u,v,w=[tuple(p[j]-points[0][j] for j in range(3)) for p in points[1:]]
    determinant=u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0])
    if determinant==0:raise RuntimeError('affinely dependent fixture')
    squared=[sum((F(p[j])-center[j])**2 for j in range(3)) for p in points]
    if len(set(squared))!=1:raise RuntimeError('not cospherical')
    low=[min(p[j] for p in points) for j in range(3)];high=[max(p[j] for p in points)+1 for j in range(3)]
    if not all(a<=c<b for a,c,b in zip(low,center,high)):raise RuntimeError('not owner root')
    return {'name':name,'points':points,'center':list(map(str,center)),'barycentric_weights':list(map(str,weights)),'affine_determinant':determinant,'strictly_inside':all(w>0 for w in weights),'owner_root':True,'level':str(squared[0]),'q':4,'initial_qmin':4,'p':0,'m':4,'kmax':3}


def pipeline(case,enforce_inside=True):
    stages=['valid_q4_center'];judged=census=levels=emitted=0
    if enforce_inside and not case['strictly_inside']:return {'stages':stages+['reject_not_strictly_inside'],'judged':judged,'census_tests':census,'q4_levels':levels,'emitted':emitted}
    stages+=['strict_inside_pass','half_open_owner_pass'];judged+=1;census=case['m']
    stages+=['census_full_shell','canonical_skipped_m_equals_q','support_equals_generated','admit_p_plus_qmin']
    if case['p']+case['initial_qmin']>case['kmax']+1:raise RuntimeError('fixture is not admitted')
    levels+=1;stages+=['materialize_q4_level','collector_accept'];emitted+=1
    return {'stages':stages,'judged':judged,'census_tests':census,'q4_levels':levels,'emitted':emitted}


def model():
    bad=fixture('closed_q4_outside_hull',[(0,0,0),(4,0,0),(0,4,0),(0,0,4)],(F(2),F(2),F(2)),(F(-1,2),F(1,2),F(1,2),F(1,2)))
    good=fixture('strict_q4',[(0,0,0),(2,2,0),(2,0,2),(0,2,2)],(F(1),F(1),F(1)),(F(1,4),)*4)
    out=[]
    for case in [bad,good]:
        first=pipeline(case);second=pipeline(case)
        if first!=second:raise RuntimeError('two pass trace diverges')
        out.append({'fixture':case,'first_pass':first,'second_pass':second,'logical_q4_levels':first['q4_levels'],'actual_two_pass_q4_levels':first['q4_levels']+second['q4_levels']})
    mutant=pipeline(bad,False)
    if mutant['emitted']!=1 or out[0]['first_pass']['emitted']!=0:raise RuntimeError('inside omission witness failed')
    return {'fixtures':out,'missing_inside_mutant':mutant,'scope':'Exact tiny fixtures and abstract stage trace only; no native candidate code exists in pinned source and no native run was made.'}
