// Entrees contractuellement impossibles (LOCAL_DIAGNOSTICS §1, COVERAGE_THRESHOLDS) : acceptees par le format ?
#include <cstdio>
#include <string>
#include "src/forest/full_coverage_certificate.hpp"
using namespace mhgp7;
static ExactLevel lv(u64 n, i128 d = 1) { return {{n, 0, 0}, d}; }
static const char* st(FullCertificateStatus s) { return s == FullCertificateStatus::kOk ? "ok" : s == FullCertificateStatus::kInvalidInput ? "invalid" : "exhausted"; }
static void show(const char* name, const FullCoverageBuildResult& r) {
  std::printf("%-78s -> %s:%s nodes=%zu contribs=%zu\n", name, st(r.status), r.reason, r.value.nodes().size(), r.value.contributions().size());
}
static std::string cov(const FullCoverageCertificate& f, FullNodeId root, ExactLevel cut, bool closed) {
  auto r = full_coverage_at(f, root, cut, closed); std::string s = std::string(st(r.status)) + " {";
  for (size_t i = 0; i < r.values.size(); ++i) { if (i) s += ","; s += std::to_string(r.values[i]); } return s + "}";
}
int main() {
  // P1 : contribution hors naissance a l'ordre K=3 depuis une population de cardinal 2 (aucun bloc a K>|S|)
  auto b1 = build_full_coverage_populations(std::vector<PointId>{0, 1, 2, 3}, std::vector<FullCoveragePopulation>{{{}, {0, 1, 2}}, {{}, {2, 3}}}).value;
  auto r1 = build_full_coverage_certificate(3, b1, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{1, 2, false}}}}}});
  show("P1 K=3, continuation depuis |S|=2 (ligne {2,3}, masque bit1=3)", r1);
  if (r1.status == FullCertificateStatus::kOk) std::printf("   cov(0) coupe 2 fermee = %s\n", cov(r1.value, 0, lv(2), true).c_str());
  // P2 : fusion a K=3 avec contribution depuis |S|=2
  auto b2 = build_full_coverage_populations(std::vector<PointId>{0, 1, 2, 3, 4, 5}, std::vector<FullCoveragePopulation>{{{}, {0, 1, 2}}, {{}, {3, 4, 5}}, {{}, {2, 3}}}).value;
  auto r2 = build_full_coverage_certificate(3, b2, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}, {{}, {{1, 7, false}}}}}, {lv(2), {{{0, 1}, {{2, 3, false}}}}}});
  show("P2 K=3, multifusion avec contribution depuis |S|=2", r2);
  // P3 : include_interior=true sur une continuation (I doit etre dans Q_B des qu'une classe stricte existe)
  auto b3 = build_full_coverage_populations(std::vector<PointId>{0, 1, 2, 3}, std::vector<FullCoveragePopulation>{{{}, {0, 1, 2}}, {{3}, {0, 1, 2}}}).value;
  auto r3 = build_full_coverage_certificate(3, b3, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{1, 0, true}}}}}});
  show("P3 K=3, continuation include_interior=true, masque 0", r3);
  if (r3.status == FullCertificateStatus::kOk) std::printf("   cov(0) coupe 2 fermee = %s\n", cov(r3.value, 0, lv(2), true).c_str());
  // P4 : deux naissances du meme ordre referencant la meme ligne (meme boule deux fois)
  auto r4 = build_full_coverage_certificate(3, b3, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}, {{}, {{0, 7, false}}}}}});
  show("P4 K=3, deux naissances meme niveau meme ligne", r4);
  auto r4b = build_full_coverage_certificate(3, b3, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{}, {{0, 7, false}}}}}});
  show("P4b K=3, deux naissances niveaux 1 et 2 meme ligne", r4b);
  // P5 : naissance avec coquille vide (aucune MEB de rayon > 0 n'a une coquille vide)
  auto b5 = build_full_coverage_populations(std::vector<PointId>{0, 1, 2}, std::vector<FullCoveragePopulation>{{{0, 1, 2}, {}}}).value;
  auto r5 = build_full_coverage_certificate(2, b5, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 0, true}}}}}});
  show("P5 K=2, naissance U vide, I={0,1,2}", r5);
  // P6 : coquille a un seul point (rayon > 0 exige au moins deux points de support)
  auto b6 = build_full_coverage_populations(std::vector<PointId>{0, 1, 2}, std::vector<FullCoveragePopulation>{{{0, 1}, {2}}}).value;
  auto r6 = build_full_coverage_certificate(2, b6, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 1, true}}}}}});
  show("P6 K=2, naissance |U|=1", r6);
  // P7 : contribution d'une continuation deja entierement couverte ET une deuxieme contribution identique (idempotence)
  auto r7 = build_full_coverage_certificate(3, b3, std::vector<FullCoverageBatch>{{lv(1), {{{}, {{0, 7, false}}}}}, {lv(2), {{{0}, {{0, 7, false}, {0, 7, false}, {1, 4, false}}}}}});
  show("P7 K=3, trois contributions dont deux identiques", r7);
  if (r7.status == FullCertificateStatus::kOk) std::printf("   cov(0) coupe 2 fermee = %s ; contribs=%zu\n", cov(r7.value, 0, lv(2), true).c_str(), r7.value.contributions().size());
  // P8 : lecture avec coupe dont le numerateur a trois limbs et den = 1 alors que les niveaux sont petits
  std::printf("P8 root(0) coupe 2^190/1 fermee = %llu (attendu 0)\n", (unsigned long long)full_coverage_root_at(r7.value, 0, ExactLevel{{0, 0, u64{1} << 62}, 1}, true));
  return 0;
}
