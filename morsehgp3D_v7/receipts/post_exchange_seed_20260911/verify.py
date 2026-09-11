#!/usr/bin/env python3
"""Read-only science and closure verification; effective with Python -O."""
import hashlib
import json
from pathlib import Path
import re
import sys

BASE=Path(__file__).resolve().parent
HEADER='morsehgp3D_v7/src/forest/full_ball_tower.hpp'
PINS={'base':'33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209',
      'observation':'6a04f1e0925906f4f95a67c969c6e493a687832b7ed8451bfa229ab56efee118',
      'shortcut':'a73b89bfc8b03d38c546d30ee578610e4bb1d26911e252bf19399b8384af4ba6',
      'clean':'6763a877f43d79a45532bce4426feca645b6ee97f18c9a4ee1fea4c47cd408a5',
      'patch':'70453fb670019b97f1eb8d11f406625f412580bb97d11810523065a641d88535'}
RUNS={'o2_observation_r1':16,'o2_shortcut_r1':20,'san_root_r1':14,'micro_r1':2,
      'causal_r1':7,'causal_r2':7,'causal_r3':12,'clean_o2_r1':14,'clean_san_root_r1':14}
MODES=('cache','no_cache','static1','static4')
CAPACITY=('static_peak_worker_bytes','static_peak_retained_bytes')
TIMES={'index_s','generate_s','sort_s','prefilter_s','census_s','tower_s','digest_s','total_s'}
checks=0


def need(ok,why):
    global checks
    checks+=1
    if not ok:
        raise RuntimeError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    path=Path(name)
    need(not path.is_absolute() and '..' not in path.parts and path.as_posix()==name,'safe_relative_path')
    need((BASE/path).resolve().is_relative_to(BASE),'path_inside_packet')
    return path


def physical(text):
    result=[]
    for block in text.split('BEGIN')[1:]:
        lines=block.splitlines()
        stats={line.split()[1]:list(map(int,line.split()[2:])) for line in lines if line.startswith('S ')}
        result.append((stats,[line for line in lines if not line.startswith('S ')]))
    need(len(result)>=33,'physical_call_floor')
    return result


