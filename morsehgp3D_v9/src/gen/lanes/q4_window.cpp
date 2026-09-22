#include "lanes/q4_window.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp9::gen {
namespace {
i64 distance(Point3 a,Point3 b) {
  i64 out=0;
  for (std::size_t i=0;i<3;++i) {
    const i64 d=static_cast<i64>(a[i])-b[i];out+=d*d;
  }
  return out;
}
bool acute(Point3 a,Point3 b,Point3 x) {
  const auto d=distance(a,b),e=distance(a,x),f=distance(b,x);
  return d+e>f && d+f>e && e+f>d;
}
bool owned(std::span<const Point3> points,std::span<const std::size_t> ids,u64& tests) {
  const auto owner=std::minmax(ids[0],ids[1]);
  const auto d=distance(points[ids[0]],points[ids[1]]);
  for (std::size_t i=0;i<ids.size();++i) for (std::size_t j=i+1;j<ids.size();++j) {
    if (i==0 && j==1) continue;
    counter_add(tests);
    const auto e=distance(points[ids[i]],points[ids[j]]);
    if (e>d || (e==d && std::minmax(ids[i],ids[j])<owner)) return false;
  }
  return true;
}
u64 bytes(std::size_t capacity) {
  if (capacity>std::numeric_limits<u64>::max()/sizeof(std::size_t))
    throw std::overflow_error("mhgp9 gen window buffer capacity overflow");
  return static_cast<u64>(capacity)*sizeof(std::size_t);
}

struct Engine {
  Q4ShallowSetPtr witnesses;
  Q4WindowSweepWork work;
  // Exactly six owned ID vectors, reused across seeds. The heaps retain
  // min(T,population) IDs, never an unconditional reserve(T).
  std::vector<std::size_t> entry_heap,exit_heap,shell,lower,upper,inner;
  explicit Engine(Q4ShallowSetPtr value):witnesses(std::move(value)) {}

  void observe() {
    u64 heaps=bytes(entry_heap.capacity());counter_add(heaps,bytes(exit_heap.capacity()));
    work.window.peak_heap_bytes=std::max(work.window.peak_heap_bytes,heaps);
    u64 total=heaps;
    for (const auto* vector:{&shell,&lower,&upper,&inner}) counter_add(total,bytes(vector->capacity()));
    work.window.peak_buffer_bytes=std::max(work.window.peak_buffer_bytes,total);
    work.sweep.peak_buffer_bytes=work.window.peak_buffer_bytes;
    work.sweep.family.retained_capacity_bytes=work.window.peak_buffer_bytes;
  }

  void group(std::span<const std::size_t> ids,std::size_t x,const Q4FamilySeed& family,
             std::size_t& inside,const Q34SeedConsumer& consumer) {
    if (ids.empty()) return;
    auto& sweep=work.sweep;
    const auto cover=witnesses->geometry()->cover();
    const auto points=cover->index()->cloud().points();
    const auto edge=cover->edge_ids();
    std::size_t entries=0,exits=0;
    for (const auto id:ids) {
      if (family.side(points[id])>0) ++entries;else ++exits;
    }
    if (exits>inside) throw std::logic_error("mhgp9 gen window exit underflow");
    inside-=exits;
    counter_add(sweep.family.groups);counter_add(sweep.family.callbacks);
    sweep.family.max_group=std::max(sweep.family.max_group,static_cast<u64>(ids.size()));
    if (inside>=witnesses->kmax()-2) {
      counter_add(sweep.depth_rejected_groups);
      counter_add(sweep.depth_skipped_ids,static_cast<u64>(ids.size()));
    } else {
      bool emitted=false;
      for (std::size_t pos=0;pos<ids.size();++pos) {
        const auto y=ids[pos];counter_add(sweep.presentations);
        std::array<std::size_t,4> support{edge[0],edge[1],x,y};
        if (!owned(points,support,sweep.owner_tests)) {counter_add(sweep.owner_rejections);continue;}
        counter_add(sweep.positive_tests);
        const auto ball=ExactBall::make_q4({points[edge[0]],points[edge[1]],points[x],points[y]});
        if (!ball) {counter_add(sweep.positive_rejections);continue;}
        counter_add(sweep.canonical_tests);
        if (y<x && acute(points[edge[0]],points[edge[1]],points[y])) {
          counter_add(sweep.canonical_rejections);continue;
        }
        // The window count is exact on its CLOSED interval. Retained depth
        // below T restores the whole cover (29), then positive ownership
        // restores the whole cloud. No deep clipped count is promoted.
        std::sort(support.begin(),support.end());
        consumer(Q34SeedCandidate{4,support,*ball,inside,ids,shell});
        counter_add(sweep.emitted);
        counter_add(sweep.shell_ids,static_cast<u64>(ids.size()+shell.size()));
        counter_add(sweep.unexamined_after_emit,static_cast<u64>(ids.size()-pos-1));
        emitted=true;break;
      }
      if (!emitted) counter_add(sweep.groups_without_support);
    }
    const auto population=witnesses->retained_ids().size();
    if (inside>population || entries>population-inside)
      throw std::logic_error("mhgp9 gen window entry overflow");
    inside+=entries;
  }

