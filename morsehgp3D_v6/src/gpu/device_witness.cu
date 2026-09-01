// MorseHGP3D v6 — TEMOIN DEVICE de la serie C (docs/GPU.md). Compile par nvcc
// seulement (option CMake MHGP6_ENABLE_CUDA, sm_120, -fmad=false) ; sans
// __CUDACC__, le programme rend 2 (refus : aucun device compile).
//
// SOCLE ARITHMETIQUE PARTIEL (REPONSE_AUDITEURS_MULTICPU § 5.4 et § 5.8) :
// ce temoin etablit que le toolchain device fournit une arithmetique large
// EXACTE — il ne compile ni BallKey::power, ni AxisBounds, ni la division
// plancher de C3 (ces ports arrivent avec leur propre temoin, decision wire
// C3 : division device vs minimisateurs precalcules hote). Deux lots,
// generes sur l'hote de facon deterministe, executes sur le device,
// rapatries et compares BIT A BIT a l'oracle hote (__int128 natif) :
//   1. DI128 (add, sub, mul_i64_i64, mul_di128_i64, cmp, shl1, div_by_4_exact
//      sur 4x) — port contractuel du lot 1 du temoin v5 (pin :
//      docs/PROVENANCE.md), temoin croise de l'arithmetique large ;
//   2. __int128 NATIF device (add, mul i64xi64, mulx, cmp, quotient/reste par
//      i64 non nul — la division exigee par § 5.4, pas seulement add/mul).
// Cas : 1 << 18 au total. Les 81 premiers recoivent la grille (a, b) 9x9,
// PUIS neuf substitutions s'appliquent : aux indices PAIRS (mode B) dont le
// b initial vaut INT64_MAX ou INT64_MIN — un damier dans ces deux colonnes —
// b est retire vers |b| <= 2^40 ; 72 couples de la grille initiale survivent
// tels quels. Ensuite 50 bords DI128 plantes pour x/y (0, ±1, ±max du
// mode), le reste tire (mt19937_64 graine fixe).
// Modes : A (|x| <= 2^62 - 1, b sur tout i64), B (|x| <= 2^78 - 1,
// |b| <= 2^40 — l'egalite ±2^40 est DANS le domaine, les cas plantes la
// contiennent) — toujours dans le contrat exact de dint.hpp, l'oracle hote
// __int128 ne deborde jamais (mulx <= 2^125 ; a*b <= 2^126 : dans i128).
// Preuve d'ecriture : les sorties device sont initialisees a une SENTINELLE
// (cmp hors domaine) televersee avant les kernels ; toute sentinelle
// survivante = case jamais ecrite = invariant viole (3). Attestation de
// branche : chaque sortie grave di_mulhi_branch() ; une branche inattendue
// (stub != portable, nvcc != intrinseque) est refusee (3).
// Codes : 0 conforme, 1 desaccord device/hote, 2 refus (pas de device,
// erreur CUDA), 3 plancher/invariant, 4 mutant tue
// Trois dents SEPAREES, chacune a 4 : witness-di128-lost-carry (l'addition
// device perd sa retenue basse->haute — verdict PAR PRIMITIVE et PAR CAS :
// desaccord de somme SSI retenue attendue, zero violation du ssi, toutes
// les autres primitives conformes) ; witness-skip-write (ecriture ArithOut
// sautee un cas sur 4096 — indices EXACTS exiges, natif complet, branche
// propre, puis oracles conformes sur les cases ecrites) ;
// witness-skip-native-write (meme dent cote NativeOut, arith complet). La
// contre-fixture COMPOSEE skip+carry est gravee a 1 : les oracles tranchent,
// jamais un 4 aveugle.
//
// STUB HOTE (tests/device_witness_stub.cpp + tests/cuda_stub.hpp,
// MHGP6_FAKE_DEVICE) : les kernels tournent en boucles hote sequentielles et
// di_mulhi_u64 prend le REPLI PORTABLE (attesté) — portes locales 0/4 sur la
// LOGIQUE du temoin avant toute session G4. Un run stub n'est JAMAIS un recu
// device (`device=stub-hote sm=0.0` l'affiche).
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../core/dint.hpp"
#include "../core/mutants.hpp"
#include "../core/types.hpp"

