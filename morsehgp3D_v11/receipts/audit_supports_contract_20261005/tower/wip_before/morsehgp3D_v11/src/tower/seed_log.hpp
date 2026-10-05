// Journal des graines de l'arbre d'ordre K (tranche S3), interne a tower : pour chaque cellule appliquee, en ordre
// BallIdx strictement croissant, les naissances rendues par la descente de chacune de ses traces strictes (jamais une
// racine du DSU ni un top). Ecrit par le seul fil qui applique les cellules (ForestBuilder::cell et regular_cell) ;
// capacites majorees sur la fenetre et admises avant le parcours. Lu ensuite par le balayage du lemme D.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::tower_detail {

// Fenetre des evenements d'ordre K : W_K = {b dans Cat_K : p+q-1 <= K <= p+m}, evenements FAIBLES compris
// (MATHEMATIQUES.md, T3 ; meme fenetre que classify_range). Ce n'est pas le predicat fort p+q <= K des temoins P3.
inline bool in_window(const CatalogueBall& ball, u64 k) noexcept {
  return u64{ball.p} + ball.qmin - 1 <= k && k <= u64{ball.p} + ball.m;
}

class SeedLog {
 public:
  SeedLog() = default;
  SeedLog(const SeedLog&) = delete;
  SeedLog& operator=(const SeedLog&) = delete;
  SeedLog(SeedLog&& other) noexcept
      : balls_(std::move(other.balls_)), offsets_(std::move(other.offsets_)), seeds_(std::move(other.seeds_)),
        cells_(std::exchange(other.cells_, 0)), count_(std::exchange(other.count_, 0)) {}
  // Remplacement : les tampons precedents sont rendus a leur budget.
  SeedLog& operator=(SeedLog&& other) noexcept {
    balls_ = std::move(other.balls_);
    offsets_ = std::move(other.offsets_);
    seeds_ = std::move(other.seeds_);
    cells_ = std::exchange(other.cells_, 0);
    count_ = std::exchange(other.count_, 0);
    return *this;
  }

  // Capacites de l'ordre k sur la fenetre : une cellule possible par boule de W_K hors naissance reguliere, q graines
  // pour une jonction reguliere, C(m,t) pour une coquille etendue (t=K-p ; majorant du nombre de traces strictes,
  // naissances etendues comprises). Octets 4C+8(C+1)+4G admis avant allocation.
  static Result<SeedLog> make(const Catalogue& catalogue, u32 k, MemoryBudget& budget) noexcept;

  // Nouvelle cellule : BallIdx strictement croissant, capacite respectee ; sinon tower_invariant.
  Outcome open(BallIdx ball) noexcept {
    if (cells_ >= balls_.size() || (cells_ != 0 && idx(balls_[cells_ - 1]) >= idx(ball)))
      return fail(Reason::tower_invariant);
    balls_[cells_] = ball;
    offsets_[cells_++] = count_;
    return {};
  }
  // Graine de naissance de la cellule ouverte.
  Outcome add(NodeIdx seed) noexcept {
    if (cells_ == 0 || count_ >= seeds_.size()) return fail(Reason::tower_invariant);
    seeds_[count_++] = seed;
    return {};
  }
  // Fin de la construction : borne de la derniere cellule.
  void close() noexcept { offsets_[cells_] = count_; }

  u32 cells() const noexcept { return cells_; }
  u64 seeds() const noexcept { return count_; }
  BallIdx ball(u32 cell) const noexcept { return balls_[cell]; }
  u64 begin(u32 cell) const noexcept { return offsets_[cell]; }
  u64 end(u32 cell) const noexcept { return offsets_[cell + 1]; }
  // Le balayage reecrit les graines en place (noeuds de coupe ouverte, puis branches compactees).
  std::span<NodeIdx> storage() noexcept { return seeds_.span().first(count_); }

 private:
  Buffer<BallIdx> balls_;
  Buffer<u64> offsets_;
  Buffer<NodeIdx> seeds_;
  u32 cells_ = 0;
  u64 count_ = 0;
};

class WindowAttachment;
// Balayage du lemme D apres finish() : rattachement a la coupe FERMEE de chaque boule de W_K, roles par les rangs,
// branches a la coupe OUVERTE ; controles I1 a I4 contre la foret et son registre, tout ecart rend tower_invariant.
// Consomme le journal (graines reecrites en place). attachment.cpp.
[[nodiscard]] Result<WindowAttachment> attach_window(const FullDomain&, const OrderForest&, SeedLog&,
                                                    MemoryBudget&) noexcept;

}  // namespace mhgp11::tower_detail
