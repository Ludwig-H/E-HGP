// MorseHGP3D v6 — PORTE DU WIRE SERIE C v1 (C2, docs/GPU.md § Wire).
// Ce qui est exige :
//   (1) ALLER-RETOUR BIT-EXACT : chaque tableau serialise, reparse, est
//       EGAL au CloudIndex source (left/right/first/last, boites serrees,
//       positions, prefixe des multiplicites) — comptes exacts, jamais un
//       prefixe ;
//   (2) t1 = floor_div128(-b, 2a) confronte, ET la liste de candidats
//       {clamp(t1), clamp(t1+1), lo, hi} confrontee a un BALAYAGE EXHAUSTIF
//       du minimum d'axe sur des boites graves (l'exactitude d'axis_min ne
//       depend que du bon minimiseur — le mutant wire-t1-plus-one meurt sur
//       la fixture vertex-interieur) ;
//   (3) digest GRAVE (uniform 400 graine 3) : identite du televersement,
//       stable et independante du processus ;
//   (4) REFUS hors domaine : boite hors u16, position hors u16, prefixe de
//       multiplicites au-dela de u32 — trois refus, jamais une troncature ;
//   (5) framing des entrees par boule : 112 octets exacts par boule, refus
//       d'une cle non canonisee (a <= 0).
// Codes : 0 ; 1 desaccord ; 2 refus d'argument ; 3 plancher ; 4 mutant tue
// (`gpu-index-drop-node` : un nœud omis — comptes ET digest divergent ;
//  `wire-t1-plus-one` : minimiseur decale — balayage exhaustif divergent).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/wire.hpp"

using namespace mhgp7;

namespace {
// host_wire_digest de reference (uniform 400 graine 3) — a graver au premier
// run ; le mutant drop-node exige la DIVERGENCE de cette valeur.
constexpr const char* kGravedDigest = "3402912149ee9b4d008bb5123e9e07d96003c1d7a730cd25af649dfbc3355f57";
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}

u16 get_u16(const std::vector<u8>& b, size_t i) { return (u16)(b[2 * i] | ((u16)b[2 * i + 1] << 8)); }
u32 get_u32(const std::vector<u8>& b, size_t i) {
  u32 v = 0;
  for (int j = 0; j < 4; ++j) v |= (u32)b[4 * i + j] << (8 * j);
  return v;
}
i32 get_i32(const std::vector<u8>& b, size_t i) { return (i32)get_u32(b, i); }

// Minimum EXHAUSTIF de la parabole d'axe a*t^2 + b*t sur [lo, hi].
i128 brute_axis_min(i128 a, i128 b, i64 lo, i64 hi) {
  i128 best = a * ((i128)lo * lo) + b * lo;
  for (i64 t = lo + 1; t <= hi; ++t) {
    const i128 v = a * ((i128)t * t) + b * t;
    if (v < best) best = v;
  }
  return best;
}

// Le minimum via la LISTE DE CANDIDATS du wire (la meme que le kernel :
// {clamp_boite(c0), clamp_boite(c1), lo, hi}, candidats u32 du wire).
i128 candidate_axis_min(i128 a, i128 b, u32 c0, u32 c1, i64 lo, i64 hi) {
  i128 best = 0;
  bool first = true;
  for (const i64 cand : {(i64)c0, (i64)c1, lo, hi}) {
    const i64 c = std::min(std::max(cand, lo), hi);
    const i128 v = a * ((i128)c * c) + b * c;
    if (first || v < best) {
      best = v;
      first = false;
    }
  }
  return best;
}

