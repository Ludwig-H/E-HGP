#!/usr/bin/env python3
"""Read-only proposed reader fix on frozen Python source and 200-byte synthetic files."""
import difflib,hashlib,json,struct,sys,tempfile,types
from pathlib import Path
root=Path(__file__).resolve().parent
p=json.loads((root/'proposal.json').read_text())
capture=root.parent/'formats'
meta_raw=(capture/'sources.json').read_bytes()
if hashlib.sha256(meta_raw).hexdigest()!=p['source_meta_sha256']:raise ValueError('metadata capture')
meta=json.loads(meta_raw)
for entry in meta['sources']:
 raw=(capture/entry['capture_path']).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=entry['sha256']:raise ValueError('snapshot dependency')
raw=(capture/'sources/mhgp11_formats.py').read_bytes()
if hashlib.sha256(raw).hexdigest()!=p['captured_reader_sha256']:raise ValueError('captured reader')
s=raw.decode()
if s.count(p['needle'])!=1:raise ValueError('unique patch anchor')
fixed=s.replace(p['needle'],p['replacement'])
if hashlib.sha256(fixed.encode()).hexdigest()!=p['proposed_reader_sha256']:raise ValueError('patched reader')
if hashlib.sha256((root/'version_equality.patch').read_bytes()).hexdigest()!=p['patch_sha256']:raise ValueError('patch')
expected_patch=''.join(difflib.unified_diff(s.splitlines(keepends=True),fixed.splitlines(keepends=True),fromfile='a/morsehgp3D_v11/bench/mhgp11_formats.py',tofile='b/morsehgp3D_v11/bench/mhgp11_formats.py'))
if (root/'version_equality.patch').read_text()!=expected_patch:raise ValueError('patch differs from proposed source')
sys.path.insert(0,str(capture/'sources'))
import mhgp11_formats as before
after=types.ModuleType('formats_proposal');after.__file__=str(capture/'sources/mhgp11_formats.py')
exec(compile(fixed,'formats_proposed_equality.py','exec'),after.__dict__)
def col(values,size):
 raw=b''.join(v.to_bytes(size,'little') for v in values)
 return raw+b'\0'*((-len(raw))%8)
def data(version):
 words=[version,21,1,1,1,0,0,0,0,0,136,168,200,200,200,200]
 return b'MHGP11SP'+struct.pack('<16Q',*words)+col([0],4)*3+col([7],4)+col([before.NONE],4)+col([0],4)+col([0],1)+col([0],4)
def manifest(binary,version):
 sp=before.read_supports(binary,21)
 return dict(schema=before.SCHEMA,output='supports',status='complete',public_status='not_claimed',coord_bits=21,k=1,
  parameters=dict(budget_bytes=4096,grid_step=None,origin=None),
  inputs=[dict(name='points',bytes=12,sha256='0'*64),dict(name='ids',bytes=4,sha256='0'*64)],
  files=[dict(name=before.SUPPORTS_NAME,format='MHGP11SP',version=version,bytes=len(binary),sha256=hashlib.sha256(binary).hexdigest())],
  tree_k_sha256=sp.tree_signature(),counts=sp.manifest_counts())
def outcome(reader,binary,declared):
 with tempfile.TemporaryDirectory(prefix='audit-version-equality-') as temporary:
  directory=Path(temporary)/'D';directory.mkdir()
  (directory/before.SUPPORTS_NAME).write_bytes(binary)
  (directory/before.MANIFEST).write_bytes((json.dumps(manifest(binary,declared),separators=(',',':'))+'\n').encode())
  try:
   result=reader.check_directory(str(directory),21)
   return {'accepted':True,'declared_version':result['manifest']['files'][0]['version'],'decoded_version':result['decoded'].version}
  except ValueError as error:return {'accepted':False,'reason':str(error)}
rows=[]
for name,actual,declared in [('honest_v2',2,2),('v1_binary_false_v2_manifest',1,2),('same_v2_binary_false_v1_manifest',2,1)]:
 binary=data(actual)
 rows.append({'case':name,'actual_version':actual,'declared_version':declared,'file_bytes':len(binary),
  'file_sha256':hashlib.sha256(binary).hexdigest(),'before':outcome(before,binary,declared),'proposed':outcome(after,binary,declared)})
if not rows[0]['before']['accepted'] or not rows[0]['proposed']['accepted']:raise ValueError('honest v2')
if not rows[1]['before']['accepted'] or rows[1]['proposed'].get('reason')!='MHGP11SP : version du fichier et du manifeste':raise ValueError('guard not causal')
if rows[2]['proposed']['accepted'] or rows[2]['file_sha256']!=rows[0]['file_sha256']:raise ValueError('same binary false declaration')
result={'schema':'audit_mhgp11sp_version_equality_fix_replay_v1','base_pin':p['base_pin'],'captured_reader_sha256':p['captured_reader_sha256'],
 'proposed_reader_sha256':p['proposed_reader_sha256'],'cases':rows,'compatibility_policy_changed':False,'native_runs':0,'cloud_actions':0,
 'limitation':'False v1 declaration for a v2 file remains refused by the existing manifest version policy before the new guard; the new equality guard causally rejects a v1 file falsely declared v2.'}
text=json.dumps(result,indent=2,sort_keys=True)+'\n'
if '--capture' in sys.argv:(root/'summary.json').write_text(text)
else:
 if json.loads((root/'summary.json').read_text())!=result:raise ValueError('summary changed')
print(text,end='')
