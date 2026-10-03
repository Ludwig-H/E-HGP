// Blocs fixes budgetes, sans agrandissement ; proprietaires intrusifs detruits iterativement apres leur contenu.
#pragma once
#include <cstddef>
#include <memory>
#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {

template <class T, u32 Width>
class FixedPages {
  static_assert(Width > 0);
  static_assert(std::is_nothrow_copy_assignable_v<T>);
  struct Page {
    Buffer<T> values;
    Buffer<std::byte> envelope;
    Page* next = nullptr;
    u64 used = 0;
    Page(Buffer<T>&& data, Buffer<std::byte>&& storage) noexcept
        : values(std::move(data)), envelope(std::move(storage)) {}
  };
  static_assert(alignof(Page) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
  static void release(Page* page) noexcept {
    // Le Buffer qui contient Page survit a la destruction de Page. Aucun acces apres cette liberation.
    Buffer<std::byte> envelope = std::move(page->envelope);
    std::destroy_at(page);
  }

 public:
  class Pending {
   public:
    Pending() = default;
    explicit Pending(Page* page) noexcept : page_(page) {}
    Pending(const Pending&) = delete;
    Pending& operator=(const Pending&) = delete;
    Pending(Pending&& other) noexcept : page_(std::exchange(other.page_, nullptr)) {}
    ~Pending() { if (page_ != nullptr) release(page_); }
   private:
    friend class FixedPages;
    Page* take() noexcept { return std::exchange(page_, nullptr); }
    Page* page_ = nullptr;
  };
  FixedPages() = default;
  FixedPages(const FixedPages&) = delete;
  FixedPages& operator=(const FixedPages&) = delete;
  ~FixedPages() {
    while (head_ != nullptr) { Page* next = head_->next; release(head_); head_ = next; }
  }
  static constexpr u64 metadata_bytes() noexcept { return sizeof(Page); }
  u64 size() const noexcept { return size_; }
  u64 blocks() const noexcept { return blocks_; }
  // Un append est borne par Width. S'il traverse la derniere page, UNE page supplementaire suffit.
  Result<Pending> prepare(u64 count, MemoryBudget& budget) const noexcept {
    if (count > Width) return fail(Reason::catalogue_invariant);
    if (count > Buffer<T>::kMaxCount - size_) return fail(Reason::memory_budget);
    if (count == 0 || (tail_ != nullptr && count <= Width - tail_->used)) return Pending{};
    Buffer<std::byte> storage;
    Buffer<T> data;
    MHGP11_TRY(storage.allocate(sizeof(Page), budget));
    MHGP11_TRY(data.allocate(Width, budget));
    auto* address = reinterpret_cast<Page*>(storage.data());
    return Pending{std::construct_at(address, std::move(data), std::move(storage))};
  }
  // Prive au protocole prepare/commit : les deux prepare d'une emission ont reussi avant le premier commit.
  void commit(Pending&& pending, std::span<const T> first, std::span<const T> second = {}) noexcept {
    Page* cursor = tail_;
    if (Page* page = pending.take()) {
      if (tail_ != nullptr) tail_->next = page; else head_ = page;
      if (cursor == nullptr) cursor = page;
      tail_ = page;
      ++blocks_;  // chaque page contient au moins un element ; blocks<=size apres commit
    }
    size_ += first.size() + second.size();
    for (auto values : {first, second}) while (!values.empty()) {
        const u64 copied = std::min<u64>(Width - cursor->used, values.size());
        std::copy_n(values.data(), copied, cursor->values.data() + cursor->used);
        cursor->used += copied;
        values = values.subspan(copied);
        if (!values.empty()) cursor = cursor->next;
      }
  }
  Outcome copy_to(std::span<T> output) const noexcept {
    if (output.size() != size_) return fail(Reason::catalogue_invariant);
    u64 at = 0;
    for (const Page* page = head_; page != nullptr; page = page->next) {
      if (page->used > output.size() - at) return fail(Reason::catalogue_invariant);
      std::copy_n(page->values.data(), page->used, output.data() + at);
      at += page->used;
    }
    return at == output.size() ? Outcome{} : fail(Reason::catalogue_invariant);
  }

 private:
  Page* head_ = nullptr;
  Page* tail_ = nullptr;
  u64 size_ = 0, blocks_ = 0;
};

inline constexpr u32 kEmissionBlock = 256, kPopulationBlock = 2048;
static_assert(kPopulationBlock >= kMaxLeaf);

class SinglePassOutput {
 public:
  Outcome append(const CatalogueBall& ball, const num::Level& level, std::span<const SiteIdx> interior,
                 std::span<const SiteIdx> shell, MemoryBudget& budget) noexcept {
    if (interior.size() != ball.p || shell.size() != ball.m || u64{ball.p} + ball.m > kMaxLeaf)
      return fail(Reason::catalogue_invariant);
    const u64 length = u64{ball.p} + ball.m;
    auto record = records_.prepare(1, budget);
    if (!record.ok()) return record.outcome();
    auto values = population_.prepare(length, budget);
    if (!values.ok()) return values.outcome();
    // Aucun echec ni allocation apres ce point ; valeurs triviales, longueur de feuille bornee.
    const Emission emission{ball, level, population_.size()};
    records_.commit(std::move(record.value()), {&emission, 1});
    population_.commit(std::move(values.value()), interior, shell);
    return {};
  }
  u64 balls() const noexcept { return records_.size(); }
  u64 incidences() const noexcept { return population_.size(); }
  Outcome account(CatalogueExecution& total) const noexcept;
  Outcome compact(std::span<Emission> records, std::span<SiteIdx> population, u64 offset) const noexcept {
    MHGP11_TRY(records_.copy_to(records));
    MHGP11_TRY(population_.copy_to(population));
    for (auto& record : records) MHGP11_TRY(checked_add(record.population_begin, offset));
    return {};
  }
 private:
  FixedPages<Emission, kEmissionBlock> records_;
  FixedPages<SiteIdx, kPopulationBlock> population_;
};

}  // namespace mhgp11::catalogue_detail
