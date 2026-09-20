// Independent audit prototype. No product headers, implementation or qualification.
// Usage: center_blocks input.txt K depth node_budget domain(0|1)
// Input: n followed by n distinct u16 triples; the supplied edge is IDs 0,1.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
__extension__ using I = __int128;
__extension__ using U = unsigned __int128;
using I64 = std::int64_t;
using U64 = std::uint64_t;
using V = std::array<I64, 3>;
using Point = std::array<std::uint16_t, 3>;
using Clock = std::chrono::steady_clock;
constexpr auto absent = std::numeric_limits<std::size_t>::max();

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
void add(U64& value, U64 amount = 1) {
  if (amount > std::numeric_limits<U64>::max() - value)
    throw std::overflow_error("audit work counter overflow");
  value += amount;
}
U64 as_u64(std::size_t value) { return static_cast<U64>(value); }
double ms(Clock::time_point first, Clock::time_point last) {
  return std::chrono::duration<double, std::milli>(last - first).count();
}
I64 dot(const V& a, const V& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
V cross(const V& a, const V& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}
V difference(const Point& a, const Point& b) {
  return {I64{a[0]} - b[0], I64{a[1]} - b[1], I64{a[2]} - b[2]};
}
I64 magnitude(I64 value) { return value < 0 ? -value : value; }
std::string decimal(U value) {
  if (value == 0) return "0";
  std::string result;
  while (value != 0) {
    result.push_back(static_cast<char>('0' + static_cast<unsigned>(value % 10)));
    value /= 10;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

// SHA-256 of the exact input bytes. Padding uses two small blocks, not a
// second copy of the complete point-cloud input.
std::string sha256(std::string_view input) {
  constexpr std::array<std::uint32_t, 64> constants{
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
  std::array<std::uint32_t, 8> state{0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
                                  0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};
  const auto rotate = [](std::uint32_t x, unsigned n) { return (x >> n) | (x << (32U - n)); };
  const auto block = [&](const unsigned char* data) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t i = 0; i < 16; ++i)
      words[i] = (std::uint32_t{data[4*i]} << 24U) | (std::uint32_t{data[4*i+1]} << 16U) |
                 (std::uint32_t{data[4*i+2]} << 8U) | std::uint32_t{data[4*i+3]};
    for (std::size_t i = 16; i < 64; ++i) {
      const auto x = words[i-15], y = words[i-2];
      words[i] = words[i-16] + (rotate(x,7) ^ rotate(x,18) ^ (x >> 3U)) + words[i-7] +
                 (rotate(y,17) ^ rotate(y,19) ^ (y >> 10U));
    }
    auto a=state[0], b=state[1], c=state[2], d=state[3];
    auto e=state[4], f=state[5], g=state[6], h=state[7];
    for (std::size_t i = 0; i < 64; ++i) {
      const auto t1 = h + (rotate(e,6)^rotate(e,11)^rotate(e,25)) + ((e&f)^((~e)&g)) + constants[i] + words[i];
      const auto t2 = (rotate(a,2)^rotate(a,13)^rotate(a,22)) + ((a&b)^(a&c)^(b&c));
      h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
  };
  std::size_t position = 0;
  while (input.size() - position >= 64) {
    block(reinterpret_cast<const unsigned char*>(input.data() + position));
    position += 64;
  }
  std::array<unsigned char, 128> tail{};
  const auto remainder = input.size() - position;
  for (std::size_t i = 0; i < remainder; ++i) tail[i] = static_cast<unsigned char>(input[position+i]);
  tail[remainder] = 0x80U;
  const auto padded = remainder < 56 ? std::size_t{64} : std::size_t{128};
  require(input.size() <= std::numeric_limits<U64>::max()/8, "input is too large for SHA-256 byte length");
  U64 bits = as_u64(input.size()) * 8;
  for (std::size_t i = 0; i < 8; ++i) { tail[padded-1-i] = static_cast<unsigned char>(bits & 255U); bits >>= 8U; }
  block(tail.data());
  if (padded == 128) block(tail.data()+64);
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto word : state) out << std::setw(8) << word;
  return out.str();
}

U64 argument(std::string_view input) {
  U64 value = 0;
  const auto result = std::from_chars(input.data(), input.data()+input.size(), value);
  require(result.ec == std::errc{} && result.ptr == input.data()+input.size(), "invalid unsigned CLI argument");
  return value;
}
struct Input {
  std::vector<Point> points;
  std::string hash;
  U64 input_bytes{};
};
Input read_input(const char* path) {
  std::ifstream file(path, std::ios::binary);
  require(static_cast<bool>(file), "cannot open input");
  const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  require(!file.bad(), "input read failed");
  const char* cursor = text.data();
  const char* const end = cursor + text.size();
  const auto whitespace = [](char ch) { return ch==' ' || ch=='\t' || ch=='\n' || ch=='\r' || ch=='\f' || ch=='\v'; };
  const auto next = [&]() {
    while (cursor != end && whitespace(*cursor)) ++cursor;
    U64 value = 0;
    const auto result = std::from_chars(cursor, end, value);
    require(result.ec == std::errc{} && result.ptr != cursor &&
            (result.ptr == end || whitespace(*result.ptr)), "invalid or missing input integer");
    cursor = result.ptr;
    return value;
  };
  Input result;
  result.hash = sha256(text);
  result.input_bytes = as_u64(text.size());
  const auto n = next();
  require(n >= 2 && n <= result.points.max_size(), "input requires a representable n>=2");
  result.points.reserve(static_cast<std::size_t>(n));
  std::vector<U64> unique;
  unique.reserve(static_cast<std::size_t>(n));
  for (U64 id = 0; id < n; ++id) {
    Point point{};
    for (auto& coordinate : point) {
      const auto value = next();
      require(value <= 65535, "coordinate outside u16");
      coordinate = static_cast<std::uint16_t>(value);
    }
    result.points.push_back(point);
    unique.push_back((U64{point[0]} << 32U) | (U64{point[1]} << 16U) | point[2]);
  }
  while (cursor != end && whitespace(*cursor)) ++cursor;
  require(cursor == end, "trailing data after the declared cloud");
  std::sort(unique.begin(), unique.end());
  require(std::adjacent_find(unique.begin(), unique.end()) == unique.end(), "duplicate input sites");
  return result;
}

struct Stats {
  U64 prep_cover_tests{}, prep_lens_tests{}, prep_form_sites{}, prep_seed_tests{};
  U64 eligible_lens_sites{}, hull_projection_points{}, hull_turn_tests{}, hull_sides{};
  U64 cell_visits{}, plane_cell_tests{}, disk_box_tests{}, hull_side_tests{};
  U64 out_disk{}, out_hull{}, deep{}, unknown{}, internal{}, max_depth{};
  U64 ambiguous_ids_peak{}, ambiguous_capacity_peak{}, query_visits{};
  U64 query_line_tests{}, query_no_intersection{}, query_out{}, query_deep{}, query_unknown{};
};
struct Form { I64 constant{}, alpha{}, beta{}; };
struct Side { V normal{}; I height{}; };
struct HullPoint { I x{}, y{}; V raw{}; };
struct Preparation {
  V v{}, A{}, B{};
  I64 D{}, h{}, Q{};
  std::size_t axis{}, i{}, j{};
  std::vector<Form> forms;
  std::vector<std::size_t> cover, seeds;
  std::vector<Side> sides;
  bool hull_degenerate{true};
};

void build_hull(Preparation& p, const V& low, const V& high, Stats& stats) {
  std::vector<HullPoint> points;
  points.reserve(9);
  const auto project = [&](V raw) {
    const I along = dot(raw, p.v);
    points.push_back({I{p.D}*raw[p.i]-along*p.v[p.i], I{p.D}*raw[p.j]-along*p.v[p.j], raw});
    add(stats.hull_projection_points);
  };
  project({0,0,0});
  for (unsigned corner = 0; corner < 8; ++corner) {
    V raw{};
    for (std::size_t axis = 0; axis < 3; ++axis) raw[axis] = (corner & (1U << axis)) != 0 ? high[axis] : low[axis];
    project(raw);
  }
  std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
  points.erase(std::unique(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.x==b.x && a.y==b.y; }), points.end());
  if (points.size() < 3) return;
  const auto turn = [&](const HullPoint& a, const HullPoint& b, const HullPoint& c) {
    add(stats.hull_turn_tests);
    return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
  };
  std::vector<HullPoint> hull;
  for (const auto& point : points) {
    while (hull.size() >= 2 && turn(hull[hull.size()-2],hull.back(),point) <= 0) hull.pop_back();
    hull.push_back(point);
  }
  const auto lower = hull.size();
  for (std::size_t pos = points.size()-1; pos > 0; --pos) {
    const auto& point = points[pos-1];
    while (hull.size() > lower && turn(hull[hull.size()-2],hull.back(),point) <= 0) hull.pop_back();
    hull.push_back(point);
  }
  hull.pop_back();
  if (hull.size() < 3) return;
  for (std::size_t pos = 0; pos < hull.size(); ++pos) {
    const auto& first = hull[pos].raw;
    const auto& second = hull[(pos+1)%hull.size()].raw;
    V delta{};
    for (std::size_t axis = 0; axis < 3; ++axis) delta[axis] = second[axis]-first[axis];
    auto normal = cross(p.v,delta);
    I height = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) height += I{normal[axis]}*first[axis];
    // The average of all polygon vertices is strictly interior. Use its
    // numerator so orientation never needs division or floating point.
    I interior = -height * static_cast<I>(hull.size());
    for (const auto& point : hull)
      for (std::size_t axis = 0; axis < 3; ++axis) interior += I{normal[axis]}*point.raw[axis];
    require(interior != 0, "nondegenerate hull has an invalid supporting side");
    if (interior > 0) {
      for (auto& component : normal) component = -component;
      height = -height;
    }
    // Qualify all projected input corners and zero against this orientation.
    for (const auto& point : points) {
      I value = -height;
      for (std::size_t axis = 0; axis < 3; ++axis) value += I{normal[axis]}*point.raw[axis];
      require(value <= 0, "hull side excludes a projected source point");
    }
    p.sides.push_back({normal,height});
  }
  stats.hull_sides = as_u64(p.sides.size());
  p.hull_degenerate = false;
}