using namespace mhgp6;

namespace {

struct ArithCase {
  i64 a, b;
  DI128 x, y;
};
struct ArithOut {
  DI128 sum, dif, mul64, mulx, shl, div4;
  int cmp;
  int branch;  // di_mulhi_branch() vu par le code (device ou stub)
};
struct NativeOut {
  u64 sum_lo, sum_hi, mul_lo, mul_hi, mulx_lo, mulx_hi, quo_lo, quo_hi, rem_lo, rem_hi;
  int cmp;
};

// Sentinelle de non-ecriture : cmp est contraint a {-1, 0, 1} par les deux
// kernels ; cette valeur ne peut donc survivre qu'a une case jamais ecrite.
inline constexpr int kUnwrittenCmp = 0x5EED;

#if defined(__CUDACC__) || defined(MHGP6_FAKE_DEVICE)
#if !defined(MHGP6_FAKE_DEVICE)
// Lancement reel nvcc ; le stub fournit sa propre definition (boucles hote).
#define MHGP6_LAUNCH(kernel, blocks, threads, ...) kernel<<<(blocks), (threads)>>>(__VA_ARGS__)
#endif
#define CUDA_OK(call)                                                                                 \
  do {                                                                                                \
    cudaError_t e_ = (call);                                                                          \
    if (e_ != cudaSuccess) {                                                                          \
      std::fprintf(stderr, "REFUS cuda : %s (%s:%d)\n", cudaGetErrorString(e_), __FILE__, __LINE__); \
      return 2;                                                                                       \
    }                                                                                                 \
  } while (0)

__global__ void k_arith(const ArithCase* in, ArithOut* out, unsigned n, bool mut_lost_carry,
                        bool mut_skip_write) {
  const unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= n) return;
  if (mut_skip_write && (i % 4096u) == 7u) return;  // MUTANT : ecriture sautee, sentinelle laissee
  const ArithCase c = in[i];
  ArithOut o;
  o.sum = di_add(c.x, c.y);
  if (mut_lost_carry) o.sum.hi = c.x.hi + c.y.hi;  // retenue basse->haute perdue
  o.dif = di_sub(c.x, c.y);
  o.mul64 = di_mul_i64_i64(c.a, c.b);
  o.mulx = di_mul_di128_i64(c.x, c.b);
  o.shl = di_shl1(c.x);
  o.div4 = di_div_by_4_exact(di_shl1(di_shl1(c.x)));  // 4x/4 : precondition tenue par construction
  o.cmp = di_cmp(c.x, c.y);
  o.branch = di_mulhi_branch();
  out[i] = o;
}

__global__ void k_native(const ArithCase* in, NativeOut* out, unsigned n, bool mut_skip_write) {
  const unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= n) return;
  if (mut_skip_write && (i % 4096u) == 7u) return;  // MUTANT natif : ecriture sautee
  const ArithCase c = in[i];
  const i128 X = (i128)(((u128)c.x.hi << 64) | (u128)c.x.lo);
  const i128 Y = (i128)(((u128)c.y.hi << 64) | (u128)c.y.lo);
  NativeOut o;
  const i128 s = X + Y;
  const i128 m = (i128)c.a * (i128)c.b;
  const i128 mx = X * (i128)c.b;
  const i64 d = c.b != 0 ? c.b : 1;  // diviseur non nul ; |X| <= 2^78 : jamais MIN128/-1
  const i128 q = X / (i128)d;
  const i128 r = X % (i128)d;
  o.sum_lo = (u64)s;
  o.sum_hi = (u64)((u128)s >> 64);
  o.mul_lo = (u64)m;
  o.mul_hi = (u64)((u128)m >> 64);
  o.mulx_lo = (u64)mx;
  o.mulx_hi = (u64)((u128)mx >> 64);
  o.quo_lo = (u64)q;
  o.quo_hi = (u64)((u128)q >> 64);
  o.rem_lo = (u64)r;
  o.rem_hi = (u64)((u128)r >> 64);
  o.cmp = X < Y ? -1 : (X > Y ? 1 : 0);
  out[i] = o;
}

