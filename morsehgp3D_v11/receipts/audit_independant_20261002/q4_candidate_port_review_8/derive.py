"""Independent tiny rational model + token comparisons. No C++ execution."""
from pathlib import Path
from fractions import Fraction as F
import itertools,json,re
ROOT=Path(__file__).resolve().parent

def require(value,msg):
 if not value: raise ValueError(msg)
def text_at(kind,name): return (ROOT/kind/'morsehgp3D_v11/src/num'/name).read_text()
def tokens(code):
 code=re.sub(r'/\*.*?\*/','',code,flags=re.S)
 code=re.sub(r'//[^\n]*','',code)
 return re.findall(r'[A-Za-z_][A-Za-z_0-9]*|[0-9]+|[^\s]',code)
def body(code,signature):
 start=code.index(signature); brace=code.index('{',start); depth=1; i=brace+1
 while depth:
  depth += (code[i]=='{')-(code[i]=='}'); i+=1
 return code[brace+1:i-1]
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def sign(v): return (v>0)-(v<0)
def solve(matrix,rhs):
 rows=[[F(x) for x in row]+[F(v)] for row,v in zip(matrix,rhs)]
 n=len(rhs)
 for col in range(n):
  pivot=next((i for i in range(col,n) if rows[i][col]),None)
  if pivot is None: return None
  rows[col],rows[pivot]=rows[pivot],rows[col]
  scale=rows[col][col]; rows[col]=[v/scale for v in rows[col]]
  for i in range(n):
   if i!=col:
    scale=rows[i][col]; rows[i]=[v-scale*w for v,w in zip(rows[i],rows[col])]
 return tuple(rows[i][-1] for i in range(n))
def gram(points):
 a=points[0]; edges=[sub(p,a) for p in points[1:]]
 weights=solve([[dot(u,v) for v in edges] for u in edges],[F(dot(u,u),2) for u in edges])
 return None if weights is None else tuple(a[j]+sum(w*u[j] for w,u in zip(weights,edges)) for j in range(3))
def cramer(points):
 a,b,c,d=points; u,v,s=sub(b,a),sub(c,a),sub(d,a)
 vs,su,uv=cross(v,s),cross(s,u),cross(u,v); det=dot(u,vs)
 if not det: return None
 n=tuple(dot(u,u)*vs[j]+dot(v,v)*su[j]+dot(s,s)*uv[j] for j in range(3)); den=2*det
 if den<0: den=-den; n=tuple(-x for x in n)
 return a,n,den,det

