// Lecture entiere des nuages u32le (io.hpp, read_u32le). Port de src/cloud/u32le_input.hpp du raccord R2 de la v10
// (commit 865f5e6 ; docs/PROVENANCE.md, section io) et du lecteur des bancs bench/whole_input.hpp.
//
// Repris de la source : le fichier est lu EN ENTIER ou refuse avant tout calcul, jamais tronque en silence ; sa nature
// et sa taille sont lues par fstat, sans allocation, avant tout tampon ; une lecture incomplete ou un fichier dont la
// taille change pendant la lecture rend input_unreadable ; index_overflow_u32 est juge sur la taille annoncee.
// Ce qui change :
//   - deux fichiers, points.u32le et ids.u32le (bench/whole_input.hpp) ; la v10 prenait le rang d'entree pour PointId ;
//   - un tube ou un peripherique est refuse (input_unreadable) au lieu d'etre lu en flux : sans taille annoncee, les
//     tableaux ne pourraient pas etre admis dans le budget avant l'allocation (ARCHITECTURE.md de la v11,
//     paragraphe 7.1, aucun agrandissement par doublement). Ouverture en O_NONBLOCK : un tube sans ecrivain ne
//     bloque pas ;
//   - un nombre d'octets incoherent (points non multiple de 12, ids non multiple de 4, nombres de points differents)
//     rend input_unreadable, comme bench/whole_input.hpp ; la v10 rendait size_mismatch ;
//   - les quatre tableaux sont des Buffer admis dans le budget ; les mots sont decodes en petit-boutiste par
//     decalages (aucune hypothese sur l'hote) ; les empreintes SHA-256 des deux fichiers sont prises au fil de la
//     lecture, sur les octets lus.
// Portes : mhgp12_io_unit_input, _input_sizes, _input_unreadable, _input_budget ; mhgp12_io_fault_starvation.
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>

#include "io/io.hpp"

namespace mhgp12::io {

namespace {

// Points lus par bloc : 48 Kio de coordonnees et 16 Kio d'identifiants sur la pile.
inline constexpr u64 kBlockPoints = 4096;

// Descripteur ferme sur tout chemin de sortie.
struct Descriptor {
  int fd = -1;
  Descriptor() = default;
  Descriptor(const Descriptor&) = delete;
  Descriptor& operator=(const Descriptor&) = delete;
  ~Descriptor() {
    if (fd >= 0) ::close(fd);
  }
};

// Ouvre un fichier regulier en lecture et lit sa taille annoncee, sans allocation.
Outcome open_regular(const char* path, Descriptor& file, u64& bytes) noexcept {
  if (path == nullptr || *path == '\0') return fail(Reason::input_unreadable);
  file.fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  if (file.fd < 0) return fail(Reason::input_unreadable);
  struct stat st {};
  if (::fstat(file.fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) return fail(Reason::input_unreadable);
  bytes = static_cast<u64>(st.st_size);
  return {};
}

// Lit exactement n octets ; une fin de fichier prematuree ou une erreur rend input_unreadable.
Outcome read_exact(int fd, u8* out, u64 n) noexcept {
  u64 done = 0;
  while (done < n) {
    const ssize_t got = ::read(fd, out + done, static_cast<std::size_t>(n - done));
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) return fail(Reason::input_unreadable);
    done += static_cast<u64>(got);
  }
  return {};
}

// Apres les octets annonces, la fin du fichier doit suivre : un octet de plus (fichier qui a grandi, fichier special
// a taille annoncee nulle) rend input_unreadable.
Outcome expect_end(int fd) noexcept {
  u8 extra = 0;
  ssize_t got = 0;
  do {
    got = ::read(fd, &extra, 1);
  } while (got < 0 && errno == EINTR);
  return got == 0 ? Outcome{} : fail(Reason::input_unreadable);
}

constexpr u32 little(const u8* p) noexcept {
  return u32{p[0]} | (u32{p[1]} << 8) | (u32{p[2]} << 16) | (u32{p[3]} << 24);
}

// Lit les n points par blocs, decode et hache au fil de la lecture.
Outcome read_points(int points_fd, int ids_fd, u64 n, InputFiles& in) noexcept {
  Sha256 points_sha, ids_sha;
  std::array<u8, 12 * kBlockPoints> block{};
  std::array<u8, 4 * kBlockPoints> names{};
  for (u64 begin = 0; begin < n; begin += kBlockPoints) {
    const u64 m = std::min(kBlockPoints, n - begin);
    MHGP12_TRY(read_exact(points_fd, block.data(), 12 * m));
    MHGP12_TRY(read_exact(ids_fd, names.data(), 4 * m));
    points_sha.update(std::span<const u8>(block.data(), 12 * m));
    ids_sha.update(std::span<const u8>(names.data(), 4 * m));
    for (u64 j = 0; j < m; ++j) {
      in.x[begin + j] = little(block.data() + 12 * j);
      in.y[begin + j] = little(block.data() + 12 * j + 4);
      in.z[begin + j] = little(block.data() + 12 * j + 8);
      in.ids[begin + j] = make_id<PointId>(little(names.data() + 4 * j));
    }
  }
  MHGP12_TRY(expect_end(points_fd));
  MHGP12_TRY(expect_end(ids_fd));
  in.points_sha256 = points_sha.finish();
  in.ids_sha256 = ids_sha.finish();
  return {};
}

}  // namespace

Outcome check_u32le_sizes(u64 points_bytes, u64 ids_bytes) noexcept {
  if (points_bytes % 12 != 0 || ids_bytes % 4 != 0 || ids_bytes / 4 != points_bytes / 12)
    return fail(Reason::input_unreadable);
  const u64 n = points_bytes / 12;
  if (n == 0) return {};
  return check_cloud_sizes(n, n, n, n);  // seul refus possible ici : index_overflow_u32
}

Result<InputFiles> read_u32le(const char* points, const char* ids, MemoryBudget& budget) noexcept {
  Descriptor points_file, ids_file;
  u64 points_bytes = 0, ids_bytes = 0;
  MHGP12_TRY(open_regular(points, points_file, points_bytes));
  MHGP12_TRY(open_regular(ids, ids_file, ids_bytes));
  MHGP12_TRY(check_u32le_sizes(points_bytes, ids_bytes));
  const u64 n = points_bytes / 12;  // n < kNone : 16 n tient dans un u64
  MHGP12_TRY(budget.admit(16 * n));
  InputFiles in;
  MHGP12_TRY(in.x.allocate(n, budget));
  MHGP12_TRY(in.y.allocate(n, budget));
  MHGP12_TRY(in.z.allocate(n, budget));
  MHGP12_TRY(in.ids.allocate(n, budget));
  MHGP12_TRY(read_points(points_file.fd, ids_file.fd, n, in));
  in.points_bytes = points_bytes;
  in.ids_bytes = ids_bytes;
  return in;
}

}  // namespace mhgp12::io
