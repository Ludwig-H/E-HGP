// MorseHGP3D v3 — LA QUALIFICATION DU KERNEL FACE-OWNER.
//
// nuage -> flat_catalogue -> chemin CPU (collect_edges) -> kernel device par
// ordre -> comparaison ARETE PAR ARETE du flux (lot d'activation, owner,
// membre) -> temps par phase et manifeste VRAM. Le differentiel est le
// verdict : une seule arete divergente refuse l'ordre entier. Le mutant
// --force-drop-edge retire la derniere arete device et doit rougir — il
// prouve que le comparateur mord.
//
// Sans MHGP3V_FACEOWNER_CUDA, le binaire refuse proprement : il ne pretend
// jamais un chemin device qu'il n'a pas.
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/order_k_flats.hpp"
#include "prototype/parallel_catalogue.hpp"
#include "prototype/saturated_fold_faceowner.hpp"
#include "prototype/faceowner_device.hpp"

int main(int argc, char** argv) {
  int n = 64, coord = 0, smax = 11, max_order = 5;
  long long seed = 20260810;
  int drop_edge = 0, timing_only = 0, catalogue_threads = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kUniform;
  auto integer = [](const char* text, long long* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--max-order")) target = &max_order;
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else if (!strcmp(argv[i], "--force-drop-edge")) target = &drop_edge;
    else if (!strcmp(argv[i], "--timing-only")) target = &timing_only;
    else if (!strcmp(argv[i], "--catalogue-threads")) target = &catalogue_threads;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  if (n < 5 || n > 100000 || coord < 0 || coord > 65536 || smax < 2 ||
      smax > mhgp::kMaxRank || max_order < 1 || max_order > 6 || max_order + 1 > smax ||
      drop_edge < 0 || drop_edge > 1 || timing_only < 0 || timing_only > 1 ||
      catalogue_threads < 0 || catalogue_threads > 256) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // Le mutant drop-edge n'a de sens que si le comparateur tourne : en mode
  // chrono il survivrait par construction, donc la combinaison est refusee.
  if (drop_edge == 1 && timing_only == 1) {
    std::printf("ECHEC : --force-drop-edge exige le differentiel (pas --timing-only)\n");
    return 2;
  }
#ifndef MHGP3V_FACEOWNER_CUDA
  (void)family;   // la famille ne sert qu'au chemin CUDA ; le refus reste premier
  std::printf("ECHEC : kernel face-owner absent de ce binaire (MHGP3V_ENABLE_CUDA)\n");
  return 2;
#else
  // La MEME autorite de generation que le pipeline (cloud_families.hpp) : le
  // meme (famille, n, coord, graine) produit le meme nuage bit a bit ici.
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
  // CONTRAT DE CARDINALITE (audit etat courant) : egalite explicite a la
  // frontiere du consommateur, jamais le seul test `< n`.
  if ((int)pts.size() != n) {
    std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                " demandes\n", pts.size(), n);
    return 1;
  }

  mhgp3v::FlatStatistics st{};
  mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
  const auto catalogue_begin = std::chrono::steady_clock::now();
  const mhgp::Catalogue catalogue =
      catalogue_threads > 1
          ? mhgp3v::flat_catalogue_parallel(pts, smax, &st, &status, catalogue_threads)
          : mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
  const auto catalogue_end = std::chrono::steady_clock::now();
  if (status != mhgp3v::CloudStatus::kOk) {
    std::printf("ECHEC : statut nuage %s\n", mhgp3v::cloud_status_name(status));
    return 3;
  }

  // LE CHEMIN CPU DE VERITE, avec le flux d'aretes exporte. En mode chrono il
  // est OMIS ET DECLARE : l'exactitude du kernel est portee par la
  // qualification differentielle aux tailles ou elle est payable, jamais par
  // le run chrono lui-meme — qui garde neanmoins la validation hostile du
  // kernel (masse binomiale exacte par generateur) et l'admission VRAM.
  mhgp3v::FaceOwnerReceipt receipt;
  auto cpu_begin = std::chrono::steady_clock::now();
  if (timing_only == 0) {
    const mhgp3v::SaturatedFold fold = mhgp3v::build_saturated_fold_faceowner(
        catalogue, max_order, /*keep_partitions=*/false, &receipt,
        /*enforce_event_guard=*/smax >= n, {}, 0, /*collect_edges=*/true);
    if (!fold.ok) { std::printf("ECHEC : fold CPU refuse : %s\n", fold.refusal); return 3; }
  }
  const auto cpu_end = std::chrono::steady_clock::now();

  // LES MEMES TABLEAUX DENSES que le fold : membres compresses, lots, rang
  // d'activation canonique — toute derive est attrapee par les aretes.
  const std::size_t count = catalogue.spheres.size();
  std::vector<std::vector<mhgp::i32>> members(count);
  std::vector<mhgp::i32> universe;
  for (std::size_t s = 0; s < count; ++s) {
    const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
    members[s].assign(catalogue.members.begin() + sphere.members_begin,
                      catalogue.members.begin() + sphere.members_begin + sphere.rank);
    universe.insert(universe.end(), members[s].begin(), members[s].end());
  }
  std::sort(universe.begin(), universe.end());
  universe.erase(std::unique(universe.begin(), universe.end()), universe.end());
  for (std::vector<mhgp::i32>& generator_members : members)
    for (mhgp::i32& x : generator_members)
      x = (mhgp::i32)(std::lower_bound(universe.begin(), universe.end(), x) -
                      universe.begin());
  std::vector<int> by_level((std::size_t)count);
  for (std::size_t s = 0; s < count; ++s) by_level[s] = (int)s;
  std::sort(by_level.begin(), by_level.end(), [&](int x, int y) {
    const int c = mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)x].sph,
                                        catalogue.spheres[(std::size_t)y].sph);
    if (c != 0) return c < 0;
    return x < y;
  });
  std::vector<int> batch_of((std::size_t)count, 0), activation_rank((std::size_t)count, 0);
  {
    std::size_t cursor = 0;
    int batch_index = 0;
    while (cursor < count) {
      std::size_t batch_end = cursor + 1;
      while (batch_end < count &&
             mhgp::sphere_cmp_beta(catalogue.spheres[(std::size_t)by_level[cursor]].sph,
                                   catalogue.spheres[(std::size_t)by_level[batch_end]].sph) == 0)
        ++batch_end;
      for (std::size_t b = cursor; b < batch_end; ++b) {
        batch_of[(std::size_t)by_level[b]] = batch_index;
        activation_rank[(std::size_t)by_level[b]] = (int)b;
      }
      ++batch_index;
      cursor = batch_end;
    }
  }
  std::vector<int> members_csr;
  std::vector<long long> member_offsets;
  std::vector<int> ranks;
  for (std::size_t s = 0; s < count; ++s) {
    member_offsets.push_back((long long)members_csr.size());
    for (mhgp::i32 x : members[s]) members_csr.push_back((int)x);
    ranks.push_back((int)members[s].size());
  }

  // MANIFESTE VRAM avant tout lancement : NO-GO au-dela de 70 % (regle
  // d'admission de la note postings §8).
  long long free_bytes = 0, total_bytes = 0;
  char error[256] = {0};
  if (!mhgp3v::device::query_device_memory(&free_bytes, &total_bytes, error,
                                           (int)sizeof(error))) {
    std::printf("ECHEC : device indisponible : %s\n", error);
    return 3;
  }
  std::printf("device     : VRAM libre %.1f Go / totale %.1f Go\n",
              (double)free_bytes / 1073741824.0, (double)total_bytes / 1073741824.0);

  std::printf("provenance : --points %d --coord %d --smax %d --max-order %d --seed %lld"
              " --family %s --timing-only %d --catalogue-threads %d\n",
              n, coord, smax, max_order, seed, mhgp3v::cloud_family_name(family), timing_only,
              catalogue_threads);
  std::printf("catalogue  : %zu generateurs en %.3f s (%s)\n", count,
              std::chrono::duration<double>(catalogue_end - catalogue_begin).count(),
              catalogue_threads > 1 ? "parallele" : "sequentiel");
  if (timing_only == 0)
    std::printf("fold CPU   : %.3f s (aretes exportees)\n",
                std::chrono::duration<double>(cpu_end - cpu_begin).count());
  else
    std::printf("fold CPU   : OMIS — chrono device seulement, exactitude portee par la"
                " qualification differentielle aux tailles payables\n");

  long long divergences = 0;
  double device_total_ms = 0.0;
  for (int k = 1; k <= max_order; ++k) {
    std::vector<long long> incidence_offsets;
    long long running = 0;
    for (std::size_t s = 0; s < count; ++s) {
      incidence_offsets.push_back(running);
      const long long rank = ranks[s];
      if (rank >= k) {
        unsigned long long binomial = 1;
        for (int i = 0; i < k; ++i) binomial = binomial * (unsigned long long)(rank - i) /
                                               (unsigned long long)(i + 1);
        running += (long long)binomial;
      }
    }
    incidence_offsets.push_back(running);
    // Admission : ~56 octets device par incidence (structures + buffers de
    // tri) — NO-GO au-dela de 70 % de la VRAM libre.
    const long long projected = running * 56 + (long long)members_csr.size() * 4;
    if (projected > free_bytes * 7 / 10) {
      std::printf("ECHEC : admission VRAM refusee a k=%d — %.1f Go projetes\n", k,
                  (double)projected / 1073741824.0);
      return 3;
    }
    mhgp3v::device::FaceOwnerDeviceResult device_result;
    if (!mhgp3v::device::run_faceowner_order_on_gpu(members_csr, member_offsets,
                                                    incidence_offsets, ranks,
                                                    activation_rank, batch_of, k,
                                                    &device_result, error,
                                                    (int)sizeof(error))) {
      std::printf("ECHEC : kernel k=%d : %s\n", k, error);
      return 3;
    }
    if (drop_edge == 1 && !device_result.edges.empty()) device_result.edges.pop_back();
    bool same = true;
    if (timing_only == 0) {
      const std::vector<std::pair<std::pair<int, int>, int>>& cpu_edges =
          receipt.edges_k[(std::size_t)(k - 1)];
      same = cpu_edges.size() == device_result.edges.size() &&
             device_result.incidences == receipt.incidences_k[(std::size_t)(k - 1)] &&
             device_result.signatures == receipt.unique_signatures_k[(std::size_t)(k - 1)];
      for (std::size_t e = 0; same && e < cpu_edges.size(); ++e)
        same = cpu_edges[e].first.first == device_result.edges[e].activation_batch &&
               cpu_edges[e].first.second == device_result.edges[e].owner &&
               cpu_edges[e].second == device_result.edges[e].member;
      if (!same) ++divergences;
    }
    device_total_ms += device_result.emit_milliseconds + device_result.sort_milliseconds +
                       device_result.reduce_milliseconds;
    std::printf("k=%d        : incidences=%lld signatures=%lld aretes=%zu %s — emission"
                " %.2f ms, tri %.2f ms, reduction %.2f ms, ~%.1f Mo device\n", k,
                device_result.incidences, device_result.signatures,
                device_result.edges.size(),
                timing_only == 1 ? "NON COMPAREES (chrono)"
                : same           ? "IDENTIQUES au CPU"
                                 : "DIVERGENTES",
                device_result.emit_milliseconds, device_result.sort_milliseconds,
                device_result.reduce_milliseconds,
                (double)device_result.device_bytes / 1048576.0);
  }
  std::printf("device     : total %.2f ms pour K=%d — contre %.3f s de fold CPU complet"
              " (rejeu DSU hote non inclus dans le chiffre device)\n", device_total_ms,
              max_order, std::chrono::duration<double>(cpu_end - cpu_begin).count());
  if (divergences != 0) {
    std::printf("ECHEC : %lld ordre(s) au flux d'aretes DIVERGENT\n", divergences);
    return 1;
  }
  if (timing_only == 1) {
    std::printf("OK : CHRONO SEULEMENT — differentiel d'aretes OMIS ET DECLARE ;"
                " validation hostile du kernel et admission VRAM conservees\n");
    return 0;
  }
  std::printf("OK : flux d'aretes device == CPU arete par arete sur les %d ordres\n",
              max_order);
  return 0;
#endif
}
