#include "cloud/site_tree.hpp"

#include <algorithm>
#include <queue>

namespace mhgp10 {

SiteTree::SiteTree(const Cloud& cloud) : cloud_(cloud) {
  const u32 n = cloud.sites();
  nodes_.reserve(2 * (n / kLeaf + 1) + 4);
  if (n > 0) root_ = build(0, n);
}

u32 SiteTree::build(u32 lo, u32 hi) {
  const u32 id = static_cast<u32>(nodes_.size());
  nodes_.push_back(Node{lo, hi, kNone, kNone, {0, 0, 0}, {0, 0, 0}});
  if (hi - lo > kLeaf) {
    const u32 mid = lo + (hi - lo) / 2;
    const u32 l = build(lo, mid);
    const u32 r = build(mid, hi);
    Node& nd = nodes_[id];
    nd.left = l;
    nd.right = r;
    for (int a = 0; a < 3; ++a) {
      nd.bmin[a] = std::min(nodes_[l].bmin[a], nodes_[r].bmin[a]);
      nd.bmax[a] = std::max(nodes_[l].bmax[a], nodes_[r].bmax[a]);
    }
  } else {
    Node& nd = nodes_[id];
    const Buffer<u32>* c[3] = {&cloud_.x, &cloud_.y, &cloud_.z};
    for (int a = 0; a < 3; ++a) {
      nd.bmin[a] = (*c[a])[lo];
      nd.bmax[a] = (*c[a])[lo];
      for (u32 s = lo + 1; s < hi; ++s) {
        nd.bmin[a] = std::min<i64>(nd.bmin[a], (*c[a])[s]);
        nd.bmax[a] = std::max<i64>(nd.bmax[a], (*c[a])[s]);
      }
    }
  }
  return id;
}

u64 SiteTree::box_dist2(const Node& nd, i64 qx, i64 qy, i64 qz) {
  const i64 q[3] = {qx, qy, qz};
  u64 d = 0;
  for (int a = 0; a < 3; ++a) {
    i64 t = 0;
    if (q[a] < nd.bmin[a]) t = nd.bmin[a] - q[a];
    else if (q[a] > nd.bmax[a]) t = q[a] - nd.bmax[a];
    d += static_cast<u64>(t * t);
  }
  return d;
}

double SiteTree::box_dist2(const Node& nd, const double q[3]) {
  double d = 0;
  for (int a = 0; a < 3; ++a) {
    double t = 0;
    if (q[a] < double(nd.bmin[a])) t = double(nd.bmin[a]) - q[a];
    else if (q[a] > double(nd.bmax[a])) t = q[a] - double(nd.bmax[a]);
    d += t * t;
  }
  return d;
}

namespace {
constexpr double kMargin = 0.02;  // erreur absolue des distances carrees approchees : < 1e-3 en u18

void approx_center(const geom::P3& a, const geom::Center& c, double q[3]) {
  const double D = static_cast<double>(c.D);
  q[0] = double(a.x) + static_cast<double>(c.N[0]) / D;
  q[1] = double(a.y) + static_cast<double>(c.N[1]) / D;
  q[2] = double(a.z) + static_cast<double>(c.N[2]) / D;
}

double approx_d2(const double q[3], i64 x, i64 y, i64 z) {
  const double dx = double(x) - q[0], dy = double(y) - q[1], dz = double(z) - q[2];
  return dx * dx + dy * dy + dz * dz;
}
}  // namespace

void SiteTree::nearest(const geom::P3& anchor, const geom::Center& c, u32 count,
                       std::vector<std::pair<i128, u32>>& out) const {
  out.clear();
  if (root_ == kNone || count == 0) return;
  double q[3];
  approx_center(anchor, c, q);
  struct Item {
    i128 key;
    u32 site;
    double d2;
    bool operator<(const Item& o) const { return key != o.key ? key < o.key : site < o.site; }
  };
  std::vector<Item> heap;  // max-tas sur (cle, site)
  using QE = std::pair<double, u32>;
  std::priority_queue<QE, std::vector<QE>, std::greater<QE>> open;
  open.push({box_dist2(nodes_[root_], q), root_});
  while (!open.empty()) {
    const auto [bd, id] = open.top();
    open.pop();
    if (heap.size() == count && bd - kMargin > heap.front().d2 + kMargin) break;
    const Node& nd = nodes_[id];
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        const geom::P3 z{i64(cloud_.x[s]), i64(cloud_.y[s]), i64(cloud_.z[s])};
        const Item it{geom::side_key(c, anchor, z), s, approx_d2(q, z.x, z.y, z.z)};
        if (heap.size() < count) {
          heap.push_back(it);
          std::push_heap(heap.begin(), heap.end());
        } else if (it < heap.front()) {
          std::pop_heap(heap.begin(), heap.end());
          heap.back() = it;
          std::push_heap(heap.begin(), heap.end());
        }
      }
    } else {
      open.push({box_dist2(nodes_[nd.left], q), nd.left});
      open.push({box_dist2(nodes_[nd.right], q), nd.right});
    }
  }
  std::sort_heap(heap.begin(), heap.end());
  out.reserve(heap.size());
  for (const Item& it : heap) out.push_back({it.key, it.site});
}

