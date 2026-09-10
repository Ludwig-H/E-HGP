// Sonde d'audit independante : semantique de full_coverage_certificate.hpp.
// Chaque scenario imprime l'attendu (calcule a la main) et l'observe.
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "src/forest/full_coverage_certificate.hpp"

using namespace mhgp7;
using Bank = std::shared_ptr<const FullCoveragePopulations>;

static int diffs = 0, checks = 0;
static void line(const char* name, bool ok, const std::string& expected, const std::string& got) {
  ++checks;
  if (!ok) ++diffs;
  std::printf("%-58s %s  attendu=[%s] observe=[%s]\n", name, ok ? "OK  " : "DIFF", expected.c_str(),
              got.c_str());
}
static std::string vec(const std::vector<PointId>& v) {
  std::string s = "{";
  for (size_t i = 0; i < v.size(); ++i) { if (i) s += ","; s += std::to_string(v[i]); }
  return s + "}";
}
static std::string id(FullNodeId x) { return x == kFullCoverageAbsent ? "ABSENT" : std::to_string(x); }
static ExactLevel lv(u64 n, i128 d = 1) { return {{n, 0, 0}, d}; }
static ExactLevel lv3(u64 n0, u64 n1, u64 n2, i128 d) { return {{n0, n1, n2}, d}; }

static Bank bank(const std::vector<PointId>& domain, const std::vector<FullCoveragePopulation>& rows) {
  auto r = build_full_coverage_populations(domain, rows);
  if (r.status != FullCertificateStatus::kOk) { std::printf("BANK REFUSE (%s)\n", r.reason); return {}; }
  return r.value;
}
static std::string status_of(FullCertificateStatus s) {
  switch (s) {
    case FullCertificateStatus::kOk: return "ok";
    case FullCertificateStatus::kInvalidInput: return "invalid";
    case FullCertificateStatus::kResourceExhausted: return "exhausted";
  }
  return "?";
}
static FullCoverageBuildResult build(unsigned k, Bank b, const std::vector<FullCoverageBatch>& batches) {
  return build_full_coverage_certificate(k, std::move(b), batches);
}
static void expect_reject(const char* name, unsigned k, Bank b, const std::vector<FullCoverageBatch>& batches,
                          const char* reason) {
  auto r = build(k, std::move(b), batches);
  const std::string got = status_of(r.status) + ":" + r.reason + " nodes=" + std::to_string(r.value.nodes().size());
  line(name, r.status == FullCertificateStatus::kInvalidInput && std::string_view(r.reason) == reason &&
       r.value.order() == 0 && r.value.nodes().empty(), std::string("invalid:") + reason + " nodes=0", got);
}
static void expect_ok(const char* name, const FullCoverageBuildResult& r, size_t nodes, size_t contribs) {
  const std::string got = status_of(r.status) + ":" + r.reason + " nodes=" + std::to_string(r.value.nodes().size()) +
      " contribs=" + std::to_string(r.value.contributions().size());
  line(name, r.status == FullCertificateStatus::kOk && r.value.nodes().size() == nodes &&
       r.value.contributions().size() == contribs,
       "ok:structural_only nodes=" + std::to_string(nodes) + " contribs=" + std::to_string(contribs), got);
}
static void expect_root(const char* name, const FullCoverageCertificate& f, FullNodeId seg, ExactLevel cut,
                        bool closed, FullNodeId expected) {
  const auto got = full_coverage_root_at(f, seg, cut, closed);
  line(name, got == expected, id(expected), id(got));
}
static void expect_cov(const char* name, const FullCoverageCertificate& f, FullNodeId root, ExactLevel cut,
                       bool closed, const std::vector<PointId>& expected) {
  const auto r = full_coverage_at(f, root, cut, closed);
  line(name, r.status == FullCertificateStatus::kOk && r.values == expected, "ok " + vec(expected),
       status_of(r.status) + ":" + r.reason + " " + vec(r.values));
}
static void expect_cov_invalid(const char* name, const FullCoverageCertificate& f, FullNodeId root, ExactLevel cut,
                               bool closed) {
  const auto r = full_coverage_at(f, root, cut, closed);
  line(name, r.status == FullCertificateStatus::kInvalidInput && r.values.empty(), "invalid {}",
       status_of(r.status) + ":" + r.reason + " " + vec(r.values));
}

