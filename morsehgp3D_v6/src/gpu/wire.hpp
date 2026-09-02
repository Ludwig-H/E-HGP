// MorseHGP3D v6 — WIRE SERIE C v1 (C2, docs/GPU.md § « Wire série C v1 »).
// Brique HOTE PURE : construction de l'index resident `GpuCloudIndexWire`
// depuis `CloudIndex`, serialisation CANONIQUE champ par champ
// (petit-boutiste explicite — jamais un memcpy de struct ABI ni son
// padding), digest par tableau chaine (tag + taille + octets, puis racine,
// comptes et version), et preparation des entrees par boule (`GpuBallIn` :
// cle i128 en paires u64 + les trois MINIMISEURS ENTIERS EXACTS t1[i] =
// floor_div128(-b[i], 2a) hisses cote hote — AUCUNE division device en
// serie C, le kernel n'execute que add/mul/cmp, le socle temoigne).
//
// § 5.11 (27eb5026) integre :
//   - PAS de retrecissement i64 non garde du quotient : le wire porte SIX
//     candidats u32 par boule — clamp_domaine(t1) et clamp_domaine(t1+1)
//     par axe, rabattus hote sur [0, 65535]. Pour toute boite incluse dans
//     le domaine, rabattre d'abord sur le domaine puis sur la boite donne le
//     MEME candidat que rabattre directement sur la boite (argmin d'une
//     parabole convexe) — la division ET le +1 disparaissent du device, et
//     une cle valide au centre rationnel lointain (a=1, b=-2^70) reste
//     acceptee (fixture de la porte) ;
//   - REFUS TRANSACTIONNELS : a la premiere erreur, TOUT est vide (octets,
//     comptes, digest, metadonnees) et plus rien ne s'ecrit — jamais un
//     prefixe consommable ; h == 0 et plus de UINT32_MAX boules refuses ;
//   - le digest s'appelle host_wire_digest : il identifie le PAYLOAD HOTE
//     (chainage HEXADECIMAL : sha256 de version ∥ hex(sha256(tag ∥ taille_le
//     ∥ octets)) par tableau ∥ en-tete), jamais les octets residents — la
//     porte device de VALIDATION relit une fois les sept tableaux entiers et
//     recalcule (hors mur benchmark) ; la route produit se contente des
//     erreurs CUDA et du digest hote ;
//   - vues typees hote (decode_*) : les portes stub decodent explicitement
//     les octets — jamais un reinterpret_cast (alignement/aliasing).
// Mutants : `gpu-index-drop-node` (un nœud omis — comptes ET digest
// divergent) ; `wire-t1-plus-one` (candidats decales, traversant le chemin
// append -> octets -> reparse — tue par le balayage exhaustif d'axis_min).
#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "../core/intmath.hpp"
#include "../core/mutants.hpp"
#include "../core/sha256.hpp"
#include "../core/types.hpp"
#include "../lanes/keys.hpp"
#include "../tree/cloud_index.hpp"

