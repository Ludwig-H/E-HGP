// Porte du lecteur strict MHGP12TR (microbanc MES-M5, hors produit) : constat CST-0223 de l'auditeur Codex.
//
// Ecrit par le VRAI ecrivain (format::write, permissif) de petits vidages synthetiques a empreinte FNV-1a exacte, puis
// les relit par le VRAI lecteur (format::read). Attendus : les vidages valides sont admis ; chaque vidage invalide est
// refuse, avec sa raison. Invalides : les quatre references semantiques de l'auditeur (tests G1 impossibles, somme des
// tests qui deborde u64, feuille qui aurait du etre coupee, racine hors de l'enveloppe exacte), ses six corruptions
// deja refusees (site hors du nuage, K nul, coordonnee hors profil, debut qui deborde, grand livre faux, refus avec
// prefixe), un fichier qui annonce 2^40 noeuds (refus avant allocation), et les mutants des regles ajoutees (tests sous
// la borne basse, boite de feuille differente de l'enveloppe de ses sites, racine trop large en y). Plus la somme
// controlee a la borne de u64 (le debordement reel exigerait plus de 10^8 noeuds : teste sur la fonction elle-meme).
//
// Usage : mhgp12_traversal_format_selftest <dossier temporaire>
// Sortie : une ligne JSON par cas ({"cas", "attendu", "admis", "raison"}), puis un bilan. Codes : 0 conforme,
// 1 un cas differe de l'attendu, 2 refus (arguments, ecriture impossible).
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include "mhgp12/traversal/format.hpp"

namespace {

namespace f = mhgp12::traversal::format;

// Un site par position (i, 0, 0), K1, feuille 4, max_leaf 8 : racine feuille (temoin valide de l'auditeur).
f::Dump one(f::u32 n = 1) {
  f::Dump d;
  auto& h = d.header;
  h.producer = f::kProducerOracle;
  h.coord_bits = 21;
  h.kmax = 1;
  h.leaf_size = 4;
  h.max_leaf = 8;
  h.n_sites = n;
  h.n_nodes = h.nodes = 1;
  h.n_leaves = h.leaves = 1;
  h.n_leaf_sites = n;
  h.filter_tests = n;
  h.max_leaf_seen = n;
  for (f::u32 i = 0; i < n; ++i) {
    d.x.push_back(i);
    d.y.push_back(0);
    d.z.push_back(0);
    d.sites.push_back(i);
  }
  f::Node node;
  node.hi[0] = n;
  node.hi[1] = node.hi[2] = 1;
  node.candidates = node.count = n;
  node.kind = f::kKindLeaf;
  node.tests = n;
  node.list_fnv = f::list_fnv(d.sites.data(), n);
  d.nodes.push_back(node);
  f::Leaf leaf;
  leaf.m = n;
  leaf.hi[0] = n;
  leaf.hi[1] = leaf.hi[2] = 1;
  d.leaves.push_back(leaf);
  return d;
}

// Topologie a trois noeuds de l'auditeur (racine coupee, deux feuilles), rendue valide : cinq sites sur l'axe x,
// K1, feuille 4 ; racine [0,5) coupee au milieu 2 ; feuilles {0,1} dans [0,2) et {2,3,4} dans [2,5) ; tests de chaque
// noeud dans [5, 15] (cinq candidats, K1).
f::Dump split(f::u64 root_tests = 5, f::u64 child_tests = 5) {
  f::Dump d = one(5);
  d.nodes[0].kind = f::kKindSplit;
  d.nodes[0].tests = root_tests;
  auto left = d.nodes[0], right = left;
  left.kind = right.kind = f::kKindLeaf;
  left.depth = right.depth = 1;
  left.count = 2;
  right.count = 3;
  left.hi[0] = right.lo[0] = 2;
  right.path[0] = f::u64{1} << 63;
  left.tests = right.tests = child_tests;
  left.list_fnv = f::list_fnv(d.sites.data(), 2);
  right.list_fnv = f::list_fnv(d.sites.data() + 2, 3);
  d.nodes.push_back(left);
  d.nodes.push_back(right);
  auto a = d.leaves[0], b = a;
  a.depth = b.depth = 1;
  a.m = 2;
  b.m = 3;
  b.begin = 2;
  a.hi[0] = b.lo[0] = 2;
  b.path[0] = f::u64{1} << 63;
  d.leaves = {a, b};
  d.header.nodes = d.header.n_nodes = 3;
  d.header.leaves = d.header.n_leaves = 2;
  d.header.max_depth = 1;
  d.header.max_leaf_seen = 3;
  d.header.filter_tests = root_tests + 2 * child_tests;
  return d;
}

// Six sites a la meme position (0,0,0) : feuille de largeur 1 au-dela de la taille de feuille (6 > 4), admise par la
// v11 (largeur <= 1) tant que m <= max_leaf.
f::Dump unit_box() {
  f::Dump d = one(6);
  for (f::u32 i = 0; i < 6; ++i) d.x[i] = 0;
  d.nodes[0].hi[0] = 1;
  d.leaves[0].hi[0] = 1;
  return d;
}

struct Gate {
  std::string folder;
  int failures = 0, cases = 0;