// ---------------------------------------------------------------------------
static void s01_niveaux_rationnels() {
  std::puts("== S01 niveaux rationnels non reduits, egalite exacte au lecteur");
  auto b = bank({0, 1, 2, 3, 4, 5}, {{{}, {0, 1, 2}}, {{}, {3, 4, 5}}, {{1}, {0, 2, 3}}});
  // 9/1 puis 18/2 : meme niveau exact -> deux lots interdits.
  expect_reject("S01a 9/1 puis 18/2 rejete (nonincreasing)", 3, b,
      {{lv(9), {{{}, {{0, 7, false}}}}}, {lv(18, 2), {{{}, {{1, 7, false}}}}}}, "coverage_nonincreasing_batch");
  // 9/1 puis 19/2 : accepte ; coupe 18/2 (=9) fermee/ouverte, coupe 38/4 (=19/2).
  auto r = build(3, b, {{lv(9), {{{}, {{0, 7, false}}}}}, {lv(19, 2), {{{}, {{1, 7, false}}}}},
                        {lv(10), {{{0, 1}, {{2, 4, true}}}}}});
  expect_ok("S01b 9/1 puis 19/2 puis 10/1 accepte", r, 3, 3);
  const auto& f = r.value;
  expect_root("S01c root(0) coupe 18/2 fermee = 0", f, 0, lv(18, 2), true, 0);
  expect_root("S01d root(0) coupe 18/2 ouverte = ABSENT", f, 0, lv(18, 2), false, kFullCoverageAbsent);
  expect_root("S01e root(1) coupe 38/4 ouverte = ABSENT", f, 1, lv(38, 4), false, kFullCoverageAbsent);
  expect_root("S01f root(1) coupe 38/4 fermee = 1", f, 1, lv(38, 4), true, 1);
  expect_root("S01g root(0) coupe 30/3 fermee = 2", f, 0, lv(30, 3), true, 2);
  expect_root("S01h root(0) coupe 30/3 ouverte = 0", f, 0, lv(30, 3), false, 0);
  expect_cov("S01i cov(2) coupe 20/2 fermee = {0..5} (I={1} + coq bit2=3)", f, 2, lv(20, 2), true, {0, 1, 2, 3, 4, 5});
  expect_cov("S01j cov(0) coupe 20/2 ouverte = {0,1,2}", f, 0, lv(20, 2), false, {0, 1, 2});
  // Coupe a denominateur negatif / nul.
  expect_root("S01k coupe den=0 -> ABSENT", f, 0, lv(9, 0), true, kFullCoverageAbsent);
  expect_root("S01l coupe den<0 -> ABSENT", f, 0, lv(9, -1), true, kFullCoverageAbsent);
  expect_cov_invalid("S01m cov coupe den<0 -> invalid", f, 0, lv(9, -1), true);
}

static void s02_trois_limbs() {
  std::puts("== S02 niveaux a trois limbs et grands denominateurs");
  auto b = bank({0, 1, 2}, {{{}, {0, 1, 2}}});
  const i128 big = (i128{1} << 60);
  // niveau A = 2^128 / 1 ; niveau B = (2^128+1)/1 ; coupe = 2^129/2 == A ; coupe' = (2^128+1)*2^120 / 2^120 == B
  auto r = build(3, b, {{lv3(0, 0, 1, 1), {{{}, {{0, 7, false}}}}},
                        {lv3(1, 0, 1, 1), {{{}, {{0, 7, false}}}}}});
  expect_ok("S02a 2^128 puis 2^128+1 accepte", r, 2, 2);
  const auto& f = r.value;
  expect_root("S02b root(0) coupe 2^129/2 fermee = 0", f, 0, lv3(0, 0, 2, 2), true, 0);
  expect_root("S02c root(0) coupe 2^129/2 ouverte = ABSENT", f, 0, lv3(0, 0, 2, 2), false, kFullCoverageAbsent);
  expect_root("S02d root(1) coupe 2^129/2 fermee = ABSENT", f, 1, lv3(0, 0, 2, 2), true, kFullCoverageAbsent);
  expect_root("S02e root(1) coupe (2^128+1)*2^60/2^60 fermee = 1", f, 1,
              lv3(u64{1} << 60, 0, u64{1} << 60, big), true, 1);
  expect_root("S02e2 root(1) coupe (2^128+1)*2^60/2^60 ouverte = ABSENT", f, 1,
              lv3(u64{1} << 60, 0, u64{1} << 60, big), false, kFullCoverageAbsent);
  expect_root("S02e3 root(0) coupe (2^128+1)*2^60/2^60 fermee = 0", f, 0,
              lv3(u64{1} << 60, 0, u64{1} << 60, big), true, 0);
  // niveaux 1/2^126 puis 1/2^125 (croissants) ; coupe 2/2^127 == premier
  auto r2 = build(3, b, {{lv(1, i128{1} << 125), {{{}, {{0, 7, false}}}}},
                         {lv(1, i128{1} << 124), {{{}, {{0, 7, false}}}}}});
  expect_ok("S02f 1/2^125 puis 1/2^124 accepte", r2, 2, 2);
  expect_root("S02g root(0) coupe 2/2^126 fermee = 0", r2.value, 0, lv(2, i128{1} << 126), true, 0);
  expect_root("S02g2 root(0) coupe 2/2^126 ouverte = ABSENT", r2.value, 0, lv(2, i128{1} << 126), false, kFullCoverageAbsent);
  expect_root("S02h root(1) coupe 2/2^126 fermee = ABSENT", r2.value, 1, lv(2, i128{1} << 126), true, kFullCoverageAbsent);
  expect_root("S02h2 root(1) coupe 4/2^126 fermee = 1", r2.value, 1, lv(4, i128{1} << 126), true, 1);
  expect_reject("S02i 1/2^125 puis 1/2^126 rejete", 3, b,
      {{lv(1, i128{1} << 125), {{{}, {{0, 7, false}}}}}, {lv(1, i128{1} << 126), {{{}, {{0, 7, false}}}}}},
      "coverage_nonincreasing_batch");
  // denominateur i128 maximal
  const i128 dmax = ~(i128{1} << 127);
  auto r3 = build(3, b, {{lv(1, dmax), {{{}, {{0, 7, false}}}}}, {lv(2, dmax), {{{}, {{0, 7, false}}}}}});
  expect_ok("S02j den = INT128_MAX accepte", r3, 2, 2);
  expect_root("S02k root(1) coupe 2/INT128_MAX fermee = 1", r3.value, 1, lv(2, dmax), true, 1);
  expect_root("S02l root(1) coupe 1/1 fermee = 1", r3.value, 1, lv(1), true, 1);
}

