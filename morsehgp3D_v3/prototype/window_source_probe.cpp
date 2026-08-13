// MorseHGP3D v3 — SUJET BORNE DE FENETRE CERTIFIEE, ET SON JUGE RATIONNEL.
//
// Specification : audits/AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md,
// section 8, points 2 a 4. Cadre : phase=exploration_v3_hors_registre,
// backend=cpu_reference, profile=quantized_u16_input_only,
// mode=proposition_math_non_recue, public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE BINAIRE EST, ET CE QU'IL N'EST PAS
//
// Il est la PORTE du certificat de fenetre, pas le producteur de l'etape 2.
// Son enumerateur local est deliberement naif : il forme tous les sous-ensembles
// de taille 2, 3 et 4 de la fenetre. Aucun temps, aucun compteur de travail et
// aucune pente ne sont revendiques ici — le prouver ne serait qu'un artefact de
// ce choix. Ce qui est revendique est une IDENTITE :
//
//   S_certifiee(sujet)  ==  { S global : il existe a dans S avec 4R^2 < delta_out(a)^2 }
//
// verifiee dans LES DEUX SENS contre un juge exhaustif, sur les identites
// completes (SupportKey, |I_B|, |U_B|, classe de BallKey) — pas sur des comptes.
//
// Le juge est le juge rationnel de `oracle/exact_geometry.hpp` : elimination de
// Gauss sur des rationnels signe-magnitude. Le sujet emploie des determinants
// ENTIERS de Cramer et `mhgp::BigInt<4>`. Les deux representations ne partagent
// aucune quantite intermediaire.
//
// LE RESIDUEL N'EST PAS ENUMERE. Ce binaire publie la masse des candidats
// locaux qui echouent le certificat, et il affirme explicitement que cette masse
// n'est PAS le residuel : un support global dont aucun tuple n'a jamais ete
// forme dans une fenetre n'y figure pas. La porte `--min-jamais-proposes` exige
// au contraire que le juge en trouve, pour que cette insuffisance soit MESUREE
// et non oubliee.
//
// CODES DE SORTIE : 1 desaccords du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "oracle/exact_geometry.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/window_source.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::window::IntSphere;
using mhgp3v::window::WindowMutant;

namespace win = mhgp3v::window;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

// ---------------------------------------------------------------------------
// LEDGER. Les compteurs de la requete, de la proposition, de la positivite, du
// census, des supports certifies et du residuel sont SEPARES : un total unique
// masquerait ou passe le travail.
// ---------------------------------------------------------------------------
struct Ledger {
  long long knn_queries = 0;
  long long knn_point_tests = 0;
  long long windows_total_scan = 0;   // delta_out = +infini
  long long tuples_proposed = 0;      // sous-ensembles formes
  long long products_q[5] = {0, 0, 0, 0, 0};  // propositions par arite
  long long affinely_dependent = 0;   // refuses avant toute geometrie
  long long positivity_tests = 0;
  long long positive_candidates = 0;
  long long census_point_tests = 0;
  long long relevant_candidates = 0;  // |I_B| + |S| <= smax
  long long jung_fast_hits = 0;       // certifies par le chemin i64
  long long exact_path_calls = 0;     // recours au test rationnel exact
  long long certified_supports = 0;
  long long uncertified_candidates = 0;  // proposes ET refuses : PAS le residuel
  long long certified_q[5] = {0, 0, 0, 0, 0};
  long long shell_max = 0;            // plus grand |U_B| observe
  long long interior_max = 0;
  // Histogramme des propositions PAR ANCRE. Une moyenne masque l'ancre qui
  // paie C(M,3) toute seule ; l'audit exige p50/p90/p99/max.
  std::vector<long long> per_anchor_products;
};

// ---------------------------------------------------------------------------
// Enregistrement canonique d'un support certifie.
// ---------------------------------------------------------------------------
struct Record {
  std::vector<int> support;  // PointId tries : c'est la `SupportKey`
  int interior = 0;          // |I_B|
  int shell = 0;             // |U_B|, shell FERME global (dans la fenetre)
  int ball_group = -1;       // classe de `BallKey`
  bool operator<(const Record& o) const { return support < o.support; }
};

