// MorseHGP3D v3 — LA SONDE DE MASSE DE LA SOURCE PAR CELLULES DE CENTRES.
//
// L'experience demandee par la reponse auditeur du 11 aout (« Preflight et
// critere de decision a 50 k ») : AVANT tout kernel de predicats, mesurer les
// masses de la source locale par cellule de centre — sans enumerer un seul
// tuple. Le theoreme cellule--candidat garantit :
//
//   pour une miniboule propre de support U (|U| = q), de centre possede par la
//   cellule C et d'interieur strict I avec q + |I| <= s_max, le rayon carre
//   verifie beta < Q_{q,C} et TOUT le census vit dans la dilation A_{q,C},
//
// avec t_q = s_max - q + 1 temoins QUELCONQUES W de la cellule et
// Q_{q,C} = 1 + max_{w in W} max_{y in coins(C)} ||w - y||^2. La sonde publie,
// par arite q dans {2,3,4} :
//
//   B_q      = nombre de cellules (l'espace des CENTRES est pave, les
//              cellules vides comprises — un centre critique peut tomber dans
//              une cellule Morton vide) ;
//   L_{A,q}  = somme des m_{q,C} = |A_{q,C}| ;
//   R_q      = somme des C(m_{q,C}, q)          (tuples a former) ;
//   T_q^max  = somme des m_{q,C} * C(m_{q,C},q) (borne census par tuple) ;
//
// plus la distribution p50/p95/p99/max de m_{q,C}, les cellules lourdes et
// les temps par etage. Les temoins sont choisis par anneaux de cellules avec
// borne d'arret exacte : le top-t_q du score de coin separable
// g_C(w) = somme_i max((w_i - lo_i)^2, (w_i - hi_i)^2) est ATTEINT (des ex
// aequo quelconques restant valides). Tout est entier 64 bits : coordonnees
// u16, distances carrees < 2^35, accumulateurs en __int128 satures et
// verifies.
//
// C'EST UN DIAGNOSTIC DE MASSE, PAS UN VERDICT : aucune sphere n'est
// calculee, aucun tuple n'est forme. Si ces masses debordent le budget, un
// kernel aveugle ne sauvera rien (il faut une autre partition, jamais un
// cap) ; si elles passent, M2 executera les tuples sur EXACTEMENT le meme
// manifeste. `--verify-bruteforce 1` recompte chaque m_{q,C} par balayage
// complet des n points — l'identite exacte de la marche par anneaux.
//
// Codes : 0 OK ; 1 identite violee ; 2 CLI ; 3 nuage/plancher.
#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cell_prune.hpp"
#include "prototype/cloud_families.hpp"

