// Export FUL1 (export_full.hpp) : un seul ecrivain, deux puits (fichier du dossier transactionnel, ou SHA-256 seul).
// Les octets sont ceux de serialize() de la sonde de la v11 : memes mots, memes largeurs d'entiers au profil 21.
#include <algorithm>
#include <array>
#include <memory>

#include "tower/export_full.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower {
namespace {

// Mots d'un numerateur global de centre : i128 aux profils 21 et 24 (forme de la v11), trois mots au profil 32.
inline constexpr int kCenterWords = kCoordBits <= 24 ? 2 : 3;

struct FileSink {
  io::FileWriter& out;
  Outcome put(std::span<const u8> bytes) noexcept { return out.bytes(bytes); }
};
struct HashSink {
  io::Sha256 sha;
  Outcome put(std::span<const u8> bytes) noexcept {
    sha.update(bytes);
    return {};
  }
};

// Flot d'octets par blocs de 64 Kio ; la premiere erreur du puits est gardee et rendue par finish.
template <class Sink>
class Stream {
 public:
  explicit Stream(Sink& sink) noexcept : sink_(sink) {}
  void bytes(const u8* data, u64 n) noexcept {
    size_ += n;
    while (n > 0) {
      const u64 room = std::min<u64>(n, block_.size() - used_);
      std::copy(data, data + room, block_.data() + used_);
      used_ += room;
      data += room;
      n -= room;
      if (used_ == block_.size()) flush();
    }
  }
  void word(u64 value) noexcept {
    std::array<u8, 8> b{};
    for (u32 i = 0; i < 8; ++i) b[i] = static_cast<u8>(value >> (8 * i));
    bytes(b.data(), 8);
  }
  // Entier exact : signe, nombre de mots (largeur du type), mots petit-boutistes.
  template <int Words>
  void integer(const num::Wide<Words>& value) noexcept {
    word(value.neg ? 1 : 0);
    word(Words);
    for (const u64 w : value.words) word(w);
  }
  Outcome finish() noexcept {
    flush();
    return status_;
  }
  u64 size() const noexcept { return size_; }

 private:
  void flush() noexcept {
    if (used_ > 0 && status_.ok()) status_ = sink_.put(std::span<const u8>(block_.data(), used_));
    used_ = 0;
  }
  Sink& sink_;
  std::array<u8, 1u << 16> block_{};
  u64 used_ = 0, size_ = 0;
  Outcome status_;
};

// Centre exact d'une naissance : numerateurs globaux a_j D + N_j puis D, comme la v11.
template <class Sink>
Outcome write_center(Stream<Sink>& out, const num::Sphere& sphere) noexcept {
  const auto den = num::to_wide(sphere.denominator());
  for (u32 axis = 0; axis < 3; ++axis) {
    num::Wide<4> product, numerator, global;
    if (!num::multiply_into<4>(num::Wide<1>::from_u64(sphere.anchor().coordinates()[axis]), den, product) ||
        !num::resize<4>(num::to_wide(sphere.numerator()[axis]), numerator) ||
        !num::add(product, numerator, global))
      return fail(Reason::tower_invariant);
    num::Wide<kCenterWords> written;
    if (!num::resize<kCenterWords>(global, written)) return fail(Reason::tower_invariant);
    out.integer(written);
  }
  out.integer(den);
  return {};
}

template <class Sink>
Outcome write_order(const FullSource& source, const OrderForest& f, Stream<Sink>& out) noexcept {
  const Cloud& cloud = *source.cloud;
  const u32 nn = f.nodes();
  out.word(f.k);
  out.word(f.births);
  out.word(nn);
  out.word(f.children.val.size());
  out.word(f.root);
  for (u32 v = 0; v < nn; ++v) {
    const u64 begin = f.children.off[v], end = f.children.off[u64{v} + 1];
    out.word(f.parent[v]);
    out.word(v < f.births ? 0 : begin);
    out.word(end - begin);
    if (f.rank[v] >= source.levels.size()) return fail(Reason::tower_invariant);
    const num::Level& level = source.levels[f.rank[v]];
    out.integer(num::to_wide(level.numerator()));
    out.integer(num::to_wide(level.denominator()));
    if (v < f.births) {
      const u32 key = f.birth_key[v];
      if (f.k == 1) {
        if (key >= cloud.sites()) return fail(Reason::tower_invariant);
        auto p = num::Point::make(cloud.x()[key], cloud.y()[key], cloud.z()[key]);
        if (!p.ok()) return p.outcome();
        MHGP12_TRY(write_center(out, num::Sphere::point(p.value())));
      } else {
        auto sphere = detail::ball_sphere(cloud, source.balls, key);
        if (!sphere.ok()) return sphere.outcome();
        MHGP12_TRY(write_center(out, sphere.value()));
      }
    }
    if (f.k > 1) out.word(f.lower[v]);
  }
  for (const u32 child : f.children.val) out.word(child);
  return {};
}

template <class Sink>
Outcome write_full(const FullSource& source, Stream<Sink>& out) noexcept {
  if (source.cloud == nullptr || source.forests == nullptr) return fail(Reason::tower_invariant);
  const Cloud& cloud = *source.cloud;
  const TowerForests& forests = *source.forests;
  if (forests.kmax < 1 || forests.kmax > kMaxOrder) return fail(Reason::tower_invariant);
  // Signature de dix octets du format FUL1 de la v11, ecrite caractere par caractere (regle [mhgp11]).
  static constexpr std::array<u8, 10> kSignature = {'M', 'H', 'G', 'P', '1', '1', 'F', 'U', 'L', '1'};
  out.bytes(kSignature.data(), kSignature.size());
  out.word(static_cast<u64>(kCoordBits));
  out.word(forests.kmax);
  out.word(cloud.sites());
  out.word(cloud.weight());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    out.word(cloud.x()[s]);
    out.word(cloud.y()[s]);
    out.word(cloud.z()[s]);
    out.word(cloud.w()[s]);
    for (const PointId id : cloud.points(SiteIdx{s})) out.word(idx(id));
  }
  for (u32 i = 0; i < forests.kmax; ++i) {
    const OrderForest& f = forests.orders[i];
    if (f.k != i + 1 || f.root >= f.nodes() || (f.k > 1 && f.lower.size() != f.nodes()))
      return fail(Reason::tower_invariant);
    MHGP12_TRY(write_order(source, f, out));
  }
  return {};
}

}  // namespace

Outcome export_full(const FullSource& source, io::FileWriter& out) noexcept {
  return guarded([&]() -> Outcome {
    FileSink sink{out};
    auto stream = std::make_unique<Stream<FileSink>>(sink);
    MHGP12_TRY(write_full(source, *stream));
    return stream->finish();
  });
}

Result<io::Digest> full_digest(const FullSource& source, u64* bytes) noexcept {
  return guarded([&]() -> Result<io::Digest> {
    HashSink sink;
    auto stream = std::make_unique<Stream<HashSink>>(sink);
    MHGP12_TRY(write_full(source, *stream));
    MHGP12_TRY(stream->finish());
    if (bytes != nullptr) *bytes = stream->size();
    return sink.sha.finish();
  });
}

}  // namespace mhgp12::tower
