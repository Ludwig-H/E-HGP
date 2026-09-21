// Independent traversal experiment. Compile this TU with -fno-access-control
// and link normally (NOT --whole-archive) against the pinned product32 archive.
// This byte-identical product source supplies its real atlas and leaf sweep;
// no header/layout rewrite, mirrored atlas or benchmark main is included.
#include "snapshot/q4_local.cpp"

#include <bit>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>

namespace audit_join {
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
using AtlasState = Q4LocalAtlas::Impl::State;

void require(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}
u64 number(std::string_view text) {
  u64 value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  require(!text.empty() && error == std::errc{} && end == text.data() + text.size(), "invalid unsigned integer");
  return value;
}
double milliseconds(Clock::time_point first, Clock::time_point last) {
  return std::chrono::duration<double, std::milli>(last - first).count();
}
void hash_word(u64& hash, u64 value) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= value & 255U;
    hash *= 1099511628211ULL;  // Intentional modulo 2^64 FNV-1a.
    value >>= 8;
  }
}
std::string decimal(i128 value) {
  if (value == 0) return "0";
  const bool negative = value < 0;
  if (!negative) value = -value;  // Keep MIN representable too.
  std::string result;
  while (value != 0) {
    result.push_back(static_cast<char>('0' - static_cast<int>(value % 10)));
    value /= 10;
  }
  if (negative) result.push_back('-');
  std::reverse(result.begin(), result.end());
  return result;
}
template<class Range> void numbers(const Range& values) {
  std::cout << '[';
  bool first = true;
  for (const auto value : values) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << value;
  }
  std::cout << ']';
}
template<std::size_t Count, class T> void raw_words(const T& value) {
  static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
  static_assert(sizeof(T) == Count * sizeof(u64));
  numbers(std::bit_cast<std::array<u64, Count>>(value));
}
static_assert(sizeof(Q4LocalPartitionWork) == 20 * sizeof(u64));
static_assert(sizeof(Q4LocalGeometryQueryWork) == 2 * sizeof(u64));
static_assert(sizeof(Q4PositiveDomainWork) == 12 * sizeof(u64));

struct Record {
  unsigned arity{};
  std::array<std::size_t, 4> support{};
  std::array<i128, 5> coefficients{};
  std::size_t depth{};
  std::vector<std::size_t> shell;
  bool operator==(const Record&) const = default;
  bool operator<(const Record& other) const {
    return std::tie(arity, support, coefficients, depth, shell) <
           std::tie(other.arity, other.support, other.coefficients, other.depth, other.shell);
  }
};
Record copy_record(const Q34SeedCandidate& candidate, std::size_t n, std::size_t k) {
  require(candidate.arity == 4 && candidate.depth < k - 2, "invalid emitted arity/depth");
  Record result{candidate.arity, candidate.support_ids, candidate.ball.coefficients(), candidate.depth, {}};
  for (std::size_t i = 0; i < 4; ++i)
    require(result.support[i] < n && (i == 0 || result.support[i - 1] < result.support[i]), "invalid support IDs");
  for (const auto part : {candidate.shell_first, candidate.shell_second})
    require(std::is_sorted(part.begin(), part.end()), "unsorted shell part");
  std::merge(candidate.shell_first.begin(), candidate.shell_first.end(),
             candidate.shell_second.begin(), candidate.shell_second.end(), std::back_inserter(result.shell));
  for (std::size_t i = 0; i < result.shell.size(); ++i)
    require(result.shell[i] < n && (i == 0 || result.shell[i - 1] < result.shell[i]), "duplicate/invalid shell ID");
  for (const auto id : result.support)
    require(std::binary_search(result.shell.begin(), result.shell.end(), id), "support absent from shell");
  return result;
}
void dump_record(const Record& record) {
  std::cout << "{\"arity\":4,\"support\":";
  numbers(record.support);
  std::cout << ",\"coefficients\":[";
  for (std::size_t i = 0; i < 5; ++i) {
    if (i != 0) std::cout << ',';
    std::cout << '"' << decimal(record.coefficients[i]) << '"';
  }
  std::cout << "],\"depth\":" << record.depth << ",\"shell\":";
  numbers(record.shell);
  std::cout << '}';
}