namespace {

using i64 = long long;
using i128 = __int128;

struct Percentiles {
  i64 p50 = 0, p95 = 0, p99 = 0, max = 0;
};

Percentiles percentiles_of(std::vector<i64>* values) {
  Percentiles out;
  if (values->empty()) return out;
  std::sort(values->begin(), values->end());
  const auto at = [&](double p) {
    const std::size_t index = (std::size_t)((double)(values->size() - 1) * p + 0.5);
    return (*values)[std::min(index, values->size() - 1)];
  };
  out.p50 = at(0.50);
  out.p95 = at(0.95);
  out.p99 = at(0.99);
  out.max = values->back();
  return out;
}

double to_double(i128 value) { return (double)value; }

// C(m, q) en 128 bits — m <= 100000, q <= 4 : produit < 2^68, exact.
i128 binomial(i64 m, int q) {
  if (m < q) return 0;
  i128 numerator = 1;
  for (int i = 0; i < q; ++i) numerator *= (i128)(m - i);
  i64 factorial = 1;
  for (int i = 2; i <= q; ++i) factorial *= i;
  return numerator / factorial;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, smax = 11, cell_side = 0, verify_bruteforce = 0, threads = 0;
  i64 seed = 20260810, heavy_r = 100000;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (i64)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else if (!strcmp(argv[i], "scanline_single_pass"))
        family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (!strcmp(argv[i], "scanline_overlap_multiecho"))
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--smax")) smax = (int)value;
    else if (!strcmp(argv[i], "--cell-side")) cell_side = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--heavy-r")) heavy_r = value;
    else if (!strcmp(argv[i], "--verify-bruteforce")) verify_bruteforce = (int)value;
    else if (!strcmp(argv[i], "--threads")) threads = (int)value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 12 || n > 100000 || coord < 0 || coord > 65536 || smax < 5 || smax > 32 ||
      cell_side < 0 || cell_side > 4096 || heavy_r < 1 || verify_bruteforce < 0 ||
      verify_bruteforce > 1 || threads < 0 || threads > 256) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  if (threads == 0) threads = std::max(1, (int)std::thread::hardware_concurrency());
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }

  // La boite racine : produit des [min_i, max_i + 1), fermetures entieres.
  i64 lo[3] = {65536, 65536, 65536}, hi[3] = {0, 0, 0};
  for (const mhgp::P3& p : pts) {
    const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
    for (int d = 0; d < 3; ++d) {
      lo[d] = std::min(lo[d], c[d]);
      hi[d] = std::max(hi[d], c[d] + 1);
    }
  }
  // Le pas par defaut vise environ t_2 = smax - 1 points par cellule OCCUPEE
  // sur l'epaisseur reelle du nuage — jamais moins de 2.
  if (cell_side == 0) {
    const double volume = (double)(hi[0] - lo[0]) * (double)(hi[1] - lo[1]) *
                          (double)(hi[2] - lo[2]);
    const double target = volume * (double)(smax - 1) / (double)n;
    double side = 1.0;
    while (side * side * side < target && side < 4096.0) side += 1.0;
    cell_side = (int)std::max(2.0, side);
  }
  const i64 side = cell_side;
  const i64 grid[3] = {(hi[0] - lo[0] + side - 1) / side, (hi[1] - lo[1] + side - 1) / side,
                       (hi[2] - lo[2] + side - 1) / side};
  const i64 total_cells = grid[0] * grid[1] * grid[2];
  if (total_cells <= 0 || total_cells > 50000000) {
    std::printf("ECHEC : grille de %lld cellules hors sonde (pas %lld)\n", total_cells, side);
    return 3;
  }
  std::printf("provenance : --points %d --coord %d --smax %d --seed %lld --family %s"
              " --cell-side %lld\n", n, coord, smax, seed, mhgp3v::cloud_family_name(family),
              side);
  std::printf("boite      : [%lld,%lld)x[%lld,%lld)x[%lld,%lld) — grille %lldx%lldx%lld ="
              " %lld cellules half-open (l'espace des CENTRES, vides comprises)\n",
              lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], grid[0], grid[1], grid[2],
              total_cells);

  // Les points par cellule (CSR simple), pour les anneaux.
  std::vector<std::vector<int>> bucket((std::size_t)total_cells);
  const auto cell_of = [&](const mhgp::P3& p) {
    const i64 cx = ((i64)p.x - lo[0]) / side;
    const i64 cy = ((i64)p.y - lo[1]) / side;
    const i64 cz = ((i64)p.z - lo[2]) / side;
    return (cx * grid[1] + cy) * grid[2] + cz;
  };
  for (int i = 0; i < n; ++i) bucket[(std::size_t)cell_of(pts[(std::size_t)i])].push_back(i);

  // La fermeture de la cellule (cx,cy,cz) : [lo + c*side, min(lo+(c+1)*side, hi)].
  const auto cell_box = [&](i64 cx, i64 cy, i64 cz, i64* box_lo, i64* box_hi) {
    const i64 c[3] = {cx, cy, cz};
    for (int d = 0; d < 3; ++d) {
      box_lo[d] = lo[d] + c[d] * side;
      box_hi[d] = std::min(box_lo[d] + side, hi[d]);
    }
  };
  // g_C(w) : le score de coin separable — max des coins de la fermeture.
  const auto corner_score = [&](const mhgp::P3& w, const i64* box_lo, const i64* box_hi) {
    i64 total = 0;
    const i64 c[3] = {(i64)w.x, (i64)w.y, (i64)w.z};
    for (int d = 0; d < 3; ++d) {
      const i64 a = c[d] - box_lo[d], b = c[d] - box_hi[d];
      total += std::max(a * a, b * b);
    }
    return total;
  };
  // dist^2(point, fermeture) et dist^2(fermeture, fermeture) — exacts.
  const auto point_gap = [&](const mhgp::P3& p, const i64* box_lo, const i64* box_hi) {
    i64 total = 0;
    const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
    for (int d = 0; d < 3; ++d) {
      i64 gap = 0;
      if (c[d] < box_lo[d]) gap = box_lo[d] - c[d];
      else if (c[d] > box_hi[d]) gap = c[d] - box_hi[d];
      total += gap * gap;
    }
    return total;
  };

  // L'ITERATION DE COQUILLE exacte : seules les cellules a distance L-infini
  // exactement `ring` sont visitees — jamais le cube plein re-enumere.
  const auto for_ring = [&](i64 cx, i64 cy, i64 cz, i64 ring, auto&& fn) {
    if (ring == 0) {
      fn(cx, cy, cz);
      return;
    }
    for (i64 dx = -ring; dx <= ring; ++dx) {
      const i64 ux = cx + dx;
      if (ux < 0 || ux >= grid[0]) continue;
      const bool face_x = dx == -ring || dx == ring;
      for (i64 dy = -ring; dy <= ring; ++dy) {
        const i64 uy = cy + dy;
        if (uy < 0 || uy >= grid[1]) continue;
        if (face_x || dy == -ring || dy == ring) {
          for (i64 dz = -ring; dz <= ring; ++dz) {
            const i64 uz = cz + dz;
            if (uz >= 0 && uz < grid[2]) fn(ux, uy, uz);
          }
        } else {
          if (cz - ring >= 0) fn(ux, uy, cz - ring);
          if (cz + ring < grid[2]) fn(ux, uy, cz + ring);
        }
      }
    }
  };

  for (int q = 2; q <= 4; ++q) {
    const int tq = smax - q + 1;
    if (n < tq) { std::printf("ECHEC : n < t_q, fallback racine hors sonde\n"); return 3; }
    const auto s0 = std::chrono::steady_clock::now();
    struct Slice {
      std::vector<i64> masses;
      i128 sum_m = 0, sum_r = 0, sum_t = 0;
      // LE PRUNE CONVEXE (reponse pont/q4 20260811 §6) : closure(C) et
      // conv(A_C) strictement separes => la cellule ne possede aucun support
      // propre de la BRANCHE beta<Q ; l'autre branche (tous temoins
      // interieurs) est normalized_h0_inert — jamais un « no_support ». Le
      // predicat partage `cell_prune` certifie en entiers : six axes puis la
      // direction du barycentre (le plan general) — le flottant propose, la
      // verification exacte decide, sans certificat la cellule est gardee.
      i128 sum_r_pruned = 0, sum_t_pruned = 0;
      i64 pruned_axis = 0, pruned_plane = 0, heavy = 0, empty = 0, rings_max = 0;
    };
    std::vector<Slice> slices((std::size_t)threads);
    std::atomic<i64> next_x{0};
    std::atomic<bool> failed{false};
    std::mutex failure_mutex;
    std::string failure_message;
    int failure_code = 0;
    const i64 ring_limit = std::max(grid[0], std::max(grid[1], grid[2]));
    const auto worker = [&](int w) {
      Slice& slice = slices[(std::size_t)w];
      std::vector<std::pair<i64, int>> best;   // (score, point) — top t_q
      std::vector<mhgp::P3> dilation;          // A_C materialisee pour le prune
      for (i64 cx = next_x.fetch_add(1); cx < grid[0] && !failed.load();
           cx = next_x.fetch_add(1))
        for (i64 cy = 0; cy < grid[1] && !failed.load(); ++cy)
          for (i64 cz = 0; cz < grid[2]; ++cz) {
            i64 box_lo[3], box_hi[3];
            cell_box(cx, cy, cz, box_lo, box_hi);
            if (bucket[(std::size_t)((cx * grid[1] + cy) * grid[2] + cz)].empty())
              ++slice.empty;
            // 1. LES TEMOINS : top-t_q du score de coin par anneaux, arret
            // quand l'anneau suivant ne peut plus ameliorer — le score
            // terminal est ATTEINT (des temoins quelconques resteraient
            // valides ; le top donne le meilleur Q possible pour ce pas).
            best.clear();
            for (i64 ring = 0; ring <= ring_limit; ++ring) {
              if ((int)best.size() >= tq) {
                const i64 floor_gap = (ring - 1) * side;
                if (ring >= 1 && floor_gap * floor_gap > best.back().first) break;
              }
              for_ring(cx, cy, cz, ring, [&](i64 ux, i64 uy, i64 uz) {
                for (int p : bucket[(std::size_t)((ux * grid[1] + uy) * grid[2] + uz)]) {
                  const i64 score = corner_score(pts[(std::size_t)p], box_lo, box_hi);
                  if ((int)best.size() < tq) {
                    best.push_back({score, p});
                    std::sort(best.begin(), best.end());
                  } else if (score < best.back().first) {
                    best.back() = {score, p};
                    std::sort(best.begin(), best.end());
                  }
                }
              });
              slice.rings_max = std::max(slice.rings_max, ring);
            }
            if ((int)best.size() < tq) {
              std::lock_guard<std::mutex> lock(failure_mutex);
              failure_message = "ECHEC : temoins insuffisants";
              failure_code = 3;
              failed.store(true);
              return;
            }
            const i64 q_cell = 1 + best.back().first;
            // 2. LA DILATION : m = |{x : dist^2(x, fermeture) < Q}| par
            // anneaux, coupure exacte quand l'anneau ne peut plus toucher.
            i64 m = 0;
            dilation.clear();
            for (i64 ring = 0; ring <= ring_limit; ++ring) {
              const i64 floor_gap = (ring - 1) * side;
              if (ring >= 1 && floor_gap * floor_gap >= q_cell) break;
              for_ring(cx, cy, cz, ring, [&](i64 ux, i64 uy, i64 uz) {
                for (int p : bucket[(std::size_t)((ux * grid[1] + uy) * grid[2] + uz)])
                  if (point_gap(pts[(std::size_t)p], box_lo, box_hi) < q_cell) {
                    ++m;
                    dilation.push_back(pts[(std::size_t)p]);
                  }
              });
            }
            const mhgp::i64 prune_lo[3] = {(mhgp::i64)box_lo[0], (mhgp::i64)box_lo[1],
                                           (mhgp::i64)box_lo[2]};
            const mhgp::i64 prune_hi[3] = {(mhgp::i64)box_hi[0], (mhgp::i64)box_hi[1],
                                           (mhgp::i64)box_hi[2]};
            const mhgp3v::CellPruneVerdict prune_verdict = mhgp3v::cell_prune(
                prune_lo, prune_hi, [&](std::size_t i) { return dilation[i]; },
                dilation.size());
            const bool separated = prune_verdict.separated;
            if (verify_bruteforce == 1) {
              i64 check = 0;
              for (const mhgp::P3& p : pts)
                if (point_gap(p, box_lo, box_hi) < q_cell) ++check;
              if (check != m) {
                std::lock_guard<std::mutex> lock(failure_mutex);
                failure_message = "ECHEC : identite de dilation violee (anneaux != balayage)";
                failure_code = 1;
                failed.store(true);
                return;
              }
            }
            slice.masses.push_back(m);
            slice.sum_m += m;
            const i128 r_cell = binomial(m, q);
            slice.sum_r += r_cell;
            slice.sum_t += (i128)m * r_cell;
            if (separated) {
              if (prune_verdict.by_axis) ++slice.pruned_axis;
              else ++slice.pruned_plane;
            } else {
              slice.sum_r_pruned += r_cell;
              slice.sum_t_pruned += (i128)m * r_cell;
            }
            if (r_cell > (i128)heavy_r) ++slice.heavy;
          }
    };
    {
      std::vector<std::thread> pool;
      for (int w = 0; w < threads; ++w) pool.push_back(std::thread(worker, w));
      for (std::thread& t : pool) t.join();
    }
    if (failed.load()) {
      std::printf("%s\n", failure_message.c_str());
      return failure_code;
    }
    std::vector<i64> masses;
    i128 sum_m = 0, sum_r = 0, sum_t = 0, sum_r_pruned = 0, sum_t_pruned = 0;
    i64 heavy_cells = 0, empty_cells = 0, witness_rings_max = 0;
    i64 pruned_axis = 0, pruned_plane = 0;
    for (Slice& slice : slices) {
      masses.insert(masses.end(), slice.masses.begin(), slice.masses.end());
      sum_m += slice.sum_m;
      sum_r += slice.sum_r;
      sum_t += slice.sum_t;
      sum_r_pruned += slice.sum_r_pruned;
      sum_t_pruned += slice.sum_t_pruned;
      pruned_axis += slice.pruned_axis;
      pruned_plane += slice.pruned_plane;
      heavy_cells += slice.heavy;
      empty_cells += slice.empty;
      witness_rings_max = std::max(witness_rings_max, slice.rings_max);
    }
    const auto s1 = std::chrono::steady_clock::now();
    Percentiles pm = percentiles_of(&masses);
    std::printf("q=%d t_q=%-2d : B=%lld cellules (%lld vides) — m p50=%lld p95=%lld p99=%lld"
                " max=%lld — anneaux temoins max=%lld\n", q, tq, total_cells, empty_cells,
                pm.p50, pm.p95, pm.p99, pm.max, witness_rings_max);
    std::printf("           : L_A=%.6e  R=%.6e tuples  T_max=%.6e census — cellules lourdes"
                " (R>%lld)=%lld — %.3f s (%d threads)\n", to_double(sum_m), to_double(sum_r),
                to_double(sum_t), heavy_r, heavy_cells,
                std::chrono::duration<double>(s1 - s0).count(), threads);
    std::printf("           : prune convexe = %lld cellules (axe %lld + plan %lld) —"
                " branche beta<Q vide, l'autre branche normalized_h0_inert — R'=%.6e"
                " T'=%.6e\n",
                pruned_axis + pruned_plane, pruned_axis, pruned_plane,
                to_double(sum_r_pruned), to_double(sum_t_pruned));
  }
  std::printf("OK : sonde de masse de la source par cellules terminee (aucun tuple forme —"
              " l'admission 50 k se lit sur R_q et l'arene, jamais sur un cap)\n");
  return 0;
}
