#include "lanes/q34_witness_search.hpp"

// Explicit reuse of the constructor's test-only rational Gram solver. The
// citron oracle below is new and uses midpoint/Gram identities, not product
// bounds or the independent auditors' implementation.
#include "exact_ball_oracle.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
using mhgp8::Point3;
using mhgp8::Box3;
using mhgp8::u64;
using Points=std::vector<Point3>;
using Big=boost::multiprecision::cpp_int;
namespace rational=mhgp8_test::ball_oracle;

struct Gate {
  u64 checks{},queries{},singleton_queries{},rectangle_queries{},oracle_sites{},oracle_pairs{};
  u64 q3_rejections{},q4_rejections{},q3_only_rejections{},q4_only_rejections{},both_rejections{};
  u64 no_rejections{},inactive_queries{},zero_mask_queries{},partial_q3{},partial_q4{};
  u64 h_contacts{},q3_contacts{},q4_contacts{},wrong_alpha_cases{},positive_q4_external{};
  u64 whole_admissions{},point_admission_cases{},node_rejections{},saturations{},near_right_splits{};
  u64 exhausted_queries{},permutations{},extreme_queries{},invalid_inputs{},parallel_calls{};
  u64 repeated_calls{},source_alias_checks{},left_tie_cases{},partial_admission_cases{};
  u64 deep_index_cases{},peak_stack{},wide_predicate_cases{},overflow_exceptions{};
  u64 bounds_calls{},legacy_overload_calls{},exclusion_calls{},affine_calls{},mode_singletons{},mode_rectangles{};
  u64 local_exclusion_cases{},nonpositive_minimum_cases{},mixed_terminal_cases{},mode_parallel_calls{};
  u64 mode_invalid_inputs{},mode_repeated_calls{},mode_overflows{},affine_preparations{},general_preparations{};
  u64 excluded_q3_nodes{},excluded_q4_nodes{},fully_excluded_nodes{},mode_xi_nonpositive{};
  u64 extreme18_queries{},wide18_predicate_cases{};
  void require(bool condition,const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
};

struct Citron {
  Big h4;
  Big xi16;
  bool q3{},q4{};
};

// v=2z-a-b, d=b-a. H4=|d|^2-|v|^2 and
// 16Xi=4(|d|^2|v|^2-(v.d)^2). These are algebraically independent of
// the product's interval cross-products, with unbounded integer arithmetic.
Citron citron(Point3 a,Point3 b,Point3 z) {
  Big distance=0,radius=0,projection=0;
  for (std::size_t axis=0;axis<3;++axis) {
    const Big d=Big(b[axis])-a[axis];
    const Big v=2*Big(z[axis])-a[axis]-b[axis];
    distance+=d*d;radius+=v*v;projection+=v*d;
  }
  Citron result{Big(distance-radius),Big(4*(distance*radius-projection*projection)),false,false};
  if (result.xi16<0) throw std::runtime_error("negative exact Gram determinant");
  const Big square=result.h4*result.h4;
  result.q3=result.h4>0 && 3*square>result.xi16;
  result.q4=result.h4>0 && 2*square>result.xi16;
  return result;
}

std::array<u64,2> counts(Gate& gate,const Points& sites,Point3 a,Point3 b) {
  std::array<u64,2> result{};
  ++gate.oracle_pairs;
  for (const auto z:sites) {
    const auto value=citron(a,b,z);
    ++gate.oracle_sites;
    if (value.h4*value.h4>(Big(1)<<64)) ++gate.wide_predicate_cases;
    // |H4|<=3*65535^2<2^33.6 on 16-bit inputs, so H4^2>2^68 is reachable only by
    // an 18-bit fixture: the historical extreme cloud cannot satisfy this floor.
    if (value.h4*value.h4>(Big(1)<<68)) ++gate.wide18_predicate_cases;
    result[0]+=static_cast<u64>(value.q3);
    result[1]+=static_cast<u64>(value.q4);
  }
  return result;
}

std::uint8_t active(unsigned k,unsigned mask) {
  return static_cast<std::uint8_t>(mask & (k<2?0U:k<3?2U:6U));
}
std::uint8_t expected(unsigned k,unsigned mask,const std::array<u64,2>& total) {
  auto result=active(k,mask);
  if ((result&2U)!=0 && total[0]>=k-1) result=static_cast<std::uint8_t>(result&~2U);
  if ((result&4U)!=0 && total[1]>=k-2) result=static_cast<std::uint8_t>(result&~4U);
  return result;
}

std::vector<Point3> corners(const Box3& box) {
  std::vector<Point3> result;
  for (unsigned bits=0;bits<8;++bits) {
    Point3 point{(bits&1U)!=0?box.high.x:box.low.x,
                 (bits&2U)!=0?box.high.y:box.low.y,
                 (bits&4U)!=0?box.high.z:box.low.z};
    if (std::find(result.begin(),result.end(),point)==result.end()) result.push_back(point);
  }
  return result;
}

// The exact cone test is separately convex in each endpoint, so the box
// corners suffice for this stronger independent set of common witnesses.
std::array<u64,2> common_counts(Gate& gate,const Points& sites,const Box3& a,const Box3& b) {
  const auto ca=corners(a),cb=corners(b);
  std::array<u64,2> result{};
  for (const auto z:sites) {
    bool q3=true,q4=true;
    for (const auto x:ca) for (const auto y:cb) {
      const auto value=citron(x,y,z);
      ++gate.oracle_sites;
      q3=q3&&value.q3;q4=q4&&value.q4;
    }
    result[0]+=static_cast<u64>(q3);result[1]+=static_cast<u64>(q4);
  }
  return result;
}

Points clustered() {
  Points result{{100,100,100},{200,100,100},{0,0,0},{500,500,500}};
  for (unsigned i=0;i<18;++i)
    result.push_back({static_cast<mhgp8::Coordinate>(143+i),
                      static_cast<mhgp8::Coordinate>(98+i%5),
                      static_cast<mhgp8::Coordinate>(99+i%3)});
  return result;
}

Points random_points(unsigned seed,std::size_t n) {
  std::uint32_t state=seed;
  const auto next=[&]() {
    state=1664525U*state+1013904223U;
    return static_cast<mhgp8::Coordinate>((state>>16)%41U);
  };
  Points result;
  while (result.size()<n) {
    const Point3 p{next(),next(),next()};
    if (std::find(result.begin(),result.end(),p)==result.end()) result.push_back(p);
  }
  return result;
}

using Work=mhgp8::Q34WitnessSearchWork;
static_assert(std::is_trivially_copyable_v<Work> && std::is_standard_layout_v<Work> &&
              sizeof(Work)==25*sizeof(u64));

void ledger(Gate& gate,const Work& work,unsigned k,unsigned mask,std::uint8_t result) {
  const auto requested=active(k,mask);
  gate.require(work.queries==1,"query count");
  gate.require(work.q3_queries==static_cast<u64>((requested&2U)!=0) &&
               work.q4_queries==static_cast<u64>((requested&4U)!=0),"active lane query counts");
  gate.require((result&~requested)==0,"output introduced a lane");
  gate.require(work.q3_rejected==static_cast<u64>((requested&2U)!=0 && (result&2U)==0) &&
               work.q4_rejected==static_cast<u64>((requested&4U)!=0 && (result&4U)==0),"rejection lane counts");
  if (requested==0) {
    Work empty;empty.queries=1;
    gate.require(work==empty,"inactive query did geometric work");
    ++gate.inactive_queries;
    return;
  }
  gate.require(work.prepared_bounds==1 && work.node_visits==work.h_bound_tests &&
      work.node_visits==work.h_excluded_nodes+work.fully_admitted_nodes+work.leaf_remainders+work.split_nodes,
      "node classification partition");
  gate.require(work.midpoint_box_tests==2*work.split_nodes,"midpoint child test accounting");
  gate.require(work.point_tests<=work.node_visits && work.xi_bound_tests<=work.node_visits &&
      work.admitted_nodes<=work.node_visits && work.fully_admitted_nodes<=work.admitted_nodes &&
      work.leaf_remainders<=work.point_tests,"node counter inclusion");
  gate.require(work.q3_admitted_nodes<=work.q3_lane_tests && work.q4_admitted_nodes<=work.q4_lane_tests &&
      work.q3_lane_tests<=work.xi_bound_tests && work.q4_lane_tests<=work.xi_bound_tests,
      "per-lane admission/test inclusion");
  gate.require(work.q3_credits<=k-1 && work.q4_credits<=(k>=3?k-2:0),"credit exceeds saturation threshold");
  for (unsigned lane=0;lane<2;++lane) {
    const unsigned bit=2U<<lane;
    const auto credit=lane==0?work.q3_credits:work.q4_credits;
    const auto threshold=k>=lane+2?k-lane-1:0;
    if ((requested&bit)==0)
      gate.require(credit==0 && (lane==0?work.q3_lane_tests:work.q4_lane_tests)==0,
                   "inactive lane obtained a credit or test");
    else gate.require(((result&bit)==0)==(credit==threshold),"credit/rejection threshold equivalence");
  }
  gate.require(work.peak_stack>=1 && work.peak_stack<=mhgp8::index_stack_frames && work.stack_storage_bytes>=mhgp8::index_stack_frames*sizeof(std::size_t),
               "proven DFS storage contract");
  if (work.admitted_nodes>work.point_tests) gate.whole_admissions+=work.admitted_nodes-work.point_tests;
  gate.node_rejections+=work.h_excluded_nodes;
  gate.saturations+=work.q3_rejected+work.q4_rejected;
  gate.peak_stack=std::max(gate.peak_stack,work.peak_stack);
  if (result!=0) ++gate.exhausted_queries;
  if (work.q3_credits!=0 && (result&2U)!=0) ++gate.partial_q3;
  if (work.q4_credits!=0 && (result&4U)!=0) ++gate.partial_q4;
}

void outcome(Gate& gate,unsigned k,unsigned mask,std::uint8_t result) {
  ++gate.queries;
  if (mask==0) ++gate.zero_mask_queries;
  const auto removed=static_cast<unsigned>(active(k,mask)&~result);
  if ((removed&2U)!=0) ++gate.q3_rejections;
  if ((removed&4U)!=0) ++gate.q4_rejections;
  if (removed==2) ++gate.q3_only_rejections;
  if (removed==4) ++gate.q4_only_rejections;
  if (removed==6) ++gate.both_rejections;
  if (removed==0) ++gate.no_rejections;
}

Work singleton(Gate& gate,const mhgp8::Q2CensusIndex& index,const Points& sites,
               Point3 a,Point3 b,unsigned k,unsigned mask) {
  const auto exact=counts(gate,sites,a,b);
  Work work;
  const auto result=mhgp8::filter_q34_witnesses(index,mhgp8::singleton_box(a),mhgp8::singleton_box(b),
      static_cast<std::uint8_t>(k),static_cast<std::uint8_t>(mask),work);
  // Geometric assertion precedes all work identities: mutation qualification
  // cannot accidentally use a bookkeeping failure instead of the citron.
  gate.require(result==expected(k,mask,exact),"singleton witness decision differs from independent oracle");
  const auto requested=active(k,mask);
  gate.require(work.q3_credits==((requested&2U)!=0?std::min<u64>(exact[0],k-1):0) &&
      work.q4_credits==((requested&4U)!=0?std::min<u64>(exact[1],k-2):0),
      "singleton witness decision differs from independent oracle");
  ++gate.singleton_queries;outcome(gate,k,mask,result);ledger(gate,work,k,mask,result);
  return work;
}

void rectangle(Gate& gate,const mhgp8::Q2CensusIndex& index,const Points& sites,
               const Box3& a,const Box3& b,unsigned k,unsigned mask) {
  const auto exact=common_counts(gate,sites,a,b);
  Work work;
  const auto result=mhgp8::filter_q34_witnesses(index,a,b,static_cast<std::uint8_t>(k),
      static_cast<std::uint8_t>(mask),work);
  gate.require(work.q3_credits<=exact[0] && work.q4_credits<=exact[1],
      "rectangle credited more than its independent common-witness set");
  const auto removed=static_cast<unsigned>(active(k,mask)&~result);
  const auto ca=corners(a),cb=corners(b);
  for (const auto x:ca) for (const auto y:cb) {
    const auto pair=counts(gate,sites,x,y);
    gate.require(((removed&2U)==0 || pair[0]>=k-1) && ((removed&4U)==0 || pair[1]>=k-2),
        "rectangle rejection removed a pair with too few exact citron witnesses");
  }
  ++gate.rectangle_queries;outcome(gate,k,mask,result);ledger(gate,work,k,mask,result);
}

void exhaustive(Gate& gate,const Points& sites,bool all_k=false) {
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(sites));
  for (std::size_t a=0;a<sites.size();++a) for (std::size_t b=a+1;b<sites.size();++b)
    for (unsigned k=1;k<=10;++k) {
      if (!all_k && k!=1 && k!=2 && k!=3 && k!=5 && k!=10) continue;
      for (const unsigned mask:{0U,2U,4U,6U})
        static_cast<void>(singleton(gate,*index,sites,sites[a],sites[b],k,mask));
    }
  const auto nodes=index->spatial_nodes();
  unsigned visited=0;
  for (std::size_t a=0;a<nodes.size() && visited<14;++a)
    for (std::size_t b=a+1;b<nodes.size() && visited<14;++b) {
      if (nodes[a].range.first<nodes[b].range.last && nodes[b].range.first<nodes[a].range.last) continue;
      for (const unsigned k:{2U,3U,5U,10U}) rectangle(gate,*index,sites,nodes[a].box,nodes[b].box,k,6);
      ++visited;
    }
  rectangle(gate,*index,sites,nodes[0].box,nodes[0].box,3,6);
}