static void s03_chaine_longue() {
  std::puts("== S03 chaine longue de successeurs (10 fusions successives)");
  std::vector<PointId> domain;
  std::vector<FullCoveragePopulation> rows;
  for (PointId p = 0; p < 24; p += 2) { domain.push_back(p); domain.push_back(p + 1); rows.push_back({{}, {p, p + 1}}); }
  auto b = bank(domain, rows);
  std::vector<FullCoverageBatch> batches;
  batches.push_back({lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}});  // nodes 0,1
  // lot i (i>=2): naissance de la population i (node 2i-1) puis fusion {2i-2 ou racine courante, nouveau} -> node 2i
  // Simplifions : lot t=2 : fusion {0,1} -> node 2 ; lot t=3 : naissance pop2 -> node 3, fusion {2,3} -> node 4 ; ...
  batches.push_back({lv(2), {{{0, 1}, {}}}});
  FullNodeId current = 2;
  for (u64 t = 3; t <= 12; ++t) {
    const u64 pop = t - 1;
    batches.push_back({lv(2 * t), {{{}, {{pop, 3, false}}}}});                 // noeud current+1
    batches.push_back({lv(2 * t + 1), {{{current, current + 1}, {}}}});      // noeud current+2
    current += 2;
  }
  auto r = build(2, b, batches);
  expect_ok("S03a chaine construite", r, 23, 12);
  const auto& f = r.value;
  // fusion a 2t+1 -> noeud 2t-2 ; naissance a 2t -> noeud 2t-3 (t>=3) ; noeud 22 = fusion a 25
  expect_root("S03b root(0) coupe 25 fermee = 22", f, 0, lv(25), true, 22);
  expect_root("S03c root(0) coupe 25 ouverte = 20", f, 0, lv(25), false, 20);
  expect_root("S03d root(0) coupe 2 ouverte = 0", f, 0, lv(2), false, 0);
  expect_root("S03e root(0) coupe 15 fermee = 12 (fusion a 15)", f, 0, lv(15), true, 12);
  expect_root("S03e2 root(0) coupe 16 fermee = 12", f, 0, lv(16), true, 12);
  expect_root("S03f root(21) coupe 24 ouverte = ABSENT (nait a 24)", f, 21, lv(24), false, kFullCoverageAbsent);
  expect_root("S03f2 root(21) coupe 24 fermee = 21", f, 21, lv(24), true, 21);
  expect_root("S03g root(21) coupe 25 fermee = 22", f, 21, lv(25), true, 22);
  std::vector<PointId> all; for (PointId p = 0; p < 24; ++p) all.push_back(p);
  expect_cov("S03h cov(22) coupe 25 fermee = {0..23}", f, 22, lv(25), true, all);
  std::vector<PointId> upto7; for (PointId p = 0; p < 14; ++p) upto7.push_back(p);
  expect_cov("S03i cov(12) coupe 15 fermee = {0..13}", f, 12, lv(15), true, upto7);
  expect_cov("S03i2 cov(12) coupe 16 fermee = {0..13}", f, 12, lv(16), true, upto7);
  std::vector<PointId> upto6; for (PointId p = 0; p < 12; ++p) upto6.push_back(p);
  expect_cov("S03i3 cov(10) coupe 15 ouverte = {0..11}", f, 10, lv(15), false, upto6);
  expect_cov_invalid("S03j cov(0) coupe 15 fermee -> invalid (pas racine)", f, 0, lv(15), true);
  expect_cov_invalid("S03k cov(10) coupe 15 fermee -> invalid (fusionne a 15)", f, 10, lv(15), true);
}