  void expect(const std::string& name, const f::Dump& d, bool accepted, const char* why = "") {
    const std::string path = folder + "/" + name + ".bin";
    std::string error;
    if (!f::write(path, d, error)) {
      std::cerr << "ecriture impossible : " << error << '\n';
      std::exit(2);
    }
    f::Dump read;
    error.clear();
    const bool ok = f::read(path, read, error);
    ++cases;
    if (ok != accepted) ++failures;
    std::remove(path.c_str());
    std::cout << "{\"cas\":\"" << name << "\",\"attendu\":" << (accepted ? "true" : "false")
              << ",\"admis\":" << (ok ? "true" : "false") << ",\"origine\":\"" << why << "\",\"raison\":\"";
    for (char c : error.substr(0, error.find(" : " + path)))
      std::cout << (c == '"' || c == '\\' ? ' ' : c);
    std::cout << "\"}\n";
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  Gate g{argv[1]};
  const auto valid = one();
  g.expect("valide_un_site", valid, true, "temoin valide de l'auditeur");
  g.expect("valide_coupe", split(), true, "topologie de l'auditeur, rendue valide");
  g.expect("valide_largeur_unite", unit_box(), true, "feuille de largeur 1 au-dela de la taille de feuille");
  g.expect("valide_tests_borne_haute", split(15, 15), true, "tests a la borne haute c min(c, 3K)");
  // CST-0223 : les quatre references de l'auditeur (format_probe.cpp, recu audit_b_m5_20261007/juge_format).
  auto d = valid;
  d.nodes[0].tests = d.header.filter_tests = std::numeric_limits<f::u64>::max();
  g.expect("impossible_test_count", d, false, "auditeur");
  d = split(std::numeric_limits<f::u64>::max(), 2);
  d.header.filter_tests = 3;  // somme modulo 2^64
  g.expect("wrapped_test_sum", d, false, "auditeur");
  g.expect("nonterminal_leaf", one(5), false, "auditeur");
  d = valid;
  d.nodes[0].hi[0] = 2;
  g.expect("wrong_root_envelope", d, false, "auditeur");
  // Corruptions deja refusees avant CST-0223 (auditeur).
  d = valid;
  d.sites[0] = 1;
  d.nodes[0].list_fnv = f::list_fnv(d.sites.data(), 1);
  g.expect("site_oob", d, false, "auditeur");
  d = valid;
  d.header.kmax = 0;
  g.expect("k_zero", d, false, "auditeur");
  d = valid;
  d.x[0] = 1u << 21;
  g.expect("coordinate_oob", d, false, "auditeur");
  d = valid;
  d.leaves[0].begin = std::numeric_limits<f::u64>::max();
  g.expect("begin_wrap", d, false, "auditeur");
  d = valid;
  d.header.filter_tests = 2;
  g.expect("ledger_mismatch", d, false, "auditeur");
  d = valid;
  d.header.status = f::kStatusWideLeaf;
  g.expect("refusal_with_prefix", d, false, "auditeur");
  // Mutants des regles ajoutees.
  g.expect("tests_sous_la_borne_basse", split(4, 5), false, "c min(c, K) = 5 > 4");
  g.expect("tests_au_dela_de_la_borne_haute", split(5, 16), false, "c min(c, 3K) = 15 < 16");
  d = split();
  d.leaves[1].lo[0] = 3;  // sites {2,3,4} : enveloppe [2,5)
  g.expect("boite_de_feuille_hors_enveloppe", d, false, "boite ajustee exacte");
  d = split();
  d.nodes[0].hi[1] = 2;  // enveloppe du nuage en y : [0,1)
  g.expect("racine_trop_large_en_y", d, false, "enveloppe exacte de la racine");
  d = unit_box();
  d.header.max_leaf = d.header.leaf_size = 5;
  g.expect("largeur_unite_au_dela_de_max_leaf", d, false, "refus wide_leaf attendu, pas une feuille");
  // Fichier de 264 octets annoncant 2^40 noeuds : refus avant toute allocation (auditeur, short_count).
  const std::string short_path = g.folder + "/short_count.bin";
  auto header = valid.header;
  header.n_nodes = f::u64{1} << 40;
  std::FILE* file = std::fopen(short_path.c_str(), "wb");
  if (file == nullptr) return 2;
  f::u64 tail = 0;
  const bool written =
      std::fwrite(&header, sizeof(header), 1, file) == 1 && std::fwrite(&tail, sizeof(tail), 1, file) == 1;
  if (std::fclose(file) != 0 || !written) return 2;
  f::Dump small;
  std::string error;
  const bool short_ok = f::read(short_path, small, error);
  const bool short_conform = !short_ok && small.x.empty() && small.nodes.empty();
  std::remove(short_path.c_str());
  ++g.cases;
  if (!short_conform) ++g.failures;
  std::cout << "{\"cas\":\"short_count\",\"attendu\":false,\"admis\":" << (short_ok ? "true" : "false")
            << ",\"origine\":\"auditeur\",\"raison\":\"taille annoncee avant allocation\"}\n";
  // Somme controlee a la borne de u64.
  f::u64 sum = std::numeric_limits<f::u64>::max() - 3;
  const bool fits = f::detail::checked_add(sum, 3) && sum == std::numeric_limits<f::u64>::max();
  f::u64 full = std::numeric_limits<f::u64>::max() - 2;
  const bool refuses = !f::detail::checked_add(full, 3) && full == std::numeric_limits<f::u64>::max() - 2;
  ++g.cases;
  if (!fits || !refuses) ++g.failures;
  std::cout << "{\"cas\":\"somme_controlee_borne_u64\",\"attendu\":true,\"admis\":"
            << (fits && refuses ? "true" : "false") << ",\"origine\":\"checked_add\",\"raison\":\"\"}\n";
  std::cout << "{\"porte\":\"format_m5\",\"cas\":" << g.cases << ",\"ecarts\":" << g.failures << "}\n";
  return g.failures == 0 ? 0 : 1;
}
