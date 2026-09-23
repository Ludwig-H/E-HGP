import sys, subprocess, collections, re
samples, exe = sys.argv[1], sys.argv[2]; want = sys.argv[3] if len(sys.argv)>3 else None
maps=[]; pcs=collections.Counter()
for line in open(samples):
    if line.startswith('M '):
        parts=line[2:].split()
        if len(parts)>=6 and 'x' in parts[1]:
            lo,hi=[int(x,16) for x in parts[0].split('-')]; off=int(parts[2],16); maps.append((lo,hi,off,parts[5]))
    elif line.startswith('P '): pcs[int(line[2:],16)]+=1
total=sum(pcs.values()); exe_pcs=collections.Counter()
for pc,c in pcs.items():
    for lo,hi,off,path in maps:
        if lo<=pc<hi and path.endswith(exe.split('/')[-1]): exe_pcs[pc-lo+off]+=c; break
addrs=sorted(exe_pcs)
out=subprocess.run(['addr2line','-a','-f','-i','-C','-e',exe],input='\n'.join(hex(a) for a in addrs)+'\n',capture_output=True,text=True).stdout.splitlines()
chains={}; cur=None; buf=[]
for l in out:
    if re.fullmatch(r'0x[0-9a-f]+',l):
        if cur is not None: chains[cur]=buf
        cur=int(l,16); buf=[]
    else: buf.append(l)
if cur is not None: chains[cur]=buf
def clean(fn): return re.sub(r'\(.*','',fn.replace('(anonymous namespace)::',''))[-70:]
outer=collections.Counter(); sub=collections.defaultdict(collections.Counter)
for a,c in exe_pcs.items():
    ch=chains.get(a,[])
    fr=[(clean(ch[i]), ch[i+1].split('/')[-1].split(' ')[0] if i+1<len(ch) else '') for i in range(0,len(ch),2)]
    if not fr: continue
    top=fr[-1][0]; outer[top]+=c
    # line within the outermost function = location of frame -1
    sub[top][fr[-1][1]]+=c
print('total samples', total)
for f,c in outer.most_common(int(sys.argv[4]) if len(sys.argv)>4 else 25):
    print(f'{100*c/total:6.2f}% {f}')
    if want and want in f:
        for l,cc in sub[f].most_common(30): print(f'        {100*cc/total:6.2f}% {l}')