#endif

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_carry = MHGP6_MUTANT("witness-di128-lost-carry");
  const bool m_skip = MHGP6_MUTANT("witness-skip-write");
  const bool m_skip_nat = MHGP6_MUTANT("witness-skip-native-write");
#if !defined(__CUDACC__) && !defined(MHGP6_FAKE_DEVICE)
  (void)m_carry;
  (void)m_skip;
  (void)m_skip_nat;
  std::fprintf(stderr, "REFUS : temoin device compile sans nvcc (aucun device)\n");
  return 2;
#else
#if defined(MHGP6_FAKE_DEVICE)
  const int expect_branch = 2;  // stub : repli portable atteste
#else
  const int expect_branch = 1;  // nvcc : intrinseque __umul64hi atteste
#endif
  int ndev = 0;
  if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
    std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
    return 2;
  }
  cudaDeviceProp prop{};
  CUDA_OK(cudaGetDeviceProperties(&prop, 0));
  std::printf("device=%s sm=%d.%d\n", prop.name, prop.major, prop.minor);

  // ---- Generation deterministe des cas (voir l'en-tete pour la structure).
  std::mt19937_64 rng(0x5eed0d1a7ull);
  const unsigned n = 1u << 18;
  const unsigned k_grid = 81;             // grille (a, b) 9x9 de bords i64
  const unsigned k_di_borders = 50;       // bords DI128 plantes pour (x, y)
  const unsigned k_planted = k_grid + k_di_borders;
  std::vector<ArithCase> in(n);
  const i64 bords[] = {0, 1, -1, INT64_MAX, INT64_MIN, (i64)1 << 40, -((i64)1 << 40), 65535, -65535};
  // Bords DI128 par mode (deux complements construits par di_neg, tous dans
  // le contrat : mode A |x| <= 2^62 - 1, mode B |x| <= 2^78 - 1).
  const DI128 max_a = DI128{0x3fffffffffffffffull, 0};        // 2^62 - 1
  const DI128 max_b = DI128{~0ull, 0x3fffull};                // 2^78 - 1
  const DI128 borders_a[5] = {di_zero(), di_from_i64(1), di_from_i64(-1), max_a, di_neg(max_a)};
  const DI128 borders_b[5] = {di_zero(), di_from_i64(1), di_from_i64(-1), max_b, di_neg(max_b)};
  const auto rnd = [&](int bits) -> i64 { return (i64)(rng() % ((u64)1 << bits)) - ((i64)1 << (bits - 1)); };
  for (unsigned i = 0; i < n; ++i) {
    ArithCase c;
    if (i < k_grid) {
      c.a = bords[i / 9];
      c.b = bords[i % 9];
    } else if (i < k_planted) {
      c.a = bords[(i * 7) % 9];
      c.b = bords[(i * 5) % 9];
    } else {
      const int mode = (int)(rng() % 3);
      c.a = mode == 0 ? (i64)rng() : rnd(36);
      c.b = mode == 1 ? (i64)rng() : rnd(36);
    }
    const bool mode_a = (i & 1) != 0;
    if (i >= k_grid && i < k_planted) {
      const unsigned j = i - k_grid;
      const DI128* tab = mode_a ? borders_a : borders_b;
      c.x = tab[(j / 5) % 5];
      c.y = tab[j % 5];
    } else if (mode_a) {
      c.x = di_mul_i64_i64(rnd(31), rnd(31));
      c.y = di_mul_i64_i64(rnd(31), rnd(31));
    } else {
      c.x = di_mul_i64_i64(rnd(39), rnd(39));
      c.y = di_mul_i64_i64(rnd(39), rnd(39));
    }
    if (mode_a) {
      if (i >= k_planted && (rng() & 1)) c.b = (i64)rng();
    } else {
      // Mode B : |b| <= 2^40 obligatoire (contrat di_mul_di128_i64 et oracle).
      if (i >= k_planted) c.b = rnd(40);
      else if (c.b == INT64_MAX || c.b == INT64_MIN) c.b = rnd(40);
    }
    in[i] = c;
  }
  // Retenues attendues de di_add (comptees par l'hote AVANT tout kernel) : le
  // verdict du mutant s'y compare exactement.
  size_t expected_carries = 0;
  for (const ArithCase& c : in)
    if (c.x.lo + c.y.lo < c.x.lo) ++expected_carries;

  // ---- Execution device, sorties pre-remplies de la sentinelle.
  std::vector<ArithOut> out(n);
  std::vector<NativeOut> nat(n);
  for (unsigned i = 0; i < n; ++i) {
    out[i].cmp = kUnwrittenCmp;
    out[i].branch = 0;
    nat[i].cmp = kUnwrittenCmp;
  }
  {
    ArithCase* d_in = nullptr;
    ArithOut* d_out = nullptr;
    NativeOut* d_nat = nullptr;
    CUDA_OK(cudaMalloc(&d_in, n * sizeof(ArithCase)));
    CUDA_OK(cudaMalloc(&d_out, n * sizeof(ArithOut)));
    CUDA_OK(cudaMalloc(&d_nat, n * sizeof(NativeOut)));
    CUDA_OK(cudaMemcpy(d_in, in.data(), n * sizeof(ArithCase), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_out, out.data(), n * sizeof(ArithOut), cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_nat, nat.data(), n * sizeof(NativeOut), cudaMemcpyHostToDevice));
    MHGP6_LAUNCH(k_arith, (n + 255) / 256, 256, d_in, d_out, n, m_carry, m_skip);
    CUDA_OK(cudaGetLastError());
    MHGP6_LAUNCH(k_native, (n + 255) / 256, 256, d_in, d_nat, n, m_skip_nat);
    CUDA_OK(cudaGetLastError());
    CUDA_OK(cudaDeviceSynchronize());
    CUDA_OK(cudaMemcpy(out.data(), d_out, n * sizeof(ArithOut), cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(nat.data(), d_nat, n * sizeof(NativeOut), cudaMemcpyDeviceToHost));
    CUDA_OK(cudaFree(d_in));
    CUDA_OK(cudaFree(d_out));
    CUDA_OK(cudaFree(d_nat));
  }

  // ---- Preuve d'ecriture et attestation de branche AVANT tout verdict —
  // compteurs SEPARES (5e contre-lecture § 5.8 : une union arith+natif qui
  // rendrait 4 avant la branche et les oracles masquerait une perte native
  // aux memes indices, une mauvaise branche ou une divergence d'oracle ; la
  // contre-fixture double injection skip+carry doit rendre 1, jamais un 4
  // aveugle).
  unsigned unwritten_arith = 0, unwritten_native = 0, bad_branch_written = 0, skip_index_viol = 0,
           skip_nat_index_viol = 0;
  for (unsigned i = 0; i < n; ++i) {
    const bool a_unw = out[i].cmp == kUnwrittenCmp;
    const bool n_unw = nat[i].cmp == kUnwrittenCmp;
    if (a_unw) ++unwritten_arith;
    if (n_unw) ++unwritten_native;
    if (!a_unw && out[i].branch != expect_branch) ++bad_branch_written;
    // Ensemble EXACT des indices, PAR TABLEAU (les deux dents sont
    // separees : witness-skip-write = ArithOut, witness-skip-native-write =
    // NativeOut) : absence SSI le mutant du tableau est actif ET
    // i % 4096 == 7 ; sans mutant, aucune absence admissible.
    if (a_unw != (m_skip && (i % 4096u) == 7u)) ++skip_index_viol;
    if (n_unw != (m_skip_nat && (i % 4096u) == 7u)) ++skip_nat_index_viol;
  }
  std::printf("ecritures_arith=%u/%u ecritures_native=%u/%u mulhi_branche_attendue=%d hors_branche_ecrites=%u\n",
              n - unwritten_arith, n, n - unwritten_native, n, expect_branch, bad_branch_written);
  if (m_skip) {
    if (skip_index_viol != 0 || unwritten_native != 0 || bad_branch_written != 0) {
      std::printf("MUTANT NON TUE : motif d'absence inattendu (viol_indices=%u natif=%u branche=%u)\n",
                  skip_index_viol, unwritten_native, bad_branch_written);
      return 1;
    }
    // Indices exacts, natif complet, branche propre : les ORACLES complets
    // tranchent APRES (voir le verdict apres la boucle d'oracle).
  } else if (m_skip_nat) {
    if (skip_nat_index_viol != 0 || unwritten_arith != 0 || bad_branch_written != 0) {
      std::printf("MUTANT NON TUE : motif d'absence natif inattendu (viol_indices=%u arith=%u branche=%u)\n",
                  skip_nat_index_viol, unwritten_arith, bad_branch_written);
      return 1;
    }
  } else if (unwritten_arith || unwritten_native || bad_branch_written) {
    std::printf("INVARIANT VIOLE : cases non ecrites ou branche mulhi inattendue\n");
    return 3;
  }

  // ---- Oracle hote __int128, desaccords PAR PRIMITIVE.
  unsigned mism_sum = 0, mism_dif = 0, mism_mul64 = 0, mism_mulx = 0, mism_shl = 0, mism_div4 = 0,
           mism_cmp = 0;
  unsigned nat_sum = 0, nat_mul = 0, nat_mulx = 0, nat_divrem = 0, nat_cmp = 0;
  unsigned iff_violations = 0;  // sous mutant carry : desaccord de somme SSI retenue attendue, PAR CAS
  for (unsigned i = 0; i < n; ++i) {
    const ArithCase& c = in[i];
    const i128 X = di_to_i128(c.x), Y = di_to_i128(c.y);
    if (out[i].cmp != kUnwrittenCmp) {  // oracle DI sur les cases ECRITES (mutant skip : 64 absentes)
      const ArithOut& o = out[i];
      const bool sum_mism = di_to_i128(o.sum) != X + Y;
      if (sum_mism) ++mism_sum;
      if (m_carry && sum_mism != (c.x.lo + c.y.lo < c.x.lo)) ++iff_violations;
      if (di_to_i128(o.dif) != X - Y) ++mism_dif;
      if (di_to_i128(o.mul64) != (i128)c.a * (i128)c.b) ++mism_mul64;
      if (di_to_i128(o.mulx) != X * (i128)c.b) ++mism_mulx;
      if (di_to_i128(o.shl) != X * 2) ++mism_shl;
      if (di_to_i128(o.div4) != X) ++mism_div4;
      if (o.cmp != (X < Y ? -1 : X > Y ? 1 : 0)) ++mism_cmp;
    }
    if (nat[i].cmp != kUnwrittenCmp) {  // oracle natif sur les cases ECRITES (mutant skip-native : 64 absentes)
      const NativeOut& o = nat[i];
      const i128 s = X + Y, m = (i128)c.a * (i128)c.b, mx = X * (i128)c.b;
      const i64 d = c.b != 0 ? c.b : 1;
      const i128 q = X / (i128)d, r = X % (i128)d;
      if (o.sum_lo != (u64)s || o.sum_hi != (u64)((u128)s >> 64)) ++nat_sum;
      if (o.mul_lo != (u64)m || o.mul_hi != (u64)((u128)m >> 64)) ++nat_mul;
      if (o.mulx_lo != (u64)mx || o.mulx_hi != (u64)((u128)mx >> 64)) ++nat_mulx;
      if (o.quo_lo != (u64)q || o.quo_hi != (u64)((u128)q >> 64) || o.rem_lo != (u64)r ||
          o.rem_hi != (u64)((u128)r >> 64))
        ++nat_divrem;
      if (o.cmp != (X < Y ? -1 : X > Y ? 1 : 0)) ++nat_cmp;
    }
  }
  const unsigned mism_di = mism_sum + mism_dif + mism_mul64 + mism_mulx + mism_shl + mism_div4 + mism_cmp;
  const unsigned mism_native = nat_sum + nat_mul + nat_mulx + nat_divrem + nat_cmp;
  std::printf("arith cas=%u desaccords=%u\n", n, mism_di);
  std::printf("arith_detail sum=%u dif=%u mul64=%u mulx=%u shl=%u div4=%u cmp=%u retenues_attendues=%zu\n",
              mism_sum, mism_dif, mism_mul64, mism_mulx, mism_shl, mism_div4, mism_cmp, expected_carries);
  std::printf("arith_native cas=%u desaccords=%u (sum=%u mul=%u mulx=%u divrem=%u cmp=%u)\n", n,
              mism_native, nat_sum, nat_mul, nat_mulx, nat_divrem, nat_cmp);
  if (m_skip) {
    // Verdict du mutant sentinelle APRES les oracles : les cases ecrites
    // doivent etre toutes conformes (DI et natif) — une panne concomitante
    // (double injection comprise) rend 1, jamais un 4 par accident.
    if (mism_di == 0 && mism_native == 0) {
      std::printf("mutant witness-skip-write TUE : %u sentinelles aux indices exacts, natif complet, oracles conformes\n",
                  unwritten_arith);
      return 4;
    }
    std::printf("MUTANT NON TUE OU DIVERGENCE PARASITE (di=%u natif=%u)\n", mism_di, mism_native);
    return 1;
  }
  if (m_skip_nat) {
    // Dent SEPAREE du second tableau (8e contre-lecture, non bloquante mais
    // fermee tout de suite) : memes exigences que la dent arithmetique,
    // cote NativeOut — oracles conformes sur les cases ecrites avant le 4.
    if (mism_di == 0 && mism_native == 0) {
      std::printf("mutant witness-skip-native-write TUE : %u sentinelles natives aux indices exacts, arith complet, oracles conformes\n",
                  unwritten_native);
      return 4;
    }
    std::printf("MUTANT NON TUE OU DIVERGENCE PARASITE (di=%u natif=%u)\n", mism_di, mism_native);
    return 1;
  }
  if (m_carry) {
    // Verdict PAR PRIMITIVE (§ 5.8) : le mutant ne corrompt QUE la retenue de
    // l'addition — les desaccords de somme doivent EGALER exactement les
    // retenues attendues comptees par l'hote, toutes les autres primitives
    // (DI et natives) restant conformes. Un plancher nul est un 3 ; tout
    // autre motif de divergence est un 1, jamais un 4 par accident.
    if (expected_carries == 0) {
      std::printf("PLANCHER : aucun cas a retenue attendu\n");
      return 3;
    }
    const bool only_sum = mism_dif == 0 && mism_mul64 == 0 && mism_mulx == 0 && mism_shl == 0 &&
                          mism_div4 == 0 && mism_cmp == 0 && mism_native == 0;
    if (mism_sum == expected_carries && iff_violations == 0 && only_sum) {
      std::printf(
          "mutant witness-di128-lost-carry TUE : %u desaccords de somme = retenues attendues, ssi par cas (0 violation)\n",
          mism_sum);
      return 4;
    }
    std::printf("MUTANT NON TUE OU DIVERGENCE PARASITE\n");
    return 1;
  }
  if (mism_di || mism_native) {
    std::printf("DESACCORD device/hote\n");
    return 1;
  }
  std::printf("device_witness OK\n");
  return 0;
#endif
}
