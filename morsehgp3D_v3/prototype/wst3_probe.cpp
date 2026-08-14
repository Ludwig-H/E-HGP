// MorseHGP3D v3 — `OwnedCK-WST3` : LA SOURCE D'ORDRE TROIS, COUNTER-ONLY.
//
// Specification : section 5.2 de `PROPOSITION.md` et section 6 de
// audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=source_factorisee_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE PROBE FAIT, ET CE QU'IL NE FAIT PAS
//
// Une WSPD decompose les `C(n,2)` paires en `O(s^3 n)` rectangles. La question
// est de l'etendre aux TRIPLETS sans developper un seul triangle. Ce probe
// construit cette extension et la COMPTE ; il n'emet aucun triangle, ne calcule
// aucun circumcentre et ne decide aucun rang.
//
// ---------------------------------------------------------------------------
// LE DOMAINE DU TROISIEME SOMMET, ET POURQUOI IL EST BORNE
//
// Chaque triangle est attribue a son arete MAXIMALE — l'owner, avec tie-break
// par `EdgeKey`. Si `ab` est l'arete maximale de `{a,b,x}`, alors par
// definition :
//
//   ||x-a|| <= ||b-a||   et   ||x-b|| <= ||b-a||
//
// Le troisieme sommet vit donc dans la LENTILLE, l'intersection des deux boules
// de rayon `||b-a||` centrees aux deux endpoints. Pour un rectangle `(A,B)`
// entier, une condition NECESSAIRE — donc sure pour la couverture — s'obtient
// en majorant `||b-a||` par `Dmax(A,B)` et en remplacant chaque endpoint par sa
// boite :
//
//   dist(x, A) <= Dmax(A,B)   et   dist(x, B) <= Dmax(A,B)
//
// où `dist` est la distance a la BOITE. Aucune racine carree n'est formee : les
// deux membres sont compares au carre, et la distance a une boite se separe par
// axe. C'est exact sur les entiers.
//
// ---------------------------------------------------------------------------
// LA DECOMPOSITION EN BLOCS
//
// Pour chaque rectangle `(A,B)`, on descend l'arbre des temoins :
//   - un nœud ENTIEREMENT dans le domaine devient un bloc `(A,B,C)` ;
//   - un nœud entierement HORS du domaine est elague ;
//   - sinon on descend, et une feuille indecise devient son propre bloc.
// Les blocs emis sont donc deux a deux disjoints et couvrent exactement le
// domaine : c'est une partition, condition de l'exact-once.
//
// ---------------------------------------------------------------------------
// LE JUGE
//
// A petit `n`, il enumere les `C(n,3)` triangles, determine l'arete maximale de
// chacun par `EdgeKey`, et exige que le troisieme sommet appartienne a
// EXACTEMENT UN bloc du rectangle owner. Une couverture manquante et un doublon
// sont deux fautes distinctes, comptees separement.
//
// Codes : 1 = desaccord du juge, 2 = refus avant calcul, 3 = plancher,
// 4 = mutant tue.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "corner8_ball.hpp"
#include "midball_block.hpp"
#include "rect_front.hpp"
#include "wspd_wavefront.hpp"

namespace {

using mhgp::i64;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS: %s\n", why);
  std::exit(2);
}

long long arg_ll(const char* s, long long lo, long long hi, const char* nom) {
  char* fin = nullptr;
  const long long v = std::strtoll(s, &fin, 10);
  if (fin == s || (fin != nullptr && *fin != '\0')) refuse(nom);
  if (v < lo || v > hi) refuse(nom);
  return v;
}

enum class Wst3Mutant {
  kNone,
  kRayonMin,        // `Dmin` au lieu de `Dmax` : la lentille retrecit, on perd
  kUneSeuleBoule,   // oublie la contrainte sur `B` : sur-couverture
  kPasDeDescente,   // n'emet que la racine : un seul bloc, jamais disjoint
};

const char* mutant_name(Wst3Mutant m) {
  switch (m) {
    case Wst3Mutant::kNone: return "none";
    case Wst3Mutant::kRayonMin: return "wst3-rayon-min";
    case Wst3Mutant::kUneSeuleBoule: return "wst3-une-seule-boule";
    case Wst3Mutant::kPasDeDescente: return "wst3-pas-de-descente";
  }
  return "?";
}

