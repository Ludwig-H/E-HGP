// Porte causale du cout de la tete (constat H4 ; verificateur du premier tour : la porte par compteur laissait vivre
// trois remontees quadratiques non comptees), 30 septembre 2026. Observable externe : l'horloge murale.
//
// Chenille de q + 1 feuilles de deux points (rang 0) ; s_i (i = 1..q) fusionne s_(i-1) (la feuille 0 pour i = 1) et
// la feuille i au rang i, et porte un point entre a ce rang (il sort du cluster de s_i). mcs = 2 : toutes les
// composantes sont grosses, le condense a 2q + 1 clusters et une profondeur q. Niveaux L0 + r (r = 0..q), z = 1.
//   A (L0 = 1) : les enfants l'emportent partout ; EOM et feuilles, avec et sans allow_single, retiennent les q + 1
//     feuilles : chaque feuille retenue (desactivation EOM), chaque cluster de la chenille (etiquette du premier
//     ancetre retenu) et chaque point de fusion (etiquette de son cluster) remonterait jusqu'a la racine ;
//   B (L0 = 2^30) : EOM avec allow_single, la racine l'emporte (regle de la racine, desactivation de tous ses
//     descendants) ; EOM sans allow_single : s_1 et les feuilles 2..q retenues.
// Une remontee par cluster ou par point y fait ~ q^2 / 2 pas : a q = 2^20, plus de 5e11 lectures de parent, soit
// environ 960 s de CPU par execution en Release (1,7 ns par pas mesure de q = 2^14 a 2^17), 64 fois la limite ; le
// code lineaire rend chaque execution en moins d'une seconde. Chaque execution de cluster() a une limite d'horloge
// murale kLimitSeconds (15 s ; x 8 sous ASan ou TSan, dont le code lineaire est 7 a 15 fois plus lent) ; un chien de
// garde sort avec le code 3 des qu'elle est depassee, sans attendre la fin d'un calcul quadratique (CTest TIMEOUT en
// second rempart).
// Juge : la partition attendue de chaque chemin, ecrite par definition, et le nombre de clusters retenus
// (lineaire). Le compteur Clustering::ancestor_steps n'est pas lu : la porte n'utilise que l'API de cluster().
//
//   mhgp10_head_complexity                  porte (q = 2^20, limite kLimitSeconds) : ligne head_complexity_ok
//   mhgp10_head_complexity Q LIMITE         diagnostic (mesure des mutants a petite taille) : ligne
//                                           head_complexity_diagnostic, jamais la ligne de la porte
// Codes : 0 conforme, 1 partition differente du juge, 3 limite murale depassee.
#include <time.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "head/head.hpp"

using namespace mhgp10;

namespace {

using Clock = std::chrono::steady_clock;

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define MHGP10_COMPLEXITY_SANITIZED 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define MHGP10_COMPLEXITY_SANITIZED 1
#endif
#endif
#ifdef MHGP10_COMPLEXITY_SANITIZED
constexpr double kSanitizerFactor = 8.0;
#else
constexpr double kSanitizerFactor = 1.0;
#endif

constexpr u32 kGateQ = 1u << 20;
constexpr double kLimitSeconds = 15.0 * kSanitizerFactor;

int failures = 0;

void expect(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::printf("ECHEC %s\n", what.c_str());
  }
}

double thread_cpu_seconds() {
  timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
}

// Chien de garde : une execution armee qui depasse sa limite d'horloge murale termine la porte (code 3).
class Watchdog {
 public:
  Watchdog() : thread_([this] { run(); }) {}
  ~Watchdog() {
    {
      std::lock_guard<std::mutex> lock(m_);
      stop_ = true;
    }
    cv_.notify_all();
    thread_.join();
  }
  void arm(const std::string& what, double seconds) {
    std::lock_guard<std::mutex> lock(m_);
    what_ = what;
    limit_ = seconds;
    deadline_ = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
    armed_ = true;
    cv_.notify_all();
  }
  void disarm() {
    std::lock_guard<std::mutex> lock(m_);
    armed_ = false;
    cv_.notify_all();
  }

 private:
  void run() {
    std::unique_lock<std::mutex> lock(m_);
    while (!stop_) {
      if (!armed_) {
        cv_.wait(lock);
        continue;
      }
      if (cv_.wait_until(lock, deadline_) == std::cv_status::timeout && armed_ && Clock::now() >= deadline_) {
        std::printf("PLAFOND %s : limite murale de %.1f s depassee\n", what_.c_str(), limit_);
        std::fflush(stdout);
        std::_Exit(3);
      }
    }
  }
  std::mutex m_;
  std::condition_variable cv_;
  bool stop_ = false, armed_ = false;
  Clock::time_point deadline_{};
  std::string what_;
  double limit_ = 0;
  std::thread thread_;  // dernier membre : demarre une fois les autres initialises
};

// Chenille (voir en tete). Points : 2l, 2l + 1 pour la feuille l ; 2(q + 1) + i - 1 pour la fusion s_i.
PointDendrogram caterpillar(u32 q, double l0) {
  PointDendrogram d;
  const u32 nodes = 2 * q + 1;
  d.level.resize(u64(q) + 1);
  for (u32 r = 0; r <= q; ++r) d.level[r] = l0 + r;
  d.node_rank.assign(nodes, 0);
  d.parent.assign(nodes, kNone);
  d.child_off.assign(u64(nodes) + 1, 0);
  d.child_val.reserve(2 * u64(q));
  for (u32 i = 1; i <= q; ++i) {
    const u32 v = q + i, left = i == 1 ? 0 : v - 1;
    d.node_rank[v] = i;
    d.child_val.push_back(left);
    d.child_val.push_back(i);
    d.parent[left] = d.parent[i] = v;
    d.child_off[v + 1] = static_cast<u32>(d.child_val.size());
  }
  const u64 pts = 2 * (u64(q) + 1) + q;
  d.point_node.reserve(pts);
  d.point_rank.reserve(pts);
  d.point_weight.assign(pts, 1);
  for (u32 l = 0; l <= q; ++l)
    for (int j = 0; j < 2; ++j) {
      d.point_node.push_back(l);
      d.point_rank.push_back(0);
    }
  for (u32 i = 1; i <= q; ++i) {
    d.point_node.push_back(q + i);
    d.point_rank.push_back(i);
  }
  return d;
}