namespace mhgp6 {
namespace gpu {

inline constexpr const char* kWireVersion = "gpu_wire_v1";
// Tailles WIRE par element (les types du contrat, jamais un sizeof CPU).
inline constexpr size_t kWireBytesPerNode = 4 + 4 + 4 + 4 + 12;  // left,right,first,last,box u16x6
inline constexpr size_t kWireBytesPerUpos = 6 + 4;               // pos u16x3 + wsum u32
inline constexpr size_t kWireBallInBytes = 112;                  // cle 80 + t1 24 + h 8
// SORTIES par boule, decomposition EXACTE du transport reel (cli/mhgp6_cuda.cu,
// juge tests/pilote_juge.py : 100 o par boule en D2H comme en sentinelles) —
// les valeurs 12 et 92 gravees ici jusqu'au 2 septembre etaient fausses et
// inutilisees : deux autorites concurrentes pour dimensionner le meme slab
// (retour auditeur, REPONSE_AUDITEUR_CONCEPTION_C6_20260902).
inline constexpr size_t kWirePrefilterOutBytes = 8 + 1;                    // count u64 + statut u8
inline constexpr size_t kWireCensusOutBytes = 21 * 4 + 1 + 1 + 1 + 4;      // ids upos + cstatut + n_int + n_shell + cand_idx
inline constexpr size_t kWireOutBytesPerBall = kWirePrefilterOutBytes + kWireCensusOutBytes;
static_assert(kWireBytesPerNode == 28 && kWireBytesPerUpos == 10,
              "cible < 60 o/upos demontree par les types wire (38 o/upos)");
static_assert(kWirePrefilterOutBytes == 9 && kWireCensusOutBytes == 91,
              "payload census fixe : 21 ids upos, statut et comptes");
static_assert(kWireOutBytesPerBall == 100,
              "contrat D2H et sentinelles : 100 o par boule (juge pilote_juge.py)");

namespace wire_detail {

inline void put_u16(std::vector<u8>* out, u16 v) {
  out->push_back((u8)(v & 0xff));
  out->push_back((u8)(v >> 8));
}
inline void put_u32(std::vector<u8>* out, u32 v) {
  for (int i = 0; i < 4; ++i) out->push_back((u8)(v >> (8 * i)));
}
inline void put_u64(std::vector<u8>* out, u64 v) {
  for (int i = 0; i < 8; ++i) out->push_back((u8)(v >> (8 * i)));
}
inline void put_i32(std::vector<u8>* out, i32 v) { put_u32(out, (u32)v); }
inline void put_i64(std::vector<u8>* out, i64 v) { put_u64(out, (u64)v); }
inline void put_i128(std::vector<u8>* out, i128 v) {
  const u128 u = (u128)v;
  put_u64(out, (u64)u);
  put_u64(out, (u64)(u >> 64));
}

inline std::string array_digest(const char* tag, const std::vector<u8>& bytes) {
  Sha256 h;
  h.update(tag, std::strlen(tag));
  u8 len[8];
  for (int i = 0; i < 8; ++i) len[i] = (u8)((u64)bytes.size() >> (8 * i));
  h.update(len, 8);
  h.update(bytes.data(), bytes.size());
  return h.hex();
}

}  // namespace wire_detail

// Index resident serialise : tableaux SoA prets au televersement + digest
// hote. `error` non vide = refus TRANSACTIONNEL : tout le reste est VIDE.
struct GpuCloudIndexWire {
  std::string error;
  u32 n_nodes = 0, n_upos = 0;
  i32 root = 0;
  std::vector<u8> node_left, node_right, node_first, node_last;  // i32 serialise
  std::vector<u8> node_box;                                      // u16 x 6 (tlo puis thi)
  std::vector<u8> upos;                                          // u16 x 3
  std::vector<u8> wsum;                                          // u32 (prefixe des multiplicites)
  std::string host_wire_digest;  // identite du PAYLOAD HOTE (jamais des octets residents)

