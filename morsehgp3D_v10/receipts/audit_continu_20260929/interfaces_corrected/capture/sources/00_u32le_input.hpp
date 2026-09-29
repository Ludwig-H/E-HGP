// Lecture commune des nuages u32le des sondes (cli/ et temoin tests/head/mreach_cluster) : mots de 32 bits
// petit-boutistes, un point = trois mots x y z, PointId = rang d'entree.
//
// Frontiere d'entree (audits du 29 septembre 2026, fins de fichier incompletes acceptees) : le fichier est lu EN ENTIER
// ou refuse AVANT tout calcul, jamais tronque en silence.
//   - fichier illisible, erreur de lecture, lecture incomplete : input_unreadable (invalid_input) ;
//   - taille non multiple de 12 octets (point incomplet, mot surnumeraire) : size_mismatch (invalid_input : les
//     tableaux x, y, z n'auraient pas la meme longueur) ;
//   - au moins kNone points : index_overflow_u32 (indices de points u32, kNone reserve), avant toute lecture quand la
//     taille est connue ;
//   - memoire insuffisante pour la lecture : memory_budget.
// Le fichier vide (empty_input) et le domaine des coordonnees (coordinate_out_of_domain) restent juges par
// prepare_cloud, sur TOUS les mots du fichier. check_u32le_size est pure : testee sans fichier (tests/unit).
#pragma once

#include <bit>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <new>
#include <vector>

#include "core/status.hpp"

namespace mhgp10 {

static_assert(std::endian::native == std::endian::little, "lecture u32le : hote petit-boutiste suppose");

struct U32leCloud {
  std::vector<u32> x, y, z, pid;
};

// Nombre de points d'un fichier u32le de `bytes` octets, ou refus (size_mismatch, index_overflow_u32).
inline Result<u64> check_u32le_size(u64 bytes) {
  if (bytes % 12 != 0) return fail(Reason::size_mismatch);
  if (bytes / 12 >= kNone) return fail(Reason::index_overflow_u32);
  return bytes / 12;
}

inline Result<U32leCloud> read_u32le_cloud(const char* path) {
  // taille annoncee d'un fichier regulier : refus avant lecture ; un tube ou un peripherique se lit en flux et se
  // juge a la fin, sur les octets effectivement lus
  std::error_code ec;
  const bool regular = std::filesystem::is_regular_file(path, ec);
  u64 announced = 0;
  if (regular) {
    announced = std::filesystem::file_size(path, ec);
    if (ec) return fail(Reason::input_unreadable);
    const Result<u64> points = check_u32le_size(announced);
    if (!points.ok()) return points.outcome();
  }
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return fail(Reason::input_unreadable);
  std::vector<unsigned char> bytes;
  bool io_error = false;
  try {
    if (regular) bytes.reserve(announced);
    std::vector<unsigned char> chunk(1u << 16);
    size_t got;
    while ((got = std::fread(chunk.data(), 1, chunk.size(), f)) > 0)
      bytes.insert(bytes.end(), chunk.data(), chunk.data() + got);
    io_error = std::ferror(f) != 0 || !std::feof(f);  // la boucle ne s'arrete proprement qu'en fin de fichier
  } catch (const std::bad_alloc&) {
    std::fclose(f);
    return fail(Reason::memory_budget);
  }
  std::fclose(f);
  // erreur de lecture, ou fichier regulier dont la taille a change pendant la lecture : lecture incomplete
  if (io_error || (regular && bytes.size() != announced)) return fail(Reason::input_unreadable);
  const Result<u64> points = check_u32le_size(bytes.size());
  if (!points.ok()) return points.outcome();
  const u64 n = points.value();
  U32leCloud c;
  try {
    c.x.resize(n);
    c.y.resize(n);
    c.z.resize(n);
    c.pid.resize(n);
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  }
  for (u64 i = 0; i < n; ++i) {
    std::memcpy(&c.x[i], bytes.data() + 12 * i, 4);
    std::memcpy(&c.y[i], bytes.data() + 12 * i + 4, 4);
    std::memcpy(&c.z[i], bytes.data() + 12 * i + 8, 4);
    c.pid[i] = static_cast<u32>(i);
  }
  return c;
}

}  // namespace mhgp10
