// Ecriture controlee d'un fichier de sortie (io.hpp, FileWriter). Port de l'etape 3 de OutputSet
// (src/core/cli_output.hpp du raccord R2 de la v10, commit 865f5e6 ; docs/PROVENANCE.md, section io).
//
// Repris de la source : le FILE* d'ecriture a un proprietaire des son ouverture ; fflush, ferror et fclose sont
// controles, un echec rend output_unwritable et rien n'est publie ; le fichier est cree en exclusif (O_EXCL,
// O_NOFOLLOW) dans le dossier de la destination.
// Ce qui change :
//   - le fichier est cree directement sous son nom final dans D.pending/ (la transaction porte sur le dossier) : ni
//     temporaire par fichier, ni sauvegarde, ni copie des droits d'une destination remplacee (il n'y en a jamais) ;
//   - fsync du fichier avant fclose (paragraphe 6.7 : chaque fichier est synchronise avant la publication) ;
//   - les octets sont ecrits par des appels successifs (bytes, u32s, u64s, pad8) au lieu d'un rappel unique, et
//     hashes au fil de l'ecriture ; une erreur est definitive (failed_) ;
//   - tampon d'ecriture fixe de kBufferBytes (setvbuf), donc une frontiere de vidage independante du systeme de
//     fichiers ; aucun operator new.
// Portes : mhgp11_io_transaction_commit, _write_failure ; mutant ecriture_non_controlee.
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <utility>

#include "io/io.hpp"

namespace mhgp11::io {

namespace {

// Tampon de stdio de chaque fichier de sortie, en octets.
inline constexpr std::size_t kBufferBytes = std::size_t{1} << 16;
// Mots convertis en petit-boutiste par bloc, sur la pile.
inline constexpr std::size_t kChunkBytes = 4096;

}  // namespace

FileWriter::~FileWriter() {
  if (file_ != nullptr) std::fclose(file_);
}

void FileWriter::take(FileWriter& other) noexcept {
  file_ = std::exchange(other.file_, nullptr);
  sha_ = other.sha_;
  size_ = other.size_;
  failed_ = other.failed_;
  created_ = std::exchange(other.created_, false);
  name_ = other.name_;
  name_length_ = other.name_length_;
}

Outcome FileWriter::open(int directory_fd, std::string_view name) noexcept {
  if (name.empty() || name.size() > kMaxFileName) return fail(Reason::parameter_out_of_range);
  std::memcpy(name_.data(), name.data(), name.size());
  name_[name.size()] = '\0';
  name_length_ = name.size();
  // EEXIST n'arrive que si un autre que cet objet ecrit dans D.pending : jamais un double de create (refuse avant).
  const int fd = ::openat(directory_fd, name_.data(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0666);
  if (fd < 0) {
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  created_ = true;
  file_ = ::fdopen(fd, "wb");
  if (file_ == nullptr) {
    ::close(fd);
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  if (std::setvbuf(file_, nullptr, _IOFBF, kBufferBytes) != 0) {
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  return {};
}

Outcome FileWriter::write_raw(const u8* data, u64 n) noexcept {
  if (file_ == nullptr || failed_) return fail(Reason::output_unwritable);
  if (n > kMaxMessageBytes - size_) {
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  if (n != 0 && std::fwrite(data, 1, static_cast<std::size_t>(n), file_) != n) {
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  sha_.update(std::span<const u8>(data, static_cast<std::size_t>(n)));
  size_ += n;
  return {};
}

Outcome FileWriter::bytes(std::span<const u8> data) noexcept { return write_raw(data.data(), data.size()); }

Outcome FileWriter::u32s(std::span<const u32> words) noexcept {
  std::array<u8, kChunkBytes> chunk{};
  constexpr std::size_t kPer = kChunkBytes / 4;
  for (std::size_t begin = 0; begin < words.size(); begin += kPer) {
    const std::size_t m = std::min(kPer, words.size() - begin);
    for (std::size_t j = 0; j < m; ++j)
      for (std::size_t b = 0; b < 4; ++b) chunk[4 * j + b] = static_cast<u8>(words[begin + j] >> (8 * b));
    MHGP11_TRY(write_raw(chunk.data(), 4 * m));
  }
  return file_ == nullptr || failed_ ? fail(Reason::output_unwritable) : Outcome{};
}

Outcome FileWriter::u64s(std::span<const u64> words) noexcept {
  std::array<u8, kChunkBytes> chunk{};
  constexpr std::size_t kPer = kChunkBytes / 8;
  for (std::size_t begin = 0; begin < words.size(); begin += kPer) {
    const std::size_t m = std::min(kPer, words.size() - begin);
    for (std::size_t j = 0; j < m; ++j)
      for (std::size_t b = 0; b < 8; ++b) chunk[8 * j + b] = static_cast<u8>(words[begin + j] >> (8 * b));
    MHGP11_TRY(write_raw(chunk.data(), 8 * m));
  }
  return file_ == nullptr || failed_ ? fail(Reason::output_unwritable) : Outcome{};
}

Outcome FileWriter::pad8() noexcept {
  constexpr std::array<u8, 8> kZeros{};
  return write_raw(kZeros.data(), (8 - size_ % 8) % 8);
}

// Vide, controle et synchronise le fichier, puis le ferme. Deja ferme sans erreur : rien a refaire.
Outcome FileWriter::close_synced() noexcept {
  if (failed_) {
    if (file_ != nullptr) std::fclose(std::exchange(file_, nullptr));
    return fail(Reason::output_unwritable);
  }
  if (file_ == nullptr) return created_ ? Outcome{} : fail(Reason::output_unwritable);
  bool bad = std::fflush(file_) != 0 || std::ferror(file_) != 0;
  bad = bad || ::fsync(::fileno(file_)) != 0;
  const bool closed = std::fclose(std::exchange(file_, nullptr)) == 0;
  if (bad || !closed) {
    failed_ = true;
    return fail(Reason::output_unwritable);
  }
  return {};
}

}  // namespace mhgp11::io
