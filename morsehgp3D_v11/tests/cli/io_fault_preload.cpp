// Bibliotheque de prechargement de la porte mhgp11_cli_contract (hors produit) : double echec du commit, sans crochet
// dans le produit (docs/ARCHITECTURE.md, regle 6 ; docs/SORTIES.md, paragraphe 9). Remplace fsync et syscall du
// processus (LD_PRELOAD) :
//   - fsync du dossier MHGP11_FAULT_PARENT (le parent de D, compare par peripherique et inode) : EIO, toujours ;
//   - syscall(SYS_renameat2, ...) dont l'ancien nom est MHGP11_FAULT_NAME (le nom final de D, donc un renommage de D
//     vers D.pending) : EIO pour les MHGP11_FAULT_RENAME_BACK premiers appels, puis l'appel reel.
// La publication renomme D.pending en D (appel reel), echoue a synchroniser le parent, puis tente le retour de D vers
// D.pending (refuse) : commit refuse avec committed() vrai. Le retrait de l'api (withdraw) tente ensuite le meme
// renommage : refuse si MHGP11_FAULT_RENAME_BACK vaut au moins 2 (D reste publie et complet, published_complete),
// reussi s'il vaut 1 (rien ne reste publie). Sans MHGP11_FAULT_PARENT, fsync et renameat2 ne changent pas. Sous ASan
// ou TSan, un prechargement precederait le runtime du sanitizer : la porte n'y joue pas ces cas.
// Contrat des arguments variables (C11 7.16.1.1 ; C++20 17.13.1) : seul SYS_renameat2 est decode, avec exactement
// ses cinq arguments, dans les types que lui passe src/io/directory.cpp (rename_noreplace) : int, const char*, int,
// const char*, unsigned ; ils sont transmis dans ces memes types a l'appel reel. Tout autre numero de syscall rend la
// porte INVALIDE, car la bibliotheque ne sait pas en relire les arguments : elle ecrit sur la sortie d'erreur le jeton
// "mhgp11_io_fault_preload_invalide syscall=<numero>", puis arrete le processus (abort). Un arret par signal est un
// echec de la porte, et cli_contract.py nomme cette cause (correction de harnais des auditeurs, 238734f1d).
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

// Porte invalide : jeton et numero du syscall sur la sortie d'erreur (write, sans tampon ni allocation), puis abort.
[[noreturn]] void invalid_gate(long number) noexcept {
  char text[64] = "mhgp11_io_fault_preload_invalide syscall=";
  std::size_t size = std::strlen(text);
  char digits[24];
  std::size_t count = 0;
  unsigned long value = number < 0 ? 0ul - static_cast<unsigned long>(number) : static_cast<unsigned long>(number);
  do {
    digits[count++] = static_cast<char>('0' + value % 10);
    value /= 10;
  } while (value != 0 && count < sizeof digits);
  if (number < 0) text[size++] = '-';
  while (count != 0) text[size++] = digits[--count];
  text[size++] = '\n';
  const ssize_t written = ::write(STDERR_FILENO, text, size);
  static_cast<void>(written);
  std::abort();
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
  if (number != SYS_renameat2) invalid_gate(number);
  va_list list;
  va_start(list, number);
  const int from_directory = va_arg(list, int);
  const char* const from = va_arg(list, const char*);
  const int to_directory = va_arg(list, int);
  const char* const to = va_arg(list, const char*);
  const unsigned flags = va_arg(list, unsigned);
  va_end(list);
  const char* name = std::getenv("MHGP11_FAULT_NAME");
  if (name != nullptr && from != nullptr && std::strcmp(from, name) == 0 && refuse_rename_back()) {
    errno = EIO;
    return -1;
  }
  using Function = long (*)(long, ...);
  static const Function real = next_symbol<Function>("syscall");
  if (real == nullptr) {
    errno = ENOSYS;
    return -1;
  }
  return real(number, from_directory, from, to_directory, to, flags);
}
