// Oracle brut INDEPENDANT du catalogue critique (audit L05, 2 octobre 2026). Aucun code commun avec morsehgp3D_v10.
// Enumere tous les sous-ensembles de 2, 3, 4 positions, calcule le centre circonscrit dans aff(S) par les coordonnees
// barycentriques (systeme de Gram, Cramer, entiers exacts __int128), garde ceux dont toutes les barycentriques sont
// > 0 (centre dans l'interieur relatif), deduplique par (centre, rayon) rationnels reduits, recense sur tout le nuage.
// Admission (regle du CODE v10) : coquille sans site pondere : p + q_min <= K + 1 ; coquille ponderee : p <= K - 1.
// Domaine : petites etendues (differences < 2^7 : toutes les quantites < 2^118 en __int128), sinon refus. Sortie : une ligne canonique par boule admise :
//   q p u|I trie|U trie|S* (ordre de Morton)|num/den reduit
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
typedef __int128 I;
typedef long long L;
static I gcdI(I a, I b) { if (a < 0) a = -a; if (b < 0) b = -b; while (b) { I t = a % b; a = b; b = t; } return a; }
static std::string str(I v) { if (v == 0) return "0"; bool n = v < 0; if (n) v = -v; std::string s; while (v) { s.push_back(char('0' + int(v % 10))); v /= 10; } if (n) s.push_back('-'); return std::string(s.rbegin(), s.rend()); }
struct P { L x, y, z; };
static uint64_t spread(uint64_t v) { uint64_t r = 0; for (int b = 0; b < 21; ++b) r |= ((v >> b) & 1ull) << (3 * b); return r; }
struct Ball { I r2n, r2d; int qmin; std::vector<int> sup; };
int main(int argc, char** argv) {
  if (argc < 3) return 2;
  FILE* f = fopen(argv[1], "rb"); if (!f) return 2;
  const int K = atoi(argv[2]);
  std::vector<std::array<uint32_t, 3>> raw; uint32_t b[3];
  while (fread(b, 4, 3, f) == 3) raw.push_back({b[0], b[1], b[2]});
  fclose(f);
  // sites ponderes, ordre de Morton (x bit 0, y bit 1, z bit 2 de chaque triplet), ecrit independamment
  std::map<std::pair<uint64_t, std::array<uint32_t, 3>>, int> m;
  for (auto& r : raw) ++m[{spread(r[0]) | (spread(r[1]) << 1) | (spread(r[2]) << 2), r}];
  std::vector<P> X; std::vector<int> W; std::vector<std::array<uint32_t, 3>> A;
  uint32_t mn[3] = {~0u, ~0u, ~0u};
  for (auto& kv : m) for (int i = 0; i < 3; ++i) mn[i] = std::min(mn[i], kv.first.second[i]);
  for (auto& kv : m) { auto c = kv.first.second; X.push_back({L(c[0]) - mn[0], L(c[1]) - mn[1], L(c[2]) - mn[2]}); W.push_back(kv.second); A.push_back(c); }
  const int n = int(X.size());
  for (auto& p : X) if (p.x >= 128 || p.y >= 128 || p.z >= 128) { fprintf(stderr, "etendue trop grande pour l'oracle\n"); return 2; }
  // cle : centre absolu (Cx, Cy, Cz) / D reduit
  std::map<std::array<I, 6>, Ball> balls;
  auto add = [&](I cx, I cy, I cz, I D, const std::vector<int>& S) {
    // rayon carre = |C - a D|^2 / D^2
    const P& a = X[S[0]];
    I nx = cx - I(a.x) * D, ny = cy - I(a.y) * D, nz = cz - I(a.z) * D;
    I num = nx * nx + ny * ny + nz * nz, den = D * D;
    I g = gcdI(gcdI(gcdI(cx, cy), cz), D);
    I g2 = gcdI(num, den);
    std::array<I, 6> key = {cx / g, cy / g, cz / g, D / g, num / g2, den / g2};  // (centre, rayon carre) reduits
    auto it = balls.find(key);
    if (it == balls.end()) { balls[key] = Ball{num / g2, den / g2, int(S.size()), S}; return; }
    Ball& B = it->second;
    if (int(S.size()) < B.qmin || (int(S.size()) == B.qmin && S < B.sup)) { B.qmin = int(S.size()); B.sup = S; }
  };
  auto dot = [](const P& u, const P& v) { return I(u.x) * v.x + I(u.y) * v.y + I(u.z) * v.z; };
  auto sub = [](const P& u, const P& v) { return P{u.x - v.x, u.y - v.y, u.z - v.z}; };
  unsigned long long n2 = 0, n3 = 0, n4 = 0;
  for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) {
    // q = 2 : milieu, barycentriques (1/2, 1/2)
    add(I(X[i].x) + X[j].x, I(X[i].y) + X[j].y, I(X[i].z) + X[j].z, 2, {i, j}); ++n2;
    for (int k = j + 1; k < n; ++k) {
      const P u = sub(X[j], X[i]), v = sub(X[k], X[i]);
      const I uu = dot(u, u), vv = dot(v, v), uv = dot(u, v);
      const I det = uu * vv - uv * uv;  // Gram 2x2, > 0 ssi non alignes (Cauchy-Schwarz)
      if (det != 0) {
        // G (al, be)^T = (uu/2, vv/2) : al = vv (uu - uv) / (2 det), be = uu (vv - uv) / (2 det)
        const I al = vv * (uu - uv), be = uu * (vv - uv), D = 2 * det;
        if (al > 0 && be > 0 && D - al - be > 0) {
          add(I(X[i].x) * D + al * u.x + be * v.x, I(X[i].y) * D + al * u.y + be * v.y, I(X[i].z) * D + al * u.z + be * v.z, D, {i, j, k}); ++n3;
        }
      }
      for (int l = k + 1; l < n; ++l) {
        const P s = sub(X[l], X[i]);
        const I ss = dot(s, s), us = dot(u, s), vs = dot(v, s);
        // Gram 3x3 [[uu uv us][uv vv vs][us vs ss]], second membre (uu, vv, ss) / 2 ; Cramer
        const I d3 = uu * (vv * ss - vs * vs) - uv * (uv * ss - vs * us) + us * (uv * vs - vv * us);
        if (d3 == 0) continue;  // coplanaires (ou alignes)
        const I l1 = uu * (vv * ss - vs * vs) - uv * (vv * ss - vs * ss) + us * (vv * vs - vv * ss);
        const I l2 = uu * (vv * ss - vs * ss) - uu * (uv * ss - vs * us) + us * (uv * ss - vv * us);
        const I l3 = uu * (vv * ss - vs * vv) - uv * (uv * ss - vv * us) + uu * (uv * vs - vv * us);
        const I D = 2 * d3;  // d3 > 0 (Gram defini positif)
        if (l1 > 0 && l2 > 0 && l3 > 0 && D - l1 - l2 - l3 > 0) {
          add(I(X[i].x) * D + l1 * u.x + l2 * v.x + l3 * s.x, I(X[i].y) * D + l1 * u.y + l2 * v.y + l3 * s.y,
              I(X[i].z) * D + l1 * u.z + l2 * v.z + l3 * s.z, D, {i, j, k, l}); ++n4;
        }
      }
    }
  }
  unsigned long long out = 0;
  for (auto& kv : balls) {
    const auto& key = kv.first; const Ball& B = kv.second;
    const I D = key[3];
    std::vector<int> In, Sh; long long p = 0, u = 0; bool weighted = false;
    // rayon : |X[sup0] D - C|^2
    const P& a = X[B.sup[0]];
    const I ax = I(a.x) * D - key[0], ay = I(a.y) * D - key[1], az = I(a.z) * D - key[2];
    const I rr = ax * ax + ay * ay + az * az;
    for (int z = 0; z < n; ++z) {
      const I dx = I(X[z].x) * D - key[0], dy = I(X[z].y) * D - key[1], dz = I(X[z].z) * D - key[2];
      const I dd = dx * dx + dy * dy + dz * dz;
      if (dd < rr) { In.push_back(z); p += W[z]; }
      else if (dd == rr) { Sh.push_back(z); u += W[z]; if (W[z] > 1) weighted = true; }
    }
    const bool admit = weighted ? p <= K - 1 : p + B.qmin <= K + 1;
    if (!admit) continue;
    ++out;
    auto coord = [&](int s) { char buf[64]; snprintf(buf, sizeof buf, "%u,%u,%u", A[s][0], A[s][1], A[s][2]); return std::string(buf); };
    auto sorted = [&](const std::vector<int>& v) { std::vector<std::array<uint32_t, 3>> c; for (int s : v) c.push_back(A[s]); std::sort(c.begin(), c.end()); std::string r; for (auto& t : c) { char buf[64]; snprintf(buf, sizeof buf, " %u,%u,%u", t[0], t[1], t[2]); r += buf; } return r; };
    std::string S; for (int s : B.sup) S += " " + coord(s);
    printf("%d %lld %lld|%s|%s|%s|%s/%s\n", B.qmin, p, u, sorted(In).c_str(), sorted(Sh).c_str(), S.c_str(), str(B.r2n).c_str(), str(B.r2d).c_str());
  }
  fprintf(stderr, "oracle : sites %d, supports q2 %llu q3 %llu q4 %llu, spheres %zu, admises %llu\n", n, n2, n3, n4, balls.size(), out);
  return 0;
}
