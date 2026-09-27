#pragma once
#include "cuda_api.hpp"
#include "../b_q34_arena_waves_20260927/waves.hpp"
#include "gpu/flat_index.hpp"
#include <chrono>
#include <type_traits>

namespace mhgp9::audit::cuda_waves {
namespace wa=waves;
inline void require(bool ok,const char* why) {if (!ok) throw std::runtime_error(why);}
static_assert(std::is_trivially_copyable_v<Rectangle> && std::is_trivially_copyable_v<Segment> &&
  std::is_trivially_copyable_v<Factor> && std::is_trivially_copyable_v<Class> &&
  std::is_trivially_copyable_v<Band> && std::is_trivially_copyable_v<Edge>);

class Snapshot final {
 public:
  static std::shared_ptr<const Snapshot> build(std::shared_ptr<const wa::Prepared> prepared) {
    require(bool(prepared),"cuda_waves.null_prepared");
    auto out=std::shared_ptr<Snapshot>(new Snapshot(std::move(prepared)));out->construct();return out;
  }
  Snapshot(const Snapshot&)=delete;Snapshot& operator=(const Snapshot&)=delete;
  Snapshot(Snapshot&&)=delete;Snapshot& operator=(Snapshot&&)=delete;
  const wa::Prepared& prepared() const {return *prepared_;}
  View view() const {
    return {rectangles_.data(),segments_.data(),factors_.data(),classes_.data(),bands_.data(),
      ranks_.data(),credits_.data(),nodes_.data(),points_.data(),rectangles_.size(),segments_.size(),
      factors_.size(),classes_.size(),bands_.size(),ranks_.size(),credits_.size(),nodes_.size(),
      points_.size()/3,prepared_->effective().all,prepared_->k()};
  }
  u64 array_bytes() const {
    u64 result=0;
    const auto account=[&](const auto& v) {
      using T=typename std::decay_t<decltype(v)>::value_type;
      result=wa::sum(result,wa::product(v.capacity(),sizeof(T)));
    };
    account(rectangles_);account(segments_);account(factors_);account(classes_);account(bands_);
    account(ranks_);account(credits_);account(nodes_);account(points_);return result;
  }
 private:
  explicit Snapshot(std::shared_ptr<const wa::Prepared> p):prepared_(std::move(p)) {}
  void construct() {
    const auto& index=prepared_->index();nodes_=gpu::flatten_nodes(index);
    const auto order=index.spatial_order();const auto points=index.cloud().points();
    require(order.size()<gpu::absent32 && order.size()<=points_.max_size()/3,"cuda_waves.point_count");
    points_.resize(3*order.size());
    for (size_t i=0;i<order.size();++i) {
      require(order[i]<points.size(),"cuda_waves.rank_id");
      for (size_t d=0;d<3;++d) {
        const auto x=points[order[i]][d];require(x>=0 && x<=262143,"cuda_waves.coordinate_u18");
        points_[3*i+d]=x;
      }
    }
    // Owned private copies, not adopted mutable caller buffers. Prepared's
    // factories certify ranges/boxes/masks and retain the immutable owner.
    rectangles_.reserve(prepared_->rectangles().size());segments_.reserve(prepared_->segments().size());
    for (const auto& r:prepared_->rectangles()) rectangles_.push_back({r.raw_base,r.arena_rectangle,r.a_first,r.b_first,r.a_size,r.b_size,r.mask});
    for (const auto& s:prepared_->segments()) segments_.push_back({s.rectangle,s.band,s.end});
    if (const auto* arena=prepared_->arena()) {
      factors_.reserve(arena->factors().size());classes_.reserve(arena->classes().size());bands_.reserve(arena->bands().size());
      for (const auto& f:arena->factors()) factors_.push_back({f.original_first,f.first,f.class_first,f.size,f.classes});
      for (const auto& c:arena->classes()) classes_.push_back({c.first,c.last,c.credit});
      for (const auto& b:arena->bands()) bands_.push_back({b.a_class,b.b_first,b.b_last});
      ranks_.assign(arena->ranks().begin(),arena->ranks().end());
      credits_.assign(arena->credits().begin(),arena->credits().end());
    }
    u64 previous=0;
    for (const auto& s:segments_) {require(s.end>previous && s.rectangle<rectangles_.size(),"cuda_waves.segment_shape");previous=s.end;}
    require(previous==prepared_->effective().all,"cuda_waves.segment_mass");
  }
  std::shared_ptr<const wa::Prepared> prepared_;
  std::vector<Rectangle> rectangles_;std::vector<Segment> segments_;
  std::vector<Factor> factors_;std::vector<Class> classes_;std::vector<Band> bands_;
  std::vector<u32> ranks_;std::vector<u8> credits_;
  std::vector<gpu::FlatNode> nodes_;std::vector<std::int32_t> points_;
};

inline gen::Q34FilterBatch finish(const Snapshot& s,std::vector<Edge>& edges,const Counters& c) {
  const auto& p=s.prepared();require(c.queries==p.effective().all && c.q3==p.effective().q3 && c.q4==p.effective().q4,"cuda_waves.physical_counts");
  std::sort(edges.begin(),edges.end(),[](const Edge& a,const Edge& b){return a.ordinal<b.ordinal;});
  for (size_t i=1;i<edges.size();++i) require(edges[i-1].ordinal<edges[i].ordinal,"cuda_waves.duplicate_ordinal");
  gen::Q34FilterBatch out;out.backend="audit_cuda_waves";
  for (const auto& r:p.rectangles()) out.rectangle_masks.push_back(r.mask);
  out.survivors.reserve(edges.size());
  for (const auto& e:edges) out.survivors.push_back({e.a,e.b,e.mask});
  out.expanded_pairs=p.logical().all;
  out.pair_q3_rejected=wa::sum(p.logical().q3-p.effective().q3,c.rejected3);
  out.pair_q4_rejected=wa::sum(p.logical().q4-p.effective().q4,c.rejected4);
  out.rectangle_visits=p.rectangle_work().node_visits;out.pair_visits=c.visits;return out;
}
inline gen::Q34FilterBatch portable(const Snapshot& s,size_t q,Counters& c) {
  require(q>0,"cuda_waves.zero_capacity");c={};const auto v=s.view();std::vector<Edge> survivors;
  // q may exceed int: split into CUB-addressable waves, never truncate E.
  const u64 capacity=std::min<u64>(q,0x7fffffffU);
  for (u64 base=0;base<v.effective;) {
    const auto count=std::min(capacity,v.effective-base);
    for (u64 j=0;j<count;++j) {
      Edge e{};require(decode(v,base+j,e),"cuda_waves.decode");
      const auto input=e.mask;u64 visits=0;e.mask=point_filter(v,e,visits);
      require(e.mask!=gpu::stack_failure,"cuda_waves.stack_failure");
      ++c.queries;c.q3+=(input&2U)!=0;c.q4+=(input&4U)!=0;c.visits=wa::sum(c.visits,visits);
      c.rejected3+=(input&2U)!=0 && (e.mask&2U)==0;c.rejected4+=(input&4U)!=0 && (e.mask&4U)==0;
      if (e.mask!=0) {if (!survivors.empty() && survivors.back().ordinal>=e.ordinal) ++c.out_of_order;survivors.push_back(e);}
    }
    base+=count;++c.waves;
  }
  return finish(s,survivors,c);
}
} // namespace mhgp9::audit::cuda_waves
