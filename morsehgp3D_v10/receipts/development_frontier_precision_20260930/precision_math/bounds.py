#!/usr/bin/env python3
"""Conservative exact bit ledger and valid-support overflow fixtures; no engine."""
import json
from fractions import Fraction as F

def req(ok,msg):
    if not ok: raise RuntimeError(msg)
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def center3(a,b,c):
    u,v=sub(b,a),sub(c,a)
    w=cross(u,v)
    t=tuple(dot(u,u)*y-dot(v,v)*x for x,y in zip(u,v))
    return cross(t,w),2*dot(w,w)
def center4(a,b,c,d):
    u,v,s=sub(b,a),sub(c,a),sub(d,a)
    N=tuple(dot(u,u)*x+dot(v,v)*y+dot(s,s)*z for x,y,z in zip(cross(v,s),cross(s,u),cross(u,v)))
    D=2*dot(u,cross(v,s))
    return (tuple(-x for x in N),-D) if D<0 else (N,D)
def side(N,D,a,z):
    u=sub(z,a)
    return D*dot(u,u)-2*dot(N,u)
def bits(v): return abs(v).bit_length()

tables=[]
for B in (18,24,32):
    M=(1<<B)-1
    Ep=1<<(B+6)
    upper={
      'distance_dot':3*M**2,'cross_component':2*M**2,'orientation_points':6*M**3,
      'center3_N':36*M**5,'center3_D':24*M**4,'center4_N':18*M**4,'center4_D':12*M**3,
      'side3':288*M**6,'side4':144*M**5,
      'orient_center3':360*M**7,'orient_center4':180*M**6,
      'midpoint3':120*M**5,'midpoint4':60*M**4,
      'center_box3':64*60*M**5,'center_box4':64*30*M**4,
      'level2_num':3*M**2,'level3_num':27*M**6,'level3_den':48*M**4,
      'level4_num':972*M**8,'level4_den':144*M**6,
      'level_cross_product':139968*M**14,
      'box_pool_key':27*Ep**2,'box_line_final':72*Ep**3,
    }
    tables.append({'coordinate_bits':B,'magnitude_bits_upper_bound':{k:bits(v) for k,v in upper.items()},
                   '64_bit_limbs_sufficient':{k:(bits(v)+63)//64 for k,v in upper.items()}})

fixtures=[]
for B in (24,32):
    M=(1<<B)-1
    a=(0,0,0)
    b,c,d=(M,M,0),(M,0,M),(0,M,M)
    N3,D3=center3(a,b,c)
    N4,D4=center4(a,b,c,d)
    req(N3==(4*M**5,2*M**5,2*M**5) and D3==6*M**4,'q3 formula')
    req(N4==(2*M**4,)*3 and D4==4*M**3,'q4 formula')
    # The triangle is equilateral/strictly acute; tetrahedron regular with all barycentrics1/4.
    req(all(v>0 for v in (dot(sub(b,a),sub(c,a)),dot(sub(a,b),sub(c,b)),dot(sub(a,c),sub(b,c)))), 'acute')
    ctr4=tuple(F(x,D4) for x in N4)
    req(ctr4==(F(M,2),)*3 and tuple(F(sum(p[i] for p in (a,b,c,d)),4) for i in range(3))==ctr4,
        'positive tetra centre')
    # Raw exact radius coefficients, as in current geometry.cpp (no hot gcd).
    lv3_num=dot(sub(b,a),sub(b,a))*dot(sub(c,a),sub(c,a))*dot(sub(c,b),sub(c,b))
    lv3_den=4*dot(cross(sub(b,a),sub(c,a)),cross(sub(b,a),sub(c,a)))
    lv4_num,lv4_den=dot(N4,N4),D4*D4
    req(F(lv3_num,lv3_den)==F(2*M*M,3),'triangle radius')
    req(F(lv4_num,lv4_den)==F(3*M*M,4),'tetra radius')
    w=cross(sub(b,a),sub(c,a))
    orient4=dot(w,N4)
    values={'distance_squared':3*M*M,'cross_component':M*M,
            'q3_Nmax':max(N3),'q3_D':D3,'q3_side_at_MMM':side(N3,D3,a,(M,M,M)),
            'q4_Nmax':max(N4),'q4_D':D4,'q4_orientation_center':orient4,
            'q3_level_num':lv3_num,'q3_level_den':lv3_den,
            'q4_level_num':lv4_num,'q4_level_den':lv4_den,
            'q4_level_cross_product':lv4_num*lv4_den}
    fixtures.append({'coordinate_bits':B,'support':[a,b,c,d],
                     'exact_values':{k:str(v) for k,v in values.items()},
                     'magnitude_bits':{k:bits(v) for k,v in values.items()},
                     'all_supports_strictly_positive':True})

req(tables[-1]['magnitude_bits_upper_bound']['distance_dot']==66,'distance bits')
req(tables[-1]['magnitude_bits_upper_bound']['level_cross_product']==466,'cross bits')
req(fixtures[-1]['magnitude_bits']['q3_level_num']>192,'q3 level counterexample')
req(fixtures[-1]['magnitude_bits']['q4_Nmax']>127,'q4 center counterexample')
req(fixtures[0]['magnitude_bits']['q4_level_num']>192,'u24 level counterexample')
print(json.dumps({'status':'PASS','scope':'exact arithmetic only; no engine/native/GCP',
                  'tables':tables,'valid_positive_supports':fixtures},sort_keys=True,indent=1))