  void refuse(const char* why) {  // transactionnel : plus rien a consommer
    error = why;
    n_nodes = n_upos = 0;
    root = 0;
    node_left.clear();
    node_right.clear();
    node_first.clear();
    node_last.clear();
    node_box.clear();
    upos.clear();
    wsum.clear();
    host_wire_digest.clear();
  }
};

inline GpuCloudIndexWire build_index_wire(const CloudIndex& ix) {
  using namespace wire_detail;
  GpuCloudIndexWire w;
  const size_t m = ix.upos.size();
  if (m == 0) {
    w.refuse("invalid_input : index vide");
    return w;
  }
  if (ix.wsum.size() != m + 1) {
    w.refuse("invalid_input : prefixe de multiplicites incoherent");
    return w;
  }
  const bool drop = MHGP6_MUTANT("gpu-index-drop-node");
  w.n_nodes = (u32)ix.nodes.size();
  w.n_upos = (u32)m;
  w.root = (i32)ix.root();
  for (size_t v = 0; v < ix.nodes.size(); ++v) {
    if (drop && v == ix.nodes.size() / 2) continue;  // MUTANT : un nœud omis
    const RadixNode& nd = ix.nodes[v];
    for (int i = 0; i < 3; ++i) {
      if (nd.tlo[i] < 0 || nd.tlo[i] > 65535 || nd.thi[i] < 0 || nd.thi[i] > 65535) {
        w.refuse("invalid_input : boite serree hors du profil u16 (refus, jamais une troncature)");
        return w;
      }
    }
    put_i32(&w.node_left, nd.left);
    put_i32(&w.node_right, nd.right);
    put_i32(&w.node_first, nd.first);
    put_i32(&w.node_last, nd.last);
    for (int i = 0; i < 3; ++i) put_u16(&w.node_box, (u16)nd.tlo[i]);
    for (int i = 0; i < 3; ++i) put_u16(&w.node_box, (u16)nd.thi[i]);
  }
  for (size_t u = 0; u < m; ++u) {
    const P3& p = ix.upos[u];
    if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 || p.z < 0 || p.z > 65535) {
      w.refuse("invalid_input : position hors du profil u16 (refus, jamais une troncature)");
      return w;
    }
    put_u16(&w.upos, (u16)p.x);
    put_u16(&w.upos, (u16)p.y);
    put_u16(&w.upos, (u16)p.z);
  }
  for (size_t u = 0; u < ix.wsum.size(); ++u) {
    if (ix.wsum[u] > 0xffffffffull) {
      w.refuse("invalid_input : prefixe de multiplicites au-dela de u32 (refus)");
      return w;
    }
    put_u32(&w.wsum, (u32)ix.wsum[u]);
  }
  // DIGEST HOTE — ORDRE DE HACHAGE FIGE (chainage HEXADECIMAL) : sha256 de
  //   octets(version) ∥ hex(sha256(tag ∥ taille_le64 ∥ octets)) pour chacun
  //   des sept tableaux dans l'ordre ci-dessous ∥ n_nodes_le32 ∥ n_upos_le32
  //   ∥ root_le32.
  Sha256 h;
  const auto chain = [&](const char* tag, const std::vector<u8>& bytes) {
    const std::string d = array_digest(tag, bytes);  // hex (64 caracteres)
    h.update(d.data(), d.size());
  };
  h.update(kWireVersion, std::strlen(kWireVersion));
  chain("node_left", w.node_left);
  chain("node_right", w.node_right);
  chain("node_first", w.node_first);
  chain("node_last", w.node_last);
  chain("node_box", w.node_box);
  chain("upos", w.upos);
  chain("wsum", w.wsum);
  std::vector<u8> head;
  put_u32(&head, w.n_nodes);
  put_u32(&head, w.n_upos);
  put_i32(&head, w.root);
  h.update(head.data(), head.size());
  w.host_wire_digest = h.hex();
  return w;
}

// VUES TYPEES HOTE (decodage explicite petit-boutiste — jamais un
// reinterpret_cast sur les octets : alignement et aliasing indefinis) :
// les portes stub et la route stub consomment CES vecteurs ; le cudaMemcpy
// reel vers une allocation device correctement alignee n'est pas concerne.
struct GpuIndexHostView {
  std::vector<i32> node_left, node_right, node_first, node_last;
  std::vector<u16> node_box, upos;
  std::vector<u32> wsum;
};

namespace wire_detail {
inline u16 read_u16(const std::vector<u8>& b, size_t i) { return (u16)(b[2 * i] | ((u16)b[2 * i + 1] << 8)); }
inline u32 read_u32(const std::vector<u8>& b, size_t i) {
  u32 v = 0;
  for (int j = 0; j < 4; ++j) v |= (u32)b[4 * i + j] << (8 * j);
  return v;
}
inline u64 read_u64(const std::vector<u8>& b, size_t i) {
  u64 v = 0;
  for (int j = 0; j < 8; ++j) v |= (u64)b[8 * i + j] << (8 * j);
  return v;
}
}  // namespace wire_detail

inline GpuIndexHostView decode_index_wire(const GpuCloudIndexWire& w) {
  using namespace wire_detail;
  GpuIndexHostView v;
  const size_t nn = w.node_left.size() / 4;
  for (size_t i = 0; i < nn; ++i) {
    v.node_left.push_back((i32)read_u32(w.node_left, i));
    v.node_right.push_back((i32)read_u32(w.node_right, i));
    v.node_first.push_back((i32)read_u32(w.node_first, i));
    v.node_last.push_back((i32)read_u32(w.node_last, i));
  }
  for (size_t i = 0; i < w.node_box.size() / 2; ++i) v.node_box.push_back(read_u16(w.node_box, i));
  for (size_t i = 0; i < w.upos.size() / 2; ++i) v.upos.push_back(read_u16(w.upos, i));
  for (size_t i = 0; i < w.wsum.size() / 4; ++i) v.wsum.push_back(read_u32(w.wsum, i));
  return v;
}

// Entree par boule : cle i128 serialisee en paires u64 + SIX CANDIDATS u32
// (clamp_domaine(t1) et clamp_domaine(t1 + 1) par axe, § 5.11 : le quotient
// reste en i128, aucun retrecissement i64 — un centre rationnel lointain
// donne des candidats satures a 0 ou 65535, geometrie u16 valide acceptee)
// + seuil h > 0. Precondition du contrat : k.a > 0 (cle canonisee).
// Layout des mots u64 10..12 : w[10+i] = c0[i] | (c1[i] << 32).
struct GpuBallInWire {
  std::string error;
  std::vector<u8> bytes;  // kWireBallInBytes par boule
  u64 balls = 0;

