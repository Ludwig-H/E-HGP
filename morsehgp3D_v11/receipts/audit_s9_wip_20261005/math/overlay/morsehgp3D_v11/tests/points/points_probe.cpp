// Sonde de la hierarchie de points (tranche S9), hors produit :
//   --input=<xyz.u32le>,<ids.u32le> --k=K [--m=M] [--workers=W] [--dump=<fichier.json>] [--cover]
//       arbre d'ordre K seul (parametres de la facade : api_detail::catalogue_params, order_params), puis
//       points::hang_qualified (m par defaut : m(K)) ; une ligne JSON de comptes et de durees (domaine, arbre,
//       rattachement, incidences, qualification, racines, pendaison, arbre de points) sur la sortie standard. --dump :
//       colonnes completes (sites, noeuds, niveaux exacts en hexadecimal des rangs references et des rangs des noeuds,
//       pendaison, arbre de points) ; --cover (petits nuages) : par noeud, les PointId couverts a sa naissance (sites
//       d'une incidence forte de rang <= naissance sous le noeud), identite du noeud independante de la numerotation.
//   --arith : lignes de stdin « d t m q t' m' q' » (ordre de deux dates, num::compare_dates) ou « c a b c d » (signe de
//       sqrt a + sqrt b - sqrt c - sqrt d, num::sqrt_cmp2), rationnels en hexadecimal signe ; une ligne par decision
//       (signe, ou « refused <raison> »).
// Codes : 0 conforme, 2 refus (usage ou raison du moteur), 3 invariant viole.
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../num/big_io.hpp"
#include "api/internal.hpp"
#include "cloud/cloud.hpp"
#include "index/index.hpp"
#include "points/internal.hpp"
#include "whole_input.hpp"

using namespace mhgp11;

namespace {

struct Args {
  std::string xyz, ids, dump;
  u32 k = 0, m = 0, workers = 1;
  bool cover = false, arith = false;
};

bool number(std::string_view text, u32& out) {
  const auto got = std::from_chars(text.data(), text.data() + text.size(), out);
  return got.ec == std::errc() && got.ptr == text.data() + text.size();
}

bool parse(int argc, char** argv, Args& a) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    const auto value = [&](std::string_view key) { return arg.substr(key.size()); };
    if (arg == "--arith") a.arith = true;
    else if (arg == "--cover") a.cover = true;
    else if (arg.rfind("--input=", 0) == 0) {
      const std::string_view both = value("--input=");
      const auto comma = both.find(',');
      if (comma == std::string_view::npos) return false;
      a.xyz = std::string(both.substr(0, comma));
      a.ids = std::string(both.substr(comma + 1));
    } else if (arg.rfind("--dump=", 0) == 0) a.dump = std::string(value("--dump="));
    else if (arg.rfind("--k=", 0) == 0) { if (!number(value("--k="), a.k)) return false; }
    else if (arg.rfind("--m=", 0) == 0) { if (!number(value("--m="), a.m)) return false; }
    else if (arg.rfind("--workers=", 0) == 0) { if (!number(value("--workers="), a.workers)) return false; }
    else return false;
  }
  return a.arith || (!a.xyz.empty() && a.k >= 1 && a.k <= 12 && a.workers >= 1 && a.workers <= 256);
}

int arith() {
  std::string line;
  u64 decisions = 0;
  Result<num::RadicalSum> sum = [] {
    static MemoryBudget budget(MemoryBudget::kUnlimited);
    return num::RadicalSum::make(budget);
  }();
  if (!sum.ok()) return 3;
  while (std::getline(std::cin, line)) {
    std::istringstream words(line);
    std::string kind, token;
    words >> kind;
    std::vector<num::Rational> x;
    Outcome parsed;
    while (words >> token && parsed.ok()) {
      x.emplace_back();
      parsed = num::probe::parse_rational(token, x.back());
    }
    int sign = 0;
    Outcome got = parsed;
    if (got.ok() && kind == "d" && x.size() == 6)
      got = num::compare_dates({x[0], x[1], x[2]}, {x[3], x[4], x[5]}, sum.value(), sign);
    else if (got.ok() && kind == "c" && x.size() == 4)
      got = num::sqrt_cmp2(x[0], x[1], x[2], x[3], sign);
    else if (got.ok())
      return 2;
    if (got.ok()) std::cout << sign << '\n';
    else num::probe::refused(got);
    ++decisions;
  }
  std::cout << "points_arith decisions=" << decisions << '\n';
  return 0;
}

