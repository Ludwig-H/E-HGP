"""Independent timing inequalities and bounded schedule model; no product imports."""
import json
from itertools import product
checks=0

def need(value,message):
    global checks
    checks+=1
    if not value: raise ValueError(message)

wall_fields=('prefix_ns','count_ns','replay_ns','fill_ns','sort_ns','level_scan_ns','allocation_ns','assembly_ns')
fields=wall_fields+('count_task_sum_ns','count_task_max_ns','fill_task_sum_ns','fill_task_max_ns','sort_comparisons','tasks')

def declared_relations(event):
    # Mathematical restatement of pin's check_timings relations, not executing its function.
    t=event['timings']
    if set(t)!=set(fields) or not all(type(t[k]) is int and 0<=t[k]<2**64 for k in fields): return False
    if not 1<=t['tasks']<=256 or sum(t[k] for k in wall_fields)>event['wall_ns']: return False
    for phase in ('count','fill'):
        maximum,total,wall=(t[phase+s] for s in ('_task_max_ns','_task_sum_ns','_ns'))
        if not (maximum<=total<=t['tasks']*maximum and maximum<=wall): return False
    b=event['balls'];c=t['sort_comparisons']
    return c==0 if b<2 else 0<c<=4*b*(b-1).bit_length()

# Impossible aggregate accepted by those relations. No canonical artifact/parser claim.
t=dict.fromkeys(fields,0)
t.update(tasks=256,count_ns=100_000_000,count_task_max_ns=100_000_000,
         count_task_sum_ns=900_000_000,sort_comparisons=1)
event={'workers':8,'balls':2,'wall_ns':200_000_000,'timings':t}
need(declared_relations(event),'counterexample no longer matches pinned relations')
parallel_capacity=min(event['workers'],t['tasks'])*t['count_ns']
need(t['count_task_sum_ns']>parallel_capacity,'missing-concurrency-bound counterexample')

# Intervals of one worker cannot overlap; sum of their floored lengths is <= floored outer wall.
# Small exact interval schedules justify the added inequality at fine and coarse resolutions.
for workers,tasks,phase_wall in product((1,2,8,48),(1,2,8,48,256),(1,7,100)):
    active=min(workers,tasks)
    worker_sums=[0]*active
    lengths=[]
    for ordinal in range(tasks):
        worker=ordinal%active
        remaining=phase_wall-worker_sums[worker]
        length=remaining//2 if ordinal+active<tasks else remaining
        worker_sums[worker]+=length
        lengths.append(length)
    need(max(worker_sums)<=phase_wall,'per-worker intervals escaped outer phase')
    need(sum(lengths)<=active*phase_wall,'total interval capacity')
    need(max(lengths)<=phase_wall,'longest task escaped phase')
    need(sum(lengths)<=tasks*max(lengths),'existing max relation')

# Protocol inventory; no child process invoked. One W48 baseline failure removes W8 too.
cases=('lidar0','lidar1','lidar2','synthetic8','synthetic16','synthetic32')
requests=[]
for case in cases:
    for bits in (21,24): requests.append((case,bits,5,48,0))
for case in cases[:3]:
    for bits in (21,24):
        for repeat in (1,2): requests.append((case,bits,5,48,repeat))
for case in cases[:3]:
    for bits in (21,24): requests.append((case,bits,5,8,0))
for case in cases[:3]:
    for bits in (21,24): requests.append((case,bits,10,48,0))
need(len(requests)==36 and len(set(requests))==36,'protocol floor')
failed=('lidar0',21)
omitted=[r for r in requests if r[:2]==failed and r!=('lidar0',21,5,48,0)]
need(len(omitted)==4,'baseline causal omission inventory')
need(('lidar0',21,5,8,0) in omitted,'W8 diagnostic arm actually suppressed')
need(len([r for r in requests if r[3]==1])==0,'current campaign unexpectedly contains fresh W1')

# Valid diagnostic ratios are presence in measured execute_task windows, not CPU utilization.
phase_wall=100
measured_task_sum=320
workers=8
effective_concurrency=measured_task_sum/phase_wall
presence_fraction=measured_task_sum/(workers*phase_wall)
need(effective_concurrency==3.2 and presence_fraction==0.4,'dimensionless ratio')
serial_fields={'prefix_ns':10,'replay_ns':10,'sort_ns':20,'level_scan_ns':5,'allocation_ns':7,'assembly_ns':10}
serial_wall=sum(serial_fields.values())
api_wall=300
all_measured_wall=serial_wall+100+100
need(serial_wall==62 and api_wall-all_measured_wall==38,'serial/residual accounting')
need(serial_wall<=api_wall,'serial lower bound')

print(json.dumps({'status':'PASS','checks':checks,'native':False,'build':False,'gcp':False,'product_imports':False,
 'scope':'Independent exact interval/schedule model; no product function or native catalogue executed',
 'counterexample':{'event':event,'declared_relations_hold':True,'required_capacity_ns':parallel_capacity,
                    'actual_sum_ns':t['count_task_sum_ns'],'complete_artifact_parser_claim':False},
 'schedule':{'requested':36,'omitted_after_one_W48_failure':omitted,'fresh_W1':0},
 'metric_example':{'effective_concurrency':effective_concurrency,'presence_fraction':presence_fraction,
                   'cpu_utilization_claim':False,'serial_stage_wall':serial_wall,'residual_api_wall':38}},sort_keys=True))