static void s04_multifusion_contributions() {
  std::puts("== S04 multifusion avec contributions, coupe ouverte/fermee au niveau de fusion");
  auto b = bank({0, 1, 2, 3, 4, 5, 6}, {{{}, {0, 1}}, {{}, {2, 3}}, {{}, {4, 5}}, {{1, 2, 4}, {0, 3, 5, 6}}});
  auto r = build(2, b, {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}, {{}, {{2, 3, false}}}}},
                        {lv(4), {{{0, 1, 2}, {{3, 8, false}}}}}});
  expect_ok("S04a trois naissances + multifusion a 3 parents", r, 4, 4);
  const auto& f = r.value;
  expect_root("S04b root(0) coupe 4 ouverte = 0", f, 0, lv(4), false, 0);
  expect_root("S04c root(2) coupe 4 fermee = 3", f, 2, lv(4), true, 3);
  expect_cov("S04d cov(0) coupe 4 ouverte = {0,1}", f, 0, lv(4), false, {0, 1});
  expect_cov("S04e cov(3) coupe 4 fermee = {0..6}", f, 3, lv(4), true, {0, 1, 2, 3, 4, 5, 6});
  expect_cov_invalid("S04f cov(3) coupe 4 ouverte -> invalid (pas encore ne)", f, 3, lv(4), false);
  expect_cov_invalid("S04g cov(0) coupe 4 fermee -> invalid (fusionne)", f, 0, lv(4), true);
  expect_cov_invalid("S04h cov(3) coupe 3 fermee -> invalid", f, 3, lv(3), true);
  // Multifusion sans contribution + continuation d'un autre segment dans le meme lot
  auto b2 = bank({0, 1, 2, 3, 4, 5}, {{{}, {0, 1}}, {{}, {2, 3}}, {{}, {4, 5}}, {{4}, {0, 5}}});
  auto r2 = build(2, b2, {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}, {{}, {{2, 3, false}}}}},
                          {lv(4), {{{0, 1}, {}}, {{2}, {{3, 1, false}}}}}});
  expect_ok("S04i fusion sans contribution + continuation meme lot", r2, 4, 4);
  expect_cov("S04j cov(3) coupe 4 fermee = {0,1,2,3}", r2.value, 3, lv(4), true, {0, 1, 2, 3});
  expect_cov("S04k cov(2) coupe 4 fermee = {0,4,5} (continuation)", r2.value, 2, lv(4), true, {0, 4, 5});
  expect_cov("S04l cov(2) coupe 4 ouverte = {4,5}", r2.value, 2, lv(4), false, {4, 5});
}

static void s05_continuation_puis_fusion() {
  std::puts("== S05 continuation puis fusion : meme lot rejete, lots differents acceptes");
  auto b = bank({0, 1, 2, 3, 4}, {{{}, {0, 1}}, {{}, {2, 3}}, {{1}, {0, 4}}});
  expect_reject("S05a continuation {0} et fusion {0,1} meme lot -> rejete", 2, b,
      {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}}, {lv(2), {{{0}, {{2, 2, false}}}, {{0, 1}, {}}}}},
      "coverage_parent_not_unique_prebatch_root");
  expect_reject("S05b fusion {0,1} puis continuation {0} meme lot -> rejete", 2, b,
      {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}}, {lv(2), {{{0, 1}, {}}, {{0}, {{2, 2, false}}}}}},
      "coverage_parent_not_unique_prebatch_root");
  auto r = build(2, b, {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}},
                        {lv(2), {{{0}, {{2, 2, false}}}}}, {lv(3), {{{0, 1}, {}}}}});
  expect_ok("S05c continuation a 2 puis fusion a 3", r, 3, 3);
  const auto& f = r.value;
  expect_cov("S05d cov(0) coupe 2 fermee = {0,1,4}", f, 0, lv(2), true, {0, 1, 4});
  expect_cov("S05e cov(0) coupe 2 ouverte = {0,1}", f, 0, lv(2), false, {0, 1});
  expect_cov("S05f cov(2) coupe 3 fermee = {0,1,2,3,4}", f, 2, lv(3), true, {0, 1, 2, 3, 4});
  expect_cov("S05g cov(0) coupe 3 ouverte = {0,1,4}", f, 0, lv(3), false, {0, 1, 4});
  expect_root("S05h successeur de 0 = 2 (immuable)", f, 0, lv(100), true, 2);
  line("S05i successors()[0]==2, [1]==2, [2]==ABSENT",
       f.successors()[0] == 2 && f.successors()[1] == 2 && f.successors()[2] == kFullCoverageAbsent, "oui",
       id(f.successors()[0]) + "," + id(f.successors()[1]) + "," + id(f.successors()[2]));
  // contribution datee sur un segment deja fusionne (continuation sur parent mort) -> rejete
  expect_reject("S05j continuation sur parent fusionne au lot precedent -> rejete", 2, b,
      {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}}, {lv(2), {{{0, 1}, {}}}},
       {lv(3), {{{0}, {{2, 2, false}}}}}}, "coverage_parent_not_unique_prebatch_root");
}

