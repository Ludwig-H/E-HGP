// Bibliotheque de test (hors produit), chargee par LD_PRELOAD : remplace toutes les fonctions globales d'allocation
// (scalaire, tableau, nothrow, et alignees, que prend std::vector d'un type alignas(64)) et fait echouer :
//  - la N-ieme allocation faite hors du fil principal, formes alignees comprises (N = MHGP10_FAULT_OFF_MAIN) ;
//  - la N-ieme allocation alignee, tous fils confondus (N = MHGP10_FAULT_ALIGNED ; verificateur du tour 1 : le
//    std::vector<Local> alignas(64) de build_catalogue n'etait injecte par aucune porte).
// Variable absente : aucune injection. Dans les executables de la v10, seuls les ouvriers du pool allouent hors du fil
// principal, dans des tranches de parallel_for, toutes a l'interieur d'un point d'entree (build_catalogue,
// build_tower, mreach_dendrogram) ; l'allocation alignee de build_catalogue est faite par l'appelant. L'injection est
// signalee sur stderr (« mhgp10_fault_injected », « mhgp10_fault_injected_aligned » pour une forme alignee) ; avec
// MHGP10_FAULT_ALIGNED, le nombre d'allocations alignees vues est ecrit a la sortie (« mhgp10_fault_aligned_seen N »).
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>

namespace {

// initialisation constante : desarme tant que l'environnement n'est pas lu
std::atomic<long long> g_countdown{-1};          // hors du fil principal, toutes formes
std::atomic<long long> g_countdown_aligned{-1};  // formes alignees, tous fils
std::atomic<unsigned long long> g_aligned_seen{0};
bool g_report = false;

struct ReadEnvironment {
  ReadEnvironment() {
    if (const char* s = std::getenv("MHGP10_FAULT_OFF_MAIN")) g_countdown.store(std::atoll(s));
    if (const char* s = std::getenv("MHGP10_FAULT_ALIGNED")) {
      g_countdown_aligned.store(std::atoll(s));
      g_report = true;
    }
  }
  ~ReadEnvironment() {  // a la sortie normale du processus, apres main (les fils du pool sont joints)
    if (!g_report) return;
    char msg[64];
    const int len = std::snprintf(msg, sizeof msg, "mhgp10_fault_aligned_seen %llu\n", g_aligned_seen.load());
    if (len > 0 && write(2, msg, static_cast<size_t>(len)) < 0) {
    }
  }
} g_read_environment;  // au chargement, dans le fil principal, avant main et donc avant tout ouvrier

thread_local int t_off_main = -1;  // -1 : inconnu ; le fil principal a tid == pid

bool off_main() {
  if (t_off_main < 0) t_off_main = static_cast<pid_t>(syscall(SYS_gettid)) != getpid() ? 1 : 0;
  return t_off_main == 1;
}

bool fire(std::atomic<long long>& countdown) {
  return countdown.load(std::memory_order_relaxed) >= 0 && countdown.fetch_sub(1, std::memory_order_relaxed) == 0;
}

// Decision commune a toutes les formes : vrai si cette allocation doit echouer (et le signale).
bool must_fail(bool aligned) {
  if (aligned) g_aligned_seen.fetch_add(1, std::memory_order_relaxed);
  if ((off_main() && fire(g_countdown)) || (aligned && fire(g_countdown_aligned))) {
    static const char msg[] = "mhgp10_fault_injected\n";
    static const char msg_aligned[] = "mhgp10_fault_injected_aligned\n";
    if ((aligned ? write(2, msg_aligned, sizeof msg_aligned - 1) : write(2, msg, sizeof msg - 1)) < 0) {
    }
    return true;
  }
  return false;
}

void* aligned_block(std::size_t n, std::align_val_t al) noexcept {  // taille multiple de l'alignement
  const std::size_t a = static_cast<std::size_t>(al);
  if (n > std::numeric_limits<std::size_t>::max() - a) return nullptr;
  return std::aligned_alloc(a, n == 0 ? a : (n + a - 1) / a * a);
}

}  // namespace

void* operator new(std::size_t n) {
  if (must_fail(false)) throw std::bad_alloc();
  if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept { return ::operator new(n, tag); }
void* operator new(std::size_t n, std::align_val_t al) {
  if (must_fail(true)) throw std::bad_alloc();
  if (void* p = aligned_block(n, al)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t al) { return ::operator new(n, al); }
void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n, al);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t& tag) noexcept {
  return ::operator new(n, al, tag);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
