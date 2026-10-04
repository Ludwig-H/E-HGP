#!/usr/bin/env python3
"""Verify this portable metadata capsule; no Git/native/cloud call."""
import hashlib
import json
from pathlib import Path
B=Path(__file__).resolve().parent
checks=0

def need(ok, msg):
 global checks
 checks+=1
 if not ok: raise ValueError(msg)

def load(path): return json.loads((B/path).read_text())

pin='e02a6c235bc4a706519cdaa15f4b1465a6275eba'
for doc in ['SOURCE_BEFORE.json','SOURCE_EXTRA_BEFORE.json','ENGINE_BEFORE.json']:
 o=load(doc);need(o['pin']==pin,'git capture pin')
 for s in o.get('git_sources',o.get('sources',[])):
  raw=(B/s['payload']).read_bytes()
  need(len(raw)==s['bytes'] and hashlib.sha256(raw).hexdigest()==s['sha256'],'git source capture '+s['path'])
r2=load('R2_BEFORE.json');need(r2['pin']=='865f5e64ddd08bedf6ab8f94e8bb94812e380e79','separate R2 pin')
for s in r2['sources']:
 raw=(B/s['payload']).read_bytes();need(len(raw)==s['bytes'] and hashlib.sha256(raw).hexdigest()==s['sha256'],'R2 source capture')
for source in load('SOURCE_EXTRA_BEFORE.json')['archives']:
 tag=source['tag']; members={x['path']:x for x in source['members']}
 need(len(members)==len(source['members']),'archive member path uniqueness')
 manifest=(B/'selected'/tag/'results/MANIFEST.sha256').read_text()
 hashes={}
 for line in manifest.splitlines():
  digest,name=line.split('  ',1)
  need(len(digest)==64 and name.startswith('./'),'closed archive manifest syntax')
  path='results/'+name[2:]
  need(path not in hashes,'manifest unique path')
  hashes[path]=digest
 need(set(hashes)==set(members)-{'results/MANIFEST.sha256'},'archive original manifest exact inventory')
 for path,digest in hashes.items(): need(members[path]['sha256']==digest,'archive source manifest equality')
 for p in (B/'selected'/tag).rglob('*'):
  if not p.is_file(): continue
  name=p.relative_to(B/'selected'/tag).as_posix()
  need(name in members,'selected original archive member')
  raw=p.read_bytes();need(len(raw)==members[name]['bytes'] and hashlib.sha256(raw).hexdigest()==members[name]['sha256'],'selected member hash')
played=load('b872_engine_to_e02.json')
need(played['equal']==106 and len(played['files'])==106 and not played['different'],'played current engine equality')
need(all(x['equal'] and x['played_sha256']==x['current_git_sha256'] for x in played['files']),'each played engine source equal')
actor=load('ACTOR_DELTA.json');need(actor['relevant_source_deltas']==[],'actor no relevant engine/harness performance delta')
print(json.dumps({'schema':'ehgp.audit.deep_performance_capture_check.v1','checks':checks,'source_pin':pin,'verdict':'metadata_integrity_consistent','native_or_cloud_executed':False,'archive_manifests':3,'current_engine_blobs_equal_played':106,'scope':'selected metadata and hash inventories; full result archives remain pinned Git sources'},sort_keys=True,indent=2))
