// Formats binaires des vidages de la tour v11 pour les microbancs v12 (MES-M3, MES-M4), hors produit.
// Contrat complet : ../README.md, paragraphe « Formats ». Version 1 du 7 octobre 2026.
//
// Fichier = en-tete fixe de 64 octets, puis des sections. Section = en-tete de 24 octets (etiquette de 8 octets,
// taille d'un element, nombre d'elements), puis les octets des elements, completes par des zeros jusqu'a un
// multiple de 8. Tous les entiers sont little-endian ; un i128 est ecrit en deux mots de 64 bits (bas, haut), en
// complement a deux. Les sections sont lues par etiquette : un lecteur refuse une etiquette attendue absente, une
// taille d'element differente ou un fichier tronque, jamais en silence.
#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mhgp12::dump {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

inline constexpr char kMagic[8] = {'M', 'H', 'G', 'P', '1', '2', 'D', 'P'};
inline constexpr u32 kVersion = 1;
inline constexpr u32 kNone = 0xFFFFFFFFu;

enum Kind : u32 { kCatalogue = 1, kOrder = 2, kForest = 3 };

struct Header {
  char magic[8];
  u32 version, kind, coord_bits, kmax, order, sections;
  u64 sites;
  char frame[24];
};
static_assert(sizeof(Header) == 64, "en-tete de 64 octets");

struct SectionHeader {
  char tag[8];
  u32 elem_bytes, reserved;
  u64 count;
};
static_assert(sizeof(SectionHeader) == 24, "en-tete de section de 24 octets");

// ---- Enregistrements (tailles fixees ; aucun remplissage implicite) ----------------------------------------------
// CAT : une boule du catalogue Cat_K, dans l'ordre canonique de la v11 (niveau exact, puis S*).
struct BallRec {
  u32 rank;      // rang dense du niveau exact (>= 1 ; 0 = niveau nul des sites)
  u32 p, m, q;   // |I|, |U|, cardinal du support minimal (2..4)
  u32 sstar[4];  // S* croissant (SiteIdx), kNone au-dela de q
};
static_assert(sizeof(BallRec) == 32, "BallRec");

// ORDER : une naissance de l'ordre k (cle = BallIdx, ou SiteIdx a l'ordre 1).
struct BirthRec {
  u32 key, rank;
  u32 v11_node;  // indice du noeud de naissance dans la foret publiee par la v11 (ordre canonique)
  u32 flags;     // bit 0 : coquille etendue (m > q)
};
static_assert(sizeof(BirthRec) == 16, "BirthRec");

// ORDER : centre exact d'une naissance, (x/d, y/d, z/d), d > 0 (sites : d = 1).
struct CenterRec {
  i128 x, y, z, d;
};
static_assert(sizeof(CenterRec) == 64, "CenterRec");

// ORDER : une cellule (boule de W_k qui n'est pas une naissance), dans l'ordre des boules (rangs croissants).
struct CellRec {
  u32 ball, rank, p, m, q;
  u32 flags;  // bit 0 : coquille etendue (m > q)
};
static_assert(sizeof(CellRec) == 24, "CellRec");

// ORDER : graine d'une trace stricte, telle que la v11 la calcule (naissance terminale de la descente).
struct SeedRec {
  u32 key;    // cle de la naissance (BallIdx, SiteIdx a l'ordre 1)
  u32 node;   // noeud de naissance dans la foret v11 de l'ordre
  u8 end;     // fin de descente : 1 table de populations sur la trace, 2 table apres des pas, 3 pas terminal
  u8 steps;   // pas avec plus petite boule (parties videes pour cette trace)
  u16 pad;
};
static_assert(sizeof(SeedRec) == 12, "SeedRec");

// ORDER : une partie de descente dont la v11 calcule la plus petite boule.
enum Route : u8 { kRouteCatalogue = 1, kRouteCensusSaturated = 2, kRouteCensusComplete = 3 };
enum Action : u8 { kActionInterior = 1, kActionTrace = 2, kActionTerminal = 3 };
struct PartRec {
  u8 route;       // Route de la v11 pour ce pas
  u8 action;      // Action du pas
  u8 sstar_in_f;  // 1 si S*(B(F)) est dans F (support local canonique present dans la table S* -> boule)
  u8 pad;
  u32 ball;       // B(F) dans Cat_K (BallIdx), ou kNone si la sphere n'est pas au catalogue
};
static_assert(sizeof(PartRec) == 8, "PartRec");

// FOREST : un noeud de la foret publiee par la v11 (ordre des noeuds de la v11).
struct NodeRec {
  u32 rank, parent, birth_key, child_count;
  u64 child_begin;
};
static_assert(sizeof(NodeRec) == 24, "NodeRec");

