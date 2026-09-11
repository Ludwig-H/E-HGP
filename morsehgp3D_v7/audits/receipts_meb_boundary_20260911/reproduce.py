#!/usr/bin/env python3
import argparse,shutil,subprocess,sys,tarfile
from pathlib import Path
p=Path(__file__).resolve().parent
arg=argparse.ArgumentParser();arg.add_argument('--work',required=True);a=arg.parse_args()
w=Path(a.work).resolve()
audits=p.parent
if not w.is_relative_to(audits):raise RuntimeError('work_must_be_under_audits')
w.mkdir(exist_ok=False)
with tarfile.open(p/'base.tar.gz') as t:t.extractall(w/'source',filter='data')
for n in ('guard.cpp','guarded_run.hpp','meb_hybrid.cpp'):shutil.copyfile(p/n,w/n)
shutil.copyfile(p/'record_original.source',w/'record.py')
r=subprocess.run([sys.executable,'-B',str(w/'record.py')],check=False)
raise SystemExit(r.returncode)