std::string level_text(const num::Level& level) {
  return "[\"" + num::probe::hex(num::Big::from_wide(num::to_wide(level.numerator()))) + "\",\"" +
         num::probe::hex(num::Big::from_wide(num::to_wide(level.denominator()))) + "\"]";
}

template <class Span>
void column(std::ostream& out, const char* name, const Span& values) {
  out << ",\"" << name << "\":[";
  for (std::size_t i = 0; i < values.size(); ++i) out << (i ? "," : "") << u64{values[i]};
  out << ']';
}

// PointId couverts a sa naissance par chaque noeud (petits nuages) : incidences fortes de rang <= naissance sous lui.
Outcome cover(std::ostream& out, const OrderTree& tree, MemoryBudget& budget) {
  points_detail::Incidences inc;
  MHGP11_TRY(points_detail::build_incidences(tree, budget, nullptr, inc));
  Result<AncestorIndex> anc = AncestorIndex::build(tree.forest(), budget);
  if (!anc.ok()) return anc.outcome();
  const Cloud& cloud = tree.domain().index().cloud();
  const auto nodes = tree.forest().nodes();
  out << ",\"cover\":[";
  for (u32 v = 0; v < nodes.size(); ++v) {
    std::vector<u32> ids;
    for (u32 s = 0; s < cloud.sites(); ++s) {
      for (u64 j = inc.offsets[s]; j < inc.offsets[s + 1]; ++j) {
        const u32 r = points_detail::rank_of(inc.packed[j]), u = points_detail::node_of(inc.packed[j]);
        if (r <= idx(nodes[v].rank) && anc.value().lca(NodeIdx{u}, NodeIdx{v}) == NodeIdx{v}) {
          ids.push_back(idx(cloud.points(SiteIdx{s})[0]));
          break;
        }
      }
    }
    std::sort(ids.begin(), ids.end());
    out << (v ? "," : "") << '[';
    for (std::size_t i = 0; i < ids.size(); ++i) out << (i ? "," : "") << ids[i];
    out << ']';
  }
  out << ']';
  return {};
}

