// En-tete public de supports : supports positifs minimaux Q_b des boules du catalogue et comptes exacts du lemme G
// (sortie parametree, tranche S6a), et leur assemblage en hierarchie des supports d'ordre K sur l'arbre d'ordre K et
// le rattachement de la tour (SupportHierarchy, build_support_hierarchy, tranche S6b). Un autre module n'inclut que ce
// fichier.
//
// Q_b = { Q inclus dans U_b : Q affinement independant, c_b dans l'interieur relatif de conv(Q), 2 <= |Q| <= 4 } : les
// parties non separables MINIMALES de la coquille (lemme F, Caratheodory strict ; docs/MATHEMATIQUES.md, M1). Q_b est
// enumere sur TOUTE la coquille, jamais limite a qmin ni filtre par un compte de cofaces, par les trois predicats
// stricts publics de num : is_midpoint (|Q| = 2), strictly_acute puis orientation nulle du centre (|Q| = 3),
// strictly_inside (|Q| = 4). Aucun test d'independance ni de minimalite n'est necessaire : un triangle droit n'est
// pas un support (son hypotenuse l'est), et le drapeau q4_presentation_strictly_inside de num::Sphere ne vaut que pour
// le tetraedre generateur, jamais pour un autre quadruplet. Ordre publie : (arite, ordre lexicographique des SiteIdx),
// donc S* en tete (son plus petit cardinal est qmin).
#pragma once

#include <array>
#include <span>

#include "num/num.hpp"
#include "supports/counts.hpp"
#include "tower/tower.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::supports {

// Support positif minimal d'une boule : SiteIdx croissants, kNone au-dela de l'arite.
struct Support {
  std::array<SiteIdx, 4> sites{};
  u8 arity = 0;  // 2..4
  friend bool operator==(const Support&, const Support&) = default;
};

// Nombre de parties de 2 a 4 sites d'une coquille de m <= kMaxShell sites : majorant de |Q_b|.
constexpr u32 support_capacity(u32 m) noexcept {
  const i32 x = static_cast<i32>(m);
  return supports_detail::binomial(x, 2) + supports_detail::binomial(x, 3) + supports_detail::binomial(x, 4);
}
inline constexpr u32 kMaxSupports = support_capacity(kMaxShell);
static_assert(kMaxSupports == 12926, "supports : C(24,2) + C(24,3) + C(24,4)");

// Mots u64 du brouillon de fermeture d'une coquille etendue de m sites : 2^(m-6), un seul si m <= 6 ; 0 au-dela du
// plafond, qui n'a pas de brouillon (fonction totale : aucun decalage hors domaine).
constexpr u64 closure_words(u32 m) noexcept { return m > kMaxShell ? 0 : u64{1} << (m > 6 ? m - 6 : 0); }
inline constexpr u64 kMaxClosureWords = closure_words(kMaxShell);
static_assert(kMaxClosureWords * sizeof(u64) == (u64{2} << 20), "supports : brouillon de 2 Mio a m = 24");

// Plafond des coquilles etendues : refus support_shell_capacity si m > kMaxShell. Seul controle du plafond :
// ball_supports, make_shape et les pre-passes d'un appel entier l'appellent avant tout calcul.
[[nodiscard]] Outcome check_shell(u32 m) noexcept;

// Travail cumule par ball_supports, sur succes seulement (mesure et registres ; jamais une decision). Aucune
// protection contre la concurrence : add n'est pas atomique, un registre partage entre fils est une course. Un
// registre par fil, sommes par add apres la jointure (porte concurrency).
struct SupportLedger {
  u64 balls = 0, regular = 0, extended = 0, supports = 0;
  u64 midpoint_tests = 0, acute_tests = 0, orientation_tests = 0, inside_tests = 0;
  constexpr void add(const SupportLedger& part) noexcept {
    balls += part.balls;
    regular += part.regular;
    extended += part.extended;
    supports += part.supports;
    midpoint_tests += part.midpoint_tests;
    acute_tests += part.acute_tests;
    orientation_tests += part.orientation_tests;
    inside_tests += part.inside_tests;
  }
  friend bool operator==(const SupportLedger&, const SupportLedger&) = default;
};

