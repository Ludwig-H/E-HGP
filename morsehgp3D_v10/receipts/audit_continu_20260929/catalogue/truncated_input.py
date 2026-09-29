"""Observe malformed-record handling; no source change."""
import hashlib,json,pathlib,struct,subprocess
p=pathlib.Path(__file__).resolve().parent
exe=pathlib.Path('/workspaces/E-HGP/build/v10-j2/j2c/build/mhgp10_catalogue')
base=struct.pack('<6I',0,0,0,2,0,0)
rows=[]
for extra in (0,1,4,8,11):
    src=p/f'trailing_{extra}.u32le'
    src.write_bytes(base+b'\x00'*extra)
    cmd=[str(exe),str(src),'--k=1','--threads=1']
    r=subprocess.run(cmd,text=True,capture_output=True)
    rows.append(dict(extra_bytes=extra,argv=cmd,code=r.returncode,stdout=r.stdout,stderr=r.stderr))
out=dict(schema='mhgp10_truncated_input_audit_v1',binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),rows=rows)
(p/'truncated_input.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps([dict(extra_bytes=r['extra_bytes'],code=r['code'],status=json.loads(r['stdout'])['status'],n=json.loads(r['stdout'])['n']) for r in rows]))
