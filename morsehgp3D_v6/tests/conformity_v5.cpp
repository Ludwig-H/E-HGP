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
// Codes : 0 conforme ; 1 desaccord de digest (juge) ; 2 refus avant calcul ;
// 3 mutant injecte non tue ; 4 mutant injecte tue (divergence detectee).
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

namespace {

struct Expected {
  std::string balls, all;
  std::map<u64, std::string> forest;
};

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
    }
  }
  return !bad && n_all == 1 && !e->forest.empty();
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  long long n = 400, threads = 1, seed = 3;
  std::string expected_path, inject, postprefilter_golden, compat_golden, counts_golden;
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

  RunOptions opt;
  opt.s = 8;
  opt.smax = 11;
  opt.threads = (int)threads;
  opt.digest = true;
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

  if (!expected_path.empty()) {
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
  u64 mismatches = 0;
  // La conformite d'OBJET juge digest_all et les forets (P0 du 31 aout : les
  // deux monnaies de candidats sont gelees separement ; depuis le correctif
  // du cover q4 au coefficient 4, le multiensemble de candidats diverge
  // legitimement de la v5 — rapporte, jamais un critere).
  if (!e.balls.empty() && e.balls != rr.digest_balls)
    std::fprintf(stderr,
                 "note : divergence diagnostique NON JUGEE des candidats v5-compat (v5=%.16s... v6=%.16s...)\n",
                 e.balls.c_str(), rr.digest_balls.c_str());
  for (const auto& [k, dg] : e.forest) {
    if (k <= rr.kmax_eff && dg != rr.digest_forest[k]) {
      ++mismatches;
      std::fprintf(stderr, "digest_forest_K%llu : v5=%s v6=%s\n", (unsigned long long)k, dg.c_str(),
                   rr.digest_forest[k].c_str());
    }
  }
  if (!e.all.empty() && e.all != rr.digest_all) {
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
  if (!inject.empty()) {
    if (mismatches) {
      std::fprintf(stderr, "mutant %s : %llu divergence(s) — tue\n", inject.c_str(), (unsigned long long)mismatches);
      return 4;
    }
    std::fprintf(stderr, "mutant %s : AUCUNE divergence — survivant\n", inject.c_str());
    return 3;
  }
  if (mismatches) return 1;
  std::printf("conformite v5=v6 : %s n=%lld : %zu forets + digest_all identiques (objet)\n",
              cloud_family_name(family), n, e.forest.size());
  return 0;
}
