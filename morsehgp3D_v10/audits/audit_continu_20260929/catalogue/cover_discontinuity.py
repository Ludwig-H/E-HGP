"""Exact three-point geometric witness; development fixture, no benchmark seeds."""
from fractions import Fraction
import hashlib,json,pathlib,struct,subprocess
p=pathlib.Path(__file__).resolve().parent
exe=pathlib.Path('/workspaces/E-HGP/build/v10-j2/j2c/build/mhgp10_tower')
rows=[]
for middle in (999,1000,1001):
    pts=[(0,0,0),(middle,0,0),(2000,0,0)]
    src=p/f'cover_{middle}.u32le';src.write_bytes(b''.join(struct.pack('<3I',*x) for x in pts))
    for entry in ('cover','core'):
        dump=p/f'cover_{middle}_{entry}.txt'
        cmd=[str(exe),str(src),'--k=2','--threads=1','--entry='+entry,'--dump='+str(dump)]
        run=subprocess.run(cmd,text=True,capture_output=True,timeout=5)
        if run.returncode: raise RuntimeError(run.stdout+run.stderr)
        nodes={};points={};order=None
        for line in dump.read_text().splitlines():
            s=line.split()
            if s[0]=='order': order=int(s[1])
            elif order==2 and s[0]=='node': nodes[int(s[1])]=(int(s[2]),Fraction(int(s[3]),int(s[4])))
            elif order==2 and s[0]=='point': points[int(s[1])]=(int(s[4]),Fraction(int(s[6]),int(s[7])) if s[5].startswith('r') else Fraction(int(s[5])))
        def join_beta(x,y):
            nx,bx=points[x];ny,by=points[y];a=set()
            while nx!=-1: a.add(nx);nx=nodes[nx][0]
            while ny not in a: ny=nodes[ny][0]
            return max(bx,by,nodes[ny][1])
        rows.append(dict(middle=middle,entry=entry,argv=cmd,code=run.returncode,
                         beta_left_middle=str(join_beta(0,middle)),beta_middle_right=str(join_beta(middle,2000)),
                         dump_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),dump=dump.read_text()))
out={'schema':'mhgp10_cover_discontinuity_witness_v1','binary_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'rows':rows}
(p/'cover_discontinuity.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps([{k:r[k] for k in ('middle','entry','beta_left_middle','beta_middle_right')} for r in rows]))
