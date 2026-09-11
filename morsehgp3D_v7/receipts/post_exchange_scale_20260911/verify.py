#!/usr/bin/env python3
"""Portable closure, exact work, three standalone runs and synthetic format gates."""
import copy
import hashlib
import importlib.machinery
import importlib.util
import json
import math
from pathlib import Path
import re
import sys

BASE=Path(__file__).resolve().parent
ORIGIN='/workspaces/E-HGP/build/v7_post_exchange_scale_20260911'
RECORD_SHA='5c4e31fd05cf034899ac535d00ea4f2938d3b858ebb647cc743bd660012902e0'
INPUT_SHA='7ce9df73726bcfdb946949b94a36ac32fe7b97f80e8ba35894b8f8487535ff74'
HEADER_SHA='6763a877f43d79a45532bce4426feca645b6ee97f18c9a4ee1fea4c47cd408a5'
BINARY_SHA='ade5dbdd7b35fb1f66a8e9f7ea6f33c315458b1ce12897396686ce7782cd9512'
REFERENCE_SHA='2485cafaf912c9138cb618199f7fd59cd3670bcc2365be05295d4e3eca54d5e9'


def need(ok,why):
    if not ok:raise RuntimeError(why)


def sha(data):return hashlib.sha256(data).hexdigest()


def relative(name):
    path=Path(name)
    need(not path.is_absolute() and '..' not in path.parts and path.as_posix()==name,'relative_path')
    need((BASE/path).resolve().is_relative_to(BASE),'path_inside_packet')
    return path


