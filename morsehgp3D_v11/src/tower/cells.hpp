// Cellules privees : toutes les traces strictes, sans quotient local ni decision de fusion globale.
#pragma once

#include <limits>

#include "tower/meb.hpp"

namespace mhgp11::tower_detail {

struct CellTrace {
  std::array<SiteIdx, kMaxMebSites> sites{};
  u8 arity = 0;
  std::span<const SiteIdx> part() const noexcept { return {sites.data(), arity}; }
};

enum class CellKind : u8 { birth, strict_traces };

// combinations est C(m,t), meme pour la voie analytique ; tous les autres compteurs sont le travail
// reel cumule des DEUX passes et non une prediction. Une cellule analytique a passes=trace_tests=0.
struct CellLedger {
  u64 combinations = 0, passes = 0, trace_tests = 0, meb_calls = 0;
  MebLedger meb;
  friend bool operator==(const CellLedger&, const CellLedger&) = default;
};

// Classification seule : combinations est l'univers C(m,t), examined le prefixe reellement teste.
// Les deux raccourcis analytiques ont examined=meb_calls=0. Ce ledger ne compte aucun rejeu de traces.
struct ClassificationLedger {
  u64 combinations = 0, examined = 0, meb_calls = 0;
  MebLedger meb;
  friend bool operator==(const ClassificationLedger&, const ClassificationLedger&) = default;
};

struct CellClassifier;
class CellClassification {
 public:
  CellKind kind() const noexcept { return kind_; }
  const ClassificationLedger& ledger() const noexcept { return ledger_; }
 private:
  friend struct CellClassifier;
  CellClassification(CellKind kind, ClassificationLedger ledger) noexcept : kind_(kind), ledger_(ledger) {}
  CellKind kind_;
  ClassificationLedger ledger_;
};

// Meme fenetre et garde combinatoire que build_cell ; aucune allocation ni trace materialisee.
// t=m donne une naissance, t<qmin une trace stricte ; sinon premier A lexicographique de beta(A)<lambda.
// L'absence de temoin exige le parcours complet. Le rejeu FULL doit encore visiter TOUTES les traces.
[[nodiscard]] Result<CellClassification> classify_cell(const FullDomain&, BallIdx, Order) noexcept;

// Helpers de capacite/coherence, aussi exerces aux frontieres scalaires par les tests.
[[nodiscard]] Result<u64> cell_binomial(u32 m, u32 t) noexcept;
// En ligne : appele a chaque cellule et a chaque union par les publieurs (profil du 6 octobre 2026).
[[nodiscard]] inline Outcome cell_add(u64& target, u64 value) noexcept {
  if (value > std::numeric_limits<u64>::max() - target) return fail(Reason::tower_capacity);
  target += value;
  return {};
}
[[nodiscard]] Outcome cell_same_pass(u64 counted, u64 filled, const CellLedger& first,
                                     const CellLedger& second) noexcept;

struct CellBuilder;
class LocalCell {
 public:
  LocalCell(const LocalCell&) = delete;
  LocalCell& operator=(const LocalCell&) = delete;
  LocalCell& operator=(LocalCell&&) = delete;
  LocalCell(LocalCell&& other) noexcept
      : traces_(std::move(other.traces_)), ball_(std::exchange(other.ball_, BallIdx{kNone})),
        order_(std::exchange(other.order_, 0)), regular_(std::exchange(other.regular_, false)),
        ledger_(std::exchange(other.ledger_, {})) {}
  BallIdx ball() const noexcept { return ball_; }
  Order order() const noexcept { return order_; }
  bool regular() const noexcept { return regular_; }
  CellKind kind() const noexcept { return traces_.size() == 0 ? CellKind::birth : CellKind::strict_traces; }
  std::span<const CellTrace> traces() const noexcept { return traces_.span(); }
  const CellLedger& ledger() const noexcept { return ledger_; }

 private:
  friend struct CellBuilder;
  LocalCell(Buffer<CellTrace>&& traces, BallIdx ball, Order order, bool regular, CellLedger ledger) noexcept
      : traces_(std::move(traces)), ball_(ball), order_(order), regular_(regular), ledger_(ledger) {}
  Buffer<CellTrace> traces_;
  BallIdx ball_;
  Order order_;
  bool regular_;
  CellLedger ledger_;
};

// Domaine et BallIdx lies ; k dans [p+qmin-1,min(p+m,K)] seulement. Hors fenetre : refus de parametre.
// Traces possedees I union A, SiteIdx croissants, padding kNone ; ordre lexicographique des A de U.
// Le nombre de traces N'EST PAS le nombre de morceaux, ni l'arite d'une future fusion. Le plateau devra
// resoudre toutes les traces et dedupliquer les racines globales. Une naissance a zero trace stricte.
// C(m,t) controle avant parcours ; pas de quota de coquille/traces. Refus sans resultat partiel.
// Une allocation exacte sizeof(CellTrace)*S, pas de tableau de C(m,t) candidats ni DSU quadratique.
// Les IDs restent ceux du domaine ; aucune vue vers celui-ci dans LocalCell. Le budget survit au resultat.
[[nodiscard]] Result<LocalCell> build_cell(const FullDomain& domain, BallIdx ball, Order order,
                                          MemoryBudget& budget) noexcept;

}  // namespace mhgp11::tower_detail
