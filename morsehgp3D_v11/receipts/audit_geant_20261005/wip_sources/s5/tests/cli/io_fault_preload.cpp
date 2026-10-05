// Bibliotheque de prechargement de la porte mhgp11_cli_contract (hors produit) : double echec du commit, sans crochet
// dans le produit (docs/ARCHITECTURE.md, regle 6 ; docs/SORTIES.md, paragraphe 9). Remplace fsync et syscall du
// processus (LD_PRELOAD) :
//   - fsync du dossier MHGP11_FAULT_PARENT (le parent de D, compare par peripherique et inode) : EIO, toujours ;
//   - syscall(SYS_renameat2, ...) dont l'ancien nom est MHGP11_FAULT_NAME (le nom final de D, donc un renommage de D
//     vers D.pending) : EIO pour les MHGP11_FAULT_RENAME_BACK premiers appels, puis l'appel reel.
// La publication renomme D.pending en D (appel reel), echoue a synchroniser le parent, puis tente le retour de D vers
// D.pending (refuse) : commit refuse avec committed() vrai. Le retrait de l'api (withdraw) tente ensuite le meme
// renommage : refuse si MHGP11_FAULT_RENAME_BACK vaut au moins 2 (D reste publie et complet, published_complete),
// reussi s'il vaut 1 (rien ne reste publie). Sans MHGP11_FAULT_PARENT, la bibliotheque ne change rien. Sous ASan ou
// TSan, un prechargement precederait le runtime du sanitizer : la porte n'y joue pas ces cas.
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

namespace {

template <class Function>
Function next_symbol(const char* name) noexcept {
  void* address = ::dlsym(RTLD_NEXT, name);
  Function function = nullptr;
  static_assert(sizeof(function) == sizeof(address));
  std::memcpy(&function, &address, sizeof(function));
  return function;
}

// Renommages de D encore a refuser ; lu une fois dans l'environnement.
std::atomic<long> rename_back_left{-1};

bool refuse_rename_back() noexcept {
  long expected = -1;
  if (rename_back_left.load() == -1) {
    const char* text = std::getenv("MHGP11_FAULT_RENAME_BACK");
    rename_back_left.compare_exchange_strong(expected, text != nullptr ? std::strtol(text, nullptr, 10) : 0);
  }
  long left = rename_back_left.load();
  while (left > 0 && !rename_back_left.compare_exchange_weak(left, left - 1)) {
  }
  return left > 0;
}

bool is_fault_parent(int fd) noexcept {
  const char* parent = std::getenv("MHGP11_FAULT_PARENT");
  struct stat target {}, st {};
  return parent != nullptr && ::stat(parent, &target) == 0 && ::fstat(fd, &st) == 0 && S_ISDIR(st.st_mode) &&
         st.st_dev == target.st_dev && st.st_ino == target.st_ino;
}

}  // namespace

extern "C" int fsync(int fd) {
  using Function = int (*)(int);
  static const Function real = next_symbol<Function>("fsync");
  if (is_fault_parent(fd)) {
    errno = EIO;
    return -1;
  }
  if (real == nullptr) {
    errno = ENOSYS;
    return -1;
  }
  return real(fd);
}

extern "C" long syscall(long number, ...) noexcept {
  // Six mots d'arguments, comme l'enveloppe syscall de la glibc (convention x86-64 : registres).
  va_list list;
  va_start(list, number);
  long words[6];
  for (long& word : words) word = va_arg(list, long);
  va_end(list);
  if (number == SYS_renameat2) {
    const char* name = std::getenv("MHGP11_FAULT_NAME");
    const char* from = reinterpret_cast<const char*>(words[1]);
    if (name != nullptr && from != nullptr && std::strcmp(from, name) == 0 && refuse_rename_back()) {
      errno = EIO;
      return -1;
    }
  }
  using Function = long (*)(long, ...);
  static const Function real = next_symbol<Function>("syscall");
  if (real == nullptr) {
    errno = ENOSYS;
    return -1;
  }
  return real(number, words[0], words[1], words[2], words[3], words[4], words[5]);
}