def main():
    need(len(sys.argv)==1,'no_arguments')
    manifest=json.loads((BASE/'MANIFEST.json').read_text())
    actual={p.relative_to(BASE).as_posix() for p in BASE.rglob('*') if p.is_file() and p.name!='MANIFEST.json'}
    need(actual==set(manifest['files']),'exact_publication_inventory')
    for name,pin in manifest['files'].items():
        path=BASE/relative(name);value=path.read_bytes()
        need(not path.is_symlink() and sha(value)==pin and not value.startswith(b'\x7fELF'),'pin_and_no_ELF')
    need(manifest['public_status']=='not_claimed' and manifest['GCP_used'] is False and
         manifest['ELF_included'] is False and manifest['vendor_included'] is False,'scope')
    mapping=json.loads((BASE/'storage_map.json').read_text());need(len(mapping)==manifest['logical_files'],'logical_file_count')
    data={}
    for logical,row in mapping.items():
        relative(logical)
        value=(BASE/relative(row['storage'])).read_bytes()
        need(sha(value)==row['sha256'] and len(value)==row['size'],'reversible_original_bytes:'+logical)
        data[logical]=value
    def obj(name):return json.loads(data[name])
    omitted=json.loads((BASE/'omitted_ELF.json').read_text())
    need(set(omitted)=={'input/probe'} and omitted['input/probe']['sha256']==BINARY_SHA,'only_qualified_ELF_omitted')
    need(sha(data['record.py'])==sha((BASE/'record_comparison.py.source').read_bytes())==RECORD_SHA,'comparator_pin')
    loader=importlib.machinery.SourceFileLoader('pinned_comparator',str(BASE/'record_comparison.py.source'))
    spec=importlib.util.spec_from_loader(loader.name,loader);module=importlib.util.module_from_spec(spec)
    loader.exec_module(module)  # Pinned definitions only; main() is never called.
    need(sha(data['input/MANIFEST.json'])==INPUT_SHA,'input_manifest_pin')
    for name,pin in obj('input/MANIFEST.json').items():
        relative(name)
        logical='input/'+name
        need((omitted[logical]['sha256'] if logical in omitted else sha(data[logical]))==pin,'every_original_input_pin')
    need(sha(data['input/source/morsehgp3D_v7/src/forest/full_ball_tower.hpp'])==HEADER_SHA,'qualified_header_pin')
    need(sha(data['input/reference_manifest.json'])==REFERENCE_SHA,'historical_reference_manifest')
    references=obj('input/reference_manifest.json')['files'];source=obj('input/source_pins.json')
    qualification=obj('input/qualification/receipt.json')
    need(qualification['status']=='passed' and qualification['selected_CTests']==24 and
         qualification['sources_and_dependencies_stable'] and qualification['binary_sha256']['mhgp7_full_ball_tower_probe']==BINARY_SHA,
         'active_CMake_input_provenance')
    rows=[];prior_end=0
    for n in (8000,16000,32000):
        prefix=f'n{n}_s8_static1/'
        receipt=obj(prefix+'receipt.json')
        need(receipt['status']=='completed' and receipt['exit_code']==0 and receipt['sources_and_binary_stable'] and
             receipt['standalone_process'] and receipt['baseline_retained'] is False and receipt['GCP_used'] is False,
             'three_complete_standalone_runs')
        need(obj(prefix+'sources_before.json')==obj(prefix+'sources_after.json')==source,'source_before_after')
        need(obj(prefix+'binary_before.json')==obj(prefix+'binary_after.json')==dict(sha256=BINARY_SHA),'binary_before_after')
        need(obj(prefix+'input_manifest_pin.json')['sha256']==INPUT_SHA and sha(data[prefix+'record.py.source'])==RECORD_SHA,
             'run_pins')
        authorization=obj(prefix+'exclusive_session.json')
        need(authorization['authorized'] is True and authorization['sizes']==[8000,16000,32000] and
             authorization['source_header_sha256']==HEADER_SHA and authorization['binary_sha256']==BINARY_SHA,'coordinated_session')
        intent=obj(prefix+'intent.json')
        need(intent['algorithmic_timeout'] is None and intent['baseline_retained'] is False and intent['standalone_process'],
             'no_algorithmic_timeout_or_retained_baseline')
        command=obj(prefix+'command.json')
        need(command['exit_code']==0 and prior_end<=command['started_ns']<=command['ended_ns'],'sequential_real_processes')
        prior_end=command['ended_ns']
        need(command['argv']==['/usr/bin/time','-v',ORIGIN+'/input/probe',f'--n={n}','--s=8','--kmax=10',
             '--threads=1','--static-threads=1'],'exact_process_arguments')
        need(obj(prefix+'owned_process.json')['process_group']>0,'owned_process_capture')
        for stream in ('stdout','stderr'):
            need(sha(data[prefix+stream])==command[stream+'_sha256'],'raw_stream_pin')
        need(sha(data[prefix+'reference.stdout'])==references[f'n{n}_s8_static4/stdout'],'published_reference_pin')
        a,b=obj(prefix+'stdout'),obj(prefix+'reference.stdout')
        derived=module.compare(a,b,n);need(derived==obj(prefix+'comparison.json'),'35_fields_RUS_QHT_actual_work_recomputed')
        stderr=data[prefix+'stderr'].decode()
        rss=re.search(r'Maximum resident set size \(kbytes\): (\d+)',stderr)
        need(rss is not None and int(rss.group(1))>0 and '\tSwaps: 0' in stderr and '\tExit status: 0' in stderr,
             'RSS_success_no_swap')
        for stage in ('index','generate','sort_rle','prefilter','census','full_ball_tower','payload_digest'):
            need('stage_complete='+stage+' seconds=' in stderr,'every_stage_closed')
        progress=sorted(name for name in data if name.startswith(prefix+'progress_') and name.endswith('.json'))
        need(len(progress)>1,'progress_captures_present')
        previous=0
        for name in progress:
            sample=obj(name);need(sample['time_ns']>=previous and sample['processes'],'progress_monotone_and_nonempty')
            previous=sample['time_ns']
        need(prefix+'host_before.json' in data and prefix+'host_after.json' in data,'host_context_retained')
        rows.append(dict(derived,max_RSS_KiB=int(rss.group(1)),max_RSS_GiB=int(rss.group(1))/(1<<20),
                         nodes=a['nodes'],retained_capacity_bytes=a['static_sampled_retained_capacity_peak_bytes']))
    doublings=[]
    for a,b in zip(rows,rows[1:]):
        keys=('R','U','S','Q','H','new_MEB','new_supports','nodes','retained_capacity_bytes','max_RSS_KiB','total_s','tower_s')
        ratios={key:b[key]/a[key] for key in keys}
        doublings.append(dict(n_from=a['n'],n_to=b['n'],ratios=ratios,
                             MEB_log2_ratio=math.log2(ratios['new_MEB']),support_log2_ratio=math.log2(ratios['new_supports'])))
    original=obj('readback_r1/normal.stdout')
    need(original==obj('readback_r1/optimized.stdout') and original['rows']==rows and original['doublings']==doublings,
         'original_readers_and_derived_values_agree')
    need(original['historical_speedup_claimed'] is False and original['universal_subquadratic_claimed'] is False and
         original['latency_contract_qualified'] is False,'limited_historical_scope')
    format_receipt=obj('format_checks/receipt.json')
    need(format_receipt['status']=='passed' and format_receipt['before']==format_receipt['after'],'format_source_stability')
    need(sha(data['format_checks/record.py.source'])==RECORD_SHA,'same_format_comparator')
    captured=obj('format_checks/parser_normal.stdout')
    need(captured==obj('format_checks/parser_optimized.stdout') and captured['engine_executed'] is False and
         captured['scale_qualified'] is False and len(captured['rejected'])==9,'nine_format_faults_not_Cpp_mutants')
    for command in format_receipt['commands']:
        need(command['exit_code']==0,'format_readback_command_passed')
        for stream in ('stdout','stderr'):
            need(sha(data['format_checks/'+command['name']+'.'+stream])==command[stream+'_sha256'],'format_raw_stream_pin')
    reference=obj('input/reference_8000.stdout');nominal=copy.deepcopy(reference)
    nominal['static_threads']=1
    nominal['resolver_cache_accounting']='static_exact_sort_unique_complete_population_seeds_after_exchange_v2'
    for row in nominal['static_orders']:row.update(post_seed_queries=0,post_seed_hits=0,post_seed_terminals=0)
    nominal['static_orders'][1].update(post_seed_queries=nominal['intruder_queries'],post_seed_hits=1,post_seed_terminals=1)
    for key in ('resolver_meb_calls','anchor_hits','resolver_supports_tested'):nominal[key]-=1
    need(module.compare(nominal,reference,8000)['H']==1,'synthetic_format_positive')
    mutations=[('digest',lambda v:v.update(payload_digest='a'*64)),
        ('R',lambda v:v['static_orders'][3].update(requests=v['static_orders'][3]['requests']+1)),
        ('H_T',lambda v:v['static_orders'][1].update(post_seed_terminals=0)),
        ('Q',lambda v:v['static_orders'][1].update(post_seed_queries=2)),
        ('fake_anchor_hit',lambda v:v.update(anchor_hits=v['anchor_hits']+1)),
        ('MEB_not_removed',lambda v:v.update(resolver_meb_calls=v['resolver_meb_calls']+1)),
        ('float_H',lambda v:v['static_orders'][1].update(post_seed_hits=1.0)),
        ('boolean_threads',lambda v:v.update(threads=True)),
        ('wrong_tag',lambda v:v.update(resolver_cache_accounting='old'))]
    rejected=[]
    for name,mutate in mutations:
        bad=copy.deepcopy(nominal);mutate(bad)
        try:module.compare(bad,reference,8000)
        except RuntimeError as exc:rejected.append(dict(name=name,reason=str(exc)))
        else:raise RuntimeError('format_fault_survived:'+name)
    need(rejected==captured['rejected'],'nine_format_causes_reexercised')
    print(json.dumps(dict(status='verified_portable_post_exchange_mono_scale',runs=3,fields_each=35,
        format_faults_reexercised=9,source_header_sha256=HEADER_SHA,binary_sha256=BINARY_SHA,
        measurements=[{k:r[k] for k in ('n','new_MEB','H','Q','total_s','tower_s','max_RSS_KiB')} for r in rows],
        doublings=doublings,GCP_used=False,engine_executed_here=False,compiler_executed_here=False,
        public_status='not_claimed',historical_speedup_claimed=False,RSS_gain_claimed=False,
        universal_subquadratic_claimed=False,latency_contract_qualified=False)))


if __name__=='__main__':main()
