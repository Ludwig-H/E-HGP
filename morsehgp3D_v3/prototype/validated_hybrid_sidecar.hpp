// MorseHGP3D v3 — LA FACTORY `ValidatedHybridSidecar` (contrat auditeur du
// 10 aout, portes 1-2 de AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811).
//
// LA FRONTIERE DE CONFIANCE TYPEE : un booleen `source_complete_for_order`,
// meme accompagne d'un digest, reste une assertion hostile — le digest lie
// des donnees, il ne prouve pas leur completude. Le sidecar est un objet
// OPAQUE construit apres le tri canonique, PROPRIETAIRE d'un snapshot
// immuable des points et du catalogue (possession par deplacement : la
// fenetre TOCTOU d'un catalogue mute apres validation est fermee par le
// type). Le fold autoritaire recoit `const ValidatedHybridSidecar&` ;
// l'ancienne API brute reste un HARNAIS relatif de juge, jamais le chemin
// public.
//
// La factory execute atomiquement les validations du contrat :
//   1. possession et recalcul de tous les digests ;
//   2. tranches du pool sans chevauchement, trou ni reste ;
//   3. membres/supports tries, uniques, dans le nuage, support inclus ;
//   4. den>0, DOMAINE ABI u16 de la representation (|num|<2^90, den<2^73 —
//      refus AVANT toute arithmetique : INT128_MIN multiplierait en UB),
//      sphere exacte, miniboule des membres EGALE a la boule declaree et
//      saturation fermee COMPLETE (census exact du nuage) ;
//   5. support declare valide semantiquement (sur coquille, cardinal du
//      support minimal recalcule, engendrement, minimalite) ET support
//      canonique RECONSTRUIT par la convention publique — coquille triee par
//      COORDONNEES puis miniboule — jamais recopie de la declaration ;
//   6. rejet de deux handles pour la meme boule exacte, l'index etant trie
//      par (centre reduit, NIVEAU exact, indice) : un doublon non adjacent
//      dans l'ordre du catalogue devient adjacent dans l'ordre des niveaux ;
//   7. ordre final canonique et lots par `sphere_cmp_beta` exact ;
//   8. pour chaque u du support, miniboule de M sans u : STRICT pour tous
//      donne principal ; EGALITE exacte avec un support alternatif excluant
//      u donne non-principal ; calcul incomplet donne UNKNOWN, jamais un
//      faux bit — temoins indexes par PointId supprime, pas par position ;
//   9. derivation de la fermeture par ordre depuis le RECU opaque du
//      producteur (l'enumeration exhaustive achevee sans censure, liee par
//      digests SHA-256) — jamais depuis `smax>=n` lu ailleurs ;
//  10. index injectif et digest final construits seulement apres succes ; le
//      digest final lie le certificat COMPLET : cles de centre, digests de
//      membres, supports canoniques, q_min, etats principaux, TOUTES les
//      RemovalEvidence, maximum_order et les fermetures par ordre.
//
// Digests : SHA-256 contractuel sur serialisation canonique little-endian
// taggee et versionnee (`sidecar_sha256.hpp`) — FNV-1a est retire de toute
// decision de confiance. Le jeton de producteur et le recu sont NON
// TRIVIALEMENT COPIABLES et a constructeur prive : la forge par
// `std::bit_cast` est refusee a la COMPILATION, pas seulement par convention.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/sidecar_sha256.hpp"

