from fractions import Fraction as F
from math import isqrt
from pathlib import Path
from types import SimpleNamespace
import ast, base64, hashlib, json, sys

def need(ok, label):
    if not ok:
        raise RuntimeError(label)

def rational_sqrt(q):
    a,b = isqrt(q.numerator),isqrt(q.denominator)
    return F(a,b) if a*a == q.numerator and b*b == q.denominator else None

class QS:
    # Exact square-class combination and dyadic rational sign enclosures.
    def __init__(self, terms=None):
        self.t = {}
        for q,c in (terms or {}).items():
            self.add_term(F(q),F(c))

    def add_term(self,q,c):
        if q == 0 or c == 0:
            return
        r = rational_sqrt(q)
        if r is not None:
            q,c = F(1),c*r
        for key in list(self.t):
            r = rational_sqrt(q/key)
            if r is not None:
                q,c = key,c*r
                break
        self.t[q] = self.t.get(q,F(0))+c
        if self.t[q] == 0:
            del self.t[q]

    @staticmethod
    def sqrt(q):
        return QS({F(q):1})

    def __add__(self,other):
        other = other if isinstance(other,QS) else QS({1:F(other)})
        result = QS(self.t)
        for q,c in other.t.items():
            result.add_term(q,c)
        return result

    def __neg__(self):
        return QS({q:-c for q,c in self.t.items()})

    def __sub__(self,other):
        other = other if isinstance(other,QS) else QS({1:F(other)})
        return self+(-other)

    def __mul__(self,coefficient):
        return QS({q:c*F(coefficient) for q,c in self.t.items()})

    def cmp(self,other):
        terms = (self-other).t
        if not terms:
            return 0
        bits = 32
        while True:
            low,high = F(0),F(0)
            scale = 2**bits
            for q,c in terms.items():
                k = isqrt(q.numerator*scale*scale//q.denominator)
                lo = F(k,scale)
                hi = lo if lo*lo == q else F(k+1,scale)
                low += c*(lo if c > 0 else hi)
                high += c*(hi if c > 0 else lo)
            if low > 0:
                return 1
            if high < 0:
                return -1
            bits *= 2

    def dump(self):
        return {str(q):str(c) for q,c in sorted(self.t.items())}

class Rayon:
    def __init__(self,b2=None,value=None):
        self.value = value if value is not None else QS.sqrt(b2)

    @staticmethod
    def somme(value):
        return Rayon(value=value)

    def cmp(self,other):
        return self.value.cmp(other.value)

def oracle(eta,kappa):
    # True geometric cloud: (0,0,0),(2,0,0), K=2. Unique Gamma_2 vertex.
    # A=1; mass(s)=s-1 on [1,1+eta]; W=eta.
    half,end = 1+eta/2,1+eta
    def value(s):
        return QS.sqrt(s)-QS.sqrt(1)*(kappa*(2*(s-1)/eta-1))
    candidates = [value(half),value(end)]
    critical = eta*eta/(16*kappa*kappa)
    if half < critical < end:
        candidates.append(value(critical))
    result = candidates[0]
    for candidate in candidates[1:]:
        if candidate.cmp(result) > 0:
            result = candidate
    return result

if len(sys.argv) == 3 and sys.argv[1] == '--snapshot-b64':
    raw = base64.b64decode(sys.argv[2],validate=True)
else:
    need(len(sys.argv) == 1, 'argv')
    raw = Path(__file__).with_name('mmt_snapshot.py').read_bytes()
SOURCE_SHA = '93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818'
need(hashlib.sha256(raw).hexdigest() == SOURCE_SHA, 'actual shared source snapshot pin')
parsed = ast.parse(raw)
defs = [node for node in parsed.body if isinstance(node,ast.FunctionDef) and node.name in {'_anc','mmt_point'}]
need({d.name for d in defs} == {'_anc','mmt_point'}, 'actual function inventory')

class EndpointFix(ast.NodeTransformer):
    def __init__(self):
        self.changed = 0

    def visit_Compare(self,node):
        if (isinstance(node.left,ast.Name) and node.left.id == 'Gm'
                and len(node.ops) == len(node.comparators) == 1
                and isinstance(node.ops[0],ast.Lt)
                and isinstance(node.comparators[0],ast.Name) and node.comparators[0].id == 'W'):
            self.changed += 1
            return ast.copy_location(ast.BoolOp(op=ast.Or(),values=[
                node,
                ast.BoolOp(op=ast.And(),values=[
                    ast.Compare(left=ast.Name(id='Gm',ctx=ast.Load()),ops=[ast.Eq()],comparators=[ast.Name(id='W',ctx=ast.Load())]),
                    ast.Compare(left=ast.Name(id='T1',ctx=ast.Load()),ops=[ast.Is()],comparators=[ast.Constant(value=None)])])]),node)
        return self.generic_visit(node)

T = SimpleNamespace(birth=[F(1)],death=[None],parent=[-1])
eta = F(2,3)
rows = []
for patched in (False,True):
    module = ast.Module(body=defs,type_ignores=[])
    # Reparse for each run; transforming a shared AST would mutate the original path.
    module = ast.parse(ast.unparse(module))
    if patched:
        fix = EndpointFix()
        module = fix.visit(module)
        need(fix.changed == 2, 'two endpoint exclusion guards')
    namespace = {'Fraction':F,'QS':QS,'Rayon':Rayon,'exiger':need}
    exec(compile(ast.fix_missing_locations(module),'<pinned actual mmt_point>','exec'),namespace)
    for kappa in (F(1,10),F(2,15),F(4)):
        result = namespace['mmt_point'](T,{0:F(1)},eta,kappa)
        expected = oracle(eta,kappa)
        agrees = result['date'].cmp(expected) == 0
        need(agrees == (patched or kappa != F(1,10)), 'causal endpoint mismatch')
        rows.append({'patched_endpoint_only':patched,'eta':str(eta),'kappa':str(kappa),
                     'actual':result['date'].dump(),'analytic_sup':expected.dump(),
                     'agrees':agrees,'argmax':str(result['argmax'])})
print(json.dumps({'status':'PASS','scope':'actual_function_snapshot_with_exact_helpers_geometric_two_site_branch_not_native',
    'source_sha256':SOURCE_SHA,'cloud':[[0,0,0],[2,0,0]],'K':2,'rows':rows},sort_keys=True))
