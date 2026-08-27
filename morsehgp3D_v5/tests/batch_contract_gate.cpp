// MorseHGP3D v5 — porte du CONTRAT STRUCTUREL des lots q3/q4 (P1 audit d3144fb3) :
// validate_q3_batch_view / validate_q4_batch_view / validate_q4_results_view
// refusent (false + motif) tout lot malforme AVANT scan ou emission, et
// scan_*_host / emit_*_batch levent std::invalid_argument sur un tel lot —
// jamais un SEGV. Fixtures : lot tronque (SoA de tailles differentes),
// tranche d'ancre debordante, ancre de seed invalide, un candidat manquant,
// lentille hors sites / hors tranche, x_site hors tranche, skip hors tranche,
// emission hors seeds / d'un seed mort / hors tranche / non ordonnee /
// dupliquee, somme des etages != completions, et la LIMITE UINT32_MAX
// representee par une vue synthetique (aucune allocation geante). Limites
// positives : lot vide, lot minimal valide. Codes : 0, 1.
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/gpu/q3_lane_batched.hpp"
#include "../src/gpu/q4_lane_batched.hpp"

using namespace mhgp5;

namespace {
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}
void expect_reject3(const Q3BatchView& v, const char* what) {
  std::string why;
  const bool ok = validate_q3_batch_view(v, &why);
  expect(!ok && !why.empty(), what);
}
void expect_reject4(const Q4BatchView& v, const char* what) {
  std::string why;
  const bool ok = validate_q4_batch_view(v, &why);
  expect(!ok && !why.empty(), what);
}
Q3Batch minimal3() {
  Q3Batch b;
  for (int i = 0; i < 3; ++i) {
    b.u0.push_back(i); b.u1.push_back(i); b.u2.push_back(i); b.q.push_back(i);
  }
  b.anchors.push_back(Q3BatchAnchor{0, 3});
  Q3BatchSeed s{};
  s.seed.bound = 1.0;
  s.anchor = 0;
  b.seeds.push_back(s);
  b.emit_if_alive.push_back(BallCandidate{});
  return b;
}
Q4Batch minimal4() {
  Q4Batch b;
  for (int i = 0; i < 3; ++i) {
    b.u0.push_back(i); b.u1.push_back(i); b.u2.push_back(i); b.q.push_back(i);
    b.px.push_back(i); b.py.push_back(i); b.pz.push_back(i); b.pid.push_back((PointId)i);
  }
  b.lens_sites = {0, 1, 2};
  Q4BatchAnchor an;
  an.begin = 0; an.count = 3; an.lens_begin = 0; an.lens_count = 3;
  an.skip_a = 0; an.skip_b = 1;
  an.D2 = 1;
  b.anchors.push_back(an);
  Q4BatchSeed s{};
  s.anchor = 0; s.x_site = 2; s.jneg = 1;
  b.seeds.push_back(s);
  return b;
}
}  // namespace

