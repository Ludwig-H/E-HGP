#include "spindle/predicates.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using mhgp8::Box3;
using mhgp8::Point3;
using u64 = std::uint64_t;

void require(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
}

Box3 join(Box3 a, Box3 b) {
  return {{std::min(a.low.x, b.low.x), std::min(a.low.y, b.low.y),
           std::min(a.low.z, b.low.z)},
          {std::max(a.high.x, b.high.x), std::max(a.high.y, b.high.y),
           std::max(a.high.z, b.high.z)}};
}

// Independent exact midpoint test, not the product H implementation.
bool interior(Point3 a, Point3 b, Point3 z) {
  std::int64_t value = 0;
  for (unsigned axis = 0; axis < 3; ++axis) {
    const auto d = std::int64_t(b[axis]) - a[axis];
    const auto t = 2 * std::int64_t(z[axis]) - a[axis] - b[axis];
    value += d * d - t * t;
  }
  return value > 0;
}

struct Counts {
  u64 descriptors{}, candidates{}, rejected_pairs{}, certificates{}, fallbacks{};
  u64 witness_visits{}, cache_point_visits{}, oracle_tests{}, survivors{};
};

void run(unsigned m, unsigned h, unsigned separation, bool perturb) {
  unsigned width = 1;
  while (width * width < m) ++width;
  const unsigned rows = (m + width - 1) / width;
  const auto length = [=](unsigned row) { return std::min(width, m - row * width); };
  std::vector<Point3> points;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned id = 0; id < m; ++id)
      points.push_back({static_cast<std::uint16_t>(1000 + 59000 * side),
                        static_cast<std::uint16_t>(1000 + id / width),
                        static_cast<std::uint16_t>(1000 + id % width)});
  if (perturb) {
    // Preserve distinct sites and x separation; deliberately destroy the
    // rank/geometry alignment. Certificates must then keep unsafe proposals.
    for (unsigned side = 0; side < 2; ++side)
      for (unsigned id = 0; id < m; ++id) {
        points[side * m + id].y = static_cast<std::uint16_t>(1000 + (id * 7) % m);
        points[side * m + id].z = static_cast<std::uint16_t>(1000 + (id * 11 + side * 3) % m);
      }
  }
  auto a_box = mhgp8::singleton_box(points[0]);
  auto b_box = mhgp8::singleton_box(points[m]);
  for (unsigned i = 1; i < m; ++i) {
    a_box = join(a_box, mhgp8::singleton_box(points[i]));
    b_box = join(b_box, mhgp8::singleton_box(points[m + i]));
  }
  for (const auto box : {a_box, b_box}) {
    u64 diameter2 = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
      const auto d = u64(box.high[axis]) - box.low[axis];
      diameter2 += d * d;
    }
    require(u64(59000) * 59000 >= u64(separation) * separation * diameter2,
            "fixture separation");
  }
  // Actual geometry caches, not synthetic boxes inferred from the ranks.
  // Global tails and row tails are each queried in constant time.
  std::vector<Box3> prefix(m), suffix(m), row_prefix(m), row_suffix(m);
  Counts counts;
  for (unsigned i = 0; i < m; ++i) {
    const auto box = mhgp8::singleton_box(points[m + i]);
    prefix[i] = i == 0 ? box : join(prefix[i - 1], box);
    row_prefix[i] = i % width == 0 ? box : join(row_prefix[i - 1], box);
    ++counts.cache_point_visits;
  }
  for (unsigned i = m; i-- > 0;) {
    const auto box = mhgp8::singleton_box(points[m + i]);
    suffix[i] = i + 1 == m ? box : join(suffix[i + 1], box);
    row_suffix[i] = i + 1 == m || (i + 1) % width == 0
        ? box : join(row_suffix[i + 1], box);
    ++counts.cache_point_visits;
  }
  const bool exhaustive = m <= 64;
  const unsigned w = (h + 1) / 2;
  mhgp8::PredicateWork work;
  for (unsigned a = 0; a < m; ++a) {
    const unsigned i = a / width, j = a % width;
    std::vector<unsigned char> cover(exhaustive ? m : 0, 0);
    std::vector<bool> kept(exhaustive ? m : 0, false);
    const auto emit = [&](unsigned first, unsigned last, Box3 box,
                          const std::vector<unsigned>& witnesses) {
      if (first == last) return;
      ++counts.descriptors;
      bool reject = false;
      if (!witnesses.empty()) {
        auto ids = witnesses;
        std::sort(ids.begin(), ids.end());
        require(std::adjacent_find(ids.begin(), ids.end()) == ids.end(), "duplicate witness ID");
        auto zbox = mhgp8::singleton_box(points.at(ids.front()));
        for (auto z : ids) {
          require(z != a && !(z >= m + first && z < m + last), "endpoint witness ID");
          zbox = join(zbox, mhgp8::singleton_box(points.at(z)));
          ++counts.witness_visits;
        }
        const auto decision = mhgp8::classify_witness_block(
            mhgp8::Lane::Q2, mhgp8::singleton_box(points[a]), box, zbox, work);
        reject = ids.size() >= h && decision == mhgp8::BlockDecision::Credit;
        if (reject) ++counts.certificates;
        else ++counts.fallbacks;
      }
      if (reject) counts.rejected_pairs += last - first;
      else counts.candidates += last - first;
      if (exhaustive) {
        for (unsigned b = first; b < last; ++b) {
          ++cover[b];
          kept[b] = !reject;
          if (reject)
            for (auto z : witnesses) {
              ++counts.oracle_tests;
              require(interior(points[a], points[m + b], points[z]), "unsafe geometric rejection");
            }
        }
      }
    };
    const unsigned low_row = i > w ? i - w : 0;
    const unsigned high_row = std::min(rows, i + w + 1);
    if (low_row > 0) {
      std::vector<unsigned> ids;
      for (unsigned t = 1; t <= w; ++t)
        for (unsigned side = 0; side < 2; ++side)
          ids.push_back(side * m + (i - t) * width + j);
      emit(0, low_row * width, prefix[low_row * width - 1], ids);
    }
    if (high_row < rows) {
      std::vector<unsigned> ids;
      for (unsigned t = 1; t <= w; ++t)
        for (unsigned side = 0; side < 2; ++side)
          ids.push_back(side * m + (i + t) * width + j);
      emit(high_row * width, m, suffix[high_row * width], ids);
    }
    for (unsigned u = low_row; u < high_row; ++u) {
      const bool two_rows = (length(i) == width && length(u) == width) || i == u;
      const unsigned span = two_rows ? w : h;
      const unsigned low_z = std::min(length(u), j > span ? j - span : 0);
      const unsigned high_z = std::min(length(u), j + span + 1);
      const auto ids_for = [&](bool right) {
        std::vector<unsigned> ids;
        for (unsigned t = 1; t <= span; ++t) {
          const unsigned z = right ? j + t : j - t;
          if (two_rows) {
            ids.push_back(i * width + z);
            ids.push_back(m + u * width + z);
          } else if (length(i) == width) {
            ids.push_back(i * width + z);
          } else {
            ids.push_back(m + u * width + z);
          }
        }
        return ids;
      };
      const unsigned offset = u * width;
      if (low_z > 0) emit(offset, offset + low_z, row_prefix[offset + low_z - 1], ids_for(false));
      emit(offset + low_z, offset + high_z, {}, {});
      if (high_z < length(u)) emit(offset + high_z, offset + length(u), row_suffix[offset + high_z], ids_for(true));
    }
    if (exhaustive) {
      for (unsigned b = 0; b < m; ++b) {
        require(cover[b] == 1, "nonpartitioning descriptors");
        unsigned depth = 0;
        for (unsigned z = 0; z < 2 * m; ++z) {
          ++counts.oracle_tests;
          depth += interior(points[a], points[m + b], points[z]);
        }
        if (depth < h) {
          ++counts.survivors;
          require(kept[b], "lost q2 survivor");
        }
      }
    }
  }
  require(counts.candidates + counts.rejected_pairs == u64(m) * m, "pair accounting");
  require(counts.cache_point_visits == 2 * m, "cache cost");
  if (!perturb) {
    require(counts.fallbacks == 0, "aligned certificate failure");
    require(counts.candidates <= u64(m) * (2 * w + 1) * (2 * h + 1), "sheet residual bound");
  } else require(counts.fallbacks > 0, "perturbed fallback nonvacuity");
  require(counts.descriptors <= u64(m) * (2 + 3 * (2 * w + 1)), "descriptor cost bound");
  std::cout << "{\"n\":" << 2 * m << ",\"h\":" << h << ",\"s\":" << separation
            << ",\"perturbed\":" << (perturb ? "true" : "false")
            << ",\"exhaustive\":" << (exhaustive ? "true" : "false")
            << ",\"total_pairs\":" << u64(m) * m << ",\"candidates\":" << counts.candidates
            << ",\"descriptors\":" << counts.descriptors << ",\"certificates\":" << counts.certificates
            << ",\"fallbacks\":" << counts.fallbacks << ",\"witness_visits\":" << counts.witness_visits
            << ",\"cache_point_visits\":" << counts.cache_point_visits
            << ",\"oracle_tests\":" << counts.oracle_tests << ",\"q2_survivors\":";
  if (exhaustive) std::cout << counts.survivors;
  else std::cout << "null";
  std::cout << "}\n";
}
}  // namespace

int main() {
  try {
    for (unsigned m : {1U, 2U, 7U, 16U, 20U, 36U, 64U})
      for (unsigned h : {1U, 5U, 10U}) run(m, h, 12, false);
    for (unsigned h : {1U, 5U, 10U}) run(64, h, 12, true);
    for (unsigned m : {4000U, 8000U, 16000U})
      for (unsigned h : {5U, 10U})
        for (unsigned s : {8U, 10U, 12U}) run(m, h, s, false);
  } catch (const std::exception& error) {
    std::cerr << "sheet rectangles probe failed: " << error.what() << '\n';
    return 1;
  }
}