void contacts_and_counterexamples(Gate& gate) {
  const Point3 a{30,30,30},b{36,36,30};
  const Points q3{a,b,{36,30,36},{32,34,28}};
  const auto c3=citron(a,b,q3[3]);
  gate.require(c3.h4>0 && 3*c3.h4*c3.h4==c3.xi16 && !c3.q3,"q3 strict contact fixture");
  ++gate.q3_contacts;
  const auto ball3=rational::make(std::span<const Point3>(q3.data(),3));
  gate.require(ball3.ball && ball3.ball->power(q3[3]).numerator()==0,"q3 contact is not on its positive ball");
  const auto i3=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(q3));
  static_cast<void>(singleton(gate,*i3,q3,a,b,2,2));
  const Points q4{a,b,{30,36,24},{36,30,24},{32,32,32}};
  const auto c4=citron(a,b,q4[4]);
  gate.require(c4.h4>0 && 2*c4.h4*c4.h4==c4.xi16 && !c4.q4,"q4 strict contact fixture");
  ++gate.q4_contacts;
  const auto ball4=rational::make(std::span<const Point3>(q4.data(),4));
  gate.require(ball4.ball && ball4.ball->power(q4[4]).numerator()==0,"q4 contact is not on its positive ball");
  const auto i4=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(q4));
  static_cast<void>(singleton(gate,*i4,q4,a,b,3,4));
  gate.require(citron(a,b,a).h4==0 && !citron(a,b,a).q3 && !citron(a,b,a).q4,"support endpoint credited");
  ++gate.h_contacts;
  // Explicit coordinate fixture from auditor B, translated by (+100,+100,+100).
  // Its geometry is re-proved here by our Gram solver; no audit result is inherited.
  const Points wrong{{100,100,100},{160,100,100},{120,142,100},{128,110,149},{128,88,88}};
  const auto witness=citron(wrong[0],wrong[1],wrong[4]);
  gate.require(witness.h4==2432 && witness.xi16==16588800 && witness.q3 && !witness.q4,
      "alpha4 counterexample changed");
  const auto ball=rational::make(std::span<const Point3>(wrong.data(),4));
  gate.require(ball.ball && ball.ball->power(wrong[4]).numerator()>0,
      "wrong-alpha witness is not strictly outside a positive q4 ball");
  ++gate.wrong_alpha_cases;++gate.positive_q4_external;
  const auto iw=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(wrong));
  static_cast<void>(singleton(gate,*iw,wrong,wrong[0],wrong[1],3,4));
  Points two=wrong;two.push_back({127,88,88});
  const auto it=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(two));
  static_cast<void>(singleton(gate,*it,two,two[0],two[1],3,6));
  exhaustive(gate,q3);exhaustive(gate,q4);exhaustive(gate,wrong,true);
}

