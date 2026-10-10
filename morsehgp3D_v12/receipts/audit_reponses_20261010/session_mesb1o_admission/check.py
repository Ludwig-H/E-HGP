#!/usr/bin/env python3
"""MES-B L1o provenance/admission only; no cloud, engine, compiler or coordinate/ID payload read."""
from pathlib import Path
import argparse, csv, hashlib, importlib.util, io, json, re, subprocess, sys, tarfile, tempfile
sys.dont_write_bytecode = True
SOURCE = 'ae8f8107cc5a13356da89addf90808b5aaad9d67'
B3K = '2aaed1847e63ff86550db1fb1ba6e36313aa58bc'
P = 'morsehgp3D_v12/'
HERE = Path(__file__).resolve().parent
PUBLISHED = 'fbd5923a8a892e1dbab8117f0e62d04f1b432d2a'
PUBLIC = P+'receipts/g4_mesb1o_20261010/'
EXTRA = {P+'microbancs/mes_b_scenes/pilote_b.py', P+'microbancs/outils/lecteur_full.py',
         P+'microbancs/outils/banc_full.py', 'gcp-migration/v12_worker.sh'}
SCOPES = tuple(P+x for x in ('src/','bench/','tests/','cmake/'))
def need(ok, why):
    if not ok: raise ValueError(why)
def sha(raw): return hashlib.sha256(raw).hexdigest()
def pin(raw): return dict(bytes=len(raw),sha256=sha(raw))
def unique(pairs):
    result={}
    for k,v in pairs:
        need(k not in result,'duplicate JSON key'); result[k]=v
    return result
def bad_constant(value): raise ValueError('nonfinite JSON constant')
def decode(raw): return json.loads(raw,object_pairs_hook=unique,parse_constant=bad_constant)
def selected(name): return name.startswith(SCOPES) or name in EXTRA or name==P+'CMakeLists.txt'
def git(repo, rev, name): return subprocess.check_output(['git','show',rev+':'+name],cwd=repo)
def bundle(repo, session, pre):
    path=session/'package/package.tar.gz'; raw=path.read_bytes()
    need(pin(raw)==dict(bytes=pre['package_bytes'],sha256=pre['package_sha256']),'package hash')
    files={}
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        for member in archive:
            name=member.name
            need(not Path(name).is_absolute() and '..' not in Path(name).parts and not member.issym() and not member.islnk(),'package member')
            if member.isfile() and selected(name):
                need(name not in files,'duplicate selected source'); files[name]=archive.extractfile(member).read()
    names=sorted(files)
    gitnames=subprocess.check_output(['git','ls-tree','-r','--name-only',SOURCE],cwd=repo,text=True).splitlines()
    need(set(names)=={n for n in gitnames if selected(n)},'exhaustive selected source scope')
    response=subprocess.check_output(['git','cat-file','--batch'],cwd=repo,input=('\n'.join(SOURCE+':'+n for n in names)+'\n').encode())
    offset=0
    for name in names:
        end=response.index(b'\n',offset); fields=response[offset:end].split(); need(fields[1]==b'blob','Git blob')
        size=int(fields[2]); offset=end+1; need(response[offset:offset+size]==files[name],'Git/package '+name);offset+=size+1
    need(offset==len(response) and path.read_bytes()==raw,'package stable')
    native={n:b for n,b in files.items() if n.startswith(P+'src/')}
    need(len(native)==138,'138 native source files')
    need(set(native)=={n for n in subprocess.check_output(['git','ls-tree','-r','--name-only',B3K,'--',P+'src'],cwd=repo,text=True).splitlines()},'B3K native inventory')
    for name,raw_native in native.items(): need(git(repo,B3K,name)==raw_native,'B3K native '+name)
    inventory=lambda fs:sha(''.join(sha(b)+'  '+n+'\n' for n,b in sorted(fs.items())).encode())
    return dict(source_git=SOURCE,package=pin(raw),selected_files_exact=len(files),selected_inventory_sha256=inventory(files),
                native_files_exact=138,native_inventory_sha256=inventory(native),native_equal_B3K=B3K,
                critical_sources={n:pin(files[n]) for n in sorted(EXTRA|{P+'bench/full_probe.cpp',P+'CMakeLists.txt'})}),files

