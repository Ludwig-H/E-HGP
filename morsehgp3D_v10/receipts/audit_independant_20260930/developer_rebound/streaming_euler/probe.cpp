#include "extrema.hpp"
#include <charconv>
#include <iostream>
#include <string>
#include <vector>

using audit_euler::Entry;
using audit_euler::Summary;
using audit_euler::U32;

namespace {
bool read_u32(U32& value) {
  std::string token;
  if (!(std::cin >> token)) return false;
  U32 candidate = 0;
  const auto result = std::from_chars(token.data(), token.data()+token.size(), candidate);
  if (result.ec != std::errc{} || result.ptr != token.data()+token.size()) return false;
  value = candidate;
  return true;
}
int reject(const char* message) {
  std::cerr << "reject:" << message << '\n';
  return 2;
}
void emit(const Summary& s) {
  if (!s.has) std::cout << "[0]";
  else std::cout << "[1," << s.left.node << ',' << s.right.node << ']';
}
}  // namespace

int main() {
  U32 cases = 0;
  if (!read_u32(cases) || cases > 10000) return reject("case_count");
  for (U32 index = 0; index < cases; ++index) {
    U32 n = 0, count = 0, workers = 0;
    if (!read_u32(n) || !read_u32(count) || !read_u32(workers)) return reject("header");
    // These are transport limits of this isolated bounded probe, not a search
    // quota or restriction on the mathematical reduction. N is never allocated.
    if (count > 10000 || workers == 0 || workers > 64) return reject("transport_limit");
    Summary whole;
    std::vector<Summary> parts(workers);
    for (U32 j = 0; j < count; ++j) {
      Entry value;
      U32 worker = 0;
      if (!read_u32(value.node) || !read_u32(value.tin) || !read_u32(value.tout) ||
          !read_u32(worker)) return reject("entry_parse");
      if (!audit_euler::valid(value,n) || worker >= workers) return reject("entry_domain");
      whole.push(value);
      parts[worker].push(value);
    }
    Summary forward, reverse, rotated;
    for (const auto& p : parts) forward.merge(p);
    for (auto i = parts.rbegin(); i != parts.rend(); ++i) reverse.merge(*i);
    for (U32 j = 0; j < workers; ++j) rotated.merge(parts[(j+index%workers)%workers]);
    auto balanced = parts;
    while (balanced.size() > 1) {
      std::vector<Summary> next;
      for (std::size_t j = 0; j < balanced.size(); j += 2) {
        Summary joined = balanced[j];
        if (j+1 < balanced.size()) joined.merge(balanced[j+1]);
        next.push_back(joined);
      }
      balanced = std::move(next);
    }
    // Merge is also idempotent, including the explicit empty state.
    Summary duplicate = forward;
    duplicate.merge(forward);
    std::cout << "{\"case\":" << index << ",\"states\":[";
    const Summary states[] = {whole,forward,reverse,rotated,balanced[0],duplicate};
    bool first = true;
    for (const auto& state : states) {
      if (!first) std::cout << ',';
      first = false;
      emit(state);
    }
    std::cout << "]}\n";
  }
  std::string trailing;
  if (std::cin >> trailing) return reject("trailing_token");
  return 0;
}
