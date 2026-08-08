// MorseHGP3D v2 — construction du catalogue des spheres critiques.
//
// Pour chaque point p, on enumere toutes les spheres critiques (U, I) telles
// que p ∈ U et |I| + |U| <= s_max, dans un voisinage W_p dont le rayon croit
// jusqu'a certification (DESIGN.md §3.1).
//
// ATTENTION — elagage supprime le 8 aout 2026.
//
// Une version anterieure filtrait les tetraedres candidats par un graphe de
// "faces admissibles" : elle supposait que la boule circonscrite a une face
// (centree dans le plan de la face) est incluse dans la circumboule du
// tetraedre. C'est FAUX : l'intersection de la circumboule avec le plan est le
// disque circonscrit de la face, mais la boule tridimensionnelle de meme bord
// deborde, puisque h + sqrt(R^2 - h^2) > R des que h > 0. Le §2 de
// WARNING_AUDIT_IMPLEMENTATION_2.md en donne une fixture entiere ou les quatre
// faces d'un tetraedre regulier bien centre de rang 4 ont toutes un rang >= 12.
//
// Le seul elagage conserve est prouve : si U est bien centre de rang <= s_max
// et de rayon R, alors R <= tau (theoreme 4) et tous les points de U sont deux a
// deux a distance au plus 2R <= 2 tau.

#include "mhgp/mhgp.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <numeric>
#include <thread>
#include <unordered_map>

namespace mhgp {
namespace {

// ---------------------------------------------------------------------------
// Grille de hachage spatiale
// ---------------------------------------------------------------------------
struct Grid {
  i64 cell = 1;
  i64 nx = 1, ny = 1, nz = 1;
  i64 ox = 0, oy = 0, oz = 0;
  std::vector<i32> start;
  std::vector<i32> items;

