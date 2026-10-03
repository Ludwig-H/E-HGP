#!/usr/bin/env python3
"""Read-only derivation from the captured paired JSON; no product execution."""
import argparse,hashlib,json,statistics,sys,tarfile
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--archive',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args()
ARCHIVE=args.archive
MEMBER='results/cmd/002_paired_full/files/full_paired.json'
OUT=args.out

def need(condition,message):
    if not condition: raise ValueError(message)

def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1<<20),b''): h.update(block)
    return h.hexdigest()

def stats(values):
    return {'median':statistics.median(values),'min':min(values),'max':max(values)}

def work_value(order,key,variant):
    work=order['work']
    if key=='population_hits' and variant=='baseline' and key not in work:
        return 0  # pinned historical producer895 has no population table/field
    need(key in work, 'mandatory work field '+key+' for '+variant)
    return work[key]

def event(row,name):
    found=[e for e in row['events'] if e['phase']==name]
    need(len(found)==1,'unique event '+name)
    return found[0]

need(digest(ARCHIVE)=='a78816ef714a4d8f54fba1c1a1946a11d31e85e1c0e628a242cb0e416b5fc606','captured paired archive pin')
with tarfile.open(ARCHIVE) as archive:
    payload=archive.extractfile(MEMBER).read()
data=json.loads(payload)
rows=data['runs']
need(data['complete'] is True and data['conforming'] is True,'closed successful native report')
need(data['source']=='commit:c40f40798375a0fc37917499401f16876cccbd2a','current source')
need(data['requested_runs']==81 and len(rows)==81 and not data['not_run'],'81 completed rows')
cases=sorted({r['case'] for r in rows})
need(cases==['lidar_ng00','lidar_ng01','lidar_ng02'],'three intended cases')
variants={'baseline':2047,'current2047':2047,'current16379':16379}
expected={(c,v,w,i) for c in cases for v in variants for w in (1,8,48) for i in range(3)}
actual={(r['case'],r['build_variant'],r['workers'],r['repetition']) for r in rows}
need(len(actual)==81 and actual==expected,'complete unique Cartesian calendar')
ledger_orders=0
for r in rows:
    f=event(r,'full')
    need(r['status']=='ok' and r['exit_code']==0 and not r['errors'],'successful process/collector')
    need(r['coord_bits']==21 and r['kmax']==5 and r['whole_input'] is True,'same CPU/u21/K5 whole inputs')
    need(r['optimizations']==variants[r['build_variant']],'variant option mask')
    need(r['build_pin']==r['build_pin_after'],'unchanged binary/build pin')
    need(r['artifact_comparison']['raw_sha256']==r['semantic']['raw_sha256'],'reported artifact/semantic hash agrees')
    for k in ('domain','forest','index'):
        need(abs(r['stage_ms'][k]-f[k+'_ns']/1e6)<1e-7,'top-stage unit agreement')
    need(f['wall_ns']>=sum(f[k+'_ns'] for k in ('domain','forest','index')),'disjoint top-stage accounting')
    need(abs(r['cloud_pool_full_ms']-(r['cloud_ms']+r['pool_ms']+r['full_ms']))<1e-6,'outer native stages agreement')
    for o in f['orders']:
        q=o['work']
        need(q['census_calls']+q['catalogue_hits']+q['singleton_hits']==q['descent_steps'],'per-order step ledger')
        need(work_value(o,'population_hits',r['build_variant'])<=q['catalogue_hits']+q['singleton_hits'],'per-order population ledger')
        ledger_orders+=1
    c=event(r,'domain')['catalogue_work']
    need(c['region_line_tests']==c['region_line_evaluations']+c['region_line_cache_hits'],'catalogue line ledger')
    if r['build_variant']=='current16379':
        need(f['concurrent_orders'] is True and f['population_lookup'] is True and f['memo_capacity']==0,'bundle semantics')
        need(sum(f['phases'].values())<=f['forest_ns'],'global forest phases disjoint')
        need(all(v>=0 for v in f['phases'].values()),'nonnegative global phases')
for case in cases:
    R=[r for r in rows if r['case']==case]
    need(len({(r['semantic']['raw_sha256'],r['semantic']['sha256']) for r in R})==1,'reported whole outputs equal per case')
    need(len({(r['semantic_reuse']['context']['xyz_sha256'],r['semantic_reuse']['context']['ids_sha256']) for r in R})==1,'same input hashes per case')
need(len(data['comparisons'])==3 and all(c['status']=='equal' and c['semantic_and_bytes_equal'] is True for c in data['comparisons']),'three complete comparison verdicts')