// ---- Ecriture ------------------------------------------------------------------------------------------------------
class Writer {
 public:
  Writer(const std::string& path, Kind kind, u32 coord_bits, u32 kmax, u32 order, u64 sites,
         const std::string& frame)
      : path_(path) {
    file_ = std::fopen(path.c_str(), "wb");
    if (file_ == nullptr) throw std::runtime_error("vidage : ouverture impossible de " + path);
    std::memset(&header_, 0, sizeof(header_));
    std::memcpy(header_.magic, kMagic, 8);
    header_.version = kVersion;
    header_.kind = kind;
    header_.coord_bits = coord_bits;
    header_.kmax = kmax;
    header_.order = order;
    header_.sites = sites;
    if (frame.size() >= sizeof(header_.frame)) throw std::runtime_error("vidage : nom de trame trop long");
    std::memcpy(header_.frame, frame.data(), frame.size());
    put(&header_, sizeof(header_));
  }
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  ~Writer() {
    if (file_ != nullptr) std::fclose(file_);
  }

  template <class T>
  void section(const char* tag, const T* data, u64 count) {
    raw(tag, sizeof(T), data, count);
  }
  template <class T>
  void section(const char* tag, const std::vector<T>& values) {
    raw(tag, sizeof(T), values.data(), values.size());
  }
  void raw(const char* tag, u32 elem_bytes, const void* data, u64 count) {
    SectionHeader h{};
    const std::size_t length = std::strlen(tag);
    if (length == 0 || length > 8) throw std::runtime_error("vidage : etiquette invalide");
    std::memcpy(h.tag, tag, length);
    h.elem_bytes = elem_bytes;
    h.count = count;
    put(&h, sizeof(h));
    const u64 bytes = u64{elem_bytes} * count;
    if (bytes != 0) put(data, bytes);
    static const char zeros[8] = {};
    if (bytes % 8 != 0) put(zeros, 8 - bytes % 8);
    ++header_.sections;
  }
  void close() {
    if (std::fseek(file_, 0, SEEK_SET) != 0) throw std::runtime_error("vidage : fseek");
    put(&header_, sizeof(header_));
    if (std::fclose(file_) != 0) {
      file_ = nullptr;
      throw std::runtime_error("vidage : fermeture de " + path_);
    }
    file_ = nullptr;
  }

 private:
  void put(const void* data, u64 bytes) {
    if (std::fwrite(data, 1, bytes, file_) != bytes) throw std::runtime_error("vidage : ecriture de " + path_);
  }
  std::string path_;
  std::FILE* file_ = nullptr;
  Header header_{};
};

// ---- Lecture (projection en memoire, lecture seule) ----------------------------------------------------------------
struct SectionView {
  const unsigned char* data = nullptr;
  u32 elem_bytes = 0;
  u64 count = 0;
};

class Reader {
 public:
  explicit Reader(const std::string& path) : path_(path) {
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) throw std::runtime_error("lecture : ouverture impossible de " + path);
    struct stat st {};
    if (::fstat(fd_, &st) != 0) throw std::runtime_error("lecture : fstat " + path);
    size_ = static_cast<u64>(st.st_size);
    if (size_ < sizeof(Header)) throw std::runtime_error("lecture : fichier tronque " + path);
    void* map = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (map == MAP_FAILED) throw std::runtime_error("lecture : mmap " + path);
    base_ = static_cast<const unsigned char*>(map);
    std::memcpy(&header_, base_, sizeof(header_));
    if (std::memcmp(header_.magic, kMagic, 8) != 0) throw std::runtime_error("lecture : magie inconnue " + path);
    if (header_.version != kVersion) throw std::runtime_error("lecture : version non prise en charge " + path);
    // Invariant : at <= size_ a chaque instant ; chaque avancee (en-tete de section, donnees, remplissage) est
    // comparee a la place restante size_ - at, jamais par une addition qui pourrait deborder (CST-0225 : un fichier
    // de 88 octets annoncant 2^62 - 1 elements passait par debordement de at + bytes).
    u64 at = sizeof(Header);
    for (u32 s = 0; s < header_.sections; ++s) {
      if (size_ - at < sizeof(SectionHeader)) throw std::runtime_error("lecture : section tronquee " + path);
      SectionHeader h{};
      std::memcpy(&h, base_ + at, sizeof(h));
      at += sizeof(h);
      if (h.elem_bytes != 0 && h.count > UINT64_MAX / h.elem_bytes) throw std::runtime_error("lecture : taille " + path);
      const u64 bytes = u64{h.elem_bytes} * h.count;
      if (bytes > size_ - at) throw std::runtime_error("lecture : donnees tronquees " + path);
      const u64 pad = (8 - bytes % 8) % 8;
      if (pad > size_ - at - bytes) throw std::runtime_error("lecture : remplissage tronque " + path);
      char tag[9] = {};
      std::memcpy(tag, h.tag, 8);
      if (sections_.count(tag) != 0) throw std::runtime_error("lecture : section en double " + std::string(tag));
      sections_[tag] = SectionView{base_ + at, h.elem_bytes, h.count};
      at += bytes + pad;
    }
    if (at != size_) throw std::runtime_error("lecture : octets en trop " + path);
  }
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  ~Reader() {
    if (base_ != nullptr) ::munmap(const_cast<unsigned char*>(base_), size_);
    if (fd_ >= 0) ::close(fd_);
  }
  const Header& header() const noexcept { return header_; }
  std::string frame() const { return std::string(header_.frame, strnlen(header_.frame, sizeof(header_.frame))); }
  bool has(const std::string& tag) const { return sections_.count(tag) != 0; }

  // Section attendue, elements de taille sizeof(T) (ou elem_bytes donne pour un tableau a largeur variable).
  template <class T>
  std::pair<const T*, u64> get(const std::string& tag, u32 elem_bytes = sizeof(T)) const {
    const auto found = sections_.find(tag);
    if (found == sections_.end()) throw std::runtime_error("lecture : section absente " + tag + " dans " + path_);
    if (found->second.elem_bytes != elem_bytes)
      throw std::runtime_error("lecture : taille d'element inattendue pour " + tag + " dans " + path_);
    return {reinterpret_cast<const T*>(found->second.data), found->second.count};
  }

 private:
  std::string path_;
  int fd_ = -1;
  const unsigned char* base_ = nullptr;
  u64 size_ = 0;
  Header header_{};
  std::map<std::string, SectionView> sections_;
};

