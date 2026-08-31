// MorseHGP3D v6 — selftests du cœur : arithmetique, sha256, familles, index,
// ledger WSPD, descente fusionnee, oracle du sweep (objet contre enumeration
// exhaustive).
//
// L'ORACLE DU SWEEP est une autorite d'ENUMERATION independante (tous les
// supports possibles, sans WSPD ni cover), pas encore d'arithmetique
// independante (il emploie les formes produit ; le juge OBig n <= 14 a
// arithmetique volontairement autre reste un livrable J2+, cf. PLAN_DE_TESTS).
// Il etablit : l'ensemble des BallKey survivantes au prefiltre exact et leurs
// arites minimales sont EXACTEMENT ceux de l'enumeration exhaustive des
// supports (paires / triangles strictement aigus / tetraedres strictement
// bien centres) au seuil h_q de leur arite minimale.
//
// Codes : 0 conforme ; 1 desaccord ; 2 refus (mode ou mutant inconnu) ;
// 3 mutant injecte non tue ; 4 mutant injecte tue.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/dint.hpp"
#include "../src/core/sha256.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp6;

namespace {

int g_failures = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    ++g_failures;
    std::fprintf(stderr, "ECHEC : %s\n", what);
  }
}

// ---- --arith : bornes et primitives entieres aux extremes u16.
int run_arith() {
  check(floor_sqrt(0) == 0 && floor_sqrt(1) == 1 && floor_sqrt(3) == 1 && floor_sqrt(4) == 2, "floor_sqrt petites");
  check(ceil_sqrt(0) == 0 && ceil_sqrt(1) == 1 && ceil_sqrt(2) == 2 && ceil_sqrt(4) == 2, "ceil_sqrt petites");
  const i64 big = (i64)3 * 65535 * 65535;  // carre de distance maximal du profil
  const i64 r = floor_sqrt(big);
  check(r * r <= big && (r + 1) * (r + 1) > big, "floor_sqrt au maximum du profil");
  // U192/U320 : produits croises traversant les mots hauts.
  const u128 a = ((u128)1 << 100) + 12345, b = ((u128)1 << 90) + 6789;
  const U192 ab = mul_128x128_192(a, b), ba = mul_128x128_192(b, a);
  check(cmp_u192(ab, ba) == 0, "mul_128x128_192 commutatif");
  const U192 ab1 = mul_128x128_192(a + 1, b);
  check(cmp_u192(ab1, ab) > 0, "cmp_u192 strict");
  const U320 p1 = mul_192x128_320(ab, (u128)3), p2 = mul_192x128_320(ab, (u128)2);
  check(cmp_u320(p1, p2) > 0, "cmp_u320 strict");
  // DI128 contre __int128 sur un echantillon deterministe.
  std::mt19937_64 rng(7);
  for (int i = 0; i < 20000; ++i) {
    const i64 x = (i64)(rng() >> 12) - (i64)(1ll << 51);
    const i64 y = (i64)(rng() >> 12) - (i64)(1ll << 51);
    const i128 want = (i128)x * y;
    const DI128 got = di_mul_i64_i64(x, y);
    check(di_to_i128(got) == want, "di_mul_i64_i64 == __int128");
    if (g_failures) break;
  }
  return g_failures ? 1 : 0;
}

// ---- --sha256 : vecteurs FIPS 180-4 et streaming.
int run_sha256() {
  const auto hex = [](const char* msg) {
    Sha256 h;
    h.update(msg, std::strlen(msg));
    return h.hex();
  };
  check(hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "FIPS abc");
  check(hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "FIPS vide");
  check(hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
        "FIPS deux blocs");
  Sha256 s;
  s.update("ab", 2);
  s.update("c", 1);
  check(s.hex() == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "streaming");
  return g_failures ? 1 : 0;
}

