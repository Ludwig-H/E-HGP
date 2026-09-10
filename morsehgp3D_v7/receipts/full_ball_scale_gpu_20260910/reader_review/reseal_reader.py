#!/usr/bin/env python3
"""Reseal a reader-only schema correction; captured run bytes stay unchanged."""
import hashlib
import json
from pathlib import Path
import shutil

HERE=Path(__file__).resolve().parent
OUT=Path('/workspaces/E-HGP/morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910')
review=OUT/'reader_review'
review.mkdir(exist_ok=False)
for path in (HERE/'reader_review_r1').iterdir():
    if path.is_file(): shutil.copyfile(path,review/path.name)
shutil.copyfile(HERE/'record_reader_fix.py',review/'record_reader_fix.py')
shutil.copyfile(__file__,review/'reseal_reader.py')
manifest={p.relative_to(OUT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
          for p in sorted(OUT.rglob('*')) if p.is_file() and p!=OUT/'manifest.json'}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
print('reader corrected; initial reader and manifest retained; run bytes untouched')
