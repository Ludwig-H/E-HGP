// Format MHGP12TR v1 : parcours des boites du catalogue (noeuds visites, feuilles finales, grand livre), pour le
// microbanc MES-M5 de la v12. Hors produit, hote seulement, bibliotheque standard. Les vidages des trames sont derives
// de SemanticKITTI : jamais dans le depot (seules les fixtures synthetiques peuvent y entrer).
//
// Disposition (petit-boutiste, sections dans cet ordre, chacune completee a un multiple de 8 octets) :
//   en-tete   Header (256 octets)
//   nuage     x[n_sites], y[n_sites], z[n_sites] (u32, ordre SiteIdx du nuage prepare)
//   noeuds    Node[n_nodes] (96 octets), ordre PREFIXE du parcours en profondeur gauche/droite de la v11
//   feuilles  Leaf[n_leaves] (80 octets ; les 64 premiers ont la disposition de LeafJob de la v11, depth dans pad)
//   sites     u32[n_leaf_sites] : SiteIdx croissants de chaque feuille, feuilles bout a bout dans l'ordre prefixe
//   fin       u64 : FNV-1a 64 de tous les octets precedents (primitives du format MHGP12LF de MES-M2)
//
// Un noeud est un appel de prepare_node (v11, boxes.cpp) : boite d'entree [lo, hi) (enveloppe de la racine, ou moitie
// de la boite ajustee du parent), liste candidate = liste du parent (tous les sites a la racine), tests du filtre G1,
// liste retenue (count sites ; count = 0 si la liste est vide OU si la boite ajustee est vide), genre (vide, feuille,
// coupe). Le chemin est celui de la v11 (CatalogueTaskDiagnostic::path) : le bit de l'arete de la profondeur d vers
// d+1 est au mot d/64, position 63 - d%64 (1 : enfant droit). L'ordre prefixe est l'ordre (chemin, profondeur).
//
// Le lecteur est STRICT (constat CST-0215 de MES-M2) : tailles controlees contre la taille reelle du fichier avant
// toute allocation, profil, K et parametres dans leur domaine, coordonnees dans [0, 2^B), indices de sites bornes et
// strictement croissants, debuts contigus sans debordement, boites dans le domaine T0, arbre reconstruit et verifie
// (enfants = moitiés de la boite ajustee, candidats = liste du parent, profondeur, chemins), feuilles = noeuds feuilles
// dans l'ordre prefixe (meme boite d'entree contenant la boite ajustee, meme empreinte de liste), grand livre = sommes
// des sections. Un FNV valide protege l'integrite du fichier, pas ces invariants : ils sont tous verifies.
// Regles semantiques de la v11 (boxes.cpp) verifiables sans rejouer G1 (CST-0223) : tests G1 d'un noeud dans
// [c min(c, K), c min(c, 3K)] (c candidats ; chacun examine au moins min(c, K) et au plus min(c, 3K) temoins du
// reservoir), somme des tests controlee avant chaque addition (aucun debordement de u64) ; racine = enveloppe exacte
// [min, max+1) de tous les sites ; boite d'une feuille = enveloppe exacte de ses sites inter boite d'entree de son
// noeud ; feuille seulement si m <= taille de feuille ou largeur <= 1 (sinon la v11 aurait coupe).
#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp12/leaf/dump_format.hpp"  // FNV-1a et primitives d'ecriture/lecture de MES-M2

namespace mhgp12::traversal::format {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;

inline constexpr std::array<char, 8> kMagic = {'M', 'H', 'G', 'P', '1', '2', 'T', 'R'};
inline constexpr u64 kVersion = 1;
inline constexpr u32 kNoParent = 0xFFFFFFFFu;

// Producteur du vidage (le contenu ne depend pas du producteur ; seul le juge le lit).
enum : u64 { kProducerV11 = 1, kProducerOracle = 2, kProducerV12Host = 3, kProducerV12Device = 4 };
// Statut du parcours : refus transactionnels de la v11 (wide_leaf, profondeur) ; aucun prefixe n'est publie.
enum : u64 { kStatusOk = 0, kStatusWideLeaf = 1, kStatusDepth = 2 };
enum : u32 { kKindEmpty = 0, kKindLeaf = 1, kKindSplit = 2 };

struct Header {
  std::array<char, 8> magic = kMagic;
  u64 version = kVersion;
  u64 producer = 0;
  u64 coord_bits = 0;  // B : coordonnees dans [0, 2^B), boites dans [0, 2^B] ; profondeur <= 3B
  u64 kmax = 0;
  u64 leaf_size = 0;
  u64 max_leaf = 0;
  u64 n_sites = 0;
  u64 n_nodes = 0;
  u64 n_leaves = 0;
  u64 n_leaf_sites = 0;
  u64 status = kStatusOk;
  // Grand livre de la v11 (CatalogueLedger, champs du parcours) : sommes et maxima indépendants de l'ordre de visite.
  u64 nodes = 0, leaves = 0, filter_tests = 0, max_depth = 0, max_leaf_seen = 0;
  u64 reserved[15] = {};
};
static_assert(sizeof(Header) == 256);

struct Node {
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};  // boite d'entree demi-ouverte
  u64 path[2] = {0, 0};
  u64 tests = 0;     // tests G1 de ce noeud
  u64 list_fnv = 0;  // FNV-1a 64 de la liste retenue (u32 petit-boutistes), 0 si count = 0
  u32 candidates = 0, count = 0, depth = 0, kind = kKindEmpty;
};
static_assert(sizeof(Node) == 96);

