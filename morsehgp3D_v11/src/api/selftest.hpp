// Auto-test F5 de l'environnement flottant (docs/ARCHITECTURE.md, paragraphe 4), interne au module api : la Session le
// joue a sa creation. Defense en profondeur, sans role dans les preuves. Mesure et jugement sont separes pour que la
// porte juge aussi des mesures fausses (mhgp11_api_selftest_judge) ; le produit n'appelle que environment_selftest.
#pragma once

#include <array>

#include "core/core.hpp"

namespace mhgp11::api_detail {

// Temoins : operations elementaires sur des operandes lus par volatile, dans l'environnement flottant courant.
inline constexpr std::size_t kWitnessCount = 15;

struct FloatProbe {
  bool traps_masked = false;  // aucune exception flottante demasquee (SSE et x87) : une operation rend une valeur
  int rounding = -1;          // std::fegetround()
  std::array<double, kWitnessCount> values{};  // resultats mesures, dans l'ordre de la table ; nuls si traps
};

// Mesure : lit l'etat des exceptions et le mode, puis, seulement si aucune exception n'est demasquee, calcule les
// temoins (une exception demasquee arreterait le processus par un signal au lieu d'un refus).
[[nodiscard]] FloatProbe measure_float_environment() noexcept;
// Jugement exact, sur les motifs binaires (zero signe confondu) : environment_selftest si une exception est
// demasquee, si le mode n'est pas l'un des quatre modes IEEE-754, si un noyau entier n'est pas exact (F2), si la
// precision n'est pas celle du binaire64, ou si un resultat n'est pas l'un des deux voisins du resultat exact (F3).
[[nodiscard]] Outcome judge_float_environment(const FloatProbe& probe) noexcept;
// judge_float_environment(measure_float_environment()).
[[nodiscard]] Outcome environment_selftest() noexcept;

}  // namespace mhgp11::api_detail
