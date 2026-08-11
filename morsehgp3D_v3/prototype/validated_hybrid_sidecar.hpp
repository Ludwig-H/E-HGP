// MorseHGP3D v3 — LA FACTORY `ValidatedHybridSidecar` (contrat auditeur du
// 10 aout, decision fast ex aequo du 11 aout).
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
// La factory execute atomiquement les dix validations du contrat :
//   1. possession et recalcul de tous les digests ;
//   2. tranches du pool sans chevauchement, trou ni reste ;
//   3. membres/supports tries, uniques, dans le nuage, support inclus ;
//   4. den>0, sphere exacte, miniboule des membres EGALE a la boule declaree
//      et saturation fermee COMPLETE (census exact du nuage) ;
//   5. support canonique geometrique verifie (chaque point du support est
//      SUR la coquille) et q_min recalcule ;
//   6. rejet de deux handles pour la meme boule exacte (cle centre+rayon :
//      des boules concentriques de rayons distincts restent acceptees) ;
//   7. ordre final canonique et lots par `sphere_cmp_beta` exact ;
//   8. pour chaque u du support, miniboule de M sans u : STRICT pour tous
//      donne principal ; EGALITE exacte avec un support alternatif excluant
//      u donne non-principal ; calcul incomplet donne UNKNOWN, jamais un
//      faux bit — temoins indexes par PointId supprime, pas par position ;
//   9. derivation de la fermeture par ordre depuis le RECU opaque du
//      producteur (l'enumeration exhaustive achevee sans censure, liee par
//      digests) — jamais depuis `smax>=n` lu ailleurs ;
//  10. index injectif et digest final construits seulement apres succes.
//
// Digest v0 : FNV-1a 64 bits sur le flux canonique — il LIE les octets pour
// detecter mutation et desynchronisation ; ce n'est pas une preuve
// cryptographique et le schema final exigera le SHA-256 des contrats JSON.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"

namespace mhgp3v {

enum class PrincipalState : std::uint8_t {
  kUnknown = 0,
  kPrincipalCertified = 1,
  kNonPrincipalCertified = 2,
};

enum class CarrierClosure : std::uint8_t { kUnknown = 0, kCertified = 1 };

// La cle de boule EXACTE : centre rationnel reduit ET rayon carre rationnel
// reduit. Le `HybridBallKey` du fold live est un index de CENTRE a deux
// etages ; il n'est pas une cle de boule autonome.
struct ExactBallKey {
  mhgp::i128 cx = 0, cy = 0, cz = 0, cden = 0;   // centre reduit, cden > 0
  mhgp::i128 r2num = 0, r2den = 0;               // rayon carre reduit, r2den > 0
  bool operator==(const ExactBallKey& other) const {
    return cx == other.cx && cy == other.cy && cz == other.cz && cden == other.cden &&
           r2num == other.r2num && r2den == other.r2den;
  }
  bool operator<(const ExactBallKey& other) const {
    if (cx != other.cx) return cx < other.cx;
    if (cy != other.cy) return cy < other.cy;
    if (cz != other.cz) return cz < other.cz;
    if (cden != other.cden) return cden < other.cden;
    if (r2num != other.r2num) return r2num < other.r2num;
    return r2den < other.r2den;
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
  ExactBallKey exact_ball;
  unsigned long long members_digest = 0;
  SidecarSmallSupport canonical_support;
  std::uint8_t q_min = 0;
  PrincipalState principal = PrincipalState::kUnknown;
  std::array<RemovalEvidence, 4> evidence;
  std::uint8_t evidence_count = 0;
};

// LE RECU DE SOURCE opaque : construit par le PRODUCTEUR exact, au moment ou
// lui seul sait que l'enumeration est achevee sans censure. Les digests
// lient le recu au nuage et au catalogue exacts ; la factory refuse tout
// recu desynchronise. `complete_rank_bound >= n` signifie : aucun support
// n'a ete censure par le rang — la pretention de famille complete.
class HybridSourceReceipt {
 public:
  static HybridSourceReceipt make(unsigned long long points_digest,
                                  unsigned long long catalogue_digest, int rank_bound,
                                  int point_count, bool enumeration_completed) {
    HybridSourceReceipt receipt;
    receipt.points_digest_ = points_digest;
    receipt.catalogue_digest_ = catalogue_digest;
    receipt.rank_bound_ = rank_bound;
    receipt.point_count_ = point_count;
    receipt.enumeration_completed_ = enumeration_completed;
    return receipt;
  }
  unsigned long long points_digest() const { return points_digest_; }
  unsigned long long catalogue_digest() const { return catalogue_digest_; }
  bool claims_complete_family() const {
    return enumeration_completed_ && rank_bound_ >= point_count_;
  }

