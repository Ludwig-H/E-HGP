// Sorties des sondes (dumps, etiquettes, arbres exportes) : publiees en entier ou pas du tout, jamais une ligne de
// statut ok (ni un code 0) avant qu'elles soient sures, jamais un fichier preexistant tronque ni retire.
//
// Frontiere (audits du 29 septembre 2026) : un dump (catalogue, tour) ou des etiquettes (cluster) ecrits vers /dev/full
// rendaient le code 0 sans aucune sortie (ni fflush, ni ferror, ni fclose controles) ; --dump vers un dossier absent
// rendait le code 2 APRES une ligne status=ok.
// Raccord R2 (prealable P2 ; rapport de raccord, R2-04 et R2-05 ; verification adverse, § 3.4 ; auditeur continu,
// CONTRE_AUDIT_R2 et outputset_exception_20260930) : la version du second tour ouvrait chaque sortie en "wb" a la
// reservation, donc un refus apres la reservation retirait un fichier preexistant, et l'entree elle-meme si --dump la
// designait ; deux sorties sur le meme fichier (etiquettes et --tree) rendaient le code 0, l'arbre ecrasant les
// etiquettes ; une allocation qui levait apres fopen laissait fuir le descripteur et la sentinelle tronquee. Ici :
//   1. add_input(chemin) enregistre chaque entree de l'appel (nuage, --configs) ; declare(chemin[, optionnelle])
//      enregistre chaque sortie, AVANT toute reservation. Rien n'est cree ni ouvert en ecriture a cette etape.
//      Refus output_conflict (invalid_input) : une sortie designe le meme fichier qu'une entree ou qu'une autre sortie,
//      meme fichier resolu (liens symboliques suivis, ./ et .. , dossiers resolus) ou meme inode (st_dev, st_ino :
//      liens physiques) ; en succes comme en refus, jamais une sortie sur une entree. Refus output_unwritable : chemin
//      vide, dossier absent ou non inscriptible, destination qui est un dossier, fichier existant sans droit
//      d'ecriture (il n'est jamais remplace).
//   2. reserve(), AVANT le calcul : un fichier temporaire par sortie reguliere, dans le dossier de la destination
//      resolue (« .<nom>.mhgp10-<pid>-<n>.tmp », cree en O_EXCL, droits de la destination si elle existe). La
//      destination n'est ni ouverte ni tronquee.
//   3. write(chemin, writer) : writer(FILE*) ecrit le temporaire, puis fflush, ferror et fclose sont controles. Une
//      sortie speciale (peripherique, tube : /dev/full, /dev/null) n'a pas de temporaire : elle est ouverte et ecrite
//      directement a cette etape, chaque erreur controlee. Une sortie optionnelle (etiquettes .vote, qui dependent de
//      la tour) n'a son temporaire qu'a sa premiere ecriture.
//   4. commit() : tout ou rien. Chaque sortie non optionnelle doit avoir ete ecrite en entier (sinon
//      output_unwritable et rien n'est publie) ; ensuite seulement, chaque temporaire est renomme sur sa destination
//      (rename dans le meme dossier : atomique par fichier).
//   5. Sans commit (refus, exception), le destructeur retire les temporaires ; les destinations restent intactes.
// Choix : un lien symbolique est SUIVI jusqu'a sa cible, meme pendante : la cible recoit la sortie et le lien reste un
// lien (comme l'ecriture par fopen d'avant R2). Un fichier a plusieurs liens physiques est remplace sous le nom resolu
// seulement : ses autres noms gardent l'ancien contenu (un inode partage n'est jamais tronque). Les droits d'une
// destination remplacee sont repris ; le proprietaire devient l'utilisateur de l'appel.
// Limites : pas de fsync (garantie contre les refus et les erreurs d'ecriture, pas contre une panne du systeme) ; un
// echec de rename apres un premier renommage laisse publiees les sorties deja renommees (meme dossier, meme systeme de
// fichiers : cas extreme, refus output_unwritable) ; une sortie speciale ecrite avant un refus garde ce qui y a ete
// ecrit ; un arret brutal (SIGKILL) laisse le temporaire, la destination intacte ; une destination reguliere atteinte
// par un lien que la resolution ne retrouve pas (liens magiques de /proc) est refusee output_unwritable.
// Garantie d'exception : les allocations d'une etape precedent toute ressource qu'elle acquiert (temporaire enregistre
// par un deplacement noexcept des sa creation) ; le FILE* d'ecriture a un proprietaire RAII des son ouverture ; un
// writer qui leve std::bad_alloc rend memory_budget, toute autre exception sort apres la fermeture du fichier ; le
// destructeur n'alloue pas.
// La ligne de statut s'ecrit apres commit() ; finish_stdout() vide et controle la sortie standard elle-meme.
#pragma once

#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "core/status.hpp"

