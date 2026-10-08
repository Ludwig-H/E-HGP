// Proposition flottante de plus petite boule (LEV-MEB-CERT, MES-M3 adopte) : port explicite de
// microbancs/mes_m3_m4_tour/mes_m3/welzl_proposal.hpp (lui-meme port de DWelzl de la v10, src/tower/tower.cpp,
// l. 250-440, commit 777406b82), dans le module tower.
//
// Statut : PROPOSITION. Rien ne se decide ici : seul le support propose (indices dans la partie) sort ; il est certifie
// par LEM-T1 (table S* -> boule, S dans F, F dans P_b), ou en exact, ou abandonne pour le repli exact. Un echec
// (degenerescence flottante, tours epuises) rend ok = false. Repere local : coordonnees translatees par le premier site
// de la partie (differences entieres < 2^32, exactes en binaire64) ; tolerances et constantes de la v10 inchangees
// (1e-9, 1e-10, 8 tours) ; au plus 12 sites (K <= 12).
#pragma once

#include <array>

namespace mhgp12::tower_detail {

struct DBall {
  double c[3] = {0, 0, 0};
  double r2 = -1;
  int R[4] = {0, 0, 0, 0};
  int nr = 0;
};

class DWelzl {
 public:
  static constexpr int kCapacity = 12;
  double p[kCapacity][3];
  bool ok = true;

  DBall run(int n) {
    {
      const DBall B = run_support(n);
      if (ok) return B;
      ok = true;
    }
    // repli : Welzl a deplacement en tete, du plus loin au plus proche du barycentre
    double g[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) g[a] += p[i][a];
    for (int a = 0; a < 3; ++a) g[a] /= n;
    double key[kCapacity];
    for (int i = 0; i < n; ++i) {
      double d = 0;
      for (int a = 0; a < 3; ++a) d += (p[i][a] - g[a]) * (p[i][a] - g[a]);
      int j = i;
      while (j > 0 && key[j - 1] < d) {
        key[j] = key[j - 1];
        L[j] = L[j - 1];
        --j;
      }
      key[j] = d;
      L[j] = i;
    }
    ns = 0;
    mtf(n);
    return ball;
  }