// ---- --families : determinisme, profil, cardinalite ; stationnaires comprises.
int run_families() {
  const CloudFamily fams[] = {CloudFamily::kUniform,          CloudFamily::kTerrain,
                              CloudFamily::kEightClusters,    CloudFamily::kScanlineSinglePass,
                              CloudFamily::kScanlineOverlapMultiecho, CloudFamily::kTwoLines,
                              CloudFamily::kCollinearSeven,   CloudFamily::kTerrainStationnaire,
                              CloudFamily::kScanlineStationnaire};
  for (const CloudFamily f : fams) {
    const int n = 1500;
    const int coord = cloud_family_default_coord(f, n);
    const std::vector<InputPoint> p1 = make_family_input(f, n, coord, 3);
    const std::vector<InputPoint> p2 = make_family_input(f, n, coord, 3);
    check(p1.size() == p2.size(), "determinisme : cardinal");
    for (size_t i = 0; i < p1.size() && i < p2.size(); ++i) {
      if (p1[i].id != p2[i].id || p1[i].position.x != p2[i].position.x || p1[i].position.y != p2[i].position.y ||
          p1[i].position.z != p2[i].position.z) {
        check(false, "determinisme : point");
        break;
      }
    }
    check(!p1.empty() && p1.size() <= (size_t)n, "cardinal borne par n");
    std::vector<u64> keys;
    keys.reserve(p1.size());
    for (const InputPoint& p : p1) {
      check(p3_in_profile(p.position), "profil u16");
      keys.push_back(((u64)p.position.x << 34) | ((u64)p.position.y << 17) | (u64)p.position.z);
    }
    std::sort(keys.begin(), keys.end());
    check(std::adjacent_find(keys.begin(), keys.end()) == keys.end(), "positions uniques");
  }
  // Familles stationnaires : plein cardinal a la taille de reference.
  for (const CloudFamily f : {CloudFamily::kTerrainStationnaire, CloudFamily::kScanlineStationnaire}) {
    const std::vector<InputPoint> p = make_family_input(f, 8000, cloud_family_default_coord(f, 8000), 3);
    check(p.size() == 8000, "stationnaire : cardinal plein a n=8000");
  }
  return g_failures ? 1 : 0;
}

// ---- --tree : invariants de l'index radix.
int run_tree() {
  for (const long long seed : {3ll, 4ll}) {
    const std::vector<InputPoint> in =
        make_family_input(CloudFamily::kUniform, 3000, cloud_family_default_coord(CloudFamily::kUniform, 3000), seed);
    const CloudIndex ix = build_cloud_index(in);
    check(ix.valid, "index valide");
    const size_t m = ix.upos.size();
    check(ix.nodes.size() == m - 1, "m-1 nœuds internes");
    for (size_t i = 0; i + 1 < m; ++i) check(ix.keys[i] < ix.keys[i + 1], "cles Morton strictement croissantes");
    for (size_t v = 0; v < ix.nodes.size(); ++v) {
      const NodeRange r = ix.range_of((NodeRef)v);
      const AxisBox bb = ix.box_of((NodeRef)v);
      check(r.first <= r.last, "plage non vide");
      for (i32 u = r.first; u <= r.last; ++u) {
        const P3& p = ix.upos[(size_t)u];
        const i64 c[3] = {p.x, p.y, p.z};
        for (int k = 0; k < 3; ++k) check(bb.lo[k] <= c[k] && c[k] <= bb.hi[k], "boite serree contient ses points");
      }
    }
    // Equivariance : une permutation physique de l'entree donne le meme index.
    std::vector<InputPoint> shuffled = in;
    std::mt19937_64 rng(99);
    std::shuffle(shuffled.begin(), shuffled.end(), rng);
    const CloudIndex ix2 = build_cloud_index(shuffled);
    check(ix2.valid && ix2.upos.size() == m, "equivariance : memes positions uniques");
    for (size_t i = 0; i < m; ++i)
      check(ix.upos[i].x == ix2.upos[i].x && ix.upos[i].y == ix2.upos[i].y && ix.upos[i].z == ix2.upos[i].z,
            "equivariance : upos");
  }
  return g_failures ? 1 : 0;
}

// ---- --wspd-ledger : partition exacte des paires par la WSPD brute.
int run_wspd_ledger() {
  for (const CloudFamily f : {CloudFamily::kUniform, CloudFamily::kTerrain, CloudFamily::kCollinearSeven}) {
    const int n = 800;
    const std::vector<InputPoint> in = make_family_input(f, n, cloud_family_default_coord(f, n), 3);
    const CloudIndex ix = build_cloud_index(in);
    WspdStats st = wspd_wavefront(ix, 8, 1, [](const WspdRect&) {});
    check(st.pair_mass == expected_pair_mass(ix), "pair_mass == C(n,2) - somme C(mult,2)");
    check(st.rectangles > 0, "front non vide");
  }
  return g_failures ? 1 : 0;
}