 private:
  HybridSourceReceipt() = default;
  unsigned long long points_digest_ = 0;
  unsigned long long catalogue_digest_ = 0;
  int rank_bound_ = 0;
  int point_count_ = 0;
  bool enumeration_completed_ = false;
};

inline unsigned long long sidecar_fnv1a(unsigned long long seed, const void* data,
                                        std::size_t bytes) {
  const unsigned char* p = (const unsigned char*)data;
  unsigned long long h = seed;
  for (std::size_t i = 0; i < bytes; ++i) {
    h ^= p[i];
    h *= 1099511628211ULL;
  }
  return h;
}

// Les digests canoniques partages entre le PRODUCTEUR (qui scelle son recu)
// et la factory (qui les recalcule sur le snapshot possede).
inline unsigned long long sidecar_points_digest(const std::vector<mhgp::P3>& points) {
  unsigned long long digest = 1469598103934665603ULL;
  for (const mhgp::P3& p : points) {
    const mhgp::i64 c[3] = {(mhgp::i64)p.x, (mhgp::i64)p.y, (mhgp::i64)p.z};
    digest = sidecar_fnv1a(digest, c, sizeof(c));
  }
  return digest;
}

inline unsigned long long sidecar_catalogue_digest(const mhgp::Catalogue& catalogue) {
  unsigned long long digest = 1469598103934665603ULL;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres)
    digest = sidecar_fnv1a(digest, &sphere, sizeof(sphere));
  digest = sidecar_fnv1a(digest, catalogue.members.data(),
                         catalogue.members.size() * sizeof(mhgp::i32));
  return digest;
}

// Les mutants de la factory (contrat §Fixtures) : oubli du dernier u,
// « < » change en « <= », temoin indexe par la POSITION avant permutation.
struct SidecarMutants {
  bool skip_last_removal = false;
  bool strict_leq = false;
  bool witness_by_position = false;
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
  unsigned long long points_digest() const { return points_digest_; }
  unsigned long long catalogue_digest() const { return catalogue_digest_; }
  unsigned long long sidecar_digest() const { return sidecar_digest_; }
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
  unsigned long long points_digest_ = 0;
  unsigned long long catalogue_digest_ = 0;
  unsigned long long sidecar_digest_ = 0;
  std::vector<GeneratorCertificate> generators_;
  std::vector<int> activation_order_;
  std::vector<int> activation_rank_;
  std::vector<std::uint32_t> batch_offsets_;
  std::vector<CarrierClosure> closure_by_order_;
  std::vector<std::pair<ExactBallKey, int>> ball_index_;   // trie, injectif
};

inline mhgp::i128 sidecar_gcd(mhgp::i128 a, mhgp::i128 b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) {
    const mhgp::i128 r = a % b;
    a = b;
    b = r;
  }
  return a;
}

