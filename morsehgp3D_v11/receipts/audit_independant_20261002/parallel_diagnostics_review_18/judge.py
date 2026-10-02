"""Read-only receipt judge; no product import or removable assert."""
from pathlib import Path
import ast,hashlib,json
root=Path(__file__).resolve().parent
checks=0

def need(value,message):
    global checks
    checks+=1
    if not value: raise ValueError(message)

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

before=json.loads((root/'SOURCE_BEFORE.json').read_text())
extra=json.loads((root/'ADDITIONAL_PIN.json').read_text())
need(before['head_before']=='a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e','pin head')
need(extra['head']==before['head_before'],'additional fixed Git')
need(len(before['sources'])==37 and len(extra['sources'])==1,'source floors')
need(not any(before[k] for k in ('native','build','gcp')),'execution scope')
allrows=before['sources']+extra['sources']
for row in allrows:
    need(sha(root/'sources'/row['path'])==row['sha256'],'source hash '+row['path'])
for row in before['sources']:
    need(row['live_matches_head'] is True and row['sha256']==row['git_head_sha256'],'clean source pin')
need((root/'scalar.stdout.json').read_bytes()==(root/'scalar_opt.stdout.json').read_bytes(),'model normal/-O differ')
need(not (root/'scalar.stderr').read_bytes() and not (root/'scalar_opt.stderr').read_bytes(),'model stderr')
m=json.loads((root/'scalar.stdout.json').read_text())
need(m['status']=='PASS' and m['checks']>=240,'model floor')
need(not any(m[k] for k in ('native','build','gcp','product_imports')),'model scope')
w=m['counterexample']
need(w['declared_relations_hold'] and w['actual_sum_ns']>w['required_capacity_ns'],'counterexample')
need(w['complete_artifact_parser_claim'] is False,'counterexample scope')
need(m['schedule']['requested']==36 and m['schedule']['fresh_W1']==0,'schedule scope')
need(len(m['schedule']['omitted_after_one_W48_failure'])==4,'omission floor')
base=root/'sources/morsehgp3D_v11'
source=(base/'bench/catalogue_parallel.py').read_text()
module=ast.parse(source)
fn=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name=='check_timings')
constants=[n.value for n in ast.walk(fn) if isinstance(n,ast.Constant)]
need('workers' not in constants,'pinned missing concurrency guard has changed')
for rel,fragments in {
 'bench/catalogue_parallel.py':["total <= t['tasks'] * longest",'if key in failed_baselines:',"if row['status'] != 'ok':",'add(lidar, 5, 8, (0,))'],
 'bench/catalogue_profiles.py':["str(8 * 1024**3)","'16', '256', '0'",'timeout=timeout, check=False'],
 'bench/catalogue_probe.cpp':['std::clock()','budget.restart_peak()','if (pool) timings(measured);','MHGP11_TRY(serialize(argv[3]'],
 'src/catalogue/parallel.cpp':['std::optional<Stopwatch> task_clock;','if (timing) task_clock.emplace();','MHGP11_TRY(frontier.execute_task(ordinal, run));','expected.ledger = run.ledger','*timings = draft','std::min(pool.size(), frontier.size())'],
 'src/catalogue/assemble.cpp':['heap_sort(records.span()','stage->nanoseconds()','result.population_.val.allocate(population.size(), budget)'],
 'src/catalogue/boxes.cpp':['if (limit_ == 0) return {};'],
 'src/core/buffer.cpp':['account.used.compare_exchange_weak','account.peak.compare_exchange_weak'],
 'docs/CATALOGUE_PARALLELE.md':['Ces intervalles murs sont disjoints sans couvrir tout le temps API','Les sommes et maxima par tâche couvrent `execute_task` seul']
}.items():
    code=(base/rel).read_text()
    for fragment in fragments: need(fragment in code,'source binding '+rel+' '+fragment)
after=root/'SOURCE_AFTER.json'
if after.exists():
    value=json.loads(after.read_text())
    need(value['scope']=='LIVE after recorded separately; only frozen pin analyzed','after scope')
    need(len(value['sources'])==len(allrows),'after source count')
    lookup={r['path']:r for r in allrows}
    for row in value['sources']:
        need(row['before_sha256']==lookup[row['path']]['sha256'],'after pin identity')
        need(row['changed']==(row['before_sha256']!=row['live_sha256']),'drift classification')
manifest=root/'SHA256SUMS'
if manifest.exists():
    expected={}
    for line in manifest.read_text().splitlines():
        digest,rel=line.split('  ',1)
        need(rel!='SHA256SUMS' and rel not in expected,'manifest entry')
        expected[rel]=digest
    actual={str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=manifest}
    need(actual==set(expected),'root-only exclusion exhaustive')
    for rel,digest in expected.items(): need(sha(root/rel)==digest,'payload hash '+rel)
print(json.dumps({'status':'PASS','checks':checks,'sources':38,'scalar_checks':m['checks'],
 'scope':'Pin source review and independent interval/protocol model; no native/build/GCP'},sort_keys=True))