struct Leaf {  // 64 premiers octets : disposition de catalogue_detail::LeafJob (v11), depth dans pad
  u64 begin = 0;
  u32 m = 0, depth = 0;
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};  // boite ajustee demi-ouverte transmise a la feuille
  u64 path[2] = {0, 0};
};
static_assert(sizeof(Leaf) == 80);
static_assert(sizeof(dump::Job) == 64, "les 64 premiers octets de Leaf sont un LeafJob");

struct Dump {
  Header header;
  std::vector<u32> x, y, z;
  std::vector<Node> nodes;
  std::vector<Leaf> leaves;
  std::vector<u32> sites;
  const u32* leaf_sites(u64 j) const { return sites.data() + leaves[j].begin; }
};

inline u64 list_fnv(const u32* sites, u64 count) {
  if (count == 0) return 0;
  return dump::fnv1a(dump::kFnvBasis, sites, count * sizeof(u32));
}

// Ordre prefixe : (chemin, profondeur).
inline bool prefix_before(const u64* pa, u32 da, const u64* pb, u32 db) {
  if (pa[0] != pb[0]) return pa[0] < pb[0];
  if (pa[1] != pb[1]) return pa[1] < pb[1];
  return da < db;
}

inline u64 section_bytes(u64 count, u64 size) { return (count * size + 7) / 8 * 8; }

inline bool write(const std::string& path, const Dump& d, std::string& error) {
  const Header& h = d.header;
  if (d.x.size() != h.n_sites || d.y.size() != h.n_sites || d.z.size() != h.n_sites ||
      d.nodes.size() != h.n_nodes || d.leaves.size() != h.n_leaves || d.sites.size() != h.n_leaf_sites) {
    error = "tailles incoherentes avec l'en-tete";
    return false;
  }
  dump::detail::Writer w;
  w.f = std::fopen(path.c_str(), "wb");
  if (w.f == nullptr) {
    error = "ouverture en ecriture impossible : " + path;
    return false;
  }
  w.put(&h, sizeof(h));
  w.section(d.x);
  w.section(d.y);
  w.section(d.z);
  w.section(d.nodes);
  w.section(d.leaves);
  w.section(d.sites);
  const u64 digest = w.hash;
  if (w.ok) w.ok = std::fwrite(&digest, 1, sizeof(digest), w.f) == sizeof(digest);
  w.ok = (std::fclose(w.f) == 0) && w.ok;
  if (!w.ok) error = "ecriture incomplete : " + path;
  return w.ok;
}

namespace detail {

inline bool fail(std::string& error, const std::string& what) {
  error = what;
  return false;
}

// Tests G1 d'un noeud a c candidats (filter de boxes.cpp, v11) : le reservoir garde min(c, 3K) temoins ; chaque
// candidat en examine dans l'ordre jusqu'au K-ieme dominateur, donc au moins min(c, K) et au plus min(c, 3K).
// c < 2^32, K <= 12 : produits < 2^38, exacts en u64.
inline bool tests_in_bounds(u64 tests, u64 candidates, u64 kmax) {
  const u64 low = candidates * (candidates < kmax ? candidates : kmax);
  const u64 high = candidates * (candidates < 3 * kmax ? candidates : 3 * kmax);
  return tests >= low && tests <= high;
}

// Somme controlee : faux (somme inchangee) si sum + value deborderait u64.
inline bool checked_add(u64& sum, u64 value) {
  if (value > ~u64{0} - sum) return false;
  sum += value;
  return true;
}

// Moities de la boite ajustee du parent (split_ready de la v11) : memes axes hors coupe, coupe au milieu.
inline bool halves(const Node& left, const Node& right) {
  int axis = -1;
  for (int a = 0; a < 3; ++a) {
    if (left.lo[a] == right.lo[a] && left.hi[a] == right.hi[a]) continue;
    if (axis >= 0) return false;
    axis = a;
  }
  if (axis < 0) return false;
  const i64 lo = left.lo[axis], hi = right.hi[axis], middle = left.hi[axis];
  if (right.lo[axis] != middle || hi - lo <= 1 || middle != lo + (hi - lo) / 2) return false;
  // axe de plus grande largeur, premier en cas d'egalite
  for (int a = 0; a < 3; ++a) {
    const i64 w = left.hi[a] - left.lo[a];
    const i64 wa = hi - lo;
    if (a < axis && w >= wa) return false;
    if (a > axis && w > wa) return false;
  }
  return true;
}

}  // namespace detail