void block_and_order_fixtures(Gate& gate) {
  const Point3 a{100,100,100},b{200,100,100};
  const auto dense=clustered();
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(dense));
  for (unsigned k=1;k<=10;++k) for (const unsigned mask:{0U,2U,4U,6U})
    static_cast<void>(singleton(gate,*index,dense,a,b,k,mask));
  rectangle(gate,*index,dense,{{99,99,99},{101,101,101}},{{199,99,99},{201,101,101}},10,6);
  const Points middle(dense.begin()+4,dense.end());
  const auto im=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(middle));
  const auto admitted=singleton(gate,*im,middle,a,b,10,6);
  gate.require(admitted.node_visits==1 && admitted.point_tests==0 && admitted.q3_credits==9 && admitted.q4_credits==8,
      "whole root admission not exercised");
  const Points partial{{149,125,100},{150,126,100}};
  const auto ip=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(partial));
  const auto split=singleton(gate,*ip,partial,a,b,10,6);
  gate.require(split.q3_credits==2 && split.q4_credits==1 && split.q3_admitted_nodes==1 && split.split_nodes!=0,
      "partial q3 admission followed by q4 refinement not exercised");
  ++gate.partial_admission_cases;
  const Points right{{0,0,0},{100,0,0}};
  const auto ir=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(right));
  const auto ordered=singleton(gate,*ir,right,{90,0,0},{110,0,0},2,2);
  gate.require(ordered.node_visits==2 && ordered.point_tests==1 && ordered.h_excluded_nodes==0,
      "nearest right child did not precede the distant left child");
  gate.require(ordered.q3_admitted_nodes==1 && ordered.q3_credits==1 && ordered.fully_admitted_nodes==1,
      "near-right singleton did not supply exactly one witness credit");
  ++gate.near_right_splits;++gate.point_admission_cases;
  const Points tied{{94,100,100},{100,106,100}};
  const auto il=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(tied));
  const auto left=singleton(gate,*il,tied,{90,100,100},{110,100,100},2,2);
  gate.require(left.node_visits==2 && left.point_tests==1,"equal midpoint distances did not choose left first");
  ++gate.left_tie_cases;
  // Equality fixture of the proven boundary: one power of two per axis and
  // bit, so the midpoint index reaches exactly max_index_depth (54 at 18 bits).
  Points deep{{0,0,0}};
  for (unsigned axis=0;axis<3;++axis) for (unsigned bit=0;bit<mhgp8::coordinate_bits;++bit) {
    std::array<mhgp8::Coordinate,3> coordinate{};coordinate[axis]=static_cast<mhgp8::Coordinate>(1U<<bit);
    deep.push_back({coordinate[0],coordinate[1],coordinate[2]});
  }
  const auto id=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(deep));
  const auto depth=singleton(gate,*id,deep,{0,0,0},{1,1,1},10,6);
  gate.require(id->work().max_depth==mhgp8::max_index_depth && depth.peak_stack==mhgp8::index_stack_frames,"proven boundary depth not exercised");
  ++gate.deep_index_cases;
}

