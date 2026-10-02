#!/usr/bin/env python3
"""Receipt-only independent checks; no subprocesses, builds, native execution or network."""
from pathlib import Path
import collections,hashlib,json,re,tarfile,xml.etree.ElementTree as ET
import run_reader
ROOT=Path(__file__).resolve().parent
R=run_reader.reader
FOLDER=run_reader.RECEIPTS/'index_20261002/index1'
def need(value,message):
    if not value:raise ValueError(message)
def sha(data):return hashlib.sha256(data).hexdigest()
def js(path):return json.loads(path.read_bytes())
def tar_data(path):
    seen=set();data={};total=0
    with tarfile.open(path) as t:
        for member in t:
            need(member.name not in seen and not Path(member.name).is_absolute() and '..' not in Path(member.name).parts,'tar path')
            need(member.isfile() or member.isdir(),'tar member type')
            seen.add(member.name);total+=1
            if member.isfile():data[member.name]=t.extractfile(member).read()
    return data,total

def review():
    receipt,worker,data=R.old.read_capture(FOLDER)
    need(receipt['commit']==R.SOURCE_COMMIT,'executed index source')
    raw=js(ROOT/'raw/index1_receipt.json')
    package,_=tar_data(ROOT/'raw/package/package.tar.gz')
    expected=js(ROOT/'package_git_manifest.json')
    need(expected['exact_file_bytes_match'] is True and expected['source_commit']==receipt['commit'],'Git package identity')
    need(sha((ROOT/'raw/package/package.tar.gz').read_bytes())==receipt['package_sha256']==expected['package_sha256'],'package hash')
    need(set(package)=={row['path'] for row in expected['files']},'package file names')
    for row in expected['files']:need(len(package[row['path']])==row['bytes'] and sha(package[row['path']])==row['sha256'],'package exact bytes')
    need(sha((ROOT/'raw/package/plan.json').read_bytes())==receipt['plan_sha256'],'plan JSON')
    need(sha((ROOT/'raw/package/plan.sh').read_bytes())==receipt['worker_plan_sha256']==worker['plan_sha256'],'worker plan')
    need(data['results/plan.sh']==(ROOT/'raw/package/plan.sh').read_bytes(),'archived plan')
    manifest=data['results/MANIFEST.sha256'].decode().splitlines();observed={}
    for line in manifest:
        digest,name=line.split('  ',1);name='results/'+name.removeprefix('./')
        need(name not in observed and name in data and sha(data[name])==digest,'results manifest hash')
        observed[name]=digest
    need(set(observed)==set(data)-{'results/MANIFEST.sha256'} and len(observed)==raw['results_manifest_files'],'manifest complete')
    data_manifest=(ROOT/'raw/package/data/SHA256SUMS').read_bytes()
    need(sha(data_manifest)==receipt['data_manifest_sha256'],'data SHA256SUMS')
    uploaded={row['name']:row for row in receipt['data_files']}
    for line in data_manifest.decode().splitlines():
        digest,name=line.split('  ',1);need(uploaded[name.removeprefix('./')]['sha256']==digest,'uploaded input hash')
    inputs,input_hash=R.old.inputs(FOLDER,receipt)
    counts,builds,matrix_code=R.judge_matrix(data)
    mapped={R.BASE+path[len(R.SUPPLEMENT):]:value for path,value in data.items() if path.startswith(R.SUPPLEMENT)}
    supplement,required,supplement_code=R.judge_supplement(mapped)
    need(matrix_code==supplement_code==0 and sum(c[0] for c in counts.values())==sum(c[1] for c in counts.values())==1149,'main matrix gates')
    need(supplement==(36,36,0,0),'supplement gates')
    selected=json.loads(mapped[R.asan.PREFIX+'tests.json'])
    domains=collections.Counter('style' if row['name'].startswith('mhgp11_style') else 'num' if row['name'].startswith('mhgp11_num_') else 'index' if row['name'].startswith('mhgp11_index_') else 'other' for row in selected)
    need(domains=={'num':15,'index':19,'style':2},'supplement attribution')
    mutroot=ET.fromstring(data[R.BASE+'mutants/junit.xml']);cases={case.get('name'):case for case in mutroot};mutants={}
    for module,count in [('core',78),('num',20),('cloud',16),('catalogue',9),('index',8)]:
        definition=json.loads(package['morsehgp3D_v11/tests/mutants/'+module+'.json'])
        expected_kills={item['id']:item.get('attendu','code') for item in definition['mutants']}
        name='mhgp11_mutants_'+module;case=cases[name]
        need(case.get('status')=='run' and case.find('failure') is None and case.find('skipped') is None,'mutant JUnit status')
        lines=R.full_test_output(data,R.BASE+'mutants/',name,case.findtext('system-out') or '')
        killed={}
        for line in lines:
            m=re.fullmatch(r'(\S+)\s+TUE\s+(code|ligne|construction)',line)
            if m:
                ident,cause=m.groups();need(ident not in killed,'duplicate mutant');killed[ident]=cause
        need(set(killed)==set(expected_kills) and len(killed)==count and all((killed[i]=='construction') is (cause=='construction') for i,cause in expected_kills.items()),'mutant exact identities/causes')
        causes=collections.Counter(killed.values())
        line='mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' % (module,count,count,causes['construction'],definition['plancher'])
        need(line in lines and 'run_expect_verdict conforme' in lines,'mutant summary')
        overrides=[{'id':item['id'],'options':item['options']} for item in definition['mutants'] if item.get('options')]
        mutants[module]={'detected':count,'causes':dict(causes),'declared_options':overrides}
    need(sum(value['detected'] for value in mutants.values())==131,'total mutants')
    report=js(FOLDER/'index.json')
    provenance={name:sha(data[R.BASE+name+'/build_provenance.json']) for name in R.profiles.PROFILES.values()}
    supplement_provenance=sha(data[R.SUPPLEMENT+R.asan.NAME+'/build_provenance.json'])
    verdict=R.judge_report(report,inputs,input_hash,sha(data[R.BASE+'summary.json']),builds,provenance,sha(data[R.SUPPLEMENT+'summary.json']),supplement_provenance)
    need(verdict=={'conforming':True,'attempted':18,'unplayed':0,'ok':18,'different':0},'complete benchmark')
    metas=[R.fields(data['results/cmd/'+name+'/meta.txt']) for name in R.COMMANDS]
    need(all(meta['group_closed']=='1' and meta['exit_code']=='0' and meta['residual_group_killed']=='0' and meta['streams_truncated']=='0' for meta in metas),'command groups closed')
    for first,second in zip(metas,metas[1:]):need(float(first['group_closed_epoch'])<=float(second['started_epoch']),'no command overlap')
    measurements=[];total_queries=0
    for row in report['runs']:
        points,index,total,end=row['events'][0],row['events'][1],row['events'][-2],row['events'][-1]
        need(total['queries']==64 and total['degenerate']==0,'actual query inventory')
        total_queries+=total['queries']
        queries=row['events'][2:-2]
        measurements.append({'case':row['case'],'sites':row['count'],'coord_bits':row['coord_bits'],'read_ns':points['read_ns'],'cloud_ns':points['cloud_ns'],'index_ns':index['wall_ns'],'factory_ns_sum':sum(event['factory_ns'] for event in queries),'census_ns_sum':total['query_ns'],'reference_scan_ns_sum':total['reference_ns'],'process_wall_seconds':row['process_wall_seconds'],'semantic_wall_seconds':row['semantic_wall_seconds'],'queries':64,'complete':total['complete'],'saturated':total['saturated'],'degenerate':total['degenerate'],'arities':total['arities'],'index_nodes':index['nodes'],'index_max_depth':index['max_depth'],'cloud_buffer_peak':points['peak_reserved_bytes'],'cloud_buffer_after':points['reserved_after_bytes'],'index_buffer_peak':index['peak_reserved_bytes'],'census_max_buffer_peak':max(event['peak_reserved_bytes'] for event in queries),'canonical_bytes':row['canonical_bytes'],'canonical_sha256':row['canonical_sha256'],'semantic_sha256':row['semantic']['sha256']})
    need(total_queries==1152,'aggregate query count')
    _,members=tar_data(FOLDER/'results.tar.gz')
    return {'verdict':'coherent_closed_index_campaign','executed_source_commit':receipt['commit'],'campaign_versioning_at_initial_capture':'untracked_on_developer_checkout','audit_native_executions':0,'audit_cloud_actions':0,'package':{key:expected[key] for key in ['package_bytes','package_sha256','file_count','expanded_file_bytes','exact_file_bytes_match']},'archive':{'members':members,'files':len(data),'verified_manifest_files':len(observed),'bytes':receipt['results_bytes'],'expanded_bytes':receipt['results_expanded_bytes'],'sha256':receipt['results_sha256']},'main_matrix_counts':{name:list(values) for name,values in counts.items()},'main_matrix_total':1149,'supplement_total':36,'supplement_domains':dict(domains),'required_num_index_gates':required,'mutants':mutants,'benchmark':verdict,'queries_total':total_queries,'comparisons':report['comparisons'],'commands':metas,'builds':report['builds'],'closure':{key:receipt[key] for key in ['generation','closing_generation','observed_after','targeted_shutdown_certified','private_key_deleted','oslogin_key_removed','reserve_released','status','worker_exit_code']},'measurements':measurements,'canonical_payloads_rehashed_by_audit':False,'oracle_scope':'qualification Gram/Fraction is independent arithmetic; large-input scan independent traversal reuses num::side','weighted_census_qualified':False,'scope':report['scope'],'timing_scope':report['timing_scope'],'memory_scope':report['memory_scope'],'reference_scope':report['reference_scope'],'query_scope':report['query_scope']}
if __name__=='__main__':print(json.dumps(review(),indent=2,sort_keys=True))
