#pragma once
// Explicit port of waves::Prepared and cuda_waves::Snapshot/finish at
// 33c1d28d7. Crucial change: no CPU rectangle witness search in this path.
#include "device.hpp"
#include "../b_q34_arena_waves_20260927/waves.hpp"
#include "gpu/flat_index.hpp"
#include <chrono>
#include <type_traits>
namespace mhgp9::audit::resident {
namespace wa=waves;namespace ca=collective;namespace fp=factor_plan;
using namespace mhgp9::gen;
using Clock=std::chrono::steady_clock;
inline double milliseconds(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
inline void need(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
struct CompactRectangle {u64 source_ordinal,raw_base;u32 a_node,b_node;u8 mask;};
struct OpenWork {
  u64 R{},raw_pairs{},raw_q3{},raw_q4{},closed{},rectangle_visits{},rectangle_waves{},resident_bytes{},temporary_input_bytes{};
  u64 rectangle_upload_bytes{},rectangle_download_bytes{},rectangle_device_peak_bytes{};
  double input_ms{},index_copy_ms{},init_ms{},index_upload_ms{},rectangle_upload_ms{},rectangle_kernel_ms{},
    rectangle_download_ms{},rectangle_release_ms{},compaction_ms{},input_release_ms{},total_ms{};
};
enum class Backend {Portable,CUDA};
class Session;class Prepared;
class FilteredRectangles final {
 public:
  FilteredRectangles(const FilteredRectangles&)=delete;FilteredRectangles& operator=(const FilteredRectangles&)=delete;
  std::span<const u8> masks() const {return masks_;}
  std::span<const CompactRectangle> compact() const {return compact_;}
  wa::Mass logical() const {return logical_;}
  unsigned k() const {return k_;}
  u64 visits() const {return visits_;}
  u64 retained_bytes() const {return wa::sum(sizeof(*this),wa::sum(masks_.capacity(),wa::product(compact_.capacity(),sizeof(CompactRectangle))));}
  const Q2CensusIndex& index() const {return *index_;}
 private:
  struct Origin {};
  FilteredRectangles()=default;
  Q2CensusIndexPtr index_;unsigned k_{};std::shared_ptr<const Origin> origin_;
  std::vector<u8> masks_;std::vector<CompactRectangle> compact_;wa::Mass logical_;u64 visits_{};
  friend class Session;friend class Prepared;
};
using Decision=std::shared_ptr<const FilteredRectangles>;
class Prepared final {
 public:
  static std::shared_ptr<const Prepared> build(Decision decision,unsigned workers) {
    need(bool(decision) && workers>0,"resident.preparation_configuration");
    auto p=std::shared_ptr<Prepared>(new Prepared(std::move(decision)));p->construct(workers);return p;
  }
  Prepared(const Prepared&)=delete;Prepared& operator=(const Prepared&)=delete;
  const FilteredRectangles& decision() const {return *decision_;}
  wa::Mass effective() const {return effective_;}
  u64 planned() const {return planned_;}u64 fallbacks() const {return fallbacks_;}
  const fp::Work& geometry_work() const {return geometry_;}
  const ca::ExtraWork& arena_work() const {return arena_work_;}
  u64 arena_owned_peak_bound() const {return arena_owned_peak_bound_;}
  u64 before_arena_release_bytes() const {return before_arena_release_bytes_;}
  u64 retained_bytes() const {
    u64 out=sizeof(*this);
    const auto add=[&](const auto& v){using T=typename std::decay_t<decltype(v)>::value_type;out=wa::sum(out,wa::product(v.capacity(),sizeof(T)));};
    add(rectangles_);add(segments_);add(factors_);add(classes_);add(bands_);add(ranks_);add(credits_);
    return wa::sum(out,arena_?arena_->retained_bytes():0);
  }
 private:
  explicit Prepared(Decision d):decision_(std::move(d)) {}
  void construct(unsigned workers) {
    const auto nodes=decision_->index().spatial_nodes();std::vector<ca::Request> requests;
    rectangles_.reserve(decision_->compact().size());
    for(const auto& r:decision_->compact()) {
      const auto a=nodes[r.a_node].range,b=nodes[r.b_node].range;
      cw::Rectangle meta{r.raw_base,cw::absent,static_cast<u32>(a.first),static_cast<u32>(b.first),
        static_cast<u32>(a.size()),static_cast<u32>(b.size()),r.mask};
      const auto threshold=(r.mask&4U)!=0?decision_->k()-2U:decision_->k()-1U;
      if(std::min(a.size(),b.size())<2 || wa::sum(a.size(),b.size())-2<threshold) {
        ++fallbacks_;wa::add(effective_,wa::product(a.size(),b.size()),r.mask);
      } else {
        meta.arena=requests.size();requests.push_back({r.a_node,r.b_node,r.source_ordinal,r.mask});++planned_;
      }
      rectangles_.push_back(meta);
    }
    if(!requests.empty()) {
      arena_=ca::Arena::build(decision_->index_,requests,decision_->k(),workers,128);
      geometry_=arena_->geometry_work();arena_work_=arena_->work();arena_owned_peak_bound_=arena_->owned_bytes_peak_bound();
      const auto m=arena_->mass();effective_.all=wa::sum(effective_.all,m.all);
      effective_.q3=wa::sum(effective_.q3,m.q3);effective_.q4=wa::sum(effective_.q4,m.q4);
      factors_.reserve(arena_->factors().size());classes_.reserve(arena_->classes().size());bands_.reserve(arena_->bands().size());
      for(const auto& f:arena_->factors()) factors_.push_back({f.original_first,f.first,f.class_first,f.size,f.classes});
      for(const auto& c:arena_->classes()) classes_.push_back({c.first,c.last,c.credit});
      for(const auto& b:arena_->bands()) bands_.push_back({b.a_class,b.b_first,b.b_last});
      ranks_.assign(arena_->ranks().begin(),arena_->ranks().end());credits_.assign(arena_->credits().begin(),arena_->credits().end());
    }
    need(effective_.all<=decision_->logical().all && effective_.q3<=decision_->logical().q3 &&
         effective_.q4<=decision_->logical().q4,"resident.effective_mass");
    segments_.reserve(wa::sum(fallbacks_,bands_.size()));u64 end=0;
    for(size_t r=0;r<rectangles_.size();++r) {
      const auto& meta=rectangles_[r];
      if(meta.arena==cw::absent) {end=wa::sum(end,wa::product(meta.a_size,meta.b_size));segments_.push_back({r,cw::absent,end});continue;}
      const auto& rm=arena_->rectangles()[meta.arena];const auto& af=factors_[2*meta.arena];
      for(u64 j=0;j<rm.bands;++j) {
        const auto bi=rm.band_first+j;const auto& band=bands_[bi];const auto& ac=classes_[af.class_first+band.a_class];
        const auto mass=wa::product(ac.last-ac.first,band.b_last-band.b_first);need(mass!=0,"resident.empty_band");
        end=wa::sum(end,mass);segments_.push_back({r,bi,end});
      }
    }
    need(end==effective_.all,"resident.segment_mass");
    before_arena_release_bytes_=retained_bytes();arena_.reset();
  }
  cw::View view() const {
    return {rectangles_.data(),segments_.data(),factors_.data(),classes_.data(),bands_.data(),ranks_.data(),credits_.data(),
      nullptr,nullptr,rectangles_.size(),segments_.size(),factors_.size(),classes_.size(),bands_.size(),ranks_.size(),credits_.size(),
      0,0,effective_.all,decision_->k()};
  }
  Decision decision_;std::shared_ptr<const ca::Arena> arena_;wa::Mass effective_;u64 planned_{},fallbacks_{};
  fp::Work geometry_;ca::ExtraWork arena_work_;u64 arena_owned_peak_bound_{},before_arena_release_bytes_{};
  std::vector<cw::Rectangle> rectangles_;std::vector<cw::Segment> segments_;std::vector<cw::Factor> factors_;
  std::vector<cw::Class> classes_;std::vector<cw::Band> bands_;std::vector<u32> ranks_;std::vector<u8> credits_;
  friend class Session;
};
struct Run {Q34FilterBatch output;cw::Counters counters;cw::Timing timing;};
class Session final {
 public:
  static std::unique_ptr<Session> open(Q2CensusIndexPtr index,std::span<const WspdRectangle> requests,
      unsigned k,Backend backend,size_t qr) {
    auto s=std::unique_ptr<Session>(new Session);s->construct(std::move(index),requests,k,backend,qr);return s;
  }
  Session(const Session&)=delete;Session& operator=(const Session&)=delete;
  ~Session() {try {close();} catch(...) {}}
  Decision rectangles() const {return decision_;}
  const OpenWork& work() const {return work_;}
  const std::string& device_name() const {return opened_.name;}
  bool cuda() const {return opened_.cuda;}
  Run consume(const Prepared& p,size_t q) {
#ifdef MHGP9_RESIDENT_MUTANT_OWNER
    need(opened_.device && q>0,"resident.session_identity_or_closed");
#else
    need(opened_.device && decision_->origin_==p.decision_->origin_ && q>0,"resident.session_identity_or_closed");
#endif
    const auto start=Clock::now();auto device=opened_.device->consume(p.view(),q);
    need(device.available && device.error.empty(),device.error.empty()?"resident.backend_unavailable":device.error.c_str());
    Run out;out.counters=device.counters;out.timing=device.timing;
    const auto& c=out.counters;const auto e=p.effective();
    need(c.queries==e.all && c.q3==e.q3 && c.q4==e.q4,"resident.physical_counts");
    const auto order_start=Clock::now();auto& edges=device.survivors;
    for(size_t i=1;i<edges.size();++i) out.counters.out_of_order+=edges[i-1].ordinal>=edges[i].ordinal;
    std::sort(edges.begin(),edges.end(),[](const auto& a,const auto& b){return a.ordinal<b.ordinal;});
    for(size_t i=1;i<edges.size();++i) need(edges[i-1].ordinal<edges[i].ordinal,"resident.duplicate_ordinal");
    out.output.backend=cuda()?"audit_resident_CUDA":"audit_resident_portable";
    out.output.rectangle_masks.assign(decision_->masks().begin(),decision_->masks().end());
    out.output.survivors.reserve(edges.size());for(const auto& edge:edges) out.output.survivors.push_back({edge.a,edge.b,edge.mask});
    out.output.expanded_pairs=decision_->logical().all;
    out.output.pair_q3_rejected=wa::sum(decision_->logical().q3-e.q3,c.rejected3);
    out.output.pair_q4_rejected=wa::sum(decision_->logical().q4-e.q4,c.rejected4);
    out.output.rectangle_visits=decision_->visits();out.output.pair_visits=c.visits;
    out.timing.order_ms=milliseconds(order_start);const auto release_start=Clock::now();std::vector<cw::Edge>().swap(edges);
    out.timing.release_ms+=milliseconds(release_start);out.timing.total_ms=milliseconds(start);return out;
  }
  double close() {
    if(!opened_.device) return 0;
    const auto start=Clock::now();opened_.device->close();opened_.device.reset();
    std::vector<gpu::FlatNode>().swap(nodes_);std::vector<std::int32_t>().swap(points_);return milliseconds(start);
  }
 private:
  Session()=default;
  void construct(Q2CensusIndexPtr index,std::span<const WspdRectangle> input,unsigned k,Backend backend,size_t qr) {
    const auto start=Clock::now();need(index && k>=1 && k<=10 && qr>0 &&
      (backend==Backend::Portable || backend==Backend::CUDA),"resident.configuration");
    std::vector<WspdRectangle> requests(input.begin(),input.end());
    // Mutable aliases of the caller's requests never survive this copy.
    std::vector<u64> raw_begin;raw_begin.reserve(wa::sum(requests.size(),1));raw_begin.push_back(0);
    const auto nodes=index->spatial_nodes();
    for(const auto& r:requests) {
      need(r.a_node<nodes.size() && r.b_node<nodes.size() && r.lane_mask!=0 && (r.lane_mask&~6U)==0,"resident.rectangle");
      const auto a=nodes[r.a_node].range,b=nodes[r.b_node].range;
      need(a.size() && b.size() && (a.last<=b.first || b.last<=a.first),"resident.disjoint_factors");
      const auto mass=wa::product(a.size(),b.size());raw_begin.push_back(wa::sum(raw_begin.back(),mass));
      if(r.lane_mask&2U) work_.raw_q3=wa::sum(work_.raw_q3,mass);
      if(r.lane_mask&4U) work_.raw_q4=wa::sum(work_.raw_q4,mass);
    }
    work_.R=requests.size();work_.raw_pairs=raw_begin.back();work_.input_ms=milliseconds(start);
    auto decision=std::shared_ptr<FilteredRectangles>(new FilteredRectangles);
    decision->index_=std::move(index);decision->k_=k;decision->origin_=std::make_shared<FilteredRectangles::Origin>();
    const auto copy_start=Clock::now();nodes_=gpu::flatten_nodes(decision->index());
    const auto order=decision->index().spatial_order();const auto coordinates=decision->index().cloud().points();
    need(order.size()<=points_.max_size()/3,"resident.points_size");points_.resize(3*order.size());
    for(size_t r=0;r<order.size();++r) for(size_t d=0;d<3;++d) points_[3*r+d]=coordinates[order[r]][d];
    work_.index_copy_ms=milliseconds(copy_start);
    const IndexInput view{nodes_.data(),points_.data(),nodes_.size(),order.size(),k};
    opened_=backend==Backend::CUDA?open_cuda(view):open_portable(view);
    need(bool(opened_.device),"resident.no_backend");work_.init_ms=opened_.init_ms;work_.index_upload_ms=opened_.upload_ms;
    work_.resident_bytes=opened_.resident_bytes;decision->masks_.resize(requests.size());
    const auto capacity=std::min<u64>(qr,0x7fffffffU);std::vector<RectQuery> batch;
    batch.reserve(static_cast<size_t>(std::min<u64>(capacity,requests.size())));
    for(size_t base=0;base<requests.size();) {
      const auto count=static_cast<u32>(std::min<u64>(capacity,requests.size()-base));batch.clear();
      for(u32 j=0;j<count;++j) {const auto& r=requests[base+j];batch.push_back({static_cast<u32>(r.a_node),static_cast<u32>(r.b_node),r.lane_mask});}
      auto pass=opened_.device->rectangles(batch.data(),count);need(pass.masks.size()==count,"resident.rectangle_mask_count");
      std::copy(pass.masks.begin(),pass.masks.end(),decision->masks_.begin()+base);
      work_.rectangle_visits=wa::sum(work_.rectangle_visits,pass.visits);++work_.rectangle_waves;
      work_.rectangle_upload_ms+=pass.upload_ms;work_.rectangle_kernel_ms+=pass.kernel_ms;
      work_.rectangle_upload_bytes=wa::sum(work_.rectangle_upload_bytes,pass.upload_bytes);
      work_.rectangle_download_bytes=wa::sum(work_.rectangle_download_bytes,pass.download_bytes);
      work_.rectangle_device_peak_bytes=std::max(work_.rectangle_device_peak_bytes,pass.device_bytes);
      work_.rectangle_download_ms+=pass.download_ms;work_.rectangle_release_ms+=pass.release_ms;base+=count;
    }
    const auto compact_start=Clock::now();
    for(size_t r=0;r<requests.size();++r) {
      const auto& request=requests[r];const auto mask=decision->masks_[r];
      const unsigned available=k>=3?6U:k==2?2U:0U;
      need(mask!=gpu::stack_failure && (mask&~request.lane_mask)==0 && (mask&~available)==0,"resident.invalid_filtered_mask");
      if(mask==0) {++work_.closed;continue;}
      const auto mass=raw_begin[r+1]-raw_begin[r];wa::add(decision->logical_,mass,mask);
      u64 base=raw_begin[r];
#ifdef MHGP9_RESIDENT_MUTANT_RAW_BASE
      base=decision->compact_.size();
#endif
      decision->compact_.push_back({r,base,static_cast<u32>(request.a_node),static_cast<u32>(request.b_node),mask});
    }
    decision->visits_=work_.rectangle_visits;decision_=std::move(decision);work_.compaction_ms=milliseconds(compact_start);
    work_.temporary_input_bytes=wa::sum(wa::product(requests.capacity(),sizeof(WspdRectangle)),
      wa::sum(wa::product(raw_begin.capacity(),sizeof(u64)),wa::product(batch.capacity(),sizeof(RectQuery))));
    const auto release_start=Clock::now();std::vector<WspdRectangle>().swap(requests);std::vector<u64>().swap(raw_begin);std::vector<RectQuery>().swap(batch);
    work_.input_release_ms=milliseconds(release_start);work_.total_ms=milliseconds(start);
  }
  std::vector<gpu::FlatNode> nodes_;std::vector<std::int32_t> points_;Opened opened_;Decision decision_;OpenWork work_;
};
}
