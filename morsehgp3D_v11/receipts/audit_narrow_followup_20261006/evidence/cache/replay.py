#!/usr/bin/env python3
"""Reproduce stale source/binary attribution in gpu_ab --variants; stdlib/process stubs only."""
import argparse,contextlib,gzip,hashlib,importlib.util,io,json,sys,tarfile,tempfile
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parent
PIN='05db6f5b78d43a2657e519bc02841738d4dd02c1'

def need(ok,detail):
    if not ok:raise RuntimeError(detail)

def digest(data):return hashlib.sha256(data).hexdigest()

def archive(path,payload):
    # Fixed metadata keeps replay SHA deterministic.
    memory=io.BytesIO()
    with gzip.GzipFile(filename='',mode='wb',fileobj=memory,mtime=0) as compressed:
        with tarfile.open(fileobj=compressed,mode='w') as tar:
            member=tarfile.TarInfo('morsehgp3D_v11/src/catalogue/marker.hpp')
            member.size=len(payload);member.mtime=0
            tar.addfile(member,io.BytesIO(payload))
    path.write_bytes(memory.getvalue())


def experiment():
    spec=importlib.util.spec_from_file_location('audit_gpu_ab',ROOT/'gpu_ab.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    calls=[]
    with tempfile.TemporaryDirectory(prefix='mhgp11-audit-variants-') as temporary:
        root=Path(temporary);data=root/'data';data.mkdir();work=root/'work';work.mkdir()
        reference=root/'reference.stub';reference.write_bytes(b'REFERENCE-NATIVE-PROCESS-STUB')
        extracted=work/'src_C/morsehgp3D_v11/src/catalogue/marker.hpp'
        extracted.parent.mkdir(parents=True);extracted.write_bytes(b'OLD-SOURCE')
        cached=work/'b_cuda_C/mhgp11_full_bench';cached.parent.mkdir(parents=True)
        cached.write_bytes(b'BINARY-FROM-OLD-SOURCE-STUB')
        source_archive=data/'C_src.tar.gz'

        def native(argv,timeout):
            need(argv[0] in (str(reference),str(cached)),'any build/native command outside the stub refused')
            calls.append(argv[0])
            Path(argv[3]).write_bytes(b'CANONICAL-PROTOCOL-STUB:'+Path(argv[1]).name.encode())
            passes=int(argv[12]) if len(argv)==13 else 1
            events=[{'phase':'domain','catalogue_work':{'prefixes':1},'leaf_batch':{}},
                    {'phase':'full','status':'ok'}, {'phase':'exit','status':'ok'}]
            if passes>1:
                events += [{'phase':'pass','pass':i,'status':'ok','wall_ns':1,'domain_ns':1,'forest_ns':1,
                            'batch_executor_ns':1,'batch_device_init_ns':1} for i in range(1,passes+1)]
            return 0,'\n'.join(json.dumps(e) for e in events),'',0

        results=[]
        for index,payload in enumerate((b'OLD-SOURCE',b'NEW-SOURCE')):
            archive(source_archive,payload)
            out=root/('out'+str(index))
            argv=['gpu_ab.py','--bench',str(reference),'--data',str(data),'--work',str(work),'--out',str(out),
                  '--variants','new,C','--modes','gpu=81915','--reps','1','--workers','48','--warm-passes','2']
            # Module.run is the only process boundary used; any unmocked subprocess call is forbidden.
            with patch.object(module,'run',side_effect=native), patch.object(module.subprocess,'run',side_effect=RuntimeError('subprocess forbidden')),patch.object(sys,'argv',argv),contextlib.redirect_stdout(io.StringIO()):
                code=module.main()
            report=json.loads((out/'gpu_ab_report.json').read_text())
            logged=[entry for entry in report['build'] if entry['step']=='variant']
            need(len(logged)==1 and logged[0]['archive_sha256']==module.sha256(source_archive),'logged archive identity')
            need(code==0 and report['verdict']=='conforme' and report['refusals']==[],'judge accepted cases')
            results.append({'exit_code':code,'verdict':report['verdict'],'archive_source_sha256':digest(payload),
                'logged_archive_matches_current_bytes':True,'logged_archive_sha256':logged[0]['archive_sha256'],'extracted_source_sha256':digest(extracted.read_bytes()),
                'cached_binary_sha256':report['bench_sha256']['C'],
                'reported_source_matches_extracted':payload==extracted.read_bytes(),
                'build_steps':[{k:v for k,v in e.items() if k in ('step','name','sha256','archive_sha256')} for e in report['build']],
                'cold_calls':len(report['cold']),'warm_calls':len(report['warm'])})
        need(results[0]['reported_source_matches_extracted'] and not results[1]['reported_source_matches_extracted'],
             'archive replacement distinguishes stale extraction')
        need(results[0]['cached_binary_sha256']==results[1]['cached_binary_sha256']==digest(b'BINARY-FROM-OLD-SOURCE-STUB'),
             'old binary reused')
        return results,len(calls)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true');args=parser.parse_args()
    results,calls=experiment()
    out={'schema':'audit_gpu_ab_variant_stale_cache_v1','source_pin':PIN,
         'source_path':'morsehgp3D_v11/bench/gpu_ab.py','source_sha256':digest((ROOT/'gpu_ab.py').read_bytes()),
         'dependency_path':'morsehgp3D_v11/bench/ab_g4.py','dependency_sha256':digest((ROOT/'ab_g4.py').read_bytes()),
         'native_executions':0,'cloud_actions':0,'mock_process_calls':calls,'cases':results,
         'limits':['Process output and binary files are protocol stubs, not geometric/native evidence.',
                   'This demonstrates a reusable-cache attribution defect, not a false attribution in closed real sessions.',
                   'No native build/run, cloud action, LiDAR data or developer modification.']}
    text=json.dumps(out,sort_keys=True,indent=2)+'\n'
    if args.capture:(ROOT/'summary.json').write_text(text)
    else:need((ROOT/'summary.json').read_text()==text,'summary differs')
    print(text,end='')