// Same strict acute/longest-edge filters as the existing edge entry. A node
// may be tested again in another cell: rejected_site_incidents is therefore
// work over products, NOT a distinct population. passed_nodes means that the
// necessary box test passed; only the baseline immediately descends each one.
struct GeneratorWork {
  u64 node_visits{}, bound_tests{}, point_tests{}, rejected_nodes{}, passed_nodes{};
  u64 rejected_site_incidents{}, acute_seeds{}, owner_tests{}, owner_rejections{}, seeds{};
};
struct ExtraWork {
  u64 whole_atlas_skips{}, alive_skipped_nodes{}, product_visits{};
  u64 positive_products{}, negative_products{}, uncertain_products{}, zero_bound_products{};
  u64 block_bound_tests{}, singleton_bound_tests{}, x_splits{}, cell_splits{}, terminal_pairs{};
  u64 antichain_node_visits{}, antichain_splits{}, blocks{}, block_sites{}, block_max_sites{};
  u64 cache_entries_initialized{}, cache_bytes_peak{}, cache_hits{}, seed_cache_misses{};
  u64 family_cache_hits{}, family_preparations{}, form_preparations{};
  u64 peak_product_stack{}, product_stack_bytes{};
};
struct ModeResult {
  GeneratorWork generator;
  ExtraWork extra;
  Q4LocalSweepWork sweep;
  std::vector<Record> records;
  u64 shell_ids{};
  double elapsed_ms{}, normalize_ms{};
};

struct Context {
  Q4LocalAtlasPtr atlas;
  const Q2CensusIndex& index;
  std::span<const Q2SpatialNode> nodes;
  std::span<const std::size_t> order;
  std::span<const Point3> points;
  std::array<std::size_t, 2> edge;
  Point3 a, b;
  i64 diameter;
  explicit Context(Q4LocalAtlasPtr parent)
      : atlas(std::move(parent)), index(*atlas->geometry()->cover()->index()),
        nodes(index.spatial_nodes()), order(index.spatial_order()), points(index.cloud().points()),
        edge(atlas->geometry()->cover()->edge_ids()), a(points[edge[0]]), b(points[edge[1]]),
        diameter(mhgp8::distance(a, b)) {}

  bool spatial_pass(std::size_t node_id, GeneratorWork& work) const {
    const auto& node = nodes[node_id];
    require(node.range.size() > 1, "box seed test requires an internal node");
    counter_add(work.node_visits); counter_add(work.bound_tests);
    i64 min_a = 0, min_b = 0, max_sum = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 al = static_cast<i64>(node.box.low[axis]) - a[axis];
      const i64 ah = static_cast<i64>(node.box.high[axis]) - a[axis];
      const i64 bl = static_cast<i64>(node.box.low[axis]) - b[axis];
      const i64 bh = static_cast<i64>(node.box.high[axis]) - b[axis];
      const i64 na = al > 0 ? al : ah < 0 ? ah : 0;
      const i64 nb = bl > 0 ? bl : bh < 0 ? bh : 0;
      min_a += na * na; min_b += nb * nb;
      max_sum += std::max(al * al + bl * bl, ah * ah + bh * bh);
    }
    if (min_a > diameter || min_b > diameter || max_sum <= diameter) {
      counter_add(work.rejected_nodes);
      counter_add(work.rejected_site_incidents, static_cast<u64>(node.range.size()));
      return false;
    }
    counter_add(work.passed_nodes);
    return true;
  }
  bool seed_pass(std::size_t id, GeneratorWork& work) const {
    counter_add(work.node_visits); counter_add(work.point_tests);
    if (!mhgp8::acute(a, b, points[id])) return false;
    counter_add(work.acute_seeds);
    const std::array<std::size_t, 3> support{edge[0], edge[1], id};
    if (!mhgp8::owned(points, support, work.owner_tests)) {
      counter_add(work.owner_rejections);
      return false;
    }
    counter_add(work.seeds);
    return true;
  }
};

