// Exportateur de TEST de l'etage G (hors produit, hors du chemin chronometre) : compteurs d'un ordre dans un ordre
// fixe, et fichier res.bin au format MHGP12DP version 1, genre 4 << resolution >> (disposition de
// microbancs/mes_m3_m4_tour/common/format.hpp : en-tete de 64 octets ; section = etiquette de 8 octets, taille d'un
// element u32, reserve u32 nul, nombre d'elements u64, elements, zeros jusqu'a un multiple de 8 octets). Lu par
// tests/tower/g_dump.py. Empreinte : SHA-256 des octets exacts des sections de cet export, en-tete exclu (portes de
// determinisme ; memes empreintes aux trois profils sur les memes coordonnees).
#pragma once

#include <array>
#include <span>
#include <string_view>
#include <type_traits>

#include "io/io.hpp"
#include "tower/tower.hpp"

namespace mhgp12::probe {

inline constexpr std::size_t kObjectCounters = 5;
inline constexpr std::size_t kScalarCounters = 28;
inline constexpr std::size_t kCounterCount = kScalarCounters + kChainBins;
inline constexpr const char* kCounterNames[kScalarCounters] = {
    "births", "cells", "inert_cells", "extended_cells", "representatives", "probes", "first_probe_hits",
    "probe_hits_after_steps", "route_t1", "route_cert_table", "route_cert_census", "route_fallback_table",
    "route_fallback_census", "fallback_no_proposal", "fallback_not_in_part", "fallback_certificate",
    "census_saturated", "census_complete", "census_sites", "census_sites_max", "census_nodes", "jumps_catalogue",
    "jumps_census", "inert_steps", "cell_stops", "birth_stops", "controls", "max_chain"};

inline std::array<u64, kCounterCount> counter_values(const OrderCounters& c) {
  std::array<u64, kCounterCount> v{c.births, c.cells, c.inert_cells, c.extended_cells, c.representatives, c.probes,
                                   c.first_probe_hits, c.probe_hits_after_steps, c.route_t1, c.route_cert_table,
                                   c.route_cert_census, c.route_fallback_table, c.route_fallback_census,
                                   c.fallback_no_proposal, c.fallback_not_in_part, c.fallback_certificate,
                                   c.census_saturated, c.census_complete, c.census_sites, c.census_sites_max,
                                   c.census_nodes, c.jumps_catalogue, c.jumps_census, c.inert_steps, c.cell_stops,
                                   c.birth_stops, c.controls, c.max_chain};
  for (int i = 0; i < kChainBins; ++i) v[kScalarCounters + static_cast<std::size_t>(i)] = c.chain_histogram[i];
  return v;
}

namespace detail {

inline void put32(u8* at, u32 v) {
  for (int i = 0; i < 4; ++i) at[i] = static_cast<u8>(v >> (8 * i));
}
inline void put64(u8* at, u64 v) {
  for (int i = 0; i < 8; ++i) at[i] = static_cast<u8>(v >> (8 * i));
}

template <class Out>
Outcome section(Out& out, std::string_view tag, u32 element, u64 count) {
  std::array<u8, 24> h{};
  for (std::size_t i = 0; i < tag.size() && i < 8; ++i) h[i] = static_cast<u8>(tag[i]);
  put32(h.data() + 8, element);
  put64(h.data() + 16, count);
  return out.bytes(h);
}

template <class Out, class T>
Outcome column(Out& out, std::string_view tag, int k, std::span<const T> values) {
  std::array<char, 8> name{};  // etiquette XXXX_kk, completee par un zero
  for (std::size_t i = 0; i < 4 && i < tag.size(); ++i) name[i] = tag[i];
  name[4] = '_';
  name[5] = static_cast<char>('0' + k / 10);
  name[6] = static_cast<char>('0' + k % 10);
  MHGP12_TRY(section(out, std::string_view(name.data(), 8), sizeof(T), values.size()));
  std::array<u8, 8 * 512> block{};
  std::size_t used = 0;
  for (const T& value : values) {
    u64 raw = 0;
    if constexpr (std::is_enum_v<T>) raw = static_cast<u64>(idx(value));
    else raw = static_cast<u64>(value);
    for (std::size_t b = 0; b < sizeof(T); ++b) block[used++] = static_cast<u8>(raw >> (8 * b));
    if (used + 8 > block.size()) {
      MHGP12_TRY(out.bytes(std::span<const u8>(block.data(), used)));
      used = 0;
    }
  }
  if (used != 0) MHGP12_TRY(out.bytes(std::span<const u8>(block.data(), used)));
  return out.pad8();
}

// header = false : sections seules (empreinte : l'en-tete porte le profil et la trame, les sections sont les memes aux
// profils 21, 24 et 32 sur les memes coordonnees).
template <class Out>
Outcome write_all(const Cloud& cloud, const Resolution& r, std::string_view frame, Out& out, bool header = true) {
  if (frame.size() >= 24) return fail(Reason::parameter_out_of_range);
  constexpr u32 kPerOrder = 9;
  std::array<u8, 64> h{};
  const char magic[8] = {'M', 'H', 'G', 'P', '1', '2', 'D', 'P'};
  for (int i = 0; i < 8; ++i) h[i] = static_cast<u8>(magic[i]);
  put32(h.data() + 8, 1);
  put32(h.data() + 12, 4);  // genre << resolution >>
  put32(h.data() + 16, static_cast<u32>(kCoordBits));
  put32(h.data() + 20, r.kmax());
  put32(h.data() + 24, r.orders());
  put32(h.data() + 28, kPerOrder * r.orders());
  put64(h.data() + 32, cloud.sites());
  for (std::size_t i = 0; i < frame.size(); ++i) h[40 + i] = static_cast<u8>(frame[i]);
  if (header) MHGP12_TRY(out.bytes(h));
  for (Order k = 1; k <= r.orders(); ++k) {
    const ResolvedOrder& o = r.order(k);
    MHGP12_TRY(column(out, "BKEY", k, o.birth_keys()));
    MHGP12_TRY(column(out, "BRNK", k, o.birth_ranks()));
    MHGP12_TRY(column(out, "CBAL", k, o.cell_balls()));
    MHGP12_TRY(column(out, "CRNK", k, o.cell_ranks()));
    MHGP12_TRY(column(out, "CFLG", k, o.cell_flags()));
    MHGP12_TRY(column(out, "COFF", k, o.cell_offsets()));
    MHGP12_TRY(column(out, "TMSK", k, o.trace_masks()));
    MHGP12_TRY(column(out, "TARG", k, o.targets()));
    const auto v = counter_values(o.counters());
    MHGP12_TRY(column(out, "CNTR", k, std::span<const u64>(v.data(), v.size())));
  }
  return {};
}

struct DigestSink {
  io::Sha256 sha;
  Outcome bytes(std::span<const u8> data) {
    sha.update(data);
    return {};
  }
  Outcome pad8() {
    const std::array<u8, 8> zeros{};
    sha.update(std::span<const u8>(zeros.data(), static_cast<std::size_t>((8 - sha.bytes() % 8) % 8)));
    return {};
  }
};

}  // namespace detail

inline Outcome write_resolution(const Cloud& cloud, const Resolution& r, std::string_view frame, io::FileWriter& out) {
  return detail::write_all(cloud, r, frame, out);
}

inline Result<io::Digest> resolution_digest(const Cloud& cloud, const Resolution& r, std::string_view frame) {
  detail::DigestSink sink;
  MHGP12_TRY(detail::write_all(cloud, r, frame, sink, false));
  return sink.sha.finish();
}

}  // namespace mhgp12::probe