def derive():
 checks=[]
 def checked(name,value):
  require(value,name); checks.append(name)
 old=text_at('baseline9df','sphere.cpp'); new=text_at('sources','sphere.cpp')
 for name in ('budgets.hpp','geometry_internal.hpp','integer.hpp','level.hpp','wide.hpp','module.cmake','num.hpp'):
  checked('unchanged file '+name,text_at('baseline9df',name)==text_at('sources',name))
 for sig in ('Result<Point> Point::make(', 'Sphere Sphere::point(', 'Sphere::through(Point a, Point b) noexcept', 'Sphere::through(Point a, Point b, Point c) noexcept'):
  checked('unchanged body '+sig,tokens(body(old,sig))==tokens(body(new,sig)))
 oldq=body(old,'Sphere::through(Point a, Point b, Point c, Point d) noexcept')
 newq=body(new,'Q4Candidate::through(Point a, Point b, Point c, Point d) noexcept')
 oldcentre=oldq[:oldq.index('constexpr int words')].replace('std::optional<Sphere>','std::optional<Q4Candidate>')
 newcentre=newq[:newq.index('return std::optional<Q4Candidate>{Q4Candidate')]
 checked('same q4 centre operations and affine refusal',tokens(oldcentre)==tokens(newcentre))
 oldlevel=oldq[oldq.index('constexpr int words'):oldq.index('return std::optional<Sphere>{Sphere')]
 newlevel=body(new,'Q4Candidate::materialize() const noexcept')
 newlevel=newlevel[newlevel.index('constexpr int words'):newlevel.index('return Sphere')]
 newlevel=re.sub(r'\bnumerator_\b','n',newlevel); newlevel=re.sub(r'\bdenominator_\b','denominator',newlevel)
 checked('same q4 Level operations and propagated failures',tokens(oldlevel)==tokens(newlevel))
 oldp=text_at('baseline9df','predicates.cpp'); newp=text_at('sources','predicates.cpp')
 pairs=[('bool use_native_power(', 'bool use_native_power('),('i128 native_power(', 'i128 native_power('),('Result<SideInt> wide_power(', 'Result<SideInt> wide_power('),('Result<SideInt> power(', 'Result<SideInt> center_power('),('Result<int> side(', 'Result<int> center_side('),('DeterminantInt orientation(', 'DeterminantInt orientation('),('Result<int> orientation(', 'Result<int> center_orientation('),('bool strictly_acute(', 'bool strictly_acute('),('Result<bool> strictly_inside(', 'Result<bool> center_inside('),('bool is_midpoint(', 'bool center_midpoint(')]
 for before,after in pairs:
  b=body(oldp,before); a=body(newp,after)
  if after=='Result<bool> center_inside(': a=a.replace('center_orientation(face','orientation(face')
  checked('same predicate body '+after,tokens(b)==tokens(a))
 h=text_at('sources','geometry.hpp'); candidate=h[h.index('class Q4Candidate {'):h.index('// Signes geometriques')]
 public,private=candidate.split(' private:',1)
 checked('candidate constructor is private','Q4Candidate(Point anchor' not in public and 'Q4Candidate(Point anchor' in private)
 checked('candidate has no Level/default factory/cache','Level' not in candidate and 'mutable' not in candidate and 'Q4Candidate()' not in candidate)
 checked('candidate owns N','std::array<CenterInt, 3> numerator_;' in private)
 checked('candidate tag4 constant','return 4;' in public)
 checked('Sphere publication has complete same coefficients','return Sphere(anchor_, numerator_, denominator_, level.value(), 4);' in new)
 checked('shared centre view private anonymous namespace',newp.index('namespace {')<newp.index('class CenterView')<newp.index('}  // namespace'))
 for kind,kernel in [('power','center_power'),('side','center_side'),('orientation','center_orientation'),('strictly_inside','center_inside'),('is_midpoint','center_midpoint')]:
  
  ret='bool' if kind=='is_midpoint' else ('Result<SideInt>' if kind=='power' else ('Result<bool>' if kind=='strictly_inside' else 'Result<int>'))
  for owner in ('Sphere','Q4Candidate'):
   sig=(ret+' '+kind+'(Point a, Point b, Point c, const '+owner+'& center') if kind=='orientation' else ret+' '+kind+'(const '+owner+'&'
   checked('shared overload '+kind+' '+owner,kernel+'(' in body(newp,sig) and 'CenterView(' in body(newp,sig))

 profile_rows=[]; examples={}
 for bits in (18,21,24):
  m=(1<<bits)-1
  fixtures=[('regular_extreme',[(0,0,0),(m,m,0),(m,0,m),(0,m,m)]),('outside',[(0,0,0),(4,0,0),(0,4,0),(0,0,4)]),('zero_weight',[(0,0,0),(4,0,0),(2,3,0),(2,0,2)]),('sliver',[(0,0,0),(m,1,0),(m-1,1,0),(m,1,1)]),('antipodal',[(0,1,1),(2,1,1),(1,2,1),(1,1,2)]),('coplanar',[(0,0,0),(4,0,0),(0,4,0),(4,4,0)]),('duplicate',[(0,0,0),(1,0,0),(0,1,0),(0,0,0)])]
  nchecks=0; count=degenerate=0; max_native_bits=0; pos=neg=0
  def check(value,msg):
   nonlocal nchecks
   require(value,msg); nchecks+=1
  for label,points in fixtures:
   for pp in itertools.permutations(points):
    c=gram(pp); candidate=cramer(pp); count+=1
    check((c is None)==(candidate is None),'affine degeneracy '+label)
    if c is None: degenerate+=1; continue
    a,n,den,det=candidate; pos+=det>0; neg+=det<0
    check(den>0 and den==2*abs(det),'positive nonreduced D')
    check(tuple(a[j]+F(n[j],den) for j in range(3))==c,'Gram centre')
    r=dot(sub(c,a),sub(c,a)); check(F(dot(n,n),den*den)==r,'nonreduced q4 Level')
    weights=solve([[F(p[j]) for p in pp] for j in range(3)]+[[F(1)]*4],list(c)+[F(1)])
    strict=True
    for opposite in range(4):
     face=[p for i,p in enumerate(pp) if i!=opposite]; normal=cross(sub(face[1],face[0]),sub(face[2],face[0]))
     vs=dot(normal,sub(pp[opposite],face[0])); cs=dot(normal,tuple(n[j]+den*(a[j]-face[0][j]) for j in range(3)))
     check(sign(cs)==sign(dot(normal,sub(c,face[0]))),'centre orientation')
     strict &= bool(vs) and sign(cs)==sign(vs)
    check(strict==(weights is not None and all(w>0 for w in weights)),'strict containment including null/negative barycentric')
    mid=all(2*(den*a[j]+n[j])==den*(pp[0][j]+pp[1][j]) for j in range(3))
    check(mid==all(2*c[j]==pp[0][j]+pp[1][j] for j in range(3)),'midpoint')
    check(max(abs(x).bit_length() for x in n)<=(4*bits+5) and den.bit_length()<=(3*bits+4),'closed candidate coefficient budgets')
    check(dot(n,n).bit_length()<=(8*bits+12) and (den*den).bit_length()<=(6*bits+8),'Level budgets')
    for query in list(pp)+[(0,0,0),(m,m,m),(m//2,m//2,m//2)]:
     v=sub(query,a); first=den*dot(v,v); terms=[first]+[-2*n[j]*v[j] for j in range(3)]; total=0
     for term in terms:
      total+=term; check(abs(term)<1<<127 and abs(total)<1<<127,'q4 native intermediate in this bounded fixture'); max_native_bits=max(max_native_bits,abs(term).bit_length(),abs(total).bit_length())
     check(total==den*(dot(sub(query,c),sub(query,c))-r),'candidate power sign before materialization')
    if bits==18 and pp==tuple(points):
     examples[label]={'center':[str(v) for v in c],'weights':[str(w) for w in weights],'inside':strict,'level':str(r),'N':list(n),'D':den,'det':det}
  profile_rows.append({'bits':bits,'supports_permuted':count,'degenerate':degenerate,'positive_det':pos,'negative_det':neg,'checks':nchecks,'max_native_intermediate_bits_on_these_fixtures':max_native_bits})
 require(examples['outside']['inside'] is False and examples['zero_weight']['inside'] is False,'noncritical candidate controls')
 require(examples['regular_extreme']['inside'] is True,'positive candidate control')
 return {'status':'PASS','scope':'Static body/token equality against9df and tiny rational models; not C++ execution, universal proof or native/G4 qualification','source_checks':checks,'profiles':profile_rows,'examples':examples,'checks':len(checks)+sum(x['checks'] for x in profile_rows)}
if __name__=='__main__': print(json.dumps(derive(),sort_keys=True,indent=2))
