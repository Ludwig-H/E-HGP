// Descripteur de fichier possede (ferme a la destruction) et ecriture complete d'une suite d'octets. Interne au
// module io, a une exception pres : write_all sert aussi a la ligne de statut du CLI.
#pragma once

#include <string_view>
#include <utility>

#include "core/core.hpp"

namespace mhgp11::io {

// Possede un descripteur ouvert, ou rien (-1). Ne se copie pas ; ferme ce qu'il possede a sa destruction, sur tout
// chemin de sortie, exception comprise.
class Descriptor {
 public:
  Descriptor() noexcept = default;
  explicit Descriptor(int fd) noexcept : fd_(fd) {}
  Descriptor(const Descriptor&) = delete;
  Descriptor& operator=(const Descriptor&) = delete;
  Descriptor(Descriptor&& o) noexcept : fd_(std::exchange(o.fd_, -1)) {}
  Descriptor& operator=(Descriptor&& o) noexcept {
    if (this != &o) {
      (void)close();
      fd_ = std::exchange(o.fd_, -1);
    }
    return *this;
  }
  ~Descriptor() { (void)close(); }

  bool valid() const noexcept { return fd_ >= 0; }
  int get() const noexcept { return fd_; }
  // Ferme le descripteur ; vrai si la fermeture reussit (ou s'il n'y avait rien a fermer). Le descripteur est rendu
  // au systeme dans tous les cas : un echec de close dit qu'une ecriture differee a echoue.
  [[nodiscard]] bool close() noexcept;

 private:
  int fd_ = -1;
};

// Ecrit tous les octets dans le descripteur (reprise apres une ecriture partielle ou interrompue). Faux a la
// premiere erreur ; ce qui a deja ete ecrit le reste.
[[nodiscard]] bool write_all(int fd, std::string_view bytes) noexcept;

}  // namespace mhgp11::io
