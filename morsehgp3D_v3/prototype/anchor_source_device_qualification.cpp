// MorseHGP3D v3 — QUALIFICATION HOTE/DEVICE DE LA SOURCE PAR ANCRE.
//
// L'hote et le device executent la MEME fonction `run_anchor_point`. Le
// differentiel exige donc davantage qu'un accord de cardinal : il compare la
// liste triee des cles de support avec leur census `(p, extra)`, ET les
// compteurs de travail, qui sont deterministes par point et donc independants
// du partitionnement. Une divergence de compilateur, d'arrondi entier ou de
// materiel se voit immediatement.
//
// Ce binaire ne qualifie NI le SLO `warm_e2e`, NI le payload officiel.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "prototype/anchor_device.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::MortonLbvh;
using namespace mhgp3v::anchor;

struct HostSink {
  std::vector<EmittedSupport>* out;
  bool store;
  void emit(const EmittedSupport& e) {
    if (store) out->push_back(e);
  }
};

struct ScratchOwner {
  std::vector<int> partners, kept, lens, site_id;
  std::vector<i64> site_d2, margin_g, margin_llow, margin_uhigh;
  std::vector<unsigned char> acute;
  Scratch view() {
    partners.resize(kPartnerCap);
    site_d2.resize(kSiteCap);
    site_id.resize(kSiteCap);
    margin_g.resize(kSiteCap);
    margin_llow.resize(kSiteCap);
    margin_uhigh.resize(kSiteCap);
    kept.resize(kKeptCap);
    lens.resize(kLensCap);
    acute.resize(kLensCap);
    Scratch sc{};
    sc.partners = partners.data();
    sc.site_d2 = site_d2.data();
    sc.site_id = site_id.data();
    sc.margin_g = margin_g.data();
    sc.margin_llow = margin_llow.data();
    sc.margin_uhigh = margin_uhigh.data();
    sc.kept = kept.data();
    sc.lens = lens.data();
    sc.acute = acute.data();
    return sc;
  }
};

void accumulate(WorkCounters* dst, const WorkCounters& w) {
  long long* d = reinterpret_cast<long long*>(dst);
  const long long* s = reinterpret_cast<const long long*>(&w);
  const int fields = (int)(sizeof(WorkCounters) / sizeof(long long));
  for (int i = 0; i < fields - 4; ++i) d[i] += s[i];
  for (int i = fields - 4; i < fields; ++i) d[i] = std::max(d[i], s[i]);
}

[[noreturn]] void refuse(const char* msg) {
  std::fprintf(stderr, "REFUS : %s\n", msg);
  std::exit(2);
}

bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return false;
  *out = v;
  return true;
}

bool less_support(const EmittedSupport& u, const EmittedSupport& v) {
  return u.key < v.key || (u.key == v.key && (u.p < v.p || (u.p == v.p && u.extra < v.extra)));
}

}  // namespace

