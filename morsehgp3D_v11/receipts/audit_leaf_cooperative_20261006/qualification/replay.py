#!/usr/bin/env python3
"""Relecture de métadonnées seulement, sans calcul HGP/cloud/native."""
import hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parent
proof=json.loads((root/'proof.json').read_text())
pin=proof['source_commit']
if len(pin)!=40:raise ValueError('pin')
for s in proof['sources']:
 raw=subprocess.check_output(['git','show',pin+':'+s['path']],cwd=root)
 if hashlib.sha256(raw).hexdigest()!=s['sha256']:raise ValueError(s['path'])
 lines=raw.decode().splitlines()
 for a in s['anchors']:
  if not 1<=a['first']<=a['last']<=len(lines):raise ValueError('anchor')
  excerpt=('\n'.join(lines[a['first']-1:a['last']])+'\n').encode()
  if hashlib.sha256(excerpt).hexdigest()!=a['sha256']:raise ValueError('excerpt')
if proof['existing_gate']['input_coordinate_bits']!=16 or proof['existing_gate']['required_unresolved']!=0:raise ValueError('gate scope')
if proof['fixtures']['triples_at_m32']!=4960 or proof['fixtures']['seen_words']!=155:raise ValueError('boundaries')
if len({m['id'] for m in proof['reuse_mutants']})!=8:raise ValueError('mutants')
print(json.dumps({'verdict':'conforme','source_commit':pin,'source_files':len(proof['sources']),'native_runs':0,'cloud_actions':0},sort_keys=True))