Outcome dump(const Args& a, const OrderTree& tree, const points::PointHierarchy& h, MemoryBudget& budget) {
  std::ofstream out(a.dump, std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  const Cloud& cloud = tree.domain().index().cloud();
  const auto levels = tree.domain().catalogue().levels();
  const auto nodes = tree.forest().nodes();
  std::vector<u32> ids, parent, rank;
  for (u32 s = 0; s < cloud.sites(); ++s) ids.push_back(idx(cloud.points(SiteIdx{s})[0]));
  for (const ForestNode& node : nodes) {
    parent.push_back(idx(node.parent));
    rank.push_back(idx(node.rank));
  }
  out << "{\"k\":" << u32{h.order()} << ",\"m\":" << h.qualification() << ",\"n\":" << cloud.sites();
  column(out, "ids", ids);
  column(out, "x", cloud.x());
  column(out, "y", cloud.y());
  column(out, "z", cloud.z());
  column(out, "parent", parent);
  column(out, "rank", rank);
  for (const auto& [name, values] : {std::pair{"t", h.t()}, std::pair{"M", h.m()}, std::pair{"Q", h.q()},
                                     std::pair{"owner", h.owner()}, std::pair{"floor", h.floor()}})
    column(out, name, values);
  column(out, "strict", h.strict());
  column(out, "referenced", h.levels());
  const points::PointTree& t = h.tree();
  column(out, "plateau_t", t.plateau_t());
  column(out, "plateau_m", t.plateau_m());
  column(out, "plateau_q", t.plateau_q());
  column(out, "block_plateau", t.block_plateau());
  column(out, "block_parent", t.block_parent());
  column(out, "site_block", t.site_block());
  column(out, "site_plateau", t.site_plateau());
  out << ",\"levels\":{";
  for (std::size_t i = 0; i < h.levels().size(); ++i)
    out << (i ? "," : "") << '"' << h.levels()[i] << "\":" << level_text(levels[h.levels()[i]]);
  out << '}';
  if (a.cover) MHGP11_TRY(cover(out, tree, budget));
  out << "}\n";
  out.close();
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

int run(const Args& a) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto input = bench::read_input(a.xyz.c_str(), a.ids.c_str(), budget);
  if (!input.ok()) return exit_code(input.outcome());
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return exit_code(cloud.outcome());
  const u32 n = cloud.value().sites();
  auto pool = sched::make_pool({a.workers});
  if (!pool.ok() || cloud.value().weight() != n || a.k > n) return 2;
  const Stopwatch domain_clock;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return exit_code(index.outcome());
  const Order k = static_cast<Order>(a.k);
  auto domain = prepare_full_domain(std::move(index.value()), api_detail::catalogue_params(k), budget, *pool.value());
  if (!domain.ok()) return exit_code(domain.outcome());
  const u64 domain_ns = domain_clock.nanoseconds();
  const Stopwatch tree_clock;
  u64 attach_ns = 0;
  auto tree = build_order(std::move(domain.value()), k, budget, api_detail::order_params(), pool.value().get(), nullptr,
                          &attach_ns);
  if (!tree.ok()) return exit_code(tree.outcome());
  const u64 tree_ns = tree_clock.nanoseconds();
  const u32 m = a.m == 0 ? points::qualification(k) : a.m;
  const Stopwatch points_clock;
  auto h = points::hang_qualified(tree.value(), m, budget, pool.value().get());
  const u64 points_ns = points_clock.nanoseconds();
  if (!h.ok()) {
    std::cout << "{\"phase\":\"points_probe\",\"status\":\"" << status_name(h.outcome().status())
              << "\",\"reason\":\"" << reason_name(h.outcome().reason) << "\"}\n";
    return exit_code(h.outcome());
  }
  const points::PointsStats& st = h.value().stats();
  const points::PointTree& pt = h.value().tree();
  std::cout << "{\"phase\":\"points_probe\",\"status\":\"ok\",\"coord_bits\":" << kCoordBits << ",\"k\":" << a.k
            << ",\"m\":" << m << ",\"workers\":" << a.workers << ",\"sites\":" << n
            << ",\"nodes\":" << tree.value().forest().nodes().size() << ",\"incidences\":" << st.incidences
            << ",\"qualified_nodes\":" << st.qualified_nodes << ",\"delayed\":" << st.delayed
            << ",\"strict\":" << st.strict << ",\"rivals\":" << st.rivals << ",\"dominated\":" << st.dominated
            << ",\"roots\":" << st.roots << ",\"table_decisions\":" << st.table_decisions
            << ",\"exact_decisions\":" << st.exact_decisions << ",\"floor_steps\":" << st.floor_steps
            << ",\"levels\":" << h.value().levels().size() << ",\"plateaus\":" << pt.plateau_t().size()
            << ",\"blocks\":" << pt.block_plateau().size() << ",\"ns\":{\"domain\":" << domain_ns
            << ",\"tree\":" << tree_ns - std::min(tree_ns, attach_ns) << ",\"attach\":" << attach_ns
            << ",\"incidences\":" << st.incidences_ns << ",\"qualify\":" << st.qualify_ns
            << ",\"roots\":" << st.roots_ns << ",\"hang\":" << st.hang_ns << ",\"point_tree\":" << st.tree_ns
            << ",\"points_total\":" << points_ns << "}}\n";
  if (!a.dump.empty()) {
    const Outcome written = dump(a, tree.value(), h.value(), budget);
    if (!written.ok()) return exit_code(written);
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  Args a;
  if (!parse(argc, argv, a)) {
    std::cerr << "usage : points_probe --input=<xyz>,<ids> --k=K [--m=M] [--workers=W] [--dump=F] [--cover] | --arith\n";
    return 2;
  }
  return a.arith ? arith() : run(a);
}
