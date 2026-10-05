// Transaction de dossier (io.hpp, OutputDirectory). Port des etapes 1, 4 et 6 de OutputSet (src/core/cli_output.hpp
// du raccord R2 de la v10, commit 865f5e6 ; docs/PROVENANCE.md, section io), adaptees a un dossier entier.
//
// Repris de la source : declaration prealable, AVANT le calcul, sans rien creer ; une sortie ne designe jamais une
// entree, comparee par chemin resolu (liens symboliques suivis, . et .. resolus) ; dossier absent ou non
// inscriptible : output_unwritable ; tout ou rien a la publication ; sans publication, le destructeur retire ce que
// l'appel a cree et ne touche jamais une destination ; aucune allocation par operator new (chemins dans des tableaux
// fixes, realpath vers un tampon de l'appelant).
// Ce qui change (paragraphe 6.7 de la specification de la sortie parametree) :
//   - la cible est un dossier D qui ne doit pas exister : D, ou D.pending, deja present (lien symbolique pendant
//     compris) rend output_conflict. Un D.pending orphelin, laisse par un arret brutal, n'est jamais retire ;
//   - un seul renommage, D.pending -> D, par renameat2 avec RENAME_NOREPLACE : un lecteur voit D entier ou ne le voit
//     pas. Sans RENAME_NOREPLACE (noyau ou systeme de fichiers qui ne l'offre pas) : refus output_unwritable, jamais
//     un rename POSIX, qui remplacerait un dossier vide apparu entre-temps ;
//   - plus de sauvegardes .bak ni de restauration (rien n'est jamais remplace), plus de sortie speciale (/dev/full
//     ne peut pas etre un fichier de D) ;
//   - les fichiers, le dossier D.pending et le parent sont synchronises (fsync) ; le manifeste est ecrit en dernier,
//     seulement apres la fermeture controlee de tous les fichiers de donnees.
// Une entree resolue sous D ou D.pending implique que D ou D.pending existe : le controle des entrees et celui de
// l'existence se recouvrent dans une transaction de dossier ; le premier est garde parce qu'il est la regle ecrite
// du CLI (paragraphe 5, etape 2).
// Portes : mhgp11_io_transaction_* ; mutants conflit_ignore, orphelin_ignore, renommage_ecrasant,
// pending_non_retire, manifeste_avant_donnees, nom_reserve_admis, nom_double_admis,
// empreinte_manifeste_apres_publication.
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "io/io.hpp"