 private:
  DBall through(const int* R, int nr) {
    DBall B;
    B.nr = nr;
    for (int i = 0; i < nr; ++i) B.R[i] = R[i];
    if (nr == 0) return B;
    const double* a = p[R[0]];
    if (nr == 1) {
      for (int i = 0; i < 3; ++i) B.c[i] = a[i];
      B.r2 = 0;
      return B;
    }
    if (nr == 2) {
      const double* b = p[R[1]];
      double r2 = 0;
      for (int i = 0; i < 3; ++i) {
        B.c[i] = 0.5 * (a[i] + b[i]);
        const double d = b[i] - a[i];
        r2 += d * d;
      }
      B.r2 = 0.25 * r2;
      return B;
    }
    if (nr == 3) return through3(B, a, p[R[1]], p[R[2]]);
    return through4(B, a, p[R[1]], p[R[2]], p[R[3]]);
  }
  DBall through3(DBall B, const double* a, const double* b, const double* d) {
    const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const double v[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
    const double w[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    const double ww = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    if (!(ww > 1e-9 * uu * vv)) {
      ok = false;
      return B;
    }
    const double t[3] = {uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]};
    const double n[3] = {t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2], t[0] * w[1] - t[1] * w[0]};
    const double D = 2 * ww;
    double r2 = 0;
    for (int i = 0; i < 3; ++i) {
      const double o = n[i] / D;
      B.c[i] = a[i] + o;
      r2 += o * o;
    }
    B.r2 = r2;
    return B;
  }
  DBall through4(DBall B, const double* a, const double* b, const double* d, const double* e) {
    const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const double v[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
    const double s[3] = {e[0] - a[0], e[1] - a[1], e[2] - a[2]};
    const double vs[3] = {v[1] * s[2] - v[2] * s[1], v[2] * s[0] - v[0] * s[2], v[0] * s[1] - v[1] * s[0]};
    const double su[3] = {s[1] * u[2] - s[2] * u[1], s[2] * u[0] - s[0] * u[2], s[0] * u[1] - s[1] * u[0]};
    const double uv[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const double det = u[0] * vs[0] + u[1] * vs[1] + u[2] * vs[2];
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2],
                 ss = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    if (!(det * det > 1e-9 * uu * vv * ss)) {
      ok = false;
      return B;
    }
    const double D = 2 * det;
    double r2 = 0;
    for (int i = 0; i < 3; ++i) {
      const double o = (uu * vs[i] + vv * su[i] + ss * uv[i]) / D;
      B.c[i] = a[i] + o;
      r2 += o * o;
    }
    B.r2 = r2;
    return B;
  }
  bool contains(const DBall& B, int i) const {
    if (B.r2 < 0) return false;
    double d2 = 0;
    for (int a = 0; a < 3; ++a) {
      const double t = p[i][a] - B.c[a];
      d2 += t * t;
    }
    return d2 <= B.r2 * (1 + 1e-10) + 1e-9;
  }
  // Welzl a deplacement en tete (Gaertner) : L est la liste des points, S le support courant.
  int L[kCapacity];
  int S[4];
  int ns = 0;
  DBall ball;
  void mtf(int end) {
    ball = through(S, ns);
    if (!ok || ns == 4) return;
    for (int i = 0; i < end; ++i) {
      if (contains(ball, L[i])) continue;
      S[ns++] = L[i];
      mtf(i);
      --ns;
      if (!ok) return;
      const int v = L[i];
      for (int j = i; j > 0; --j) L[j] = L[j - 1];
      L[0] = v;
    }
  }
  // Welzl recursif sur T[0..n) avec R au bord (petits ensembles : |T| <= 4).
  // Bornes explicites (n <= 4, nr < 4 avant ecriture) : meme resultat, sans le faux positif -Warray-bounds de GCC
  // une fois la recursion expansee dans run_support.
  DBall small(const int* T, int n, int* R, int nr) {
    if (!ok) return DBall{};
    if (n <= 0 || n > 4 || nr < 0 || nr >= 4) return through(R, nr < 0 ? 0 : nr > 4 ? 4 : nr);
    DBall B = small(T, n - 1, R, nr);
    if (!ok || contains(B, T[n - 1])) return B;
    R[nr] = T[n - 1];
    return small(T, n - 1, R, nr + 1);
  }
  int farthest(int n, const double* q) const {
    int best = 0;
    double bd = -1;
    for (int i = 0; i < n; ++i) {
      double d = 0;
      for (int a = 0; a < 3; ++a) d += (p[i][a] - q[a]) * (p[i][a] - q[a]);
      if (d > bd) {
        bd = d;
        best = i;
      }
    }
    return best;
  }
  // Depart sur une paire eloignee (le plus loin du barycentre, puis le plus loin de lui), puis, tant qu'un point sort
  // de la boule, plus petite boule du support et du pire point, ce point au bord (lemme de Welzl). Cout seulement.
  DBall run_support(int n) {
    double gc[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) gc[a] += p[i][a];
    for (int a = 0; a < 3; ++a) gc[a] /= n;
    const int ia = farthest(n, gc);
    const int ib = farthest(n, p[ia]);
    if (ia == ib) {
      ok = false;
      return DBall{};
    }
    int S2[4] = {ia, ib, 0, 0};
    DBall B = through(S2, 2);
    for (int iter = 0; iter < 8 && ok; ++iter) {
      int v = -1;
      double worst = 0;
      for (int i = 0; i < n; ++i) {
        if (contains(B, i)) continue;
        double d = 0;
        for (int a = 0; a < 3; ++a) d += (p[i][a] - B.c[a]) * (p[i][a] - B.c[a]);
        if (d - B.r2 > worst) {
          worst = d - B.r2;
          v = i;
        }
      }
      if (v < 0) return B;
      int T[4];
      const int nt = B.nr < 0 ? 0 : B.nr > 4 ? 4 : B.nr;
      for (int i = 0; i < nt; ++i) T[i] = B.R[i];
      int R[4] = {v, 0, 0, 0};
      B = small(T, nt, R, 1);
    }
    ok = false;
    return DBall{};
  }
};

}  // namespace mhgp12::tower_detail
