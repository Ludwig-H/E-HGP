// Private terminal differential gate, NOT a production CUDA owner or GPU FULL.
#define MHGP7_MEB_KEY_FORBID_LEGACY_HOST_MATERIALIZE 1
#include "../terminal_bridge.hpp"
#include "../source/morsehgp3D_v7/src/gpu/anchor_meb_key_route.cuh"
#include "fixture_types.hpp"
#include <cstdio>
#include <string_view>

#if !defined(__CUDACC__) && !defined(MHGP7_FAKE_DEVICE)
#error Build the host gate explicitly with MHGP7_FAKE_DEVICE
#endif

namespace mhgp7::terminal_cuda_gate {
#include "terminal_fixtures.inc"
namespace transport = gpu_meb_key_route_private;

struct WireRequest { terminal::Request request; u64 trace_begin = 0, trace_capacity = 0; };
struct WireResult {
  terminal::Result result;
  u64 snapshot = 0, batch = 0, trace_written = 0;
  u32 trace_overflow = 0, written = 0;
};
static_assert(std::is_standard_layout_v<WireRequest> && std::is_trivially_copyable_v<WireRequest>);
static_assert(std::is_standard_layout_v<WireResult> && std::is_trivially_copyable_v<WireResult>);
static_assert(std::is_standard_layout_v<terminal::TraceRow> && std::is_trivially_copyable_v<terminal::TraceRow>);
static_assert(sizeof(void*) == 8 && std::endian::native == std::endian::little);
inline constexpr u32 kOmitLast = 1, kWrongTarget = 2, kWrongOrdinal = 4, kWrongWork = 8,
    kWrongKey = 16, kWrongSlot = 32, kWrongSnapshot = 64, kWrongBatch = 128;
inline constexpr size_t kAbiWords = 48;
MHGP7_HD inline void layout(u64* output) {
  const u64 values[kAbiWords]{1,sizeof(terminal::View),alignof(terminal::View),
      sizeof(terminal::spatial::IndexView),offsetof(terminal::spatial::IndexView,snapshot),
      offsetof(terminal::spatial::IndexView,nodes),sizeof(terminal::CatalogView),
      offsetof(terminal::CatalogView,snapshot),sizeof(terminal::CatalogEntry),
      offsetof(terminal::CatalogEntry,level),offsetof(terminal::CatalogEntry,arity),
      sizeof(terminal::Request),alignof(terminal::Request),offsetof(terminal::Request,ordinal),
      offsetof(terminal::Request,k),offsetof(terminal::Request,selected),offsetof(terminal::Request,before),
      sizeof(terminal::Result),alignof(terminal::Result),offsetof(terminal::Result,ordinal),
      offsetof(terminal::Result,target),offsetof(terminal::Result,work),
      sizeof(terminal::Work),offsetof(terminal::Work,supports),offsetof(terminal::Work,powers),
      offsetof(terminal::Work,stack_peak),sizeof(terminal::TraceRow),
      offsetof(terminal::TraceRow,selection),offsetof(terminal::TraceRow,key),
      offsetof(terminal::TraceRow,level),offsetof(terminal::TraceRow,intruder),
      sizeof(terminal::meb_selection::Selection),offsetof(terminal::meb_selection::Selection,calls),
      offsetof(terminal::meb_selection::Selection,supports),offsetof(terminal::meb_selection::Selection,powers),
      sizeof(WireRequest),alignof(WireRequest),offsetof(WireRequest,trace_begin),offsetof(WireRequest,trace_capacity),
      sizeof(WireResult),alignof(WireResult),offsetof(WireResult,snapshot),offsetof(WireResult,batch),
      offsetof(WireResult,trace_written),offsetof(WireResult,trace_overflow),offsetof(WireResult,written),
      sizeof(terminal::LevelWords),sizeof(terminal::meb_key::PrimitiveKeyWords)};
  for (size_t i = 0; i < kAbiWords; ++i) output[i] = values[i];
}
MHGP7_HD inline void process(const terminal::View& view, const WireRequest* input, u64 count,
    WireResult* output, terminal::TraceRow* rows, u64 row_count, u64 batch, u64 gid, u32 flags) {
  if (gid >= count || ((flags & kOmitLast) && gid + 1 == count)) return;
  const auto request = input[gid];
  WireResult result;
  result.snapshot = view.index.snapshot; result.batch = batch; result.written = 1;
  result.result.ordinal = request.request.ordinal;
  if (request.trace_begin <= row_count && request.trace_capacity <= row_count - request.trace_begin &&
      (!request.trace_capacity || rows)) {
    terminal::TraceBuffer trace{request.trace_capacity ? rows + request.trace_begin : nullptr,
        request.trace_capacity,0,false};
    result.result = terminal::resolve(view,request.request,&trace);
    result.trace_written = trace.written; result.trace_overflow = trace.overflow ? 1 : 0;
  }
  if (gid + 1 == count) {
    if (flags & kWrongTarget) result.result.target = terminal::kAbsentBall;
    if (flags & kWrongOrdinal) ++result.result.ordinal;
    if (flags & kWrongWork) ++result.result.work.powers;
    if (flags & kWrongSnapshot) ++result.snapshot;
    if (flags & kWrongBatch) ++result.batch;
    if (result.trace_written && request.trace_capacity) {
      if (flags & kWrongKey) rows[request.trace_begin].key.words[0] ^= 1;
      if (flags & kWrongSlot) rows[request.trace_begin].selection.slots[0] = 99;
    }
  }
  output[gid] = result;
}
#if defined(__CUDACC__)
__global__ void terminal_kernel(terminal::View view,const WireRequest* input,u64 count,WireResult* output,
    terminal::TraceRow* rows,u64 row_count,u64 batch,u32 flags) {
  process(view,input,count,output,rows,row_count,batch,
      static_cast<u64>(blockIdx.x)*blockDim.x+threadIdx.x,flags);
}
__global__ void terminal_abi_kernel(u64* output) { layout(output); }
#endif

void need(bool good,const char* why) { if (!good) throw std::runtime_error(why); }
u64 checks = 0, compared = 0, trace_rows = 0, q[5]{}, extra = 0, strict = 0, equal = 0;
u64 zero_trace = 0, rejections = 0, causal_mutations = 0, intruder_queries = 0;
u64 large_ordinals = 0;
void check(bool good,const char* why) { ++checks; need(good,why); }

// Gate-lifetime storage only, reusing the already qualified allocation/copy/
// release backend. This is not the future asynchronous production owner.
class Arena {
  transport::Backend& backend_;
  struct Allocation { void* pointer; size_t bytes; };
  std::vector<Allocation> allocations_;
  bool closed_ = false, closed_ok_ = true;
 public:
  explicit Arena(transport::Backend& backend) : backend_(backend) {}
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena() { (void)close(); }
  template <class T> T* copy(const T* source,size_t count) {
    if (!count) return nullptr;
    need(!closed_ && source && count <= std::numeric_limits<size_t>::max()/sizeof(T),"arena.copy_size");
    const size_t bytes = count*sizeof(T);
    allocations_.push_back({nullptr,bytes});
    allocations_.back().pointer = backend_.allocate(bytes);
    auto* result = static_cast<T*>(allocations_.back().pointer);
    backend_.upload(result,source,bytes); return result;
  }
  bool close() noexcept {
    if (closed_) return closed_ok_;
    closed_ = true;
    for (auto i = allocations_.rbegin(); i != allocations_.rend(); ++i)
      if (!backend_.release(i->pointer,i->bytes)) closed_ok_ = false;
    return closed_ok_ = closed_ok_ && backend_.cleanup_certified;
  }
};
void abi(transport::Backend& backend) {
  Arena arena(backend);
  std::array<u64,kAbiWords> expected{},observed{};
  layout(expected.data());
  auto* data = arena.copy(observed.data(),observed.size());
#if defined(__CUDACC__)
  terminal_abi_kernel<<<1,1>>>(data);
  transport::Backend::check(cudaGetLastError());
#else
  layout(data);
#endif
  ++backend.work.launches; backend.synchronize();
  backend.download(observed.data(),data,sizeof(observed));
  for (size_t i = 0; i < observed.size(); ++i) check(observed[i] == expected[i],"gate.terminal_abi");
  check(arena.close(),"gate.abi_cleanup");
}
terminal::View resident(Arena& arena,const terminal::View& host) {
  auto view = host;
  view.index.left = arena.copy(host.index.left,host.index.nodes);
  view.index.right = arena.copy(host.index.right,host.index.nodes);
  view.index.first = arena.copy(host.index.first,host.index.nodes);
  view.index.last = arena.copy(host.index.last,host.index.nodes);
  view.index.box6 = arena.copy(host.index.box6,6ull*host.index.nodes);
  view.index.positions3 = arena.copy(host.index.positions3,3ull*host.index.positions);
  view.catalog.entries = arena.copy(host.catalog.entries,host.catalog.count);
  view.catalog.by_key = arena.copy(host.catalog.by_key,host.catalog.count);
  return view;
}
struct Batch { std::vector<WireResult> output; std::vector<terminal::TraceRow> trace; };
Batch execute(transport::Backend& backend,const terminal::View& device,
    const std::vector<WireRequest>& input,u64 row_count,u64 batch,u32 flags = 0) {
  Batch result;
  if (input.empty()) { need(row_count == 0,"gate.empty_trace_count"); return result; }
  need(input.size() <= std::numeric_limits<unsigned>::max()*128ull,"gate.launch_dimension");
  Arena arena(backend);
  result.output.resize(input.size()); result.trace.resize(static_cast<size_t>(row_count));
  const auto* requests = arena.copy(input.data(),input.size());
  auto* output = arena.copy(result.output.data(),result.output.size());
  auto* trace = arena.copy(result.trace.data(),result.trace.size());
  backend.injection(transport::Fault::kLaunch);
#if defined(__CUDACC__)
  const auto blocks = static_cast<unsigned>((input.size()+127)/128);
  terminal_kernel<<<blocks,128>>>(device,requests,input.size(),output,trace,row_count,batch,flags);
  transport::Backend::check(cudaGetLastError());
#else
  for (u64 gid = 0; gid < input.size()+3; ++gid)
    process(device,requests,input.size(),output,trace,row_count,batch,gid,flags);
#endif
  ++backend.work.launches; backend.synchronize();
  backend.download(result.output.data(),output,result.output.size()*sizeof(WireResult));
  if (row_count) backend.download(result.trace.data(),trace,result.trace.size()*sizeof(terminal::TraceRow));
  need(arena.close(),"gate.batch_cleanup");
  return result;
}
bool same_work(const terminal::Work& a,const terminal::Work& b) {
  for (u8 i = 0; i < 5; ++i) if (a.supports[i] != b.supports[i]) return false;
  for (auto member : {&terminal::Work::calls,&terminal::Work::powers,&terminal::Work::materializations,
      &terminal::Work::level_materializations,&terminal::Work::key_lookups,&terminal::Work::anchor_hits,
      &terminal::Work::intruder_queries,&terminal::Work::intruder_nodes,&terminal::Work::intruder_power_tests,
      &terminal::Work::interior_ranges,&terminal::Work::same_radius_steps,&terminal::Work::descending_steps,
      &terminal::Work::max_chain_steps,&terminal::Work::axis_divisions})
    if (a.*member != b.*member) return false;
  return a.stack_peak == b.stack_peak;
}
void compare(const Batch& actual,const CloudFixture& cloud,const std::vector<WireRequest>& requests,
    u64 snapshot,u64 batch,bool no_trace,bool account) {
  check(actual.output.size() == requests.size(),"gate.whole_batch_count");
  for (size_t i = 0; i < requests.size(); ++i) {
    const auto& expected = fixture_requests[cloud.request_begin+i];
    const auto& got = actual.output[i];
    check(got.written == 1 && got.snapshot == snapshot && got.batch == batch,"gate.whole_batch_identity");
    check(got.result.status == expected.result.status && got.result.ordinal == expected.result.ordinal &&
        got.result.target == expected.result.target,"gate.terminal_target_ordinal");
    check(same_work(got.result.work,expected.result.work),"gate.exact_all_work");
    check(got.trace_written == expected.trace_count && got.trace_overflow == (no_trace ? 1u : 0u),
        "gate.trace_is_not_quota");
    if (no_trace) { if (account) ++zero_trace; continue; }
    for (u64 j = 0; j < expected.trace_count; ++j) {
      const auto& row = actual.trace[requests[i].trace_begin+j];
      const auto& pinned = fixture_traces[expected.trace_begin+j];
      check(terminal::same_trace(row,pinned),"gate.exact_complete_trace");
      if (account) { ++q[row.selection.q]; extra += row.selection.shell > row.selection.q; ++trace_rows; }
    }
    if (account) {
      ++compared; strict += got.result.work.descending_steps; equal += got.result.work.same_radius_steps;
      intruder_queries += got.result.work.intruder_queries;
      large_ordinals += got.result.ordinal > (u64{1} << 32);
    }
  }
}
void run(transport::Backend& backend) {
  abi(backend);
  const auto initial_launches = backend.work.launches;
  const auto empty = execute(backend,terminal::View{}, {},0,0);
  check(empty.output.empty() && empty.trace.empty() && backend.work.launches == initial_launches,
      "gate.empty_has_no_launch");
  u64 batch = 1;
  bool faults_done = false;
  for (const auto& cloud : fixture_clouds) {
    const auto index = build_cloud_index(std::vector<P3>(fixture_points+cloud.point_begin,
        fixture_points+cloud.point_begin+cloud.point_count));
    std::vector<BallData> balls;
    for (u32 i = 0; i < cloud.ball_count; ++i) {
      const auto& row = fixture_balls[cloud.ball_begin+i];
      BallData ball;
      ball.key = terminal::meb_key::decode_key(row.key); ball.level = terminal::decode_level(row.level);
      ball.arity = static_cast<u8>(row.arity); ball.n_interior = static_cast<u8>(row.interior);
      ball.n_shell = static_cast<u8>(row.shell); balls.push_back(ball);
    }
    terminal::HostOwner owner(index,balls);
    const auto host = owner.view();
    Arena arena(backend);
    const auto device = resident(arena,host);
    std::vector<WireRequest> requests;
    u64 row_count = 0;
    for (u32 i = 0; i < cloud.request_count; ++i) {
      const auto& pinned = fixture_requests[cloud.request_begin+i];
      auto request = pinned.request; request.snapshot = host.index.snapshot;
      requests.push_back({request,row_count,pinned.trace_count}); row_count += pinned.trace_count;
    }
    const auto actual = execute(backend,device,requests,row_count,batch);
    compare(actual,cloud,requests,host.index.snapshot,batch,false,true); ++batch;
    auto no_trace = requests;
    for (auto& request : no_trace) request.trace_begin = request.trace_capacity = 0;
    const auto without = execute(backend,device,no_trace,0,batch);
    compare(without,cloud,no_trace,host.index.snapshot,batch,true,true); ++batch;
    if (!faults_done) {
      for (u32 flags : {kOmitLast,kWrongTarget,kWrongOrdinal,kWrongWork,kWrongKey,kWrongSlot,kWrongSnapshot,kWrongBatch}) {
        const auto bad = execute(backend,device,requests,row_count,batch,flags);
        bool caught = false;
        try { compare(bad,cloud,requests,host.index.snapshot,batch,false,false); }
        catch (const std::runtime_error& error) {
          const auto why = std::string_view(error.what());
          const auto expected = flags == kOmitLast || flags == kWrongSnapshot || flags == kWrongBatch ?
              "gate.whole_batch_identity" : (flags == kWrongTarget || flags == kWrongOrdinal ?
              "gate.terminal_target_ordinal" : (flags == kWrongWork ? "gate.exact_all_work" : "gate.exact_complete_trace"));
          caught = why == expected;
        }
        check(caught,"gate.transport_mutation_causal"); ++causal_mutations; ++batch;
      }
      for (u32 mutation = 0; mutation < 12; ++mutation) {
        auto invalid = requests;
        auto changed = device;
        auto& last = invalid.back().request;
        switch (mutation) {
          case 0: last.snapshot = 0; break;
          case 1: last.k = 1; break;
          case 2: last.k = 11; break;
          case 3: last.selected[0] = -1; break;
          case 4: last.selected[1] = static_cast<i32>(cloud.point_count); break;
          case 5: last.selected[1] = last.selected[0]; break;
          case 6: std::swap(last.selected[0],last.selected[1]); break;
          case 7: last.before = {}; break;
          case 8: changed.catalog.snapshot = 0; break;
          case 9: changed.index.positions3 = nullptr; break;
          case 10: changed.index.nodes = 999; break;
          case 11: last.before = fixture_traces[fixture_requests[cloud.request_begin+cloud.request_count-1].trace_begin].level; break;
        }
        const auto bad = execute(backend,changed,invalid,row_count,batch);
        const auto& result = bad.output.back().result;
        const auto expected = mutation >= 8 && mutation <= 10 ? terminal::Status::kInvalidView :
            (mutation == 11 ? terminal::Status::kNotStrict : terminal::Status::kInvalidRequest);
        check(result.status == expected && result.target == terminal::kAbsentBall &&
            result.ordinal == last.ordinal && result.work.calls == (mutation == 11 ? 1u : 0u),
            "gate.invalid_input_exact_refusal");
        ++rejections; ++batch;
      }
      faults_done = true;
    }
    check(arena.close(),"gate.resident_cleanup");
  }
  check(compared == std::size(fixture_requests) && zero_trace == compared && q[2] && q[3] && q[4] &&
      strict && equal && extra && intruder_queries && large_ordinals == compared &&
      rejections == 12 && causal_mutations == 8,
      "gate.nonvacuity");
  check(backend.cleanup_certified && backend.free_failures == 0 &&
      backend.certified_free_bytes == backend.allocation_bytes,"gate.all_allocations_released");
}
}  // namespace mhgp7::terminal_cuda_gate