// ---- --fused-descent : la descente a masque plein egale les trois descentes
// a masque singleton (meme code, masque reduit), et le grand-livre ferme.
int run_fused_descent(bool injected) {
  u64 mismatches = 0;
  for (const CloudFamily f :
       {CloudFamily::kUniform, CloudFamily::kTerrain, CloudFamily::kEightClusters, CloudFamily::kCollinearSeven}) {
    const int n = 700;
    const std::vector<InputPoint> in = make_family_input(f, n, cloud_family_default_coord(f, n), 3);
    const CloudIndex ix = build_cloud_index(in);
    const u64 h_of[3] = {lane_h(Lane::kQ2, std::min<u64>(11, in.size())), lane_h(Lane::kQ3, std::min<u64>(11, in.size())),
                         lane_h(Lane::kQ4, std::min<u64>(11, in.size()))};
    std::vector<MultiAliveRect> full;
    GenerateStats stf;
    alive_rectangles_fused(ix, 8, h_of, 0b111, 2, &full, &stf);
    const u128 expected = expected_pair_mass(ix);
    // Invariant structurel : une lane emise a un cœur strictement sous h_q
    // (le mutant fused-mask-stuck emet des lanes mortes et le viole — c'est
    // son detecteur, car il mute les deux bras de la porte d'egalite a
    // l'identique et laisse le grand-livre ferme en versant tout aux emis).
    for (const MultiAliveRect& r : full)
      for (int q = 0; q < 3; ++q)
        if ((r.mask & (1u << q)) && r.core[q] >= h_of[q]) {
          ++mismatches;
          std::fprintf(stderr, "lane %d : rectangle emis avec cœur >= h (%s)\n", q + 2, cloud_family_name(f));
          q = 3;
        }
    // Plancher CAUSAL de l'auditeur (31 aout) : en nominal, uniform n=700 tue
    // une masse non nulle dans chaque lane ; fused-mask-stuck la fait tomber a
    // zero partout (il verse tout aux emis, le grand-livre ferme quand meme).
    if (f == CloudFamily::kUniform)
      for (int q = 0; q < 3; ++q)
        if (stf.ledger_killed_mass[q] == 0) {
          ++mismatches;
          std::fprintf(stderr, "plancher : masse tuee nulle en lane %d (uniform)\n", q + 2);
        }
    for (int q = 0; q < 3; ++q) {
      if (stf.ledger_emitted_mass[q] + stf.ledger_killed_mass[q] != expected) {
        ++mismatches;
        std::fprintf(stderr, "grand-livre lane %d non ferme (%s)\n", q + 2, cloud_family_name(f));
      }
      std::vector<MultiAliveRect> single;
      GenerateStats sts;
      alive_rectangles_fused(ix, 8, h_of, (u8)(1u << q), 2, &single, &sts);
      std::vector<std::pair<WspdRect, u64>> a, b;
      for (const MultiAliveRect& r : full)
        if (r.mask & (1u << q)) a.push_back({r.r, r.core[q]});
      for (const MultiAliveRect& r : single)
        if (r.mask & (1u << q)) b.push_back({r.r, r.core[q]});
      if (a.size() != b.size()) {
        ++mismatches;
        std::fprintf(stderr, "lane %d : %zu vs %zu rectangles (%s)\n", q + 2, a.size(), b.size(),
                     cloud_family_name(f));
        continue;
      }
      for (size_t i = 0; i < a.size(); ++i)
        if (a[i].first.a != b[i].first.a || a[i].first.b != b[i].first.b || a[i].second != b[i].second) {
          ++mismatches;
          std::fprintf(stderr, "lane %d : rectangle %zu divergent (%s)\n", q + 2, i, cloud_family_name(f));
          break;
        }
    }
  }
  if (injected) return mismatches ? 4 : 3;
  return mismatches ? 1 : 0;
}

