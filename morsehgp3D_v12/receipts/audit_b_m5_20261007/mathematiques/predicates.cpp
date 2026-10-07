// Tiny exact probe of the unchanged MES-M5 source, host simulation only.
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include "mhgp12/traversal/bfs.hpp"

namespace b = mhgp12::traversal::bfs;

std::string decimal(b::i128 x) {
  if (x == 0) return "0";
  const bool negative = x < 0;
  b::u128 y = negative ? b::u128(-x) : b::u128(x);
  std::string out;
  while (y != 0) { out += char('0' + y % 10); y /= 10; }
  if (negative) out += '-';
  std::reverse(out.begin(), out.end());
  return out;
}

template <class D>
void predicate(const b::u32* x, const b::u32* y, const b::i64* lo, const b::i64* hi) {
  const auto key = b::reservoir_key<D>(x[0], x[1], x[2], lo, hi);
  const auto a = b::terms<D>(x[0], x[1], x[2], lo, hi);
  const auto c = b::terms<D>(y[0], y[1], y[2], lo, hi);
  std::cout << decimal(key) << ' ' << b::dominates(a, c) << '\n';
}

int main() {
  std::string command;
  while (std::cin >> command) {
    if (command == "P") {
      b::i64 lo[3], hi[3];
      b::u32 mn[3], mx[3], x[3], y[3];
      for (auto* p : {lo, hi}) for (int a = 0; a < 3; ++a) std::cin >> p[a];
      for (auto* p : {mn, mx, x, y}) for (int a = 0; a < 3; ++a) std::cin >> p[a];
      if (!std::cin) return 2;
      const auto s = b::parent_frame_bits(lo, hi, mn, mx);
      std::cout << s << ' ';
      if (s <= b::kNarrowBits) predicate<b::i64>(x, y, lo, hi);
      else predicate<b::i128>(x, y, lo, hi);
    } else if (command == "T") {
      b::u32 n, k;
      std::cin >> n >> k;
      if (n < 1 || n > 1024 || k < 1 || k > b::kMaxOrder) return 2;
      std::vector<b::u32> x(n), y(n), z(n), list(n);
      for (b::u32 i = 0; i < n; ++i) std::cin >> x[i] >> y[i] >> z[i];
      if (!std::cin) return 2;
      std::iota(list.begin(), list.end(), 0);
      b::Parent p{};
      const b::u32* columns[3] = {x.data(), y.data(), z.data()};
      b::u32 mn[3], mx[3];
      for (int a = 0; a < 3; ++a) {
        mn[a] = *std::min_element(columns[a], columns[a]+n);
        mx[a] = *std::max_element(columns[a], columns[a]+n);
        p.lo[a] = mn[a]; p.hi[a] = b::i64(mx[a])+1;
      }
      p.count = n; p.sides = 1; p.tasks = (n+255)/256;
      p.frame_bits = b::parent_frame_bits(p.lo, p.hi, mn, mx);
      std::vector<b::u32> chunks(p.tasks*b::kMaxRes), reservoir(b::kMaxRes);
      const b::u32 begin = 0;
      b::Level lv{};
      lv.x=x.data(); lv.y=y.data(); lv.z=z.data(); lv.list=list.data();
      lv.parents=&p; lv.task_begin=&begin; lv.n_parents=1; lv.n_children=1;
      lv.chunk_top=chunks.data(); lv.reservoir=reservoir.data(); lv.params.kmax=k;
      typename b::SelectKernel<b::kNone>::Shared selection;
      for (b::u32 t=0; t<p.tasks; ++t) b::SelectKernel<b::kNone>{lv}(t,selection);
      typename b::MergeKernel<b::kNone>::Shared merge;
      b::MergeKernel<b::kNone>{lv}(0,merge);
      const auto& out = p.tasks==1 ? chunks : reservoir;
      const auto cap=std::min(n,3*k);
      std::cout << cap;
      for (b::u32 i=0; i<cap; ++i) std::cout << ' ' << out[i];
      std::cout << '\n';
    } else return 2;
  }
  return std::cin.eof() ? 0 : 2;
}
