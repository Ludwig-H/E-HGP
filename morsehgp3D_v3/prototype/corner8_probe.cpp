// MorseHGP3D v3 — PORTES DE `Corner8BallDepth`.
//
// Le support complet n'a qu'un evenement : sa circumsphere. Ce probe juge le
// classifieur de bloc contre l'enumeration exhaustive des supports et des
// temoins entiers, puis rejoue la fixture u16 mise a l'echelle de la section 4
// de `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`.
//
// Codes : 1 = desaccord du juge, 2 = refus avant calcul, 3 = plancher ou mutant
// survivant, 4 = mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "corner8_ball.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp3v::c8::BallVerdict;
using mhgp3v::c8::Box;
using mhgp3v::c8::C8Mutant;

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

void print_i128(i128 v, char* buf) {
  if (v == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
  const bool neg = v < 0;
  unsigned long long mag[3] = {0, 0, 0};
  i128 x = neg ? -v : v;
  char tmp[64];
  int n = 0;
  while (x > 0) { tmp[n++] = (char)('0' + (int)(x % 10)); x /= 10; }
  int p = 0;
  if (neg) buf[p++] = '-';
  while (n > 0) buf[p++] = tmp[--n];
  buf[p] = '\0';
  (void)mag;
}

// ---------------------------------------------------------------------------
// L'ORACLE. Il n'emprunte au sujet aucune borne d'intervalle : il enumere les
// supports et les temoins ENTIERS et applique le predicat ponctuel.
// ---------------------------------------------------------------------------
bool tous_interieurs(const Box& A, const Box& B, const Box& X, const Box& Y,
                     const Box& Z, long long* supports) {
  for (i64 ax = A.lo[0]; ax <= A.hi[0]; ++ax)
  for (i64 ay = A.lo[1]; ay <= A.hi[1]; ++ay)
  for (i64 az = A.lo[2]; az <= A.hi[2]; ++az)
  for (i64 bx = B.lo[0]; bx <= B.hi[0]; ++bx)
  for (i64 by = B.lo[1]; by <= B.hi[1]; ++by)
  for (i64 bz = B.lo[2]; bz <= B.hi[2]; ++bz)
  for (i64 xx = X.lo[0]; xx <= X.hi[0]; ++xx)
  for (i64 xy = X.lo[1]; xy <= X.hi[1]; ++xy)
  for (i64 xz = X.lo[2]; xz <= X.hi[2]; ++xz)
  for (i64 yx = Y.lo[0]; yx <= Y.hi[0]; ++yx)
  for (i64 yy = Y.lo[1]; yy <= Y.hi[1]; ++yy)
  for (i64 yz = Y.lo[2]; yz <= Y.hi[2]; ++yz) {
    const i64 a[3] = {ax, ay, az}, b[3] = {bx, by, bz};
    const i64 x[3] = {xx, xy, xz}, y[3] = {yx, yy, yz};
    if (supports != nullptr) ++*supports;
    for (i64 zx = Z.lo[0]; zx <= Z.hi[0]; ++zx)
    for (i64 zy = Z.lo[1]; zy <= Z.hi[1]; ++zy)
    for (i64 zz = Z.lo[2]; zz <= Z.hi[2]; ++zz) {
      const i64 z[3] = {zx, zy, zz};
      if (!mhgp3v::c8::interieur_strict(a, b, x, y, z)) return false;
    }
  }
  return true;
}

int selftest(long long tours, C8Mutant mu, long long seed) {
  unsigned long long etat = (unsigned long long)seed * 0x2545F4914F6CDD1DULL + 3ULL;
  long long desaccords = 0, all = 0, mixed = 0;
  for (long long t = 0; t < tours; ++t) {
    Box A{}, B{}, X{}, Y{}, Z{};
    Box* bs[5] = {&A, &B, &X, &Y, &Z};
    // Boites minuscules : le juge enumere le produit des cinq, donc la taille
    // doit rester bornee. Un tetraedre engendre par des positions proches est
    // aussi le regime ou l'orientation change de signe, donc ou le classifieur
    // doit renoncer.
    for (int k = 0; k < 5; ++k)
      for (int i = 0; i < 3; ++i) {
        const i64 c0 = (i64)(splitmix(etat) % 14);
        const i64 wd = (i64)(splitmix(etat) % 2);
        bs[k]->lo[i] = c0;
        bs[k]->hi[i] = c0 + wd;
      }
    const BallVerdict v = mhgp3v::c8::corner8_block(A, B, X, Y, Z, mu, nullptr);
    if (v == BallVerdict::kAllInterior) {
      ++all;
      if (!tous_interieurs(A, B, X, Y, Z, nullptr)) ++desaccords;
    } else {
      ++mixed;
    }
  }
  std::printf("corner8_selftest accord=%s tours=%lld all=%lld mixed=%lld"
              " desaccords=%lld mutant=%s\n",
              desaccords == 0 ? "OUI" : "NON", tours, all, mixed, desaccords,
              mhgp3v::c8::c8_mutant_name(mu));
  if (desaccords != 0) {
    if (mu != C8Mutant::kNone) {
      std::printf("mutant_killed=1 %s : %lld blocs declares interieurs a tort\n",
                  mhgp3v::c8::c8_mutant_name(mu), desaccords);
      return 4;
    }
    std::fprintf(stderr, "DESACCORD: %lld blocs declares interieurs a tort\n", desaccords);
    return 1;
  }
  if (mu != C8Mutant::kNone) {
    std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a produit aucun desaccord\n",
                 mhgp3v::c8::c8_mutant_name(mu));
    return 3;
  }
  if (all == 0 || mixed == 0) {
    std::fprintf(stderr, "PLANCHER: all=%lld mixed=%lld\n", all, mixed);
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// LA CONTRE-FIXTURE DE CONVEXITE. Coins EXTERIEURS, centre INTERIEUR : elle
// tue `corners-outside-implies-none` et interdit d'inverser l'implication.
// ---------------------------------------------------------------------------
int fixture_convexite() {
  const i64 a[3] = {3, 2, 2}, b[3] = {1, 2, 2}, x[3] = {2, 3, 2}, y[3] = {2, 2, 3};
  Box Z{};
  for (int i = 0; i < 3; ++i) { Z.lo[i] = 1; Z.hi[i] = 3; }
  long long coins_ext = 0;
  for (int k = 0; k < 8; ++k) {
    i64 q[3];
    mhgp3v::c8::box_corner(Z, k, q);
    if (!mhgp3v::c8::interieur_strict(a, b, x, y, q)) ++coins_ext;
  }
  const i64 centre[3] = {2, 2, 2};
  const bool centre_int = mhgp3v::c8::interieur_strict(a, b, x, y, centre);
  std::printf("corner8_convexite : coins_exterieurs=%lld centre_interieur=%d\n",
              coins_ext, centre_int ? 1 : 0);
  if (coins_ext != 8 || !centre_int) {
    std::fprintf(stderr, "DESACCORD: la contre-fixture de convexite ne tient pas\n");
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// LA FIXTURE u16 MISE A L'ECHELLE. Cinq spans de huit points, quarante
// `PointId` distincts. Ses `4096` supports doivent tous etre bien centres et
// leurs huit temoins `Z` uniformement interieurs — donc UN SEUL bloc doit
// fermer, AVANT tout fill.
// ---------------------------------------------------------------------------
int fixture_bloc(C8Mutant mu) {
  Box A{}, B{}, C{}, D{}, Z{};
  const i64 orig[5][3] = {{20000, 20000, 20000},
                          {30000, 30000, 30000},
                          {19000, 31000, 31000},
                          {31000, 19000, 31000},
                          {20000, 20000, 30000}};
  Box* bs[5] = {&A, &B, &C, &D, &Z};
  for (int k = 0; k < 5; ++k)
    for (int i = 0; i < 3; ++i) { bs[k]->lo[i] = orig[k][i]; bs[k]->hi[i] = orig[k][i] + 1; }

  long long coins = 0;
  const BallVerdict v = mhgp3v::c8::corner8_block(A, B, C, D, Z, mu, &coins);

  // Contre-calcul ponctuel : les `4096` supports et les huit temoins.
  long long supports = 0, interieurs = 0, total = 0;
  i128 pire = 0;
  bool premier = true;
  for (int ia = 0; ia < 8; ++ia)
  for (int ib = 0; ib < 8; ++ib)
  for (int ic = 0; ic < 8; ++ic)
  for (int id = 0; id < 8; ++id) {
    i64 a[3], b[3], c[3], d[3];
    mhgp3v::c8::box_corner(A, ia, a);
    mhgp3v::c8::box_corner(B, ib, b);
    mhgp3v::c8::box_corner(C, ic, c);
    mhgp3v::c8::box_corner(D, id, d);
    ++supports;
    const i128 o = mhgp3v::c8::orient3d(a, b, c, d);
    for (int iz = 0; iz < 8; ++iz) {
      i64 z[3];
      mhgp3v::c8::box_corner(Z, iz, z);
      ++total;
      if (mhgp3v::c8::interieur_strict(a, b, c, d, z)) ++interieurs;
      const i128 j = mhgp3v::c8::insphere_j(a, b, c, d, z);
      const i128 sj = (o > 0) ? j : -j;
      if (premier || sj > pire) { pire = sj; premier = false; }
    }
  }
  char buf[64];
  print_i128(pire, buf);
  std::printf("corner8_bloc : verdict=%s supports=%lld temoins=%lld interieurs=%lld"
              " pire_sigma_J=%s coins=%lld\n",
              v == BallVerdict::kAllInterior ? "ALL_INTERIOR" : "MIXED", supports,
              total, interieurs, buf, coins);
  if (supports != 4096 || total != 32768 || interieurs != 32768) {
    std::fprintf(stderr, "DESACCORD: la fixture n'a pas 4096 supports uniformement"
                 " interieurs\n");
    return 1;
  }
  if (v != BallVerdict::kAllInterior) {
    if (mu != C8Mutant::kNone) {
      std::printf("mutant_killed=1 %s : le bloc de reference n'est plus ferme\n",
                  mhgp3v::c8::c8_mutant_name(mu));
      return 4;
    }
    std::fprintf(stderr, "DESACCORD: le bloc de reference doit fermer avant fill\n");
    return 1;
  }
  if (mu != C8Mutant::kNone) {
    std::fprintf(stderr, "MUTANT SURVIVANT : %s ferme encore le bloc\n",
                 mhgp3v::c8::c8_mutant_name(mu));
    return 3;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// LES FIXTURES DE BONNE CENTRALITE.
// ---------------------------------------------------------------------------
int fixtures_centre() {
  // 1. Tetraedre regulier : quatre poids `1/4`, donc bien centre.
  const i64 r0[3] = {0, 0, 0}, r1[3] = {0, 1, 1}, r2[3] = {1, 0, 1}, r3[3] = {1, 1, 0};
  const bool reg = mhgp3v::c8::bien_centre(r0, r1, r2, r3);

  // 2. Le tetraedre PLAT du contre-audit : poids `1/4` malgre l'aplatissement.
  const i64 f0[3] = {3000, 2000, 2001}, f1[3] = {2000, 3000, 1999};
  const i64 f2[3] = {1000, 2000, 2001}, f3[3] = {2000, 1000, 1999};
  const bool plat = mhgp3v::c8::bien_centre(f0, f1, f2, f3);

  // 3. La fixture de SHELL : `a,b,c,d` cospheriques autour de `(2,0,0)` mais
  //    `ab` est un DIAMETRE, donc les poids de `c` et `d` sont nuls et
  //    l'evenement reste q2. Elle doit etre refusee.
  const i64 s0[3] = {0, 0, 0}, s1[3] = {4, 0, 0}, s2b[3] = {2, 2, 0}, s3[3] = {2, 0, 2};
  const bool shell = mhgp3v::c8::bien_centre(s0, s1, s2b, s3);

  // 4. Un tetraedre tres obtus : le centre sort par une face.
  const i64 o0[3] = {0, 0, 0}, o1[3] = {100, 0, 0}, o2[3] = {50, 1, 0}, o3[3] = {50, 0, 1};
  const bool obtus = mhgp3v::c8::bien_centre(o0, o1, o2, o3);

  std::printf("corner8_centre : regulier=%d plat=%d shell=%d obtus=%d\n",
              reg ? 1 : 0, plat ? 1 : 0, shell ? 1 : 0, obtus ? 1 : 0);
  if (!reg || !plat || shell || obtus) {
    std::fprintf(stderr, "DESACCORD: la bonne centralite ne classe pas les quatre cas\n");
    return 1;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  long long tours = 0, seed = 1;
  bool veut_bloc = false, veut_convexite = false, veut_centre = false;
  C8Mutant mu = C8Mutant::kNone;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--selftest=", 0) == 0)
      tours = arg_ll(val("--selftest=").c_str(), 1, (1LL << 20), "selftest");
    else if (a.rfind("--seed=", 0) == 0)
      seed = arg_ll(val("--seed=").c_str(), 1, (1LL << 40), "seed");
    else if (a == "--bloc") veut_bloc = true;
    else if (a == "--convexite") veut_convexite = true;
    else if (a == "--centre") veut_centre = true;
    else if (a == "--inject=corners-outside-implies-none")
      mu = C8Mutant::kCornersOutsideImpliesNone;
    else if (a == "--inject=c8-accept-equality") mu = C8Mutant::kAcceptEquality;
    else if (a == "--inject=c8-drop-corner") mu = C8Mutant::kDropCorner;
    else if (a == "--inject=c8-norme-aux-coins") mu = C8Mutant::kNormeAuxCoins;
    else refuse("argument inconnu");
  }
  if (tours == 0 && !veut_bloc && !veut_convexite && !veut_centre)
    refuse("--selftest, --bloc ou --convexite est exige");
  if (veut_centre) {
    const int rc = fixtures_centre();
    if (rc != 0) return rc;
  }
  if (mu != C8Mutant::kNone && tours == 0 && !veut_bloc)
    refuse("un mutant exige --selftest ou --bloc");
  if (veut_convexite) {
    const int rc = fixture_convexite();
    if (rc != 0) return rc;
  }
  if (veut_bloc) {
    const int rc = fixture_bloc(mu);
    if (rc != 0) return rc;
  }
  if (tours > 0) {
    const int rc = selftest(tours, mu, seed);
    if (rc != 0) return rc;
  }
  return 0;
}