int main() {
  // ---- q3.
  {
    Q3Batch b = minimal3();
    std::string why;
    expect(validate_q3_batch(b, &why), "q3 lot minimal valide");
    Q3Batch e;
    expect(validate_q3_batch(e, &why), "q3 lot vide valide");
  }
  { Q3Batch b = minimal3(); b.q.pop_back(); std::string w; expect(!validate_q3_batch(b, &w), "q3 SoA tronquee refusee"); }
  { Q3Batch b = minimal3(); b.anchors[0].count = 4; std::string w; expect(!validate_q3_batch(b, &w), "q3 tranche debordante refusee"); }
  { Q3Batch b = minimal3(); b.anchors[0].begin = 2; b.anchors[0].count = 2; std::string w; expect(!validate_q3_batch(b, &w), "q3 tranche (begin+count) debordante refusee"); }
  { Q3Batch b = minimal3(); b.seeds[0].anchor = 1; std::string w; expect(!validate_q3_batch(b, &w), "q3 ancre de seed invalide refusee"); }
  { Q3Batch b = minimal3(); b.emit_if_alive.clear(); std::string w; expect(!validate_q3_batch(b, &w), "q3 candidat manquant refuse"); }
  {
    // Limite UINT32_MAX par vue synthetique : 2^32 sites annonces, aucune allocation.
    Q3BatchView v;
    v.n_sites = (size_t)UINT32_MAX + 1;
    v.n_u1 = v.n_u2 = v.n_q = v.n_sites;
    expect_reject3(v, "q3 2^32 sites refuses (vue synthetique)");
    Q3BatchAnchor an{UINT32_MAX, 1};
    Q3BatchView w;
    w.n_sites = (size_t)UINT32_MAX;
    w.n_u1 = w.n_u2 = w.n_q = w.n_sites;
    w.n_anchors = 1; w.anchors = &an;
    expect_reject3(w, "q3 begin + count = 2^32 refuse sans debordement");
  }
  {
    Q3Batch b = minimal3();
    b.anchors[0].count = 9;
    bool thrown = false;
    try { scan_q3_batch_host(&b, 4, false); } catch (const std::invalid_argument&) { thrown = true; }
    expect(thrown, "q3 scan hote leve invalid_argument sur un lot malforme");
    Q3Batch c = minimal3();
    thrown = false;
    std::vector<BallCandidate> lo; GenerateStats ls;
    try { emit_q3_batch(c, &lo, &ls); } catch (const std::invalid_argument&) { thrown = true; }
    expect(thrown, "q3 emission sans verdict leve invalid_argument");
  }
  // ---- q4.
  {
    Q4Batch b = minimal4();
    std::string why;
    expect(validate_q4_batch(b, &why), "q4 lot minimal valide");
    Q4Batch e;
    expect(validate_q4_batch(e, &why), "q4 lot vide valide");
  }
  { Q4Batch b = minimal4(); b.pid.pop_back(); std::string w; expect(!validate_q4_batch(b, &w), "q4 SoA tronquee refusee"); }
  { Q4Batch b = minimal4(); b.lens_sites[1] = 7; std::string w; expect(!validate_q4_batch(b, &w), "q4 lentille hors sites refusee"); }
  { Q4Batch b = minimal4(); b.anchors[0].count = 2; b.lens_sites[2] = 2; std::string w; expect(!validate_q4_batch(b, &w), "q4 lentille hors tranche refusee"); }
  { Q4Batch b = minimal4(); b.anchors[0].lens_count = 4; std::string w; expect(!validate_q4_batch(b, &w), "q4 tranche de lentille debordante refusee"); }
  { Q4Batch b = minimal4(); b.seeds[0].x_site = 3; std::string w; expect(!validate_q4_batch(b, &w), "q4 x_site hors tranche refuse"); }
  { Q4Batch b = minimal4(); b.anchors[0].skip_a = 5; std::string w; expect(!validate_q4_batch(b, &w), "q4 skip_a hors tranche refuse"); }
  { Q4Batch b = minimal4(); b.seeds[0].anchor = 3; std::string w; expect(!validate_q4_batch(b, &w), "q4 ancre de seed invalide refusee"); }
  {
    Q4BatchView v;
    v.n_sites = (size_t)UINT32_MAX + 1;
    for (int i = 0; i < 7; ++i) v.soa_sizes[i] = v.n_sites;
    expect_reject4(v, "q4 2^32 sites refuses (vue synthetique)");
  }
  {
    // Resultats : emissions.
    Q4Batch b = minimal4();
    b.seeds[0].jneg = 0;
    b.verdicts.assign(1, Q4SeedVerdict{});
    b.stages.completions = 2; b.stages.emit = 2;
    b.emits = {Q4Emit{0, 0}, Q4Emit{0, 2}};
    std::string w;
    expect(validate_q4_results(b, &w), "q4 resultats valides");
    Q4Batch c = b; c.emits[1].seed = 1; expect(!validate_q4_results(c, &w), "q4 emission hors seeds refusee");
    Q4Batch d = b; d.verdicts[0].dead = 1; expect(!validate_q4_results(d, &w), "q4 emission d'un seed mort refusee");
    Q4Batch f = b; f.emits[1].y_site = 3; expect(!validate_q4_results(f, &w), "q4 y_site hors tranche refuse");
    Q4Batch g = b; g.emits = {Q4Emit{0, 2}, Q4Emit{0, 0}}; expect(!validate_q4_results(g, &w), "q4 emissions non ordonnees refusees");
    Q4Batch h = b; h.emits = {Q4Emit{0, 2}, Q4Emit{0, 2}}; expect(!validate_q4_results(h, &w), "q4 emissions dupliquees refusees");
    Q4Batch k = b; k.stages.completions = 3; expect(!validate_q4_results(k, &w), "q4 somme des etages != completions refusee");
    Q4Batch m = b; m.verdicts.clear(); expect(!validate_q4_results(m, &w), "q4 verdict manquant refuse");
    bool thrown = false;
    std::vector<BallCandidate> lo; GenerateStats ls;
    try { emit_q4_batch(g, &lo, &ls); } catch (const std::invalid_argument&) { thrown = true; }
    expect(thrown, "q4 emission non ordonnee leve invalid_argument");
    Q4Batch n = minimal4(); n.anchors[0].count = 9;
    thrown = false;
    try { scan_q4_batch_host(&n, 4, false, false, false); } catch (const std::invalid_argument&) { thrown = true; }
    expect(thrown, "q4 scan hote leve invalid_argument sur un lot malforme");
  }
  if (failures) {
    std::printf("DESACCORDS : %d\n", failures);
    return 1;
  }
  std::printf("batch_contract OK\n");
  return 0;
}
