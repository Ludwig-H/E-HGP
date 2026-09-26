// MorseHGP3D v9 — ablation hote du telechargement des enregistrements des
// voies (levier pinned_records, 25 septembre 2026). Sans GPU : ne mesure que
// la part HOTE de la fenetre de telechargement, pour lire les reçus G4.
//
//   mhgp9_lanes_download_ablation <records> <pairs>
//
// Deux bras entrelaces par paire (ordre alterne d'une paire a l'autre), meme
// binaire, meme source residente de `records` enregistrements de 128 o :
//   - pageable : ce que fait l'hote sur le chemin temoin, hors DMA :
//     RawVector<LaneRecord>::resize (alloc_ms), puis une copie memcpy dans
//     ses pages neuves (copy_ms : la copie du pilote depuis ses tampons de
//     transit, premier contact des pages compris), puis la liberation
//     (free_ms, munmap) ;
//   - resident : le bail d'un bloc du bassin resident (alloc_ms), puis la
//     meme copie dans des pages deja touchees (copy_ms : la bande passante
//     seule, sans faute de page), puis le retour du bail (free_ms). Sur le
//     chemin epingle reel, la DMA ecrit le bloc sans copie hote : ce bras
//     separe seulement les fautes de page de la copie.
// Une ligne par bras et par paire, puis les medianes. Compteur independant
// de la charge : les fautes de page mineures de chaque bras (getrusage,
// ru_minflt), dont la copie dans des pages neuves paie une par page de
// 4 Kio (sans pages geantes transparentes). Murs indicatifs (hote partage) ;
// jamais un gain credite a l'epinglage sans reçu G4.
// Code 0, 2 argument.
#include <sys/resource.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/gpu/filter_runner.hpp"

namespace {

using mhgp9::gpu::LaneRecord;
using Clock = std::chrono::steady_clock;

double ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }

bool parse(const char* text, std::size_t& value) {
  const std::string s(text);
  if (s.empty() || s.size() > 12 || s.find_first_not_of("0123456789") != std::string::npos) return false;
  value = static_cast<std::size_t>(std::stoull(s));
  return true;
}

struct Arm {
  double alloc = 0, copy = 0, free = 0;
  long faults = 0;  // minor page faults of the arm
};

long minor_faults() {
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0) return -1;
  return usage.ru_minflt;
}

double median(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  return v.empty() ? 0.0 : v[v.size() / 2];
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t records = 0, pairs = 0;
  if (argc != 3 || !parse(argv[1], records) || !parse(argv[2], pairs) || records == 0 || records > 50000000 ||
      pairs == 0 || pairs > 100) {
    std::fprintf(stderr, "usage: mhgp9_lanes_download_ablation <records 1..5e7> <pairs 1..100>\n");
    return 2;
  }
  const std::size_t bytes = records * sizeof(LaneRecord);
  // The source stands for the device arena: resident, touched once.
  mhgp9::RawVector<LaneRecord> source(records);
  std::memset(static_cast<void*>(source.data()), 0x5a, bytes);
  mhgp9::gpu::AlignedHostMemory memory;
  mhgp9::gpu::RecordPool pool(memory);
  pool.reserve(records);  // the session's reservation (touched below by a first warm copy)
  {
    auto warm = pool.acquire(records);
    std::memcpy(static_cast<void*>(warm.data()), source.data(), bytes);
  }
  unsigned long long checksum = 0;
  const auto pageable = [&] {
    Arm arm;
    const long f0 = minor_faults();
    const auto t0 = Clock::now();
    {
      mhgp9::RawVector<LaneRecord> out;
      out.resize(records);
      const auto t1 = Clock::now();
      std::memcpy(static_cast<void*>(out.data()), source.data(), bytes);
      const auto t2 = Clock::now();
      checksum += out[records / 2].support[0];
      arm.alloc = ms(t0, t1);
      arm.copy = ms(t1, t2);
      const auto t3 = Clock::now();
      mhgp9::RawVector<LaneRecord>().swap(out);
      arm.free = ms(t3, Clock::now());
    }
    arm.faults = minor_faults() - f0;
    return arm;
  };
  const auto resident = [&] {
    Arm arm;
    const long f0 = minor_faults();
    const auto t0 = Clock::now();
    auto lease = pool.acquire(records);
    const auto t1 = Clock::now();
    std::memcpy(static_cast<void*>(lease.data()), source.data(), bytes);
    const auto t2 = Clock::now();
    checksum += lease.data()[records / 2].support[0];
    lease.reset();
    arm.alloc = ms(t0, t1);
    arm.copy = ms(t1, t2);
    arm.free = ms(t2, Clock::now());
    arm.faults = minor_faults() - f0;
    return arm;
  };
  std::vector<double> pa, pc, pf, ra, rc, rf, pflt, rflt;
  for (std::size_t p = 0; p < pairs; ++p) {
    Arm a, b;
    if (p % 2 == 0) {
      a = pageable();
      b = resident();
    } else {
      b = resident();
      a = pageable();
    }
    std::printf("pair=%zu order=%s pageable_alloc_ms=%.3f pageable_copy_ms=%.3f pageable_free_ms=%.3f "
                "pageable_faults=%ld resident_alloc_ms=%.3f resident_copy_ms=%.3f resident_free_ms=%.3f "
                "resident_faults=%ld\n",
                p, p % 2 == 0 ? "pageable_first" : "resident_first", a.alloc, a.copy, a.free, a.faults, b.alloc, b.copy,
                b.free, b.faults);
    pflt.push_back(static_cast<double>(a.faults));
    rflt.push_back(static_cast<double>(b.faults));
    pa.push_back(a.alloc);
    pc.push_back(a.copy);
    pf.push_back(a.free);
    ra.push_back(b.alloc);
    rc.push_back(b.copy);
    rf.push_back(b.free);
  }
  const auto stats = pool.stats();
  std::printf("lanes_download_ablation records=%zu bytes=%zu pairs=%zu median_pageable_alloc_ms=%.3f "
              "median_pageable_copy_ms=%.3f median_pageable_free_ms=%.3f median_resident_alloc_ms=%.3f "
              "median_resident_copy_ms=%.3f median_resident_free_ms=%.3f median_pageable_faults=%.0f "
              "median_resident_faults=%.0f pool_allocations=%llu pool_bytes=%llu checksum=%llu\n",
              records, bytes, pairs, median(pa), median(pc), median(pf), median(ra), median(rc), median(rf), median(pflt),
              median(rflt),
              static_cast<unsigned long long>(stats.allocations), static_cast<unsigned long long>(stats.bytes),
              checksum);
  return 0;
}
