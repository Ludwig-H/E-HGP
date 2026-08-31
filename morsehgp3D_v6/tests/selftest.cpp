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
#include <limits>
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
  // Plafond de capacite du prefiltre (frontiere, sans allocation geante).
  check(candidates_capacity_ok((size_t)std::numeric_limits<u32>::max()), "capacite : 2^32-1 accepte");
  check(!candidates_capacity_ok((size_t)std::numeric_limits<u32>::max() + 1), "capacite : 2^32 refuse");
  // Racine entiere PURE : les deux cotes de la frontiere d'arrondi.
  check(isqrt64_pure(0) == 0 && isqrt64_pure(1) == 1 && isqrt64_pure(2) == 1, "isqrt64_pure petites");
  for (const i64 r : {(i64)7, (i64)447, (i64)565, (i64)566, (i64)293938}) {
    check(isqrt64_pure(r * r) == r, "isqrt64_pure carre exact");
    check(isqrt64_pure(r * r - 1) == r - 1, "isqrt64_pure sous le carre");
    check(isqrt64_pure(r * r + 2 * r) == r, "isqrt64_pure dernier avant le carre suivant");
  }
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

// ---- --sha256 : vecteurs FIPS 180-4, streaming, et EGALITE EXPLICITE des
// deux chemins de compression (SHA-NI et portable, `Sha256(force_portable)`).
// Quand le CPU n'a pas SHA-NI, la comparaison est portable contre portable
// (trivialement egale) : le verdict inter-chemins n'est etabli que la ou
// `Sha256::accelerated()` est vrai — la ligne imprimee le dit.
int run_sha256() {
  const auto hex = [](const char* msg, bool force_portable) {
    Sha256 h(force_portable);
    h.update(msg, std::strlen(msg));
    return h.hex();
  };
  // Vecteurs FIPS sur CHAQUE chemin (le portable est correct par lui-meme,
  // pas seulement egal au chemin par defaut).
  for (const bool portable : {false, true}) {
    check(hex("abc", portable) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "FIPS abc");
    check(hex("", portable) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "FIPS vide");
    check(hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", portable) ==
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
          "FIPS deux blocs");
    Sha256 s(portable);
    s.update("ab", 2);
    s.update("c", 1);
    check(s.hex() == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "streaming");
  }
  // Tampon pseudo-aleatoire de 1 Mo (deterministe, graine 20260831) : les deux
  // chemins doivent rendre le meme digest, d'un bloc comme en tranches
  // irregulieres (61 o, premier avec 64 : toutes les phases du tampon interne).
  std::vector<unsigned char> buf(1u << 20);
  std::mt19937_64 rng(20260831ull);
  for (size_t i = 0; i < buf.size(); i += 8) {
    const u64 v = rng();
    for (size_t j = 0; j < 8 && i + j < buf.size(); ++j) buf[i + j] = (unsigned char)(v >> (8 * j));
  }
  Sha256 h_auto, h_port(true), h_chunk(true);
  h_auto.update(buf.data(), buf.size());
  h_port.update(buf.data(), buf.size());
  for (size_t off = 0; off < buf.size(); off += 61)
    h_chunk.update(buf.data() + off, std::min<size_t>(61, buf.size() - off));
  const std::string d_auto = h_auto.hex(), d_port = h_port.hex(), d_chunk = h_chunk.hex();
  check(d_auto == d_port, "1 Mo : chemin par defaut == chemin portable force");
  check(d_port == d_chunk, "1 Mo : portable d'un bloc == portable en tranches de 61 o");
  std::printf("sha256 : SHA-NI %s ; 1 Mo auto=%.16s... portable=%.16s...\n",
              Sha256::accelerated() ? "ACTIF (comparaison inter-chemins effective)"
                                    : "absent (portable contre portable seulement)",
              d_auto.c_str(), d_port.c_str());
  return g_failures ? 1 : 0;
}

// Digest du FLUX de points d'une famille : pour chaque point, id en u32 LE
// puis x/y/z en i64 LE (aucune ambiguite de concatenation : largeurs fixes).
std::string family_stream_digest(CloudFamily f, int n, long long seed) {
  const int coord = cloud_family_default_coord(f, n);
  const std::vector<InputPoint> pts = make_family_input(f, n, coord, seed);
  Sha256 h;
  for (const InputPoint& p : pts) {
    unsigned char b[4];
    for (int i = 0; i < 4; ++i) b[i] = (unsigned char)((u32)p.id >> (8 * i));
    h.update(b, 4);
    h.i64le((int64_t)p.position.x);
    h.i64le((int64_t)p.position.y);
    h.i64le((int64_t)p.position.z);
  }
  return h.hex();
}

