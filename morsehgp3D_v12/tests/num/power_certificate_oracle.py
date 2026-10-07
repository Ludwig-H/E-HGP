"""Certificat q3 global : centres Gram/Fraction et preuve des intermediaires pour points et boites."""
import hashlib
import json
import subprocess
import sys
from collections import Counter
import checked_power_oracle as base

require = base.require


def cases(bits):
    rows = base.cases(bits)
    zero, top = (0,0,0), ((1 << bits)-1,)*3
    m=(1 << bits)-1
    near=(1,0,0)
    rows.append(('uncertified_small_query',(zero,(m,m,0),(m,0,m)),near,near,near))
    cross_bits = (123-2*bits-1)//2
    a,b = 1 << (cross_bits//2), 1 << (cross_bits-cross_bits//2)
    for offset in (-1,0,1):
        points = (zero,(a,0,0),(0,b+offset,0))
        rows.append(('denominator_boundary_'+str(offset),points,top,zero,top))
    if bits == 24:
        # D reste petit mais le centre d'un triangle obtus peut etre loin du domaine Point.
        m=(1 << bits)-1
        rows.append(('numerator_only', (zero,(1 << (bits-1),0,0),(m,256,0)),top,zero,top))
    return rows


def certificate(value,bits):
    return 0 < value['d'] < 1 << (123-2*bits) and all(abs(n) < 1 << (124-bits) for n in value['n'])


def line_of(value,bits):
    line = base.line_of(value)
    return line if isinstance(value,str) else line+' '+str(int(certificate(value,bits)))


def judge(case,bits,line):
    expected = base.geometry(case,bits)
    if isinstance(expected,str):
        return base.judge(case,bits,line)
    words = line.split()
    require(len(words)==13, 'certificate response shape')
    count = base.judge(case,bits,' '.join(words[:-1]))
    require(words[-1] == str(int(certificate(expected,bits))), 'global certificate')
    return count+1


def proof_cases(bits):
    d_limit,n_limit=1 << (123-2*bits),1 << (124-bits)
    m=(1 << bits)-1
    norm,factor=3*m*m,2*m
    # Derivation globale : tester tous signes extremaux aux coins de la boite des COEFFICIENTS.
    checks=0
    for mask in range(8):
        ns=[(n_limit-1)*(1 if mask & (1 << j) else -1) for j in range(3)]
        terms=[(d_limit-1)*norm]+[n*factor for n in ns]
        total=0
        for term in terms:
            require(abs(term) < 1 << 127, 'certificate product bound')
            total+=term
            require(-(1 << 127) <= total < 1 << 127, 'certificate partial sum bound')
            checks+=2
    return checks


def facts(rows,bits):
    checks=base.facts(rows,bits)+proof_cases(bits)
    values={r[0]:base.geometry(r,bits) for r in rows}
    require(certificate(values['contact'],bits), 'small triangle globally certified')
    require(not certificate(values['product_cancellation'],bits), 'extreme cancellation fallback')
    small_query=values['uncertified_small_query']
    require(not certificate(small_query,bits), 'small query is not a global certificate')
    require(small_query['flags'][0]==1, 'non-certified controlled path succeeds')
    require(small_query['power']<0, 'non-certified controlled path is nonzero')
    all_values=[base.geometry(row,bits) for row in rows]
    require(any(any(n<0 for n in v['n']) and certificate(v,bits) for v in all_values if isinstance(v,dict)),
            'negative numerator certification')
    checks+=6
    for offset in (-1,0,1):
        value=values['denominator_boundary_'+str(offset)]
        require(certificate(value,bits) == (offset<0), 'strict D threshold'); checks+=1
    require(values['denominator_boundary_0']['d'] == 1 << (123-2*bits), 'exact equality D'); checks+=1
    if bits==24:
        value=values['numerator_only']
        require(value['d'] < 1 << (123-2*bits) and not certificate(value,bits), 'N bound independently necessary')
        require(not certificate(values['local_support_global_query'],bits), 'local19bit support not a global certificate')
        checks+=2
    return checks


def inventory(rows,bits):
    values=[base.geometry(row,bits) for row in rows]
    good=[v for v in values if isinstance(v,dict)]
    return dict(requests=len(rows),certified=sum(certificate(v,bits) for v in good),
                uncertified=sum(not certificate(v,bits) for v in good),
                outcomes=dict(Counter('ok' if isinstance(v,dict) else v for v in values)))


def selftest():
    reports=[]
    for bits in (21,24):
        rows=cases(bits)
        checks=sum(judge(row,bits,line_of(base.geometry(row,bits),bits)) for row in rows)+facts(rows,bits)
        corruptions=0
        for name in ('contact','product_cancellation'):
            row=next(r for r in rows if r[0]==name)
            good=line_of(base.geometry(row,bits),bits).split()
            bads=[]
            for i in range(1,13):
                bad=good.copy(); bad[i]='deadbeef' if i<6 or i in (7,8) else '9'; bads.append(' '.join(bad))
            bad=good.copy();bad[-1]=str(1-int(bad[-1]));bads.append(' '.join(bad))
            bads.extend(('', 'ok', ' '.join(good[:-1]), ' '.join(good+['0'])))
            for line in bads:
                try: judge(row,bits,line)
                except ValueError: corruptions+=1
                else: raise ValueError('corrupted certificate response accepted')
        require(corruptions==34,'non-vacuous corruptions')
        reports.append(dict(bits=bits,checks=checks,corruptions=corruptions,native=0,**inventory(rows,bits)))
    print(json.dumps(dict(verdict='conforme',profiles=reports),sort_keys=True))


def run(executable):
    header=subprocess.run([executable],input='',text=True,capture_output=True,timeout=5)
    require(header.returncode==0 and not header.stderr,'header process')
    words=header.stdout.split()
    require(len(words)==2 and words[0]=='bits' and int(words[1]) in (21,24),'header bits')
    bits=int(words[1]); rows=cases(bits); payload=base.payload(rows)
    child=subprocess.run([executable],input=payload,text=True,capture_output=True,timeout=45)
    require(child.returncode==0 and not child.stderr,'certificate native process')
    lines=child.stdout.splitlines()
    require(lines and lines[0]=='bits '+str(bits) and len(lines)==len(rows)+1,'complete output')
    checks=sum(judge(row,bits,line) for row,line in zip(rows,lines[1:]))+facts(rows,bits)
    require(checks>=3900 and len(rows)>=329,'non-vacuous native judge')
    print(json.dumps(dict(verdict='conforme',bits=bits,checks=checks,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                         **inventory(rows,bits)),sort_keys=True))


if __name__=='__main__':
    try:
        if sys.argv[1:]==['--selftest']: selftest()
        elif len(sys.argv)==2: run(sys.argv[1])
        else: raise ValueError('usage: power_certificate_oracle.py EXE|--selftest')
    except (ValueError,OSError,subprocess.SubprocessError,IndexError) as error:
        print('REFUS power_certificate_oracle:',error,file=sys.stderr);sys.exit(1)
