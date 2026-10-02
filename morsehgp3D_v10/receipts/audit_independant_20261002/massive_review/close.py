from pathlib import Path
import hashlib,json
p=Path(__file__).resolve().parent
def sha(b):return hashlib.sha256(b).hexdigest()
a=json.loads((p/'SOURCE_BEFORE.json').read_text());b=json.loads((p/'SOURCE_AFTER.json').read_text())
before={e['path']:e for e in a['files']};after={e['path']:e for e in b['files']}
if before.keys()!=after.keys() or any(before[k]['copy_sha256']!=after[k]['copy_sha256'] for k in before) or not b['product_and_compilation_sources_match_copy'] or not all(b['source_vs_4b7d70422'].values()):raise SystemExit('source mismatch')
if set(b['changed_named_paths']) != {'morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md'}:raise SystemExit('unexpected documentary change')
x=json.loads((p/'normal.stdout.json').read_text());y=json.loads((p/'optimized.stdout.json').read_text())
if x!=y or x['status']!='PASS':raise SystemExit('reader mismatch')
for f in ('compiler.stderr.txt','layout_compiler.stderr.txt','nearest.stderr.txt','layout.stderr.txt','normal.stderr.txt','optimized.stderr.txt','dependencies.stderr.txt'):
 if (p/f).read_bytes():raise SystemExit('nonempty stderr '+f)
binary={name:sha(Path(name).read_bytes()) for name in ['/tmp/mhgp10-massive-nearest-20261002','/tmp/mhgp10-massive-layout-20261002']}
(p/'CLOSURE.json').write_text(json.dumps({'status':'PASS','head_readonly':b['head_readonly'],'source_files':len(b['files']),'all_captured_bytes_preserved':True,'live_compilation_sources_unchanged':True,'live_documentary_change_preserved':b['changed_named_paths'],'native_scope':'normal, 144 sites; candidate layouts only','reader_checks':x['checks'],'normal_optimized_identical':True,'binary_sha256':binary,'no_massive_allocation':True,'no_current_native_large_or_LiDAR_capacity_qualification':True},indent=2)+'\n')
files=sorted(f for f in p.rglob('*') if f.is_file() and f.name!='SHA256SUMS')
(p/'SHA256SUMS').write_text(''.join(sha(f.read_bytes())+'  '+str(f.relative_to(p))+'\n' for f in files))
print(json.dumps({'status':'PASS','artifacts':len(files),'manifest_sha256':sha((p/'SHA256SUMS').read_bytes())}))
