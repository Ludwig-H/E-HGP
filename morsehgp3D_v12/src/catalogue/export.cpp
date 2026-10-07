// Export du catalogue au format MHGP12DP version 1 de genre << catalogue >> (CONTRAT_CATALOGUE.md, paragraphe 8 bis ;
// disposition de microbancs/mes_m3_m4_tour/common/format.hpp, ecrite ici a neuf : le produit ne depend pas des
// microbancs). Petit-boutiste. En-tete de 64 octets : magie MHGP12DP, version 1, genre 1, bits du profil, K, ordre 0,
// nombre de sections (5), nombre de sites, nom de la trame (24 octets, zeros en queue). Section : etiquette de 8
// octets, taille d'un element (u32), reserve (u32 nul), nombre d'elements (u64), elements, zeros jusqu'a un multiple
// de 8 octets. Sections, dans cet ordre : SITEXYZ (sites par SiteIdx, x y z en u32), BALLS (ordre canonique : rang,
// p, m, q, S* en SiteIdx croissants, 0xFFFFFFFF au-dela de q), POPOFF (B + 1 decalages u64), POPVAL (I puis U en u32),
// NLEVELS (nombre de niveaux distincts, niveau nul compris). Hors du chemin chronometre.
#include <array>

#include "catalogue/catalogue.hpp"