// PGCD sur i128, pour reduire la cle de boule.
mhgp::i128 gcd128(mhgp::i128 a, mhgp::i128 b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) {
    const mhgp::i128 t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// `BallKey` du sujet : centre absolu reduit, plus le plus petit `PointId` du
// shell fermé. Deux boules de meme centre ont des rayons distincts, donc des
// shells disjoints : le couple identifie donc bien la boule.
struct BallKey {
  mhgp::i128 n[3] = {0, 0, 0};
  mhgp::i128 den = 0;
  int shell_min = -1;
  bool operator<(const BallKey& o) const {
    for (int i = 0; i < 3; ++i)
      if (n[i] != o.n[i]) return n[i] < o.n[i];
    if (den != o.den) return den < o.den;
    return shell_min < o.shell_min;
  }
};

BallKey ball_key_of(const IntSphere& s, const P3& p0, int shell_min) {
  BallKey k{};
  const mhgp::i128 p[3] = {(mhgp::i128)(i64)p0.x, (mhgp::i128)(i64)p0.y, (mhgp::i128)(i64)p0.z};
  for (int j = 0; j < 3; ++j) k.n[j] = p[j] * s.den + s.g[j];
  k.den = s.den;
  mhgp::i128 g = k.den;
  for (int j = 0; j < 3; ++j) g = gcd128(g, k.n[j]);
  if (g > 1) {
    for (int j = 0; j < 3; ++j) k.n[j] /= g;
    k.den /= g;
  }
  k.shell_min = shell_min;
  return k;
}

// ---------------------------------------------------------------------------
// FENETRE EXACTE : `M+1` voisins lus, `M` conserves, coupure au PREMIER OMIS.
// ---------------------------------------------------------------------------
struct Window {
  std::vector<int> ids;   // les `M` plus proches, hors `a`
  i64 delta_out2 = -1;    // -1 : rien n'est omis
};

Window build_window(const std::vector<P3>& pts, int a, int cap, WindowMutant mu, Ledger* led) {
  const int n = (int)pts.size();
  std::vector<std::pair<i64, int>> d;
  d.reserve((std::size_t)n - 1);
  ++led->knn_queries;
  for (int z = 0; z < n; ++z) {
    if (z == a) continue;
    const i64 dx = (i64)pts[(std::size_t)z].x - (i64)pts[(std::size_t)a].x;
    const i64 dy = (i64)pts[(std::size_t)z].y - (i64)pts[(std::size_t)a].y;
    const i64 dz = (i64)pts[(std::size_t)z].z - (i64)pts[(std::size_t)a].z;
    d.push_back({dx * dx + dy * dy + dz * dz, z});
    ++led->knn_point_tests;
  }
  // Ordre canonique (distance carree, PointId) : il ne depend pas du scheduling.
  std::sort(d.begin(), d.end());
  Window w{};
  const int keep = std::min<int>(cap, (int)d.size());
  for (int i = 0; i < keep; ++i) w.ids.push_back(d[(std::size_t)i].second);
  if ((int)d.size() > keep) {
    // LA COUPURE EST LE PREMIER OMIS. Le mutant prend le M-ieme INCLUS : la
    // comparaison reste sure mais un support dont la boule touche exactement ce
    // site-la serait certifie a tort si l'inegalite etait large, et perdu si
    // elle est stricte. La porte le tue par le ledger.
    w.delta_out2 = (mu == WindowMutant::kCutAtIncluded && keep > 0)
                       ? d[(std::size_t)keep - 1].first
                       : d[(std::size_t)keep].first;
  } else {
    ++led->windows_total_scan;
  }
  return w;
}

// ---------------------------------------------------------------------------
// SUJET : les supports propres positifs certifies par la fenetre de `a`.
// ---------------------------------------------------------------------------
void produce_anchor(const std::vector<P3>& pts, int a, const Window& w, int smax,
                    WindowMutant mu, Ledger* led, std::vector<Record>* out,
                    std::vector<BallKey>* keys) {
  const long long products_before = led->tuples_proposed;
  const int m = (int)w.ids.size();
  int ids[4];
  ids[0] = a;
  for (int q = 2; q <= 4; ++q) {
    const int k = q - 1;
    std::vector<int> idx((std::size_t)k);
    for (int i = 0; i < k; ++i) idx[(std::size_t)i] = i;
    if (k > m) continue;
    while (true) {
      for (int i = 0; i < k; ++i) ids[i + 1] = w.ids[(std::size_t)idx[(std::size_t)i]];
      ++led->tuples_proposed;
      ++led->products_q[q];
      const IntSphere s = win::sphere_of(pts.data(), ids, q);
      if (!s.ok) {
        ++led->affinely_dependent;
      } else {
        ++led->positivity_tests;
        const bool positive = s.positive || (mu == WindowMutant::kSkipPositivity);
        if (positive) {
          ++led->positive_candidates;
          // Census DANS LA FENETRE. Si le certificat passe, le theoreme garantit
          // que ce census est le census GLOBAL, shell compris.
          int interior = 0, shell = 0, shell_min = a;
          bool ok = true;
          for (int t = -1; t < m && ok; ++t) {
            const int z = (t < 0) ? a : w.ids[(std::size_t)t];
            bool in_support = false;
            for (int i = 0; i < q; ++i)
              if (ids[i] == z) in_support = true;
            const int side = win::side_of(s, pts[(std::size_t)a], pts[(std::size_t)z]);
            ++led->census_point_tests;
            if (side < 0) {
              ++interior;
              if (mu == WindowMutant::kShellAsInterior && in_support) continue;
            } else if (side == 0) {
              ++shell;
              if (z < shell_min) shell_min = z;
              // MUTANT : le shell compte comme interieur. Le support lui-meme
              // est sur le shell, donc le rang ferme explose et la pertinence
              // change.
              if (mu == WindowMutant::kShellAsInterior) ++interior;
            }
          }
          if (interior + q <= smax) {
            ++led->relevant_candidates;
            const bool jung = win::jung_fast_path(pts.data(), ids, q, w.delta_out2);
            if (jung) ++led->jung_fast_hits;
            bool certified = jung;
            if (!certified && mu != WindowMutant::kJungOnly) {
              ++led->exact_path_calls;
              certified = win::window_certified(s, w.delta_out2, mu);
            }
            if (certified) {
              ++led->certified_supports;
              ++led->certified_q[q];
              if (shell > led->shell_max) led->shell_max = shell;
              if (interior > led->interior_max) led->interior_max = interior;
              Record r{};
              for (int i = 0; i < q; ++i) r.support.push_back(ids[i]);
              std::sort(r.support.begin(), r.support.end());
              r.interior = interior;
              r.shell = shell;
              out->push_back(r);
              keys->push_back(ball_key_of(s, pts[(std::size_t)a], shell_min));
            } else {
              ++led->uncertified_candidates;
            }
          }
        }
      }
      int i = k - 1;
      while (i >= 0 && idx[(std::size_t)i] == m - k + i) --i;
      if (i < 0) break;
      ++idx[(std::size_t)i];
      for (int j = i + 1; j < k; ++j)
        idx[(std::size_t)j] = idx[(std::size_t)j - 1] + 1;
    }
  }
  led->per_anchor_products.push_back(led->tuples_proposed - products_before);
}

// ---------------------------------------------------------------------------
// JUGE RATIONNEL EXHAUSTIF. Il enumere TOUS les sous-ensembles de taille 2 a 4
// du nuage entier, sans fenetre, avec `oracle/exact_geometry.hpp`.
// ---------------------------------------------------------------------------
struct JudgeOut {
  std::vector<Record> global;      // tous les supports pertinents
  std::vector<Record> certifiable; // ceux qu'une fenetre DOIT certifier
  long long never_proposed = 0;    // globaux dont aucun tuple n'est dans une fenetre
};

JudgeOut judge_all(const std::vector<P3>& pts, int smax, int cap,
                   const std::vector<Window>& windows) {
  const int n = (int)pts.size();
  JudgeOut out{};
  // Coupures rationnelles, une par ancre.
  std::vector<mhgp3v::Rational> cut((std::size_t)n);
  for (int a = 0; a < n; ++a)
    cut[(std::size_t)a] = mhgp3v::Rational(
        mhgp3v::BigInt(windows[(std::size_t)a].delta_out2 < 0 ? 0 : windows[(std::size_t)a].delta_out2));

  std::vector<std::set<int>> in_window((std::size_t)n);
  for (int a = 0; a < n; ++a)
    for (int z : windows[(std::size_t)a].ids) in_window[(std::size_t)a].insert(z);

  const mhgp3v::Rational four(mhgp3v::BigInt(4));
  for (int q = 2; q <= 4; ++q) {
    mhgp3v::geometry::each_subset(n, q, [&](const std::vector<int>& sub) {
      const mhgp3v::geometry::RationalSphere sph = mhgp3v::geometry::sphere_through(pts, sub);
      if (!sph.ok || !mhgp3v::geometry::well_centred(sph)) return;
      int interior = 0, shell = 0;
      for (int z = 0; z < n; ++z) {
        const int side = mhgp3v::geometry::side_of(sph, pts[(std::size_t)z]);
        if (side < 0) ++interior;
        else if (side == 0) ++shell;
      }
      if (interior + q > smax) return;
      Record r{};
      r.support = sub;
      r.interior = interior;
      r.shell = shell;
      out.global.push_back(r);
      // Certifiable : il existe `a` dans le support avec 4 R^2 < delta_out(a)^2,
      // et le support entier tient dans la fenetre de `a`.
      bool certifiable = false;
      bool proposable = false;
      for (int a : sub) {
        bool all_in = true;
        for (int z : sub)
          if (z != a && in_window[(std::size_t)a].count(z) == 0) all_in = false;
        if (!all_in) continue;
        proposable = true;
        if (windows[(std::size_t)a].delta_out2 < 0) { certifiable = true; break; }
        if (compare(four * sph.squared_radius, cut[(std::size_t)a]) < 0) {
          certifiable = true;
          break;
        }
      }
      if (certifiable) out.certifiable.push_back(r);
      if (!proposable) ++out.never_proposed;
    });
  }
  (void)cap;
  std::sort(out.global.begin(), out.global.end());
  std::sort(out.certifiable.begin(), out.certifiable.end());
  return out;
}

// ---------------------------------------------------------------------------
bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (end == nullptr || *end != '\0') return false;
  if (errno == ERANGE) return false;
  *out = v;
  return true;
}