  MHGP_HD i64 index(i64 cx, i64 cy, i64 cz) const {
    return (cz * ny + cy) * nx + cx;
  }
};

Grid build_grid(const std::vector<P3>& pts, i64 cell) {
  Grid g;
  g.cell = std::max<i64>(cell, 1);
  i64 lox = pts[0].x, hix = pts[0].x;
  i64 loy = pts[0].y, hiy = pts[0].y;
  i64 loz = pts[0].z, hiz = pts[0].z;
  for (const P3& p : pts) {
    lox = std::min(lox, p.x); hix = std::max(hix, p.x);
    loy = std::min(loy, p.y); hiy = std::max(hiy, p.y);
    loz = std::min(loz, p.z); hiz = std::max(hiz, p.z);
  }
  g.ox = lox; g.oy = loy; g.oz = loz;
  g.nx = (hix - lox) / g.cell + 1;
  g.ny = (hiy - loy) / g.cell + 1;
  g.nz = (hiz - loz) / g.cell + 1;
  const i64 ncell = g.nx * g.ny * g.nz;
  g.start.assign(static_cast<size_t>(ncell) + 1, 0);
  std::vector<i32> cellof(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) {
    const i64 cx = (pts[i].x - g.ox) / g.cell;
    const i64 cy = (pts[i].y - g.oy) / g.cell;
    const i64 cz = (pts[i].z - g.oz) / g.cell;
    const i64 c = g.index(cx, cy, cz);
    cellof[i] = static_cast<i32>(c);
    ++g.start[static_cast<size_t>(c) + 1];
  }
  for (size_t i = 1; i < g.start.size(); ++i) g.start[i] += g.start[i - 1];
  g.items.resize(pts.size());
  std::vector<i32> cursor(g.start.begin(), g.start.end() - 1);
  for (size_t i = 0; i < pts.size(); ++i) {
    g.items[static_cast<size_t>(cursor[static_cast<size_t>(cellof[i])]++)] =
        static_cast<i32>(i);
  }
  return g;
}

// Points a distance <= radius de centre (indices), sans le point self.
void query_ball(const Grid& g, const std::vector<P3>& pts, const P3& centre,
                i64 radius, i32 self, std::vector<i32>* out) {
  out->clear();
  const i64 r2 = radius * radius;
  const i64 cx0 = std::max<i64>((centre.x - radius - g.ox) / g.cell, 0);
  const i64 cy0 = std::max<i64>((centre.y - radius - g.oy) / g.cell, 0);
  const i64 cz0 = std::max<i64>((centre.z - radius - g.oz) / g.cell, 0);
  const i64 cx1 = std::min<i64>((centre.x + radius - g.ox) / g.cell, g.nx - 1);
  const i64 cy1 = std::min<i64>((centre.y + radius - g.oy) / g.cell, g.ny - 1);
  const i64 cz1 = std::min<i64>((centre.z + radius - g.oz) / g.cell, g.nz - 1);
  if (cx1 < cx0 || cy1 < cy0 || cz1 < cz0) return;
  for (i64 cz = cz0; cz <= cz1; ++cz) {
    for (i64 cy = cy0; cy <= cy1; ++cy) {
      const i64 base = g.index(cx0, cy, cz);
      for (i64 cx = cx0; cx <= cx1; ++cx) {
        const i64 c = base + (cx - cx0);
        for (i32 t = g.start[static_cast<size_t>(c)];
             t < g.start[static_cast<size_t>(c) + 1]; ++t) {
          const i32 j = g.items[static_cast<size_t>(t)];
          if (j == self) continue;
          const P3 d = p3_sub(pts[static_cast<size_t>(j)], centre);
          const i64 dd = d.x * d.x + d.y * d.y + d.z * d.z;
          if (dd <= r2) out->push_back(j);
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Directions de certification : spirale de Fibonacci sur S^2.
// ---------------------------------------------------------------------------
struct Cover {
  std::vector<double> dir;  // 3 * m
  double sin_theta = 1.0;
  double cos_theta = 0.0;
  bool valid = false;       // faux => aucune certification possible
};

Cover make_cover(int m) {
  Cover c;
  c.dir.resize(static_cast<size_t>(m) * 3);
  const double ga = 3.399186938124345;  // pi * (3 - sqrt(5)) * ... golden angle
  for (int i = 0; i < m; ++i) {
    const double z = 1.0 - 2.0 * (i + 0.5) / m;
    const double r = std::sqrt(std::max(0.0, 1.0 - z * z));
    const double phi = ga * i;
    c.dir[3 * static_cast<size_t>(i) + 0] = r * std::cos(phi);
    c.dir[3 * static_cast<size_t>(i) + 1] = r * std::sin(phi);
    c.dir[3 * static_cast<size_t>(i) + 2] = z;
  }
  // rayon de couverture : max sur un echantillon dense de S^2 de l'angle a la
  // direction la plus proche, majore d'une marge.
  double worst = 0.0;
  const int S = 20000;
  for (int i = 0; i < S; ++i) {
    const double z = 1.0 - 2.0 * (i + 0.5) / S;
    const double r = std::sqrt(std::max(0.0, 1.0 - z * z));
    const double phi = 2.399963229728653 * i;
    const double px = r * std::cos(phi), py = r * std::sin(phi), pz = z;
    double best = -1.0;
    for (int j = 0; j < m; ++j) {
      const double d = px * c.dir[3 * static_cast<size_t>(j)]
                     + py * c.dir[3 * static_cast<size_t>(j) + 1]
                     + pz * c.dir[3 * static_cast<size_t>(j) + 2];
      best = std::max(best, d);
    }
    worst = std::max(worst, std::acos(std::min(1.0, std::max(-1.0, best))));
  }
  // Marge de surete sur un rayon de couverture ESTIME par echantillonnage :
  // ce n'est pas une preuve de couverture (WARNING_AUDIT_IMPLEMENTATION_2 §3).
  // La borne n'a d'utilite que si cos(theta) - sin(theta) > 0, donc
  // theta < pi/4 ; au-dela on force un theta invalide et radius_bound rendra
  // +infini, ce qui fait croitre le voisinage au lieu de certifier a tort.
  const double theta = worst * 1.15 + 0.02;
  c.valid = (m > 0) && (theta < 0.785398163397448);
  c.sin_theta = std::sin(theta);
  c.cos_theta = std::cos(theta);
  return c;
}

// ---------------------------------------------------------------------------
// Contexte de travail d'un point
// ---------------------------------------------------------------------------
struct Local {
  std::vector<i32> nb;        // indices globaux, tries par distance a p
  std::vector<i64> nb_d2;
  std::vector<P3> nb_pt;
  std::vector<i32> tmp;
  std::vector<u32> live;      // bitset des paires vivantes (m x m)
  std::vector<i32> adj_start;
  std::vector<i32> adj;
  std::vector<i32> members;
};

struct Emit {
  std::vector<CriticalSphere> spheres;
  std::vector<i32> members;
  i64 pairs = 0, triples = 0, quads = 0;
  i64 degenerate = 0;   // points cospheriques hors support rencontres
  i64 shell_anomaly = 0;  // support non entierement retrouve sur le bord
};

// Codes de rejet de `classify`, distincts parce qu'ils ne disent pas la meme
// chose : -1 est une boule trop peuplee (rien d'anormal, elle sort du rang
// contractuel), -2 est une COSPHERICITE (le nuage sort du domaine declare au
// DESIGN §6.4), -3 est un support qui ne se retrouve pas entierement sur le
// bord (anomalie).
constexpr int kTooMany = -1;
constexpr int kDegenerateShell = -2;
constexpr int kShellAnomaly = -3;

// Compte les points de X dans la boule fermee, en s'arretant des que le total
// depasse s_max. Renvoie -1 si depassement. Remplit membres et verifie que le
// shell est exactement le support.
int classify(const Sphere& s, i32 pid, const Local& L,
             const i32* sup, int nsup, int s_max, std::vector<i32>* members) {
  members->clear();
  members->push_back(pid);
  int on_boundary = 1;  // p est toujours sur le bord
  const double beta = sphere_beta(s);
  const double cut = 4.0 * beta * (1.0 + 1e-9) + 1.0;
  const size_t m = L.nb.size();
  for (size_t t = 0; t < m; ++t) {
    if (static_cast<double>(L.nb_d2[t]) > cut) break;  // filtre sur : |z-p| <= 2r
    const int side = sphere_side(s, L.nb_pt[t]);
    if (side > 0) continue;
    if (side == 0) {
      // doit appartenir au support, sinon degenerescence : le support n'est
      // pas minimal unique et la sphere n'est pas critique sous cette forme.
      bool in_sup = false;
      for (int u = 0; u < nsup; ++u) if (sup[u] == L.nb[t]) in_sup = true;
      if (!in_sup) return kDegenerateShell;  // cospherique : hors modele
      ++on_boundary;
    }
    members->push_back(L.nb[t]);
    if (static_cast<int>(members->size()) > s_max) return kTooMany;
  }
  if (on_boundary != nsup) return kShellAnomaly;
  return static_cast<int>(members->size());
}

// Comptabilise un rejet et dit si la sphere doit etre emise. Une degenerescence
// ne doit JAMAIS etre jetee en silence : c'est elle qui rend le resultat
// incomplet sans que rien ne le signale (echec O2 du 8 aout).
bool accept(int rank, Emit* e) {
  if (rank >= 0) return true;
  if (rank == kDegenerateShell) ++e->degenerate;
  else if (rank == kShellAnomaly) ++e->shell_anomaly;
  return false;
}

void push(Emit* e, const Sphere& s, const i32* sup, int nsup, int rank,
          const std::vector<i32>& members) {
  CriticalSphere c;
  for (int i = 0; i < nsup; ++i) c.support[i] = sup[i];
  for (int i = nsup; i < kMaxSupport; ++i) c.support[i] = -1;
  c.n_support = nsup;
  c.rank = rank;
  c.sph = s;
  c.beta = sphere_beta(s);
  c.members_begin = static_cast<i32>(e->members.size());
  // Le contrat public (mhgp.hpp) annonce la tranche I u U TRIEE. classify
  // insere l'ancre puis les voisins par distance : sans ce tri, le contrat est
  // faux par construction, ce qu'aucun test ne voyait -- O1 ne lit jamais
  // cat.members et O2 trie avant de comparer.
  std::vector<i32> ordered(members.begin(), members.end());
  std::sort(ordered.begin(), ordered.end());
  e->members.insert(e->members.end(), ordered.begin(), ordered.end());
  e->spheres.push_back(c);
}

// Enumeration locale complete autour de p, dans le voisinage courant.
// Renvoie le plus grand rayon^2 emis.
double enumerate_point(i32 pid, const std::vector<P3>& pts, Local& L,
                       int s_max, double tau, Emit* out) {
  const P3 p = pts[static_cast<size_t>(pid)];
  const size_t m = L.nb.size();
  double rmax2 = 0.0;
  i32 sup[4];
  std::vector<i32>& members = L.members;

  // ---- support 1 : le point lui-meme, boule de rayon nul ---------------
  {
    const Sphere s = sphere1(p);
    sup[0] = pid;
    const int rank = classify(s, pid, L, sup, 1, s_max, &members);
    if (accept(rank, out)) push(out, s, sup, 1, rank, members);
  }

  // ---- support 2 -------------------------------------------------------
  for (size_t a = 0; a < m; ++a) {
    const Sphere s = sphere2(p, L.nb_pt[a]);
    sup[0] = std::min(pid, L.nb[a]);
    sup[1] = std::max(pid, L.nb[a]);
    ++out->pairs;
    const int rank = classify(s, pid, L, sup, 2, s_max, &members);
    if (!accept(rank, out)) continue;
    push(out, s, sup, 2, rank, members);
    rmax2 = std::max(rmax2, sphere_beta(s));
  }

  // ---- support 3, et paires admissibles par la borne de diametre --------
  //
  // Seul critere de rejet employe ici : |z_a - z_b| <= 2 tau, consequence
  // directe du theoreme 4 (R <= tau) et du fait que les sommets d'un support
  // bien centre sont sur une sphere de rayon R.
  const size_t words = (m + 31) / 32;
  L.live.assign(words * m, 0u);
  const double diam_cut = 4.0 * tau * tau * (1.0 + 1e-9) + 1.0;
  for (size_t a = 0; a < m; ++a) {
    for (size_t b = a + 1; b < m; ++b) {
      const P3 d = p3_sub(L.nb_pt[a], L.nb_pt[b]);
      const double dd = static_cast<double>(d.x) * d.x + static_cast<double>(d.y) * d.y
                      + static_cast<double>(d.z) * d.z;
      if (std::isfinite(diam_cut) && dd > diam_cut) continue;
      L.live[a * words + (b >> 5)] |= (1u << (b & 31));
      L.live[b * words + (a >> 5)] |= (1u << (a & 31));

      Sphere s;
      if (!sphere3(p, L.nb_pt[a], L.nb_pt[b], &s)) continue;
      ++out->triples;
      if (!well_centered3(p, L.nb_pt[a], L.nb_pt[b])) continue;
      sup[0] = pid; sup[1] = L.nb[a]; sup[2] = L.nb[b];
      std::sort(sup, sup + 3);
      const int rank = classify(s, pid, L, sup, 3, s_max, &members);
      if (!accept(rank, out)) continue;
      push(out, s, sup, 3, rank, members);
      rmax2 = std::max(rmax2, sphere_beta(s));
    }
  }

  // ---- support 4 : triplets dont les trois paires respectent le diametre -
  for (size_t a = 0; a < m; ++a) {
    for (size_t b = a + 1; b < m; ++b) {
      if (!(L.live[a * words + (b >> 5)] & (1u << (b & 31)))) continue;
      for (size_t c = b + 1; c < m; ++c) {
        if (!(L.live[a * words + (c >> 5)] & (1u << (c & 31)))) continue;
        if (!(L.live[b * words + (c >> 5)] & (1u << (c & 31)))) continue;
        Sphere s;
        if (!sphere4(p, L.nb_pt[a], L.nb_pt[b], L.nb_pt[c], &s)) continue;
        ++out->quads;
        if (!well_centered4(s, p, L.nb_pt[a], L.nb_pt[b], L.nb_pt[c])) continue;
        sup[0] = pid; sup[1] = L.nb[a]; sup[2] = L.nb[b]; sup[3] = L.nb[c];
        std::sort(sup, sup + 4);
        const int rank = classify(s, pid, L, sup, 4, s_max, &members);
        if (!accept(rank, out)) continue;
        push(out, s, sup, 4, rank, members);
        rmax2 = std::max(rmax2, sphere_beta(s));
      }
    }
  }
  return rmax2;
}

// Majoration a priori du rayon d'une sphere critique tangente en p.
//
// Pour une direction unitaire e et un point z, z appartient a la boule tangente
// en p de direction e et de rayon r si et seulement si
//     pi_e(z) = |z - p|^2 / (2 <z - p, e>)  <=  r        (et <z-p,e> > 0).
// Le rang ferme etant au plus s_max, r est au plus la s_max-ieme plus petite
// valeur de pi_e. Sur un cone de demi-angle theta autour de u,
//     <z - p, e>  >=  <z - p, u> cos(theta) - |z - p| sin(theta),
// donc pi_e(z) <= pit(z) avec ce denominateur minore. La s_max-ieme plus petite
// valeur de pit majore donc r pour toute direction du cone.
//
// Enfin pit(z) >= |z - p| / 2 : les points qui contribuent a cette valeur sont
// tous dans B(p, 2 tau). Calculer la statistique sur W_rho au lieu de X ne peut
// que l'augmenter (moins de candidats), donc la valeur obtenue reste un
// majorant. Le test 2 tau <= rho est ainsi une implication, pas une constatation
// a posteriori : c'est la reparation du contre-exemple de
// WARNING_AUDIT_COMPLETUDE.md §1.
//
// Renvoie +inf si un cone ne fournit pas s_max valeurs finies.
double radius_bound(const P3& p, const Cover& cov, const Local& L, int s_max) {
  const int m = static_cast<int>(cov.dir.size() / 3);
  // Un recouvrement vide ou de demi-angle >= pi/4 ne borne rien : renvoyer 0
  // certifierait a tort (WARNING_AUDIT_IMPLEMENTATION_2 §3).
  if (!cov.valid || m == 0) return std::numeric_limits<double>::infinity();
  const double ct = cov.cos_theta, st = cov.sin_theta;
  double worst = 0.0;
  std::vector<double> vals;
  vals.reserve(L.nb.size());
  for (int i = 0; i < m; ++i) {
    const double ux = cov.dir[3 * static_cast<size_t>(i)];
    const double uy = cov.dir[3 * static_cast<size_t>(i) + 1];
    const double uz = cov.dir[3 * static_cast<size_t>(i) + 2];
    vals.clear();
    for (size_t t = 0; t < L.nb.size(); ++t) {
      const double dx = static_cast<double>(L.nb_pt[t].x - p.x);
      const double dy = static_cast<double>(L.nb_pt[t].y - p.y);
      const double dz = static_cast<double>(L.nb_pt[t].z - p.z);
      const double d2 = dx * dx + dy * dy + dz * dz;
      const double d = std::sqrt(d2);
      const double denom = (dx * ux + dy * uy + dz * uz) * ct - d * st;
      if (denom <= 0.0) continue;
      // majoration conservatrice : on gonfle legerement le quotient
      vals.push_back(d2 / (2.0 * denom) * (1.0 + 1e-12) + 1e-9);
    }
    if (static_cast<int>(vals.size()) < s_max) return std::numeric_limits<double>::infinity();
    std::nth_element(vals.begin(), vals.begin() + (s_max - 1), vals.end());
    worst = std::max(worst, vals[static_cast<size_t>(s_max - 1)]);
  }
  return worst;
}

}  // namespace

Catalogue build_catalogue(const std::vector<P3>& pts, int s_max,
                          const Options& opt) {
  Catalogue cat;
  const size_t n = pts.size();
  if (n == 0) return cat;
  cat.neighbourhood_size.assign(n, 0);
  cat.growth_rounds.assign(n, 0);
  cat.certified.assign(n, 0);

  // Rayon de depart : estimation du rayon des s_max plus proches voisins.
  i64 lo[3] = {pts[0].x, pts[0].y, pts[0].z};
  i64 hi[3] = {pts[0].x, pts[0].y, pts[0].z};
  for (const P3& p : pts) {
    lo[0] = std::min(lo[0], p.x); hi[0] = std::max(hi[0], p.x);
    lo[1] = std::min(lo[1], p.y); hi[1] = std::max(hi[1], p.y);
    lo[2] = std::min(lo[2], p.z); hi[2] = std::max(hi[2], p.z);
  }
  const double vol = std::max(1.0, static_cast<double>(hi[0] - lo[0] + 1))
                   * std::max(1.0, static_cast<double>(hi[1] - lo[1] + 1))
                   * std::max(1.0, static_cast<double>(hi[2] - lo[2] + 1));
  const double r_typ = std::cbrt(vol * (s_max + 2) / (static_cast<double>(n) * 4.18879));
  const i64 diameter = static_cast<i64>(std::ceil(std::sqrt(
      static_cast<double>(hi[0] - lo[0]) * (hi[0] - lo[0])
      + static_cast<double>(hi[1] - lo[1]) * (hi[1] - lo[1])
      + static_cast<double>(hi[2] - lo[2]) * (hi[2] - lo[2])))) + 2;

  const i64 cell = std::max<i64>(1, static_cast<i64>(r_typ));
  const Grid grid = build_grid(pts, cell);
  const Cover cover = make_cover(opt.cone_directions);

  int nthreads = opt.threads > 0 ? opt.threads
                                 : static_cast<int>(std::thread::hardware_concurrency());
  if (nthreads <= 0) nthreads = 1;
  nthreads = std::min<int>(nthreads, static_cast<int>(n));

  std::vector<Emit> partial(static_cast<size_t>(nthreads));
  std::atomic<i32> uncert{0};

  auto worker = [&](int tid) {
    Local L;
    std::vector<i32> tmp;
    Emit& out = partial[static_cast<size_t>(tid)];
    for (size_t i = static_cast<size_t>(tid); i < n;
         i += static_cast<size_t>(nthreads)) {
      const P3 p = pts[i];
      i64 rho = opt.exhaustive_neighbourhood
                    ? diameter
                    : std::max<i64>(2, static_cast<i64>(std::ceil(2.0 * r_typ)));
      int rounds = 0;
      bool ok = false;
      size_t mark_sph = out.spheres.size();
      size_t mark_mem = out.members.size();
      while (true) {
        out.spheres.resize(mark_sph);
        out.members.resize(mark_mem);
        query_ball(grid, pts, p, rho, static_cast<i32>(i), &L.nb);
        // tri par distance a p
        L.nb_d2.resize(L.nb.size());
        for (size_t t = 0; t < L.nb.size(); ++t) {
          const P3 d = p3_sub(pts[static_cast<size_t>(L.nb[t])], p);
          L.nb_d2[t] = d.x * d.x + d.y * d.y + d.z * d.z;
        }
        std::vector<size_t> ord(L.nb.size());
        std::iota(ord.begin(), ord.end(), size_t{0});
        std::sort(ord.begin(), ord.end(),
                  [&](size_t a, size_t b) { return L.nb_d2[a] < L.nb_d2[b]; });
        std::vector<i32> nb2(L.nb.size());
        std::vector<i64> d22(L.nb.size());
        L.nb_pt.resize(L.nb.size());
        for (size_t t = 0; t < ord.size(); ++t) {
          nb2[t] = L.nb[ord[t]];
          d22[t] = L.nb_d2[ord[t]];
          L.nb_pt[t] = pts[static_cast<size_t>(nb2[t])];
        }
        L.nb.swap(nb2);
        L.nb_d2.swap(d22);

        // Majoration a priori : si 2 tau <= rho, toute sphere critique tangente
        // en p a un rayon <= rho/2, donc tous ses points sont dans W_rho et le
        // catalogue local est complet.
        const double tau = radius_bound(p, cover, L, s_max);
        ok = (2.0 * tau <= static_cast<double>(rho));
        if (ok) {
          enumerate_point(static_cast<i32>(i), pts, L, s_max, tau, &out);
          break;
        }
        if (static_cast<size_t>(L.nb.size()) + 1 >= n) {
          // le voisinage est le nuage entier : le calcul est exact par
          // epuisement, sans avoir besoin de la majoration.
          enumerate_point(static_cast<i32>(i), pts, L, s_max, tau, &out);
          ok = true;
          break;
        }
        if (rho >= diameter) {
          enumerate_point(static_cast<i32>(i), pts, L, s_max, tau, &out);
          ok = true;  // W_rho couvre le diametre : tout le nuage est vu
          break;
        }
        i64 next = rho * 2;
        if (std::isfinite(tau)) next = std::max<i64>(next, static_cast<i64>(std::ceil(2.0 * tau)));
        rho = std::min<i64>(diameter, next);
        ++rounds;
        if (rounds > opt.max_growth_rounds) {
          enumerate_point(static_cast<i32>(i), pts, L, s_max, tau, &out);
          ok = false;
          break;
        }
      }
      if (!ok) uncert.fetch_add(1, std::memory_order_relaxed);
      cat.neighbourhood_size[i] = static_cast<i32>(L.nb.size());
      cat.growth_rounds[i] = rounds;
      cat.certified[i] = ok ? 1u : 0u;
    }
  };

  std::vector<std::thread> ths;
  ths.reserve(static_cast<size_t>(nthreads));
  for (int t = 0; t < nthreads; ++t) ths.emplace_back(worker, t);
  for (auto& t : ths) t.join();

  // Fusion des sorties partielles + propriete canonique (min du support).
  size_t total = 0, totmem = 0;
  for (const Emit& e : partial) { total += e.spheres.size(); totmem += e.members.size(); }
  cat.spheres.reserve(total);
  cat.members.reserve(totmem);
  for (const Emit& e : partial) {
    cat.candidate_pairs += e.pairs;
    cat.candidate_triples += e.triples;
    cat.candidate_quads += e.quads;
    cat.degenerate_shells += e.degenerate;
    cat.shell_anomalies += e.shell_anomaly;
    for (const CriticalSphere& s : e.spheres) {
      if (s.support[0] != -1) {
        // conserve uniquement chez le proprietaire canonique
        // (le point courant est celui qui a emis ; on filtre par min(U))
      }
      CriticalSphere c = s;
      c.members_begin = static_cast<i32>(cat.members.size());
      const i32 cnt = c.rank;
      cat.members.insert(cat.members.end(),
                         e.members.begin() + s.members_begin,
                         e.members.begin() + s.members_begin + cnt);
      cat.spheres.push_back(c);
    }
  }
  // deduplication par support
  std::vector<i32> idx(cat.spheres.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&](i32 a, i32 b) {
    const CriticalSphere& A = cat.spheres[static_cast<size_t>(a)];
    const CriticalSphere& B = cat.spheres[static_cast<size_t>(b)];
    for (int t = 0; t < kMaxSupport; ++t)
      if (A.support[t] != B.support[t]) return A.support[t] < B.support[t];
    return false;
  });
  std::vector<CriticalSphere> uniq;
  std::vector<i32> umem;
  uniq.reserve(cat.spheres.size());
  for (size_t t = 0; t < idx.size(); ++t) {
    const CriticalSphere& s = cat.spheres[static_cast<size_t>(idx[t])];
    if (!uniq.empty()) {
      const CriticalSphere& q = uniq.back();
      bool same = true;
      for (int u = 0; u < kMaxSupport; ++u)
        if (q.support[u] != s.support[u]) { same = false; break; }
      if (same) continue;
    }
    CriticalSphere c = s;
    c.members_begin = static_cast<i32>(umem.size());
    umem.insert(umem.end(), cat.members.begin() + s.members_begin,
                cat.members.begin() + s.members_begin + s.rank);
    uniq.push_back(c);
  }
  cat.spheres.swap(uniq);
  cat.members.swap(umem);
  return cat;
}

}  // namespace mhgp
