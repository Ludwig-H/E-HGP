#!/usr/bin/env python3
"""Autonomous receipt-only checks: no native execution, build, network or cloud."""
from pathlib import Path
import collections,hashlib,json,re,tarfile,xml.etree.ElementTree as ET
import run_reader
ROOT=Path(__file__).resolve().parent
R=run_reader.reader
FOLDER=run_reader.RECEIPTS/'catalogue_q4_20261002/q4levels1'
def need(value,message):
    if not value:raise ValueError(message)
def sha(value):return hashlib.sha256(value).hexdigest()
def js(path):return json.loads(path.read_bytes())
def tar_data(path):
    seen=set();data={};members=0
    with tarfile.open(path) as archive:
        for m in archive:
            need(m.name not in seen and not Path(m.name).is_absolute() and '..' not in Path(m.name).parts,'unsafe/duplicate tar member')
            need(m.isfile() or m.isdir(),'special tar member')
            seen.add(m.name);members+=1
            if m.isfile():data[m.name]=archive.extractfile(m).read()
    return data,members

def review():
    receipt,worker,data=R.old.read_capture(FOLDER)
    package,total_package_members=tar_data(ROOT/'raw/package/package.tar.gz')
    expected=js(ROOT/'package_git_manifest.json')
    need(expected['exact_file_bytes_match'] is True and expected['source_commit']==receipt['commit'],'Git package pin')
    need(sha((ROOT/'raw/package/package.tar.gz').read_bytes())==receipt['package_sha256']==expected['package_sha256'],'package hash')
    need(len(package)==expected['file_count'],'package file inventory')
    need(set(package)=={x['path'] for x in expected['files']},'package exact names')
    for row in expected['files']:need(len(package[row['path']])==row['bytes'] and sha(package[row['path']])==row['sha256'],'Git package exact bytes')
    need(sha((ROOT/'raw/package/plan.json').read_bytes())==receipt['plan_sha256'],'plan JSON hash')
    need(sha((ROOT/'raw/package/plan.sh').read_bytes())==receipt['worker_plan_sha256']==worker['plan_sha256'],'worker plan hash')
    need(data['results/plan.sh']==(ROOT/'raw/package/plan.sh').read_bytes(),'archived plan bytes')
    raw=js(ROOT/'raw/q4levels1_receipt.json')
    manifest=data['results/MANIFEST.sha256'].decode().splitlines();observed={}
    for line in manifest:
        digest,name=line.split('  ',1);name='results/'+name.removeprefix('./')
        need(name not in observed and name in data and sha(data[name])==digest,'result manifest hash')
        observed[name]=digest
    need(set(observed)==set(data)-{'results/MANIFEST.sha256'} and len(observed)==raw['results_manifest_files'],'manifest completeness')
    uploaded={row['name']:(row['sha256'],row['size']) for row in receipt['data_files']}
    data_lines=(ROOT/'raw/package/data/SHA256SUMS').read_text().splitlines()
    need(sha((ROOT/'raw/package/data/SHA256SUMS').read_bytes())==receipt['data_manifest_sha256'],'data SHA256SUMS hash')
    for line in data_lines:
        digest,name=line.split('  ',1);need(uploaded[name.removeprefix('./')][0]==digest,'data manifest upload pin')
    counts,builds,matrix_code=R.judge_matrix(data)
    mapped={R.BASE+key[len(R.SUPPLEMENT):]:value for key,value in data.items() if key.startswith(R.SUPPLEMENT)}
    supplement,special,supplement_code=R.judge_supplement(mapped)
    need(matrix_code==supplement_code==0 and special==4,'native qualification')
    matrix_total=sum(c[0] for c in counts.values());matrix_passed=sum(c[1] for c in counts.values())
    need(matrix_total==matrix_passed==1014 and supplement[:2]==(14,14),'gate arithmetic')
    supplement_selected=json.loads(mapped[R.asan.PREFIX+'tests.json'])
    names=[x['name'] for x in supplement_selected]
    need(sum(name.startswith('mhgp11_num_') for name in names)==12 and set(name for name in names if not name.startswith('mhgp11_num_'))=={'mhgp11_style','mhgp11_style_opt'},'supplement num/style attribution')
    mutroot=ET.fromstring(data[R.BASE+'mutants/junit.xml']);mutants={}
    for module,count in [('core',78),('num',16),('cloud',16),('catalogue',9)]:
        definition=json.loads(package['morsehgp3D_v11/tests/mutants/'+module+'.json'])
        expected_causes={m['id']:m.get('attendu','code') for m in definition['mutants']}
        cases=[c for c in mutroot if c.get('name')=='mhgp11_mutants_'+module]
        need(len(cases)==1 and cases[0].get('status')=='run' and cases[0].find('failure') is None and cases[0].find('skipped') is None,'mutant JUnit gate')
        lines=(cases[0].findtext('system-out') or '').splitlines()
        kills={};causes=collections.Counter()
        for line in lines:
            match=re.fullmatch(r'(\S+)\s+TUE\s+(code|construction)',line)
            if match:
                ident,cause=match.groups();need(ident not in kills,'duplicate mutant verdict');kills[ident]=cause;causes[cause]+=1
        need(kills==expected_causes and len(kills)==count,'manifest/verdict causal inventory')
        expected_summary='mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=%d plancher=%d' % (module,count,count,causes['construction'],definition['plancher'])
        need(expected_summary in lines and 'run_expect_verdict conforme' in lines,'mutant summary')
        mutants[module]={'detected':len(kills),'causes':dict(causes)}
    need(sum(m['detected'] for m in mutants.values())==119,'mutant total')
    profile=js(FOLDER/'profiles.json');baseline=js(run_reader.RECEIPTS/'catalogue_profiles_20261002/profiles1/profiles.json')
    comparison=R.compare_baseline(profile,baseline)
    need(comparison['complete'] and len(comparison['matched'])==15 and not comparison['missing'],'15 baseline matches')
    rows=profile['runs'];status=collections.Counter(row['status'] for row in rows)
    need(status=={'ok':15,'timeout':18} and len(profile['not_run'])==3 and profile['conforming'] is False and profile['complete'] is True,'failed benchmark preserved')
    need(all(r['kmax']==5 for r in rows if r['status']=='ok'),'K5-only success')
    metas=[R.fields(data['results/cmd/'+name+'/meta.txt']) for name in R.COMMANDS]
    need([m['exit_code'] for m in metas]==['0','0','1'] and all(m['group_closed']=='1' and m['residual_group_killed']=='0' and m['streams_truncated']=='0' for m in metas),'closed command groups')
    for first,second in zip(metas,metas[1:]):need(float(first['group_closed_epoch'])<=float(second['started_epoch']),'commands overlap')
    measurements=[];oldrows={R.profiles.unit(row):row for row in baseline['runs'] if row['status']=='ok'}
    for row in rows:
        if row['status']!='ok':continue
        event=row['events'][1];before=oldrows[R.profiles.unit(row)]
        q4=R.q4_signature(row)
        measurements.append({'case':row['case'],'sites':row['count'],'coord_bits':row['coord_bits'],'kmax':row['kmax'],'generation_passes':event['generation_passes'],'api_catalogue_ns':event['wall_ns'],'baseline_api_catalogue_ns':before['events'][1]['wall_ns'],'baseline_over_current_ratio':before['events'][1]['wall_ns']/event['wall_ns'],'process_wall_seconds':row['process_wall_seconds'],'semantic_wall_seconds':row['semantic_wall_seconds'],'canonical_sha256':row['canonical_sha256'],'canonical_bytes':row['canonical_bytes'],'balls':event['balls'],'levels':event['levels'],'incidences':event['incidences'],'buffer_peak_bytes':event['peak_reserved_bytes'],'q4_candidates_per_pass':q4[0],'q4_levels_per_pass':q4[1],'q4_levels_avoided_fraction':1-q4[1]/q4[0]})
    _,archive_members=tar_data(FOLDER/'results.tar.gz')
    return {'verdict':'coherent_closed_evidence_failed_benchmark','source_commit':receipt['commit'],'native_audit_executions':0,'cloud_audit_actions':0,'package_git_file_count':len(package),'package_git_expanded_bytes':expected['expanded_file_bytes'],'package_bytes':expected['package_bytes'],'package_sha256':expected['package_sha256'],'archive_members':archive_members,'archive_files':len(data),'manifest_files_verified':len(observed),'archive_bytes':receipt['results_bytes'],'archive_expanded_bytes':receipt['results_expanded_bytes'],'archive_sha256':receipt['results_sha256'],'gate_counts':{name:list(value) for name,value in counts.items()},'main_matrix_passed':matrix_passed,'main_matrix_selected':matrix_total,'supplement_passed':supplement[1],'supplement_selected':supplement[0],'supplement_num_gates':12,'supplement_style_gates':2,'supplement_special_q4_gates':special,'mutants':mutants,'baseline_recorded_hashes_work_memory_matched':len(comparison['matched']),'baseline_canonical_payloads_rehashed':False,'attempt_statuses':dict(status),'not_run':profile['not_run'],'comparisons':profile['comparisons'],'q4_comparisons':profile['q4_comparisons'],'commands':metas,'closure':{'generation':receipt['generation'],'closing_generation':receipt['closing_generation'],'observed_after':receipt['observed_after'],'targeted_shutdown_certified':receipt['targeted_shutdown_certified'],'private_key_deleted':receipt['private_key_deleted'],'oslogin_key_removed':receipt['oslogin_key_removed'],'reserve_released':receipt['reserve_released'],'worker_exit_code':receipt['worker_exit_code'],'status':receipt['status']},'measurements':measurements,'timing_scope':profile['timing_scope'],'memory_scope':profile['memory_scope'],'q4_work_scope':profile['q4_work_scope'],'scope':'CPU exact catalogue on common u18 integer input across B18/B21/B24; no FULL/GPU; one repetition'}
if __name__=='__main__':print(json.dumps(review(),indent=2,sort_keys=True))
