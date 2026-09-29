// Portes unitaires des fondations : entiers larges (juge a arithmetique decimale independante),
// ordonnanceur (couverture exacte, determinisme, imbrication serialisee), statuts et tampons.
// Codes : 0 conforme, 1 desaccord d'un juge, 3 plancher non atteint.
#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "arith/wide.hpp"
#include "cloud/cloud.hpp"
#include "cloud/site_tree.hpp"
#include "core/buffer.hpp"
#include "core/status.hpp"
#include "sched/pool.hpp"

using namespace mhgp10;

namespace {

int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    ++failures;
    std::printf("ECHEC %s\n", what);
  }
}

// ---- juge decimal : chaines de chiffres, arithmetique d'ecole (algorithme volontairement autre) ----
std::string dec_abs(const std::string& s) { return s[0] == '-' ? s.substr(1) : s; }
int dec_cmp_abs(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  return a < b ? -1 : (a > b ? 1 : 0);
}
std::string dec_strip(std::string s) {
  size_t i = 0;
  while (i + 1 < s.size() && s[i] == '0') ++i;
  return s.substr(i);
}
std::string dec_add_abs(const std::string& a, const std::string& b) {
  std::string r;
  int carry = 0;
  for (int i = int(a.size()) - 1, j = int(b.size()) - 1; i >= 0 || j >= 0 || carry; --i, --j) {
    int d = carry + (i >= 0 ? a[i] - '0' : 0) + (j >= 0 ? b[j] - '0' : 0);
    r.push_back(char('0' + d % 10));
    carry = d / 10;
  }
  return dec_strip(std::string(r.rbegin(), r.rend()));
}
std::string dec_sub_abs(const std::string& a, const std::string& b) {  // |a| >= |b|
  std::string r;
  int borrow = 0;
  for (int i = int(a.size()) - 1, j = int(b.size()) - 1; i >= 0; --i, --j) {
    int d = (a[i] - '0') - borrow - (j >= 0 ? b[j] - '0' : 0);
    borrow = d < 0;
    if (d < 0) d += 10;
    r.push_back(char('0' + d));
  }
  return dec_strip(std::string(r.rbegin(), r.rend()));
}
std::string dec_add(const std::string& a, const std::string& b) {
  const bool na = a[0] == '-', nb = b[0] == '-';
  const std::string A = dec_abs(a), B = dec_abs(b);
  std::string m;
  bool neg;
  if (na == nb) {
    m = dec_add_abs(A, B);
    neg = na;
  } else if (dec_cmp_abs(A, B) >= 0) {
    m = dec_sub_abs(A, B);
    neg = na;
  } else {
    m = dec_sub_abs(B, A);
    neg = nb;
  }
  return (neg && m != "0" ? "-" : "") + m;
}
std::string dec_mul(const std::string& a, const std::string& b) {
  const std::string A = dec_abs(a), B = dec_abs(b);
  std::vector<int> r(A.size() + B.size(), 0);
  for (int i = int(A.size()) - 1; i >= 0; --i)
    for (int j = int(B.size()) - 1; j >= 0; --j) r[i + j + 1] += (A[i] - '0') * (B[j] - '0');
  for (int k = int(r.size()) - 1; k > 0; --k) {
    r[k - 1] += r[k] / 10;
    r[k] %= 10;
  }
  std::string s;
  for (int d : r) s.push_back(char('0' + d));
  s = dec_strip(s);
  const bool neg = (a[0] == '-') != (b[0] == '-');
  return (neg && s != "0" ? "-" : "") + s;
}

i128 random_i128(std::mt19937_64& g) {
  const int bits = int(g() % 127);
  u128 v = (u128(g()) << 64) | g();
  v = bits == 0 ? 0 : (v >> (128 - bits));
  return (g() & 1) ? -static_cast<i128>(v) : static_cast<i128>(v);
}

