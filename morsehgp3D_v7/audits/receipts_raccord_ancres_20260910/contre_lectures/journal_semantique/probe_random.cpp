// Juge differentiel aleatoire : journaux valides generes, rejeu avant independant
// (ensembles explicites, comparaison rationnelle i128), confrontation au lecteur C++.
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "src/forest/full_coverage_certificate.hpp"

using namespace mhgp7;

struct Rat { long long num, den; };  // den > 0, valeurs petites
static int cmp_rat(Rat a, Rat b) {
  const long long l = a.num * b.den, r = b.num * a.den;  // valeurs < 2^20, pas de debordement
  return l < r ? -1 : (l > r ? 1 : 0);
}
static ExactLevel to_level(Rat r) { return {{(u64)r.num, 0, 0}, (i128)r.den}; }

struct Journal {
  std::vector<PointId> domain;
  std::vector<FullCoveragePopulation> rows;
  std::vector<FullCoverageBatch> batches;
  std::vector<Rat> levels;  // par lot, valeur exacte
};

static long long failures = 0, journals = 0, cuts = 0, comparisons = 0, live_nodes = 0, nonempty_reads = 0;
static long long continuation_contribs = 0, fusion_contribs = 0, births = 0, fusions = 0, continuations = 0;

static std::set<PointId> expand(const FullCoveragePopulation& row, const FullCoverageRef& ref) {
  std::set<PointId> s;
  if (ref.include_interior) s.insert(row.interior.begin(), row.interior.end());
  for (size_t j = 0; j < row.shell.size(); ++j) if (ref.shell_mask & (1u << j)) s.insert(row.shell[j]);
  return s;
}

// Rejeu independant : etat des racines a une coupe (niveau < cut, ou <= si fermee).
static std::map<FullNodeId, std::set<PointId>> replay(const Journal& j, Rat cut, bool closed) {
  std::map<FullNodeId, std::set<PointId>> roots;
  FullNodeId next = 0;
  for (size_t b = 0; b < j.batches.size(); ++b) {
    const int c = cmp_rat(j.levels[b], cut);
    if (c > 0 || (c == 0 && !closed)) break;
    for (const auto& action : j.batches[b].actions) {
      std::set<PointId> pts;
      for (auto p : action.parents) { pts.insert(roots.at(p).begin(), roots.at(p).end()); roots.erase(p); }
      for (const auto& ref : action.contributions) {
        auto e = expand(j.rows[ref.population], ref);
        pts.insert(e.begin(), e.end());
      }
      const FullNodeId id = action.parents.size() == 1 ? action.parents.front() : next++;
      roots[id] = std::move(pts);
    }
  }
  return roots;
}

static Journal generate(std::mt19937_64& rng, unsigned order) {
  Journal j;
  const size_t n = 4 + rng() % 9;  // 4..12 points, identifiants non denses
  PointId p = rng() % 5;
  for (size_t i = 0; i < n; ++i) { j.domain.push_back(p); p += 1 + rng() % 4; }
  // populations : I et U disjoints, |U| <= 6 ; au moins une ligne de cardinal >= order
  const size_t nrows = 3 + rng() % 6;
  for (size_t r = 0; r < nrows; ++r) {
    FullCoveragePopulation row;
    std::vector<PointId> perm = j.domain;
    std::shuffle(perm.begin(), perm.end(), rng);
    const size_t u = std::min<size_t>(1 + rng() % 6, perm.size());
    const size_t in = rng() % (perm.size() - u + 1);
    row.shell.assign(perm.begin(), perm.begin() + u);
    row.interior.assign(perm.begin() + u, perm.begin() + u + in);
    std::sort(row.shell.begin(), row.shell.end()); std::sort(row.interior.begin(), row.interior.end());
    if (r == 0) {  // garantir une ligne de naissance possible
      while (row.interior.size() + row.shell.size() < order) {
        // ajouter un point manquant a l'interieur
        for (PointId q : j.domain) {
          if (!std::binary_search(row.shell.begin(), row.shell.end(), q) &&
              !std::binary_search(row.interior.begin(), row.interior.end(), q)) {
            row.interior.push_back(q); std::sort(row.interior.begin(), row.interior.end()); break;
          }
        }
      }
    }
    j.rows.push_back(std::move(row));
  }
  // niveaux : fractions distinctes croissantes, representation non reduite aleatoire
  std::set<std::pair<long long, long long>> seen;
  std::vector<Rat> values;
  const size_t nb = 2 + rng() % 7;
  while (values.size() < nb) {
    long long num = 1 + rng() % 40, den = 1 + rng() % 6;
    const long long g = std::gcd(num, den); num /= g; den /= g;
    if (seen.insert({num, den}).second) values.push_back({num, den});
  }
  std::sort(values.begin(), values.end(), [](Rat a, Rat b) { return cmp_rat(a, b) < 0; });
  for (auto& v : values) { const long long k = 1 + rng() % 3; v.num *= k; v.den *= k; }
  std::vector<FullNodeId> live;
  FullNodeId next = 0;
  for (size_t b = 0; b < nb; ++b) {
    FullCoverageBatch batch; batch.level = to_level(values[b]);
    std::vector<FullNodeId> avail = live; std::shuffle(avail.begin(), avail.end(), rng);
    const size_t nactions = 1 + rng() % 3;
    for (size_t a = 0; a < nactions; ++a) {
      FullCoverageAction action;
      const unsigned kind = rng() % 3;  // 0 naissance, 1 continuation, 2 fusion
      if (kind == 2 && avail.size() >= 2) {
        const size_t np = std::min<size_t>(avail.size(), 2 + rng() % 3);
        action.parents.assign(avail.end() - np, avail.end()); avail.resize(avail.size() - np);
        std::sort(action.parents.begin(), action.parents.end());
        const size_t nc = rng() % 3;
        for (size_t c = 0; c < nc; ++c) {
          const u64 row = rng() % j.rows.size();
          const auto& R = j.rows[row];
          const u16 all = (u16)((1u << R.shell.size()) - 1);
          u16 mask = (u16)(rng() & all); bool inter = !R.interior.empty() && (rng() & 1);
          if (!inter && mask == 0) mask = all;
          action.contributions.push_back({row, mask, inter});
        }
        ++fusions; fusion_contribs += nc;
      } else if (kind == 1 && !avail.empty()) {
        action.parents = {avail.back()}; avail.pop_back();
        const size_t nc = 1 + rng() % 2;
        for (size_t c = 0; c < nc; ++c) {
          const u64 row = rng() % j.rows.size();
          const auto& R = j.rows[row];
          const u16 all = (u16)((1u << R.shell.size()) - 1);
          u16 mask = (u16)(rng() & all); bool inter = !R.interior.empty() && (rng() & 1);
          if (!inter && mask == 0) mask = all;
          action.contributions.push_back({row, mask, inter});
        }
        ++continuations; continuation_contribs += nc;
      } else {
        // naissance : ligne de cardinal >= order
        u64 row; int guard = 0;
        do { row = rng() % j.rows.size(); } while (j.rows[row].interior.size() + j.rows[row].shell.size() < order && ++guard < 100);
        if (j.rows[row].interior.size() + j.rows[row].shell.size() < order) row = 0;
        const auto& R = j.rows[row];
        action.contributions.push_back({row, (u16)((1u << R.shell.size()) - 1), !R.interior.empty()});
        ++births;
      }
      batch.actions.push_back(std::move(action));
    }
    // mise a jour des racines vivantes (identifiants denses hors continuations)
    std::vector<FullNodeId> after = avail;  // non touches
    for (const auto& action : batch.actions) {
      if (action.parents.size() == 1) after.push_back(action.parents.front());
      else after.push_back(next++);
    }
    live = after;
    j.batches.push_back(std::move(batch));
    j.levels.push_back(values[b]);
  }
  return j;
}