void validation_and_lifecycle(Gate& gate) {
  auto sites=clustered();const auto original=sites;
  auto cloud=mhgp8::prepare_cloud(sites);
  auto index=mhgp8::make_q2_cloud_index(cloud);
  const auto a=mhgp8::singleton_box(sites[0]),b=mhgp8::singleton_box(sites[1]);
  Work baseline;
  const auto expected_mask=mhgp8::filter_q34_witnesses(*index,a,b,5,6,baseline);
  const auto invalid=[&](unsigned k,unsigned mask,Box3 first,Box3 second) {
    auto work=baseline;bool threw=false;
    try { static_cast<void>(mhgp8::filter_q34_witnesses(*index,first,second,
               static_cast<std::uint8_t>(k),static_cast<std::uint8_t>(mask),work)); }
    catch (const std::invalid_argument&) { threw=true; }
    gate.require(threw && work==baseline,"invalid search arguments changed accumulated work");++gate.invalid_inputs;
  };
  for (unsigned k:{0U,11U,255U}) invalid(k,6,a,b);
  for (unsigned mask:{1U,3U,5U,7U,8U,255U}) invalid(5,mask,a,b);
  invalid(1,0,{{2,0,0},{1,0,0}},b);invalid(1,0,a,{{0,2,0},{0,1,0}});
  static_cast<void>(singleton(gate,*index,sites,sites[0],sites[0],5,6));
  Work accumulated=baseline;
  gate.require(mhgp8::filter_q34_witnesses(*index,a,b,5,6,accumulated)==expected_mask,"repeat changed decision");
  const auto old=std::bit_cast<std::array<u64,25>>(baseline),twice=std::bit_cast<std::array<u64,25>>(accumulated);
  for (std::size_t i=0;i<25;++i) gate.require(twice[i]==(i<23?2*old[i]:old[i]),"SUM/MAX accumulation differs");
  ++gate.repeated_calls;
  Work overflow;overflow.queries=std::numeric_limits<u64>::max();bool threw=false;
  try { static_cast<void>(mhgp8::filter_q34_witnesses(*index,a,b,5,6,overflow)); }
  catch (const std::overflow_error&) { threw=true; }
  gate.require(threw,"checked work overflow did not throw");++gate.overflow_exceptions;
  sites[0]={65535,65535,65535};cloud.reset();
  gate.require(std::equal(index->cloud().points().begin(),index->cloud().points().end(),original.begin()),
      "shared input owner changed through the source alias");++gate.source_alias_checks;
  std::vector<std::future<std::pair<std::uint8_t,Work>>> tasks;
  for (unsigned i=0;i<4;++i) tasks.push_back(std::async(std::launch::async,[owned=index,a,b]() {
    Work work;const auto mask=mhgp8::filter_q34_witnesses(*owned,a,b,5,6,work);
    return std::make_pair(mask,work);
  }));
  index.reset();
  for (auto& task:tasks) {
    const auto result=task.get();
    gate.require(result.first==expected_mask && result.second==baseline,"independent concurrent search differs");
    ++gate.parallel_calls;
  }
}

