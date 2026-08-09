// MorseHGP3D v3 — QUALIFICATION DU FRONT D'ONDE, hôte et device par le même code.
//
// Ce binaire construit un lot de travail réel — les sommets du niveau superficiel
// d'un nuage —, l'évalue par le noyau borné, et compare terme à terme avec le chemin
// CPU non borné. Compilé sans CUDA il ne fait que l'hôte, ce qui suffit à qualifier
// le lot, le reçu et la comparaison. Compilé par `nvcc` avec
// `MHGP3V_WAVEFRONT_CUDA`, il exécute EN PLUS le même corps dans un kernel `sm_120`
// et exige l'égalité BIT À BIT des deux.
//
// La séparation est volontaire : tout ce qui peut être falsifié sans GPU l'est sans
// GPU, et la session facturée ne sert qu'à ce qui exige réellement le matériel.
//
// ---------------------------------------------------------------------------
// CE QUE CETTE PORTE NE PROUVAIT PAS, ET POURQUOI ELLE ÉTAIT VIDE
// ---------------------------------------------------------------------------
//
// La boucle de référence exécutait `continue` sur chaque sommet refusé. Le refus
// était donc COMPTÉ et jamais JUGÉ, et les refus d'admission — coquille au-delà de
// trente-deux — n'entraient même pas dans le compteur. Un mutant refusant tous les
// sommets restait vert : zéro flat, zéro couple, zéro désaccord, `OK`.
//
// Quatre choses le ferment, et il faut les quatre.
//
//   1. des planchers SÉPARÉS : nuages décidés, sommets acceptés, flats, couples,
//      lancements de noyau et rejeux — un seul plancher agrégé se satisfait d'un
//      seul chiffre ;
//   2. un oracle non borné exécuté pour CHAQUE statut, refus compris ;
//   3. le ledger `refusés = rejoués + en attente`, publié par raison ;
//   4. l'identité de masse : le CPU complet sur TOUS les sommets doit égaler
//      l'union des résultats committés et rejoués, flat par flat et couple par
//      couple.
//
// Le mutant vit dans le binaire, sous `--force-refuse-all`, pour qu'une porte
// négative puisse exiger qu'il ROUGISSE.
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/device_wavefront_job.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;

#if defined(MHGP3V_WAVEFRONT_CUDA)
// Le kernel et son lancement vivent dans l'unité `.cu`. Ici on ne déclare que le
// point de couture, exactement comme le fait `morsehgp3d` : l'hôte appelle un
// symbole qu'il ne définit pas.
namespace mhgp3v {
namespace device {
bool launch_wavefront_on_gpu(const WavefrontJob& job, VertexVerdict* out, double* milliseconds,
                             char* error, int error_capacity);
}  // namespace device
}  // namespace mhgp3v
#endif

