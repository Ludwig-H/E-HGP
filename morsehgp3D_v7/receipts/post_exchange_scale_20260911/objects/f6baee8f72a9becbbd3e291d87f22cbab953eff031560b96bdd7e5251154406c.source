// Overlay v7: bounded arithmetic of real u16 supports, not pipeline exactness.
// Oracle: OBig384, six-permutation determinants, q3 linear equations and
// barycentric signs. Binary gcd/division explicitly ported from the bounded
// independent judge tests/linked_arcs_gate.cpp:80-139, not from src/.
// Fixtures G1-G6: docs/PLAN_PORTES_ARITHMETIQUES.md. Cassini: independent
// audits/ARITHMETIQUE_LANES_COURANTE.md (2026-09-04), sources pinned in overlay.
// Boost absent locally: authority is OBig AND literal identities, never Boost.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "oracle/obig.hpp"
#include "src/lanes/q2.hpp"
#include "src/lanes/q4.hpp"

namespace {
using B = mhgp7_oracle::OBig384;
using Vec = std::array<B, 3>;
using Mat = std::array<Vec, 3>;
using Key = std::array<B, 5>;
using mhgp7::P3;
using mhgp7::i64;
using mhgp7::i128;
constexpr i64 kM = 65535;

struct Failure : std::runtime_error { using std::runtime_error::runtime_error; };
struct Stats {
  int oracle_carry = 0, q2 = 0, q3 = 0, q4 = 0, q3_rank_zero = 0, q4_rank_zero = 0;
  int det_positive = 0, det_negative = 0, inside = 0, outside = 0, boundary = 0;
  int power_negative = 0, power_zero = 0, power_positive = 0, q3_acute = 0, q3_rejected = 0;
  int diam_negative = 0, diam_zero = 0, diam_positive = 0, fixture_domain_rejected = 0;
  int q4_num_high = 0, q4_cross_w4_zero = 0, q4_cross_w3_nonzero = 0, interlane_equal = 0;
  int cassini_large_center = 0, cassini_axis_calls = 0, face_power_zero = 0;
  int max_g = 0, max_w = 0, max_b3 = 0, max_c3 = 0, max_det4 = 0, max_np4 = 0;
  int max_num4 = 0, max_cross4 = 0, c3_negative = 0, c3_positive = 0, c4_nonzero = 0;
} stats;

B ob(i128 x) { return B::from_i128(x); }
void require(bool yes, const std::string& label) {
  if (mhgp7_oracle::overflow_seen()) throw Failure("oracle.overflow");
  if (!yes) throw Failure(label);
}
void equal(const B& x, const B& y, const std::string& label) { require(x == y, label); }
Vec point(const P3& p) { return {ob(p.x), ob(p.y), ob(p.z)}; }
Vec subtract(const Vec& a, const Vec& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
B dot(const Vec& a, const Vec& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
B norm(const Vec& v) { return dot(v, v); }
Vec cross(const Vec& a, const Vec& b) {
  return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
B determinant(const Mat& m) {
  // Leibniz, not the product's cofactor/adjugate layout.
  constexpr int perm[6][3] = {{0,1,2},{1,2,0},{2,0,1},{0,2,1},{2,1,0},{1,0,2}};
  B out;
  for (int s=0; s<6; ++s) {
    const B t = m[0][static_cast<std::size_t>(perm[s][0])]
              * m[1][static_cast<std::size_t>(perm[s][1])]
              * m[2][static_cast<std::size_t>(perm[s][2])];
    out = s<3 ? out+t : out-t;
  }
  return out;
}
struct Center { B det; Vec np; Vec a; int raw_sign = 0; };
Center solve(const Mat& m, const Vec& rhs, const Vec& a) {
  Center c{determinant(m), {}, a, 0};
  c.raw_sign = c.det.sign();
  for (std::size_t k=0; k<3; ++k) {
    Mat replaced=m;
    for (std::size_t row=0; row<3; ++row) replaced[row][k]=rhs[row];
    c.np[k]=determinant(replaced);
  }
  if (c.raw_sign<0) { c.det=-c.det; for (B& x:c.np) x=-x; }
  return c;
}
Center center3(const std::array<P3,3>& p) {
  const Vec a=point(p[0]), d=subtract(point(p[1]),a), u=subtract(point(p[2]),a);
  const Vec n=cross(d,u);
  Mat m{{d,u,n}};
  for (std::size_t j=0; j<3; ++j) { m[0][j]=m[0][j]*ob(2); m[1][j]=m[1][j]*ob(2); }
  return solve(m, {norm(d),norm(u),ob(0)}, a);
}
Center center4(const std::array<P3,4>& p) {
  const Vec a=point(p[0]); Mat m; Vec rhs;
  for (std::size_t j=0; j<3; ++j) {
    const Vec v=subtract(point(p[j+1]),a); rhs[j]=norm(v);
    for (std::size_t k=0; k<3; ++k) m[j][k]=ob(2)*v[k];
  }
  return solve(m,rhs,a);
}
B power(const Center& c, const P3& p) {
  const Vec v=subtract(point(p),c.a);
  return c.det*norm(v)-ob(2)*dot(c.np,v);
}
Key raw_key(const Center& c) {
  return {c.det, -ob(2)*(c.det*c.a[0]+c.np[0]), -ob(2)*(c.det*c.a[1]+c.np[1]),
          -ob(2)*(c.det*c.a[2]+c.np[2]), c.det*norm(c.a)+ob(2)*dot(c.np,c.a)};
}

// Explicit small port of the independent binary gcd/long division judge.
bool even(const B& a) { return (a.w[0]&1U)==0; }
void shr(B& a) {
  for (int i=0; i+1<B::kLimbs; ++i) a.w[i]=(a.w[i]>>1)|(a.w[i+1]<<31);
  a.w[B::kLimbs-1]>>=1;
}
B gcd(B a, B b) {
  a.neg=false; b.neg=false;
  if (a.is_zero()) return b;
  if (b.is_zero()) return a;
  int shift=0;
  while (even(a)&&even(b)) { shr(a); shr(b); ++shift; }
  while (even(a)) shr(a);
  for (;;) {
    while (even(b)) shr(b);
    if (B::cmp_mag(a,b)>0) std::swap(a,b);
    b=B::sub_mag(b,a);
    if (b.is_zero()) break;
  }
  while (shift-- > 0) a=B::add_mag(a,a);
  return a;
}
B divide_exact(const B& a, const B& g) {
  require(g.sign()>0,"oracle.divisor");
  B q,r;
  for (int bit=a.bit_length()-1; bit>=0; --bit) {
    r=B::add_mag(r,r);
    if (((a.w[bit/32]>>(bit%32))&1U)!=0) r=B::add_mag(r,ob(1));
    if (B::cmp_mag(r,g)>=0) { r=B::sub_mag(r,g); q.w[bit/32]|=std::uint32_t{1}<<(bit%32); }
  }
  require(r.is_zero(),"oracle.nonexact-division"); q.neg=a.neg; q.canon(); return q;
}
Key primitive(Key k) {
  B g=k[0]; for (std::size_t i=1; i<k.size(); ++i) g=gcd(g,k[i]);
  for (B& t:k) t=divide_exact(t,g);
  return k;
}
void check_key(const Center& c, const mhgp7::BallKey& k, const std::string& label) {
  const Key expected=primitive(raw_key(c));
  const Key actual{ob(k.a),ob(k.b[0]),ob(k.b[1]),ob(k.b[2]),ob(k.c)};
  for (std::size_t i=0; i<5; ++i) equal(actual[i],expected[i],label+".key."+std::to_string(i));
}
void check_form(const Center& c, const mhgp7::BallForm& f, const std::string& label) {
  const Key ref=raw_key(c), got{ob(f.a),ob(f.b[0]),ob(f.b[1]),ob(f.b[2]),ob(f.c)};
  for (std::size_t i=0; i<5; ++i) equal(got[i]*c.det,ref[i]*ob(f.a),label+".form."+std::to_string(i));
}
void count_power(const B& p) {
  if (p.sign()<0) ++stats.power_negative;
  else if (p.sign()==0) ++stats.power_zero;
  else ++stats.power_positive;
}
std::vector<P3> probes() {
  std::vector<P3> p;
  for (int mask=0; mask<8; ++mask) p.push_back(P3{(mask&1)?kM:0,(mask&2)?kM:0,(mask&4)?kM:0});
  p.push_back(P3{1,1,1}); p.push_back(P3{kM/2,kM/2,kM/2});
  return p;
}
bool valid_u16(const P3& p) { return p.x>=0&&p.x<=kM&&p.y>=0&&p.y<=kM&&p.z>=0&&p.z<=kM; }
void check_domain() {
  for (const P3& p:std::array<P3,2>{{{-1,0,0},{0,65536,0}}}) {
    require(!valid_u16(p),"fixture.u16-domain"); ++stats.fixture_domain_rejected;
  }
}
void oracle_qualification() {
  // Literal from (R^3-1)(R^2-1)=R^5-R^3-R^2+1. No product primitive here.
  const std::uint64_t n[3]={UINT64_MAX,UINT64_MAX,UINT64_MAX};
  const std::uint64_t d[2]={UINT64_MAX,UINT64_MAX};
  const std::uint64_t expected[5]={1,0,UINT64_MAX,UINT64_MAX-1,UINT64_MAX};
  ++stats.oracle_carry;
  equal(B::from_u64_words(n,3)*B::from_u64_words(d,2),B::from_u64_words(expected,5),"oracle.literal-carry");
  equal(gcd(ob(12),ob(-18)),ob(6),"oracle.gcd-literal");
  equal(divide_exact(ob(-18),ob(6)),ob(-3),"oracle.division-literal");
}

void check_q2(const P3& a, const P3& b) {
  require(valid_u16(a)&&valid_u16(b),"fixture.q2-domain"); ++stats.q2;
  const Center c{ob(2),subtract(point(b),point(a)),point(a),1};
  const auto key=mhgp7::q2_ball_key(a,b);
  check_key(c,key,"q2");
  require(key==mhgp7::q2_ball_key(b,a),"q2.permutation");
  const B distance=norm(c.np); mhgp7_oracle::oi128 di=0;
  require(distance.to_i128(&di)&&di<=static_cast<i128>(INT64_MAX),"fixture.q2-distance");
  const auto l=mhgp7::q2_exact_level(static_cast<i64>(di));
  equal(ob(l.num)*ob(4),distance*ob(l.den),"q2.level");
  for (const P3& z:probes()) { const B p=power(c,z); count_power(p); equal(ob(key.power(z))*ob(2),p,"q2.power"); }
}

bool acute_reference(const std::array<P3,3>& p) {
  for (std::size_t i=0; i<3; ++i)
    if (dot(subtract(point(p[(i+1)%3]),point(p[i])),subtract(point(p[(i+2)%3]),point(p[i]))).sign()<=0) return false;
  return true;
}
void check_seed(const std::array<P3,3>& p) {
  std::size_t a=0,b=1; B longest=norm(subtract(point(p[1]),point(p[0])));
  for (std::size_t i=0; i<3; ++i) for (std::size_t j=i+1; j<3; ++j) {
    const B d=norm(subtract(point(p[j]),point(p[i])));
    if (d>longest) { longest=d; a=i; b=j; }
  }
  const std::size_t x=3-a-b;
  mhgp7_oracle::oi128 dl=0; require(longest.to_i128(&dl),"fixture.seed-distance");
  const bool expected=acute_reference(p);
  if (expected) ++stats.q3_acute; else ++stats.q3_rejected;
  require(mhgp7::is_acute_seed(p[a],p[b],p[x],static_cast<i64>(dl),
          static_cast<mhgp7::PointId>(a),static_cast<mhgp7::PointId>(b),static_cast<mhgp7::PointId>(x))==expected,"q3.seed-strict");
  Vec v;
  for (std::size_t j=0; j<3; ++j) v[j]=ob(2)*point(p[x])[j]-point(p[a])[j]-point(p[b])[j];
  const int s=(norm(v)-longest).sign();
  if (s<0) ++stats.diam_negative; else if (s==0) ++stats.diam_zero; else ++stats.diam_positive;
}
void check_q3(const std::array<P3,3>& p) {
  for (const auto& z:p) require(valid_u16(z),"fixture.q3-domain");
  ++stats.q3; check_seed(p);
  const Center c=center3(p); const auto f=mhgp7::q3_form(p[0],p[1],p[2]);
  if (c.det.is_zero()) { ++stats.q3_rank_zero; require(f.g==0,"q3.rank-zero"); return; }
  require(f.g>0,"q3.rank-positive"); stats.max_g=std::max(stats.max_g,ob(f.g).bit_length());
  for (std::size_t j=0; j<3; ++j) {
    equal(ob(f.w[j])*c.det,ob(2)*ob(f.g)*c.np[j],"q3.center."+std::to_string(j));
    stats.max_w=std::max(stats.max_w,ob(f.w[j]).bit_length());
  }
  const auto form=mhgp7::q3_ball_form(f); check_form(c,form,"q3");
  for (const i128 b:form.b) { require(ob(b).bit_length()<=86,"q3.bound-b86"); stats.max_b3=std::max(stats.max_b3,ob(b).bit_length()); }
  require(ob(form.c).bit_length()<=104,"q3.bound-c104"); stats.max_c3=std::max(stats.max_c3,ob(form.c).bit_length());
  if (form.c<0) ++stats.c3_negative;
  if (form.c>0) ++stats.c3_positive;
  const auto key=mhgp7::q3_ball_key(f); check_key(c,key,"q3");
  const auto level=mhgp7::q3_exact_level(p[0],p[1],p[2]);
  equal(ob(level.num)*c.det*c.det,norm(c.np)*ob(level.den),"q3.level");
  auto ps=probes(); ps.insert(ps.end(),p.begin(),p.end());
  for (const P3& z:ps) {
    const B value=power(c,z); count_power(value);
    equal(ob(mhgp7::q3_power(f,z))*c.det,value*ob(f.g),"q3.power");
    equal(ob(key.power(z))*c.det,value*ob(key.a),"q3.key-power");
  }
}

int location4(const Center& c, const std::array<P3,4>& p) {
  // Barycentric Cramer, not the product's alternating face orientations.
  Mat columns;
  for (std::size_t j=0; j<3; ++j) {
    const Vec d=subtract(point(p[j+1]),point(p[0]));
    for (std::size_t row=0; row<3; ++row) columns[row][j]=d[row];
  }
  B denom=determinant(columns)*c.det; std::array<B,4> beta;
  for (std::size_t j=0; j<3; ++j) {
    Mat m=columns; for (std::size_t row=0; row<3; ++row) m[row][j]=c.np[row];
    beta[j+1]=determinant(m);
  }
  beta[0]=denom-beta[1]-beta[2]-beta[3];
  if (denom.sign()<0) { denom=-denom; for (B& b:beta) b=-b; }
  require(denom.sign()>0,"oracle.barycentric-den"); bool zero=false;
  for (const B& b:beta) { if (b.sign()<0) return -1; if (b.is_zero()) zero=true; }
  return zero?0:1;
}
void check_q4(const std::array<P3,4>& p, bool regular) {
  for (const auto& z:p) require(valid_u16(z),"fixture.q4-domain");
  ++stats.q4; const Center c=center4(p); const auto f=mhgp7::q4_form(p[0],p[1],p[2],p[3]);
  if (c.det.is_zero()) { ++stats.q4_rank_zero; require(f.det==0,"q4.rank-zero"); return; }
  if (c.raw_sign>0) ++stats.det_positive; else ++stats.det_negative;
  equal(ob(f.det),c.det,"q4.det"); stats.max_det4=std::max(stats.max_det4,c.det.bit_length());
  for (std::size_t j=0; j<3; ++j) { equal(ob(f.np[j]),c.np[j],"q4.cramer."+std::to_string(j)); stats.max_np4=std::max(stats.max_np4,c.np[j].bit_length()); }
  const int location=location4(c,p);
  if (location>0) ++stats.inside; else if (location<0) ++stats.outside; else ++stats.boundary;
  require(mhgp7::q4_center_strictly_inside(f,p[0],p[1],p[2],p[3])==(location>0),"q4.center");
  const auto form=mhgp7::q4_ball_form(f); check_form(c,form,"q4");
  if (form.c!=0) ++stats.c4_nonzero;
  const auto key=mhgp7::ball_key_reduce(form); check_key(c,key,"q4");
  const B expected_num=norm(c.np), expected_den=c.det*c.det;
  stats.max_num4=std::max(stats.max_num4,expected_num.bit_length());
  if (expected_num.bit_length()>128) ++stats.q4_num_high;
  const auto level=mhgp7::q4_level_raw(f);
  equal(B::from_u64_words(level.num,3),expected_num,"q4.level.num");
  equal(ob(level.den),expected_den,"q4.level.den");
  const mhgp7::U192 n{{level.num[0],level.num[1],level.num[2]}};
  const auto crossed=mhgp7::mul_192x128_320(n,static_cast<mhgp7::u128>(level.den));
  equal(B::from_u64_words(crossed.w,5),expected_num*expected_den,"q4.cross");
  require(crossed.w[4]==0,"q4.u16-w4-must-be-zero"); ++stats.q4_cross_w4_zero;
  if (crossed.w[3]!=0) ++stats.q4_cross_w3_nonzero;
  stats.max_cross4=std::max(stats.max_cross4,(expected_num*expected_den).bit_length());
  if (regular) {
    const auto q2=mhgp7::promote_level(mhgp7::q2_exact_level(3*kM*kM));
    require(mhgp7::compare_exact_level(level,q2)==0&&mhgp7::same_exact_level(level,q2),"q4.interlane-level");
    require(level!=q2,"q4.interlane-representation");
    require(key==mhgp7::q2_ball_key(P3{0,0,0},P3{kM,kM,kM}),"q4.interlane-key"); ++stats.interlane_equal;
  }
  auto ps=probes(); ps.insert(ps.end(),p.begin(),p.end());
  for (const P3& z:ps) {
    const B value=power(c,z); count_power(value);
    equal(ob(mhgp7::q4_power(f,z)),value,"q4.power");
    equal(ob(key.power(z))*c.det,value*ob(key.a),"q4.key-power");
  }
  const std::array<P3,3> face{{p[0],p[1],p[2]}}; const Center fc=center3(face);
  require(fc.det.sign()>0,"fixture.face-rank"); const B face_power=power(fc,p[3]);
  if (face_power.is_zero()) ++stats.face_power_zero;
  require(mhgp7::q4_face_power_prefilter(mhgp7::q3_form(p[0],p[1],p[2]),p[3])==(face_power.sign()>0),"q4.face_power");
}

void cassini() {
  const std::array<P3,3> p{{{0,0,0},{46368,28657,0},{28657,17711,0}}}; check_q3(p);
  const auto f=mhgp7::q3_form(p[0],p[1],p[2]);
  require(f.g==1&&f.w[0]==static_cast<i128>(-20100270015213LL)
          &&f.w[1]==static_cast<i128>(32522920160401LL)&&f.w[2]==0,"q3.cassini-literal");
  for (int axis=0; axis<3; ++axis) {
    if (ob(f.w[axis]).abs()>B::pow2(41)) ++stats.cassini_large_center;
    for (const std::array<i64,2>& range:std::array<std::array<i64,2>,3>{{{0,4},{32766,32770},{65531,65535}}}) {
      B smallest,largest; bool first=true;
      for (i64 t=range[0]; t<=range[1]; ++t) {
        const B value=ob(f.g)*ob(t)*ob(t)-ob(f.w[axis])*ob(t);
        if (first||value<smallest) smallest=value;
        if (first||value>largest) largest=value;
        first=false;
      }
      equal(ob(mhgp7::q3_detail::axis_min(f,axis,range[0],range[1])),smallest,"q3.cassini-axis-min");
      equal(ob(mhgp7::q3_detail::axis_max(f,axis,range[0],range[1])),largest,"q3.cassini-axis-max"); ++stats.cassini_axis_calls;
    }
  }
}
const std::array<P3,4> regular4{{{0,0,0},{kM,kM,0},{kM,0,kM},{0,kM,kM}}};
void geometric_cases() {
  check_domain(); check_q2(P3{0,0,0},P3{kM,kM,kM}); check_q2(P3{0,0,0},P3{2,0,0});
  for (int perturb=0; perturb<2; ++perturb) {
    const std::array<P3,3> base{{{0,0,0},{kM,kM,0},{kM,0,kM-perturb}}};
    std::array<int,3> axes{{0,1,2}};
    do {
      std::array<P3,3> permuted;
      for (std::size_t i=0; i<3; ++i) {
        const i64 v[3]={base[i].x,base[i].y,base[i].z};
        permuted[i]=P3{v[axes[0]],v[axes[1]],v[axes[2]]};
      }
      std::array<int,3> vertices{{0,1,2}};
      do { check_q3({permuted[static_cast<std::size_t>(vertices[0])],permuted[static_cast<std::size_t>(vertices[1])],permuted[static_cast<std::size_t>(vertices[2])]}); }
      while (std::next_permutation(vertices.begin(),vertices.end()));
    } while (std::next_permutation(axes.begin(),axes.end()));
  }
  check_q3({P3{0,0,0},P3{2,0,0},P3{1,0,0}});
  check_q3({P3{0,0,0},P3{2,0,0},P3{0,2,0}});
  check_q3({P3{0,0,0},P3{4,0,0},P3{1,1,0}});
  for (i64 h=1; h<=3; ++h) check_q3({P3{0,0,0},P3{4,0,0},P3{2,h,0}});
  cassini();
  // Otherwise every G2/G3 sphere used above passes through the origin:
  // C would stay zero despite all support/axis permutations. Exercise both signs.
  check_q3({P3{kM,0,0},P3{0,kM,0},P3{0,0,kM}});
  check_q3({P3{kM/2,kM/2,kM/2},P3{kM,kM/2,kM/2},P3{kM/2,kM,kM/2}});
  std::array<int,4> order{{0,1,2,3}};
  const std::array<P3,4> boundary{{{0,0,0},{4,0,0},{2,3,0},{2,0,2}}};
  do {
    std::array<P3,4> p,q;
    for (std::size_t i=0; i<4; ++i) { p[i]=regular4[static_cast<std::size_t>(order[i])]; q[i]=boundary[static_cast<std::size_t>(order[i])]; }
    check_q4(p,true); check_q4(q,false);
  } while (std::next_permutation(order.begin(),order.end()));
  check_q4({P3{0,0,0},P3{kM,1,0},P3{kM-1,1,0},P3{0,0,1}},false);
  check_q4({P3{0,0,0},P3{kM,0,0},P3{0,kM,0},P3{kM,kM,0}},false);
  constexpr i64 t=kM/2+1;
  check_q4({P3{t,t,t},P3{kM,kM,t},P3{kM,t,kM},P3{t,kM,kM}},false);
  // The G2/G4/G5/G6 literals certify this oracle beyond merely comparing methods.
  const auto g2=mhgp7::q3_form(P3{0,0,0},P3{kM,kM,0},P3{kM,0,kM});
  const B m=ob(kM), m2=m*m, m4=m2*m2, m5=m4*m;
  equal(ob(g2.g),ob(3)*m4,"q3.G2-g-literal");
  equal(ob(g2.w[0]),ob(4)*m5,"q3.G2-w0-literal");
  equal(ob(g2.w[1]),ob(2)*m5,"q3.G2-w1-literal");
  equal(ob(g2.w[2]),ob(2)*m5,"q3.G2-w2-literal");
  const Center g4=center4(regular4); equal(g4.det,ob(16)*m2*m,"oracle.G4-det-literal");
  for (const B& n:g4.np) equal(n,ob(8)*m4,"oracle.G4-np-literal");
  const Center g5=center4({P3{0,0,0},P3{kM,1,0},P3{kM-1,1,0},P3{0,0,1}});
  equal(g5.det,ob(8),"oracle.G5-det-literal"); equal(g5.np[0],ob(8)*m-ob(4),"oracle.G5-np0-literal");
  equal(g5.np[1],ob(4)*(-m2+m+ob(1)),"oracle.G5-np1-literal"); equal(g5.np[2],ob(4),"oracle.G5-np2-literal");
  const Center g6=center4(boundary); equal(g6.det,ob(192),"oracle.G6-det-literal");
  equal(g6.np[0],ob(384),"oracle.G6-np0-literal"); equal(g6.np[1],ob(160),"oracle.G6-np1-literal"); equal(g6.np[2],ob(0),"oracle.G6-np2-literal");
}
void report() {
  std::printf("authority=obig_and_literals boost=not_used public_status=not_claimed "
    "oracle_carry=%d q2=%d q3=%d q4=%d q3_rank_zero=%d q4_rank_zero=%d det_positive=%d det_negative=%d "
    "inside=%d outside=%d boundary=%d power_negative=%d power_zero=%d power_positive=%d q3_acute=%d q3_rejected=%d "
    "diam_negative=%d diam_zero=%d diam_positive=%d fixture_domain_rejected=%d q4_num_high=%d "
    "q4_cross_w4_zero=%d q4_cross_w3_nonzero=%d interlane_equal=%d cassini_large_center=%d cassini_axis_calls=%d face_power_zero=%d "
    "max_g_bits=%d max_w_bits=%d max_b3_bits=%d max_c3_bits=%d max_det4_bits=%d max_np4_bits=%d max_num4_bits=%d max_cross4_bits=%d "
    "c3_negative=%d c3_positive=%d c4_nonzero=%d\n",
    stats.oracle_carry,stats.q2,stats.q3,stats.q4,stats.q3_rank_zero,stats.q4_rank_zero,stats.det_positive,stats.det_negative,
    stats.inside,stats.outside,stats.boundary,stats.power_negative,stats.power_zero,stats.power_positive,stats.q3_acute,stats.q3_rejected,
    stats.diam_negative,stats.diam_zero,stats.diam_positive,stats.fixture_domain_rejected,stats.q4_num_high,
    stats.q4_cross_w4_zero,stats.q4_cross_w3_nonzero,stats.interlane_equal,stats.cassini_large_center,stats.cassini_axis_calls,stats.face_power_zero,
    stats.max_g,stats.max_w,stats.max_b3,stats.max_c3,stats.max_det4,stats.max_np4,stats.max_num4,stats.max_cross4,
    stats.c3_negative,stats.c3_positive,stats.c4_nonzero);
}
void floor_check(std::string_view selected) {
  require(stats.oracle_carry==1,"nonvacuity.oracle");
  if (selected=="oracle-only") return;
  if (selected=="q4-level-only") { require(stats.q4_num_high==1&&stats.q4_cross_w4_zero==1&&stats.interlane_equal==1,"nonvacuity.q4-isolated"); return; }
  require(stats.q2==2&&stats.q3>=81&&stats.q4==51,"nonvacuity.forms");
  require(stats.q3_rank_zero==1&&stats.q4_rank_zero==1,"nonvacuity.ranks");
  require(stats.det_positive>=24&&stats.det_negative>=24,"nonvacuity.orientation");
  require(stats.inside==25&&stats.outside==1&&stats.boundary==24,"nonvacuity.center");
  require(stats.power_negative>0&&stats.power_zero>=16&&stats.power_positive>0,"nonvacuity.power");
  require(stats.q3_acute>0&&stats.q3_rejected>=6&&stats.diam_negative>0&&stats.diam_zero>0&&stats.diam_positive>0,"nonvacuity.seed-boundary");
  require(stats.fixture_domain_rejected==2&&stats.cassini_large_center==2&&stats.cassini_axis_calls==9,"nonvacuity.cassini-domain");
  require(stats.q4_num_high==24&&stats.q4_cross_w4_zero==50&&stats.q4_cross_w3_nonzero>=24&&stats.interlane_equal==24,"nonvacuity.geometric-wide");
  require(stats.face_power_zero>=6&&stats.max_g>64&&stats.max_w>80&&stats.max_num4>128&&stats.max_cross4>192,"nonvacuity.widths");
  require(stats.c3_negative>0&&stats.c3_positive>0&&stats.c4_nonzero>0&&stats.max_c3>90,"nonvacuity.constant-coefficients");
}
} // namespace

int main(int argc, char** argv) {
  std::string selected="all", inject;
  bool seen_case=false, seen_inject=false;
  for (int i=1; i<argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--case=")&&!seen_case) { selected=arg.substr(7); seen_case=true; }
    else if (arg.starts_with("--inject=")&&!seen_inject) { inject=arg.substr(9); seen_inject=true; }
    else return 2;
  }
  if (selected!="all"&&selected!="q4-level-only"&&selected!="oracle-only"&&selected!="oracle-overflow") return 2;
  const bool known=inject.empty()||inject=="q3-level-4g"||inject=="q4-center-parity"||inject=="q4-eq-nonstrict"||inject=="level-trunc-hi"||inject=="obig-carry-lost";
  if ((seen_inject&&inject.empty())||!known) return 2;
  if ((inject=="level-trunc-hi"&&selected!="q4-level-only")
      ||(inject=="obig-carry-lost"&&selected!="oracle-only")
      ||((inject=="q3-level-4g"||inject=="q4-center-parity"||inject=="q4-eq-nonstrict")&&selected!="all")) return 2;
  if (!mhgp7::mutants_enable(inject)) return 2;
  mhgp7_oracle::clear_overflow();
  try {
    oracle_qualification();
    if (selected=="oracle-overflow") {
      const B top=B::pow2(383);
      static_cast<void>(top+top); // Poisoned result is deliberately never consumed.
      require(mhgp7_oracle::overflow_seen(),"oracle.overflow-not-flagged");
    }
    if (selected=="all") geometric_cases();
    else if (selected=="q4-level-only") check_q4(regular4,true);
    floor_check(selected); report();
    if (!inject.empty()) { std::fprintf(stderr,"mutant_not_killed=%s\n",inject.c_str()); return 1; }
    std::puts("arithmetic_lanes_gate PASS"); return 0;
  } catch (const Failure& f) {
    report(); const std::string_view label(f.what());
    const bool causal=(inject=="q3-level-4g"&&label=="q3.level")
      ||(inject=="q4-center-parity"&&label=="q4.center")
      ||(inject=="q4-eq-nonstrict"&&label=="q4.face_power")
      ||(inject=="level-trunc-hi"&&label=="q4.level.num"&&stats.q4_num_high>0)
      ||(inject=="obig-carry-lost"&&label=="oracle.literal-carry"&&stats.oracle_carry==1);
    std::printf("first_divergence=%s inject=%s\n",f.what(),inject.c_str());
    if (mhgp7_oracle::overflow_seen()||label.starts_with("nonvacuity.")) return 3;
    return causal?4:1;
  } catch (const std::exception& e) {
    report(); std::fprintf(stderr,"unexpected_exception=%s\n",e.what()); return 1;
  }
}
