// Clustering hierarchique depuis la tour : nuage u32le -> catalogue (K) -> ordre K de la tour -> hierarchie de
// points C n X -> condensation HDBSCAN exacte -> etiquettes (i32 par point d'entree, -1 = bruit).
//
//   mhgp10_cluster IN.u32le OUT.i32le --k=K --mcs=M [--z=Z] [--selection=eom|leaf] [--allow-single]
//                  [--threads=W] [--tree=FILE] [--configs=FILE]
// --configs : une configuration de tete par ligne « mcs z eom|leaf 0|1 » ; la i-eme ecrit OUT.i (i = 0, 1, ...),
// toutes sur la meme construction de la tour.
// --k-list=1,2,5 : un seul catalogue (a l'ordre maximal), puis chaque ordre de la liste ; les sorties deviennent
// OUT.k<K>.<i> (les ordres sont independants a catalogue donne).
// --tree : exporte la hierarchie de points (niveaux, parents, attaches) pour les tetes Python de developpement.
// --entry=core|cover : entree des points par leur propre rayon K-NN (coeurs, C n X, defaut) ou par premiere
// couverture (amas discrets), voir TowerParams ; coverE (E = 1..9) : boule couvrante de poids >= K + E (cover1 :
// entree a alpha_{K+1}, comme HGP-old). --entry=core,cover,cover1 : plusieurs entrees sur le meme catalogue (ordre
// max(K) + max(E)) ; les sorties deviennent OUT.<entree>.k<K>.<i> (et l'arbre TREE.<entree>.k<K>). --label=vote (entree cover) : ecrit en
// plus OUT[...].vote, ou chaque point recoit l'amas retenu qui le couvre par sa boule de plus bas niveau (-1 si
// aucun). --cover-extra=E (entree cover) : boule couvrante de poids >= K + E (E = 1 : entree a alpha_{K+1}, comme
// HGP-old) ; le catalogue est construit a l'ordre max(K) + E.
// Validation avant calcul (audits du 29 septembre 2026 : orders[kk - 1] etait lu sans borne alors que la tour tronque a
// min(K, sites) ordres, SIGSEGV a 1 point pour K = 2 ou a 4 points pour K = 5 ; --k-list=2,0 lisait orders[-1]) :
//   - chaque ordre K de --k / --k-list est >= 1 (sinon k_out_of_range), <= kMaxCatalogueOrder (sinon
//     kmax_out_of_range, comme le catalogue), sans doublon (parameter_out_of_range), et <= nombre de sites du nuage
//     (sinon k_out_of_range : l'ordre K de la tour n'existe pas sous K sites) ;
//   - options numeriques lues par le parseur strict commun (core/cli_options.hpp) : jeton entier, chiffres decimaux,
//     borne du type cible verifiee avant conversion ; --z fini >= 0 ; --threads dans [0, 1024] ; --cover-extra et le
//     suffixe de coverE dans [0, 9] ; --entry sans doublon ;
//   - --configs lu en entier avant le calcul (illisible : input_unreadable ; contenu hors format : parameter_out_of_range).
// Entree lue en entier ou refusee avant calcul (cloud/u32le_input.hpp). Sorties (etiquettes, arbres) ouvertes avant le
// calcul quand leur chemin est connu, ecrites en entier ou refusees output_unwritable (core/cli_output.hpp), retirees
// sur refus ; la ligne status=ok n'est ecrite qu'apres toutes les sorties. Un refus ecrit une ligne JSON (statut,
// raison).
// Codes : 0 conforme, 2 refus, 3 invariant viole.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "cloud/u32le_input.hpp"
#include "core/cli_options.hpp"
#include "core/cli_output.hpp"
#include "head/head.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;
using cli::print_refusal;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mhgp10_cluster IN.u32le OUT.i32le --k=K --mcs=M [options]\n");
    return print_refusal(fail(Reason::parameter_out_of_range));
  }
  int k = 2;
  unsigned threads = 0;
  ClusterParams cp;
  std::string tree_out, configs;
  struct EntrySpec {
    PointEntry entry;
    int extra;        // entree cover : boule couvrante de poids >= K + extra
    std::string tag;  // suffixe des sorties quand plusieurs entrees sont demandees
  };
  std::vector<EntrySpec> entries;
  bool vote = false;
  int cover_extra = -1;  // --cover-extra : applique aux jetons « cover » sans suffixe
  std::vector<int> klist;
  constexpr int kIntMax = std::numeric_limits<int>::max();
  for (int i = 3; i < argc; ++i) {
    const std::string_view a = argv[i];
    std::string_view v;
    Outcome bad;
    if (cli::option_value(a, "--k", v)) {
      const auto r = cli::parse_integer<int>(v, 0, kIntMax);
      if (r.ok()) k = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--mcs", v)) {
      const auto r = cli::parse_integer<u64>(v, 0, std::numeric_limits<u64>::max());
      if (r.ok()) cp.min_cluster_size = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--z", v)) {
      const auto r = cli::parse_real(v, 0.0, std::numeric_limits<double>::max());
      if (r.ok()) cp.z = r.value();
      bad = r.outcome();
    } else if (a == "--selection=leaf") {
      cp.selection = Selection::leaf;
    } else if (a == "--selection=eom") {
      cp.selection = Selection::eom;
    } else if (a == "--allow-single") {
      cp.allow_single_cluster = true;
    } else if (cli::option_value(a, "--threads", v)) {
      const auto r = cli::parse_integer<unsigned>(v, 0, cli::kMaxThreads);
      if (r.ok()) threads = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--tree", v)) {
      tree_out = std::string(v);
    } else if (cli::option_value(a, "--configs", v)) {
      configs = std::string(v);
    } else if (cli::option_value(a, "--entry", v)) {
      entries.clear();
      size_t pos = 0;
      while (bad.ok()) {
        const size_t e = v.find(',', pos);
        const std::string tok(v.substr(pos, e == std::string_view::npos ? std::string_view::npos : e - pos));
        if (tok == "core") {
          entries.push_back({PointEntry::core, 0, tok});
        } else if (tok.rfind("cover", 0) == 0 && tok.size() <= 6) {  // cover, ou coverE a un chiffre
          const auto r = tok.size() == 5 ? Result<int>(0) : cli::parse_integer<int>(tok.substr(5), 0, 9);
          if (r.ok()) entries.push_back({PointEntry::cover, r.value(), tok});
          bad = r.outcome();
        } else {
          std::fprintf(stderr, "entree inconnue %s\n", tok.c_str());
          bad = fail(Reason::parameter_out_of_range);
        }
        for (size_t j = 0; bad.ok() && j + 1 < entries.size(); ++j)
          if (entries[j].tag == entries.back().tag) bad = fail(Reason::parameter_out_of_range);  // doublon
        if (e == std::string_view::npos) break;
        pos = e + 1;
      }
    } else if (a == "--label=vote") {
      vote = true;
    } else if (cli::option_value(a, "--cover-extra", v)) {
      const auto r = cli::parse_integer<int>(v, 0, 9);
      if (r.ok()) cover_extra = r.value();
      bad = r.outcome();
    } else if (a == "--label=tree") {
      vote = false;
    } else if (cli::option_value(a, "--k-list", v)) {
      const auto r = cli::parse_integer_list<int>(v, 0, kIntMax);
      if (r.ok()) klist.insert(klist.end(), r.value().begin(), r.value().end());
      bad = r.outcome();
    } else {
      std::fprintf(stderr, "option inconnue %s\n", argv[i]);
      bad = fail(Reason::parameter_out_of_range);
    }
    if (!bad.ok()) return print_refusal(bad);
  }
  if (klist.empty()) klist.push_back(k);
  for (size_t i = 0; i < klist.size(); ++i) {
    if (klist[i] < 1) return print_refusal(fail(Reason::k_out_of_range));
    if (klist[i] > kMaxCatalogueOrder) return print_refusal(fail(Reason::kmax_out_of_range));
    for (size_t j = 0; j < i; ++j)
      if (klist[j] == klist[i]) return print_refusal(fail(Reason::parameter_out_of_range));  // ordre en double
  }
  // configurations de tete, lues en entier avant tout calcul
  std::vector<ClusterParams> list;
  if (configs.empty()) {
    list.push_back(cp);
  } else {
    const Result<std::string> text = cli::read_text_file(configs.c_str());
    if (!text.ok()) return print_refusal(text.outcome());
    const auto parsed = cli::parse_head_configs(text.value());
    if (!parsed.ok()) return print_refusal(parsed.outcome());
    for (const cli::HeadConfig& h : parsed.value()) {
      ClusterParams q;
      q.min_cluster_size = h.min_cluster_size;
      q.z = h.z;
      q.selection = h.leaf ? Selection::leaf : Selection::eom;
      q.allow_single_cluster = h.allow_single;
      list.push_back(q);
    }
  }
  auto input = read_u32le_cloud(argv[1]);
  if (!input.ok()) return print_refusal(input.outcome());
  const U32leCloud& in = input.value();
  const u32 n = static_cast<u32>(in.x.size());  // < kNone : garde du lecteur
  using clk = std::chrono::steady_clock;
  const auto t0 = clk::now();
  auto prepared = prepare_cloud(in.x, in.y, in.z, in.pid, kCoordinateBits);
  if (!prepared.ok()) return print_refusal(prepared.outcome());
  const Cloud& cloud = prepared.value();
  // la tour ne construit que min(K, sites) ordres : un ordre au-dela n'existe pas (et ne serait jamais lu)
  for (int kk : klist)
    if (u32(kk) > cloud.sites()) return print_refusal(fail(Reason::k_out_of_range));
  if (entries.empty()) entries.push_back({PointEntry::core, 0, "core"});
  if (cover_extra >= 0)
    for (EntrySpec& es : entries)
      if (es.entry == PointEntry::cover && es.tag == "cover") es.extra = cover_extra;
  int max_extra = 0;
  for (const EntrySpec& es : entries) max_extra = std::max(max_extra, es.extra);
  int kmax = 0;
  for (int kk : klist) kmax = std::max(kmax, kk);
  const bool multi = klist.size() > 1 || !configs.empty() || entries.size() > 1;
  auto label_path = [&](int kk, const std::string& tag, size_t i) {
    return !multi ? std::string(argv[2]) : std::string(argv[2]) + tag + ".k" + std::to_string(kk) + "." + std::to_string(i);
  };
  auto tree_path = [&](int kk, const std::string& tag) {
    return tree_out + (multi ? tag + ".k" + std::to_string(kk) : std::string());
  };
  // sorties connues avant le calcul : ouvertes (creees ou tronquees) maintenant ; les etiquettes .vote dependent de la
  // tour et sont ouvertes a l'ecriture
  cli::OutputSet outputs;  // sans commit : toute sortie reservee ou ecrite est retiree
  for (int kk : klist)
    for (const EntrySpec& es : entries) {
      const std::string tag = entries.size() > 1 ? "." + es.tag : "";
      for (size_t i = 0; i < list.size(); ++i)
        if (const Outcome o = outputs.reserve(label_path(kk, tag, i)); !o.ok()) return print_refusal(o);
      if (!tree_out.empty())
        if (const Outcome o = outputs.reserve(tree_path(kk, tag)); !o.ok()) return print_refusal(o);
    }
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  CatalogueParams catp;
  catp.kmax = kmax + max_extra;  // l'entree cover a K + extra lit des boules de poids K + extra
  auto cat = build_catalogue(cloud, catp, pool);
  if (!cat.ok()) return print_refusal(cat.outcome());
  const auto t1 = clk::now();
  double tower_s = 0, head_s = 0;
  size_t clusters = 0;
  for (int kk : klist)
  for (const EntrySpec& es : entries) {
    const PointEntry entry = es.entry;
    const std::string tag = entries.size() > 1 ? "." + es.tag : "";
    const auto a0 = clk::now();
    TowerParams tp;
    tp.kmax = kmax;
    tp.only_order = kk;
    tp.entry = entry;
    tp.cover_extra = es.extra;
    tp.ball_nodes = vote && entry == PointEntry::cover;  // la relation de couverture complete ne sert qu'au vote
    auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
    if (!tw.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\",\"k\":%d}\n", std::string(status_name(tw.outcome().status())).c_str(),
                  std::string(reason_name(tw.outcome().reason)).c_str(), kk);
      return tw.outcome().status() == Status::invariant_violated ? 3 : 2;
    }
    const auto a1 = clk::now();
    const OrderForest& forest = tw.value().orders[kk - 1];  // kk <= min(kmax, sites) = orders.size() (garde plus haut)
    const PointDendrogram d = point_dendrogram(cat.value(), forest, cloud);
    const Outcome v = validate(d);
    if (!v.ok()) {
      std::printf("{\"status\":\"invariant_violated\",\"reason\":\"%s\"}\n", std::string(reason_name(v.reason)).c_str());
      return 3;
    }
    // vote de couverture : boules couvrantes de chaque site, par niveau croissant (ordre canonique du catalogue)
    std::vector<u64> cov_off;
    std::vector<u32> cov_ball;
    if (vote && !forest.ball_node.empty()) {
      const Catalogue& C = cat.value();
      cov_off.assign(u64(cloud.sites()) + 1, 0);
      for (u32 b = 0; b < C.balls(); ++b)
        if (forest.ball_node[b] != kNone)
          for (u64 q = C.pop_off[b]; q < C.pop_off[b + 1]; ++q) ++cov_off[C.pop[q] + 1];
      for (u32 s = 0; s < cloud.sites(); ++s) cov_off[s + 1] += cov_off[s];
      cov_ball.resize(cov_off[cloud.sites()]);
      std::vector<u64> fillc(cov_off.begin(), cov_off.end() - 1);
      for (u32 b = 0; b < C.balls(); ++b)
        if (forest.ball_node[b] != kNone)
          for (u64 q = C.pop_off[b]; q < C.pop_off[b + 1]; ++q) cov_ball[fillc[C.pop[q]]++] = b;
    }
    for (size_t i = 0; i < list.size(); ++i) {
      const Clustering cl = cluster(d, list[i]);
      clusters = cl.selected.size();
      std::vector<i32> lab_vote;
      if (!cov_off.empty()) {
        lab_vote.assign(cloud.sites(), -1);
        for (u32 s = 0; s < cloud.sites(); ++s)
          for (u64 q = cov_off[s]; q < cov_off[s + 1] && lab_vote[s] < 0; ++q) {
            const u32 c = cl.tree.node_cluster[forest.ball_node[cov_ball[q]]];
            if (c != kNone) lab_vote[s] = cl.cluster_label[c];
          }
      }
      const std::string path = label_path(kk, tag, i);
      for (int variant = 0; variant < (lab_vote.empty() ? 1 : 2); ++variant) {
        const std::vector<i32>& lab = variant == 0 ? cl.label : lab_vote;
        std::vector<i32> out(n, -1);
        for (u32 s = 0; s < cloud.sites(); ++s)
          for (PointId p : cloud.ids.row(s)) out[idx(p)] = lab[s];
        const Outcome written = outputs.write(path + (variant == 0 ? "" : ".vote"),
                                              [&](std::FILE* o) { std::fwrite(out.data(), 4, n, o); });
        if (!written.ok()) return print_refusal(written);
      }
    }
    if (!tree_out.empty()) {
      const Outcome written = outputs.write(tree_path(kk, tag), [&](std::FILE* t) {
        std::fprintf(t, "levels %zu\n", d.level.size());
        for (double lv : d.level) std::fprintf(t, "%.17g\n", lv);
        std::fprintf(t, "nodes %u\n", d.nodes());
        for (u32 v2 = 0; v2 < d.nodes(); ++v2)
          std::fprintf(t, "%u %lld\n", d.node_rank[v2], d.parent[v2] == kNone ? -1LL : (long long)d.parent[v2]);
        std::fprintf(t, "points %u\n", n);
        for (u32 s = 0; s < cloud.sites(); ++s)
          for (PointId p : cloud.ids.row(s)) std::fprintf(t, "%u %u %u %u\n", idx(p), d.point_node[s], d.point_rank[s], 1u);
      });
      if (!written.ok()) return print_refusal(written);
    }
    const auto a2 = clk::now();
    tower_s += std::chrono::duration<double>(a1 - a0).count();
    head_s += std::chrono::duration<double>(a2 - a1).count();
  }
  outputs.commit();
  auto sec = [](auto a, auto b) { return std::chrono::duration<double>(b - a).count(); };
  std::printf("{\"status\":\"ok\",\"n\":%u,\"kmax\":%d,\"balls\":%u,\"clusters\":%zu,\"catalogue_s\":%.3f,\"tower_s\":%.3f,"
              "\"head_s\":%.3f}\n",
              n, kmax, cat.value().balls(), clusters, sec(t0, t1), tower_s, head_s);
  return cli::finish_stdout(0);
}
