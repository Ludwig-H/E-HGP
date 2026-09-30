"""Independent Fraction cut oracle; no engine/sklearn calls."""
from fractions import Fraction as F
import json
import sys


def require(ok,message):
    if not ok: raise ValueError(message)


def part(matrix,points,beta,closed):
    remaining=set(points); groups=[]
    while remaining:
        first=min(remaining); remaining.remove(first); group={first}; work=[first]
        while work:
            u=work.pop()
            found={v for v in remaining if matrix[u][v]<beta or (closed and matrix[u][v]==beta)}
            remaining-=found; group|=found; work.extend(found)
        groups.append(group)
    return groups


def strict_oracle(matrix,z):
    clusters=[dict(parent=None,birth=F(0),stability=F(0))]
    live={0:set(range(12))}; exited=[None]*12; exits=[None]*12
    def drop(points,c,lam):
        for x in points:
            require(exited[x] is None,'duplicate drop')
            exited[x],exits[x]=c,lam
        clusters[c]['stability']+=len(points)*(lam-clusters[c]['birth'])
    for beta in (F(25),F(1)):
        lam=F(1,5 if z==1 else 25) if beta==25 else F(1)
        for c,points in list(live.items()):
            components=part(matrix,points,beta,False)
            if len(components)==1: continue
            del live[c]
            big=[p for p in components if len(p)>=5]
            if len(big)<2:
                keep=big[0] if big else set(); drop(points-keep,c,lam)
                if keep: live[c]=keep
            else:
                for p in components:
                    if len(p)<5: drop(p,c,lam)
                    else:
                        clusters[c]['stability']+=len(p)*(lam-clusters[c]['birth'])
                        child=len(clusters); clusters.append(dict(parent=c,birth=lam,stability=F(0)))
                        live[child]=p
    require(not live and all(c is not None for c in exited),'finished oracle')
    require(len(clusters)==1,'one big continuation, no split')
    return clusters,exits


def groups(labels):
    result={}; noise=[]
    for x,label in enumerate(labels):
        if label<0: noise.append(x)
        else: result.setdefault(label,[]).append(x)
    return {'clusters':sorted(result.values()),'noise':noise}


def near(observed,expected):
    return abs(F(str(observed))-expected)<=F(1,10**12)*max(F(1),abs(expected))


def judge(text):
    rows=[json.loads(line) for line in text.splitlines() if line.strip()]
    require(len(rows)==4,'four rows')
    by={(row['case'],row['z']):row for row in rows}
    require(set(by)=={(c,z) for c in ('flat','factored') for z in (1,2)},'row inventory')
    expected=[[0 if i==j else 1 if i//3==j//3 or (i>=6 and j>=6) else 25
               for j in range(12)] for i in range(12)]
    pair_checks=0; cuts=0; reports=[]
    for z in (1,2):
        flat,fact=by['flat',z],by['factored',z]
        for row in (flat,fact):
            require(row['api_valid'] is True and row['mcs']==5 and row['allow_single'] is False,'native API/params')
            require(row['point_rank']==[0]*12 and row['point_weight']==[1]*12,'unit direct points')
            require(len(row['matrix_beta'])==12,'matrix dimension')
            for i in range(12):
                require(len(row['matrix_beta'][i])==12,'matrix row')
                for j in range(12):
                    require(row['matrix_beta'][i][j]==expected[i][j],'ultrametric differs')
                    pair_checks+=1
        for beta in (F(0),F(1),F(4),F(25),F(26)):
            for closed in (False,True):
                require(part(flat['matrix_beta'],range(12),beta,closed)
                        ==part(fact['matrix_beta'],range(12),beta,closed),'cut differs')
                cuts+=1
        oracle,exits=strict_oracle(expected,z)
        lam=F(1,5 if z==1 else 25)
        require(flat['selected']==[] and groups(flat['labels'])=={'clusters':[],'noise':list(range(12))},
                'flat selection')
        require(len(flat['clusters'])==1 and near(flat['clusters'][0]['stability'],6*lam+6),
                'flat stability')
        require(oracle[0]['stability']==6*lam+6,'Fraction oracle stability')
        require(all(near(a,b) for a,b in zip(flat['point_lambda'],exits)),'flat exits')
        require(fact['selected']==[1,2] and groups(fact['labels'])
                =={'clusters':[list(range(6)),list(range(6,12))],'noise':[]},'factored selection observed')
        require(len(fact['clusters'])==3,'factored condensed branches')
        for row,(birth,mass,stability) in zip(fact['clusters'],[(F(0),12,12*lam),(lam,6,F(0)),(lam,6,6*(1-lam))]):
            require(near(row['birth'],birth) and row['mass']==mass and near(row['stability'],stability),
                    'factored branch diagnostic')
        require(fact['clusters'][1]['stability']==0 and 1 in fact['selected'],'zero stability native leaf selected')
        reports.append({'z':z,'oracle_flat_root_stability':str(oracle[0]['stability']),
                        'factored_exact_stabilities':[str(12*lam),'0',str(6*(1-lam))],
                        'flat_partition':groups(flat['labels']),'factored_partition':groups(fact['labels']),
                        'native_zero_stability_leaf_selected':True})
    return {'status':'EXPECTED_PLATEAU_API_NONINVARIANCE','rows':4,'pair_checks':pair_checks,
            'equal_cut_checks':cuts,'reports':reports,'new_native_calls':0,'GCP_used':False,
            'scope':'two equivalent API ultrametrics, no 3D/Gamma/sklearn realization'}


if __name__=='__main__':
    require(len(sys.argv)==2,'usage judge.py capture')
    with open(sys.argv[1],encoding='utf-8') as stream: text=stream.read()
    print(json.dumps(judge(text),sort_keys=True,indent=2))