Preparation prepare(const std::vector<Point>& points, unsigned depth, bool polygon, Stats& stats) {
  Preparation p;
  p.Q = I64{1} << depth;
  p.v = difference(points[1],points[0]);
  p.D = dot(p.v,p.v);
  require(p.D > 0, "zero-length owner edge");
  for (std::size_t axis = 1; axis < 3; ++axis)
    if (magnitude(p.v[axis]) > magnitude(p.v[p.axis])) p.axis=axis;
  std::array<std::size_t,2> others{};
  std::size_t next = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) if (axis != p.axis) others[next++]=axis;
  p.i=others[0]; p.j=others[1]; p.h=magnitude(p.v[p.axis]);
  const I64 sign = p.v[p.axis] > 0 ? 1 : -1;
  p.A[p.i]=p.h; p.A[p.axis]=-sign*p.v[p.i];
  p.B[p.j]=p.h; p.B[p.axis]=-sign*p.v[p.j];
  require(dot(p.A,p.v)==0 && dot(p.B,p.v)==0, "invalid center-plane basis");
  p.forms.reserve(points.size());
  V low{}, high{};
  bool have_lens = false;
  for (std::size_t id = 0; id < points.size(); ++id) {
    V w{};
    for (std::size_t axis = 0; axis < 3; ++axis)
      w[axis]=2*I64{points[id][axis]}-points[0][axis]-points[1][axis];
    const auto norm=dot(w,w);
    // u16 gives |w_i|<=2M, |A_i|,|B_i|<=M. These three coefficients
    // fit i64; every cell evaluation promotes before multiplying by Q/alpha/beta.
    p.forms.push_back({norm-p.D,-2*dot(w,p.A),-2*dot(w,p.B)});
    add(stats.prep_form_sites);
    add(stats.prep_cover_tests);
    if (norm <= 4*p.D) p.cover.push_back(id);
    if (id < 2) continue;
    const auto u=difference(points[id],points[0]);
    const auto other=difference(points[id],points[1]);
    const auto E=dot(u,u), X=dot(other,other), F=dot(u,p.v);
    add(stats.prep_lens_tests);
    if (E <= p.D && X <= p.D) {
      add(stats.eligible_lens_sites);
      if (!have_lens) { low=w; high=w; have_lens=true; }
      else for (std::size_t axis=0;axis<3;++axis) { low[axis]=std::min(low[axis],w[axis]); high[axis]=std::max(high[axis],w[axis]); }
    }
    add(stats.prep_seed_tests);
    if (F>0 && p.D-F>0 && E-F>0 && E<=p.D && X<=p.D) p.seeds.push_back(id);
  }
  if (polygon && have_lens) build_hull(p,low,high,stats);
  return p;
}

