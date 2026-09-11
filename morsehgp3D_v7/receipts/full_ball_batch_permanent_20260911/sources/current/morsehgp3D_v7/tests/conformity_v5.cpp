// MorseHGP3D v6 — porte de conformite differentielle v5 ≡ v6.
//
// Execute le pipeline v6 complet sur une famille et compare les digests
// canoniques (format mhgp4-digest-v1) aux valeurs calculees par la v5 au pin
// 3bad233d : digest_balls (le multiensemble post-RLE est identique par
// construction en v6-J2, voir generate.hpp), les digest_forest_K* et
// digest_all (l'objet). La reference est soit un fichier de recu v5
// (--expected=chemin, lignes `digest_...=<hex>`), soit la table gravee
// ci-dessous pour les petites tailles.
//
// MODE PREFIXE (--prefix --smax=S, S < 11) : la tour a smax=S doit etre le
// PREFIXE EXACT de la tour a smax=11 — memes digest_forest_K et memes
// cardinalites pour K = 1..S-1. La reference gravee porte les dix ordres ; le
// prefixe en lit les premiers, et l'on EXIGE qu'elle en porte strictement plus
// que kmax_eff (sans quoi ce ne serait pas la lecture d'un prefixe). digest_all
// n'est PAS compare : il chaine un nombre different de forets, l'egaler serait
// faux. Trois observables supplementaires vivent dans ce mode :
//   (i)  batch_levels.size() == batches sur CHAQUE ForestResult recu — un
//        invariant de lot, verifie au callback (mutant prefix-tamper-batch-levels,
//        qui altere r.batch_levels APRES le digest : aucun digest ne le voit) ;
//   (ii) --prefix-witness : multiensemble CANONIQUE des evenements de l'ordre K
//        (evenements TRIES, interieurs compris) compare a celui du meme K dans
//        un second run smax=11 execute dans le MEME processus. L'ordre brut des
//        evenements depend du decoupage en tranches, qui differe entre les deux
//        smax : seul le multiensemble trie est comparable. C'est le seul
//        observable qui voit prefix-tamper-event-order (l'echange interior[0] /
//        interior[1] est invisible aux facettes, qui sont des ensembles tries) ;
//   (iii) planchers --min-orders / --min-deltas / --min-facets / --min-events
//        contre le vert par vacuite.
//
// Codes : 0 conforme ; 1 desaccord de digest (juge) ; 2 refus avant calcul ;
// 3 mutant injecte non tue, ou plancher de couverture viole ;
// 4 mutant injecte tue (divergence detectee).
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {

struct Expected {
  std::string balls, all;
  std::map<u64, std::string> forest;
  std::map<u64, KCardinalities> cards;
};

// Multiensemble CANONIQUE des evenements d'un ordre : copie POD triee par
// l'ordre de REPRESENTATION (q, d, masque, support, interieur, niveau). Les
// interieurs y entrent : c'est ce qui rend prefix-tamper-event-order visible.
struct EventKey {
  u8 q = 0, d = 0;
  u16 mask = 0;
  PointId support[11] = {};
  PointId interior[9] = {};
  ExactLevel level{};
};

inline bool event_less(const EventKey& a, const EventKey& b) {
  if (a.q != b.q) return a.q < b.q;
  if (a.d != b.d) return a.d < b.d;
  if (a.mask != b.mask) return a.mask < b.mask;
  for (int i = 0; i < 11; ++i)
    if (a.support[i] != b.support[i]) return a.support[i] < b.support[i];
  for (int i = 0; i < 9; ++i)
    if (a.interior[i] != b.interior[i]) return a.interior[i] < b.interior[i];
  if (a.level != b.level) return a.level < b.level;
  return false;
}

inline bool event_equal(const EventKey& a, const EventKey& b) {
  return !event_less(a, b) && !event_less(b, a);
}

inline std::vector<EventKey> canonical_events(const std::vector<ForestEvent>& ev) {
  std::vector<EventKey> out(ev.size());
  for (size_t i = 0; i < ev.size(); ++i) {
    out[i].q = ev[i].q;
    out[i].d = ev[i].d;
    out[i].mask = ev[i].active_mask;
    for (int t = 0; t < 11; ++t) out[i].support[t] = ev[i].support[t];
    for (int t = 0; t < 9; ++t) out[i].interior[t] = ev[i].interior[t];
    out[i].level = ev[i].level;
  }
  std::sort(out.begin(), out.end(), event_less);
  return out;
}

// Table gravee des petites tailles (calculee par la v5 au pin 3bad233d,
// binaire sha256 945c9a7f..., graine 3, s=8, smax=11, coord par defaut).
// Format : famille, n, digest_balls, digest_all (les forets sont couvertes
// par digest_all, qui les chaine).
struct SmallRef {
  const char* family;
  int n;
  const char* balls;
  const char* all;
};
inline constexpr SmallRef kSmallRefs[] = {
    // GRAVE_SMALL_REFS_ICI (rempli par tools : sortie v5 --digest)
    {nullptr, 0, nullptr, nullptr},
};

bool valid_hex64(const std::string& h) {
  if (h.size() != 64) return false;
  for (const char c : h)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

// CHARGEUR FAIL-CLOSED (audit du 31 aout) : exactement un digest_all, des
// hex minuscules de 64 caracteres, aucun doublon de foret, aucun K hors
// domaine. Un recu tronque ou altere n'est jamais accepte comme s'il avait
// compare chaque foret.
bool load_expected_file(const char* path, Expected* e) {
  std::ifstream f(path);
  if (!f) return false;
  std::string line;
  u64 n_all = 0;
  bool bad = false;
  while (std::getline(f, line)) {
    const auto take = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return line.compare(0, l, prefix) == 0 ? line.c_str() + l : nullptr;
    };
    if (const char* v = take("digest_balls=")) {
      e->balls = v;
      if (!valid_hex64(e->balls)) bad = true;
    } else if (const char* v = take("digest_all=")) {
      e->all = v;
      ++n_all;
      if (!valid_hex64(e->all)) bad = true;
    } else if (line.compare(0, 15, "digest_forest_K") == 0) {
      const size_t eq = line.find('=');
      if (eq == std::string::npos) {
        bad = true;
        continue;
      }
      i64 k = 0;
      if (!parse_i64_exact(line.substr(15, eq - 15).c_str(), &k) || k < 1 || k > 10) {
        bad = true;
        continue;
      }
      const std::string dg = line.substr(eq + 1);
      if (!valid_hex64(dg) || e->forest.count((u64)k)) {
        bad = true;
        continue;
      }
      e->forest[(u64)k] = dg;
    } else if (line.compare(0, 15, "cardinalites K=") == 0) {
      // `cardinalites K=<k> evenements=<u> facettes=<u> deltas=<u>
      //  attachements=<u> fusions=<u> noeuds=<u>` — SEPT champs, dans cet
      //  ordre, tous parses exactement. Un champ manquant, un nom inattendu,
      //  un K hors [1,10] ou un doublon = reference refusee (code 2), jamais
      //  une comparaison silencieusement plus courte.
      static constexpr const char* kFields[6] = {"evenements=", "facettes=", "deltas=",
                                                 "attachements=", "fusions=", "noeuds="};
      std::vector<std::string> tok;
      for (size_t b = 0; b < line.size();) {
        const size_t e2 = line.find(' ', b);
        const std::string t = line.substr(b, e2 == std::string::npos ? std::string::npos : e2 - b);
        if (!t.empty()) tok.push_back(t);
        if (e2 == std::string::npos) break;
        b = e2 + 1;
      }
      // Huit jetons : `cardinalites`, `K=<k>`, puis les six champs nommes.
      i64 k = 0;
      if (tok.size() != 8 || tok[1].compare(0, 2, "K=") != 0 || !parse_i64_exact(tok[1].c_str() + 2, &k) ||
          k < 1 || k > 10 || e->cards.count((u64)k)) {
        bad = true;
        continue;
      }
      u64 v[6] = {0, 0, 0, 0, 0, 0};
      bool field_ok = true;
      for (int j = 0; j < 6; ++j) {
        const size_t l = std::strlen(kFields[j]);
        i64 x = 0;
        if (tok[(size_t)j + 2].compare(0, l, kFields[j]) != 0 ||
            !parse_i64_exact(tok[(size_t)j + 2].c_str() + l, &x) || x < 0) {
          field_ok = false;
          break;
        }
        v[j] = (u64)x;
      }
      if (!field_ok) {
        bad = true;
        continue;
      }
      e->cards[(u64)k] = KCardinalities{v[0], v[1], v[2], v[3], v[4], v[5]};
    }
  }
  return !bad && n_all == 1 && !e->forest.empty();
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  long long n = 400, threads = 1, seed = 3, smax = 11;
  long long min_orders = 0, min_deltas = 0, min_facets = 0, min_events = 0;
  bool prefix_mode = false, prefix_witness = false;
  std::string expected_path, inject, postprefilter_golden, compat_golden, counts_golden;
  ForestLayout layout = ForestLayout::kClassic;
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
    else if (const char* s = val("--expected=")) expected_path = s;
    else if (const char* s = val("--postprefilter=")) postprefilter_golden = s;
    else if (const char* s = val("--expect-compat=")) compat_golden = s;
    else if (const char* s = val("--expect-counts=")) counts_golden = s;
    else if (const char* s = val("--inject=")) inject = s;
    else if (const char* s = val("--layout=")) ok = parse_forest_layout(s, &layout) && ok;  // classic | csr, exact
    else if (const char* s = val("--smax=")) { ok = parse_i64_exact(s, &v) && v >= 2 && v <= 11 && ok; smax = v; }
    else if (const char* s = val("--min-orders=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_orders = v; }
    else if (const char* s = val("--min-deltas=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_deltas = v; }
    else if (const char* s = val("--min-facets=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_facets = v; }
    else if (const char* s = val("--min-events=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_events = v; }
    else if (arg == "--prefix") prefix_mode = true;
    else if (arg == "--prefix-witness") { prefix_mode = true; prefix_witness = true; }
    else { std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str()); ok = false; }
  }
  if (!ok) return 2;
  if (!inject.empty() && !mutants_enable(inject.c_str())) {
    std::fprintf(stderr, "mutant inconnu : %s\n", inject.c_str());
    return 2;
  }
  const int coord = cloud_family_default_coord(family, (int)n);
  const std::vector<InputPoint> in = make_family_input(family, (int)n, coord, seed);
  if (in.size() < 2) return 2;

  // REFUS AVANT CALCUL du mode prefixe mal arme (jamais un vert par defaut) :
  // un « prefixe » a smax=11 n'est pas un prefixe, et sans reference il n'y a
  // rien a comparer.
  if (prefix_mode && smax >= 11) {
    std::fprintf(stderr, "--prefix exige --smax < 11 (recu %lld)\n", smax);
    return 2;
  }
  if (prefix_mode && expected_path.empty()) {
    std::fprintf(stderr, "--prefix exige --expected=<reference gravee des dix ordres>\n");
    return 2;
  }

  // Multiensembles canoniques des evenements par K, collectes au callback.
  std::vector<std::vector<EventKey>> canon;
  u64 batch_levels_violations = 0;
  u64 observed_events = 0;

  RunOptions opt;
  opt.s = 8;
  opt.smax = (u64)smax;
  opt.threads = (int)threads;
  opt.digest = true;
  opt.forest_layout = layout;
  if (prefix_mode) {
    canon.assign(11, {});
    opt.on_forest = [&](u64 K, const std::vector<ForestEvent>& ev, const ForestResult& r) {
      // INVARIANT DE LOT (le seul observable de prefix-tamper-batch-levels, qui
      // altere r.batch_levels APRES le calcul du digest de la foret) : un
      // niveau de lot est pousse exactement une fois par lot ferme.
      if (r.batch_levels.size() != (size_t)r.batches) ++batch_levels_violations;
      observed_events += (u64)ev.size();
      if (prefix_witness && K < canon.size()) canon[(size_t)K] = canonical_events(ev);
    };
  }
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    // Sous mutant, un refus/invariant EST une divergence detectee.
    if (!inject.empty()) {
      std::fprintf(stderr, "mutant %s : statut %s — tue\n", inject.c_str(), rr.message.c_str());
      return 4;
    }
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  // NON-VACUITE du stockage demande (palier KeyCSR) : sous --layout=csr, chaque
  // K publie doit avoir CONSTRUIT le csr (csr_fallback est mesure, 0 exige).
  if (layout == ForestLayout::kCsr && (rr.csr_fallback != 0 || rr.forest_storage_conformes != rr.kmax_eff)) {
    std::fprintf(stderr, "layout=csr demande mais non construit sur tous les K (fallback=%llu, conformes=%llu/%llu)\n",
                 (unsigned long long)rr.csr_fallback, (unsigned long long)rr.forest_storage_conformes,
                 (unsigned long long)rr.kmax_eff);
    return 1;
  }
  std::printf("forest_layout=%s csr_fallback=%llu ordres_storage_conformes=%llu\n", forest_layout_name(layout),
              (unsigned long long)rr.csr_fallback, (unsigned long long)rr.forest_storage_conformes);

  Expected e;
  if (!expected_path.empty()) {
    if (!load_expected_file(expected_path.c_str(), &e)) {
      std::fprintf(stderr, "reference illisible : %s\n", expected_path.c_str());
      return 2;
    }
  } else {
    const char* fname = cloud_family_name(family);
    for (const SmallRef& r : kSmallRefs) {
      if (r.family && std::strcmp(r.family, fname) == 0 && r.n == (int)n) {
        e.balls = r.balls;
        e.all = r.all;
        break;
      }
    }
    if (e.all.empty()) {
      std::fprintf(stderr, "aucune reference gravee pour %s n=%lld ; digests v6 :\n", fname, n);
      std::fprintf(stderr, "    {\"%s\", %lld, \"%s\", \"%s\"},\n", fname, n, rr.digest_balls.c_str(),
                   rr.digest_all.c_str());
      return 2;
    }
  }

  if (!expected_path.empty() && !prefix_mode) {
    // Ensemble EXACT exige (audit du 31 aout, cinquieme cycle) : les clefs de
    // la reference doivent EGALER {1..kmax_eff} — chaque K present (une
    // reference reduite a K10 ne doit jamais eviter la comparaison K1) ET
    // aucune clef au-dela de kmax_eff (le chargeur ne connait que le domaine
    // global [1,10] ; une clef en trop serait silencieusement ignoree par la
    // boucle de comparaison).
    for (u64 K = 1; K <= rr.kmax_eff; ++K)
      if (!e.forest.count(K)) {
        std::fprintf(stderr, "reference incomplete : digest_forest_K%llu absent\n", (unsigned long long)K);
        return 2;
      }
    if (e.forest.size() != (size_t)rr.kmax_eff) {
      std::fprintf(stderr,
                   "reference avec forets hors profil : %zu clef(s) pour kmax_eff=%llu (ensemble exact exige)\n",
                   e.forest.size(), (unsigned long long)rr.kmax_eff);
      return 2;
    }
  }
  if (prefix_mode) {
    // La reference doit porter le prefixe COMPLET {1..kmax_eff} — forets ET
    // cardinalites — et STRICTEMENT PLUS d'ordres que kmax_eff : sans quoi ce
    // n'est pas la lecture d'un prefixe, et la porte se validerait elle-meme.
    for (u64 K = 1; K <= rr.kmax_eff; ++K)
      if (!e.forest.count(K) || !e.cards.count(K)) {
        std::fprintf(stderr, "reference incomplete pour le prefixe : K%llu absent (foret ou cardinalites)\n",
                     (unsigned long long)K);
        return 2;
      }
    if (e.forest.size() <= (size_t)rr.kmax_eff || e.cards.size() <= (size_t)rr.kmax_eff) {
      std::fprintf(stderr,
                   "reference non prefixable : %zu foret(s) / %zu cardinalite(s) pour kmax_eff=%llu "
                   "(une reference strictement plus longue est exigee)\n",
                   e.forest.size(), e.cards.size(), (unsigned long long)rr.kmax_eff);
      return 2;
    }
  }
  u64 mismatches = 0;
  // La conformite d'OBJET juge digest_all et les forets (P0 du 31 aout : les
  // deux monnaies de candidats sont gelees separement ; depuis le correctif
  // du cover q4 au coefficient 4, le multiensemble de candidats diverge
  // legitimement de la v5 — rapporte, jamais un critere).
  if (!e.balls.empty() && e.balls != rr.digest_balls)
    std::fprintf(stderr,
                 "note : divergence diagnostique NON JUGEE des candidats v5-compat (v5=%.16s... v6=%.16s...)%s\n",
                 e.balls.c_str(), rr.digest_balls.c_str(),
                 prefix_mode ? " — attendue en mode prefixe : l'elagage de generation depend de smax" : "");
  for (const auto& [k, dg] : e.forest) {
    if (k <= rr.kmax_eff && dg != rr.digest_forest[k]) {
      ++mismatches;
      std::fprintf(stderr, "digest_forest_K%llu : v5=%s v6=%s\n", (unsigned long long)k, dg.c_str(),
                   rr.digest_forest[k].c_str());
    }
  }
  // digest_all chaine TOUS les ordres publies : a smax < 11 il en chaine moins,
  // et l'egaler serait faux. Jamais compare en mode prefixe (dit, jamais tu).
  if (!prefix_mode && !e.all.empty() && e.all != rr.digest_all) {
    ++mismatches;
    std::fprintf(stderr, "digest_all : v5=%s v6=%s\n", e.all.c_str(), rr.digest_all.c_str());
  }
  if (!compat_golden.empty() && compat_golden != rr.digest_balls) {
    ++mismatches;
    std::fprintf(stderr, "digest_candidates_v5_compat : golden=%s v6=%s\n", compat_golden.c_str(),
                 rr.digest_balls.c_str());
  }
  if (!counts_golden.empty()) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%llu/%llu/%llu", (unsigned long long)rr.expand.unique_balls,
                  (unsigned long long)rr.expand.dead_depth, (unsigned long long)rr.expand.survivors);
    if (counts_golden != buf) {
      ++mismatches;
      std::fprintf(stderr, "cardinalites uniques/mortes/survivantes : golden=%s v6=%s\n", counts_golden.c_str(),
                   buf);
    }
  }
  if (!postprefilter_golden.empty() && postprefilter_golden != rr.digest_postprefilter) {
    ++mismatches;
    std::fprintf(stderr, "digest_postprefilter : golden=%s v6=%s\n", postprefilter_golden.c_str(),
                 rr.digest_postprefilter.c_str());
  }

  // ------------------------------------------------------------------ PREFIXE
  u64 cmp_orders = 0, cmp_deltas = 0, cmp_facets = 0, cmp_events = 0;
  if (prefix_mode) {
    for (u64 K = 1; K <= rr.kmax_eff; ++K) {
      const KCardinalities& ref = e.cards.at(K);
      const KCardinalities& got = rr.cards[K];
      ++cmp_orders;
      // Les planchers accumulent les cardinalites DU RUN (`got`), jamais
      // celles de la reference gravee : un plancher doit etre un observable
      // du CALCUL juge, et rester un temoin si la comparaison ci-dessous
      // regressait un jour. Les deux sont egales tant que l'egalite tient —
      // c'est precisement ce que la porte exige.
      cmp_deltas += got.deltas;
      cmp_facets += got.facets;
      cmp_events += got.events;
      if (ref == got) continue;
      ++mismatches;
      std::fprintf(stderr,
                   "cardinalites K=%llu : v5(smax=11) ev=%llu fa=%llu de=%llu at=%llu fu=%llu no=%llu | "
                   "v6(smax=%lld) ev=%llu fa=%llu de=%llu at=%llu fu=%llu no=%llu\n",
                   (unsigned long long)K, (unsigned long long)ref.events, (unsigned long long)ref.facets,
                   (unsigned long long)ref.deltas, (unsigned long long)ref.attachments,
                   (unsigned long long)ref.fusions, (unsigned long long)ref.nodes, smax,
                   (unsigned long long)got.events, (unsigned long long)got.facets,
                   (unsigned long long)got.deltas, (unsigned long long)got.attachments,
                   (unsigned long long)got.fusions, (unsigned long long)got.nodes);
    }
    if (batch_levels_violations) {
      ++mismatches;
      std::fprintf(stderr, "invariant de lot : batch_levels.size() != batches sur %llu foret(s) recue(s)\n",
                   (unsigned long long)batch_levels_violations);
    }
    // TEMOIN DU MULTIENSEMBLE D'EVENEMENTS : second run smax=11 dans le MEME
    // processus. Le mutant prefix-tamper-event-order est garde par kmax < 10 :
    // il frappe le run de prefixe et JAMAIS le run complet, ce qui rend la
    // comparaison causale. L'ordre brut des evenements depend du decoupage en
    // tranches (les deux runs n'ont pas les memes boules) : seul le
    // multiensemble TRIE est comparable.
    if (prefix_witness) {
      RunOptions full = opt;
      full.smax = 11;
      full.digest = false;
      std::vector<std::string> witness_diff;
      u64 seen = 0;
      full.on_forest = [&](u64 K, const std::vector<ForestEvent>& ev, const ForestResult&) {
        if (K > rr.kmax_eff || K >= canon.size()) return;
        ++seen;
        const std::vector<EventKey> full_canon = canonical_events(ev);
        const std::vector<EventKey>& pref_canon = canon[(size_t)K];
        if (full_canon.size() != pref_canon.size()) {
          witness_diff.push_back("K" + std::to_string(K) + " : " + std::to_string(pref_canon.size()) +
                                 " evenements au prefixe contre " + std::to_string(full_canon.size()) +
                                 " a smax=11");
          return;
        }
        for (size_t i = 0; i < full_canon.size(); ++i)
          if (!event_equal(full_canon[i], pref_canon[i])) {
            witness_diff.push_back("K" + std::to_string(K) + " : evenement canonique " + std::to_string(i) +
                                   " different (support/interieur/masque/niveau)");
            return;
          }
      };
      const RunResult rf = run_pipeline(in, full);
      if (rf.status != PipelineStatus::kCompleteRegular) {
        std::fprintf(stderr, "temoin smax=11 : REFUS %s\n", rf.message.c_str());
        if (!inject.empty()) return 4;
        return status_exit_code(rf.status);
      }
      if (seen != rr.kmax_eff) {
        std::fprintf(stderr, "temoin smax=11 : %llu ordre(s) confronte(s) pour kmax_eff=%llu\n",
                     (unsigned long long)seen, (unsigned long long)rr.kmax_eff);
        ++mismatches;
      }
      for (const std::string& d : witness_diff) {
        ++mismatches;
        std::fprintf(stderr, "multiensemble canonique des evenements : %s\n", d.c_str());
      }
      std::printf("temoin_evenements ordres=%llu evenements_observes=%llu divergences=%zu\n",
                  (unsigned long long)seen, (unsigned long long)observed_events, witness_diff.size());
    }
  }

  if (!inject.empty()) {
    if (mismatches) {
      std::fprintf(stderr, "mutant %s : %llu divergence(s) — tue\n", inject.c_str(), (unsigned long long)mismatches);
      return 4;
    }
    std::fprintf(stderr, "mutant %s : AUCUNE divergence — survivant\n", inject.c_str());
    return 3;
  }
  if (mismatches) return 1;
  // PLANCHERS DE COUVERTURE (jamais evalues sous --inject : un mutant qui
  // abaisse une cardinalite doit rendre 3 « survivant », pas 3 « plancher »).
  if ((long long)cmp_orders < min_orders || (long long)cmp_deltas < min_deltas ||
      (long long)cmp_facets < min_facets || (long long)cmp_events < min_events) {
    std::fprintf(stderr,
                 "plancher de couverture viole : ordres=%llu (>=%lld) deltas=%llu (>=%lld) facettes=%llu (>=%lld) "
                 "evenements=%llu (>=%lld)\n",
                 (unsigned long long)cmp_orders, min_orders, (unsigned long long)cmp_deltas, min_deltas,
                 (unsigned long long)cmp_facets, min_facets, (unsigned long long)cmp_events, min_events);
    return 3;
  }
  if (prefix_mode) {
    std::printf("prefixe exact : %s n=%lld smax=%lld : %llu ordre(s) sur %zu de la reference "
                "(digests + cardinalites), deltas=%llu facettes=%llu evenements=%llu\n",
                cloud_family_name(family), n, smax, (unsigned long long)cmp_orders, e.forest.size(),
                (unsigned long long)cmp_deltas, (unsigned long long)cmp_facets, (unsigned long long)cmp_events);
    return 0;
  }
  std::printf("conformite v5=v6 : %s n=%lld : %zu forets + digest_all identiques (objet)\n",
              cloud_family_name(family), n, e.forest.size());
  return 0;
}

