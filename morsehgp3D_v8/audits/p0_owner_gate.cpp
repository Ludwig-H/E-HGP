#include "pipeline/local_credits.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {

void require(bool condition, const char* cause) {
  if (!condition) throw std::runtime_error(cause);
}

mhgp8::RectangleInput input(bool square) {
  mhgp8::RectangleInput result;
  if (square) {
    result.points = {{0, 0, 0}, {0, 10, 0}, {1000, 0, 0}, {1000, 10, 0}};
  } else {
    result.points = {{0, 0, 0}, {1, 0, 0}, {100, 0, 0}, {101, 0, 0}};
  }
  result.a = {0, 2};
  result.b = {2, 4};
  return result;
}

// Independent bounded q2 judge: direct dot product, no product predicate.
// All coordinates here are <=1000, so these products fit signed long long.
bool alive(const mhgp8::PreparedRectangle& rectangle,
           std::size_t a, std::size_t b) {
  const auto points = rectangle.points();
  unsigned interior = 0;
  for (std::size_t z = 0; z < points.size(); ++z) {
    if (z == a || z == b) continue;
    long long h = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto u = static_cast<long long>(points[z][axis]) - points[a][axis];
      const auto v = static_cast<long long>(points[b][axis]) - points[z][axis];
      h += u * v;
    }
    interior += static_cast<unsigned>(h > 0);
  }
  return interior < rectangle.threshold(mhgp8::Lane::Q2);
}

std::size_t lost_pairs(const mhgp8::CreditPlan& plan) {
  std::size_t count = 0;
  for (std::size_t a = 0; a < 2; ++a)
    for (std::size_t b = 2; b < 4; ++b)
      count += static_cast<std::size_t>(alive(plan.rectangle(), a, b) &&
                                       !plan.keeps(a, b));
  return count;
}

template <typename Rectangle>
void run() {
  const auto source = mhgp8::prepare_rectangle(input(false), 1, 8);
  const auto replacement = mhgp8::prepare_rectangle(input(true), 1, 8);
  const auto first = mhgp8::make_credit_plan(source, mhgp8::Lane::Q2,
                                            mhgp8::Strategy::DualBlocks);
  const auto second = mhgp8::make_credit_plan(replacement, mhgp8::Lane::Q2,
                                             mhgp8::Strategy::DualBlocks);
  require(first.candidate_pairs() == 1 && second.candidate_pairs() == 4,
          "owner.nominal_pair_counts");
  require(lost_pairs(first) == 0 && lost_pairs(second) == 0,
          "owner.nominal_geometry");
  if constexpr (std::is_copy_constructible_v<Rectangle> &&
                std::is_copy_assignable_v<Rectangle>) {
    // These are ordinary public API operations, with no const_cast or UB.
    auto mutable_owner = std::make_shared<Rectangle>(*source);
    const auto plan = mhgp8::make_credit_plan(mutable_owner, mhgp8::Lane::Q2,
                                             mhgp8::Strategy::DualBlocks);
    require(lost_pairs(plan) == 0, "owner.before_assignment");
    *mutable_owner = *replacement;
    require(lost_pairs(plan) == 3, "owner.mutation_not_exercised");
    require(lost_pairs(first) == 0, "owner.source_was_not_copied");
    std::cout << "{\"status\":\"observed_owner_rebinding\",\"lost_q2_pairs\":3,"
                 "\"nominal_pairs\":[1,4],\"product_scope\":\"P0_only\","
                 "\"full_qualification\":false,\"gcp_used\":false}\n";
  } else {
    require(!std::is_copy_constructible_v<Rectangle> &&
            !std::is_copy_assignable_v<Rectangle> &&
            !std::is_move_constructible_v<Rectangle> &&
            !std::is_move_assignable_v<Rectangle>, "owner.special_members_open");
    auto shared_copy = source;
    const auto plan = mhgp8::make_credit_plan(shared_copy, mhgp8::Lane::Q2,
                                             mhgp8::Strategy::DualBlocks);
    require(plan.candidate_pairs() == 1 && lost_pairs(plan) == 0,
            "owner.shared_factory_use");
    std::cout << "{\"status\":\"passed_owner_closure\",\"closed_special_members\":4,"
                 "\"nominal_pairs\":[1,4],\"product_scope\":\"P0_only\","
                 "\"full_qualification\":false,\"gcp_used\":false}\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    run<mhgp8::PreparedRectangle>();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
