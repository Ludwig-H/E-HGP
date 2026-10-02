"""Read-only receipt judge; explicit checks remain active under -O."""
from pathlib import Path
import hashlib,json,subprocess,sys
root=Path(__file__).resolve().parent
checks=0

def need(value,message):
    global checks
    checks+=1
    if not value: raise ValueError(message)

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

before=json.loads((root/'SOURCE_BEFORE.json').read_text())
selection=json.loads((root/'SELECTION.json').read_text())
rows={r['path']:r for r in before['sources']}
need(before['developer_head_before']=='7f1922c7743d8682e2665a491b01d32e8f2d546c','Git baseline')
need(len(selection['selected_sources'])==98,'selected source count')
need(set(selection['selected_sources']).isdisjoint(selection['excluded_initial_capture']),'selection overlaps')
need(set(selection['selected_sources'])|set(selection['excluded_initial_capture'])==set(rows),'capture selection coverage')
for rel in selection['selected_sources']:
    need(sha(root/'sources'/rel)==rows[rel]['sha256'],'snapshot hash '+rel)
for row in json.loads((root/'V10_REFERENCE.json').read_text())['sources']:
    need(sha(root/'v10_pin'/row['path'])==row['sha256'],'v10 pin hash')
need(json.loads((root/'V10_REFERENCE.json').read_text())['head']=='865f5e64ddd08bedf6ab8f94e8bb94812e380e79','v10 head')
need(not any(before[k] for k in ('native','build','gcp')),'execution scope')
for a,b in [('scalar.stdout.json','scalar_opt.stdout.json'),('scalar.stderr','scalar_opt.stderr')]:
    need((root/a).read_bytes()==(root/b).read_bytes(),'normal/-O '+a)
need(not (root/'scalar.stderr').read_bytes(),'model stderr')
report=json.loads((root/'scalar.stdout.json').read_text())
need(report['status']=='PASS' and report['checks']==5173,'bounded model floor')
need(not any(report[k] for k in ('native','build','gcp')),'model scope')
need(report['nmax_U0']==8073246 and report['nmax_unit_cloud_same_budget']==7866240,'8 GiB thresholds')
need(report['root_filter_tests_two_passes_39885_K5']==1196550,'root work count')
for row in report['capacity']:
    need(row['frontier_pre_admission_bytes']==1064*row['n'],'first admission')
    need(row['unit_cloud_retained_bytes']==28*row['n']+8,'Cloud bytes')
    need(row['same_budget_first_admission_bytes']==1092*row['n']+8,'combined admission')
need('assert ' not in (root/'scalar_model.py').read_text(),'model uses removable assert')
base=root/'sources/morsehgp3D_v11'
for rel,fragments in {
    'src/catalogue/frontier.hpp':['kFrontierDepth = 8','kFrontierTasks = u32{1} << kFrontierDepth'],
    'src/catalogue/frontier.cpp':['cut_depth > kFrontierDepth','cloud_ == &run.cloud','!std::equal','run.ledger == ledger_','largest[j]','kMaxDepth - node.depth'],
    'src/catalogue/parallel.cpp':['frontier_memory_bound(cloud.sites(), kFrontierDepth, bytes)','std::min(pool.size(), frontier.size())','expected.population_begin','checked_add(record.population_begin, expected.population_begin)','std::max(replay_bytes, suffix_bytes)','NodeQuota second_quota(params.max_nodes)','Assembly::finish(records, population, params, ledger, budget)'],
    'src/catalogue/catalogue.hpp':['budget, sched::Pool& pool','Le pilote de budget est unique.'],
    'tests/catalogue/parallel.cpp':['frontier_overlap','CHECK_EQ(logical, 6u)','CHECK_EQ(capacity, 10u)','CHECK_EQ(work.peak(), 100u)','max_nodes = allowed ? nodes : nodes - 1','ball_limit = balls + (allowed ? 1 : 0)'],
    'tests/catalogue/parallel_fault.cpp':['ledger().max_depth > 8','CHECK_EQ(refused, count)','CHECK_EQ(clean, count)','CHECK_EQ(triggered, count)']
}.items():
    code=(base/rel).read_text()
    for fragment in fragments: need(fragment in code,'source binding '+rel+' '+fragment)
after=root/'SOURCE_AFTER.json'
if after.exists():
    data=json.loads(after.read_text())
    need(data['scope']=='LIVE drift logged, not source replacement or qualification','after scope')
    for row in data['sources']:
        need(row['before_sha256']==rows[row['path']]['sha256'],'before/after identity')
        need(row['changed']==(row['before_sha256']!=row['live_sha256']),'source drift classification')
pool=json.loads((root/'POOL_COPY.json').read_text())
need(pool['identical'] is True,'root pool copy status')
need(sha(root/'pool_review/SHA256SUMS')==pool['root_manifest_sha256'],'pool root manifest')
for row in pool['all_files_including_root_manifest']:
    need(sha(root/'pool_review'/row['path'])==row['sha256'],'pool exact copy '+row['path'])
manifest=root/'SHA256SUMS'
if manifest.exists():
    expected={}
    for line in manifest.read_text().splitlines():
        digest,rel=line.split('  ',1)
        need(rel not in expected and rel!='SHA256SUMS','root manifest entry')
        expected[rel]=digest
    actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=manifest}
    need(actual==set(expected),'manifest exhaustive/root-only exclusion')
    for rel,digest in expected.items(): need(sha(root/rel)==digest,'payload hash '+rel)
print(json.dumps({'status':'PASS','checks':checks,'sources':98,'scalar_checks':report['checks'],
                  'scope':'WIP source review and standalone scalar model; native/build/GCP/FULL not executed'},sort_keys=True))
