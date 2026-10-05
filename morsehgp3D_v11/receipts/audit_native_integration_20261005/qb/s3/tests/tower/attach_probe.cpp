// Sonde du rattachement de W_K (tranche S3), hors produit. Deux modes :
//   requetes (defaut) : sur l'entree standard, des requetes "K budget n" puis n lignes "x y z PointId" (n <= 14) ;
//     pour chacune, une ligne JSON au format du vidage canonique de l'oracle borne S1 (Supports.canonical(k, ids) de
//     reference/hgp11_ref/supports.py, cles triees, sans espace), restreint aux champs de la tranche S3, pour le futur
//     differentiel mhgp11_tower_attach_fraction (cable apres l'integration du contrat L0) :
//       format "hgp11_attach_probe", version 1, k, n ; sites : coordonnees en ordre lexicographique (pas Morton) ;
//       ids : PointId dans le meme ordre ;
//       nodes[v] (numerotation canonique de la foret d'ordre K) : level (fraction reduite ecrite comme
//       str(fractions.Fraction) : "a/b", ou "a"), parent (null a la racine), children, kind (0 feuille-site a K = 1,
//       1 naissance, 2 fusion), post (rang de postordre, enfants par numero croissant), balls (indices des boules
//       propres), birth_center (centre exact de la boule de naissance, le site a K = 1 ; null pour une fusion) ;
//       balls, ordre (postordre du noeud, niveau, centre exact) : node (att(b), coupe FERMEE), level, center, role
//       (naissance, fusion, interne), p, m, qmin, components (|ant(b)|), prior (ant(b), coupe OUVERTE, role fusion
//       seulement), strict_traces (journal), et s_star (S* en coordonnees, ordre des SiteIdx : seul champ propre a la
//       sonde, que le differentiel ignore).
//     Option --workers=W (W > 1) : voie par lots sur un Pool de W fils (memes sorties attendues).
//   --incidences=<sortie> --input=<xyz.u32le>,<ids.u32le> --k=K [--workers=W] : ecrit, en mots u64 petit-boutistes,
//     le bloc de foret (k, N, naissances, racine, N x (parent, rang)) puis le bloc d'incidences fortes
//     (T, decalages n+1, T mots rang << 32 | noeud, groupes par site et tries) tires de WindowAttachment, aux memes
//     conventions que l'ordre K de MHGP11PH (bench/points_export.cpp, write_order sans le bloc core) : porte
//     mhgp11_tower_attach_export.
// Codes : 0 succes, 2 usage ou refus, 3 invariant (exit_code du refus).
#include <algorithm>
#include <bit>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "whole_input.hpp"
#include "sched/sched.hpp"
#include "tower/order_tree.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace mhgp11::tower_detail;

