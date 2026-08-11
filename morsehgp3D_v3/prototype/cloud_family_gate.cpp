// MorseHGP3D v3 — LA PORTE DIRECTE DU GENERATEUR PARTAGE (audit etat
// courant, porte 1) : le contrat durable « exactement n, ou refus ferme »
// est teste ICI, sur `make_family_cloud` directement, pour les QUATRE
// familles — sans passer par un consommateur particulier. La porte ferme
// trois obligations que le contre-exemple 12 500 multiecho avait revelees :
//
//   1. JAMAIS PLUS que n : chaque push est borne — l'ancienne garde
//      avant-pixel laissait un ou deux echos de recouvrement depasser n
//      (12 501 points pour 12 500 a coord 707) et desynchronisait ledger,
//      fate et oracle des drivers. Le mutant `generator-overshoot` retablit
//      cette faille et doit mourir ICI, sur le generateur nu.
//   2. EXACTEMENT n au nominal : sur les emprises par defaut, chaque
//      famille remplit sa demande — c'est la regression gravee.
//   3. L'INSUFFISANCE EST VISIBLE : une emprise trop petite rend MOINS de n
//      points (jamais plus, jamais de jitter d'appoint) — le refus ferme
//      appartient aux consommateurs, qui exigent l'egalite avant tout
//      calcul ; la porte grave le comportement du generateur qui le permet.
//
// La porte verifie aussi les bornes u16 [0, 65536) et la distinction des
// points (clef de deduplication du generateur). Codes : 0 OK ; 1 contrat
// viole ; 2 CLI ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <set>

#include "prototype/cloud_families.hpp"

namespace {

using i64 = long long;

const char* check_cloud(const std::vector<mhgp::P3>& pts, int n, bool expect_exact) {
  if ((int)pts.size() > n) return "le generateur a rendu PLUS de points que n";
  if (expect_exact && (int)pts.size() != n)
    return "le generateur n'a pas rempli une demande nominale";
  std::set<long long> keys;
  for (const mhgp::P3& p : pts) {
    if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 || p.z < 0 || p.z > 65535)
      return "une coordonnee sort du domaine u16";
    const long long key = ((long long)p.x << 34) | ((long long)p.y << 17) | (long long)p.z;
    if (!keys.insert(key).second) return "deux PointId partagent la meme coordonnee";
  }
  return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
  bool mutant_overshoot = false;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--mutant")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --mutant\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "generator-overshoot")) mutant_overshoot = true;
      else { std::printf("ECHEC : mutant inconnu %s\n", argv[i]); return 2; }
      continue;
    }
    std::printf("ECHEC : argument inconnu %s\n", argv[i]);
    return 2;
  }

  const mhgp3v::CloudFamily families[4] = {
      mhgp3v::CloudFamily::kUniform, mhgp3v::CloudFamily::kTerrain,
      mhgp3v::CloudFamily::kScanlineSinglePass,
      mhgp3v::CloudFamily::kScanlineOverlapMultiecho};
  const i64 seeds[2] = {13, 20260810};
  const int sizes[3] = {24, 400, 1100};

  i64 nominal_runs = 0;
  const char* violation = nullptr;
  const char* where = "";

  // 1-2. Le nominal : quatre familles x trois tailles x deux graines, emprise
  // par defaut — exactement n, u16, points distincts. Sous le mutant, le
  // parcours continue : c'est le contre-exemple historique qui doit mordre.
  for (const mhgp3v::CloudFamily family : families)
    for (const int n : sizes)
      for (const i64 seed : seeds) {
        const int coord = mhgp3v::cloud_family_default_coord(family, n);
        const std::vector<mhgp::P3> pts =
            mhgp3v::make_family_cloud(family, n, coord, seed, mutant_overshoot);
        const char* what = check_cloud(pts, n, /*expect_exact=*/true);
        if (what != nullptr && violation == nullptr) {
          violation = what;
          where = mhgp3v::cloud_family_name(family);
        }
        ++nominal_runs;
      }

  // Le CONTRE-EXEMPLE PERMANENT de l'audit : multiecho, n=12500, coord=707,
  // graine 20260810 — l'ancien generateur rendait 12 501 points. La
  // regression est gravee ici a l'identique.
  {
    const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(
        mhgp3v::CloudFamily::kScanlineOverlapMultiecho, 12500, 707, 20260810,
        mutant_overshoot);
    const char* what = check_cloud(pts, 12500, /*expect_exact=*/true);
    if (what != nullptr && violation == nullptr) {
      violation = what;
      where = "scanline_overlap_multiecho (contre-exemple 12500/707)";
    }
    ++nominal_runs;
  }

  if (violation != nullptr) {
    if (mutant_overshoot) {
      std::printf("mutant tue par la porte du generateur : %s (%s)\n", violation, where);
      return 4;
    }
    std::printf("ECHEC de la porte du generateur : %s (%s)\n", violation, where);
    return 1;
  }
  if (mutant_overshoot) {
    std::printf("MUTANT SURVIVANT : l'overshoot n'a pas ete detecte\n");
    return 0;
  }

  // 3. L'insuffisance : une emprise de 4 ne porte que 64 cellules — le
  // generateur uniform rend 64 points pour 1000 demandes, JAMAIS plus,
  // jamais de complement invente. Les consommateurs refusent l'inegalite.
  {
    const std::vector<mhgp::P3> pts =
        mhgp3v::make_family_cloud(mhgp3v::CloudFamily::kUniform, 1000, 4, 13);
    const char* what = check_cloud(pts, 1000, /*expect_exact=*/false);
    if (what != nullptr) {
      std::printf("ECHEC de la porte du generateur : %s (uniform affame)\n", what);
      return 1;
    }
    if ((int)pts.size() != 64) {
      std::printf("ECHEC de la porte du generateur : l'emprise 4^3 devrait saturer a"
                  " 64 points distincts, pas %zu\n", pts.size());
      return 1;
    }
  }

  std::printf("porte du generateur : %lld campagnes nominales, quatre familles,"
              " contre-exemple 12500/707 rejoue, insuffisance visible a 64/1000\n",
              nominal_runs);
  std::printf("OK : contrat de cardinalite du generateur partage — exactement n au"
              " nominal, jamais plus, insuffisance sans complement\n");
  return 0;
}
