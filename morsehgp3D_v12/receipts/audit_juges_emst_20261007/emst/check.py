#!/usr/bin/env python3
"""JUG-EMST : petit oracle independant sur graphe complet, mutations FULL et identites."""
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
BASE = ROOT / 'morsehgp3D_v12/juges/emst'
PIN = '1f7642e105aebd76632c58c63fdfd5b5c0824779'
WORD = struct.Struct('<Q')
NONE = (1 << 32) - 1


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


sources = sorted(str(p.relative_to(ROOT)) for p in BASE.rglob('*') if p.is_file())
sources += ['morsehgp3D_v12/src/cloud/cloud.hpp', 'morsehgp3D_v12/docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md']
before = {p: sha((ROOT / p).read_bytes()) for p in sources}
for p, expected in before.items():
    require(sha(subprocess.check_output(['git', 'show', PIN + ':' + p], cwd=ROOT)) == expected, 'pin ' + p)
self_sha = sha(Path(__file__).read_bytes())


def all_edges(points):
    return sorted((sum((a-b)**2 for a,b in zip(points[i],points[j])),i,j)
                  for i in range(len(points)) for j in range(i+1,len(points)))


def kruskal(points):
    groups = [{i} for i in range(len(points))]
    tree = []
    for edge in all_edges(points):
        _, a, b = edge
        ga = next(g for g in groups if a in g)
        gb = next(g for g in groups if b in g)
        if ga is not gb:
            groups.remove(gb)
            ga.update(gb)
            tree.append(edge)
    return tree


def graph_tree(points):
    """Balayage de TOUS les seuils du graphe complet, composantes recalculees par parcours."""
    n = len(points)
    edges = all_edges(points)
    adjacency = [set() for _ in points]
    components = [{i} for i in range(n)]
    heads = list(range(n))
    fusions = []
    i = 0
    while i < len(edges):
        d2 = edges[i][0]
        while i < len(edges) and edges[i][0] == d2:
            _, a, b = edges[i]
            adjacency[a].add(b)
            adjacency[b].add(a)
            i += 1
        unseen, current = set(range(n)), []
        while unseen:
            todo, found = [min(unseen)], set()
            while todo:
                v = todo.pop()
                if v in found:
                    continue
                found.add(v)
                todo.extend(adjacency[v] - found)
            unseen.difference_update(found)
            current.append(found)
        new_heads = []
        for group in current:
            old = [j for j,c in enumerate(components) if c <= group]
            if len(old) == 1:
                new_heads.append(heads[old[0]])
            else:
                new_heads.append(n + len(fusions))
                fusions.append((Fraction(d2,4), tuple(sorted(heads[j] for j in old))))
        components, heads = current, new_heads
    return fusions


