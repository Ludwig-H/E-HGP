// Audit-only TU: compile the sealed implementation exactly once, then exercise
// its true internal append/finish functions. No copied collector or product API.
#include "../b_q34_resident_survivors_20260927/device_cuda.cu"
#include <iostream>
#include <map>
#include <string_view>
#include <tuple>

namespace mhgp9::audit::resident_survivors::device_gate_audit {
void demand(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
struct Fixture {std::vector<Edge> edges;std::vector<u64> chunks;};
struct Coverage {
  u64 cases{},refused{},items{},empty{},single{},empty_waves{},high32{},high63{},
      boundary_inversions{},inversions{},growths{},growth_copy_bytes{},upload_bytes{},download_bytes{},peak_bytes{};
};
Edge edge(u64 key,u32 id,u8 mask) {return {key,id,static_cast<u32>(1000+3*id),mask};}
std::vector<Fixture> fixtures(u64 q) {
  std::vector<Fixture> out{{{}, {0,0,0}},{{edge(cw::absent-2,7,6)}, {0,1,0}}};
  Fixture high;
  const u64 keys[]{(u64{1}<<63)+3,0,cw::absent-2,(u64{1}<<32)-1,
    (u64{1}<<32)+2,u64{1}<<63,(u64{1}<<63)-1,u64{1}<<32,9};
  const u8 masks[]{2,4,6};
  for(u32 i=0;i<9;++i) high.edges.push_back(edge(keys[i],i+17,masks[i%3]));
  high.chunks.push_back(0);
  for(u64 n=0;n<high.edges.size();) {const auto take=std::min({q,u64{2},u64(high.edges.size())-n});
    high.chunks.push_back(take);high.chunks.push_back(0);n+=take;}
  out.push_back(std::move(high));
  Fixture sparse;
  for(u32 wave=0;wave<289;++wave) {
    const bool live=wave%3==0;sparse.chunks.push_back(live?1:0);
    if(live) sparse.edges.push_back(edge(cw::absent-2-u64(wave)*(u64{1}<<32),wave+71,masks[wave%3]));
  }
  out.push_back(std::move(sparse));return out;
}
using Expected=std::map<u64,std::tuple<u32,u32,u8>>;
Expected expected(const Fixture& fixture,u64 q,Coverage& c) {
  Expected result;u64 consumed=0;bool any=false;u64 previous=0;
  for(u64 chunk:fixture.chunks) {
    demand(chunk<=q && chunk<=fixture.edges.size()-consumed,"device_gate.fixture_chunks");
    c.empty_waves+=chunk==0;
    if(chunk && any) c.boundary_inversions+=previous>=fixture.edges[consumed].ordinal;
    for(u64 j=0;j<chunk;++j) {
      const auto& e=fixture.edges[consumed+j];
      demand(e.mask!=0 && (e.mask&~6U)==0,"device_gate.fixture_mask");
      demand(result.emplace(e.ordinal,std::make_tuple(e.a,e.b,e.mask)).second,"device_gate.fixture_duplicate");
      if(any) c.inversions+=previous>=e.ordinal;
      any=true;previous=e.ordinal;c.high32+=e.ordinal>0xffffffffULL;c.high63+=e.ordinal>=(u64{1}<<63);
    }
    consumed+=chunk;
  }
  demand(consumed==fixture.edges.size(),"device_gate.fixture_complete");
  c.items+=consumed;c.empty+=consumed==0;c.single+=consumed==1;++c.cases;return result;
}
void compare(const OrderedRun& output,const Expected& wanted,bool debug,u64 inversions) {
  demand(output.survivors.size()==wanted.size() && output.counters.out_of_order==inversions,"device_gate.result_count");
  demand(output.debug_keys.size()==(debug?wanted.size():0),"device_gate.debug_count");
  size_t i=0;
  for(const auto& item:wanted) {
    const auto& got=output.survivors[i];
    demand(std::make_tuple(got.a,got.b,got.mask)==item.second && got.reserved[0]==0 && got.reserved[1]==0 && got.reserved[2]==0,
      "device_gate.key_payload");
    if(debug) demand(output.debug_keys[i]==item.first,"device_gate.full_u64_order");
    ++i;
  }
}
void device_case(const Fixture& fixture,u64 q,bool debug,cudaDeviceProp prop,Coverage& c) {
  const auto before_inversions=c.inversions;const auto wanted=expected(fixture,q,c);const auto inversions=c.inversions-before_inversions;
  Ledger ledger(0);OrderedRun output;
  {
    DeviceBuffer<Edge> compact(&ledger),stored(&ledger);u64 size=0,position=0,copy_sum=0,growths=0;
    const auto maximum=*std::max_element(fixture.chunks.begin(),fixture.chunks.end());compact.allocate(count_size<Edge>(maximum));
    for(u64 count:fixture.chunks) {
      if(count) {CW_CUDA(cudaMemcpy(compact.p,fixture.edges.data()+position,count_size<Edge>(count)*sizeof(Edge),cudaMemcpyHostToDevice));
        c.upload_bytes=plus(c.upload_bytes,times(count,sizeof(Edge)));}
      const auto old_size=size,old_capacity=stored.count;
      append_survivors(stored,size,compact.p,count,q,output.memory,output.output_times);
      if(stored.count!=old_capacity) {copy_sum=plus(copy_sum,times(old_size,sizeof(Edge)));++growths;}
      demand(size==position+count && stored.count<=plus(times(size,2),q),"device_gate.capacity_bound");
      demand(ledger.live==plus(compact.bytes(),stored.bytes()),"device_gate.append_ownership");position+=count;
    }
    demand(position==fixture.edges.size(),"device_gate.exhaustion");
    const auto compact_bytes=compact.bytes(),edge_bytes=stored.bytes();compact.release();
    finish_survivors(stored,size,debug,prop,output);
    demand(ledger.live==0 && stored.count==0 && stored.p==nullptr,"device_gate.released_before_return");
    compare(output,wanted,debug,inversions);
    const auto& m=output.memory;
    demand(m.size==size && m.capacity*sizeof(Edge)==edge_bytes && m.growths==growths &&
      m.growth_copy_bytes==copy_sum && m.append_copy_bytes==times(size,sizeof(Edge)),"device_gate.copy_accounting");
    demand(m.host_payload_bytes>=times(size,sizeof(Payload)) && m.debug_key_bytes>=(debug?times(size,sizeof(u64)):0),"device_gate.host_capacity");
    const u64 expected_download=size?plus(24,plus(times(size,sizeof(Payload)),debug?times(size,sizeof(u64)):0)):0;
    demand(output.timing.download_bytes==expected_download,"device_gate.actual_download_bytes");
    const u64 accumulation=plus(compact_bytes,m.edge_peak_bytes);
    const u64 split=size?plus(edge_bytes,plus(times(size,40),24)):0;
    const u64 sort=size?plus(times(size,40),plus(24,m.sort_scratch_bytes)):0;
    demand(ledger.peak==std::max({accumulation,split,sort}),"device_gate.actual_allocation_peak");
    c.growths+=m.growths;c.growth_copy_bytes=plus(c.growth_copy_bytes,m.growth_copy_bytes);
    c.download_bytes=plus(c.download_bytes,output.timing.download_bytes);
  }
  demand(ledger.live==0,"device_gate.final_ownership");c.peak_bytes=std::max(c.peak_bytes,ledger.peak);
}
void device_refusal(u64 q,cudaDeviceProp prop,bool duplicate,Coverage& c) {
  Ledger ledger(0);OrderedRun output;output.survivors.push_back({77,99,6,{0,0,0}});output.debug_keys.push_back(123);
  output.counters.out_of_order=9;bool refused=false;
  {
    DeviceBuffer<Edge> compact(&ledger),stored(&ledger);compact.allocate(1);u64 size=0;
    const Edge input[]{edge(cw::absent-2,1,duplicate?2:1),edge(cw::absent-2,2,4)};
    for(unsigned i=0;i<(duplicate?2U:1U);++i) {
      CW_CUDA(cudaMemcpy(compact.p,input+i,sizeof(Edge),cudaMemcpyHostToDevice));
      append_survivors(stored,size,compact.p,1,q,output.memory,output.output_times);
      append_survivors(stored,size,compact.p,0,q,output.memory,output.output_times);
    }
    compact.release();
    try {finish_survivors(stored,size,true,prop,output);}
    catch(const std::runtime_error& error) {
      demand(std::string_view(error.what())==(duplicate?"survivors.duplicate_or_unsorted":"survivors.invalid_payload"),"device_gate.refusal_reason");
      refused=true;
    }
    demand(refused,"device_gate.refusal_missing");
    demand(output.survivors.size()==1 && output.survivors[0].a==77 && output.survivors[0].b==99 && output.survivors[0].mask==6 &&
      output.debug_keys==std::vector<u64>{123} && output.counters.out_of_order==9,"device_gate.no_partial_failure_output");
  }
  demand(ledger.live==0,"device_gate.failure_ownership");++c.refused;c.peak_bytes=std::max(c.peak_bytes,ledger.peak);
}
void run(bool cuda) {
  Coverage c;cudaDeviceProp prop{};
  if(cuda) {
    int devices=0;CW_CUDA(cudaGetDeviceCount(&devices));demand(devices>0,"device_gate.no_device");
    CW_CUDA(cudaSetDevice(0));CW_CUDA(cudaGetDeviceProperties(&prop,0));demand(prop.multiProcessorCount>0,"device_gate.no_multiprocessors");
    CW_CUDA(cudaFree(nullptr));
  }
  for(u64 q:{1,7,64}) {
    for(bool debug:{false,true}) for(const auto& fixture:fixtures(q)) {
      if(cuda) device_case(fixture,q,debug,prop,c);
      else {
        const auto inversions=c.inversions;const auto wanted=expected(fixture,q,c);
        PortableCollector portable(q);u64 position=0;
        for(auto count:fixture.chunks) {portable.append(count?fixture.edges.data()+position:nullptr,count);position+=count;}
        OrderedRun result;portable.finish(result,debug);compare(result,wanted,debug,c.inversions-inversions);
      }
    }
    if(cuda) {device_refusal(q,prop,true,c);device_refusal(q,prop,false,c);}
  }
  demand(c.cases==24 && c.empty==6 && c.single==6 && c.high32>0 && c.high63>0 && c.boundary_inversions>0 && c.empty_waves>0,
    "device_gate.nonvacuity");
  demand(!cuda || (c.refused==6 && c.growths>0 && c.growth_copy_bytes>0 && c.peak_bytes>0),"device_gate.device_nonvacuity");
  std::cout<<"{\"schema\":\"mhgp9_survivors_device_gate_v1\",\"status\":\"passed\",\"mode\":\""<<(cuda?"cuda":"host_fixture")
    <<"\",\"cuda_executed\":"<<(cuda?"true":"false")<<",\"cases\":"<<c.cases<<",\"refused\":"<<c.refused
    <<",\"items\":"<<c.items<<",\"empty\":"<<c.empty<<",\"single\":"<<c.single<<",\"empty_waves\":"<<c.empty_waves
    <<",\"high32\":"<<c.high32<<",\"high63\":"<<c.high63<<",\"boundary_inversions\":"<<c.boundary_inversions
    <<",\"inversions\":"<<c.inversions<<",\"growths\":"<<c.growths<<",\"growth_copy_bytes\":"<<c.growth_copy_bytes
    <<",\"upload_bytes\":"<<c.upload_bytes<<",\"download_bytes\":"<<c.download_bytes<<",\"peak_bytes\":"<<c.peak_bytes<<"}\n";
}
}
int main(int argc,char** argv) {
  try {
    const bool cuda=argc==2 && std::string_view(argv[1])=="--cuda";
    const bool host=argc==2 && std::string_view(argv[1])=="--host-gate";
    mhgp9::audit::resident_survivors::device_gate_audit::demand(cuda || host,"device_gate.usage");
    mhgp9::audit::resident_survivors::device_gate_audit::run(cuda);return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 2;}
}
