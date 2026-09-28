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
constexpr double kMargin = 0.02;  // deux fois une borne large de l'erreur absolue des distances carrees approchees (< 1e-3 en u18)

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

namespace {
// Pile et tampons par fil : aucune allocation par requete en regime etabli.
struct QueryScratch {
  std::vector<std::pair<double, u32>> cand;  // (distance approchee, site)
  std::vector<u32> stack;
  double best[64];
  int nbest = 0;
};
thread_local QueryScratch tls;

// Insere d dans les `count` plus petites distances approchees (tableau trie croissant, count <= 64).
inline void push_best(QueryScratch& q, double d, u32 count) {
  int i;
  if (q.nbest < static_cast<int>(count)) {
    i = q.nbest++;
  } else {
    if (d >= q.best[count - 1]) return;
    i = static_cast<int>(count) - 1;
  }
  while (i > 0 && q.best[i - 1] > d) {
    q.best[i] = q.best[i - 1];
    --i;
  }
  q.best[i] = d;
}
}  // namespace

void SiteTree::nearest(const geom::P3& anchor, const geom::Center& c, u32 count,
                       std::vector<std::pair<i128, u32>>& out) const {
  // Exactitude : l'erreur des distances carrees approchees est < kMargin / 2. Soit W la count-ieme plus
  // petite distance approchee ; tout site parmi les count plus proches (ex aequo compris) a une distance
  // approchee <= W + kMargin : on les collecte tous, puis on trie exactement (cle i128, puis indice).
  out.clear();
  if (root_ == kNone || count == 0) return;
  if (count > 64) count = 64;
  QueryScratch& q = tls;
  q.cand.clear();
  q.nbest = 0;
  double cq[3];
  approx_center(anchor, c, cq);
  auto bound = [&]() { return q.nbest == static_cast<int>(count) ? q.best[count - 1] + kMargin : 1e300; };
  q.stack.clear();
  q.stack.push_back(root_);
  while (!q.stack.empty()) {
    const u32 id = q.stack.back();
    q.stack.pop_back();
    const Node& nd = nodes_[id];
    if (box_dist2(nd, cq) > bound()) continue;
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        const double d = approx_d2(cq, cloud_.x[s], cloud_.y[s], cloud_.z[s]);
        if (d > bound()) continue;
        push_best(q, d, count);
        q.cand.push_back({d, s});
      }
    } else {
      const double dl = box_dist2(nodes_[nd.left], cq), dr = box_dist2(nodes_[nd.right], cq);
      if (dl <= dr) {  // le plus proche en dernier : depile en premier
        q.stack.push_back(nd.right);
        q.stack.push_back(nd.left);
      } else {
        q.stack.push_back(nd.left);
        q.stack.push_back(nd.right);
      }
    }
  }
  const double W = bound();
  for (const auto& [d, site] : q.cand) {
    if (d > W) continue;
    const geom::P3 z{i64(cloud_.x[site]), i64(cloud_.y[site]), i64(cloud_.z[site])};
    out.push_back({geom::side_key(c, anchor, z), site});
  }
  std::sort(out.begin(), out.end());
  if (out.size() > count) out.resize(count);
}

void SiteTree::closed_ball(const geom::P3& anchor, const geom::Center& c, std::vector<u32>& interior,
                           std::vector<u32>& shell) const {
  interior.clear();
  shell.clear();
  if (root_ == kNone) return;
  double cq[3];
  approx_center(anchor, c, cq);
  const double r2 = approx_d2(cq, anchor.x, anchor.y, anchor.z) + kMargin;
  QueryScratch& q = tls;
  q.stack.clear();
  q.stack.push_back(root_);
  while (!q.stack.empty()) {
    const u32 id = q.stack.back();
    q.stack.pop_back();
    const Node& nd = nodes_[id];
    if (box_dist2(nd, cq) > r2) continue;
    if (nd.left == kNone) {
      for (u32 s = nd.lo; s < nd.hi; ++s) {
        if (approx_d2(cq, cloud_.x[s], cloud_.y[s], cloud_.z[s]) > r2) continue;
        const geom::P3 z{i64(cloud_.x[s]), i64(cloud_.y[s]), i64(cloud_.z[s])};
        const i128 k = geom::side_key(c, anchor, z);
        if (k < 0) interior.push_back(s);
        else if (k == 0) shell.push_back(s);
      }
    } else {
      q.stack.push_back(nd.right);
      q.stack.push_back(nd.left);
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
