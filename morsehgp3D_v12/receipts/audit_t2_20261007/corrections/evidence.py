#!/usr/bin/env python3
"""Contre-lecture seule des traces locales du développeur ; argument = dossier de preuves."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PIN='274592a30f6961cb7702125dcd2f031ff22b7df2'
def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    need(len(sys.argv)==2,'usage: evidence.py evidence_root')
    root=Path(sys.argv[1]).resolve(); own=sha(Path(__file__)); seen={}
    def take(rel):
        p=root/rel;seen[rel]=sha(p);return p.read_bytes()
    manifest_data=subprocess.check_output(['git','-C',str(ROOT),'show',PIN+':morsehgp3D_v11/tests/tower/v10_frozen_manifest.json'])
    manifest=json.loads(manifest_data)
    for row in manifest['files']:
        data=take('src/'+row['path'])
        need(len(data)==row['bytes'] and hashlib.sha256(data).hexdigest()==row['sha256'],'frozen source mismatch')
    historical_pool=subprocess.check_output(['git','-C',str(ROOT),'show','c764e121a:morsehgp3D_v10/src/sched/pool.cpp'])
    need(historical_pool==take('src/morsehgp3D_v10/src/sched/pool.cpp'),'frozen pool mismatch')
    need(subprocess.run(['git','-C',str(ROOT),'merge-base','--is-ancestor','c764e121a','8e3b76245'],capture_output=True).returncode==0,'fix chronology')
    warnings=[];build_ids=set()
    for i in range(1,4):
        err=take('tsan_%d.err'%i).decode();warnings.append(err.count('WARNING: ThreadSanitizer: data race'))
        build_ids.update(re.findall(r'BuildId: ([a-f0-9]+)',err.split('libstdc++')[0]))
        if err:
            need('Pool::run_chunks' in err and 'Pool::parallel_for' in err and 'Location is stack of main thread' in err,'TSan report shape')
        take('tsan_dump_%d.txt'%i)
    need(warnings==[2,0,1],'TSan counts')
    for i in range(1,6):
        need(take('tsan1_%d.err'%i)==b'','single threaded TSan warning')
        take('tsan1_dump_%d.txt'%i)
    need(take('tsan1_dump_1.txt')==take('tsan_dump_2.txt'),'clean dumps differ')
    binary=root/'build_tsan/mhgp10_tower';take('build_tsan/mhgp10_tower')
    notes=subprocess.check_output(['readelf','-n',str(binary)],text=True)
    binary_id=re.search(r'Build ID: ([a-f0-9]+)',notes).group(1)
    need(build_ids=={binary_id},'TSan binary BuildId mismatch')
    flags=take('build_tsan/CMakeFiles/mhgp10_tower.dir/flags.make').decode()
    need('-fsanitize=thread' in flags,'missing TSan flags')
    take('build_tsan/CMakeFiles/mhgp10_tower.dir/link.txt');take('build_tsan.log')
    for name in ['build/mhgp10_tower','build/mhgp10_catalogue','stress.sh','test_dump_v10.corrige.py','hung_cmdline','hung_gdb.txt']:
        take(name)
    current=(ROOT/'morsehgp3D_v12/reference/test_dump_v10.py').read_bytes()
    need(take('test_dump_v10.corrige.py')==current,'corrected stress script differs from pinned gate')
    groups={}
    good='reference_diff_v10_ok nuages=190 catalogues=190 tours=640 lignes=26530'
    for folder in ['runs_avant','runs']:
        report=take(folder+'/bilan.txt').decode()
        rows=re.findall(r'^(\d+) (\d+) code=(\d+) secondes=(.*)$',report,re.M)
        seen_keys=set();classified=[]
        for round_,instance,code,seconds in rows:
            name='r%s_%s.out'%(round_,instance);need(name not in seen_keys,'duplicate run');seen_keys.add(name)
            text=take(folder+'/'+name).decode()
            if code=='0':need(good in text and 'ECART' not in text,'recorded success lacks verdict')
            elif code=='1':need('ECART ' in text,'recorded failure lacks discrepancy')
            classified.append({'run':name,'code':int(code),'discrepancy_lines':sum(line.startswith('ECART ') for line in text.splitlines())})
        unlisted=[]
        for path in sorted((root/folder).glob('r*.out')):
            if path.name not in seen_keys:
                text=take(folder+'/'+path.name).decode()
                unlisted.append({'run':path.name,'exit_unknown':True,'success_line':good in text,
                                 'discrepancy_lines':sum(line.startswith('ECART ') for line in text.splitlines())})
        groups[folder]={'recorded':len(rows),'exit_counts':dict(Counter(code for _,_,code,_ in rows)),
                        'durations_absent':all(not seconds for *_,seconds in rows),'unlisted_outputs':unlisted,
                        'first_six_rounds_exit_counts':dict(Counter(code for round_,_,code,_ in rows if int(round_)<=6))}
    need(groups['runs']['recorded']==45 and groups['runs']['exit_counts']=={'0':45},'corrected stress counts')
    need(groups['runs_avant']['first_six_rounds_exit_counts']=={'0':16,'1':1,'124':1},'published prefix counts')
    need(all(sha(root/name)==digest for name,digest in seen.items()) and sha(Path(__file__))==own,'evidence changed')
    print(json.dumps({'schema':'audit.t2.corrections.evidence.v1','pin':PIN,'witness_sha256':own,
        'frozen_manifest_sha256':hashlib.sha256(manifest_data).hexdigest(),'frozen_source_files_verified':len(manifest['files']),
        'frozen_commit':manifest['source_commit'],'later_pool_fix':'8e3b76245f43fb9c98b974e1720e68cf929aa285',
        'tsan_warning_counts_multithread':warnings,'tsan_single_thread_clean_stderr':5,
        'tsan_binary_build_id':binary_id,'one_clean_single_and_multi_dump_identical':True,
        'corrected_gate_matches_pin':True,'stress':groups,'local_evidence_sha256':seen,
        'before_after_unchanged':True,'binaries_executed':False,'gcp_used':False,
        'scope':'existing developer logs read; no independent stress/TSan rerun; no prebuild source attestation reconstructed'},sort_keys=True,indent=2))
if __name__=='__main__':main()
