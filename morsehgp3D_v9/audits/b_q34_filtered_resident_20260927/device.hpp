#pragma once
// Plain C++17 seam. Exact predicates/wire are reused unchanged from the
// qualified CUDA-waves source commit 33c1d28d7 (publication a7e80d7f9).
#include "../b_q34_cuda_waves_20260927/cuda_api.hpp"
#include <memory>
namespace mhgp9::audit::resident {
namespace cw=cuda_waves;
using cw::u64;using cw::u32;using cw::u8;
struct RectQuery {u32 a,b;u8 mask;};
struct IndexInput {const gpu::FlatNode* nodes{};const std::int32_t* points{};u64 nodes_count{},points_count{};unsigned k{};};
struct RectPass {std::vector<u8> masks;u64 visits{},upload_bytes{},download_bytes{},device_bytes{};double upload_ms{},kernel_ms{},download_ms{},release_ms{};};
class Device {
 public:
  virtual ~Device()=default;
  virtual RectPass rectangles(const RectQuery*,u32 count)=0;
  virtual cw::DeviceRun consume(cw::View,size_t q)=0;
  virtual double close()=0; // Our buffers only, never cudaDeviceReset.
};
struct Opened {std::unique_ptr<Device> device;std::string name;bool cuda{};double init_ms{},upload_ms{};u64 resident_bytes{};};
// Internal seam only: Session creates validated immutable input and does
// not accept user-constructed Device/Opened objects or a trusted-mask flag.
Opened open_portable(IndexInput);
Opened open_cuda(IndexInput);
}