namespace {
// Entier naturel en mots de 32 bits (petit-boutiste, sans zero de tete) : reduction exacte et ecriture decimale des
// niveaux, dont le numerateur depasse 128 bits (8B+12 bits au profil B). Outil de sonde, jamais dans le produit.
struct Natural {
  std::vector<u32> w;
  bool zero() const { return w.empty(); }
  void trim() { while (!w.empty() && w.back() == 0) w.pop_back(); }
  u64 bits() const { return w.empty() ? 0 : 32 * (w.size() - 1) + (32 - std::countl_zero(w.back())); }
  bool bit(u64 i) const { return ((w[i / 32] >> (i % 32)) & 1u) != 0; }
};
Natural natural(u128 value) {
  Natural out;
  for (; value != 0; value >>= 32) out.w.push_back(static_cast<u32>(value));
  return out;
}
template <int Words>
Natural natural(const num::Wide<Words>& value) {  // magnitude ; le signe est lu a part
  Natural out;
  for (u64 word : value.words) { out.w.push_back(static_cast<u32>(word)); out.w.push_back(static_cast<u32>(word >> 32)); }
  out.trim();
  return out;
}
int compare(const Natural& a, const Natural& b) {
  if (a.w.size() != b.w.size()) return a.w.size() < b.w.size() ? -1 : 1;
  for (std::size_t i = a.w.size(); i-- > 0;) if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}
void subtract(Natural& a, const Natural& b) {  // a >= b
  u64 borrow = 0;
  for (std::size_t i = 0; i < a.w.size(); ++i) {
    const u64 take = u64{i < b.w.size() ? b.w[i] : 0u} + borrow;
    borrow = a.w[i] < take ? 1 : 0;
    a.w[i] = static_cast<u32>((u64{a.w[i]} + (borrow << 32)) - take);
  }
  a.trim();
}
void shift_in(Natural& r, bool bit) {  // r = 2r + bit
  u32 carry = bit ? 1u : 0u;
  for (u32& x : r.w) { const u32 next = x >> 31; x = (x << 1) | carry; carry = next; }
  if (carry != 0) r.w.push_back(carry);
}
std::pair<Natural, Natural> divide(const Natural& a, const Natural& b) {  // b non nul ; decalages et soustractions
  Natural q, r;
  q.w.assign(a.w.size(), 0);
  for (u64 i = a.bits(); i-- > 0;) {
    shift_in(r, a.bit(i));
    if (compare(r, b) >= 0) { subtract(r, b); q.w[i / 32] |= u32{1} << (i % 32); }
  }
  q.trim();
  return {std::move(q), std::move(r)};
}
Natural gcd(Natural a, Natural b) {
  while (!b.zero()) { Natural r = divide(a, b).second; a = std::move(b); b = std::move(r); }
  return a;
}
std::string decimal(Natural a) {
  if (a.zero()) return "0";
  std::string out;
  while (!a.zero()) {
    u64 rest = 0;
    for (std::size_t i = a.w.size(); i-- > 0;) {
      const u64 current = (rest << 32) | a.w[i];
      a.w[i] = static_cast<u32>(current / 10); rest = current % 10;
    }
    a.trim();
    out.push_back(static_cast<char>('0' + rest));
  }
  std::reverse(out.begin(), out.end());
  return out;
}
// Fraction reduite ecrite comme str(fractions.Fraction) : "a/b", "a" si b = 1, signe en tete ; den non nul.
std::string fraction(bool negative, const Natural& num, const Natural& den) {
  if (num.zero()) return "0";
  const Natural g = gcd(num, den);
  const std::string n = decimal(divide(num, g).first), d = decimal(divide(den, g).first);
  return (negative ? "-" : "") + (d == "1" ? n : n + "/" + d);
}
std::string level_text(const num::Level& level) {
  const auto n = num::to_wide(level.numerator());  // largeurs differentes selon le profil (u24 : 4 et 3 mots)
  const auto d = num::to_wide(level.denominator());
  return fraction(n.sign() < 0, natural(n), natural(d));
}
// Coordonnees exactes du centre a + N/D (|a_i D + N_i| < 2^(5B+6) <= 2^126 : i128, comme num::compare_centers).
std::array<std::string, 3> center_text(const num::Sphere& sphere) {
  std::array<std::string, 3> out;
  const i128 den = sphere.denominator();
  for (u32 axis = 0; axis < 3; ++axis) {
    const i128 value = i128{sphere.anchor().coordinates()[axis]} * den + sphere.numerator()[axis];
    const u128 magnitude = value < 0 ? u128{0} - static_cast<u128>(value) : static_cast<u128>(value);
    out[axis] = fraction(value < 0, natural(magnitude), natural(static_cast<u128>(den)));
  }
  return out;
}
const char* role_name(BallRole role) {
  return role == BallRole::birth ? "naissance" : role == BallRole::merge ? "fusion" : "interne";
}

FullParams params_for(u32 workers) {
  FullParams p;
  if (workers > 1) {
    p.regular_batch_capacity = 4096; p.descent_lanes = 48;
    p.reuse_census_workspace = p.dense_birth_lookup = p.population_lookup = true;
  }
  return p;
}

Result<num::Point> point_of(const Cloud& cloud, SiteIdx s) {
  return num::Point::make(cloud.x()[idx(s)], cloud.y()[idx(s)], cloud.z()[idx(s)]);
}
Result<num::Sphere> ball_sphere(const Cloud& cloud, const CatalogueBall& ball) {
  std::array<num::Point, 4> points{};
  for (u32 j = 0; j < ball.qmin; ++j) {
    auto p = point_of(cloud, ball.support[j]);
    if (!p.ok()) return p.outcome();
    points[j] = p.value();
  }
  auto made = ball.qmin == 2 ? num::Sphere::through(points[0], points[1]) : ball.qmin == 3 ?
      num::Sphere::through(points[0], points[1], points[2]) :
      num::Sphere::through(points[0], points[1], points[2], points[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::tower_invariant);
  return *made.value();
}
std::string xyz(const Cloud& cloud, SiteIdx s) {
  return "[" + std::to_string(cloud.x()[idx(s)]) + "," + std::to_string(cloud.y()[idx(s)]) + "," +
         std::to_string(cloud.z()[idx(s)]) + "]";
}
std::string strings(const std::array<std::string, 3>& values) {
  return "[\"" + values[0] + "\",\"" + values[1] + "\",\"" + values[2] + "\"]";
}
template <class Range>
std::string list(const Range& values) {
  std::string out = "[";
  for (const auto& v : values) { if (out.size() > 1) out += ','; out += std::to_string(v); }
  return out + "]";
}

// Postordre iteratif depuis la racine, enfants par numero croissant (meme convention que l'oracle et SP).
std::vector<u32> postorder(const OrderForest& forest) {
  std::vector<u32> post(forest.nodes().size(), kNone);
  std::vector<std::pair<u32, bool>> stack{{idx(forest.root()), false}};
  u32 rank = 0;
  while (!stack.empty()) {
    const auto [v, done] = stack.back();
    stack.pop_back();
    if (done) { post[v] = rank++; continue; }
    stack.push_back({v, true});
    const auto kids = forest.children(NodeIdx{v});
    for (std::size_t j = kids.size(); j-- > 0;) stack.push_back({idx(kids[j]), false});
  }
  return post;
}

Outcome json(std::ostream& out, const OrderTree& tree) {
  const auto& domain = tree.domain();
  const auto& cloud = domain.index().cloud();
  const auto& cat = domain.catalogue();
  const auto& forest = tree.forest();
  const auto& a = tree.attachment();
  const u32 k = tree.order(), n = cloud.sites();
  // Sites en ordre lexicographique des coordonnees, PointId dans le meme ordre (poids un : un identifiant par site).
  std::vector<u32> lex(n);
  for (u32 s = 0; s < n; ++s) lex[s] = s;
  std::sort(lex.begin(), lex.end(), [&](u32 l, u32 r) {
    return std::array<u32, 3>{cloud.x()[l], cloud.y()[l], cloud.z()[l]} <
           std::array<u32, 3>{cloud.x()[r], cloud.y()[r], cloud.z()[r]};
  });
  // Boules de W_K : spheres de S*, puis ordre canonique (postordre du noeud, niveau, centre exact).
  std::vector<num::Sphere> spheres;
  for (BallIdx b : a.balls()) {
    auto made = ball_sphere(cloud, cat.balls_data()[idx(b)]);
    if (!made.ok()) return made.outcome();
    spheres.push_back(made.value());
  }
  const auto post = postorder(forest);
  for (u32 v : post) if (v == kNone) return fail(Reason::tower_invariant);
  std::vector<u32> order(a.size());
  for (u32 i = 0; i < a.size(); ++i) order[i] = i;
  std::stable_sort(order.begin(), order.end(), [&](u32 l, u32 r) {
    const u32 pl = post[idx(a.node()[l])], pr = post[idx(a.node()[r])];
    if (pl != pr) return pl < pr;
    const u32 rl = idx(cat.balls_data()[idx(a.balls()[l])].rank), rr = idx(cat.balls_data()[idx(a.balls()[r])].rank);
    if (rl != rr) return rl < rr;
    return num::compare_centers(spheres[l], spheres[r]) < 0;
  });
  std::vector<std::vector<u32>> own(forest.nodes().size());
  for (u32 i = 0; i < order.size(); ++i) own[idx(a.node()[order[i]])].push_back(i);
  out << "{\"balls\":[";
  for (u32 i = 0; i < order.size(); ++i) {
    const u32 at = order[i];
    const auto& ball = cat.balls_data()[idx(a.balls()[at])];
    std::vector<u32> prior;
    for (u64 j = a.prior_offsets()[at]; j < a.prior_offsets()[at + 1]; ++j) prior.push_back(idx(a.prior()[j]));
    std::string s_star = "[";
    for (u32 j = 0; j < ball.qmin; ++j) { if (j) s_star += ','; s_star += xyz(cloud, ball.support[j]); }
    out << (i ? "," : "") << "{\"center\":" << strings(center_text(spheres[at]))
        << ",\"components\":" << a.components()[at] << ",\"level\":\"" << level_text(cat.levels()[idx(ball.rank)])
        << "\",\"m\":" << ball.m << ",\"node\":" << idx(a.node()[at]) << ",\"p\":" << ball.p
        << ",\"prior\":" << list(prior) << ",\"qmin\":" << unsigned{ball.qmin} << ",\"role\":\""
        << role_name(a.role()[at]) << "\",\"s_star\":" << s_star << "],\"strict_traces\":" << a.strict_traces()[at]
        << '}';
  }
  out << "],\"format\":\"hgp11_attach_probe\",\"ids\":[";
  for (u32 j = 0; j < n; ++j) out << (j ? "," : "") << idx(cloud.ids()[cloud.offsets()[lex[j]]]);
  out << "],\"k\":" << k << ",\"n\":" << n << ",\"nodes\":[";
  for (u32 v = 0; v < forest.nodes().size(); ++v) {
    const auto& node = forest.nodes()[v];
    const auto kids = forest.children(NodeIdx{v});
    std::vector<u32> children;
    for (NodeIdx c : kids) children.push_back(idx(c));
    std::string center = "null";
    if (v < forest.births() && k == 1) {
      const SiteIdx s{node.birth_key};
      center = "[\"" + std::to_string(cloud.x()[idx(s)]) + "\",\"" + std::to_string(cloud.y()[idx(s)]) + "\",\"" +
               std::to_string(cloud.z()[idx(s)]) + "\"]";
    } else if (v < forest.births()) {
      auto made = ball_sphere(cloud, cat.balls_data()[node.birth_key]);
      if (!made.ok()) return made.outcome();
      center = strings(center_text(made.value()));
    }
    const u32 kind = v >= forest.births() ? 2 : k == 1 ? 0 : 1;
    out << (v ? "," : "") << "{\"balls\":" << list(own[v]) << ",\"birth_center\":" << center
        << ",\"children\":" << list(children) << ",\"kind\":" << kind << ",\"level\":\""
        << level_text(cat.levels()[idx(node.rank)]) << "\",\"parent\":";
    if (node.parent == NodeIdx{kNone}) out << "null"; else out << idx(node.parent);
    out << ",\"post\":" << post[v] << '}';
  }
  out << "],\"sites\":[";
  for (u32 j = 0; j < n; ++j) out << (j ? "," : "") << xyz(cloud, SiteIdx{lex[j]});
  out << "],\"version\":1}\n";
  return {};
}

template <class T>
bool number(std::string_view word, T& value) {
  const auto result = std::from_chars(word.data(), word.data() + word.size(), value);
  return !word.empty() && result.ec == std::errc{} && result.ptr == word.data() + word.size();
}
template <class T>
bool next(T& value) { std::string word; return bool(std::cin >> word) && number(word, value); }

Outcome request(u32 k, u64 bytes, const std::vector<u32>& x, const std::vector<u32>& y, const std::vector<u32>& z,
                const std::vector<PointId>& ids, u32 workers) {
  MemoryBudget budget(bytes);
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(k);
  auto domain = prepare_full_domain(std::move(index.value()), params, budget);
  if (!domain.ok()) return domain.outcome();
  std::unique_ptr<sched::Pool> pool;
  if (workers > 1) {
    auto made = sched::make_pool({workers});
    if (!made.ok()) return made.outcome();
    pool = std::move(made.value());
  }
  auto tree = build_order(std::move(domain.value()), static_cast<Order>(k), budget, params_for(workers), pool.get());
  if (!tree.ok()) return tree.outcome();
  return json(std::cout, tree.value());
}

int requests(u32 workers) {
  std::string first;
  int code = 0;
  while (std::cin >> first) {
    u32 k = 0, n = 0;
    u64 bytes = 0;
    if (!number(first, k) || !next(bytes) || !next(n) || n == 0 || n > 14) return 2;
    std::vector<u32> x(n), y(n), z(n);
    std::vector<PointId> ids(n);
    for (u32 i = 0; i < n; ++i) {
      u32 id = 0;
      if (!next(x[i]) || !next(y[i]) || !next(z[i]) || !next(id)) return 2;
      ids[i] = PointId{id};
    }
    const Outcome outcome = guarded([&]() { return request(k, bytes, x, y, z, ids, workers); });
    if (!outcome.ok()) {
      std::cout << "{\"k\":" << k << ",\"status\":\"" << status_name(outcome.status()) << "\",\"reason\":\""
                << reason_name(outcome.reason) << "\"}\n";
      code = std::max(code, exit_code(outcome));
    }
  }
  return code;
}

// Bloc de foret et bloc d'incidences fortes de l'ordre K, conventions de write_order (bench/points_export.cpp).
Outcome write_blocks(std::ostream& out, const OrderTree& tree) {
  const auto& domain = tree.domain();
  const auto& cat = domain.catalogue();
  const auto& forest = tree.forest();
  const auto& a = tree.attachment();
  const u32 n = domain.index().cloud().sites(), k = tree.order();
  word(out, k); word(out, forest.nodes().size()); word(out, forest.births()); word(out, idx(forest.root()));
  for (const auto& node : forest.nodes()) { word(out, idx(node.parent)); word(out, idx(node.rank)); }
  std::vector<u64> offsets(u64{n} + 1, 0), cursor;
  std::vector<u64> packed;
  std::vector<NodeIdx> leaf(n, NodeIdx{kNone});
  auto strong = [&](u64 at) {
    const auto& ball = cat.balls_data()[idx(a.balls()[at])];
    return u64{ball.p} + ball.qmin <= k && k <= u64{ball.p} + ball.m;
  };
  auto each_site = [&](u64 at, auto&& run) {
    for (SiteIdx s : cat.interior(a.balls()[at])) run(idx(s));
    for (SiteIdx s : cat.shell(a.balls()[at])) run(idx(s));
  };
  if (k == 1) {
    for (u32 v = 0; v < forest.births(); ++v) leaf[forest.nodes()[v].birth_key] = NodeIdx{v};
    for (u32 s = 0; s < n; ++s) offsets[s + 1] = 1;
  } else {
    for (u64 at = 0; at < a.size(); ++at)
      if (strong(at)) each_site(at, [&](u32 s) { offsets[s + 1] += 1; });
  }
  for (u32 s = 0; s < n; ++s) offsets[s + 1] += offsets[s];
  packed.resize(offsets[n]);
  cursor.assign(offsets.begin(), offsets.end() - 1);
  if (k == 1) {
    for (u32 s = 0; s < n; ++s) {
      if (leaf[s] == NodeIdx{kNone}) return fail(Reason::tower_invariant);
      packed[cursor[s]++] = idx(leaf[s]);
    }
  } else {
    for (u64 at = 0; at < a.size(); ++at) {
      if (!strong(at)) continue;
      const u64 value = (u64{idx(cat.balls_data()[idx(a.balls()[at])].rank)} << 32) | u64{idx(a.node()[at])};
      each_site(at, [&](u32 s) { packed[cursor[s]++] = value; });
    }
  }
  for (u32 s = 0; s < n; ++s) {
    if (cursor[s] != offsets[s + 1] || offsets[s + 1] == offsets[s]) return fail(Reason::tower_invariant);
    std::sort(packed.begin() + static_cast<std::ptrdiff_t>(offsets[s]),
              packed.begin() + static_cast<std::ptrdiff_t>(offsets[s + 1]));
  }
  word(out, offsets[n]);
  for (u64 value : offsets) word(out, value);
  for (u64 value : packed) word(out, value);
  return {};
}

Outcome incidences(const std::string& path, const std::string& xyz, const std::string& ids, u32 k, u32 workers) {
  MemoryBudget budget(u64{32} << 30);
  auto input = read_input(xyz.c_str(), ids.c_str(), budget);
  if (!input.ok()) return input.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  if (k > cloud.value().sites()) return fail(Reason::parameter_out_of_range);
  auto pool = sched::make_pool({workers});
  if (!pool.ok()) return pool.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(k); params.leaf_size = 16; params.max_leaf = 256;
  params.cache_center_lines = params.indirect_sort = params.adaptive_frontier = true;
  params.parallel_assembly = params.single_pass = params.pair_graph = true;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  auto domain = prepare_full_domain(std::move(index.value()), params, budget, *pool.value());
  if (!domain.ok()) return domain.outcome();
  FullParams full;
  full.regular_batch_capacity = 4096; full.descent_lanes = 48;
  full.reuse_census_workspace = full.dense_birth_lookup = full.population_lookup = true;
  auto tree = build_order(std::move(domain.value()), static_cast<Order>(k), budget, full, pool.value().get());
  if (!tree.ok()) return tree.outcome();
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  MHGP11_TRY(write_blocks(out, tree.value()));
  out.close();
  if (!out) return fail(Reason::output_unwritable);
  std::cout << "{\"phase\":\"attach_incidences\",\"k\":" << k << ",\"sites\":" << tree.value().domain().index().cloud().sites()
            << ",\"nodes\":" << tree.value().forest().nodes().size() << ",\"balls\":" << tree.value().attachment().size()
            << "}\n";
  return {};
}
}  // namespace

int main(int argc, char** argv) {
  std::string path, xyz, ids;
  u64 k = 0, workers = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string_view a(argv[i]);
    if (a.substr(0, 13) == "--incidences=") path = std::string(a.substr(13));
    else if (a.substr(0, 8) == "--input=") {
      const auto comma = a.find(',');
      if (comma == std::string_view::npos) return 2;
      xyz = std::string(a.substr(8, comma - 8)); ids = std::string(a.substr(comma + 1));
    } else if (a.substr(0, 4) == "--k=") { if (!parse(a.substr(4), k)) return 2; }
    else if (a.substr(0, 10) == "--workers=") {
      if (!parse(a.substr(10), workers) || workers == 0 || workers > sched::kMaxWorkers) return 2;
    } else return 2;
  }
  if (path.empty()) return xyz.empty() && k == 0 ? requests(static_cast<u32>(workers)) : 2;
  if (xyz.empty() || k == 0 || k > kMaxMebSites) return 2;
  const Outcome outcome = guarded([&]() {
    return incidences(path, xyz, ids, static_cast<u32>(k), static_cast<u32>(workers));
  });
  if (!outcome.ok())
    std::cout << "{\"phase\":\"attach_incidences\",\"status\":\"" << status_name(outcome.status())
              << "\",\"reason\":\"" << reason_name(outcome.reason) << "\"}\n";
  return exit_code(outcome);
}