def planned(plan,pre):
    need(plan['schema']=='ehgp.v12.session_plan.v1' and plan['default_build'] is False and plan['python_packages']=='none','plan modes')
    commands=plan['commands']; need([c['name'] for c in commands]==['mes_b_flux_identite','mes_b_l1'],'planned commands')
    need([c['timeout_seconds'] for c in commands]==[900,2900],'command timeouts')
    configs=[]
    for index,c in enumerate(commands):
        a=c['argv']; need(a[:2]==['python3','{src}/'+P+'microbancs/mes_b_scenes/pilote_b.py'],'pilot command')
        need(len(a[2:])%2==0,'argument pairs'); options=dict(zip(a[2::2],a[3::2])); need(len(options)*2==len(a)-2,'unique options')
        need(options['--fils']=='48' and options['--jobs']=='44' and options['--budget-gio']=='160' and options['--empreinte-max-sites']=='1600000','common parameters')
        need(options['--budget-appareil-gio']==['8','88'][index] and options['--delai-global']==['840','2800'][index],'device budget/delay')
        need(not any(x in a for x in ('--essai','--sequentiel','--sonde')),'product overlap path')
        cases=[]
        for item in options['--cas'].split(','):
            n,k,v,count=item.split(':');cases.append(dict(name=n,k=int(k),path=v,passes=int(count)))
        need(len(cases)==[2,17][index] and len({(c['name'],c['k'],c['path']) for c in cases})==len(cases),'planned distinct cases')
        configs.append(dict(name=c['name'],timeout=c['timeout_seconds'],options=options,cases=cases))
    budget=pre['budget']; need(budget['worker_window_seconds']==2200 and budget['command_timeouts_sum_seconds']==3800 and budget['oversubscribed'] is True,'oversubscribed window')
    return configs

def archive_files(raw):
    fs={}
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        for m in archive:
            p=Path(m.name);need(not p.is_absolute() and '..' not in p.parts and not m.issym() and not m.islnk(),'returned member')
            need(p.suffix not in {'.u32le','.f32le','.ply','.pcd','.las','.laz','.bin'},'unexpected payload member')
            if m.isfile(): need(m.name not in fs,'repeated returned member');fs[m.name]=archive.extractfile(m).read()
    return fs

