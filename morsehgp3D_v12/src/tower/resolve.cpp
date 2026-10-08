// Resolution d'un representant (CONTRAT_TOUR.md, paragraphe 4.1 ; resolve_v12 de reference/hgp12_ref/constructive.py,
// politique v12_indices). Fonction pure du domaine immuable : elle ne lit que le catalogue, l'index, la table de
// populations de l'ordre et la table des cellules de fenetre ; elle n'ecrit que ses compteurs. Chaque pas est un pas
// valide du theoreme D (L02) : une k-partie de I quand p >= k (les k plus petits SiteIdx, catalogue ou census sature) ;
// I u (les t plus petits SiteIdx de U) sous la fenetre (t <= q_min - 2 : partie separable). Le niveau decroit
// strictement a chaque pas, CONTROLE avant toute sortie par saut (census sature compris) et des la premiere plus petite
// boule ou la premiere sonde, contre le rang de la cellule de la jonction : la boucle termine sans plafond.
#include <algorithm>

#include "tower/internal.hpp"
#include "tower/profile.hpp"
#include "tower/proposal.hpp"

namespace mhgp12::tower_detail {
namespace {

enum Route : u8 { kT1, kCertTable, kCertCensus, kFallbackTable, kFallbackCensus };

// Boule precedente de la chaine : rang du catalogue, ou niveau exact d'une sphere certifiee hors du catalogue.
struct Previous {
  bool exact = false;
  LevelRank rank{};
  num::Level level{};
};

Outcome control_rank(const Catalogue& cat, Previous& prev, LevelRank rank, OrderCounters& n, Order k) noexcept {
  ++n.controls;
  const bool lower = prev.exact ? num::compare(cat.levels()[idx(rank)], prev.level) < 0 : idx(rank) < idx(prev.rank);
  if (!lower) return fail(Reason::tower_invariant, k);  // decroissance stricte violee
  prev.exact = false;
  prev.rank = rank;
  return {};
}

Outcome control_level(const Catalogue& cat, Previous& prev, const num::Level& level, OrderCounters& n,
                      Order k) noexcept {
  ++n.controls;
  const num::Level& before = prev.exact ? prev.level : cat.levels()[idx(prev.rank)];
  if (num::compare(level, before) >= 0) return fail(Reason::tower_invariant, k);
  prev.exact = true;
  prev.level = level;
  return {};
}

bool sorted_subset(std::span<const u32> s, const Part& f) noexcept {
  u32 j = 0;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (i > 0 && s[i] <= s[i - 1]) return false;  // multiensemble : aucune repetition
    while (j < f.k && f.id[j] < s[i]) ++j;
    if (j == f.k || f.id[j] != s[i]) return false;
    ++j;
  }
  return true;
}

// F dans P_b = I u U : la ligne de population de b est exactement I puis U ; elle est lue par les decalages du
// catalogue et balayee (T2-d-B3), sans relire la fiche de la boule (p et m ne servaient qu'a la couper en deux).
bool part_in_population(const Catalogue& cat, const Part& f, u32 b) noexcept {
  const auto offsets = cat.population_offsets();
  const SiteIdx* row = cat.population().data() + offsets[b];
  const u64 n = offsets[u64{b} + 1] - offsets[b];
  for (u32 i = 0; i < f.k; ++i) {
    bool found = false;
    for (u64 j = 0; j < n && !found; ++j) found = idx(row[j]) == f.id[i];
    if (!found) return false;
  }
  return true;
}

// I u (les take premiers sites de U), fusion croissante ; I et U croissants et disjoints.
Part inner_and_shell(std::span<const SiteIdx> inner, std::span<const SiteIdx> shell, u32 take) noexcept {
  Part f;
  std::size_t i = 0, j = 0;
  while (i < inner.size() || j < take) {
    const bool from_inner = j == take || (i < inner.size() && idx(inner[i]) < idx(shell[j]));
    f.id[f.k++] = idx(from_inner ? inner[i++] : shell[j++]);
  }
  return f;
}

void record_chain(OrderCounters& n, u64 chain) noexcept {
  ++n.chain_histogram[std::min<u64>(chain, kChainBins - 1)];
  n.max_chain = std::max(n.max_chain, chain);
}

struct Proposal {
  bool ok = false;
  std::array<u32, 4> s{};
  u32 q = 0;
};

// DWelzl dans le repere local de la partie (translation par son premier site) ; ne decide rien.
Proposal propose(const Domain& d, const Part& f) noexcept {
  DWelzl w;
  const auto& origin = d.points[f.id[0]].coordinates();
  for (u32 i = 0; i < f.k; ++i)
    for (int a = 0; a < 3; ++a)
      w.p[i][a] = static_cast<double>(d.points[f.id[i]].coordinates()[a]) - static_cast<double>(origin[a]);
  const DBall ball = w.run(static_cast<int>(f.k));
  Proposal out;
  if (!w.ok || ball.nr < 2 || ball.nr > 4) return out;
  out.ok = true;
  out.q = static_cast<u32>(ball.nr);
  for (int i = 0; i < ball.nr; ++i) out.s[i] = f.id[ball.R[i]];
  std::sort(out.s.begin(), out.s.begin() + ball.nr);
  return out;
}

// Plus petite boule de F : boule du catalogue (ball), ou boule certifiee hors de la table (certified) avec son support
// canonique parmi les sites de F sur la sphere (temoins du census, T2-d : ces sites sont SUR la sphere).
struct Located {
  u32 ball = kNone;
  std::optional<num::CertifiedBall> certified;
  std::array<SiteIdx, 4> support{};
  u8 arity = 0;
  Route route = kT1;
};

Result<Located> locate(const Domain& d, const Part& f, OrderCounters& n, SectionClock& clock) noexcept {
  const auto& cat = d.catalogue;
  Located out;
  const Proposal prop = propose(d, f);
  const bool in_part = prop.ok && sorted_subset(std::span<const u32>(prop.s.data(), prop.q), f);
  clock.lap(kProfileProposal);
  bool table_miss = false;  // la table a repondu << absent >> pour le support propose
  const auto t1 = in_part ? lem_t1(d, f, std::span<const u32>(prop.s.data(), prop.q), &table_miss) : std::nullopt;
  clock.lap(kProfileT1);
  std::optional<Certified> cert;
  if (!prop.ok) ++n.fallback_no_proposal;
  else if (!in_part) ++n.fallback_not_in_part;
  else {
    if (t1) {
      ++n.route_t1;
      out.ball = *t1;
      return out;
    }
    auto made = certify_part(d, f, std::span<const u32>(prop.s.data(), prop.q));
    if (!made.ok()) return made.outcome();
    cert = made.value();
    if (!cert) ++n.fallback_certificate;
  }
  const bool fallback = !cert.has_value();
  if (fallback) {  // repli exact : un support strict de la plus petite boule, puis le meme certificat
    if (in_part) clock.lap(kProfileCertificate);  // certificat du support propose, en echec
    std::array<u32, 4> support{};
    auto q = exact_support(d, f, support);
    if (!q.ok()) return q.outcome();
    auto made = certify_part(d, f, std::span<const u32>(support.data(), q.value()));
    if (!made.ok()) return made.outcome();
    if (!made.value()) return fail(Reason::tower_invariant);
    cert = made.value();
  }
  std::array<SiteIdx, 4> key{};
  for (u32 i = 0; i < cert->arity; ++i) key[i] = make_id<SiteIdx>(cert->support[i]);
  // T2-d-B2 : support certifie egal au support propose, que la table vient de dire absent : meme requete, meme reponse,
  // sans la relire (en position generale, chaque echec de LEM-T1 mene ici au census ; sur ng00 K5, 252 152 recherches
  // par passe). Un support canonique different (sites de F cosphericaux) est cherche comme avant.
  const bool same_key = table_miss && !fallback && cert->arity == prop.q &&
                        std::equal(cert->support.begin(), cert->support.begin() + cert->arity, prop.s.begin());
  const auto b =
      same_key ? std::optional<BallIdx>{} : cat.find_support(std::span<const SiteIdx>(key.data(), cert->arity));
  if (b && !part_in_population(cat, f, idx(*b))) return fail(Reason::tower_invariant);  // meme sphere : F dans P_b
  clock.lap(fallback ? kProfileFallback : kProfileCertificate);
  if (b) {
    ++(fallback ? n.route_fallback_table : n.route_cert_table);
    out.ball = idx(*b);
    out.route = fallback ? kFallbackTable : kCertTable;
    return out;
  }
  ++(fallback ? n.route_fallback_census : n.route_cert_census);
  out.certified.emplace(cert->ball);
  out.support = key;
  out.arity = cert->arity;
  out.route = fallback ? kFallbackCensus : kCertCensus;
  return out;
}

// Census garde de seuil k d'une sphere certifiee (une passe, espace de travail du fil).
struct CensusStep {
  const Domain& domain;
  Order k;
  OrderCounters& counters;
  const num::Sphere& sphere;  // sphere de la boule certifiee recensee
  bool saturated = false, catalogue = false;
  u32 ball = kNone;
  Part next;

