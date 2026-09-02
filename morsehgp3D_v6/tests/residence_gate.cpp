// MorseHGP3D v6 — PORTE DE RESIDENCE des jalons memoire (palier P2, RECTIFIEE
// le 2 septembre 2026 apres les deux relectures adverses).
//
// CE QUE CETTE PORTE EST : une porte de RESIDENCE. Elle juge l'INSTRUMENTATION
// de la memoire (les six jalons `rss_mb` et les six jalons `residence_hwm_mb`)
// et la RESIDENCE de l'etage de census, pas l'objet calcule. Elle ne prouve
// AUCUNE correction : un run dont tous les digests seraient faux la passerait
// (`opt.digest=false`). La correction est jugee ailleurs (mhgp6_conformity_*,
// mhgp6_prefix_*). Son label CTest est `gate;residence` — elle ne compte pas
// comme une porte de correction.
//
// CE QU'ELLE JUGE.
//   (0) SONDE D'INSTRUMENTATION AUTO-PORTEE, INDEPENDANTE DE L'ALLOCATEUR.
//       Apres le pipeline : mmap anonyme de X Mo, TOUTES les pages touchees,
//       puis munmap. Le noyau retire les pages du resident de facon
//       deterministe — aucun allocateur dans la boucle — tandis que VmHWM,
//       maximum HISTORIQUE, garde le pic. On exige alors `hwm - rss >= X` a la
//       tolerance pres, ce qui est EXACTEMENT la distinction pic/instantane.
//       CETTE SONDE EST LE TUEUR DE `hwm-instant-rss` : elle vaut sur toute
//       machine, alors que les criteres bases sur la retombee du RSS a la fin
//       du fold dependent de la politique de restitution de pages de la libc
//       (MESURE : `MALLOC_TRIM_THRESHOLD_` suffit a les faire mentir, et un
//       plancher d'ecart y rend 3 sur du code sain). Elle est valide dans les
//       deux regimes : apres le munmap, `hwm >= max(pic du pipeline, rss + X)`
//       et le resident est revenu a sa valeur d'avant la sonde, donc l'ecart
//       vaut au moins X quelle que soit la politique de trim.
//   (1) JALONS RELEVES. Chaque jalon releve porte un pic non nul (VmHWM = 0
//       signifie « pas de mesure », jamais « pas de memoire ») ET leur nombre
//       atteint `--min-jalons`. Sans ce compte la porte serait verte par
//       vacuite contre « un jalon a disparu » (mutant `drop-stage-milestone`).
//   (2) hwm_mb[j] >= rss_mb[j] - tolerance au MEME jalon (deux sources : les
//       compteurs de RSS par tache ne sont synchronises que par lots,
//       TASK_RSS_EVENTS_THRESH = 64 pages, soit 256 Kio par fil).
//   (3) hwm_mb est non decroissant A LA MEME TOLERANCE — et NON strictement.
//       CONTRE-MESURE du 2 septembre, qui refute la lecture « meme champ
//       noyau, donc monotone par construction » : /proc/pid/status publie
//       `VmHWM = max(mm->hiwater_rss, get_mm_rss(mm))`, et `mm->hiwater_rss`
//       n'est rafraichi qu'a certains points (chemins de demappage). Une
//       lecture prise pendant que le resident DEPASSE le hiwater enregistre
//       sur-rapporte, et la lecture suivante, apres la retombee du resident,
//       redescend au hiwater enregistre. MESURE (sonde dediee mmap/touch/
//       munmap, 40 paliers) : baisses de VmHWM jusqu'a 0,18 Mo ; MESURE sur le
//       pipeline (uniform n=2000, smax=6, 4 fils) : 0,758 Mo entre max_fold et
//       fin. Exiger la monotonie stricte rendrait donc 3 sur du code SAIN.
//       Une baisse FRANCHE (au-dela de la tolerance) reste une faute.
//   (4) hwm_mb[fin] >= max_j rss_mb[j] - tolerance (deux sources : tolerance).
//   (5) COHERENCE avec ru_maxrss — et NON une seconde source independante.
//       RECTIFICATION du 2 septembre : `VmHWM` (fs/proc/task_mmu.c) et
//       `ru_maxrss` (kernel/sys.c) lisent le MEME champ `mm->hiwater_rss` ;
//       leur egalite est une tautologie et n'est comptee dans AUCUNE propriete
//       jugee. Le controle reste comme garde du chemin de LECTURE (une ligne
//       VmHWM mal analysee s'y verrait), rien de plus.
//   (6) IDENTITE `census_balls == expand.survivors` : une BallData par
//       survivante. Le compteur `boules_census` fait double emploi avec
//       `survivantes=` deja publie ; l'identite le transforme en observable au
//       lieu d'un doublon (mutant `par-drop-ball-chunk` : elle tombe).
//   (7) PLAFOND DE COEXISTENCE DE LA FUSION DU CENSUS, en pour-cent de
//       `survivants x sizeof(BallData)` : `--max-coexistence-census-pct`.
//       C'est la garde du palier P3 (liberations par tranche). Le compteur
//       `expand.census_merge_peak_bytes` est DETERMINISTE — octets copies plus
//       octets encore detenus par les tranches non consommees, maximise sur les
//       pas de la fusion — donc fonction de l'entree et du nombre de tranches,
//       JAMAIS de l'allocateur. MESURE qui impose ce choix : un plafond sur le
//       RSS ne separe pas (increment de pic du census a n=2000/4 fils : 201 Mo
//       avec liberation contre 213 sans, pour une coexistence reelle qui passe
//       de ~1,5 a ~2,5 copies) — glibc ne rend une tranche liberee a l'OS que
//       si elle etait mmap'ee, ce qui n'arrive qu'au-dela du seuil dynamique
//       de mmap. Le RSS reste PUBLIE (`increment_census_mb`), jamais juge.
//       Un plafond va dans le SENS du chantier : il rougit quand la residence
//       MONTE, la ou un plancher rougirait quand elle baisse. Tueur de
//       `keep-ball-chunks`.
//   (8) PLANCHERS DE COUVERTURE, sur les seules quantites DETERMINISTES
//       (boules de census, boules a plateau, Sigma|parents|) : elles ne
//       dependent que de l'entree, jamais de l'allocateur ni de la machine.
//       AUCUN plancher memoire : dans un chantier qui abaisse le mur, un
//       plancher sur la memoire rougirait sur un palier reussi.
//
// PLANCHERS vs PLAFONDS sous `--inject` : les PLANCHERS de couverture ne sont
// jamais evalues sous injection (un mutant qui abaisse une cardinalite doit
// rendre 3 « survivant », pas 3 « plancher »). Les PLAFONDS et les invariants
// (0)-(7) le sont TOUJOURS : ce sont eux que les mutants visent.
//
// Codes : 0 conforme ; 2 refus (argument, refus du pipeline, ou instrumentation
// INDISPONIBLE) ; 3 plancher/invariant de residence viole (ou mutant injecte
// NON tue) ; 4 mutant injecte tue.
#include <sys/mman.h>
#include <sys/resource.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