namespace {

P3 pt(int x, int y, int z) {
  P3 p{};
  p.x = (i32)x; p.y = (i32)y; p.z = (i32)z;
  return p;
}

// ---------------------------------------------------------------------------
// L'ORACLE NON BORNÉ
// ---------------------------------------------------------------------------
//
// Il parcourt le chemin CPU complet — `flats::for_each_flat`, sans aucune
// capacité — et rend deux choses à la fois : la masse TOTALE du sommet, et la
// signature du PRÉFIXE que le noyau borné a le droit de committer. C'est ce qui
// permet de juger un refus au lieu de le compter : le préfixe doit concorder bit
// à bit, et la masse totale doit rentrer dans l'identité du lot.
struct ReferenceVertex {
  long long flats = 0;              // flats du chemin non borné, sans capacité
  long long couples = 0;            // couples admissibles, sans capacité
  unsigned long long prefix_mask = 0;
  unsigned long long prefix_signature = 0;
  int prefix_flats = 0;
};

ReferenceVertex reference_vertex(const std::vector<P3>& pts, const mhgp3v::flats::Vertex& v,
                                 const std::vector<i32>& root_base) {
  ReferenceVertex out;
  out.prefix_signature = mhgp3v::device::kDigestSeed;
  long long index = 0;
  mhgp3v::flats::for_each_flat(pts, v, [&](const mhgp3v::flats::FlatAtVertex& f) {
    int slots = 0;
    for (int slot = 0; slot < 2; ++slot) {
      const int direction = slot == 0 ? -1 : 1;
      if (mhgp3v::flats::pair_admissible(pts, v, f.base, direction, root_base)) {
        slots |= (1 << slot);
        ++out.couples;
      }
    }
    if (index < mhgp3v::device::kMaxFlatsPerVertex) {
      if (slots & 1) out.prefix_mask |= (1ULL << (2 * index));
      if (slots & 2) out.prefix_mask |= (1ULL << (2 * index + 1));
      out.prefix_signature = mhgp3v::device::fold_digest(out.prefix_signature, index);
      out.prefix_signature = mhgp3v::device::fold_digest(out.prefix_signature, f.base[0]);
      out.prefix_signature = mhgp3v::device::fold_digest(out.prefix_signature, f.base[1]);
      out.prefix_signature = mhgp3v::device::fold_digest(out.prefix_signature, f.base[2]);
      out.prefix_signature =
          mhgp3v::device::fold_digest(out.prefix_signature, (long long)f.closure.size());
      out.prefix_signature = mhgp3v::device::fold_digest(out.prefix_signature, slots);
      ++out.prefix_flats;
    }
    ++index;
    return true;
  });
  out.flats = index;
  return out;
}

// ---------------------------------------------------------------------------
// LA FIXTURE PERMANENTE DU REFUS
// ---------------------------------------------------------------------------
//
// Sept points entiers sur la sphère de centre (100,100,100) et de rayon 25, sans
// quadruplet coplanaire, plus le centre comme point intérieur : le sommet est
// valide, de niveau un, et sa coquille porte C(7,3) = 35 flats. Le noyau borné
// s'arrête à trente-deux et rend `kFlatOverflow`.
//
// C'est la fixture qui prouve que le refus est JUGÉ. Elle exige trois choses :
// que le préfixe committé concorde bit à bit avec le préfixe du chemin non
// borné, que le rejeu retrouve les trente-cinq flats, et que rien de tout cela
// ne soit silencieux.
int refusal_fixture() {
  int faults = 0;
  std::vector<P3> pts{pt(75, 100, 100), pt(76, 93, 100),  pt(76, 100, 93), pt(76, 100, 107),
                      pt(80, 85, 100),  pt(80, 88, 91),   pt(80, 91, 112), pt(100, 100, 100)};
  // Auto-certification : les sept premiers sont bien sur la sphère de rayon 25.
  for (int i = 0; i < 7; ++i) {
    const long long dx = pts[(std::size_t)i].x - 100, dy = pts[(std::size_t)i].y - 100,
                    dz = pts[(std::size_t)i].z - 100;
    if (dx * dx + dy * dy + dz * dz != 625) {
      printf("[fixture refus] le point %d n'est pas sur la sphere de rayon 25\n", i);
      ++faults;
    }
  }

  mhgp3v::flats::Vertex v;
  for (i32 i = 0; i < 7; ++i) v.shell.push_back(i);
  v.interior.push_back(7);
  v.level = 1;

  std::vector<i32> root_base;
  for (i32 z = 0; z < (i32)pts.size() && root_base.size() < 4; ++z) {
    const std::size_t have = root_base.size();
    if (have == 2) {
      const P3 u = mhgp::p3_sub(pts[(std::size_t)root_base[1]], pts[(std::size_t)root_base[0]]);
      const P3 w = mhgp::p3_sub(pts[(std::size_t)z], pts[(std::size_t)root_base[0]]);
      const P3 cr = mhgp::p3_cross(u, w);
      if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
    } else if (have == 3) {
      if (mhgp3v::flats::orient3d_exact(pts[(std::size_t)root_base[0]],
                                        pts[(std::size_t)root_base[1]],
                                        pts[(std::size_t)root_base[2]],
                                        pts[(std::size_t)z]) == 0) continue;
    }
    root_base.push_back(z);
  }
  if (root_base.size() != 4) {
    printf("[fixture refus] base independante absente\n");
    return faults + 1;
  }

  mhgp3v::device::BoundedVertex bv;
  mhgp3v::device::AdmissionStats astats;
  if (mhgp3v::device::admit(v, &bv, &astats) != mhgp3v::device::Admission::kOk) {
    printf("[fixture refus] le sommet a sept points de coquille n'est pas admis\n");
    return faults + 1;
  }

  mhgp3v::device::WavefrontJob job;
  job.points = pts.data();
  job.point_count = (int)pts.size();
  job.root_base = root_base.data();
  job.root_size = (int)root_base.size();
  job.vertices = &bv;
  job.vertex_count = 1;
  job.declared_cloud_digest = mhgp3v::device::cloud_digest(pts.data(), (int)pts.size());
  const mhgp3v::device::JobVerdict jv = mhgp3v::device::validate_job(job);
  if (jv != mhgp3v::device::JobVerdict::kValid) {
    printf("[fixture refus] le lot est refuse : %s\n", mhgp3v::device::job_verdict_name(jv));
    return faults + 1;
  }

  mhgp3v::device::VertexVerdict verdict;
  mhgp3v::device::evaluate_vertex(job, 0, &verdict);
  const ReferenceVertex reference = reference_vertex(pts, v, root_base);

  if (verdict.status != (int)mhgp3v::device::VerdictStatus::kFlatOverflow ||
      verdict.flat_count != mhgp3v::device::kMaxFlatsPerVertex) {
    printf("[fixture refus] statut=%d flats=%d, attendu kFlatOverflow a 32\n", verdict.status,
           verdict.flat_count);
    ++faults;
  }
  if (reference.flats != 35) {
    printf("[fixture refus] le chemin non borne rend %lld flats, attendu 35\n", reference.flats);
    ++faults;
  }
  if (verdict.admissible != reference.prefix_mask ||
      verdict.signature != reference.prefix_signature) {
    printf("[fixture refus] le prefixe committe ne concorde pas : masque %llx contre %llx\n",
           verdict.admissible, reference.prefix_mask);
    ++faults;
  }
  if (verdict.admissible != 0x940800000009ULL) {
    printf("[fixture refus] masque partiel %llx, l'audit a grave 0x940800000009\n",
           verdict.admissible);
    ++faults;
  }
  if (faults == 0)
    printf("[fixture refus] sept points cospheriques : 35 flats contre la capacite 32, refus"
           " juge, prefixe 0x%llx concordant avec le chemin non borne\n", verdict.admissible);
  return faults;
}

// Un lot malformé doit être refusé AVANT lancement, avec une raison nommée, et
// non devenir un accès hors limites device.
int job_contract_fixture() {
  int faults = 0;
  std::vector<P3> pts{pt(0, 0, 0), pt(10, 0, 0), pt(0, 10, 0), pt(0, 0, 10), pt(3, 3, 3)};
  mhgp3v::flats::Vertex v;
  v.shell = {0, 1, 2, 3};
  v.interior = {4};
  v.level = 1;
  mhgp3v::device::BoundedVertex bv;
  if (mhgp3v::device::admit(v, &bv) != mhgp3v::device::Admission::kOk) {
    printf("[fixture contrat] le sommet temoin n'est pas admis\n");
    return 1;
  }
  const std::vector<i32> root{0, 1, 2, 3};
  mhgp3v::device::WavefrontJob base;
  base.points = pts.data();
  base.point_count = (int)pts.size();
  base.root_base = root.data();
  base.root_size = 4;
  base.vertices = &bv;
  base.vertex_count = 1;
  base.declared_cloud_digest = mhgp3v::device::cloud_digest(pts.data(), (int)pts.size());

  auto expect = [&](const char* what, const mhgp3v::device::WavefrontJob& job,
                    mhgp3v::device::JobVerdict wanted) {
    const mhgp3v::device::JobVerdict got = mhgp3v::device::validate_job(job);
    if (got != wanted) {
      printf("[fixture contrat] %s rend %s, attendu %s\n", what,
             mhgp3v::device::job_verdict_name(got), mhgp3v::device::job_verdict_name(wanted));
      ++faults;
    }
  };

  expect("lot conforme", base, mhgp3v::device::JobVerdict::kValid);
  {
    // LE CAS EXACT DE L'AUDIT : root_size = 0 et root_base nul obtenaient `kOk`.
    mhgp3v::device::WavefrontJob job = base;
    job.root_base = nullptr;
    job.root_size = 0;
    expect("racine nulle", job, mhgp3v::device::JobVerdict::kNullRoot);
  }
  {
    mhgp3v::device::WavefrontJob job = base;
    job.root_size = 3;
    expect("racine de taille trois", job, mhgp3v::device::JobVerdict::kRootSizeNotFour);
  }
  {
    const std::vector<i32> flat_root{0, 1, 2, 2};
    mhgp3v::device::WavefrontJob job = base;
    job.root_base = flat_root.data();
    expect("racine degeneree", job, mhgp3v::device::JobVerdict::kRootNotIndependent);
  }
  {
    mhgp3v::device::WavefrontJob job = base;
    job.declared_cloud_digest ^= 1ULL;
    expect("digest faux", job, mhgp3v::device::JobVerdict::kCloudDigestMismatch);
  }
  {
    std::vector<P3> off = pts;
    off[4] = pt(65536, 0, 0);
    mhgp3v::device::WavefrontJob job = base;
    job.points = off.data();
    job.declared_cloud_digest = mhgp3v::device::cloud_digest(off.data(), (int)off.size());
    expect("coordonnee hors grille u16", job, mhgp3v::device::JobVerdict::kCoordinateOffGrid);
  }
  {
    mhgp3v::device::BoundedVertex bad = bv;
    bad.shell[0] = bad.shell[1];
    mhgp3v::device::WavefrontJob job = base;
    job.vertices = &bad;
    expect("coquille non triee", job, mhgp3v::device::JobVerdict::kShellNotStrictlySorted);
  }
  {
    mhgp3v::device::BoundedVertex bad = bv;
    bad.interior[0] = bad.shell[0];
    mhgp3v::device::WavefrontJob job = base;
    job.vertices = &bad;
    expect("coquille et interieur se coupent", job,
           mhgp3v::device::JobVerdict::kShellMeetsInterior);
  }
  {
    mhgp3v::device::BoundedVertex bad = bv;
    bad.level = 7;
    mhgp3v::device::WavefrontJob job = base;
    job.vertices = &bad;
    expect("niveau different de l'interieur", job,
           mhgp3v::device::JobVerdict::kLevelIsNotInteriorSize);
  }
  {
    mhgp3v::device::BoundedVertex bad = bv;
    bad.shell[0] = (i32)pts.size();
    bad.shell[1] = (i32)pts.size() + 1;
    bad.shell[2] = (i32)pts.size() + 2;
    bad.shell[3] = (i32)pts.size() + 3;
    mhgp3v::device::WavefrontJob job = base;
    job.vertices = &bad;
    expect("identifiant hors nuage", job, mhgp3v::device::JobVerdict::kShellIdOutOfRange);
  }
  {
    mhgp3v::device::WavefrontJob job = base;
    job.vertex_count = 0;
    expect("lot vide", job, mhgp3v::device::JobVerdict::kVertexCountOutOfRange);
  }
  if (faults == 0)
    printf("[fixture contrat] onze formes de lot jugees avant lancement, dont la racine nulle"
           " qui obtenait kOk\n");
  return faults;
}

}  // namespace