void test_wide() {
  using namespace mhgp10::arith;
  std::mt19937_64 g(20260928);
  u64 checks = 0;
  for (int t = 0; t < 20000; ++t) {
    const i128 x = random_i128(g), y = random_i128(g), z = random_i128(g);
    const auto X = I192::from_i128(x), Y = I192::from_i128(y);
    const auto P = mul(X, Y);                          // Wide<6>
    const auto Z = I128w::from_i128(z);
    const auto Q = mul(P, Z);                          // Wide<8>
    const std::string sx = to_string(X), sy = to_string(Y);
    expect(to_string(P) == dec_mul(sx, sy), "mul");
    expect(to_string(Q) == dec_mul(dec_mul(sx, sy), to_string(Z)), "mul triple");
    I192 S;
    if (add(X, Y, S)) expect(to_string(S) == dec_add(sx, sy), "add");
    I192 D;
    if (sub(X, Y, D)) expect(to_string(D) == dec_add(sx, (sy[0] == '-' ? sy.substr(1) : (sy == "0" ? sy : "-" + sy))), "sub");
    const int c = cmp(X, Y);
    const int cr = (x < y) ? -1 : (x > y ? 1 : 0);
    expect(c == cr, "cmp");
    // produits croises : comparaison de fractions x/1 * y vs y * x
    expect(cmp(mul(X, Y), mul(Y, X)) == 0, "commutativite");
    checks += 5;
  }
  // debordement detecte
  I192 big;
  big.w[2] = ~u64{0};
  big.w[1] = ~u64{0};
  big.w[0] = ~u64{0};
  I192 out;
  expect(!add(big, I192::from_i128(1), out), "debordement add");
  std::printf("wide_checks %llu\n", static_cast<unsigned long long>(checks));
  if (checks < 100000) {
    std::printf("PLANCHER wide_checks\n");
    failures += 1000;
  }
}

void test_pool() {
  for (unsigned threads : {1u, 2u, 5u, 8u}) {
    sched::Pool pool(threads);
    const u64 n = 1000003;
    std::vector<u64> out(n, 0);
    pool.parallel_for(n, 777, [&](u64 b, u64 e, unsigned) {
      for (u64 i = b; i < e; ++i) out[i] = i * i + 1;
    });
    bool ok = true;
    for (u64 i = 0; i < n; ++i) ok &= out[i] == i * i + 1;
    expect(ok, "pool couverture");
    // imbrication : serialisee et comptee
    std::vector<u64> acc(64, 0);
    pool.parallel_for(64, 1, [&](u64 b, u64, unsigned) {
      u64 s = 0;
      pool.parallel_for(100, 7, [&](u64 bb, u64 ee, unsigned) {
        for (u64 i = bb; i < ee; ++i) s += i;
      });
      acc[b] = s;
    });
    bool ok2 = true;
    for (u64 v : acc) ok2 &= v == 4950;
    expect(ok2, "pool imbrique");
    if (threads > 1) expect(pool.nested_calls() == 64, "pool imbrication comptee");
    // reutilisation repetee
    for (int rep = 0; rep < 200; ++rep) {
      std::vector<u32> v(1000, 0);
      pool.parallel_for(1000, 3, [&](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) v[i] = u32(i);
      });
      bool ok3 = true;
      for (u32 i = 0; i < 1000; ++i) ok3 &= v[i] == i;
      expect(ok3, "pool reutilisation");
    }
  }
}

void test_status_and_buffer() {
  expect(status_of(Reason::none) == Status::ok, "statut none");
  expect(status_of(Reason::coordinate_out_of_domain) == Status::invalid_input, "statut entree");
  expect(status_of(Reason::shell_quotient_budget) == Status::unsupported_degeneracy, "statut degenerescence");
  expect(status_of(Reason::multiplicity_unsupported) == Status::unsupported_degeneracy, "statut multiplicite");
  expect(fail(Reason::root_count, 2).precedes(fail(Reason::arith_guard, 3)), "priorite plus petit K");
  MemoryBudget budget(1000);
  Buffer<u64> a;
  expect(a.allocate(100, budget), "budget 800 o");
  Buffer<u64> b;
  expect(!b.allocate(100, budget), "budget refuse");
  a.reset();
  expect(budget.used() == 0 && budget.peak() == 800, "budget libere");
  Csr<u32> c;
  expect(c.off.allocate(3) && c.val.allocate(2), "csr alloc");
  c.off[0] = 0;
  c.off[1] = 1;
  c.off[2] = 2;
  expect(c.well_formed() && c.row(1).size() == 1, "csr");
}