namespace {

// Plafond de la reservation de la sonde : au-dela, refus (code 2) plutot
// qu'une porte qui ferait tomber la machine.
constexpr long long kSondePlafondMb = 8192;

// SONDE : mmap anonyme, toutes les pages touchees, puis munmap.
// TAILLE ADAPTATIVE : `mb` + l'ecart pic/resident deja en place, pour que la
// reservation DEPASSE a coup sur le maximum historique laisse par le pipeline.
// Sans cela le critere serait satisfait par le seul pic residuel du fold et la
// sonde n'attesterait rien d'elle-meme. `mb_effectif` est rendu a l'appelant.
// Rend false si la reservation echoue ou depasse le plafond (refus code 2).
bool sonde_pic_instantane(long long mb, long long plafond_mb, long long* mb_effectif, double* hwm_avant,
                          double* rss_avant, double* hwm_apres, double* rss_apres) {
  *hwm_avant = run_detail::vm_hwm_mb_now();
  *rss_avant = run_detail::rss_mb_now();
  const double marge = *hwm_avant > *rss_avant ? *hwm_avant - *rss_avant : 0.0;
  const long long eff = mb + (long long)(marge + 1.0);
  *mb_effectif = eff;
  if (eff > plafond_mb) return false;
  const size_t bytes = (size_t)eff * 1024u * 1024u;
  void* p = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) return false;
  volatile unsigned char* q = (volatile unsigned char*)p;
  for (size_t off = 0; off < bytes; off += 4096) q[off] = (unsigned char)(off >> 12);
  if (::munmap(p, bytes) != 0) return false;
  *hwm_apres = run_detail::vm_hwm_mb_now();
  *rss_apres = run_detail::rss_mb_now();
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  long long n = 8000, threads = 4, seed = 3, smax = 11;
  long long min_census_balls = 0, min_plateau_balls = 0, min_sum_parents = 0;
  long long min_jalons = 0, max_coexistence_pct = -1, sonde_mb = 256, tol_milli_mb = -1;
  std::string inject;
  bool ok = true;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    i64 v = 0;
    if (const char* s = val("--family=")) ok = parse_cloud_family(s, &family) && ok;
    else if (const char* s = val("--n=")) { ok = parse_i64_exact(s, &v) && v >= 2 && v <= 2147483647 && ok; n = v; }
    else if (const char* s = val("--threads=")) { ok = parse_i64_exact(s, &v) && v >= 1 && v <= 1024 && ok; threads = v; }
    else if (const char* s = val("--seed=")) { ok = parse_i64_exact(s, &v) && ok; seed = v; }
    else if (const char* s = val("--smax=")) { ok = parse_i64_exact(s, &v) && v >= 2 && v <= 11 && ok; smax = v; }
    else if (const char* s = val("--min-census-balls=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_census_balls = v; }
    else if (const char* s = val("--min-plateau-balls=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_plateau_balls = v; }
    else if (const char* s = val("--min-sum-parents=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_sum_parents = v; }
    else if (const char* s = val("--min-jalons=")) { ok = parse_i64_exact(s, &v) && v >= 0 && v <= 6 && ok; min_jalons = v; }
    else if (const char* s = val("--max-coexistence-census-pct=")) { ok = parse_i64_exact(s, &v) && v >= 0 && v <= 100000 && ok; max_coexistence_pct = v; }
    else if (const char* s = val("--sonde-mb=")) { ok = parse_i64_exact(s, &v) && v >= 16 && v <= 4096 && ok; sonde_mb = v; }
    else if (const char* s = val("--tolerance-milli-mb=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; tol_milli_mb = v; }
    else if (const char* s = val("--inject=")) inject = s;
    else { std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str()); ok = false; }
  }
  if (!ok) return 2;
  if (!inject.empty() && !mutants_enable(inject.c_str())) {
    std::fprintf(stderr, "mutant inconnu : %s\n", inject.c_str());
    return 2;
  }
  // Tolerance de VmHWM comme maximum historique : 1 Mo + 256 Kio par fil. DEUX
  // mecanismes MESURES, non supposes : (a) les compteurs de RSS par tache ne
  // sont synchronises que par lots de 64 pages (256 Kio par fil), d'ou un
  // retard sur le resident lu dans statm ; (b) VmHWM est publie comme
  // max(mm->hiwater_rss, resident courant) et `hiwater_rss` n'est rafraichi
  // qu'a certains points, d'ou des baisses observees jusqu'a 0,758 Mo.
  const double tol = tol_milli_mb >= 0 ? (double)tol_milli_mb / 1000.0 : 1.0 + 0.25 * (double)threads;
  // INSTRUMENTATION INDISPONIBLE = REFUS (code 2), jamais un plancher viole :
  // /proc/self/status sans VmHWM (hidepid, conteneur restreint, noyau sans le
  // champ) n'est pas une regression du pipeline.
  if (run_detail::vm_hwm_mb_now() <= 0.0 || run_detail::rss_mb_now() <= 0.0) {
    std::fprintf(stderr, "REFUS : instrumentation de residence indisponible "
                         "(/proc/self/status:VmHWM ou /proc/self/statm illisible)\n");
    return 2;
  }
  const int coord = cloud_family_default_coord(family, (int)n);
  const std::vector<InputPoint> in = make_family_input(family, (int)n, coord, seed);
  if (in.size() < 2) return 2;

  RunOptions opt;
  opt.s = 8;
  opt.smax = (u64)smax;
  opt.threads = (int)threads;
  opt.digest = false;  // porte de RESIDENCE : aucun digest juge ici
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    if (!inject.empty()) {
      std::fprintf(stderr, "mutant %s : statut %s — tue\n", inject.c_str(), rr.message.c_str());
      return 4;
    }
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  struct rusage ru;
  const long maxrss_kb = getrusage(RUSAGE_SELF, &ru) == 0 ? ru.ru_maxrss : 0;
  const double maxrss_mb = (double)maxrss_kb / 1024.0;

  u64 viol = 0, juges = 0;
  double max_rss = 0.0, prev_hwm = 0.0, retard_max = 0.0;
  std::printf("residence_jalons famille=%s n=%lld smax=%lld threads=%lld tolerance_mb=%.3f\n",
              cloud_family_name(family), n, smax, threads, tol);
  for (int j = 0; j < 6; ++j) {
    const double r = rr.rss_mb[j], h = rr.hwm_mb[j];
    max_rss = std::max(max_rss, r);
    // INCREMENT : seule quantite imputable a l'intervalle ]j-1, j]. hwm[j] est
    // un maximum HISTORIQUE ; hwm[j] - rss[j] serait un majorant global.
    const double inc = h > prev_hwm ? h - prev_hwm : 0.0;
    std::printf("  jalon %-16s rss_mb=%7.0f hwm_mb=%7.0f increment_mb=%7.0f\n",
                run_detail::kResidenceStageLabel[j], r, h, inc);
    if (r <= 0.0 && h <= 0.0) continue;  // jalon non releve (route serie C, ou mutant)
    ++juges;
    if (h <= 0.0) {
      ++viol;
      std::fprintf(stderr, "jalon %s : HWM nul alors que rss_mb=%.0f\n", run_detail::kResidenceStageLabel[j], r);
    }
    if (r > h) retard_max = std::max(retard_max, r - h);
    if (h + tol < r) {
      ++viol;
      std::fprintf(stderr,
                   "jalon %s : hwm_mb=%.3f < rss_mb=%.3f au-dela de la tolerance %.3f (inversion franche)\n",
                   run_detail::kResidenceStageLabel[j], h, r, tol);
    }
    // MONOTONIE A LA TOLERANCE : VmHWM = max(hiwater enregistre, resident
    // courant) et le hiwater n'est rafraichi qu'a certains points — deux
    // lectures successives peuvent decroitre de quelques centaines de kio
    // (MESURE : 0,758 Mo sur ce pipeline, 0,18 Mo en sonde dediee).
    if (h + tol < prev_hwm) {
      ++viol;
      std::fprintf(stderr, "jalon %s : hwm_mb=%.3f < jalon precedent %.3f (VmHWM est monotone)\n",
                   run_detail::kResidenceStageLabel[j], h, prev_hwm);
    }
    prev_hwm = std::max(prev_hwm, h);
  }
  if ((long long)juges < min_jalons) {
    ++viol;
    std::fprintf(stderr, "jalons juges=%llu < --min-jalons=%lld (un jalon a disparu de l'instrumentation)\n",
                 (unsigned long long)juges, min_jalons);
  }
  if (rr.hwm_mb[5] + tol < max_rss) {
    ++viol;
    std::fprintf(stderr, "hwm_mb[fin]=%.3f < max des rss_mb=%.3f (le pic historique domine tout instantane)\n",
                 rr.hwm_mb[5], max_rss);
  }
  // COHERENCE de LECTURE, jamais une seconde source : ru_maxrss et VmHWM lisent
  // le MEME champ mm->hiwater_rss (kernel/sys.c et fs/proc/task_mmu.c). Non
  // comptee dans les proprietes jugees ; elle ne verrait qu'une ligne VmHWM mal
  // analysee. ru_maxrss est lu APRES le retour du pipeline : il majore hwm[fin].
  if (maxrss_mb + 1.0 < rr.hwm_mb[5]) {
    ++viol;
    std::fprintf(stderr, "ru_maxrss=%.1f Mo < hwm_mb[fin]=%.0f Mo (le champ mm->hiwater_rss est lu de travers)\n",
                 maxrss_mb, rr.hwm_mb[5]);
  }
  // IDENTITE : une BallData par survivante (le compteur `boules_census` fait
  // sinon double emploi avec `survivantes=`, deja publie sur la ligne famille).
  if (rr.census_balls != rr.expand.survivors) {
    ++viol;
    std::fprintf(stderr, "boules_census=%llu != survivantes=%llu (une BallData par survivante)\n",
                 (unsigned long long)rr.census_balls, (unsigned long long)rr.expand.survivors);
  }
  // PLAFOND DE COEXISTENCE de la fusion du census (palier P3) — DETERMINISTE.
  const double inc_census = rr.hwm_mb[3] > rr.hwm_mb[2] ? rr.hwm_mb[3] - rr.hwm_mb[2] : 0.0;
  const u64 base_bytes = rr.expand.survivors * (u64)sizeof(BallData);
  const double coexist_pct =
      base_bytes ? 100.0 * (double)rr.expand.census_merge_peak_bytes / (double)base_bytes : 0.0;
  if (max_coexistence_pct >= 0 && coexist_pct > (double)max_coexistence_pct) {
    ++viol;
    std::fprintf(stderr,
                 "PLAFOND de residence viole : coexistence de la fusion du census = %.1f %% de "
                 "survivants x sizeof(BallData) (<= %lld %%) — la fusion reporte deux fois la residence "
                 "de l'etage\n",
                 coexist_pct, max_coexistence_pct);
  }
  // SONDE (0), APRES le pipeline pour ne pas polluer les jalons.
  double sh0 = 0, sr0 = 0, sh1 = 0, sr1 = 0;
  long long sonde_eff = 0;
  if (!sonde_pic_instantane(sonde_mb, kSondePlafondMb, &sonde_eff, &sh0, &sr0, &sh1, &sr1)) {
    std::fprintf(stderr, "REFUS : la sonde d'instrumentation n'a pas pu reserver %lld Mo (plafond %lld)\n",
                 sonde_eff, kSondePlafondMb);
    return 2;
  }
  const double sonde_ecart = sh1 - sr1;
  if (sonde_ecart + tol < (double)sonde_eff) {
    ++viol;
    std::fprintf(stderr,
                 "sonde d'instrumentation : apres munmap de %lld Mo, hwm=%.1f rss=%.1f ecart=%.1f < %lld — la "
                 "ligne hwm rend un INSTANTANE, pas un PIC\n",
                 sonde_eff, sh1, sr1, sonde_ecart, sonde_eff);
  }
  std::printf("residence_sonde mb_demande=%lld mb_effectif=%lld avant_hwm=%.1f avant_rss=%.1f apres_hwm=%.1f "
              "apres_rss=%.1f ecart=%.1f\n",
              sonde_mb, sonde_eff, sh0, sr0, sh1, sr1, sonde_ecart);
  const double plateau_pct = rr.census_balls ? 100.0 * (double)rr.plateau_balls / (double)rr.census_balls : 0.0;
  std::printf("residence_synthese jalons_juges=%llu coexistence_census_pct=%.1f coexistence_census_octets=%llu "
              "increment_census_mb=%.0f hwm_fin_mb=%.0f rss_fin_mb=%.0f max_rss_mb=%.0f ru_maxrss_mb=%.1f "
              "retard_vmhwm_max_mb=%.3f\n",
              (unsigned long long)juges, coexist_pct, (unsigned long long)rr.expand.census_merge_peak_bytes,
              inc_census, rr.hwm_mb[5], rr.rss_mb[5], max_rss, maxrss_mb, retard_max);
  std::printf("residence_compteurs boules_census=%llu boules_plateau=%llu plateau_pct=%.3f somme_parents_total=%llu\n",
              (unsigned long long)rr.census_balls, (unsigned long long)rr.plateau_balls, plateau_pct,
              (unsigned long long)rr.sum_parents_total);

  if (!inject.empty()) {
    if (viol) {
      std::fprintf(stderr, "mutant %s : %llu violation(s) de residence — tue\n", inject.c_str(),
                   (unsigned long long)viol);
      return 4;
    }
    std::fprintf(stderr, "mutant %s : AUCUNE violation — survivant\n", inject.c_str());
    return 3;
  }
  if (viol) return 3;
  // PLANCHERS DE COUVERTURE DETERMINISTES (jamais evalues sous --inject) :
  // fonctions de la seule entree, insensibles a l'allocateur et a la machine.
  if ((long long)rr.census_balls < min_census_balls || (long long)rr.plateau_balls < min_plateau_balls ||
      (long long)rr.sum_parents_total < min_sum_parents) {
    std::fprintf(stderr,
                 "plancher de couverture viole : boules_census=%llu (>=%lld) boules_plateau=%llu (>=%lld) "
                 "somme_parents=%llu (>=%lld)\n",
                 (unsigned long long)rr.census_balls, min_census_balls, (unsigned long long)rr.plateau_balls,
                 min_plateau_balls, (unsigned long long)rr.sum_parents_total, min_sum_parents);
    return 3;
  }
  std::printf("residence conforme : %llu jalon(s) juge(s), pic monotone, sonde d'instrumentation passee, "
              "coexistence du census a %.1f %% sous plafond\n",
              (unsigned long long)juges, coexist_pct);
  return 0;
}