def natural(v):
    b = v.to_bytes(max(1,(v.bit_length()+7)//8), 'little')
    return WORD.pack(len(b)) + b


def hashes(points, fusions, edges):
    n = len(points)
    f = WORD.pack(len(fusions))
    for level, kids in fusions:
        f += natural(level.numerator) + natural(level.denominator) + WORD.pack(len(kids))
        f += b''.join(WORD.pack(x) for x in kids)
    htree = sha(b'ehgp.v12.jug_emst.ordre1.v1\0' + WORD.pack(n) +
                b''.join(WORD.pack(x) for p in points for x in p) + f)
    hf = sha(b'ehgp.v12.jug_emst.fusions.v1\0' + WORD.pack(n) + f)
    he = sha(b'ehgp.v12.jug_emst.emst.v1\0' + WORD.pack(n) +
             b''.join(natural(d)+WORD.pack(a)+WORD.pack(b) for d,a,b in edges))
    return {'sha256_arbre': htree, 'sha256_fusions': hf, 'sha256_emst': he}


def morton(point):
    return sum(((point[a]>>b)&1) << (3*b+a) for a in range(3) for b in range(32))


def full(points, fusions, ids, changes=None, factor=1, padding=0, kmax=1):
    changes = changes or {}
    n = len(points)
    out = bytearray(b'MHGP11FUL1')
    def word(key, value):
        out.extend(WORD.pack(changes.get(key,value)))
    def integer(key, value):
        sign = changes.get(key+'.sign', int(value < 0))
        v = abs(changes.get(key,value))
        limbs = max(1,(v.bit_length()+63)//64) + padding
        word(key+'.sign',sign)
        word(key+'.limbs',limbs)
        for j in range(limbs):
            word(key+'.word'+str(j), (v>>(64*j)) & ((1<<64)-1))
    for key,v in zip(('bits','kmax','sites','points'),(32,kmax,n,n)):
        word(key,v)
    for slot,i in enumerate(sorted(range(n),key=lambda i:morton(points[i]))):
        for a,v in enumerate((*points[i],1,ids[i])):
            word('site%d.%d'%(slot,a),v)
    nn=n+len(fusions)
    for key,v in zip(('ordre','births','nodes','edges','root'),(1,n,nn,nn-1,nn-1)):
        word(key,v)
    parent=[NONE]*nn
    for j,(_,kids) in enumerate(fusions):
        for c in kids: parent[c]=n+j
    cursor=0
    for j in range(nn):
        if j<n:
            level,kids,offset=Fraction(0),(),0
        else:
            level,kids=fusions[j-n]
            offset=cursor
            cursor+=len(kids)
        for key,v in zip(('parent','offset','cardinal'),(parent[j],offset,len(kids))):
            word('node%d.%s'%(j,key),v)
        integer('node%d.num'%j,level.numerator*factor)
        integer('node%d.den'%j,level.denominator*factor)
        if j<n:
            for a,v in enumerate((*points[j],1)):
                integer('node%d.center%d'%(j,a),v*factor)
    for j,(_,kids) in enumerate(fusions):
        for k,c in enumerate(kids): word('fusion%d.child%d'%(j,k),c)
    return bytes(out)


M=(1<<32)-1
fixtures=[('singleton',[(7,8,9)]),('equilateral',[(0,0,0),(1,1,0),(1,0,1)]),
 ('cube',[(x,y,z) for x in (0,2) for y in (0,2) for z in (0,2)]),
 ('grid27',[(x,y,z) for x in (0,1,2) for y in (0,1,2) for z in (0,1,2)]),
 ('u32_diagonal',[(0,0,0),(M,M,M)]),('u32_corners',[(x,y,z) for x in (0,M) for y in (0,M) for z in (0,M)]),
 ('two_equal_pairs',[(0,0,0),(1,0,0),(9,0,0),(10,0,0)]),
 ('u31_limit',[(0,0,0),((1<<31)-1,)*3]),('u32_start',[(0,0,0),(1<<31,)*3])]
state=20261007
for case in range(15):
    pts=set()
    n=17+(case%4)*5
    while len(pts)<n:
        p=[]
        for a in range(3):
            state=(1664525*state+1013904223)&M
            p.append((state>>8)%41 if case<5 else state if case>=10 else (state>>8)%41+(1<<31))
        pts.add(tuple(p))
    fixtures.append(('seeded_%02d'%case,sorted(pts)))

rows, mutations=[],[]
with tempfile.TemporaryDirectory(prefix='ehgp-emst-audit-') as temp:
    work=Path(temp)
    binary=work/'judge'
    compiler='c++'
    args=[compiler,'-std=c++20','-O2','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror',
          '-MMD','-MF',str(work/'deps.d'),str(BASE/'src/jug_emst.cpp'),'-o',str(binary)]
    built=subprocess.run(args,capture_output=True,text=True,timeout=120)
    require(built.returncode==0,'compilation '+built.stderr)
    binary_sha=sha(binary.read_bytes())
    version=subprocess.check_output([compiler,'--version'],text=True).splitlines()[0]
    dependencies=(work/'deps.d').read_text().replace('\\\n','').split(':',1)[1].split()
    closure={str(Path(p).relative_to(ROOT)):sha(Path(p).read_bytes()) for p in dependencies}
    require(all(before[p]==s for p,s in closure.items()),'dependances compilation')
    def run(arguments):
        proc=subprocess.run([str(binary)]+[str(x) for x in arguments],capture_output=True,timeout=10)
        doc=json.loads(proc.stdout) if proc.stdout else None
        if doc is not None: doc.pop('temps_ms',None)
        return proc.returncode,doc,proc.stderr.decode('utf-8')
    for name,input_points in fixtures:
        points=sorted(input_points)
        fs=graph_tree(points)
        es=kruskal(points)
        expected=hashes(points,fs,es)
        cloud,tree,edge,dump,ids=(work/x for x in ('cloud','tree','edge','dump','ids'))
        cloud.write_bytes(b''.join(struct.pack('<III',*p) for p in reversed(points)))
        pid=[NONE-i*7 for i in range(len(points))]
        ids.write_bytes(b''.join(struct.pack('<I',i) for i in reversed(pid)))
        dump.write_bytes(full(points,fs,pid))
        a,doc,err=run([cloud,'--arbre',tree,'--emst',edge,'--vidage',dump,'--ids',ids])
        require(a==0 and doc['vidage']['verdict']=='identique',name+' dump '+err)
        require(all(doc[k]==v for k,v in expected.items()),name+' empreintes')
        parsed=[tuple(map(int,line.split())) for line in edge.read_text().splitlines()]
        require(parsed==es,name+' emst')
        tree_lines=tree.read_text().splitlines()
        want=['N '+str(len(points))]+['B %d %d %d'%p for p in points]
        want+=['F %d %d %s'%(f.numerator,f.denominator,' '.join(map(str,k))) for f,k in fs]
        require(tree_lines==want,name+' arbre graphe complet')
        b,forced,_=run([cloud,'--force-u128'])
        require(b==0 and all(forced[k]==v for k,v in expected.items()),name+' force u128')
        route='u64' if max(c for p in points for c in p)<(1<<31) else 'u128'
        require(doc['arithmetique']==route,name+' voie')
        rows.append({'name':name,'sites':len(points),'input_sha256':sha(cloud.read_bytes()),
                     'nodes':len(points)+len(fs),'arithmetic':route,
                     'fusions':len(fs),'multifusions':sum(len(k)>=3 for _,k in fs),
                     'boruvka':doc['boruvka'],**expected})
    # Mutations minimales de la comparaison FULL : vrai CLI, nuage fixe distinct.
    points=[(0,0,0),(2,0,0)]
    fs=[(Fraction(1),(0,1))]
    cloud.write_bytes(b''.join(struct.pack('<III',*p) for p in points))
    good=full(points,fs,[17,NONE])
    cases=[('positive',good,None,0),('level',full(points,fs,[17,NONE],{'node2.num':2}),None,1),
      ('parent',full(points,fs,[17,NONE],{'node0.parent':1}),None,1),
      ('child',full(points,fs,[17,NONE],{'fusion0.child1':0}),None,1),
      ('center',full(points,fs,[17,NONE],{'node0.center0':1}),None,1),
      ('site',full(points,fs,[17,NONE],{'site1.0':3}),None,1),
      ('zero_den',full(points,fs,[17,NONE],{'node2.den':0}),None,3),
      ('negative_zero',full(points,fs,[17,NONE],{'node0.num.sign':1}),None,3),
      ('truncated',good[:-1],None,3),
      ('huge_equivalent',full(points,fs,[17,NONE],factor=(1<<3800)+17),None,0),
      ('padded_equivalent',full(points,fs,[17,NONE],padding=63),None,0),
      ('ids_positive',good,[17,NONE],0),('ids_disagree',good,[19,NONE],1),
      ('ids_duplicate_without_reference',full(points,fs,[17,17]),None,3),
      ('ids_duplicate_with_reference',full(points,fs,[17,17]),[17,17],0),
      ('higher_orders_not_read',full(points,fs,[17,NONE],kmax=2),None,0)]
    for name,payload,labels,want in cases:
        dump.write_bytes(payload)
        arguments=[cloud,'--vidage',dump]
        if labels is not None:
            ids.write_bytes(b''.join(struct.pack('<I',x) for x in labels))
            arguments+=['--ids',ids]
        code,doc,err=run(arguments)
        require(code==want,name+' unexpected code '+str(code))
        mutations.append({'name':name,'bytes':len(payload),'payload_sha256':sha(payload),
                          'ids':labels,'code':code,'verdict':doc['vidage']})
after={p:sha((ROOT/p).read_bytes()) for p in sources}
require(before==after and self_sha==sha(Path(__file__).read_bytes()),'source modification')
out={'pin':PIN,'source_sha256':before,'sources_equal_pin_and_stable':True,'check_sha256':self_sha,
     'compiler':version,'compile_options':['-std=c++20','-O2','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror'],
     'compiled_dependency_sha256':closure,'binary_sha256':binary_sha,'compilation_returncode':0,
     'clouds':rows,'cloud_count':len(rows),'actual_cli_calls':len(rows)*2+len(mutations),
     'mutations':mutations,'native_product_modified':False,'gcp_used':False,'licensed_data_used':False,
     'historical_nine_dumps_replayed':False}
print(json.dumps(out,sort_keys=True,separators=(',',':')))