// ---- --sweep-oracle : l'objet post-prefiltre contre l'enumeration exhaustive.
int run_sweep_oracle(bool injected) {
  u64 mismatches = 0;
  struct Case {
    CloudFamily f;
    int n;
    long long seed;
  };
  const Case cases[] = {{CloudFamily::kUniform, 44, 3},        {CloudFamily::kUniform, 44, 4},
                        {CloudFamily::kEightClusters, 48, 3},  {CloudFamily::kTerrain, 44, 3},
                        {CloudFamily::kScanlineSinglePass, 44, 3}, {CloudFamily::kTwoLines, 30, 3},
                        {CloudFamily::kCollinearSeven, 600, 3}};
  for (const Case& tc : cases) {
    const int coord = cloud_family_default_coord(tc.f, tc.n);
    const std::vector<InputPoint> in = make_family_input(tc.f, tc.n, coord, tc.seed);
    const CloudIndex ix = build_cloud_index(in);
    if (!ix.valid || ix.has_duplicate_positions()) {
      check(false, "entree d'oracle invalide");
      continue;
    }
    const u64 smax_eff = std::min<u64>(11, in.size());
    // Cote v6 : generation -> RLE -> prefiltre exact.
    GenerateOptions go;
    go.smax = smax_eff;
    go.threads = 2;
    std::vector<BallCandidate> cands;
    GenerateStats gs;
    generate_candidates(ix, go, &cands, &gs);
    sort_candidates(&cands, 1);
    deduplicate_candidates(&cands);
    std::vector<Survivor> surv;
    ExpandStats es;
    prefilter_balls(ix, cands, smax_eff, 1, &surv, &es);
    std::map<BallKey, u8> got;
    for (const Survivor& s : surv) got[cands[s.idx].key] = cands[s.idx].arity;
    // Cote oracle : enumeration exhaustive des supports.
    std::map<BallKey, u8> want_supports;
    const auto note = [&](const BallKey& k, u8 arity) {
      auto [it, fresh] = want_supports.try_emplace(k, arity);
      if (!fresh && arity < it->second) it->second = arity;
    };
    const size_t m = ix.upos.size();
    for (size_t i = 0; i < m; ++i)
      for (size_t j = i + 1; j < m; ++j) {
        const P3 &pa = ix.upos[i], &pb = ix.upos[j];
        if (p3_norm2(p3_sub(pb, pa)) == 0) continue;
        note(q2_ball_key(pa, pb), 2);
      }
    for (size_t i = 0; i < m; ++i)
      for (size_t j = i + 1; j < m; ++j)
        for (size_t k = j + 1; k < m; ++k) {
          // Etiquette (a, b) = arete maximale ; triangle strictement aigu ssi
          // l'angle au sommet oppose a l'arete maximale est strictement aigu.
          const P3 *pa = &ix.upos[i], *pb = &ix.upos[j], *px = &ix.upos[k];
          i64 lab = p3_norm2(p3_sub(*pb, *pa)), lax = p3_norm2(p3_sub(*px, *pa)), lbx = p3_norm2(p3_sub(*px, *pb));
          if (lax >= lab && lax >= lbx) std::swap(pb, px);
          else if (lbx >= lab && lbx >= lax) std::swap(pa, px);
          lab = p3_norm2(p3_sub(*pb, *pa));
          const P3 v{2 * px->x - pa->x - pb->x, 2 * px->y - pa->y - pb->y, 2 * px->z - pa->z - pb->z};
          if (!(p3_norm2(v) > lab)) continue;  // rectangle ou obtus : support d'arite 2
          const Q3Form f3 = q3_form(*pa, *pb, *px);
          if (f3.g <= 0) continue;  // colineaires
          note(q3_ball_key(f3), 3);
        }
    for (size_t i = 0; i < m; ++i)
      for (size_t j = i + 1; j < m; ++j)
        for (size_t k = j + 1; k < m; ++k)
          for (size_t l = k + 1; l < m; ++l) {
            const Q4Form f4 = q4_form(ix.upos[i], ix.upos[j], ix.upos[k], ix.upos[l]);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, ix.upos[i], ix.upos[j], ix.upos[k], ix.upos[l])) continue;
            note(ball_key_reduce(q4_ball_form(f4)), 4);
          }
    std::map<BallKey, u8> want;
    for (const auto& [k, arity] : want_supports) {
      u64 depth = 0;
      for (size_t z = 0; z < m; ++z)
        if (k.power(ix.upos[z]) < 0) ++depth;
      const u64 h = smax_eff >= arity ? smax_eff - arity + 1 : 0;
      if (depth < h) want[k] = arity;
    }
    if (got != want) {
      ++mismatches;
      std::fprintf(stderr, "oracle %s n=%d seed=%lld : v6=%zu boules, oracle=%zu\n", cloud_family_name(tc.f), tc.n,
                   tc.seed, got.size(), want.size());
      for (const auto& [k, a] : want)
        if (!got.count(k)) {
          std::fprintf(stderr, "  MANQUANTE arite=%d (perte de completude)\n", (int)a);
          break;
        }
      for (const auto& [k, a] : got)
        if (!want.count(k) || want.at(k) != a) {
          std::fprintf(stderr, "  EXCEDENTAIRE/arite fausse arite=%d\n", (int)a);
          break;
        }
    }
  }
  if (injected) return mismatches ? 4 : 3;
  return mismatches ? 1 : 0;
}

}  // namespace

int main(int argc, char** argv) {
  const char* mode = nullptr;
  const char* inject = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = argv[i] + 9;
    else if (arg.rfind("--", 0) == 0 && !mode) mode = argv[i] + 2;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      return 2;
    }
  }
  if (!mode) {
    std::fprintf(stderr, "mode requis : --arith --sha256 --families --tree --wspd-ledger --fused-descent --sweep-oracle\n");
    return 2;
  }
  if (inject && !mutants_enable(inject)) {
    std::fprintf(stderr, "mutant inconnu : %s\n", inject);
    return 2;
  }
  const bool injected = inject != nullptr;
  const std::string m = mode;
  int rc = 2;
  if (m == "arith") rc = run_arith();
  else if (m == "sha256") rc = run_sha256();
  else if (m == "families") rc = run_families();
  else if (m == "tree") rc = run_tree();
  else if (m == "wspd-ledger") rc = run_wspd_ledger();
  else if (m == "fused-descent") rc = run_fused_descent(injected);
  else if (m == "sweep-oracle") rc = run_sweep_oracle(injected);
  else {
    std::fprintf(stderr, "mode inconnu : %s\n", mode);
    return 2;
  }
  if (rc == 0) std::printf("selftest --%s : conforme\n", mode);
  return rc;
}
