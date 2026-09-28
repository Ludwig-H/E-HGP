# Invariance par homothetie entiere : grille 5^3 et quasi-plan, facteur s ; u18 si s = 65535.
import json, random, struct, subprocess
def gen(n,seed,fam):
    random.seed(seed); used=set(); P=[]
    while len(P)<n:
        if fam=='grid': x,y,z=random.randrange(5),random.randrange(5),random.randrange(5)
        else:
            x,y=random.randrange(8),random.randrange(8); z=(x+2*y)%3
        if (x,y,z) not in used: used.add((x,y,z)); P.append((x,y,z))
    return P
def run(P,K,tag):
    fn=f'sc_{tag}.u32le'
    open(fn,'wb').write(b''.join(struct.pack('<3I',*p) for p in P))
    return json.loads(subprocess.run(['nice','-n','19','./cble',fn,f'--K={K}',f'--M={max(16,2*K+4)}','--dom=3'],capture_output=True,text=True,check=True,timeout=120).stdout)
fails=0;tot=0
for fam in ('grid','plane'):
  for seed in (1,2,3,4):
    P=gen(60 if fam=='grid' else 40,seed,fam)
    for K in (3,5,8):
      base=run(P,K,'base')
      for s in (65533, 40000):
        Q=[(s*x+1,s*y+2,s*z+3) for x,y,z in P]
        if max(max(q) for q in Q) >= 1<<18: continue
        import time; t0=time.time()
        try:
            r=run(Q,K,'scaled')
        except subprocess.TimeoutExpired:
            print('TIMEOUT',fam,seed,K,s,flush=True); fails+=1; tot+=1; continue
        print('run',fam,seed,K,s,'%.2fs'%(time.time()-t0),'nodes',r['nodes'],'leaves',r['leaves'],'max_m',r['max_m'],flush=True)
        tot+=1
        ok = r['by_q_p']==base['by_q_p'] and r['extra_shell']==base['extra_shell']
        if not ok:
            fails+=1; print('MISMATCH',fam,seed,K,s,base['balls'],r['balls'],base['extra_shell'],r['extra_shell'],base['by_q_p'],r['by_q_p'])
print('total',tot,'fails',fails)