void run(Gate& gate) {
  contacts_and_counterexamples(gate);
  block_and_order_fixtures(gate);
  exhaustive(gate,random_points(173,8));exhaustive(gate,random_points(901,9));
  Points permuted=clustered();
  const auto forward=counts(gate,permuted,permuted[0],permuted[1]);
  std::reverse(permuted.begin(),permuted.end());
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(permuted));
  for (unsigned k:{3U,5U,10U}) {
    gate.require(counts(gate,permuted,{100,100,100},{200,100,100})==forward,"ID permutation changed oracle count");
    static_cast<void>(singleton(gate,*index,permuted,{100,100,100},{200,100,100},k,6));
  }
  ++gate.permutations;
  const Points extreme{{0,0,0},{65535,65535,65535},{32767,32768,32767},{32768,32767,32768},
      {0,65535,65535},{65535,0,65535},{65535,65535,0},{65535,1,2}};
  const auto ie=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(extreme));
  for (unsigned k:{1U,2U,3U,5U,10U}) for (unsigned mask:{2U,4U,6U}) {
    static_cast<void>(singleton(gate,*ie,extreme,extreme[0],extreme[1],k,mask));++gate.extreme_queries;
  }
  rectangle(gate,*ie,extreme,{{0,0,0},{1,1,1}},{{65534,65534,65534},{65535,65535,65535}},5,6);
  // 18-bit twins: corners at 262143, the halving midpoints 131071/131072, the
  // far-corner rectangle pair, and the old 65535 corner as an interior site.
  const Points extreme18{{0,0,0},{262143,262143,262143},{131071,131072,131071},{131072,131071,131072},
      {0,262143,262143},{262143,0,262143},{262143,262143,0},{262143,1,2},{65535,65535,65535}};
  const auto ie18=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(extreme18));
  for (unsigned k:{1U,2U,3U,5U,10U}) for (unsigned mask:{2U,4U,6U}) {
    static_cast<void>(singleton(gate,*ie18,extreme18,extreme18[0],extreme18[1],k,mask));++gate.extreme18_queries;
  }
  rectangle(gate,*ie18,extreme18,{{0,0,0},{1,1,1}},{{262142,262142,262142},{262143,262143,262143}},5,6);
  rectangle(gate,*ie18,extreme18,{{65534,65534,65534},{65535,65535,65535}},{{262142,262142,262142},{262143,262143,262143}},3,6);
  validation_and_lifecycle(gate);
  gate.require(gate.q3_only_rejections && gate.q4_only_rejections && gate.both_rejections && gate.no_rejections &&
      gate.partial_q3 && gate.partial_q4 && gate.whole_admissions && gate.point_admission_cases && gate.node_rejections &&
      gate.saturations && gate.near_right_splits && gate.exhausted_queries && gate.wide_predicate_cases,
      "required geometric branch was not exercised");
  gate.require(gate.extreme18_queries==15 && gate.wide18_predicate_cases,
      "18-bit extreme branch was not exercised");
}

