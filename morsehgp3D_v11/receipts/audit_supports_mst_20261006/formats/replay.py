#!/usr/bin/env python3
"""Bounded stdlib synthetic format read only; no HGP, native/cloud or input data."""
import hashlib,json,struct,subprocess,sys,tempfile,types
from pathlib import Path
root=Path(__file__).resolve().parent
meta=json.loads((root/'sources.json').read_text())
for entry in meta['sources']:
 raw=(root/entry['capture_path']).read_bytes()
 if len(raw)!=entry['bytes'] or hashlib.sha256(raw).hexdigest()!=entry['sha256']:raise ValueError('snapshot')
sys.path.insert(0,str(root/'sources'))
import mhgp11_formats as current
original=subprocess.check_output(['git','show',meta['base_pin']+':morsehgp3D_v11/bench/mhgp11_formats.py'],cwd=root)
if hashlib.sha256(original).hexdigest()!=meta['old_reader_sha256']:raise ValueError('old reader')
old=types.ModuleType('old_formats');old.__file__=str(root/'sources/mhgp11_formats.py')
exec(compile(original,'old_formats_at_pin.py','exec'),old.__dict__)
def col(values,size):
 raw=b''.join(v.to_bytes(size,'little') for v in values)
 return raw+b'\0'*((-len(raw))%8)
words=[1,21,1,1,1,0,0,0,0,0,136,168,200,200,200,200]
data=b'MHGP11SP'+struct.pack('<16Q',*words)+col([0],4)*3+col([7],4)+col([old.NONE],4)+col([0],4)+col([0],1)+col([0],4)
if len(data)!=200:raise ValueError('synthetic size')
legacy=old.read_supports(data,21)
manifest=dict(schema=current.SCHEMA,output='supports',status='complete',public_status='not_claimed',coord_bits=21,k=1,
 parameters=dict(budget_bytes=4096,grid_step=None,origin=None),
 inputs=[dict(name='points',bytes=12,sha256='0'*64),dict(name='ids',bytes=4,sha256='0'*64)],
 files=[dict(name=current.SUPPORTS_NAME,format='MHGP11SP',version=1,bytes=len(data),sha256=hashlib.sha256(data).hexdigest())],
 tree_k_sha256=legacy.tree_signature(),counts=legacy.manifest_counts())
def canonical(obj):return(json.dumps(obj,separators=(',',':'))+'\n').encode()
def outcome(function):
 try:return {'accepted':True,'result':function()}
 except ValueError as e:return {'accepted':False,'reason':str(e)}
old_manifest=outcome(lambda:old.read_manifest(canonical(manifest))['files'][0]['version'])
new_v1=outcome(lambda:current.read_manifest(canonical(manifest))['files'][0]['version'])
decoded_v1=current.read_supports(data,21)
pretend=json.loads(canonical(manifest));pretend['files'][0]['version']=2;pretend['counts']=decoded_v1.manifest_counts()
with tempfile.TemporaryDirectory(prefix='audit-mhgp11sp-v2-') as temporary:
 directory=Path(temporary)/'D';directory.mkdir()
 (directory/current.SUPPORTS_NAME).write_bytes(data);(directory/current.MANIFEST).write_bytes(canonical(pretend))
 def directory_read():
  value=current.check_directory(str(directory),21)
  return {'declared_version':value['manifest']['files'][0]['version'],'decoded_version':value['decoded'].version}
 wrong_version=outcome(directory_read)
result={'schema':'audit_mhgp11sp_v2_reader_coherence_v1','base_pin':meta['base_pin'],
 'wip_reader_sha256':meta['sources'][0]['sha256'],'synthetic_file_bytes':200,'synthetic_file_sha256':hashlib.sha256(data).hexdigest(),
 'old_reader_valid_v1_manifest':old_manifest,'wip_reader_valid_v1_manifest':new_v1,
 'wip_read_supports_v1_binary_accepted':True,'wip_directory_declared_v2_actual_v1':wrong_version,'native_runs':0,'cloud_actions':0,
 'limits':['Only synthetic single-site metadata/file, no C++ publisher executed.','Whether whole-directory v1 compatibility is required remains a contract decision; version equality is an integrity requirement.']}
if not old_manifest['accepted'] or new_v1['accepted'] or wrong_version.get('result')!={'declared_version':2,'decoded_version':1}:raise ValueError('finding not reproduced')
text=json.dumps(result,indent=2,sort_keys=True)+'\n'
if '--capture' in sys.argv:(root/'summary.json').write_text(text)
else:
 if json.loads((root/'summary.json').read_text())!=result:raise ValueError('summary changed')
print(text,end='')
