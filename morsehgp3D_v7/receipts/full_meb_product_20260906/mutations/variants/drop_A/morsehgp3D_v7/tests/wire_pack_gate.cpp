// MorseHGP3D v6 — PORTE `pack == append` DE L'ENCODEUR PUR A OFFSETS FIXES
// (C6, jalon 2 de la sequence de livraison de
// audits/REPONSE_AUDITEUR_CONCEPTION_C6_20260902).
//
// Ce qui est exige — et RIEN d'autre : l'encodeur pur de src/gpu/wire.hpp
// (`pack_ball_in` / `pack_ball_range`, adapte aux candidats par
// src/gpu/pack.hpp) produit EXACTEMENT les memes octets que
// `append_ball_in`, le chemin de production actuel. Aucun CUDA, aucun flux,
// aucun tampon epingle, aucune mesure de temps : du code HOTE PUR.
//
//   (1) EGALITE OCTET POUR OCTET sur des candidats REELS du pipeline
//       (uniform 400 et eight_clusters 400, graine 3, s = 8, smax = 11 :
//       generation -> tri -> RLE), le tampon `pack` etant confronte au
//       vecteur `append` sur TOUTE sa longueur ;
//   (2) QUEUES : plusieurs tailles de lot dont le nombre de boules N'EST PAS
//       un multiple (dernier lot incomplet), et BORDS : 0 boule, 1 boule, et
//       une taille exactement egale au lot (aucune queue) ;
//   (3) PLUSIEURS NOMBRES DE FILS (1, 2, 4, 8) sur des plages DISJOINTES du
//       MEME tampon : la sortie doit etre BIT-IDENTIQUE au chemin append,
//       donc identique entre elles ;
//   (4) FIXTURES GRAVEES aux coordonnees exactes (dont le centre rationnel
//       lointain a = 1, b = -2^70 du § 5.11, b = 0, b > 0, h = 2^64 - 1) :
//       l'egalite ne depend pas du pipeline ;
//   (5) PREVALIDATION DES PRODUITS DE TAILLES AVANT TOUTE ECRITURE : tampon
//       nul, tampon trop petit, `base + nb` au-dela du contrat (2^32 - 1
//       boules), debordement de size_t, `nb * 112` et `nb * 100` — chaque
//       depassement est un REFUS NET rendu comme valeur, et la plage reste
//       octet pour octet dans son etat anterieur (motif temoin 0xA5) ;
//   (6) REFUS DE BOULE identiques a append (cle non canonisee a <= 0, seuil
//       h nul) : refus des DEUX cotes, aucune ecriture partielle ;
//   (7) constantes du contrat inchangees (112 en entree ; 9 + 91 = 100 en
//       sortie).
//
// Codes : 0 conforme ; 1 desaccord ; 2 refus d'argument ; 3 plancher ;
// 4 mutant tue —
//   `wire-pack-stride-short` : le pas d'ecriture n'est plus 112, les offsets
//       cessent d'etre fixes — dent OCTETS (divergence pack/append), aucune
//       dent de refus ;
//   `wire-pack-slack-size`   : la prevalidation tolere UNE boule au-dela du
//       tampon — dent REFUS (une capacite refusee devient acceptee), aucun
//       octet ne diverge.
// Les deux dents sont exigees SEPAREMENT (selectivite : un mutant ne peut
// pas etre declare tue par le motif de l'autre).
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/gpu/pack.hpp"
#include "../src/gpu/wire.hpp"
#include "../src/pipeline/candidates.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp7;

namespace {

int failures = 0;
u64 balls_compared = 0;   // boules confrontees octet pour octet (toutes scenes)
u64 tails_exercised = 0;  // derniers lots INCOMPLETS reellement packes
u64 thread_scenes = 0;    // scenes (famille x nombre de fils) executees
u64 byte_div = 0;         // dent du mutant de PAS : divergences d'octets
u64 slack_div = 0;        // dent du mutant de PREVALIDATION : refus manquant
u64 edge_scenes = 0;      // bords exerces (0 boule, 1 boule, lot exact)

void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}

// Motif temoin : toute case laissee intacte doit rester a 0xA5.
constexpr u8 kPoison = 0xA5;