struct BallSupports {
  u32 count = 0;    // |Q_b|, ecrits dans out[0, count)
  Closure closure;  // N_0 .. N_m
};

// Q_b d'une boule du catalogue du domaine, ecrit dans out[0, count) dans l'ordre publie (out[0] = S*), et sa fermeture.
// Coquille reguliere (m = qmin) : {S*} et N_j = [j = qmin], sans sphere ni predicat. Coquille etendue : plafond
// (check_shell) ; sphere refaite depuis S* (Sphere::through de l'arite qmin) et niveau exactement egal a celui du
// catalogue ; paires, triplets puis quadruplets de positions de U_b ; premier support egal a S* ; fermeture zeta en OU
// dans scratch[0, closure_words(m)), puis N_j. Q_b ne depend pas de K : aucun support n'est retire.
// Refus, avant toute ecriture pour les deux premiers : parameter_out_of_range (BallIdx hors du catalogue) ;
// support_shell_capacity (m > kMaxShell) ; supports_invariant (scratch trop court, plus de out.size() supports ; puis,
// gardes sans porte possible sur un domaine prepare, voir enumerate.cpp : champs du catalogue incoherents, S*
// degenere, niveau different, premier support different de S*) ; refus arithmetiques de num (sans porte possible
// non plus). Aucune allocation : out et scratch appartiennent a l'appelant (un brouillon par fil), leur contenu est
// indetermine apres un refus. Le domaine est lu en partage : appels concurrents permis sur des tampons distincts et
// des registres distincts. Le registre facultatif n'est pas protege (SupportLedger) : un registre par fil, sommes par
// SupportLedger::add apres la jointure, jamais un registre partage entre fils.
[[nodiscard]] Result<BallSupports> ball_supports(const FullDomain& domain, BallIdx ball, std::span<Support> out,
                                                 std::span<u64> scratch, SupportLedger* ledger = nullptr) noexcept;

// Forme d'une boule du domaine a l'ordre K, par make_shape (counts.hpp). Refus : parameter_out_of_range (BallIdx hors
// du catalogue) ; puis ceux de make_shape : support_shell_capacity (m > kMaxShell, par check_shell) et
// supports_invariant (forme hors du domaine de Shape : K hors de 1..kMaxOrder, ou boule hors de Cat_K a cet ordre,
// p + qmin > K + 1 ; les autres clauses de ce domaine sont des invariants du catalogue).
[[nodiscard]] Result<Shape> ball_shape(const FullDomain& domain, BallIdx ball, Order k) noexcept;

// ---------------------------------------------------------------- assemblage (tranche S6b)

namespace supports_detail {
struct Assembly;
}

// Boule publiee de W_K : identite, rattachement de la tour, forme, et comptes du lemme G calcules en memoire (jamais
// stockes dans MHGP11SP). strict_traces est C(m, t) - N_t, egal au journal du constructeur (contre-epreuve) ;
// components est |ant(b)| du rattachement (0 naissance, 1 interne).
struct Ball {
  BallIdx key;      // boule du catalogue
  NodeIdx node;     // att(b), coupe FERMEE
  LevelRank rank;   // lambda_b
  u32 kparties_reliees = 0, compressed_parts = 0, strict_traces = 0, cofaces = 0, gabriel_cofaces = 0;
  u32 components = 0;
  BallRole role = BallRole::birth;
  u8 p = 0, m = 0, qmin = 0;
  friend bool operator==(const Ball&, const Ball&) = default;
};

// Diagnostics d'un assemblage, publies au succes seulement (mesure, jamais une decision).
//   tree_ns  : pre-passe, admission, postordre et tri des boules ; count_ns, fill_ns : les deux passes de Q_b
//   workers  : fils actifs (taille du Pool, appelant compris ; 1 sans Pool) ; widest : plus grande coquille etendue
//   admitted : octets admis avant la passe count (premier) et avant la passe fill (second), formule de l'en-tete
struct HierarchyTimings {
  u64 tree_ns = 0, count_ns = 0, fill_ns = 0;
  u64 workers = 0, widest = 0;
  std::array<u64, 2> admitted{};
  friend bool operator==(const HierarchyTimings&, const HierarchyTimings&) = default;
};