static void s06_k1_et_kn() {
  std::puts("== S06 K1 et K=n");
  auto b = bank({5, 7, 9}, {{{}, {5}}, {{7}, {}}, {{}, {9}}, {{}, {5, 7, 9}}});
  auto r = build(1, b, {{lv(0), {{{}, {{0, 1, false}}}, {{}, {{1, 0, true}}}, {{}, {{2, 1, false}}}}},
                        {lv(4), {{{0, 1}, {}}}}, {lv(9), {{{2, 3}, {}}}}});
  expect_ok("S06a K1 trois singletons + deux fusions", r, 5, 3);
  const auto& f = r.value;
  expect_root("S06b root(0) coupe 0 ouverte = ABSENT", f, 0, lv(0), false, kFullCoverageAbsent);
  expect_root("S06c root(0) coupe 0 fermee = 0", f, 0, lv(0), true, 0);
  expect_cov("S06d cov(1) coupe 0 fermee = {7}", f, 1, lv(0), true, {7});
  expect_cov("S06e cov(4) coupe 9 fermee = {5,7,9}", f, 4, lv(9), true, {5, 7, 9});
  expect_reject("S06f K1 singletons hors ordre du domaine -> rejete", 1, b,
      {{lv(0), {{{}, {{2, 1, false}}}, {{}, {{1, 0, true}}}, {{}, {{0, 1, false}}}}}}, "coverage_k1_roots");
  expect_reject("S06g K1 naissance tardive -> rejete", 1, b,
      {{lv(0), {{{}, {{0, 1, false}}}, {{}, {{1, 0, true}}}, {{}, {{2, 1, false}}}}}, {lv(1), {{{}, {{0, 1, false}}}}}},
      "coverage_k1_roots");
  expect_reject("S06h K1 naissance de cardinal 3 -> rejete", 1, b,
      {{lv(0), {{{}, {{3, 7, false}}}, {{}, {{1, 0, true}}}, {{}, {{2, 1, false}}}}}}, "coverage_k1_roots");
  expect_reject("S06i K1 premier lot niveau 1 -> rejete", 1, b,
      {{lv(1), {{{}, {{0, 1, false}}}, {{}, {{1, 0, true}}}, {{}, {{2, 1, false}}}}}}, "coverage_k1_roots");
  expect_reject("S06j K2 niveau 0 -> rejete", 2, b, {{lv(0), {{{}, {{3, 7, false}}}}}}, "coverage_positive_level_required");
  // K = n = 3 sur le meme bank, et K=10 avec n=10
  auto rn = build(3, b, {{lv(9), {{{}, {{3, 7, false}}}}}});
  expect_ok("S06k K=n=3 naissance terminale", rn, 1, 1);
  expect_cov("S06l cov(0) K=n coupe 9 fermee = {5,7,9}", rn.value, 0, lv(9), true, {5, 7, 9});
  std::vector<PointId> ten; for (PointId p = 0; p < 10; ++p) ten.push_back(p);
  auto b10 = bank(ten, {{{2, 3, 4, 5, 6}, {0, 1, 7, 8, 9}}});
  auto r10 = build(10, b10, {{lv(1), {{{}, {{0, 31, true}}}}}});
  expect_ok("S06m K=n=10", r10, 1, 1);
  expect_cov("S06n cov K=10 = {0..9}", r10.value, 0, lv(1), true, ten);
  expect_reject("S06o K=11 -> rejete", 11, b10, {{lv(1), {{{}, {{0, 31, true}}}}}}, "coverage_invalid_domain");
  expect_reject("S06p K=10 avec naissance de 9 points -> rejete", 10,
      bank(ten, {{{2, 3, 4, 5}, {0, 1, 7, 8, 9}}}), {{lv(1), {{{}, {{0, 31, true}}}}}}, "coverage_birth_population");
}

static void s07_masque16() {
  std::puts("== S07 masque 16 bits, bit 15, largeur");
  std::vector<PointId> dom; for (PointId p = 0; p < 20; ++p) dom.push_back(p);
  std::vector<PointId> shell; for (PointId p = 0; p < 16; ++p) shell.push_back(p + 3);
  auto b = bank(dom, {{{}, {0, 1}}, {{1, 2}, shell}});
  auto r = build(2, b, {{lv(1), {{{}, {{0, 3, false}}}}}, {lv(2), {{{0}, {{1, 0x8000, false}}}}},
                        {lv(3), {{{0}, {{1, 0x0001, true}}}}}});
  expect_ok("S07a coquille 16, masques 0x8000 puis 0x0001+I", r, 1, 3);
  expect_cov("S07b cov coupe 2 fermee = {0,1,18}", r.value, 0, lv(2), true, {0, 1, 18});
  expect_cov("S07c cov coupe 3 fermee = {0,1,2,3,18}", r.value, 0, lv(3), true, {0, 1, 2, 3, 18});
  expect_reject("S07d masque 0x10000 tronque en u16 ? (0 -> contribution vide)", 2, b,
      {{lv(1), {{{}, {{0, 3, false}}}}}, {lv(2), {{{0}, {{1, static_cast<u16>(0x10000), false}}}}}},
      "coverage_empty_or_invalid_mask");
  auto b3 = bank(dom, {{{}, {0, 1, 2}}});
  expect_reject("S07e masque 8 sur coquille de 3 -> rejete", 3, b3,
      {{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{0, 8, false}}}}}}, "coverage_empty_or_invalid_mask");
  expect_reject("S07f naissance masque partiel -> rejete", 3, b3, {{lv(1), {{{}, {{0, 3, false}}}}}},
      "coverage_birth_population");
  std::vector<PointId> shell17; for (PointId p = 0; p < 17; ++p) shell17.push_back(p);
  auto rb = build_full_coverage_populations(dom, std::vector<FullCoveragePopulation>{{{}, shell17}});
  line("S07g banque coquille 17 refusee", rb.status == FullCertificateStatus::kInvalidInput && !rb.value,
       "invalid", status_of(rb.status));
}