  static Outcome consume(void* raw, const BorrowedCensus& census) noexcept {
    auto& s = *static_cast<CensusStep*>(raw);
    // Census a temoins (T2-d) : la racine contient toujours le support, temoin sur la sphere ; aucune decision par
    // temoin signifie que le support n'a pas ete transmis (travail perdu, resultat inchange).
    if (census.ledger().guard_witness == 0) return fail(Reason::tower_invariant, s.k);
    s.counters.census_sites += census.ledger().point_tests;
    s.counters.census_sites_max = std::max(s.counters.census_sites_max, census.ledger().point_tests);
    s.counters.census_nodes += census.ledger().nodes;
    const auto inner = census.interior(), shell = census.shell();
    if (census.kind() == CensusKind::saturated) {
      ++s.counters.census_saturated;
      if (inner.size() != s.k) return fail(Reason::tower_invariant, s.k);
      s.saturated = true;
      s.next = inner_and_shell(inner, shell, 0);  // les k plus petits SiteIdx de I
      return {};
    }
    ++s.counters.census_complete;
    return s.complete(inner, shell);
  }

  Outcome complete(std::span<const SiteIdx> inner, std::span<const SiteIdx> shell) noexcept {
    const auto& cat = domain.catalogue;
    if (shell.size() > kMaxShell) return fail(Reason::shell_capacity, k);
    if (inner.size() >= k || shell.empty()) return fail(Reason::tower_invariant, k);
    std::array<u32, kMaxShell> sites{};
    for (std::size_t j = 0; j < shell.size(); ++j) sites[j] = idx(shell[j]);
    std::array<u32, 4> support{};
    auto q = canonical_support(domain, sphere, std::span<const u32>(sites.data(), shell.size()), support);
    if (!q.ok()) return q.outcome();
    if (q.value() == 0) return fail(Reason::tower_invariant, k);  // le centre est dans l'enveloppe de U
    std::array<SiteIdx, 4> key{};
    for (u32 i = 0; i < q.value(); ++i) key[i] = make_id<SiteIdx>(support[i]);
    if (const auto b = cat.find_support(std::span<const SiteIdx>(key.data(), q.value()))) {
      const auto& data = cat.balls_data()[idx(*b)];
      const auto ci = cat.interior(*b), cu = cat.shell(*b);
      if (data.p != inner.size() || data.m != shell.size() || data.qmin != q.value() ||
          !std::equal(ci.begin(), ci.end(), inner.begin()) || !std::equal(cu.begin(), cu.end(), shell.begin()))
        return fail(Reason::census_mismatch, k);
      catalogue = true;
      ball = idx(*b);
      return {};
    }
    const u32 p = static_cast<u32>(inner.size());
    if (k > p + q.value() - 2) return fail(Reason::catalogue_missing_ball, k);  // (H2) violee
    next = inner_and_shell(inner, shell, k - p);  // pas inerte sous la fenetre
    return {};
  }
};

}  // namespace