int main(int argc, char** argv) {
  const unsigned long long seed = argc > 1 ? std::stoull(argv[1]) : 3;
  const long long count = argc > 2 ? std::stoll(argv[2]) : 3000;
  std::mt19937_64 rng(seed);
  for (long long it = 0; it < count; ++it) {
    const unsigned order = 2 + rng() % 3;
    Journal j = generate(rng, order);
    auto bank = build_full_coverage_populations(j.domain, j.rows);
    if (bank.status != FullCertificateStatus::kOk) { std::printf("BANK refusee it=%lld (%s)\n", it, bank.reason); ++failures; continue; }
    auto built = build_full_coverage_certificate(order, bank.value, j.batches);
    if (built.status != FullCertificateStatus::kOk) { std::printf("BUILD refuse it=%lld (%s)\n", it, built.reason); ++failures; continue; }
    ++journals;
    const auto& f = built.value;
    // coupes : chaque niveau de lot (representation non reduite ET reduite), entre deux niveaux, avant, apres
    std::vector<Rat> cutset;
    for (auto v : j.levels) {
      cutset.push_back(v);
      const long long g = std::gcd(v.num, v.den); cutset.push_back({v.num / g, v.den / g});
      cutset.push_back({v.num * 2 + 1, v.den * 2});  // legerement au-dessus
      cutset.push_back({v.num * 2 - 1, v.den * 2});  // legerement en dessous
    }
    cutset.push_back({0, 1}); cutset.push_back({1000, 1});
    for (auto cut : cutset) for (bool closed : {false, true}) {
      auto expected = replay(j, cut, closed);
      const auto lvl = to_level(cut);
      ++cuts;
      for (FullNodeId id = 0; id < f.nodes().size(); ++id) {
        const bool live = full_coverage_root_at(f, id, lvl, closed) == id;
        const bool exp_live = expected.count(id) != 0;
        ++comparisons;
        if (live != exp_live) {
          ++failures; std::printf("DIFF vivacite it=%lld id=%llu cut=%lld/%lld closed=%d cpp=%d attendu=%d\n", it,
              (unsigned long long)id, cut.num, cut.den, closed, live, exp_live); continue;
        }
        auto read = full_coverage_at(f, id, lvl, closed);
        if (!live) {
          if (read.status != FullCertificateStatus::kInvalidInput || !read.values.empty()) {
            ++failures; std::printf("DIFF lecture non racine acceptee it=%lld id=%llu\n", it, (unsigned long long)id);
          }
          continue;
        }
        ++live_nodes;
        std::vector<PointId> exp(expected.at(id).begin(), expected.at(id).end());
        if (!exp.empty()) ++nonempty_reads;
        if (read.status != FullCertificateStatus::kOk || read.values != exp) {
          ++failures;
          std::printf("DIFF couverture it=%lld id=%llu cut=%lld/%lld closed=%d\n", it, (unsigned long long)id, cut.num, cut.den, closed);
        }
      }
    }
  }
  std::printf("seed=%llu journaux=%lld coupes=%lld comparaisons=%lld racines_vivantes=%lld lectures_non_vides=%lld "
              "naissances=%lld continuations=%lld(+%lld contrib) fusions=%lld(+%lld contrib) ECARTS=%lld\n",
              seed, journals, cuts, comparisons, live_nodes, nonempty_reads, births, continuations, continuation_contribs,
              fusions, fusion_contribs, failures);
  return failures ? 1 : 0;
}
