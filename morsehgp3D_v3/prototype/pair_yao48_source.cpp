// MorseHGP3D v3 — LA SOURCE q2 CANDIDATE PRODUIT : MORTON/LBVH RESIDENT,
// COUPE YAO48 STRICTE FAIL-OPEN, CLASSIFICATION TERMINALE ET CENSUS FERME
// (NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811, architecture de
// CATALOGUE_PAIRES_DIAMETRALES_EXACT.md respecialisee u16 : toute
// l'arithmetique decisive tient en i64, |Phi| < 3*2^34).
//
// LE THEOREME DE COUPE DIRECTIONNELLE (variante STRICTE, doc. catalogue §2) :
// pour une ancre p, une chambre canonique (8 octants x 6 permutations par
// magnitudes decroissantes), K temoins de PointId distincts de la chambre a
// distance carree <= D et STRICTEMENT > 0, et une cible q de coordonnees
// canoniques (x,y,z) dans la MEME chambre,
//
//     x^2 > D   et   (x+y)^2 > 2D   et   (x+y+z)^2 > 3D
//
// certifient Phi_{p,q}(w) < 0 pour les K temoins : K interieurs STRICTS
// distincts, q hors banque (x^2 > D >= dist^2 des temoins), p exclu
// (dist^2 > 0). A K = 10, la paire est tombstonee (p >= 10 donc p+q >= 12,
// bloc H0-inerte a K=10) SANS visite. Toute egalite descend ; l'echec du
// certificat ne classe RIEN (non-converse grave). Sur un noeud LBVH
// entierement contenu dans la chambre, les minima canoniques par axe donnent
// le meme certificat pour toutes ses feuilles : reçu de masse, zero paire
// materialisee.
//
// MUTANT MORT-NE DOCUMENTE (temoin colocalise avec l'ancre) : un temoin a
// dist^2 = 0 ne peut entrer que dans la chambre du vecteur nul (octant +++,
// permutation identite). Or une cible de cette chambre verifie d >= 0 par
// axe, donc domine l'ancre composante par composante, donc a une cle Morton
// superieure : elle n'est JAMAIS possedee par l'ancre — sauf colocalisee,
// auquel cas x = 0 et x^2 > D >= 0 ne passe jamais. La banque exclut
// dist^2 = 0 par defense en profondeur ; aucun mutant ne peut le rendre
// observable.
//
// LA CLASSIFICATION TERMINALE : chaque paire survivante (u,v) est classifiee
// par un parcours LBVH avec l'infimum/supremum separables exacts de
// 4*Phi sur les boites (les formules de clip recues de la lane self-join).
// inf4 > 0 ecarte le noeud ; inf4 = 0 n'a AUCUN interieur strict mais peut
// porter des CONTACTS : le census ferme doit y descendre (mutant grave) ;
// sup4 < 0 credite le noeud entier en stricts ; l'arret anticipe a dix
// stricts emet la tombstone, sinon le record publie le census ferme complet
// (rang, stricts, contacts, extremites comprises — Phi(u) = Phi(v) = 0).
//
// LE LEDGER (par ancre puis global) :
//     region_pruned_mass + point_tombstones + survivantes = positions possedees,
//     somme des possessions = C(n,2),
//     survivantes = tombstones_classifieur + records_census.
// Aucun tableau global de paires : le mode mesure compte, le mode oracle
// (n <= 256) tient les sorts pour le juge.
//
// LE JUGE INDEPENDANT (oracle) : scan quadratique complet, arithmetique
// VOLONTAIREMENT distincte (4*Phi = ||2x-u-v||^2 - ||u-v||^2 en i128, jamais
// la forme produit du sujet), sans Morton, sans LBVH, sans chambre. Il rejoue
// chaque reçu de prune temoin par temoin et compare TOUS les sorts, stricts,
// rangs et census.
//
// Codes : 0 OK ; 1 violation ; 2 CLI ; 3 plancher/budget ; 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <queue>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"
#include "prototype/yao48_source.hpp"

