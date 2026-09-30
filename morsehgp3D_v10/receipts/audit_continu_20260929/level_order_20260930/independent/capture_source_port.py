"""Pin real source blobs and preserve the expected generic-width compile refusal."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent
repository = Path('/workspaces/E-HGP/build/v9-open-worktree')
snapshot = 'bbc21eef7223565fc8ba682199046c0c137cf87d'
folder = root/'source_port'
names = ['morsehgp3D_v10/src/'+name for name in (
    'arith/wide.hpp','core/types.hpp','arith/geometry.hpp','arith/geometry.cpp',
    'catalogue/generator.cpp','tower/tower.cpp','tower/tower.hpp','sched/sort.hpp')]
before = {name:hashlib.sha256((repository/name).read_bytes()).hexdigest() for name in names}
for name in names:
    result = subprocess.run(['git','show',snapshot+':'+name],cwd=repository,capture_output=True,check=True)
    target = folder/'sources'/name
    target.parent.mkdir(parents=True,exist_ok=True)
    with target.open('xb') as stream:
        stream.write(result.stdout)
    if hashlib.sha256(result.stdout).hexdigest()!=before[name]:
        raise ValueError('source differs from pinned commit')
compiler = Path(shutil.which('g++')).resolve()
argv = [str(compiler),'-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror',
        '-I'+str(folder/'sources'/'morsehgp3D_v10'/'src'),'-c',str(folder/'generic_product.cpp'),
        '-o',str(folder/'generic_product.o')]
result = subprocess.run(argv,capture_output=True,text=True,check=False,timeout=30)
for stream in ('stdout','stderr'):
    with (folder/('generic_product.compile.'+stream)).open('x') as file:
        file.write(getattr(result,stream))
after = {name:hashlib.sha256((repository/name).read_bytes()).hexdigest() for name in names}
passed = (before==after and result.returncode==1 and
          'Wide<L> : 1 <= L <= 8 mots' in result.stderr and
          not (folder/'generic_product.o').exists())
record = dict(schema='mhgp10_level_source_port_capture_v1',status='PASS' if passed else 'FAIL',
              source_commit=snapshot,worktree_paths_same_as_commit=True,before=before,after=after,
              command=dict(argv=argv,exit_code=result.returncode,expected_exit_code=1),
              compiler=str(compiler),compiler_sha256=hashlib.sha256(compiler.read_bytes()).hexdigest(),
              source_sha256=hashlib.sha256((folder/'generic_product.cpp').read_bytes()).hexdigest(),
              native_executions=0,GCP_used=False,
              scope='expected compile-time width refusal, not an execution or production u18 defect')
with (folder/'receipt.json').open('x') as file:
    file.write(json.dumps(record,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status=record['status'],sources=len(names),compile_exit=result.returncode),sort_keys=True))
raise SystemExit(0 if passed else 1)