struct Cell { I64 al{}, ah{}, bl{}, bh{}; };
enum class Kind : unsigned char { Unknown, Internal, Out, Deep };
struct Node {
  Cell cell;
  std::array<std::size_t,4> children{absent,absent,absent,absent};
  Kind kind{Kind::Unknown};
  unsigned char depth{};
};
std::pair<I,I> bounds(const Form& form, const Cell& cell, I64 Q) {
  I minimum=0, maximum=0;
  bool first=true;
  for (const auto alpha : {cell.al,cell.ah}) for (const auto beta : {cell.bl,cell.bh}) {
    const I value=I{Q}*form.constant+I{alpha}*form.alpha+I{beta}*form.beta;
    if (first) { minimum=value; maximum=value; first=false; }
    else { minimum=std::min(minimum,value); maximum=std::max(maximum,value); }
  }
  return {minimum,maximum};
}

class Map {
 public:
  Map(const Preparation& prep, U64 threshold, unsigned depth, std::size_t budget, Stats& stats)
      : p_(prep), threshold_(threshold), depth_(depth), budget_(budget), stats_(stats) {
    nodes_.reserve(std::min<std::size_t>(budget_,4096));
    nodes_.push_back({{-2*p_.Q,2*p_.Q,-2*p_.Q,2*p_.Q},{absent,absent,absent,absent},Kind::Unknown,0});
  }
  void build() {
    // The initial ambiguity list IS the one retained cover-ID vector. Count
    // it once; every recursively live child list is additional storage.
    live_ids_=as_u64(p_.cover.size()); live_capacity_=as_u64(p_.cover.capacity());
    stats_.ambiguous_ids_peak=live_ids_; stats_.ambiguous_capacity_peak=live_capacity_;
    build_node(0,p_.cover,0);
    require(live_ids_==p_.cover.size() && live_capacity_==p_.cover.capacity(), "ambiguous storage lifetime ledger failed");
  }
  bool rejects(std::size_t seed) { return covered(0,p_.forms[seed]); }
  std::size_t size() const { return nodes_.size(); }
  std::size_t capacity() const { return nodes_.capacity(); }
 private:
  struct Ambiguity {
    Map& owner;
    std::vector<std::size_t> ids;
    explicit Ambiguity(Map& map) : owner(map) {}
    void push(std::size_t id) {
      const auto old=ids.capacity(); ids.push_back(id);
      add(owner.live_ids_);
      add(owner.live_capacity_,as_u64(ids.capacity()-old));
      owner.stats_.ambiguous_ids_peak=std::max(owner.stats_.ambiguous_ids_peak,owner.live_ids_);
      owner.stats_.ambiguous_capacity_peak=std::max(owner.stats_.ambiguous_capacity_peak,owner.live_capacity_);
    }
    ~Ambiguity() { owner.live_ids_-=as_u64(ids.size()); owner.live_capacity_-=as_u64(ids.capacity()); }
  };
  bool outside(const Cell& cell) {
    add(stats_.disk_box_tests);
    std::array<I,3> low{},high{};
    bool first=true;
    for (const auto alpha : {cell.al,cell.ah}) for (const auto beta : {cell.bl,cell.bh}) {
      for (std::size_t axis=0;axis<3;++axis) {
        const I value=I{p_.A[axis]}*alpha+I{p_.B[axis]}*beta;
        if (first) low[axis]=high[axis]=value;
        else { low[axis]=std::min(low[axis],value); high[axis]=std::max(high[axis],value); }
      }
      first=false;
    }
    I minimum_norm=0;
    for (std::size_t axis=0;axis<3;++axis) {
      const I nearest=low[axis]>0?low[axis]:(high[axis]<0?high[axis]:0);
      minimum_norm+=nearest*nearest;
    }
    if (2*minimum_norm>I{p_.Q}*p_.Q*p_.D) { add(stats_.out_disk); return true; }
    for (const auto& side:p_.sides) {
      add(stats_.hull_side_tests);
      I alpha_coefficient=0,beta_coefficient=0;
      for (std::size_t axis=0;axis<3;++axis) {
        alpha_coefficient+=I{side.normal[axis]}*p_.A[axis];
        beta_coefficient+=I{side.normal[axis]}*p_.B[axis];
      }
      const I minimum=alpha_coefficient*(alpha_coefficient<0?cell.ah:cell.al)+
                      beta_coefficient*(beta_coefficient<0?cell.bh:cell.bl);
      if (minimum>side.height*p_.Q) { add(stats_.out_hull); return true; }
    }
    return false;
  }
  void build_node(std::size_t id,const std::vector<std::size_t>& candidates,U64 count) {
    add(stats_.cell_visits);
    const auto cell=nodes_[id].cell;
    const auto depth=nodes_[id].depth;
    stats_.max_depth=std::max(stats_.max_depth,static_cast<U64>(depth));
    if (outside(cell)) { nodes_[id].kind=Kind::Out; return; }
    Ambiguity ambiguous(*this);
    for (const auto point:candidates) {
      add(stats_.plane_cell_tests);
      const auto [minimum,maximum]=bounds(p_.forms[point],cell,p_.Q);
      if (maximum<0) {
        add(count);
        if (count>=threshold_) { nodes_[id].kind=Kind::Deep; add(stats_.deep); return; }
      } else if (minimum<0) ambiguous.push(point);
      // min>=0 proves non-interiority throughout the closed cell, including
      // all-zero forms of a,b and a seed on its own family line.
    }
    // No ambiguous witness can add a credit below this cell. Keeping UNKNOWN
    // is conservative; further domain-only refinement is deliberately unpaid.
    if (ambiguous.ids.empty() || depth>=depth_ || budget_-nodes_.size()<4) { add(stats_.unknown); return; }
    const I64 am=cell.al+(cell.ah-cell.al)/2, bm=cell.bl+(cell.bh-cell.bl)/2;
    const std::array<Cell,4> children{{{cell.al,am,cell.bl,bm},{am,cell.ah,cell.bl,bm},
                                        {cell.al,am,bm,cell.bh},{am,cell.ah,bm,cell.bh}}};
    std::array<std::size_t,4> child_ids{};
    for (std::size_t child=0;child<4;++child) {
      child_ids[child]=nodes_.size();
      nodes_.push_back({children[child],{absent,absent,absent,absent},Kind::Unknown,
                        static_cast<unsigned char>(depth+1)});
    }
    nodes_[id].children=child_ids; nodes_[id].kind=Kind::Internal; add(stats_.internal);
    for (const auto child:child_ids) build_node(child,ambiguous.ids,count);
  }
  bool covered(std::size_t id,const Form& line) {
    add(stats_.query_visits); add(stats_.query_line_tests);
    const auto& node=nodes_[id];
    const auto [minimum,maximum]=bounds(line,node.cell,p_.Q);
    if (minimum>0 || maximum<0) { add(stats_.query_no_intersection); return true; }
    if (node.kind==Kind::Out) { add(stats_.query_out); return true; }
    if (node.kind==Kind::Deep) { add(stats_.query_deep); return true; }
    if (node.kind==Kind::Unknown) { add(stats_.query_unknown); return false; }
    for (const auto child:node.children) if (!covered(child,line)) return false;
    return true;
  }
  const Preparation& p_;
  U64 threshold_;
  unsigned depth_;
  std::size_t budget_;
  Stats& stats_;
  std::vector<Node> nodes_;
  U64 live_ids_{},live_capacity_{};
};

