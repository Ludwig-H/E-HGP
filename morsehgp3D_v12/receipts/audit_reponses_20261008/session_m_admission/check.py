#!/usr/bin/env python3
"""Bounded synthetic FULLM replay; no probe, binary, data or cloud invocation."""
import argparse
import ast
import copy
import json
from pathlib import Path
import subprocess
import types
import reader as r

def dumps(rows):return ('\n'.join(json.dumps(x,sort_keys=True) for x in rows)+'\n').encode()

def nominal(metadata,old,repo,commit):
    path='morsehgp3D_v12/microbancs/outils/test_lecteur_full.py'
    src=subprocess.check_output(['git','show',commit+':'+path],cwd=repo).decode()
    node=next(x for x in ast.parse(src).body if isinstance(x,ast.FunctionDef) and x.name=='overlapped_row')
    ns=dict(lf=old,LABEL='synthetic',SITES=1,SHA='ab'*32)
    exec(compile(ast.Module(body=[node],type_ignores=[]),'<pinned native-format fixture>','exec'),ns)
    specs,sizes=r.specs(metadata);raws={}
    for filename,(group,k,device,names) in specs.items():
        rep=int(filename.rsplit('_r',1)[1].split('.')[0]);floor=16*sum(sizes[n] for n in set(names));peak=floor+100
        rows=[dict(phase='open',status='ok',reason='none',wall_ns=5,budget_appareil='partage')] if device else []
        for i,name in enumerate(names):
            wall=3_000_000 if not i else 2_000_000
            if group==r.GROUPS[0] and i:
                wall=(1_000_000 if i<=5 else 101_000_000) if rep<3 else 2_000_000
            row=ns['overlapped_row'](i,wall)
            row['pass']=row.pop('pass_');row.update(trame=name[-23:],voie='device' if device else 'cpu',
                sites=sizes[name],kmax=k,full_sha256=r.sha((name+':'+str(k)).encode()),pic_octets=peak,
                cpu_ns=wall*2,rss_max_octets=peak*2,pic_appareil_octets=0)
            row['memoire_octets']={x:[floor,peak] for x in ('P','C','tour')}
            g=wall//2
            row['fins_par_ordre_ns']=[[g,g+1,g+2,0,g+3]]+[[g-10*j,g,g+1,g+2,g+3] for j in range(1,k)]
            if not device:row.update(appareil_octets=0,epinglee_octets=0)
            rows.extend([row,{'phase':'liberation','pass':i,'liberation_ns':3}])
        rows.append(dict(phase='exit',status='ok',reason='none'));raws[filename]=dumps(rows)
    return raws

def report_for(computed,c):
    env=dict(nvcc='synthetic CUDA',cmake='synthetic CMake',gpu='synthetic GPU',gpu_apps='')
    return dict(mesure='MES-FULL',budget_ns=r.LIMIT,
        options=dict(fils=48,processus=5,passes=10,jobs=44,delai=900,essai=False,sequentiel=False,sonde=None,
                     src='synthetic',travail='synthetic',donnees='synthetic',sortie='synthetic',archive_v12set='synthetic'),
        refus=[],environnement_avant=env,environnement_apres=copy.deepcopy(env),duree_s=1.0,
        provenance=dict(sonde_sha256='ab'*32,pilote_sha256=c['sources']['microbancs/mes_full/pilote_full.py'],
                        lecteur_sha256=c['sources']['microbancs/outils/lecteur_full.py'],
                        cmake=['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21','MHGP12_ENABLE_CUDA:BOOL=ON']),
        **copy.deepcopy(computed))

def first_full(raw,fn):
    rows=[r.json_bytes(x) for x in raw.splitlines()];row=next(x for x in rows if x['phase']=='full');fn(row)
    return dumps(rows)

def main():
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--reader-patch',type=Path,required=True)
    a=p.parse_args();c=r.json_bytes((r.HERE/'capture.json').read_bytes());metadata=r.json_bytes((r.HERE/'input_metadata.json').read_bytes())
    old,new,pilot=r.readers(a.reader_patch,a.repo);raws=nominal(metadata,old,a.repo,c['source_commit'])
    computed,detail=r.reconstruct(raws,metadata,old,new,pilot);report=report_for(computed,c);r.review(report,computed,c)
    v=detail['groups']['k5_appareil']['ng00']
    r.need((v['pooled_median_ns'],v['median_process_medians_ns'],v['max_process_median_ns'],v['max_raw_ns'])==
           (2_000_000,1_000_000,2_000_000,101_000_000),'four statistics not distinguished')
    r.need(computed['verdict']=='tenu','contract changed by max raw')
    rejected={}
    def reject(name,rawchange=None,reportchange=None):
        rr=raws.copy();rp=copy.deepcopy(report)
        if rawchange:rawchange(rr)
        if reportchange:reportchange(rp)
        try:
            cc,_=r.reconstruct(rr,metadata,old,new,pilot);r.review(rp,cc,c)
        except ValueError as e:rejected[name]=str(e)
        else:raise ValueError('counterexample admitted: '+name)
    reject('missing_process',lambda d:d.pop('k5_ng00_r0.jsonl'))
    reject('extra_process',lambda d:d.update(foreign=d['k5_ng00_r0.jsonl']))
    reject('sequence_foreign_label',lambda d:d.update({'v12set_r0.jsonl':first_full(d['v12set_r0.jsonl'],lambda x:x.update(trame='foreign'))}))
    reject('cpu_device_fingerprint',lambda d:d.update({'cpu_ng00_r0.jsonl':first_full(d['cpu_ng00_r0.jsonl'],lambda x:x.update(full_sha256='00'*32))}))
    def lowpeak(x):
        x['pic_octets']=16*x['sites']-1;x['memoire_octets']={s:[0,x['pic_octets']] for s in ('P','C','tour')}
    reject('input_peak_floor',lambda d:d.update({'k5_ng00_r0.jsonl':first_full(d['k5_ng00_r0.jsonl'],lowpeak)}))
    reject('shared_device_peak',lambda d:d.update({'k5_ng00_r0.jsonl':first_full(d['k5_ng00_r0.jsonl'],lambda x:x.update(pic_appareil_octets=1))}))
    reject('shared_capacity_exceeds_peak',lambda d:d.update({'k5_ng00_r0.jsonl':first_full(d['k5_ng00_r0.jsonl'],lambda x:x.update(appareil_octets=x['pic_octets']+1))}))
    reject('pooled_median_forged',reportchange=lambda x:x['statistiques']['k5_appareil']['ng00'].update(mediane_ns=1_000_000))
    reject('not_known_idle',reportchange=lambda x:x['environnement_apres'].update(gpu_apps=None))
    reject('reported_refusal',reportchange=lambda x:x['refus'].append('synthetic process failed'))
    print(json.dumps(dict(nominal=dict(processes=detail['processes'],passes=detail['passes'],warm_passes=detail['warm_passes'],verdict=computed['verdict']),
        four_statistics={k:v[k] for k in ('pooled_median_ns','median_process_medians_ns','max_process_median_ns','max_raw_ns')},
        counterexamples=rejected),sort_keys=True,indent=2))

if __name__=='__main__':main()
