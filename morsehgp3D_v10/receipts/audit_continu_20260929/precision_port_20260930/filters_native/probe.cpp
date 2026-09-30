// Three causal closed_ball queries on unchanged SiteTree bodies.
// Deliberate direct Cloud construction bypasses the supported u18 factory.
// Code 0 means THREE EXPECTED FAILURES confirmed, NOT numerical qualification.
#include <algorithm>
#include <array>
#include <cfenv>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "cloud/site_tree.hpp"
using namespace mhgp10;

namespace {
unsigned checks=0, failures=0, wrong_cases=0, key_controls=0;
void check(bool value,const char* label) {
  ++checks;
  if (!value) { ++failures; std::fprintf(stderr,"FAIL %s\n",label); }
}
using Points=std::array<geom::P3,3>;
struct Fixture {
  const char* name;
  int bits;
  Points points;
  std::array<i128,3> numerator;
  i128 denominator;
  bool false_interior;
};
u128 morton96(const geom::P3& p) {
  const u32 axes[3]={u32(p.x),u32(p.y),u32(p.z)};
  u128 key=0;
  for (unsigned b=0;b<32;++b)
    for (unsigned axis=0;axis<3;++axis)
      key |= u128{(axes[axis]>>b)&1u} << (3*b+axis);
  return key;
}
std::string dec(i128 n) { return arith::to_string(arith::I128w::from_i128(n)); }
void array(const std::vector<u32>& x) {
  std::cout << '[';
  for (std::size_t j=0;j<x.size();++j) { if(j)std::cout<<',';std::cout<<x[j]; }
  std::cout << ']';
}
void point(const geom::P3& p) { std::cout<<'['<<p.x<<','<<p.y<<','<<p.z<<']'; }
void run(const Fixture& f) {
  const u64 used=default_budget().used();
  {
    Cloud cloud;
    cloud.bits=f.bits;
    cloud.weight=3;
    const bool allocated=cloud.x.allocate(3)&&cloud.y.allocate(3)&&cloud.z.allocate(3)&&
      cloud.w.allocate(3)&&cloud.ids.off.allocate(4)&&cloud.ids.val.allocate(3);
    check(allocated,"three-site buffers");
    if (!allocated) return;
    std::array<u32,3> ids={0,1,2};
    std::sort(ids.begin(),ids.end(),[&](u32 a,u32 b) { return morton96(f.points[a])<morton96(f.points[b]); });
    u32 target=kNone;
    for (u32 i=0;i<3;++i) {
      const auto& p=f.points[ids[i]];
      check(p.x>=0&&p.y>=0&&p.z>=0&&u64(p.x)<(u64{1}<<f.bits)&&
        u64(p.y)<(u64{1}<<f.bits)&&u64(p.z)<(u64{1}<<f.bits),"declared u24/u32 coordinate domain");
      cloud.x[i]=u32(p.x);cloud.y[i]=u32(p.y);cloud.z[i]=u32(p.z);cloud.w[i]=1;
      cloud.ids.off[i]=i;cloud.ids.val[i]=PointId{ids[i]};
      if(ids[i]==2)target=i;
    }
    cloud.ids.off[3]=3;
    check(cloud.ids.well_formed(),"site-original-fixture CSR");
    check(target!=kNone,"target is original fixture point 2");
    const geom::P3 anchor=f.points[0];
    geom::Center center{};
    for(unsigned axis=0;axis<3;++axis) center.N[axis]=f.numerator[axis];
    center.D=f.denominator;
    check(center.D>0,"positive reduced denominator");
    check(geom::acute(f.points[0],f.points[1],f.points[2]),"strict acute support");
    std::array<i128,3> keys{};
    for(u32 i=0;i<3;++i) {
      const geom::P3 p{i64(cloud.x[i]),i64(cloud.y[i]),i64(cloud.z[i])};
      keys[i]=geom::side_key(center,anchor,p);
      check(keys[i]==0,"unchanged exact side classifies every support on shell");
      ++key_controls;
    }
    SiteTree tree(cloud);
    std::vector<u32> interior,shell;
    tree.closed_ball(anchor,center,interior,shell);
    check(std::is_sorted(interior.begin(),interior.end()),"interior sorted");
    check(std::is_sorted(shell.begin(),shell.end()),"shell sorted");
    for(u32 s:interior)check(s<3,"interior site bound");
    for(u32 s:shell)check(s<3,"shell site bound");
    const bool in_interior=std::find(interior.begin(),interior.end(),target)!=interior.end();
    const bool in_shell=std::find(shell.begin(),shell.end(),target)!=shell.end();
    const bool expected_wrong=f.false_interior ? (in_interior&&!in_shell) : (!in_interior&&!in_shell);
    check(expected_wrong,"expected target filter failure reproduced");
    const std::vector<u32> full_shell={0,1,2};
    check(interior.size()!=0||shell!=full_shell,"closed_ball differs from exact empty-interior/full-shell");
    wrong_cases+=unsigned(expected_wrong);
    std::cout << "{\"name\":\""<<f.name<<"\",\"bits_tag\":"<<f.bits
      <<",\"cloud_construction\":\"direct_private_outside_u18_factory\",\"points_fixture_order\":[";
    for(unsigned j=0;j<3;++j){if(j)std::cout<<',';point(f.points[j]);}
    std::cout<<"],\"site_original_fixture_ids\":["<<ids[0]<<','<<ids[1]<<','<<ids[2]
      <<"],\"target_site\":"<<target<<",\"N\":[\""<<dec(center.N[0])<<"\",\""
      <<dec(center.N[1])<<"\",\""<<dec(center.N[2])<<"\"],\"D\":\""<<dec(center.D)
      <<"\",\"exact_side_keys\":[\""<<dec(keys[0])<<"\",\""<<dec(keys[1])<<"\",\""<<dec(keys[2])
      <<"\"],\"interior\":";
    array(interior);
    std::cout<<",\"shell\":";array(shell);
    std::cout<<",\"expected_target_failure\":\""<<(f.false_interior?"false_interior":"lost_shell")
      <<"\",\"failure_reproduced\":"<<(expected_wrong?"true":"false")<<'}';
  }
  check(default_budget().used()==used,"buffers released");
}
}
int main() {
  check(std::fesetround(FE_TONEAREST)==0,"set RN");
  check(std::fegetround()==FE_TONEAREST,"observe RN");
  const std::array<Fixture,3> cases={{
    {"u24_reduced_false_interior",24,{{{0,0,0},{15000005,15000005,0},{15000005,0,15000005}}},
      {{30000010,15000005,15000005}},3,true},
    {"u24_reduced_lost",24,{{{15000010,0,15000010},{15000010,15000010,0},{0,0,0}}},
      {{-15000010,15000010,-30000020}},3,false},
    {"u32_translated_reduced_lost",32,{{{4000000000ll,4000000000ll,4000000000ll},
      {4000200005ll,4000000000ll,4000000000ll},{4000040001ll,4000200005ll,4000000000ll}}},
      {{1000025,840021,0}},10,false}
  }};
  std::cout<<"{\"schema\":\"mhgp10_site_filter_native_failure_probe_v1\",\"scope\":\"unchanged_u18_bodies_outside_contract\",\"cases\":[";
  for(unsigned i=0;i<cases.size();++i){if(i)std::cout<<',';run(cases[i]);}
  check(wrong_cases==3,"three expected failures");
  check(key_controls==9,"nine exact shell controls");
  check(checks>=40,"non-vacuity");
  std::cout<<"],\"checks\":"<<checks<<",\"key_controls\":"<<key_controls
    <<",\"expected_failures_confirmed\":"<<wrong_cases<<",\"gate_failures\":"<<failures
    <<",\"status\":\""<<(failures?"FAIL":"EXPECTED_FAILURES_CONFIRMED")<<"\"}\n";
  if(failures)return 1;
  return std::cout.good()?0:2;
}