using BoundsMode=mhgp8::Q34WitnessBoundsMode;
using BoundsWork=mhgp8::Q34WitnessBoundsWork;
static_assert(std::is_trivially_copyable_v<BoundsWork> && std::is_standard_layout_v<BoundsWork> &&
              sizeof(BoundsWork)==12*sizeof(u64));

struct ModeResult {std::uint8_t mask;Work search;BoundsWork bounds;};
ModeResult mode_query(Gate& gate,const mhgp8::Q2CensusIndex& index,const Points& sites,
    const Box3& a,const Box3& b,unsigned k,unsigned mask,BoundsMode mode) {
  const bool pair=a.low==a.high && b.low==b.high;
  const auto exact=pair?counts(gate,sites,a.low,b.low):common_counts(gate,sites,a,b);
  ModeResult result{};
  result.mask=mhgp8::filter_q34_witnesses(index,a,b,static_cast<std::uint8_t>(k),
      static_cast<std::uint8_t>(mask),result.search,mode,result.bounds);
  const auto requested=active(k,mask);
  const auto& w=result.search;const auto& v=result.bounds;
  // All geometric checks precede counter identities, including the exact
  // saturated credits for singleton pairs. Local exclusions earn NO credit.
  if (pair) {
    gate.require(result.mask==expected(k,mask,exact) &&
      w.q3_credits==((requested&2U)?std::min<u64>(exact[0],k-1):0) &&
      w.q4_credits==((requested&4U)?std::min<u64>(exact[1],k-2):0),
      "singleton witness decision differs from independent oracle");
    ++gate.mode_singletons;
  } else {
    gate.require(w.q3_credits<=exact[0] && w.q4_credits<=exact[1],
        "rectangle credited more than its independent common-witness set");
    for (const auto x:corners(a)) for (const auto y:corners(b)) {
      const auto total=counts(gate,sites,x,y);
      const auto removed=static_cast<unsigned>(requested&~result.mask);
      gate.require(((removed&2U)==0 || total[0]>=k-1) && ((removed&4U)==0 || total[1]>=k-2),
          "rectangle rejection removed a pair with too few exact citron witnesses");
    }
    ++gate.mode_rectangles;
  }
  ++gate.bounds_calls;
  if (mode==BoundsMode::Legacy) ++gate.legacy_overload_calls;
  if (mode==BoundsMode::Exclusion) ++gate.exclusion_calls;
  if (mode==BoundsMode::Affine) ++gate.affine_calls;
  gate.require(v.queries==static_cast<u64>(mode!=BoundsMode::Legacy) && w.queries==1,"mode query accounting");
  gate.require(w.node_visits==w.h_bound_tests && w.node_visits==w.h_excluded_nodes+
      w.fully_admitted_nodes+w.leaf_remainders+w.split_nodes+v.fully_excluded_nodes+v.mixed_terminal_nodes,
      "new node classification partition");
  gate.require(w.midpoint_box_tests==2*w.split_nodes && v.q3_excluded_nodes<=v.q3_exclusion_tests &&
      v.q4_excluded_nodes<=v.q4_exclusion_tests && v.xi_on_nonpositive_minimum<=w.xi_bound_tests,
      "new exclusion work inclusions");
  if (requested==0) {
    Work empty;empty.queries=1;BoundsWork other;other.queries=static_cast<u64>(mode!=BoundsMode::Legacy);
    gate.require(w==empty && v==other,"inactive bounds mode did hidden geometry");
  } else if (mode==BoundsMode::Legacy) {
    Work old;
    const auto old_mask=mhgp8::filter_q34_witnesses(index,a,b,static_cast<std::uint8_t>(k),
        static_cast<std::uint8_t>(mask),old);
    BoundsWork only;
    gate.require(result.mask==old_mask && w==old && v==only,"explicit Legacy differs from old overload");
  } else {
    gate.require(v.pair_preparations+v.general_preparations==1 &&
      v.pair_preparations==static_cast<u64>(mode==BoundsMode::Affine && pair),"bound preparation selection");
    gate.require(v.affine_h_tests==(v.pair_preparations?w.h_bound_tests:0) &&
      v.affine_xi_tests==(v.pair_preparations?w.xi_bound_tests:0),"affine primitive work accounting");
  }
  gate.affine_preparations+=v.pair_preparations;gate.general_preparations+=v.general_preparations;
  gate.excluded_q3_nodes+=v.q3_excluded_nodes;gate.excluded_q4_nodes+=v.q4_excluded_nodes;
  gate.fully_excluded_nodes+=v.fully_excluded_nodes;gate.mode_xi_nonpositive+=v.xi_on_nonpositive_minimum;
  return result;
}

