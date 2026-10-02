#!/usr/bin/env python3
"""Portable closed-receipt parsing only: no native, product test, build, git or cloud execution."""
from pathlib import Path, PurePosixPath
import collections, hashlib, importlib.util, json, re, tarfile
ROOT=Path(__file__).resolve().parent
READERS=ROOT/'developer_copies/morsehgp3D_v11/receipts'
FOLDER=READERS/'catalogue_profiles_20261002/profiles1'
spec=importlib.util.spec_from_file_location('profile_reader',READERS/'catalogue_profiles_20261002/check.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
need,js,sha,fields=reader.need,reader.js,reader.sha,reader.fields
BASE=reader.BASE

def review():
    receipt=js((FOLDER/'receipt.json').read_bytes())
    raw_bytes=(ROOT/'original_local/receipt.json').read_bytes();raw=js(raw_bytes)
    need(sha(raw_bytes)==receipt['original_receipt_sha256'],'original receipt hash')
    need(all(json.dumps(v,sort_keys=True)==json.dumps(raw[k],sort_keys=True) for k,v in receipt.items() if k in raw),'compact/raw mismatch')
    need(receipt['commit']=='9df77494732b03ddf11dbcf1dcb11d96bef54a3b' and receipt['source_kind']=='commit','source pin')
    need(receipt['closure']=='stopped' and receipt['targeted_shutdown_certified'] is True and receipt['stop_exit_code']==0 and not receipt['errors'],'closure')
    need(receipt['generation']==receipt['closing_generation']==receipt['observed_after']['lastStartTimestamp'] and receipt['observed_after']['status']=='TERMINATED','generation')
    need(all(receipt[k] is True for k in ['private_key_deleted','oslogin_key_removed','reserve_released','results_verified']),'cleanup')
    need(raw['guest_guard_intact'] is True and raw['start_certified'] is True,'start/guest guard')
    archive=FOLDER/'results.tar.gz';need(archive.stat().st_size==receipt['results_bytes'] and sha(archive.read_bytes())==receipt['results_sha256'],'archive identity')
    data={};seen=set();expanded=0
    with tarfile.open(archive) as stream:
        for member in stream:
            path=PurePosixPath(member.name)
            need(member.name not in seen and not path.is_absolute() and '..' not in path.parts and (member.isfile() or member.isdir()),'tar member')
            seen.add(member.name);expanded+=member.size
            need(0<=member.size<=64*1024**2 and expanded<=64*1024**2,'tar bounds')
            if member.isfile():data[member.name]=stream.extractfile(member).read()
    need(expanded==receipt['results_expanded_bytes'] and not receipt['results_skipped_members'],'archive bounds/omissions')
    manifest=data['results/MANIFEST.sha256'];mapped=[]
    for line in manifest.decode().splitlines():
        digest,name=line.split('  ',1);path='results/'+name.removeprefix('./')
        need(re.fullmatch('[0-9a-f]{64}',digest) and path in data and sha(data[path])==digest,'archive manifest hash')
        mapped.append(path)
    need(len(mapped)==len(set(mapped))==raw['results_manifest_files']==106,'manifest inventory')
    need(set(mapped)==set(data)-{'results/MANIFEST.sha256'},'unmanifested archive file')
    worker=fields(data['results/worker.txt'])
    need(worker['source']=='commit:'+receipt['commit'] and worker['package_sha256']==receipt['package_sha256'] and worker['generation']==receipt['generation'],'worker identity')
    need(worker['interrupted']=='0' and worker['overflow_unresolved']=='0' and worker['truncated_streams']=='0','worker loss')
    plan_json=(ROOT/'original_local/package/plan.json').read_bytes();plan_sh=(ROOT/'original_local/package/plan.sh').read_bytes()
    need(sha(plan_json)==receipt['plan_sha256'] and sha(plan_sh)==receipt['worker_plan_sha256'] and data['results/plan.sh']==plan_sh,'plan identity')
    need(js(plan_json)==js((ROOT/'sources/morsehgp3D_v11/bench/plans/catalogue_profiles_g4.json').read_bytes()),'published plan')
    package=js((ROOT/'source_package_verified.json').read_bytes())
    need(package['byte_identical_to_git'] is True and package['matches_raw_receipt'] is True and package['sha256']==receipt['package_sha256']==package['git_archive_sha256'],'package comparison')
    summary,counts,builds,matrix_code=reader.old.matrix(data,FOLDER)
    need(matrix_code==0 and sum(c[0] for c in counts.values())==sum(c[1] for c in counts.values())==1002,'matrix counts')
    for name,records in builds.items():
        if records:
            bits=[line.split('=',1)[1] for line in records['CMakeCache.txt']['text'].splitlines() if line.startswith('MHGP11_COORD_BITS:')]
            need(bits==[str(reader.CONFIG_BITS[name])],'compiled bits')
    manifest_inputs,input_sha=reader.old.inputs(FOLDER,receipt)
    report_bytes=data['results/cmd/001_profiles/files/profiles.json']
    need(report_bytes==(FOLDER/'profiles.json').read_bytes(),'report original bytes')
    report=js(report_bytes)
    provenance={name:sha(data[BASE+name+'/build_provenance.json']) for name in reader.PROFILES.values()}
    verdict=reader.judge_report(report,manifest_inputs,input_sha,sha(data[BASE+'summary.json']),builds,provenance)
    need(verdict['complete'] is True and verdict['conforming'] is False and verdict['attempted']==33 and verdict['unplayed']==3 and verdict['equal']==5 and verdict['different']==0,'profile verdict')
    statuses=collections.Counter(x['status'] for x in report['runs']);need(statuses=={'ok':15,'timeout':18},'attempt statuses')
    metas=[fields(data['results/cmd/'+name+'/meta.txt']) for name in ['000_matrice','001_profiles']]
    need(metas[0]['status']=='ok' and metas[0]['exit_code']=='0' and metas[1]['status']=='failed' and metas[1]['exit_code']=='1','command verdicts')
    need(all(m['group_closed']=='1' and m['residual_group_killed']=='0' and m['streams_truncated']=='0' for m in metas),'command groups')
    need(float(metas[1]['started_epoch'])>float(metas[0]['group_closed_epoch']),'group handoff')
    need(worker['commands_total']=='2' and worker['commands_ok']=='1' and worker['status']=='failed' and receipt['status']=='failed_remote' and receipt['worker_exit_code']==1,'global verdict')
    mutant_log=data[BASE+'mutants/LastTest.log'].decode()
    mutant_hits=re.findall(r'^([a-z0-9_]+)\s+(TUE|INVALIDE|SURVIVANT)\s+(\w+)\s*$',mutant_log,re.M)
    mutation_causes=collections.Counter(verdict+'/'+cause for _,verdict,cause in mutant_hits)
    need(len(mutant_hits)==len({x[0] for x in mutant_hits})==116 and mutation_causes=={'TUE/code':111,'TUE/ligne':3,'TUE/construction':2},'mutant exact verdict inventory')
    summaries=[line for line in mutant_log.splitlines() if line.startswith('mutants_ok ')]
    need(len(summaries)==4 and all('dont_signal=0 dont_delai=0' in line for line in summaries),'mutant summaries')
    num_manifest=js((ROOT/'sources/morsehgp3D_v11/tests/mutants/num.json').read_bytes())
    high_mutants=[{'id':x['id'],'gate':x['porte'],'options':x.get('options',[])} for x in num_manifest['mutants'] if x.get('options')]
    need(len(high_mutants)==4,'explicit high-bit mutant profiles')
    prior=js((ROOT/'comparison/catalogue3_e6fe34cb0_leaf16.json').read_bytes())
    need(prior['manifest']==report['manifest'] and prior['manifest_sha256']==report['manifest_sha256'],'same old/new inputs')
    comparisons=[];table=[]
    for row in report['runs']:
        if row['status']!='ok':continue
        event=row['events'][1]
        table.append(dict(case=row['case'],sites=row['count'],bits=row['coord_bits'],kmax=row['kmax'],api_ns=event['wall_ns'],process_seconds=row['process_wall_seconds'],decode_seconds=row['semantic_wall_seconds'],balls=event['balls'],levels=event['levels'],incidences=event['incidences'],peak_reserved_bytes=event['peak_reserved_bytes'],reserved_after_bytes=event['reserved_after_bytes'],canonical_bytes=row['canonical_bytes'],canonical_sha256=row['canonical_sha256'],semantic_sha256=row['semantic']['sha256'],logical=event['logical']))
        if row['coord_bits']==18:
            baseline=[x for x in prior['runs'] if x['case']==row['case'] and x['kmax']==row['kmax'] and x['status']=='ok']
            need(baseline,'B18 baseline absent')
            need(all(x['canonical_sha256']==row['canonical_sha256'] and x['canonical_bytes']==row['canonical_bytes'] and x['events'][1]['logical']==event['logical'] and all(x['events'][1][k]==event[k] for k in ['balls','levels','incidences','peak_reserved_bytes','reserved_after_bytes']) for x in baseline),'B18 geometry/work changed')
            comparisons.append(dict(case=row['case'],old_api_ns=[x['events'][1]['wall_ns'] for x in baseline],new_api_ns=event['wall_ns'],raw_hash_and_size_equal=True,geometric_counts_and_work_equal=True,native_buffer_peak_and_after_equal=True))
    need(len(comparisons)==5,'old/new B18 comparison inventory')
    for row in report['runs']:
        if row['status']=='timeout':need('catalogue_ms' not in row and 'semantic' not in row and row['exit_code'] is None,'timeout promoted')
    # Exact archive manifest is preserved; every other excerpt is a selected unmodified archived payload.
    for name in ['results/MANIFEST.sha256','results/worker.txt','results/cmd/000_matrice/meta.txt','results/cmd/001_profiles/meta.txt','results/cmd/001_profiles/time.txt','results/cmd/000_matrice/files/matrix/mutants/LastTest.log']:
        dest=ROOT/'excerpts'/name;dest.parent.mkdir(parents=True,exist_ok=True)
        if dest.exists():need(dest.read_bytes()==data[name],'immutable excerpt mismatch')
        else:dest.write_bytes(data[name])
    output={'coherence':'conforme','campaign':'failed_remote','source_commit':receipt['commit'],'native_executions':0,'cloud_actions':0,'product_builds':0,'manifest_entries':len(mapped),'archive_entries':len(seen),'archive_bytes':archive.stat().st_size,'archive_expanded_bytes':expanded,'matrix_gate_instances':1002,'matrix_configurations':[{k:c.get(k) for k in ['name','status','compiler','cmake_options','tests']} for c in summary['configurations']],'mutant_unique_count':len(mutant_hits),'mutant_causes':dict(mutation_causes),'mutant_summaries':summaries,'high_bit_mutant_overrides':high_mutants,'profile_verdict':verdict,'attempt_statuses':dict(statuses),'not_run':report['not_run'],'complete_comparisons':report['comparisons'],'success_table':table,'prior_B18_comparisons':comparisons,'native_images':report['builds'],'input_manifest_sha256':input_sha,'command_metas':metas,'worker':worker,'closure':{k:receipt[k] for k in ['generation','closing_generation','observed_after','targeted_shutdown_certified','private_key_deleted','oslogin_key_removed','reserve_released']},'vm_facts':raw['vm_facts'],'measurement_limits':['CPU exact catalogue only, no FULL/GPU','Same u18 input coordinates, no physical precision comparison','One repeat per profile, sequential unpaired observations','Canonical payloads deleted; raw/semantic hashes are recorded, not rehashed from this archive','Buffer reservations during catalogue are not process/global/Python RSS','Timeout bound is native process30s, no completed API duration']}
    print(json.dumps(output,indent=2,sort_keys=True))
if __name__=='__main__':review()
