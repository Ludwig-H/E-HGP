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

static P3 pt(int x, int y, int z) {
  P3 p{};
  p.x = (i32)x; p.y = (i32)y; p.z = (i32)z;
  return p;
}

int main(int argc, char** argv) {
  int points_count = 24, coord = 4000, smax = 8, clouds = 8, min_refused = 0;
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
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    *target = (int)value;
  }
  if (points_count < 4 || points_count > 100000 || coord < 2 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || clouds < 1 || clouds > 10000) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  long long total_vertices = 0, total_flats = 0, total_admissible = 0, total_refused = 0;
  long long total_mismatch = 0;
  int flat_high_water = 0, shell_high_water = 0, interior_high_water = 0;
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

    // Le LOT : les sommets admis par le noyau borné, à plat.
    std::vector<mhgp3v::device::BoundedVertex> batch;
    std::vector<const mhgp3v::flats::Vertex*> origin;
    mhgp3v::device::AdmissionStats astats;
    for (const auto& v : seen_vertices) {
      mhgp3v::device::BoundedVertex bv;
      if (mhgp3v::device::admit(v, &bv, &astats) != mhgp3v::device::Admission::kOk) continue;
      batch.push_back(bv);
      origin.push_back(&v);
    }
    shell_high_water = std::max<int>(shell_high_water, (int)astats.shell_high_water);
    interior_high_water = std::max<int>(interior_high_water, (int)astats.interior_high_water);
    if (batch.empty()) continue;

    mhgp3v::device::WavefrontJob job;
    job.points = pts.data();
    job.point_count = (int)pts.size();
    job.root_base = root_base.data();
    job.root_size = (int)root_base.size();
    job.vertices = batch.data();
    job.vertex_count = (int)batch.size();

    std::vector<mhgp3v::device::VertexVerdict> host(batch.size());
    for (int i = 0; i < job.vertex_count; ++i)
      mhgp3v::device::evaluate_vertex(job, i, &host[(std::size_t)i]);

    // LA RÉFÉRENCE est le chemin CPU NON BORNÉ, pas une seconde écriture du même
    // corps : c'est ce qui rend la comparaison informative.
    for (int i = 0; i < job.vertex_count; ++i) {
      if (host[(std::size_t)i].status != (int)mhgp3v::device::VerdictStatus::kOk) continue;
      unsigned long long expected = 0;
      int flats = 0;
      bool over = false;
      mhgp3v::flats::for_each_flat(pts, *origin[(std::size_t)i],
                                   [&](const mhgp3v::flats::FlatAtVertex& f) {
        if (flats >= mhgp3v::device::kMaxFlatsPerVertex) { over = true; return false; }
        for (int slot = 0; slot < 2; ++slot) {
          const int direction = slot == 0 ? -1 : 1;
          if (mhgp3v::flats::pair_admissible(pts, *origin[(std::size_t)i], f.base, direction,
                                             root_base))
            expected |= (1ULL << (2 * flats + slot));
        }
        ++flats;
        return true;
      });
      if (over || expected != host[(std::size_t)i].admissible ||
          flats != host[(std::size_t)i].flat_count)
        ++total_mismatch;
    }

    const mhgp3v::device::WavefrontReceipt receipt =
        mhgp3v::device::summarise(host.data(), job.vertex_count);
    total_vertices += receipt.vertices;
    total_flats += receipt.flats;
    total_admissible += receipt.admissible_couples;
    total_refused += receipt.refused;
    flat_high_water = std::max(flat_high_water, receipt.flat_high_water);

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
      total_mismatch += bad;
      device_milliseconds += milliseconds;
      device_vertices += job.vertex_count;
    }
#endif
  }

  printf("provenance : --clouds %d --points %d --coord %d --smax %d --seed %lld\n", clouds,
         points_count, coord, smax, seed);
  printf("lot        : sommets=%lld  flats=%lld  couples admissibles=%lld  refuses=%lld\n",
         total_vertices, total_flats, total_admissible, total_refused);
  printf("capacites  : flats/sommet max=%d (capacite %d)  coquille max=%d (capacite %d)"
         "  interieur max=%d (capacite %d)\n", flat_high_water,
         mhgp3v::device::kMaxFlatsPerVertex, shell_high_water, mhgp3v::device::kMaxShell,
         interior_high_water, mhgp3v::device::kMaxInterior);
#if defined(MHGP3V_WAVEFRONT_CUDA)
  printf("device     : sommets=%lld  temps kernel=%.3f ms  debit=%.0f sommets/s\n",
         device_vertices, device_milliseconds,
         device_milliseconds > 0.0 ? 1000.0 * (double)device_vertices / device_milliseconds : 0.0);
#else
  printf("device     : non compile (binaire hote seul)\n");
#endif
  printf("\n%lld desaccords\n", total_mismatch);
  if (total_vertices == 0) {
    printf("ECHEC : lot vide, la campagne ne mesure rien\n");
    return 3;
  }
  // LE REFUS DOIT ETRE EXERCE. Une capacite dont le depassement n'arrive jamais
  // n'est pas prouvee bornee : elle est seulement jamais atteinte. Le plancher
  // impose donc une campagne ou le refus se produit, et ou les sommets admis
  // concordent quand meme.
  if (total_refused < min_refused) {
    printf("ECHEC : plancher de refus non atteint — %lld/%d\n", total_refused, min_refused);
    return 3;
  }
  if (total_mismatch != 0) return 1;
  printf("OK : le noyau borne rend exactement les verdicts du chemin non borne\n");
  return 0;
}