void SiteTree::closed_ball(const geom::P3& anchor, const geom::Center& c, std::vector<u32>& interior,
                           std::vector<u32>& shell) const {
  interior.clear();
  shell.clear();
  if (root_ == kNone) return;
  double q[3];
  approx_center(anchor, c, q);
  const double r2 = approx_d2(q, anchor.x, anchor.y, anchor.z);
  std::vector<u32> stack{root_};
  while (!stack.empty()) {
    const u32 id = stack.back();
    stack.pop_back();
    const Node& nd = nodes_[id];
    if (box_dist2(nd, q) - kMargin > r2 + kMargin) continue;
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        const geom::P3 z{i64(cloud_.x[s]), i64(cloud_.y[s]), i64(cloud_.z[s])};
        const i128 k = geom::side_key(c, anchor, z);
        if (k < 0) interior.push_back(s);
        else if (k == 0) shell.push_back(s);
      }
    } else {
      stack.push_back(nd.right);
      stack.push_back(nd.left);
    }
  }
  std::sort(interior.begin(), interior.end());
  std::sort(shell.begin(), shell.end());
}

u64 SiteTree::kth_distance(i64 qx, i64 qy, i64 qz, u64 k) const {
  if (root_ == kNone || k == 0) return 0;
  // Max-tas des (distance, poids) courants dont la somme des poids couvre k ; best-first sur les noeuds.
  struct Cand {
    u64 d2;
    u32 w;
    bool operator<(const Cand& o) const { return d2 < o.d2; }
  };
  std::priority_queue<Cand> best;  // sommet = plus grande distance retenue
  u64 held = 0;                    // poids retenu
  auto bound = [&]() -> u64 { return held >= k ? best.top().d2 : ~u64{0}; };
  using QE = std::pair<u64, u32>;
  std::priority_queue<QE, std::vector<QE>, std::greater<QE>> open;
  open.push({box_dist2(nodes_[root_], qx, qy, qz), root_});
  while (!open.empty()) {
    const auto [bd, id] = open.top();
    open.pop();
    if (bd > bound()) break;
    const Node& nd = nodes_[id];
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        const i64 dx = i64(cloud_.x[s]) - qx, dy = i64(cloud_.y[s]) - qy, dz = i64(cloud_.z[s]) - qz;
        const u64 d2 = static_cast<u64>(dx * dx + dy * dy + dz * dz);
        if (held >= k && d2 >= best.top().d2) continue;
        best.push({d2, cloud_.w[s]});
        held += cloud_.w[s];
        // retirer le plus lointain tant que le reste couvre encore k
        while (held - best.top().w >= k) {
          held -= best.top().w;
          best.pop();
        }
      }
    } else {
      open.push({box_dist2(nodes_[nd.left], qx, qy, qz), nd.left});
      open.push({box_dist2(nodes_[nd.right], qx, qy, qz), nd.right});
    }
  }
  return held >= k ? best.top().d2 : ~u64{0};
}

void SiteTree::within(i64 qx, i64 qy, i64 qz, u64 r2, std::vector<u32>& out) const {
  out.clear();
  if (root_ == kNone) return;
  std::vector<u32> stack{root_};
  while (!stack.empty()) {
    const u32 id = stack.back();
    stack.pop_back();
    const Node& nd = nodes_[id];
    if (box_dist2(nd, qx, qy, qz) > r2) continue;
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        const i64 dx = i64(cloud_.x[s]) - qx, dy = i64(cloud_.y[s]) - qy, dz = i64(cloud_.z[s]) - qz;
        if (static_cast<u64>(dx * dx + dy * dy + dz * dz) <= r2) out.push_back(s);
      }
    } else {
      stack.push_back(nd.right);
      stack.push_back(nd.left);
    }
  }
  std::sort(out.begin(), out.end());
}

}  // namespace mhgp10
