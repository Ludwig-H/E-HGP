#!/usr/bin/env python3
"""Contrepreuve MES-P, sorties publiques rejouees par executable Python ; aucun moteur HGP."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

HERE = Path(__file__).resolve().parent


def need(value, why):
    if not value:
        raise ValueError(why)


def load(data, name):
    module = types.ModuleType(name)
    exec(compile(data, name, 'exec'), module.__dict__)
    return module


def proof(repo):
    pins = json.loads((HERE / 'sources.json').read_text())
    sources = {}
    for item in pins['inputs']:
        data = subprocess.check_output(['git','-C',str(repo),'show',item['pin']+':'+item['path']])
        need(len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256'],item['path'])
        sources[item['path']]=data
    source=pins['inputs'][0]['path']
    before=load(sources[source],'mes_p_before')
    real_path=pins['inputs'][-1]['path']
    nominal=[json.loads(line) for line in sources[real_path].splitlines()]
    # Sortie REELLE v11 publiee, quatre passes indexees 1..4 ; pas de fabrication du controle nominal.
    need([r.get('phase') for r in nominal]==['cloud','pass','pass','pass','domain','pass','full','exit'], 'forme archive')
    with tempfile.TemporaryDirectory(prefix='mes_p_admission_') as directory:
        root=Path(directory); target=root/source; target.parent.mkdir(parents=True)
        target.write_bytes(sources[source])
        analysis_path='morsehgp3D_v12/microbancs/mes_p_petits/analyse_p.py'
        analysis_target=root/analysis_path
        analysis_target.write_bytes(sources[analysis_path])
        for extra in (['--check'],[]):
            subprocess.run(['git','apply',*extra,str(HERE/'proposition.patch')],cwd=root,check=True)
        patched=target.read_bytes()
        need(hashlib.sha256(patched).hexdigest()==pins['proposed_sha256'],'patch')
        after=load(patched,'mes_p_proposed')
        need(hashlib.sha256(analysis_target.read_bytes()).hexdigest()==pins['proposed_analyse_sha256'],'patch analyse')
        stub=root/'probe'
        stub.write_text('#!'+sys.executable+'\nimport pathlib,sys\n'
                        'sys.stdout.buffer.write(pathlib.Path(__file__).with_name("stdout").read_bytes())\n'
                        'sys.exit(int(pathlib.Path(__file__).with_name("code").read_text()))\n')
        stub.chmod(0o700)
        out=[]

        def replay(name,rows,expected=False,passes=4,k=5,threads=48,code=0,raw=None):
            data=raw if raw is not None else ('\n'.join(json.dumps(r) for r in rows)+'\n').encode()
            (root/'stdout').write_bytes(data);(root/'code').write_text(str(code))
            old=before.run_cloud(str(stub),str(root),str(root),'public_fixture',k,threads,passes,5)
            new=after.run_cloud(str(stub),str(root),str(root),'public_fixture',k,threads,passes,5)
            kept=(root/('public_fixture_k%d_f%d.jsonl'%(k,threads))).read_bytes()
            need((new['chaud'] is not None)==expected and kept==data,'verdict '+name)
            out.append(dict(name=name,process_code=code,requested_passes=passes,
                            old_admitted=old['chaud'] is not None,proposed_admitted=new['chaud'] is not None,
                            old_warm_seconds=old['chaud'],proposed_warm_seconds=new['chaud'],
                            proposed_admission=new['admission'],raw_preserved=True))

        replay('real_published_v11_P4',nominal,expected=True)
        two=copy.deepcopy([nominal[0],nominal[1],*nominal[-4:]])
        two[-3]['pass']=2
        replay('real_layout_P2_requested_P2',two,expected=True,passes=2)
        replay('two_passes_requested_four',two)
        rows=copy.deepcopy(nominal)
        for row in rows:
            if row['phase']=='pass':row['status']='refused'
            if row['phase'] in ('full','exit'):row.update(status='resource_exhausted',reason='memory_budget')
        replay('explicit_refusal_process_code_zero',rows)
        for label,value in [('duplicate',1),('boolean',True),('float',2.0),('negative',-2)]:
            rows=copy.deepcopy(nominal);rows[2]['pass']=value
            replay('pass_index_'+label,rows)
        for label,value in [('boolean',True),('negative',-1),('float',123.0)]:
            rows=copy.deepcopy(nominal)
            for row in rows:
                if row['phase']=='pass':row['wall_ns']=value
            replay('wall_'+label,rows)
        rows=copy.deepcopy(nominal)
        for row in rows:
            if row['phase']=='pass':
                for field in ('wall_ns','index_ns','forest_ns'):row.pop(field)
        replay('domain_time_only',rows)
        replay('missing_final_exit',nominal[:-1])
        replay('missing_final_full',nominal[:-2]+[nominal[-1]])
        replay('wrong_requested_K',nominal,k=10)
        replay('wrong_requested_threads',nominal,threads=4)
        for field,value in [('coord_bits',24),('coord_bits',True),('optimizations',0),('workers',True),('wall_ns',1)]:
            rows=copy.deepcopy(nominal);rows[-2][field]=value
            replay('full_'+field+'_'+str(value),rows)
        rows=copy.deepcopy(nominal);rows[0]['sites']=True
        replay('cloud_boolean_sites',rows)
        rows=copy.deepcopy(nominal);rows[-4]['leaf_size']=24
        replay('wrong_leaf',rows)
        replay('nonzero_process_code',nominal,code=2)
        encoded=('\n'.join(json.dumps(r) for r in nominal)+'\n').encode()
        replay('invalid_json_line',nominal,raw=b'NOT_JSON\n'+encoded)
        replay('non_object_line',nominal,raw=b'[]\n'+encoded)
        replay('invalid_utf8',nominal,raw=b'\xff\n'+encoded)
        replay('duplicate_json_key',nominal,raw=encoded.replace(b'"pass": 1',b'"pass": 1, "pass": 1',1))
        # Le lecteur CLI doit rendre visible un code processus zero refuse par le pilote,
        # meme si le brut se termine par exit ok/none. Les selections de cohorte restent identiques.
        analysis_before=root/'analyse_before.py';analysis_before.write_bytes(sources[analysis_path])
        refusal=dict(nuage='knn_300',k=5,fils=1,code=0,sites=300,passes=[],froid=None,chaud=None,
                     admission='processus_ou_protocole_invalide')
        (root/'knn_300_k5_f1.jsonl').write_text('{"phase":"exit","status":"ok","reason":"none"}\n')
        def analysis_cli(script,takes):
            report=root/'mes_p.json';report.write_text(json.dumps(dict(prises=takes)))
            done=subprocess.run([sys.executable,'-B','-S',*(['-O'] if sys.flags.optimize else []),
                                 str(script),str(report),'--brut',str(root)],capture_output=True,text=True)
            need(done.returncode==0,'analyse CLI')
            return done.stdout
        old_single=analysis_cli(analysis_before,[refusal])
        new_single=analysis_cli(analysis_target,[refusal])
        need('Prises : 1 ; rendues : 0 ; en echec ou expirees : 0.' in old_single,'ancien diagnostic')
        need('Prises : 1 ; rendues : 0 ; en echec ou expirees : 1.' in new_single and
             '| knn_300 | 5 | 1 | 300 | 0 | processus_ou_protocole_invalide |' in new_single and
             '| 0 | ok / none |' not in new_single,'refus protocole visible')
        slopes={1:100e-6,4:60e-6,48:10e-6}
        cohort=[dict(nuage='knn_%d'%n,k=5,fils=f,code=0,sites=n,chaud=0.001+n*b)
                for f,b in slopes.items() for n in (100,200)]
        cohort.extend(dict(nuage='knn_300',k=5,fils=f,code=0,sites=300,chaud=0.001+300*slopes[f]) for f in (4,48))
        cohort.append(refusal)
        old_cohort=analysis_cli(analysis_before,cohort);new_cohort=analysis_cli(analysis_target,cohort)
        extract=lambda value:[line for line in value.splitlines() if ' fils | 2 | ' in line]
        wanted=['| 1 fils | 2 | 0 | 1.00 | 100.00 |','| 4 fils | 2 | 1 | 1.00 | 60.00 |',
                '| 48 fils | 2 | 1 | 1.00 | 10.00 |']
        need(extract(old_cohort)==wanted and extract(new_cohort)==wanted,'cohorte changee')
        analysis_checks=dict(single_old_header=old_single.splitlines()[2],single_proposed_header=new_single.splitlines()[2],
                             proposed_failure_row=next(line for line in new_single.splitlines() if line.startswith('| knn_300 |')),
                             common_cohort_unchanged=True,cohort_rows=wanted)
        # Gate existante complete : y compris le delai qui doit encore conserver les deux temps diagnostiques,
        # tuer le groupe du double et ne jamais fabriquer un temps chaud. Aucun benchmark natif.
        folder=target.parent
        for path,data in sources.items():
            if path.endswith('/test_pilote_p.py'):(folder/Path(path).name).write_bytes(data)
        done=subprocess.run([sys.executable,'-B','-S',*(['-O'] if sys.flags.optimize else []),str(folder/'test_pilote_p.py')],
                            stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(done.returncode==0,'porte existante : '+done.stderr.decode())
        gate=json.loads(done.stdout)
        need(gate=={'porte':'mes_p','cas':5,'ecarts':0},'porte resultat')
        return dict(base_pin=pins['inputs'][0]['pin'],pilot_sha256=pins['inputs'][0]['sha256'],
                    proposed_sha256=pins['proposed_sha256'],nominal_fixture_sha256=pins['inputs'][-1]['sha256'],
                    cases=out,analysis_cli=analysis_checks,proposed_analyse_sha256=pins['proposed_analyse_sha256'],existing_gate=gate,native_runs=0,builds=0,cloud_calls=0,
                    historical_H_838_complete_successes_not_revoked=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path.cwd())
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    encoded=json.dumps(proof(args.repo),indent=1,sort_keys=True)+'\n'
    if args.out:args.out.write_text(encoded)
    else:sys.stdout.write(encoded)


if __name__=='__main__':
    main()