namespace mhgp3v {

enum class PrincipalState : std::uint8_t {
  kUnknown = 0,
  kPrincipalCertified = 1,
  kNonPrincipalCertified = 2,
};

enum class CarrierClosure : std::uint8_t { kUnknown = 0, kCertified = 1 };

// La version du schema de serialisation et les tags de section du flux
// canonique : deux flux de sections differentes ne se confondent jamais.
inline constexpr std::uint32_t kSidecarSchemaVersion = 2;
inline constexpr std::uint32_t kSidecarTagPoints = 0x53544e50u;        // "PNTS"
inline constexpr std::uint32_t kSidecarTagCatalogue = 0x474c5443u;     // "CTLG"
inline constexpr std::uint32_t kSidecarTagMembers = 0x53424d4du;       // "MMBS"
inline constexpr std::uint32_t kSidecarTagSidecar = 0x52434453u;       // "SDCR"
inline constexpr std::uint32_t kSidecarTagCertificate = 0x54524543u;   // "CERT"
inline constexpr std::uint32_t kSidecarTagClosure = 0x534f4c43u;       // "CLOS"

// L'IDENTITE DU PRODUCTEUR (audit etat courant, defaut 3) : le recu porte le
// contrat de producteur, le profil d'entree, le schema de taches et le statut
// terminal — un digest lie des donnees, l'identite dit QUI les a produites et
// sous quel contrat. La factory ne certifie la fermeture que pour le contrat
// attendu, au statut terminal kOk.
inline constexpr std::uint32_t kSidecarProducerFlatCatalogueSealed = 0x43544c46u;  // "FLTC"
inline constexpr std::uint32_t kSidecarProfileQuantizedU16 = 0x51363155u;          // "U16Q"
inline constexpr std::uint32_t kSidecarTaskSchemaSequentialV1 = 1;

// GRILLE u16 DECLAREE (audit etat courant, defaut 1) : toute coordonnee du
// nuage ET toute base de sphere sont verifiees dans [0, 65535] AVANT la
// moindre geometrie — `p3_sub` sur des i32 extremes deborderait (UB) dans
// `sphere_side`, et `base*den` deborderait dans la cle. Comparaisons seules.
inline constexpr mhgp::i32 kSidecarGridMaximum = 65535;

inline bool sidecar_point_in_grid(const mhgp::P3& p) {
  return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x <= kSidecarGridMaximum &&
         p.y <= kSidecarGridMaximum && p.z <= kSidecarGridMaximum;
}

// La cle de CENTRE exacte (audit S2, renommee par l'audit delta P1) : le
// carre des numerateurs u16 peut atteindre ~181 bits et DEBORDE i128 — la
// cle ne porte donc QUE le centre rationnel reduit (numerateurs ~90 bits,
// surs en i128), et l'egalite de NIVEAU est deleguee a `sphere_cmp_beta`,
// deja multiprecision (BigInt<6>). Son nom dit ce qu'elle porte : un CENTRE.
// L'egalite de BOULE = egalite de centre reduit ET sphere_cmp_beta == 0.
struct ExactCenterKey {
  mhgp::i128 cx = 0, cy = 0, cz = 0, cden = 0;   // centre reduit, cden > 0
  bool operator==(const ExactCenterKey& other) const {
    return cx == other.cx && cy == other.cy && cz == other.cz && cden == other.cden;
  }
  bool operator<(const ExactCenterKey& other) const {
    if (cx != other.cx) return cx < other.cx;
    if (cy != other.cy) return cy < other.cy;
    if (cz != other.cz) return cz < other.cz;
    return cden < other.cden;
  }
};

struct SidecarSmallSupport {
  std::uint8_t size = 0;
  std::array<mhgp::i32, 4> ids = {0, 0, 0, 0};
};

// Le temoin d'une suppression : pour le PointId `removed` du support, la
// miniboule de M sans lui est STRICTEMENT sous la boule (obligatoire), ou
// EGALE avec un support alternatif excluant `removed` (non-principal).
struct RemovalEvidence {
  mhgp::i32 removed = -1;
  SidecarSmallSupport witness;
  enum class Relation : std::uint8_t { kStrict = 0, kEqual = 1 } relation = Relation::kStrict;
};

struct GeneratorCertificate {
  ExactCenterKey exact_center;
  Sha256Digest members_digest;
  // Le support de la CONVENTION PUBLIQUE, reconstruit par la factory —
  // coquille triee par coordonnees puis miniboule — jamais une recopie du
  // support declare (deux tie-breaks legitimes peuvent differer ; le
  // certificat publie le canonique, la declaration reste jugee
  // semantiquement).
  SidecarSmallSupport canonical_support;
  std::uint8_t q_min = 0;
  PrincipalState principal = PrincipalState::kUnknown;
  std::array<RemovalEvidence, 4> evidence;
  std::uint8_t evidence_count = 0;
};

// LE JETON DE PRODUCTEUR (audit S1, durci par l'audit delta P0) : construit
// par le seul producteur terminal (constructeur prive, amitie), et NON
// TRIVIALEMENT COPIABLE (move user-provided, copie supprimee) — un
// `std::bit_cast<SourceProducerToken>` est desormais refuse a la
// COMPILATION, la cloture nominale est devenue une cloture de TYPE. Le jeton
// est CONSOMME par deplacement par le constructeur du recu.
class SourceProducerToken {
 public:
  SourceProducerToken(const SourceProducerToken&) = delete;
  SourceProducerToken& operator=(const SourceProducerToken&) = delete;
  SourceProducerToken(SourceProducerToken&&) noexcept {}
  SourceProducerToken& operator=(SourceProducerToken&&) noexcept { return *this; }

 private:
  SourceProducerToken() = default;
  friend class SealedSourceProducer;
};

// LE RECU DE SOURCE opaque : construit par le PRODUCTEUR exact, au moment ou
// lui seul sait que l'enumeration est achevee sans censure. Le constructeur
// est PRIVE et possede par le producteur ; le type n'est pas trivialement
// copiable (copie supprimee, move user-provided) : ni forge par
// `std::bit_cast`, ni recopie silencieuse. Les digests SHA-256 lient le recu
// au nuage et au catalogue exacts ; la factory refuse tout recu
// desynchronise.
class HybridSourceReceipt {
 public:
  HybridSourceReceipt(const HybridSourceReceipt&) = delete;
  HybridSourceReceipt& operator=(const HybridSourceReceipt&) = delete;
  // CONSOMMATION PAR DEPLACEMENT effective (audit etat courant, defaut 4) :
  // la source deplacee est INVALIDEE — un pointeur retenu sur elle ne
  // certifie plus jamais une fermeture.
  HybridSourceReceipt(HybridSourceReceipt&& other) noexcept
      : points_digest_(other.points_digest_),
        catalogue_digest_(other.catalogue_digest_),
        producer_version_digest_(other.producer_version_digest_),
        producer_contract_(other.producer_contract_),
        profile_(other.profile_),
        task_schema_(other.task_schema_),
        terminal_status_(other.terminal_status_),
        rank_bound_(other.rank_bound_),
        point_count_(other.point_count_),
        enumeration_completed_(other.enumeration_completed_),
        valid_(other.valid_) {
    other.valid_ = false;
    other.enumeration_completed_ = false;
  }
  HybridSourceReceipt& operator=(HybridSourceReceipt&&) = delete;

