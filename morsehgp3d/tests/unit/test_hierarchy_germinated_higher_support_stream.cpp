// The differential that justifies replacing the candidate source.
//
// The product subdivision does not prune: measured on the sealed sweep of
// 8 August 2026, a prune certificate covers 1.105 support tuples at n=128 and
// between 1.05 and 1.13 at every size from n=12, while the traversal visits
// 1.83 U nodes against 2U-1 for a fully expanded binary tree.  The germinated
// generator examines 1.81 % of the arity-three universe at n=512 on
// uniform_latin where the subdivision examines 183 %.
//
// None of that matters unless the two produce the SAME accepted supports.  So
// this file compares them as sets, with the same exact classifier on both
// sides -- `classify_exact_higher_support` is called by the germinated stream
// and by the exhaustive reference alike, which is what makes the comparison
// isolate the candidate source and nothing else.
//
// `eight_clusters` is mandatory: `uniform_latin` contains no minimal
// well-centred quadruple at any measured size, so arity four would never be
// exercised by it.

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/germinated_higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/local_germination.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::GerminatedHigherSupportClassificationCounters;
using morsehgp3d::hierarchy::GerminatedHigherSupportEvent;
using morsehgp3d::hierarchy::GerminatedHigherSupportResult;
using morsehgp3d::hierarchy::LocalGerminationCertificate;
using morsehgp3d::hierarchy::LocalGerminationConfig;
using morsehgp3d::hierarchy::build_germinated_higher_support_stream;
using morsehgp3d::hierarchy::enumerate_exhaustive_higher_support_events;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void require(bool condition, std::string_view message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAILED: " << message << '\n';
  }
}

[[nodiscard]] std::vector<CertifiedPoint3> uniform_latin_points(
    std::size_t point_count) {
  constexpr std::size_t modulus = 65537U;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t value = index + 1U;
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(value) / static_cast<double>(modulus),
        static_cast<double>((value * 25173U + 13849U) % modulus) /
            static_cast<double>(modulus),
        static_cast<double>((value * 13849U + 25173U) % modulus) /
            static_cast<double>(modulus)));
  }
  return points;
}

[[nodiscard]] std::vector<CertifiedPoint3> eight_clusters_points(
    std::size_t point_count) {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t cluster = index % 8U;
    const std::size_t local = index / 8U + 1U;
    points.push_back(CertifiedPoint3::from_binary64(
        ((cluster & 1U) == 0U ? 0.25 : 0.75) +
            static_cast<double>(local) * local_scale,
        ((cluster & 2U) == 0U ? 0.25 : 0.75) +
            static_cast<double>((local * 40503U + cluster * 7919U) % 65536U) *
                transverse_scale,
        ((cluster & 4U) == 0U ? 0.25 : 0.75) +
            static_cast<double>((local * 25717U + cluster * 104729U) % 65536U) *
                transverse_scale));
  }
  return points;
}

[[nodiscard]] std::string describe(const GerminatedHigherSupportEvent& event) {
  std::ostringstream text;
  text << '{';
  for (std::size_t position = 0U; position < event.support_size; ++position) {
    if (position != 0U) {
      text << ',';
    }
    text << static_cast<unsigned long long>(event.support_ids[position]);
  }
  text << "} rang=" << event.observed_closed_rank;
  return text.str();
}