struct Options {
  long long n = 24;
  long long coord = -1;
  long long seed = 1;
  long long smax = 11;
  long long cap = 8;
  bool judge = false;
  bool fixtures = false;
  bool cloud_egalite = false;
  CloudFamily family = CloudFamily::kUniform;
  WindowMutant mutant = WindowMutant::kNone;
  long long min_certified = 0;
  long long min_uncertified = 0;
  long long min_jamais_proposes = 0;
  long long min_shell = 0;
  long long min_jung = 0;
  long long min_exact_path = 0;
};

// ---------------------------------------------------------------------------
// FIXTURES GRAVEES.
// ---------------------------------------------------------------------------
int run_fixtures(bool verbose) {
  int bad = 0;
  auto say = [&](const char* name, bool ok, const char* why) {
    if (!ok) ++bad;
    if (verbose) std::printf("fixture %-22s %s  (%s)\n", name, ok ? "OK" : "FAUX", why);
  };

  // 1. EGALITE : a=(0,0,0), b=(2,0,0), c=(0,2,0), M=1 vu de `a`.
  //    Le support {a,b} a R=1, donc 4R^2=4 ; le premier omis est `c` a
  //    distance carree 4. L'egalite doit REFUSER.
  {
    const P3 pts[3] = {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}};
    const int ids[2] = {0, 1};
    const IntSphere s = win::sphere_of(pts, ids, 2);
    const bool strict = win::window_certified(s, 4, WindowMutant::kNone);
    const bool large = win::window_certified(s, 4, WindowMutant::kAcceptEquality);
    say("egalite-tie", s.ok && s.positive && !strict && large,
        "4R^2 = delta_out^2 : le strict refuse, le mutant <= accepte");
  }

  // 2. ENDPOINT NON-OWNER SEUL CERTIFIANT : a=(0,0,0), b=(10,0,0), c=(0,1,0),
  //    M=2. Le support {a,b} a 4R^2=100. Vu de `a` avec M=2 il n'y a pas
  //    d'omis. Reduit a M=1 vu de `a`, la coupure est |ab|^2=100 : egalite,
  //    donc refus. Vu de `b` avec M=1, la coupure est |bc|^2=101 > 100 : le
  //    support est certifie. Un chemin ou seul l'owner minimal propose le perd.
  {
    const P3 pts[3] = {{0, 0, 0}, {10, 0, 0}, {0, 1, 0}};
    const int ids[2] = {0, 1};
    const IntSphere s = win::sphere_of(pts, ids, 2);
    const bool vu_de_a = win::window_certified(s, 100);
    const bool vu_de_b = win::window_certified(s, 101);
    say("owner-tardif", !vu_de_a && vu_de_b,
        "l'endpoint minimal echoue, l'autre certifie : l'owner vient apres");
  }

  // 3. POSITIVITE : un triangle obtus n'est PAS un support propre positif.
  //    (0,0,0), (10,0,0), (11,1,0) : l'angle en (10,0,0) est obtus, le centre
  //    du cercle circonscrit sort du triangle.
  {
    const P3 pts[3] = {{0, 0, 0}, {10, 0, 0}, {11, 1, 0}};
    const int ids[3] = {0, 1, 2};
    const IntSphere s = win::sphere_of(pts, ids, 3);
    const int acute[3] = {0, 1, 2};
    const P3 pa[3] = {{0, 0, 0}, {10, 0, 0}, {5, 6, 0}};
    const IntSphere t = win::sphere_of(pa, acute, 3);
    say("positivite-obtus", s.ok && !s.positive && t.ok && t.positive,
        "le triangle obtus est refuse, l'aigu est un support propre positif");
  }

  // 4. LARGEUR, DANS LA DIRECTION DANGEREUSE. Le triangle aigu
  //    (0,0,0), (40000,0,0), (20000,30000,0) est un support propre positif dont
  //    |g|^2 vaut environ 8e46 et den^2 environ 1e38 : les deux debordent i64 de
  //    tres loin. A `delta_out^2 = 1`, le test exact REFUSE — la boule est
  //    immense devant la fenetre — tandis que l'i64 enroule et CERTIFIE. Ce
  //    n'est pas une perte de travail : c'est une fausse certification, donc un
  //    support declare global alors que son census n'a jamais ete vu.
  {
    const P3 pts[3] = {{0, 0, 0}, {40000, 0, 0}, {20000, 30000, 0}};
    const int ids[3] = {0, 1, 2};
    const IntSphere s = win::sphere_of(pts, ids, 3);
    const bool exact = win::window_certified(s, 1, WindowMutant::kNone);
    const bool etroit = win::window_certified(s, 1, WindowMutant::kNarrowI64);
    say("largeur-i64", s.ok && s.positive && !exact && etroit,
        "l'i64 enroule et CERTIFIE une boule immense que le test exact refuse");
  }

  // 4b. L'ANGLE DROIT N'EST PAS UN SUPPORT PROPRE POSITIF. Pour
  //     (0,0,0), (40000,0,0), (0,40000,0), le centre du cercle circonscrit tombe
  //     exactement sur l'hypotenuse : la coordonnee barycentrique du sommet
  //     droit est nulle. La miniboule est la boule diametrale de l'hypotenuse,
  //     et le support propre est la paire, pas le triangle. La positivite
  //     STRICTE est donc necessaire, `>= 0` ne suffit pas.
  {
    const P3 pts[3] = {{0, 0, 0}, {40000, 0, 0}, {0, 40000, 0}};
    const int ids[3] = {0, 1, 2};
    const IntSphere s = win::sphere_of(pts, ids, 3);
    say("angle-droit", s.ok && !s.positive,
        "centre sur l'hypotenuse : lambda nulle, le triangle n'est pas le support");
  }

  // 5. RAYON NUL / POSITIONS COLOCALISEES : le support est degenere et doit
  //    etre refuse, jamais jitte.
  {
    const P3 pts[2] = {{7, 7, 7}, {7, 7, 7}};
    const int ids[2] = {0, 1};
    const IntSphere s = win::sphere_of(pts, ids, 2);
    say("rayon-nul", !s.ok, "positions colocalisees : refus, jamais un jitter");
  }

  // 6. CHEMIN RAPIDE DE JUNG : suffisant, jamais necessaire. Le triangle
  //    equilateral (0,0,0),(100,0,0),(50,86,0) a diam^2=10000 et
  //    4R^2 = 13328... ; a delta_out^2 = 15000, Jung refuse (3*10000=30000 >
  //    2*15000=30000 est faux de justesse) alors que le test exact certifie.
  {
    const P3 pts[3] = {{0, 0, 0}, {100, 0, 0}, {50, 86, 0}};
    const int ids[3] = {0, 1, 2};
    const IntSphere s = win::sphere_of(pts, ids, 3);
    const bool jung = win::jung_fast_path(pts, ids, 3, 15000);
    const bool exact = win::window_certified(s, 15000);
    say("jung-suffisant", s.ok && s.positive && !jung && exact,
        "Jung renonce la ou le test exact certifie : suffisant, pas necessaire");
  }
  return bad;
}

}  // namespace

