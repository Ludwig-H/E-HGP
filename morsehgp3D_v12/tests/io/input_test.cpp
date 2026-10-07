// Portes de la lecture des nuages u32le (read_u32le, check_u32le_sizes) :
//   input             decodage petit-boutiste, PointId arbitraires (0xFFFFFFFF compris), plusieurs blocs de lecture,
//                     empreintes et tailles des fichiers, fichiers vides admis puis refuses par prepare_cloud ;
//   input_sizes       tailles incoherentes : input_unreadable AVANT toute allocation (pic nul), au moins kNone
//                     points (fichiers creux) : index_overflow_u32, puis admission refusee avant allocation ;
//   input_unreadable  absent, vide de nom, dossier, tube, peripherique, fichier plus long ou plus court que sa taille
//                     annoncee (procfs, sysfs) : input_unreadable ;
//   input_budget      admission au plus juste : 16 octets par point, refus memory_budget sans rien reserver.
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "cloud/cloud.hpp"
#include "io_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::io_test;

namespace {

constexpr u64 kGiB = u64{1} << 30;

struct Files {
  std::vector<u8> points, ids;
};

// n points de coordonnees et identifiants graves : x = 0x04030201 + i, y = 7 i, z = 0xFFFFFFFF - i, id = ~(3 i).
Files make_files(u32 n) {
  Files f;
  for (u32 i = 0; i < n; ++i) {
    put32(f.points, 0x04030201u + i);
    put32(f.points, 7 * i);
    put32(f.points, 0xFFFFFFFFu - i);
    put32(f.ids, ~(3 * i));
  }
  return f;
}

bool write_pair(const Scratch& s, const Files& f) {
  return write_file(s.path("points.u32le"), f.points) && write_file(s.path("ids.u32le"), f.ids);
}

// Refus attendu, rien de reserve, et aucune reservation n'a jamais eu lieu (pic nul).
void expect_refusal_before_allocation(const std::string& points, const std::string& ids, Reason reason) {
  MemoryBudget budget(kGiB);
  const Result<io::InputFiles> r = io::read_u32le(points.c_str(), ids.c_str(), budget);
  CHECK_EQ(r.outcome().reason, reason);
  CHECK_EQ(budget.used(), 0u);
  CHECK_EQ(budget.peak(), 0u);
}

// Fichier creux de `bytes` octets (aucun bloc ecrit).
bool sparse(const std::string& path, u64 bytes) {
  const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) return false;
  const bool sized = ::ftruncate(fd, static_cast<off_t>(bytes)) == 0;
  return ::close(fd) == 0 && sized;
}

}  // namespace

MHGP12_TEST(input, 32) {
  Scratch s;
  REQUIRE(s.ok());
  for (u32 n : {3u, 10000u}) {  // un bloc ; trois blocs dont le dernier partiel
    const Files f = make_files(n);
    REQUIRE(write_pair(s, f));
    MemoryBudget budget(kGiB);
    Result<io::InputFiles> r = io::read_u32le(s.path("points.u32le").c_str(), s.path("ids.u32le").c_str(), budget);
    REQUIRE(r.ok());
    const io::InputFiles& in = r.value();
    CHECK_EQ(in.x.size(), n);
    CHECK_EQ(in.ids.size(), n);
    bool same = true;
    for (u32 i = 0; i < n; ++i)
      same = same && in.x[i] == 0x04030201u + i && in.y[i] == 7 * i && in.z[i] == 0xFFFFFFFFu - i &&
             idx(in.ids[i]) == ~(3 * i);
    CHECK(same);
    CHECK_EQ(in.points_bytes, 12u * n);
    CHECK_EQ(in.ids_bytes, 4u * n);
    CHECK(in.points_sha256 == digest_of(f.points));
    CHECK(in.ids_sha256 == digest_of(f.ids));
    CHECK_EQ(budget.used(), 16u * n);
  }
  // premier point decode octet par octet : 01 02 03 04 -> 0x04030201 ; identifiant 0 -> ~0 = 0xFFFFFFFF
  {
    MemoryBudget budget(kGiB);
    const Files f = make_files(1);
    CHECK_EQ(f.points[0], 1u);
    CHECK_EQ(f.points[3], 4u);
    REQUIRE(write_pair(s, f));
    const Result<io::InputFiles> r =
        io::read_u32le(s.path("points.u32le").c_str(), s.path("ids.u32le").c_str(), budget);
    REQUIRE(r.ok());
    CHECK_EQ(r.value().x[0], 0x04030201u);
    CHECK_EQ(idx(r.value().ids[0]), 0xFFFFFFFFu);
  }
  // deux fichiers vides : lus (aucun point), puis le nuage vide est refuse par prepare_cloud
  {
    REQUIRE(write_pair(s, Files{}));
    MemoryBudget budget(kGiB);
    const Result<io::InputFiles> r =
        io::read_u32le(s.path("points.u32le").c_str(), s.path("ids.u32le").c_str(), budget);
    REQUIRE(r.ok());
    CHECK_EQ(r.value().x.size(), 0u);
    CHECK(r.value().points_sha256 == digest_of(std::vector<u8>{}));
    const io::InputFiles& in = r.value();
    const Result<Cloud> cloud = prepare_cloud(in.x.span(), in.y.span(), in.z.span(), in.ids.span(), CoordWidth(),
                                              budget);
    CHECK_EQ(cloud.outcome().reason, Reason::empty_input);
  }
}