void test_cloud() {
  std::mt19937_64 g(7);
  const u32 n = 5000;
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = u32(g() % 64);  // grille grossiere : doublons nombreux
    y[i] = u32(g() % 64);
    z[i] = u32(g() % 8);
    pid[i] = 1000000u + 7u * i;
  }
  auto a = prepare_cloud(x, y, z, pid, 18);
  expect(a.ok(), "cloud ok");
  // permutation de l'entree : memes sites, memes poids, memes listes de PointId
  std::vector<u32> perm(n);
  for (u32 i = 0; i < n; ++i) perm[i] = i;
  std::shuffle(perm.begin(), perm.end(), g);
  std::vector<u32> x2(n), y2(n), z2(n), p2(n);
  for (u32 i = 0; i < n; ++i) {
    x2[i] = x[perm[i]];
    y2[i] = y[perm[i]];
    z2[i] = z[perm[i]];
    p2[i] = pid[perm[i]];
  }
  auto b = prepare_cloud(x2, y2, z2, p2, 18);
  expect(b.ok(), "cloud permute ok");
  const Cloud& A = a.value();
  const Cloud& B = b.value();
  bool same = A.sites() == B.sites() && A.weight == n;
  u64 wsum = 0;
  for (u32 s = 0; same && s < A.sites(); ++s) {
    same &= A.x[s] == B.x[s] && A.y[s] == B.y[s] && A.z[s] == B.z[s] && A.w[s] == B.w[s];
    same &= A.ids.row(s).size() == A.w[s];
    for (u32 t = 0; same && t < A.w[s]; ++t) same &= A.ids.row(s)[t] == B.ids.row(s)[t];
    if (s > 0) same &= morton3(A.x[s - 1], A.y[s - 1], A.z[s - 1]) < morton3(A.x[s], A.y[s], A.z[s]);
    wsum += A.w[s];
  }
  expect(same && wsum == n && A.ids.well_formed(), "cloud permutation / multiplicites");
  expect(A.sites() < n, "cloud doublons regroupes");
  // refus
  std::vector<u32> bad = x;
  bad[3] = 1u << 18;
  expect(prepare_cloud(bad, y, z, pid, 18).outcome().reason == Reason::coordinate_out_of_domain, "cloud domaine");
  std::vector<u32> dup = pid;
  dup[5] = dup[6];
  expect(prepare_cloud(x, y, z, dup, 18).outcome().reason == Reason::duplicate_point_id, "cloud pid double");
  expect(prepare_cloud({}, {}, {}, {}, 18).outcome().reason == Reason::empty_input, "cloud vide");
}

void test_site_tree() {
  std::mt19937_64 g(11);
  u64 checks = 0;
  for (int t = 0; t < 30; ++t) {
    const u32 n = 50 + u32(g() % 1500);
    const u32 span = (t % 3 == 0) ? 16 : 5000;  // grille grossiere (doublons) ou fine
    std::vector<u32> x(n), y(n), z(n), pid(n);
    for (u32 i = 0; i < n; ++i) {
      x[i] = u32(g() % span);
      y[i] = u32(g() % span);
      z[i] = u32(g() % span);
      pid[i] = i;
    }
    auto r = prepare_cloud(x, y, z, pid, 18);
    if (!r.ok()) { expect(false, "tree cloud"); continue; }
    const Cloud& c = r.value();
    SiteTree tree(c);
    std::vector<u32> got;
    for (int q = 0; q < 40; ++q) {
      const i64 qx = i64(g() % span), qy = i64(g() % span), qz = i64(g() % span);
      std::vector<std::pair<u64, u32>> all;
      for (u32 s = 0; s < c.sites(); ++s) {
        const i64 dx = i64(c.x[s]) - qx, dy = i64(c.y[s]) - qy, dz = i64(c.z[s]) - qz;
        all.push_back({u64(dx * dx + dy * dy + dz * dz), c.w[s]});
      }
      std::sort(all.begin(), all.end());
      for (u64 k : {1ull, 2ull, 5ull, 11ull}) {
        u64 acc = 0, want = ~u64{0};
        for (auto& [d, w] : all) {
          acc += w;
          if (acc >= k) { want = d; break; }
        }
        expect(tree.kth_distance(qx, qy, qz, k) == want, "kth_distance");
        ++checks;
      }
      const u64 r2 = all[std::min<size_t>(all.size() - 1, 7)].first;
      tree.within(qx, qy, qz, r2, got);
      u32 cnt = 0;
      for (auto& [d, w] : all) cnt += d <= r2;
      expect(got.size() == cnt, "within");
      ++checks;
    }
  }
  std::printf("site_tree_checks %llu\n", static_cast<unsigned long long>(checks));
  if (checks < 5000) failures += 1000;
}


