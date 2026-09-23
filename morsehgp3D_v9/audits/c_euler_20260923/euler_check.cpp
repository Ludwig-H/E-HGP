// Auditeur C (v9) — invariant d'Euler par ordre K sur le catalogue recoupe, hors produit.
// Pour K <= Kmax-2, tout point critique de d_K (distance au K-ieme voisin) est une boule
// admissible du catalogue (p <= K-1, q_min <= 4 => p+q_min <= Kmax+1). Morse-Euler : la
// somme des contributions vaut chi(R^3) = 1 (plus n a K = 1). Boule generique (coquille =
// support minimal certifie, u = q) : contribution (-1)^(q-m) * C(q-1, m-1), m = K-p, pour
// p < K <= p+q (indice et multiplicite de Reani-Bobrowski). Boule degeneree (u > q_min) :
// contribution 1 - chi_c(Lambda_m) (Euler a supports compacts), calculee en Python exact a
// partir du fichier des degenerees ; preuve sans position generale : note B
// CONTRELEC_EULER_PAR_NERF_20260923. Avec run_tower=false la positivite des supports
// reguliers n'est pas verifiee ici : elle l'est par les executions run_tower=true du meme
// code sur les memes entrees. Sortie : sommes generiques, condenses commutatifs du
// catalogue restreint a p+q_min <= R (protocole << Kmax+2 >>).
// Usage : euler_check <fichier.u32le> Kmax workers <sortie_degenerees.jsonl>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/chain/tower_chain.hpp"

using mhgp9::gen::Point3;
__extension__ typedef __int128 i128;

static std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

static std::string to_dec(i128 v) {
  if (v == 0) return "0";
  bool neg = v < 0;
  __extension__ typedef unsigned __int128 u128;
  u128 u = neg ? (u128)(-(v + 1)) + 1 : (u128)v;
  std::string s;
  while (u) { s.push_back(char('0' + int(u % 10))); u /= 10; }
  if (neg) s.push_back('-');
  return std::string(s.rbegin(), s.rend());
}

int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: euler_check file.u32le Kmax workers degenerate.jsonl\n"); return 2; }
  const auto pts = read_u32le(argv[1]);
  mhgp9::ChainOptions opt;
  opt.kmax = (unsigned)std::stoul(argv[2]);
  opt.workers = std::stoul(argv[3]);
  opt.run_tower = false;
  opt.keep_catalogue = true;
  const auto t0 = std::chrono::steady_clock::now();
  const auto res = mhgp9::run_tower_chain(pts, opt);
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  if (res.status != mhgp9::ChainStatus::kComplete) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", mhgp9::chain_status_name(res.status), res.reason.c_str());
    return 1;
  }
  const unsigned kmax = opt.kmax;
  std::vector<long long> generic_sum(kmax + 1, 0);
  std::vector<long long> degenerate_touch(kmax + 1, 0);
  std::uint64_t degenerate = 0, recount_mismatch = 0;
  // Condense commutatif du catalogue restreint a p + q_min <= R (R = 2..kmax+1) : permet le
  // protocole << Kmax+2 >> (le catalogue Kmax doit egaler la restriction du catalogue Kmax+2).
  std::vector<std::uint64_t> restricted_sum(kmax + 2, 0), restricted_count(kmax + 2, 0);
  auto mix = [](std::uint64_t h) {
    h ^= h >> 33; h *= 0xff51afd7ed558ccdull; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ull; h ^= h >> 33;
    return h;
  };
  std::FILE* deg = std::fopen(argv[4], "w");
  if (!deg) throw std::runtime_error("cannot open degenerate output");
  for (const auto& b : res.catalogue_balls) {
    const int p = b.n_interior, q = b.arity, u = b.n_shell;
    {
      std::uint64_t h = 1469598103934665603ull;
      const i128 parts[5] = {b.key.a, b.key.b[0], b.key.b[1], b.key.b[2], b.key.c};
      for (const i128 v : parts) {
        __extension__ typedef unsigned __int128 u128;
        const u128 w = (u128)v;
        for (int byte = 0; byte < 16; ++byte) { h ^= (std::uint64_t)((w >> (8 * byte)) & 0xffu); h *= 1099511628211ull; }
      }
      h ^= (std::uint64_t)p << 8 | (std::uint64_t)q << 16 | (std::uint64_t)u << 24;
      h *= 1099511628211ull;
      const std::uint64_t m = mix(h);
      for (unsigned r = (unsigned)(p + q); r <= kmax + 1; ++r) { restricted_sum[r] += m; ++restricted_count[r]; }
    }
    if (u == q) {
      // contribution (-1)^(q-m) C(q-1, m-1), m = k-p : une multifusion q3 a m=2 fusionne trois regions.
      static const int binom[4][4] = {{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 2, 1, 0}, {1, 3, 3, 1}};
      for (int k = p + 1; k <= p + q && k <= (int)kmax; ++k) {
        const int m = k - p;
        generic_sum[k] += (((q - m) % 2 == 0) ? 1 : -1) * binom[q - 1][m - 1];
      }
      continue;
    }
    ++degenerate;
    for (int k = p + 1; k <= p + u && k <= (int)kmax; ++k) ++degenerate_touch[k];
    // Recompte exact de la coquille et des interieurs par balayage du nuage d'entree.
    const i128 A = b.key.a, B0 = b.key.b[0], B1 = b.key.b[1], B2 = b.key.b[2], C = b.key.c;
    int inside = 0;
    std::string shell;
    int shell_n = 0;
    for (const auto& z : pts) {
      const i128 x = z.x, y = z.y, w = z.z;
      const i128 pw = A * (x * x + y * y + w * w) + B0 * x + B1 * y + B2 * w + C;
      if (pw < 0) ++inside;
      else if (pw == 0) {
        shell += (shell_n ? "," : "");
        shell += "[" + to_dec(x) + "," + to_dec(y) + "," + to_dec(w) + "]";
        ++shell_n;
      }
    }
    if (inside != p || shell_n != u) ++recount_mismatch;
    std::fprintf(deg, "{\"a\":\"%s\",\"b\":[\"%s\",\"%s\",\"%s\"],\"c\":\"%s\",\"p\":%d,\"q\":%d,\"u\":%d,\"p_recount\":%d,\"shell\":[%s]}\n",
                 to_dec(A).c_str(), to_dec(B0).c_str(), to_dec(B1).c_str(), to_dec(B2).c_str(), to_dec(C).c_str(), p, q, u,
                 inside, shell.c_str());
  }
  std::fclose(deg);
  std::printf("{\"status\":\"complete\",\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"wall_s\":%.3f,\"balls\":%zu,\"degenerate\":%llu,\"recount_mismatch\":%llu,\"generic_sum\":[",
              argv[1], pts.size(), kmax, wall, res.catalogue_balls.size(), (unsigned long long)degenerate,
              (unsigned long long)recount_mismatch);
  for (unsigned k = 1; k <= kmax; ++k) std::printf("%s%lld", k > 1 ? "," : "", generic_sum[k]);
  std::printf("],\"degenerate_touching_k\":[");
  for (unsigned k = 1; k <= kmax; ++k) std::printf("%s%lld", k > 1 ? "," : "", degenerate_touch[k]);
  std::printf("],\"restricted\":{");
  for (unsigned r = 2; r <= kmax + 1; ++r)
    std::printf("%s\"%u\":[%llu,\"%016llx\"]", r > 2 ? "," : "", r, (unsigned long long)restricted_count[r],
                (unsigned long long)restricted_sum[r]);
  std::printf("}}\n");
  return 0;
}