namespace {

// LA MACHINE VIT DANS L'EN-TETE PARTAGE prototype/yao48_source.hpp (harnais
// warm_e2e et probe consomment le meme cœur) ; le probe garde le juge, les
// fixtures et les mutants.
using i64 = long long;
using i128 = __int128;
using Node = mhgp3v::LbvhNode;
using Lbvh = mhgp3v::MortonLbvh;
using mhgp3v::yao48::kOrderK;
using mhgp3v::yao48::PairFate;
using mhgp3v::yao48::fate_is_tombstone;
using mhgp3v::yao48::SourceInjections;
using mhgp3v::yao48::SourceReceipt;
using mhgp3v::yao48::BankTableEntry;
using mhgp3v::yao48::YaoReceipt;
using mhgp3v::yao48::CensusRecord;
using mhgp3v::yao48::YaoSource;

inline int judge_phi_sign(const mhgp::P3& x, const mhgp::P3& u, const mhgp::P3& v) {
  i128 norm = 0, diam = 0;
  const i64 xs[3] = {(i64)x.x, (i64)x.y, (i64)x.z};
  const i64 us[3] = {(i64)u.x, (i64)u.y, (i64)u.z};
  const i64 vs[3] = {(i64)v.x, (i64)v.y, (i64)v.z};
  for (int d = 0; d < 3; ++d) {
    const i64 e = 2 * xs[d] - us[d] - vs[d];
    norm += (i128)e * e;
    const i64 dd = us[d] - vs[d];
    diam += (i128)dd * dd;
  }
  if (norm < diam) return -1;
  if (norm == diam) return 0;
  return 1;
}

mhgp::P3 pt(int x, int y, int z) {
  mhgp::P3 p{};
  p.x = (mhgp::i32)x;
  p.y = (mhgp::i32)y;
  p.z = (mhgp::i32)z;
  return p;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, leaf_size = 8, oracle = 0, differential = 0, permute = 0;
  int policy_differential = 0, antichain = 0;
  i64 seed = 20260810, bank_pops = 512, chamber_visits = 100000, max_work = 4000000000LL;
  i64 min_region_prunes = 0, min_point_tombstones = 0, min_census_records = 0;
  i64 min_radial_prunes = 0;
  i64 min_underfull = 0, min_survivors = 0, min_classifier_tombs = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name;
  SourceInjections injections;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 1000000000000ULL) return false;
    *value = (i64)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else if (!strcmp(argv[i], "scanline_single_pass"))
        family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (!strcmp(argv[i], "scanline_overlap_multiecho"))
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--fixture")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --fixture\n"); return 2; }
      fixture_name = argv[++i];
      continue;
    }
    if (!strcmp(argv[i], "--bank-mode")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --bank-mode\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "exact")) antichain = 0;
      else if (!strcmp(argv[i], "antichain")) antichain = 1;
      else { std::printf("ECHEC : bank-mode inconnu %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "strict-to-large")) injections.strict_to_large = true;
      else if (!strcmp(argv[i], "d-understated")) injections.d_understated = true;
      else if (!strcmp(argv[i], "ownership-doubled")) injections.ownership_doubled = true;
      else if (!strcmp(argv[i], "last-region-omitted"))
        injections.last_region_omitted = true;
      else if (!strcmp(argv[i], "census-skips-inf-zero"))
        injections.census_skips_inf_zero = true;
      else if (!strcmp(argv[i], "threshold-minus-one"))
        injections.threshold_minus_one = true;
      else if (!strcmp(argv[i], "chamber-perm-swapped"))
        injections.chamber_perm_swapped = true;
      else if (!strcmp(argv[i], "radial-forgets-chamber"))
        injections.radial_forgets_chamber = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--bank-pops")) bank_pops = value;
    else if (!strcmp(argv[i], "--chamber-visits")) chamber_visits = value;
    else if (!strcmp(argv[i], "--max-work")) max_work = value;
    else if (!strcmp(argv[i], "--oracle")) oracle = (int)value;
    else if (!strcmp(argv[i], "--differential")) differential = (int)value;
    else if (!strcmp(argv[i], "--policy-differential")) policy_differential = (int)value;
    else if (!strcmp(argv[i], "--permute")) permute = (int)value;
    else if (!strcmp(argv[i], "--min-region-prunes")) min_region_prunes = value;
    else if (!strcmp(argv[i], "--min-radial-prunes")) min_radial_prunes = value;
    else if (!strcmp(argv[i], "--min-point-tombstones")) min_point_tombstones = value;
    else if (!strcmp(argv[i], "--min-census-records")) min_census_records = value;
    else if (!strcmp(argv[i], "--min-underfull")) min_underfull = value;
    else if (!strcmp(argv[i], "--min-survivors")) min_survivors = value;
    else if (!strcmp(argv[i], "--min-classifier-tombstones")) min_classifier_tombs = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || bank_pops < 48 || chamber_visits < 8 || max_work < 1 ||
      oracle < 0 || oracle > 1 || policy_differential < 0 || policy_differential > 1 ||
      differential < 0 || differential > 1 || permute < 0 || permute > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  const bool is_fixture = !fixture_name.empty();
  std::vector<mhgp::P3> pts;
  if (is_fixture) {
    if (fixture_name == "directional-equality") {
      // L'EGALITE DIRECTIONNELLE EXACTE (verifiee en entiers hors bande) :
      // ancre p = (60000,60000,60000), Morton max par dominance composante
      // par composante. Banque de la chambre (---, x>=y>=z) : neuf temoins
      // proches stricts + le temoin (5,5,5) a dist^2 = 75 = D, PARALLELE a
      // (1,1,1). Cible q = p - (9,4,2) : x^2 = 81 > 75 et (x+y)^2 = 169 >
      // 150 passent, (x+y+z)^2 = 225 = 3D EXACTEMENT — la coupe stricte
      // descend. Verite : 9 stricts + le contact (5,5,5) (Phi = 75-75 = 0),
      // donc AUCUNE tombstone n'est licite : le mutant strict-to-large prune
      // et le juge le tue.
      pts = {pt(60000, 60000, 60000), pt(59991, 59996, 59998), pt(59995, 59995, 59995),
             pt(59996, 59997, 59999), pt(59996, 59998, 59999), pt(59995, 59997, 59998),
             pt(59995, 59996, 59999), pt(59994, 59997, 59998), pt(59994, 59996, 59999),
             pt(59996, 59997, 59998), pt(59995, 59998, 59999), pt(59994, 59998, 59999)};
    } else if (fixture_name == "region-prune") {
      // LE REÇU DE REGION (verifie en entiers hors bande) : dix temoins
      // proches (D = 1526) et un cluster lointain de huit cibles dans la
      // meme chambre, dont la boite passe la coupe stricte sur ses minima
      // canoniques — huit paires tombstonees par UN reçu de masse. Le mutant
      // chamber-perm-swapped desynchronise la cible et perd le prune.
      pts = {pt(60000, 60000, 60000)};
      const int wit[10][3] = {{30, 2, 1}, {31, 2, 1}, {32, 2, 1}, {33, 2, 1}, {34, 2, 1},
                              {35, 2, 1}, {36, 2, 1}, {37, 2, 1}, {38, 2, 1}, {39, 2, 1}};
      for (const auto& w : wit) pts.push_back(pt(60000 - w[0], 60000 - w[1], 60000 - w[2]));
      for (int k = 0; k < 8; ++k)
        pts.push_back(pt(60000 - (3000 + 7 * k), 60000 - (40 + 3 * k), 60000 - (20 + 2 * k)));
    } else if (fixture_name == "radial-straddle") {
      // LA BOITE A CHEVAL (audit de reemploi §4) : deux chambres denses
      // adjacentes (x>=y>=z et x>=z>=y, octant ---), pleines a D ~ 1526, et
      // un cluster lointain a cheval sur leur frontiere y=z a dist^2 ~
      // 160000 > 3*D — l'enveloppe radiale prune la boite entiere en UN
      // reçu, sans developper ses feuilles. Verite : les temoins des deux
      // banques sont profondement stricts pour ces paires alignees.
      pts = {pt(60000, 60000, 60000)};
      for (int k = 0; k < 10; ++k) pts.push_back(pt(60000 - (30 + k), 60000 - 2, 60000 - 1));
      for (int k = 0; k < 10; ++k) pts.push_back(pt(60000 - (30 + k), 60000 - 1, 60000 - 2));
      const int straddle[6][3] = {{400, 3, 2}, {400, 2, 3}, {401, 3, 2},
                                  {401, 2, 3}, {402, 3, 2}, {402, 2, 3}};
      for (const auto& o : straddle)
        pts.push_back(pt(60000 - o[0], 60000 - o[1], 60000 - o[2]));
    } else if (fixture_name == "underfull") {
      // LE FAIL-OPEN EXERCE : six points epars — toutes les banques restent
      // sous-pleines, aucune coupe, toutes les paires au classifieur.
      pts = {pt(100, 100, 100), pt(60000, 200, 300), pt(300, 60000, 400),
             pt(400, 500, 60000), pt(50000, 50000, 100), pt(200, 40000, 40000)};
    } else if (fixture_name == "nonconverse") {
      // LE NON-CONVERSE GRAVE (doc. catalogue §2) : l'echec de la coupe ne
      // classe rien. Trois points, w EXACTEMENT sur la sphere diametrale de
      // (p,q) : census ferme 3, stricts 0, contact 1.
      pts = {pt(200, 200, 200), pt(198, 200, 200), pt(199, 199, 200)};
    } else if (fixture_name == "u16-extremes") {
      // LES EXTREMES u16 : diagonale complete + dix-huit temoins stricts en
      // neuf paires antipodales (la fixture skew de la lane profondeur) —
      // la paire diagonale est tombstonee par le CLASSIFIEUR (18 stricts).
      pts = {pt(0, 0, 0), pt(65535, 65535, 65535)};
      const int skew[18][3] = {
          {65535, 0, 32768},     {0, 65535, 32767},     {0, 32768, 65535},
          {65535, 32767, 0},     {32768, 65535, 0},     {32767, 0, 65535},
          {65535, 32768, 0},     {0, 32767, 65535},     {32768, 0, 65535},
          {32767, 65535, 0},     {0, 65535, 32768},     {65535, 0, 32767},
          {49151, 65535, 1},     {16384, 0, 65534},     {1, 16384, 65535},
          {65534, 49151, 0},     {65535, 16384, 49152}, {0, 49151, 16383}};
      for (const auto& s : skew) pts.push_back(pt(s[0], s[1], s[2]));
    } else if (fixture_name == "colocated") {
      // Vingt PointId a la meme coordonnee : aucune coupe possible (toutes
      // les distances nulles), toutes les paires en census de rang 20 avec
      // zero strict — les contacts sont TOUS les autres points.
      for (int j = 0; j < 20; ++j) pts.push_back(pt(12345, 23456, 34567));
    } else {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    n = (int)pts.size();
    leaf_size = 2;
    oracle = 1;
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d, oracle"
                " force)\n", fixture_name.c_str(), n, leaf_size);
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
                " --leaf-size %d --bank-pops %lld\n", n, coord, seed,
                mhgp3v::cloud_family_name(family), leaf_size, bank_pops);
  }
  if (oracle == 1 && n > 256) {
    std::printf("ECHEC : l'oracle exhaustif exige n <= 256\n");
    return 2;
  }
  if (differential == 1 && n > 3000) {
    std::printf("ECHEC : le differentiel bi-mode exige n <= 3000\n");
    return 2;
  }
  if (policy_differential == 1 && n > 3000) {
    std::printf("ECHEC : le differentiel de politiques exige n <= 3000\n");
    return 2;
  }
  const auto fail = [&](const char* what, const char* detail) {
    if (injections.any()) {
      std::printf("mutant tue par %s : %s\n", what, detail);
      return 4;
    }
    std::printf("ECHEC %s : %s\n", what, detail);
    return 1;
  };

  Lbvh tree;
  tree.build(pts, leaf_size);
  const i64 all_pairs = (i64)n * (n - 1) / 2;

  std::vector<PairFate> fates;
  std::vector<YaoReceipt> receipts;
  std::vector<mhgp3v::yao48::RadialReceipt> radial_receipts;
  std::vector<BankTableEntry> bank_table;
  std::vector<CensusRecord> census;
  YaoSource source;
  source.tree = &tree;
  source.injections = injections;
  source.bank_pop_budget = bank_pops;
  source.chamber_visits = chamber_visits;
  source.antichain_banks = antichain == 1;
  source.max_work = max_work;
  if (oracle == 1 || differential == 1 || policy_differential == 1) {
    fates.assign((std::size_t)all_pairs, PairFate::kUnassigned);
    source.fate = &fates;
    source.fate_points = n;
    if (oracle == 1) {
      source.oracle_mode = true;
      source.yao_receipts = &receipts;
      source.radial_receipts = &radial_receipts;
      source.bank_table = &bank_table;
      source.census_records = &census;
    }
  }
  const auto t0 = std::chrono::steady_clock::now();
  if (!source.run()) {
    std::printf("ECHEC : budget de travail global depasse — la source refuse, elle ne"
                " tronque pas\n");
    return 3;
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(t1 - t0).count();
  const SourceReceipt& r = source.receipt;
  if (source.fate_violated)
    return fail("le ledger de sorts", "une paire a recu deux sorts — multiplicite violee");
  if (source.per_anchor_violated)
    return fail("le ledger par ancre",
                "la masse traitee d'une ancre differe de pos(j) — une omission"
                " compensee par un doublon devient visible ici");
  if (source.region_overlap_violated)
    return fail("les reçus de region", "deux intervalles d'une meme ancre se recouvrent");

  // LE LEDGER GLOBAL : reçus de region + tombstones ponctuelles +
  // survivantes = C(n,2), et survivantes = tombstones classifieur + census.
  if (r.region_pruned_mass + r.point_tombstones + r.survivors != all_pairs)
    return fail("l'identite du ledger",
                "masse prunee + tombstones + survivantes != C(n,2)");
  if (r.classifier_tombstones + r.census_records != r.survivors)
    return fail("l'identite du ledger", "survivantes != tombstones + census");
  if (oracle == 1 || differential == 1)
    for (std::size_t k = 0; k < fates.size(); ++k)
      if (fates[k] == PairFate::kUnassigned)
        return fail("le ledger de sorts", "une paire sans sort — partition violee");

  // LE DIFFERENTIEL BI-MODE : la baseline classifie TOUTES les paires sans
  // aucune coupe ; les sorts agreges tombstone/census doivent coincider.
  if (differential == 1) {
    std::vector<PairFate> base_fates((std::size_t)all_pairs, PairFate::kUnassigned);
    YaoSource base;
    base.tree = &tree;
    base.baseline = true;
    base.max_work = max_work;
    base.fate = &base_fates;
    base.fate_points = n;
    if (!base.run()) {
      std::printf("ECHEC : budget depasse dans la baseline\n");
      return 3;
    }
    if (base.fate_violated) {
      std::printf("ECHEC de la baseline : sort double\n");
      return 1;
    }
    for (std::size_t k = 0; k < fates.size(); ++k) {
      const bool subject_tomb = fate_is_tombstone(fates[k]);
      const bool base_tomb = fate_is_tombstone(base_fates[k]);
      if (subject_tomb != base_tomb)
        return fail("le differentiel bi-mode",
                    "un sort tombstone/census differe de la baseline sans coupe");
    }
    std::printf("differentiel : sorts IDENTIQUES — baseline visites=%lld tests=%lld ;"
                " sujet visites=%lld tests=%lld (regions=%lld masse=%lld points=%lld)\n",
                base.receipt.classify_node_visits, base.receipt.classify_point_tests,
                r.classify_node_visits, r.classify_point_tests, r.region_prunes,
                r.region_pruned_mass, r.point_tombstones);
  }

  // L'ORACLE : rejeu des reçus puis differentiel exhaustif du juge.
  if (oracle == 1) {
    // 1. Chaque reçu Yao : dix PointId distincts, hors extremites, et
    // Phi < 0 rejoue par l'arithmetique DISTINCTE du juge pour chaque paire.
    for (const YaoReceipt& receipt_item : receipts) {
      const int anchor_id = tree.order[(std::size_t)receipt_item.anchor_pos];
      std::vector<int> targets;
      if (receipt_item.target_pos >= 0) targets.push_back(receipt_item.target_pos);
      else
        for (int t = receipt_item.target_begin; t < receipt_item.target_end; ++t)
          targets.push_back(t);
      // La banque FACTORISEE du reçu : (ancre, chambre, version) — dix
      // identifiants references une seule fois, jamais recopies par noeud.
      if (receipt_item.bank_index < 0 ||
          receipt_item.bank_index >= (int)bank_table.size())
        return fail("le rejeu des reçus Yao48", "un reçu ne reference aucune banque");
      const BankTableEntry& bank_entry = bank_table[(std::size_t)receipt_item.bank_index];
      if (bank_entry.anchor_pos != receipt_item.anchor_pos)
        return fail("le rejeu des reçus Yao48",
                    "un reçu reference la banque d'une autre ancre");
      std::array<int, 10> ids = bank_entry.ids;
      std::sort(ids.begin(), ids.end());
      for (int i = 1; i < 10; ++i)
        if (ids[(std::size_t)i] == ids[(std::size_t)(i - 1)])
          return fail("le rejeu des reçus Yao48", "temoin duplique dans un reçu");
      for (int target_pos : targets) {
        const int target_id = tree.order[(std::size_t)target_pos];
        for (int witness_id : ids) {
          if (witness_id == anchor_id || witness_id == target_id)
            return fail("le rejeu des reçus Yao48", "temoin egal a une extremite");
          if (judge_phi_sign(pts[(std::size_t)witness_id], pts[(std::size_t)anchor_id],
                             pts[(std::size_t)target_id]) >= 0)
            return fail("le rejeu des reçus Yao48",
                        "un temoin de reçu n'est pas strictement interieur");
        }
      }
    }
    // 1 ter. Les reçus RADIAUX : chaque cible retrouve la banque de SA
    // chambre nominale parmi les banques listees, et chacun de ses dix
    // temoins est strictement interieur par l'arithmetique du juge.
    for (const mhgp3v::yao48::RadialReceipt& receipt_item : radial_receipts) {
      const int anchor_id = tree.order[(std::size_t)receipt_item.anchor_pos];
      const mhgp::P3& anchor_point = pts[(std::size_t)anchor_id];
      for (int t = receipt_item.target_begin; t < receipt_item.target_end; ++t) {
        const int target_id = tree.order[(std::size_t)t];
        const mhgp::P3& target_point = pts[(std::size_t)target_id];
        i64 canon[3];
        const int chamber = mhgp3v::yao48::chamber_of(
            (i64)target_point.x - anchor_point.x, (i64)target_point.y - anchor_point.y,
            (i64)target_point.z - anchor_point.z, canon, false);
        const BankTableEntry* found = nullptr;
        for (int bank_index : receipt_item.bank_indices) {
          if (bank_index < 0 || bank_index >= (int)bank_table.size())
            return fail("le rejeu des reçus radiaux", "index de banque invalide");
          const BankTableEntry& entry = bank_table[(std::size_t)bank_index];
          if (entry.chamber == chamber && entry.anchor_pos == receipt_item.anchor_pos) {
            found = &entry;
            break;
          }
        }
        if (found == nullptr)
          return fail("le rejeu des reçus radiaux",
                      "une cible n'a aucune banque pour sa chambre — l'enveloppe a"
                      " oublie une chambre possible");
        for (int witness_id : found->ids) {
          if (witness_id == anchor_id || witness_id == target_id)
            return fail("le rejeu des reçus radiaux", "temoin egal a une extremite");
          if (judge_phi_sign(pts[(std::size_t)witness_id], pts[(std::size_t)anchor_id],
                             pts[(std::size_t)target_id]) >= 0)
            return fail("le rejeu des reçus radiaux",
                        "un temoin radial n'est pas strictement interieur");
        }
      }
    }
    // 2. Le juge exhaustif : sort et census de CHAQUE paire.
    std::size_t census_cursor = 0;
    std::vector<CensusRecord> census_sorted = census;
    std::sort(census_sorted.begin(), census_sorted.end(),
              [&](const CensusRecord& a, const CensusRecord& b) {
                int a_lo = tree.order[(std::size_t)a.pos_low];
                int a_hi = tree.order[(std::size_t)a.pos_high];
                if (a_lo > a_hi) std::swap(a_lo, a_hi);
                int b_lo = tree.order[(std::size_t)b.pos_low];
                int b_hi = tree.order[(std::size_t)b.pos_high];
                if (b_lo > b_hi) std::swap(b_lo, b_hi);
                if (a_lo != b_lo) return a_lo < b_lo;
                return a_hi < b_hi;
              });
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) {
        i64 strict = 0, closed = 0;
        std::vector<int> judge_closed;
        for (int x = 0; x < n; ++x) {
          const int sign =
              judge_phi_sign(pts[(std::size_t)x], pts[(std::size_t)i], pts[(std::size_t)j]);
          if (sign < 0) { ++strict; ++closed; judge_closed.push_back(x); }
          else if (sign == 0) { ++closed; judge_closed.push_back(x); }
        }
        const i64 index = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
        const PairFate fate_value = fates[(std::size_t)index];
        // Le juge n'herite JAMAIS des injections : seuil NOMINAL — c'est lui
        // qui tue threshold-minus-one.
        const bool judge_tomb = strict >= kOrderK;
        if (judge_tomb != fate_is_tombstone(fate_value))
          return fail("le juge exhaustif", "un sort machine contredit le juge");
        if (fate_value == PairFate::kCensus) {
          // Le record census suivant dans l'ordre canonique doit etre
          // exactement cette paire, avec la meme liste fermee.
          if (census_cursor >= census_sorted.size())
            return fail("le juge exhaustif", "un record census manque");
          const CensusRecord& record = census_sorted[census_cursor++];
          int lo = tree.order[(std::size_t)record.pos_low];
          int hi = tree.order[(std::size_t)record.pos_high];
          if (lo > hi) std::swap(lo, hi);
          if (lo != i || hi != j)
            return fail("le juge exhaustif", "l'ordre canonique des census diverge");
          if (record.strict != strict || record.closed != (i64)judge_closed.size() ||
              record.closed_ids != judge_closed)
            return fail("le juge exhaustif",
                        "un census ferme differe de la liste du juge");
        }
      }
    if (census_cursor != census_sorted.size())
      return fail("le juge exhaustif", "des records census surnumeraires existent");
    // 3. L'EQUIVARIANCE PAR PERMUTATIONS (l'audit exige PLUSIEURS ordres) :
    // le renversement ET un melange LCG grave rendent les memes sorts apres
    // renommage.
    if (permute == 1) {
      std::vector<std::vector<int>> sigmas;
      {
        std::vector<int> reversed_sigma((std::size_t)n);
        for (int i = 0; i < n; ++i) reversed_sigma[(std::size_t)i] = n - 1 - i;
        sigmas.push_back(std::move(reversed_sigma));
        std::vector<int> shuffled((std::size_t)n);
        for (int i = 0; i < n; ++i) shuffled[(std::size_t)i] = i;
        unsigned long long lcg = 0x9E3779B97F4A7C15ULL ^ (unsigned long long)seed;
        for (int i = n - 1; i > 0; --i) {
          lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
          const int j = (int)(lcg % (unsigned long long)(i + 1));
          std::swap(shuffled[(std::size_t)i], shuffled[(std::size_t)j]);
        }
        sigmas.push_back(std::move(shuffled));
      }
      for (const std::vector<int>& sigma : sigmas) {
        std::vector<mhgp::P3> permuted((std::size_t)n);
        for (int i = 0; i < n; ++i)
          permuted[(std::size_t)i] = pts[(std::size_t)sigma[(std::size_t)i]];
        Lbvh tree2;
        tree2.build(permuted, leaf_size);
        std::vector<PairFate> fates2((std::size_t)all_pairs, PairFate::kUnassigned);
        YaoSource source2;
        source2.tree = &tree2;
        source2.injections = injections;
        source2.bank_pop_budget = bank_pops;
        source2.chamber_visits = chamber_visits;
        source2.antichain_banks = antichain == 1;
        source2.max_work = max_work;
        source2.fate = &fates2;
        source2.fate_points = n;
        if (!source2.run()) {
          std::printf("ECHEC : budget depasse dans la permutation\n");
          return 3;
        }
        for (int i = 0; i < n; ++i)
          for (int j = i + 1; j < n; ++j) {
            const i64 index2 = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
            int oi = sigma[(std::size_t)i], oj = sigma[(std::size_t)j];
            if (oi > oj) std::swap(oi, oj);
            const i64 oindex = (i64)oi * (2 * (i64)n - oi - 1) / 2 + (oj - oi - 1);
            if (fate_is_tombstone(fates2[(std::size_t)index2]) !=
                fate_is_tombstone(fates[(std::size_t)oindex]))
              return fail("l'equivariance par permutation",
                          "un sort depend de la numerotation des PointId");
          }
      }
    }
  }

  // L'INVARIANCE DES POLITIQUES DE TRAVAIL (exigence d'audit) : les budgets
  // minimal et ample rendent les MEMES sorts et les memes agregats de census
  // — la coupe est une acceleration, jamais une autorite. Les sorts sont
  // objectifs (dix stricts existent ou non) et le classifieur exact rattrape
  // toute coupe manquee : toute divergence est un defaut.
  if (policy_differential == 1) {
    std::vector<PairFate> fates_min((std::size_t)all_pairs, PairFate::kUnassigned);
    YaoSource source_min;
    source_min.tree = &tree;
    source_min.injections = injections;
    source_min.bank_pop_budget = 48;
    source_min.chamber_visits = 8;
    source_min.max_work = max_work;
    source_min.fate = &fates_min;
    source_min.fate_points = n;
    if (!source_min.run()) {
      std::printf("ECHEC : budget depasse dans la politique minimale\n");
      return 3;
    }
    if (source_min.receipt.census_records != r.census_records ||
        source_min.receipt.census_closed_total != r.census_closed_total ||
        source_min.receipt.census_strict_total != r.census_strict_total ||
        source_min.receipt.census_contact_total != r.census_contact_total)
      return fail("l'invariance des politiques",
                  "les agregats de census dependent du budget des banques");
    for (std::size_t k = 0; k < fates_min.size(); ++k)
      if (fate_is_tombstone(fates_min[k]) != fate_is_tombstone(fates[k]))
        return fail("l'invariance des politiques",
                    "un sort depend du budget des banques");
    // Le mode ANTICHAINE est une politique de travail : memes sorts exiges.
    std::vector<PairFate> fates_anti((std::size_t)all_pairs, PairFate::kUnassigned);
    YaoSource source_anti;
    source_anti.tree = &tree;
    source_anti.injections = injections;
    source_anti.bank_pop_budget = bank_pops;
    source_anti.chamber_visits = chamber_visits;
    source_anti.antichain_banks = true;
    source_anti.max_work = max_work;
    source_anti.fate = &fates_anti;
    source_anti.fate_points = n;
    if (!source_anti.run()) {
      std::printf("ECHEC : budget depasse dans la politique antichaine\n");
      return 3;
    }
    if (source_anti.receipt.census_records != r.census_records ||
        source_anti.receipt.census_closed_total != r.census_closed_total ||
        source_anti.receipt.census_strict_total != r.census_strict_total ||
        source_anti.receipt.census_contact_total != r.census_contact_total)
      return fail("l'invariance des politiques",
                  "les agregats de census dependent du mode de banque");
    for (std::size_t k = 0; k < fates_anti.size(); ++k)
      if (fate_is_tombstone(fates_anti[k]) != fate_is_tombstone(fates[k]))
        return fail("l'invariance des politiques",
                    "un sort depend du mode de banque (exact contre antichaine)");
    std::printf("politiques : sorts et census IDENTIQUES entre budgets minimal"
                " (48/8), ample (%lld/%lld) et mode antichaine\n", bank_pops,
                chamber_visits);
  }

  // LES PLANCHERS ANTI VERT-PAR-VACUITE.
  const auto floor_violated = [&](const char* what, i64 got, i64 want) {
    std::printf("ECHEC : plancher viole — %s=%lld < %lld\n", what, got, want);
    return 3;
  };
  if (min_region_prunes > 0 && r.region_prunes < min_region_prunes)
    return floor_violated("reçus-de-region", r.region_prunes, min_region_prunes);
  if (min_radial_prunes > 0 && r.radial_prunes < min_radial_prunes)
    return floor_violated("reçus-radiaux", r.radial_prunes, min_radial_prunes);
  if (min_point_tombstones > 0 && r.point_tombstones < min_point_tombstones)
    return floor_violated("tombstones-ponctuelles", r.point_tombstones,
                          min_point_tombstones);
  if (min_census_records > 0 && r.census_records < min_census_records)
    return floor_violated("records-census", r.census_records, min_census_records);
  if (min_underfull > 0 && r.underfull_chambers < min_underfull)
    return floor_violated("chambres-sous-pleines", r.underfull_chambers, min_underfull);
  if (min_survivors > 0 && r.survivors < min_survivors)
    return floor_violated("survivantes", r.survivors, min_survivors);
  if (min_classifier_tombs > 0 && r.classifier_tombstones < min_classifier_tombs)
    return floor_violated("tombstones-classifieur", r.classifier_tombstones,
                          min_classifier_tombs);

  // LES ASSERTIONS DE FIXTURE.
  if (is_fixture) {
    const auto fate_of = [&](int i, int j) {
      if (i > j) std::swap(i, j);
      const i64 index = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
      return fates[(std::size_t)index];
    };
    if (fixture_name == "directional-equality") {
      // Honnetete : l'egalite (x+y+z)^2 = 3D doit etre exacte et le temoin
      // (5,5,5) un contact exact de la paire (0,1).
      if (judge_phi_sign(pts[2], pts[0], pts[1]) != 0)
        return fail("la fixture directional-equality",
                    "le temoin parallele n'est plus un contact exact — fixture derivee");
      if (fate_of(0, 1) != PairFate::kCensus)
        return fail("la fixture directional-equality",
                    "la paire d'egalite exacte a ete tombstonee — l'egalite doit"
                    " descendre");
      if (r.point_tombstones != 0 || r.region_prunes != 0)
        return fail("la fixture directional-equality",
                    "une coupe a mordu alors que la seule paire coupable est en"
                    " egalite exacte");
    } else if (fixture_name == "region-prune") {
      if (r.region_prunes < 1)
        return fail("la fixture region-prune",
                    "aucun reçu de region — la coupe de boite ne mord plus");
      i64 cluster_tombs = 0;
      for (int k = 11; k < 19; ++k)
        if (fate_is_tombstone(fate_of(0, k))) ++cluster_tombs;
      if (cluster_tombs != 8)
        return fail("la fixture region-prune",
                    "les huit paires du cluster ne sont pas toutes tombstonees");
    } else if (fixture_name == "radial-straddle") {
      // LA BOITE A CHEVAL (audit de reemploi §4) : deux chambres denses
      // adjacentes (x>=y>=z et x>=z>=y, octant ---), pleines a D ~ 1526, et
      // un cluster lointain a cheval sur leur frontiere y=z a dist^2 ~
      // 160000 > 3*D — l'enveloppe radiale prune la boite entiere en UN
      // reçu, sans developper ses feuilles. Verite : les temoins des deux
      // banques sont profondement stricts pour ces paires alignees.
      pts = {pt(60000, 60000, 60000)};
      for (int k = 0; k < 10; ++k) pts.push_back(pt(60000 - (30 + k), 60000 - 2, 60000 - 1));
      for (int k = 0; k < 10; ++k) pts.push_back(pt(60000 - (30 + k), 60000 - 1, 60000 - 2));
      const int straddle[6][3] = {{400, 3, 2}, {400, 2, 3}, {401, 3, 2},
                                  {401, 2, 3}, {402, 3, 2}, {402, 2, 3}};
      for (const auto& o : straddle)
        pts.push_back(pt(60000 - o[0], 60000 - o[1], 60000 - o[2]));
    } else if (fixture_name == "radial-straddle") {
      if (r.radial_prunes < 1)
        return fail("la fixture radial-straddle",
                    "aucun reçu radial — la boite a cheval n'a pas ete prunee par"
                    " l'enveloppe 3*D_c");
      for (int k = 21; k < 27; ++k)
        if (!fate_is_tombstone(fate_of(0, k)))
          return fail("la fixture radial-straddle",
                      "une paire du cluster a cheval n'est pas tombstonee");
    } else if (fixture_name == "underfull") {
      if (r.underfull_chambers == 0 || r.region_prunes != 0 || r.point_tombstones != 0)
        return fail("la fixture underfull",
                    "le fail-open des banques sous-pleines n'est pas exerce");
      if (r.census_records != all_pairs)
        return fail("la fixture underfull",
                    "toutes les paires devraient finir en census");
    } else if (fixture_name == "nonconverse") {
      if (fate_of(0, 1) != PairFate::kCensus)
        return fail("la fixture nonconverse", "la paire (p,q) n'est pas un record");
      bool found = false;
      for (const CensusRecord& record : census) {
        int lo = tree.order[(std::size_t)record.pos_low];
        int hi = tree.order[(std::size_t)record.pos_high];
        if (lo > hi) std::swap(lo, hi);
        if (lo == 0 && hi == 1) {
          found = true;
          if (record.closed != 3 || record.strict != 0 || record.contacts != 1)
            return fail("la fixture nonconverse",
                        "le census de (p,q) doit etre ferme=3, stricts=0, contact=1");
        }
      }
      if (!found)
        return fail("la fixture nonconverse", "le record de (p,q) est introuvable");
    } else if (fixture_name == "u16-extremes") {
      if (!fate_is_tombstone(fate_of(0, 1)))
        return fail("la fixture u16-extremes",
                    "la paire diagonale n'est pas tombstonee malgre 18 stricts");
    } else if (fixture_name == "colocated") {
      if (r.region_prunes != 0 || r.point_tombstones != 0 ||
          r.classifier_tombstones != 0)
        return fail("la fixture colocated",
                    "une tombstone a ete fabriquee sur des points colocalises");
      if (r.census_records != all_pairs)
        return fail("la fixture colocated", "chaque paire colocalisee doit etre un"
                                            " record de rang n");
    }
  }

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : aucune porte n'a mordu\n");
    return 0;
  }

  const i64 tree_bytes =
      (i64)(tree.nodes.size() * sizeof(Node) + tree.order.size() * (sizeof(int) + 8));
  std::printf("arbre      : %zu noeuds, feuilles <= %d, %lld octets (cles Morton"
              " comprises)\n", tree.nodes.size(), leaf_size, tree_bytes);
  std::printf("banques    : pops=%lld visites=%lld cone-visites=%lld pleines=%lld"
              " sous-pleines=%lld (budget %lld par ancre, tas max=%lld)\n", r.bank_pops,
              r.bank_node_visits, r.bank_cone_visits, r.full_chambers,
              r.underfull_chambers, bank_pops, r.heap_high_water);
  std::printf("coupe      : visites=%lld reçus-region=%lld (masse %lld dont"
              " radiale %lld en %lld reçus) tombstones-point=%lld\n",
              r.prune_node_visits, r.region_prunes, r.region_pruned_mass,
              r.radial_pruned_mass, r.radial_prunes, r.point_tombstones);
  std::printf("classifieur: survivantes=%lld visites-lot=%lld boites=%lld tests=%lld"
              " liste=%lld (paires=%lld) tombstones=%lld census=%lld\n", r.survivors,
              r.classify_node_visits, r.classify_box_tests, r.classify_point_tests,
              r.classify_list_tests, r.classify_list_pairs, r.classifier_tombstones,
              r.census_records);
  std::printf("census     : fermes=%lld stricts=%lld contacts=%lld — pile max=%lld\n",
              r.census_closed_total, r.census_strict_total, r.census_contact_total,
              r.stack_high_water);
  std::printf("ledger     : %lld + %lld + %lld = %lld = C(n,2) — FERME (%.3f s de"
              " phase locale, 1 thread, pas un warm_e2e)\n", r.region_pruned_mass,
              r.point_tombstones, r.survivors, all_pairs, seconds);
  if (oracle == 1)
    std::printf("oracle     : reçus rejoues et juge exhaustif compare — aucun"
                " desaccord\n");
  if (is_fixture)
    std::printf("OK : fixture %s recue\n", fixture_name.c_str());
  else
    std::printf("OK : source q2 Yao48/LBVH count-only — le ledger est la sortie,"
                " aucune admission n'est prononcee\n");
  return 0;
}