// Lecture stricte : rend faux et une raison au premier invariant viole, sans jamais allouer avant le controle de taille.
inline bool read(const std::string& path, Dump& d, std::string& error) {
  using detail::checked_add;
  using detail::fail;
  using detail::tests_in_bounds;
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) return fail(error, "ouverture en lecture impossible : " + path);
  if (std::fseek(f, 0, SEEK_END) != 0) {
    std::fclose(f);
    return fail(error, "taille illisible : " + path);
  }
  const long size_long = std::ftell(f);
  std::rewind(f);
  if (size_long < 0) {
    std::fclose(f);
    return fail(error, "taille illisible : " + path);
  }
  const u64 file_size = static_cast<u64>(size_long);
  dump::detail::Reader r;
  r.f = f;
  Header& h = d.header;
  if (file_size < sizeof(Header) + 8) {
    std::fclose(f);
    return fail(error, "fichier trop court : " + path);
  }
  r.get(&h, sizeof(h));
  const auto refuse = [&](const std::string& what) {
    std::fclose(f);
    return fail(error, what + " : " + path);
  };
  if (!r.ok || h.magic != kMagic || h.version != kVersion) return refuse("en-tete MHGP12TR v1 invalide");
  if (h.producer < kProducerV11 || h.producer > kProducerV12Device) return refuse("producteur inconnu");
  if (h.coord_bits < 1 || h.coord_bits > 32) return refuse("profil hors de [1, 32]");
  if (h.kmax < 1 || h.kmax > 12) return refuse("K hors de [1, 12]");
  if (h.max_leaf < 1 || h.max_leaf > 1024 || h.leaf_size < h.kmax + 3 || h.leaf_size > h.max_leaf)
    return refuse("tailles de feuille hors domaine");
  if (h.n_sites < 1 || h.n_sites >= 0xFFFFFFFFull) return refuse("nombre de sites hors domaine");
  if (h.status > kStatusDepth) return refuse("statut inconnu");
  for (u64 v : h.reserved)
    if (v != 0) return refuse("champ reserve non nul");
  // Taille attendue, chaque terme borne avant multiplication.
  const u64 limit = u64{1} << 40;
  if (h.n_nodes > limit || h.n_leaves > limit || h.n_leaf_sites > limit) return refuse("comptes hors domaine");
  const u64 expected = sizeof(Header) + 3 * section_bytes(h.n_sites, 4) + section_bytes(h.n_nodes, sizeof(Node)) +
                       section_bytes(h.n_leaves, sizeof(Leaf)) + section_bytes(h.n_leaf_sites, 4) + 8;
  if (expected != file_size) return refuse("taille du fichier differente des comptes de l'en-tete");
  r.section(d.x, h.n_sites);
  r.section(d.y, h.n_sites);
  r.section(d.z, h.n_sites);
  r.section(d.nodes, h.n_nodes);
  r.section(d.leaves, h.n_leaves);
  r.section(d.sites, h.n_leaf_sites);
  const u64 computed = r.hash;
  u64 digest = 0;
  const bool tail = r.ok && std::fread(&digest, 1, sizeof(digest), f) == sizeof(digest);
  char extra = 0;
  const bool at_end = std::fread(&extra, 1, 1, f) == 0;
  std::fclose(f);
  if (!r.ok || !tail || !at_end || digest != computed) return fail(error, "vidage tronque ou altere (FNV-1a) : " + path);

  const u64 bits = h.coord_bits, top = u64{1} << bits;  // coordonnees < top, bornes de boites <= top
  for (u64 i = 0; i < h.n_sites; ++i)
    if (d.x[i] >= top || d.y[i] >= top || d.z[i] >= top) return fail(error, "coordonnee hors du profil : " + path);
  // Enveloppe exacte du nuage [min, max+1) : boite d'entree de la racine (v11 : envelope de la liste de la racine).
  i64 cloud_lo[3] = {d.x[0], d.y[0], d.z[0]}, cloud_hi[3] = {d.x[0], d.y[0], d.z[0]};
  for (u64 i = 1; i < h.n_sites; ++i) {
    const i64 c[3] = {d.x[i], d.y[i], d.z[i]};
    for (int a = 0; a < 3; ++a) {
      cloud_lo[a] = c[a] < cloud_lo[a] ? c[a] : cloud_lo[a];
      cloud_hi[a] = c[a] > cloud_hi[a] ? c[a] : cloud_hi[a];
    }
  }
  const u64 max_depth = 3 * bits;
  const auto box_ok = [&](const i64* lo, const i64* hi) {
    for (int a = 0; a < 3; ++a)
      if (lo[a] < 0 || lo[a] >= hi[a] || static_cast<u64>(hi[a]) > top) return false;
    return true;
  };
  // Arbre : reconstruction de l'ordre prefixe par une pile ; enfants = moities de la boite ajustee du parent.
  if (h.status == kStatusOk && h.n_nodes == 0) return fail(error, "parcours vide sans refus : " + path);
  // Refus transactionnel : aucun prefixe publie (ni noeud, ni feuille, ni grand livre).
  if (h.status != kStatusOk && (h.n_nodes != 0 || h.n_leaves != 0 || h.n_leaf_sites != 0 || h.nodes != 0 ||
                                h.leaves != 0 || h.filter_tests != 0 || h.max_depth != 0 || h.max_leaf_seen != 0))
    return fail(error, "refus avec un prefixe publie : " + path);
  struct Open {
    u64 node;
    u32 children;  // enfants deja vus (0, 1, 2)
  };
  std::vector<Open> stack;
  u64 tests = 0, leaf_nodes = 0, deepest = 0, widest = 0;
  for (u64 i = 0; i < h.n_nodes; ++i) {
    const Node& n = d.nodes[i];
    if (n.kind > kKindSplit || n.depth > max_depth || !box_ok(n.lo, n.hi)) return fail(error, "noeud invalide : " + path);
    if (n.count > n.candidates) return fail(error, "noeud : plus de sites retenus que de candidats : " + path);
    if ((n.count == 0) != (n.kind == kKindEmpty) || (n.count == 0 && n.list_fnv != 0))
      return fail(error, "noeud : genre incoherent : " + path);
    if (n.kind == kKindSplit && n.count <= h.leaf_size) return fail(error, "noeud coupe sous la taille de feuille : " + path);
    if (!tests_in_bounds(n.tests, n.candidates, h.kmax))
      return fail(error, "noeud : tests G1 hors de [c min(c, K), c min(c, 3K)] : " + path);
    if (i == 0) {
      if (n.depth != 0 || n.candidates != h.n_sites || n.path[0] != 0 || n.path[1] != 0)
        return fail(error, "racine invalide : " + path);
      for (int a = 0; a < 3; ++a)
        if (n.lo[a] != cloud_lo[a] || n.hi[a] != cloud_hi[a] + 1)
          return fail(error, "racine : boite d'entree differente de l'enveloppe exacte du nuage : " + path);
    } else {
      // le parent est le sommet de pile qui attend encore un enfant
      while (!stack.empty() && stack.back().children == 2) stack.pop_back();
      if (stack.empty()) return fail(error, "noeud sans parent : " + path);
      Open& open = stack.back();
      const Node& p = d.nodes[open.node];
      if (n.depth != p.depth + 1 || n.candidates != p.count) return fail(error, "noeud : profondeur ou candidats : " + path);
      u64 expect[2] = {p.path[0], p.path[1]};
      if (open.children == 1) expect[p.depth / 64] |= u64{1} << (63 - p.depth % 64);
      if (n.path[0] != expect[0] || n.path[1] != expect[1]) return fail(error, "noeud : chemin : " + path);
      if (open.children == 1) {
        // second enfant : le premier est le noeud qui suit le parent
        if (!detail::halves(d.nodes[open.node + 1], n)) return fail(error, "noeud : enfants qui ne sont pas les moities : " + path);
        for (int a = 0; a < 3; ++a)
          if (d.nodes[open.node + 1].lo[a] < p.lo[a] || n.hi[a] > p.hi[a])
            return fail(error, "noeud : boite ajustee hors de la boite d'entree : " + path);
      } else if (open.node + 1 != i) {
        return fail(error, "noeud : premier enfant non contigu : " + path);
      }
      ++open.children;
    }
    if (n.kind == kKindSplit) stack.push_back({i, 0});
    if (n.kind == kKindLeaf) ++leaf_nodes;
    if (!checked_add(tests, n.tests)) return fail(error, "somme des tests G1 hors de u64 : " + path);
    deepest = n.depth > deepest ? n.depth : deepest;
  }
  while (!stack.empty() && stack.back().children == 2) stack.pop_back();
  if (h.status == kStatusOk && !stack.empty()) return fail(error, "noeud coupe sans ses deux enfants : " + path);
  // Feuilles : noeuds feuilles dans l'ordre prefixe, debuts contigus, sites bornes et strictement croissants.
  if (h.status == kStatusOk && leaf_nodes != h.n_leaves) return fail(error, "feuilles et noeuds feuilles different : " + path);
  if (h.n_leaves > leaf_nodes) return fail(error, "feuilles en trop : " + path);
  u64 begin = 0, at = 0;
  for (u64 j = 0; j < h.n_leaves; ++j) {
    const Leaf& l = d.leaves[j];
    if (l.begin != begin || l.m == 0 || l.m > h.n_leaf_sites - begin || l.depth > max_depth || !box_ok(l.lo, l.hi))
      return fail(error, "feuille invalide : " + path);
    while (at < h.n_nodes && d.nodes[at].kind != kKindLeaf) ++at;
    if (at == h.n_nodes) return fail(error, "feuille sans noeud : " + path);
    const Node& n = d.nodes[at++];
    if (n.depth != l.depth || n.count != l.m || n.path[0] != l.path[0] || n.path[1] != l.path[1] ||
        n.list_fnv != list_fnv(d.sites.data() + begin, l.m))
      return fail(error, "feuille differente de son noeud : " + path);
    for (int a = 0; a < 3; ++a)
      if (l.lo[a] < n.lo[a] || l.hi[a] > n.hi[a]) return fail(error, "feuille hors de la boite du noeud : " + path);
    const u32* s = d.sites.data() + begin;
    for (u32 k = 0; k < l.m; ++k)
      if (s[k] >= h.n_sites || (k > 0 && s[k] <= s[k - 1])) return fail(error, "feuille : sites hors du nuage ou non croissants : " + path);
    // Boite ajustee exacte (prepare_node de la v11) : enveloppe [min, max+1) des sites retenus, inter boite d'entree.
    i64 env_lo[3] = {d.x[s[0]], d.y[s[0]], d.z[s[0]]}, env_hi[3] = {d.x[s[0]], d.y[s[0]], d.z[s[0]]};
    for (u32 k = 1; k < l.m; ++k) {
      const i64 c[3] = {d.x[s[k]], d.y[s[k]], d.z[s[k]]};
      for (int a = 0; a < 3; ++a) {
        env_lo[a] = c[a] < env_lo[a] ? c[a] : env_lo[a];
        env_hi[a] = c[a] > env_hi[a] ? c[a] : env_hi[a];
      }
    }
    i64 width = 0;
    for (int a = 0; a < 3; ++a) {
      const i64 lo = env_lo[a] > n.lo[a] ? env_lo[a] : n.lo[a];
      const i64 hi = env_hi[a] + 1 < n.hi[a] ? env_hi[a] + 1 : n.hi[a];
      if (l.lo[a] != lo || l.hi[a] != hi)
        return fail(error, "feuille : boite differente de l'enveloppe de ses sites inter boite du noeud : " + path);
      width = l.hi[a] - l.lo[a] > width ? l.hi[a] - l.lo[a] : width;
    }
    // Feuille seulement si m <= taille de feuille ou largeur <= 1 (run_ready de la v11) : sinon elle aurait ete coupee.
    if (l.m > h.leaf_size && width > 1) return fail(error, "feuille qui devrait etre coupee (m et largeur) : " + path);
    if (l.m > widest) widest = l.m;
    begin += l.m;
  }
  if (begin != h.n_leaf_sites) return fail(error, "sites de feuilles en trop : " + path);
  if (h.status == kStatusOk) {
    if (h.nodes != h.n_nodes || h.leaves != h.n_leaves || h.filter_tests != tests || h.max_depth != deepest ||
        h.max_leaf_seen != widest)
      return fail(error, "grand livre different des sections : " + path);
    for (u64 j = 0; j < h.n_leaves; ++j)
      if (d.leaves[j].m > h.max_leaf) return fail(error, "feuille plus large que max_leaf sans refus : " + path);
  }
  return true;
}

}  // namespace mhgp12::traversal::format