work_keys=['descent_steps','census_calls','census_point_tests','part_meb_presentations','part_diameter_pairs','population_hits','memo_hits','memo_suffix_hits','catalogue_hits','singleton_hits','unions','plateaus','traces','replayed_cells','vertical_descents','vertical_reuses']
groups=[]
for case in cases:
    for variant in variants:
        for workers in (1,8,48):
            R=sorted((r for r in rows if (r['case'],r['build_variant'],r['workers'])==(case,variant,workers)),key=lambda r:r['repetition'])
            F=[event(r,'full') for r in R]
            g={'case':case,'variant':variant,'workers':workers,'repetitions':3,
               'full_ms':stats([r['full_ms'] for r in R]),
               'stage_ms':{k:stats([r['stage_ms'][k] for r in R]) for k in ('index','domain','forest')},
               'cpu_seconds':stats([f['cpu_seconds'] for f in F]),
               'effective_cpu_parallelism':stats([f['cpu_seconds']/(r['full_ms']/1000) for r,f in zip(R,F)]),
               'whole_peak_reserved_bytes':stats([r['whole_peak_reserved_bytes'] for r in R]),
               'reserved_after_bytes':stats([f['reserved_after_bytes'] for f in F]),
               'process_wall_ms':stats([1000*r['process_wall_seconds'] for r in R]),
               'semantic_wall_ms':stats([1000*r['semantic_wall_seconds'] for r in R])}
            if workers==48:
                g['work']={k:stats([sum(work_value(o,k,variant) for o in f['orders']) for f in F]) for k in work_keys}
                if variant=='current16379':
                    g['global_phase_ms']={k[:-3]:stats([f['phases'][k]/1e6 for f in F]) for k in F[0]['phases']}
                    g['forest_uninstrumented_ms']=stats([(f['forest_ns']-sum(f['phases'].values()))/1e6 for f in F])
                    g['catalogue_stage_ms']={k:stats([r['catalogue_stage_ms'][k] for r in R]) for k in R[0]['catalogue_stage_ms']}
                    g['catalogue_work']={k:stats([event(r,'domain')['catalogue_work'][k] for r in R]) for k in event(R[0],'domain')['catalogue_work']}
                    g['outer_native_ms']={k:stats([r[k] for r in R]) for k in ('read_ms','cloud_ms','pool_ms','cloud_pool_full_ms')}
                    g['process_minus_native_full_ms']=stats([1000*r['process_wall_seconds']-r['full_ms'] for r in R])
                    g['memory_posts_bytes']={k:F[0][k] for k in ('lookup_reserved_bytes','memo_reserved_bytes','census_workspace_reserved_bytes','population_lookup_reserved_bytes','regular_vertical_reserved_bytes')}
                    g['part_meb_presentations_by_k']={str(o['k']):o['work']['part_meb_presentations'] for o in F[0]['orders']}
            groups.append(g)
lookup={(g['case'],g['variant'],g['workers']):g for g in groups}
paired=[]
for case in cases:
    b=lookup[case,'baseline',48]; c=lookup[case,'current2047',48]; f=lookup[case,'current16379',48]
    paired.append({'case':case,'current16379_reduction_vs_baseline_percent':100*(1-f['full_ms']['median']/b['full_ms']['median']),
                   'current2047_reduction_vs_baseline_percent':100*(1-c['full_ms']['median']/b['full_ms']['median']),
                   'current16379_reduction_vs_current2047_percent':100*(1-f['full_ms']['median']/c['full_ms']['median']),
                   'current16379_additional_peak_vs_current2047_bytes':f['whole_peak_reserved_bytes']['median']-c['whole_peak_reserved_bytes']['median'],
                   'current16379_meb_presentation_reduction_vs_current2047_percent':100*(1-f['work']['part_meb_presentations']['median']/c['work']['part_meb_presentations']['median'])})
result={'schema':'ehgp.v11.pairedc40_readonly_derived.v1',
        'evidence':{'archive':str(ARCHIVE),'archive_sha256':digest(ARCHIVE),'member':MEMBER,'member_sha256':hashlib.sha256(payload).hexdigest(),'source':data['source'],'package_sha256':data['package_sha256']},
        'verification':{'rows':81,'order_step_ledgers':ledger_orders,'catalogue_line_ledgers':81,'unchanged_build_pins':81,'reported_input_and_output_hash_groups_equal':3,'native_builds_or_runs':0,'cloud_actions':0,'scope':'JSON arithmetic and reported hashes only; disposed native output files not independently rehashed'},
        'timing_scope':{'full':'native index+domain+forest','process':'input IO/Cloud/Pool/FULL/dump/destruction, excludes offline semantic inspection','campaign':'530.322270337 seconds, includes builds/control/Python','os_cache':'not dropped','phase_medians':'independent medians; do not sum to reconstruct one run'},
        'baseline_compatibility':'population_hits absent from pinned historical895 producer; explicitly treated as zero because this path has no population table',
        'semantic_reuse_counts':{mode:sum(r['semantic_reuse']['mode']==mode for r in rows) for mode in sorted({r['semantic_reuse']['mode'] for r in rows})},
        'groups':groups,'w48_comparisons':paired,
        'case_output_identity':[{'case':case,'count':next(r['count'] for r in rows if r['case']==case),'output_bytes':next(r['semantic']['bytes'] for r in rows if r['case']==case),'raw_sha256':next(r['semantic']['raw_sha256'] for r in rows if r['case']==case),'semantic_sha256':next(r['semantic']['sha256'] for r in rows if r['case']==case)} for case in cases],
        'limitations':['CPU/u21/K1..5/unit weights/three nonground whole frames from one sequence, 1mm','no 200ms or 100ms success in the 81 runs','16379 bundles graph+population lookup+concurrent orders and removes memo; no isolated attribution','Buffer+Cloud reservations are not RSS','archive closure/controller VM stop verified separately by root; no claim in this derivation']}
OUT.write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'),sort_keys=True)+'\n')
print('PASS rows=81 ledgers=%d equal_groups=3 output=%s sha256=%s'%(ledger_orders,OUT,digest(OUT)))
