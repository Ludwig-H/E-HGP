// Copies extraites mecaniquement de a10605a06 (tower.cpp, head.cpp, dendrogram.cpp), renommees ref_*.
namespace mhgp10 {
static double ref_lambda_of(double level, double z) {
  if (level <= 0) return std::numeric_limits<double>::infinity();
  return std::pow(level, -0.5 * z);
}

static PointDendrogram ref_point_dendrogram(const Catalogue& cat, const OrderForest& f, const Cloud& cloud) {
  // Table de niveaux fusionnee des noeuds et des points, dans l'ordre exact des niveaux.
  //  - Cles du catalogue (noeuds ; points en entree cover) : rang r, de niveau 0 si r = 0 et cat.level[r - 1] sinon.
  //    Les niveaux du catalogue croissent strictement avec le rang : le tri par denombrement des rangs est exact.
  //  - Cles entieres (points en entree core) : D_K(x), triees comme entiers.
  //  Les deux suites sont fusionnees par comparaison exacte, avec un raccourci double quand les approximations
  //  s'ecartent de plus de 1e-9 en relatif, tres au-dessus de l'erreur d'arrondi de approx(). Les cles exactement
  //  egales recoivent le meme rang : leur ordre relatif ne change pas la sortie.
  const u32 nn = static_cast<u32>(f.rank.size());
  const u32 n = cloud.sites();
  const bool cover = !f.point_cat_rank.empty();
  auto cat_level = [&](u32 r) -> geom::Level {
    if (r == 0) return geom::Level{arith::I192{}, arith::I128w::from_u128(1)};
    return cat.level[r - 1];
  };
  auto int_level = [](u64 e) {
    geom::Level L;
    L.num = arith::I192::from_u128(e);
    L.den = arith::I128w::from_u128(1);
    return L;
  };
  auto near = [](double x, double y) { return x == y || std::abs(x - y) <= 1e-9 * std::max(x, y); };
  // A : cles du catalogue (0..nn-1 : noeuds ; nn.. : points en entree cover), par rang croissant
  const u32 na = nn + (cover ? n : 0);
  auto rank_of = [&](u32 i) { return i < nn ? f.rank[i] : f.point_cat_rank[i - nn]; };
  u32 rmax = 0;
  for (u32 i = 0; i < na; ++i) rmax = std::max(rmax, rank_of(i));
  std::vector<u32> start(u64(rmax) + 2, 0), A(na);
  for (u32 i = 0; i < na; ++i) ++start[u64(rank_of(i)) + 1];
  for (u64 r = 0; r <= rmax; ++r) start[r + 1] += start[r];
  for (u32 i = 0; i < na; ++i) A[start[rank_of(i)]++] = i;
  // B : points en entree core, par D_K croissant
  std::vector<u32> B;
  if (!cover) {
    B.resize(n);
    for (u32 x = 0; x < n; ++x) B[x] = x;
    std::sort(B.begin(), B.end(), [&](u32 a, u32 b) { return f.point_level[a] < f.point_level[b]; });
  }
  u32 cached = kNone;
  double cached_x = 0;
  auto x_rank = [&](u32 r) {  // approximation d'un niveau du catalogue, une fois par rang (A est trie par rang)
    if (r != cached) {
      cached = r;
      cached_x = cat_level(r).approx();
    }
    return cached_x;
  };
  std::vector<u32> merged(u64(nn) + n);
  PointDendrogram d;
  u64 ia = 0, ib = 0;
  bool first = true, prev_cat = false;
  u32 prev_r = 0;
  u64 prev_e = 0;
  double prev_x = 0;
  while (ia < A.size() || ib < B.size()) {
    bool take_a = ib == B.size();
    if (!take_a && ia < A.size()) {
      const u32 r = rank_of(A[ia]);
      const u64 e = f.point_level[B[ib]];
      const double xa = x_rank(r), xb = int_level(e).approx();
      take_a = !near(xa, xb) ? xa < xb : geom::compare(cat_level(r), int_level(e)) <= 0;
    }
    u32 slot, r = 0;
    u64 e = 0;
    double x;
    if (take_a) {
      slot = A[ia++];
      r = rank_of(slot);
      x = x_rank(r);
    } else {
      const u32 p = B[ib++];
      slot = nn + p;
      e = f.point_level[p];
      x = int_level(e).approx();
    }
    // niveaux publies en double, strictement croissants : deux niveaux exacts distincts dont les doubles coincident
    // (ou s'inversent d'un ulp) partagent un rang ; l'ordre exact des noeuds et des points est preserve
    // (regression tests/regression/test_level_collision.py)
    bool distinct = true;
    if (!first) {
      if (take_a && prev_cat) distinct = r != prev_r;
      else if (!take_a && !prev_cat) distinct = e != prev_e;
      else
        distinct = !near(prev_x, x) || geom::compare(prev_cat ? cat_level(prev_r) : int_level(prev_e),
                                                      take_a ? cat_level(r) : int_level(e)) != 0;
    }
    if (first || (distinct && x > d.level.back())) d.level.push_back(x);
    merged[slot] = static_cast<u32>(d.level.size() - 1);
    first = false;
    prev_cat = take_a;
    prev_r = r;
    prev_e = e;
    prev_x = x;
  }
  d.node_rank.resize(nn);
  for (u32 v = 0; v < nn; ++v) d.node_rank[v] = merged[v];
  d.parent = f.parent;
  d.child_off.assign(nn + 1, 0);
  // CSR des enfants depuis les parents (les enfants d'une fusion sont crees avant elle)
  std::vector<u32> cnt(nn, 0);
  for (u32 v = 0; v < nn; ++v)
    if (f.parent[v] != kNone) ++cnt[f.parent[v]];
  for (u32 v = 0; v < nn; ++v) d.child_off[v + 1] = d.child_off[v] + cnt[v];
  d.child_val.resize(d.child_off[nn]);
  std::vector<u32> fill(d.child_off.begin(), d.child_off.end() - 1);
  for (u32 v = 0; v < nn; ++v)
    if (f.parent[v] != kNone) d.child_val[fill[f.parent[v]]++] = v;
  d.point_node = f.point_node;
  d.point_rank.resize(n);
  d.point_weight.assign(n, 1);
  for (u32 x = 0; x < n; ++x) d.point_rank[x] = merged[nn + x];
  return d;
}

static CondensedTree ref_condense(const PointDendrogram& d, const ClusterParams& p) {
  const u32 n = d.nodes();
  // masse de chaque sous-arbre ; points directement attaches a chaque noeud (CSR)
  std::vector<u64> mass(n, 0);
  std::vector<u32> att_off(n + 1, 0), att(d.points());
  for (u32 x = 0; x < d.points(); ++x) ++att_off[d.point_node[x] + 1];
  for (u32 v = 0; v < n; ++v) att_off[v + 1] += att_off[v];
  {
    std::vector<u32> fill(att_off.begin(), att_off.end() - 1);
    for (u32 x = 0; x < d.points(); ++x) att[fill[d.point_node[x]]++] = x;
  }
  for (u32 v = 0; v < n; ++v) {  // enfants crees avant les parents
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) mass[v] += d.point_weight[att[j]];
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) mass[v] += mass[d.child_val[j]];
  }
  u32 root = kNone;
  for (u32 v = 0; v < n; ++v)
    if (d.parent[v] == kNone) root = v;

  CondensedTree t;
  t.point_cluster.assign(d.points(), kNone);
  t.node_cluster.assign(n, kNone);
  t.point_lambda.assign(d.points(), 0.0);
  auto new_cluster = [&](u32 parent, double birth, u64 m) {
    t.parent.push_back(parent);
    t.birth.push_back(birth);
    t.stability.push_back(0.0);
    t.mass.push_back(m);
    return static_cast<u32>(t.parent.size() - 1);
  };
  // sortie de tous les points d'un sous-arbre au lambda donne
  std::vector<u32> stack;
  auto drop_subtree = [&](u32 v, u32 c, double lam) {
    stack.assign(1, v);
    while (!stack.empty()) {
      const u32 u = stack.back();
      stack.pop_back();
      t.node_cluster[u] = c;
      for (u32 j = att_off[u]; j < att_off[u + 1]; ++j) {
        const u32 x = att[j];
        t.point_cluster[x] = c;
        t.point_lambda[x] = lam;
        t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
      }
      for (u32 j = d.child_off[u]; j < d.child_off[u + 1]; ++j) stack.push_back(d.child_val[j]);
    }
  };
  // travail : (noeud, cluster courant)
  std::vector<std::pair<u32, u32>> work;
  work.push_back({root, new_cluster(kNone, 0.0, mass[root])});
  std::vector<u32> big;
  while (!work.empty()) {
    auto [v, c] = work.back();
    work.pop_back();
    t.node_cluster[v] = c;
    // points attaches directement : sortent a leur niveau d'entree
    for (u32 j = att_off[v]; j < att_off[v + 1]; ++j) {
      const u32 x = att[j];
      const double lam = ref_lambda_of(d.level[d.point_rank[x]], p.z);
      t.point_cluster[x] = c;
      t.point_lambda[x] = lam;
      t.stability[c] += d.point_weight[x] * (lam - t.birth[c]);
    }
    if (d.child_off[v] == d.child_off[v + 1]) continue;
    const double lam = ref_lambda_of(d.level[d.node_rank[v]], p.z);
    big.clear();
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j)
      if (mass[d.child_val[j]] >= p.min_cluster_size) big.push_back(d.child_val[j]);
    if (big.size() >= 2) {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (mass[u] >= p.min_cluster_size) {
          t.stability[c] += double(mass[u]) * (lam - t.birth[c]);
          work.push_back({u, new_cluster(c, lam, mass[u])});
        } else {
          drop_subtree(u, c, lam);
        }
      }
    } else {
      for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
        const u32 u = d.child_val[j];
        if (big.size() == 1 && u == big[0]) work.push_back({u, c});
        else drop_subtree(u, c, lam);
      }
    }
  }
  return t;
}