// Comparaison exacte de fractions a numerateurs i128 (|x| < 2^126) et denominateurs positifs (< 2^126) :
// signe de a/b - c/d par produits croises sur 256 bits. Sert a l'ordre canonique des centres (naissances).
namespace detail {
struct U256 {
  u64 w[4];
};
inline U256 mul_u128(u128 a, u128 b) noexcept {
  const u64 a0 = static_cast<u64>(a), a1 = static_cast<u64>(a >> 64);
  const u64 b0 = static_cast<u64>(b), b1 = static_cast<u64>(b >> 64);
  const u128 p00 = static_cast<u128>(a0) * b0;
  const u128 p01 = static_cast<u128>(a0) * b1;
  const u128 p10 = static_cast<u128>(a1) * b0;
  const u128 p11 = static_cast<u128>(a1) * b1;
  U256 r{};
  r.w[0] = static_cast<u64>(p00);
  u128 mid = (p00 >> 64) + static_cast<u64>(p01) + static_cast<u64>(p10);
  r.w[1] = static_cast<u64>(mid);
  u128 high = (mid >> 64) + (p01 >> 64) + (p10 >> 64) + static_cast<u64>(p11);
  r.w[2] = static_cast<u64>(high);
  r.w[3] = static_cast<u64>((high >> 64) + (p11 >> 64));
  return r;
}
inline int compare_u256(const U256& a, const U256& b) noexcept {
  for (int i = 3; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}
}  // namespace detail

// Signe de a/b - c/d, b > 0, d > 0.
inline int compare_fraction(i128 a, i128 b, i128 c, i128 d) noexcept {
  if (b == d) return a < c ? -1 : (a > c ? 1 : 0);  // meme denominateur positif (sites : d = 1)
  const int sa = a < 0 ? -1 : (a > 0 ? 1 : 0), sc = c < 0 ? -1 : (c > 0 ? 1 : 0);
  if (sa != sc) return sa < sc ? -1 : 1;
  if (sa == 0) return 0;
  using U = u128;
  const U ma = sa < 0 ? static_cast<U>(-(a + 1)) + 1 : static_cast<U>(a);
  const U mc = sc < 0 ? static_cast<U>(-(c + 1)) + 1 : static_cast<U>(c);
  const int magnitude = detail::compare_u256(detail::mul_u128(ma, static_cast<U>(d)),
                                             detail::mul_u128(mc, static_cast<U>(b)));
  return sa > 0 ? magnitude : -magnitude;
}

// Ordre lexicographique exact (x, y, z) de deux centres.
inline int compare_centers(const CenterRec& a, const CenterRec& b) noexcept {
  int s = compare_fraction(a.x, a.d, b.x, b.d);
  if (s != 0) return s;
  s = compare_fraction(a.y, a.d, b.y, b.d);
  if (s != 0) return s;
  return compare_fraction(a.z, a.d, b.z, b.d);
}

}  // namespace mhgp12::dump