std::vector<u64> live_leaf_counts(const Q4LocalAtlas& atlas) {
  const auto& nodes = atlas.impl_->nodes;
  std::vector<u64> live(nodes.size());
  // Atlas storage is NOT a preorder. All children still have larger IDs.
  for (std::size_t i = nodes.size(); i != 0;) {
    --i;
    if (nodes[i].state == AtlasState::Leaf) live[i] = 1;
    else if (nodes[i].state == AtlasState::Branch)
      for (std::size_t q = 0; q < 4; ++q) {
        require(nodes[i].children + q > i, "atlas is not parent-before-child");
        counter_add(live[i], live.at(nodes[i].children + q));
      }
  }
  return live;
}
void visit_alive(Q4LocalEngine& engine, std::span<const u64> live, std::size_t cell_id,
                 std::size_t x, const Q4FamilySeed& family, Q4LocalForm line,
                 const Q34SeedConsumer& sink, ExtraWork& extra) {
  const auto& parent = *engine.atlas->impl_;
  const auto& node = parent.nodes[cell_id];
  counter_add(engine.work.query_visits);
  if (live[cell_id] == 0) { counter_add(extra.alive_skipped_nodes); return; }
  counter_add(engine.work.line_tests);
  const auto bound = parent.geometry->bounds(line, node.cell);
  if (bound.minimum > 0 || bound.maximum < 0) { counter_add(engine.work.line_skips); return; }
  if (node.state == AtlasState::Leaf) engine.sweep(*node.fragment, x, family, line, sink);
  else {
    require(node.state == AtlasState::Branch, "live atlas node has no leaf or children");
    for (std::size_t q = 0; q < 4; ++q)
      visit_alive(engine, live, node.children + q, x, family, line, sink, extra);
  }
}
void normalize(ModeResult& result) {
  const auto start = Clock::now();
  std::sort(result.records.begin(), result.records.end());  // Preserve multiplicity.
  for (const auto& record : result.records) counter_add(result.shell_ids, static_cast<u64>(record.shell.size()));
  require(result.sweep.emitted == result.records.size() && result.sweep.shell_ids == result.shell_ids,
          "sweep/output ledger mismatch");
  result.normalize_ms = milliseconds(start, Clock::now());
}
ModeResult scalar_mode(const Context& context, std::span<const u64> live, bool alive_only) {
  ModeResult result;
  const auto start = Clock::now();
  {
    Q4LocalEngine engine(context.atlas);  // Reuse event/shell buffers for the whole edge.
    const Q34SeedConsumer sink = [&](const auto& candidate) {
      result.records.push_back(copy_record(candidate, context.points.size(), context.atlas->kmax()));
    };
    if (alive_only && live[0] == 0) counter_add(result.extra.whole_atlas_skips);
    else {
      std::size_t cursor = 0;
      while (cursor < context.nodes.size()) {
        const auto& node = context.nodes[cursor];
        if (node.range.size() == 1) {
          const auto x = context.order[node.range.first];
          if (context.seed_pass(x, result.generator)) {
            counter_add(result.extra.family_preparations);
            if (!alive_only) {
              engine.seed(x, sink);  // Unmodified reference entry on this actual atlas.
              counter_add(result.extra.form_preparations);
            } else {
              // Match seed() exactly before changing only atlas navigation.
              const auto family = Q4FamilySeed::make(context.a, context.b, context.points[x]);
              require(family.has_value(), "acute seed has no family");
              counter_add(engine.work.seed_queries);
              const std::array<std::size_t, 3> ids{context.edge[0], context.edge[1], x};
              if (!mhgp8::owned(context.points, ids, engine.work.seed_owner_tests))
                counter_add(engine.work.seed_owner_rejections);
              else {
                const auto line = context.atlas->geometry()->form(x);
                counter_add(result.extra.form_preparations);
                visit_alive(engine, live, 0, x, *family, line, sink, result.extra);
              }
            }
          }
          cursor = node.escape;
        } else if (!context.spatial_pass(cursor, result.generator)) cursor = node.escape;
        else cursor = node.left;
      }
    }
    result.sweep = engine.work;
  }
  result.elapsed_ms = milliseconds(start, Clock::now());
  normalize(result);
  return result;
}