def closed(session, pre, plan, configs, src, metadata, snapshot):
    receipt=decode((session/'receipt.json').read_bytes());raw=(session/'results/results.tar.gz').read_bytes()
    need(pin(raw)==dict(bytes=receipt['results_bytes'],sha256=receipt['results_sha256']),'returned archive pin')
    fs=archive_files(raw);covered=set()
    for line in fs['results/MANIFEST.sha256'].decode().splitlines():
        digest,name=line.split(None,1);name='results/'+name.strip().removeprefix('./')
        need(name not in covered and name in fs and sha(fs[name])==digest,'result manifest entry');covered.add(name)
    need(set(fs)-covered=={'results/MANIFEST.sha256'} and len(covered)==receipt['results_manifest_files'],'result manifest coverage')
    need(sum(map(len,fs.values()))==receipt['results_expanded_bytes'],'expanded bytes')
    commands=list(csv.DictReader(io.StringIO(fs['results/commands.tsv'].decode()),delimiter='\t'))
    need(commands==receipt['commands'] and [c['name'] for c in commands]==[c['name'] for c in configs],'command closure')
    need(receipt['commit']==SOURCE and receipt['package_sha256']==pre['package_sha256'] and receipt['plan_sha256']==pre['plan_sha256'],'receipt source pins')
    need(fs['results/plan.sh']==(session/'package/plan.sh').read_bytes(),'worker plan copy')
    worker=dict(line.split('=',1) for line in fs['results/worker.txt'].decode().splitlines() if '=' in line)
    need(worker['source']=='commit:'+SOURCE and worker['package_sha256']==pre['package_sha256'] and worker['plan_sha256']==pre['worker_plan_sha256'],'worker sources')
    closure={k:receipt[k] for k in ('status','worker_exit_code','results_verified','targeted_shutdown_certified','stop_exit_code')}
    closure.update(done=(session/'DONE').read_text().strip(),errors_count=len(receipt['errors']),warnings_count=len(receipt['warnings']),
                   observed_before=receipt['observed_before_stop']['status'],observed_after=receipt['observed_after']['status'])
    need(closure['targeted_shutdown_certified'] is True and closure['stop_exit_code']==0 and closure['observed_after']=='TERMINATED','archived stop certification')
    complete=closure['status']=='completed' and closure['worker_exit_code']==0 and closure['done']=='0' and closure['results_verified'] is True and closure['errors_count']==0
    results=[]
    with tempfile.TemporaryDirectory(prefix='mesb1o-reader-') as td:
        reader_path=Path(td)/'lecteur_full.py';reader_path.write_bytes(src[P+'microbancs/outils/lecteur_full.py'])
        spec=importlib.util.spec_from_file_location('mesb1o_full_reader',reader_path);lf=importlib.util.module_from_spec(spec);spec.loader.exec_module(lf)
        for index,(config,command) in enumerate(zip(configs,commands)):
            prefix='results/cmd/%03d_%s/'%(index,config['name']);base=prefix+'files/'+['b_identite','b'][index]+'/'
            meta=dict(line.split('=',1) for line in fs[prefix+'meta.txt'].decode().splitlines() if '=' in line)
            need(meta['status']==command['status'] and meta['requested_timeout_seconds']==str(config['timeout']),'command metadata')
            if 'effective_timeout_seconds' in meta:
                need(0<int(meta['effective_timeout_seconds'])<=config['timeout'] and meta['exit_code']==command['exit_code'],
                     'effective timeout and code')
            destination=snapshot/base.removeprefix('results/');destination.mkdir(parents=True,exist_ok=True)
            report_name=base+'rapport_b.json'
            item=dict(name=config['name'],command={k:command[k] for k in ('status','exit_code','wall_seconds','timeout_seconds')},
                      deadline={k:meta[k] for k in ('requested_timeout_seconds','effective_timeout_seconds',
                                'group_closed','streams_truncated') if k in meta},
                      report_present=report_name in fs,stderr_bytes=len(fs.get(prefix+'stderr',b'')))
            need(report_name in fs,'both final reports retained in the frozen cohort')
            report=decode(fs[report_name]);provenance=report['provenance']
            need(provenance['pilote_sha256']==sha(src[P+'microbancs/mes_b_scenes/pilote_b.py']) and provenance['manifeste_sha256']==metadata['sha256'],'report source/metadata')
            cmake=provenance['cmake'];need({'CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON'}<=set(cmake),'compile profile')
            need(re.fullmatch('[0-9a-f]{64}',provenance['sonde_sha256']) is not None,'initial ELF digest')
            params=report['parametres'];options=config['options']
            need(params['schema']=='recouvert' and params['fils']==48 and params['budget_octets']==160*(1<<30) and params['budget_appareil_octets']==int(options['--budget-appareil-gio'])*(1<<30),'report configuration')
            report_argv=params['argv'];need(len(report_argv)==len(options)*2,'report argv size')
            actual_options=dict(zip(report_argv[::2],report_argv[1::2]));need(actual_options.keys()==options.keys(),'report argv keys')
            for key,value in options.items():
                if key not in ('--src','--travail','--donnees','--sortie'):need(actual_options[key]==value,'report argv '+key)
            need(actual_options['--sortie'].endswith('/'+base.removeprefix('results/').rstrip('/')),'report output destination')
            need(len(report['cas'])==len(config['cases']),'report case coverage')
            cases=[];expected_raw=set();used=set()
            for planned_case,case in zip(config['cases'],report['cas']):
                name=planned_case['name'];site=metadata['sites'][name];label=name[-23:];attempt=1
                while label in used:
                    start=str(attempt)+'_';label=start+name[-(23-len(start)):];attempt+=1
                used.add(label)
                need((case['nom'],case['k'],case['voie'],case['sites'],case['etiquette'])==(name,planned_case['k'],planned_case['path'],site,label),'case contract')
                digest=site<=1600000; need(case['empreinte'] is digest and case['fils']==48,'case digest/threads')
                stem='%s_k%d_%s'%(name,case['k'],case['voie']);json_name=base+'brut/'+stem+'.jsonl';err_name=base+'brut/'+stem+'.err'
                outcome=dict(name=name,k=case['k'],path=case['voie'],sites=site,requested=planned_case['passes'],state=case['etat'],reason=case['raison'])
                if case['etat']=='non_joue':
                    need(json_name not in fs and err_name not in fs and not case['passes'],'unplayed case has no native output');outcome['native_code']=None
                else:
                    need(json_name in fs and err_name in fs,'native output pair');expected_raw.update((json_name,err_name))
                    need(type(case['code']) is int or case['code']=='expire','native code type')
                    expected=dict(voie=case['voie'],k=case['k'],fils=48,passes=planned_case['passes'],empreinte=digest,trames=[(label,site)],budget_appareil='separe',bits=21,schema='recouvert')
                    parsed=lf.parse_output(case['code'],fs[json_name].decode('ascii'),expected)
                    need(all(case[k]==v for k,v in parsed.items()),'FULL reader versus report')
                    outcome.update(native_code=case['code'],complete_full_passes=len(parsed['passes']),stderr_bytes=len(fs[err_name]),
                                   raw=pin(fs[json_name]),stderr=pin(fs[err_name]))
                    if case['etat']=='refus':
                        rows,problem=lf.read_rows(fs[json_name].decode('ascii'));need(not problem,'refusal rows')
                        outcome.update(native_phases=[row['phase'] for row in rows],
                                       nvidia_smi_peak_mio=case['pic_nvidia_smi_mio'],
                                       failed_budget_recorded=False,failure_stage_recorded=False)
                    for n in (json_name,err_name):
                        dest=destination/'brut'/Path(n).name;dest.parent.mkdir(parents=True,exist_ok=True)
                        if dest.exists():need(dest.read_bytes()==fs[n],'snapshot stable')
                        else:dest.write_bytes(fs[n])
                cases.append(outcome)
            actual_raw={n for n in fs if n.startswith(base+'brut/')};need(actual_raw==expected_raw,'no unexplained native output')
            dest=destination/'rapport_b.json'
            if dest.exists():need(dest.read_bytes()==fs[report_name],'report snapshot stable')
            else:dest.write_bytes(fs[report_name])
            item.update(initial_elf_sha256=provenance['sonde_sha256'],cmake=cmake,final_elf_hash_archived=False,
                        cases=cases,build_log_present=base+'construction.log' in fs,report=pin(fs[report_name]),
                        environment={when:{key:report['environnement'][when].get(key) for key in
                                    ('cmake','nvcc','gpu','noyau','fils_hote','memoire_hote')} for when in ('avant','apres')},
                        environment_empty_endpoints=all(report['environnement'][when].get('gpu_apps')=='' for when in ('avant','apres')))
            results.append(item)
    initial_hashes=[c['initial_elf_sha256'] for c in results if 'initial_elf_sha256' in c]
    return dict(state='closed',archive=pin(raw),manifest_entries=len(covered),closure=closure,commands=results,
                session_complete=complete,all_commands_zero=all(c['status']=='ok' and c['exit_code']=='0' for c in commands),
                initial_ELF_equal_between_commands=len(initial_hashes)==len(configs) and len(set(initial_hashes))==1,
                native_return_codes='declared by pinned pilot in report; no independent per-native exit file',
                new_native_test_gates=0,physical_ELF_returned=any(b.startswith(b'\x7fELF') for b in fs.values()),
                final_ELF_hashes_archived=False,native_executed_by_audit=False,payload_read=False)