// La cle exacte depuis la representation entiere : centre (base*den+n)/den
// reduit, rayon carre |n|^2/den^2 reduit.
inline ExactBallKey exact_ball_key(const mhgp::Sphere& sphere) {
  ExactBallKey key;
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
  const mhgp::i128 r2num = (mhgp::i128)sphere.nx * sphere.nx +
                           (mhgp::i128)sphere.ny * sphere.ny +
                           (mhgp::i128)sphere.nz * sphere.nz;
  const mhgp::i128 r2den = (mhgp::i128)sphere.den * sphere.den;
  mhgp::i128 rg = sidecar_gcd(r2num, r2den);
  if (rg == 0) rg = 1;
  key.r2num = r2num / rg;
  key.r2den = r2den / rg;
  return key;
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
  if (maximum_order < 1 || maximum_order > mhgp::kMaxRank)
    return refuse("ordre maximal hors contrat");
  const int n = (int)points.size();
  if (n < 1) return refuse("nuage vide");
  sidecar.points_ = std::move(points);
  sidecar.catalogue_ = std::move(catalogue);
  const std::size_t count = sidecar.catalogue_.spheres.size();
  if (count < 1) return refuse("catalogue vide");

  // 1. DIGESTS recalcules sur le snapshot possede.
  const unsigned long long pd = sidecar_points_digest(sidecar.points_);
  const unsigned long long cd = sidecar_catalogue_digest(sidecar.catalogue_);
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

  // 3+4+5. MEMBRES, SATURATION COMPLETE, SUPPORT SUR COQUILLE, den > 0,
  // miniboule des membres EGALE a la boule declaree.
  std::vector<std::vector<mhgp::i32>> members(count);
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = sidecar.catalogue_.spheres[s];
    if (sphere.sph.den <= 0) return refuse("den nul ou negatif");
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
    for (int u = 0; u < q; ++u) {
      if (u > 0 && sphere.support[u - 1] >= sphere.support[u])
        return refuse("support non trie ou duplique");
      if (!std::binary_search(members[s].begin(), members[s].end(), sphere.support[u]))
        return refuse("support hors des membres");
      if (mhgp::sphere_side(sphere.sph, sidecar.points_[(std::size_t)sphere.support[u]]) != 0)
        return refuse("support hors de la coquille : le certificat geometrique ment");
    }
    // SATURATION FERMEE COMPLETE : census exact du nuage entier — un point de
    // la boule fermee absent des membres est une censure, refusee.
    long long inside = 0;
    for (int p = 0; p < n; ++p)
      if (mhgp::sphere_side(sphere.sph, sidecar.points_[(std::size_t)p]) <= 0) ++inside;
    if (inside != (long long)members[s].size())
      return refuse("saturation incomplete : le census ferme contredit les membres");
    // MINIBOULE DES MEMBRES == BOULE DECLAREE, exactement.
    const mhgp::MiniballResult recomputed =
        mhgp::miniball_of(sidecar.points_, members[s].data(), (int)members[s].size());
    if (!recomputed.ok || mhgp::sphere_cmp_beta(recomputed.sph, sphere.sph) != 0 ||
        !(exact_ball_key(recomputed.sph) == exact_ball_key(sphere.sph)))
      return refuse("la boule declaree n'est pas la miniboule exacte de ses membres");
  }

  // 6. UNICITE DES HANDLES par cle exacte centre+rayon.
  sidecar.ball_index_.reserve(count);
  for (std::size_t s = 0; s < count; ++s)
    sidecar.ball_index_.push_back({exact_ball_key(sidecar.catalogue_.spheres[s].sph), (int)s});
  std::sort(sidecar.ball_index_.begin(), sidecar.ball_index_.end(),
            [](const std::pair<ExactBallKey, int>& a, const std::pair<ExactBallKey, int>& b) {
              if (!(a.first == b.first)) return a.first < b.first;
              return a.second < b.second;
            });
  for (std::size_t i = 1; i < sidecar.ball_index_.size(); ++i)
    if (sidecar.ball_index_[i].first == sidecar.ball_index_[i - 1].first)
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
    certificate.exact_ball = exact_ball_key(sphere.sph);
    certificate.members_digest = sidecar_fnv1a(1469598103934665603ULL, members[s].data(),
                                               members[s].size() * sizeof(mhgp::i32));
    const int q = (int)sphere.n_support;
    certificate.q_min = (std::uint8_t)q;
    certificate.canonical_support.size = (std::uint8_t)q;
    for (int u = 0; u < q; ++u) certificate.canonical_support.ids[(std::size_t)u] = sphere.support[u];
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
      } else if (cmp == 0 && exact_ball_key(removed.sph) == certificate.exact_ball) {
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
  // digests. Un recu absent ou desynchronise laisse kUnknown partout.
  sidecar.closure_by_order_.assign((std::size_t)maximum_order, CarrierClosure::kUnknown);
  if (receipt != nullptr && receipt->points_digest() == sidecar.points_digest_ &&
      receipt->catalogue_digest() == sidecar.catalogue_digest_ &&
      receipt->claims_complete_family()) {
    for (std::size_t k = 0; k < sidecar.closure_by_order_.size(); ++k)
      sidecar.closure_by_order_[k] = CarrierClosure::kCertified;
  }

  // 10. DIGEST FINAL, seulement apres succes.
  unsigned long long sd = sidecar_fnv1a(1469598103934665603ULL, &pd, sizeof(pd));
  sd = sidecar_fnv1a(sd, &cd, sizeof(cd));
  for (const GeneratorCertificate& certificate : sidecar.generators_)
    sd = sidecar_fnv1a(sd, &certificate.principal, sizeof(certificate.principal));
  sidecar.sidecar_digest_ = sd;
  sidecar.ok_ = true;
  if (refusal != nullptr) refusal->clear();
  return sidecar;
}

}  // namespace mhgp3v
