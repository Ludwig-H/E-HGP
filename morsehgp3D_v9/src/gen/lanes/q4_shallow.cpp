#include "lanes/q4_shallow.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp9::gen {
namespace {
i64 distance(Point3 a, Point3 b) {
  i64 out=0;
  for (std::size_t i=0; i<3; ++i) {
    const i64 d=static_cast<i64>(a[i])-b[i];
    out+=d*d;
  }
  return out;
}
bool acute(Point3 a, Point3 b, Point3 c) {
  const auto d=distance(a,b), e=distance(a,c), f=distance(b,c);
  return d+e>f && d+f>e && e+f>d;
}
bool owned(std::span<const Point3> points, std::span<const std::size_t> ids, u64& tests) {
  const auto owner=std::minmax(ids[0],ids[1]);
  const auto d=distance(points[ids[0]],points[ids[1]]);
  for (std::size_t i=0; i<ids.size(); ++i) for (std::size_t j=i+1; j<ids.size(); ++j) {
    if (i==0 && j==1) continue;
    counter_add(tests);
    const auto e=distance(points[ids[i]],points[ids[j]]);
    if (e>d || (e==d && std::minmax(ids[i],ids[j])<owner)) return false;
  }
  return true;
}
u64 bytes(std::size_t capacity) {
  if (capacity>std::numeric_limits<u64>::max()/sizeof(std::size_t))
    throw std::overflow_error("mhgp9 gen shallow buffer capacity overflow");
  return static_cast<u64>(capacity)*sizeof(std::size_t);
}

struct Engine {
  Q4ShallowSetPtr witnesses;
  Q4ShallowSweepWork work;
  std::vector<std::size_t> events, shell;
  explicit Engine(Q4ShallowSetPtr w):witnesses(std::move(w)) {}

  void observe() {
    u64 value=bytes(events.capacity());
    counter_add(value,bytes(shell.capacity()));
    work.peak_buffer_bytes=std::max(work.peak_buffer_bytes,value);
    work.family.retained_capacity_bytes=work.peak_buffer_bytes;
  }

  void seed(std::size_t x, const Q34SeedConsumer& consumer) {
    const auto cover=witnesses->geometry()->cover();
    const auto points=cover->index()->cloud().points();
    if (x>=points.size()) throw std::out_of_range("mhgp9 gen shallow seed outside cloud");
    const auto edge=cover->edge_ids();
    const auto a=points[edge[0]], b=points[edge[1]];
    const auto family=Q4FamilySeed::make(a,b,points[x]);
    if (!family) throw std::invalid_argument("mhgp9 gen shallow seed must be strictly acute");
    counter_add(work.seed_queries);
    const std::array<std::size_t,3> seed_ids{edge[0],edge[1],x};
    if (!owned(points,seed_ids,work.seed_owner_tests)) {
      counter_add(work.seed_owner_rejections); return;
    }
    const auto retained=witnesses->retained_ids();
    if (!std::binary_search(retained.begin(),retained.end(),x,[&](auto left,auto right) {
      counter_add(work.membership_comparisons);return left<right;
    })) {counter_add(work.removed_seed_rejections);return;}

    // Explicitly requalified port of the covered event sweep, not a scan of
    // original index node populations. The certificate concerns RETAINED IDs.
    // No contributions from peeled witnesses are added to this zero census.
    events.clear(); shell.clear();
    std::size_t inside=0, constant_inside=0, entry_count=0;
    for (const auto id:retained) {
      counter_add(work.family.sites);
      const auto side=family->side(points[id]);
      if (side!=0) {
        events.push_back(id);counter_add(work.family.event_count);
        if (side>0) {++entry_count;counter_add(work.family.entries);}
        else {++inside;counter_add(work.family.exits);}
      } else {
        const auto power=family->power(points[id]);
        if (power<0) {++inside;++constant_inside;counter_add(work.family.constant_inside);}
        else if (power==0) {shell.push_back(id);counter_add(work.family.constant_on);}
        else counter_add(work.family.constant_outside);
      }
    }
    observe();
    // Retained IDs are already sorted by original ID, so the constant shell
    // needs no second sort. Event ties likewise sort by original ID.
    std::sort(events.begin(),events.end(),[&](auto left,auto right) {
      counter_add(work.family.sort_comparisons);
      const auto comparison=family->compare_roots(points[left],points[right]);
      return comparison<0 || (comparison==0 && left<right);
    });
    for (std::size_t first=0; first<events.size();) {
      auto last=first+1;
      while (last<events.size()) {
        counter_add(work.family.group_comparisons);
        if (family->compare_roots(points[events[first]],points[events[last]])!=0) break;
        ++last;
      }
      std::size_t entries=0, exits=0;
      for (auto pos=first; pos<last; ++pos) {
        if (family->side(points[events[pos]])>0) ++entries; else ++exits;
      }
      if (exits>inside) throw std::logic_error("mhgp9 gen shallow exit underflow");
      inside-=exits;
      counter_add(work.family.groups);counter_add(work.family.callbacks);
      work.family.max_group=std::max(work.family.max_group,static_cast<u64>(last-first));
      if (inside>=witnesses->kmax()-2) {
        counter_add(work.depth_rejected_groups);
        counter_add(work.depth_skipped_ids,static_cast<u64>(last-first));
      } else {
        bool emitted=false;
        for (auto pos=first; pos<last; ++pos) {
          const auto y=events[pos];counter_add(work.presentations);
          std::array<std::size_t,4> ids{edge[0],edge[1],x,y};
          if (!owned(points,ids,work.owner_tests)) {counter_add(work.owner_rejections);continue;}
          counter_add(work.positive_tests);
          const auto ball=ExactBall::make_q4({a,b,points[x],points[y]});
          if (!ball) {counter_add(work.positive_rejections);continue;}
          counter_add(work.canonical_tests);
          if (y<x && acute(a,b,points[y])) {counter_add(work.canonical_rejections);continue;}
          // Reduced depth<T first restores the ENTIRE cover depth/shell;
          // positivity and owner then restore the ENTIRE cloud depth/shell.
          // Do not call a deep reduced count the exact full-cloud depth.
          std::sort(ids.begin(),ids.end());
          consumer(Q34SeedCandidate{4,ids,*ball,inside,
              std::span<const std::size_t>(events).subspan(first,last-first),shell});
          counter_add(work.emitted);
          counter_add(work.shell_ids,static_cast<u64>(last-first+shell.size()));
          counter_add(work.unexamined_after_emit,static_cast<u64>(last-pos-1));
          emitted=true;break;
        }
        if (!emitted) counter_add(work.groups_without_support);
      }
      if (inside>retained.size() || entries>retained.size()-inside)
        throw std::logic_error("mhgp9 gen shallow entry overflow");
      inside+=entries;first=last;
    }
    if (inside!=constant_inside+entry_count)
      throw std::logic_error("mhgp9 gen shallow final population mismatch");
  }
};
}  // namespace