def run(repo,session,snapshot):
    pre_raw=(session/'preflight.json').read_bytes();pre=decode(pre_raw)
    need(pre['commit']==SOURCE,'source pin')
    plan_raw=(session/'package/plan.json').read_bytes();plan=decode(plan_raw)
    need(sha(plan_raw)==pre['plan_sha256'] and sha((session/'package/plan.sh').read_bytes())==pre['worker_plan_sha256'],'preflight plan pins')
    source,src=bundle(repo,session,pre);configs=planned(plan,pre)
    metadata_snapshot=snapshot/'bundle_manifest.json'
    manifest_path=metadata_snapshot if metadata_snapshot.exists() else Path(pre['data_dir'])/'bundle_manifest.json'
    manifest_raw=manifest_path.read_bytes();manifest=decode(manifest_raw)
    declared={d['name']:d for d in pre['data_files']}
    need(pin(manifest_raw)==dict(bytes=declared['bundle_manifest.json']['size'],sha256=declared['bundle_manifest.json']['sha256']),'metadata pin')
    if not metadata_snapshot.exists():
        metadata_snapshot.parent.mkdir(parents=True,exist_ok=True);metadata_snapshot.write_bytes(manifest_raw)
    sites={}
    for case in manifest['cases']:
        entry=case['distinct'] if case.get('bundled')=='distinct' else case
        n=entry.get('count',case.get('count'));need(type(n)is int and n>0,'site count');sites[case['name']]=n
        need(declared[entry['coordinates']]['size']==12*n and declared[entry['point_ids']]['size']==4*n,'declared input sizes')
    metadata=dict(sha256=sha(manifest_raw),sites=sites,payload_reread=False)
    result=dict(source=source,preflight=pin(pre_raw),plans={name:pin((session/'package'/name).read_bytes()) for name in ('plan.json','plan.sh')},
                budget=pre['budget'],metadata=metadata,
                planned_commands=[dict(name=c['name'],timeout=c['timeout'],cases=len(c['cases'])) for c in configs],
                expected_compile=['Release','u21','CUDA ON'],qualification='not_claimed')
    closing=('receipt.json','DONE','results/results.tar.gz')
    presence={name:(session/name).exists() for name in closing}
    result['admission']=closed(session,pre,plan,configs,src,metadata,snapshot) if all(presence.values()) else dict(state='open',present=presence,complete=False)
    if all(presence.values()):result['closing_files']={name:pin((session/name).read_bytes()) for name in ('receipt.json','DONE')}
    return result