namespace mhgp10::cli {

// Proprietaire d'un FILE* ouvert en ecriture : ferme sur tout chemin de sortie, exception comprise.
struct FileCloser {
  void operator()(std::FILE* f) const noexcept { std::fclose(f); }
};
using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

class OutputSet {
 public:
  OutputSet() = default;
  OutputSet(const OutputSet&) = delete;
  OutputSet& operator=(const OutputSet&) = delete;
  // Sans commit : les temporaires sont retires, les destinations jamais touchees. N'alloue pas.
  ~OutputSet() {
    for (const Entry& e : entries_)
      if (e.temp_live) ::unlink(e.temp.c_str());
  }

  // Entree lue par l'appel : aucune sortie ne peut designer ce fichier.
  Outcome add_input(const std::string& path) {
    try {
      struct stat st {};
      if (::stat(path.c_str(), &st) != 0) return {};  // entree deja lue : sans identite lisible, rien a comparer
      for (const Entry& e : entries_)
        if (e.exists && e.dev == st.st_dev && e.ino == st.st_ino) return fail(Reason::output_conflict);
      inputs_.push_back(Key{st.st_dev, st.st_ino});
      return {};
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);
    }
  }

  // Sortie de l'appel (etape 1 de l'en-tete) : resolue, jugee, rien n'est cree.
  Outcome declare(const std::string& path, bool optional = false) {
    try {
      Entry e;
      e.path = path;
      e.optional = optional;
      if (const Outcome o = resolve(path, e); !o.ok()) return o;
      for (const Key& k : inputs_)
        if (e.exists && e.dev == k.dev && e.ino == k.ino) return fail(Reason::output_conflict);
      for (const Entry& other : entries_)
        if (same_file(e, other)) return fail(Reason::output_conflict);
      entries_.push_back(std::move(e));
      return {};
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);
    }
  }

  // Etape 2 : un temporaire par sortie reguliere non optionnelle, avant le calcul.
  Outcome reserve() {
    try {
      for (Entry& e : entries_)
        if (e.kind == Kind::regular && !e.optional && !e.temp_live)
          if (const Outcome o = make_temp(e); !o.ok()) return o;
      return {};
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);
    }
  }

  // Etape 3 : ecriture complete du temporaire (ou de la sortie speciale), fflush, ferror et fclose controles.
  template <class Writer>
  Outcome write(const std::string& path, Writer&& writer) {
    Entry* e = find(path);
    if (e == nullptr) return fail(Reason::output_unwritable);  // sortie non declaree : jamais ecrite
    if (e->kind == Kind::regular && !e->temp_live) {             // sortie optionnelle : temporaire a l'ecriture
      Outcome made;
      try {
        made = make_temp(*e);
      } catch (const std::bad_alloc&) {
        made = fail(Reason::memory_budget);
      }
      if (!made.ok()) return made;
    }
    e->written = false;
    const int fd = e->kind == Kind::regular ? ::open(e->temp.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC | O_NOFOLLOW)
                                            : ::open(e->dest.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC);
    if (fd < 0) return fail(Reason::output_unwritable);
    FilePtr f(::fdopen(fd, "wb"));
    if (!f) {
      ::close(fd);
      return fail(Reason::output_unwritable);
    }
    try {
      writer(f.get());
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);  // fichier ferme par son proprietaire ; temporaire retire sans commit
    }
    const bool bad = std::fflush(f.get()) != 0 || std::ferror(f.get()) != 0;
    const bool closed = std::fclose(f.release()) == 0;
    if (bad || !closed) return fail(Reason::output_unwritable);
    e->written = true;
    return {};
  }

  // Etape 4 : tout ou rien ; rien n'est renomme tant qu'une sortie non optionnelle n'est pas ecrite.
  Outcome commit() {
    for (const Entry& e : entries_)
      if (!e.optional && !e.written) return fail(Reason::output_unwritable);
    for (Entry& e : entries_) {
      if (e.kind != Kind::regular || !e.written || !e.temp_live) continue;  // deja renommee : rien a refaire
      if (::rename(e.temp.c_str(), e.dest.c_str()) != 0) return fail(Reason::output_unwritable);
      e.temp_live = false;
    }
    return {};
  }

 private:
  enum class Kind : u8 { regular, special };
  struct Entry {
    std::string path;          // chemin tel que donne (cle de write)
    std::string dest;          // reguliere : dossier canonique / nom ; speciale : chemin donne
    std::string dir, name;     // reguliere : dossier canonique et nom de la destination
    std::string temp;          // temporaire (vide avant sa creation)
    Kind kind = Kind::regular;
    bool optional = false;
    bool exists = false;       // destination existante : identite (dev, ino) et droits relus
    dev_t dev = 0;
    ino_t ino = 0;
    mode_t mode = 0;
    bool temp_live = false;    // temporaire cree, pas encore renomme : retire par le destructeur
    bool written = false;      // ecrite en entier
  };
  struct Key {
    dev_t dev;
    ino_t ino;
  };
  struct Freer {
    void operator()(char* p) const noexcept { std::free(p); }
  };
  static constexpr int kMaxLinks = 40;         // chaine de liens symboliques suivie au plus (ELOOP du noyau)
  static constexpr int kTempAttempts = 64;     // noms de temporaire essayes (O_EXCL)
  static constexpr size_t kTempNameKeep = 200; // prefixe du nom garde dans le temporaire (NAME_MAX = 255)

  static std::string dir_of(const std::string& p) {
    const size_t s = p.find_last_of('/');
    if (s == std::string::npos) return ".";
    return s == 0 ? std::string("/") : p.substr(0, s);
  }
  static std::string base_of(const std::string& p) {
    const size_t s = p.find_last_of('/');
    return s == std::string::npos ? p : p.substr(s + 1);
  }
  static bool same_file(const Entry& a, const Entry& b) {
    return (a.exists && b.exists && a.dev == b.dev && a.ino == b.ino) || (!a.dest.empty() && a.dest == b.dest);
  }

  // Nature, destination et identite d'une sortie ; aucun fichier cree.
  static Outcome resolve(const std::string& path, Entry& e) {
    if (path.empty()) return fail(Reason::output_unwritable);
    struct stat st {};
    if (::stat(path.c_str(), &st) == 0) {
      if (S_ISDIR(st.st_mode)) return fail(Reason::output_unwritable);
      e.exists = true;
      e.dev = st.st_dev;
      e.ino = st.st_ino;
      e.mode = st.st_mode & 07777;
      if (::access(path.c_str(), W_OK) != 0) return fail(Reason::output_unwritable);
      if (!S_ISREG(st.st_mode)) {  // peripherique, tube, socket : ecrit directement
        e.kind = Kind::special;
        e.dest = path;
        return {};
      }
    } else if (errno != ENOENT) {
      return fail(Reason::output_unwritable);
    }
    // sortie reguliere : liens symboliques suivis jusqu'a la cible, meme pendante
    std::string p = path;
    for (int hop = 0;; ++hop) {
      struct stat ls {};
      if (::lstat(p.c_str(), &ls) != 0) {
        if (errno != ENOENT) return fail(Reason::output_unwritable);
        break;
      }
      if (!S_ISLNK(ls.st_mode)) break;
      if (hop == kMaxLinks) return fail(Reason::output_unwritable);
      std::string target(PATH_MAX, '\0');
      const ssize_t n = ::readlink(p.c_str(), target.data(), target.size());
      if (n <= 0 || static_cast<size_t>(n) >= target.size()) return fail(Reason::output_unwritable);
      target.resize(static_cast<size_t>(n));
      p = target[0] == '/' ? target : dir_of(p) + "/" + target;
    }
    const std::string name = base_of(p);
    if (name.empty() || name == "." || name == "..") return fail(Reason::output_unwritable);
    const std::unique_ptr<char, Freer> dir(::realpath(dir_of(p).c_str(), nullptr));
    if (!dir) return fail(Reason::output_unwritable);  // dossier absent ou inaccessible
    e.dir = dir.get();
    e.name = name;
    e.dest = e.dir + (e.dir == "/" ? "" : "/") + name;
    if (::access(e.dir.c_str(), W_OK | X_OK) != 0) return fail(Reason::output_unwritable);
    struct stat rs {};
    const bool found = ::lstat(e.dest.c_str(), &rs) == 0;
    if (found != e.exists || (found && (rs.st_dev != e.dev || rs.st_ino != e.ino)))
      return fail(Reason::output_unwritable);  // la resolution ne retrouve pas le fichier designe
    return {};
  }

  // Temporaire d'une sortie reguliere, dans le dossier de sa destination ; enregistre des sa creation.
  Outcome make_temp(Entry& e) {
    for (int attempt = 0; attempt < kTempAttempts; ++attempt) {
      std::string t = e.dir + (e.dir == "/" ? "" : "/") + "." + e.name.substr(0, kTempNameKeep) + ".mhgp10-" +
                      std::to_string(::getpid()) + "-" + std::to_string(counter_++) + ".tmp";
      const int fd = ::open(t.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0666);
      if (fd < 0) {
        if (errno == EEXIST) continue;
        return fail(Reason::output_unwritable);
      }
      e.temp = std::move(t);  // noexcept : aucune operation qui leve entre la creation et l'enregistrement
      e.temp_live = true;
      const bool mode_ok = !e.exists || ::fchmod(fd, e.mode) == 0;  // droits de la destination remplacee
      const bool closed = ::close(fd) == 0;
      return mode_ok && closed ? Outcome{} : fail(Reason::output_unwritable);
    }
    return fail(Reason::output_unwritable);
  }

  Entry* find(const std::string& path) {
    for (Entry& e : entries_)
      if (e.path == path) return &e;
    return nullptr;
  }

  std::vector<Key> inputs_;
  std::vector<Entry> entries_;
  unsigned long long counter_ = 0;
};

// Sortie standard (ligne de statut JSON, lignes du temoin mreach) videe et controlee a la fin : un code 0 devient 2 si
// elle n'a pas pu etre ecrite (par exemple vers /dev/full) ; un autre code est garde.
inline int finish_stdout(int code) {
  const bool bad = std::fflush(stdout) != 0 || std::ferror(stdout) != 0;
  if (bad && code == 0) {
    std::fprintf(stderr, "refus output_unwritable (sortie standard)\n");
    return 2;
  }
  return code;
}

}  // namespace mhgp10::cli