struct SeedCache {
  enum class State : unsigned char { Unknown, Invalid, Valid };
  State state{State::Unknown};
  Q4LocalForm line{};
  std::optional<Q4FamilySeed> family;
};
struct JoinFrame { std::size_t x{}, cell{}; bool spatial_test_paid{}; };
constexpr std::size_t join_stack_capacity = 1 + 48 + 3 * 44;

ModeResult joined_mode(const Context& context, std::span<const u64> live, std::size_t grain) {
  ModeResult result;
  const auto start = Clock::now();
  {
    Q4LocalEngine engine(context.atlas);
    const Q34SeedConsumer sink = [&](const auto& candidate) {
      result.records.push_back(copy_record(candidate, context.points.size(), context.atlas->kmax()));
    };
    const auto& cells = context.atlas->impl_->nodes;
    const auto& geometry = *context.atlas->geometry();
    std::vector<SeedCache> cache;
    std::array<JoinFrame, join_stack_capacity> stack{};
    result.extra.product_stack_bytes = sizeof(stack);
    std::size_t size = 0;
    const auto push = [&](JoinFrame frame) {
      require(size < stack.size(), "joined traversal exceeds proven X/C depth bound");
      stack[size++] = frame;
      result.extra.peak_product_stack = std::max(result.extra.peak_product_stack, static_cast<u64>(size));
    };
    if (live[0] == 0) counter_add(result.extra.whole_atlas_skips);
    else {
      // Stream maximal surviving spatial blocks <= grain. No antichain list
      // and no global per-point cache. A block is finished before the next;
      // its rank interval is disjoint from every other block in this edge.
      std::size_t block_cursor = 0;
      while (block_cursor < context.nodes.size()) {
        const auto& block = context.nodes[block_cursor];
        counter_add(result.extra.antichain_node_visits);
        if (block.range.size() > 1 && !context.spatial_pass(block_cursor, result.generator)) {
          block_cursor = block.escape;
          continue;
        }
        if (block.range.size() > grain) {
          counter_add(result.extra.antichain_splits);
          block_cursor = block.left;
          continue;
        }
        counter_add(result.extra.blocks);
        counter_add(result.extra.block_sites, static_cast<u64>(block.range.size()));
        result.extra.block_max_sites = std::max(result.extra.block_max_sites, static_cast<u64>(block.range.size()));
        cache.clear();
        cache.resize(block.range.size());
        counter_add(result.extra.cache_entries_initialized, static_cast<u64>(cache.size()));
        result.extra.cache_bytes_peak = std::max(result.extra.cache_bytes_peak,
            mhgp8::capacity_bytes(cache.capacity(), sizeof(SeedCache)));
        require(cache.size() <= grain, "cache exceeds the block grain");
        push({block_cursor, 0, block.range.size() > 1});
        while (size != 0) {
          auto frame = stack[--size];
          counter_add(result.extra.product_visits);
          const auto& xnode = context.nodes[frame.x];
          const auto& cell = cells[frame.cell];
          if (live[frame.cell] == 0) { counter_add(result.extra.alive_skipped_nodes); continue; }
          const bool singleton = xnode.range.size() == 1;
          SeedCache* seed = nullptr;
          std::size_t x = 0;
          Q4LocalBounds bound{};
          if (singleton) {
            require(xnode.range.first >= block.range.first && xnode.range.last <= block.range.last,
                    "singleton escaped its owning cache block");
            seed = &cache[xnode.range.first - block.range.first];
            x = context.order[xnode.range.first];
            if (seed->state == SeedCache::State::Unknown) {
              counter_add(result.extra.seed_cache_misses);
              if (!context.seed_pass(x, result.generator)) seed->state = SeedCache::State::Invalid;
              else {
                seed->state = SeedCache::State::Valid;
                seed->line = geometry.form(x);
                counter_add(result.extra.form_preparations);
              }
            } else counter_add(result.extra.cache_hits);
            if (seed->state == SeedCache::State::Invalid) continue;
            counter_add(result.extra.singleton_bound_tests);
            bound = geometry.bounds(seed->line, cell.cell);
          } else {
            if (!frame.spatial_test_paid && !context.spatial_pass(frame.x, result.generator)) continue;
            frame.spatial_test_paid = true;
            counter_add(result.extra.block_bound_tests);
            bound = geometry.node_bounds(frame.x, cell.cell);
          }
          if (bound.minimum > 0) { counter_add(result.extra.positive_products); continue; }
          if (bound.maximum < 0) { counter_add(result.extra.negative_products); continue; }
          counter_add(result.extra.uncertain_products);
          if (bound.minimum == 0 || bound.maximum == 0) counter_add(result.extra.zero_bound_products);
          if (singleton && cell.state == AtlasState::Leaf) {
            counter_add(result.extra.terminal_pairs);
            if (!seed->family) {
              const auto prepared = Q4FamilySeed::make(context.a, context.b, context.points[x]);
              require(prepared.has_value(), "cached acute owner seed lost its family");
              seed->family.emplace(*prepared);
              counter_add(result.extra.family_preparations);
              counter_add(engine.work.seed_queries);
            } else counter_add(result.extra.family_cache_hits);
            require(static_cast<bool>(cell.fragment), "live leaf has no exact fragment");
            engine.sweep(*cell.fragment, x, *seed->family, seed->line, sink);
          } else {
            require(cell.state == AtlasState::Leaf || cell.state == AtlasState::Branch,
                    "live cell has neither fragment nor children");
            // Balance by potential population cardinality, not geometric
            // box width: split X if |X| >= live leaf count under C. A tie
            // chooses X. The singleton/leaf cases force the other factor.
            const bool split_x = !singleton &&
                (cell.state == AtlasState::Leaf || xnode.range.size() >= live[frame.cell]);
            if (split_x) {
              counter_add(result.extra.x_splits);
              push({xnode.right, frame.cell, false});
              push({xnode.left, frame.cell, false});
            } else {
              counter_add(result.extra.cell_splits);
              for (std::size_t q = 4; q != 0; --q)
                push({frame.x, cell.children + q - 1, frame.spatial_test_paid});
            }
          }
        }
        block_cursor = block.escape;
      }
    }
    require(result.extra.family_preparations <= result.generator.seeds &&
            result.extra.family_preparations <= result.extra.seed_cache_misses,
            "a family was prepared repeatedly within disjoint cache blocks");
    result.sweep = engine.work;
  }
  result.elapsed_ms = milliseconds(start, Clock::now());
  normalize(result);
  return result;
}