def publication(repo,snapshot,result):
    count=0;reports=0
    for folder in ('cmd/000_mes_b_flux_identite/files/b_identite','cmd/001_mes_b_l1/files/b'):
        local=snapshot/folder
        for path in sorted((local/'brut').iterdir()):
            need(path.suffix in ('.jsonl','.err'),'native publication scope')
            need(path.read_bytes()==git(repo,PUBLISHED,PUBLIC+'resultats/'+folder+'/brut/'+path.name),'published native bytes')
            count+=1
        raw=decode((local/'rapport_b.json').read_bytes())
        published=decode(git(repo,PUBLISHED,PUBLIC+'resultats/'+folder+'/rapport_b.json'))
        need(all(raw[k]==published[k] for k in ('cas','criteres','controles','duree_s','empreintes','environnement','provenance','regime','verdict','mesure')),'published report numerical fields')
        reports+=1
    need(count==38 and reports==2,'published output inventory')
    readme=git(repo,PUBLISHED,PUBLIC+'README.md')
    need('la mémoire de l’hôte pendant la tour' in readme.decode().replace("l'hôte","l’hôte"),'published causal attribution pin')
    return dict(commit=PUBLISHED,readme=pin(readme),native_files_exact=count,reports_numeric_equal=reports)

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for arg in ('repo','session','snapshot'):ap.add_argument('--'+arg,type=Path,required=True)
    ap.add_argument('--write',action='store_true');args=ap.parse_args()
    result=run(args.repo.resolve(),args.session.resolve(),args.snapshot.resolve())
    need(result['admission']['state']=='closed','closed session required')
    result['publication']=publication(args.repo.resolve(),args.snapshot.resolve(),result)
    admission=result.pop('admission')
    for name,value in (('capture.json',result),('results.json',admission)):
        path=HERE/name
        if args.write:path.write_text(json.dumps(value,ensure_ascii=False,separators=(',',':'))+'\n')
        else:need(decode(path.read_bytes())==value,'stored '+name)
    cases=[case for command in admission['commands'] for case in command['cases']]
    print(json.dumps(dict(source_files=result['source']['selected_files_exact'],native_files=result['source']['native_files_exact'],
                         manifest_entries=admission['manifest_entries'],processes=len(cases),
                         full_passes=sum(c.get('complete_full_passes',0) for c in cases),
                         successes=sum(c['state']=='ok' for c in cases),refusals=sum(c['state']=='refus' for c in cases),
                         native_executed=False)))
