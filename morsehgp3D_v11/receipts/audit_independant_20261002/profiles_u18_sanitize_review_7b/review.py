#!/usr/bin/env python3
"""Closed ASan18 evidence reader, using local copies only; no native/build/cloud."""
from pathlib import Path, PurePosixPath
import importlib.util,json,re,tarfile
ROOT=Path(__file__).resolve().parent
RECEIPTS=ROOT/'developer_copies/morsehgp3D_v11/receipts'
FOLDER=RECEIPTS/'catalogue_profiles_20261002/asan18'
spec=importlib.util.spec_from_file_location('asan18_reader',RECEIPTS/'catalogue_profiles_20261002/check_asan18.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
need,js,sha,fields=reader.need,reader.js,reader.sha,reader.fields

def review():
    receipt=js((FOLDER/'receipt.json').read_bytes());rawb=(ROOT/'original_local/receipt.json').read_bytes();raw=js(rawb)
    need(sha(rawb)==receipt['original_receipt_sha256'],'original receipt SHA')
    need(all(json.dumps(v,sort_keys=True)==json.dumps(raw[k],sort_keys=True) for k,v in receipt.items() if k in raw),'compact/raw values')
    need(receipt['commit']=='d77e4b77c0bb38be82b83908b2724bec1663645b' and receipt['status']=='completed' and receipt['worker_exit_code']==0,'source/session status')
    need(receipt['closure']=='stopped' and receipt['targeted_shutdown_certified'] is True and receipt['stop_exit_code']==0 and not receipt['errors'],'targeted closure')
    need(receipt['generation']==receipt['closing_generation']==receipt['observed_after']['lastStartTimestamp'] and receipt['observed_after']['status']=='TERMINATED','exact generation')
    need(all(receipt[k] is True for k in ['private_key_deleted','oslogin_key_removed','reserve_released','results_verified']),'cleanup')
    need(raw['guest_guard_intact'] is True and raw['start_certified'] is True,'guest guard/start')
    data={};names=set();expanded=0;archive=FOLDER/'results.tar.gz'
    need(sha(archive.read_bytes())==receipt['results_sha256'] and archive.stat().st_size==receipt['results_bytes'],'archive identity')
    with tarfile.open(archive) as stream:
      for member in stream:
        path=PurePosixPath(member.name)
        need(member.name not in names and not path.is_absolute() and '..' not in path.parts and (member.isfile() or member.isdir()),'archive path')
        names.add(member.name);expanded+=member.size
        need(0<=member.size<=16*1024**2 and expanded<=16*1024**2,'archive bound')
        if member.isfile():data[member.name]=stream.extractfile(member).read()
    need(expanded==receipt['results_expanded_bytes'] and not receipt['results_skipped_members'],'expanded archive')
    manifest=data['results/MANIFEST.sha256'];listed=[]
    for line in manifest.decode().splitlines():
      digest,path=line.split('  ',1);name='results/'+path.removeprefix('./')
      need(re.fullmatch('[0-9a-f]{64}',digest) and name in data and sha(data[name])==digest,'manifest hash')
      listed.append(name)
    need(len(listed)==len(set(listed))==raw['results_manifest_files'] and set(listed)==set(data)-{'results/MANIFEST.sha256'},'manifest inventory')
    worker=fields(data['results/worker.txt']);meta=fields(data['results/cmd/000_matrice/meta.txt'])
    need(worker['source']=='commit:'+receipt['commit'] and worker['package_sha256']==receipt['package_sha256'] and worker['generation']==receipt['generation'],'worker pin')
    need(worker['commands_total']==worker['commands_ok']=='1' and worker['status']=='completed' and worker['interrupted']=='0','worker success')
    need(meta['status']=='ok' and meta['exit_code']=='0' and meta['group_closed']=='1' and meta['residual_group_killed']=='0','command group/status')
    need(meta['streams_truncated']=='0' and worker['truncated_streams']=='0' and worker['overflow_unresolved']=='0','lossless collection')
    plan=(ROOT/'original_local/package/plan.json').read_bytes();script=(ROOT/'original_local/package/plan.sh').read_bytes()
    need(sha(plan)==receipt['plan_sha256'] and sha(script)==receipt['worker_plan_sha256'] and script==data['results/plan.sh'],'plan SHA')
    package=js((ROOT/'source_package_verified.json').read_bytes())
    need(package['byte_identical'] is True and package['sha256']==package['git_archive_sha256']==receipt['package_sha256'],'source archive Git equality')
    need(data[reader.BASE+'summary.json']==(FOLDER/'matrix.json').read_bytes(),'matrix exact bytes')
    counts,required,code=reader.judge(data);need(counts==(13,13,0,0) and required==3 and code==0,'actual gates')
    reader.old.inputs(FOLDER,receipt)
    prov=js(data[reader.PREFIX+'build_provenance.json']);rows={r['path']:r for r in prov['files']}
    for target in reader.TARGETS:
      need('-fsanitize=address,undefined' in rows['CMakeFiles/'+target+'.dir/flags.make']['text'],'compiled sanitizer')
    for target in ['mhgp11_num_probe','mhgp11_num_unit']:
      need('-fsanitize=address,undefined' in rows['CMakeFiles/'+target+'.dir/link.txt']['text'],'linked sanitizer')
    junit=reader.old.foundation.ET.fromstring(data[reader.PREFIX+'junit.xml']);outputs={c.get('name'):c.findtext('system-out') or '' for c in junit.iter('testcase')}
    gate_outputs={name:outputs[name] for name in sorted(reader.REQUIRED)}
    for name in ['results/MANIFEST.sha256','results/worker.txt','results/cmd/000_matrice/meta.txt',reader.PREFIX+'build_provenance.json',reader.PREFIX+'junit.xml']:
      dest=ROOT/'excerpts'/name;dest.parent.mkdir(parents=True,exist_ok=True)
      if dest.exists():need(dest.read_bytes()==data[name],'excerpt unchanged')
      else:dest.write_bytes(data[name])
    output={'coherence':'conforme','campaign':'completed','source_commit':receipt['commit'],'native_executions_by_auditor':0,'cloud_actions_by_auditor':0,'gates_selected_passed':13,'required_gates_passed':3,'archive_bytes':archive.stat().st_size,'archive_expanded_bytes':expanded,'archive_members':len(names),'manifest_entries':len(listed),'required_gate_outputs':gate_outputs,'compiled_and_linked_sanitizers_verified':True,'provenance':prov,'meta':meta,'closure':{k:receipt[k] for k in ['generation','closing_generation','observed_after','targeted_shutdown_certified','private_key_deleted','oslogin_key_removed','reserve_released']},'q3_causal_coverage':{'compiled_bits':18,'Budget_side':6*18+8,'native_selector':'Budget::side <= 127 || sphere.presentation_arity() != 3','q3_uses_native':6*18+8<=127,'constructed_arities':[1,2,3,4],'fixtures':['small axes','large regular tetra corners at CoordMax'],'queries_per_arity_per_fixture':7,'calls':['power','side'],'expected_formula':'Wide<4> first product and all three linear terms, independent of native selector','extra_q3_cases':['4*CoordMax^6 outside triangle, expected bit length110','shell power0 with first-term cancellation; first term fits i128 in B18']},'limits':['num module plus style only','No catalogue timing/FULL/GPU or whole u18 sanitizer matrix','Causal execution follows selected source plus compiled profile; no native path counters added','Same C++ product/test source as9df; tests/num README only difference in compared paths']}
    print(json.dumps(output,indent=2,sort_keys=True))
if __name__=='__main__':review()
