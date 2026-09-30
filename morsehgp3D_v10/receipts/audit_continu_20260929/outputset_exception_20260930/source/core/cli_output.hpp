// Sorties des sondes (dumps, etiquettes, arbres exportes) : ecrites en entier ou refusees, jamais une ligne de statut ok
// (ni un code 0) avant qu'elles soient sures.
//
// Frontiere (audits du 29 septembre 2026) : un dump (catalogue, tour) ou des etiquettes (cluster) ecrits vers /dev/full
// rendaient le code 0 sans aucune sortie (ni fflush, ni ferror, ni fclose controles) ; --dump vers un dossier absent
// rendait le code 2 APRES une ligne status=ok ; un echec d'ouverture rendait le code 2 sans ligne de statut. Ici :
//   - reserve() ouvre (cree ou tronque) un chemin connu AVANT le calcul, puis le referme : un chemin inaccessible
//     (dossier absent, droits) est refuse output_unwritable (resource_exhausted) sans calcul ;
//   - write() ouvre, ecrit, vide (fflush), lit l'etat du flux (ferror) et controle la fermeture (fclose) : un
//     peripherique ou un disque plein est refuse output_unwritable ;
//   - sans commit() (refus apres une reservation ou une ecriture), le destructeur retire les fichiers REGULIERS reserves
//     ou ecrits : aucune sortie partielle ou vide ne reste sous son nom ; un lien symbolique, un peripherique ou un tube
//     n'est jamais retire ;
//   - la ligne de statut s'ecrit apres commit() ; finish_stdout() vide et controle la sortie standard elle-meme.
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "core/status.hpp"

namespace mhgp10::cli {

class OutputSet {
 public:
  OutputSet() = default;
  OutputSet(const OutputSet&) = delete;
  OutputSet& operator=(const OutputSet&) = delete;
  ~OutputSet() {
    if (committed_) return;
    for (const Entry& e : entries_)
      if (e.regular) std::remove(e.path.c_str());
  }

  // Chemin ouvert en ecriture (cree ou tronque) puis referme, avant le calcul.
  Outcome reserve(const std::string& path) {
    std::FILE* f = open(path);
    if (!f) return fail(Reason::output_unwritable);
    return std::fclose(f) == 0 ? Outcome{} : fail(Reason::output_unwritable);
  }

  // Ecriture complete : writer(FILE*), puis fflush, ferror et fclose controles.
  template <class Writer>
  Outcome write(const std::string& path, Writer&& writer) {
    std::FILE* f = open(path);
    if (!f) return fail(Reason::output_unwritable);
    writer(f);
    const bool bad = std::fflush(f) != 0 || std::ferror(f) != 0;
    const bool closed = std::fclose(f) == 0;
    return bad || !closed ? fail(Reason::output_unwritable) : Outcome{};
  }

  // Toutes les sorties sont ecrites : plus rien ne sera retire.
  void commit() { committed_ = true; }

 private:
  struct Entry {
    std::string path;
    bool regular;  // fichier regulier (ni lien, ni peripherique, ni tube) : retire sans commit
  };
  std::FILE* open(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return nullptr;
    for (const Entry& e : entries_)
      if (e.path == path) return f;
    std::error_code ec;
    const bool regular = std::filesystem::symlink_status(path, ec).type() == std::filesystem::file_type::regular;
    entries_.push_back({path, !ec && regular});
    return f;
  }
  std::vector<Entry> entries_;
  bool committed_ = false;
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