static Outcome ref_validate(const PointDendrogram& d) {
  const u32 n = d.nodes();
  MHGP10_CHECK(d.child_off.size() == u64(n) + 1 && d.child_off[0] == 0, csr_bounds);
  MHGP10_CHECK(d.child_off[n] == d.child_val.size() && d.parent.size() == n, csr_bounds);
  for (u64 i = 1; i < d.level.size(); ++i) MHGP10_CHECK(d.level[i - 1] < d.level[i], rank_order);
  u32 roots = 0;
  for (u32 v = 0; v < n; ++v) {
    MHGP10_CHECK(d.child_off[v] <= d.child_off[v + 1], csr_bounds);
    MHGP10_CHECK(d.node_rank[v] < d.level.size(), csr_bounds);
    for (u32 j = d.child_off[v]; j < d.child_off[v + 1]; ++j) {
      const u32 c = d.child_val[j];
      MHGP10_CHECK(c < v, rank_order);  // enfants crees avant
      MHGP10_CHECK(d.parent[c] == v, csr_bounds);
      MHGP10_CHECK(d.node_rank[c] <= d.node_rank[v], rank_order);
    }
    if (d.parent[v] == kNone) ++roots;
  }
  MHGP10_CHECK(roots == 1, root_count);
  MHGP10_CHECK(d.point_rank.size() == d.points() && d.point_weight.size() == d.points(), csr_bounds);
  for (u32 x = 0; x < d.points(); ++x) {
    const u32 v = d.point_node[x];
    MHGP10_CHECK(v < n && d.point_weight[x] >= 1, csr_bounds);
    MHGP10_CHECK(d.point_rank[x] >= d.node_rank[v], rank_order);
    if (d.parent[v] != kNone) MHGP10_CHECK(d.point_rank[x] <= d.node_rank[d.parent[v]], rank_order);
  }
  return Outcome{};
}

