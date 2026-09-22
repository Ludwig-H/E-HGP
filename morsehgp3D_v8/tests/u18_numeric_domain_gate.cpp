#include "core/types.hpp"
#include "lanes/exact_ball.hpp"
#include "lanes/family_certificate.hpp"
#include "lanes/q34_pair_bounds.hpp"
#include "lanes/q4_local.hpp"
#include "pipeline/q2_joint_bounds.hpp"
#include "spindle/q2_prepared_bounds.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using namespace mhgp8;

struct Gate {
  u64 checks{}, factory_refusals{}, box_refusals{}, form_refusals{}, center_refusals{}, valid_extremes{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class F> void rejects(F&& operation, u64& category) {
    bool rejected = false;
    try { operation(); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid numeric input was not refused before arithmetic");
    ++category;
  }
};

Point3 axis_point(std::size_t axis, Coordinate coordinate) {
  Point3 point{};
  if (axis == 0) point.x = coordinate;
  else if (axis == 1) point.y = coordinate;
  else point.z = coordinate;
  return point;
}

void run(Gate& gate) {
  constexpr auto m = coordinate_limit;
  const Point3 a{0,0,0}, b{m,m,0}, x{m,0,m}, y{0,m,m};
  const std::array<Point3,4> support{a,b,x,y};
  const auto q2 = ExactBall::make_q2(a,b);
  const auto q3 = ExactBall::make_q3({a,b,x});
  const auto q4 = ExactBall::make_q4(support);
  const auto family = Q4FamilySeed::make(a,b,x);
  const auto certificate = Q34FamilyCertificate::make(a,b,x);
  gate.require(q2 && q3 && q4 && family && certificate, "valid u18 extreme supports rejected");
  for (std::size_t i=0; i<support.size(); ++i) {
    gate.require(q4->power(support[i]) == 0, "q4 u18 support is not on shell");
    if (i<3) {
      gate.require(q3->power(support[i]) == 0, "q3 u18 support is not on shell");
      gate.require(family->power(support[i]) == 0 && family->side(support[i]) == 0,
                   "u18 family seed contact lost");
    }
    ++gate.valid_extremes;
  }
  const Box3 valid{{0,0,0},{m,m,m}};
  const Q2PreparedBounds single(a,singleton_box(b));
  const Q2JointPreparedBounds joint(singleton_box(a),singleton_box(b));
  const PreparedPairCitronBounds pair(a,b);
  const auto one = single.bounds(valid), two = joint.bounds(valid), three = pair.h_bounds(valid);
  gate.require(one.minimum4 == two.minimum4 && one.maximum4 == two.maximum4 &&
               one.minimum4 == three.minimum4 && one.maximum4 == three.maximum4,
               "prepared u18 H bounds disagree");
  gate.require(pair.xi_bounds(valid).low >= 0 && pair.xi_bounds(valid).high >= 0,
               "u18 Xi bounds escaped nonnegative range");
  const std::array<Coordinate,4> invalid{-1,coordinate_limit+1,
      std::numeric_limits<Coordinate>::min(),std::numeric_limits<Coordinate>::max()};
  for (const auto value : invalid) for (std::size_t axis=0;axis<3;++axis) {
    const auto bad = axis_point(axis,value);
    gate.require(!valid_point(bad), "invalid point passed domain predicate");
    gate.rejects([&] { require_valid_point(bad); }, gate.factory_refusals);
    for (unsigned slot=0;slot<2;++slot)
      gate.rejects([&] { static_cast<void>(ExactBall::make_q2(slot==0?bad:a,slot==1?bad:b)); }, gate.factory_refusals);
    for (std::size_t slot=0;slot<3;++slot) {
      std::array<Point3,3> points{a,b,x}; points[slot]=bad;
      gate.rejects([&] { static_cast<void>(ExactBall::make_q3(points)); }, gate.factory_refusals);
      gate.rejects([&] { static_cast<void>(Q4FamilySeed::make(points[0],points[1],points[2])); }, gate.factory_refusals);
      gate.rejects([&] { static_cast<void>(Q34FamilyCertificate::make(points[0],points[1],points[2])); }, gate.factory_refusals);
    }
    for (std::size_t slot=0;slot<4;++slot) {
      auto points=support; points[slot]=bad;
      gate.rejects([&] { static_cast<void>(ExactBall::make_q4(points)); }, gate.factory_refusals);
    }
    gate.rejects([&] { static_cast<void>(Q2PreparedBounds(bad,valid)); }, gate.factory_refusals);
    gate.rejects([&] { static_cast<void>(PreparedPairCitronBounds(bad,b)); }, gate.factory_refusals);
    gate.rejects([&] { static_cast<void>(PreparedPairCitronBounds(a,bad)); }, gate.factory_refusals);
    const auto box=singleton_box(bad);
    gate.require(!valid_box(box), "ordered out-of-domain box was accepted");
    gate.rejects([&] { static_cast<void>(Q2PreparedBounds(a,box)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(Q2JointPreparedBounds(box,valid)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(Q2JointPreparedBounds(valid,box)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(single.bounds(box)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(joint.bounds(box)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(pair.h_bounds(box)); }, gate.box_refusals);
    gate.rejects([&] { static_cast<void>(pair.xi_bounds(box)); }, gate.box_refusals);
  }
  // Former definite overflow: this acute triangle has W_x=4*INT32_MAX^5
  // (157 bits); all arguments are now refused before even computing D/E/F.
  const auto large=std::numeric_limits<Coordinate>::max();
  gate.rejects([&] { static_cast<void>(ExactBall::make_q3({a,{large,large,0},{large,0,large}})); },
               gate.factory_refusals);

  const auto cloud=prepare_cloud(support);
  const auto index=make_q2_cloud_index(cloud);
  const auto cover=Q34EdgeCover::make(index,{0,1});
  Q4LocalOptions options;
  options.domain=Q4CenterDomainMode::Disk; options.max_depth=0; options.node_budget=1;
  const auto atlas=Q4LocalAtlas::make(cover,3,options);
  const auto& geometry=*atlas->geometry();
  const auto form=geometry.form(2);
  const auto before=geometry.bounds(form,{});
  gate.require(before.minimum <= before.maximum, "valid extreme form rejected");
  const auto center=geometry.q3_center(2);
  gate.require(atlas->certified_inside_count(center).has_value(), "valid generated q3 centre rejected");
  ++gate.valid_extremes;
  constexpr i64 square=static_cast<i64>(coordinate_limit)*coordinate_limit;
  for (const i64 value : std::array<i64,5>{15*square+1,-15*square-1,i64{1}<<44,
          std::numeric_limits<i64>::min(),std::numeric_limits<i64>::max()})
    gate.rejects([&] { static_cast<void>(geometry.bounds({value,0,0},{})); }, gate.form_refusals);
  for (const i64 value : std::array<i64,4>{8*square+1,-8*square-1,
          std::numeric_limits<i64>::min(),std::numeric_limits<i64>::max()}) {
    gate.rejects([&] { static_cast<void>(geometry.bounds({0,value,0},{})); }, gate.form_refusals);
    gate.rejects([&] { static_cast<void>(geometry.bounds({0,0,value},{})); }, gate.form_refusals);
  }
  const i128 min128=std::numeric_limits<i128>::min(), max128=std::numeric_limits<i128>::max();
  constexpr i128 limit=i128{1}<<117;
  gate.require(!atlas->certified_inside_count({limit-1,0,1}) &&
               !atlas->certified_inside_count({0,-limit+1,1}),
               "in-domain far rational center did not leave the root safely");
  gate.require(atlas->certified_inside_count({limit-1,0,limit-1}).has_value(),
               "in-domain large homogeneous center rejected");
  for (const auto value : std::array<i128,6>{limit,-limit,i128{1}<<126,-(i128{1}<<126),min128,max128}) {
    gate.rejects([&] { static_cast<void>(atlas->certified_inside_count({value,0,1})); }, gate.center_refusals);
    gate.rejects([&] { static_cast<void>(atlas->certified_inside_count({0,value,1})); }, gate.center_refusals);
  }
  for (const auto value : std::array<i128,6>{0,-1,limit,(i128{1}<<126)+1,min128,max128})
    gate.rejects([&] { static_cast<void>(atlas->certified_inside_count({0,0,value})); }, gate.center_refusals);
  gate.rejects([&] { static_cast<void>(atlas->certified_inside_count({i128{1}<<126,0,(i128{1}<<126)+1})); },
               gate.center_refusals);
  const auto after=geometry.bounds(form,{});
  gate.require(before.minimum==after.minimum && before.maximum==after.maximum &&
               atlas->certified_inside_count(center).has_value(), "numeric refusal mutated valid geometry");
  gate.require(gate.factory_refusals>=200 && gate.box_refusals>=80 && gate.form_refusals>=13 &&
               gate.center_refusals>=19 && gate.valid_extremes>=5, "numeric rejection fixture coverage empty");
}
}  // namespace

int main(int argc,char** argv) {
  try {
    if(argc!=2 || std::string_view(argv[1])!="--selftest")
      throw std::invalid_argument("usage: mhgp8_u18_numeric_domain_gate --selftest");
    Gate gate;run(gate);
    std::cout << "{\"schema\":\"mhgp8_u18_numeric_domain_gate_v1\",\"status\":\"passed\",\"checks\":" << gate.checks
      << ",\"factory_refusals\":" << gate.factory_refusals << ",\"box_refusals\":" << gate.box_refusals
      << ",\"form_refusals\":" << gate.form_refusals << ",\"center_refusals\":" << gate.center_refusals
      << ",\"valid_extremes\":" << gate.valid_extremes << "}\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr << "u18 numeric domain gate: " << error.what() << '\n';return 1;
  }
}