  void refuse(const char* why) {  // transactionnel : plus rien ne s'ecrit
    error = why;
    bytes.clear();
    balls = 0;
  }
};

// Construction BORNEE des candidats (1cb08aa8 : aucun i128 signe
// intermediaire — `-b` deborde a INT128_MIN et `2*a` au-dela de 2^126) :
//   b > 0  : sommet a t < 0 strictement — les deux candidats rabattus = 0 ;
//   b == 0 : sommet a 0 — candidats 0 et 1 ;
//   b < 0  : q = uabs128(b) / (2 * u128(a)) en NON SIGNE, puis q et q + 1
//            satures a 65535. Couvre b = INT128_MIN et tout a > 0, sans
//            sous-profil de coefficients.
inline void wire_t1_candidates(const BallKey& k, int axis, u32* c0, u32* c1) {
  const i128 b = k.b[axis];
  u32 v0 = 0, v1 = 0;
  if (b > 0) {
    v0 = v1 = 0;
  } else if (b == 0) {
    v0 = 0;
    v1 = 1;
  } else {
    const u128 q = uabs128(b) / (2 * (u128)k.a);
    v0 = q > 65535 ? 65535u : (u32)q;
    v1 = q + 1 > 65535 ? 65535u : (u32)(q + 1);
  }
  if (MHGP6_MUTANT("wire-t1-plus-one")) {  // MUTANT : candidats decales (append -> octets -> reparse)
    v0 = v0 >= 65535 ? 65535u : v0 + 1;
    v1 = v1 >= 65535 ? 65535u : v1 + 1;
  }
  *c0 = v0;
  *c1 = v1;
}

inline void append_ball_in(GpuBallInWire* w, const BallKey& k, u64 h) {
  using namespace wire_detail;
  if (!w->error.empty()) return;  // premiere erreur : plus rien ne s'ecrit
  if (!(k.a > 0)) {
    w->refuse("invalid_input : cle non canonisee (a <= 0)");
    return;
  }
  if (h == 0) {
    w->refuse("invalid_input : seuil h nul");
    return;
  }
  if (w->balls >= 0xffffffffull) {
    w->refuse("invalid_input : plus de 2^32 - 1 boules");
    return;
  }
  put_i128(&w->bytes, k.a);
  for (int i = 0; i < 3; ++i) put_i128(&w->bytes, k.b[i]);
  put_i128(&w->bytes, k.c);
  for (int i = 0; i < 3; ++i) {
    u32 c0 = 0, c1 = 0;
    wire_t1_candidates(k, i, &c0, &c1);
    put_u32(&w->bytes, c0);
    put_u32(&w->bytes, c1);
  }
  put_u64(&w->bytes, h);
  ++w->balls;
}

// Reparse champ par champ (la porte confronte le chemin COMPLET
// append -> octets -> reparse ; la route stub consomme ces mots).
struct GpuBallHostView {
  i128 a = 0, b[3] = {0, 0, 0}, c = 0;
  u32 c0[3] = {0, 0, 0}, c1[3] = {0, 0, 0};
  u64 h = 0;
};

inline GpuBallHostView decode_ball_in(const GpuBallInWire& w, u64 ball) {
  using namespace wire_detail;
  GpuBallHostView v;
  const size_t base = (size_t)ball * (kWireBallInBytes / 8);
  const auto rd128 = [&](size_t word) {
    return (i128)(((u128)read_u64(w.bytes, base + word + 1) << 64) | (u128)read_u64(w.bytes, base + word));
  };
  v.a = rd128(0);
  for (int i = 0; i < 3; ++i) v.b[i] = rd128(2 + 2 * (size_t)i);
  v.c = rd128(8);
  for (int i = 0; i < 3; ++i) {
    const u64 wrd = read_u64(w.bytes, base + 10 + (size_t)i);
    v.c0[i] = (u32)wrd;
    v.c1[i] = (u32)(wrd >> 32);
  }
  v.h = read_u64(w.bytes, base + 13);
  return v;
}

// Mots u64 des boules, decodes explicitement pour la route STUB (jamais un
// reinterpret_cast des octets).
inline std::vector<u64> decode_ball_words(const GpuBallInWire& w) {
  using namespace wire_detail;
  std::vector<u64> out(w.bytes.size() / 8);
  for (size_t i = 0; i < out.size(); ++i) out[i] = read_u64(w.bytes, i);
  return out;
}

}  // namespace gpu
}  // namespace mhgp6
