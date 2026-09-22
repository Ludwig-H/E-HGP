#include "lanes/q4_shallow_set.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {
struct DualSite {
  Q4LocalForm coordinates;  // Homogeneous (x,y,c), with c strictly positive.
  std::size_t id{};
};
struct CoordinateGroup { std::size_t first{}, last{}; };

u64 capacity_bytes(std::size_t capacity,std::size_t width) {
  if (capacity>std::numeric_limits<u64>::max()/width)
    throw std::overflow_error("mhgp8 shallow set capacity overflow");
  return static_cast<u64>(capacity)*static_cast<u64>(width);
}
int compare_coordinates(Q4LocalForm a,Q4LocalForm b) {
  const i128 ax=static_cast<i128>(a.x)*b.constant;
  const i128 bx=static_cast<i128>(b.x)*a.constant;
  if (ax!=bx) return ax<bx?-1:1;
  const i128 ay=static_cast<i128>(a.y)*b.constant;
  const i128 by=static_cast<i128>(b.y)*a.constant;
  return ay==by?0:(ay<by?-1:1);
}
i128 orientation(Q4LocalForm a,Q4LocalForm b,Q4LocalForm c) {
  // The positive product a.c*b.c*c.c need never be formed. Its sign is
  // known, so the rational orientation is the following 3x3 determinant.
  // With M=262143: |c|<=15M^2, |x|,|y|<=8M^2. Each lex cross product is
  // <=120M^4<2^79; the determinant is <=5760M^6<2^121. No cross product
  // of rational differences (degree eight) and no division is used.
  return static_cast<i128>(a.x)*(static_cast<i128>(b.y)*c.constant-static_cast<i128>(b.constant)*c.y)
       - static_cast<i128>(a.y)*(static_cast<i128>(b.x)*c.constant-static_cast<i128>(b.constant)*c.x)
       + static_cast<i128>(a.constant)*(static_cast<i128>(b.x)*c.y-static_cast<i128>(b.y)*c.x);
}
}  // namespace

Q4ShallowSetPtr Q4ShallowSet::make(Q4LocalGeometryPtr geometry,std::size_t kmax) {
  if (!geometry || kmax<3)
    throw std::invalid_argument("mhgp8 shallow set requires geometry and K>=3");
  return Q4ShallowSetPtr(new Q4ShallowSet(std::move(geometry),kmax));
}