  const Sha256Digest& points_digest() const { return points_digest_; }
  const Sha256Digest& catalogue_digest() const { return catalogue_digest_; }
  const Sha256Digest& producer_version_digest() const { return producer_version_digest_; }
  std::uint32_t producer_contract() const { return producer_contract_; }
  std::uint32_t profile() const { return profile_; }
  std::uint32_t task_schema() const { return task_schema_; }
  int terminal_status() const { return terminal_status_; }
  bool claims_complete_family() const {
    return valid_ && enumeration_completed_ && rank_bound_ >= point_count_ &&
           producer_contract_ == kSidecarProducerFlatCatalogueSealed &&
           profile_ == kSidecarProfileQuantizedU16 &&
           task_schema_ == kSidecarTaskSchemaSequentialV1 && terminal_status_ == 0;
  }

 private:
  HybridSourceReceipt(SourceProducerToken&&, const Sha256Digest& points_digest,
                      const Sha256Digest& catalogue_digest,
                      const Sha256Digest& producer_version_digest,
                      std::uint32_t producer_contract, std::uint32_t profile,
                      std::uint32_t task_schema, int terminal_status, int rank_bound,
                      int point_count, bool enumeration_completed)
      : points_digest_(points_digest),
        catalogue_digest_(catalogue_digest),
        producer_version_digest_(producer_version_digest),
        producer_contract_(producer_contract),
        profile_(profile),
        task_schema_(task_schema),
        terminal_status_(terminal_status),
        rank_bound_(rank_bound),
        point_count_(point_count),
        enumeration_completed_(enumeration_completed) {}
  friend class SealedSourceProducer;

  Sha256Digest points_digest_;
  Sha256Digest catalogue_digest_;
  Sha256Digest producer_version_digest_;
  std::uint32_t producer_contract_ = 0;
  std::uint32_t profile_ = 0;
  std::uint32_t task_schema_ = 0;
  int terminal_status_ = -1;
  int rank_bound_ = 0;
  int point_count_ = 0;
  bool enumeration_completed_ = false;
  bool valid_ = true;
};

// Les digests canoniques partages entre le PRODUCTEUR (qui scelle son recu)
// et la factory (qui les recalcule sur le snapshot possede). Serialisation
// champ par champ, little-endian explicite, sections taggees, longueurs
// prefixees — jamais une image memoire (audit S4).
inline Sha256Digest sidecar_points_digest(const std::vector<mhgp::P3>& points) {
  Sha256Stream stream;
  stream.put_tag(kSidecarTagPoints);
  stream.put_u32(kSidecarSchemaVersion);
  stream.put_length(points.size());
  for (const mhgp::P3& p : points) {
    stream.put_i64((long long)p.x);
    stream.put_i64((long long)p.y);
    stream.put_i64((long long)p.z);
  }
  return stream.finalize();
}

inline Sha256Digest sidecar_catalogue_digest(const mhgp::Catalogue& catalogue) {
  Sha256Stream stream;
  stream.put_tag(kSidecarTagCatalogue);
  stream.put_u32(kSidecarSchemaVersion);
  stream.put_length(catalogue.spheres.size());
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) {
    stream.put_i64((long long)sphere.sph.base.x);
    stream.put_i64((long long)sphere.sph.base.y);
    stream.put_i64((long long)sphere.sph.base.z);
    stream.put_i128(sphere.sph.nx);
    stream.put_i128(sphere.sph.ny);
    stream.put_i128(sphere.sph.nz);
    stream.put_i128(sphere.sph.den);
    stream.put_i64((long long)sphere.sph.support);
    stream.put_i64((long long)sphere.rank);
    stream.put_i64((long long)sphere.members_begin);
    stream.put_i64((long long)sphere.n_support);
    for (int u = 0; u < 4; ++u) stream.put_i64((long long)sphere.support[u]);
  }
  stream.put_length(catalogue.members.size());
  for (mhgp::i32 member : catalogue.members) stream.put_i64((long long)member);
  return stream.finalize();
}

inline Sha256Digest sidecar_members_digest(const std::vector<mhgp::i32>& members) {
  Sha256Stream stream;
  stream.put_tag(kSidecarTagMembers);
  stream.put_u32(kSidecarSchemaVersion);
  stream.put_length(members.size());
  for (mhgp::i32 member : members) stream.put_i64((long long)member);
  return stream.finalize();
}

// Les mutants de la factory (contrat §Fixtures) : oubli du dernier u,
// « < » change en « <= », temoin indexe par la POSITION avant permutation,
// acceptation d'une declaration de support NON canonique (le rejet du
// tie-break etranger est la coherence exigee par l'audit etat courant,
// defaut 2 : evidence, digests et fold consomment le meme support), et
// selftest SHA-256 saboté — la factory doit refuser par SON controle
// interne, sans pretest externe de la porte.
struct SidecarMutants {
  bool skip_last_removal = false;
  bool strict_leq = false;
  bool witness_by_position = false;
  bool skip_canonical_check = false;
  bool sha_fault = false;
};