static void s08_populations_dupliquees_et_permissivites() {
  std::puts("== S08 entrees geometriquement impossibles mais acceptees par le format ?");
  auto b = bank({0, 1, 2, 3}, {{{}, {0, 1, 2}}, {{}, {0, 1, 2}}, {{3}, {0, 1}}});
  auto r = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{}, {{0, 7, false}}}}}});
  line("S08a meme ligne de population pour deux naissances (niveaux 1 et 2) : acceptee ?",
       r.status == FullCertificateStatus::kOk, "(observation)", status_of(r.status) + ":" + r.reason);
  auto r1 = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}, {{}, {{0, 7, false}}}}}});
  line("S08b meme ligne pour deux naissances AU MEME niveau : acceptee ?",
       r1.status == FullCertificateStatus::kOk, "(observation)", status_of(r1.status) + ":" + r1.reason);
  auto r1b = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}, {{}, {{1, 7, false}}}}}});
  line("S08c deux lignes identiques (0 et 1) pour deux naissances au meme niveau : acceptee ?",
       r1b.status == FullCertificateStatus::kOk, "(observation)", status_of(r1b.status) + ":" + r1b.reason);
  auto r2 = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{2, 0, true}}}}}});
  line("S08d continuation avec include_interior=true (D_B doit etre dans U) : acceptee ?",
       r2.status == FullCertificateStatus::kOk, "(observation)", status_of(r2.status) + ":" + r2.reason);
  auto r3 = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{0, 7, false}}}}}});
  line("S08e continuation redondante = population entiere de la naissance : acceptee ?",
       r3.status == FullCertificateStatus::kOk, "(observation)", status_of(r3.status) + ":" + r3.reason);
  if (r3.status == FullCertificateStatus::kOk)
    expect_cov("S08f union idempotente {0,1,2}", r3.value, 0, lv(2), true, {0, 1, 2});
  // naissance dont la population ne contient AUCUN point d'ordre K ... (cardinal < K) -> rejet
  expect_reject("S08g naissance cardinal 2 a K=3 -> rejete", 3, bank({0, 1, 2, 3}, {{{}, {0, 1}}}),
      {{lv(1), {{{}, {{0, 3, false}}}}}}, "coverage_birth_population");
  // K1 : lignes de population dans un ordre different du domaine, references par index libre
  auto bk1 = bank({5, 7, 9}, {{{}, {9}}, {{7}, {}}, {{}, {5}}});
  auto rk1 = build(1, bk1, {{lv(0), {{{}, {{2, 1, false}}}, {{}, {{1, 0, true}}}, {{}, {{0, 1, false}}}}}});
  expect_ok("S08j K1 lignes permutees, references correctes acceptees", rk1, 3, 3);
  expect_cov("S08k K1 cov(0) = {5}", rk1.value, 0, lv(0), true, {5});
  // naissance a K=2 avec population de cardinal 3 : autorisee (couverture > K)
  auto r4 = build(2, b, {{lv(1), {{{}, {{2, 3, true}}}}}});
  expect_ok("S08h naissance cardinal 3 a K=2 acceptee", r4, 1, 1);
  // population avec coquille vide (impossible geometriquement) acceptee ?
  auto bi = bank({0, 1, 2}, {{{0, 1, 2}, {}}});
  auto r5 = build(2, bi, {{lv(1), {{{}, {{0, 0, true}}}}}});
  line("S08i naissance avec coquille vide (U=0) : acceptee ?", r5.status == FullCertificateStatus::kOk,
       "(observation)", status_of(r5.status) + ":" + r5.reason);
}