// ---- LE FILTRE D'ACUITE, ET LE SIGNE QUE J'AVAIS INVERSE.
//
// La face `(a,b,x)` est aigue au sommet `x` lorsque `(a-x).(b-x) > 0`. Or
// `H = (x-a).(b-x) = -(a-x).(b-x)`, donc l'acuite est `H < 0` — et non `H > 0`.
// L'identite le confirme : avec `b-a = (b-x) + (x-a)`,
// `D = X + E + 2H`, donc `E + X - D = -2H`.
//
// C'est exactement Thales, dans le bon sens : a l'INTERIEUR de la boule
// diametrale l'angle vu depuis `x` est OBTUS, sur la sphere il est droit, et
// c'est a l'EXTERIEUR qu'il est aigu. Ma premiere ecriture gardait donc les
// blocs obtus et jetait les aigus, ce qui perd des q4 bien centres — le
// contre-audit l'a pris en flagrant delit sur le tetraedre regulier
// `(0,0,0),(0,1,1),(1,0,1),(1,1,0)`, dont les deux carriers ont `H=-1`.
//
// La classification exacte de bloc est donc :
//
//   Hmax <  0  -> ALL_ACUTE
//   Hmin >= 0  -> NONE_ACUTE     (l'egalite est droite, donc NON aigue)
//   sinon      -> MIXED
//
// Un couple `{C,D}` n'est forme que si l'un des deux blocs peut porter un
// carrier aigu, c'est-a-dire des que `Hmin < 0` — le `OR` du lemme du porteur,
// jamais le `AND`.
struct Bloc {
  int c;
  bool aigu;   // le bloc peut-il contenir un carrier aigu ?
};

// Distance carree MAXIMALE d'un point de `C` a la boite `A`. Separable par axe :
// sur chaque axe, la distance a l'intervalle est une fonction convexe par
// morceaux dont le maximum sur `[C.lo,C.hi]` est a une extremite.
i64 dist2_max_box_to_box(const mhgp3v::RectBox& C, const mhgp3v::RectBox& A) {
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 dl0 = A.lo[i] - C.lo[i], dl1 = C.lo[i] - A.hi[i];
    const i64 dh0 = A.lo[i] - C.hi[i], dh1 = C.hi[i] - A.hi[i];
    i64 dl = dl0 > dl1 ? dl0 : dl1;
    i64 dh = dh0 > dh1 ? dh0 : dh1;
    if (dl < 0) dl = 0;
    if (dh < 0) dh = 0;
    const i64 d = dl > dh ? dl : dh;
    acc += d * d;
  }
  return acc;
}