  void seed(std::size_t x,const Q34SeedConsumer& consumer) {
    auto& sweep=work.sweep;
    auto& window=work.window;
    const auto cover=witnesses->geometry()->cover();
    const auto points=cover->index()->cloud().points();
    if (x>=points.size()) throw std::out_of_range("mhgp9 gen window seed outside cloud");
    const auto edge=cover->edge_ids();
    const auto family=Q4FamilySeed::make(points[edge[0]],points[edge[1]],points[x]);
    if (!family) throw std::invalid_argument("mhgp9 gen window seed must be strictly acute");
    counter_add(sweep.seed_queries);
    const std::array<std::size_t,3> seed_ids{edge[0],edge[1],x};
    if (!owned(points,seed_ids,sweep.seed_owner_tests)) {counter_add(sweep.seed_owner_rejections);return;}
    const auto retained=witnesses->retained_ids();
    if (!std::binary_search(retained.begin(),retained.end(),x,[&](auto a,auto b) {
      counter_add(sweep.membership_comparisons);return a<b;
    })) {counter_add(sweep.removed_seed_rejections);return;}

    counter_add(window.seed_queries);
    for (auto* vector:{&entry_heap,&exit_heap,&shell,&lower,&upper,&inner}) vector->clear();
    const auto threshold=witnesses->kmax()-2;
    std::size_t constants=0,event_count=0;
    const auto heap_less=[&](std::size_t a,std::size_t b,bool entry) {
      counter_add(window.heap_comparisons);
      const auto comparison=family->compare_roots(points[a],points[b]);
      const bool less=comparison<0 || (comparison==0 && a<b);
      const bool greater=comparison>0 || (comparison==0 && a>b);
      return entry?less:greater;
    };
    const auto insert=[&](std::size_t id,bool entry) {
      auto& heap=entry?entry_heap:exit_heap;
      const auto comparator=[&](auto a,auto b) {return heap_less(a,b,entry);};
      if (heap.size()<threshold) {
        heap.push_back(id);std::push_heap(heap.begin(),heap.end(),comparator);
        counter_add(entry?window.entry_heap_insertions:window.exit_heap_insertions);
      } else if (comparator(id,heap.front())) {
        std::pop_heap(heap.begin(),heap.end(),comparator);
        heap.back()=id;std::push_heap(heap.begin(),heap.end(),comparator);
        counter_add(entry?window.entry_heap_replacements:window.exit_heap_replacements);
      }
    };
    // Full first pass even if the constants already reject the seed. No
    // credited sites, integer root fractions or P1*B2 products are introduced.
    for (const auto id:retained) {
      counter_add(sweep.family.sites);
      const auto side=family->side(points[id]);
      if (side!=0) {
        ++event_count;counter_add(sweep.family.event_count);
        counter_add(side>0?sweep.family.entries:sweep.family.exits);
        insert(id,side>0);
      } else {
        const auto power=family->power(points[id]);
        if (power<0) {++constants;counter_add(sweep.family.constant_inside);}
        else if (power==0) {shell.push_back(id);counter_add(sweep.family.constant_on);}
        else counter_add(sweep.family.constant_outside);
      }
    }
    observe();
    if (constants>=threshold) {
      counter_add(window.constant_rejected_seeds);
      counter_add(window.rejected_event_ids,static_cast<u64>(event_count));return;
    }
    const auto height=threshold-constants;  // Positive, even when K is huge.
    const auto sort_heap=[&](auto& heap,bool entry) {
      std::sort(heap.begin(),heap.end(),[&](auto a,auto b) {
        counter_add(window.heap_sort_comparisons);
        const auto comparison=family->compare_roots(points[a],points[b]);
        return entry?(comparison<0 || (comparison==0 && a<b)):
                     (comparison>0 || (comparison==0 && a>b));
      });
    };
    sort_heap(entry_heap,true);sort_heap(exit_heap,false);
    std::optional<std::size_t> low,high;
    if (exit_heap.size()>=height) {low=exit_heap[height-1];counter_add(window.lower_bounds);}
    if (entry_heap.size()>=height) {high=entry_heap[height-1];counter_add(window.upper_bounds);}
    const auto compare=[&](std::size_t a,std::size_t b) {
      counter_add(window.window_comparisons);
      return family->compare_roots(points[a],points[b]);
    };
    if (low && high) {
      const auto order=compare(*low,*high);
      if (order>0) {
        counter_add(window.disjoint_rejected_seeds);
        counter_add(window.rejected_event_ids,static_cast<u64>(event_count));return;
      }
      if (order==0) counter_add(window.point_windows);
    }

    std::size_t fixed_inside=0,window_entries=0,window_exits=0,inner_entries=0,inner_exits=0;
    // Complete second pass: endpoint ties can exceed T arbitrarily. They are
    // streamed into ID-sorted endpoint vectors, never truncated by either heap.
    for (const auto id:retained) {
      counter_add(window.second_pass_sites);
      const auto side=family->side(points[id]);
      if (side==0) continue;
      const auto to_low=low?compare(id,*low):1;
      const auto to_high=high?compare(id,*high):-1;
      if (low && to_low==0) {lower.push_back(id);counter_add(window.lower_ids);}
      else if (high && to_high==0) {upper.push_back(id);counter_add(window.upper_ids);}
      else if (to_low>0 && to_high<0) {
        inner.push_back(id);counter_add(window.inner_ids);
        if (side>0) ++inner_entries;else ++inner_exits;
      }
      else {
        counter_add(window.outside_ids);
        if ((side>0 && to_low<0) || (side<0 && to_high>0)) {
          ++fixed_inside;counter_add(window.fixed_inside_sites);
        }
        continue;
      }
      if (side>0) ++window_entries;else ++window_exits;
    }
    observe();
    window.max_inner_ids=std::max(window.max_inner_ids,static_cast<u64>(inner.size()));
    window.max_endpoint_ids=std::max(window.max_endpoint_ids,
        static_cast<u64>(std::max(lower.size(),upper.size())));
    // Avoid evaluating 2*height for an arbitrary size_t K. Each strict
    // interior entry/exit population is at most H-1 independently.
    if (inner_entries>=height || inner_exits>=height)
      throw std::logic_error("mhgp9 gen window interior rank bound violated");
    if (constants>retained.size() || fixed_inside>retained.size()-constants)
      throw std::logic_error("mhgp9 gen window fixed population overflow");
    const auto base=constants+fixed_inside;
    if (base>=threshold) {
      counter_add(window.fixed_depth_rejected_seeds);
      counter_add(window.rejected_event_ids,
          static_cast<u64>(lower.size()+upper.size()+inner.size()));return;
    }
    if (window_exits>retained.size()-base)
      throw std::logic_error("mhgp9 gen window initial population overflow");
    std::size_t inside=base+window_exits;
    std::sort(inner.begin(),inner.end(),[&](auto a,auto b) {
      counter_add(sweep.family.sort_comparisons);
      const auto comparison=family->compare_roots(points[a],points[b]);
      return comparison<0 || (comparison==0 && a<b);
    });
    group(lower,x,*family,inside,consumer);
    for (std::size_t first=0;first<inner.size();) {
      auto last=first+1;
      while (last<inner.size()) {
        counter_add(sweep.family.group_comparisons);
        if (family->compare_roots(points[inner[first]],points[inner[last]])!=0) break;
        ++last;
      }
      group(std::span<const std::size_t>(inner).subspan(first,last-first),x,*family,inside,consumer);
      first=last;
    }
    group(upper,x,*family,inside,consumer);
    if (window_entries>retained.size()-base || inside!=base+window_entries)
      throw std::logic_error("mhgp9 gen window final population mismatch");
  }
};
}  // namespace

