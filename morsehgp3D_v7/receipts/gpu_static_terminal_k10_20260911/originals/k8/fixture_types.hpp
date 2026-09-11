#pragma once
#include "../terminal.cuh"

namespace mhgp7::terminal_cuda_gate {
namespace terminal = gpu_terminal_private;
struct CloudFixture {
  u32 point_begin, point_count, ball_begin, ball_count, request_begin, request_count;
};
struct PinnedRequest {
  terminal::Request request;
  terminal::Result result;
  u64 trace_begin, trace_count;
};
}  // namespace mhgp7::terminal_cuda_gate