// Confrontation OCTET POUR OCTET. Rend true si egal ; sinon compte une
// divergence (dent du mutant de pas) et nomme le PREMIER offset fautif.
bool compare_bytes(const std::vector<u8>& ref, const std::vector<u8>& got, const char* what) {
  if (ref.size() != got.size()) {
    ++byte_div;
    std::printf("DIVERGENCE (%s) : taille %zu != %zu\n", what, got.size(), ref.size());
    return false;
  }
  for (size_t i = 0; i < ref.size(); ++i) {
    if (ref[i] != got[i]) {
      ++byte_div;
      std::printf("DIVERGENCE (%s) : premier octet a l'offset %zu (boule %zu, champ +%zu) : %02x != %02x\n",
                  what, i, i / gpu::kWireBallInBytes, i % gpu::kWireBallInBytes, (unsigned)got[i],
                  (unsigned)ref[i]);
      return false;
    }
  }
  return true;
}

bool all_poison(const std::vector<u8>& b) {
  for (const u8 v : b)
    if (v != kPoison) return false;
  return true;
}

// Reference : le chemin de PRODUCTION actuel, octet par octet pousse dans un
// vecteur global (src/gpu/wire.hpp, `append_ball_in`).
bool append_reference(const std::vector<BallCandidate>& cands, u64 smax, std::vector<u8>* out) {
  gpu::GpuBallInWire bw;
  for (const BallCandidate& bc : cands) gpu::append_ball_in(&bw, bc.key, gpu::wire_threshold(smax, bc.arity));
  if (!bw.error.empty()) return false;
  *out = bw.bytes;
  return true;
}

