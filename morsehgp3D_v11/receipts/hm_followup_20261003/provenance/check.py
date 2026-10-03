#!/usr/bin/env python3
import json,hashlib,pathlib,sys
p=pathlib.Path(__file__).resolve().parent
r=json.loads((p/'claudepts3_source.json').read_text())
checks=[]
for key,v in r['package_members_copied'].items():
 q=p/v['local_copy'];checks.append(hashlib.sha256(q.read_bytes()).hexdigest()==v['sha256']==v['snapshot_manifest_sha256'])
for key,local in r['live_copies'].items():
 h=hashlib.sha256((p/local).read_bytes()).hexdigest();b=json.loads((p/'live_source_before_after.json').read_text());checks.append(h==b['before'][key]==b['after'][key])
checks.extend([r['packaged_radius_sha256'].startswith('f023f6d0'),r['live_fix_radius_sha256'].startswith('58952a8b'),r['packaged_radius_sha256']!=r['live_fix_radius_sha256'],r['packaged_hierarchy_sha256']==r['live_hierarchy_sha256'],r['source_package_sha256'].startswith('a6b44d9a')])
print(json.dumps({'scope':'hashes des copies de code et métadonnées seulement ; aucun import produit','checks':len(checks),'all_pass':all(checks)},sort_keys=True,ensure_ascii=False))
raise SystemExit(0 if all(checks) else 1)