namespace mhgp12 {
namespace {

constexpr u32 kSections = 5;
constexpr u64 kBlock = 4096;

void put32(u8* at, u32 v) noexcept {
  for (int i = 0; i < 4; ++i) at[i] = static_cast<u8>(v >> (8 * i));
}
void put64(u8* at, u64 v) noexcept {
  for (int i = 0; i < 8; ++i) at[i] = static_cast<u8>(v >> (8 * i));
}

template <class Out>
Outcome header(Out& out, const Cloud& cloud, const Catalogue& catalogue, std::string_view frame) noexcept {
  std::array<u8, 64> h{};
  const char magic[8] = {'M', 'H', 'G', 'P', '1', '2', 'D', 'P'};
  for (int i = 0; i < 8; ++i) h[i] = static_cast<u8>(magic[i]);
  put32(h.data() + 8, 1);  // version
  put32(h.data() + 12, 1);  // genre catalogue
  put32(h.data() + 16, static_cast<u32>(kCoordBits));
  put32(h.data() + 20, catalogue.kmax());
  put32(h.data() + 24, 0);  // ordre
  put32(h.data() + 28, kSections);
  put64(h.data() + 32, cloud.sites());
  for (std::size_t i = 0; i < frame.size(); ++i) h[40 + i] = static_cast<u8>(frame[i]);
  return out.bytes(h);
}

template <class Out>
Outcome section(Out& out, std::string_view tag, u32 element_bytes, u64 count) noexcept {
  std::array<u8, 24> h{};
  for (std::size_t i = 0; i < tag.size(); ++i) h[i] = static_cast<u8>(tag[i]);
  put32(h.data() + 8, element_bytes);
  put64(h.data() + 16, count);
  return out.bytes(h);
}

template <class Out>
Outcome sites(Out& out, const Cloud& cloud) noexcept {
  MHGP12_TRY(section(out, "SITEXYZ", 12, cloud.sites()));
  std::array<u32, 3 * kBlock> block{};
  for (u64 first = 0; first < cloud.sites(); first += kBlock) {
    const u64 n = cloud.sites() - first < kBlock ? cloud.sites() - first : kBlock;
    for (u64 i = 0; i < n; ++i) {
      block[3 * i] = cloud.x()[first + i];
      block[3 * i + 1] = cloud.y()[first + i];
      block[3 * i + 2] = cloud.z()[first + i];
    }
    MHGP12_TRY(out.u32s({block.data(), static_cast<std::size_t>(3 * n)}));
  }
  return out.pad8();
}

template <class Out>
Outcome balls(Out& out, const Catalogue& catalogue) noexcept {
  MHGP12_TRY(section(out, "BALLS", 32, catalogue.balls()));
  std::array<u32, 8 * kBlock> block{};
  const auto data = catalogue.balls_data();
  for (u64 first = 0; first < data.size(); first += kBlock) {
    const u64 n = data.size() - first < kBlock ? data.size() - first : kBlock;
    for (u64 i = 0; i < n; ++i) {
      const CatalogueBall& b = data[first + i];
      u32* r = block.data() + 8 * i;
      r[0] = idx(b.rank);
      r[1] = b.p;
      r[2] = b.m;
      r[3] = b.qmin;
      for (u32 k = 0; k < 4; ++k) r[4 + k] = idx(b.support[k]);
    }
    MHGP12_TRY(out.u32s({block.data(), static_cast<std::size_t>(8 * n)}));
  }
  return out.pad8();
}

template <class Out>
Outcome populations(Out& out, const Catalogue& catalogue) noexcept {
  MHGP12_TRY(section(out, "POPOFF", 8, catalogue.population_offsets().size()));
  MHGP12_TRY(out.u64s(catalogue.population_offsets()));
  MHGP12_TRY(out.pad8());
  MHGP12_TRY(section(out, "POPVAL", 4, catalogue.population().size()));
  static_assert(sizeof(SiteIdx) == sizeof(u32));
  std::array<u32, kBlock> block{};
  const auto values = catalogue.population();
  for (u64 first = 0; first < values.size(); first += kBlock) {
    const u64 n = values.size() - first < kBlock ? values.size() - first : kBlock;
    for (u64 i = 0; i < n; ++i) block[i] = idx(values[first + i]);
    MHGP12_TRY(out.u32s({block.data(), static_cast<std::size_t>(n)}));
  }
  return out.pad8();
}

// Empreinte au fil de l'ecriture : memes octets que FileWriter (petit-boutiste, zeros d'alignement).
struct DigestSink {
  io::Sha256 sha;
  Outcome bytes(std::span<const u8> data) noexcept {
    sha.update(data);
    return {};
  }
  Outcome u32s(std::span<const u32> words) noexcept {
    for (const u32 w : words) {
      std::array<u8, 4> b{};
      put32(b.data(), w);
      sha.update(b);
    }
    return {};
  }
  Outcome u64s(std::span<const u64> words) noexcept {
    for (const u64 w : words) {
      std::array<u8, 8> b{};
      put64(b.data(), w);
      sha.update(b);
    }
    return {};
  }
  Outcome pad8() noexcept {
    const std::array<u8, 8> zeros{};
    const u64 rest = (8 - sha.bytes() % 8) % 8;
    sha.update(std::span<const u8>(zeros.data(), static_cast<std::size_t>(rest)));
    return {};
  }
};

template <class Out>
Outcome write_all(const Cloud& cloud, const Catalogue& catalogue, std::string_view frame, Out& out) noexcept {
  if (frame.size() >= 24) return fail(Reason::parameter_out_of_range);
  for (const char c : frame)  // ASCII imprimable (CST-0227 : le lecteur de transition refuse le reste)
    if (c < 0x20 || c > 0x7e) return fail(Reason::parameter_out_of_range);
  MHGP12_TRY(header(out, cloud, catalogue, frame));
  MHGP12_TRY(sites(out, cloud));
  MHGP12_TRY(balls(out, catalogue));
  MHGP12_TRY(populations(out, catalogue));
  MHGP12_TRY(section(out, "NLEVELS", 8, 1));
  const std::array<u64, 1> levels{catalogue.levels().size()};
  return out.u64s(levels);
}

}  // namespace

Outcome export_catalogue(const Cloud& cloud, const Catalogue& catalogue, std::string_view frame,
                         io::FileWriter& out) noexcept {
  return write_all(cloud, catalogue, frame, out);
}

Result<io::Digest> catalogue_digest(const Cloud& cloud, const Catalogue& catalogue, std::string_view frame) noexcept {
  DigestSink sink;
  MHGP12_TRY(write_all(cloud, catalogue, frame, sink));
  return sink.sha.finish();
}

}  // namespace mhgp12