// Partition attendue (groupe par point, -1 bruit) et nombre de clusters retenus, par definition.
enum class Path { leaves, root, s1_and_leaves };

std::vector<i32> expected_groups(u32 q, Path path, u64& clusters) {
  std::vector<i32> g(2 * (u64(q) + 1) + q, -1);
  for (u32 l = 0; l <= q; ++l) {
    i32 grp = static_cast<i32>(l);
    if (path == Path::root) grp = 0;
    if (path == Path::s1_and_leaves) grp = l <= 1 ? 0 : static_cast<i32>(l) - 1;
    g[2 * u64(l)] = g[2 * u64(l) + 1] = grp;
  }
  if (path == Path::root)
    for (u32 i = 1; i <= q; ++i) g[2 * (u64(q) + 1) + i - 1] = 0;
  if (path == Path::s1_and_leaves) g[2 * (u64(q) + 1)] = 0;  // le point de s_1 sort du cluster de s_1
  clusters = path == Path::leaves ? u64(q) + 1 : path == Path::root ? 1 : q;
  return g;
}

// Egalite de partitions en temps lineaire (etiquettes et groupes denses), bruit fixe.
bool same_partition(const std::vector<i32>& got, const std::vector<i32>& want, u64 groups) {
  if (got.size() != want.size()) return false;
  std::vector<i32> fwd(want.size() + 1, -1), bwd(groups + 1, -1);
  for (size_t x = 0; x < got.size(); ++x) {
    const i32 a = got[x], b = want[x];
    if ((a < 0) != (b < 0)) return false;
    if (a < 0) continue;
    if (u64(a) >= fwd.size() || u64(b) >= bwd.size()) return false;
    if (fwd[a] < 0) fwd[a] = b;
    if (bwd[b] < 0) bwd[b] = a;
    if (fwd[a] != b || bwd[b] != a) return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  const bool gate = argc == 1;
  if (!gate && argc != 3) return 2;
  const u32 q = gate ? kGateQ : static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
  const double limit = gate ? kLimitSeconds : std::strtod(argv[2], nullptr);
  if (q < 4 || q > (1u << 28) || !(limit > 0)) return 2;
  struct Run {
    Selection sel;
    bool single;
    Path path;
  };
  struct Comb {
    const char* name;
    double l0;
    std::vector<Run> runs;
  };
  const Comb combs[] = {
      {"A", 1.0,
       {{Selection::eom, false, Path::leaves},
        {Selection::eom, true, Path::leaves},
        {Selection::leaf, false, Path::leaves},
        {Selection::leaf, true, Path::leaves}}},
      {"B", 1073741824.0, {{Selection::eom, true, Path::root}, {Selection::eom, false, Path::s1_and_leaves}}},
  };
  Watchdog dog;
  double worst = 0;
  u32 runs = 0;
  for (const Comb& c : combs) {
    const PointDendrogram d = caterpillar(q, c.l0);
    expect(validate(d).ok(), std::string("chenille ") + c.name + " refusee par validate");
    for (const Run& r : c.runs) {
      ClusterParams p;
      p.min_cluster_size = 2;
      p.z = 1.0;
      p.selection = r.sel;
      p.allow_single_cluster = r.single;
      const std::string tag = std::string("chenille ") + c.name + " q=" + std::to_string(q) +
                              (r.sel == Selection::eom ? " eom" : " feuilles") + (r.single ? " allow_single" : "");
      std::fflush(stdout);
      dog.arm(tag, limit);
      const auto t0 = Clock::now();
      const double c0 = thread_cpu_seconds();
      const Clustering cl = cluster(d, p);
      const double c1 = thread_cpu_seconds();
      const double wall = std::chrono::duration<double>(Clock::now() - t0).count();
      dog.disarm();
      if (wall > limit) {
        std::printf("PLAFOND %s : %.3f s > %.1f s\n", tag.c_str(), wall, limit);
        return 3;
      }
      worst = std::max(worst, wall);
      u64 clusters = 0;
      const std::vector<i32> want = expected_groups(q, r.path, clusters);
      const bool ok = cl.selected.size() == clusters && same_partition(cl.label, want, u64(q) + 1);
      expect(ok, tag + " : partition ou nombre de clusters (" + std::to_string(cl.selected.size()) + ", attendu " +
                     std::to_string(clusters) + ") differents du juge");
      std::printf("%s : %zu clusters condenses, %zu retenus, mur %.3f s, CPU du fil %.3f s (limite %.1f s)\n",
                  tag.c_str(), cl.tree.parent.size(), cl.selected.size(), wall, c1 - c0, limit);
      ++runs;
    }
  }
  std::printf("pire execution : %.3f s, limite %.1f s (facteur d'instrumentation %.0f), %u executions\n", worst, limit,
              kSanitizerFactor, runs);
  if (failures) {
    std::printf("head_complexity : %d echecs\n", failures);
    return 1;
  }
  if (runs != 6) return 3;
  std::printf(gate ? "head_complexity_ok\n" : "head_complexity_diagnostic\n");
  return 0;
}
