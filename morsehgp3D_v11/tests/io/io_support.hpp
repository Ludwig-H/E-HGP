// Aides des portes du module io (hors produit) : dossier de travail jetable, fichiers ecrits et relus, inventaire
// d'un dossier, empreinte de reference par le Sha256 du module (lui-meme juge contre FIPS 180-4 et hashlib).
#pragma once

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "io/io.hpp"

namespace mhgp11::io_test {

using io::Digest;
using io::Sha256;
using io::to_hex;

// Dossier unique sous $TMPDIR (ou /tmp), retire en entier a la destruction (droits retablis d'abord).
class Scratch {
 public:
  Scratch() {
    const char* base = std::getenv("TMPDIR");
    std::string pattern = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp11_io_XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    if (::mkdtemp(buffer.data()) != nullptr) root_ = buffer.data();
  }
  Scratch(const Scratch&) = delete;
  Scratch& operator=(const Scratch&) = delete;
  ~Scratch() {
    if (root_.empty()) return;
    std::error_code error;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_, error))
      if (entry.is_directory(error) && !entry.is_symlink(error)) ::chmod(entry.path().c_str(), 0700);
    std::filesystem::remove_all(root_, error);
  }
  bool ok() const { return !root_.empty(); }
  std::string path(const std::string& name) const { return root_ + "/" + name; }
  const std::string& root() const { return root_; }

 private:
  std::string root_;
};

inline bool write_file(const std::string& path, const std::vector<u8>& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(out);
}

inline std::vector<u8> read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<u8>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Present au sens de lstat : un lien symbolique pendant existe.
inline bool exists(const std::string& path) {
  struct stat st {};
  return ::lstat(path.c_str(), &st) == 0;
}

// Noms des entrees d'un dossier, tries ; vide si le dossier manque.
inline std::vector<std::string> entries(const std::string& dir) {
  std::vector<std::string> names;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(dir, error))
    names.push_back(entry.path().filename().string());
  std::sort(names.begin(), names.end());
  return names;
}

inline void put32(std::vector<u8>& out, u32 v) {
  for (int b = 0; b < 4; ++b) out.push_back(static_cast<u8>(v >> (8 * b)));
}

inline Digest digest_of(const std::vector<u8>& bytes) {
  Sha256 sha;
  sha.update(std::span<const u8>(bytes.data(), bytes.size()));
  return sha.finish();
}

inline Digest digest_of(std::string_view text) {
  Sha256 sha;
  sha.update(text);
  return sha.finish();
}

inline std::string hex_of(const Digest& d) {
  const auto h = to_hex(d);
  return std::string(h.data(), h.size());
}

}  // namespace mhgp11::io_test