void bounds_mode_fixtures(Gate& gate) {
  std::vector<Points> fixtures{
    random_points(173,8),random_points(901,9),clustered(),
    {{30,30,30},{36,36,30},{36,30,36},{32,34,28}},
    {{30,30,30},{36,36,30},{30,36,24},{36,30,24},{32,32,32}},
    {{100,100,100},{160,100,100},{120,142,100},{128,110,149},{128,88,88}},
    {{0,0,0},{65535,65535,65535},{0,65535,65535},{65535,0,65535},{65535,65535,0},{32767,32768,32767}},
    {{0,0,0},{262143,262143,262143},{0,262143,262143},{262143,0,262143},{262143,262143,0},{131071,131072,131071}}};
  for (std::size_t fixture=0;fixture<fixtures.size();++fixture) {
    auto sites=fixtures[fixture];
    if (fixture%2) std::reverse(sites.begin(),sites.end());
    const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(sites));
    for (std::size_t a=0;a<sites.size();++a) for (std::size_t b=a+1;b<sites.size();++b) {
      // The larger clustered fixture is exercised only on its supplied edge;
      // the other clouds (16-bit and 18-bit corners alike) cover ALL pairs and
      // K1..10 in all lane submasks.
      if (fixture==2 && (a!=0 || b!=1)) continue;
      for (unsigned k=1;k<=10;++k) for (unsigned mask:{0U,2U,4U,6U})
        for (const auto mode:{BoundsMode::Legacy,BoundsMode::Exclusion,BoundsMode::Affine})
          static_cast<void>(mode_query(gate,*index,sites,mhgp8::singleton_box(sites[a]),
              mhgp8::singleton_box(sites[b]),k,mask,mode));
    }
    const auto nodes=index->spatial_nodes();
    for (std::size_t i=1;i<std::min<std::size_t>(nodes.size(),6);++i)
      for (const auto mode:{BoundsMode::Exclusion,BoundsMode::Affine})
        static_cast<void>(mode_query(gate,*index,sites,nodes[0].box,nodes[i].box,5,6,mode));
  }
  const Point3 a{100,100,100},b{200,100,100};
  const auto abox=mhgp8::singleton_box(a),bbox=mhgp8::singleton_box(b);
  const Points outside{{149,145,100},{151,151,100}},mixed{{150,126,100}};
  for (const auto mode:{BoundsMode::Exclusion,BoundsMode::Affine}) {
    // Xi_low must not be replaced by Xi_high: the node contains a central
    // strict witness and a distant non-witness. Exclusion of the WHOLE node
    // from its far corner would lose the K2 q3 rejection.
    const Points crossing{{150,100,100},{150,145,100}};
    const auto ic=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(crossing));
    static_cast<void>(mode_query(gate,*ic,crossing,abox,bbox,2,2,mode));
    const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(outside));
    const auto result=mode_query(gate,*index,outside,abox,bbox,10,6,mode);
    gate.require(result.mask==6 && result.search.q3_credits==0 && result.search.q4_credits==0 &&
      result.bounds.fully_excluded_nodes>0,"local witness exclusion removed an entire global lane");
    ++gate.local_exclusion_cases;
    gate.require(result.bounds.xi_on_nonpositive_minimum>0,"nonpositive minimum did not require Xi");
    ++gate.nonpositive_minimum_cases;
    const auto im=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(mixed));
    const auto value=mode_query(gate,*im,mixed,abox,bbox,10,6,mode);
    gate.require(value.mask==6 && value.search.q3_credits==1 && value.search.q4_credits==0 &&
      value.bounds.mixed_terminal_nodes==1 && value.bounds.q4_excluded_nodes==1,
      "mixed q3 admission/q4 exclusion fixture failed");++gate.mixed_terminal_cases;
  }
  const auto sites=clustered();auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(sites));
  const auto baseline=mode_query(gate,*index,sites,abox,bbox,5,6,BoundsMode::Affine);
  Work accumulated=baseline.search;BoundsWork accumulated_bounds=baseline.bounds;
  gate.require(mhgp8::filter_q34_witnesses(*index,abox,bbox,5,6,accumulated,BoundsMode::Affine,
      accumulated_bounds)==baseline.mask,"repeated affine query changed geometry");
  const auto first=std::bit_cast<std::array<u64,12>>(baseline.bounds);
  const auto second=std::bit_cast<std::array<u64,12>>(accumulated_bounds);
  for (std::size_t i=0;i<12;++i) gate.require(second[i]==2*first[i],"new SUM ledger differs");
  ++gate.mode_repeated_calls;
  std::array<u64,12> sentinels{};
  for (std::size_t i=0;i<sentinels.size();++i) sentinels[i]=static_cast<u64>(i+1);
  auto untouched=std::bit_cast<BoundsWork>(sentinels);const auto original_bounds=untouched;
  Work old,explicit_old;
  const auto old_mask=mhgp8::filter_q34_witnesses(*index,abox,bbox,5,6,old);
  gate.require(mhgp8::filter_q34_witnesses(*index,abox,bbox,5,6,explicit_old,BoundsMode::Legacy,untouched)==old_mask &&
    explicit_old==old && untouched==original_bounds,"Legacy modified an existing new-work ledger");
  for (const unsigned k:{1U,5U}) {
    Work w=baseline.search;BoundsWork v=baseline.bounds;bool threw=false;
    try {static_cast<void>(mhgp8::filter_q34_witnesses(*index,abox,bbox,static_cast<std::uint8_t>(k),0,w,
        static_cast<BoundsMode>(99),v));} catch (const std::invalid_argument&) {threw=true;}
    gate.require(threw && w==baseline.search && v==baseline.bounds,"invalid bounds enum changed work or was ignored");
    ++gate.mode_invalid_inputs;
  }
  BoundsWork overflow;overflow.queries=std::numeric_limits<u64>::max();Work work;bool threw=false;
  try {static_cast<void>(mhgp8::filter_q34_witnesses(*index,abox,bbox,5,6,work,BoundsMode::Affine,overflow));}
  catch (const std::overflow_error&) {threw=true;}
  gate.require(threw,"new bounds counters silently wrapped");++gate.mode_overflows;
  std::array<std::future<ModeResult>,4> tasks;
  for (auto& task:tasks) task=std::async(std::launch::async,[owned=index,abox,bbox] {
    ModeResult r{};r.mask=mhgp8::filter_q34_witnesses(*owned,abox,bbox,5,6,r.search,BoundsMode::Affine,r.bounds);return r;
  });
  index.reset();
  for (auto& task:tasks) {const auto result=task.get();
    gate.require(result.mask==baseline.mask && result.search==baseline.search && result.bounds==baseline.bounds,
      "private affine searches shared mutable state");++gate.mode_parallel_calls;}
  gate.require(gate.affine_preparations && gate.general_preparations && gate.excluded_q3_nodes &&
      gate.excluded_q4_nodes && gate.fully_excluded_nodes && gate.mode_xi_nonpositive,
      "new witness-bound branches not exercised");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    Gate gate;run(gate);bounds_mode_fixtures(gate);
    std::cout<<"{\"schema\":\"mhgp8_q34_witness_search_gate_v1\",\"status\":\"PASS\"";