namespace mhgp11::io {

namespace {

inline constexpr std::string_view kPendingSuffix = ".pending";
// RENAME_NOREPLACE, valeur fixee par l'ABI du noyau (include/uapi/linux/fs.h) ; pas de macro de la libc.
inline constexpr unsigned kRenameNoReplace = 1u;
static_assert(NAME_MAX <= 255 && PATH_MAX >= 256, "io : bornes de noms du systeme");

// Chemin du dossier decoupe en parent, nom final et nom de D.pending.
struct Split {
  std::array<char, PATH_MAX> parent{};
  std::array<char, 256> base{};
  std::array<char, 256> pending{};
};

Outcome split_directory(const char* directory, Split& out) noexcept {
  if (directory == nullptr) return fail(Reason::parameter_out_of_range);
  std::size_t length = ::strnlen(directory, PATH_MAX);
  if (length == 0 || length >= PATH_MAX) return fail(Reason::parameter_out_of_range);
  while (length > 1 && directory[length - 1] == '/') --length;  // barres finales : "D/" designe D
  const std::string_view path(directory, length);
  const std::size_t slash = path.rfind('/');
  const std::string_view base = slash == std::string_view::npos ? path : path.substr(slash + 1);
  if (base.empty() || base == "." || base == ".." || base.size() + kPendingSuffix.size() > NAME_MAX)
    return fail(Reason::parameter_out_of_range);
  const std::string_view parent = slash == std::string_view::npos ? std::string_view(".")
                                  : slash == 0                     ? std::string_view("/")
                                                                   : path.substr(0, slash);
  std::memcpy(out.parent.data(), parent.data(), parent.size());
  std::memcpy(out.base.data(), base.data(), base.size());
  std::memcpy(out.pending.data(), base.data(), base.size());
  std::memcpy(out.pending.data() + base.size(), kPendingSuffix.data(), kPendingSuffix.size());
  return {};
}

// Vrai si le chemin resolu `real` est parent/name ou se trouve dessous.
bool under(std::string_view real, std::string_view parent, std::string_view name) noexcept {
  const std::size_t sep = parent == "/" ? 0 : 1;
  const std::size_t length = parent.size() + sep + name.size();
  if (real.size() < length || real.substr(0, parent.size()) != parent) return false;
  if (sep == 1 && real[parent.size()] != '/') return false;
  if (real.substr(parent.size() + sep, name.size()) != name) return false;
  return real.size() == length || real[length] == '/';
}

// Aucune entree resolue n'est D, D.pending, ni sous l'un d'eux ; une entree sans chemin resolu n'est pas comparee.
Outcome inputs_outside(const char* parent_real, const Split& split, std::span<const char* const> inputs) noexcept {
  std::array<char, PATH_MAX> real{};
  for (const char* input : inputs) {
    if (input == nullptr || ::realpath(input, real.data()) == nullptr) continue;
    if (under(real.data(), parent_real, split.base.data()) || under(real.data(), parent_real, split.pending.data()))
      return fail(Reason::output_conflict);
  }
  return {};
}

// Le nom n'existe pas dans le dossier (lien symbolique pendant compris) : sinon output_conflict.
Outcome absent(int directory_fd, const char* name) noexcept {
  struct stat st {};
  if (::fstatat(directory_fd, name, &st, AT_SYMLINK_NOFOLLOW) == 0) return fail(Reason::output_conflict);
  return errno == ENOENT ? Outcome{} : fail(Reason::output_unwritable);
}

Outcome valid_name(std::string_view name) noexcept {
  if (name.empty() || name.size() > kMaxFileName || name.front() == '.' || name == kManifestName)
    return fail(Reason::parameter_out_of_range);
  for (const char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.'))
      return fail(Reason::parameter_out_of_range);
  return {};
}

// Renommage sans remplacement, dans un meme dossier. D apparu : output_conflict ; indisponible : output_unwritable.
Outcome rename_noreplace(int directory_fd, const char* from, const char* to) noexcept {
#ifdef SYS_renameat2
  if (::syscall(SYS_renameat2, directory_fd, from, directory_fd, to, kRenameNoReplace) == 0) return {};
  return fail(errno == EEXIST || errno == ENOTEMPTY ? Reason::output_conflict : Reason::output_unwritable);
#else
  (void)directory_fd;
  (void)from;
  (void)to;
  return fail(Reason::output_unwritable);
#endif
}

}  // namespace

Result<OutputDirectory> OutputDirectory::plan(const char* directory, std::span<const char* const> inputs) noexcept {
  Split split;
  MHGP11_TRY(split_directory(directory, split));
  OutputDirectory out;
  out.parent_fd_ = ::open(split.parent.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (out.parent_fd_ < 0) return fail(Reason::output_unwritable);
  std::array<char, PATH_MAX> parent_real{};
  if (::realpath(split.parent.data(), parent_real.data()) == nullptr) return fail(Reason::output_unwritable);
  MHGP11_TRY(inputs_outside(parent_real.data(), split, inputs));
  MHGP11_TRY(absent(out.parent_fd_, split.base.data()));
  MHGP11_TRY(absent(out.parent_fd_, split.pending.data()));
  if (::faccessat(out.parent_fd_, ".", W_OK | X_OK, 0) != 0) return fail(Reason::output_unwritable);
  out.base_ = split.base;
  out.pending_ = split.pending;
  return out;
}

OutputDirectory::OutputDirectory(OutputDirectory&& other) noexcept
    : parent_fd_(std::exchange(other.parent_fd_, -1)),
      pending_fd_(std::exchange(other.pending_fd_, -1)),
      base_(other.base_),
      pending_(other.pending_),
      count_(std::exchange(other.count_, 0)),
      pending_created_(std::exchange(other.pending_created_, false)),
      committed_(other.committed_),
      failed_(other.failed_),
      manifest_sha256_(other.manifest_sha256_) {
  for (std::size_t i = 0; i < writers_.size(); ++i) writers_[i].take(other.writers_[i]);
}

OutputDirectory::~OutputDirectory() { discard(); }

// Sans commit reussi : fichiers crees par cet objet retires, puis D.pending s'il l'a cree. D n'est jamais touche.
void OutputDirectory::discard() noexcept {
  for (FileWriter& w : writers_)
    if (w.file_ != nullptr) std::fclose(std::exchange(w.file_, nullptr));
  if (!committed_ && pending_created_) {
    if (pending_fd_ >= 0)
      for (const FileWriter& w : writers_)
        if (w.created_) ::unlinkat(pending_fd_, w.name_.data(), 0);
    ::unlinkat(parent_fd_, pending_.data(), AT_REMOVEDIR);
  }
  if (pending_fd_ >= 0) ::close(std::exchange(pending_fd_, -1));
  if (parent_fd_ >= 0) ::close(std::exchange(parent_fd_, -1));
}

Outcome OutputDirectory::open_pending() noexcept {
  if (pending_fd_ >= 0) return {};
  if (::mkdirat(parent_fd_, pending_.data(), 0777) != 0)
    return fail(errno == EEXIST ? Reason::output_conflict : Reason::output_unwritable);
  pending_created_ = true;
  pending_fd_ = ::openat(parent_fd_, pending_.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  return pending_fd_ >= 0 ? Outcome{} : fail(Reason::output_unwritable);
}

Result<FileWriter*> OutputDirectory::create(std::string_view name) noexcept {
  if (committed_ || failed_ || parent_fd_ < 0) return fail(Reason::output_unwritable);
  MHGP11_TRY(valid_name(name));
  for (u32 i = 0; i < count_; ++i)
    if (writers_[i].name() == name) return fail(Reason::output_conflict);
  if (count_ == kMaxOutputFiles) return fail(Reason::parameter_out_of_range);
  Outcome opened = open_pending();
  FileWriter& writer = writers_[count_];
  if (opened.ok()) {
    ++count_;
    opened = writer.open(pending_fd_, name);
  }
  if (!opened.ok()) {
    failed_ = true;  // une erreur d'entree-sortie est definitive : le dossier ne sera pas publie
    return opened;
  }
  return &writer;
}

// Fichiers de donnees vides, controles, synchronises et fermes, tous avant le manifeste.
Outcome OutputDirectory::close_data() noexcept {
  for (u32 i = 0; i < count_; ++i) MHGP11_TRY(writers_[i].close_synced());
  return {};
}

Outcome OutputDirectory::write_manifest(std::string_view manifest_json) noexcept {
  FileWriter& manifest = writers_[kMaxOutputFiles];
  MHGP11_TRY(manifest.open(pending_fd_, kManifestName));
  MHGP11_TRY(manifest.bytes(std::span<const u8>(reinterpret_cast<const u8*>(manifest_json.data()),
                                                 manifest_json.size())));
  return manifest.close_synced();
}

// D.pending synchronise, renomme en D sans remplacement, puis le parent synchronise. Si cette derniere
// synchronisation echoue, la publication est defaite (D renomme en D.pending, que le destructeur retire) et le refus
// est output_unwritable ; si le retour echoue aussi, D reste publie et complet, et le refus est rendu quand meme.
Outcome OutputDirectory::publish() noexcept {
  if (::fsync(pending_fd_) != 0) return fail(Reason::output_unwritable);
  MHGP11_TRY(rename_noreplace(parent_fd_, pending_.data(), base_.data()));
  committed_ = true;
  if (::fsync(parent_fd_) == 0) return {};
  if (rename_noreplace(parent_fd_, base_.data(), pending_.data()).ok()) committed_ = false;
  return fail(Reason::output_unwritable);
}

// L'empreinte du manifeste est gardee des sa fermeture, avant le renommage : si la publication echoue ensuite et que
// son retour echoue aussi (double echec), D reste publie et son manifeste en fait foi (docs/SORTIES.md, paragraphe 9,
// etape 4 ; reponse D.3 de l'auditeur).
Outcome OutputDirectory::commit_steps(std::string_view manifest_json) noexcept {
  MHGP11_TRY(open_pending());
  MHGP11_TRY(close_data());
  MHGP11_TRY(write_manifest(manifest_json));
  manifest_sha256_ = writers_[kMaxOutputFiles].digest();
  return publish();
}

Outcome OutputDirectory::retract() noexcept {
  if (!committed_ || parent_fd_ < 0) return fail(Reason::output_unwritable);
  MHGP11_TRY(rename_noreplace(parent_fd_, base_.data(), pending_.data()));
  committed_ = false;
  failed_ = true;  // definitif : aucun create ni commit apres un retrait
  return {};
}

Outcome OutputDirectory::commit(std::string_view manifest_json) noexcept {
  if (committed_ || failed_ || parent_fd_ < 0) return fail(Reason::output_unwritable);
  const Outcome done = commit_steps(manifest_json);
  if (!done.ok()) failed_ = true;
  return done;
}

}  // namespace mhgp11::io