Q4ShallowSet::Q4ShallowSet(Q4LocalGeometryPtr geometry,std::size_t kmax)
    :geometry_(std::move(geometry)),kmax_(kmax) {
  counter_add(work_.preparations);
  std::vector<DualSite> positive,negative;
  const auto observe=[&](std::size_t groups=0,std::size_t hull=0,std::size_t flags=0) {
    u64 bytes=capacity_bytes(retained_.capacity(),sizeof(std::size_t));
    counter_add(bytes,capacity_bytes(positive.capacity(),sizeof(DualSite)));
    counter_add(bytes,capacity_bytes(negative.capacity(),sizeof(DualSite)));
    counter_add(bytes,capacity_bytes(groups,sizeof(CoordinateGroup)));
    counter_add(bytes,capacity_bytes(hull,sizeof(std::size_t)));
    counter_add(bytes,capacity_bytes(flags,sizeof(unsigned char)));
    work_.peak_live_bytes=std::max(work_.peak_live_bytes,bytes);
  };
  const auto index=geometry_->cover()->index();
  const auto nodes=index->spatial_nodes();
  const auto order=index->spatial_order();
  for (const auto node_id:geometry_->cover_nodes()) {
    const auto range=nodes[node_id].range;
    for (auto rank=range.first;rank<range.last;++rank) {
      const auto id=order[rank];
      auto form=geometry_->form(id);
      counter_add(work_.input_sites);counter_add(work_.form_tests);
      if (form.constant==0) {
        retained_.push_back(id);counter_add(work_.zero_sites);
      } else {
        const bool above=form.constant>0;
        if (!above) {
          // The original sign remains represented by its separate list.
          // Negation is safe under the u16 coefficient bounds, not INT64_MIN.
          form={-form.constant,-form.x,-form.y};
        }
        (above?positive:negative).push_back(DualSite{form,id});
        counter_add(above?work_.positive_sites:work_.negative_sites);
        counter_add(work_.record_insertions);
      }
      observe();
    }
  }
  if (work_.input_sites!=geometry_->cover()->site_count())
    throw std::logic_error("mhgp8 shallow set cover partition mismatch");

  const auto peel=[&](std::vector<DualSite>& sites,bool positive_sign) {
    if (sites.empty()) return;
    std::sort(sites.begin(),sites.end(),[&](const auto& a,const auto& b) {
      counter_add(work_.lex_comparisons);
      const auto result=compare_coordinates(a.coordinates,b.coordinates);
      return result<0 || (result==0 && a.id<b.id);
    });
    std::vector<CoordinateGroup> groups;
    groups.reserve(sites.size());
    observe(groups.capacity());
    for (std::size_t first=0;first<sites.size();) {
      auto last=first+1;
      while (last<sites.size()) {
        counter_add(work_.lex_comparisons);
        if (compare_coordinates(sites[first].coordinates,sites[last].coordinates)!=0) break;
        ++last;
      }
      groups.push_back({first,last});
      counter_add(work_.coordinate_groups);counter_add(work_.group_insertions);
      counter_add(work_.duplicate_ids,static_cast<u64>(last-first-1));
      first=last;
    }
    if (groups.size()>std::numeric_limits<std::size_t>::max()/2)
      throw std::overflow_error("mhgp8 shallow set hull capacity overflow");
    std::vector<std::size_t> hull;
    hull.reserve(2*groups.size());
    std::vector<unsigned char> boundary(groups.size());
    const auto observe_layer=[&] {observe(groups.capacity(),hull.capacity(),boundary.capacity());};
    observe_layer();
    const auto turn=[&](std::size_t a,std::size_t b,std::size_t c) {
      counter_add(work_.orientation_tests);
      return orientation(sites[groups[a].first].coordinates,
                         sites[groups[b].first].coordinates,
                         sites[groups[c].first].coordinates);
    };
    const auto retain_group=[&](const CoordinateGroup& group) {
      for (auto i=group.first;i<group.last;++i) {
        retained_.push_back(sites[i].id);
        observe_layer();
      }
    };
    for (std::size_t layer=0;layer<kmax_-2 && !groups.empty();++layer) {
      counter_add(positive_sign?work_.positive_layers:work_.negative_layers);
      counter_add(work_.layer_input_groups,static_cast<u64>(groups.size()));
      for (const auto group:groups)
        counter_add(work_.layer_input_ids,static_cast<u64>(group.last-group.first));
      bool degenerate=true;
      for (std::size_t i=1;i+1<groups.size();++i) {
        if (turn(0,groups.size()-1,i)!=0) {degenerate=false;break;}
      }
      if (degenerate) {
        counter_add(work_.degenerate_groups,static_cast<u64>(groups.size()));
        for (const auto group:groups) retain_group(group);
        groups.clear();
        break;
      }

      hull.clear();
      std::fill(boundary.begin(),boundary.begin()+groups.size(),0);
      const auto push=[&](std::size_t i) {
        hull.push_back(i);counter_add(work_.hull_index_copies);
      };
      for (std::size_t i=0;i<groups.size();++i) {
        // STRICT right-turn removal keeps every collinear boundary point.
        while (hull.size()>=2 && turn(hull[hull.size()-2],hull.back(),i)<0) hull.pop_back();
        push(i);
      }
      const auto lower=hull.size();
      for (std::size_t i=groups.size()-1;i>0;--i) {
        const auto next=i-1;
        while (hull.size()>lower && turn(hull[hull.size()-2],hull.back(),next)<0) hull.pop_back();
        push(next);
      }
      hull.pop_back();  // Closing copy of the first coordinate group.
      for (const auto i:hull) boundary[i]=1;
      std::size_t remaining=0;
      for (std::size_t i=0;i<groups.size();++i) {
        if (boundary[i]) {
          counter_add(work_.boundary_groups);retain_group(groups[i]);
        } else {
          if (remaining!=i) {groups[remaining]=groups[i];counter_add(work_.compaction_moves);}
          ++remaining;
        }
      }
      groups.resize(remaining);  // Preserve the ONE initial rational ordering.
    }
    observe_layer();
  };
  peel(positive,true);
  peel(negative,false);
  std::sort(retained_.begin(),retained_.end(),[&](auto a,auto b) {
    counter_add(work_.retained_id_sort_comparisons);return a<b;
  });
  if (std::adjacent_find(retained_.begin(),retained_.end())!=retained_.end())
    throw std::logic_error("mhgp8 shallow set duplicated an original site");
  work_.retained_ids=static_cast<u64>(retained_.size());
  if (work_.retained_ids>work_.input_sites)
    throw std::logic_error("mhgp8 shallow set retained population overflow");
  work_.discarded_ids=work_.input_sites-work_.retained_ids;
  work_.retained_bytes=static_cast<u64>(retained_bytes());
  observe();
  // If a discarded dual p has c>0 and L_p(t)<=0, linearity gives a
  // STRICTLY smaller t.dot(p_i) on each earlier full-dimensional boundary.
  // If c<0 use a strictly larger value instead. The chosen sites belong to
  // disjoint retained layers, giving T strict interiors. For t==0, positive
  // c is outside; negative c makes all T earlier sites strict interiors.
  // Retained depth<T therefore certifies ALL removed sites strictly outside,
  // including shell exclusion. This implication does not assume beforehand
  // that the original depth was small. Each discarded seed has L_x=0 on its
  // entire family line, so that line cannot carry an accepted q4 root.
}

std::size_t Q4ShallowSet::retained_bytes() const {
  if (retained_.capacity()>std::numeric_limits<std::size_t>::max()/sizeof(std::size_t))
    throw std::overflow_error("mhgp8 shallow set retained capacity overflow");
  return retained_.capacity()*sizeof(std::size_t);
}

}  // namespace mhgp8
