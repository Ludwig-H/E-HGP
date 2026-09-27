// Explicit narrow output-only port of b_q34_filtered_resident_20260927 at af369c44.
#include "device.hpp"
#include <algorithm>
#include <chrono>
#include <stdexcept>
namespace mhgp9::audit::resident_survivors {
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void need(bool b,const char* why) {if(!b) throw std::runtime_error(why);}
u64 sum(u64 a,u64 b) {u64 out=0;need(cw::add(a,b,out),"resident.counter_overflow");return out;}
class Portable final:public Device {
 public:
  explicit Portable(IndexInput input):index_(input) {}
  RectPass rectangles(const RectQuery* queries,u32 count) override {
    need(!closed_,"resident.closed");RectPass out;out.masks.resize(count);const auto start=Clock::now();
    for(u32 i=0;i<count;++i) {u64 visits=0;const auto& q=queries[i];
      out.masks[i]=gpu::filter_boxes(index_.nodes,index_.nodes[q.a].box,index_.nodes[q.b].box,index_.k,q.mask,visits);
#ifdef MHGP9_RESIDENT_MUTANT_CLOSE
      if(out.masks[i]!=gpu::stack_failure) out.masks[i]=0;
#endif
#ifdef MHGP9_RESIDENT_FAULT_RECTANGLE
      out.masks[i]=gpu::stack_failure;
#endif
      need(out.masks[i]!=gpu::stack_failure,"resident.rectangle_stack_failure");out.visits=sum(out.visits,visits);}
    out.kernel_ms=ms(start);return out;
  }
  OrderedRun consume(cw::View v,size_t q,bool debug) override {
    need(!closed_ && !failed_ && q>0,"resident.closed_or_zero_q");OrderedRun out;out.available=true;out.device="portable_exact_predicates";
    try {
    v.nodes=index_.nodes;v.points=index_.points;v.nodes_count=index_.nodes_count;v.points_count=index_.points_count;v.k=index_.k;
    const auto start=Clock::now();const auto capacity=std::min<u64>(q,0x7fffffffU);out.timing.q_actual=std::min(capacity,v.effective);
    PortableCollector collector(std::max<u64>(out.timing.q_actual,1));std::vector<cw::Edge> compact;compact.reserve(count_size<cw::Edge>(out.timing.q_actual));
    for(u64 base=0;base<v.effective;) {
      const auto count=std::min(capacity,v.effective-base);compact.clear();
      for(u64 j=0;j<count;++j) {cw::Edge e{};need(cw::decode(v,base+j,e),"resident.decode");
        const auto input=e.mask;u64 visits=0;e.mask=cw::point_filter(v,e,visits);
#ifdef MHGP9_RESIDENT_FAULT_PAIR
        e.mask=gpu::stack_failure;
#endif
        need(e.mask!=gpu::stack_failure,"resident.pair_stack_failure");
        auto& c=out.counters;++c.queries;c.q3+=(input&2U)!=0;c.q4+=(input&4U)!=0;c.visits=sum(c.visits,visits);
        c.rejected3+=(input&2U)!=0 && (e.mask&2U)==0;c.rejected4+=(input&4U)!=0 && (e.mask&4U)==0;
        if(e.mask) compact.push_back(e);
      }
      const auto append_start=Clock::now();collector.append(compact.data(),compact.size());out.output_times.append_ms+=ms(append_start);
      base+=count;++out.counters.waves;
    }
    out.timing.waves_ms=ms(start);const auto order_start=Clock::now();collector.finish(out,debug);
    out.output_times.sort_ms=ms(order_start);out.timing.order_ms=out.output_times.sort_ms;
    out.memory.array_peak_bytes=plus(out.memory.array_peak_bytes,times(compact.capacity(),sizeof(cw::Edge)));
    const auto release_start=Clock::now();std::vector<cw::Edge>().swap(compact);out.timing.release_ms=ms(release_start);
    out.timing.total_ms=ms(start);return out;
    } catch(...) {failed_=true;throw;}
  }
  double close() override {closed_=true;return 0;}
 private:IndexInput index_;bool closed_{},failed_{};
};
}
Opened open_portable(IndexInput input) {Opened out;out.device=std::make_unique<Portable>(input);out.name="portable_exact_predicates";return out;}
}
