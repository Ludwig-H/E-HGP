"""Small Fraction witnesses for numeric doctrine; no compiler, native FENV, or product test."""
from fractions import Fraction as F
from pathlib import Path
import hashlib,json

HERE=Path(__file__).resolve().parent


def need(ok,why):
    if not ok:raise RuntimeError(why)


def p2(e):
    return F(2**e) if e>=0 else F(1,2**(-e))


def rounded(value,mode):
    """Exact emulation of binary64 rounding, restricted here to finite normal results or zero."""
    value=F(value)
    if value==0:return value
    sign=1 if value>0 else -1;x=abs(value)
    exponent=x.numerator.bit_length()-x.denominator.bit_length()
    if p2(exponent)>x:exponent-=1
    need(-1022<=exponent<=1023,'emulation outside normal domain')
    unit=p2(exponent-52)
    scaled=x/unit;q,r=divmod(scaled.numerator,scaled.denominator)
    inc=(mode=='up' and sign>0 or mode=='down' and sign<0) and r!=0
    if mode=='nearest':
        inc=2*r>scaled.denominator or (2*r==scaled.denominator and q%2==1)
    if inc:q+=1
    result=sign*q*unit
    need(abs(result)<p2(1024),'rounded overflow')
    return result


pins=json.loads((HERE/'sources.json').read_text())
for row in pins:need(hashlib.sha256((HERE/row['copy']).read_bytes()).hexdigest()==row['sha256'],'document hash')
u=p2(-52);L=1-u
n=F(2**54+1)
a=rounded(n,'up');b=rounded(a*a,'up')
error=b/(n*n)-1
need(error>(1+u)**2-1,'old counterexample missing')
need(L**3<=b/(n*n)<=L**-3,'new F3 exponent 3 failed')

# F3 endpoint algebra: inverse preserves the symmetric exponent interval.
checks=0
for ea,eb in ((0,0),(0,1),(1,1),(2,3),(12,17)):
    for ra in (L**ea,L**-ea):
        for rb in (L**eb,L**-eb):
            for r1 in (L,L**-1):
                for r2 in (L,L**-1):
                    need(L**(ea+eb+1)<=ra*rb*r1<=L**-(ea+eb+1),'product interval')
                    need(L**(ea+eb+2)<=ra/rb*r1*r2<=L**-(ea+eb+2),'inverse quotient interval')
                    weighted=(F(2)*ra+F(3)*rb)/5*r1
                    need(L**(max(ea,eb)+1)<=weighted<=L**-(max(ea,eb)+1),'same-sign sum interval')
                    checks+=3

# F4 constant and the extra factor for rounding c*y; Bernoulli is also proved in the report.
c=1-p2(-40)
need(c<=L**4096,'F4 constant')
f4=[]
for ex,ey in ((0,0),(1,3),(17,31),(2047,2048)):
    need(ex+ey+1<=4096,'F4 test precondition')
    worst_comparison=c/L*L**(-ex-ey)
    need(worst_comparison<=1,'F4 safety margin')
    f4.append([ex,ey])
for e in (0,1,2,3,67,4096):
    need(e*u<=F(1,2),'gamma precondition')
    need(L**-e-1<=2*e*u,'gamma <= 2 E u')

# Existing Q2 cancellation witness: F2 is inapplicable, F6 threshold safely requests exact replay.
A=F(512*32767**3)
need(A>2**53 and rounded(A,'nearest')==A,'large exact leaf witness')
M=2*A+2;E=3;q=55;e=2;tau=p2(q+e-51)
need(M<=p2(q) and E<=2**e,'Q2 threshold budgets')
cancellation=[]
for mode in ('nearest','up','down','zero'):
    value=rounded(rounded(rounded(A+1,mode)-A,mode)-1,mode)
    need(abs(value)<= (L**-E-1)*M,'F6 cancellation error')
    need(abs(value)<=tau,'wrong sign could pass threshold')
    cancellation.append({'mode':mode,'value':str(value),'exact':'0','threshold':str(tau),'replay_exact':True})

# Distinct domain obligation: expression remains exact and finite while the threshold is not finite binary64.
a=p2(17);Ma=a;Ea=0
for _ in range(5):
    a=a*a;Ma=Ma*Ma;Ea=2*Ea+1
    need(a<=p2(1023),'power operand overflow')
need(a==p2(544) and Ea==31,'five-square construction')
Mv=(2*Ma+1)**2;Ev=2*(Ea+2)+1
qv=Mv.numerator.bit_length();ev=(Ev-1).bit_length();te=qv+ev-51
need(Mv<=p2(qv) and Ev<=2**ev and Ev*u<=F(1,2),'threshold witness budgets')
need((qv,ev,Ev,te)==(1091,7,67,1047),'threshold witness')
for mode in ('nearest','up','down','zero'):
    inside=rounded(rounded(a-a,mode)+1,mode)
    need(rounded(inside*inside,mode)==1,'finite exact cancellation expression')
need(te>1023,'threshold must be out of finite binary64 range')

# Transformation domain: a/b can be normal while 1/b is subnormal.
den=p2(1023)
need(den/den==1 and 0<1/den<p2(-1022),'inverse intermediate domain witness')
for row in pins:need(hashlib.sha256((HERE/row['copy']).read_bytes()).hexdigest()==row['sha256'],'document changed')
print(json.dumps({'status':'PASS','old_F3_rejected_new_E3_accepts':True,
                  'F3_endpoint_checks':checks,'F4_pairs':f4,'gamma_exponents':[0,1,2,3,67,4096],
                  'Q2_cancellation':cancellation,
                  'threshold_domain_witness':{'input':2**17,'five_square_exponent':544,'exact_value':1,
                                              'E':Ev,'q':qv,'e':ev,'threshold_exponent':te,
                                              'finite_binary64_threshold':False},
                  'inverse_domain_witness':{'quotient':'2^1023 / 2^1023 = 1','inverse':'2^-1023 (subnormal)'},
                  'scope':'bounded Fraction proof witnesses, no native numeric filter, no compilation or GCP'},
                 sort_keys=True,indent=2))
