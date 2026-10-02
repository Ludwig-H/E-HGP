#include "cloud/site_tree.hpp"

#include <algorithm>
#include <numeric>
#include <queue>

namespace mhgp10 {

SiteTree::SiteTree(const Cloud& cloud) : cloud_(cloud) {
  const u32 n = cloud.sites();
  site_.resize(n);
  std::iota(site_.begin(), site_.end(), 0u);
  nodes_.reserve(4 * (n / kLeaf + 1) + 4);
  if (n > 0) root_ = build(0, n);
  px_.resize(n);
  py_.resize(n);
  pz_.resize(n);
  for (u32 t = 0; t < n; ++t) {
    px_[t] = double(cloud.x[site_[t]]);
    py_[t] = double(cloud.y[site_[t]]);
    pz_[t] = double(cloud.z[site_[t]]);
  }
}

// Coupe mediane sur l'axe le plus etendu (departage : coordonnee puis indice de site, donc arbre deterministe).
u32 SiteTree::build(u32 lo, u32 hi) {
  const Buffer<u32>* c[3] = {&cloud_.x, &cloud_.y, &cloud_.z};
  u32 mn[3], mx[3];
  for (int a = 0; a < 3; ++a) mn[a] = mx[a] = (*c[a])[site_[lo]];
  for (u32 t = lo + 1; t < hi; ++t)
    for (int a = 0; a < 3; ++a) {
      const u32 v = (*c[a])[site_[t]];
      mn[a] = std::min(mn[a], v);
      mx[a] = std::max(mx[a], v);
    }
  const u32 id = static_cast<u32>(nodes_.size());
  Node nd{};
  for (int a = 0; a < 3; ++a) {
    nd.bmin[a] = double(mn[a]);
    nd.bmax[a] = double(mx[a]);
  }
  nd.lo = lo;
  nd.hi = hi;
  nd.left = nd.right = kNone;
  nodes_.push_back(nd);
  if (hi - lo > kLeaf) {
    int axis = 0;
    for (int a = 1; a < 3; ++a)
      if (mx[a] - mn[a] > mx[axis] - mn[axis]) axis = a;
    const u32 mid = lo + (hi - lo) / 2;
    const Buffer<u32>& ca = *c[axis];
    std::nth_element(site_.begin() + lo, site_.begin() + mid, site_.begin() + hi, [&](u32 p, u32 q) {
      return ca[p] != ca[q] ? ca[p] < ca[q] : p < q;
    });
    const u32 l = build(lo, mid);
    const u32 r = build(mid, hi);
    nodes_[id].left = l;
    nodes_[id].right = r;
  }
  return id;
}

namespace {
constexpr double kMargin = 0.02;  // deux fois une borne large de l'erreur absolue des distances carrees approchees (< 1e-3 en u18)

void approx_center(const geom::P3& a, const geom::Center& c, double q[3]) {
  const double D = static_cast<double>(c.D);
  q[0] = double(a.x) + static_cast<double>(c.N[0]) / D;
  q[1] = double(a.y) + static_cast<double>(c.N[1]) / D;
  q[2] = double(a.z) + static_cast<double>(c.N[2]) / D;
}

inline double approx_d2(const double q[3], double x, double y, double z) {
  const double dx = x - q[0], dy = y - q[1], dz = z - q[2];
  return dx * dx + dy * dy + dz * dz;
}

// Distance carree approchee d'un point a une boite. Pour un site de la boite, elle est <= sa distance approchee
// (arrondi IEEE monotone, memes operandes q) : l'elagage ne perd aucun site de distance approchee <= la borne.
inline double box_d2(const double bmin[3], const double bmax[3], const double q[3]) {
  double d = 0;
  for (int a = 0; a < 3; ++a) {
    double t = 0;
    if (q[a] < bmin[a]) t = bmin[a] - q[a];
    else if (q[a] > bmax[a]) t = q[a] - bmax[a];
    d += t * t;
  }
  return d;
}

inline u64 box_d2_exact(const double bmin[3], const double bmax[3], i64 qx, i64 qy, i64 qz) {
  const i64 q[3] = {qx, qy, qz};
  u64 d = 0;
  for (int a = 0; a < 3; ++a) {
    const i64 lo = static_cast<i64>(bmin[a]), hi = static_cast<i64>(bmax[a]);
    i64 t = 0;
    if (q[a] < lo) t = lo - q[a];
    else if (q[a] > hi) t = q[a] - hi;
    d += static_cast<u64>(t * t);
  }
  return d;
}

// Pile et tampons par fil : aucune allocation par requete en regime etabli.
struct QueryScratch {
  std::vector<std::pair<double, u32>> cand;  // (distance approchee, position dans l'ordre de l'arbre)
  std::vector<std::pair<double, u32>> stack;  // (distance approchee de la boite, noeud)
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
  // petite distance approchee (sur tous les sites) ; tout site parmi les count plus proches (ex aequo compris)
  // a une distance approchee <= W + kMargin. La borne d'elagage ne descend jamais sous W + kMargin, donc tous
  // ces sites sont examines ; on les collecte, puis on trie exactement (cle i128, puis indice).
  out.clear();
  if (root_ == kNone || count == 0) return;
  if (count > 64) count = 64;
  QueryScratch& q = tls;
  q.cand.clear();
  q.nbest = 0;
  double cq[3];
  approx_center(anchor, c, cq);
  double bound = 1e300;
  q.stack.clear();
  q.stack.push_back({box_d2(nodes_[root_].bmin, nodes_[root_].bmax, cq), root_});
  while (!q.stack.empty()) {
    const auto [bd, id] = q.stack.back();
    q.stack.pop_back();
    if (bd > bound) continue;
    const Node& nd = nodes_[id];
    if (nd.left == kNone) {
      for (u32 t = nd.lo; t < nd.hi; ++t) {
        const double d = approx_d2(cq, px_[t], py_[t], pz_[t]);
        if (d > bound) continue;
        push_best(q, d, count);
        q.cand.push_back({d, t});
        if (q.nbest == static_cast<int>(count)) bound = q.best[count - 1] + kMargin;
      }
    } else {
      const Node& l = nodes_[nd.left];
      const Node& r = nodes_[nd.right];
      const double dl = box_d2(l.bmin, l.bmax, cq), dr = box_d2(r.bmin, r.bmax, cq);
      if (dl <= dr) {  // le plus proche en dernier : depile en premier
        if (dr <= bound) q.stack.push_back({dr, nd.right});
        if (dl <= bound) q.stack.push_back({dl, nd.left});
      } else {
        if (dl <= bound) q.stack.push_back({dl, nd.left});
        if (dr <= bound) q.stack.push_back({dr, nd.right});
      }
    }
  }
  for (const auto& [d, t] : q.cand) {
    if (d > bound) continue;
    const u32 site = site_[t];
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
  // Erreur absolue de chaque distance carree approchee (site ou ancre) < kMargin / 2 : un site de distance
  // approchee > r2a + kMargin est exterieur, < r2a - kMargin strictement interieur ; seule la bande entre les deux
  // est decidee par la cle exacte.
  const double r2a = approx_d2(cq, double(anchor.x), double(anchor.y), double(anchor.z));
  const double r2 = r2a + kMargin, inner = r2a - kMargin;
  QueryScratch& q = tls;
  q.stack.clear();
  q.stack.push_back({box_d2(nodes_[root_].bmin, nodes_[root_].bmax, cq), root_});
  while (!q.stack.empty()) {
    const auto [bd, id] = q.stack.back();
    q.stack.pop_back();
    if (bd > r2) continue;
    const Node& nd = nodes_[id];
    if (nd.left == kNone) {
      for (u32 t = nd.lo; t < nd.hi; ++t) {
        const double d = approx_d2(cq, px_[t], py_[t], pz_[t]);
        if (d > r2) continue;
        const u32 s = site_[t];
        if (d < inner) {  // |z - c|^2 - r^2 < d - r2a + 2 * erreur < 0 : strictement interieur, sans calcul exact
          interior.push_back(s);
          continue;
        }
        const geom::P3 z{i64(cloud_.x[s]), i64(cloud_.y[s]), i64(cloud_.z[s])};
        const i128 k = geom::side_key(c, anchor, z);
        if (k < 0) interior.push_back(s);
        else if (k == 0) shell.push_back(s);
      }
    } else {
      const Node& l = nodes_[nd.left];
      const Node& r = nodes_[nd.right];
      const double dl = box_d2(l.bmin, l.bmax, cq), dr = box_d2(r.bmin, r.bmax, cq);
      if (dr <= r2) q.stack.push_back({dr, nd.right});
      if (dl <= r2) q.stack.push_back({dl, nd.left});
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
  open.push({box_d2_exact(nodes_[root_].bmin, nodes_[root_].bmax, qx, qy, qz), root_});
  while (!open.empty()) {
    const auto [bd, id] = open.top();
    open.pop();
    if (bd > bound()) break;
    const Node& nd = nodes_[id];
    if (nd.left == kNone) {
      for (u32 t = nd.lo; t < nd.hi; ++t) {
        const u32 s = site_[t];
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
      open.push({box_d2_exact(nodes_[nd.left].bmin, nodes_[nd.left].bmax, qx, qy, qz), nd.left});
      open.push({box_d2_exact(nodes_[nd.right].bmin, nodes_[nd.right].bmax, qx, qy, qz), nd.right});
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
    if (box_d2_exact(nd.bmin, nd.bmax, qx, qy, qz) > r2) continue;
    if (nd.left == kNone) {
      for (u32 t = nd.lo; t < nd.hi; ++t) {
        const u32 s = site_[t];
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