MHGP12_TEST(input_sizes, 31) {
  // controle pur des tailles
  CHECK(io::check_u32le_sizes(0, 0).ok());
  CHECK(io::check_u32le_sizes(24, 8).ok());
  CHECK_EQ(io::check_u32le_sizes(13, 4).reason, Reason::input_unreadable);
  CHECK_EQ(io::check_u32le_sizes(24, 4).reason, Reason::input_unreadable);
  CHECK_EQ(io::check_u32le_sizes(24, 12).reason, Reason::input_unreadable);
  CHECK_EQ(io::check_u32le_sizes(12, 6).reason, Reason::input_unreadable);
  CHECK_EQ(io::check_u32le_sizes(12 * u64{kNone}, 4 * u64{kNone}).reason, Reason::index_overflow_u32);
  CHECK(io::check_u32le_sizes(12 * u64{kNone - 1}, 4 * u64{kNone - 1}).ok());

  Scratch s;
  REQUIRE(s.ok());
  const std::string points = s.path("points.u32le"), ids = s.path("ids.u32le");
  Files f = make_files(2);
  // points de 25 octets (un mot surnumeraire), ids de 4 octets pour 2 points, ids de 9 octets
  f.points.push_back(0);
  REQUIRE(write_pair(s, f));
  expect_refusal_before_allocation(points, ids, Reason::input_unreadable);
  f = make_files(2);
  f.ids.resize(4);
  REQUIRE(write_pair(s, f));
  expect_refusal_before_allocation(points, ids, Reason::input_unreadable);
  f = make_files(2);
  f.ids.push_back(0);
  REQUIRE(write_pair(s, f));
  expect_refusal_before_allocation(points, ids, Reason::input_unreadable);
  // au moins kNone points, annonces par des fichiers creux : refus avant toute allocation et toute lecture
  REQUIRE(sparse(points, 12 * u64{kNone}));
  REQUIRE(sparse(ids, 4 * u64{kNone}));
  expect_refusal_before_allocation(points, ids, Reason::index_overflow_u32);
  // kNone - 1 points : tailles admises, mais 64 Gio ne tiennent pas dans 1 Gio, refus a l'admission
  REQUIRE(sparse(points, 12 * u64{kNone - 1}));
  REQUIRE(sparse(ids, 4 * u64{kNone - 1}));
  expect_refusal_before_allocation(points, ids, Reason::memory_budget);
}

MHGP12_TEST(input_unreadable, 25) {
  Scratch s;
  REQUIRE(s.ok());
  const Files f = make_files(4);
  REQUIRE(write_pair(s, f));
  const std::string points = s.path("points.u32le"), ids = s.path("ids.u32le");
  MemoryBudget budget(kGiB);
  const auto refused = [&](const char* p, const char* i) {
    const Result<io::InputFiles> r = io::read_u32le(p, i, budget);
    CHECK_EQ(r.outcome().reason, Reason::input_unreadable);
    CHECK_EQ(budget.used(), 0u);
  };
  refused(s.path("absent").c_str(), ids.c_str());
  refused(points.c_str(), s.path("absent").c_str());
  refused(nullptr, ids.c_str());
  refused("", ids.c_str());
  refused(s.root().c_str(), ids.c_str());  // dossier
  REQUIRE(::mkfifo(s.path("fifo").c_str(), 0600) == 0);
  refused(s.path("fifo").c_str(), ids.c_str());  // tube sans ecrivain : refuse sans bloquer
  refused("/dev/null", "/dev/null");             // peripherique
  // fichier plus long que sa taille annoncee (procfs annonce 0 octet) : un octet de plus apres les octets annonces
  REQUIRE(write_pair(s, Files{}));
  refused("/proc/self/stat", ids.c_str());
  // fichier plus court que sa taille annoncee (sysfs annonce 4096 octets) : lecture incomplete
  const char* const candidates[] = {"/sys/devices/system/cpu/online", "/sys/power/state",
                                    "/sys/kernel/mm/transparent_hugepage/enabled"};
  const char* shorter = nullptr;
  u64 announced = 0;
  for (const char* c : candidates) {
    struct stat st {};
    if (::stat(c, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size % 4 == 0) {
      shorter = c;
      announced = static_cast<u64>(st.st_size);
      break;
    }
  }
  REQUIRE(shorter != nullptr);
  REQUIRE(write_file(points, std::vector<u8>(3 * announced, 0)));
  refused(points.c_str(), shorter);
  CHECK_EQ(budget.peak(), 16u * (announced / 4));  // admis puis rendu : la lecture a bien commence
}

MHGP12_TEST(input_budget, 9) {
  Scratch s;
  REQUIRE(s.ok());
  REQUIRE(write_pair(s, make_files(100)));
  const std::string points = s.path("points.u32le"), ids = s.path("ids.u32le");
  {
    MemoryBudget budget(16 * 100 - 1);
    const Result<io::InputFiles> r = io::read_u32le(points.c_str(), ids.c_str(), budget);
    CHECK_EQ(r.outcome().reason, Reason::memory_budget);
    CHECK_EQ(budget.used(), 0u);
    CHECK_EQ(budget.peak(), 0u);  // refus a l'admission, avant le premier tampon
  }
  {
    MemoryBudget budget(16 * 100);
    Result<io::InputFiles> r = io::read_u32le(points.c_str(), ids.c_str(), budget);
    CHECK(r.ok());
    CHECK_EQ(budget.used(), 1600u);
    io::InputFiles taken = std::move(r).take();
    CHECK_EQ(taken.z.size(), 100u);
    taken = io::InputFiles{};
    CHECK(budget.released().ok());
  }
}
