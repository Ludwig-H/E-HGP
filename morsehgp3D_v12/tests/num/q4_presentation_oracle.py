"""Poids de presentation : centre Gram/Fraction, predicate affine sur deux tetraedres.

Aucun calcul de N.cross/H ni reprise du certificat bbox produit pour la verite.
N,D,Level sont reconstruits depuis le centre rationnel, y compris leur echelle non reduite.
"""
import itertools as it
import hashlib
import json
import random
import subprocess
import sys
from fractions import Fraction as F
from fraction_oracle import center_of, determinant, dot, is_inside, orient, require, sub


def cases(bits):
    m=(1 << bits)-1; zero=(0,0,0)
    regular=lambda s,o=0: ((o,o,o),(s+o,s+o,o),(s+o,o,s+o),(o,s+o,s+o))
    rows=[]
    foreign=regular(2)
    bases=(('regular',regular(2)),('zero',(zero,(4,0,0),(2,3,0),(2,0,2))),
           ('outside',(zero,(4,0,0),(0,4,0),(0,0,4))),
           ('obtuse',((10,5,5),(9,8,5),(5,2,1),(1,5,8))),
           ('flat',(zero,(4,0,0),(4,4,0),(0,4,0))),
           ('duplicate',(zero,(4,0,0),(4,4,0),zero)))
    for name,p in bases:
        for scale in (1,1 << (bits-4)):
            for order in it.permutations(p):
                points=tuple(tuple(v*scale for v in x) for x in order)
                rows.append((name,4,points,foreign))
    for order in it.permutations(regular(m)):
        rows.append(('globalmax',4,order,foreign))
    for delta in (-1,0,1):
        span=(1 << 20)+delta
        for offset in (0,m-span):
            for order in it.permutations(regular(span,offset)):
                rows.append(('threshold',4,order,foreign))
    shell=((5,5,0),(2,1,5),(10,5,5),(2,9,5),(5,9,8))
    for support in it.combinations(shell,4):
        for order in it.permutations(support):
            rows.append(('coshell',4,order,shell[:4]))
    for q in (1,2,3):
        p=((1,1,1),(3,1,1),(1,3,1),(1,1,3))
        rows.append(('other_arity',q,p,regular(4)))
    rng=random.Random(3042026)
    for width in (7,m):
        for _ in range(24):
            p=tuple(tuple(rng.randrange(width+1) for _ in range(3)) for _ in range(4))
            query=tuple(tuple(rng.randrange(width+1) for _ in range(3)) for _ in range(4))
            rows.append(('random',4,p,query))
    for q in (0,5):rows.append(('badq',q,foreign,foreign))
    for axis in range(3):
        for v in (-1,m+1):
            bad=list(zero);bad[axis]=v
            rows.append(('badpoint',4,foreign,(tuple(bad),*foreign[1:])))
    return rows


def geometry(row,bits):
    _,q,p,query=row
    if q not in (1,2,3,4):return 'refused parameter_out_of_range'
    if any(not 0<=v<1 << bits for point in (*p,*query) for v in point):
        return 'refused coordinate_out_of_domain'
    c=center_of(p[:q])
    if c is None:return 'degenerate'
    edges=[sub(x,p[0]) for x in p[1:q]]
    if q==1:d=1;level=(0,1)
    elif q==2:d=2;level=(dot(edges[0],edges[0]),4)
    elif q==3:
        gram=determinant([[dot(a,b) for b in edges] for a in edges]);d=2*gram
        level=(dot(edges[0],edges[0])*dot(edges[1],edges[1])*dot(sub(p[1],p[2]),sub(p[1],p[2])),4*gram)
    else:d=2*abs(orient(*p))
    ns=tuple(d*(x-a) for x,a in zip(c,p[0]))
    require(all(v.denominator==1 for v in ns),'integral coefficients')
    ns=tuple(map(int,ns))
    if q==4:level=(dot(ns,ns),d*d)
    original=is_inside(c,p);external=is_inside(c,query)
    return dict(flags=(int(q==4 and original),int(original),int(external)),anchor=p[0],n=ns,d=int(d),level=level)


def block(value,level):
    words=list(map(str,value['flags']+value['anchor']))
    words += [format(v,'x') for v in value['n']+(value['d'],)]
    if level:words += [format(v,'x') for v in value['level']]
    return words


def labels(row):
    return (('candidate',False),('materialized',True),('eager',True)) if row[1]==4 else (('sphere',True),)


def line_of(row,bits):
    expected=geometry(row,bits)
    if isinstance(expected,str):return expected
    words=['ok']
    for name,level in labels(row):words.extend([name]+block(expected,level))
    return ' '.join(words)


def judge(row,bits,line):
    expected=geometry(row,bits)
    if isinstance(expected,str):require(line==expected,'refusal/degeneracy');return 1
    words=line.split();require(words and words.pop(0)=='ok','status');checks=1
    for name,level in labels(row):
        require(words and words.pop(0)==name,'representation');checks+=1
        count=12 if level else 10
        require(len(words)>=count,'truncation')
        wanted=block(expected,level);raw=words[:count];words=words[count:]
        for j,(actual,target) in enumerate(zip(raw,wanted)):
            if j<3:require(actual in ('0','1'),'boolean token')
            require(int(actual,10 if j<6 else 16)==int(target,10 if j<6 else 16),'field'+str(j));checks+=1
    require(not words,'trailing');return checks+1


def encode(rows):
    return ''.join(' '.join(map(str,(q,)+tuple(v for point in (*p,*query) for v in point)))+'\n'
                   for _,q,p,query in rows)


