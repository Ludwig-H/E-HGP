// MorseHGP3D v6 — PORTE DE LA ROUTE PERMUTATION ET DES PILES HISSEES (palier
// P4 d'echelle, docs/ECHELLE.md § 6.4).
//
// CE QU'ELLE JUGE. Deux economies de residence qui, par construction, ne
// changent PAS l'objet — donc qu'aucun digest et aucune cardinalite ne peut
// juger :
//   (A) `parallel_stable_sort` trie les gros elements par une PERMUTATION
//       d'indices u32 appliquee par suivi de cycles, au lieu de materialiser
//       un double exact du tableau (theoreme de stabilite en tete de
//       src/parallel/sort.hpp) ;
//   (B) les deux passes de `census.hpp` recoivent une PILE DE DESCENTE hissee
//       au niveau de l'ouvrier au lieu d'en construire une par boule.
// Une economie de residence ne se prouve pas sur un RSS (la libc ne rend pas
// toujours la memoire au systeme) : la porte juge des GRANDEURS
// DETERMINISTES — octets reellement demandes a `operator new` pendant le tri,
// nombre de piles possedees pendant le census — et l'IDENTITE de la sortie.
//
// PROPRIETES JUGEES
//   (1) SCENE A CHARGE UTILE (`WideRec`, 144 octets comme `BallCandidate`,
//       cle u64 + etiquette distincte a cle egale) : la route permutation
//       rend EXACTEMENT `std::stable_sort`, etiquettes comprises, a 1, 2, 4
//       et 8 fils et a trois cardinaux qui encadrent les seuils du module.
//       C'est la SEULE scene ou la stabilite est observable : deux
//       `BallCandidate` equivalents pour `ball_candidate_less` sont identiques
//       champ par champ, donc les echanger n'ecrit rien de different.
//   (2) La ROUTE DIRECTE rend la meme suite sur la meme scene (les deux
//       routes ne peuvent pas diverger sans qu'une des deux soit fausse).
//   (3) SCENE REELLE (candidats du generateur sur uniform 400 et
//       eight_clusters 400, graine 3) : `sort_candidates` == `std::stable_sort`
//       octet pour octet, et la sortie ne depend pas du nombre de fils.
//   (4) OCTETS DU TRI, compteur DETERMINISTE (remplacement de `operator new`,
//       mesure a 1 fil) : la route permutation demande 8 n octets la ou la
//       route directe en demande sizeof(BallCandidate) * n. La porte exige un
//       PLAFOND `--max-octets-tri-par-element`.
//   (5) PILES POSSEDEES : `DepthStats::owned_stacks` doit valoir 0 aux deux
//       passes sur la voie produit (plafond `--max-piles-possedees`), et les
//       resultats du census avec pile hissee doivent etre IDENTIQUES a ceux
//       obtenus sans pile fournie (le hissage ne change pas l'objet).
//   (6) PLANCHERS DE NON-VACUITE, sans lesquels la stabilite serait verte par
//       vacuite : `--min-ex-aequo` (paires adjacentes de cle egale dans la
//       scene A), `--min-classes-ex-aequo` (classes d'equivalence de taille
//       >= 2), `--min-ex-aequo-reels` (candidats reels STRICTEMENT egaux deux
//       a deux — sans eux la scene reelle n'exerce aucune egalite),
//       `--min-candidats`, `--min-boules-census`.
//
// Codes : 0 conforme ; 1 desaccord ou mutant survivant ; 2 refus d'argument ;
//         3 plancher ou plafond viole ; 4 mutant tue.
//
// MUTANTS ET LEUR SIGNATURE PROPRE (aucune clause terminale « tue par
// n'importe quoi ») :
//   `perm-apply-scatter`     : (1) diverge — l'application prend le sens
//                              inverse, le tableau n'est plus trie ;
//   `perm-apply-partial`     : (1) diverge — la fermeture de chaque cycle est
//                              sautee, un element par cycle est perdu ;
//   `perm-tie-desc`          : (1) diverge sur les ETIQUETTES seules, la suite
//                              restant TRIEE pour `less` — signature exacte de
//                              l'instabilite, distincte des deux precedentes
//                              qui, elles, cassent l'ordre ;
//   `parallel-sort-unstable` : (2) diverge — la fusion de la route DIRECTE
//                              prend la droite d'abord sur les ex aequo. La
//                              route permutation y est IMMUNE par theoreme
//                              (son comparateur d'indices est un ordre TOTAL,
//                              il n'a plus d'ex aequo a echanger) : la porte
//                              l'exige, c'est la portee exacte du departage ;
//   `census-stack-per-ball`  : (5) `owned_stacks` cesse d'etre nul.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/candidates.hpp"
#include "../src/pipeline/digest.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp7;

