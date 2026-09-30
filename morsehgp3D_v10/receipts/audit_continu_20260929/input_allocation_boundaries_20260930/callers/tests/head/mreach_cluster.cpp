// Temoin de test (hors produit) de la tete : lit des coordonnees u32le (x y z par point, PointId = rang d'entree),
// construit la hierarchie de points demandee et ecrit une etiquette i32 par point d'entree.
//
//   mhgp10_cluster IN.u32le OUT.i32le --source=mreach --k=5 --mcs=20 [--z=1] [--selection=eom|leaf]
//                  [--allow-single] [--threads=0] [--alpha=1|2] [--configs=FILE]
// --entry=border : entree des points par la regle des points-bord (analogue MR de l'entree cover de la tour).
// --alpha : parametre alpha de scikit-learn (mreach = max(coeurs, distance / alpha)). --configs : une tete par ligne
// « mcs z eom|leaf 0|1 », la i-eme ecrite dans OUT.i, toutes sur la meme hierarchie (diagnostic dev : meme tete sur
// la tour et sur l'atteignabilite mutuelle).
// Options numeriques lues par le parseur strict commun (core/cli_options.hpp) : jeton entier, chiffres decimaux, borne
// du type cible verifiee avant conversion ; --k dans [1, n] (sinon k_out_of_range : --k=-2, 0 ou 99999999999 etaient
// admis) ; --alpha dans {1, 2} ; --z fini >= 0 ; --threads dans [0, 1024] ; --configs lu en entier avant le calcul
// (illisible : input_unreadable ; hors format : parameter_out_of_range ; --configs= vide refuse). Entree lue en entier
// ou refusee avant calcul (cloud/u32le_input.hpp). Sorties (core/cli_output.hpp, raccord R2) : declarees avant toute
// reservation (deux sorties sur un meme fichier, ou une sortie sur l'entree ou sur --configs : output_conflict), un
// temporaire par sortie avant le pool et le calcul, ecrites en entier ou refusees output_unwritable, publiees toutes
// ensemble au commit ; un refus ne touche aucun fichier preexistant. Les lignes « clusters N » ne s'ecrivent qu'apres
// toutes les sorties.
// Un refus ecrit « refus <raison> » sur la sortie d'erreur, sauf les deux refus du pool (raccord R2), ecrits en une
// ligne JSON sur la sortie standard comme dans les sondes produit : pool impossible a creer
// (resource_exhausted/session_overhead, avant tout calcul) et allocation qui echoue pendant la hierarchie
// (resource_exhausted/memory_budget, avant toute ecriture de sortie).
// Codes : 0 conforme, 2 refus (avant calcul, ou memoire epuisee pendant la hierarchie : resource_exhausted),
// 3 dendrogramme invalide.
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <vector>

#include "cloud/site_tree.hpp"
#include "cloud/u32le_input.hpp"
#include "core/cli_options.hpp"
#include "core/cli_output.hpp"
#include "head/head.hpp"
#include "mreach.hpp"

using namespace mhgp10;

