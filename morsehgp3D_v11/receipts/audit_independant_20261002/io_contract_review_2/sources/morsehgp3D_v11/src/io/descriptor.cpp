// Fermeture controlee et ecriture complete (descriptor.hpp).
#include "io/descriptor.hpp"

#include <unistd.h>

#include <cerrno>

namespace mhgp11::io {

bool Descriptor::close() noexcept {
  if (fd_ < 0) return true;
  const int fd = std::exchange(fd_, -1);
  // Un close interrompu (EINTR) a quand meme rendu le descripteur sous Linux : il ne se rejoue pas.
  return ::close(fd) == 0;
}

bool write_all(int fd, std::string_view bytes) noexcept {
  while (!bytes.empty()) {
    const ssize_t written = ::write(fd, bytes.data(), bytes.size());
    if (written < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    // Une ecriture de 0 octet pour une demande non vide ne progresserait jamais : c'est un echec.
    if (written == 0) return false;
    // 0 < written <= bytes.size() : le prefixe ecrit est retire.
    bytes.remove_prefix(static_cast<std::size_t>(written));
  }
  return true;
}

}  // namespace mhgp11::io