static Clustering ref_cluster(const PointDendrogram& d, const ClusterParams& p) {
  Clustering out;
  out.tree = ref_condense(d, p);
  const CondensedTree& t = out.tree;
  const u32 m = static_cast<u32>(t.parent.size());
  // enfants de chaque cluster (les enfants ont un indice superieur au parent)
  std::vector<std::vector<u32>> kids(m);
  for (u32 c = 1; c < m; ++c) kids[t.parent[c]].push_back(c);
  std::vector<unsigned char> chosen(m, 0);
  if (p.selection == Selection::eom) {
    std::vector<double> best(m, 0.0);
    for (u32 c = m; c-- > 0;) {
      double sub = 0;
      for (u32 k : kids[c]) sub += best[k];
      const bool is_root = t.parent[c] == kNone;
      if (kids[c].empty()) {
        best[c] = t.stability[c];
        chosen[c] = 1;
      } else if (is_root && !p.allow_single_cluster) {
        best[c] = sub;
      } else if (sub > t.stability[c]) {
        best[c] = sub;
      } else {
        best[c] = t.stability[c];
        chosen[c] = 1;
      }
    }
    // un cluster retenu desactive ses descendants
    for (u32 c = 0; c < m; ++c) {
      if (!chosen[c]) continue;
      for (u32 a = t.parent[c]; a != kNone; a = t.parent[a])
        if (chosen[a]) {
          chosen[c] = 0;
          break;
        }
    }
  } else {
    for (u32 c = 0; c < m; ++c) chosen[c] = kids[c].empty();
  }
  if (!p.allow_single_cluster && m > 0 && m > 1) chosen[0] = 0;
  if (!p.allow_single_cluster && m == 1) chosen[0] = 0;
  std::vector<i32> id(m, -1);
  for (u32 c = 0; c < m; ++c)
    if (chosen[c]) {
      id[c] = static_cast<i32>(out.selected.size());
      out.selected.push_back(c);
    }
  out.cluster_label.assign(m, -1);
  for (u32 c = 0; c < m; ++c)
    for (u32 a = c; a != kNone; a = t.parent[a])
      if (chosen[a]) {
        out.cluster_label[c] = id[a];
        break;
      }
  out.label.assign(d.points(), -1);
  for (u32 x = 0; x < d.points(); ++x) {
    for (u32 c = t.point_cluster[x]; c != kNone; c = t.parent[c])
      if (chosen[c]) {
        out.label[x] = id[c];
        break;
      }
  }
  return out;
}
}  // namespace mhgp10
