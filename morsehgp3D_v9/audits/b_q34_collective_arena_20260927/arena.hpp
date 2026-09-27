#pragma once

// Audit-only collective implementation. Explicit geometry port from
// b_q34_direct_bands_20260927/direct.hpp at 4badf8b7d; no Plan object is
// constructed here. All append/scatter targets are allocated before workers.
#include "../b_q34_factor_plan_20260926/plan.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <span>
#include <thread>

namespace mhgp9::audit::collective {
namespace fp = factor_plan;
using namespace mhgp9::gen;
enum class Mutant { None, Scatter, Mask6, WorkerThrow };
struct Request { u64 a_node, b_node, source_ordinal; std::uint8_t mask; };
struct FactorMeta {
  u64 original_first, first, class_first;
  std::uint32_t size;
  std::uint16_t classes;
  std::uint16_t reserved{};
  bool operator==(const FactorMeta&) const=default;
};
struct RectangleMeta {
  u64 source_ordinal, band_first;
  std::uint32_t bands;
  std::uint8_t mask;
  std::array<std::uint8_t,3> reserved{};
  bool operator==(const RectangleMeta&) const=default;
};
struct Class {
  std::uint32_t first, last;
  std::uint8_t credit;
  std::array<std::uint8_t,3> reserved{};
  bool operator==(const Class&) const=default;
};
struct Band { std::uint32_t a_class, b_first, b_last; bool operator==(const Band&) const=default; };
static_assert(sizeof(FactorMeta)==32 && sizeof(RectangleMeta)==24 && sizeof(Class)==12 && sizeof(Band)==12);
struct ExtraWork {
  u64 requests{}, factors{}, anchor_jobs{}, max_factor{}, grouping_reads{}, scatter_writes{};
  u64 histogram_slots{}, class_visits{}, band_passes{}, band_classes_b{}, band_classes_a{};
  u64 band_rows{}, band_binary_tests{}, prefix_entries{}, emitted_bands{};
  bool operator==(const ExtraWork&) const=default;
};
struct Mass { u64 all{}, q3{}, q4{}; bool operator==(const Mass&) const=default; };

namespace detail {
inline std::uint32_t u32(std::size_t value) {
  if (value>std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("arena.local_u32");
  return static_cast<std::uint32_t>(value);
}
inline std::size_t checked_size(u64 value) {
  if (value>std::numeric_limits<std::size_t>::max()) throw std::overflow_error("arena.address_space");
  return static_cast<std::size_t>(value);
}
inline std::uint8_t pack(unsigned q3,unsigned q4) { return static_cast<std::uint8_t>((q3<<4U)|q4); }
inline unsigned q3(std::uint8_t packed) { return packed>>4U; }
inline unsigned q4(std::uint8_t packed) { return packed&15U; }
inline std::size_t key(std::uint8_t packed) { return 10*q3(packed)+q4(packed); }
struct Pool {std::array<u64,10> ids{};std::size_t size{};};
inline Pool select_pool(const Q2CensusIndex& index,std::size_t own_node,std::size_t opposite_node,
                        unsigned k,fp::Work& work) {
  const auto own=index.spatial_nodes()[own_node],opposite=index.spatial_nodes()[opposite_node];
  const auto points=index.cloud().points();const auto order=index.spatial_order();
  std::array<i64,3> direction{};
  for (std::size_t d=0;d!=3;++d)
    direction[d]=static_cast<i64>(opposite.box.low[d])+opposite.box.high[d]-own.box.low[d]-own.box.high[d];
  struct Proposal {i64 score;std::size_t id;};
  std::array<Proposal,10> pool{};std::size_t used=0,capacity=std::min<std::size_t>(own.range.size(),k);
  counter_add(work.factor_sites,own.range.size());
  for (auto rank=own.range.first;rank!=own.range.last;++rank) {
    counter_add(work.selection_visits);const auto id=order[rank];i64 score=0;
    for (std::size_t d=0;d!=3;++d) score+=direction[d]*points[id][d];
    std::size_t at=0;
    while (at!=used) {counter_add(work.selection_tests);if (score>pool[at].score || (score==pool[at].score && id<pool[at].id)) break;++at;}
    if (at==capacity) continue;
    if (used<capacity) ++used;
    for (auto j=used-1;j!=at;--j) {pool[j]=pool[j-1];counter_add(work.selection_shifts);}
    pool[at]={score,id};
  }
  Pool out;out.size=used;
  for (std::size_t i=0;i!=used;++i) out.ids[i]=pool[i].id;
  counter_add(work.selected_sites,used);return out;
}
template<class Fn> void parallel_for(std::size_t tasks,unsigned workers,Fn fn) {
  if (tasks==0) return;
  std::atomic<std::size_t> next{};
  std::atomic<bool> stop{};
  std::exception_ptr error;
  std::mutex mutex;
  const auto worker=[&](unsigned id) {
    try {
      while (!stop.load(std::memory_order_relaxed)) {
        const auto task=next.fetch_add(1,std::memory_order_relaxed);
        if (task>=tasks) break;
        fn(id,task);
      }
    } catch (...) {
      { std::lock_guard lock(mutex);if (!error) error=std::current_exception(); }
      stop.store(true,std::memory_order_relaxed);
    }
  };
  std::vector<std::thread> threads;
  threads.reserve(workers-1);
  try {
    for (unsigned id=1;id!=workers;++id) threads.emplace_back(worker,id);
    worker(0);
  } catch (...) {
    stop.store(true,std::memory_order_relaxed);
    for (auto& t:threads) t.join();
    throw;
  }
  for (auto& t:threads) t.join();
  if (error) std::rethrow_exception(error);
}
inline void add_work(fp::Work& a,const fp::Work& b) {
#define ADD(field) counter_add(a.field,b.field)
  ADD(factor_sites);ADD(selection_visits);ADD(selection_tests);ADD(selection_shifts);ADD(selected_sites);
  ADD(anchor_visits);ADD(witness_attempts);ADD(self_skips);ADD(q3_credits);ADD(q4_credits);
  ADD(predicates.point_tests);ADD(predicates.universal_queries);ADD(predicates.q2_axis_terms);
  ADD(predicates.corner_tests);ADD(predicates.block_bound_tests);ADD(predicates.negative_probes);
#undef ADD
}
inline void add_extra(ExtraWork& a,const ExtraWork& b) {
#define ADD(field) counter_add(a.field,b.field)
  ADD(grouping_reads);ADD(scatter_writes);ADD(histogram_slots);ADD(class_visits);ADD(band_passes);
  ADD(band_classes_b);ADD(band_classes_a);ADD(band_rows);ADD(band_binary_tests);ADD(emitted_bands);
#undef ADD
}
}  // namespace detail

class Arena final {
 public:
  static std::shared_ptr<const Arena> build(Q2CensusIndexPtr index,std::span<const Request> requests,
                                           unsigned k,unsigned workers,std::uint32_t grain,
                                           Mutant mutant=Mutant::None) {
    auto result=std::shared_ptr<Arena>(new Arena(std::move(index),k,mutant));
    // Copy before validation. No caller-owned vector or mutable output alias
    // is adopted. Nothing escapes until every phase and worker has joined.
    const std::vector<Request> owned(requests.begin(),requests.end());
    result->construct(owned,workers,grain);
    return result;
  }
  Arena(const Arena&)=delete;Arena& operator=(const Arena&)=delete;
  Arena(Arena&&)=delete;Arena& operator=(Arena&&)=delete;
  [[nodiscard]] std::span<const RectangleMeta> rectangles() const {return rectangles_;}
  [[nodiscard]] std::span<const FactorMeta> factors() const {return factors_;}
  [[nodiscard]] std::span<const Class> classes() const {return classes_;}
  [[nodiscard]] std::span<const Band> bands() const {return bands_;}
  [[nodiscard]] std::span<const std::uint32_t> ranks() const {return ranks_;}
  [[nodiscard]] std::span<const std::uint8_t> credits() const {return credits_;}
  [[nodiscard]] const fp::Work& geometry_work() const {return geometry_;}
  [[nodiscard]] const ExtraWork& work() const {return extra_;}
  [[nodiscard]] Mass mass() const {return mass_;}
  [[nodiscard]] u64 temporary_bytes_peak() const {return temporary_peak_;}
  [[nodiscard]] u64 owned_bytes_peak_bound() const {return owned_peak_bound_;}
  [[nodiscard]] std::size_t retained_bytes() const {
    return sizeof(*this)+rectangles_.capacity()*sizeof(RectangleMeta)+factors_.capacity()*sizeof(FactorMeta)+
      classes_.capacity()*sizeof(Class)+bands_.capacity()*sizeof(Band)+ranks_.capacity()*sizeof(std::uint32_t)+credits_.capacity();
  }
  [[nodiscard]] unsigned retained_buffers() const {
    return unsigned(rectangles_.capacity()!=0)+unsigned(factors_.capacity()!=0)+unsigned(classes_.capacity()!=0)+
      unsigned(bands_.capacity()!=0)+unsigned(ranks_.capacity()!=0)+unsigned(credits_.capacity()!=0);
  }
  [[nodiscard]] unsigned pair_mask(std::size_t rectangle,std::size_t a_class,std::size_t b_rank) const {
    if (rectangle>=rectangles_.size()) throw std::out_of_range("arena.rectangle_query");
    const auto& a=factors_[2*rectangle];const auto& b=factors_[2*rectangle+1];
    if (a_class>=a.classes || b_rank>=b.size) throw std::out_of_range("arena.local_query");
    const auto ac=classes_[a.class_first+a_class].credit,bc=credits_[b.first+b_rank];
    unsigned out=0;const auto mask=rectangles_[rectangle].mask;
    if ((mask&2U)!=0 && detail::q3(ac)+detail::q3(bc)<k_-1U) out|=2U;
    if ((mask&4U)!=0 && detail::q4(ac)+detail::q4(bc)<k_-2U) out|=4U;
    return mutant_==Mutant::Mask6 && out!=0?6U:out;
  }
 private:
  struct alignas(64) Worker {fp::Work geometry;ExtraWork extra;};
  struct Job {u64 factor;std::uint32_t first,last;};
  struct BandResult {Mass mass;u64 count{};};
  Arena(Q2CensusIndexPtr index,unsigned k,Mutant mutant):index_(std::move(index)),k_(k),mutant_(mutant) {}
  template<class Emit> BandResult group(std::size_t rectangle,ExtraWork& work,Emit emit) const {
    counter_add(work.band_passes);
    const auto& a=factors_[2*rectangle];const auto& b=factors_[2*rectangle+1];
    const auto ag=std::span(classes_).subspan(a.class_first,a.classes);
    const auto bg=std::span(classes_).subspan(b.class_first,b.classes);
    struct Row {unsigned credit;std::size_t first,last;std::uint32_t rank;};
    std::array<Row,10> rows{};std::size_t row_count=0;
    std::array<std::uint32_t,11> q3_prefix{},q4_prefix{};
    counter_add(work.histogram_slots,22);
    for (std::size_t i=0;i!=bg.size();++i) {
      counter_add(work.band_classes_b);const auto& g=bg[i];
      const auto c3=detail::q3(g.credit),c4=detail::q4(g.credit);
      q3_prefix[c3+1]+=g.last-g.first;q4_prefix[c4+1]+=g.last-g.first;
      if (row_count==0 || rows[row_count-1].credit!=c3) rows[row_count++]={c3,i,i+1,g.first};
      else rows[row_count-1].last=i+1;
    }
    for (std::size_t i=1;i!=11;++i) {q3_prefix[i]+=q3_prefix[i-1];q4_prefix[i]+=q4_prefix[i-1];}
    BandResult result;const auto mask=rectangles_[rectangle].mask;
    const auto append=[&](std::size_t ai,std::uint32_t first,std::uint32_t last) {
      if (first==last) return;
      emit(Band{detail::u32(ai),first,last},result.count);
      counter_add(result.count);
      counter_add(result.mass.all,fp::product(ag[ai].last-ag[ai].first,last-first));
    };
    for (std::size_t ai=0;ai!=ag.size();++ai) {
      counter_add(work.band_classes_a);const auto& g=ag[ai];
      const unsigned r3=(mask&2U)!=0?k_-1U-detail::q3(g.credit):0;
      const unsigned r4=(mask&4U)!=0?k_-2U-detail::q4(g.credit):0;
      counter_add(result.mass.q3,fp::product(g.last-g.first,q3_prefix[r3]));
      counter_add(result.mass.q4,fp::product(g.last-g.first,q4_prefix[r4]));
      append(ai,0,q3_prefix[r3]);if (r4==0) continue;
      for (std::size_t ri=0;ri!=row_count;++ri) {
        counter_add(work.band_rows);const auto& row=rows[ri];if (row.credit<r3) continue;
        auto first=row.first,last=row.last;
        while (first!=last) {
          counter_add(work.band_binary_tests);const auto middle=first+(last-first)/2;
          if (detail::q4(bg[middle].credit)<r4) first=middle+1;else last=middle;
        }
        append(ai,row.rank,first==row.last?bg[first-1].last:bg[first].first);
      }
    }
    return result;
  }
  void construct(const std::vector<Request>& requests,unsigned workers,std::uint32_t grain) {
    if (!index_ || k_<2 || k_>10 || workers==0 || grain==0) throw std::invalid_argument("arena.configuration");
    const auto nodes=index_->spatial_nodes();const auto points=index_->cloud().points();const auto order=index_->spatial_order();
    if (requests.size()>std::numeric_limits<std::size_t>::max()/2) throw std::overflow_error("arena.factor_count");
    rectangles_.resize(requests.size());factors_.resize(2*requests.size());
    std::vector<u64> pool_first(factors_.size()+1,0);
    u64 F=0,P=0;
    for (std::size_t r=0;r!=requests.size();++r) {
      const auto& request=requests[r];
      if (request.a_node>=nodes.size() || request.b_node>=nodes.size() || request.mask==0 || (request.mask&~6U)!=0 ||
          (k_==2 && (request.mask&4U)!=0) || (r!=0 && requests[r-1].source_ordinal>=request.source_ordinal))
        throw std::invalid_argument("arena.request");
      const auto ar=nodes[request.a_node].range,br=nodes[request.b_node].range;
      if (ar.size()==0 || br.size()==0 || !(ar.last<=br.first || br.last<=ar.first))
        throw std::invalid_argument("arena.factors_disjoint");
      rectangles_[r]={request.source_ordinal,0,0,request.mask,{}};
      for (unsigned side=0;side!=2;++side) {
        const auto range=side==0?ar:br;const auto fi=2*r+side;
        factors_[fi]={static_cast<u64>(range.first),F,0,detail::u32(range.size()),0,0};
        pool_first[fi]=P;counter_add(F,static_cast<u64>(range.size()));
        counter_add(P,static_cast<u64>(std::min<std::size_t>(range.size(),k_)));
        extra_.max_factor=std::max(extra_.max_factor,static_cast<u64>(range.size()));
      }
    }
    pool_first.back()=P;extra_.requests=requests.size();extra_.factors=factors_.size();
    extra_.prefix_entries=factors_.size();
    std::vector<u64> pools(detail::checked_size(P));
    std::vector<std::uint8_t> original_credit(detail::checked_size(F));
    std::vector<Worker> states(workers);
    detail::parallel_for(factors_.size(),workers,[&](unsigned worker,std::size_t fi) {
      const auto& req=requests[fi/2];
      const auto pool=detail::select_pool(*index_,(fi&1U)==0?req.a_node:req.b_node,
          (fi&1U)==0?req.b_node:req.a_node,k_,states[worker].geometry);
      for (std::size_t i=0;i!=pool.size;++i) pools[pool_first[fi]+i]=pool.ids[i];
    });
    std::vector<Job> jobs;
    u64 job_count=0;for (const auto& f:factors_) counter_add(job_count,(u64(f.size)+grain-1)/grain);
    jobs.reserve(detail::checked_size(job_count));
    for (std::size_t fi=0;fi!=factors_.size();++fi) for (u64 first=0;first<factors_[fi].size;first+=grain)
      jobs.push_back({fi,static_cast<std::uint32_t>(first),static_cast<std::uint32_t>(std::min<u64>(first+grain,factors_[fi].size))});
    extra_.anchor_jobs=jobs.size();
    const auto scratch_bytes=[&] {
      return requests.capacity()*sizeof(Request)+pool_first.capacity()*sizeof(u64)+pools.capacity()*sizeof(u64)+
          original_credit.capacity()+states.capacity()*sizeof(Worker)+jobs.capacity()*sizeof(Job);
    };
    // Capacity accounting includes both credit orders when live. It excludes
    // thread-library allocations and call stacks: not a process RSS bound.
    const auto account=[&] {
      const auto temporary=scratch_bytes();
      temporary_peak_=std::max(temporary_peak_,static_cast<u64>(temporary));
      owned_peak_bound_=std::max(owned_peak_bound_,static_cast<u64>(retained_bytes()+temporary));
    };
    account();
    detail::parallel_for(jobs.size(),workers,[&](unsigned worker,std::size_t ji) {
      if (mutant_==Mutant::WorkerThrow && ji==0) throw std::runtime_error("arena.injected_worker");
      const auto& job=jobs[ji];const auto& f=factors_[job.factor];const auto& req=requests[job.factor/2];
      const auto opposite=nodes[(job.factor&1U)==0?req.b_node:req.a_node].box;
      auto& work=states[worker].geometry;
      for (u64 local=job.first;local!=job.last;++local) {
        counter_add(work.anchor_visits);const auto id=order[f.original_first+local];unsigned c3=0,c4=0;
        for (u64 pi=pool_first[job.factor];pi!=pool_first[job.factor+1];++pi) {
          const auto witness=pools[pi];if (witness==id) {counter_add(work.self_skips);continue;}
          if ((req.mask&2U)!=0 && c3<k_-1U) {
            counter_add(work.witness_attempts);
            if (fp::certify(Lane::Q3,points[id],opposite,points[witness],work.predicates,fp::Mutant::None)) {++c3;counter_add(work.q3_credits);}
          }
          if ((req.mask&4U)!=0 && c4<k_-2U) {
            counter_add(work.witness_attempts);
            if (fp::certify(Lane::Q4,points[id],opposite,points[witness],work.predicates,fp::Mutant::None)) {++c4;counter_add(work.q4_credits);}
          }
        }
        original_credit[f.first+local]=detail::pack(c3,c4);
      }
    });
    detail::parallel_for(factors_.size(),workers,[&](unsigned worker,std::size_t fi) {
      auto& f=factors_[fi];auto& work=states[worker].extra;std::array<std::uint32_t,100> counts{};
      counter_add(work.histogram_slots,100);
      for (u64 local=0;local!=f.size;++local) {++counts[detail::key(original_credit[f.first+local])];counter_add(work.grouping_reads);}
      for (const auto count:counts) if (count!=0) ++f.classes;
    });
    u64 C=0;for (auto& f:factors_) {f.class_first=C;counter_add(C,f.classes);counter_add(extra_.prefix_entries);}
    classes_.resize(detail::checked_size(C));ranks_.resize(detail::checked_size(F));credits_.resize(detail::checked_size(F));account();
    detail::parallel_for(factors_.size(),workers,[&](unsigned worker,std::size_t fi) {
      const auto& f=factors_[fi];auto& work=states[worker].extra;
      std::array<std::uint32_t,100> counts{},cursor{};counter_add(work.histogram_slots,200);
      for (u64 local=0;local!=f.size;++local) {++counts[detail::key(original_credit[f.first+local])];counter_add(work.grouping_reads);}
      std::uint32_t at=0;u64 ci=f.class_first;
      for (std::size_t key=0;key!=100;++key) if (counts[key]!=0) {
        cursor[key]=at;classes_[ci++]={at,at+counts[key],detail::pack(key/10,key%10),{}};at+=counts[key];counter_add(work.class_visits);
      }
      if (ci!=f.class_first+f.classes || at!=f.size) throw std::runtime_error("arena.class_count_disagrees");
      for (std::uint32_t local=0;local!=f.size;++local) {
        counter_add(work.grouping_reads);const auto credit=original_credit[f.first+local];const auto target=f.first+cursor[detail::key(credit)]++;
        ranks_[target]=mutant_==Mutant::Scatter?(local+1U)%f.size:local;credits_[target]=credit;counter_add(work.scatter_writes);
      }
    });
    // Scratch needed only by preparation is released before final bands.
    std::vector<u64>().swap(pools);std::vector<u64>().swap(pool_first);
    std::vector<std::uint8_t>().swap(original_credit);std::vector<Job>().swap(jobs);
    std::vector<Mass> rectangle_mass(requests.size());
    detail::parallel_for(rectangles_.size(),workers,[&](unsigned worker,std::size_t r) {
      const auto result=group(r,states[worker].extra,[](const Band&,u64) {});
      rectangles_[r].bands=detail::u32(detail::checked_size(result.count));rectangle_mass[r]=result.mass;
    });
    u64 D=0;for (auto& r:rectangles_) {r.band_first=D;counter_add(D,r.bands);counter_add(extra_.prefix_entries);}
    bands_.resize(detail::checked_size(D));
    account();
    owned_peak_bound_=std::max(owned_peak_bound_,static_cast<u64>(retained_bytes()+scratch_bytes()+rectangle_mass.capacity()*sizeof(Mass)));
    temporary_peak_=std::max(temporary_peak_,static_cast<u64>(scratch_bytes()+rectangle_mass.capacity()*sizeof(Mass)));
    detail::parallel_for(rectangles_.size(),workers,[&](unsigned worker,std::size_t r) {
      const auto result=group(r,states[worker].extra,[&](const Band& band,u64 i) {
        if (i>=rectangles_[r].bands) throw std::runtime_error("arena.band_count_overrun");
        bands_[rectangles_[r].band_first+i]=band;counter_add(states[worker].extra.emitted_bands);
      });
      if (result.count!=rectangles_[r].bands || result.mass!=rectangle_mass[r]) throw std::runtime_error("arena.band_count_disagrees");
    });
    for (const auto mass:rectangle_mass) {counter_add(mass_.all,mass.all);counter_add(mass_.q3,mass.q3);counter_add(mass_.q4,mass.q4);}
    for (const auto& state:states) {detail::add_work(geometry_,state.geometry);detail::add_extra(extra_,state.extra);}
  }
  Q2CensusIndexPtr index_;unsigned k_;Mutant mutant_;
  std::vector<RectangleMeta> rectangles_;std::vector<FactorMeta> factors_;
  std::vector<std::uint32_t> ranks_;std::vector<std::uint8_t> credits_;
  std::vector<Class> classes_;std::vector<Band> bands_;
  fp::Work geometry_;ExtraWork extra_;Mass mass_;
  u64 temporary_peak_{},owned_peak_bound_{};
};
}  // namespace mhgp9::audit::collective
