#!/usr/bin/env python3
"""Post-capture score/mass replay, not a campaign source or geometry gate.

Refuses incomplete/failed captures. ARI and classwise score arithmetic use
integer contingencies and Fraction, independently of sklearn/evaluation.py.
Hungarian optimization still uses scipy.optimize.linear_sum_assignment: its
solver is shared, not independently qualified here. No EOM, fit or geometry
is rerun. NMI is not recomputed. Only a fresh, separate report is written.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from fractions import Fraction as Q
import gzip
import hashlib
import importlib.metadata
import json
import math
from pathlib import Path

from scipy.optimize import linear_sum_assignment

BASELINE_SHA = '8f9aba99396b6341f7a6872956e66f50a7776cc1535d9a3f9137907d2945157a'
MANIFEST_SHA = '1cab6404055aebf0dfa31b76f16f8a4c84e4acf9678b2b910dabd9f367f46c67'
SCHEMA = 'mhgp9_weighted_full_gaussian_pilot_v2'
METHOD = 'hgp_weighted_full_vote'
CASES = tuple(f'spherical_g{g}_d{d}_s{s}' for g,d in ((2,8),(8,4),(16,2)) for s in (1,2,3)) + tuple(
    f'{regime}_g8_d4_s{s}' for regime in ('anisotropic','unbalanced') for s in (1,2))
ROW_FIELDS = ('case','regime','communities','separation','seed','n','k','min_cluster_size','exp_z','method','metrics','extra')


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    digest=hashlib.sha256()
    with Path(path).open('rb') as handle:
        for block in iter(lambda:handle.read(1024*1024),b''):
            digest.update(block)
    return digest.hexdigest()


def read(path):
    if str(path).endswith('.gz'):
        with gzip.open(path,'rt') as handle:
            return json.load(handle)
    return json.loads(Path(path).read_text())


def labels(values):
    need(isinstance(values,list) and values and all(type(x) is int for x in values),'nonempty integer labels')
    return values


def close_float(actual, exact, why):
    need(isinstance(actual,(int,float)) and not isinstance(actual,bool) and math.isfinite(actual),'finite score '+why)
    target=float(exact)
    tolerance=8*max(math.ulp(target),math.ulp(float(actual)))
    need(abs(actual-target)<=tolerance,'score differs from independent arithmetic: '+why)


def ari(truth, prediction):
    need(len(truth)==len(prediction),'ARI input length')
    choose=lambda x:x*(x-1)//2
    pairs=choose(len(truth))
    if not pairs:
        return Q(1)
    both=sum(choose(v) for v in Counter(zip(truth,prediction)).values())
    left=sum(choose(v) for v in Counter(truth).values())
    right=sum(choose(v) for v in Counter(prediction).values())
    numerator=2*(both*pairs-left*right)
    denominator=(left+right)*pairs-2*left*right
    return Q(numerator,denominator) if denominator else Q(1)


def independent_scores(truth, prediction, row):
    truth,prediction=labels(truth),labels(prediction)
    need(len(truth)==len(prediction)==1200,'whole scene scores')
    kept=[p for p,x in enumerate(prediction) if x>=0]
    metrics=row['metrics']; counts=Counter(x for x in prediction if x>=0)
    need(metrics['clusters']==len(counts) and metrics['noise_count']==len(truth)-len(kept),'cluster/noise count')
    close_float(metrics['coverage'],Q(len(kept),len(truth)),'coverage')
    inliers=[i for i,value in enumerate(truth) if value>=0]
    actual=[truth[i] for i in inliers]; assigned=[prediction[i] for i in inliers]
    singles=[value if value>=0 else ('inactive',i) for i,value in enumerate(assigned)]
    exact={'ari_all':ari(truth,prediction),'ari_true_inliers':ari(actual,assigned),
           'ari_inliers_noise_singletons':ari(actual,singles)}
    exact['ari_classified']=ari([truth[i] for i in kept],[prediction[i] for i in kept]) if len(kept)>=2 else None
    for key,value in exact.items():
        if value is None:
            need(metrics[key] is None,'undefined classified ARI')
        else:
            close_float(metrics[key],value,key)
    # Independent contingency construction; shared Hungarian solver only.
    classes=sorted(set(x for x in truth if x>=0)); predicted=sorted(counts)
    class_sizes=Counter(truth); intersections=Counter(zip(truth,prediction))
    matrix=[[intersections[a,b] for b in predicted] for a in classes]
    matching={}
    if classes and predicted:
        left,right=linear_sum_assignment(matrix,maximize=True)
        matching={int(i):int(j) for i,j in zip(left,right) if matrix[int(i)][int(j)]>0}
    extras=row['extra']; correct=0; precision=[]; recall=[]; f1=[]; exact_count=0
    need(extras['min_cluster_size'] is None,'point labels must not inherit a facet mass threshold')
    need(len(extras['per_class'])==len(classes),'class count')
    for i,a in enumerate(classes):
        saved=extras['per_class'][i]; j=matching.get(i)
        cluster=predicted[j] if j is not None else None
        tp=intersections[a,cluster] if cluster is not None else 0
        size=class_sizes[a]; found=counts[cluster] if cluster is not None else 0
        recovered=any(intersections[a,b]==size==counts[b] for b in predicted)
        for key,value in dict(truth_label=a,size=size,matched_label=cluster,matched_size=found,
                              intersection=tp,exact=recovered,eligible=None).items():
            need(saved[key]==value,'Hungarian per-class metadata '+key)
        p=Q(tp,found) if found else Q(0); r=Q(tp,size); f=Q(2*tp,size+found)
        for key,value in (('precision',p),('recall',r),('f1',f)):
            close_float(saved[key],value,'per-class '+key)
        precision.append(p); recall.append(r); f1.append(f); correct+=tp; exact_count+=recovered
    for key,value in dict(truth_classes=len(classes),predicted_clusters=len(counts),
                          matched_correct_points=correct,exact_classes=exact_count,
                          cluster_count_error=len(counts)-len(classes),
                          cluster_count_absolute_error=abs(len(counts)-len(classes))).items():
        need(extras[key]==value,'label summary '+key)
    for key,value in dict(exact_class_fraction=Q(exact_count,len(classes)),
                          matched_macro_precision=sum(precision)/len(classes),
                          matched_macro_recall=sum(recall)/len(classes),matched_macro_f1=sum(f1)/len(classes),
                          matched_micro_precision=Q(correct,len(kept)) if kept else Q(0),
                          matched_micro_recall=Q(correct,len(actual)),
                          matched_micro_f1=Q(2*correct,len(actual)+len(kept))).items():
        close_float(extras[key],value,key)
    return {key:None if value is None else [value.numerator,value.denominator] for key,value in exact.items()}


def inspect_measure(model,n,k):
    facets,scores,totals,masses=(model[key] for key in ('facets','scores','point_totals','masses'))
    need(len(facets)==len(scores)==len(masses) and len(totals)==n,'measure dimensions')
    need(len(model['attachments'])==len(model['leaf_birth_betas'])==len(facets),'FULL attachment dimensions')
    need([row['beta'] for row in model['attachments']]==model['leaf_birth_betas'],'facet birth/attachment binding')
    terms=[[] for _ in range(n)]
    for facet,score in zip(facets,scores):
        need(len(facet)==k and facet==sorted(set(facet)) and all(type(x) is int and 0<=x<n for x in facet),'facet IDs')
        need(isinstance(score,float) and math.isfinite(score) and score>0,'positive score')
        for point in facet:
            terms[point].append(score)
    need([math.fsum(row) for row in terms]==totals,'point totals incidence replay')
    for facet,score,mass in zip(facets,scores,masses):
        need(math.fsum(score/totals[p] for p in facet)==mass,'facet mass incidence replay')
        need(math.isfinite(mass) and 0<mass<=1+1e-12,'positive complete-boundary facet mass <=1')
    ratios=[mass.as_integer_ratio() for mass in masses]
    denominator=max(den for _,den in ratios)
    units=[num*(denominator//den) for num,den in ratios]
    exact=Q(sum(units),denominator); covered=sum(value>0 for value in totals)
    need(abs(exact-covered)<=Q(1,10**12)*max(1,covered),'total floating mass conservation')
    return units,denominator,abs(exact-covered)


def inspect_selection(selection,model,units,denominator):
    n=len(units); leaf_parent=selection['leaf_parent']
    need(selection['leaf_masses']==model['masses'] and len(leaf_parent)==n,'selection mass binding')
    parent={int(c):p for c,p in selection['cluster_parent'].items()}
    births={int(c):value for c,value in selection['births'].items()}
    cluster_units={c:0 for c in births}
    for leaf,c in enumerate(leaf_parent):
        need(c in cluster_units,'leaf assigned to missing cluster')
        cluster_units[c]+=units[leaf]
    for c in sorted(cluster_units,reverse=True):
        if c in parent:
            need(parent[c]<c and parent[c] in cluster_units,'cluster parent order')
            cluster_units[parent[c]]+=cluster_units[c]
    need(cluster_units[n]==sum(units),'root exact dyadic mass')
    for c,value in selection['cluster_mass_at_birth'].items():
        need(value==float(Q(cluster_units[int(c)],denominator)),'cluster rounded exact mass')
    leaf_edges={}
    outgoing=Counter()
    for edge in selection['condensed_tree']:
        child=edge['child']; c=edge['parent']
        if child<n:
            need(child not in leaf_edges and leaf_parent[child]==c,'unique leaf exit')
            leaf_edges[child]=edge; mass=units[child]
        else:
            need(parent.get(child)==c,'cluster edge parent')
            mass=cluster_units[child]
        need(edge['mass']==float(Q(mass,denominator)),'edge mass')
        outgoing[c]+=mass
    need(set(leaf_edges)==set(range(n)),'all facet leaves retained')
    need(all(outgoing[c]==mass for c,mass in cluster_units.items()),'exact outgoing mass conservation')
    need(selection['allow_single_cluster'] is False and n not in selection['selected'],'root exclusion')


def inspect_vote(model,selection,vote,row):
    facet_labels=selection['labels']; n=len(model['point_totals'])
    need(len(facet_labels)==len(model['facets']),'facet label count')
    terms=[defaultdict(list) for _ in range(n)]
    for facet,score,label in zip(model['facets'],model['scores'],facet_labels):
        need(type(label) is int and label>=-1,'facet label domain')
        if label>=0:
            for point in facet:
                terms[point][label].append(score)
    expected=[]; ties=0
    for point,groups in enumerate(terms):
        sums={c:math.fsum(values) for c,values in groups.items()}
        order=sorted(sums,key=lambda c:(-sums[c],c))
        expected.append(order[0] if order else -1)
        margin=sums[order[0]]-(sums[order[1]] if len(order)>1 else 0) if order else 0
        need(vote['raw_margins'][point]==margin,'raw vote margin')
        membership={str(c):value/model['point_totals'][point] for c,value in sums.items()}
        need(vote['memberships'][point]==membership,'normalized membership replay')
        ties+=bool(order) and margin==0
    need(vote['labels']==expected and row['vote_ties']==ties,'raw-score vote/argmax replay')
    sizes=sorted(Counter(value for value in expected if value>=0).values())
    need(row['final_point_sizes']==sizes,'final point sizes')
    need(row['final_point_clusters_below_mass_threshold']==sum(size<row['min_cluster_size'] for size in sizes),'no post-vote size filtering')


def audit(capture):
    receipt_path=capture/'receipt.json'; receipt_hash=sha(receipt_path); receipt=read(receipt_path)
    need(receipt['status']=='completed','score audit requires completed capture; failed stays failed')
    need(receipt['schema']==SCHEMA,'capture schema')
    need(receipt['plan']==dict(cases=list(CASES),k=[5,10],sizes=[20,50],exp_z=[1,2]),'fixed pilot plan')
    need(len(receipt['rows'])==364 and len(receipt['commands'])==26,'complete grid')
    need(receipt['sources_before']==receipt['sources_after'],'source closure recorded')
    pins={str(receipt_path):receipt_hash,**receipt['sources_before'],**receipt['input_hashes'],**receipt['artifacts']}
    for path_key,hash_key in (('native_binary','native_binary_sha256'),('qualification','qualification_sha256'),
                              ('baseline_receipt','baseline_receipt_sha256'),('manifest','manifest_sha256')):
        pins[receipt[path_key]]=receipt[hash_key]
    need(receipt['baseline_receipt_sha256']==BASELINE_SHA and receipt['manifest_sha256']==MANIFEST_SHA,'inherited pins')
    for path,value in pins.items():
        need(sha(path)==value,'input/source/artifact pin: '+path)
    baseline=read(receipt['baseline_receipt']); manifest=read(receipt['manifest'])
    cases={row['id']:row for row in manifest['cases']}
    row_key=lambda row:tuple(row[name] for name in ('case','k','min_cluster_size','exp_z','method'))
    saved_rows={row_key(row):row for row in receipt['rows']}
    need(len(saved_rows)==364,'duplicate score row')
    expected=set()
    for case in CASES:
        for k in (5,10):
            for minimum in (20,50):
                for method in (METHOD,'hgp_first_coverage','hdbscan_common'):
                    expected.update((case,k,minimum,z,method) for z in (1,2))
                expected.add((case,k,minimum,1,'hdbscan_standard'))
    need(set(saved_rows)==expected,'exact method/parameter grid')
    copied=0
    for previous in baseline['rows']:
        key=row_key(previous)
        if key in expected:
            need(saved_rows[key]=={field:previous[field] for field in ROW_FIELDS},'copied comparator changed')
            copied+=1
    need(copied==260,'all comparator rows copied')
    commands={(command['case'],command['k']):command for command in receipt['commands']}
    need(set(commands)=={(case,k) for case in CASES for k in (5,10)},'command grid')
    fractions=[]; mass_error=Q(0); facet_exits=0; measure_count=0
    for case in CASES:
        truth=read(cases[case]['labels_json'])
        for k in (5,10):
            folder=capture/f'{case}_k{k}'; command=commands[case,k]
            need(command['returncode']==0,'native command failure')
            for suffix,filename in (('stdout','native.json'),('stderr','native.stderr')):
                path=str(folder/filename); pins[path]=command[suffix+'_sha256']
                need(sha(path)==pins[path],'native command output pin')
            for z in (1,2):
                model_path=str(folder/f'measure_z{z}.json.gz'); need(model_path in pins,'measure authority')
                model=read(model_path); units,denominator,error=inspect_measure(model,1200,k)
                mass_error=max(mass_error,error); measure_count+=1
                for minimum in (20,50):
                    path=str(folder/f'weighted_m{minimum}_z{z}.json.gz'); need(path in pins,'label authority')
                    result=read(path); row=saved_rows[case,k,minimum,z,METHOD]
                    inspect_selection(result['selection'],model,units,denominator)
                    inspect_vote(model,result['selection'],result['vote'],row)
                    exact=independent_scores(truth,result['vote']['labels'],row)
                    facet_exits+=len(units)
                    fractions.append(dict(case=case,k=k,min_cluster_size=minimum,exp_z=z,ari=exact))
    for path,value in pins.items():
        need(sha(path)==value,'closure after audit: '+path)
    return dict(schema='mhgp9_weighted_post_capture_score_audit_v1',status='passed',capture=str(capture),
        receipt_sha256=receipt_hash,audit_source_sha256=sha(__file__),weighted_rows=len(fractions),
        comparator_rows=copied,total_rows=364,measures=measure_count,facet_exits_checked=facet_exits,
        mass_max_absolute_error_from_covered_points=[mass_error.numerator,mass_error.denominator],
        exact_ARI=fractions,pins_checked=len(pins),pins_sha256=pins,
        Hungarian_solver_shared=True,Hungarian_solver='scipy.optimize.linear_sum_assignment',
        scipy_version=importlib.metadata.version('scipy'),NMI_recomputed=False,
        geometry_or_EOM_rerun=False,GCP_used=False,
        limitations=['Post-capture addition, not inherited qualification.',
          'Exact ARI arithmetic is independent; Hungarian optimization uses the shared SciPy solver.',
          'Mass conservation concerns rounded input masses, not certified real geometric weights.',
          'No statistical dominance, nested point partition, or final cardinality threshold is inferred.'])


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path); parser.add_argument('output',type=Path)
    args=parser.parse_args(); need(not args.output.exists(),'fresh report path required')
    result=audit(args.capture.resolve())
    with args.output.open('x') as handle:
        json.dump(result,handle,sort_keys=True,indent=2,allow_nan=False); handle.write('\n')
    print(json.dumps({key:value for key,value in result.items() if key not in ('exact_ARI','pins_sha256','limitations')},sort_keys=True))