// table_miss (T2-d-B2) : vrai si S, dans F, a ete cherche dans la table et n'y est pas (aucune boule de S* = S).
std::optional<u32> lem_t1(const Domain& d, const Part& f, std::span<const u32> support, bool* table_miss) noexcept {
  if (table_miss != nullptr) *table_miss = false;
  if (support.size() < 2 || support.size() > 4 || !sorted_subset(support, f)) return std::nullopt;  // S dans F
  std::array<SiteIdx, 4> key{};
  for (std::size_t i = 0; i < support.size(); ++i) key[i] = make_id<SiteIdx>(support[i]);
  const auto b = d.catalogue.find_support(std::span<const SiteIdx>(key.data(), support.size()));
  if (!b && table_miss != nullptr) *table_miss = true;
  if (!b || !part_in_population(d.catalogue, f, idx(*b))) return std::nullopt;  // S = S*(b) et F dans P_b
  return idx(*b);
}

void add_counters(OrderCounters& total, const OrderCounters& part) noexcept {
  total.births += part.births;
  total.cells += part.cells;
  total.inert_cells += part.inert_cells;
  total.extended_cells += part.extended_cells;
  total.representatives += part.representatives;
  total.probes += part.probes;
  total.first_probe_hits += part.first_probe_hits;
  total.probe_hits_after_steps += part.probe_hits_after_steps;
  total.route_t1 += part.route_t1;
  total.route_cert_table += part.route_cert_table;
  total.route_cert_census += part.route_cert_census;
  total.route_fallback_table += part.route_fallback_table;
  total.route_fallback_census += part.route_fallback_census;
  total.fallback_no_proposal += part.fallback_no_proposal;
  total.fallback_not_in_part += part.fallback_not_in_part;
  total.fallback_certificate += part.fallback_certificate;
  total.census_saturated += part.census_saturated;
  total.census_complete += part.census_complete;
  total.census_sites += part.census_sites;
  total.census_sites_max = std::max(total.census_sites_max, part.census_sites_max);
  total.census_nodes += part.census_nodes;
  total.jumps_catalogue += part.jumps_catalogue;
  total.jumps_census += part.jumps_census;
  total.inert_steps += part.inert_steps;
  total.cell_stops += part.cell_stops;
  total.birth_stops += part.birth_stops;
  total.controls += part.controls;
  total.max_chain = std::max(total.max_chain, part.max_chain);
  for (int i = 0; i < kChainBins; ++i) total.chain_histogram[i] += part.chain_histogram[i];
}