// Hierarchie des supports d'ordre K (docs/SORTIES.md, section 6 ; MATHEMATIQUES.md, section 10). Deplacement seulement.
// Les SiteIdx des supports et les NodeIdx se rapportent a l'OrderTree source, qui doit survivre a leur lecture.
//   post, subtree_size : NodeIdx -> rang de postordre (racine en dernier, enfants par NodeIdx croissant), taille
//   ball_offsets       : N + 1 decalages, indexes par RANG DE POSTORDRE : les boules propres du noeud de rang j sont
//                        balls()[ball_offsets[j], ball_offsets[j + 1]) ; un sous-arbre est une tranche contigue
//   balls              : W_K entiere, ordre (postordre du noeud de rattachement, rang, BallIdx)
//   support_offsets, supports, support_cofaces : Q_b de chaque boule (B + 1 decalages), ordre (arite, SiteIdx
//                        lexicographiques), donc S* en tete ; incidences (Q, G) de chaque support
//   prior_offsets, prior : ant(b) croissants (B + 1 decalages), role fusion seulement
//   ledger             : travail de ball_supports pendant la passe count (une enumeration par boule)
class SupportHierarchy {
 public:
  SupportHierarchy(const SupportHierarchy&) = delete;
  SupportHierarchy& operator=(const SupportHierarchy&) = delete;
  SupportHierarchy& operator=(SupportHierarchy&&) = delete;
  SupportHierarchy(SupportHierarchy&&) noexcept = default;
  Order order() const noexcept { return order_; }
  NodeIdx root() const noexcept { return root_; }
  std::span<const u32> post() const noexcept { return post_.span(); }
  std::span<const u32> subtree_size() const noexcept { return size_.span(); }
  std::span<const u64> ball_offsets() const noexcept { return ball_offsets_.span(); }
  std::span<const Ball> balls() const noexcept { return balls_.span(); }
  std::span<const u64> support_offsets() const noexcept { return support_offsets_.span(); }
  std::span<const Support> supports() const noexcept { return supports_.span(); }
  std::span<const u32> support_cofaces() const noexcept { return support_cofaces_.span(); }
  std::span<const u64> prior_offsets() const noexcept { return prior_offsets_.span(); }
  std::span<const NodeIdx> prior() const noexcept { return prior_.span(); }
  const SupportLedger& ledger() const noexcept { return ledger_; }

 private:
  friend struct supports_detail::Assembly;
  SupportHierarchy() noexcept = default;
  Buffer<u32> post_, size_;
  Buffer<u64> ball_offsets_;
  Buffer<Ball> balls_;
  Buffer<u64> support_offsets_;
  Buffer<Support> supports_;
  Buffer<u32> support_cofaces_;
  Buffer<u64> prior_offsets_;
  Buffer<NodeIdx> prior_;
  SupportLedger ledger_;
  Order order_ = 0;
  NodeIdx root_{kNone};
};

// Octets admis par build_support_hierarchy (formule normative, recalculee par les portes). N noeuds, B = |W_K|,
// A = |prior| du rattachement, W fils actifs, w la plus grande coquille etendue de W_K (0 s'il n'y en a aucune).
//   avant la passe count : retenus 4N + 4N + 8(N+1) + sizeof(Ball) B + 8(B+1) + 8(B+1) + 4A, temporaire 4B (boule
//                          d'origine de chaque position), et PAR FIL ACTIF sizeof(SupportLedger), le brouillon de
//                          fermeture 8 closure_words(w) (0 si w = 0) et la liste temporaire de supports
//                          sizeof(Support) support_capacity(w) entrees (1 si w = 0) ;
//   avant la passe fill  : (sizeof(Support) + 4) S, S = somme des |Q_b|, tous les tampons precedents vivants.
struct HierarchyAdmission {
  u64 nodes = 0, balls = 0, prior = 0, workers = 0, widest = 0;
  constexpr u64 per_worker() const noexcept {
    return sizeof(SupportLedger) + (widest == 0 ? 0 : sizeof(u64) * closure_words(static_cast<u32>(widest))) +
           sizeof(Support) * (widest == 0 ? 1 : support_capacity(static_cast<u32>(widest)));
  }
  constexpr u64 first() const noexcept {
    return 8 * nodes + 8 * (nodes + 1) + sizeof(Ball) * balls + 16 * (balls + 1) + 4 * prior + 4 * balls +
           workers * per_worker();
  }
  static constexpr u64 second(u64 supports) noexcept { return (sizeof(Support) + sizeof(u32)) * supports; }
};