class ValidatedHybridSidecar {
 public:
  // LA FACTORY : rend un sidecar valide, ou un refus avec la raison exacte.
  // Aucun objet partiellement valide n'existe.
  static ValidatedHybridSidecar build(std::vector<mhgp::P3> points, mhgp::Catalogue catalogue,
                                      const HybridSourceReceipt* receipt, int maximum_order,
                                      std::string* refusal, SidecarMutants mutants = {});

  bool ok() const { return ok_; }
  const std::vector<mhgp::P3>& points() const { return points_; }
  const mhgp::Catalogue& catalogue() const { return catalogue_; }
  const Sha256Digest& points_digest() const { return points_digest_; }
  const Sha256Digest& catalogue_digest() const { return catalogue_digest_; }
  const Sha256Digest& sidecar_digest() const { return sidecar_digest_; }
  const std::vector<GeneratorCertificate>& generators() const { return generators_; }
  CarrierClosure closure_for_order(int k) const {
    if (k < 1 || k > (int)closure_by_order_.size()) return CarrierClosure::kUnknown;
    return closure_by_order_[(std::size_t)(k - 1)];
  }
  bool closure_certified_all_orders() const {
    for (CarrierClosure closure : closure_by_order_)
      if (closure != CarrierClosure::kCertified) return false;
    return !closure_by_order_.empty();
  }
  // Les drapeaux `principal` du fold, derives des certificats — le fold
  // autoritaire ne recalcule pas les miniboules de certificat.
  std::vector<char> principal_flags() const {
    std::vector<char> flags(generators_.size(), 0);
    for (std::size_t s = 0; s < generators_.size(); ++s)
      flags[s] = generators_[s].principal == PrincipalState::kPrincipalCertified ? 1 : 0;
    return flags;
  }

 private:
  ValidatedHybridSidecar() = default;
  bool ok_ = false;
  std::vector<mhgp::P3> points_;
  mhgp::Catalogue catalogue_;
  Sha256Digest points_digest_;
  Sha256Digest catalogue_digest_;
  Sha256Digest sidecar_digest_;
  std::vector<GeneratorCertificate> generators_;
  std::vector<int> activation_order_;
  std::vector<int> activation_rank_;
  std::vector<std::uint32_t> batch_offsets_;
  std::vector<CarrierClosure> closure_by_order_;
  std::vector<std::pair<ExactCenterKey, int>> ball_index_;   // trie, injectif
};

// PGCD sur MAGNITUDES NON SIGNEES (audit delta P2) : `-INT128_MIN` n'est pas
// representable en i128 ; la negation passe par u128, ou elle est definie
// pour toute entree. Sur le domaine ABI valide, le resultat tient toujours
// en i128 signe.
inline mhgp::i128 sidecar_gcd(mhgp::i128 a, mhgp::i128 b) {
  using u128 = unsigned __int128;
  u128 ua = a < 0 ? u128(0) - (u128)a : (u128)a;
  u128 ub = b < 0 ? u128(0) - (u128)b : (u128)b;
  while (ub != 0) {
    const u128 r = ua % ub;
    ua = ub;
    ub = r;
  }
  return (mhgp::i128)ua;
}

// DOMAINE ABI DE `Sphere` (audit delta P2) : `sphere.hpp` (v2) borne le
// domaine u16 a |num| <= 2^89.6 et 0 < den <= 2^72.6. Toute representation
// au-dela est HOSTILE et se refuse AVANT la moindre arithmetique —
// `sphere_side`, les miniboules et le pgcd de la cle multiplient des i128 et
// deborderaient (UB) sur nx = INT128_MIN. Ici : comparaisons seules, aucune
// negation, aucun produit.
inline bool sidecar_sphere_abi_ok(const mhgp::Sphere& sphere) {
  const mhgp::i128 numerator_bound = (mhgp::i128)1 << 90;
  const mhgp::i128 denominator_bound = (mhgp::i128)1 << 73;
  const auto magnitude_ok = [&](mhgp::i128 value) {
    return value > -numerator_bound && value < numerator_bound;
  };
  // La BASE est un point de grille comme les autres (audit etat courant,
  // defaut 1) : une base hostile atteindrait `base*den` et `p3_sub`.
  return sidecar_point_in_grid(sphere.base) && sphere.den > 0 &&
         sphere.den < denominator_bound && magnitude_ok(sphere.nx) &&
         magnitude_ok(sphere.ny) && magnitude_ok(sphere.nz);
}

// La cle de centre depuis la representation entiere : (base*den+n)/den
// reduit. AUCUN carre n'est forme (audit S2) : le niveau est compare par
// `sphere_cmp_beta`, multiprecision. Precondition : `sidecar_sphere_abi_ok`.
inline ExactCenterKey exact_center_key(const mhgp::Sphere& sphere) {
  ExactCenterKey key;
  const mhgp::i128 cx = (mhgp::i128)sphere.base.x * sphere.den + sphere.nx;
  const mhgp::i128 cy = (mhgp::i128)sphere.base.y * sphere.den + sphere.ny;
  const mhgp::i128 cz = (mhgp::i128)sphere.base.z * sphere.den + sphere.nz;
  mhgp::i128 den = sphere.den;
  mhgp::i128 g = sidecar_gcd(sidecar_gcd(sidecar_gcd(cx, cy), cz), den);
  if (g == 0) g = 1;
  key.cx = cx / g;
  key.cy = cy / g;
  key.cz = cz / g;
  key.cden = den / g;
  return key;
}