#define FIELD(name) std::cout<<",\"" #name "\":"<<gate.name
    FIELD(checks);FIELD(queries);FIELD(singleton_queries);FIELD(rectangle_queries);FIELD(oracle_sites);FIELD(oracle_pairs);
    FIELD(q3_rejections);FIELD(q4_rejections);FIELD(q3_only_rejections);FIELD(q4_only_rejections);FIELD(both_rejections);
    FIELD(no_rejections);FIELD(inactive_queries);FIELD(zero_mask_queries);FIELD(partial_q3);FIELD(partial_q4);
    FIELD(h_contacts);FIELD(q3_contacts);FIELD(q4_contacts);FIELD(wrong_alpha_cases);FIELD(positive_q4_external);
    FIELD(whole_admissions);FIELD(point_admission_cases);FIELD(node_rejections);FIELD(saturations);FIELD(near_right_splits);
    FIELD(exhausted_queries);FIELD(permutations);FIELD(extreme_queries);FIELD(invalid_inputs);FIELD(parallel_calls);
    FIELD(repeated_calls);FIELD(source_alias_checks);FIELD(left_tie_cases);FIELD(partial_admission_cases);
    FIELD(deep_index_cases);FIELD(peak_stack);FIELD(wide_predicate_cases);FIELD(overflow_exceptions);
    FIELD(bounds_calls);FIELD(legacy_overload_calls);FIELD(exclusion_calls);FIELD(affine_calls);FIELD(mode_singletons);FIELD(mode_rectangles);
    FIELD(local_exclusion_cases);FIELD(nonpositive_minimum_cases);FIELD(mixed_terminal_cases);FIELD(mode_parallel_calls);
    FIELD(mode_invalid_inputs);FIELD(mode_repeated_calls);FIELD(mode_overflows);FIELD(affine_preparations);FIELD(general_preparations);
    FIELD(excluded_q3_nodes);FIELD(excluded_q4_nodes);FIELD(fully_excluded_nodes);FIELD(mode_xi_nonpositive);
    FIELD(extreme18_queries);FIELD(wide18_predicate_cases);
#undef FIELD
    std::cout<<"}\n";return 0;
  } catch (const std::exception& error) {
    std::cerr<<"q34 witness search gate: "<<error.what()<<'\n';return 1;
  }
}