// Distance carree MINIMALE d'un point de `C` a la boite `A`.
i64 dist2_min_box_to_box(const mhgp3v::RectBox& C, const mhgp3v::RectBox& A) {
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    i64 d = 0;
    if (A.lo[i] > C.hi[i]) d = A.lo[i] - C.hi[i];
    else if (C.lo[i] > A.hi[i]) d = C.lo[i] - A.hi[i];
    acc += d * d;
  }
  return acc;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 0, seed = 12345, sep_num = 8, sep_den = 1, coord = 0;
  bool juge = false, fixture_tetra = false;
  // ---- LA JONCTION : FERMER LE BLOC AVANT DE LE DEVELOPPER.
  //
  // La source rend des blocs `(A,B,C,D)` de supports candidats. `Corner8` teste
  // leur UNIQUE circumsphere sur des nœuds temoins : si la population creditee
  // atteint le seuil, tout le bloc sort du contrat et n'est JAMAIS developpe.
  // C'est le seul endroit ou l'on economise un produit entier.
  long long corner8 = 0;   // nombre de blocs WST4 soumis, 0 = desactive
  long long min_blocs = 0;
  // ---- L'ECHELLE DU BLOC, ET POURQUOI ELLE DECIDE TOUT.
  //
  // Exiger qu'un nœud soit ENTIEREMENT dans la lentille force a raffiner
  // jusqu'a sa frontiere : le cout devient proportionnel a une SURFACE, et le
  // nombre de blocs redevient quadratique — mesure `n^1,8` sur `uniform`.
  //
  // L'audit prescrit l'inverse : des cellules d'une echelle LIEE au rectangle.
  // On arrete donc la descente des que la cellule est assez petite devant le
  // rayon du domaine, et on garde tout nœud qui le RENCONTRE. La couverture
  // reste exact-once — les nœuds d'une antichaine sont disjoints — et le domaine
  // est seulement SUR-couvert, ce qui est sur : la lentille est une condition
  // necessaire, le predicat exact filtrera en aval.
  long long echelle = 0, echelle_den = 1;
  // ---- L'ORDRE QUATRE EST UN PRODUIT, PAS UNE NOUVELLE RECHERCHE.
  //
  // Pour un tetraedre `{a,b,x,y}` d'arete maximale `ab`, les DEUX autres
  // sommets satisfont la meme contrainte que le troisieme sommet d'un
  // triangle : ils sont dans la lentille. Les blocs `WST4` d'un rectangle sont
  // donc exactement les COUPLES NON ORDONNES de ses blocs `WST3`, diagonale
  // comprise — un couple `{C,C}` porte les paires internes a une meme cellule.
  //
  // L'exact-once est alors automatique : les blocs `WST3` sont disjoints, donc
  // `x` tombe dans au plus un `C` et `y` dans au plus un `D`, et le couple non
  // ordonne `{C,D}` est unique.
  long long ordre = 3;
  Wst3Mutant mu = Wst3Mutant::kNone;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = arg_ll(val("--points=").c_str(), 4, 200000, "points");
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 1, (1LL << 40), "seed");
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 4, 65536, "coord");
    else if (a.rfind("--sep-euclid=", 0) == 0) {
      const std::string v = val("--sep-euclid=");
      const size_t p = v.find('/');
      if (p == std::string::npos) refuse("sep-euclid");
      sep_num = arg_ll(v.substr(0, p).c_str(), 1, 1024, "sep-euclid");
      sep_den = arg_ll(v.substr(p + 1).c_str(), 1, 1024, "sep-euclid");
    }
    else if (a == "--juge") juge = true;
    else if (a == "--fixture-tetra") fixture_tetra = true;
    else if (a.rfind("--corner8=", 0) == 0) corner8 = arg_ll(val("--corner8=").c_str(), 1, (1LL << 30), "corner8");
    else if (a.rfind("--ordre=", 0) == 0) ordre = arg_ll(val("--ordre=").c_str(), 3, 4, "ordre");
    else if (a.rfind("--echelle=", 0) == 0) {
      // `num/den` : la descente s'arrete des que `diag^2 <= (num/den) rayon^2`.
      // Un rapport PLUS GRAND donne des cellules plus grosses, donc moins de
      // blocs et davantage de sur-couverture — et comme l'ordre quatre est un
      // produit, le gain y est QUADRATIQUE.
      const std::string v = val("--echelle=");
      const size_t sl = v.find('/');
      if (sl == std::string::npos) {
        echelle = arg_ll(v.c_str(), 1, (1LL << 20), "echelle");
        echelle_den = 1;
      } else {
        echelle = arg_ll(v.substr(0, sl).c_str(), 1, (1LL << 20), "echelle");
        echelle_den = arg_ll(v.substr(sl + 1).c_str(), 1, (1LL << 20), "echelle");
      }
    }
    else if (a.rfind("--min-blocs=", 0) == 0) min_blocs = arg_ll(val("--min-blocs=").c_str(), 1, (1LL << 40), "min-blocs");
    else if (a == "--inject=wst3-rayon-min") mu = Wst3Mutant::kRayonMin;
    else if (a == "--inject=wst3-une-seule-boule") mu = Wst3Mutant::kUneSeuleBoule;
    else if (a == "--inject=wst3-pas-de-descente") mu = Wst3Mutant::kPasDeDescente;
    else refuse("argument inconnu");
  }
  // ---- LA FIXTURE DU TETRAEDRE REGULIER, EXIGEE PAR LE CONTRE-AUDIT.
  //
  // `p0=(0,0,0)`, `p1=(0,1,1)`, `p2=(1,0,1)`, `p3=(1,1,0)` : six aretes de
  // longueur carree deux, owner `EdgeKey(0,1)`, centre `(1/2,1/2,1/2)` et
  // quatre poids `1/4`. Pour `x=p2` et `x=p3`, `H = (x-p0).(p1-x) = -1`.
  //
  // Le filtre d'acuite doit donc les GARDER. Ma premiere ecriture, qui lisait
  // l'acuite comme `H > 0`, retirait exactement ce couple de carriers et perdait
  // un q4 bien centre.
  if (fixture_tetra) {
    const i64 p[4][3] = {{0, 0, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
    long long aigus = 0;
    i64 hs[2] = {0, 0};
    for (int k = 2; k < 4; ++k) {
      i64 h = 0;
      for (int i = 0; i < 3; ++i) h += (p[k][i] - p[0][i]) * (p[1][i] - p[k][i]);
      hs[k - 2] = h;
      if (h < 0) ++aigus;
    }
    // Les six aretes ont la meme longueur carree : l'owner vient du tie-break.
    long long egales = 0;
    for (int u = 0; u < 4; ++u)
      for (int v = u + 1; v < 4; ++v) {
        i64 d2 = 0;
        for (int i = 0; i < 3; ++i) {
          const i64 e = p[u][i] - p[v][i];
          d2 += e * e;
        }
        if (d2 == 2) ++egales;
      }
    std::printf("wst3_tetra_regulier : H2=%lld H3=%lld carriers_aigus=%lld"
                " aretes_egales=%lld\n", (long long)hs[0], (long long)hs[1], aigus,
                egales);
    if (hs[0] != -1 || hs[1] != -1 || aigus != 2 || egales != 6) {
      std::fprintf(stderr, "DESACCORD: la fixture du tetraedre regulier ne tient pas\n");
      return 1;
    }
    return 0;
  }
  if (n == 0) refuse("--points est exige");
  if (mu != Wst3Mutant::kNone && !juge) refuse("un mutant exige --juge");

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "two_lines") fam = mhgp3v::CloudFamily::kTwoLines;
  else refuse("famille inconnue");
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(fam, (int)n);

  const std::vector<mhgp::P3> cloud =
      mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
  std::vector<std::array<long long, 3>> pts;
  for (const mhgp::P3& p : cloud) pts.push_back({p.x, p.y, p.z});
  if (pts.size() < 4) refuse("nuage trop petit apres deduplication");

  // Tri de Morton, identique au front WSPD : le meme arbre, les memes `RectId`.
  std::vector<int> pid(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) pid[i] = (int)i;
  std::sort(pid.begin(), pid.end(), [&](int u, int v) {
    const unsigned long long ku = mhgp3v::wf_morton48(pts[u][0], pts[u][1], pts[u][2]);
    const unsigned long long kv = mhgp3v::wf_morton48(pts[v][0], pts[v][1], pts[v][2]);
    if (ku != kv) return ku < kv;
    return u < v;
  });
  std::vector<std::array<long long, 3>> sp(pts.size());
  for (size_t i = 0; i < pid.size(); ++i) sp[i] = pts[pid[i]];
  std::vector<unsigned long long> keys(sp.size());
  for (size_t i = 0; i < sp.size(); ++i)
    keys[i] = mhgp3v::wf_morton48(sp[i][0], sp[i][1], sp[i][2]);
  for (size_t i = 1; i < keys.size(); ++i)
    if (keys[i] == keys[i - 1]) refuse("positions dupliquees : le profil les rejette");

  std::vector<mhgp3v::WfNode> nodes = mhgp3v::wf_build(keys);
  mhgp3v::wf_tight_boxes(&nodes, sp);
  const long long m = (long long)sp.size();

  auto box_of = [&](int nd, mhgp3v::RectBox* out) {
    if (nd < 0) {
      const auto& p = sp[(size_t)(-1 - nd)];
      for (int i = 0; i < 3; ++i) { out->lo[i] = p[i]; out->hi[i] = p[i]; }
    } else {
      for (int i = 0; i < 3; ++i) { out->lo[i] = nodes[nd].tlo[i]; out->hi[i] = nodes[nd].thi[i]; }
    }
  };
  auto count_of = [&](int nd) -> long long {
    return (nd < 0) ? 1LL : (long long)(nodes[nd].last - nodes[nd].first + 1);
  };

  // ---- LA VAGUE WSPD, exactement celle du front : `count -> scan -> fill`.
  struct Pair { int a, b; };
  std::vector<Pair> wave;
  for (size_t i = 0; i < nodes.size(); ++i)
    wave.push_back({nodes[i].left, nodes[i].right});
  std::vector<Pair> terms;
  auto sep_ok = [&](int u, int v) -> bool {
    mhgp3v::RectBox ba{}, bb{};
    box_of(u, &ba);
    box_of(v, &bb);
    // Separation euclidienne : `dist >= s * max(rayon)`, au carre et en entiers.
    i64 r2 = 0;
    for (int i = 0; i < 3; ++i) {
      const i64 da = ba.hi[i] - ba.lo[i], db = bb.hi[i] - bb.lo[i];
      const i64 d = std::max(da, db);
      r2 += d * d;
    }
    const i64 dmin = dist2_min_box_to_box(ba, bb);
    // `dist^2 * (2*den)^2 >= (num)^2 * r2` avec rayon = diagonale/2.
    return (__int128)dmin * (__int128)(4 * sep_den * sep_den) >=
           (__int128)(sep_num * sep_num) * (__int128)r2;
  };
  while (!wave.empty()) {
    std::vector<Pair> next;
    for (const Pair& p : wave) {
      if (sep_ok(p.a, p.b)) { terms.push_back(p); continue; }
      // Scinder le plus GROS des deux, comme la recursion canonique.
      const bool split_a = count_of(p.a) >= count_of(p.b);
      const int v = split_a ? p.a : p.b;
      if (v < 0) { terms.push_back(p); continue; }   // deux feuilles : terminal
      const int l = nodes[v].left, r = nodes[v].right;
      if (split_a) { next.push_back({l, p.b}); next.push_back({r, p.b}); }
      else { next.push_back({p.a, l}); next.push_back({p.a, r}); }
    }
    wave.swap(next);
  }

  // ---- L'EXTENSION D'ORDRE TROIS, COUNTER-ONLY.
  long long blocs = 0;
  double masse = 0.0;
  long long visites = 0;
  // Pour le juge : les blocs de chaque rectangle, dans l'ordre des terminaux.
  std::vector<std::vector<Bloc>> par_terminal;
  par_terminal.resize(terms.size());

  for (size_t t = 0; t < terms.size(); ++t) {
    mhgp3v::RectBox ba{}, bb{};
    box_of(terms[t].a, &ba);
    box_of(terms[t].b, &bb);
    const i64 dmax = mhgp3v::rect_maxsq(ba, bb);
    const i64 dmin = dist2_min_box_to_box(ba, bb);
    // MUTANT `wst3-rayon-min` : borner la lentille par la distance MINIMALE du
    // rectangle. Le domaine retrecit et des troisiemes sommets legitimes en
    // sortent — le juge doit trouver des triangles non couverts.
    const i64 rayon2 = (mu == Wst3Mutant::kRayonMin) ? dmin : dmax;

    int st[128];
    int sn = 0;
    st[sn++] = 0;   // racine de l'arbre des temoins
    if (nodes.empty()) continue;
    while (sn > 0) {
      const int nd = st[--sn];
      ++visites;
      mhgp3v::RectBox bc{};
      box_of(nd, &bc);
      // Hors du domaine ? Un seul des deux tests suffit a elaguer.
      if (dist2_min_box_to_box(bc, ba) > rayon2) continue;
      if (mu != Wst3Mutant::kUneSeuleBoule && dist2_min_box_to_box(bc, bb) > rayon2)
        continue;
      // Entierement dedans ? Alors le nœud EST un bloc.
      const bool dedans =
          dist2_max_box_to_box(bc, ba) <= rayon2 &&
          (mu == Wst3Mutant::kUneSeuleBoule || dist2_max_box_to_box(bc, bb) <= rayon2);
      // Assez petit devant le rayon du domaine ? Alors on s'arrete aussi, et le
      // bloc SUR-couvre : c'est le regime a echelle liee.
      bool assez_petit = false;
      if (echelle > 0) {
        i64 diag2 = 0;
        for (int i = 0; i < 3; ++i) {
          const i64 e = bc.hi[i] - bc.lo[i];
          diag2 += e * e;
        }
        // `diag^2 * echelle_den <= rayon^2 * echelle` — aucun flottant.
        assez_petit = (__int128)diag2 * (__int128)echelle <= (__int128)rayon2 * (__int128)echelle_den;
      }
      if (dedans || assez_petit || nd < 0 || mu == Wst3Mutant::kPasDeDescente) {
        ++blocs;
        masse += (double)count_of(terms[t].a) * (double)count_of(terms[t].b) *
                 (double)count_of(nd);
        {
          mhgp3v::midball::Box ma{}, mb{}, mc{};
          for (int i = 0; i < 3; ++i) {
            ma.lo[i] = ba.lo[i]; ma.hi[i] = ba.hi[i];
            mb.lo[i] = bb.lo[i]; mb.hi[i] = bb.hi[i];
            mc.lo[i] = bc.lo[i]; mc.hi[i] = bc.hi[i];
          }
          mhgp::i64 hmin = 0, hmax = 0;
          (void)mhgp3v::midball::midball_block(ma, mb, mc,
                                               mhgp3v::midball::MidballMutant::kNone,
                                               &hmin, &hmax);
          // Le bloc peut porter un carrier aigu des que `Hmin < 0`. `NONE_ACUTE`
          // est `Hmin >= 0` : l'egalite est un angle droit, donc non aigu.
          par_terminal[t].push_back(Bloc{nd, hmin < 0});
        }
        continue;
      }
      if (sn + 2 > 128) { std::fprintf(stderr, "PLANCHER: pile temoin saturee\n"); return 3; }
      st[sn++] = nodes[nd].left;
      st[sn++] = nodes[nd].right;
    }
  }

  std::printf("wst3 : n=%lld famille=%s sep=%lld/%lld | rectangles=%zu blocs=%lld"
              " blocs/n=%.2f masse=%.0f visites=%lld\n",
              m, family.c_str(), sep_num, sep_den, terms.size(), blocs,
              (double)blocs / (double)m, masse, visites);

  // ---- ORDRE QUATRE : LE PRODUIT NON ORDONNE DES BLOCS D'UN MEME RECTANGLE.
  long long blocs4 = 0, blocs4_aigu = 0, blocs3_aigu = 0;
  unsigned __int128 masse4 = 0;
  if (ordre >= 4) {
    for (size_t t = 0; t < terms.size(); ++t) {
      const long long k = (long long)par_terminal[t].size();
      long long na = 0;   // blocs qui ne peuvent PAS porter de carrier aigu
      for (const Bloc& b : par_terminal[t]) if (!b.aigu) ++na; else ++blocs3_aigu;
      // Les couples dont AUCUN membre ne peut etre aigu sont exclus : aucun
      // support q4 d'owner `ab` ne peut y vivre.
      blocs4_aigu += k * (k + 1) / 2 - na * (na + 1) / 2;
      blocs4 += k * (k + 1) / 2;
      // ---- L'IDENTITE BINOMIALE REMPLACE LA BOUCLE QUADRATIQUE.
      //
      // Les blocs temoins d'un rectangle sont DISJOINTS, donc
      //   somme_i C(|C_i|,2) + somme_{i<j} |C_i||C_j| = C(somme_i |C_i|, 2).
      // Le count passe de `O(k^2)` a `O(k)`, et il est exact en `u128` — un
      // `double` perd les unites des `10^15` atteints des `n=8000`.
      unsigned __int128 tot = 0;
      for (const Bloc& b : par_terminal[t]) tot += (unsigned __int128)count_of(b.c);
      const unsigned __int128 mab =
          (unsigned __int128)count_of(terms[t].a) * (unsigned __int128)count_of(terms[t].b);
      masse4 += mab * (tot * (tot - 1) / 2);
    }
    std::printf("wst4 : candidate_blocks=%lld blocs/n=%.2f candidate_mass=%.6g"
                " blocs/rect=%.2f | aigus : blocs3=%lld blocs4=%lld blocs4/n=%.2f"
                " gain=%.2fx\n",
                blocs4, (double)blocs4 / (double)m, (double)masse4,
                (double)blocs4 / (double)std::max<size_t>(1, terms.size()),
                blocs3_aigu, blocs4_aigu, (double)blocs4_aigu / (double)m,
                (double)blocs4 / (double)std::max(1LL, blocs4_aigu));
  }

  if (min_blocs > 0 && blocs < min_blocs) {
    std::fprintf(stderr, "PLANCHER: %lld blocs, %lld exiges\n", blocs, min_blocs);
    return 3;
  }

  // ---- LE JUGE DE COUVERTURE EXACT-ONCE.
  if (juge) {
    if (m > 220) refuse("le juge exhaustif est borne a 220 points");
    // Appartenance d'un point a un nœud : plage `[first,last]` de l'ordre trie.
    auto contient = [&](int nd, long long r) -> bool {
      if (nd < 0) return (-1 - nd) == (int)r;
      return r >= nodes[nd].first && r <= nodes[nd].last;
    };
    long long manquants = 0, doublons = 0, juges = 0;
    for (long long i = 0; i < m; ++i)
      for (long long j = i + 1; j < m; ++j)
        for (long long k = j + 1; k < m; ++k) {
          const long long idx[3] = {i, j, k};
          // Arete maximale du triangle, tie-break par indice trie — un ordre
          // total, donc un owner unique.
          i64 best = -1;
          int bu = 0, bv = 1;
          for (int u = 0; u < 3; ++u)
            for (int v = u + 1; v < 3; ++v) {
              i64 d2 = 0;
              for (int c = 0; c < 3; ++c) {
                const i64 e = sp[(size_t)idx[u]][c] - sp[(size_t)idx[v]][c];
                d2 += e * e;
              }
              if (d2 > best || (d2 == best && (idx[u] < idx[bu] ||
                                               (idx[u] == idx[bu] && idx[v] < idx[bv])))) {
                best = d2; bu = u; bv = v;
              }
            }
          int w = 3 - bu - bv;
          const long long ra = idx[bu], rb = idx[bv], rx = idx[w];
          ++juges;
          // Le rectangle owner : celui qui contient la paire `(ra,rb)`.
          long long vus = 0;
          for (size_t t = 0; t < terms.size(); ++t) {
            const bool ab = contient(terms[t].a, ra) && contient(terms[t].b, rb);
            const bool ba2 = contient(terms[t].a, rb) && contient(terms[t].b, ra);
            if (!ab && !ba2) continue;
            for (const Bloc& b : par_terminal[t])
              if (contient(b.c, rx)) ++vus;
          }
          if (vus == 0) ++manquants;
          else if (vus > 1) ++doublons;
        }
    std::printf("wst3_juge : triangles=%lld manquants=%lld doublons=%lld\n", juges,
                manquants, doublons);
    if (manquants != 0 || doublons != 0) {
      if (mu != Wst3Mutant::kNone) {
        std::printf("mutant_killed=1 %s : %lld triangles non couverts et %lld"
                    " doubles\n", mutant_name(mu), manquants, doublons);
        return 4;
      }
      std::fprintf(stderr, "DESACCORD: %lld triangles non couverts, %lld doubles\n",
                   manquants, doublons);
      return 1;
    }
    if (mu != Wst3Mutant::kNone) {
      std::fprintf(stderr, "MUTANT SURVIVANT : %s couvre encore exact-once\n",
                   mutant_name(mu));
      return 3;
    }
    if (juges == 0) {
      std::fprintf(stderr, "PLANCHER: aucun triangle juge\n");
      return 3;
    }
    // ---- LE JUGE D'ORDRE QUATRE.
    if (ordre >= 4) {
      long long manq4 = 0, doub4 = 0, juges4 = 0;
      for (long long i = 0; i < m; ++i)
      for (long long j = i + 1; j < m; ++j)
      for (long long k = j + 1; k < m; ++k)
      for (long long l = k + 1; l < m; ++l) {
        const long long idx[4] = {i, j, k, l};
        i64 best = -1;
        int bu = 0, bv = 1;
        for (int u = 0; u < 4; ++u)
          for (int v = u + 1; v < 4; ++v) {
            i64 d2 = 0;
            for (int c = 0; c < 3; ++c) {
              const i64 e = sp[(size_t)idx[u]][c] - sp[(size_t)idx[v]][c];
              d2 += e * e;
            }
            if (d2 > best || (d2 == best && (idx[u] < idx[bu] ||
                                             (idx[u] == idx[bu] && idx[v] < idx[bv])))) {
              best = d2; bu = u; bv = v;
            }
          }
        long long rx = -1, ry = -1;
        for (int u = 0; u < 4; ++u)
          if (u != bu && u != bv) { if (rx < 0) rx = idx[u]; else ry = idx[u]; }
        const long long ra = idx[bu], rb = idx[bv];
        ++juges4;
        long long vus = 0;
        for (size_t t = 0; t < terms.size(); ++t) {
          const bool ab = contient(terms[t].a, ra) && contient(terms[t].b, rb);
          const bool ba2 = contient(terms[t].a, rb) && contient(terms[t].b, ra);
          if (!ab && !ba2) continue;
          // Le couple non ordonne `{C,D}` qui porte `x` et `y`.
          for (size_t u = 0; u < par_terminal[t].size(); ++u)
            for (size_t v = u; v < par_terminal[t].size(); ++v) {
              const int cu = par_terminal[t][u].c, cv = par_terminal[t][v].c;
              const bool xy = contient(cu, rx) && contient(cv, ry);
              const bool yx = contient(cu, ry) && contient(cv, rx);
              if (u == v) { if (contient(cu, rx) && contient(cu, ry)) ++vus; }
              else if (xy || yx) ++vus;
            }
        }
        if (vus == 0) ++manq4;
        else if (vus > 1) ++doub4;
      }
      std::printf("wst4_juge : quadruplets=%lld manquants=%lld doublons=%lld\n",
                  juges4, manq4, doub4);
      if (manq4 != 0 || doub4 != 0) {
        std::fprintf(stderr, "DESACCORD: %lld quadruplets non couverts, %lld doubles\n",
                     manq4, doub4);
        return 1;
      }
    }
  }
  // ---- CORNER8 SUR LES BLOCS DE LA SOURCE.
  if (corner8 > 0) {
    long long soumis = 0, fermes = 0, orient_mixte = 0, coins = 0, orient_indecise = 0;
    long long cd_non_separes = 0;
    long long visites8 = 0;
    unsigned __int128 masse_fermee = 0;
    const int seuil = 8;   // `smax=11` : huit interieurs sortent q4 du contrat
    for (size_t t = 0; t < terms.size() && soumis < corner8; ++t) {
      mhgp3v::c8::Box A{}, B{};
      {
        mhgp3v::RectBox ra{}, rb{};
        box_of(terms[t].a, &ra);
        box_of(terms[t].b, &rb);
        for (int i = 0; i < 3; ++i) {
          A.lo[i] = ra.lo[i]; A.hi[i] = ra.hi[i];
          B.lo[i] = rb.lo[i]; B.hi[i] = rb.hi[i];
        }
      }
      const auto& bl = par_terminal[t];
      for (size_t u = 0; u < bl.size() && soumis < corner8; ++u)
        for (size_t v = u; v < bl.size() && soumis < corner8; ++v) {
          mhgp3v::c8::Box C{}, D{};
          {
            mhgp3v::RectBox rc{}, rd{};
            box_of(bl[u].c, &rc);
            box_of(bl[v].c, &rd);
            for (int i = 0; i < 3; ++i) {
              C.lo[i] = rc.lo[i]; C.hi[i] = rc.hi[i];
              D.lo[i] = rd.lo[i]; D.hi[i] = rd.hi[i];
            }
          }
          ++soumis;
          // ---- LA SEPARATION DOIT PORTER SUR TOUS LES COUPLES DE FACTEURS.
          //
          // Une WSPD ne separe que `(A,B)`. Mais l'orientation d'un tetraedre
          // depend des QUATRE sommets : si `C` et `D` sont proches — et sur la
          // diagonale ils sont identiques — le signe du determinant reste
          // indecidable quelle que soit la finesse des cellules. La vraie
          // generalisation de Callahan--Kosaraju a l'ordre quatre demande donc
          // que TOUS les couples de facteurs soient bien separes.
          if (!sep_ok(bl[u].c, bl[v].c)) { ++cd_non_separes; continue; }
          // ---- L'ORIENTATION D'ABORD, ET UNE SEULE FOIS.
          //
          // Elle ne depend que des quatre facteurs support. Si son signe n'est
          // pas fixe sur le bloc, `Corner8` rendra `MIXED` pour TOUT temoin :
          // parcourir l'arbre est alors du travail pur perdu.
          if (mhgp3v::c8::orientation_signe(A, B, C, D) == 0) { ++orient_indecise; continue; }
          // Plages d'IDs des quatre facteurs : un temoin doit en etre DISJOINT,
          // sinon il pourrait etre l'un des sommets du support.
          const int fa = (terms[t].a < 0) ? (-1 - terms[t].a) : nodes[terms[t].a].first;
          const int la2 = (terms[t].a < 0) ? (-1 - terms[t].a) : nodes[terms[t].a].last;
          const int fb = (terms[t].b < 0) ? (-1 - terms[t].b) : nodes[terms[t].b].first;
          const int lb2 = (terms[t].b < 0) ? (-1 - terms[t].b) : nodes[terms[t].b].last;
          const int fc = (bl[u].c < 0) ? (-1 - bl[u].c) : nodes[bl[u].c].first;
          const int lc = (bl[u].c < 0) ? (-1 - bl[u].c) : nodes[bl[u].c].last;
          const int fd = (bl[v].c < 0) ? (-1 - bl[v].c) : nodes[bl[v].c].first;
          const int ld = (bl[v].c < 0) ? (-1 - bl[v].c) : nodes[bl[v].c].last;
          long long credits = 0;
          bool mixte = false;
          int st8[128];
          int sn8 = 0;
          st8[sn8++] = 0;
          while (sn8 > 0 && credits < seuil) {
            const int nd = st8[--sn8];
            ++visites8;
            const int nf = (nd < 0) ? (-1 - nd) : nodes[nd].first;
            const int nl = (nd < 0) ? (-1 - nd) : nodes[nd].last;
            // Chevauchement avec un facteur : on descend, on ne credite pas.
            const bool chevauche =
                !(nl < fa || nf > la2) || !(nl < fb || nf > lb2) ||
                !(nl < fc || nf > lc) || !(nl < fd || nf > ld);
            mhgp3v::c8::Box Z{};
            {
              mhgp3v::RectBox rz{};
              box_of(nd, &rz);
              for (int i = 0; i < 3; ++i) { Z.lo[i] = rz.lo[i]; Z.hi[i] = rz.hi[i]; }
            }
            if (!chevauche) {
              const auto vd = mhgp3v::c8::corner8_block(A, B, C, D, Z,
                                                        mhgp3v::c8::C8Mutant::kNone, &coins);
              if (vd == mhgp3v::c8::BallVerdict::kAllInterior) {
                credits += count_of(nd);
                continue;   // nœud credite : ses descendants ne le sont plus
              }
            }
            if (nd < 0) continue;
            if (sn8 + 2 > 128) { mixte = true; break; }
            st8[sn8++] = nodes[nd].left;
            st8[sn8++] = nodes[nd].right;
          }
          if (credits >= seuil) {
            ++fermes;
            const unsigned __int128 mm =
                (unsigned __int128)count_of(terms[t].a) * (unsigned __int128)count_of(terms[t].b) *
                (unsigned __int128)count_of(bl[u].c) * (unsigned __int128)count_of(bl[v].c);
            masse_fermee += mm;
          } else if (mixte) ++orient_mixte;
        }
    }
    std::printf("corner8_source : soumis=%lld cd_non_separes=%lld (%.2f%%)"
                " orient_indecise=%lld (%.2f%%)"
                " fermes=%lld (%.2f%% des orientes) masse_fermee=%.6g"
                " visites=%lld coins=%lld piles_saturees=%lld\n",
                soumis, cd_non_separes,
                100.0 * (double)cd_non_separes / (double)std::max(1LL, soumis),
                orient_indecise,
                100.0 * (double)orient_indecise / (double)std::max(1LL, soumis - cd_non_separes),
                fermes,
                100.0 * (double)fermes / (double)std::max(1LL, soumis - cd_non_separes - orient_indecise),
                (double)masse_fermee, visites8, coins, orient_mixte);
  }
  std::printf("OK famille=%s\n", family.c_str());
  return 0;
}
