"""Juge d'orientation : centres Gram/Gauss Fraction, determinant du plan, pas de Cramer produit."""
import hashlib
import itertools
import json
import random
import subprocess
import sys
from collections import Counter
from fraction_oracle import center_of, determinant, dot, is_inside, orient, require, sign, sub


def cases(bits):
    m=(1 << bits)-1; zero=(0,0,0)
    planes=[(zero,(0,0,m),(0,m,0)),(zero,(0,m,0),(0,0,m)),
            (zero,(m,m,0),(0,0,m)),(zero,(1,0,0),(0,1,0))]
    rows=[]
    for s in (4,m):
        regular=(zero,(s,s,0),(s,0,s),(0,s,s))
        for p in itertools.permutations(regular):
            for plane in planes: rows.append(('regular_'+str(s),4,p,plane))
        for q in (1,2,3):
            for perm in itertools.permutations(regular[:3]):
                for plane in planes: rows.append(('arity',q,(*perm,regular[3]),plane))
    for p in [(zero,(4,0,0),(0,4,0),(0,0,4)),(zero,(m,1,0),(m-1,1,0),(m,1,1)),
              ((10,5,5),(9,8,5),(5,2,1),(1,5,8)),(zero,(4,0,0),(2,3,0),(2,0,2))]:
        for plane in planes: rows.append(('special',4,p,plane))
    if bits>=21:
        s=1 << ((124-3*bits-1)//3)
        for delta in (-1,0,1):
            p=(zero,(s,0,0),(0,s,0),(0,0,s+delta))
            rows.append(('boundary_'+str(delta),4,p,planes[0]))
    rng=random.Random(431003)
    for width in (7,m):
        for q in range(1,5):
            for _ in range(16):
                p=[tuple(rng.randrange(width+1) for _ in range(3)) for _ in range(4)]
                plane=[tuple(rng.randrange(m+1) for _ in range(3)) for _ in range(3)]
                rows.append(('random',q,p,plane))
    for q in (2,3,4): rows.append(('degenerate',q,[zero]*4,planes[0]))
    for q in (0,5): rows.append(('arity_refusal',q,[zero]*4,planes[0]))
    for axis in range(3):
        bad=list(zero);bad[axis]=m+1
        rows.append(('coordinate_refusal',1,[zero]*4,[tuple(bad),zero,zero]))
    return rows


def geometry(row,bits):
    _,q,points,plane=row
    if q not in (1,2,3,4): return 'refused parameter_out_of_range'
    if any(not 0<=x<1 << bits for p in (*points,*plane) for x in p): return 'refused coordinate_out_of_domain'
    center=center_of(points[:q])
    if center is None: return 'degenerate'
    edges=[sub(p,points[0]) for p in points[1:q]]
    if q==1: d=1; level=(0,1)
    elif q==2: d=2; level=(dot(edges[0],edges[0]),4)
    elif q==3:
        gram=determinant([[dot(u,v) for v in edges] for u in edges]);d=2*gram
        level=(dot(edges[0],edges[0])*dot(edges[1],edges[1])*dot(sub(points[1],points[2]),sub(points[1],points[2])),4*gram)
    else: d=2*abs(orient(*points))
    ns=[d*(c-a) for c,a in zip(center,points[0])]
    require(all(n.denominator==1 for n in ns),'integral native representation')
    ns=list(map(int,ns))
    if q==4: level=(dot(ns,ns),d*d)
    certificate=0<d<1 << (124-3*bits) and all(abs(n)<1 << (124-2*bits) for n in ns)
    raw=d*orient(*plane,center);require(raw.denominator==1,'integral orientation')
    return dict(anchor=tuple(points[0]),n=ns,d=d,level=level,certificate=certificate,
                orientation=sign(raw),raw=int(raw),inside=is_inside(center,points))


def block(value,level):
    words=[str(x) for x in value['anchor']]+[format(x,'x') for x in value['n']+[value['d']]]
    words += [str(int(value['certificate'])),str(value['orientation']),str(-value['orientation']),str(int(value['inside']))]
    if level: words += [format(x,'x') for x in value['level']]
    return words


def line_of(row,bits):
    value=geometry(row,bits)
    if isinstance(value,str):return value
    if row[1]!=4:return ' '.join(['ok','sphere']+block(value,True))
    return ' '.join(['ok','candidate']+block(value,False)+['materialized']+block(value,True)+['eager']+block(value,True))


def judge(row,bits,line):
    value=geometry(row,bits)
    if isinstance(value,str): require(line==value,'refusal/degeneracy');return 1
    words=line.split();require(words and words.pop(0)=='ok','status')
    checks=1
    for label,level in ([('candidate',False),('materialized',True),('eager',True)] if row[1]==4 else [('sphere',True)]):
        require(words and words.pop(0)==label,'closed representation label')
        expected=block(value,level);require(len(words)>=len(expected),'truncated block')
        actual=words[:len(expected)];words=words[len(expected):]
        require([int(x) for x in actual[:3]]==list(value['anchor']),'anchor')
        require([int(x,16) for x in actual[3:7]]==value['n']+[value['d']],'coefficients')
        require(actual[7:11]==expected[7:11],'certificate/sign/reverse/strict-inside')
        if level: require([int(x,16) for x in actual[11:]]==list(value['level']),'unchanged level representation')
        checks+=5+int(level)
    require(not words,'trailing output');return checks+1


def facts(rows,bits):
    m=(1 << bits)-1;zero=(0,0,0);checks=0
    for a,b,c in itertools.product(itertools.product((0,1),repeat=2),repeat=3):
        normal=determinant([sub(b,a),sub(c,a)])
        require(abs(normal)<=1,'common-anchor square normal');checks+=1
    require(determinant([(1,1),(1,-1)])==-2,'arbitrary Vec counterexample');checks+=1
    d=(1 << (124-3*bits))-1;n=(1 << (124-2*bits))-1
    for offset in (-m,m):
        for signs in itertools.product((-1,1),repeat=6):
            total=0
            for j in range(3):
                product=d*offset;coordinate=signs[j]*n+product;term=coordinate*signs[j+3]*m*m;total+=term
                require(abs(product)<1 << (124-2*bits),'D offset intermediate')
                require(abs(coordinate)<1 << (125-2*bits),'N+D offset intermediate')
                require(abs(term)<1 << 125,'orientation product')
                require(abs(total)<1 << 127,'orientation partial sum');checks+=4
    regular=(zero,(m,m,0),(m,0,m),(0,m,m))
    negative=geometry(('',4,regular,(zero,(0,0,m),(0,m,0))),bits)
    contact=geometry(('',4,regular,(zero,(m,m,0),(0,0,m))),bits)
    require(negative['raw']==-2*m**6 and contact['raw']==0,'large +/-2m^6/contact');checks+=1
    require(negative['certificate']==(bits==18),'regular extreme certificate');checks+=1
    if bits==24:
        wrapped=(negative['raw']+(1 << 127))%(1 << 128)-(1 << 127)
        require(wrapped>0 and negative['raw']<0,'u24 wrap changes sign');checks+=1
    for row in rows:
        if row[0].startswith('boundary_'):
            value=geometry(row,bits);require(value['certificate']==(row[0]=='boundary_-1'),'strict D boundary');checks+=1
    return checks


def inventory(rows,bits):
    values=[geometry(row,bits) for row in rows];good=[v for v in values if isinstance(v,dict)]
    return dict(requests=len(rows),certified=sum(v['certificate'] for v in good),uncertified=sum(not v['certificate'] for v in good),
                outcomes=dict(Counter('ok' if isinstance(v,dict) else v for v in values)))


def selftest():
    reports=[]
    for bits in (18,21,24):
        rows=cases(bits);checks=sum(judge(row,bits,line_of(row,bits)) for row in rows)+facts(rows,bits)
        corruptions=0
        for name in ('regular_4','regular_'+str((1 << bits)-1)):
            row=next(r for r in rows if r[0]==name);good=line_of(row,bits).split()
            bads=[]
            for pos in range(len(good)):
                bad=good.copy();bad[pos]='bad';bads.append(' '.join(bad))
            bads += ['', 'degenerate', ' '.join(good[:-1]), ' '.join(good+['0'])]
            for bad in bads:
                try:judge(row,bits,bad)
                except ValueError:corruptions+=1
                else:raise ValueError('corrupted orientation accepted')
        reports.append(dict(bits=bits,checks=checks,corruptions=corruptions,native=0,**inventory(rows,bits)))
    print(json.dumps(dict(verdict='conforme',profiles=reports),sort_keys=True))


def run(executable):
    header=subprocess.run([executable],input='',text=True,capture_output=True,timeout=5)
    require(header.returncode==0 and not header.stderr,'header process')
    words=header.stdout.split();require(len(words)==2 and words[0]=='bits' and int(words[1]) in (18,21,24),'header bits')
    bits=int(words[1]);rows=cases(bits)
    payload=''.join(str(q)+' '+' '.join(str(x) for p in (*points,*plane) for x in p)+'\n' for _,q,points,plane in rows)
    child=subprocess.run([executable],input=payload,text=True,capture_output=True,timeout=60)
    require(child.returncode==0 and not child.stderr,'native process')
    lines=child.stdout.splitlines();require(lines and lines[0]=='bits '+str(bits) and len(lines)==len(rows)+1,'complete output')
    checks=sum(judge(row,bits,line) for row,line in zip(rows,lines[1:]))+facts(rows,bits)
    require(len(rows)>=480 and checks>=7000,'non-vacuous oracle')
    print(json.dumps(dict(verdict='conforme',bits=bits,checks=checks,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                         **inventory(rows,bits)),sort_keys=True))


if __name__=='__main__':
    try:
        if sys.argv[1:]==['--selftest']:selftest()
        elif len(sys.argv)==2:run(sys.argv[1])
        else:raise ValueError('usage: orientation_certificate_oracle.py EXE|--selftest')
    except (ValueError,OSError,subprocess.SubprocessError,IndexError) as error:
        print('REFUS orientation_certificate_oracle:',error,file=sys.stderr);sys.exit(1)
