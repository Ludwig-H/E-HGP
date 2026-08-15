// MorseHGP3D v3 — PORTE DU CŒUR-BOULE EN FORME CLOSE.
//
// Sujet : `prototype/spindle_core_ball.hpp`.
// Specification : audits/NOTE_CLAUDE_COEUR_BOULE_FORME_CLOSE_20260815.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed. GCP non utilise.
//
// Ce probe grave la preuve AVANT toute mesure de fermeture, comme la note s'y
// engage. Il repond a trois questions, et a rien d'autre :
//
//   1. `kappa_q` est-il la bonne constante ? La fixture `tangence` l'encadre par
//      des couples ENTIERS a une unite pres, des deux cotes, pour les trois
//      lanes. Si une constante etait fausse d'un cheveu, un des six points
//      basculerait.
//   2. La boule est-elle DANS le fuseau ? Tout point certifie par la boule est
//      reconfronte a la force brute sur toutes les paires `(a,b)` du rectangle,
//      avec l'arithmetique du fuseau et non celle de la boule.
//   3. La descente avec elagage compte-t-elle la MEME chose que le balayage
//      direct ? C'est la porte de completude : un elagage trop gourmand perd
//      des temoins sans jamais mentir, donc aucune porte de surete ne le
//      verrait.
//
// Codes de sortie : 1 desaccord du juge, 2 refus avant calcul, 3 plancher ou
// invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/spindle_core_ball.hpp"
#include "prototype/wspd_wavefront.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp3v::WfNode;
using mhgp3v::corebl::BallMutant;
using mhgp3v::corebl::ball_contains_box;
using mhgp3v::corebl::ball_contains_point;
using mhgp3v::corebl::ball_disjoint_box;
using mhgp3v::corebl::ball_mutant_name;
using mhgp3v::corebl::core_ball_radius4;

struct P3 { i64 x, y, z; };

[[noreturn]] void refuse(const char* pourquoi) {
  std::fprintf(stderr, "REFUS : %s\n", pourquoi);
  std::exit(2);
}