def main():
    need(len(sys.argv)==1,'no_arguments')
    manifest=json.loads((BASE/'MANIFEST.json').read_text())
    found={p.relative_to(BASE).as_posix() for p in BASE.rglob('*') if p.is_file() and p.name!='MANIFEST.json'}
    need(found==set(manifest['files']),'physical_inventory_exact')
    for name,pin in manifest['files'].items():
        path=BASE/safe(name)
        need(not path.is_symlink(),'no_symlink')
        data=path.read_bytes()
        need(sha(data)==pin and not data.startswith(b'\x7fELF'),'physical_hash_no_ELF:'+name)
    need(manifest['public_status']=='not_claimed' and manifest['GCP_used'] is False and
         manifest['ELF_included'] is False and manifest['vendor_included'] is False,'scope')
    mapping=json.loads((BASE/'storage_map.json').read_text())
    need(len(mapping)==manifest['logical_files'],'logical_inventory_size')
    contents={}
    for logical,row in mapping.items():
        safe(logical)
        data=(BASE/safe(row['storage'])).read_bytes()
        need(len(data)==row['size'] and sha(data)==row['sha256'],'reversible_mapping:'+logical)
        contents[logical]=data
    def raw(name):
        need(name in contents,'logical_present:'+name)
        return contents[name]
    def text(name):
        return raw(name).decode('utf-8')
    def obj(name):
        return json.loads(raw(name))
    def filtered(name,capacity=False):
        return [line for line in text(name).splitlines() if not line.startswith('S post_seed_') and
                (not capacity or not line.startswith(tuple('S '+k+' ' for k in CAPACITY)))]
    need(sha(raw('original_header.hpp.source'))==PINS['base'],'base_pin')
    for run,kind in [('o2_observation_r1','observation'),('o2_shortcut_r1','shortcut')]:
        need(sha(raw(run+'/source/candidate/'+HEADER))==PINS[kind],'experimental_header_pin')
    for run in ('clean_o2_r1','clean_san_root_r1'):
        need(sha(raw(run+'/source/product/'+HEADER))==PINS['clean'],'clean_header_pin')
    need(sha(raw('integration_proposal_r1/integration.patch'))==PINS['patch'],'patch_pin')
    proposal=obj('integration_proposal_r1/pins.json')
    need(proposal['patch_sha256']==PINS['patch'] and proposal['applied'] is False,'proposal_historical_scope')
    for name,pin in proposal['after'].items():
        need(sha(raw('clean_o2_r1/source/product/'+name))==pin,'three_file_proposal_exact')
    command_count=0
    for run,count in RUNS.items():
        receipt=obj(run+'/receipt.json')
        surviving=run in ('causal_r1','causal_r2')
        need(receipt['status']==('failed' if surviving else 'passed'),'capture_verdict:'+run)
        if surviving:
            need(receipt['error']=='RuntimeError: unexpected_exit:partial_key_run','surviving_mutant_preserved')
        commands=obj(run+'/commands.json')
        need(len(commands)==count,'command_floor:'+run)
        command_count+=len(commands)
        prior=0
        for command in commands:
            name=command['name']
            expected=command.get('expected_exit',0)
            need(command['started_ns']>=prior and command['ended_ns']>=command['started_ns'],'sequential_command')
            prior=command['ended_ns']
            code=0 if surviving and name=='partial_key_run' else expected
            need(command['exit_code']==code,'exact_exit:'+run+':'+name)
            for suffix in ('stdout','stderr'):
                need(sha(raw(run+'/'+name+'.'+suffix))==command[suffix+'_sha256'],'raw_stream_pin')
            if run in ('san_root_r1','clean_san_root_r1'):
                env=command.get('sanitizer_environment',{})
                # The older recorder stores its exact environment under env.
                if not env:
                    env=command.get('environment',{})
                if not env:
                    env=obj(run+'/sanitizer_environment.json') if run+'/sanitizer_environment.json' in contents else {}
                need(env.get('ASAN_OPTIONS')=='detect_leaks=1:halt_on_error=1' and
                     env.get('UBSAN_OPTIONS')=='halt_on_error=1:print_stacktrace=1','SAN_options_on')
        # Successful ordinary captures all freeze their whole source subtree.
        if run+'/sources_before.json' in contents:
            before=obj(run+'/sources_before.json')
            need(before==obj(run+'/sources_after.json'),'source_before_after:'+run)
            for name,pin in before.items():
                need(sha(raw(run+'/source/'+name))==pin,'source_pin:'+run)
        if run.startswith('causal_'):
            for variant in receipt['started_variants']:
                prefix=run+'/'+variant+'/'
                before=obj(prefix+'sources_before.json')
                for name,pin in before.items():
                    need(sha(raw(prefix+'source/'+name))==pin,'causal_source_pin')
                if prefix+'sources_after.json' in contents:
                    need(before==obj(prefix+'sources_after.json'),'causal_after_pin')
    for run in ('o2_observation_r1','o2_shortcut_r1'):
        for mode in MODES:
            a=run+'/baseline_'+mode+'_observe0.physical'
            b=run+'/candidate_'+mode+'_observe0.physical'
            c=run+'/candidate_'+mode+'_observe1.physical'
            need(filtered(a,True)==filtered(b,True),'baseline_physical_identity')
            need(filtered(b)==filtered(c),'observation_old_work_identity')
    for mode in MODES:
        prefix='o2_shortcut_r1/candidate_'+mode+'_observe'
        obs,opt=physical(text(prefix+'1.physical')),physical(text(prefix+'2.physical'))
        need(len(obs)==len(opt),'same_builder_call_count')
        total_h=0
        for (a,pa),(b,pb) in zip(obs,opt):
            need(pa==pb,'whole_physical_output_identity')
            h=sum(a['post_seed_hits'][1:]);total_h+=h
            need(a['post_seed_hits']==b['post_seed_hits'] and a['post_seed_queries']==b['post_seed_queries'],
                 'same_lookup_trajectory')
            need(a['post_seed_verified']==a['post_seed_hits']==b['post_seed_terminal'],'observed_then_terminal')
            need(not any(a['post_seed_terminal'][1:]) and not any(b['post_seed_verified'][1:]),'mode_separation')
            need(a['post_seed_observed_work'][0]==h and b['post_seed_observed_work'][0]==0,'one_MEB_removed_per_hit')
            for key in a:
                if key.startswith('post_seed_'):
                    continue
                if key in ('anchor_hits','key_lookups'):
                    need(a[key][0]==b[key][0]+h,'actual_paid_terminal_work')
                elif key=='resolve_work':
                    paid=a['post_seed_observed_work']
                    need(a[key][1]==b[key][1]==paid[1]==5,'support_counter_shape')
                    need(all(a[key][i]==b[key][i]+paid[i] for i in range(len(paid)) if i!=1),'all_paid_MEB_work_removed')
                else:
                    need(a[key]==b[key],'other_old_work_exact:'+key)
        need(total_h==(4 if mode.startswith('static') else 0),'bounded_hit_floor')
        clean='clean_o2_r1/'+mode+'.physical'
        need(filtered(prefix+'2.physical',True)==filtered(clean,True),'own_clean_vs_a73_physical')
        need(raw(clean)==raw('clean_san_root_r1/'+mode+'.physical'),'own_clean_SAN_physical')
        for observed in (0,1,2):
            name='candidate_'+mode+'_observe'+str(observed)
            for suffix in ('.physical','.stdout','.stderr'):
                need(raw('o2_shortcut_r1/'+name+suffix)==raw('san_root_r1/'+name+suffix),'a73_SAN_exact')
    for run in ('causal_r1','causal_r2'):
        need(obj(run+'/partial_key_run.stdout')['status']=='passed','survivor_not_hidden')
    need(obj('causal_r2/nominal_4.stdout')['post_seed_hits']==8 and
         obj('causal_r2/partial_key_run.stdout')['post_seed_hits']==12,'Gamma_not_terminal_authority')
    need(text('causal_r3/partial_key_run.stderr').strip()=='tower_status:E5:full_ball_post_seed_observation_mismatch',
         'partial_key_true_MEB_causal_reason')
    need(text('causal_r3/omit_terminal_exchange_run.stderr').strip()=='post_seed_exchange_accounting','exchange_causal_reason')
    need(text('causal_r3/accept_equal_fault_run.stderr').strip()=='controlled_equal_level_fault_was_not_rejected',
         'test_only_equal_state_causal_reason')
    for mode in ('0','1','4','observe'):
        row=obj('causal_r3/nominal_'+mode+'.stdout')
        need(row['status']=='passed' and row['rows']==9660 and row['orders']==168 and row['vertical']==6040,'causal_floors')
    need(obj('causal_r3/nominal_equal_fault.stdout')['test_only_nongeometric_state'] is True,'equal_state_scope')
    micro={}
    for line in text('micro_r1/micro.stdout').splitlines():
        if not line.startswith('n='):
            continue
        row=dict(token.split('=',1) for token in line.split())
        micro[int(row['n']),int(row['mode'])]=row
    expected={200:(17419,2945,51563,48618),400:(43638,7551,132750,125199),
              800:(106276,17933,314605,296672),1000:(135371,23205,406134,382929)}
    for n,(d,h,old,new) in expected.items():
        a,b,c=(micro[n,mode] for mode in (0,1,2))
        need(int(a['M'])==old and int(c['M'])==new and old-new==h,'micro_exact_MEB_saving')
        need(int(b['H'])==int(c['H'])==h and int(b['queries'])==int(c['queries'])==d,'micro_hits_lookups')
        for key in ('balls','nodes','parents','contributions','populations','R','U','S','D','same_radius',
                    'retained_buffers','input_digest','physical_digest'):
            need(a[key]==b[key]==c[key],'micro_same_outputs:'+key)
        for key,observed in [('M','observed_calls'),('supports','observed_supports'),('powers','observed_powers'),
                             ('materializations','observed_materializations')]:
            need(a[key]==b[key] and int(a[key])-int(c[key])==int(b[observed]),'micro_paid_work_identity:'+key)
        need(h<=min(d,int(a['U'])-int(a['S'])),'micro_hit_bound')
    for run in ('clean_o2_r1','clean_san_root_r1'):
        for mode in ('static1','static4'):
            row=obj(run+'/product_'+mode+'.stdout')
            need(row['status']=='passed' and row['clouds']==34 and row['orders']==150 and
                 row['vertical_checks']==87230,'clean_own_gate_floors')
        for threads in (0,1):
            name='probe_static'+str(threads)
            actual=obj(run+'/'+name+'.stdout')
            prior=obj('clean_o2_r1/'+name+'.stdout')
            need({k:v for k,v in actual.items() if k not in TIMES}==
                 {k:v for k,v in prior.items() if k not in TIMES},'clean_probe_all_non_time_fields')
            need(actual['orders']==10 and actual['contract_qualified'] is False,'probe_limited_scope')
            rows=actual['static_orders']
            need([r['K'] for r in rows]==list(range(1,11)),'probe_whole_tower')
            q,h,t=(sum(r[k] for r in rows) for k in ('post_seed_queries','post_seed_hits','post_seed_terminals'))
            need((q,h,t)==((17419,2945,2945) if threads else (0,0,0)),'probe_post_work')
            if threads:
                need(actual['resolver_meb_calls']==48618 and actual['resolver_cache_accounting']==
                     'static_exact_sort_unique_complete_population_seeds_after_exchange_v2','probe_accounting_v2')
        a,b=(obj(run+'/probe_static'+str(t)+'.stdout') for t in (0,1))
        for key in ('input_digest','payload_digest','balls','nodes','parent_refs','contributions','vertical_refs'):
            need(a[key]==b[key],'probe_nominal_static_output')
    need(command_count==106,'all_commands_retained')
    print(json.dumps(dict(status='verified_private_post_exchange_seed',checks=checks,commands=command_count,
        logical_files=len(mapping),compiled_mutants_refuted=3,surviving_attempts_preserved=2,
        bounded_causal_Gamma_rows=9660,clean_gate_orders=150,micro_sizes=list(expected),
        compiled_here=False,GCP_used=False,public_status='not_claimed',latency_contract_qualified=False)))


if __name__=='__main__':
    main()
