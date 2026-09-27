#pragma once

// Explicit audit port of factor_plan::Plan::prepare at ddf4776d7. The old
// source remains immutable. No fp::Plan object or old cell is built here.
#include "../b_q34_factor_plan_20260926/plan.hpp"

namespace mhgp9::audit::direct_bands {
namespace fp = factor_plan;
using namespace mhgp9::gen;
enum class Mutant { None, Overlap, Mask6, Ids };
struct Band { std::uint32_t a_class, b_first, b_last; };
static_assert(sizeof(Band) == 12);
struct BandWork { u64 classes_b{}, marginal_slots{}, marginal_steps{}, classes_a{}, row_tests{}, binary_tests{}, empty_bands{}; };
struct BandData {
  std::vector<Band> bands;
  BandWork work;
  u64 q3_mass{}, q4_mass{}, union_mass{};
};

namespace detail {
inline std::uint32_t narrow(std::size_t value) {
  if (value > std::numeric_limits<std::uint32_t>::max())
    throw std::invalid_argument("direct.local_offset_overflow");
  return static_cast<std::uint32_t>(value);
}
// Trusted *internal* adapter: factors are freshly built by Plan below, or
// manufactured and checked by the gate. Not a public geometry factory.
// It does not retain references or accept externally adopted mutable storage.
inline BandData group(const fp::Factor& a, const fp::Factor& b, unsigned k, unsigned mask,
                      Mutant mutant = Mutant::None) {
  BandData out;
  struct Row { unsigned credit; std::size_t first_group, last_group, first_rank; };
  std::array<Row,10> rows{};
  std::size_t row_count=0;
  std::array<std::uint32_t,11> q3_prefix{}, q4_prefix{};
  out.work.marginal_slots=22;
  for (std::size_t i=0; i!=b.groups.size(); ++i) {
    const auto& g=b.groups[i];
    ++out.work.classes_b;
    q3_prefix[g.credit.q3+1]+=narrow(g.ranks.size());
    q4_prefix[g.credit.q4+1]+=narrow(g.ranks.size());
    if (row_count == 0 || rows[row_count-1].credit != g.credit.q3)
      rows[row_count++]={g.credit.q3,i,i+1,g.ranks.first};
    else rows[row_count-1].last_group=i+1;
  }
  for (unsigned i=1; i!=11; ++i) {
    q3_prefix[i]+=q3_prefix[i-1]; q4_prefix[i]+=q4_prefix[i-1];
    ++out.work.marginal_steps;
  }
  const auto append=[&](std::size_t ai,std::size_t first,std::size_t last) {
    if (first == last) { ++out.work.empty_bands; return; }
    out.bands.push_back({narrow(ai),narrow(first),narrow(last)});
    counter_add(out.union_mass,fp::product(a.groups[ai].ranks.size(),last-first));
  };
  for (std::size_t ai=0; ai!=a.groups.size(); ++ai) {
    ++out.work.classes_a;
    const auto c=a.groups[ai].credit;
    const unsigned r3=(mask & 2U) != 0 ? k-1U-c.q3 : 0;
    const unsigned r4=(mask & 4U) != 0 ? k-2U-c.q4 : 0;
    const auto size=a.groups[ai].ranks.size();
    counter_add(out.q3_mass,fp::product(size,q3_prefix[r3]));
    counter_add(out.q4_mass,fp::product(size,q4_prefix[r4]));
    append(ai,0,q3_prefix[r3]);
    if (r4 == 0) continue;
    for (std::size_t ri=0; ri!=row_count; ++ri) {
      ++out.work.row_tests;
      const auto& row=rows[ri];
      if (row.credit < r3) continue;
      auto first=row.first_group, last=row.last_group;
      while (first != last) {
        ++out.work.binary_tests;
        const auto middle=first+(last-first)/2;
        if (b.groups[middle].credit.q4 < r4) first=middle+1;
        else last=middle;
      }
      const auto end=first == row.last_group ? b.groups[first-1].ranks.last : b.groups[first].ranks.first;
      append(ai,row.first_rank,end);
    }
  }
  if (mutant == Mutant::Overlap && !out.bands.empty()) out.bands.push_back(out.bands.front());
  return out;
}
}  // namespace detail

// Sole owning entry point. Coordinates/index are borrowed immutably for the
// synchronous preparation; all output factors and bands are freshly owned.
// Only const views escape. Copy/move deleted: no address/alias migration.
class Plan final {
 public:
  Plan(const Q2CensusIndex& index,std::size_t an,std::size_t bn,unsigned k,unsigned mask,
       Mutant mutant=Mutant::None) : index_(&index),k_(k),mask_(mask),mutant_(mutant) {
    const auto nodes=index.spatial_nodes();
    if (k < 2 || k > 10 || mask == 0 || (mask & ~6U) != 0 ||
        (k == 2 && (mask & 4U) != 0) || an >= nodes.size() || bn >= nodes.size())
      throw std::invalid_argument("direct.invalid_K_mask_node");
    const auto ar=nodes[an].range,br=nodes[bn].range;
    if (ar.size() == 0 || br.size() == 0 || !(ar.last <= br.first || br.last <= ar.first))
      throw std::invalid_argument("direct.nonempty_disjoint_factors_required");
    static_cast<void>(detail::narrow(ar.size()));
    static_cast<void>(detail::narrow(br.size()));
    counter_add(work_.factor_sites,static_cast<u64>(ar.size()));
    counter_add(work_.factor_sites,static_cast<u64>(br.size()));
    a_=prepare(ar,nodes[an].box,nodes[bn].box);
    b_=prepare(br,nodes[bn].box,nodes[an].box);
    output_=detail::group(a_,b_,k,mask,mutant);
  }
  Plan(const Plan&)=delete;
  Plan& operator=(const Plan&)=delete;
  Plan(Plan&&)=delete;
  Plan& operator=(Plan&&)=delete;
  [[nodiscard]] const fp::Factor& a() const { return a_; }
  [[nodiscard]] const fp::Factor& b() const { return b_; }
  [[nodiscard]] const BandData& output() const { return output_; }
  [[nodiscard]] const fp::Work& work() const { return work_; }
  [[nodiscard]] unsigned pair_mask(std::size_t ai,std::size_t bi) const {
    const auto ac=a_.groups.at(ai).credit;
    const auto rank=b_.grouped.at(bi);
    const auto bc=b_.credits.at(rank-b_.original_ranks.first);
    unsigned out=0;
    if ((mask_ & 2U) != 0 && unsigned(ac.q3)+bc.q3 < k_-1U) out|=2U;
    if ((mask_ & 4U) != 0 && unsigned(ac.q4)+bc.q4 < k_-2U) out|=4U;
    return mutant_ == Mutant::Mask6 && out != 0 ? 6U : out;
  }
  [[nodiscard]] std::size_t retained_bytes() const {
    return sizeof(*this)+a_.retained_bytes()+b_.retained_bytes()+output_.bands.capacity()*sizeof(Band);
  }
  [[nodiscard]] std::size_t retained_buffers() const {
    return 8U+static_cast<std::size_t>(output_.bands.capacity()!=0);
  }
 private:
  struct Proposal { i64 score; std::size_t id; };
  fp::Factor prepare(Range ranks,const Box3& own,const Box3& opposite) {
    fp::Factor f;
    f.original_ranks=ranks;
    const auto points=index_->cloud().points();
    const auto order=index_->spatial_order();
    std::array<i64,3> direction{};
    for (std::size_t d=0;d!=3;++d)
      direction[d]=static_cast<i64>(opposite.low[d])+opposite.high[d]-own.low[d]-own.high[d];
    const auto capacity=std::min<std::size_t>(ranks.size(),k_);
    std::array<Proposal,10> pool{};
    std::size_t used=0;
    for (std::size_t rank=ranks.first;rank!=ranks.last;++rank) {
      counter_add(work_.selection_visits);
      const auto id=mutant_ == Mutant::Ids ? rank : order[rank];
      i64 score=0;
      for (std::size_t d=0;d!=3;++d) score+=direction[d]*points[id][d];
      std::size_t at=0;
      while (at!=used) {
        counter_add(work_.selection_tests);
        if (score>pool[at].score || (score==pool[at].score && id<pool[at].id)) break;
        ++at;
      }
      if (at==capacity) continue;
      if (used<capacity) ++used;
      for (auto j=used-1;j!=at;--j) { pool[j]=pool[j-1]; counter_add(work_.selection_shifts); }
      pool[at]={score,id};
    }
    f.proposals.reserve(used);
    for (std::size_t i=0;i!=used;++i) f.proposals.push_back(pool[i].id);
    counter_add(work_.selected_sites,static_cast<u64>(used));
    f.credits.resize(ranks.size());
    for (std::size_t rank=ranks.first;rank!=ranks.last;++rank) {
      counter_add(work_.anchor_visits);
      const auto id=order[rank];
      auto& c=f.credits[rank-ranks.first];
      for (const auto witness:f.proposals) {
        if (witness==id) { counter_add(work_.self_skips); continue; }
        if ((mask_&2U)!=0 && c.q3<k_-1U) {
          counter_add(work_.witness_attempts);
          if (fp::certify(Lane::Q3,points[id],opposite,points[witness],work_.predicates,fp::Mutant::None)) {
            ++c.q3; counter_add(work_.q3_credits);
          }
        }
        if ((mask_&4U)!=0 && c.q4<k_-2U) {
          counter_add(work_.witness_attempts);
          if (fp::certify(Lane::Q4,points[id],opposite,points[witness],work_.predicates,fp::Mutant::None)) {
            ++c.q4; counter_add(work_.q4_credits);
          }
        }
      }
    }
    std::array<std::size_t,100> counts{},offsets{},cursor{},occupied{};
    std::size_t occupied_count=0;
    for (const auto c:f.credits) {
      counter_add(work_.grouping_visits);
      const auto key=static_cast<std::size_t>(c.q3)*10+c.q4;
      if (counts[key]++==0) occupied[occupied_count++]=key;
    }
    std::sort(occupied.begin(),occupied.begin()+static_cast<std::ptrdiff_t>(occupied_count),[&](auto x,auto y) {
      counter_add(work_.grouping_sort_tests); return x<y;
    });
    counter_add(work_.class_slots,400);
    std::size_t prefix=0;
    for (std::size_t i=0;i!=occupied_count;++i) {
      const auto key=occupied[i]; offsets[key]=cursor[key]=prefix; prefix+=counts[key];
      f.groups.push_back({{static_cast<std::uint8_t>(key/10),static_cast<std::uint8_t>(key%10)},
                          {offsets[key],prefix}});
    }
    counter_add(work_.occupied_classes,static_cast<u64>(occupied_count));
    f.grouped.resize(ranks.size());
    for (std::size_t i=0;i!=f.credits.size();++i) {
      counter_add(work_.grouping_visits);
      const auto c=f.credits[i];
      f.grouped[cursor[static_cast<std::size_t>(c.q3)*10+c.q4]++]=ranks.first+i;
    }
    return f;
  }
  const Q2CensusIndex* index_;
  unsigned k_,mask_;
  Mutant mutant_;
  fp::Work work_;
  fp::Factor a_,b_;
  BandData output_;
};
}  // namespace mhgp9::audit::direct_bands
