#pragma once
#include <atomic>
#include <bit>
#include <stdexcept>
#include <vector>
#include "intruder.cuh"
#include "source/morsehgp3D_v7/src/tree/cloud_index.hpp"

namespace mhgp7::gpu_intruder_private {
namespace owner_detail {
inline u64 acquire_snapshot(std::atomic<u64>& counter) {
  u64 current=counter.load(std::memory_order_relaxed);
  for (;;) {
    if (!current || current==~u64{0}) throw std::overflow_error("owner.snapshot_exhausted");
    if (counter.compare_exchange_weak(current,current+1,std::memory_order_relaxed)) return current;
  }
}
}
// Private host owner; device allocations will belong to the eventual route.
// No external mutable access is exported. Views expire with this object.
class IndexOwner {
 public:
  explicit IndexOwner(const CloudIndex& ix) {
    const auto fail=[](bool condition, const char* reason) { if (!condition) throw std::invalid_argument(reason); };
    const auto m=ix.upos.size();
    fail(ix.valid && m && m<=0x7fffffffu, "owner.input");
    fail(ix.input_count==m && ix.keys.size()==m && ix.nodes.size()==m-1, "owner.unique_count");
    for (size_t i=0; i<m; ++i) {
      fail(p3_in_profile(ix.upos[i]) && morton48(ix.upos[i])==ix.keys[i] &&
          (!i || ix.keys[i-1]<ix.keys[i]), "owner.morton");
      positions_.push_back((u16)ix.upos[i].x); positions_.push_back((u16)ix.upos[i].y); positions_.push_back((u16)ix.upos[i].z);
    }
    const auto valid=[&](NodeRef v) { return v<0 ? (-1-(i64)v<(i64)m) : ((size_t)v<ix.nodes.size()); };
    std::vector<bool> internal_seen(m-1), leaf_seen(m);
    std::vector<NodeRef> todo{ix.root()};
    size_t reached=0;
    while (!todo.empty()) {
      const NodeRef v=todo.back(); todo.pop_back();
      fail(valid(v), "owner.reference"); ++reached;
      if (is_leaf(v)) {
        const auto u=(size_t)leaf_index(v);
        fail(!leaf_seen[u], "owner.repeated_leaf"); leaf_seen[u]=true; continue;
      }
      fail(!internal_seen[(size_t)v], "owner.repeated_node"); internal_seen[(size_t)v]=true;
      const auto& nd=ix.nodes[(size_t)v];
      fail(valid(nd.left) && valid(nd.right) && nd.first>=0 && nd.last>nd.first && (size_t)nd.last<m, "owner.range");
      const auto l=ix.range_of(nd.left), r=ix.range_of(nd.right);
      fail(l.first>=0 && l.last>=l.first && (size_t)l.last<m && r.first>=0 && r.last>=r.first && (size_t)r.last<m,
          "owner.child_range");
      fail(l.first==nd.first && r.last==nd.last && (i64)l.last+1==r.first, "owner.partition");
      const int prefix=std::countl_zero(ix.keys[(size_t)nd.first]^ix.keys[(size_t)nd.last]);
      for (NodeRef child : {nd.left,nd.right}) if (!is_leaf(child)) {
        const auto range=ix.range_of(child);
        fail(range.first>=0 && range.last>range.first && (size_t)range.last<m, "owner.child_range");
        fail(std::countl_zero(ix.keys[(size_t)range.first]^ix.keys[(size_t)range.last])>prefix, "owner.prefix_depth");
      }
      const auto lb=ix.box_of(nd.left), rb=ix.box_of(nd.right);
      for (int axis=0; axis<3; ++axis)
        fail(nd.tlo[axis]==std::min(lb.lo[axis],rb.lo[axis]) && nd.thi[axis]==std::max(lb.hi[axis],rb.hi[axis]) &&
            nd.tlo[axis]>=0 && nd.thi[axis]<=65535 && nd.tlo[axis]<=nd.thi[axis], "owner.tight_box");
      todo.push_back(nd.right); todo.push_back(nd.left);
    }
    fail(reached==2*m-1, "owner.unreachable");
    for (const auto& nd : ix.nodes) {
      left_.push_back(nd.left); right_.push_back(nd.right); first_.push_back(nd.first); last_.push_back(nd.last);
      for (int axis=0; axis<3; ++axis) boxes_.push_back((u16)nd.tlo[axis]);
      for (int axis=0; axis<3; ++axis) boxes_.push_back((u16)nd.thi[axis]);
    }
    snapshot_=owner_detail::acquire_snapshot(next_snapshot_);
  }
  IndexOwner(const IndexOwner&)=delete;
  IndexOwner& operator=(const IndexOwner&)=delete;
  IndexOwner(IndexOwner&&)=delete;
  IndexOwner& operator=(IndexOwner&&)=delete;
  IndexView view() const { return {left_.data(),right_.data(),first_.data(),last_.data(),boxes_.data(),positions_.data(),
      snapshot_,(u32)left_.size(),(u32)(positions_.size()/3),left_.empty() ? -1 : 0}; }
 private:
  std::vector<i32> left_,right_,first_,last_;
  std::vector<u16> boxes_,positions_;
  inline static std::atomic<u64> next_snapshot_{1};
  u64 snapshot_=0;
};
}  // namespace mhgp7::gpu_intruder_private
