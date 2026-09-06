// Serialization gate only. It neither relaxes regularity nor builds FULL.
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../bench/full_extra_shell_diagnostic.hpp"

namespace {
using namespace mhgp7;
namespace diagnostic = extra_shell_diagnostic;
struct CloseFile { void operator()(FILE* stream) const { std::fclose(stream); } };
using File = std::unique_ptr<FILE, CloseFile>;
size_t checks = 0;

void need(bool ok, const char* reason) {
  ++checks;
  if (!ok) throw std::runtime_error(reason);
}

std::string content(FILE* stream) {
  std::rewind(stream);
  std::string value;
  char buffer[512];
  size_t size = 0;
  while ((size = std::fread(buffer, 1, sizeof(buffer), stream)) != 0) value.append(buffer, size);
  need(!std::ferror(stream), "read temporary stream");
  return value;
}

void check(bool emit) {
  const std::vector<InputPoint> input{{101, {0, 0, 0}}, {17, {2, 0, 0}},
      {1009, {2, 2, 0}}, {3, {0, 2, 0}}, {80, {1, 1, 0}}};
  const CloudIndex ix = build_cloud_index(input);
  BallData ball{};
  ball.key = {1, {-2, -2, 0}, 0};
  ball.level = promote_level({2, 1});
  ball.arity = 2;
  for (i32 i = 0; i < ix.unique_count(); ++i) {
    if (ball.key.power(ix.upos[static_cast<size_t>(i)]) < 0) ball.interior_ids[ball.n_interior++] = i;
    else ball.shell_ids[ball.n_shell++] = i;
  }
  const diagnostic::Context context{5, 8, 5, 5,
      "0000000000000000000000000000000000000000000000000000000000000000"};
  need(ball.n_interior == 1 && ball.n_shell == 4, "nonempty square and interior");
  File stream(std::tmpfile());
  need(stream != nullptr, "temporary stream");
  need(diagnostic::write(stream.get(), ix, ball, 7, context), "positive write");
  const std::string raw = content(stream.get());
  need(raw.find("\"a\":\"1\",\"b\":[\"-2\",\"-2\",\"0\"],\"c\":\"0\"") != std::string::npos,
       "signed decimal key");
  need(raw.find("\"numerator_u64_le\":[2,0,0],\"denominator\":\"1\"") != std::string::npos,
       "exact level representation");
  need(raw.find("\"point_id\":1009,\"xyz\":[2,2,0]") != std::string::npos &&
       raw.find("\"point_id\":80,\"xyz\":[1,1,0]") != std::string::npos, "external identities not Morton ranks");
  need(raw.back() == '\n' && raw.find('\n') == raw.size() - 1, "one complete JSONL record");
  for (int mutation = 0; mutation < 7; ++mutation) {
    BallData bad = ball;
    diagnostic::Context changed = context;
    if (mutation == 0) bad.n_shell = 13;
    if (mutation == 1) bad.n_interior = 10;
    if (mutation == 2) bad.shell_ids[0] = -1;
    if (mutation == 3) bad.shell_ids[0] = bad.interior_ids[0];
    if (mutation == 4) bad.key.a = 0;
    if (mutation == 5) changed.n = 6;
    if (mutation == 6) changed.input_digest = "invalid\"digest";
    File rejected(std::tmpfile());
    need(rejected != nullptr && !diagnostic::write(rejected.get(), ix, bad, 7, changed), "invalid input rejected");
    need(content(rejected.get()).empty(), "rejection before any output");
  }
  File numbers(std::tmpfile());
  need(numbers != nullptr, "integer temporary");
  const i128 minimum = -static_cast<i128>((u128{1} << 127) - 1) - 1;
  diagnostic::decimal(numbers.get(), minimum);
  need(content(numbers.get()) == "\"-170141183460469231731687303715884105728\"", "signed i128 minimum");
  need(!diagnostic::write(nullptr, ix, ball, 7, context), "null output rejected");
  if (emit) std::fputs(raw.c_str(), stdout);
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--emit-square") != 0)) return 2;
  try {
    const bool emit = std::strcmp(argv[1], "--emit-square") == 0;
    check(emit);
    if (checks != 33) throw std::runtime_error("nonvacuity: expected 33 checks");
    if (!emit) std::printf("{\"schema\":\"mhgp7-extra-shell-diagnostic-gate-v1\",\"checks\":%zu,"
                          "\"status\":\"passed\",\"FULL_built\":false}\n", checks);
    return std::fflush(stdout) == 0 && !std::ferror(stdout) ? 0 : 1;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "diagnostic gate: %s\n", error.what());
    return 1;
  }
}