// One cell of the differential: same cloud, same order, same classifier, two
// candidate sources.
void check_cell(
    std::string_view family,
    std::vector<CertifiedPoint3> points,
    std::size_t maximum_order) {
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t maximum_relevant_closed_rank =
      std::min(maximum_order + 1U, cloud.size());

  std::ostringstream label;
  label << family << " n=" << cloud.size() << " K=" << maximum_order;
  const std::string cell = label.str();

  LocalGerminationConfig config;
  // The seed loop stays exhaustive over pairs: that is what makes the
  // enumeration complete without any further argument, and completeness is
  // exactly what this differential is testing.
  config.seed_neighbourhood_cutoff_multiple = 0U;
  config.tangent_direction_count = 0U;

  const GerminatedHigherSupportResult germinated =
      build_germinated_higher_support_stream(
          index, cloud, maximum_relevant_closed_rank, config);

  GerminatedHigherSupportClassificationCounters exhaustive_counters;
  const std::vector<GerminatedHigherSupportEvent> exhaustive =
      enumerate_exhaustive_higher_support_events(
          index, cloud, maximum_relevant_closed_rank, exhaustive_counters);

  require(
      germinated.production_identity_holds,
      cell + " : l'identite de production tient (" +
          germinated.refusal_reason + ')');

  // The property that decides everything: the same accepted set.
  require(
      germinated.events == exhaustive,
      cell + " : la germination rend exactement les supports acceptes de "
             "l'enumeration exhaustive");
  if (germinated.events != exhaustive) {
    std::vector<GerminatedHigherSupportEvent> missing;
    std::set_difference(
        exhaustive.begin(),
        exhaustive.end(),
        germinated.events.begin(),
        germinated.events.end(),
        std::back_inserter(missing));
    std::vector<GerminatedHigherSupportEvent> extra;
    std::set_difference(
        germinated.events.begin(),
        germinated.events.end(),
        exhaustive.begin(),
        exhaustive.end(),
        std::back_inserter(extra));
    std::cerr << "  " << cell << " : " << missing.size() << " manquants, "
              << extra.size() << " en trop\n";
    for (std::size_t shown = 0U; shown < missing.size() && shown < 4U;
         ++shown) {
      std::cerr << "    manquant " << describe(missing[shown]) << '\n';
    }
    for (std::size_t shown = 0U; shown < extra.size() && shown < 4U; ++shown) {
      std::cerr << "    en trop  " << describe(extra[shown]) << '\n';
    }
  }

  // Selectivity is not a correctness property and is deliberately not
  // asserted -- it is data-dependent, and on a cloud this small no ball ever
  // reaches s_max, so the germination gates cannot bite.  It is reported so a
  // reader sees what the differential did and does not mistake completeness
  // for selectivity.
  std::cout << cell << " : " << germinated.events.size()
            << " evenements, " << germinated.counters.classified
            << " supports classes par germination contre "
            << exhaustive_counters.classified << " exhaustifs, "
            << germinated.duplicate_accepted_emissions
            << " emissions acceptees en double\n";

  // Acceptance is a subset of production, arity by arity, and the consumer's
  // own tally is what says so.
  const std::size_t tallied_emissions =
      germinated.audits[0].observed_triple_emissions +
      germinated.audits[0].observed_quadruple_emissions +
      germinated.audits[1].observed_triple_emissions +
      germinated.audits[1].observed_quadruple_emissions;
  require(
      tallied_emissions == germinated.counters.classified,
      cell + " : le tally du consommateur compte exactement les emissions "
             "classees");
  require(
      germinated.audits[0].accepted_supports +
              germinated.audits[1].accepted_supports ==
          germinated.counters.accepted,
      cell + " : les acceptations tallees egalent les acceptations classees");
}

// A support the generator never emitted must not appear, and a support it
// emitted twice must appear once.  Both are consumer obligations, so both are
// checked on the consumer's output rather than on the generator's.
void check_deduplication_and_containment() {
  const CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      eight_clusters_points(32U));
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t maximum_relevant_closed_rank = 6U;

  LocalGerminationConfig config;
  config.seed_neighbourhood_cutoff_multiple = 0U;
  config.tangent_direction_count = 0U;
  const GerminatedHigherSupportResult germinated =
      build_germinated_higher_support_stream(
          index, cloud, maximum_relevant_closed_rank, config);

  require(
      std::is_sorted(germinated.events.begin(), germinated.events.end()),
      "les evenements germines sont en ordre canonique");
  require(
      std::adjacent_find(
          germinated.events.begin(), germinated.events.end()) ==
          germinated.events.end(),
      "les evenements germines sont dedupliques");
  for (const GerminatedHigherSupportEvent& event : germinated.events) {
    bool increasing = true;
    for (std::size_t position = 1U; position < event.support_size;
         ++position) {
      increasing = increasing &&
          event.support_ids[position - 1U] < event.support_ids[position];
    }
    require(
        increasing && (event.support_size == 3U || event.support_size == 4U),
        "chaque support accepte est canonique et d'arite trois ou quatre");
  }
}

}  // namespace

int main() {
  // Arity three is exercised by both families; arity four only by
  // eight_clusters, which is why it carries the larger orders.
  check_cell("uniform_latin", uniform_latin_points(20U), 4U);
  check_cell("uniform_latin", uniform_latin_points(28U), 6U);
  check_cell("eight_clusters", eight_clusters_points(24U), 5U);
  check_cell("eight_clusters", eight_clusters_points(32U), 9U);
  check_deduplication_and_containment();

  if (failures != 0) {
    std::cerr << failures << " verification(s) en echec\n";
    return 1;
  }
  std::cout << "differentiel germination/exhaustif vert\n";
  return 0;
}