// Digests GRAVES des neuf familles au quadruplet (n = 2000, coord par defaut,
// graine 3) — copies d'un run d'impression sur ce depot, jamais recalcules
// par la formule sous test. Toute divergence de generation est un changement
// d'objet (l'en-tete de src/cloud/families.hpp l'exige).
struct FamilyDigest {
  CloudFamily f;
  const char* hex;
};
constexpr FamilyDigest kFamilyDigests2000[] = {
    {CloudFamily::kUniform, "0b29cc84bcdcfe0871df6d91d757278d5fe8e4fbbe2007f4529fbaf239cefb0a"},
    {CloudFamily::kTerrain, "05e45b62450e5be5e84c665f6efdbdca237ec2d0d7c54c407fd68b2812d1ec54"},
    {CloudFamily::kScanlineSinglePass, "688c72d83239dab7c415dce9fab5960692f554a0f2b9b171fc2d74085632f24b"},
    {CloudFamily::kScanlineOverlapMultiecho, "52791b0dca4774098081d1f5fc8d6265bb16579173deca923a7a46c64326d728"},
    {CloudFamily::kEightClusters, "990b26dd380a9bac67a398cea7732390c213425a694304616afedd14d2b18eb5"},
    {CloudFamily::kTwoLines, "94cb1af42b50c56231ba9fff7021bc8781ff5d817aa16f4c9637aa407f4f6caa"},
    {CloudFamily::kCollinearSeven, "652722d05b1fe242d3a2813126a335ab6dbc957b37af5e117cb360e157fa4aec"},
    {CloudFamily::kTerrainStationnaire, "015500cbbd475eab602727cfec6d06dc1274ffa4242e5125233a9b06aee7fa7c"},
    {CloudFamily::kScanlineStationnaire, "7f6b20d2590c939b83097b850fc97c95b0f02ecba592d2c4e473418e02c7b948"},
};

// ---- --families : determinisme, profil, cardinalite ; stationnaires
// comprises ; digests graves. Sous --inject (family-scanline-overshoot) :
// seul le verdict de digest compte — kill (4) si au moins une famille diverge
// de la table gravee, sinon 3. Le cas grave du kill est
// scanline_overlap_multiecho a n = 2000, graine 3. LIMITE HONNETE : sur
// scanline_single_pass le mutant est STRUCTURELLEMENT invisible a tout n —
// toutes ses passes ont multi_echo = false, donc l'unique push(ground) par
// site est deja borne par la condition de boucle `pts->size() < n` et la
// garde relachee de `push` n'est jamais le facteur limitant ; et sur
// scanline_stationnaire a ce quadruplet, aucun echo ne tombe sur l'instant
// exact du franchissement du cap (digest inchange, verifie).
int run_families(bool injected) {
  if (injected) {
    u64 diverged = 0;
    for (const FamilyDigest& fd : kFamilyDigests2000) {
      const std::string got = family_stream_digest(fd.f, 2000, 3);
      const bool differ = got != fd.hex;
      if (differ) ++diverged;
      std::printf("mutant %-28s digest %s\n", cloud_family_name(fd.f), differ ? "DIVERGE" : "identique");
    }
    if (diverged) {
      std::printf("families : mutant TUE (%llu famille(s) divergente(s))\n", (unsigned long long)diverged);
      return 4;
    }
    std::fprintf(stderr, "families : mutant NON tue (aucun digest ne diverge)\n");
    return 3;
  }
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
  // Digests graves : les neuf familles au quadruplet (n = 2000, coord par
  // defaut, graine 3) contre la table litterale.
  for (const FamilyDigest& fd : kFamilyDigests2000) {
    const std::string got = family_stream_digest(fd.f, 2000, 3);
    if (got != fd.hex) {
      check(false, "digest de famille grave (n=2000, graine 3)");
      std::fprintf(stderr, "  %s : attendu %s\n  %s : obtenu  %s\n", cloud_family_name(fd.f), fd.hex,
                   cloud_family_name(fd.f), got.c_str());
    }
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
  u64 apparatus_bad = 0;  // le mutant lui-meme doit respecter sa declaration
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
    if (injected && mutant_enabled("wspd-drop-rect")) {
      // DELTA -1 LITTERAL (audit du 31 aout, cinquieme cycle) : le mutant
      // wspd-drop-rect perd exactement UN rectangle par DESCENTE (plus un par
      // vague), et le grand-livre reconstruit ferme avec la masse omise :
      // emis + tues + omis == attendu pour chaque lane. Un mutant qui ne
      // respecte pas sa declaration rend 3 (survivant), jamais 4.
      if (stf.mutant_dropped_rects != 1) {
        ++apparatus_bad;
        std::fprintf(stderr, "mutant droprect : %llu omission(s) au lieu de 1 (%s)\n",
                     (unsigned long long)stf.mutant_dropped_rects, cloud_family_name(f));
      }
      for (int q = 0; q < 3; ++q)
        if (stf.ledger_emitted_mass[q] + stf.ledger_killed_mass[q] + stf.mutant_dropped_mass[q] != expected) {
          ++apparatus_bad;
          std::fprintf(stderr, "mutant droprect : identite emis+tues+omis != attendu en lane %d (%s)\n", q + 2,
                       cloud_family_name(f));
        }
    }
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
  if (injected) return (mismatches && !apparatus_bad) ? 4 : 3;
  return (mismatches || apparatus_bad) ? 1 : 0;
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
  else if (m == "families") rc = run_families(injected);
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