// ---------------------------------------------------------------------------
// LE FUSEAU EXACT, EN ENTIERS. C'est le JUGE : il n'utilise aucune constante
// `kappa`, aucune boule, aucun rationnel. Il n'y a donc aucun defaut commun
// possible entre lui et le sujet.
//
//   e = z-a, t = b-z, H = e.t, Xi = |e|^2 |t|^2 - H^2
//   q2 : H > 0 ; q3 : 4H^2 > |e|^2|t|^2 ; q4 : 3H^2 > |e|^2|t|^2
// ---------------------------------------------------------------------------
bool dans_fuseau(const P3& a, const P3& b, const P3& z, int q) {
  const i64 e[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  const i64 t[3] = {b.x - z.x, b.y - z.y, b.z - z.z};
  const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
  if (h <= 0) return false;
  if (q == 2) return true;
  const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
  const i128 hh = (i128)h * (i128)h;
  const i128 ex = (i128)e2 * (i128)t2;
  return (q == 3) ? (4 * hh > ex) : (3 * hh > ex);
}

// ---------------------------------------------------------------------------
// FIXTURE `tangence` : LES SIX POINTS QUI GRAVENT LES TROIS CONSTANTES.
//
// `a = (1000,1000,1000)`, `b = (1100,1000,1000)`, donc `L = 100` et le milieu
// est `m = (1050,1000,1000)`. Un point equatorial `z = (1050, 1000+rho, 1000)`
// est dans `W_q` si et seulement si `rho < kappa_q * 100` :
//
//   kappa_2 * 100 = 50,0000  ->  rho=49 dedans, rho=50 dehors (H = 0 exactement)
//   kappa_3 * 100 = 28,8675  ->  rho=28 dedans, rho=29 dehors
//   kappa_4 * 100 = 25,8819  ->  rho=25 dedans, rho=26 dehors
//
// Les trois encadrements sont serres a une unite. Une constante fausse de plus
// de 1,2 % ferait basculer un des six verdicts, et la fixture le verrait.
//
// La meme fixture verifie que le rayon RATIONNEL du sujet reproduit ces six
// verdicts sur un rectangle degenere `A = {a}`, `B = {b}` — ou `S2 = 0`, donc
// ou le rayon vaut exactement `2 kappa_q L2` tronque :
//
//   q2 : R4 = 200 (exact)   q3 : R4 = 115 (vrai 115,47)   q4 : R4 = 103 (103,53)
//
// La troncature ne fait perdre aucun des six verdicts : c'est la marge que la
// section 5 de la note annonce, et elle est ici sous l'unite.
// ---------------------------------------------------------------------------
struct Tangence { int q; i64 rho; bool attendu; };

const Tangence kTangence[6] = {
    {2, 49, true},  {2, 50, false},
    {3, 28, true},  {3, 29, false},
    {4, 25, true},  {4, 26, false},
};

int fixture_tangence(BallMutant mu, bool verbeux) {
  const P3 a{1000, 1000, 1000}, b{1100, 1000, 1000};
  // Rectangle degenere : chaque boite est un point, donc `c2 = 2p` et `r2 = 0`.
  const i64 c2a[3] = {2000, 2000, 2000};
  const i64 c2b[3] = {2200, 2000, 2000};
  const i64 M[3] = {c2a[0] + c2b[0], c2a[1] + c2b[1], c2a[2] + c2b[2]};
  const i64 d2 = 200;  // |c2b - c2a|, exact ici

  long long juge_ok = 0, boule_ok = 0, faux = 0, perdus = 0;
  for (const Tangence& t : kTangence) {
    const P3 z{1050, 1000 + t.rho, 1000};
    const bool juge = dans_fuseau(a, b, z, t.q);
    if (juge != t.attendu) {
      std::fprintf(stderr,
                   "PLANCHER : le fuseau exact contredit la fixture gravee "
                   "(q%d, rho=%lld : %s au lieu de %s)\n",
                   t.q, (long long)t.rho, juge ? "dedans" : "dehors",
                   t.attendu ? "dedans" : "dehors");
      return 3;
    }
    ++juge_ok;
    const i64 R4 = core_ball_radius4(d2, 0, 0, t.q, mu);
    const i64 p[3] = {z.x, z.y, z.z};
    const bool boule = ball_contains_point(M, R4, p, mu);
    if (boule) ++boule_ok;
    if (boule && !juge) ++faux;    // la boule sort du fuseau : FAUTE
    if (!boule && juge) ++perdus;  // conservatisme : sûr, mais a compter
    if (verbeux)
      std::printf("tangence q%d rho=%lld R4=%lld juge=%d boule=%d\n", t.q,
                  (long long)t.rho, (long long)R4, (int)juge, (int)boule);
  }
  std::printf("fixture tangence juge_ok=%lld boule_certifie=%lld faux=%lld "
              "perdus=%lld mutant=%s\n",
              juge_ok, boule_ok, faux, perdus, ball_mutant_name(mu));
  if (faux > 0) {
    std::fprintf(stderr, "DESACCORD : la boule certifie %lld point(s) hors du fuseau\n", faux);
    return 1;
  }
  // NON-VACUITE : sans mutant, les trois points interieurs doivent etre
  // certifies. Une boule vide passerait autrement toutes les portes de surete.
  if (mu == BallMutant::kNone && boule_ok != 3) {
    std::fprintf(stderr, "PLANCHER : %lld points certifies au lieu de 3 —"
                 " la troncature rationnelle mange un verdict\n", boule_ok);
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// FIXTURE `derive` : LE TERME `-S2` N'EST PAS DECORATIF.
//
// Le mutant `boule-oublie-derive` retire la correction de deplacement du
// milieu. Il faut donc une fixture ou `A` est assez large pour que le milieu
// bouge, et un temoin place dans l'ecart. `A` est le segment `x` dans
// `[1000,1040]`, `B` le point `(2000,1000,1000)`.
//
// Le vrai rayon tient compte de ce que `m_ab` se promene sur `20` unites ;
// l'oubli donne une boule centree au milieu des CENTRES, qui deborde du cote
// de `A` et certifie un point que la paire extreme refute.
// ---------------------------------------------------------------------------
int fixture_derive(BallMutant mu) {
  const i64 alo[3] = {1000, 1000, 1000}, ahi[3] = {1040, 1000, 1000};
  const i64 blo[3] = {2000, 1000, 1000}, bhi[3] = {2000, 1000, 1000};
  const i64 c2a[3] = {alo[0] + ahi[0], alo[1] + ahi[1], alo[2] + ahi[2]};
  const i64 c2b[3] = {blo[0] + bhi[0], blo[1] + bhi[1], blo[2] + bhi[2]};
  const i64 M[3] = {c2a[0] + c2b[0], c2a[1] + c2b[1], c2a[2] + c2b[2]};
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 e = c2b[i] - c2a[i];
    acc += (i128)e * (i128)e;
  }
  i64 d2 = 0;
  while ((i128)(d2 + 1) * (d2 + 1) <= acc) ++d2;  // racine entiere INFERIEURE
  // Rayons doubles majorants : `A` a pour demi-diagonale doublee 40, `B` 0.
  const i64 rA2 = 40, rB2 = 0;

  long long faux = 0, certifies = 0;
  for (int q = 2; q <= 4; ++q) {
    const i64 R4 = core_ball_radius4(d2, rA2, rB2, q, mu);
    if (R4 <= 0) continue;
    // On balaie l'axe et on confronte chaque point certifie a la force brute
    // sur les deux extremites de `A`, qui sont le pire cas.
    for (i64 x = 1000; x <= 2000; ++x)
      for (i64 dy = -60; dy <= 60; ++dy) {
        const P3 z{x, 1000 + dy, 1000};
        const i64 p[3] = {z.x, z.y, z.z};
        if (!ball_contains_point(M, R4, p, mu)) continue;
        ++certifies;
        const P3 b{blo[0], blo[1], blo[2]};
        for (i64 ax = alo[0]; ax <= ahi[0]; ++ax) {
          const P3 a{ax, alo[1], alo[2]};
          if (z.x == a.x && z.y == a.y && z.z == a.z) continue;
          if (!dans_fuseau(a, b, z, q)) { ++faux; ax = ahi[0]; }
        }
      }
  }
  std::printf("fixture derive certifies=%lld faux=%lld mutant=%s\n", certifies, faux,
              ball_mutant_name(mu));
  if (faux > 0) {
    std::fprintf(stderr, "DESACCORD : %lld point(s) certifies hors du fuseau d'une paire\n", faux);
    return 1;
  }
  if (mu == BallMutant::kNone && certifies == 0) {
    std::fprintf(stderr, "PLANCHER : aucun point certifie, la fixture est vacue\n");
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// FIXTURE `seuil` : LE SEUIL DE SEPARATION, ET UNE ERREUR CORRIGEE.
//
// La premiere redaction de la note annoncait `s > 2 + 1/kappa_q`, soit
// 4,00 / 5,46 / 5,86. C'ETAIT FAUX, et la mesure l'a montre tout de suite : a
// `s = 4` tous les cœurs etaient deja non vides. L'erreur porte sur la
// convention de separation. Le dossier ne sepe pas sur la distance des centres
// `d >= s max(r)` mais sur l'ECART des boules :
//
//   `separated()` rend `d - r_A - r_B >= s max(r_A, r_B)`.
//
// Donc `L2 = d2 - S2 >= s rmax` directement, et avec `S2 <= 2 rmax` :
//
//   R4 = 2 kappa_q L2 - S2 >= 2 rmax (kappa_q s - 1),
//
// positif si et seulement si `s > 1/kappa_q`, soit 2,000 / 3,464 / 3,864 — et
// non 4,00 / 5,46 / 5,86. Le cœur nait BEAUCOUP plus tot que je ne l'ai ecrit.
//
// Cette fixture grave le seuil corrige avec des rayons egaux `rA2 = rB2 = 1000`
// et `L2 = 1000 s`, de sorte que `R4 = floor(2 kappa_q * 1000 s) - 2000` :
//
//   s = 2,0 : q2 = 0        (vide, le seuil q2 est exactement 2)
//   s = 3,0 : q2 = +1000    q3 = -268     q4 = -448
//   s = 3,6 : q3 = +78      q4 = -137     <- SEPARE q3 DE q4
//   s = 4,0 : q3 = +309     q4 = +70
//
// La ligne `s = 3,6` est celle qui compte : elle tient entre `1/kappa_3` et
// `1/kappa_4`, donc elle ne peut pas passer si les deux constantes etaient
// confondues, ni si l'une d'elles bougeait de plus de trois centiemes.
// ---------------------------------------------------------------------------
struct Seuil { i64 L2; int q; bool positif; const char* quoi; };

const Seuil kSeuils[10] = {
    {2000, 2, false, "s=2,0 q2 au seuil exact"},
    {3000, 2, true,  "s=3,0 q2 au-dessus"},
    {3000, 3, false, "s=3,0 q3 sous 1/kappa_3"},
    {3000, 4, false, "s=3,0 q4 sous 1/kappa_4"},
    {3600, 3, true,  "s=3,6 q3 AU-DESSUS de 1/kappa_3"},
    {3600, 4, false, "s=3,6 q4 SOUS 1/kappa_4"},
    {4000, 3, true,  "s=4,0 q3 au-dessus"},
    {4000, 4, true,  "s=4,0 q4 au-dessus"},
    // LES DEUX LIGNES QUI SERRENT LES CONSTANTES A ENVIRON UN CENTIEME. Le
    // seuil vaut `2/(2 kappa_q)` ; a `s=3,4` le vrai q3 est encore vide (R4=-37)
    // alors qu'un `2 kappa_3` sur-estime a 3/5 le rendrait positif, et de meme a
    // `s=3,84` pour q4 contre 21/40. Sans elles, les deux mutants de constante
    // survivraient a cette fixture.
    {3400, 3, false, "s=3,4 q3 encore sous 1/kappa_3, a 37 unites pres"},
    {3840, 4, false, "s=3,84 q4 encore sous 1/kappa_4, a 13 unites pres"},
};

int fixture_seuil(BallMutant mu) {
  const i64 rA2 = 1000, rB2 = 1000;
  long long ok = 0;
  for (const Seuil& s : kSeuils) {
    const i64 R4 = core_ball_radius4(s.L2 + rA2 + rB2, rA2, rB2, s.q, mu);
    const bool positif = R4 > 0;
    if (positif != s.positif) {
      std::fprintf(stderr, "PLANCHER : %s — R4=%lld, attendu %s\n", s.quoi,
                   (long long)R4, s.positif ? "positif" : "vide");
      return 3;
    }
    ++ok;
  }
  std::printf("fixture seuil verifies=%lld mutant=%s\n", ok, ball_mutant_name(mu));
  return 0;
}

// ---------------------------------------------------------------------------
// FIXTURE `apex` : LA DEMI-OUVERTURE DU CONE, ET CE QUE LA BOULE EN RATE.
//
// `A` est la boite `[1000,1100]^3` — donc `r2_A = 2 r_A` majore la demi-diagonale
// — et `B` le point `(20000,1050,1050)`, tres loin sur l'axe des `x`. On prend
// `a = (1000,1050,1050)`, l'extremite proche de `A` sur l'axe.
//
// La demi-ouverture vaut `gamma_q = theta'_q - arcsin((r_B + 2 r_A)/D)`. Ici
// `r_B = 0` et `D = 19000`, donc la correction est petite et `gamma_q` reste
// proche de `theta'_q`. La fixture balaie un plan et confronte, point par point,
// ce que la boule d'apex certifie a ce que le FUSEAU EXACT dit sur la paire
// `(a,b)` — puisque `B` est reduit a un point, la force brute est exacte et
// complete, sans aucune approximation du cote du juge.
//
// Deux compteurs, et ils ne jouent pas le meme role. `faux` doit rester nul :
// c'est la surete. `rates` mesure ce que la boule inscrite perd contre le cone,
// et il n'est PAS nul — c'est le prix documente de la question Q23, pas un
// defaut. Il est publie pour que la substitution se decide sur un chiffre.
// ---------------------------------------------------------------------------
int fixture_apex(BallMutant mu, i64 bx, i64 alo, i64 ahi) {
  const P3 a{alo, 1050, 1050};
  const P3 b{bx, 1050, 1050};
  const i64 c2b[3] = {2 * b.x, 2 * b.y, 2 * b.z};
  const i64 ud[3] = {c2b[0] - 2 * a.x, c2b[1] - 2 * a.y, c2b[2] - 2 * a.z};
  // Rayon double majorant de `A` : demi-diagonale de la boite, doublee.
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) { const i64 e = ahi - alo; acc += (i128)e * e; }
  const i64 rA2 = mhgp3v::corebl::isqrt_floor_i128(acc) + 1;

  long long certifies = 0, faux = 0, rates = 0, vrais = 0;
  for (int q = 2; q <= 4; ++q) {
    for (i64 x = alo; x <= ahi; ++x)
      for (i64 y = alo; y <= ahi; ++y) {
        const P3 z{x, y, 1050};
        if (z.x == a.x && z.y == a.y && z.z == a.z) continue;
        const i64 ed[3] = {2 * z.x - 2 * a.x, 2 * z.y - 2 * a.y, 2 * z.z - 2 * a.z};
        const bool boule = mhgp3v::corebl::apex_ball_contains(ud, rA2, 0, ed, q, mu);
        const bool juge = dans_fuseau(a, b, z, q);
        if (boule) ++certifies;
        if (juge) ++vrais;
        if (boule && !juge) ++faux;
        if (!boule && juge) ++rates;
      }
  }
  std::printf("fixture apex bx=%lld certifies=%lld vrais=%lld faux=%lld rates=%lld mutant=%s\n",
              (long long)bx, certifies, vrais, faux, rates, ball_mutant_name(mu));
  if (faux > 0) {
    std::fprintf(stderr, "DESACCORD : la boule d'apex certifie %lld point(s) hors du fuseau\n",
                 faux);
    return 1;
  }
  if (mu == BallMutant::kNone && certifies == 0) {
    std::fprintf(stderr, "PLANCHER : la boule d'apex ne certifie rien, la fixture est vacue\n");
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// MODE NUAGE : COMPLETUDE DE LA DESCENTE, ET SURETE CONTRE LA FORCE BRUTE.
// ---------------------------------------------------------------------------
struct Rect { int u, v; };

struct Compte {
  long long rectangles = 0;
  long long coeurs_non_vides[3] = {0, 0, 0};
  long long direct[3] = {0, 0, 0};     // balayage de tous les points
  long long descente[3] = {0, 0, 0};   // descente avec elagage et credit bloc
  long long bulk = 0;                  // sous-arbres credites en O(1)
  long long elagues = 0;               // sous-arbres coupes
  long long visites = 0;
  long long faux[3] = {0, 0, 0};       // certifie par la boule, refute au fuseau
  long long verifies = 0;
};

int mode_nuage(const std::string& family, int n, i64 coord, long long seed, int sep,
               BallMutant mu, long long min_coeurs, long long min_bulk,
               long long min_elagues) {
  if (n < 4 || n > 4000) refuse("le mode nuage est borne a 4000 points");
  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else refuse("famille inconnue");
  const int coord_used =
      coord > 0 ? (int)coord : mhgp3v::cloud_family_default_coord(fam, n);
  const auto raw = mhgp3v::make_family_cloud(fam, n, coord_used, seed);
  const int m = (int)raw.size();
  std::vector<P3> pts((size_t)m);
  for (int i = 0; i < m; ++i)
    pts[(size_t)i] = P3{raw[(size_t)i].x, raw[(size_t)i].y, raw[(size_t)i].z};

  // Index Morton, puis arbre : c'est la structure DEJA presente. Le cœur-boule
  // n'en demande pas d'autre — seulement les deux primitives sphere-boite.
  std::vector<unsigned long long> keys((size_t)m);
  for (int i = 0; i < m; ++i)
    keys[(size_t)i] = mhgp3v::wf_morton48(pts[(size_t)i].x, pts[(size_t)i].y, pts[(size_t)i].z);
  std::vector<int> order((size_t)m);
  for (int i = 0; i < m; ++i) order[(size_t)i] = i;
  std::sort(order.begin(), order.end(), [&](int A, int B) {
    return keys[(size_t)A] != keys[(size_t)B] ? keys[(size_t)A] < keys[(size_t)B] : A < B;
  });
  std::vector<unsigned long long> sk((size_t)m);
  std::vector<P3> sp((size_t)m);
  for (int i = 0; i < m; ++i) { sk[(size_t)i] = keys[(size_t)order[i]]; sp[(size_t)i] = pts[(size_t)order[i]]; }
  std::vector<WfNode> nodes = mhgp3v::wf_build(sk);
  {
    std::vector<std::array<long long, 3>> rr((size_t)m);
    for (int i = 0; i < m; ++i) rr[(size_t)i] = {sp[(size_t)i].x, sp[(size_t)i].y, sp[(size_t)i].z};
    mhgp3v::wf_tight_boxes(&nodes, rr);
  }

  auto h_leaf = [&](int h) { return h < 0; };
  auto h_first = [&](int h) { return h >= 0 ? nodes[(size_t)h].first : -1 - h; };
  auto h_last = [&](int h) { return h >= 0 ? nodes[(size_t)h].last : -1 - h; };
  auto h_pop = [&](int h) { return h_last(h) - h_first(h) + 1; };
  auto h_lo = [&](int h, i64 out[3]) {
    if (h >= 0) { for (int i = 0; i < 3; ++i) out[i] = nodes[(size_t)h].tlo[i]; }
    else { const P3& p = sp[(size_t)(-1 - h)]; out[0] = p.x; out[1] = p.y; out[2] = p.z; }
  };
  auto h_hi = [&](int h, i64 out[3]) {
    if (h >= 0) { for (int i = 0; i < 3; ++i) out[i] = nodes[(size_t)h].thi[i]; }
    else { const P3& p = sp[(size_t)(-1 - h)]; out[0] = p.x; out[1] = p.y; out[2] = p.z; }
  };
  // Sphere englobante DOUBLEE, majorante — meme convention que le probe combine.
  auto h_sphere = [&](int h, i64 c2[3], i64* r2) {
    i64 lo[3], hi[3];
    h_lo(h, lo); h_hi(h, hi);
    i128 acc = 0;
    for (int i = 0; i < 3; ++i) {
      c2[i] = lo[i] + hi[i];
      const i64 e = hi[i] - lo[i];
      acc += (i128)e * (i128)e;
    }
    i64 r = 0;
    while ((i128)(r + 1) * (r + 1) <= acc) ++r;
    *r2 = r + 1;
  };

  std::vector<Rect> rects;
  {
    std::vector<Rect> st;
    for (size_t i = 0; i < nodes.size(); ++i)
      st.push_back({nodes[i].left, nodes[i].right});
    while (!st.empty()) {
      const Rect r = st.back();
      st.pop_back();
      i64 ca[3], cb[3], ra, rb;
      h_sphere(r.u, ca, &ra); h_sphere(r.v, cb, &rb);
      i128 acc = 0;
      for (int i = 0; i < 3; ++i) { const i64 e = cb[i] - ca[i]; acc += (i128)e * (i128)e; }
      i64 d = 0;
      while ((i128)(d + 1) * (d + 1) <= acc) ++d;
      const i64 rmax = ra > rb ? ra : rb;
      if ((i128)d - ra - rb >= (i128)sep * rmax) { rects.push_back(r); continue; }
      const bool ui = !h_leaf(r.u), vi = !h_leaf(r.v);
      if (!ui && !vi) { rects.push_back(r); continue; }
      const bool su = ui && (!vi || h_pop(r.u) >= h_pop(r.v));
      if (su) { st.push_back({nodes[(size_t)r.u].left, r.v}); st.push_back({nodes[(size_t)r.u].right, r.v}); }
      else    { st.push_back({r.u, nodes[(size_t)r.v].left}); st.push_back({r.u, nodes[(size_t)r.v].right}); }
    }
  }

  Compte C;
  C.rectangles = (long long)rects.size();
  for (const Rect& r : rects) {
    const int ua = h_first(r.u), ub = h_last(r.u);
    const int va = h_first(r.v), vb = h_last(r.v);
    i64 ca[3], cb[3], ra, rb;
    h_sphere(r.u, ca, &ra); h_sphere(r.v, cb, &rb);
    i128 acc = 0;
    for (int i = 0; i < 3; ++i) { const i64 e = cb[i] - ca[i]; acc += (i128)e * (i128)e; }
    i64 d2 = 0;
    while ((i128)(d2 + 1) * (d2 + 1) <= acc) ++d2;
    const i64 M[3] = {ca[0] + cb[0], ca[1] + cb[1], ca[2] + cb[2]};

    for (int q = 2; q <= 4; ++q) {
      const i64 R4 = core_ball_radius4(d2, ra, rb, q, mu);
      if (R4 <= 0) continue;
      ++C.coeurs_non_vides[q - 2];

      // --- (a) BALAYAGE DIRECT, la reference du comptage.
      long long direct = 0;
      for (int i = 0; i < m; ++i) {
        if ((i >= ua && i <= ub) || (i >= va && i <= vb)) continue;  // disjonction
        const i64 p[3] = {sp[(size_t)i].x, sp[(size_t)i].y, sp[(size_t)i].z};
        if (ball_contains_point(M, R4, p, mu)) ++direct;
      }
      C.direct[q - 2] += direct;

      // --- (b) DESCENTE : credit en bloc, elagage, feuilles. C'est le chemin
      // qui remplacera les soixante-quatre predicats par site.
      long long desc = 0;
      std::vector<int> st;
      if (!nodes.empty()) st.push_back(0);
      else if (m == 1) st.push_back(-1);
      while (!st.empty()) {
        const int h = st.back();
        st.pop_back();
        ++C.visites;
        i64 lo[3], hi[3];
        h_lo(h, lo); h_hi(h, hi);
        if (ball_disjoint_box(M, R4, lo, hi, mu)) { ++C.elagues; continue; }
        const int fi = h_first(h), la = h_last(h);
        const bool touche = !(la < ua || fi > ub) || !(la < va || fi > vb);
        if (!touche && ball_contains_box(M, R4, lo, hi, mu)) {
          desc += (la - fi + 1);  // POPULATION, en O(1) : c'est tout l'interet
          ++C.bulk;
          continue;
        }
        if (h_leaf(h)) {
          const int i = -1 - h;
          if ((i >= ua && i <= ub) || (i >= va && i <= vb)) continue;
          const i64 p[3] = {sp[(size_t)i].x, sp[(size_t)i].y, sp[(size_t)i].z};
          if (ball_contains_point(M, R4, p, mu)) ++desc;
          continue;
        }
        st.push_back(nodes[(size_t)h].left);
        st.push_back(nodes[(size_t)h].right);
      }
      C.descente[q - 2] += desc;
      if (desc != direct) {
        std::fprintf(stderr,
                     "PLANCHER : descente=%lld direct=%lld sur le rectangle "
                     "(%d,%d) lane q%d — l'elagage ou le credit en bloc perd\n",
                     desc, direct, r.u, r.v, q);
        return 3;
      }

      // --- (c) SURETE : chaque point certifie est reconfronte au FUSEAU EXACT
      // sur toutes les paires du rectangle. Le juge n'utilise aucune boule.
      for (int i = 0; i < m; ++i) {
        if ((i >= ua && i <= ub) || (i >= va && i <= vb)) continue;
        const i64 p[3] = {sp[(size_t)i].x, sp[(size_t)i].y, sp[(size_t)i].z};
        if (!ball_contains_point(M, R4, p, mu)) continue;
        ++C.verifies;
        bool ok = true;
        for (int ai = ua; ai <= ub && ok; ++ai)
          for (int bi = va; bi <= vb && ok; ++bi)
            if (!dans_fuseau(sp[(size_t)ai], sp[(size_t)bi], sp[(size_t)i], q)) ok = false;
        if (!ok) ++C.faux[q - 2];
      }
    }
  }

  std::printf("CoreBallReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cpu_reference "
              "profile=quantized_u16_input_only mode=diagnostic_counter_only "
              "public_status=not_claimed\n");
  std::printf("nuage famille=%s n=%d coord=%d seed=%lld separation=%d mutant=%s\n",
              family.c_str(), m, coord_used, seed, sep, ball_mutant_name(mu));
  std::printf("rectangles=%lld visites=%lld bulk=%lld elagues=%lld verifies=%lld\n",
              C.rectangles, C.visites, C.bulk, C.elagues, C.verifies);
  for (int q = 0; q < 3; ++q)
    std::printf("lane q%d coeurs_non_vides=%lld direct=%lld descente=%lld faux=%lld\n",
                q + 2, C.coeurs_non_vides[q], C.direct[q], C.descente[q], C.faux[q]);

  for (int q = 0; q < 3; ++q)
    if (C.faux[q] != 0) {
      std::fprintf(stderr, "DESACCORD : lane q%d, %lld temoins certifies hors du fuseau\n",
                   q + 2, C.faux[q]);
      return 1;
    }
  // PLANCHERS DE NON-VACUITE. Sans eux, une boule toujours vide passerait
  // toutes les portes de surete et de completude ci-dessus.
  const long long coeurs = C.coeurs_non_vides[0] + C.coeurs_non_vides[1] + C.coeurs_non_vides[2];
  if (coeurs < min_coeurs) {
    std::fprintf(stderr, "PLANCHER : %lld coeurs non vides < %lld\n", coeurs, min_coeurs);
    return 3;
  }
  if (C.bulk < min_bulk) {
    std::fprintf(stderr, "PLANCHER : %lld credits en bloc < %lld —"
                 " la voie O(1) n'est pas exercee\n", C.bulk, min_bulk);
    return 3;
  }
  if (C.elagues < min_elagues) {
    std::fprintf(stderr, "PLANCHER : %lld elagages < %lld —"
                 " la primitive de disjonction n'est pas exercee\n", C.elagues, min_elagues);
    return 3;
  }
  return 0;
}

BallMutant mutant_de(const std::string& s) {
  if (s == "none") return BallMutant::kNone;
  if (s == "boule-kappa3-sur") return BallMutant::kKappaQ3Sur;
  if (s == "boule-kappa4-sur") return BallMutant::kKappaQ4Sur;
  if (s == "boule-oublie-derive") return BallMutant::kOublieDerive;
  if (s == "boule-rayon-plus-deux") return BallMutant::kRayonPlusDeux;
  if (s == "boule-inclus-large") return BallMutant::kInclusLarge;
  if (s == "boule-disjoint-centre") return BallMutant::kDisjointCentre;
  if (s == "apex-oublie-rayon") return BallMutant::kApexOublieRayon;
  if (s == "apex-racine-oubliee") return BallMutant::kApexRacineOubliee;
  if (s == "apex-longueur-double") return BallMutant::kApexLongueurDouble;
  if (s == "apex-coeff-echange") return BallMutant::kApexCoeffEchange;
  refuse("mutant inconnu");
}

long long arg_ll(const std::string& s) {
  long long v = 0;
  const char* b = s.c_str();
  auto r = std::from_chars(b, b + s.size(), v);
  if (r.ec != std::errc()) refuse("entier illisible");
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform", fixture, mut = "none";
  int n = 200, sep = 8;
  i64 coord = 65535;
  long long seed = 3, min_coeurs = 0, min_bulk = 0, min_elagues = 0;
  bool verbeux = false;
  i64 ax_bx = 0, ax_lo = 1000, ax_hi = 1100;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--fixture=", 0) == 0) { fixture = a.substr(10); continue; }
    if (a.rfind("--family=", 0) == 0) { family = a.substr(9); continue; }
    if (a.rfind("--points=", 0) == 0) { n = (int)arg_ll(a.substr(9)); continue; }
    if (a.rfind("--coord=", 0) == 0) { coord = arg_ll(a.substr(8)); continue; }
    if (a.rfind("--seed=", 0) == 0) { seed = arg_ll(a.substr(7)); continue; }
    if (a.rfind("--separation=", 0) == 0) { sep = (int)arg_ll(a.substr(13)); continue; }
    if (a.rfind("--inject=", 0) == 0) { mut = a.substr(9); continue; }
    if (a.rfind("--min-coeurs=", 0) == 0) { min_coeurs = arg_ll(a.substr(13)); continue; }
    if (a.rfind("--min-bulk=", 0) == 0) { min_bulk = arg_ll(a.substr(11)); continue; }
    if (a.rfind("--min-elagues=", 0) == 0) { min_elagues = arg_ll(a.substr(14)); continue; }
    if (a.rfind("--apex-bx=", 0) == 0) { ax_bx = arg_ll(a.substr(10)); continue; }
    if (a.rfind("--apex-lo=", 0) == 0) { ax_lo = arg_ll(a.substr(10)); continue; }
    if (a.rfind("--apex-hi=", 0) == 0) { ax_hi = arg_ll(a.substr(10)); continue; }
    if (a == "--verbeux") { verbeux = true; continue; }
    refuse("argument inconnu");
  }
  const BallMutant mu = mutant_de(mut);
  if (fixture == "tangence") return fixture_tangence(mu, verbeux);
  if (fixture == "derive") return fixture_derive(mu);
  if (fixture == "seuil") return fixture_seuil(mu);
  if (fixture == "apex") return fixture_apex(mu, ax_bx ? ax_bx : 20000, ax_lo, ax_hi);
  if (fixture == "apex-serre") return fixture_apex(mu, ax_bx ? ax_bx : 1600, ax_lo, ax_hi);
  if (!fixture.empty()) refuse("fixture inconnue");
  return mode_nuage(family, n, coord, seed, sep, mu, min_coeurs, min_bulk, min_elagues);
}
