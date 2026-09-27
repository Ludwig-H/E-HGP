#!/usr/bin/env python3
"""Read-only post-capture mathematical audit; writes only a new separate report."""
import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import sys

CODE = Path(__file__).resolve().parent.parent
sys.path.insert(0,str(CODE))
from condensed import validate_condensed_tree, condensed_cut


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def audit(capture):
    receipt_path=capture/'receipt.json'
    receipt_hash=sha(receipt_path)
    receipt=json.loads(receipt_path.read_text())
    need(receipt['status']=='completed' and not receipt['failures'],'capture must be terminal completed')
    need(len(receipt['commands'])==96 and len(receipt['rows'])==1920,'complete prescribed capture')
    source=CODE/'condensed.py'
    source_hash=sha(source)
    need(source_hash==receipt['sources_before'][str(source)]==receipt['sources_after'][str(source)],'validator source closed')
    manifest_path=Path(receipt['manifest_path'])
    need(sha(manifest_path)==receipt['manifest_sha256'],'data manifest identity')
    manifest=json.loads(manifest_path.read_text())
    cases={case['id']:case for case in manifest['cases']}
    sizes=(10,20,50,100)
    methods=('hgp_first_coverage','hdbscan_common')
    signatures=('parent','children','children_offsets','point_exit_parent','point_exit_ids','point_exit_offsets','mass_at_birth','legacy_cluster_ids')
    pins={}; pairs=0; arrays=0; dates=0; point_exits=0; multiply_differences=0; max_ulp=0.; subset_checks=0
    sequences=[]; cut_results=[]; selected_birth_sets=0; bound_slacks=[]; leaf_mass_checks=0
    sample_cases={'spherical_g2_d8_s1','spherical_g16_d8_s1','spherical_g16_d2_s1','unbalanced_g8_d8_s1'}
    for case in manifest['cases']:
        for k in (5,10):
            folder=capture/(case['id']+'_k'+str(k))
            for method in methods:
                counts={1:[],2:[]}; previous=None
                for m in sizes:
                    pair=[]
                    for z in (1,2):
                        path=folder/f'{method}_m{m}_z{z}.json'
                        pins[str(path)]=sha(path)
                        payload=json.loads(path.read_text()); tree=payload['condensed_tree']
                        check=validate_condensed_tree(tree)
                        need(tree['n_points']==case['n']==1200 and tree['min_cluster_size']==m and tree['exp_z']==z,'case metadata')
                        bound=max(1,2*(tree['n_points']//m)-1)
                        need(check['clusters']<=bound,'C exceeds max(1,2floor(n/m)-1)')
                        bound_slacks.append(bound-check['clusters'])
                        arrays+=1; point_exits+=check['point_exits']; counts[z].append(check['clusters'])
                        need(payload['stats']['point_exits']==1200 and payload['stats']['points_removed_from_input']==0,'no points deleted')
                        births=[]
                        for c in range(check['clusters']):
                            lo,hi=tree['point_exit_offsets'][c:c+2]
                            births.append(set(tree['point_exit_ids'][lo:hi]))
                        for c in reversed(range(check['clusters'])):
                            need(len(births[c])==tree['mass_at_birth'][c],'independent birth-set mass')
                            if c:
                                p=tree['parent'][c]
                                need(not (births[p]&births[c]),'overlapping birth-set sources')
                                births[p].update(births[c])
                        leaves=[c for c in range(check['clusters']) if tree['children_offsets'][c]==tree['children_offsets'][c+1]]
                        if check['clusters']>1:
                            need(all(c!=0 and len(births[c])>=m for c in leaves),'nonroot leaf birth mass below m')
                            need(len(leaves)<=1200//m and check['clusters']<=2*len(leaves)-1,'condensed leaf-count bound')
                        need(sum(len(births[c]) for c in leaves)==len(set().union(*(births[c] for c in leaves))),
                             'leaf cluster birth sets overlap')
                        leaf_mass_checks+=len(leaves)
                        for label,c in enumerate(payload['selection']['selected']):
                            need(c!=0 and births[c]=={p for p,x in enumerate(payload['selection']['labels']) if x==label},'selection differs from full birth set')
                            selected_birth_sets+=1
                        if k==5 and m in (20,100) and case['id'] in sample_cases:
                            events=sorted(set([0.,*tree['birth_lambda'],*tree['point_exit_lambda']]))
                            cuts=sorted(set([events[0],events[len(events)//3],events[2*len(events)//3],events[-1]]))
                            prior=None
                            for at in cuts:
                                view=condensed_cut(tree,at)
                                groups=[set(row['points']) for row in view['clusters']]+[{p} for p in view['inactive_points']]
                                need(sum(map(len,groups))==1200 and set().union(*groups)==set(range(1200)),'cut partition missing/duplicating points')
                                if prior is not None:
                                    need(all(any(group<=old for old in prior) for group in groups),'lambda cuts fail refinement')
                                prior=groups
                            cut_results.append(dict(case=case['id'],k=k,m=m,z=z,method=method,cuts=len(cuts),points=1200))
                        pair.append(tree)
                    first,second=pair
                    for field in signatures:
                        need(first[field]==second[field],'expZ changed discrete tree '+field)
                    for field in ('birth_lambda','death_lambda','point_exit_lambda'):
                        for a,b in zip(first[field],second[field]):
                            need(a**2==b,'expZ date power replay')
                            dates+=1
                            if a*a!=b:
                                multiply_differences+=1
                                max_ulp=max(max_ulp,abs(a*a-b)/math.ulp(b))
                    if previous is not None:
                        # Counts are requested; these birth sets also exhibit
                        # the stronger retained-subfamily relation in this capture.
                        current_sets={frozenset(x) for x in births[1:]}
                        need(current_sets<=previous,'higher minimum introduced a new condensed birth set')
                        subset_checks+=1
                    previous={frozenset(x) for x in births[1:]}
                    pairs+=1
                for z,row in counts.items():
                    need(all(a>=b for a,b in zip(row,row[1:])),'increasing size threshold increased condensed count')
                    sequences.append(dict(case=case['id'],k=k,method=method,exp_z=z,min_cluster_size=list(sizes),counts=row))
    bound_groups={}; exact_checks=0
    for row in receipt['rows']:
        m=row['min_cluster_size']
        diagnostics=['extra'] if row['method']=='hdbscan_standard' else ['extra','raw_size_filtered_recoverability','condensed_recoverability']
        case=cases[row['case']]
        expected_counts=dict(enumerate(case['true_counts'],1))
        for name in diagnostics:
            diagnostic=row[name]
            need(diagnostic['classes_below_min_cluster_size']==sum(s<m for s in expected_counts.values()),'eligibility count')
            for cls in diagnostic['per_class']:
                size=cls['size']; need(expected_counts[cls['truth_label']]==size,'truth class count')
                if size>=m:
                    continue
                mass=cls['matched_size']; overlap=cls['intersection']
                need(mass==0 or mass>=m,'reported cluster smaller than threshold')
                need(0<=overlap<=min(size,mass),'intersection cardinality')
                value=Fraction(2*overlap,size+mass)
                bound=Fraction(2*size,size+m)
                need(value<=bound and not cls['exact'] and cls['eligible'] is False,'below-threshold F1/exactness bound')
                if name!='extra':
                    need(value==Fraction(cls['best_f1_numerator'],cls['best_f1_denominator']),'rational F1 identity')
                key=(size,m,name)
                group=bound_groups.setdefault(key,dict(class_size=size,min_cluster_size=m,diagnostic=name,
                    bound_numerator=bound.numerator,bound_denominator=bound.denominator,checks=0,max_observed=Fraction(0)))
                group['checks']+=1; group['max_observed']=max(group['max_observed'],value);exact_checks+=1
    for group in bound_groups.values():
        value=group.pop('max_observed')
        group.update(max_observed_numerator=value.numerator,max_observed_denominator=value.denominator)
    need(pairs==768 and arrays==1536 and len(sequences)==384,'complete audit cardinalities')
    need(sha(receipt_path)==receipt_hash and sha(source)==source_hash,'capture/validator changed during audit')
    need(all(sha(path)==expected for path,expected in pins.items()),'cluster artifacts changed during audit')
    return dict(schema='mhgp9_gaussian_math_audit_v1',status='passed',capture=str(capture),receipt_sha256=receipt_hash,
        audit_source_sha256=sha(__file__),validator_sha256=source_hash,cluster_artifact_sha256=pins,
        exp_z_pairs=pairs,validated_condensed_trees=arrays,individual_point_exits=point_exits,
        selected_birth_sets_replayed=selected_birth_sets,lambda_power_values=dates,
        cluster_count_bound_checks=len(bound_slacks),cluster_count_bound='C <= max(1, 2*floor(n/m)-1)',
        cluster_count_bound_min_slack=min(bound_slacks),cluster_count_bound_max_slack=max(bound_slacks),
        disjoint_leaf_cluster_birth_masses_checked=leaf_mass_checks,storage_scope='O(n+C), n individual exits retained',
        pow_vs_multiplication_differences=multiply_differences,pow_vs_multiplication_max_ulp=max_ulp,
        threshold_sequences=len(sequences),threshold_retained_birth_subfamily_checks=subset_checks,
        threshold_counts=sequences,bound_exact_checks=exact_checks,bound_groups=list(bound_groups.values()),
        sampled_cut_trees=len(cut_results),sampled_cut_checks=sum(row['cuts'] for row in cut_results),sampled_cuts=cut_results,
        limitations=['This audit does not assert monotone EOM selection or labels when m changes.',
          'Birth-set F1 is a supervised representability diagnostic, not a live cut or joint selection.',
          'Binary64 lambda**2 replays exactly; lambda*lambda can differ by one ULP.',
          'The below-threshold bound is 2s/(s+m), independent of geometry or raw-tree quality.',
          'Not a geometric producer requalification, general statistical superiority claim, or GPU result.'],GCP_used=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path);parser.add_argument('report',type=Path)
    args=parser.parse_args()
    need(not args.report.exists(),'fresh distinct report required')
    result=audit(args.capture.resolve())
    args.report.write_text(json.dumps(result,sort_keys=True,indent=2,allow_nan=False)+'\n')
    print(json.dumps({key:value for key,value in result.items() if key not in
        ('cluster_artifact_sha256','threshold_counts','sampled_cuts','limitations')},sort_keys=True))