int main(int argc, char** argv) {
  long long n = 2000, coord = -1, seed = 1, smax = 11, threads = 1, slots = 16384;
  CloudFamily family = CloudFamily::kUniform;
  bool store = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long parsed = 0;
    if (key == "--points") { if (!parse_ll(val.c_str(), &parsed)) refuse("--points invalide"); n = parsed; }
    else if (key == "--coord") { if (!parse_ll(val.c_str(), &parsed)) refuse("--coord invalide"); coord = parsed; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--seed invalide"); seed = parsed; }
    else if (key == "--smax") { if (!parse_ll(val.c_str(), &parsed)) refuse("--smax invalide"); smax = parsed; }
    else if (key == "--threads") { if (!parse_ll(val.c_str(), &parsed)) refuse("--threads invalide"); threads = parsed; }
    else if (key == "--slots") { if (!parse_ll(val.c_str(), &parsed)) refuse("--slots invalide"); slots = parsed; }
    else if (key == "--no-store") { store = false; }
    else if (key == "--family") {
      if (val == "uniform") family = CloudFamily::kUniform;
      else if (val == "terrain") family = CloudFamily::kTerrain;
      else if (val == "scanline_single_pass") family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else {
      refuse("argument inconnu");
    }
  }
  if (n < 2 || n > 65535) refuse("--points hors du profil dense u16 [2, 65535]");
  if (smax < 4 || smax > 24) refuse("--smax hors bornes");
  if (threads < 1 || threads > 256) refuse("--threads hors bornes");
  if (slots < 32 || slots > (1 << 20)) refuse("--slots hors bornes");
  if (coord < 0) coord = mhgp3v::cloud_family_default_coord(family, (int)n);

  const std::vector<P3> pts = mhgp3v::make_family_cloud(family, (int)n, (int)coord, seed);
  if ((long long)pts.size() != n) refuse("la famille n'a pas rendu le cardinal demande");

  MortonLbvh tree;
  tree.build(pts, 8);
  const int nodes = (int)tree.nodes.size();
  std::vector<int> lo(3 * (std::size_t)nodes), hi(3 * (std::size_t)nodes);
  std::vector<int> nb((std::size_t)nodes), ne((std::size_t)nodes);
  std::vector<int> nl((std::size_t)nodes), nr((std::size_t)nodes);
  for (int i = 0; i < nodes; ++i) {
    for (int d = 0; d < 3; ++d) {
      lo[3 * (std::size_t)i + (std::size_t)d] = (int)tree.nodes[(std::size_t)i].lo[d];
      hi[3 * (std::size_t)i + (std::size_t)d] = (int)tree.nodes[(std::size_t)i].hi[d];
    }
    nb[(std::size_t)i] = tree.nodes[(std::size_t)i].begin;
    ne[(std::size_t)i] = tree.nodes[(std::size_t)i].end;
    nl[(std::size_t)i] = tree.nodes[(std::size_t)i].left;
    nr[(std::size_t)i] = tree.nodes[(std::size_t)i].right;
  }
  std::vector<int> px((std::size_t)n), py((std::size_t)n), pz((std::size_t)n);
  for (long long i = 0; i < n; ++i) {
    px[(std::size_t)i] = (int)pts[(std::size_t)i].x;
    py[(std::size_t)i] = (int)pts[(std::size_t)i].y;
    pz[(std::size_t)i] = (int)pts[(std::size_t)i].z;
  }

  TreeView tv{};
  tv.lo = lo.data();
  tv.hi = hi.data();
  tv.begin = nb.data();
  tv.end = ne.data();
  tv.left = nl.data();
  tv.right = nr.data();
  tv.order = tree.order.data();
  tv.px = px.data();
  tv.py = py.data();
  tv.pz = pz.data();
  tv.nodes = nodes;
  tv.n = (int)n;

  // ---------------------------------------------------------------- hote
  WorkCounters host_ctr{};
  std::vector<EmittedSupport> host_out;
  unsigned host_flags = 0;
  const auto h0 = std::chrono::steady_clock::now();
  {
    const int nt = (int)threads;
    std::vector<WorkCounters> wc((std::size_t)nt);
    std::vector<std::vector<EmittedSupport>> outs((std::size_t)nt);
    std::vector<unsigned> fl((std::size_t)nt, 0u);
    std::vector<std::thread> pool;
    std::atomic<int> cursor{0};
    const int chunk = std::max(1, (int)n / (nt * 64) + 1);
    for (int t = 0; t < nt; ++t) {
      pool.emplace_back([&, t]() {
        ScratchOwner owner;
        Scratch sc = owner.view();
        HostSink sink{&outs[(std::size_t)t], store};
        for (;;) {
          const int a0 = cursor.fetch_add(chunk);
          if (a0 >= (int)n) break;
          const int a1 = std::min((int)n, a0 + chunk);
          for (int a = a0; a < a1; ++a)
            run_anchor_point(tv, a, (int)smax, sc, sink, &wc[(std::size_t)t],
                             &fl[(std::size_t)t]);
        }
      });
    }
    for (auto& th : pool) th.join();
    for (int t = 0; t < nt; ++t) {
      accumulate(&host_ctr, wc[(std::size_t)t]);
      host_flags |= fl[(std::size_t)t];
      host_out.insert(host_out.end(), outs[(std::size_t)t].begin(), outs[(std::size_t)t].end());
    }
  }
  const double host_ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - h0).count();
  if (host_flags != 0) {
    std::fprintf(stderr, "REFUS : capacite hote depassee, drapeaux=%u\n", host_flags);
    return 3;
  }

  // -------------------------------------------------------------- device
  const unsigned long long emitted_host =
      (unsigned long long)(host_ctr.supports_q2 + host_ctr.supports_q3 + host_ctr.supports_q4);
  const unsigned long long capacity = emitted_host + emitted_host / 8 + 1024;
  DeviceRun dev = run_device(lo, hi, nb, ne, nl, nr, tree.order, px, py, pz, (int)smax, store,
                             capacity, (int)slots);

  std::printf("AnchorDeviceReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cuda_g4_diagnostic"
              " profile=quantized_u16_input_only mode=proposition_math_non_recue"
              " public_status=not_claimed\n");
  std::printf("cloud family=%s n=%lld coord=%lld seed=%lld smax=%lld\n",
              mhgp3v::cloud_family_name(family), n, coord, seed, smax);
  std::printf("caps partners=%d sites=%d kept=%d lens=%d slots=%d\n", kPartnerCap, kSiteCap,
              kKeptCap, kLensCap, dev.slots);
  std::printf("memoire scratch_octets=%llu sortie_octets=%llu\n", dev.scratch_bytes,
              dev.output_bytes);
  std::printf("hote threads=%lld wall_ms=%.3f supports=%llu\n", threads, host_ms, emitted_host);
  std::printf("device kernel_ms=%.3f supports=%llu drapeaux=%u\n", dev.kernel_ms, dev.emitted,
              dev.flags);
  std::printf("high_water hote sites=%lld kept=%lld lens=%lld ; device sites=%lld kept=%lld"
              " lens=%lld\n",
              host_ctr.hw_sites, host_ctr.hw_kept, host_ctr.hw_lens, dev.counters.hw_sites,
              dev.counters.hw_kept, dev.counters.hw_lens);

  if (dev.flags != 0) {
    std::fprintf(stderr, "REFUS : capacite device depassee, drapeaux=%u\n", dev.flags);
    return 3;
  }

  // Les compteurs sont deterministes par point : ils doivent etre EGAUX.
  const long long* hc = reinterpret_cast<const long long*>(&host_ctr);
  const long long* dc = reinterpret_cast<const long long*>(&dev.counters);
  const int fields = (int)(sizeof(WorkCounters) / sizeof(long long));
  int mismatched = 0;
  for (int i = 0; i < fields; ++i)
    if (hc[i] != dc[i]) {
      std::fprintf(stderr, "DESACCORD compteur %d : hote=%lld device=%lld\n", i, hc[i], dc[i]);
      ++mismatched;
    }
  std::printf("compteurs champs=%d desaccords=%d\n", fields, mismatched);

  int support_mismatch = 0;
  if (store) {
    std::sort(host_out.begin(), host_out.end(), less_support);
    std::sort(dev.supports.begin(), dev.supports.end(), less_support);
    if (host_out.size() != dev.supports.size()) {
      support_mismatch = 1;
    } else {
      for (std::size_t i = 0; i < host_out.size(); ++i)
        if (host_out[i].key != dev.supports[i].key || host_out[i].p != dev.supports[i].p ||
            host_out[i].extra != dev.supports[i].extra) {
          support_mismatch = 1;
          break;
        }
    }
    long long duplicates = 0;
    for (std::size_t i = 1; i < host_out.size(); ++i)
      if (host_out[i - 1].key == host_out[i].key) ++duplicates;
    std::printf("identite occurrences=%zu doublons=%lld\n", host_out.size(), duplicates);
    if (duplicates != 0) {
      std::fprintf(stderr, "REFUS : l'emission exacte-une-fois est violee\n");
      return 3;
    }
  }
  std::printf("parite supports=%s compteurs=%s\n", support_mismatch ? "NON" : "OUI",
              mismatched ? "NON" : "OUI");
  if (support_mismatch != 0 || mismatched != 0) {
    std::fprintf(stderr, "DESACCORD : le device ne reproduit pas l'hote\n");
    return 1;
  }
  return 0;
}