Result<u32> resolve_part(const ResolveContext& c, Part f, LevelRank junction_rank, CensusWorkspace& workspace,
                         OrderCounters& n, SectionClock& clock, const FirstProbe& first) noexcept {
  const Domain& d = c.domain;
  const auto& cat = d.catalogue;
  const Order k = c.order.k;
  if (idx(junction_rank) >= cat.levels().size() || c.order.table == nullptr) return fail(Reason::tower_invariant, k);
  Previous prev;
  prev.rank = junction_rank;  // date initiale : la trace est stricte, beta(F0) < niveau de la cellule
  u64 chain = 0;
  for (;;) {
    ++n.probes;
    const auto birth = chain == 0 && first.done ? first.hit : c.order.table->find(f);  // LEM-POP
    if (birth) {
      MHGP12_TRY(control_rank(cat, prev, birth->rank, n, k));
      ++(chain == 0 ? n.first_probe_hits : n.probe_hits_after_steps);
      record_chain(n, chain);
      clock.lap(kProfileProbe);
      return birth_target(birth->birth);
    }
    clock.lap(kProfileProbe);
    ++chain;
    auto located = locate(d, f, n, clock);
    if (!located.ok()) return fail(located.outcome().reason, k);
    u32 b = located.value().ball;
    if (b != kNone) {
      MHGP12_TRY(control_rank(cat, prev, cat.balls_data()[b].rank, n, k));
    } else {
      const num::CertifiedBall& ball = *located.value().certified;
      MHGP12_TRY(control_level(cat, prev, ball.sphere().level(), n, k));  // AVANT toute sortie par saut
      CensusStep step{d, k, n, ball.sphere(), false, false, kNone, Part{}};
      const std::span<const SiteIdx> witnesses(located.value().support.data(), located.value().arity);
      MHGP12_TRY(workspace.query(d.index, ball, k, witnesses, &step, &CensusStep::consume));  // census a temoins
      clock.lap(step.saturated ? kProfileCensusSaturated : kProfileCensusComplete);
      if (step.saturated) {
        ++n.jumps_census;
        f = step.next;
        continue;
      }
      if (!step.catalogue) {
        ++n.inert_steps;
        f = step.next;
        continue;
      }
      b = step.ball;
    }
    const auto& data = cat.balls_data()[b];
    if (data.p >= u32{k}) {  // saut : les k plus petits SiteIdx de I (politique de la v11)
      ++n.jumps_catalogue;
      f = inner_and_shell(cat.interior(make_id<BallIdx>(b)).first(k), cat.shell(make_id<BallIdx>(b)), 0);
      clock.lap(kProfileStep);
      continue;
    }
    if (u32{k} + 2 <= data.p + data.qmin) {  // sous la fenetre : pas inerte
      ++n.inert_steps;
      f = inner_and_shell(cat.interior(make_id<BallIdx>(b)), cat.shell(make_id<BallIdx>(b)), k - data.p);
      clock.lap(kProfileStep);
      continue;
    }
    const u32 target = window_target_of(c.windows, b, data, k);  // premiere cellule de fenetre : arret
    if (target == kNoTarget) return fail(Reason::tower_invariant, k);
    ++(target_is_cell(target) ? n.cell_stops : n.birth_stops);
    record_chain(n, chain);
    clock.lap(kProfileStop);
    return target;
  }
}

Result<u32> resolve_part(const ResolveContext& c, Part f, LevelRank junction_rank, CensusWorkspace& workspace,
                         OrderCounters& n) noexcept {
  SectionClock clock(nullptr);
  return resolve_part(c, f, junction_rank, workspace, n, clock, FirstProbe{});
}

}  // namespace mhgp12::tower_detail