// ---------------------------------------------------------------------------
// Compteur d'allocations : remplacement des `operator new`/`delete` globaux.
// Il ne compte que pendant les fenetres explicitement armees, et seules les
// allocations d'alignement PAR DEFAUT y passent (aucun type mesure ici n'est
// suralign : alignof(BallCandidate) == alignof(i128) == 16, l'alignement de
// `new` par defaut sur cette cible).
// ---------------------------------------------------------------------------
namespace {
std::atomic<bool> g_counting{false};
std::atomic<unsigned long long> g_alloc_count{0};
std::atomic<unsigned long long> g_alloc_bytes{0};

void counter_arm() {
  g_alloc_count.store(0, std::memory_order_relaxed);
  g_alloc_bytes.store(0, std::memory_order_relaxed);
  g_counting.store(true, std::memory_order_relaxed);
}
void counter_disarm() { g_counting.store(false, std::memory_order_relaxed); }
}  // namespace

// `[[gnu::noinline]]` : sans lui, GCC 13 inline le couple remplacement/`free`
// dans le meme contexte et emet -Wmismatched-new-delete (faux positif connu du
// remplacement des operateurs globaux). Rien d'autre n'en depend.
[[gnu::noinline]] void* operator new(std::size_t bytes) {
  if (g_counting.load(std::memory_order_relaxed)) {
    g_alloc_count.fetch_add(1, std::memory_order_relaxed);
    g_alloc_bytes.fetch_add((unsigned long long)bytes, std::memory_order_relaxed);
  }
  void* p = std::malloc(bytes != 0 ? bytes : 1);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { std::free(p); }

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

// ---------------------------------------------------------------------------
// (1)(2) Scene synthetique : meme taille d'element que `BallCandidate`, cle
// volontairement pauvre (beaucoup d'ex aequo) et etiquette TOUJOURS distincte.
// Sans cette etiquette, l'instabilite n'aurait rien a ecrire de different.
// ---------------------------------------------------------------------------
struct WideRec {
  u64 key = 0;
  u64 tag = 0;
  u64 pad[16] = {};
};
static_assert(sizeof(WideRec) > kPermutationSortMinElemBytes,
              "la scene doit passer par la route permutation");
static_assert(sizeof(WideRec) == 144, "meme taille compilee que BallCandidate");

bool wide_less(const WideRec& a, const WideRec& b) { return a.key < b.key; }
bool wide_same(const WideRec& a, const WideRec& b) { return a.key == b.key && a.tag == b.tag; }

std::vector<WideRec> make_wide(size_t n, u64 classes, u64 seed) {
  std::vector<WideRec> v(n);
  u64 x = seed * 6364136223846793005ull + 1442695040888963407ull;
  for (size_t i = 0; i < n; ++i) {
    x = x * 6364136223846793005ull + 1442695040888963407ull;
    v[i].key = (x >> 17) % classes;
    v[i].tag = (u64)i;  // rang d'origine : temoin de la stabilite
    v[i].pad[0] = x;
  }
  return v;
}

bool same_sequence(const std::vector<WideRec>& a, const std::vector<WideRec>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (!wide_same(a[i], b[i])) return false;
  return true;
}

bool is_sorted_by_key(const std::vector<WideRec>& v) {
  for (size_t i = 1; i < v.size(); ++i)
    if (v[i].key < v[i - 1].key) return false;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  // Planchers GRAVES a ~50 % des valeurs MESUREES le 2 septembre sur ce depot
  // (210882 / 30037 / 172199 / 22 / 168712).
  long long min_ex_aequo = 105000, min_classes = 15000, min_candidats = 86000;
  long long min_ex_aequo_reels = 10, min_boules_census = 84000;
  long long max_piles_possedees = 0, max_octets_tri_par_element = 16;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    i64 v = 0;
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return a.compare(0, l, prefix) == 0 ? a.c_str() + l : nullptr;
    };
    if (const char* s = val("--inject=")) {
      inject = s;
    } else if (const char* s = val("--min-ex-aequo=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      min_ex_aequo = (long long)v;
    } else if (const char* s = val("--min-classes-ex-aequo=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      min_classes = (long long)v;
    } else if (const char* s = val("--min-candidats=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      min_candidats = (long long)v;
    } else if (const char* s = val("--min-ex-aequo-reels=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      min_ex_aequo_reels = (long long)v;
    } else if (const char* s = val("--min-boules-census=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      min_boules_census = (long long)v;
    } else if (const char* s = val("--max-piles-possedees=")) {
      if (!parse_i64_exact(s, &v) || v < 0) return 2;
      max_piles_possedees = (long long)v;
    } else if (const char* s = val("--max-octets-tri-par-element=")) {
      if (!parse_i64_exact(s, &v) || v <= 0) return 2;
      max_octets_tri_par_element = (long long)v;
    } else {
      return 2;
    }
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_scatter = MHGP7_MUTANT("perm-apply-scatter");
  const bool m_partial = MHGP7_MUTANT("perm-apply-partial");
  const bool m_tie = MHGP7_MUTANT("perm-tie-desc");
  const bool m_unstable = MHGP7_MUTANT("parallel-sort-unstable");
  const bool m_stack = MHGP7_MUTANT("census-stack-per-ball");

  // Signatures propres (une par mutant).
  u64 sig_perm_divergences = 0;   // (1) route permutation != stable_sort
  u64 sig_perm_desordre = 0;      // (1) et la suite n'est meme plus triee
  u64 sig_perm_ex_aequo = 0;      // (1) triee mais etiquettes permutees
  u64 sig_direct_divergences = 0; // (2) route directe != stable_sort
  u64 sig_piles_possedees = 0;    // (5) piles possedees sur la voie produit

  // -------------------------------------------------------------------------
  // (1)(2)(6) Scene synthetique.
  // -------------------------------------------------------------------------
  u64 ex_aequo_pairs = 0, classes_multiples = 0, elements_juges = 0;
  // 1000 : sous kParallelSortMinElems, sequentiel pur des DEUX routes.
  // 40000 et 200000 : au-dessus, W >= 2 des 2 fils (kParallelSortMinSlice).
  for (const size_t n : {(size_t)1000, (size_t)40000, (size_t)200000}) {
    const std::vector<WideRec> base = make_wide(n, std::max<u64>(2, (u64)n / 8), 7 + (u64)n);
    std::vector<WideRec> ref = base;
    std::stable_sort(ref.begin(), ref.end(), wide_less);
    // Planchers d'ex aequo mesures sur la REFERENCE (donc sur l'objet juge).
    u64 run = 1;
    for (size_t i = 1; i < ref.size(); ++i) {
      if (ref[i].key == ref[i - 1].key) {
        ++ex_aequo_pairs;
        ++run;
      } else {
        if (run >= 2) ++classes_multiples;
        run = 1;
      }
    }
    if (run >= 2) ++classes_multiples;
    elements_juges += (u64)n;

    for (const int th : {1, 2, 4, 8}) {
      std::vector<WideRec> perm = base;
      parallel_stable_sort(perm.begin(), perm.end(), wide_less, th);
      if (!same_sequence(perm, ref)) {
        ++sig_perm_divergences;
        if (!is_sorted_by_key(perm)) ++sig_perm_desordre;
        else ++sig_perm_ex_aequo;
      }
      std::vector<WideRec> direct = base;
      stable_sort_direct_route(direct.begin(), direct.end(), wide_less, th);
      if (!same_sequence(direct, ref)) ++sig_direct_divergences;
    }
  }

  // -------------------------------------------------------------------------
  // Verdicts de mutants (chacun sur SA signature, avant tout autre travail
  // couteux). Un mutant qui survit sort en 1, jamais en 0.
  // -------------------------------------------------------------------------
  if (m_tie) {
    std::printf("perm_sort_gate mutant=perm-tie-desc divergences=%llu ex_aequo_permutes=%llu desordre=%llu\n",
                (unsigned long long)sig_perm_divergences, (unsigned long long)sig_perm_ex_aequo,
                (unsigned long long)sig_perm_desordre);
    // SIGNATURE EXACTE : la suite reste TRIEE (aucun desordre) et diverge par
    // les seules etiquettes — c'est l'instabilite, pas une faute d'ordre.
    if (sig_perm_ex_aequo > 0 && sig_perm_desordre == 0) return 4;
    std::printf("MUTANT SURVIVANT : aucune permutation d'ex aequo a ordre conserve\n");
    return 1;
  }
  if (m_unstable) {
    std::printf("perm_sort_gate mutant=parallel-sort-unstable route_directe=%llu route_permutation=%llu\n",
                (unsigned long long)sig_direct_divergences, (unsigned long long)sig_perm_divergences);
    // La route directe doit ceder ; la route permutation doit TENIR (son
    // comparateur d'indices est total : plus d'ex aequo a echanger).
    if (sig_direct_divergences > 0 && sig_perm_divergences == 0) return 4;
    std::printf("MUTANT SURVIVANT ou hors portee : route directe intacte, ou route permutation touchee\n");
    return 1;
  }

  if (!m_scatter && !m_partial) {
    expect(sig_perm_divergences == 0, "route permutation == std::stable_sort (etiquettes comprises, 1/2/4/8 fils)");
    expect(sig_direct_divergences == 0, "route directe == std::stable_sort sur la meme scene");
  }

  // -------------------------------------------------------------------------
  // (3)(4)(5) Scene reelle : candidats du generateur.
  // -------------------------------------------------------------------------
  const u64 smax = 11;
  u64 candidats = 0, ex_aequo_reels = 0, boules_census = 0;
  u64 piles_possedees = 0;
  unsigned long long octets_perm = 0, allocs_perm = 0, octets_direct = 0, allocs_direct = 0;
  unsigned long long octets_census = 0, allocs_census = 0;
  bool routes_identiques = true, fils_identiques = true, census_identique = true;
  u64 sig_digest_raw = 0, sig_digest_balls = 0;  // divergences des DEUX monnaies gravees
  // Monnaies GRAVEES de la scene reelle (pin du palier P4). `digest_raw` signe
  // le multiensemble TRIE avant RLE (donc l'ordre du tri lui-meme),
  // `digest_balls` le multiensemble unique apres RLE : les deux mutants
  // d'application de la permutation les font diverger TOUS LES DEUX.
  struct Grave {
    CloudFamily fam;
    const char* raw;
    const char* balls;
  };
  static const Grave kGraves[2] = {
      {CloudFamily::kUniform, "199123c37677ed095f0c603283216a4c382b4ffac8e30aac7ce61a8dd3b567d0",
       "2b50f878e2ddb641f07358dc82c9d52e9db7482ae55a94d0e033b13c71f1e6f4"},
      {CloudFamily::kEightClusters, "55ddbb66fdf161bc631eab2eb19d0848a806c9e2ffd91a259295fe67bc0e4752",
       "2be6cc9b76a4b205dcd6cc988d03e3a15fbd364dcd6da9afd0eacd659c472691"},
  };

  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const std::vector<InputPoint> in = make_family_input(fam, 400, cloud_family_default_coord(fam, 400), 3);
    const CloudIndex ix = build_cloud_index(in);
    GenerateOptions go;
    go.s = 8;
    go.smax = smax;
    go.threads = 4;
    std::vector<BallCandidate> base;
    GenerateStats gs;
    generate_candidates(ix, go, &base, &gs);
    if (gs.cap_refus != kCapRefusNone) {
      std::printf("REFUS : plafond de generation atteint sur la scene de la porte\n");
      return 2;
    }
    candidats += (u64)base.size();

    std::vector<BallCandidate> ref = base;
    std::stable_sort(ref.begin(), ref.end(), ball_candidate_less);
    for (size_t i = 1; i < ref.size(); ++i)
      if (!ball_candidate_less(ref[i - 1], ref[i]) && !ball_candidate_less(ref[i], ref[i - 1])) ++ex_aequo_reels;

    const auto identical = [](const std::vector<BallCandidate>& a, const std::vector<BallCandidate>& b) {
      if (a.size() != b.size()) return false;
      return std::memcmp(a.data(), b.data(), a.size() * sizeof(BallCandidate)) == 0;
    };

    // (4) OCTETS DEMANDES : route permutation contre route directe, a 1 fil
    // (donc sans allocation de fil : le compte est deterministe).
    {
      std::vector<BallCandidate> c = base;
      counter_arm();
      sort_candidates(&c, 1);
      counter_disarm();
      octets_perm += g_alloc_bytes.load();
      allocs_perm += g_alloc_count.load();
      routes_identiques = routes_identiques && identical(c, ref);
    }
    {
      std::vector<BallCandidate> c = base;
      counter_arm();
      stable_sort_direct_route(c.begin(), c.end(), ball_candidate_less, 1);
      counter_disarm();
      octets_direct += g_alloc_bytes.load();
      allocs_direct += g_alloc_count.load();
      routes_identiques = routes_identiques && identical(c, ref);
    }
    // Monnaies gravees : `digest_raw_candidates_v6` sur la suite TRIEE (elle
    // signe l'ordre rendu par la route permutation) et `digest_balls_v4` sur
    // la suite DEDOUBLONNEE. Les deux sont exigees : une application fausse
    // change l'ordre (premiere) et, par le RLE adjacent, le multiensemble
    // unique (seconde).
    {
      std::vector<BallCandidate> c = base;
      sort_candidates(&c, 1);
      const std::string raw = digest_raw_candidates_v6(c);
      deduplicate_candidates(&c);
      const std::string blz = digest_balls_v4(c);
      const Grave* g = nullptr;
      for (const Grave& gr : kGraves)
        if (gr.fam == fam) g = &gr;
      if (g == nullptr) return 2;
      if (raw != g->raw) ++sig_digest_raw;
      if (blz != g->balls) ++sig_digest_balls;
      std::printf("monnaie famille=%d digest_raw_candidates=%s digest_balls=%s\n", (int)fam, raw.c_str(),
                  blz.c_str());
    }

    // (3) Invariance par nombre de fils.
    for (const int th : {2, 4, 8}) {
      std::vector<BallCandidate> c = base;
      sort_candidates(&c, th);
      fils_identiques = fils_identiques && identical(c, ref);
    }

    // (5) Census : piles hissees (voie produit) contre piles possedees.
    std::vector<BallCandidate> cands = base;
    sort_candidates(&cands, 1);
    deduplicate_candidates(&cands);
    ExpandStats st;
    std::vector<Survivor> surv;
    std::vector<BallData> balls;
    counter_arm();
    prefilter_balls(ix, cands, smax, 1, &surv, &st);
    const PipelineStatus ps = census_balls(ix, cands, surv, smax, kBallShellMax, 1, &balls, &st);
    counter_disarm();
    octets_census += g_alloc_bytes.load();
    allocs_census += g_alloc_count.load();
    if (ps != PipelineStatus::kCompleteRegular) {
      std::printf("REFUS : census hors complete_regular sur la scene de la porte\n");
      return 2;
    }
    boules_census += (u64)balls.size();
    piles_possedees += st.depth.owned_stacks + st.census.owned_stacks;

    // Le hissage ne change pas l'objet : meme census, boule a boule, avec une
    // pile POSSEDEE (scratch nul) et avec la pile fournie par le census.
    std::vector<i32> a_in, a_sh, b_in, b_sh;
    std::vector<NodeRef> pile;
    for (size_t i = 0; i < surv.size() && census_identique; i += 97) {
      const BallCandidate& bc = cands[surv[i].idx];
      const size_t cap = (size_t)(smax - bc.arity);
      const CensusStatus sa = ball_census(ix, bc.key, cap, kBallShellMax, &a_in, &a_sh, nullptr, nullptr);
      const CensusStatus sb = ball_census(ix, bc.key, cap, kBallShellMax, &b_in, &b_sh, nullptr, &pile);
      census_identique = census_identique && sa == sb && a_in == b_in && a_sh == b_sh;
    }
  }

  if (m_scatter || m_partial) {
    std::printf("perm_sort_gate mutant=%s divergences=%llu desordre=%llu digest_raw=%llu digest_balls=%llu\n",
                m_scatter ? "perm-apply-scatter" : "perm-apply-partial",
                (unsigned long long)sig_perm_divergences, (unsigned long long)sig_perm_desordre,
                (unsigned long long)sig_digest_raw, (unsigned long long)sig_digest_balls);
    // Signature EXIGEE : l'ordre est casse sur la scene synthetique ET les
    // DEUX monnaies gravees de la scene reelle divergent.
    if (sig_perm_desordre > 0 && sig_digest_raw > 0 && sig_digest_balls > 0) return 4;
    std::printf("MUTANT SURVIVANT : signature incomplete (ordre, digest_raw et digest_balls exiges)\n");
    return 1;
  }
  expect(sig_digest_raw == 0, "scene reelle : digest_raw_candidates conforme aux monnaies gravees");
  expect(sig_digest_balls == 0, "scene reelle : digest_balls conforme aux monnaies gravees");

  if (m_stack) {
    std::printf(
        "perm_sort_gate mutant=census-stack-per-ball piles_possedees=%llu allocations_census=%llu "
        "octets_census=%llu\n",
        (unsigned long long)piles_possedees, allocs_census, octets_census);
    if (piles_possedees > (u64)max_piles_possedees) return 4;
    std::printf("MUTANT SURVIVANT : aucune pile possedee comptee\n");
    return 1;
  }

  expect(routes_identiques, "candidats reels : les deux routes rendent std::stable_sort octet pour octet");
  expect(fils_identiques, "candidats reels : sortie identique a 2, 4 et 8 fils");
  expect(census_identique, "census : pile hissee et pile possedee rendent le meme I_B / U_B");
  sig_piles_possedees = piles_possedees;
  expect(sig_piles_possedees == 0, "census : aucune pile possedee sur la voie produit");

  std::printf(
      "perm_sort_gate elements_synthetiques=%llu ex_aequo=%llu classes_ex_aequo=%llu candidats=%llu "
      "ex_aequo_reels=%llu boules_census=%llu\n",
      (unsigned long long)elements_juges, (unsigned long long)ex_aequo_pairs,
      (unsigned long long)classes_multiples, (unsigned long long)candidats,
      (unsigned long long)ex_aequo_reels, (unsigned long long)boules_census);
  const double perm_par_elem = candidats ? (double)octets_perm / (double)candidats : 0.0;
  const double direct_par_elem = candidats ? (double)octets_direct / (double)candidats : 0.0;
  std::printf(
      "tri_octets_demandes permutation=%llu (%.2f o/element, %llu allocations) directe=%llu (%.2f o/element, "
      "%llu allocations) sizeof_candidat=%zu\n",
      octets_perm, perm_par_elem, allocs_perm, octets_direct, direct_par_elem, allocs_direct,
      sizeof(BallCandidate));
  std::printf("census_octets_demandes=%llu allocations=%llu piles_possedees=%llu\n", octets_census, allocs_census,
              (unsigned long long)piles_possedees);

  // Planchers de non-vacuite, PUIS plafonds.
  if ((long long)ex_aequo_pairs < min_ex_aequo || (long long)classes_multiples < min_classes ||
      (long long)candidats < min_candidats || (long long)ex_aequo_reels < min_ex_aequo_reels ||
      (long long)boules_census < min_boules_census) {
    std::printf("PLANCHER : ex_aequo=%llu (>=%lld) classes=%llu (>=%lld) candidats=%llu (>=%lld) "
                "ex_aequo_reels=%llu (>=%lld) boules_census=%llu (>=%lld)\n",
                (unsigned long long)ex_aequo_pairs, min_ex_aequo, (unsigned long long)classes_multiples,
                min_classes, (unsigned long long)candidats, min_candidats,
                (unsigned long long)ex_aequo_reels, min_ex_aequo_reels, (unsigned long long)boules_census,
                min_boules_census);
    return 3;
  }
  if ((long long)piles_possedees > max_piles_possedees) {
    std::printf("PLAFOND : piles_possedees=%llu (<=%lld)\n", (unsigned long long)piles_possedees,
                max_piles_possedees);
    return 3;
  }
  if (perm_par_elem > (double)max_octets_tri_par_element) {
    std::printf("PLAFOND : octets de tri par element=%.2f (<=%lld) — la route permutation a ete perdue\n",
                perm_par_elem, max_octets_tri_par_element);
    return 3;
  }
  return failures ? 1 : 0;
}