// Assemblage de la hierarchie des supports de l'arbre d'ordre K (docs/SORTIES.md, section 6) :
//   1. pre-passe sur W_K : plafond check_shell de TOUTES les coquilles etendues, avant toute allocation et tout calcul
//      de Q_b ; un refus support_shell_capacity vaut pour l'appel entier ;
//   2. admission du premier etage (HierarchyAdmission::first), puis postordre iteratif depuis la racine (enfants par
//      NodeIdx croissant, sans pile : le curseur d'enfant vit dans post), tailles de sous-arbre ; tri des boules par
//      seaux stables selon le postordre de leur noeud (dans un seau, l'ordre des BallIdx, donc des rangs) ;
//   3. passe count, parallele par tranches de positions : metadonnees (rattachement, catalogue), Q_b par
//      ball_supports dans le brouillon du fil, comptes du lemme G, contre-epreuve strict_traces = journal (sinon
//      supports_invariant) ; |Q_b| a sa position ;
//   4. sommes prefixes verifiees des decalages (u64), admission du second etage, passe fill a positions fixes : la
//      meme position designe la meme boule (meme BallIdx) qu'en count ; le nombre de supports doit etre le meme
//      (sinon supports_invariant), tri (arite, SiteIdx), incidences par support, branches ant(b) recopiees.
// Sorties identiques a l'octet quel que soit le Pool. Refus : support_shell_capacity ; memory_budget (admission,
// avant les taches, ou allocation) ; supports_invariant (arbre ou rattachement incoherents, contre-epreuve) ; ceux de
// ball_supports, make_shape, ball_counts ; ceux du Pool (pool_busy...). Aucun resultat partiel : sur refus, rien n'est
// publie, les reservations de l'appel sont rendues et *timings reste intact.
//
// Selection (decision de l'utilisateur du 6 octobre 2026 : "seulement les supports associes au minimum spanning tree
// de niveau K" ; aretes de Kruskal, S* seul) :
//   all      : W_K entiere (naissances, fusions, liaisons internes) et Q_b entier de chaque boule, comme ci-dessus ;
//   spanning : arbre couvrant d'ordre K. Naissances conservees, puis cellules de fusion retenues par Kruskal stable
//              (BallIdx/rang croissants) sur les branches
//              ouvertes de chaque plateau ; une cellule est retenue s'il reste au moins une union a faire.
//              La connexion finale des enfants de chaque multifusion est verifiee. Les roles internal et les cycles
//              entre roles merge du meme plateau sont exclus. Chaque boule
//              retenue porte S* seul (arite qmin), lu dans le catalogue. Aucune enumeration de Q_b, donc ni plafond de 24 sites ni brouillon de fermeture (widest
//              = 0) ; comptes du lemme G et incidences par support non calcules (nuls) ; seul plafond : m <= 255
//              (colonne u8 du fichier), sinon support_shell_capacity. C'est la selection de la sortie publiee.
//              Selection : admission B_all*sizeof(u8)+2*N*sizeof(u32) puis allocations budgetees du masque et du
//              DSU ; DSU rendu avant HierarchyAdmission::first, masque rendu apres sort_balls avant count. L'admission
//              first est supplementaire, le masque vivant etant deja compte dans MemoryBudget ; aucune allocation
//              supplementaire pour all. prior publie reste ant(b) original, jamais les racines DSU temporaires.
enum class Selection : u8 { all, spanning };
[[nodiscard]] Result<SupportHierarchy> build_support_hierarchy(const OrderTree& tree, MemoryBudget& budget,
                                                               sched::Pool* pool = nullptr,
                                                               HierarchyTimings* timings = nullptr,
                                                               Selection selection = Selection::all) noexcept;

}  // namespace mhgp11::supports