// Candidats obtenus par le CHEMIN COMPLET append -> octets -> reparse
// (§ 5.11 : le mutant t1 doit traverser la serialisation reelle).
void reparsed_candidates(const BallKey& k, u32 c0[3], u32 c1[3], bool* ok) {
  gpu::GpuBallInWire bw;
  gpu::append_ball_in(&bw, k, 4);
  *ok = bw.error.empty() && bw.balls == 1;
  if (!*ok) return;
  const gpu::GpuBallHostView v = gpu::decode_ball_in(bw, 0);
  *ok = v.a == k.a && v.b[0] == k.b[0] && v.b[1] == k.b[1] && v.b[2] == k.b[2] && v.c == k.c && v.h == 4;
  for (int i = 0; i < 3; ++i) {
    c0[i] = v.c0[i];
    c1[i] = v.c1[i];
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_drop = MHGP7_MUTANT("gpu-index-drop-node");
  const bool m_t1 = MHGP7_MUTANT("wire-t1-plus-one");

  // ---- Fixture t1 / balayage exhaustif (independante de l'index).
  // Grille de cles synthetiques CANONISEES : a > 0, b balayant signes et
  // non-divisibilites (la division plancher est le point sensible), boites
  // graves DONT le cas vertex-interieur strict (a=1, b=-10, [0,10] : le
  // minimum est en t=5 seulement — un t1 decale d'un rend -24 au lieu de
  // -25, la dent causale du mutant).
  {
    unsigned checked = 0, diverged = 0;
    bool reparse_ok = true;
    for (const i64 a : {1, 2, 3, 7}) {
      for (const i64 b : {-10, -9, -7, -3, -1, 0, 1, 3, 8, 15}) {
        BallKey k{(i128)a, {(i128)b, 0, 0}, -1};
        u32 c0[3], c1[3];
        bool ok = false;
        reparsed_candidates(k, c0, c1, &ok);
        reparse_ok = reparse_ok && ok;
        if (!ok) continue;
        for (const auto& [lo, hi] : {std::pair<i64, i64>{0, 10}, {0, 40}, {3, 7}, {5, 5}, {0, 1}}) {
          ++checked;
          if (candidate_axis_min(a, b, c0[0], c1[0], lo, hi) != brute_axis_min(a, b, lo, hi)) ++diverged;
        }
      }
    }
    // FIXTURE § 5.11 : quotient HORS i64 (a=1, b=-2^70 — centre rationnel
    // lointain, geometrie u16 valide) : candidats SATURES a 65535, minimum
    // d'axe toujours exact (fonction decroissante sur le domaine).
    {
      BallKey k{1, {-(i128(1) << 70), 0, 0}, -1};
      u32 c0[3], c1[3];
      bool ok = false;
      reparsed_candidates(k, c0, c1, &ok);
      reparse_ok = reparse_ok && ok;
      if (ok) {
        expect(c0[0] == 65535 && c1[0] == 65535, "quotient hors i64 : candidats satures a 65535 (jamais un wrap)");
        for (const auto& [lo, hi] : {std::pair<i64, i64>{0, 40}, {65500, 65535}, {0, 65535}}) {
          ++checked;
          // Sommet a 2^69 >> domaine : la parabole est STRICTEMENT
          // decroissante sur [0, 65535], le minimum est val(hi) — forme
          // fermee, pas de balayage.
          const i128 want = ((i128)hi * hi) + (-(i128(1) << 70)) * hi;
          if (candidate_axis_min(1, -(i128(1) << 70), c0[0], c1[0], lo, hi) != want) ++diverged;
        }
      }
    }
    // FIXTURE 1cb08aa8 : b = INT128_MIN (|-b| deborderait un i128 signe) —
    // la construction bornee u128 sature les candidats sans debordement.
    // Candidats seuls (axis_val deborderait i128 a ces coefficients : hors
    // du profil des formes, la robustesse du wire est le sujet ici).
    {
      const i128 int128_min = (i128)(((u128)1) << 127);  // motif de bits 2^127 = INT128_MIN (complement a deux)
      BallKey k{1, {int128_min, 0, 0}, -1};
      u32 c0[3], c1[3];
      bool ok = false;
      reparsed_candidates(k, c0, c1, &ok);
      reparse_ok = reparse_ok && ok;
      if (ok && !m_t1)
        expect(c0[0] == 65535 && c1[0] == 65535, "b = INT128_MIN : candidats satures, aucun debordement signe");
    }
    if (m_t1) {
      if (diverged > 0) {
        std::printf("mutant wire-t1-plus-one TUE : %u minima d'axe divergents sur %u cas exhaustifs "
                    "(chemin append->octets->reparse)\n",
                    diverged, checked);
        return 4;
      }
      std::printf("MUTANT NON TUE\n");
      return 1;
    }
    expect(reparse_ok, "reparse champ par champ == cle appendue (a, b, c, h)");
    expect(diverged == 0 && checked >= 200, "axis_min par candidats == balayage exhaustif (fixtures graves)");
  }

  // ---- Index reel : uniform 400 graine 3.
  const std::vector<InputPoint> in =
      make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
  const CloudIndex ix = build_cloud_index(in);
  const gpu::GpuCloudIndexWire w = gpu::build_index_wire(ix);

  if (m_drop) {
    // Mutant : un nœud omis — les COMPTES divergent ET le digest s'ecarte
    // de la valeur gravee (le claim du digest est verifie, pas seulement la
    // taille de node_left).
    const bool count_diverged = w.error.empty() && w.node_left.size() != (size_t)w.n_nodes * 4;
    const bool digest_diverged = w.host_wire_digest != std::string(kGravedDigest);
    if (count_diverged && digest_diverged) {
      std::printf("mutant gpu-index-drop-node TUE : %zu octets de nœuds pour n_nodes=%u, digest divergent\n",
                  w.node_left.size(), w.n_nodes);
      return 4;
    }
    std::printf("MUTANT NON TUE (comptes=%d digest=%d)\n", count_diverged ? 1 : 0, digest_diverged ? 1 : 0);
    return 1;
  }
  expect(w.error.empty(), "serialisation sans refus sur l'index reel");
  if (!w.error.empty()) return 1;

  // (1) aller-retour bit-exact, comptes exacts.
  bool rt = w.n_nodes == ix.nodes.size() && w.n_upos == ix.upos.size() && w.root == (i32)ix.root();
  rt = rt && w.node_left.size() == ix.nodes.size() * 4 && w.node_box.size() == ix.nodes.size() * 12 &&
       w.upos.size() == ix.upos.size() * 6 && w.wsum.size() == (ix.upos.size() + 1) * 4;
  for (size_t v = 0; rt && v < ix.nodes.size(); ++v) {
    const RadixNode& nd = ix.nodes[v];
    rt = get_i32(w.node_left, v) == nd.left && get_i32(w.node_right, v) == nd.right &&
         get_i32(w.node_first, v) == nd.first && get_i32(w.node_last, v) == nd.last;
    for (int i = 0; rt && i < 3; ++i)
      rt = get_u16(w.node_box, v * 6 + (size_t)i) == (u16)nd.tlo[i] &&
           get_u16(w.node_box, v * 6 + 3 + (size_t)i) == (u16)nd.thi[i];
  }
  for (size_t u = 0; rt && u < ix.upos.size(); ++u)
    rt = get_u16(w.upos, u * 3) == (u16)ix.upos[u].x && get_u16(w.upos, u * 3 + 1) == (u16)ix.upos[u].y &&
         get_u16(w.upos, u * 3 + 2) == (u16)ix.upos[u].z;
  for (size_t u = 0; rt && u < ix.wsum.size(); ++u) rt = get_u32(w.wsum, u) == (u32)ix.wsum[u];
  expect(rt, "aller-retour bit-exact (nœuds, boites, positions, prefixes)");

  // (3) host_wire_digest grave — identite du PAYLOAD HOTE (uniform 400
  // graine 3 ; l'identite des octets RESIDENTS releve de la relecture
  // complete de la porte device, § 5.11).
  if (std::strcmp(kGravedDigest, "GRAVE_AU_PREMIER_RUN") == 0) {
    std::printf("host_wire_digest_uniform400=%s (A GRAVER)\n", w.host_wire_digest.c_str());
    expect(false, "digest grave manquant : graver la valeur imprimee ci-dessus");
  } else {
    expect(w.host_wire_digest == kGravedDigest, "host_wire_digest == valeur gravee");
  }

  // (4) trois refus hors domaine, jamais une troncature.
  {
    CloudIndex bad = ix;
    if (!bad.nodes.empty()) bad.nodes[0].tlo[0] = -1;
    expect(!gpu::build_index_wire(bad).error.empty(), "refus : boite hors u16");
  }
  {
    CloudIndex bad = ix;
    bad.upos[0].x = 70000;
    expect(!gpu::build_index_wire(bad).error.empty(), "refus : position hors u16");
  }
  {
    CloudIndex bad = ix;
    bad.wsum.back() = (u64)1 << 33;
    expect(!gpu::build_index_wire(bad).error.empty(), "refus : prefixe au-dela de u32");
  }

  // (5) framing des boules : 112 octets exacts ; refus TRANSACTIONNELS
  // (§ 5.11) : cle non canonisee, h nul, et la sequence
  // valide -> invalide -> valide ne REPART jamais (tout est vide apres la
  // premiere erreur, aucun prefixe consommable).
  {
    gpu::GpuBallInWire bw;
    gpu::append_ball_in(&bw, BallKey{3, {-6, 2, 0}, -5}, 4);
    gpu::append_ball_in(&bw, BallKey{1, {0, 0, 0}, -1}, 1);
    expect(bw.error.empty() && bw.balls == 2 && bw.bytes.size() == 2 * gpu::kWireBallInBytes,
           "framing : 112 octets par boule");
    gpu::GpuBallInWire bad;
    gpu::append_ball_in(&bad, BallKey{0, {1, 1, 1}, 0}, 1);
    expect(!bad.error.empty() && bad.bytes.empty() && bad.balls == 0,
           "refus transactionnel : cle non canonisee, tout vide");
    gpu::GpuBallInWire bad2;
    gpu::append_ball_in(&bad2, BallKey{1, {0, 0, 0}, -1}, 0);
    expect(!bad2.error.empty() && bad2.bytes.empty(), "refus : seuil h nul");
    gpu::GpuBallInWire seq;
    gpu::append_ball_in(&seq, BallKey{1, {0, 0, 0}, -1}, 1);   // valide
    gpu::append_ball_in(&seq, BallKey{0, {0, 0, 0}, 0}, 1);    // invalide
    gpu::append_ball_in(&seq, BallKey{1, {0, 0, 0}, -1}, 1);   // valide — ne doit RIEN ecrire
    expect(!seq.error.empty() && seq.bytes.empty() && seq.balls == 0,
           "valide -> invalide -> valide : plus rien ne s'ecrit apres la premiere erreur");
  }
  // (6) refus transactionnel de l'index : apres un refus, TOUT est vide.
  {
    CloudIndex bad = ix;
    bad.upos[0].x = 70000;
    const gpu::GpuCloudIndexWire wb = gpu::build_index_wire(bad);
    expect(!wb.error.empty() && wb.n_nodes == 0 && wb.node_left.empty() && wb.upos.empty() &&
               wb.host_wire_digest.empty(),
           "refus d'index transactionnel : aucun prefixe consommable");
  }

  if (failures) return 1;
  if (ix.nodes.size() < 100) {
    std::printf("PLANCHER : index trop petit\n");
    return 3;
  }
  return 0;
}