int main(int argc, char** argv) {
  Options opt{};
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::size_t eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long p = 0;
    if (key == "--points") { if (!parse_ll(val.c_str(), &p)) refuse("--points invalide"); opt.n = p; }
    else if (key == "--coord") { if (!parse_ll(val.c_str(), &p)) refuse("--coord invalide"); opt.coord = p; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &p)) refuse("--seed invalide"); opt.seed = p; }
    else if (key == "--smax") { if (!parse_ll(val.c_str(), &p)) refuse("--smax invalide"); opt.smax = p; }
    else if (key == "--window") { if (!parse_ll(val.c_str(), &p)) refuse("--window invalide"); opt.cap = p; }
    else if (key == "--min-certifies") { if (!parse_ll(val.c_str(), &p)) refuse("--min-certifies invalide"); opt.min_certified = p; }
    else if (key == "--min-non-certifies") { if (!parse_ll(val.c_str(), &p)) refuse("--min-non-certifies invalide"); opt.min_uncertified = p; }
    else if (key == "--min-jamais-proposes") { if (!parse_ll(val.c_str(), &p)) refuse("--min-jamais-proposes invalide"); opt.min_jamais_proposes = p; }
    else if (key == "--min-shell") { if (!parse_ll(val.c_str(), &p)) refuse("--min-shell invalide"); opt.min_shell = p; }
    else if (key == "--min-jung") { if (!parse_ll(val.c_str(), &p)) refuse("--min-jung invalide"); opt.min_jung = p; }
    else if (key == "--min-chemin-exact") { if (!parse_ll(val.c_str(), &p)) refuse("--min-chemin-exact invalide"); opt.min_exact_path = p; }
    else if (key == "--judge") { opt.judge = true; }
    else if (key == "--cloud-egalite") { opt.cloud_egalite = true; }
    else if (key == "--fixtures") { opt.fixtures = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "window-cut-at-included") opt.mutant = WindowMutant::kCutAtIncluded;
      else if (val == "window-accept-equality") opt.mutant = WindowMutant::kAcceptEquality;
      else if (val == "window-skip-positivity") opt.mutant = WindowMutant::kSkipPositivity;
      else if (val == "window-narrow-i64") opt.mutant = WindowMutant::kNarrowI64;
      else if (val == "window-shell-as-interior") opt.mutant = WindowMutant::kShellAsInterior;
      else if (val == "window-jung-only") opt.mutant = WindowMutant::kJungOnly;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  if (opt.fixtures) {
    const int bad = run_fixtures(true);
    if (bad != 0) {
      std::fprintf(stderr, "REFUS : %d fixture(s) gravee(s) refutee(s)\n", bad);
      return 3;
    }
    std::printf("fixtures accord=OUI refutees=0\n");
    return 0;
  }

  if (opt.smax < 4 || opt.smax > 34) refuse("--smax hors du domaine d'enveloppe [4, 34]");
  // Le juge est exhaustif en C(n,4) sous-ensembles avec une geometrie
  // rationnelle : sa borne est un REFUS, pas un cap silencieux.
  if (opt.judge && opt.n > 40) refuse("le juge rationnel exhaustif est borne a --points <= 40");
  if (opt.n < 4 || opt.n > 2000) refuse("--points hors bornes [4, 2000]");
  if (opt.cap < 1 || opt.cap > 64) refuse("--window hors bornes [1, 64]");
  if (opt.mutant != WindowMutant::kNone && !opt.judge)
    refuse("un mutant sans juge ne prouve rien : --inject exige --judge");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  // NUAGE GRAVE A EGALITE. Aucune famille generique ne produit
  // `4R^2 = delta_out^2` : sur un nuage quelconque cette coincidence est de
  // mesure nulle, et le mutant `accept-equality` y SURVIT legitimement. La
  // fixture ci-dessous la force aux DEUX endpoints a la fois, sans quoi le
  // support serait certifie par l'autre et la divergence disparaitrait.
  //
  //   ids 0..3 = (0,0,0), (2,0,0), (0,2,0), (2,2,0)  puis (9,9,9)
  //
  // A `M=1`, l'ancre 0 garde l'id 1 (distance carree 4, plus petit PointId) et
  // omet l'id 2, egalement a 4 : `delta_out^2 = 4`. L'ancre 1 garde l'id 0 et
  // omet l'id 3, aussi a 4. Le support {0,1} a `4R^2 = 4` : egalite des deux
  // cotes. Le certificat strict refuse ; le mutant `<=` fabrique un support
  // dont le shell n'a jamais ete vu.
  const std::vector<P3> pts =
      opt.cloud_egalite
          ? std::vector<P3>{P3{0, 0, 0}, P3{2, 0, 0}, P3{0, 2, 0}, P3{2, 2, 0}, P3{9, 9, 9}}
          : mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  if (!opt.cloud_egalite && (long long)pts.size() != opt.n)
    refuse("le generateur n'a pas rendu la cardinalite demandee");
  const int n = (int)pts.size();

  Ledger led{};
  std::vector<Window> windows((std::size_t)n);
  // LE JUGE NE RECOIT JAMAIS LES FENETRES MUTEES. Un mutant qui deplace la
  // coupure deplacerait sinon aussi la definition de « ce que la fenetre doit
  // certifier », et sujet et juge se tromperaient ensemble — exactement le mode
  // commun que le contre-audit du cone avait exhibe sur `smax`. Les fenetres de
  // reference sont donc construites separement, sans injection.
  std::vector<Window> ref_windows((std::size_t)n);
  std::vector<Record> records;
  std::vector<BallKey> keys;
  for (int a = 0; a < n; ++a) {
    windows[(std::size_t)a] = build_window(pts, a, (int)opt.cap, opt.mutant, &led);
    Ledger sink{};
    ref_windows[(std::size_t)a] =
        (opt.mutant == WindowMutant::kNone)
            ? windows[(std::size_t)a]
            : build_window(pts, a, (int)opt.cap, WindowMutant::kNone, &sink);
    produce_anchor(pts, a, windows[(std::size_t)a], (int)opt.smax, opt.mutant, &led, &records,
                   &keys);
  }

  // Classes de `BallKey` : plusieurs supports peuvent nommer la meme boule.
  std::map<BallKey, int> group_of;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    auto it = group_of.find(keys[i]);
    if (it == group_of.end()) it = group_of.emplace(keys[i], (int)group_of.size()).first;
    records[i].ball_group = it->second;
  }

  // Un support peut etre certifie depuis plusieurs endpoints : c'est voulu, et
  // c'est meme la raison pour laquelle l'owner vient apres. On deduplique par
  // `SupportKey` pour l'identite, en verifiant que les copies coincident.
  std::map<std::vector<int>, Record> unique;
  long long duplicate_emissions = 0;
  for (const Record& r : records) {
    auto it = unique.find(r.support);
    if (it == unique.end()) {
      unique.emplace(r.support, r);
    } else {
      ++duplicate_emissions;
      if (it->second.interior != r.interior || it->second.shell != r.shell ||
          it->second.ball_group != r.ball_group)
        violate("deux emissions du meme SupportKey ne coincident pas");
    }
  }

  std::printf("WindowSourceReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%lld seed=%lld smax=%lld fenetre=%lld inject=%s\n",
              mhgp3v::cloud_family_name(opt.family), n, opt.coord, opt.seed, opt.smax, opt.cap,
              win::window_mutant_name(opt.mutant));
  std::printf("requete knn_queries=%lld point_tests=%lld scans_totaux=%lld\n", led.knn_queries,
              led.knn_point_tests, led.windows_total_scan);
  std::sort(led.per_anchor_products.begin(), led.per_anchor_products.end());
  auto pct = [&](double q) -> long long {
    if (led.per_anchor_products.empty()) return 0;
    return led.per_anchor_products[(std::size_t)(q * (double)(led.per_anchor_products.size() - 1))];
  };
  std::printf(
      "proposition tuples=%lld q2=%lld q3=%lld q4=%lld affinement_dependants=%lld "
      "tests_positivite=%lld candidats_positifs=%lld\n",
      led.tuples_proposed, led.products_q[2], led.products_q[3], led.products_q[4],
      led.affinely_dependent, led.positivity_tests, led.positive_candidates);
  // L'AUDIT EXIGE L'HISTOGRAMME PAR ANCRE : une moyenne masque l'ancre qui paie
  // C(M,3) seule. Et le rapport propositions/SupportKey unique est une PORTE,
  // pas un detail de profilage — une pente verte des seules emissions peut
  // masquer une pente quadratique des propositions.
  std::printf("propositions_par_ancre p50=%lld p90=%lld p99=%lld max=%lld\n", pct(0.50),
              pct(0.90), pct(0.99),
              led.per_anchor_products.empty() ? 0 : led.per_anchor_products.back());
  std::printf("census point_tests=%lld pertinents=%lld shell_max=%lld interieur_max=%lld\n",
              led.census_point_tests, led.relevant_candidates, led.shell_max, led.interior_max);
  std::printf(
      "certificat jung=%lld chemin_exact=%lld certifies=%lld q2=%lld q3=%lld q4=%lld "
      "emissions_dupliquees=%lld supports_uniques=%zu boules=%zu\n",
      led.jung_fast_hits, led.exact_path_calls, led.certified_supports, led.certified_q[2],
      led.certified_q[3], led.certified_q[4], duplicate_emissions, unique.size(), group_of.size());
  std::printf("rapport propositions_par_SupportKey_unique=%.2f\n",
              unique.empty() ? 0.0 : (double)led.tuples_proposed / (double)unique.size());
  // CE COMPTEUR N'EST PAS LE RESIDUEL. Il ne compte que les tuples FORMES puis
  // refuses. Les supports globaux dont aucun tuple n'a jamais ete forme dans une
  // fenetre n'y figurent pas ; seul le juge les voit.
  std::printf("candidats_refuses_non_residuel=%lld\n", led.uncertified_candidates);

  int disagreements = 0;
  long long never_proposed = -1;
  if (opt.judge) {
    const JudgeOut jj = judge_all(pts, (int)opt.smax, (int)opt.cap, ref_windows);
    never_proposed = jj.never_proposed;
    // SENS 1 : tout support certifie par le sujet est global, avec les MEMES
    // identites.
    std::map<std::vector<int>, Record> global_of;
    for (const Record& r : jj.global) global_of.emplace(r.support, r);
    long long faux_positifs = 0, identites_fausses = 0, printed = 0;
    for (const auto& kv : unique) {
      auto it = global_of.find(kv.first);
      if (it == global_of.end()) {
        ++faux_positifs;
        if (printed < 8) {
          std::fprintf(stderr, "DESACCORD : support certifie absent du juge :");
          for (int z : kv.first) std::fprintf(stderr, " %d", z);
          std::fprintf(stderr, "\n");
          ++printed;
        }
        continue;
      }
      if (it->second.interior != kv.second.interior || it->second.shell != kv.second.shell) {
        ++identites_fausses;
        if (printed < 8) {
          std::fprintf(stderr,
                       "DESACCORD : identites divergentes, sujet |I_B|=%d |U_B|=%d, "
                       "juge |I_B|=%d |U_B|=%d\n",
                       kv.second.interior, kv.second.shell, it->second.interior,
                       it->second.shell);
          ++printed;
        }
      }
    }
    // SENS 2a : tout support certifie DEVAIT l'etre. Sans cette inclusion, un
    // certificat trop large passe inapercu des que le census tombe juste par
    // hasard — et c'est exactement ce que le mutant `accept-equality` a montre :
    // il certifiait quatre supports au lieu d'un, tous globalement valides, donc
    // sans aucun faux positif ni identite fausse. Or leur census a ete calcule
    // sur une fenetre dont rien ne prouve qu'elle contenait leur boule.
    std::set<std::vector<int>> certifiable_set;
    for (const Record& r : jj.certifiable) certifiable_set.insert(r.support);
    long long sur_certifies = 0;
    for (const auto& kv : unique) {
      if (certifiable_set.count(kv.first) == 0) {
        ++sur_certifies;
        if (printed < 8) {
          std::fprintf(stderr, "DESACCORD : support certifie SANS y avoir droit :");
          for (int z : kv.first) std::fprintf(stderr, " %d", z);
          std::fprintf(stderr, "\n");
          ++printed;
        }
      }
    }
    // SENS 2b : tout support que la fenetre DEVAIT certifier l'est.
    long long manques = 0;
    for (const Record& r : jj.certifiable) {
      if (unique.find(r.support) == unique.end()) {
        ++manques;
        if (printed < 8) {
          std::fprintf(stderr, "DESACCORD : support certifiable manque au sujet :");
          for (int z : r.support) std::fprintf(stderr, " %d", z);
          std::fprintf(stderr, "\n");
          ++printed;
        }
      }
    }
    std::printf(
        "juge globaux=%zu certifiables=%zu sujet=%zu faux_positifs=%lld identites_fausses=%lld "
        "sur_certifies=%lld manques=%lld jamais_proposes=%lld accord=%s\n",
        jj.global.size(), jj.certifiable.size(), unique.size(), faux_positifs, identites_fausses,
        sur_certifies, manques, jj.never_proposed,
        (faux_positifs + identites_fausses + sur_certifies + manques) == 0 ? "OUI" : "NON");
    disagreements = (int)(faux_positifs + identites_fausses + sur_certifies + manques);
  }

  if (opt.mutant != WindowMutant::kNone) {
    if (disagreements > 0) {
      std::fprintf(stderr, "MUTANT TUE (juge) : %s\n", win::window_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }
  if (disagreements > 0) return 1;

  if (led.certified_supports < opt.min_certified) {
    std::fprintf(stderr, "REFUS : plancher de supports certifies (%lld < %lld)\n",
                 led.certified_supports, opt.min_certified);
    return 3;
  }
  if (led.uncertified_candidates < opt.min_uncertified) {
    std::fprintf(stderr, "REFUS : plancher de candidats refuses (%lld < %lld)\n",
                 led.uncertified_candidates, opt.min_uncertified);
    return 3;
  }
  if (led.shell_max < opt.min_shell) {
    std::fprintf(stderr, "REFUS : plancher de shell (%lld < %lld)\n", led.shell_max,
                 opt.min_shell);
    return 3;
  }
  if (led.jung_fast_hits < opt.min_jung) {
    std::fprintf(stderr, "REFUS : plancher du chemin rapide de Jung (%lld < %lld)\n",
                 led.jung_fast_hits, opt.min_jung);
    return 3;
  }
  if (led.exact_path_calls < opt.min_exact_path) {
    std::fprintf(stderr, "REFUS : plancher du chemin exact (%lld < %lld)\n", led.exact_path_calls,
                 opt.min_exact_path);
    return 3;
  }
  if (opt.min_jamais_proposes > 0) {
    if (!opt.judge) refuse("le plancher de supports jamais proposes exige --judge");
    if (never_proposed < opt.min_jamais_proposes) {
      std::fprintf(stderr,
                   "REFUS : plancher de supports jamais proposes (%lld < %lld) — sans eux, "
                   "l'insuffisance de la fenetre n'est pas mesuree\n",
                   never_proposed, opt.min_jamais_proposes);
      return 3;
    }
  }
  return 0;
}
