// MorseHGP3D v3 — PORTES DE `MidballBlockDepth`.
//
// Specification : section 4, section 2 et reponse Q5 de
// audits/AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md.
//
// Trois choses sont jugees ici, et elles sont independantes :
//
//   `--selftest`  le verdict de bloc contre l'ENUMERATION EXHAUSTIVE de tous
//                 les triplets entiers des trois boites. Un `ALL` refute par un
//                 seul triplet non interieur est une faute ; un `NONE` refute
//                 par un seul triplet interieur aussi.
//   `--fixtures`  les deux nuages u16 de la section 2 : un q2 PROFOND qui
//                 laisse pourtant subsister un q3 de rang trois et un q4 de
//                 rang quatre. Ils tuent le raccourci `midball-prune-q3q4`.
//   `--inject`    un mutant doit mourir, avec le code de sortie 4.
//
// Codes de sortie : 1 = desaccord du juge, 2 = refus avant calcul,
// 3 = plancher ou mutant survivant, 4 = mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "midball_block.hpp"

namespace {

using mhgp::i64;
using mhgp3v::midball::Box;
using mhgp3v::midball::MidballMutant;
using mhgp3v::midball::MidballVerdict;

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

unsigned long long splitmix(unsigned long long& s) {
  s += 0x9E3779B97F4A7C15ULL;
  unsigned long long z = s;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// L'ORACLE. Il n'emprunte rien a la primitive : il enumere les points entiers
// des trois boites et evalue le produit scalaire directement.
struct Verite {
  bool tous_interieurs = true;
  bool aucun_interieur = true;
  i64 hmin = 0;
  i64 hmax = 0;
};

Verite juger(const Box& A, const Box& B, const Box& C) {
  Verite v{};
  bool premier = true;
  for (i64 ax = A.lo[0]; ax <= A.hi[0]; ++ax)
  for (i64 ay = A.lo[1]; ay <= A.hi[1]; ++ay)
  for (i64 az = A.lo[2]; az <= A.hi[2]; ++az)
  for (i64 bx = B.lo[0]; bx <= B.hi[0]; ++bx)
  for (i64 by = B.lo[1]; by <= B.hi[1]; ++by)
  for (i64 bz = B.lo[2]; bz <= B.hi[2]; ++bz)
  for (i64 cx = C.lo[0]; cx <= C.hi[0]; ++cx)
  for (i64 cy = C.lo[1]; cy <= C.hi[1]; ++cy)
  for (i64 cz = C.lo[2]; cz <= C.hi[2]; ++cz) {
    const i64 h = (cx - ax) * (bx - cx) + (cy - ay) * (by - cy) + (cz - az) * (bz - cz);
    if (h <= 0) v.tous_interieurs = false;
    if (h > 0) v.aucun_interieur = false;
    if (premier || h < v.hmin) v.hmin = h;
    if (premier || h > v.hmax) v.hmax = h;
    premier = false;
  }
  return v;
}

int selftest(long long tours, MidballMutant mu, long long seed) {
  unsigned long long etat = (unsigned long long)seed * 0x2545F4914F6CDD1DULL + 7ULL;
  long long desaccords = 0, all = 0, none = 0, mixed = 0;
  long long bornes_fausses = 0;
  for (long long t = 0; t < tours; ++t) {
    Box A{}, B{}, C{};
    // TROIS REGIMES, ET LE DEUXIEME EST INDISPENSABLE.
    //
    // Des boites uniformement petites ne separent pas le maximum exact du
    // maximum aux sommets : il faut que la boite TEMOIN enjambe le segment
    // `[a,b]`. Alors `f` est negative aux deux bouts de `z` et positive au
    // stationnaire — exactement le cas que la concavite impose et que le
    // mutant `midball-max-aux-coins` manque. Avec `a=4`, `b=8` et
    // `z` dans `[0,10]`, les bouts valent `-32` et `-12`, le stationnaire `+4`.
    const unsigned regime = (unsigned)(splitmix(etat) % 3);
    for (int i = 0; i < 3; ++i) {
      const i64 a0 = (i64)(splitmix(etat) % 40);
      const i64 b0 = (i64)(splitmix(etat) % 40);
      const i64 c0 = (i64)(splitmix(etat) % 40);
      i64 wa = 0, wb = 0, wc = 0;
      if (regime == 0) {          // trois boites petites et proches
        wa = (i64)(splitmix(etat) % 3);
        wb = (i64)(splitmix(etat) % 3);
        wc = (i64)(splitmix(etat) % 3);
      } else if (regime == 1) {   // endpoints ponctuels, temoin LARGE
        wc = (i64)(splitmix(etat) % 9);
      } else {                    // endpoints larges, temoin ponctuel
        wa = (i64)(splitmix(etat) % 5);
        wb = (i64)(splitmix(etat) % 5);
      }
      A.lo[i] = a0; A.hi[i] = a0 + wa;
      B.lo[i] = b0; B.hi[i] = b0 + wb;
      C.lo[i] = c0; C.hi[i] = c0 + wc;
    }
    i64 hmin = 0, hmax = 0;
    const MidballVerdict v = mhgp3v::midball::midball_block(A, B, C, mu, &hmin, &hmax);
    const Verite w = juger(A, B, C);
    if (v == MidballVerdict::kAll) {
      ++all;
      if (!w.tous_interieurs) ++desaccords;
    } else if (v == MidballVerdict::kNone) {
      ++none;
      if (!w.aucun_interieur) ++desaccords;
    } else {
      ++mixed;
    }
    // Les bornes sont EXACTES par separabilite d'axes, pas des majorants : le
    // juge les compare aux extrema reellement atteints sur les entiers.
    if (mu == MidballMutant::kNone && (hmin != w.hmin || hmax != w.hmax))
      ++bornes_fausses;
  }
  std::printf("midball_selftest accord=%s tours=%lld all=%lld none=%lld mixed=%lld"
              " desaccords=%lld bornes_fausses=%lld mutant=%s\n",
              (desaccords == 0 && bornes_fausses == 0) ? "OUI" : "NON", tours, all, none,
              mixed, desaccords, bornes_fausses,
              mhgp3v::midball::midball_mutant_name(mu));
  if (desaccords != 0 || bornes_fausses != 0) {
    if (mu != MidballMutant::kNone) {
      std::printf("mutant_killed=1 %s : %lld verdicts et %lld bornes refutes\n",
                  mhgp3v::midball::midball_mutant_name(mu), desaccords, bornes_fausses);
      return 4;
    }
    std::fprintf(stderr, "DESACCORD: %lld verdicts et %lld bornes refutes\n",
                 desaccords, bornes_fausses);
    return 1;
  }
  if (mu != MidballMutant::kNone) {
    std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a produit aucun desaccord\n",
                 mhgp3v::midball::midball_mutant_name(mu));
    return 3;
  }
  // PLANCHERS CONTRE LE VERT PAR VACUITE : les trois verdicts doivent exister.
  if (all == 0 || none == 0 || mixed == 0) {
    std::fprintf(stderr, "PLANCHER: all=%lld none=%lld mixed=%lld\n", all, none, mixed);
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// LES DEUX NUAGES DE LA SECTION 2 : UN q2 PROFOND NE FERME NI q3 NI q4.
// ---------------------------------------------------------------------------
struct P { i64 x, y, z; };

i64 dist2(const P& p, const P& q) {
  const i64 dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
  return dx * dx + dy * dy + dz * dz;
}

int fixtures(bool prune_q3q4) {
  const P a{52, 114, 100}, b{148, 114, 100};
  const P c3{100, 50, 100};
  const P c4{100, 86, 148}, d4{100, 86, 52};
  std::vector<P> z;
  for (int j = 0; j < 10; ++j) z.push_back(P{100, (i64)(151 + j), 100});

  // 1. La boule diametrale de `ab` contient les dix temoins : q2 est PROFOND.
  const P mid{(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
  const i64 r2 = dist2(a, b) / 4;
  long long dans_midball = 0;
  for (const P& p : z)
    if (dist2(p, mid) < r2) ++dans_midball;

  // Le meme compte par le predicat de Thales, qui est l'autorite du bloc.
  long long par_thales = 0;
  for (const P& p : z) {
    const i64 aa[3] = {a.x, a.y, a.z}, bb[3] = {b.x, b.y, b.z};
    const i64 pp[3] = {p.x, p.y, p.z};
    if (mhgp3v::midball::midball_h(aa, bb, pp) > 0) ++par_thales;
  }

  // 2. Le q3 `a,b,c3` a pour centre (100,100,100) et rayon carre 2500 : ses
  //    dix temoins sont STRICTEMENT EXTERIEURS.
  const P o{100, 100, 100};
  const i64 R2 = 2500;
  long long int_q3 = 0;
  for (const P& p : z)
    if (dist2(p, o) < R2) ++int_q3;
  const bool q3_cospherique =
      dist2(a, o) == R2 && dist2(b, o) == R2 && dist2(c3, o) == R2;

  // 3. Le q4 `a,b,c4,d4` a le meme centre et le meme rayon.
  long long int_q4 = int_q3;
  const bool q4_cospherique =
      dist2(a, o) == R2 && dist2(b, o) == R2 && dist2(c4, o) == R2 && dist2(d4, o) == R2;

  // Le MUTANT : propager la profondeur q2 vers les lanes superieures. Il rend
  // les deux supports elimines, alors que leurs spheres sont vides.
  const long long vu_q3 = prune_q3q4 ? dans_midball : int_q3;
  const long long vu_q4 = prune_q3q4 ? dans_midball : int_q4;

  std::printf("midball_fixtures : q2_interieurs=%lld thales=%lld q3_interieurs=%lld"
              " q4_interieurs=%lld cospherique_q3=%d cospherique_q4=%d vu_q3=%lld"
              " vu_q4=%lld\n",
              dans_midball, par_thales, int_q3, int_q4, q3_cospherique ? 1 : 0,
              q4_cospherique ? 1 : 0, vu_q3, vu_q4);

  if (dans_midball != 10 || par_thales != 10) {
    std::fprintf(stderr, "DESACCORD: la boule diametrale devait contenir dix temoins\n");
    return 1;
  }
  if (!q3_cospherique || !q4_cospherique) {
    std::fprintf(stderr, "DESACCORD: les supports ne sont pas cospheriques\n");
    return 1;
  }
  // Seuils : q3 ferme a neuf interieurs, q4 a huit.
  const bool q3_elimine = vu_q3 >= 9;
  const bool q4_elimine = vu_q4 >= 8;
  if (q3_elimine || q4_elimine) {
    if (prune_q3q4) {
      std::printf("mutant_killed=1 midball-prune-q3q4 : q3 et q4 elimines alors que"
                  " leurs spheres sont vides\n");
      return 4;
    }
    std::fprintf(stderr, "DESACCORD: un support de sphere vide a ete elimine\n");
    return 1;
  }
  if (prune_q3q4) {
    std::fprintf(stderr, "MUTANT SURVIVANT : midball-prune-q3q4 n'a rien elimine\n");
    return 3;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  long long tours = 0, seed = 1;
  bool veut_fixtures = false;
  MidballMutant mu = MidballMutant::kNone;
  bool prune_q3q4 = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--selftest=", 0) == 0)
      tours = arg_ll(val("--selftest=").c_str(), 1, (1LL << 22), "selftest");
    else if (a.rfind("--seed=", 0) == 0)
      seed = arg_ll(val("--seed=").c_str(), 1, (1LL << 40), "seed");
    else if (a == "--fixtures") veut_fixtures = true;
    else if (a == "--inject=midball-max-aux-coins") mu = MidballMutant::kMaxAuxCoins;
    else if (a == "--inject=midball-accept-equality") mu = MidballMutant::kAcceptEquality;
    else if (a == "--inject=midball-drop-axis") mu = MidballMutant::kDropAxis;
    else if (a == "--inject=midball-min-au-milieu") mu = MidballMutant::kMinAuMilieu;
    else if (a == "--inject=midball-prune-q3q4") prune_q3q4 = true;
    else refuse("argument inconnu");
  }
  if (tours == 0 && !veut_fixtures) refuse("--selftest ou --fixtures est exige");
  if (mu != MidballMutant::kNone && tours == 0)
    refuse("un mutant de primitive exige --selftest");
  if (prune_q3q4 && !veut_fixtures) refuse("midball-prune-q3q4 exige --fixtures");
  if (veut_fixtures) {
    const int rc = fixtures(prune_q3q4);
    if (rc != 0) return rc;
  }
  if (tours > 0) {
    const int rc = selftest(tours, mu, seed);
    if (rc != 0) return rc;
  }
  return 0;
}