namespace {
// Refus : « refus <raison> » sur la sortie d'erreur, code 2.
int refuse(const Outcome& o) {
  std::fprintf(stderr, "refus %s\n", std::string(reason_name(o.reason)).c_str());
  return 2;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mhgp10_cluster IN.u32le OUT.i32le [options]\n");
    return refuse(fail(Reason::parameter_out_of_range));
  }
  std::string source = "mreach";
  u64 k = 5;
  ClusterParams params;
  unsigned threads = 0;
  u64 alpha = 1;
  bool border = false;
  std::string configs;
  for (int i = 3; i < argc; ++i) {
    const std::string_view a = argv[i];
    std::string_view v;
    Outcome bad;
    if (cli::option_value(a, "--source", v)) {
      source = std::string(v);
    } else if (cli::option_value(a, "--k", v)) {
      const auto r = cli::parse_integer<u64>(v, 0, std::numeric_limits<u64>::max());  // domaine [1, n] plus bas
      if (r.ok()) k = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--mcs", v)) {
      const auto r = cli::parse_integer<u64>(v, 0, std::numeric_limits<u64>::max());
      if (r.ok()) params.min_cluster_size = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--z", v)) {
      const auto r = cli::parse_real(v, 0.0, std::numeric_limits<double>::max());
      if (r.ok()) params.z = r.value();
      bad = r.outcome();
    } else if (a == "--selection=leaf") {
      params.selection = Selection::leaf;
    } else if (a == "--selection=eom") {
      params.selection = Selection::eom;
    } else if (a == "--allow-single") {
      params.allow_single_cluster = true;
    } else if (cli::option_value(a, "--threads", v)) {
      const auto r = cli::parse_integer<unsigned>(v, 0, cli::kMaxThreads);
      if (r.ok()) threads = r.value();
      bad = r.outcome();
    } else if (cli::option_value(a, "--alpha", v)) {
      const auto r = cli::parse_integer<u64>(v, 1, 2);
      if (r.ok()) alpha = r.value();
      bad = r.outcome();
    } else if (a == "--entry=border") {
      border = true;
    } else if (a == "--entry=core") {
      border = false;
    } else if (cli::option_value(a, "--configs", v)) {
      if (v.empty()) bad = fail(Reason::parameter_out_of_range);  // --configs= vide : jamais ignore en silence
      configs = std::string(v);
    } else {
      std::fprintf(stderr, "option inconnue %s\n", argv[i]);
      bad = fail(Reason::parameter_out_of_range);
    }
    if (!bad.ok()) return refuse(bad);
  }
  if (source != "mreach") {
    std::fprintf(stderr, "source inconnue %s\n", source.c_str());
    return refuse(fail(Reason::parameter_out_of_range));
  }
  if (k < 1) return refuse(fail(Reason::k_out_of_range));
  std::vector<ClusterParams> list;
  if (configs.empty()) {
    list.push_back(params);
  } else {
    const Result<std::string> text = cli::read_text_file(configs.c_str());
    if (!text.ok()) return refuse(text.outcome());
    const auto parsed = cli::parse_head_configs(text.value());
    if (!parsed.ok()) return refuse(parsed.outcome());
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
  if (!input.ok()) return refuse(input.outcome());
  const U32leCloud& in = input.value();
  const u32 n = static_cast<u32>(in.x.size());  // < kNone : garde du lecteur
  auto prepared = prepare_cloud(in.x, in.y, in.z, in.pid, 21);
  if (!prepared.ok()) return refuse(prepared.outcome());
  const Cloud& cloud = prepared.value();
  // K-ieme voisin parmi les n points (poids compris) : au-dela, la distance de coeur n'existe pas (min_samples <= n) ;
  // juge apres l'entree (un fichier vide reste empty_input)
  if (k > n) return refuse(fail(Reason::k_out_of_range));
  auto path_of = [&](size_t i) {
    return configs.empty() ? std::string(argv[2]) : std::string(argv[2]) + "." + std::to_string(i);
  };
  // entrees et sorties declarees d'abord (conflits refuses, rien n'est cree), puis un temporaire par sortie ; sans
  // commit, aucun fichier preexistant n'est touche
  cli::OutputSet outputs;
  if (const Outcome o = outputs.add_input(argv[1]); !o.ok()) return refuse(o);
  if (!configs.empty())
    if (const Outcome o = outputs.add_input(configs); !o.ok()) return refuse(o);
  for (size_t i = 0; i < list.size(); ++i)
    if (const Outcome o = outputs.declare(path_of(i)); !o.ok()) return refuse(o);
  if (const Outcome o = outputs.reserve(); !o.ok()) return refuse(o);
  SiteTree tree(cloud);
  // Fils du pool (raccord R2) : une creation impossible (fil refuse par le systeme, allocation) est refusee
  // resource_exhausted/session_overhead avant tout calcul HGP ; les sorties reservees sont abandonnees (aucun fichier
  // preexistant touche) ; impression sans allocation.
  auto made = sched::make_pool(threads);
  if (!made.ok()) return cli::print_refusal(made.outcome());
  sched::Pool& pool = *made.value();
  // mreach_dendrogram passe par le pool : une allocation qui echoue (dans l'appelant ou relancee depuis un ouvrier)
  // devient un refus resource_exhausted, avant toute ecriture de sortie.
  PointDendrogram d;
  try {
    d = mreach_dendrogram(tree, k, pool, alpha, border);
  } catch (const std::bad_alloc&) {
    return cli::print_refusal(fail(Reason::memory_budget));  // impression sans allocation
  }
  const Outcome v = validate(d);
  if (!v.ok()) {
    std::fprintf(stderr, "dendrogramme invalide %s\n", std::string(reason_name(v.reason)).c_str());
    return 3;
  }
  std::vector<size_t> selected;
  for (size_t i = 0; i < list.size(); ++i) {
    Clustering cl = cluster(d, list[i]);
    std::vector<i32> out(n, -1);
    for (u32 s = 0; s < cloud.sites(); ++s)
      for (PointId p : cloud.ids.row(s)) out[idx(p)] = cl.label[s];
    const Outcome written = outputs.write(path_of(i), [&](std::FILE* o) { std::fwrite(out.data(), 4, n, o); });
    if (!written.ok()) return refuse(written);
    selected.push_back(cl.selected.size());
  }
  if (const Outcome o = outputs.commit(); !o.ok()) return refuse(o);
  for (size_t c : selected) std::printf("clusters %zu\n", c);
  return cli::finish_stdout(0);
}
