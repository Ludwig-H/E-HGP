"""Bounded reference incidences and exact bounds of F5 witnesses; no C++ execution."""
from pathlib import Path
from fractions import Fraction
from itertools import combinations
import ast
import hashlib
import json
import math
import re
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'oracle'))
from hgp11_ref.supports import Supports

CHECKS = 0
def need(ok, message):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise SystemExit(message)

meta = json.loads((ROOT / 'ORACLE_BEFORE.json').read_text())
for rel, digest in meta['sources'].items():
    need(hashlib.sha256((ROOT/'oracle'/rel).read_bytes()).hexdigest() == digest, 'oracle source pin')

points = [(0,0,0), (1,0,0), (2,0,0)]
oracle = Supports(points)
order = oracle.order(2)
doc = oracle.canonical(2)
part_incidence = 0
parts_unique = set()
cofaces_unique = set()
coface_incidence = 0
for ball in order.balls:
    parts = set(combinations(ball.pop, 2))
    cofaces = set()
    for face in combinations(ball.pop, 3):
        if oracle.definition.meb(face)[0] == ball.level:
            cofaces.add(face)
    need(len(parts)==ball.counts['kparties_reliees'], 'K-parties per ball')
    need(len(cofaces)==ball.counts['cofaces'], 'cofaces per ball')
    need(not cofaces_unique.intersection(cofaces), 'unique MEB disjoins event cofaces')
    part_incidence += len(parts)
    parts_unique.update(parts)
    coface_incidence += len(cofaces)
    cofaces_unique.update(cofaces)
need(part_incidence==5 and len(parts_unique)==3, 'sum of K-parts is incidence count, not union cardinal')
need(coface_incidence==1 and len(cofaces_unique)==1, 'event coface count on three-site line')
weak = next(b for b in order.balls if b.p==1)
need(weak.level==1 and weak.q==2 and weak.m==2, 'weak endpoint ball')
need(oracle.definition.meb((0,2))[0]==weak.level, 'non-strict arbitrary K-part')
need(oracle.definition.meb((0,1))[0]==Fraction(1,4), 'compressed trace remains strict')

# Even with the same diametral support and the same ball, p+m can change at a contact.
contact_counts=[]
for delta in (0,1):
    cloud=[(0,10,0),(20,10,0),(10,20+delta,0)]
    instance=Supports(cloud)
    ball=instance.shell_ball((0,1))
    minimal=instance._supports(ball)
    need(ball.level==100 and ball.center==(10,10,0), 'fixed diametral ball')
    need(minimal==((0,1),), 'same Q despite a point crossing the shell')
    contact_counts.append(math.comb(ball.p+ball.m,2))
need(contact_counts==[3,1], 'K-part counts are not stable under arbitrary perturbations')

source = (ROOT/'source/src/api/selftest.cpp').read_text()
need('constexpr u64 kMinusTwoPow62MinusOne = ~(u64{1} << 62);' in source,
     'signed conversion source constant matches its exact model')
constants = {'kMinusTwoPow62MinusOne': (1<<64)-1-(1<<62)}

def cpp_constant(text):
    text = re.sub(r'u64\{(\d+)\}', r'\1', text.strip())
    names = dict(constants)
    def hex_literal(m):
        name = 'hex_' + str(len(names))
        names[name] = Fraction.from_float(float.fromhex(m.group()))
        return name
    text = re.sub(r'0x[0-9a-fA-F.]+p[+-]?\d+', hex_literal, text)
    def value(node):
        if isinstance(node, ast.Constant):
            return Fraction(node.value) if isinstance(node.value,float) else node.value
        if isinstance(node,ast.Name):
            return names[node.id]
        if isinstance(node,ast.UnaryOp):
            x=value(node.operand)
            if isinstance(node.op,ast.USub):return -x
            if isinstance(node.op,ast.Invert):return ((1<<64)-1) ^ int(x)
        if isinstance(node,ast.BinOp):
            a,b=value(node.left),value(node.right)
            if isinstance(node.op,ast.Add):return a+b
            if isinstance(node.op,ast.Sub):return a-b
            if isinstance(node.op,ast.Mult):return a*b
            if isinstance(node.op,ast.LShift):return int(a)<<int(b)
        raise ValueError(ast.dump(node))
    return value(ast.parse(text,mode='eval').body)

witnesses=[]
for match in re.finditer(r'^\s*\{Op::(\w+),\s*(.*?)\},?\s*$',source,re.M):
    op=match.group(1)
    vals=[cpp_constant(t) for t in match.group(2).split(',')]
    need(len(vals)==5, 'witness columns')
    a,b,n,lo,hi=vals
    need(Fraction.from_float(float(lo))==lo and Fraction.from_float(float(hi))==hi,
         'endpoint is exactly binary64-representable')
    if op=='excess':
        need((lo,hi)==(0,2) and a==(1<<53) and b==1, 'excess witness uses binary64 neighbors')
        exact=None
    else:
        if op=='add':exact=a+b
        elif op=='sub':exact=a-b
        elif op=='mul':exact=a*b
        elif op=='div':exact=Fraction(a)/Fraction(b)
        elif op=='from_u64':exact=n
        elif op=='from_i64':exact=n-(1<<64) if n>=(1<<63) else n
        else:raise ValueError(op)
        need(lo<=exact<=hi, 'exact result bounded by witness endpoints')
        if lo==hi:
            need(exact==lo, 'F2 witness is exact')
        else:
            need(Fraction.from_float(math.nextafter(float(lo),math.inf))==hi,
                 'F3 endpoints are adjacent binary64 values')
    witnesses.append({'op':op,'lo':str(lo),'hi':str(hi),'exact':None if exact is None else str(exact)})
need(len(witnesses)==15, 'all copied F5 witnesses covered')
budgets=[]
for bits in (18,21,24):
    bound=5*bits+6
    need(bound<=127, 'global center numerator fits signed i128')
    budgets.append({'coord_bits':bits,'global_center_numerator_bits':bound})

print(json.dumps({'status':'PASS','checks':CHECKS,'native_executed':False,
                  'reference_orders_executed':1,'line_k2':{'K_part_incidence_sum':part_incidence,
                  'K_parts_union':len(parts_unique),'event_cofaces':len(cofaces_unique)},
                  'fixed_ball_contact_counts':contact_counts,
                  'F5_exact_witnesses':witnesses,'global_center_writer_bounds':budgets,
                  'scope':'one three-site Fraction oracle and exact source witness arithmetic; no native FENV, IO or profiling'},
                 sort_keys=True,indent=2))
