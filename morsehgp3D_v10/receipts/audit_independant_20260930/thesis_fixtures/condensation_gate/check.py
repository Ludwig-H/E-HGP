"""Read-only head gate on the prescribed target tree, not its production."""
from decimal import Decimal, localcontext
import hashlib
import json
from math import isfinite
from pathlib import Path

ROOT=Path(__file__).resolve().parent


def require(ok, why):
    if not ok:raise ValueError(why)


def close(x,y):
    return type(x) in (int,float) and isfinite(x) and abs(Decimal(x)-y)<Decimal('2e-12')


def blocks(labels):
    positive={};noise=[]
    for x,c in enumerate(labels):
        (noise if c==-1 else positive.setdefault(c,[])).append(x)
    return sorted(positive.values()),noise


def run():
    identities=[dict(mcs=m,z=z,single=False,selection='eom') for m in (2,3,4) for z in (1,2)]
    require(json.loads((ROOT/'case_identity.json').read_text())==identities,'case identity')
    execution=json.loads((ROOT/'execution.json').read_text())
    require(execution['new_compilations']==execution['new_generator_invocations']==0 and
            execution['point_tree_is_prescribed_not_produced_by_candidate'] is True,'scope')
    require(len(execution['commands'])==2,'two exact native commands')
    compare=0
    for backend,command in zip(('release','ubsan'),execution['commands']):
        for name in ('stdin','stdout','stderr'):
            p=ROOT/('native.stdin' if name=='stdin' else backend+'.'+name)
            require(hashlib.sha256(p.read_bytes()).hexdigest()==command[name+'_sha256'],'command/stream '+name)
        require(command['returncode']==0 and not (ROOT/(backend+'.stderr')).read_text(),'native exit/stderr')
        rows=[json.loads(line) for line in (ROOT/(backend+'.stdout')).read_text().splitlines()]
        require(len(rows)==len(identities),'native coverage')
        for key,row in zip(identities,rows):
            with localcontext() as c:
                c.prec=80
                tri=Decimal(4)/3;global_beta=2+Decimal(3).sqrt()
                lt=(1/tri.sqrt() if key['z']==1 else 1/tri)
                lg=(1/global_beta.sqrt() if key['z']==1 else 1/global_beta)
                big=key['mcs']<=3
                expected=([[0,1,2],[3,4,5]],[]) if big else ([],list(range(6)))
                require(blocks(row['label'])==expected,'target after EOM/root exclusion')
                require(row['mass']==([6,3,3] if big else [6]),'birth masses')
                require(row['parent']==([4294967295,0,0] if big else [4294967295]),'condensed topology')
                birth_expected=[Decimal(0),lg,lg] if big else [Decimal(0)]
                require(len(row['birth'])==len(birth_expected) and
                        all(close(v,w) for v,w in zip(row['birth'],birth_expected)), 'birth dates')
                stable=[6*lg,3*(lt-lg),3*(lt-lg)] if big else [6*lg]
                require(len(row['stability'])==len(stable) and all(close(v,w) for v,w in zip(row['stability'],stable)), 'stability')
                require(len(row['point_lambda'])==6 and all(close(v,lt if big else lg) for v in row['point_lambda']), 'point exits')
                compare+=1
    return dict(status='TARGET_TREE_HEAD_PASS',native_configurations=compare,
                positive_mcs=[2,3],negative_mcs=[4],z=[1,2],selection='eom',allow_single_cluster=False,
                new_compilations=0,new_generator_invocations=0,
                scope='prescribed two-triangle point tree; candidate construction and FULL vote not exercised')


if __name__=='__main__':
    p=ROOT/'SHA256SUMS'
    if p.exists():
        for line in p.read_text().splitlines():
            sha,rel=line.split('  ',1)
            require(hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==sha,'archive '+rel)
    print(json.dumps(run(),sort_keys=True,indent=2))
