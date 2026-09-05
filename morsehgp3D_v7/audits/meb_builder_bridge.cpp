// Bounded audit transport; F and the private Builder use shared pinned primitives.
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "f_include_root/src/forest/silent_incidence.hpp"

namespace audit_hooks {
unsigned fault = 0;
mhgp7::u64 at = 0, initial_p = 0, forms = 0, pairs = 0, references = 0, violations = 0, events_seen = 0;
void stop(unsigned first, mhgp7::u64 count) {
  if (count != at) return;
  if (fault == first) throw std::bad_alloc();
  if (fault == first + 1) throw std::runtime_error("audit_injected_exception");
}
void pair(mhgp7::u64 p, mhgp7::u64 cap) {
  ++pairs;
  if (p >= cap) ++violations;
}
void form(mhgp7::u64 p, mhgp7::u64 cap) {
  ++forms;
  if (p != initial_p + forms || p > cap) ++violations;
  stop(1, forms);
}
void reference(mhgp7::u64) { ++references; stop(3, references); }
void reset(unsigned kind, mhgp7::u64 count, mhgp7::u64 p) {
  fault = kind; at = count; initial_p = p;
  forms = pairs = references = violations = events_seen = 0;
}
}  // namespace audit_hooks

// Rename only the overlay's public result types and private detail namespace.
// The reference body and both sets of arithmetic expressions are unmodified.
#define SilentIncidenceStatus AuditStatus
#define SilentIncidenceLimits AuditLimits
#define SilentIncidenceStats AuditStats
#define SilentIncidenceResult AuditResult
#define silent_detail audit_builder_detail
#define build_silent_cofaces audit_build_silent_cofaces
#include "overlay/silent_incidence.hpp"
#undef build_silent_cofaces
#undef silent_detail
#undef SilentIncidenceResult
#undef SilentIncidenceStats
#undef SilentIncidenceLimits
#undef SilentIncidenceStatus

using namespace mhgp7;

void require(bool value) { if (!value) throw std::runtime_error("audit_input_domain"); }

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 n = negative ? static_cast<u128>(-(value + 1)) + 1 : static_cast<u128>(value);
  std::string text;
  do { text.push_back(static_cast<char>('0' + n % 10)); n /= 10; } while (n);
  if (negative) text.push_back('-');
  std::reverse(text.begin(), text.end());
  return text;
}

i128 read_positive() {
  std::string s;
  require(bool(std::cin >> s) && !s.empty());
  u128 value = 0, maximum = (~u128{0}) >> 1;
  for (const char c : s) {
    require(c >= '0' && c <= '9');
    const unsigned digit = static_cast<unsigned>(c - '0');
    require(value <= (maximum - digit) / 10);
    value = value * 10 + digit;
  }
  require(value > 0);
  return static_cast<i128>(value);
}

template <class Values>
void array(std::ostream& s, const Values& values) {
  s << '[';
  size_t i = 0;
  for (const auto v : values) s << (i++ ? "," : "") << v;
  s << ']';
}

void events(std::ostream& s, const std::vector<ForestEvent>& items) {
  s << '[';
  for (size_t i = 0; i < items.size(); ++i) {
    const auto& e = items[i];
    s << (i ? "," : "") << '[' << unsigned(e.q) << ',' << unsigned(e.d)
      << ',' << e.active_mask << ',';
    array(s, e.level.num); s << ',' << decimal(e.level.den) << ',';
    array(s, e.support); s << ','; array(s, e.interior); s << ']';
  }
  s << ']';
}

template <class Result, class Ball>
std::string encode(const Result& r, const Ball& b,
                   const std::array<i32, 11>& sites, size_t n) {
  std::ostringstream s;
  const auto& t = r.stats;
  s << "{\"status\":" << static_cast<int>(r.status) << ",\"reason\":\""
    << r.reason << "\",\"stats\":";
  array(s, std::array<u64, 13>{t.core_records,t.core_facets,t.facets_with_two_intruders,
      t.chain_steps,t.added_cofaces,t.terminal_direct,t.terminal_cached,t.max_chain_length,
      t.query_nodes,t.query_leaves,t.query_range_skips,t.meb_calls,t.meb_supports});
  s << ",\"q\":" << unsigned(b.q) << ",\"key\":[" << decimal(b.key.a);
  for (const i128 v : b.key.b) s << ',' << decimal(v);
  s << ',' << decimal(b.key.c) << "],\"num\":"; array(s,b.level.num);
  s << ",\"den\":" << decimal(b.level.den) << ",\"support_slots\":[";
  for (size_t i = 0; i < 4; ++i) {
    i64 slot = b.support[i];
    if (i < b.q && slot >= 0) {
      const auto found = std::find(sites.begin(), sites.begin() + n, b.support[i]);
      slot = found == sites.begin() + n ? -99 : found - sites.begin();
    }
    s << (i ? "," : "") << slot;
  }
  s << "],\"events\":"; events(s,r.events); s << '}';
  return s.str();
}

template <class Ball>
Ball sentinel() {
  Ball b;
  b.key = {7,{11,13,17},19}; b.level = {{23,29,31},37};
  b.q = 9; b.support = {-1,-2,-3,-4};
  return b;
}

template <class Result>
void seed(Result* r, u64 c) {
  r->status = static_cast<decltype(r->status)>(4); r->reason = "audit_initial_status";
  auto& t = r->stats;
  t.core_records=2; t.core_facets=3; t.facets_with_two_intruders=5;
  t.chain_steps=7; t.added_cofaces=11; t.terminal_direct=13; t.terminal_cached=17;
  t.max_chain_length=19; t.query_nodes=23; t.query_leaves=29; t.query_range_skips=31;
  t.meb_calls=37; t.meb_supports=c;
  ForestEvent event;
  event.q=2; event.d=1; event.active_mask=3; event.level={{23,29,31},37};
  for (size_t i=0;i<11;++i) event.support[i]=101+i;
  for (size_t i=0;i<9;++i) event.interior[i]=211+i;
  r->events={event,event};
}