void dump_generator(const GeneratorWork& value) {
  static_assert(sizeof(GeneratorWork) == 10 * sizeof(u64));
  std::cout << '{';
#define F(field) std::cout << "\"" #field "\":" << value.field
  F(node_visits); std::cout << ','; F(bound_tests); std::cout << ','; F(point_tests); std::cout << ',';
  F(rejected_nodes); std::cout << ','; F(passed_nodes); std::cout << ','; F(rejected_site_incidents); std::cout << ',';
  F(acute_seeds); std::cout << ','; F(owner_tests); std::cout << ','; F(owner_rejections); std::cout << ','; F(seeds);
#undef F
  std::cout << '}';
}
void dump_extra(const ExtraWork& value) {
  static_assert(sizeof(ExtraWork) == 26 * sizeof(u64));
  std::cout << '{';
#define F(field) std::cout << "\"" #field "\":" << value.field
  F(whole_atlas_skips); std::cout << ','; F(alive_skipped_nodes); std::cout << ','; F(product_visits); std::cout << ',';
  F(positive_products); std::cout << ','; F(negative_products); std::cout << ',';
  F(uncertain_products); std::cout << ','; F(zero_bound_products); std::cout << ',';
  F(block_bound_tests); std::cout << ','; F(singleton_bound_tests); std::cout << ',';
  F(x_splits); std::cout << ','; F(cell_splits); std::cout << ','; F(terminal_pairs); std::cout << ',';
  F(antichain_node_visits); std::cout << ','; F(antichain_splits); std::cout << ',';
  F(blocks); std::cout << ','; F(block_sites); std::cout << ','; F(block_max_sites); std::cout << ',';
  F(cache_entries_initialized); std::cout << ','; F(cache_bytes_peak); std::cout << ',';
  F(cache_hits); std::cout << ','; F(seed_cache_misses); std::cout << ',';
  F(family_cache_hits); std::cout << ','; F(family_preparations); std::cout << ','; F(form_preparations); std::cout << ',';
  F(peak_product_stack); std::cout << ','; F(product_stack_bytes);
#undef F
  std::cout << '}';
}
void dump_sweep(const Q4LocalSweepWork& value) {
  static_assert(sizeof(Q4LocalSweepWork) == 41 * sizeof(u64));
  std::cout << '{';
#define F(field) std::cout << "\"" #field "\":" << value.field
  F(seed_queries); std::cout << ','; F(seed_owner_tests); std::cout << ','; F(seed_owner_rejections); std::cout << ',';
  F(query_visits); std::cout << ','; F(line_tests); std::cout << ','; F(line_skips); std::cout << ','; F(leaf_queries); std::cout << ',';
  F(reference_points); std::cout << ','; F(reference_side_tests); std::cout << ','; F(active_blocks); std::cout << ','; F(active_sites); std::cout << ',';
  F(root_locations); std::cout << ','; F(clipped_events); std::cout << ','; F(clipped_inside); std::cout << ','; F(kept_events); std::cout << ',';
  F(constant_inside); std::cout << ','; F(constant_outside); std::cout << ','; F(constant_shell_ids); std::cout << ',';
  F(entries); std::cout << ','; F(exits); std::cout << ','; F(sort_comparisons); std::cout << ',';
  F(shell_sort_comparisons); std::cout << ','; F(group_comparisons); std::cout << ','; F(groups); std::cout << ',';
  F(boundary_skips); std::cout << ','; F(boundary_skipped_ids); std::cout << ',';
  F(depth_rejections); std::cout << ','; F(depth_skipped_ids); std::cout << ','; F(presentations); std::cout << ',';
  F(owner_tests); std::cout << ','; F(owner_rejections); std::cout << ','; F(positive_tests); std::cout << ',';
  F(positive_rejections); std::cout << ','; F(canonical_tests); std::cout << ','; F(canonical_rejections); std::cout << ',';
  F(emitted); std::cout << ','; F(shell_ids); std::cout << ','; F(groups_without_support); std::cout << ',';
  F(unexamined_after_emit); std::cout << ','; F(max_group); std::cout << ','; F(peak_buffer_bytes);
#undef F
  std::cout << '}';
}
void dump_mode(const ModeResult& result) {
  std::cout << "{\"elapsed_ms\":" << result.elapsed_ms << ",\"normalize_ms\":" << result.normalize_ms
            << ",\"output_count\":" << result.records.size() << ",\"shell_ids\":" << result.shell_ids
            << ",\"matches_baseline\":true,\"generator\":";
  dump_generator(result.generator);
  std::cout << ",\"sweep\":"; dump_sweep(result.sweep);
  std::cout << ",\"extra\":"; dump_extra(result.extra);
  std::cout << '}';
}