int main(int argc,char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  namespace gate = mhgp7::terminal_cuda_gate;
  try {
    gate::transport::Backend backend;
    try { gate::run(backend); }
    catch (...) {
      std::fprintf(stderr,"cleanup_certified=%s free_failures=%llu\n",backend.cleanup_certified ? "true" : "false",
          (unsigned long long)backend.free_failures); throw;
    }
#if defined(__CUDACC__)
    constexpr const char* name = "CUDA";
#else
    constexpr const char* name = "HOST_STUB";
#endif
    std::printf("{\"status\":\"passed\",\"scope\":\"terminal_geometry_only_c03\",\"backend\":\"%s\","
        "\"sm\":\"%d.%d\",\"checks\":%llu,\"compared\":%llu,\"trace_rows\":%llu,"
        "\"q2\":%llu,\"q3\":%llu,\"q4\":%llu,\"extra_shells\":%llu,"
        "\"strict_steps\":%llu,\"same_radius_steps\":%llu,\"intruder_queries\":%llu,"
        "\"zero_trace_complete_results\":%llu,\"large_ordinals\":%llu,\"rejections\":%llu,\"causal_transport_mutations\":%llu,"
        "\"launches\":%llu,\"h2d_bytes\":%llu,\"d2h_bytes\":%llu,\"allocation_bytes\":%llu,"
        "\"certified_free_bytes\":%llu,\"cleanup_certified\":true,\"failures\":0,"
        "\"infrastructure_authority\":\"external_controller\"}\n",
        name,backend.major,backend.minor,(unsigned long long)gate::checks,(unsigned long long)gate::compared,
        (unsigned long long)gate::trace_rows,(unsigned long long)gate::q[2],(unsigned long long)gate::q[3],
        (unsigned long long)gate::q[4],(unsigned long long)gate::extra,(unsigned long long)gate::strict,
        (unsigned long long)gate::equal,(unsigned long long)gate::intruder_queries,(unsigned long long)gate::zero_trace,
        (unsigned long long)gate::large_ordinals,(unsigned long long)gate::rejections,(unsigned long long)gate::causal_mutations,
        (unsigned long long)backend.work.launches,(unsigned long long)backend.work.h2d_bytes,
        (unsigned long long)backend.work.d2h_bytes,(unsigned long long)backend.allocation_bytes,
        (unsigned long long)backend.certified_free_bytes);
    return 0;
  } catch (const gate::transport::Error& error) {
    std::fprintf(stderr,"FAIL transport: %s\n",error.reason); return 1;
  } catch (const std::exception& error) { std::fprintf(stderr,"FAIL %s\n",error.what()); return 1; }
}
