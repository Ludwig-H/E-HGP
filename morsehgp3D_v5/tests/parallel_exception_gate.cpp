// MorseHGP3D v5 — porte du CONTRAT D'EXCEPTION des primitives paralleles
// (P1 audit d3144fb3) : une exception levee dans un ouvrier est capturee,
// tous les fils sont joints, et elle est relancee dans le fil appelant — jamais
// std::terminate (un crash par signal est refuse par run_expect). Trois
// fixtures a quatre fils : parallel_items, parallel_ranges, et un executeur de
// lane q3 par lots qui leve au deuxieme lot (le vidage au seuil a lieu dans
// l'ouvrier, le vidage final dans le fil appelant : les deux chemins). Le
// message doit etre celui de la PREMIERE exception. Codes : 0, 1.
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q3_lane_batched.hpp"
#include "../src/parallel/pool.hpp"

using namespace mhgp5;

int main() {
  int failures = 0;
  const auto expect = [&](bool ok, const char* what) {
    if (!ok) { std::printf("ECHEC : %s\n", what); ++failures; } else std::printf("ok : %s\n", what); };
  {
    std::atomic<int> done{0};
    bool caught = false;
    try {
      parallel_items(1000, 4, [&](size_t i, size_t) {
        if (i == 137) throw std::runtime_error("items 137");
        ++done;
      });
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "items 137";
    }
    expect(caught, "parallel_items : exception relancee dans l'appelant avec son message");
    expect(done.load() < 1000, "parallel_items : l'arret des nouveaux travaux a ete demande");
  }
  {
    bool caught = false;
    try {
      parallel_ranges(100000, 4, [&](size_t b, size_t, size_t) {
        if (b >= 50000) throw std::logic_error("ranges");
      });
    } catch (const std::logic_error&) {
      caught = true;
    }
    expect(caught, "parallel_ranges : exception relancee dans l'appelant");
  }
  {
    // Lane q3 par lots a quatre fils, seuil 500 : l'executeur leve au 2e lot vu
    // par un ouvrier (vidage au seuil, dans l'ouvrier).
    const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kUniform, 400, 0, 3));
    GenerateOptions opt;
    opt.threads = 4;
    std::vector<BallCandidate> out;
    GenerateStats st;
    std::atomic<int> lots{0};
    bool caught = false;
    try {
      generate_q3_batched_with(ix, opt, &out, &st, [&](Q3Batch* b, u32 h3, bool ns) {
        if (++lots == 2) throw std::runtime_error("executeur lot 2");
        scan_q3_batch_host(b, h3, ns);
      }, 500);
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "executeur lot 2";
    }
    expect(caught, "lane q3 par lots : exception de l'executeur (vidage au seuil) relancee");
  }
  {
    // Vidage FINAL (fil appelant) : seuil enorme, l'executeur leve au premier lot.
    const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kUniform, 300, 0, 3));
    GenerateOptions opt;
    opt.threads = 4;
    std::vector<BallCandidate> out;
    GenerateStats st;
    bool caught = false;
    try {
      generate_q3_batched_with(ix, opt, &out, &st, [&](Q3Batch*, u32, bool) { throw std::runtime_error("vidage final"); },
                               (size_t)1 << 40);
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "vidage final";
    }
    expect(caught, "lane q3 par lots : exception au vidage final relancee");
  }
  if (failures) return 1;
  std::printf("parallel_exception OK\n");
  return 0;
}
