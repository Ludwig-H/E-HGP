#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_yao48_seed_work_profile.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

int main() {
  using morsehgp3d::exact::CertifiedPoint3;
  using morsehgp3d::spatial::CanonicalPointCloud;
  using morsehgp3d::spatial::PointId;
  const auto require = [](bool condition) {
    if (!condition) {
      throw std::runtime_error("Morton/Yao48 seed work-profile test failed");
    }
  };

  const CertifiedPoint3 origin =
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0);
  require(morsehgp3d::gpu::classify_binary64_yao48_cone_proposal(
             origin,
             CertifiedPoint3::from_binary64(3.0, 2.0, 1.0)) == 0U);
  require(morsehgp3d::gpu::classify_binary64_yao48_cone_proposal(
             origin,
             CertifiedPoint3::from_binary64(3.0, 1.0, 2.0)) == 1U);
  require(morsehgp3d::gpu::classify_binary64_yao48_cone_proposal(
             origin,
             CertifiedPoint3::from_binary64(-3.0, 2.0, 1.0)) == 6U);
  require(morsehgp3d::gpu::classify_binary64_yao48_cone_proposal(
             origin,
             CertifiedPoint3::from_binary64(1.0, 1.0, 1.0)) == 0U);

  const std::vector<CertifiedPoint3> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(3.0, 0.0, 0.0),
  };
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  const std::array<PointId, 4> morton_order{{0U, 1U, 2U, 3U}};
  const std::array<PointId, 4> neighbors{{1U, 0U, 1U, 2U}};
  const auto profile =
      morsehgp3d::gpu::summarize_morton_yao48_seed_work_profile(
          cloud, morton_order, neighbors, 1U, 2U);
  require(profile.locally_valid());
  require(profile.point_cone_bank_count == 192U);
  require(profile.bank_receipt_count == 4U);
  require(profile.nonempty_bank_count == 4U);
  require(profile.cutoff_ready_bank_count == 4U);
  require(profile.bank_fill_histogram[0] == 188U);
  require(profile.bank_fill_histogram[1] == 4U);
  require(profile.unordered_pair_universe_count == 6U);
  require(profile.survivor_pair_mass == 3U);
  require(profile.certified_pruned_pair_mass == 0U);
  require(profile.unresolved_pair_mass == 3U);
  require(!profile.dense_pair_fallback_performed);
  require(!profile.exact_pair_frontier_claimed);
}