// L'egalite de BOULE exacte : centre reduit egal ET niveau egal (multiprecision).
inline bool same_exact_ball(const mhgp::Sphere& a, const mhgp::Sphere& b) {
  return exact_center_key(a) == exact_center_key(b) && mhgp::sphere_cmp_beta(a, b) == 0;
}

inline ValidatedHybridSidecar ValidatedHybridSidecar::build(
    std::vector<mhgp::P3> points, mhgp::Catalogue catalogue,
    const HybridSourceReceipt* receipt, int maximum_order, std::string* refusal,
    SidecarMutants mutants) {
  ValidatedHybridSidecar sidecar;
  const auto refuse = [&](const char* why) {
    if (refusal != nullptr) *refusal = why;
    sidecar.ok_ = false;
    return sidecar;
  };
  // LE SHA-256 EST JUGE AVANT DE LIER QUOI QUE CE SOIT (audit etat courant,
  // defaut 5) : le controle est EFFECTIF dans la factory, pas seulement dans
  // la porte CTest qui le rejoue. Le mutant sha-fault sabote le verdict du
  // selftest : la factory doit refuser d'elle-meme, sans pretest externe.
  static const bool sha_selftest_ok = sidecar_sha256_selftest();
  if (!sha_selftest_ok || mutants.sha_fault)
    return refuse("selftest SHA-256 refuse : les vecteurs FIPS ne sont pas reproduits");
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank)
    return refuse("ordre maximal hors contrat");
  const int n = (int)points.size();
  if (n < 1) return refuse("nuage vide");
  // GRILLE u16 DECLAREE AVANT TOUTE GEOMETRIE (audit etat courant, defaut
  // 1) : une coordonnee i32 extreme deborderait `p3_sub` dans `sphere_side`.
  for (const mhgp::P3& p : points)
    if (!sidecar_point_in_grid(p))
      return refuse("nuage hors de la grille u16 declaree : refus avant toute geometrie");
  sidecar.points_ = std::move(points);
  sidecar.catalogue_ = std::move(catalogue);
  const std::size_t count = sidecar.catalogue_.spheres.size();
  if (count < 1) return refuse("catalogue vide");

  // 1. DIGESTS recalcules sur le snapshot possede.
  const Sha256Digest pd = sidecar_points_digest(sidecar.points_);
  const Sha256Digest cd = sidecar_catalogue_digest(sidecar.catalogue_);
  sidecar.points_digest_ = pd;
  sidecar.catalogue_digest_ = cd;

  // 2. TRANCHES DU POOL : couvrantes, sans chevauchement, trou ni reste —
  // dans l'ordre physique des spheres du catalogue produit.
  {
    long long cursor = 0;
    for (const mhgp::CriticalSphere& sphere : sidecar.catalogue_.spheres) {
      if (sphere.members_begin != cursor || sphere.rank < 1)
        return refuse("pool chevauche, troue ou desordonne");
      cursor += sphere.rank;
    }
    if (cursor != (long long)sidecar.catalogue_.members.size())
      return refuse("pool avec reste hors tranches");
  }

  // 3+4+5. MEMBRES, DOMAINE ABI, SATURATION COMPLETE, SUPPORT SUR COQUILLE,
  // den > 0, miniboule des membres EGALE a la boule declaree, support
  // canonique RECONSTRUIT.
  std::vector<std::vector<mhgp::i32>> members(count);
  std::vector<SidecarSmallSupport> canonical(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = sidecar.catalogue_.spheres[s];
    if (sphere.sph.den <= 0) return refuse("den nul ou negatif");
    // DOMAINE ABI AVANT TOUTE ARITHMETIQUE (audit delta P2) : une
    // representation hostile — INT128_MIN, den au-dela du domaine u16 —
    // atteindrait sinon une multiplication i128 en UB dans `sphere_side` ou
    // une negation en UB dans le pgcd. Ici : refus par comparaisons seules.
    if (!sidecar_sphere_abi_ok(sphere.sph))
      return refuse("representation de sphere hors du domaine u16 : refus avant toute"
                    " arithmetique");
    members[s].assign(sidecar.catalogue_.members.begin() + sphere.members_begin,
                      sidecar.catalogue_.members.begin() + sphere.members_begin + sphere.rank);
    for (std::size_t t = 0; t < members[s].size(); ++t) {
      if (members[s][t] < 0 || members[s][t] >= n) return refuse("membre hors du nuage");
      if (t > 0 && members[s][t - 1] >= members[s][t])
        return refuse("membres non tries ou dupliques");
    }
    const int q = (int)sphere.n_support;
    if (q < 1 || q > std::min(mhgp::kMaxSupport, (int)sphere.rank))
      return refuse("support hors contrat : cardinal invalide");
    // COHERENCE DU CHAMP INTERNE (audit S3) : la taille de support portee par
    // la representation `Sphere` doit egaler le cardinal declare.
    if (sphere.sph.support != q)
      return refuse("champ support de la sphere incoherent avec n_support");
    for (int u = 0; u < q; ++u) {
      if (u > 0 && sphere.support[u - 1] >= sphere.support[u])
        return refuse("support non trie ou duplique");
      if (!std::binary_search(members[s].begin(), members[s].end(), sphere.support[u]))
        return refuse("support hors des membres");
      if (mhgp::sphere_side(sphere.sph, sidecar.points_[(std::size_t)sphere.support[u]]) != 0)
        return refuse("support hors de la coquille : le certificat geometrique ment");
    }
    // SATURATION FERMEE COMPLETE : census exact du nuage entier — un point de
    // la boule fermee absent des membres est une censure, refusee. La
    // COQUILLE (side == 0) est collectee au passage pour la reconstruction
    // canonique.
    long long inside = 0;
    std::vector<mhgp::i32> shell;
    for (int p = 0; p < n; ++p) {
      const int side = mhgp::sphere_side(sphere.sph, sidecar.points_[(std::size_t)p]);
      if (side <= 0) ++inside;
      if (side == 0) shell.push_back((mhgp::i32)p);
    }
    if (inside != (long long)members[s].size())
      return refuse("saturation incomplete : le census ferme contredit les membres");
    // MINIBOULE DES MEMBRES == BOULE DECLAREE, exactement.
    const mhgp::MiniballResult recomputed =
        mhgp::miniball_of(sidecar.points_, members[s].data(), (int)members[s].size());
    if (!recomputed.ok || !same_exact_ball(recomputed.sph, sphere.sph))
      return refuse("la boule declaree n'est pas la miniboule exacte de ses membres");
    // SUPPORT PROPRE RECALCULE (audit S3) : le support declare doit (i) avoir
    // le CARDINAL du support minimal recalcule independamment — q_min est
    // RECALCULE, pas recopie — (ii) ENGENDRER exactement la boule declaree,
    // et (iii) etre MINIMAL : aucun sous-ensemble propre n'engendre la boule.
    // Deux supports minimaux distincts d'une meme boule cospherique restent
    // tous deux valides : l'egalite d'identifiants n'est pas exigee (le
    // tie-break du producteur est libre), la validite et la minimalite le
    // sont — un support redondant ou non generateur est refuse.
    {
      if ((int)sphere.n_support != recomputed.n_support)
        return refuse("support declare : cardinal different du support minimal recalcule");
      std::array<mhgp::i32, 4> declared = {-1, -1, -1, -1};
      const int q_declared = (int)sphere.n_support;
      for (int u = 0; u < q_declared; ++u) declared[(std::size_t)u] = sphere.support[u];
      const mhgp::MiniballResult spanned =
          mhgp::miniball_of(sidecar.points_, declared.data(), q_declared);
      if (!spanned.ok || !same_exact_ball(spanned.sph, sphere.sph))
        return refuse("support declare : il n'engendre pas la boule declaree");
      if (q_declared > 1) {
        std::array<mhgp::i32, 4> subset = {-1, -1, -1, -1};
        for (int drop = 0; drop < q_declared; ++drop) {
          int cursor = 0;
          for (int u = 0; u < q_declared; ++u)
            if (u != drop) subset[(std::size_t)cursor++] = sphere.support[u];
          const mhgp::MiniballResult sub =
              mhgp::miniball_of(sidecar.points_, subset.data(), q_declared - 1);
          if (sub.ok && same_exact_ball(sub.sph, sphere.sph))
            return refuse("support declare : non minimal, un sous-ensemble engendre la"
                          " boule");
        }
      }
    }
    // SUPPORT CANONIQUE RECONSTRUIT (audit delta, porte 2) : la convention
    // publique du catalogue — coquille triee par COORDONNEES puis miniboule,
    // l'ordre d'enumeration interne realisant le tie-break — est REJOUEE ici
    // sur la coquille du census, jamais recopiee de la declaration. La
    // miniboule de la coquille EST la boule (coquille dans la boule fermee et
    // support dans la coquille : encadrement des rayons puis unicite) ; toute
    // divergence est une contradiction arithmetique, refusee.
    {
      std::vector<mhgp::i32> by_coordinate = shell;
      std::sort(by_coordinate.begin(), by_coordinate.end(),
                [&](mhgp::i32 x, mhgp::i32 y) {
                  const mhgp::P3& u = sidecar.points_[(std::size_t)x];
                  const mhgp::P3& w = sidecar.points_[(std::size_t)y];
                  if (u.x != w.x) return u.x < w.x;
                  if (u.y != w.y) return u.y < w.y;
                  if (u.z != w.z) return u.z < w.z;
                  return x < y;
                });
      const mhgp::MiniballResult canonical_mb =
          mhgp::miniball_of(sidecar.points_, by_coordinate.data(), (int)by_coordinate.size());
      if (!canonical_mb.ok || !same_exact_ball(canonical_mb.sph, sphere.sph))
        return refuse("coquille incoherente : sa miniboule n'est pas la boule declaree");
      if (canonical_mb.n_support != q)
        return refuse("support canonique reconstruit : cardinal contradictoire");
      SidecarSmallSupport& canon = canonical[s];
      canon.size = (std::uint8_t)canonical_mb.n_support;
      for (int u = 0; u < canonical_mb.n_support && u < 4; ++u)
        canon.ids[(std::size_t)u] = canonical_mb.support[u];
      std::sort(canon.ids.begin(), canon.ids.begin() + canon.size);
      // COHERENCE DU TIE-BREAK (audit etat courant, defaut 2) : l'oracle
      // borne REJETTE toute declaration differant du support canonique — les
      // RemovalEvidence, le digest et le fold consomment ainsi UN SEUL
      // support, jamais deux tie-breaks incoherents.
      if (!mutants.skip_canonical_check) {   // MUTANT : tie-break etranger accepte
        for (int u = 0; u < q; ++u)
          if (canon.ids[(std::size_t)u] != sphere.support[u])
            return refuse("support declare : tie-break non canonique (la convention"
                          " est la coquille triee par coordonnees)");
      }
    }
  }

  // 6. UNICITE DES HANDLES (audit delta P1) : l'index est trie par (centre
  // reduit, NIVEAU exact par `sphere_cmp_beta`, indice) — un doublon separe
  // par un rayon distinct dans l'ordre du catalogue ([r1,r2,r1]) devient
  // adjacent dans l'ordre des niveaux et se refuse. Les concentriques de
  // rayons distincts partagent la cle de centre et restent acceptees.
  sidecar.ball_index_.reserve(count);
  for (std::size_t s = 0; s < count; ++s)
    sidecar.ball_index_.push_back({exact_center_key(sidecar.catalogue_.spheres[s].sph), (int)s});
  std::sort(sidecar.ball_index_.begin(), sidecar.ball_index_.end(),
            [&](const std::pair<ExactCenterKey, int>& a, const std::pair<ExactCenterKey, int>& b) {
              if (!(a.first == b.first)) return a.first < b.first;
              const int c = mhgp::sphere_cmp_beta(
                  sidecar.catalogue_.spheres[(std::size_t)a.second].sph,
                  sidecar.catalogue_.spheres[(std::size_t)b.second].sph);
              if (c != 0) return c < 0;
              return a.second < b.second;
            });
  for (std::size_t i = 1; i < sidecar.ball_index_.size(); ++i)
    if (sidecar.ball_index_[i].first == sidecar.ball_index_[i - 1].first &&
        mhgp::sphere_cmp_beta(
            sidecar.catalogue_.spheres[(std::size_t)sidecar.ball_index_[i].second].sph,
            sidecar.catalogue_.spheres[(std::size_t)sidecar.ball_index_[i - 1].second].sph) ==
            0)
      return refuse("deux handles pour la meme boule exacte");

  // 7. ORDRE D'ACTIVATION canonique et lots par niveau exact.
  sidecar.activation_order_.resize(count);
  for (std::size_t s = 0; s < count; ++s) sidecar.activation_order_[s] = (int)s;
  std::sort(sidecar.activation_order_.begin(), sidecar.activation_order_.end(),
            [&](int x, int y) {
              const int c = mhgp::sphere_cmp_beta(sidecar.catalogue_.spheres[(std::size_t)x].sph,
                                                  sidecar.catalogue_.spheres[(std::size_t)y].sph);
              if (c != 0) return c < 0;
              return x < y;
            });
  sidecar.activation_rank_.resize(count);
  for (std::size_t i = 0; i < count; ++i)
    sidecar.activation_rank_[(std::size_t)sidecar.activation_order_[i]] = (int)i;
  sidecar.batch_offsets_.push_back(0);
  for (std::size_t i = 1; i <= count; ++i) {
    if (i == count ||
        mhgp::sphere_cmp_beta(
            sidecar.catalogue_.spheres[(std::size_t)sidecar.activation_order_[i - 1]].sph,
            sidecar.catalogue_.spheres[(std::size_t)sidecar.activation_order_[i]].sph) != 0)
      sidecar.batch_offsets_.push_back((std::uint32_t)i);
    if (i == count) break;
  }

  // 8. CERTIFICATS PRINCIPAUX par temoins de suppression, indexes par le
  // PointId supprime. Strict pour TOUS les u -> principal ; une egalite
  // exacte -> non-principal (le support alternatif exclut u par le census
  // deja valide) ; jamais un faux bit.
  sidecar.generators_.resize(count);
  std::vector<mhgp::i32> scratch;
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = sidecar.catalogue_.spheres[s];
    GeneratorCertificate& certificate = sidecar.generators_[s];
    certificate.exact_center = exact_center_key(sphere.sph);
    certificate.members_digest = sidecar_members_digest(members[s]);
    certificate.canonical_support = canonical[s];
    const int q = (int)sphere.n_support;
    certificate.q_min = (std::uint8_t)q;
    if ((int)members[s].size() <= q) {
      // M == U : aucun support alternatif possible, principal sans temoin.
      certificate.principal = PrincipalState::kPrincipalCertified;
      certificate.evidence_count = 0;
      continue;
    }
    bool all_strict = true;
    bool any_equal = false;
    certificate.evidence_count = 0;
    const int removals = mutants.skip_last_removal ? q - 1 : q;   // MUTANT : oubli du dernier u
    for (int u = 0; u < removals; ++u) {
      scratch.clear();
      for (mhgp::i32 x : members[s])
        if (x != sphere.support[u]) scratch.push_back(x);
      const mhgp::MiniballResult removed =
          mhgp::miniball_of(sidecar.points_, scratch.data(), (int)scratch.size());
      if (!removed.ok) {
        certificate.principal = PrincipalState::kUnknown;
        all_strict = false;
        any_equal = false;
        break;
      }
      RemovalEvidence& evidence = certificate.evidence[(std::size_t)certificate.evidence_count];
      // MUTANT : la position avant permutation n'est PAS un identifiant.
      evidence.removed = mutants.witness_by_position ? (mhgp::i32)u : sphere.support[u];
      const int cmp = mhgp::sphere_cmp_beta(removed.sph, sphere.sph);
      if (cmp < 0 || (mutants.strict_leq && cmp == 0)) {   // MUTANT : < change en <=
        evidence.relation = RemovalEvidence::Relation::kStrict;
      } else if (cmp == 0 && exact_center_key(removed.sph) == certificate.exact_center) {
        evidence.relation = RemovalEvidence::Relation::kEqual;
        all_strict = false;
        any_equal = true;
      } else {
        // Une miniboule PLUS GRANDE que la boule des membres complets est une
        // contradiction arithmetique : refus, pas un bit.
        return refuse("temoin de suppression incoherent : miniboule au-dessus de la boule");
      }
      evidence.witness.size = (std::uint8_t)removed.n_support;
      for (int w = 0; w < removed.n_support && w < 4; ++w)
        evidence.witness.ids[(std::size_t)w] = removed.support[w];
      ++certificate.evidence_count;
    }
    if (all_strict && certificate.evidence_count == q)
      certificate.principal = PrincipalState::kPrincipalCertified;
    else if (any_equal)
      certificate.principal = PrincipalState::kNonPrincipalCertified;
  }

  // 9. FERMETURE PAR ORDRE : depuis le recu opaque SEULEMENT, lie par
  // digests SHA-256, au contrat de producteur attendu et au statut terminal
  // kOk (l'identite est verifiee dans `claims_complete_family`). Un recu
  // absent, desynchronise, deplace ou d'un autre contrat laisse kUnknown.
  sidecar.closure_by_order_.assign((std::size_t)maximum_order, CarrierClosure::kUnknown);
  const bool closure_from_receipt =
      receipt != nullptr && receipt->points_digest() == sidecar.points_digest_ &&
      receipt->catalogue_digest() == sidecar.catalogue_digest_ &&
      receipt->claims_complete_family();
  if (closure_from_receipt) {
    for (std::size_t k = 0; k < sidecar.closure_by_order_.size(); ++k)
      sidecar.closure_by_order_[k] = CarrierClosure::kCertified;
  }

  // 10. DIGEST FINAL, seulement apres succes. Il lie la DECISION COMPLETE
  // (audit delta, porte 2) : version de schema, digests points/catalogue,
  // maximum_order, et pour chaque generateur la cle de centre, le digest des
  // membres, le support canonique reconstruit, q_min, l'etat principal et
  // TOUTES les RemovalEvidence — puis les fermetures par ordre.
  {
    Sha256Stream stream;
    stream.put_tag(kSidecarTagSidecar);
    stream.put_u32(kSidecarSchemaVersion);
    stream.put_digest(pd);
    stream.put_digest(cd);
    stream.put_u32((std::uint32_t)maximum_order);
    stream.put_length(sidecar.generators_.size());
    for (const GeneratorCertificate& certificate : sidecar.generators_) {
      stream.put_tag(kSidecarTagCertificate);
      stream.put_i128(certificate.exact_center.cx);
      stream.put_i128(certificate.exact_center.cy);
      stream.put_i128(certificate.exact_center.cz);
      stream.put_i128(certificate.exact_center.cden);
      stream.put_digest(certificate.members_digest);
      stream.put_u8(certificate.canonical_support.size);
      for (int u = 0; u < 4; ++u)
        stream.put_i64((long long)certificate.canonical_support.ids[(std::size_t)u]);
      stream.put_u8(certificate.q_min);
      stream.put_u8((std::uint8_t)certificate.principal);
      stream.put_u8(certificate.evidence_count);
      for (int e = 0; e < (int)certificate.evidence_count; ++e) {
        const RemovalEvidence& evidence = certificate.evidence[(std::size_t)e];
        stream.put_i64((long long)evidence.removed);
        stream.put_u8((std::uint8_t)evidence.relation);
        stream.put_u8(evidence.witness.size);
        for (int w = 0; w < 4; ++w)
          stream.put_i64((long long)evidence.witness.ids[(std::size_t)w]);
      }
    }
    stream.put_tag(kSidecarTagClosure);
    stream.put_length(sidecar.closure_by_order_.size());
    for (CarrierClosure closure : sidecar.closure_by_order_)
      stream.put_u8((std::uint8_t)closure);
    // L'identite du producteur certifiant est LIEE (defaut 3) : deux
    // producteurs distincts ne partagent jamais un digest de fermeture.
    if (closure_from_receipt) {
      stream.put_u32(receipt->producer_contract());
      stream.put_u32(receipt->profile());
      stream.put_u32(receipt->task_schema());
      stream.put_i64((long long)receipt->terminal_status());
      stream.put_digest(receipt->producer_version_digest());
    } else {
      stream.put_u32(0);
      stream.put_u32(0);
      stream.put_u32(0);
      stream.put_i64(-1);
      stream.put_digest(Sha256Digest{});
    }
    sidecar.sidecar_digest_ = stream.finalize();
  }
  sidecar.ok_ = true;
  if (refusal != nullptr) refusal->clear();
  return sidecar;
}

}  // namespace mhgp3v