static void s09_entrees_invalides() {
  std::puts("== S09 entrees invalides");
  auto b = bank({0, 1, 2, 3, 4}, {{{}, {0, 1}}, {{}, {2, 3}}, {{1}, {0, 4}}});
  const std::vector<FullCoverageBatch> base{{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}},
      {lv(2), {{{0}, {{2, 2, false}}}}}, {lv(3), {{{0, 1}, {}}}}};
  auto bad = base; bad[2].actions[0].parents = {1, 0};
  expect_reject("S09a parents non tries", 2, b, bad, "coverage_parent_not_unique_prebatch_root");
  bad = base; bad[2].actions[0].parents = {0, 0};
  expect_reject("S09b parent repete", 2, b, bad, "coverage_parent_not_unique_prebatch_root");
  bad = base; bad[2].actions[0].parents = {0, 5};
  expect_reject("S09c parent hors domaine", 2, b, bad, "coverage_parent_not_unique_prebatch_root");
  bad = base; bad[2].actions[0].parents = {0, kFullCoverageAbsent};
  expect_reject("S09d parent = ABSENT", 2, b, bad, "coverage_parent_not_unique_prebatch_root");
  bad = base; bad[1].actions[0].contributions[0].population = 3;
  expect_reject("S09e population hors banque", 2, b, bad, "coverage_population_reference");
  bad = base; bad[1].actions[0].contributions[0].population = std::numeric_limits<u64>::max();
  expect_reject("S09f population = u64 max", 2, b, bad, "coverage_population_reference");
  bad = base; bad[1].actions[0].contributions.clear();
  expect_reject("S09g continuation vide", 2, b, bad, "coverage_empty_continuation");
  bad = base; bad[1].actions[0].contributions[0] = {2, 0, false};
  expect_reject("S09h contribution vide (masque 0, sans I)", 2, b, bad, "coverage_empty_or_invalid_mask");
  bad = base; bad[1].actions[0].contributions[0] = {0, 1, true};
  expect_reject("S09i include_interior avec I vide", 2, b, bad, "coverage_empty_or_invalid_mask");
  bad = base; bad[0].actions[0].contributions[0] = {2, 3, false};
  expect_reject("S09j naissance include_interior incoherent", 2, b, bad, "coverage_birth_population");
  bad = base; bad[0].actions[0].contributions = {{0, 3, false}, {1, 3, false}};
  expect_reject("S09k naissance a deux contributions", 2, b, bad, "coverage_birth_population");
  bad = base; bad[2].level = lv(2);
  expect_reject("S09l lots non croissants (egaux)", 2, b, bad, "coverage_nonincreasing_batch");
  bad = base; bad[2].level = lv(1);
  expect_reject("S09m lots decroissants", 2, b, bad, "coverage_nonincreasing_batch");
  bad = base; bad[2].level = lv(3, 0);
  expect_reject("S09n den = 0", 2, b, bad, "coverage_invalid_level");
  bad = base; bad[2].level = lv(3, -7);
  expect_reject("S09o den < 0", 2, b, bad, "coverage_invalid_level");
  bad = base; bad[1].actions.clear();
  expect_reject("S09p lot vide", 2, b, bad, "coverage_empty_batch");
  bad = base; bad.push_back({lv(4), {{{2}, {{2, 2, false}}}}}); bad.push_back({lv(5), {{{2, 2}, {}}}});
  expect_reject("S09q fusion {2,2} apres continuation", 2, b, bad, "coverage_parent_not_unique_prebatch_root");
  expect_reject("S09r batches vides", 2, b, {}, "coverage_invalid_domain");
  expect_reject("S09s bank nul", 2, nullptr, base, "coverage_invalid_domain");
  expect_reject("S09t ordre 0", 0, b, base, "coverage_invalid_domain");
  expect_reject("S09u domaine < ordre", 5, bank({0, 1}, {{{}, {0, 1}}}), base, "coverage_invalid_domain");
  // naissance dans un lot ulterieur a K=2 avec parents=[] : acceptee (normal), verifie identifiants
  auto r = build(2, b, {{lv(1), {{{}, {{0, 3, false}}}}}, {lv(2), {{{0}, {{2, 2, false}}}, {{}, {{1, 3, false}}}}},
                        {lv(3), {{{0, 1}, {}}}}});
  expect_ok("S09v continuation puis naissance dans le meme lot : ids 0,1,2", r, 3, 3);
  line("S09w node 1 niveau 2 et node 2 parents {0,1}", r.value.nodes()[1].level == lv(2) &&
       r.value.nodes()[2].parent_count == 2 && r.value.parents()[0] == 0 && r.value.parents()[1] == 1, "oui", "?");
  // Banque : refus
  const auto brej = [&](const char* name, const std::vector<PointId>& dom, const std::vector<FullCoveragePopulation>& rows) {
    auto rb = build_full_coverage_populations(dom, rows);
    line(name, rb.status == FullCertificateStatus::kInvalidInput && !rb.value, "invalid", status_of(rb.status));
  };
  brej("S09x banque : point hors domaine", {0, 1, 2}, {{{}, {0, 3}}});
  brej("S09y banque : coquille non triee", {0, 1, 2}, {{{}, {1, 0}}});
  brej("S09z banque : I et U se recouvrent", {0, 1, 2}, {{{0}, {0, 1}}});
  brej("S09A banque : population vide", {0, 1, 2}, {{{}, {}}});
  brej("S09B banque : domaine non trie", {1, 0}, {{{}, {0}}});
  brej("S09C banque : domaine duplique", {0, 0}, {{{}, {0}}});
  brej("S09D banque : interieur duplique", {0, 1}, {{{0, 0}, {1}}});
  brej("S09E banque : domaine vide", {}, {{{}, {}}});
}