void extras(const AuditResult& r) {
  const auto& s=r.stats;
  std::cout << ",\"extra\":";
  array(std::cout,std::array<u64,5>{s.meb_proposal_supports,s.meb_fallback_supports,
      s.meb_proposal_pivots,s.meb_proposal_certified,s.meb_proposal_fallback});
  std::cout << ",\"hooks\":";
  array(std::cout,std::array<u64,5>{audit_hooks::forms,audit_hooks::pairs,
      audit_hooks::references,audit_hooks::violations,audit_hooks::events_seen});
}

int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    char mode;
    while (std::cin >> mode) {
      size_t n=0, count=0;
      u64 c=0,p=0,a=0,pivots=0,cert=0,fall=0,limit=0,proposal=0;
      unsigned fault=0; u64 at=0;
      require(mode=='M' || mode=='R');
      require(bool(std::cin >> n >> count) && n>=2 && n<=11 && count>=1 && count<=1000);
      if (mode=='M') {
        require(count<=16 && bool(std::cin >> c >> p >> a >> pivots >> cert >> fall >> fault >> at));
        require(a<=c && pivots<=p && cert<=p && static_cast<u128>(cert)+fall<=37);
      } else require(bool(std::cin >> limit >> proposal >> fault >> at));
      std::vector<P3> points(n);
      for (auto& point:points) {
        require(bool(std::cin >> point.x >> point.y >> point.z));
        require(point.x>=0 && point.y>=0 && point.z>=0 && point.x<=65535 && point.y<=65535 && point.z<=65535);
      }
      const CloudIndex ix=build_cloud_index(points);
      require(ix.valid && !ix.has_duplicate_positions());
      std::array<i32,11> sites{};
      for (i32 u=0;u<ix.unique_count();++u) sites[ix.point_id(u)]=u;
      SilentIncidenceLimits fc; AuditLimits ac;
      SilentIncidenceResult f; AuditResult result;
      std::vector<ForestEvent> direct;
      audit_hooks::reset(fault,at,p);
      if (mode=='R') {
        for (size_t i=0;i<count;++i) {
          ForestEvent e; unsigned q=0,d=0,mask=0;
          require(bool(std::cin >> q >> d >> mask) && q>=2 && q<=4 && d<=9 && q+d<=11);
          e.q=static_cast<u8>(q); e.d=static_cast<u8>(d); e.active_mask=static_cast<u16>(mask);
          for (auto& v:e.level.num) require(bool(std::cin >> v));
          e.level.den=read_positive();
          for (size_t j=0;j<q;++j) require(bool(std::cin >> e.support[j]));
          for (size_t j=0;j<d;++j) require(bool(std::cin >> e.interior[j]));
          direct.push_back(e);
        }
        fc.max_meb_supports=limit; ac.max_meb_supports=limit; ac.max_meb_proposal_supports=proposal;
        f=build_silent_cofaces(ix,direct,fc);
        std::string exception="none";
        try { result=audit_build_silent_cofaces(ix,direct,ac); }
        catch (const std::runtime_error&) { exception="runtime_error"; }
        const silent_detail::LocalBall fb;
        const audit_builder_detail::LocalBall ab;
        const std::string ef=encode(f,fb,sites,n), er=encode(result,ab,sites,n);
        std::cout << "{\"mode\":\"R\",\"reference\":" << ef << ",\"actual\":" << er
                  << ",\"same_as_F\":" << (ef==er ? "true":"false")
                  << ",\"exception\":\"" << exception << '"';
        extras(result); std::cout << "}\n";
        continue;
      }
      seed(&f,c); seed(&result,c);
      result.stats.meb_proposal_supports=p; result.stats.meb_fallback_supports=a;
      result.stats.meb_proposal_pivots=pivots; result.stats.meb_proposal_certified=cert;
      result.stats.meb_proposal_fallback=fall;
      silent_detail::Builder reference(ix,direct,fc,&f);
      audit_builder_detail::Builder builder(ix,direct,ac,&result);
      for (size_t step=0;step<count;++step) {
        require(bool(std::cin >> limit >> proposal));
        fc.max_meb_supports=limit; ac.max_meb_supports=limit; ac.max_meb_proposal_supports=proposal;
        auto fb=sentinel<silent_detail::LocalBall>();
        auto ab=sentinel<audit_builder_detail::LocalBall>();
        const bool fok=reference.miniball(sites,n,&fb);
        bool ok=false; std::string exception="none";
        try { ok=builder.miniball(sites,n,&ab); }
        catch (const std::bad_alloc&) { exception="bad_alloc"; }
        catch (const std::runtime_error&) { exception="runtime_error"; }
        const std::string ef=encode(f,fb,sites,n), er=encode(result,ab,sites,n);
        std::cout << "{\"mode\":\"M\",\"step\":" << step << ",\"ok\":" << (ok?"true":"false")
                  << ",\"reference_ok\":" << (fok?"true":"false") << ",\"reference\":" << ef
                  << ",\"actual\":" << er << ",\"same_as_F\":" << (ef==er && fok==ok?"true":"false")
                  << ",\"exception\":\"" << exception << '"';
        extras(result); std::cout << "}\n";
      }
    }
    return std::cin.eof()?0:2;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