// Un candidat fabrique : la porte grave des cles exactes sans passer par le
// pipeline (l'arite sert uniquement a fabriquer le seuil h = smax + 1 - q).
BallCandidate make_cand(const BallKey& k, u8 arity) {
  BallCandidate c{};
  c.key = k;
  c.arity = arity;
  return c;
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  i64 min_balls = 1000000, min_tails = 12, min_thread_scenes = 8, min_edges = 8;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    i64 v = 0;
    if (a.rfind("--inject=", 0) == 0) {
      inject = a.substr(9);
    } else if (a.rfind("--min-balls=", 0) == 0) {
      if (!parse_i64_exact(a.c_str() + 12, &v) || v < 0) return 2;
      min_balls = v;
    } else if (a.rfind("--min-tails=", 0) == 0) {
      if (!parse_i64_exact(a.c_str() + 12, &v) || v < 0) return 2;
      min_tails = v;
    } else if (a.rfind("--min-thread-scenes=", 0) == 0) {
      if (!parse_i64_exact(a.c_str() + 20, &v) || v < 0) return 2;
      min_thread_scenes = v;
    } else if (a.rfind("--min-edges=", 0) == 0) {
      if (!parse_i64_exact(a.c_str() + 12, &v) || v < 0) return 2;
      min_edges = v;
    } else {
      return 2;
    }
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_stride = MHGP7_MUTANT("wire-pack-stride-short");
  const bool m_slack = MHGP7_MUTANT("wire-pack-slack-size");
  const bool mutant = m_stride || m_slack;

  // ---- (7) Le contrat de format ne bouge pas.
  expect(gpu::kWireBallInBytes == 112, "contrat : 112 octets par boule en entree");
  expect(gpu::kWirePrefilterOutBytes == 9 && gpu::kWireCensusOutBytes == 91 &&
             gpu::kWireOutBytesPerBall == 100,
         "contrat : 9 + 91 = 100 octets par boule en sortie");

  const u64 smax = 11;

  // =================================================================
  // (1)(2)(3) CANDIDATS REELS : une plage, des lots a queue, plusieurs fils.
  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const std::vector<InputPoint> in = make_family_input(fam, 400, cloud_family_default_coord(fam, 400), 3);
    const CloudIndex ix = build_cloud_index(in);
    GenerateOptions go;
    go.s = 8;
    go.smax = smax;
    go.threads = 4;
    std::vector<BallCandidate> cands;
    GenerateStats gs;
    generate_candidates(ix, go, &cands, &gs);
    if (gs.cap_refus != kCapRefusNone) return 2;
    sort_candidates(&cands, 4);
    deduplicate_candidates(&cands);
    const size_t nb = cands.size();
    if (nb < 1000) {
      std::printf("PLANCHER : famille trop pauvre (%zu boules)\n", nb);
      return 3;
    }

    std::vector<u8> ref;
    if (!append_reference(cands, smax, &ref)) return 2;
    expect(ref.size() == nb * gpu::kWireBallInBytes, "reference append : 112 octets par boule");

    const gpu::CandidateSpan all{cands.data(), nb};
    const gpu::WireSizePlan plan = gpu::wire_plan_bytes(nb);
    expect(plan.status == gpu::PackStatus::kOk && plan.in_bytes == nb * 112 && plan.out_bytes == nb * 100,
           "prevalidation : nb*112 en entree et nb*100 en sortie");

    // -- une seule plage.
    {
      std::vector<u8> buf(plan.in_bytes, kPoison);
      const gpu::PackStatus st = gpu::pack_candidate_range(buf.data(), buf.size(), 0, all, smax);
      expect(st == gpu::PackStatus::kOk, "pack d'une plage unique : accepte");
      compare_bytes(ref, buf, "plage unique");
      balls_compared += nb;
    }

    // -- LOTS : tailles choisies pour produire des QUEUES (nb % lot != 0) et
    //    un cas SANS queue (lot == nb).
    for (const size_t lot : {(size_t)1, (size_t)2, (size_t)3, (size_t)64, (size_t)256, (size_t)1000, nb,
                             nb + 7}) {
      std::vector<u8> buf(plan.in_bytes, kPoison);
      bool ok = true;
      size_t base = 0;
      while (base < nb) {
        const size_t take = (nb - base < lot) ? (nb - base) : lot;
        if (take < lot) ++tails_exercised;  // dernier lot INCOMPLET
        const gpu::CandidateSpan span{cands.data() + base, take};
        ok = ok && gpu::pack_candidate_range(buf.data(), buf.size(), base, span, smax) == gpu::PackStatus::kOk;
        base += take;
      }
      expect(ok, "pack par lots : chaque plage acceptee");
      compare_bytes(ref, buf, "lots");
      balls_compared += nb;
      if (lot == nb) ++edge_scenes;  // BORD : taille exactement egale au lot
    }

    // -- FILS 1, 2, 4, 8 sur des plages CONTIGUES DISJOINTES du meme tampon.
    //    Sous le mutant de PAS, les plages se CHEVAUCHENT (offsets non
    //    fixes) : deux fils ecriraient les memes octets, donc une course.
    //    La porte ne s'appuie jamais sur un comportement indefini — le
    //    mutant est deja tue par les scenes mono-fil ci-dessus.
    if (!m_stride) {
      std::vector<u8> first;
      for (const int threads : {1, 2, 4, 8}) {
        std::vector<u8> buf(plan.in_bytes, kPoison);
        std::vector<int> st((size_t)threads, (int)gpu::PackStatus::kOk);
        std::vector<std::thread> pool;
        const size_t chunk = nb / (size_t)threads;
        for (int t = 0; t < threads; ++t) {
          const size_t base = chunk * (size_t)t;
          const size_t take = (t + 1 == threads) ? (nb - base) : chunk;
          pool.emplace_back([&buf, &st, &cands, base, take, t, smax]() {
            const gpu::CandidateSpan span{cands.data() + base, take};
            st[(size_t)t] = (int)gpu::pack_candidate_range(buf.data(), buf.size(), base, span, smax);
          });
        }
        for (std::thread& th : pool) th.join();
        bool ok = true;
        for (const int s : st) ok = ok && s == (int)gpu::PackStatus::kOk;
        expect(ok, "pack multi-fils : chaque plage acceptee");
        compare_bytes(ref, buf, "multi-fils vs append");
        if (threads == 1)
          first = buf;
        else
          compare_bytes(first, buf, "multi-fils vs mono-fil");
        balls_compared += nb;
        ++thread_scenes;
        if (nb % (size_t)threads != 0) ++tails_exercised;  // dernier fil = queue
      }
    }

    // -- BORDS : 0 boule et 1 boule.
    {
      std::vector<u8> buf(gpu::kWireBallInBytes, kPoison);
      const gpu::CandidateSpan none{cands.data(), 0};
      expect(gpu::pack_candidate_range(buf.data(), buf.size(), 0, none, smax) == gpu::PackStatus::kOk &&
                 all_poison(buf),
             "bord : plage de 0 boule acceptee et AUCUN octet ecrit");
      expect(gpu::pack_candidate_range(nullptr, 0, 0, gpu::CandidateSpan{nullptr, 0}, smax) ==
                 gpu::PackStatus::kOk,
             "bord : plage vide sur tampon nul acceptee");
      ++edge_scenes;

      std::vector<u8> one(gpu::kWireBallInBytes, kPoison);
      const gpu::CandidateSpan single{cands.data(), 1};
      const gpu::PackStatus st = gpu::pack_candidate_range(one.data(), one.size(), 0, single, smax);
      std::vector<u8> ref1(ref.begin(), ref.begin() + (long)gpu::kWireBallInBytes);
      expect(st == gpu::PackStatus::kOk, "bord : plage de 1 boule acceptee");
      compare_bytes(ref1, one, "bord 1 boule");
      balls_compared += 1;
      ++edge_scenes;

      // `pack_ball_in` (une boule, offset fixe) confronte a la MEME boule.
      std::vector<u8> two(2 * gpu::kWireBallInBytes, kPoison);
      const bool okp =
          gpu::pack_ball_in(two.data(), two.size(), 1, cands[1].key, gpu::wire_threshold(smax, cands[1].arity)) ==
          gpu::PackStatus::kOk;
      expect(okp, "pack_ball_in : ecriture a l'offset fixe 1*112");
      std::vector<u8> ref2(ref.begin() + (long)gpu::kWireBallInBytes,
                           ref.begin() + (long)(2 * gpu::kWireBallInBytes));
      std::vector<u8> got2(two.begin() + (long)gpu::kWireBallInBytes, two.end());
      compare_bytes(ref2, got2, "pack_ball_in a l'offset 1");
      bool head_intact = true;
      for (size_t i = 0; i < gpu::kWireBallInBytes; ++i) head_intact = head_intact && two[i] == kPoison;
      expect(head_intact, "pack_ball_in : rien n'est ecrit hors de l'enregistrement vise");
      balls_compared += 1;
      ++edge_scenes;
    }
  }

  // =================================================================
  // (4) FIXTURES GRAVEES aux coordonnees exactes — l'egalite pack/append ne
  // depend pas du pipeline. Cas limites : b > 0, b = 0, quotient hors i64
  // (§ 5.11, candidats satures a 65535), coefficients tres larges, h = 1 et
  // h = 2^64 - 1 (le seuil est fabrique directement, hors arite).
  {
    struct Graved {
      BallKey key;
      u64 h;
    };
    const std::vector<Graved> graved = {
        {BallKey{1, {0, 0, 0}, -1}, 1},
        {BallKey{3, {-6, 2, 0}, -5}, 4},
        {BallKey{7, {-9, 15, -3}, 1234567}, 2},
        {BallKey{1, {-(i128(1) << 70), 0, 0}, -1}, 11},
        {BallKey{(i128(1) << 100), {-(i128(1) << 126), (i128(1) << 90), 0}, -(i128(1) << 120)}, 12},
        {BallKey{2, {5, -1, 0}, 0}, 0xffffffffffffffffull},
        {BallKey{65535, {-131070, -131070, -131070}, -4294836225ll}, 10},
    };
    std::vector<u8> ref;
    {
      gpu::GpuBallInWire bw;
      for (const Graved& g : graved) gpu::append_ball_in(&bw, g.key, g.h);
      expect(bw.error.empty() && bw.balls == graved.size(), "fixtures gravees : append accepte tout");
      ref = bw.bytes;
    }
    // Une plage unique, puis un decoupage a QUEUE (lot 3 sur 7 boules).
    for (const size_t lot : {graved.size(), (size_t)3, (size_t)1}) {
      std::vector<u8> buf(graved.size() * gpu::kWireBallInBytes, kPoison);
      bool ok = true;
      size_t base = 0;
      while (base < graved.size()) {
        const size_t take = (graved.size() - base < lot) ? (graved.size() - base) : lot;
        if (take < lot) ++tails_exercised;
        const Graved* src = graved.data() + base;
        ok = ok && gpu::pack_ball_range(buf.data(), buf.size(), base, take,
                                        [src](size_t i, BallKey* k, u64* h) {
                                          *k = src[i].key;
                                          *h = src[i].h;
                                        }) == gpu::PackStatus::kOk;
        base += take;
      }
      expect(ok, "fixtures gravees : pack accepte tout");
      compare_bytes(ref, buf, "fixtures gravees");
      balls_compared += graved.size();
    }
  }

  // =================================================================
  // (5) PREVALIDATION DES PRODUITS DE TAILLES — jamais une exception, jamais
  // une ecriture partielle. `slack_div` est la dent SEULE du mutant
  // `wire-pack-slack-size`.
  {
    std::vector<u8> buf(2 * gpu::kWireBallInBytes, kPoison);
    expect(gpu::pack_prevalidate(nullptr, 0, 0, 1) == gpu::PackStatus::kNullBuffer,
           "prevalidation : tampon nul refuse");
    expect(gpu::pack_prevalidate(buf.data(), buf.size(), 0, 2) == gpu::PackStatus::kOk,
           "prevalidation : la plage exacte est acceptee");
    // Une boule de trop : c'est EXACTEMENT la tolerance du mutant.
    if (gpu::pack_prevalidate(buf.data(), buf.size(), 0, 3) != gpu::PackStatus::kBufferTooSmall) {
      ++slack_div;
      std::printf("DIVERGENCE (prevalidation) : 3 boules acceptees dans un tampon de 2\n");
    }
    if (gpu::pack_prevalidate(buf.data(), buf.size(), 2, 1) != gpu::PackStatus::kBufferTooSmall) {
      ++slack_div;
      std::printf("DIVERGENCE (prevalidation) : base au-dela du tampon acceptee\n");
    }
    // DEUX boules de trop : refuse meme sous le mutant (selectivite de la
    // dent — le mutant ne fait glisser qu'un seul enregistrement).
    expect(gpu::pack_prevalidate(buf.data(), buf.size(), 0, 4) == gpu::PackStatus::kBufferTooSmall,
           "prevalidation : deux boules de trop refusees");
    expect(gpu::pack_prevalidate(buf.data(), buf.size(), (size_t)-1, 1) ==
               gpu::PackStatus::kBallCountOverflow,
           "prevalidation : base + nb hors size_t refuse");
    // Borne du contrat : au plus 2^32 - 1 boules (comme append_ball_in).
    expect(gpu::wire_plan_bytes(0xffffffffull).status == gpu::PackStatus::kOk,
           "prevalidation : 2^32 - 1 boules restent dans le contrat");
    expect(gpu::wire_plan_bytes((size_t)0x100000000ull).status == gpu::PackStatus::kBallCountOverflow,
           "prevalidation : 2^32 boules refusees (borne d'append_ball_in)");
    expect(gpu::wire_plan_bytes(1000).in_bytes == 112000 && gpu::wire_plan_bytes(1000).out_bytes == 100000,
           "prevalidation : produits exacts 112 000 / 100 000 octets");
    // Le refus n'ecrit rien (meme dent que ci-dessus : une boule de trop).
    if (gpu::pack_ball_in(buf.data(), gpu::kWireBallInBytes, 1, BallKey{1, {0, 0, 0}, -1}, 1) !=
            gpu::PackStatus::kBufferTooSmall ||
        !all_poison(buf)) {
      ++slack_div;
      std::printf("DIVERGENCE (prevalidation) : pack_ball_in a ecrit hors du tampon annonce\n");
    }
  }

  // =================================================================
  // (6) REFUS DE BOULE : cle non canonisee et seuil nul — MEMES refus que
  // append, et la plage entiere reste intacte (jamais une ecriture
  // partielle, meme si la boule fautive est au MILIEU de la plage).
  {
    std::vector<BallCandidate> bad = {
        make_cand(BallKey{1, {0, 0, 0}, -1}, 2), make_cand(BallKey{2, {-4, 0, 0}, -3}, 3),
        make_cand(BallKey{3, {-6, 2, 0}, -5}, 4), make_cand(BallKey{0, {1, 1, 1}, 0}, 2),
        make_cand(BallKey{5, {-1, 0, 0}, -2}, 3)};
    std::vector<u8> buf(bad.size() * gpu::kWireBallInBytes, kPoison);
    const gpu::PackStatus st =
        gpu::pack_candidate_range(buf.data(), buf.size(), 0, gpu::CandidateSpan{bad.data(), bad.size()}, smax);
    expect(st == gpu::PackStatus::kKeyNotCanonical && all_poison(buf),
           "refus : cle non canonisee au milieu de la plage, AUCUNE ecriture partielle");
    gpu::GpuBallInWire bw;
    for (const BallCandidate& bc : bad) gpu::append_ball_in(&bw, bc.key, gpu::wire_threshold(smax, bc.arity));
    expect(!bw.error.empty() && bw.bytes.empty(), "refus : append refuse la MEME plage, tout vide");

    // Seuil nul : arite 2 sous smax = 1 donne h = 0 des deux cotes.
    std::vector<BallCandidate> zero = {make_cand(BallKey{1, {0, 0, 0}, -1}, 2)};
    std::vector<u8> zbuf(gpu::kWireBallInBytes, kPoison);
    expect(gpu::pack_candidate_range(zbuf.data(), zbuf.size(), 0, gpu::CandidateSpan{zero.data(), 1}, 1) ==
                   gpu::PackStatus::kThresholdZero &&
               all_poison(zbuf),
           "refus : seuil h nul, aucune ecriture");
    gpu::GpuBallInWire bz;
    gpu::append_ball_in(&bz, zero[0].key, gpu::wire_threshold(1, zero[0].arity));
    expect(!bz.error.empty() && bz.bytes.empty(), "refus : append refuse le meme seuil nul");
    expect(gpu::pack_candidate_range(nullptr, 0, 0, gpu::CandidateSpan{nullptr, 3}, smax) ==
               gpu::PackStatus::kNullBuffer,
           "refus : source nulle a taille non nulle");
  }

  std::printf("bilan boules_confrontees=%llu queues=%llu scenes_fils=%llu bords=%llu div_octets=%llu "
              "div_prevalidation=%llu\n",
              (unsigned long long)balls_compared, (unsigned long long)tails_exercised,
              (unsigned long long)thread_scenes, (unsigned long long)edge_scenes,
              (unsigned long long)byte_div, (unsigned long long)slack_div);

  if (mutant) {
    // SELECTIVITE : chaque dent exige EXACTEMENT son motif.
    if (m_stride && byte_div > 0 && slack_div == 0) {
      std::printf("mutant wire-pack-stride-short TUE : %llu divergences d'octets (offsets non fixes)\n",
                  (unsigned long long)byte_div);
      return 4;
    }
    if (m_slack && slack_div > 0 && byte_div == 0) {
      std::printf("mutant wire-pack-slack-size TUE : %llu refus de capacite manquants\n",
                  (unsigned long long)slack_div);
      return 4;
    }
    std::printf("MUTANT NON TUE\n");
    return 1;
  }

  expect(byte_div == 0, "pack == append : aucune divergence d'octet");
  expect(slack_div == 0, "prevalidation : aucun refus de capacite manquant");
  if (failures) return 1;
  if ((i64)balls_compared < min_balls || (i64)tails_exercised < min_tails ||
      (i64)thread_scenes < min_thread_scenes || (i64)edge_scenes < min_edges) {
    std::printf("PLANCHER : boules=%llu (>= %lld), queues=%llu (>= %lld), scenes_fils=%llu (>= %lld), "
                "bords=%llu (>= %lld)\n",
                (unsigned long long)balls_compared, (long long)min_balls,
                (unsigned long long)tails_exercised, (long long)min_tails,
                (unsigned long long)thread_scenes, (long long)min_thread_scenes,
                (unsigned long long)edge_scenes, (long long)min_edges);
    return 3;
  }
  return 0;
}

