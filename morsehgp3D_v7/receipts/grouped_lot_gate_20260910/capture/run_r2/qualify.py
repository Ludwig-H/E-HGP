#!/usr/bin/env python3
"""Frozen constructor regression: grouped-lot mutants and batched host front."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
only_mutants = sys.argv[1:] == ['--mutants-only']
OUT = Path(__file__).resolve().parent / ('run_r2' if only_mutants else 'run_r1')
OUT.mkdir(exist_ok=False)
shutil.copyfile(__file__, OUT / 'qualify.py')
BASE = OUT / 'baseline'
FILES = [p for p in (ROOT / 'morsehgp3D_v7/src').rglob('*') if p.is_file() and p.suffix in ('.hpp','.cuh')]
FILES += [ROOT / ('morsehgp3D_v7/' + p) for p in (
    'oracle/local_plateau_oracle.hpp', 'tests/full_ball_tower_gate.cpp',
    'tests/full_ball_work_gate.cpp', 'tests/witness_front_gate.cpp')]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')

before = {p.relative_to(ROOT).as_posix():sha(p) for p in FILES}
for source in FILES:
    dest = BASE / source.relative_to(ROOT)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source,dest)
save(OUT / 'sources_before.json',before)
commands = []
def command(name, argv, expected=0):
    row = dict(name=name, argv=list(map(str,argv)), started_epoch=time.time(), expected_code=expected)
    save(OUT / (name+'.intent.json'),row)
    with (OUT / (name+'.stdout')).open('xb') as stdout, (OUT / (name+'.stderr')).open('xb') as stderr:
        p = subprocess.Popen(row['argv'],cwd=ROOT,stdout=stdout,stderr=stderr,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
        row['pid'] = p.pid
        row['exit_code'] = p.wait()
    row.update(ended_epoch=time.time(), stdout_sha256=sha(OUT/(name+'.stdout')),
               stderr_sha256=sha(OUT/(name+'.stderr')), closed=True)
    commands.append(row); save(OUT/(name+'.command.json'),row)
    print(name+': '+str(row['exit_code']),flush=True)
    if row['exit_code'] != expected:
        raise RuntimeError(name+' unexpected code')

flags=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-pthread',
       '-isystem',str(ROOT/'build/v7_boost_gate/extracted/usr/include')]
def build_run(name, tree, entry, san=False, expected=0):
    binary=OUT/(name+'.bin')
    opts=['-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined','-no-pie'] if san else ['-O2']
    command(name+'_compile',flags+opts+['-MMD','-MF',str(OUT/(name+'.d')),
        str(tree/'morsehgp3D_v7/tests'/entry),'-o',str(binary)])
    pin=sha(binary)
    command(name+'_run',[binary,'--selftest'],expected)
    if sha(binary)!=pin: raise RuntimeError('binary drift')
    return pin

result={'status':'failed','GCP_used':False,'CUDA_executed':False,'commands':commands}
try:
    command('compiler',['g++','--version'])
    result['nominal']={}
    for label, entry in ([] if only_mutants else [('tower','full_ball_tower_gate.cpp'),('work','full_ball_work_gate.cpp'),('front','witness_front_gate.cpp')]):
        for san in (False,True):
            name=label+('_san' if san else '_o2')
            result['nominal'][name]=build_run(name,BASE,entry,san)
        if (OUT/(label+'_san_run.stdout')).read_bytes()!=(OUT/(label+'_o2_run.stdout')).read_bytes():
            raise RuntimeError('O2/SAN output differs')
        command(label+'_argument',[OUT/(label+'_o2.bin'),'--unknown'],2)
    # Mechanical test generation; source under test is never edited in place.
    mutations={
      'drop_growth_grouped':('if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));',
                             'if (action.parents.size() != 1) batch.actions.push_back(std::move(action));'),
      'drop_inert_grouped':('for (size_t b = 0; b < blocks.size(); ++b) {\n      require(anchors[blocks[b].ball]',
                            'for (size_t b = 0; b < blocks.size(); ++b) {\n      if (blocks[b].roots.size() == 1 && !blocks[b].contribution && !blocks[b].interior) continue;\n      require(anchors[blocks[b].ball]'),
      'drop_growth_singleton':('if (action.parents.size() != 1 || !action.contributions.empty()) {',
                               'if (action.parents.size() != 1) {'),
      'drop_inert_singleton':('anchors[block.ball] = target;  // All representatives and the whole lot are closed.',
                              'if (block.roots.size() != 1 || block.contribution || block.interior) anchors[block.ball] = target;'),
      'strict_radius':('require(cmp <= 0, "full_ball_radius_increased");','require(cmp < 0, "full_ball_radius_increased");'),
      'wrong_vertical_cut':('upper.lower_nodes[root], cut, closed);','upper.lower_nodes[root], ExactLevel{U192{}, 1}, true);')}
    result['mutants']={}
    for name,(old,new) in mutations.items():
        tree=OUT/name; shutil.copytree(BASE,tree)
        header=tree/'morsehgp3D_v7/src/forest/full_ball_tower.hpp'
        text=header.read_text()
        if text.count(old)!=1: raise RuntimeError('mutation site not unique: '+name)
        header.write_text(text.replace(old,new))
        result['mutants'][name]=dict(binary_sha256=build_run(name,tree,'full_ball_tower_gate.cpp',expected=1),
                                     header_sha256=sha(header))
    after={name:sha(ROOT/name) for name in before}
    save(OUT/'sources_after.json',after)
    if before!=after: raise RuntimeError('active source drift')
    result['status']='passed'
except BaseException as error:
    result['error']=type(error).__name__+': '+str(error)
finally:
    save(OUT/'receipt.json',result)
print(json.dumps({k:v for k,v in result.items() if k in ('status','error')}),flush=True)
raise SystemExit(0 if result['status']=='passed' else 1)
