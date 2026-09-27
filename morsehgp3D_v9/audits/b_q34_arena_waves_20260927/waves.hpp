#pragma once

// Audit-only consumer of the immutable fd1a2c7ee collective arena.
// No historical Plan object, old cells, or arrays indexed by global P/E.
#include "../b_q34_collective_arena_20260927/arena.hpp"
#include "gen/pipeline/wspd_q34.hpp"

namespace mhgp9::audit::waves {
namespace ca=collective;
using namespace mhgp9::gen;
inline constexpr u64 absent=std::numeric_limits<u64>::max();
enum class Mutant { None, Mask6, SkipFallback, GroupOrdinal };
struct Mass {u64 all{},q3{},q4{};bool operator==(const Mass&) const=default;};
inline void add(Mass& m,u64 size,unsigned mask) {
  counter_add(m.all,size);if ((mask&2U)!=0) counter_add(m.q3,size);if ((mask&4U)!=0) counter_add(m.q4,size);
}
inline u64 product(u64 a,u64 b) {
  if (a!=0 && b>std::numeric_limits<u64>::max()/a) throw std::overflow_error("waves.product_overflow");
  return a*b;
}
inline u64 sum(u64 a,u64 b) {counter_add(a,b);return a;}
struct Rectangle {
  u64 raw_base{},arena_rectangle{absent};
  std::uint32_t a_first{},b_first{},a_size{},b_size{};
  std::uint8_t mask{};
};
inline std::array<u64,2> fallback_ranks(const Rectangle& r,u64 offset) {
  if (r.b_size==0 || offset>=product(r.a_size,r.b_size)) throw std::out_of_range("waves.fallback_offset");
  return {sum(r.a_first,offset/r.b_size),sum(r.b_first,offset%r.b_size)};
}
inline u64 original_ordinal(const Rectangle& r,u64 a,u64 b) {
  if (a<r.a_first || b<r.b_first || a-r.a_first>=r.a_size || b-r.b_first>=r.b_size)
    throw std::out_of_range("waves.original_ranks");
  return sum(r.raw_base,sum(product(a-r.a_first,r.b_size),b-r.b_first));
}
struct Segment {u64 rectangle{},band{absent},end{};};
struct PreparedWork {u64 input_rectangles{},closed{},fallbacks{},planned{},raw_pairs{};};
class Prepared final {
 public:
  static std::shared_ptr<const Prepared> build(Q2CensusIndexPtr index,
      std::span<const WspdRectangle> rectangles,unsigned k,unsigned arena_workers=1) {
    auto p=std::shared_ptr<Prepared>(new Prepared(std::move(index),k));
    const std::vector<WspdRectangle> owned(rectangles.begin(),rectangles.end());
    p->construct(owned,arena_workers);return p;
  }
  Prepared(const Prepared&)=delete;Prepared& operator=(const Prepared&)=delete;
  Prepared(Prepared&&)=delete;Prepared& operator=(Prepared&&)=delete;
  [[nodiscard]] const Q2CensusIndex& index() const {return *index_;}
  [[nodiscard]] unsigned k() const {return k_;}
  [[nodiscard]] std::span<const Rectangle> rectangles() const {return rectangles_;}
  [[nodiscard]] std::span<const Segment> segments() const {return segments_;}
  [[nodiscard]] const ca::Arena* arena() const {return arena_.get();}
  [[nodiscard]] Mass logical() const {return logical_;}
  [[nodiscard]] Mass effective() const {return effective_;}
  [[nodiscard]] const PreparedWork& work() const {return work_;}
  [[nodiscard]] const Q34WitnessSearchWork& rectangle_work() const {return rectangle_work_;}
  [[nodiscard]] const Q34WitnessBoundsWork& rectangle_bounds() const {return rectangle_bounds_;}
  [[nodiscard]] u64 retained_bytes() const {
    return sum(sizeof(*this),sum(product(rectangles_.capacity(),sizeof(Rectangle)),
        sum(product(segments_.capacity(),sizeof(Segment)),arena_?arena_->retained_bytes():0)));
  }
 private:
  Prepared(Q2CensusIndexPtr index,unsigned k):index_(std::move(index)),k_(k) {}
  void construct(const std::vector<WspdRectangle>& requests,unsigned workers) {
    if (!index_ || k_==0 || k_>10 || workers==0) throw std::invalid_argument("waves.configuration");
    const auto nodes=index_->spatial_nodes();
    if (index_->spatial_order().size()>std::numeric_limits<std::uint32_t>::max())
      throw std::overflow_error("waves.rank_u32");
    rectangles_.resize(requests.size());std::vector<ca::Request> planned;
    work_.input_rectangles=requests.size();
    for (std::size_t r=0;r!=requests.size();++r) {
      const auto& request=requests[r];
      if (request.a_node>=nodes.size() || request.b_node>=nodes.size() ||
          request.lane_mask==0 || (request.lane_mask&~6U)!=0) throw std::invalid_argument("waves.rectangle");
      const auto& an=nodes[request.a_node];const auto& bn=nodes[request.b_node];
      const auto a=an.range,b=bn.range;
      if (a.size()==0 || b.size()==0 || !(a.last<=b.first || b.last<=a.first))
        throw std::invalid_argument("waves.disjoint_factors");
      auto& meta=rectangles_[r];meta.raw_base=work_.raw_pairs;
      meta.a_first=ca::detail::u32(a.first);meta.b_first=ca::detail::u32(b.first);
      meta.a_size=ca::detail::u32(a.size());meta.b_size=ca::detail::u32(b.size());
      const auto mass=product(a.size(),b.size());counter_add(work_.raw_pairs,mass);
      meta.mask=filter_q34_witnesses(*index_,an.box,bn.box,static_cast<std::uint8_t>(k_),request.lane_mask,
          rectangle_work_,Q34WitnessBoundsMode::Affine,rectangle_bounds_);
      if (meta.mask==0) {counter_add(work_.closed);continue;}
      add(logical_,mass,meta.mask);
      const auto threshold=(meta.mask&4U)!=0?k_-2U:k_-1U;
      if (std::min(a.size(),b.size())<2 || sum(a.size(),b.size())-2<threshold) {
        counter_add(work_.fallbacks);add(effective_,mass,meta.mask);continue;
      }
      meta.arena_rectangle=planned.size();planned.push_back({request.a_node,request.b_node,r,meta.mask});
      counter_add(work_.planned);
    }
    if (!planned.empty()) {
      arena_=ca::Arena::build(index_,planned,k_,workers,128);
      const auto m=arena_->mass();counter_add(effective_.all,m.all);counter_add(effective_.q3,m.q3);counter_add(effective_.q4,m.q4);
    }
    if (effective_.all>logical_.all || effective_.q3>logical_.q3 || effective_.q4>logical_.q4)
      throw std::logic_error("waves.residual_mass");
    const u64 count=sum(work_.fallbacks,arena_?arena_->bands().size():0);
    segments_.reserve(ca::detail::checked_size(count));u64 end=0;
    for (std::size_t r=0;r!=rectangles_.size();++r) {
      const auto& meta=rectangles_[r];if (meta.mask==0) continue;
      if (meta.arena_rectangle==absent) {
        counter_add(end,product(meta.a_size,meta.b_size));segments_.push_back({r,absent,end});continue;
      }
      const auto& rm=arena_->rectangles()[meta.arena_rectangle];
      const auto& af=arena_->factors()[2*meta.arena_rectangle];
      for (u64 i=0;i!=rm.bands;++i) {
        const auto band_index=rm.band_first+i;const auto& band=arena_->bands()[band_index];
        const auto& ac=arena_->classes()[af.class_first+band.a_class];
        const auto size=product(ac.last-ac.first,band.b_last-band.b_first);
        if (size==0) throw std::logic_error("waves.empty_band");
        counter_add(end,size);segments_.push_back({r,band_index,end});
      }
    }
    if (segments_.size()!=count || end!=effective_.all) throw std::logic_error("waves.segment_mass");
  }
  Q2CensusIndexPtr index_;unsigned k_;
  std::vector<Rectangle> rectangles_;std::vector<Segment> segments_;
  std::shared_ptr<const ca::Arena> arena_;
  Mass logical_,effective_;PreparedWork work_;
  Q34WitnessSearchWork rectangle_work_;Q34WitnessBoundsWork rectangle_bounds_;
};

struct KeyedEdge {u64 ordinal{};Q34SurvivingEdge edge;};
struct Slot {u64 ordinal{};Q34SurvivingEdge edge;std::uint8_t input_mask{};};
struct Work {
  u64 decoded{},waves{},chunks{},band_wave_splits{},rectangle_crossings{},compacted{},sort_comparisons{};
  u64 pending_retries{},out_of_order{},slot_peak{},keyed_capacity_peak{},native_capacity{},array_bytes_peak{};
};
// This cursor is single-consumer. Prepared/index may be shared by independent
// cursors. A completed result is published only after ordering and conversion.
class Cursor final {
 public:
  Cursor(std::shared_ptr<const Prepared> prepared,std::size_t capacity,Mutant mutant=Mutant::None)
      :prepared_(std::move(prepared)),mutant_(mutant) {
    if (!prepared_ || capacity==0) throw std::invalid_argument("waves.cursor_configuration");
    const auto size=std::min<u64>(capacity,prepared_->effective().all);
    slots_.resize(ca::detail::checked_size(size));account();
  }
  Cursor(const Cursor&)=delete;Cursor& operator=(const Cursor&)=delete;
  Cursor(Cursor&&)=delete;Cursor& operator=(Cursor&&)=delete;
  [[nodiscard]] const Work& work() const {return work_;}
  [[nodiscard]] const Q34WitnessSearchWork& pair_work() const {return pair_work_;}
  [[nodiscard]] const Q34WitnessBoundsWork& pair_bounds() const {return pair_bounds_;}
  [[nodiscard]] u64 consumed() const {return consumed_;}
  [[nodiscard]] bool pending() const {return pending_;}
  // Only the reserve-failure seam is resumable. Other exceptions abort the
  // run and publish no completed result; no general checkpoint API is claimed.
  bool step(bool inject_reserve_failure=false) {
    if (finished_ || failed_) throw std::logic_error("waves.finished_or_failed");
    if (!pending_ && consumed_==prepared_->effective().all) return false;
    if (!pending_) {
      try {fill();} catch (...) {failed_=true;throw;}
    } else counter_add(work_.pending_retries);
    u64 live=0;for (std::size_t i=0;i!=used_;++i) if (slots_[i].edge.mask!=0) counter_add(live);
    const auto needed=sum(keyed_.size(),live);
    if (inject_reserve_failure) throw std::bad_alloc();
    if (needed>keyed_.capacity()) {
      const auto old_capacity=keyed_.capacity();
      const auto doubled=keyed_.capacity()>std::numeric_limits<u64>::max()/2?needed:2*u64(keyed_.capacity());
      const auto wanted=std::min(prepared_->effective().all,std::max(needed,doubled));
      keyed_.reserve(ca::detail::checked_size(wanted));
      // Both vector allocations coexist during relocation; include that
      // transient array capacity, not merely the post-reserve capacity.
      account(old_capacity);
    }
    account(); // reserve may have thrown; pending slots and cursor remain.
    for (std::size_t i=0;i!=used_;++i) if (slots_[i].edge.mask!=0) {
      keyed_.push_back({slots_[i].ordinal,slots_[i].edge});counter_add(work_.compacted);
    }
    consumed_=pending_end_;pending_=false;counter_add(work_.waves);return true;
  }
  Q34FilterBatch finish() {
    if (finished_ || failed_ || pending_ || consumed_!=prepared_->effective().all) throw std::logic_error("waves.not_exhausted");
    try {return finish_validated();} catch (...) {failed_=true;throw;}
  }
 private:
  Q34FilterBatch finish_validated() {
    for (std::size_t i=1;i<keyed_.size();++i) if (keyed_[i-1].ordinal>=keyed_[i].ordinal) counter_add(work_.out_of_order);
    std::sort(keyed_.begin(),keyed_.end(),[&](const KeyedEdge& a,const KeyedEdge& b) {
      counter_add(work_.sort_comparisons);return a.ordinal<b.ordinal;
    });
    for (std::size_t i=1;i<keyed_.size();++i)
      if (keyed_[i-1].ordinal>=keyed_[i].ordinal) throw std::logic_error("waves.duplicate_ordinal");
    Q34FilterBatch output;output.backend="audit_collective_CPU_waves";
    output.rectangle_masks.reserve(prepared_->rectangles().size());
    for (const auto& r:prepared_->rectangles()) output.rectangle_masks.push_back(r.mask);
    output.survivors.reserve(keyed_.size());
    for (const auto& edge:keyed_) output.survivors.push_back(edge.edge);
    output.expanded_pairs=prepared_->logical().all;
    output.pair_q3_rejected=sum(prepared_->logical().q3-prepared_->effective().q3,pair_work_.q3_rejected);
    output.pair_q4_rejected=sum(prepared_->logical().q4-prepared_->effective().q4,pair_work_.q4_rejected);
    output.rectangle_visits=prepared_->rectangle_work().node_visits;output.pair_visits=pair_work_.node_visits;
    work_.native_capacity=sum(output.rectangle_masks.capacity(),product(output.survivors.capacity(),sizeof(Q34SurvivingEdge)));
    account();
    if (pair_work_.queries!=prepared_->effective().all || pair_work_.q3_queries!=prepared_->effective().q3 ||
        pair_work_.q4_queries!=prepared_->effective().q4) throw std::logic_error("waves.physical_queries");
    finished_=true;return output;
  }
  void account(u64 old_keyed_capacity=0) {
    work_.slot_peak=std::max(work_.slot_peak,static_cast<u64>(slots_.capacity()));
    work_.keyed_capacity_peak=std::max(work_.keyed_capacity_peak,static_cast<u64>(keyed_.capacity()));
    work_.array_bytes_peak=std::max(work_.array_bytes_peak,
        sum(product(slots_.capacity(),sizeof(Slot)),sum(product(sum(keyed_.capacity(),old_keyed_capacity),sizeof(KeyedEdge)),work_.native_capacity)));
  }
  void fill() {
    used_=0;auto position=consumed_;const auto segments=prepared_->segments();
    const auto order=prepared_->index().spatial_order();const auto points=prepared_->index().cloud().points();
    while (used_!=slots_.size() && position!=prepared_->effective().all) {
      while (segment_!=segments.size() && position==segments[segment_].end) ++segment_;
      if (segment_==segments.size()) throw std::logic_error("waves.premature_eof");
      const auto& segment=segments[segment_];const auto& r=prepared_->rectangles()[segment.rectangle];
      const auto start=segment_==0?0:segments[segment_-1].end;
      const auto count=std::min<u64>(slots_.size()-used_,segment.end-position);
      counter_add(work_.chunks);
      if (last_rectangle_!=absent && last_rectangle_!=segment.rectangle) counter_add(work_.rectangle_crossings);
      last_rectangle_=segment.rectangle;
      for (u64 offset=position-start;offset!=position-start+count;++offset) {
        auto& slot=slots_[used_++];u64 a=0,b=0;unsigned mask=r.mask;
        if (segment.band==absent) {
          const auto ranks=fallback_ranks(r,offset);a=ranks[0];b=ranks[1];
          if (mutant_==Mutant::SkipFallback) mask=0;
        } else {
          const auto* arena=prepared_->arena();const auto& band=arena->bands()[segment.band];
          const auto& af=arena->factors()[2*r.arena_rectangle];const auto& bf=arena->factors()[2*r.arena_rectangle+1];
          const auto& ac=arena->classes()[af.class_first+band.a_class];const auto width=band.b_last-band.b_first;
          const auto ag=ac.first+offset/width,bg=band.b_first+offset%width;
          a=af.original_first+arena->ranks()[af.first+ag];b=bf.original_first+arena->ranks()[bf.first+bg];
          mask=arena->pair_mask(r.arena_rectangle,band.a_class,bg);
          if (mutant_==Mutant::Mask6) mask=r.mask;
        }
        slot.ordinal=mutant_==Mutant::GroupOrdinal?sum(r.raw_base,offset):
            original_ordinal(r,a,b);
        slot.edge={ca::detail::u32(a),ca::detail::u32(b),0};slot.input_mask=static_cast<std::uint8_t>(mask);
        slot.edge.mask=filter_q34_witnesses(prepared_->index(),singleton_box(points[order[a]]),singleton_box(points[order[b]]),
            static_cast<std::uint8_t>(prepared_->k()),slot.input_mask,pair_work_,Q34WitnessBoundsMode::Affine,pair_bounds_);
        counter_add(work_.decoded);
      }
      counter_add(position,count);
      if (position<segment.end && segment.band!=absent) counter_add(work_.band_wave_splits);
    }
    pending_end_=position;pending_=true;
  }
  std::shared_ptr<const Prepared> prepared_;Mutant mutant_;
  std::vector<Slot> slots_;std::vector<KeyedEdge> keyed_;
  std::size_t used_{},segment_{};u64 consumed_{},pending_end_{},last_rectangle_{absent};
  bool pending_{},finished_{},failed_{};Work work_;
  Q34WitnessSearchWork pair_work_;Q34WitnessBoundsWork pair_bounds_;
};
}  // namespace mhgp9::audit::waves