// Requetes a centre rationnel de SiteTree (nearest, closed_ball) contre la force brute exacte : centres de spheres
// passant par 1 a 4 sites (centre entier, formes q2, q3, q4 ; ancre sur la sphere), sur grilles grossieres
// (cospherite, ex aequo), moyennes et u18. La cle exacte decide ; l'arbre (elagage flottant a marge, bande exacte
// de la boule fermee) ne doit rien perdre ni rien ajouter.
void test_site_tree_rational() {
  std::mt19937_64 g(23);
  u64 checks = 0, shells = 0;
  for (int t = 0; t < 24; ++t) {
    const u32 n = 40 + u32(g() % 1200);
    const u32 span = (t % 3 == 0) ? 12 : ((t % 3 == 1) ? 400 : 250000);
    std::vector<u32> x(n), y(n), z(n), pid(n);
    for (u32 i = 0; i < n; ++i) {
      x[i] = u32(g() % span);
      y[i] = u32(g() % span);
      z[i] = u32(g() % span);
      pid[i] = i;
    }
    auto r = prepare_cloud(x, y, z, pid, 18);
    if (!r.ok()) {
      expect(false, "tree cloud");
      continue;
    }
    const Cloud& c = r.value();
    SiteTree tree(c);
    auto P = [&](u32 s) { return geom::P3{i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; };
    std::vector<std::pair<i128, u32>> got, want;
    std::vector<u32> gi, gu, wi, wu;
    for (int q = 0; q < 60; ++q) {
      const int arity = 1 + int(g() % 4);
      u32 s[4];
      for (int a = 0; a < 4; ++a) s[a] = u32(g() % c.sites());
      const geom::P3 anchor = P(s[0]);
      geom::Center ctr{{0, 0, 0}, 1};
      bool ok = true;
      if (arity == 2) geom::center2(P(s[0]), P(s[1]), ctr);
      else if (arity == 3) ok = geom::center3(P(s[0]), P(s[1]), P(s[2]), ctr);
      else if (arity == 4) ok = geom::center4(P(s[0]), P(s[1]), P(s[2]), P(s[3]), ctr);
      if (!ok) continue;
      want.clear();
      wi.clear();
      wu.clear();
      for (u32 zz = 0; zz < c.sites(); ++zz) {
        const i128 key = geom::side_key(ctr, anchor, P(zz));
        want.push_back({key, zz});
        if (key < 0) wi.push_back(zz);
        else if (key == 0) wu.push_back(zz);
      }
      std::sort(want.begin(), want.end());
      for (u32 count : {1u, 3u, 7u, 12u}) {
        tree.nearest(anchor, ctr, count, got);
        const size_t m = std::min<size_t>(count, want.size());
        bool same = got.size() == m;
        for (size_t i = 0; same && i < m; ++i) same = got[i] == want[i];
        expect(same, "nearest rationnel");
        ++checks;
      }
      tree.closed_ball(anchor, ctr, gi, gu);
      expect(gi == wi && gu == wu, "closed_ball rationnel");
      shells += wu.size() > 1;
      ++checks;
    }
  }
  std::printf("site_tree_rational_checks %llu shells %llu\n", static_cast<unsigned long long>(checks),
              static_cast<unsigned long long>(shells));
  if (checks < 4000 || shells < 300) failures += 1000;
}

}  // namespace

int main() {
  test_site_tree();
  test_site_tree_rational();
  test_cloud();
  test_wide();
  test_pool();
  test_status_and_buffer();
  if (failures) {
    std::printf("unit_failures %d\n", failures);
    return failures >= 1000 ? 3 : 1;
  }
  std::printf("unit_ok\n");
  return 0;
}