int main(int argc, char** argv) {
  int points_count = 24, coord = 4000, smax = 8, clouds = 8;
  int min_refused = 0, min_clouds = 0, min_accepted = 0, min_flats = 0, min_couples = 0;
  int min_kernels = 0, min_replayed = 0, force_refuse_all = 0;
  long long seed = 20260809;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto result = std::from_chars(first, last, magnitude);
    if (result.ec != std::errc{} || result.ptr != last) return false;
    if (magnitude > 1000000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    if (!strcmp(argv[i], "--points")) target = &points_count;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--clouds")) target = &clouds;
    else if (!strcmp(argv[i], "--min-refused")) target = &min_refused;
    else if (!strcmp(argv[i], "--min-clouds")) target = &min_clouds;
    else if (!strcmp(argv[i], "--min-accepted")) target = &min_accepted;
    else if (!strcmp(argv[i], "--min-flats")) target = &min_flats;
    else if (!strcmp(argv[i], "--min-couples")) target = &min_couples;
    else if (!strcmp(argv[i], "--min-kernels")) target = &min_kernels;
    else if (!strcmp(argv[i], "--min-replayed")) target = &min_replayed;
    else if (!strcmp(argv[i], "--force-refuse-all")) target = &force_refuse_all;
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    *target = (int)value;
  }
  if (points_count < 4 || points_count > 100000 || coord < 2 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || clouds < 1 || clouds > 10000 || force_refuse_all < 0 ||
      force_refuse_all > 1) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }

  {
    const int fixture_faults = refusal_fixture() + job_contract_fixture();
    if (fixture_faults != 0) {
      printf("ECHEC : %d faute(s) de fixture\n", fixture_faults);
      return 1;
    }
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  mhgp3v::device::WavefrontReceipt receipt;
  long long decided_clouds = 0, refused_clouds = 0, kernels = 0;
  long long job_refusals = 0;
  int shell_high_water = 0, interior_high_water = 0;
  [[maybe_unused]] double device_milliseconds = 0.0;
  [[maybe_unused]] long long device_vertices = 0;

  for (int c = 0; c < clouds; ++c) {
    std::vector<P3> pts;
    for (int guard = 0; (int)pts.size() < points_count && guard < 200 * points_count; ++guard) {
      const P3 q = pt(pick(rng), pick(rng), pick(rng));
      bool seen = false;
      for (const P3& r : pts) if (r.x == q.x && r.y == q.y && r.z == q.z) seen = true;
      if (!seen) pts.push_back(q);
    }
    if ((int)pts.size() < points_count) { printf("ECHEC : nuage %d non genere\n", c); return 3; }

    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    mhgp3v::CertifiedIndex tree;
    tree.build(pts, 16);
    const auto seen_vertices = mhgp3v::navigate_shallow(pts, smax - 2, &st, &status, false, &tree);
    if (status != mhgp3v::CloudStatus::kOk) {
      printf("  nuage %d : statut %s, ignore\n", c, mhgp3v::cloud_status_name(status));
      ++refused_clouds;
      continue;
    }

    // La base indépendante du potentiel de niveau zéro, lue comme le parcours la lit.
    std::vector<i32> root_base;
    for (const auto& v : seen_vertices) {
      if (v.level != 0) continue;
      for (i32 z : v.shell) {
        if (root_base.size() >= 4) break;
        const std::size_t have = root_base.size();
        if (have == 1) {
          const P3& a = pts[(std::size_t)root_base[0]];
          const P3& b = pts[(std::size_t)z];
          if (a.x == b.x && a.y == b.y && a.z == b.z) continue;
        } else if (have == 2) {
          const P3 u = mhgp::p3_sub(pts[(std::size_t)root_base[1]], pts[(std::size_t)root_base[0]]);
          const P3 w = mhgp::p3_sub(pts[(std::size_t)z], pts[(std::size_t)root_base[0]]);
          const P3 cr = mhgp::p3_cross(u, w);
          if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
        } else if (have == 3) {
          if (mhgp3v::flats::orient3d_exact(pts[(std::size_t)root_base[0]],
                                            pts[(std::size_t)root_base[1]],
                                            pts[(std::size_t)root_base[2]],
                                            pts[(std::size_t)z]) == 0) continue;
        }
        root_base.push_back(z);
      }
      break;
    }
    if (root_base.size() != 4) { printf("ECHEC : base independante absente au nuage %d\n", c); return 3; }

    // L'ORACLE NON BORNÉ SUR TOUS LES SOMMETS, avant toute admission. C'est le
    // terme de droite de l'identité de masse, et il ignore complètement ce que
    // le noyau borné acceptera ou refusera.
    std::vector<ReferenceVertex> reference;
    reference.reserve(seen_vertices.size());
    for (const auto& v : seen_vertices) {
      reference.push_back(reference_vertex(pts, v, root_base));
      receipt.reference_flats += reference.back().flats;
      receipt.reference_couples += reference.back().couples;
    }

    // Le LOT : les sommets admis par le noyau borné, à plat. Les refus
    // d'admission ne DISPARAISSENT plus : ils sont comptés par raison et rejoués.
    std::vector<mhgp3v::device::BoundedVertex> batch;
    std::vector<std::size_t> origin;
    mhgp3v::device::AdmissionStats astats;
    for (std::size_t index = 0; index < seen_vertices.size(); ++index) {
      const auto& v = seen_vertices[index];
      mhgp3v::device::BoundedVertex bv;
      const mhgp3v::device::Admission verdict =
          force_refuse_all != 0 ? mhgp3v::device::Admission::kShellOverflow
                                : mhgp3v::device::admit(v, &bv, &astats);
      if (verdict != mhgp3v::device::Admission::kOk) {
        switch (verdict) {
          case mhgp3v::device::Admission::kShellOverflow:
            ++receipt.refused_admission_shell; break;
          case mhgp3v::device::Admission::kInteriorAboveContract:
            ++receipt.refused_admission_interior; break;
          case mhgp3v::device::Admission::kShellTooSmall:
            ++receipt.refused_admission_too_small; break;
          case mhgp3v::device::Admission::kOk: break;
        }
        // LE REJEU. Un refus non rejoué est un trou dans la sortie, pas une
        // statistique : il entre dans `replayed` et dans la masse de l'union.
        receipt.replayed_flats += reference[index].flats;
        receipt.replayed_couples += reference[index].couples;
        ++receipt.replayed;
        continue;
      }
      batch.push_back(bv);
      origin.push_back(index);
    }
    shell_high_water = std::max<int>(shell_high_water, (int)astats.shell_high_water);
    interior_high_water = std::max<int>(interior_high_water, (int)astats.interior_high_water);
    ++decided_clouds;
    if (batch.empty()) continue;

    mhgp3v::device::WavefrontJob job;
    job.points = pts.data();
    job.point_count = (int)pts.size();
    job.root_base = root_base.data();
    job.root_size = (int)root_base.size();
    job.vertices = batch.data();
    job.vertex_count = (int)batch.size();
    job.declared_cloud_digest = mhgp3v::device::cloud_digest(pts.data(), (int)pts.size());
    // LE LOT EST AUTHENTIFIÉ AVANT LANCEMENT, jamais après.
    const mhgp3v::device::JobVerdict job_verdict = mhgp3v::device::validate_job(job);
    if (job_verdict != mhgp3v::device::JobVerdict::kValid) {
      printf("ECHEC : lot du nuage %d refuse — %s\n", c,
             mhgp3v::device::job_verdict_name(job_verdict));
      ++job_refusals;
      return 3;
    }

    std::vector<mhgp3v::device::VertexVerdict> host(batch.size());
    for (int i = 0; i < job.vertex_count; ++i)
      mhgp3v::device::evaluate_vertex(job, i, &host[(std::size_t)i]);
    ++kernels;

    // LA RÉFÉRENCE est le chemin CPU NON BORNÉ, pas une seconde écriture du même
    // corps : c'est ce qui rend la comparaison informative. Elle est comparée pour
    // CHAQUE statut, et la signature ordonnée en fait partie.
    for (int i = 0; i < job.vertex_count; ++i) {
      const ReferenceVertex& r = reference[origin[(std::size_t)i]];
      const mhgp3v::device::VertexVerdict& h = host[(std::size_t)i];
      if (h.status == (int)mhgp3v::device::VerdictStatus::kInvariantViolation) {
        // Toute fermeture est incluse dans une coquille déjà bornée : ce statut
        // est impossible après admission. Il est FATAL, jamais rejoué.
        printf("ECHEC : violation d'invariant au nuage %d, sommet %d\n", c, i);
        ++receipt.mismatches;
        continue;
      }
      if (h.admissible != r.prefix_mask || h.signature != r.prefix_signature ||
          h.flat_count != r.prefix_flats)
        ++receipt.mismatches;
      if (h.status == (int)mhgp3v::device::VerdictStatus::kFlatOverflow) {
        if (r.flats <= mhgp3v::device::kMaxFlatsPerVertex) ++receipt.mismatches;
        receipt.replayed_flats += r.flats;
        receipt.replayed_couples += r.couples;
        ++receipt.replayed;
      } else if (r.flats != h.flat_count) {
        ++receipt.mismatches;
      }
    }

    mhgp3v::device::summarise_into(host.data(), job.vertex_count, &receipt);

#if defined(MHGP3V_WAVEFRONT_CUDA)
    // LE DEVICE, si et seulement si le binaire a été compilé pour lui.
    {
      std::vector<mhgp3v::device::VertexVerdict> gpu(batch.size());
      double milliseconds = 0.0;
      char error[256] = {0};
      if (!mhgp3v::device::launch_wavefront_on_gpu(job, gpu.data(), &milliseconds, error,
                                                   (int)sizeof error)) {
        printf("ECHEC : lancement device — %s\n", error);
        return 4;
      }
      const long long bad =
          mhgp3v::device::count_mismatches(host.data(), gpu.data(), job.vertex_count);
      receipt.mismatches += bad;
      device_milliseconds += milliseconds;
      device_vertices += job.vertex_count;
    }
#endif
  }

  printf("provenance : --clouds %d --points %d --coord %d --smax %d --seed %lld"
         " --force-refuse-all %d\n", clouds, points_count, coord, smax, seed, force_refuse_all);
  printf("nuages     : decides=%lld  refuses pour statut=%lld  noyaux lances=%lld"
         "  lots refuses par le contrat=%lld\n", decided_clouds, refused_clouds, kernels,
         job_refusals);
  printf("lot        : presentes=%lld  acceptes=%lld  flats committes=%lld"
         "  couples committes=%lld\n", receipt.presented, receipt.accepted,
         receipt.committed_flats, receipt.committed_couples);
  printf("refus      : capacite flats=%lld  coquille=%lld  interieur hors contrat=%lld"
         "  coquille trop petite=%lld  total=%lld\n", receipt.refused_flat_overflow,
         receipt.refused_admission_shell, receipt.refused_admission_interior,
         receipt.refused_admission_too_small, receipt.total_refused());
  printf("ledger     : refuses=%lld = rejoues=%lld + en attente=%lld   fatals=%lld\n",
         receipt.total_refused(), receipt.replayed, receipt.pending, receipt.fatal_invariant);
  printf("masse      : committee+rejouee = %lld+%lld flats et %lld+%lld couples,"
         "  reference complete = %lld flats et %lld couples\n", receipt.committed_flats,
         receipt.replayed_flats, receipt.committed_couples, receipt.replayed_couples,
         receipt.reference_flats, receipt.reference_couples);
  printf("capacites  : flats/sommet max=%d (committe %d, capacite %d)  coquille max=%d"
         " (capacite %d)  interieur max=%d (contrat %d)\n", receipt.flat_high_water,
         receipt.committed_flat_high_water, mhgp3v::device::kMaxFlatsPerVertex, shell_high_water,
         mhgp3v::device::kMaxShell, interior_high_water, mhgp3v::device::kMaxInterior);
#if defined(MHGP3V_WAVEFRONT_CUDA)
  printf("device     : sommets=%lld  temps kernel=%.3f ms  debit=%.0f sommets/s\n",
         device_vertices, device_milliseconds,
         device_milliseconds > 0.0 ? 1000.0 * (double)device_vertices / device_milliseconds : 0.0);
#else
  printf("device     : non compile (binaire hote seul)\n");
#endif
  printf("\n%lld desaccords\n", receipt.mismatches);

  // LES PLANCHERS SONT SEPARES. Un plancher agrege se satisfait d'un seul
  // chiffre : c'est exactement ainsi qu'un mutant refusant tout restait vert.
  struct Floor { const char* name; long long value; int required; };
  const Floor floors[] = {
      {"nuages decides", decided_clouds, min_clouds},
      {"sommets acceptes", receipt.accepted, min_accepted},
      {"flats committes", receipt.committed_flats, min_flats},
      {"couples committes", receipt.committed_couples, min_couples},
      {"noyaux lances", kernels, min_kernels},
      {"refus", receipt.total_refused(), min_refused},
      {"rejeux", receipt.replayed, min_replayed},
  };
  for (const Floor& f : floors)
    if (f.value < f.required) {
      printf("ECHEC : plancher « %s » non atteint — %lld/%d\n", f.name, f.value, f.required);
      return 3;
    }

  if (receipt.fatal_invariant != 0) {
    printf("ECHEC : %lld violation(s) d'invariant — une fermeture ne peut pas depasser une"
           " coquille deja bornee\n", receipt.fatal_invariant);
    return 3;
  }
  if (!receipt.ledger_balances()) {
    printf("ECHEC : le ledger ne balance pas — %lld refuses contre %lld rejoues et %lld en"
           " attente\n", receipt.total_refused(), receipt.replayed, receipt.pending);
    return 3;
  }
  if (!receipt.mass_identity()) {
    printf("ECHEC : identite de masse rompue — %lld+%lld flats contre %lld, %lld+%lld couples"
           " contre %lld\n", receipt.committed_flats, receipt.replayed_flats,
           receipt.reference_flats, receipt.committed_couples, receipt.replayed_couples,
           receipt.reference_couples);
    return 3;
  }
  if (receipt.presented == 0) {
    printf("ECHEC : lot vide, la campagne ne mesure rien\n");
    return 3;
  }
  if (receipt.mismatches != 0) return 1;
  printf("OK : le noyau borne rend exactement les verdicts du chemin non borne, refus juges et"
         " rejoues, masse conservee\n");
  return 0;
}
