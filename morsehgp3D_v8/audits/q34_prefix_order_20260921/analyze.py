#!/usr/bin/env python3
"""Summarize sampled order work without extrapolating to global runtime."""
import json
from statistics import median
from campaign import BASE, load, require, sha


def summarize(queries):
    work=[[0]*11 for _ in range(4)]
    for query in queries:
        for i,row in enumerate(query['work']):
            for j,value in enumerate(row):
                work[i][j]=max(work[i][j],value) if j==8 else work[i][j]+value
        if query['alive']:
            for i in (1,3):
                for j in (0,1,2,5,6,7):
                    require(query['work'][0][j]==query['work'][i][j],'unsaturated traversal changes geometry')
    reference=work[1][0]
    return dict(queries=len(queries),work=work,geom_vs_near=[row[0]/reference if reference else None for row in work])


def main():
    folder=BASE/'capture';manifest=load(folder/'MANIFEST.json');completion=load(folder/'COMPLETION.json')
    require(manifest['status']==completion['status']=='completed' and completion['manifest_sha256']==sha(folder/'MANIFEST.json'),'incomplete capture')
    cases=[];all_queries=[];oracle_tests=0;pivot_tests=0
    for entry in manifest['records']:
        path=folder/entry['path'];require(sha(path)==entry['sha256'],'changed record')
        record=load(path)
        if record.get('regime')!='lidar':continue
        require(record['returncode']==0 and not record['stderr'],'failed command')
        row=json.loads(record['stdout']);queries=row['queries'];all_queries+=queries
        oracle_tests+=row['oracle_point_tests'];pivot_tests+=row['pivot_preparation_distance_tests']
        strata=[]
        for stratum in (0,1):
            chosen=[q for q in queries if q['stratum']==stratum]
            strata.append(dict(all=summarize(chosen),rejected=summarize([q for q in chosen if not q['alive']]),surviving=summarize([q for q in chosen if q['alive']])))
        cases.append(dict(input=record['input'],n=row['n'],K=row['K'],population_rectangles=row['population_rectangles'],sampled_rectangles=row['sampled_rectangles'],pivot_preparation_distance_tests=row['pivot_preparation_distance_tests'],strata=strata))
    require(len(cases)==18,'missing LiDAR cases')
    summaries={}
    for stratum in (0,1):
        selected=[q for q in all_queries if q['stratum']==stratum]
        require(any(q['alive'] for q in selected) and any(not q['alive'] for q in selected),'vacuous sample')
        groups=dict(all=summarize(selected),rejected=summarize([q for q in selected if not q['alive']]),surviving=summarize([q for q in selected if q['alive']]))
        ratios=[c['strata'][stratum]['rejected']['geom_vs_near'] for c in cases]
        groups['case_rejected_geom_vs_near']={name:dict(min=min(r[i] for r in ratios),median=median(r[i] for r in ratios),max=max(r[i] for r in ratios)) for i,name in enumerate(['global','near','circular','pivotpath'])}
        summaries[str(stratum)]=groups
    require(sum(q['work'][2][3] for q in all_queries)>0,'no circular clipping')
    result=dict(status='passed',orders=['global','near','circular','pivotpath'],lidar_cases=18,queries=len(all_queries),oracle_point_tests=oracle_tests,pivot_preparation_distance_tests=pivot_tests,strata=summaries,cases=cases,scope='Unweighted sampled independent-lane searches; no A/B inherited port, full q34 pipeline time, runtime speedup or population estimator')
    print(json.dumps(result,indent=2,sort_keys=True))


if __name__=='__main__':main()
