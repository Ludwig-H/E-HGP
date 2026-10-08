#!/usr/bin/env python3
"""Link the frozen prepublication identity receipt to Git, without replaying the same witnesses."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

PIN = '0e62a723202732d69e430349c0b2a7b498cbb49c'
HERE = Path(__file__).resolve().parent

def need(ok, message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    p=argparse.ArgumentParser(); p.add_argument('--repo',type=Path,required=True); a=p.parse_args()
    old=HERE.parent/'apparie_identite'
    raw=(old/'capture.json').read_bytes()
    need(sha(raw)=='116453381491bf148d12bc33d77b438d8f694bc7f2fb1c2d8927c2d59a754531','old capture changed')
    capture=json.loads(raw)
    hashes={}
    for path, expected in capture['files'].items():
        data=subprocess.check_output(['git','show',PIN+':'+path],cwd=a.repo)
        hashes[path]=sha(data)
        need(hashes[path]==expected,'published source differs: '+path)
    patch=old/'relecture.patch'
    need(sha(patch.read_bytes())==capture['patch_sha256'],'old patch changed')
    path='morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py'
    with tempfile.TemporaryDirectory(prefix='audit-apparie-link-') as tmp:
        target=Path(tmp)/path; target.parent.mkdir(parents=True)
        target.write_bytes(subprocess.check_output(['git','show',PIN+':'+path],cwd=a.repo))
        for check in (True,False):
            args=['git','apply']+(['--check'] if check else [])+[str(patch)]
            subprocess.run(args,cwd=tmp,check=True,capture_output=True)
        post=sha(target.read_bytes())
        need(post==capture['candidate_sha256'],'candidate changed')
    print(json.dumps({'published_commit':PIN,'matching_files':hashes,
                      'all_bytes_identical_to_prepublication':True,'patch_applies':True,
                      'candidate_sha256':post,
                      'scope':'byte identity and patch application only; four witness results remain in apparie_identite'},
                     indent=2,sort_keys=True))

if __name__=='__main__':
    main()