static void s10_lecteur_bords() {
  std::puts("== S10 lecteur : racine inexistante, non-racine, forest vide, banque partagee");
  auto b = bank({0, 1, 2, 3}, {{{}, {0, 1}}, {{}, {2, 3}}, {{}, {0, 1, 2, 3}}});
  auto r2 = build(2, b, {{lv(1), {{{}, {{0, 3, false}}}, {{}, {{1, 3, false}}}}}, {lv(2), {{{0, 1}, {}}}}});
  auto r3 = build(3, b, {{lv(2), {{{}, {{2, 15, false}}}}}});
  expect_ok("S10a ordre 2", r2, 3, 2);
  expect_ok("S10b ordre 3 meme banque", r3, 1, 1);
  line("S10c banque partagee (meme pointeur)", r2.value.populations().get() == r3.value.populations().get(),
       "oui", r2.value.populations().get() == r3.value.populations().get() ? "oui" : "non");
  expect_root("S10d root(99) -> ABSENT", r2.value, 99, lv(5), true, kFullCoverageAbsent);
  expect_cov_invalid("S10e cov(99) -> invalid", r2.value, 99, lv(5), true);
  expect_cov_invalid("S10f cov(ABSENT) -> invalid", r2.value, kFullCoverageAbsent, lv(5), true);
  expect_cov_invalid("S10g cov(0) coupe 2 fermee (non racine) -> invalid", r2.value, 0, lv(2), true);
  expect_cov("S10h cov(0) coupe 2 ouverte = {0,1}", r2.value, 0, lv(2), false, {0, 1});
  expect_cov("S10i cov(2) coupe 100 fermee = {0,1,2,3}", r2.value, 2, lv(100), true, {0, 1, 2, 3});
  expect_cov_invalid("S10j cov(0) coupe 0 fermee -> invalid (avant naissance)", r2.value, 0, lv(0), true);
  FullCoverageCertificate moved(std::move(r2.value));
  expect_root("S10k source deplacee : root -> ABSENT", r2.value, 0, lv(5), true, kFullCoverageAbsent);
  expect_cov_invalid("S10l source deplacee : cov -> invalid", r2.value, 0, lv(5), true);
  expect_cov("S10m cible deplacee : cov(2) = {0,1,2,3}", moved, 2, lv(5), true, {0, 1, 2, 3});
  FullCoverageCertificate empty;
  expect_root("S10n forest defaut : root -> ABSENT", empty, 0, lv(5), true, kFullCoverageAbsent);
  const auto rr = full_coverage_at(r3.value, 0, lv(1), true);
  line("S10o raison du refus lecteur (chaine)", true, "(observation)", rr.reason);
}

static void s11_recouvrement_et_dates() {
  std::puts("== S11 recouvrements entre racines, dates de contributions par segment");
  // Deux racines partageant le point 2 ; contribution datee sur 0 a t=3 ; fusion a t=5 ; contribution sur la fusion a t=5 ; continuation sur fusion a t=7
  auto b = bank({0, 1, 2, 3, 4, 5, 6}, {{{}, {0, 1, 2}}, {{}, {2, 3, 4}}, {{2}, {0, 5}}, {{}, {3, 6}}, {{}, {6}}});
  auto r = build(3, b, {{lv(1), {{{}, {{0, 7, false}}}, {{}, {{1, 7, false}}}}},
                        {lv(3), {{{0}, {{2, 2, false}}}}},
                        {lv(5), {{{0, 1}, {{3, 2, false}}}}},
                        {lv(7), {{{2}, {{4, 1, false}}}}}});
  expect_ok("S11a construit", r, 3, 5);
  const auto& f = r.value;
  expect_cov("S11b cov(0) coupe 1 fermee = {0,1,2}", f, 0, lv(1), true, {0, 1, 2});
  expect_cov("S11c cov(1) coupe 1 fermee = {2,3,4} (recouvrement conserve)", f, 1, lv(1), true, {2, 3, 4});
  expect_cov("S11d cov(0) coupe 3 ouverte = {0,1,2}", f, 0, lv(3), false, {0, 1, 2});
  expect_cov("S11e cov(0) coupe 3 fermee = {0,1,2,5}", f, 0, lv(3), true, {0, 1, 2, 5});
  expect_cov("S11f cov(1) coupe 3 fermee = {2,3,4} (pas de fuite)", f, 1, lv(3), true, {2, 3, 4});
  expect_cov("S11g cov(2) coupe 5 fermee = {0,1,2,3,4,5,6}", f, 2, lv(5), true, {0, 1, 2, 3, 4, 5, 6});
  expect_cov("S11h cov(2) coupe 7 ouverte = idem", f, 2, lv(7), false, {0, 1, 2, 3, 4, 5, 6});
  expect_cov("S11i cov(2) coupe 7 fermee = idem (6 deja present)", f, 2, lv(7), true, {0, 1, 2, 3, 4, 5, 6});
  line("S11j contributions triees par niveau", [&] {
    for (size_t i = 1; i < f.contributions().size(); ++i)
      if (compare_exact_level(f.contributions()[i - 1].level, f.contributions()[i].level) > 0) return false;
    return true; }(), "oui", "?");
  line("S11k contribution 2 (t=3) porte segment 0, contribution 3 (t=5) segment 2, 4 (t=7) segment 2",
       f.contributions()[2].segment == 0 && f.contributions()[3].segment == 2 && f.contributions()[4].segment == 2,
       "oui", id(f.contributions()[2].segment) + "," + id(f.contributions()[3].segment) + "," + id(f.contributions()[4].segment));
}

int main() {
  s01_niveaux_rationnels(); s02_trois_limbs(); s03_chaine_longue(); s04_multifusion_contributions();
  s05_continuation_puis_fusion(); s06_k1_et_kn(); s07_masque16(); s08_populations_dupliquees_et_permissivites();
  s09_entrees_invalides(); s10_lecteur_bords(); s11_recouvrement_et_dates();
  std::printf("\nchecks=%d diffs=%d\n", checks, diffs);
  return diffs ? 1 : 0;
}