Q4WindowSweepWork run_q4_window_seed_candidates(
    Q4ShallowSetPtr witnesses,std::size_t x,const Q34SeedConsumer& consumer) {
  if (!witnesses || !consumer)
    throw std::invalid_argument("mhgp9 gen window seed requires certified set and callback");
  Engine engine(std::move(witnesses));engine.seed(x,consumer);return engine.work;
}

Q4WindowEdgeWork run_q4_window_edge_candidates(
    Q34EdgeCoverPtr cover,std::size_t k,const Q34SeedConsumer& consumer) {
  if (!cover || !consumer || k==0)
    throw std::invalid_argument("mhgp9 gen window edge requires cover, callback and positive K");
  Q4WindowEdgeWork work{};
  if (k<3) return work;
  const auto geometry=Q4LocalGeometry::make(cover,Q4CenterDomainMode::Disk);
  const auto witnesses=Q4ShallowSet::make(geometry,k);
  work.geometry=geometry->work();work.selection=witnesses->work();
  Engine engine(witnesses);
  const auto points=cover->index()->cloud().points();
  const auto edge=cover->edge_ids();
  for (const auto id:witnesses->retained_ids()) {
    if (id==edge[0] || id==edge[1]) continue;
    counter_add(work.seed_candidates);counter_add(work.acute_tests);
    if (!acute(points[edge[0]],points[edge[1]],points[id])) continue;
    counter_add(work.acute_seeds);
    const std::array<std::size_t,3> ids{edge[0],edge[1],id};
    if (!owned(points,ids,work.owner_tests)) {counter_add(work.owner_rejections);continue;}
    counter_add(work.seeds);engine.seed(id,consumer);
  }
  work.sweep=engine.work;
  u64 query_bytes=static_cast<u64>(witnesses->retained_bytes());
  counter_add(query_bytes,work.sweep.window.peak_buffer_bytes);
  work.peak_live_buffer_bytes=std::max(work.selection.peak_live_bytes,query_bytes);
  counter_add(work.peak_live_buffer_bytes,static_cast<u64>(geometry->retained_bytes()));
  return work;
}

}  // namespace mhgp9::gen