def decoded(value):
    return value.decode('utf-8',errors='backslashreplace') if isinstance(value,bytes) else value or ''


def bounded(value,limit=4096):
    text=decoded(value)
    if len(text)<=limit:return text
    half=limit//2
    return text[:half]+f' [... {len(text)-limit} characters omitted ...] '+text[-half:]


def first_unverified(stdout,rows,bits):
    if rows is None:return None
    lines=decoded(stdout).splitlines(keepends=True)
    if not lines or lines[0]!=f'bits {bits}\n':return None
    for index,row in enumerate(rows):
        if index+1>=len(lines) or not lines[index+1].endswith('\n'):return index
        try:judge(row,bits,lines[index+1].rstrip('\r\n'))
        except ValueError:return index
    return None


def diagnostic(executable,phase,code,stdout,stderr,timeout=None,rows=None,bits=None,reason=None):
    record=dict(phase=phase,argv=[bounded(executable,1024)],returncode=code,timeout=timeout)
    for key,value in (('stdout',stdout),('stderr',stderr)):
        text=decoded(value);raw=value if isinstance(value,bytes) else text.encode('utf-8')
        record[key]=bounded(value);record[key+'_characters']=len(text)
        record[key+'_truncated']=len(text)>4096;record[key+'_sha256']=hashlib.sha256(raw).hexdigest()
    if reason is not None:record['reason']=bounded(str(reason),1024)
    index=first_unverified(stdout,rows,bits)
    if index is not None:
        record['first_unverified_request_index']=index;record['request']=rows[index]
    print(json.dumps(record,sort_keys=True),file=sys.stderr)


def run_child(executable,payload,timeout,phase,expected=0,rows=None,bits=None):
    try:
        child=subprocess.run([executable],input=payload.encode('utf-8'),capture_output=True,timeout=timeout)
    except subprocess.TimeoutExpired as error:
        diagnostic(executable,phase,None,error.stdout,error.stderr,timeout,rows,bits,'timeout')
        raise
    except OSError as error:
        diagnostic(executable,phase,None,'','',rows=rows,bits=bits,reason=error)
        raise
    if child.returncode!=expected or child.stderr:
        diagnostic(executable,phase,child.returncode,child.stdout,child.stderr,rows=rows,bits=bits)
        raise ValueError(phase+' process')
    return subprocess.CompletedProcess(child.args,child.returncode,decoded(child.stdout),decoded(child.stderr))


def check_output(executable,child,rows,bits):
    try:
        lines=child.stdout.splitlines();require(lines and lines.pop(0)==f'bits {bits}','batch header')
        require(len(lines)==len(rows),'request count')
        return sum(judge(row,bits,line) for row,line in zip(rows,lines))
    except ValueError as error:
        diagnostic(executable,'judge',child.returncode,child.stdout,child.stderr,rows=rows,bits=bits,reason=error)
        raise


def selftest():
    checks=corruptions=requests=0
    for bits in (21,24):
        rows=cases(bits);requests+=len(rows)
        for row in rows:checks+=judge(row,bits,line_of(row,bits))
        # Muter chaque jeton numerique du candidat, du materialise ET de through4.
        row=next(r for r in rows if r[0]=='regular');text=line_of(row,bits);words=text.split()
        indices=[i for i,x in enumerate(words) if x not in ('ok','candidate','materialized','eager')]
        for i in indices:
            bad=list(words);bad[i]='1' if bad[i]=='0' else '0'
            try:judge(row,bits,' '.join(bad))
            except ValueError:corruptions+=1
            else:raise ValueError('corruption accepte')
        for bad in ('ok',text+' trailing',text.replace('candidate','sphere',1),'degenerate'):
            try:judge(row,bits,bad)
            except ValueError:corruptions+=1
            else:raise ValueError('forme fausse acceptee')
        # Meme boule, q4 positif et quadruplets de coquille non positifs : pas de reutilisation generique du flag.
        require(any(r[0]=='coshell' and isinstance((g:=geometry(r,bits)),dict) and g['flags']==(1,1,0)
                    for r in rows),'contre-fixture autre presentation');checks+=1
        require(any(r[0]=='zero' and isinstance((g:=geometry(r,bits)),dict) and g['flags'][0]==0
                    for r in rows),'poids nul');checks+=1
    print(f'q4_presentation_model_verdict conforme requests{requests} checks{checks} corruptions{corruptions} native0')


def run(executable):
    header=run_child(executable,'',20,'header')
    try:
        fields=header.stdout.split();require(len(fields)==2 and fields[0]=='bits','header')
        bits=int(fields[1]);require(bits in (21,24),'profile')
    except ValueError as error:
        diagnostic(executable,'header',header.returncode,header.stdout,header.stderr,reason=error)
        raise
    rows=cases(bits)
    output=run_child(executable,encode(rows),90,'native',rows=rows,bits=bits)
    checks=check_output(executable,output,rows,bits)
    for malformed in ('bad\n','4 1 2\n'):
        run_child(executable,malformed,20,'malformed',expected=2)
    print(f'q4_presentation_fraction_verdict conforme bits{bits} requests{len(rows)} checks{checks} malformed2')


if __name__=='__main__':
    try:
        if len(sys.argv)==2 and sys.argv[1]=='--selftest':selftest()
        elif len(sys.argv)==2:run(sys.argv[1])
        else:raise ValueError('usage oracle EXE | --selftest')
    except (ValueError,OSError,subprocess.SubprocessError) as error:
        print('REFUS q4_presentation_oracle:',error,file=sys.stderr);sys.exit(1)
