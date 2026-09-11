// New qualification of the composed terminal against the strengthened T2 judge.
#define MHGP7_PRIVATE_TERMINAL_STUB 1
#define MHGP7_MEB_KEY_FORBID_LEGACY_HOST_MATERIALIZE 1
#include "t2_gate.cpp"

int main(int argc, char** argv) {
  const int result = terminal_t2_reference_main(argc, argv);
  if (result) return result;
  namespace terminal = mhgp7::gpu_terminal_private;
  const std::string_view mode = argc == 2 ? argv[1] : "";
  if (mode == "--shell14" || mode == "--spatial12") {
    if (!terminal::proof.calls || !terminal::proof.traces ||
        terminal::proof.empty_trace_checks != terminal::proof.calls) {
      std::fprintf(stderr, "FAIL T2.terminal_not_exercised\n"); return 1;
    }
  }
  std::printf("{\"status\":\"passed\",\"scope\":\"new_T2_terminal_host_stub_capture\","
      "\"paired_terminals\":%llu,\"paired_trace_rows\":%llu,\"paired_key_words\":%llu,"
      "\"paired_raw_levels\":%llu,\"q2_trace_rows\":%llu,\"q3_trace_rows\":%llu,\"q4_trace_rows\":%llu,"
      "\"extra_shell_trace_rows\":%llu,\"strict_steps\":%llu,\"same_radius_steps\":%llu,"
      "\"zero_trace_capacity_checks\":%llu,\"device_executed\":false,\"gcp_used\":false}\n",
      static_cast<unsigned long long>(terminal::proof.calls.load()),
      static_cast<unsigned long long>(terminal::proof.traces.load()),
      static_cast<unsigned long long>(terminal::proof.key_words.load()),
      static_cast<unsigned long long>(terminal::proof.semantically_equal_levels.load()),
      static_cast<unsigned long long>(terminal::proof.q[2].load()),
      static_cast<unsigned long long>(terminal::proof.q[3].load()),
      static_cast<unsigned long long>(terminal::proof.q[4].load()),
      static_cast<unsigned long long>(terminal::proof.extra_shells.load()),
      static_cast<unsigned long long>(terminal::proof.strict_steps.load()),
      static_cast<unsigned long long>(terminal::proof.same_steps.load()),
      static_cast<unsigned long long>(terminal::proof.empty_trace_checks.load()));
  return 0;
}
