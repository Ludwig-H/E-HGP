"""Actual sklearn on an equivalent ultrametric, not a native HGP/3D benchmark."""
from fractions import Fraction as F
import hashlib
import inspect
import itertools
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import numpy as np
import sklearn
from sklearn.cluster import HDBSCAN
from sklearn.cluster._hdbscan import _tree, _linkage


def require(value,message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def groups(labels):
    result = {}
    for point,label in enumerate(labels):
        result.setdefault(int(label),[]).append(point)
    return dict(noise=result.pop(-1,[]),clusters=sorted(result.values()))


def panel(extended,factor):
    levels = [F(1),F(4),F(9),F(16),F(25)]
    node_levels = [F(1),F(16),F(25)]
    parents = [2,2,None]
    point_nodes = [0,0,0,1,1]
    point_levels = [F(1),F(4),F(9),F(16),F(16)]
    if extended:
        levels += [F(100),F(1600)]
        node_levels += [F(100),F(1600)]
        parents = [2,2,4,4,None]
        point_nodes += [3,3]
        point_levels += [F(100),F(100)]
    point_nodes = [v for v in point_nodes for _ in range(factor)]
    point_levels = [v for v in point_levels for _ in range(factor)]
    def path(v):
        result = []
        while v is not None:
            result.append(v)
            v = parents[v]
        return result
    def ancestor(v,beta):
        while parents[v] is not None and node_levels[parents[v]]<=beta:
            v = parents[v]
        return v
    distances = np.zeros((len(point_nodes),len(point_nodes)),dtype=np.float64)
    squares = [[F(0) for _ in point_nodes] for _ in point_nodes]
    for i,j in itertools.combinations(range(len(point_nodes)),2):
        lca = next(v for v in path(point_nodes[i]) if v in path(point_nodes[j]))
        beta = max(point_levels[i],point_levels[j],node_levels[lca])
        squares[i][j] = squares[j][i] = beta
        root = int(beta)**0.5
        require(int(root)**2==beta,'fixture must have integral radius')
        distances[i,j] = distances[j,i] = root
    # Every closed cut of the compressed attachment tree agrees with its
    # pairwise ultrametric, including singleton completion before admission.
    cuts = sorted({F(0),*levels,*point_levels})
    for beta in cuts:
        owners = [(('singleton',i) if beta<date else ('component',ancestor(v,beta)))
                  for i,(v,date) in enumerate(zip(point_nodes,point_levels))]
        for i,j in itertools.combinations(range(len(point_nodes)),2):
            require((owners[i]==owners[j])==(squares[i][j]<=beta),'compressed/full cut mismatch')
    return distances,point_nodes,point_levels,len(cuts)


def run():
    paths = {str(Path(__file__).resolve()),inspect.getfile(HDBSCAN),sklearn.__file__,np.__file__,
             _tree.__file__,_linkage.__file__}
    before = {path:sha(path) for path in sorted(paths)}
    rows = []
    for extended,factor,z,single in itertools.product((False,True),(1,3),(1,2),(False,True)):
        matrix,nodes,dates,cuts = panel(extended,factor)
        # For z=2, monotone squaring leaves the ultrametric topology unchanged
        # and makes sklearn's lambda=1/d equal beta^-1, exactly as HGP z=2.
        metric = matrix**z
        mcs = 2 if factor==1 else 5
        estimator = HDBSCAN(metric='precomputed',min_samples=1,min_cluster_size=mcs,
                           cluster_selection_method='eom',allow_single_cluster=single,
                           algorithm='brute',copy=True,n_jobs=1)
        labels = estimator.fit_predict(metric)
        actual = groups(labels)
        expected = ([list(range(5*factor)),list(range(5*factor,7*factor))] if extended and z==1
                    else [list(range(5*factor))] if not extended and z==1 and single
                    else [list(range(3*factor)),list(range(3*factor,5*factor))]+
                         ([list(range(5*factor,7*factor))] if extended else []))
        require(actual['noise']==[] and actual['clusters']==expected,
                'actual sklearn selection differs from exact fixture derivation')
        rows.append(dict(extended=extended,factor=factor,z=z,allow_single=single,mcs=mcs,
                         n=len(nodes),cut_count=cuts,labels=list(map(int,labels)),partition=actual,
                         parameters=estimator.get_params(),matrix=metric.tolist(),
                         point_node=nodes,point_levels=list(map(str,dates))))
    after = {path:sha(path) for path in sorted(paths)}
    require(before==after,'dependency changed')
    return dict(status='PASS',sklearn_version=sklearn.__version__,numpy_version=np.__version__,
                optimize_flag=sys.flags.optimize,actual_fit_calls=len(rows),rows=rows,
                before=before,after=after,native_HGP_calls=0,GCP_used=False,
                scope='equivalent ultrametric head comparison; not an HGP-generated cloud or ARI benchmark')


if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True))