int run(int argc, char** argv) {
  require(argc == 6, "usage: probe input.u16le K edge_a edge_b grain");
  const auto k = number(argv[2]), a = number(argv[3]), b = number(argv[4]), grain = number(argv[5]);
  require(k >= 3 && k <= 10 && a != b && grain > 0 && grain <= std::numeric_limits<std::size_t>::max(),
          "invalid K, edge or grain");
  const auto start = Clock::now();
  std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
  require(input.is_open(), "cannot open binary cloud");
  const auto length = input.tellg();
  require(length > 0 && length % 6 == 0, "expected nonempty 6-byte u16le sites");
  const auto n64 = static_cast<u64>(length / 6);
  require(n64 <= std::numeric_limits<std::size_t>::max() && a < n64 && b < n64, "edge ID outside cloud");
  const auto n = static_cast<std::size_t>(n64);
  input.seekg(0);
  std::vector<Point3> points;
  points.reserve(n);
  u64 input_hash = 14695981039346656037ULL;
  hash_word(input_hash, n64);
  for (std::size_t id = 0; id < n; ++id) {
    std::array<unsigned char, 6> bytes{};
    require(static_cast<bool>(input.read(reinterpret_cast<char*>(bytes.data()), 6)), "truncated binary cloud");
    std::array<std::uint16_t, 3> xyz{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      xyz[axis] = static_cast<std::uint16_t>(static_cast<unsigned>(bytes[2 * axis]) |
                                            (static_cast<unsigned>(bytes[2 * axis + 1]) << 8));
      hash_word(input_hash, xyz[axis]);
    }
    points.push_back({xyz[0], xyz[1], xyz[2]});
  }
  require(input.peek() == std::char_traits<char>::eof(), "binary cloud grew while reading");
  const auto loaded = Clock::now();
  const auto cloud = prepare_cloud(points);
  const auto prepared = Clock::now();
  const auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  const auto cover = Q34EdgeCover::make(index, {static_cast<std::size_t>(a), static_cast<std::size_t>(b)});
  const auto covered = Clock::now();
  const Q4LocalOptions options{};
  const auto atlas = Q4LocalAtlas::make(cover, static_cast<std::size_t>(k), options);
  const auto mapped = Clock::now();
  const auto live = live_leaf_counts(*atlas);
  const auto annotated = Clock::now();
  const Context context(atlas);
  const auto baseline = scalar_mode(context, live, false);
  const auto alive = scalar_mode(context, live, true);
  const auto joined = joined_mode(context, live, static_cast<std::size_t>(grain));
  require(baseline.records == alive.records && baseline.records == joined.records,
          "full canonical support/key/depth/shell multiset mismatch");
  std::cout << std::fixed << std::setprecision(6)
      << "{\"schema\":\"audit_q4_seed_cell_join_probe_v1\",\"status\":\"completed\","
         "\"scope\":\"one_edge_real_atlas_shared_leaf_sweep_independent_traversals_not_global_or_FULL\","
         "\"split_policy\":\"larger_live_population_X_ties_first\","
         "\"cache_policy\":\"lazy_original_spatial_rank_in_disjoint_blocks_no_eviction\","
         "\"n\":" << n << ",\"kmax\":" << k << ",\"edge\":[" << a << ',' << b << "],\"grain\":" << grain
      << ",\"input_fnv1a_u64_le\":" << input_hash << ",\"preparation\":{\"read_ms\":" << milliseconds(start, loaded)
      << ",\"cloud_ms\":" << milliseconds(loaded, prepared) << ",\"index_ms\":" << milliseconds(prepared, indexed)
      << ",\"cover_ms\":" << milliseconds(indexed, covered) << ",\"atlas_ms\":" << milliseconds(covered, mapped)
      << ",\"alive_ms\":" << milliseconds(mapped, annotated) << ",\"cover_words\":";
  raw_words<10>(cover->work());
  std::cout << ",\"geometry_words\":"; raw_words<27>(atlas->geometry()->work());
  std::cout << ",\"atlas_words\":"; raw_words<38>(atlas->work());
  std::cout << ",\"atlas_retained_bytes\":" << atlas->retained_bytes()
      << ",\"live_nodes_bytes\":" << mhgp8::capacity_bytes(live.capacity(), sizeof(u64))
      << ",\"live_leaves\":" << live[0] << "},\"baseline\":";
  dump_mode(baseline);
  std::cout << ",\"alive\":"; dump_mode(alive);
  std::cout << ",\"join\":"; dump_mode(joined);
  std::cout << ",\"records\":[";
  for (std::size_t i = 0; i < baseline.records.size(); ++i) {
    if (i != 0) std::cout << ',';
    dump_record(baseline.records[i]);
  }
  std::cout << "]}\n";
  require(static_cast<bool>(std::cout), "JSON output failed");
  return 0;
}
}  // namespace audit_join

int main(int argc, char** argv) {
  try { return audit_join::run(argc, argv); }
  catch (const std::exception& error) {
    std::cerr << "q4 seed/cell audit: " << error.what() << '\n';
    return 1;
  }
}