Q4ShallowSweepWork run_q4_shallow_seed_candidates(
    Q4ShallowSetPtr witnesses,std::size_t x,const Q34SeedConsumer& consumer) {
  if (!witnesses || !consumer)
    throw std::invalid_argument("mhgp9 gen shallow seed requires certified set and callback");
  Engine engine(std::move(witnesses));engine.seed(x,consumer);return engine.work;
}

Q4ShallowEdgeWork run_q4_shallow_edge_candidates(
    Q34EdgeCoverPtr cover,std::size_t k,const Q34SeedConsumer& consumer) {
  if (!cover || !consumer || k==0)
    throw std::invalid_argument("mhgp9 gen shallow edge requires cover, callback and positive K");
  require_complete_q34_cover(cover);
  Q4ShallowEdgeWork work{};
  if (k<3) return work;
  const auto geometry=Q4LocalGeometry::make(cover,Q4CenterDomainMode::Disk);
  const auto witnesses=Q4ShallowSet::make(geometry,k);
  work.geometry=geometry->work();work.selection=witnesses->work();
  Engine engine(witnesses);
  const auto points=cover->index()->cloud().points();
  const auto edge=cover->edge_ids();
  const auto a=points[edge[0]],b=points[edge[1]];
  for (const auto id:witnesses->retained_ids()) {
    if (id==edge[0] || id==edge[1]) continue;
    counter_add(work.seed_candidates);counter_add(work.acute_tests);
    if (!acute(a,b,points[id])) continue;
    counter_add(work.acute_seeds);
    const std::array<std::size_t,3> seed_ids{edge[0],edge[1],id};
    if (!owned(points,seed_ids,work.owner_tests)) {counter_add(work.owner_rejections);continue;}
    counter_add(work.seeds);engine.seed(id,consumer);
  }
  work.sweep=engine.work;
  u64 query_bytes=static_cast<u64>(witnesses->retained_bytes());
  counter_add(query_bytes,work.sweep.peak_buffer_bytes);
  work.peak_live_buffer_bytes=std::max(work.selection.peak_live_bytes,query_bytes);
  counter_add(work.peak_live_buffer_bytes,static_cast<u64>(geometry->retained_bytes()));
  return work;
}

}  // namespace mhgp9::gen