void vector_json(const V& value) { std::cout<<'['<<value[0]<<','<<value[1]<<','<<value[2]<<']'; }
int run(int argc,char** argv) {
  require(argc==6,"usage: center_blocks input.txt K depth node_budget domain(0|1)");
  const auto started=Clock::now();
  const U64 K=argument(argv[2]), depth_arg=argument(argv[3]), budget_arg=argument(argv[4]), domain=argument(argv[5]);
  require(K>=3 && depth_arg<=10 && budget_arg>=1 && budget_arg<=std::numeric_limits<std::size_t>::max() && domain<=1,
          "require K>=3, depth<=10, representable positive node_budget, domain 0 or 1");
  const auto depth=static_cast<unsigned>(depth_arg);
  const auto budget=static_cast<std::size_t>(budget_arg);
  auto input=read_input(argv[1]);
  const auto input_done=Clock::now();
  Stats stats;
  const auto prep=prepare(input.points,depth,domain==1,stats);
  const auto prep_done=Clock::now();
  Map map(prep,K-2,depth,budget,stats);
  map.build();
  const auto build_done=Clock::now();
  std::vector<std::size_t> rejected;
  for (const auto seed:prep.seeds) if (map.rejects(seed)) rejected.push_back(seed);
  const auto query_done=Clock::now();
  const auto survivors=prep.seeds.size()-rejected.size();
  require(map.size()<=budget && stats.cell_visits==map.size(),"tree budget or visit ledger failed");
  require(stats.out_disk+stats.out_hull+stats.deep+stats.unknown+stats.internal==map.size(),"tree-kind ledger failed");
  require(stats.query_unknown==survivors,"query survivor ledger failed");
  std::cout<<std::fixed<<std::setprecision(6)
    <<"{\"schema\":\"mhgp8_audit_q4_center_blocks_v1\",\"status\":\"passed\","
      "\"phase\":\"independent_certificate_prototype\",\"public_status\":\"not_claimed\","
      "\"fallback_executed\":false,\"product_code_used\":false,\"input_sha256\":\""<<input.hash<<"\","
    <<"\"input_bytes\":"<<input.input_bytes<<",\"n\":"<<input.points.size()<<",\"kmax\":"<<K
    <<",\"depth\":"<<depth<<",\"node_budget\":"<<budget<<",\"domain\":"<<domain
    <<",\"Q\":"<<prep.Q<<",\"D\":"<<prep.D<<",\"basis_axis\":"<<prep.axis<<",\"basis_A\":";
  vector_json(prep.A); std::cout<<",\"basis_B\":"; vector_json(prep.B);
  std::cout<<",\"hull_degenerate\":"<<(prep.hull_degenerate?"true":"false")
    <<",\"cover_sites\":"<<prep.cover.size()<<",\"seeds\":"<<prep.seeds.size()
    <<",\"rejected\":"<<rejected.size()<<",\"survivors\":"<<survivors
    <<",\"forecast_scan_reads\":"<<decimal(U{prep.cover.size()}*survivors)
    <<",\"rejected_seed_ids\":[";
  for (std::size_t pos=0;pos<rejected.size();++pos) { if(pos!=0)std::cout<<','; std::cout<<rejected[pos]; }
  std::cout<<"],\"work\":{";
#define FIELD(name) std::cout<<"\"" #name "\":"<<stats.name<<','
  FIELD(prep_cover_tests); FIELD(prep_lens_tests); FIELD(prep_form_sites); FIELD(prep_seed_tests);
  FIELD(eligible_lens_sites); FIELD(hull_projection_points); FIELD(hull_turn_tests); FIELD(hull_sides);
  FIELD(cell_visits); FIELD(plane_cell_tests); FIELD(disk_box_tests); FIELD(hull_side_tests);
  FIELD(out_disk); FIELD(out_hull); FIELD(deep); FIELD(unknown); FIELD(internal); FIELD(max_depth);
  FIELD(ambiguous_ids_peak); FIELD(ambiguous_capacity_peak); FIELD(query_visits); FIELD(query_line_tests);
  FIELD(query_no_intersection); FIELD(query_out); FIELD(query_deep);
#undef FIELD
  std::cout<<"\"query_unknown\":"<<stats.query_unknown<<"},\"memory\":{"
    <<"\"tree_nodes\":"<<map.size()<<",\"tree_capacity\":"<<map.capacity()<<",\"node_bytes\":"<<sizeof(Node)
    <<",\"tree_capacity_bytes\":"<<decimal(U{map.capacity()}*sizeof(Node))
    <<",\"form_capacity_bytes\":"<<decimal(U{prep.forms.capacity()}*sizeof(Form))
    <<",\"cover_id_capacity_bytes\":"<<decimal(U{prep.cover.capacity()}*sizeof(std::size_t))
    <<",\"seed_id_capacity_bytes\":"<<decimal(U{prep.seeds.capacity()}*sizeof(std::size_t))
    <<",\"rejected_id_capacity_bytes\":"<<decimal(U{rejected.capacity()}*sizeof(std::size_t))
    <<",\"point_capacity_bytes\":"<<decimal(U{input.points.capacity()}*sizeof(Point))
    <<",\"hull_side_capacity_bytes\":"<<decimal(U{prep.sides.capacity()}*sizeof(Side))
    <<",\"ambiguous_capacity_peak_bytes\":"<<decimal(U{stats.ambiguous_capacity_peak}*sizeof(std::size_t))
    <<",\"scope\":\"vector capacities; ambiguity sums all simultaneously live lists including cover; excludes allocator metadata, transient reallocations, stacks, input parsing and RSS\"}"
    <<",\"timings_ms\":{\"input_read_hash_validate\":"<<ms(started,input_done)
    <<",\"preparation_cover_lens_forms_hull\":"<<ms(input_done,prep_done)
    <<",\"map_build\":"<<ms(prep_done,build_done)<<",\"queries\":"<<ms(build_done,query_done)
    <<",\"total_before_json\":"<<ms(started,query_done)<<"}}\n";
  return 0;
}
}  // namespace

int main(int argc,char** argv) {
  try { return run(argc,argv); }
  catch(const std::exception& error) { std::cerr<<"center_blocks: "<<error.what()<<'\n'; return 1; }
}
