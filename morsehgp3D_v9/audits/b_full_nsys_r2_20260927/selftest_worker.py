#!/usr/bin/env python3
"""Offline R2 checks against original FULL evidence; no build, cloud or subprocess."""
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
from unittest.mock import patch
import worker as w

ROOT = Path(__file__).resolve().parents[3]
RECEIPT = ROOT/'morsehgp3D_v9/receipts/g4_core_warm_20260927'


def main():
    positive,rejected = 0,0
    def yes(value,label):
        nonlocal positive
        w.need(value,label);positive += 1
    def no(call,label):
        nonlocal rejected
        try:call()
        except (ValueError,OSError,KeyError):rejected += 1
        else:raise ValueError('negative admitted: '+label)
    def forbidden(*_args,**_kwargs):raise ValueError('selftest forbids real subprocesses')
    manifest = json.loads((RECEIPT/'source_manifest.json').read_bytes())
    payload = w.load_payload(ROOT,manifest)
    cases = json.loads((RECEIPT/'plan.json').read_bytes())['cases'];case = cases[0]
    raw = (RECEIPT/'vm/probe_0.stdout').read_text()
    sources = (Path(w.__file__),Path(__file__),ROOT/'gcp-migration/tower_worker_v9.py',
               ROOT/'gcp-migration/full_probe_worker_v7.py')
    pins = {str(path):w.sha(path) for path in sources}
    with tempfile.TemporaryDirectory(prefix='mhgp9-full-nsys-r2-test-') as name, \
         patch.object(subprocess,'run',side_effect=forbidden),patch.object(subprocess,'Popen',side_effect=forbidden):
        folder = Path(name)
        probe = w.native_probe(raw,payload,case)
        yes(w.native_probe('Nsight starting\n'+raw+'\nReport generated\n',payload,case) == probe,'native JSON in profiler logs')
        no(lambda:w.native_probe(raw+raw,payload,case),'duplicate native output')
        no(lambda:w.native_probe('report generated\n',payload,case),'missing native output')
        for field,value in (('tower_digest','0'*16),('presentation_digest','0'*16),('status','refused')):
            changed = deepcopy(probe);changed[field] = value
            no(lambda:w.native_probe(json.dumps(changed),payload,case),'changed '+field)
        changed = deepcopy(probe);changed['frames']['results'][3]['orders'][4]['parents'] += 1
        no(lambda:w.native_probe(json.dumps(changed),payload,case),'only fourth FULL result changed')
        no(lambda:w.load_payload(ROOT,dict(manifest,**{'gcp-migration/tower_worker_v9.py':'0'*64})),'unpinned validator')
        artifact = folder/'qualified_probe';artifact.write_bytes(b'new native build')
        artifacts = {str(artifact):w.sha(artifact)}
        yes(w.pinned_artifacts(artifacts) == artifacts,'exact rebuilt binary')
        no(lambda:w.pinned_artifacts({str(artifact):'0'*64}),'wrong binary hash')
        no(lambda:w.pinned_artifacts({str(folder/'absent'):'0'*64}),'missing binary')
        link = folder/'link';link.symlink_to(artifact)
        no(lambda:w.pinned_artifacts({str(link):w.sha(artifact)}),'symlink binary')
        dbpath = folder/'trace.sqlite'
        with sqlite3.connect(dbpath) as db:db.execute('CREATE TABLE CUPTI_ACTIVITY_KIND_KERNEL (start INTEGER,end INTEGER)')
        no(lambda:w.sqlite_summary(dbpath),'empty kernel table')
        with sqlite3.connect(dbpath) as db:db.execute('INSERT INTO CUPTI_ACTIVITY_KIND_KERNEL VALUES (1,2)')
        yes(w.sqlite_summary(dbpath) == {'kernel_tables':{'CUPTI_ACTIVITY_KIND_KERNEL':1}},'actual CUDA activity')
        build,root = folder/'build',folder/'source'
        argv = payload.probe_command(build,root,case)
        command = w.profile_command(folder/'nsys',folder/'report',argv)
        yes(command[8:] == argv and argv[:2] == [str(build/payload.PROBE_TARGET),str(root/case['file'])] and
            '--frames=4' in argv and '--sample=none' in command and '--cpuctxsw=none' in command,
            'profile direct freshly built four-frame FULL command')
        tools = {'cmake':'cmake','g++':'g++','nvcc':'nvcc'}
        yes(payload.build_command(tools,build,False) ==
            ['cmake','--build',str(build),'--target','mhgp9_tower_probe','--parallel','48'] and
            payload.configure_command(tools,root,build) == ['cmake','-S',str(root/'morsehgp3D_v9'),'-B',str(build),
                '-DCMAKE_BUILD_TYPE=Release','-DBOOST_ROOT=/usr','-DCMAKE_CXX_COMPILER=g++',
                '-DMHGP9_ENABLE_CUDA=ON','-DCMAKE_CUDA_COMPILER=nvcc'],'unchanged narrow native CUDA build')
        build.mkdir();(build/'CMakeCache.txt').write_text('fixture configure')
        archive = folder/'libexternal.a';archive.write_bytes(b'external link input')
        for target in ('mhgp9_tower_probe','mhgp9_chain','mhgp9_gen','mhgp9_gpu'):
            directory = build/'CMakeFiles'/(target+'.dir');directory.mkdir(parents=True)
            for item in ('flags.make','build.make','link.txt','DependInfo.cmake'):
                (directory/item).write_text(str(archive) if item == 'link.txt' else 'direct flags')
            (directory/'fixture.o').write_bytes(b'object')
            (directory/'fixture.o.d').write_text('fixture.o: fixture.cpp')
            if target != 'mhgp9_tower_probe':(build/('lib'+target+'.a')).write_bytes(b'native archive')
        recipe = w.build_recipe_pins(build)
        yes(len(recipe) == 18 and str(archive) in recipe,'direct flags accepted with zero rsp, external archive pinned')
        rsp = build/'CMakeFiles/mhgp9_gpu.dir/includes.rsp';rsp.write_text('-I/native/include')
        recipe = w.build_recipe_pins(build)
        yes(str(rsp) in recipe,'present response file pinned')
        rsp.write_text('-I/changed/include')
        no(lambda:w.need(recipe == w.build_recipe_pins(build),'recipe drift'),'changed response file')
        products = w.build_product_pins(build)
        yes(len(products) == 11,'objects, depfiles and three native libraries closed')
        (build/'libmhgp9_chain.a').write_bytes(b'changed library')
        no(lambda:w.need(products == w.build_product_pins(build),'product drift'),'changed compiled library')
        def replay_preflight(directory,corrupt=False):
            directory.mkdir()
            def replay(name,argv):
                row = json.loads((RECEIPT/'vm'/(name+'.command.json')).read_bytes())
                w.need(argv[:2] == [payload.TIME,'-v'] and argv[4:] == row['argv'][4:],'original preflight CLI tail')
                (directory/(name+'.command.json')).write_text(json.dumps(row))
                (directory/(name+'.stderr')).write_bytes((RECEIPT/'vm'/(name+'.stderr')).read_bytes())
                value = json.loads((RECEIPT/'vm'/(name+'.stdout')).read_bytes())
                if corrupt and name == 'preflight_deferral':value['tower_digest'] = '0'*16
                return json.dumps(value)
            return w.run_preflights(replay,directory,build/payload.PROBE_TARGET,payload,cases)
        preflight = replay_preflight(folder/'preflight_good')
        yes(preflight['GPU_executed'] and preflight['engine_equal'] and preflight['deferral_equal'] and
            preflight['sites'] == 1500 and preflight['frames'] == 4,'three original native GPU preflights replay')
        no(lambda:replay_preflight(folder/'preflight_bad',True),'changed reduced-slab FULL object')
    yes(pins == {path:w.sha(path) for path in pins},'source closure')
    print(json.dumps(dict(status='passed',positive=positive,rejected=rejected,GCP_used=False,
                         real_subprocesses=0,source_sha256=pins),sort_keys=True))


if __name__ == '__main__':main()
