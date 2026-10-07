// mhgp12_mes_m3 : microbanc MES-M3 (LEV-MEB-CERT), hors produit. Plus petite boule PROPOSEE en flottant puis
// CERTIFIEE, contre la plus petite boule exacte exhaustive de la v11 (bounded_meb), sur les parties de descente videes
// par mhgp12_vidage (ordre_<k>.bin, section PARTS) et le catalogue vide (cat.bin).
//
// Voie nouvelle, pour une partie F (k sites) :
//   1. proposition : DWelzl de la v10 (welzl_proposal.hpp), qui ne decide rien ; support propose S (2 a 4 sites de F) ;
//   2. LEM-T1 corrige (constat CST-0101 de l'auditeur, 7 octobre) : si S est le S* d'une boule b du catalogue (table
//      S* -> boule) ET S dans F (inclusion d'indices de sites) ET F dans P_b, alors B(F) = b, sans arithmetique ;
//      un echec de S dans F renvoie au repli exact, jamais a un succes ;
//   3. sinon certificat exact du support propose : centre exact (num de la v11), coordonnees barycentriques
//      strictement positives (q2 milieu ; q3 triangle strictement aigu ; q4 tetraedre strictement interieur), cote
//      exact de tous les sites de F (<= 0) ; puis canonisation parmi les sites de F sur la sphere (support minimal,
//      premier dans l'ordre lexicographique des SiteIdx) et recherche de ce support dans la table ; si la table
//      l'ignore, la sphere est materialisee pour le census (comme la v11, qui recense alors la sphere) ;
//   4. sinon repli exact : bounded_meb (la reference elle-meme).
// Reference : bounded_meb (enumeration exhaustive exacte) puis recherche du support local canonique dans la table.
// Juge : IDENTITE de la sphere rendue (centre exact et niveau exact), du support local canonique et de
// l'identification au catalogue, sur toutes les parties ; un ecart sort en code 1, jamais en silence.
//
// Modes :
//   mhgp12_mes_m3 --porte                         temoins graves (dont WIT-T1-CARRE) ; code 0 conforme, 1 ecart
//   mhgp12_mes_m3 <dossier> [--repetitions R] [--ordres k1,k2,...] [--repere local|absolu]
//                 [--proposition welzl|mere_puis_welzl]
//                                                 banc sur un vidage ; JSON sur la sortie
// Le mutant mhgp12_mes_m3_mutant_sans_s_dans_f (MHGP12_MUTANT_T1_SANS_S_DANS_F) retire le test S dans F : la porte
// doit le tuer (code 1 sur WIT-T1-CARRE).
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
#include <span>
#include <string>
#include <vector>

#include "meb_cert.hpp"

using namespace mhgp11;

namespace mhgp12 {
namespace {
namespace d = ::mhgp12::dump;
using Clock = std::chrono::steady_clock;

using namespace ::mhgp12::mebcert;

// ---- Porte : temoins graves -----------------------------------------------------------------------------------------
struct Witness {
  const char* name;
  std::vector<char> part;      // lettres des sites de F
  std::vector<char> proposal;  // proposition forcee (vide : DWelzl)
  int expected_route;          // -1 : identite seule
  int expected_why;            // -1 : sans objet
};

int run_gate() {
  // Sites temoins : carre ABCD (WIT-T1-CARRE, constat CST-0101), point lointain E, triangle aigu PQR, tetraedre
  // regulier WXYZ (q4), et les boules du mini-catalogue : diametre AC (coquille ABCD, q2 etendue), diametre AB,
  // cercle PQR (q3), sphere WXYZ (q4).
  const std::map<char, std::array<u32, 3>> coords = {
      {'A', {0, 0, 0}},   {'B', {2, 0, 0}},   {'C', {2, 2, 0}},   {'D', {0, 2, 0}},  {'E', {9, 9, 9}},
      {'P', {20, 0, 0}},  {'Q', {24, 0, 0}},  {'R', {22, 3, 0}},  {'W', {40, 0, 0}}, {'X', {42, 2, 0}},
      {'Y', {42, 0, 2}},  {'Z', {40, 2, 2}}};
  MemoryBudget budget(u64{1} << 30);
  std::vector<u32> x, y, z;
  std::vector<PointId> pid;
  std::vector<char> letters;
  for (const auto& [letter, xyz] : coords) {
    letters.push_back(letter);
    x.push_back(xyz[0]);
    y.push_back(xyz[1]);
    z.push_back(xyz[2]);
    pid.push_back(PointId{static_cast<u32>(pid.size())});
  }
  auto cloud = prepare_cloud(x, y, z, pid, CoordWidth(), budget);
  if (!cloud.ok()) return 2;
  const Cloud& cl = cloud.value();
  std::map<char, u32> site;  // lettre -> SiteIdx (ordre de Morton)
  for (u32 s = 0; s < cl.sites(); ++s)
    for (const auto& [letter, xyz] : coords)
      if (cl.x()[s] == xyz[0] && cl.y()[s] == xyz[1] && cl.z()[s] == xyz[2]) site[letter] = s;
  std::vector<num::Point> points(cl.sites());
  for (u32 s = 0; s < cl.sites(); ++s) points[s] = num::Point::make(cl.x()[s], cl.y()[s], cl.z()[s]).value();
  // Mini-catalogue : chaque boule recensee en exact sur tout le nuage temoin, S* canonique par canonical_support.
  const std::vector<std::vector<char>> generators = {{'A', 'C'}, {'A', 'B'}, {'P', 'Q', 'R'}, {'W', 'X', 'Y', 'Z'}};
  std::vector<d::BallRec> rec;
  std::vector<u64> off{0};
  std::vector<u32> val;
  for (const auto& g : generators) {
    std::array<num::Point, 4> gp{};
    for (std::size_t i = 0; i < g.size(); ++i) gp[i] = points[site[g[i]]];
    auto made = g.size() == 2 ? num::Sphere::through(gp[0], gp[1])
                : g.size() == 3 ? num::Sphere::through(gp[0], gp[1], gp[2])
                                : num::Sphere::through(gp[0], gp[1], gp[2], gp[3]);
    if (!made.ok() || !made.value()) return 2;
    const num::Sphere sphere = *made.value();
    std::vector<u32> inner, shell;
    std::array<num::Point, kMaxPart> zp{};
    for (u32 s = 0; s < cl.sites(); ++s) {
      const int side = num::side(sphere, points[s]).value();
      if (side < 0) inner.push_back(s);
      if (side == 0) shell.push_back(s);
    }
    for (std::size_t i = 0; i < shell.size(); ++i) zp[i] = points[shell[i]];
    std::array<u32, 4> sstar{d::kNone, d::kNone, d::kNone, d::kNone};
    const int q = canonical_support(sphere, shell.data(), zp.data(), static_cast<u32>(shell.size()), sstar);
    if (q < 2) return 2;
    d::BallRec r{};
    r.rank = static_cast<u32>(rec.size()) + 1;
    r.p = static_cast<u32>(inner.size());
    r.m = static_cast<u32>(shell.size());
    r.q = static_cast<u32>(q);
    for (int i = 0; i < 4; ++i) r.sstar[i] = sstar[i];
    rec.push_back(r);
    val.insert(val.end(), inner.begin(), inner.end());
    val.insert(val.end(), shell.begin(), shell.end());
    off.push_back(val.size());
  }
  Cat cat;
  cat.sites = cl.sites();
  cat.balls = static_cast<u32>(rec.size());
  cat.rec = rec.data();
  cat.off = off.data();
  cat.val = val.data();
  SupportTable table;
  table.build(rec.data(), cat.balls);
  const Ctx c{cl, cat, table, points};
  // S*(boule AC) : la paire antipodale canonique du carre ; l'autre paire sert de proposition non canonique.
  const std::array<u32, 4> square = {rec[0].sstar[0], rec[0].sstar[1], d::kNone, d::kNone};
  std::vector<char> canonical_pair, other_pair;
  for (const auto& [letter, s] : site)
    if (s == square[0] || s == square[1]) canonical_pair.push_back(letter);
  other_pair = (canonical_pair == std::vector<char>{'A', 'C'}) ? std::vector<char>{'B', 'D'}
                                                               : std::vector<char>{'A', 'C'};
  const std::vector<Witness> witnesses = {
      // WIT-T1-CARRE : S = S*(boule de diametre AC) n'est pas dans F = {A, B} ; F est dans P_b. Sans le test S dans F,
      // LEM-T1 rendrait la boule de rayon carre 2 ; la plus petite boule de F a le rayon carre 1.
      {"WIT-T1-CARRE", {'A', 'B'}, canonical_pair, kFallbackTable, kSNotInF},
      {"carre_diagonale", {'A', 'C'}, {}, -1, -1},
      {"carre_triangle_droit", {'A', 'B', 'C'}, {}, -1, -1},
      {"carre_entier_proposition_non_canonique", {'A', 'B', 'C', 'D'}, other_pair, kCertTable, -1},
      {"carre_entier_welzl", {'A', 'B', 'C', 'D'}, {}, -1, -1},
      {"site_exterieur", {'A', 'C', 'E'}, {'A', 'C'}, -1, kCertOutside},
      {"triangle_droit_force", {'A', 'B', 'C'}, {'A', 'B', 'C'}, -1, kCertNotStrict},
      {"triangle_aigu_t1", {'P', 'Q', 'R'}, {}, kT1, -1},
      {"tetraedre_t1", {'W', 'X', 'Y', 'Z'}, {}, kT1, -1},
      {"hors_catalogue", {'A', 'E'}, {}, kCertCensus, -1},
  };
  int failures = 0;
  for (const auto& w : witnesses) {
    Part f;
    for (char letter : w.part) f.id[f.k++] = site[letter];
    std::sort(f.id.begin(), f.id.begin() + f.k);
    std::array<num::Point, kMaxPart> fp{};
    for (u32 i = 0; i < f.k; ++i) fp[i] = points[f.id[i]];
    NewOut n;
    if (w.proposal.empty()) {
      n = new_path(c, f);
    } else {
      std::array<u32, 4> s{d::kNone, d::kNone, d::kNone, d::kNone};
      for (std::size_t i = 0; i < w.proposal.size(); ++i) s[i] = site[w.proposal[i]];
      n = certify(c, f, fp.data(), s, static_cast<int>(w.proposal.size()));
      if (n.route == kNeedsFallback) n = fallback(c, f, n.why);
    }
    const RefOut r = reference(c, f);
    const Verdict v = judge(c, f, n, r);
    const bool route_ok = w.expected_route < 0 || n.route == w.expected_route;
    const bool why_ok = w.expected_why < 0 || n.why == w.expected_why;
    const bool pass = v.sphere && v.ball && v.support && route_ok && why_ok;
    failures += !pass;
    std::cout << "{\"temoin\":\"" << w.name << "\",\"route\":\"" << kRouteNames[n.route] << "\",\"raison\":\""
              << kWhyNames[n.why] << "\",\"sphere_identique\":" << (v.sphere ? "true" : "false")
              << ",\"boule_identique\":" << (v.ball ? "true" : "false")
              << ",\"support_identique\":" << (v.support ? "true" : "false") << ",\"route_attendue\":"
              << (route_ok ? "true" : "false") << ",\"raison_attendue\":" << (why_ok ? "true" : "false")
              << ",\"conforme\":" << (pass ? "true" : "false") << "}\n";
  }
  std::cout << "{\"porte\":\"mes_m3\",\"mutant_sans_s_dans_f\":" << (kMutantSansSDansF ? "true" : "false")
            << ",\"temoins\":" << witnesses.size() << ",\"ecarts\":" << failures << "}\n";
  return failures == 0 ? 0 : 1;
}

// ---- Banc sur un vidage ---------------------------------------------------------------------------------------------
struct OrderParts {
  u32 k = 0;
  std::vector<Part> parts;
  std::vector<d::PartRec> info;
};

double seconds(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

template <class Run>
double best_of(int repetitions, Run&& run) {
  double best = 1e300;
  for (int r = 0; r < repetitions; ++r) {
    const auto t0 = Clock::now();
    run();
    best = std::min(best, seconds(t0));
  }
  return best;
}

volatile u64 g_sink = 0;

int run_bench(const std::string& dir, int repetitions, const std::vector<u32>& only, bool absolute_frame,
              bool mother_first) {
  const d::Reader cat_file(dir + "/cat.bin");
  if (cat_file.header().kind != d::kCatalogue) throw std::runtime_error("cat.bin : genre inattendu");
  const u32 kmax = cat_file.header().kmax;
  Cat cat;
  {
    const auto [xyz, n] = cat_file.get<u32>("SITEXYZ", 12);
    cat.xyz = xyz;
    cat.sites = static_cast<u32>(n);
    const auto [rec, nb] = cat_file.get<d::BallRec>("BALLS");
    cat.rec = rec;
    cat.balls = static_cast<u32>(nb);
    const auto [off, no] = cat_file.get<u64>("POPOFF");
    if (no != nb + 1) throw std::runtime_error("cat.bin : POPOFF");
    cat.off = off;
    const auto [val, nv] = cat_file.get<u32>("POPVAL");
    if (nv != off[nb]) throw std::runtime_error("cat.bin : POPVAL");
    cat.val = val;
  }
  // Nuage v11 refait depuis les sites vides (ordre de Morton deja trie : la permutation doit etre l'identite).
  MemoryBudget budget(u64{8} << 30);
  std::vector<u32> x(cat.sites), y(cat.sites), z(cat.sites);
  std::vector<PointId> ids(cat.sites);
  for (u32 s = 0; s < cat.sites; ++s) {
    x[s] = cat.xyz[3 * u64{s}];
    y[s] = cat.xyz[3 * u64{s} + 1];
    z[s] = cat.xyz[3 * u64{s} + 2];
    ids[s] = PointId{s};
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) throw std::runtime_error("prepare_cloud refuse");
  const Cloud& cl = cloud.value();
  if (cl.sites() != cat.sites) throw std::runtime_error("nuage : nombre de sites");
  for (u32 s = 0; s < cat.sites; ++s)
    if (cl.x()[s] != x[s] || cl.y()[s] != y[s] || cl.z()[s] != z[s])
      throw std::runtime_error("nuage : ordre des sites different du vidage");
  std::vector<num::Point> points(cat.sites);
  for (u32 s = 0; s < cat.sites; ++s) points[s] = num::Point::make(x[s], y[s], z[s]).value();
  SupportTable table;
  table.build(cat.rec, cat.balls);
  const Ctx c{cl, cat, table, points, absolute_frame};

  std::cout << "{\"phase\":\"entree\",\"trame\":\"" << cat_file.frame() << "\",\"K\":" << kmax
            << ",\"sites\":" << cat.sites << ",\"boules\":" << cat.balls << ",\"repetitions\":" << repetitions
            << ",\"mutant_sans_s_dans_f\":" << (kMutantSansSDansF ? "true" : "false") << ",\"repere\":\""
            << (absolute_frame ? "absolu" : "local") << "\",\"proposition\":\""
            << (mother_first ? "mere_puis_welzl" : "welzl") << "\"}\n" << std::flush;
  int exit_code = 0;
  for (u32 k = 2; k <= kmax; ++k) {
    if (!only.empty() && std::find(only.begin(), only.end(), k) == only.end()) continue;
    const d::Reader order(dir + "/ordre_" + std::to_string(k) + ".bin");
    if (order.header().kind != d::kOrder || order.header().order != k) throw std::runtime_error("ordre : en-tete");
    const auto [raw, np] = order.get<u32>("PARTS", 4 * k);
    const auto [info, ni] = order.get<d::PartRec>("PARTINF");
    if (ni != np) throw std::runtime_error("ordre : PARTS et PARTINF");
    std::vector<Part> parts(np);
    for (u64 i = 0; i < np; ++i) {
      parts[i].k = k;
      for (u32 j = 0; j < k; ++j) parts[i].id[j] = raw[i * k + j];
    }
    // Mere de chaque partie (variante mere_puis_welzl) : boule de la cellule pour la premiere partie d'une trace, B de
    // la partie precedente (kNone hors catalogue) sinon.
    std::vector<u32> mother(np, d::kNone);
    if (mother_first) {
      const auto cells_s = order.get<d::CellRec>("CELLS");
      const auto cell_off_s = order.get<u64>("CELLOFF");
      const auto part_off_s = order.get<u64>("PARTOFF");
      const d::CellRec* cells = cells_s.first;
      const u64* cell_off = cell_off_s.first;
      const u64* part_off = part_off_s.first;
      if (part_off[part_off_s.second - 1] != np) throw std::runtime_error("PARTOFF incoherent");
      for (u64 j = 0; j < cells_s.second; ++j)
        for (u64 t = cell_off[j]; t < cell_off[j + 1]; ++t)
          for (u64 q = part_off[t]; q < part_off[t + 1]; ++q) mother[q] = q == part_off[t] ? cells[j].ball : info[q - 1].ball;
    }
    auto path = [&](u64 i, bool& rejected) -> NewOut {
      if (mother_first) return new_path_mother(c, parts[i], mother[i], rejected);
      rejected = false;
      return new_path(c, parts[i]);
    };
    // Passe de jugement (hors chronometre) : routes, identites, cas limites.
    std::array<u64, kRouteCount> routes{};
    std::array<u64, kWhyCount> whys{};
    u64 sphere_bad = 0, ball_bad = 0, support_bad = 0, dump_bad = 0, canonicalized = 0, cospherical = 0;
    u64 ref_table = 0, ref_presentations = 0, sphere_in_cat = 0, extended_in_table = 0, t1_extended = 0;
    std::array<std::vector<u32>, kRouteCount> by_route;
    u64 mother_rejected = 0;
    for (u64 i = 0; i < np; ++i) {
      bool rejected = false;
      const NewOut n = path(i, rejected);
      mother_rejected += rejected;
      const RefOut r = reference(c, parts[i]);
      const Verdict v = judge(c, parts[i], n, r);
      ++routes[n.route];
      ++whys[n.why];
      by_route[n.route].push_back(static_cast<u32>(i));
      sphere_bad += !v.sphere;
      ball_bad += !v.ball;
      support_bad += !v.support;
      canonicalized += n.canonicalized;
      cospherical += v.shell_in_f > r.arity;
      ref_presentations += r.presentations;
      if (r.ball != d::kNone) {
        ++ref_table;
        if (cat.rec[r.ball].m > cat.rec[r.ball].q) ++extended_in_table;
        if (n.route == kT1 && cat.rec[r.ball].m > cat.rec[r.ball].q) ++t1_extended;
      }
      // Coherence avec le vidage : la reference trouve S*(B(F)) dans la table <=> sstar_in_f ; meme boule.
      const bool dumped = info[i].sstar_in_f == 1;
      if (dumped != (r.ball != d::kNone) || (dumped && info[i].ball != r.ball)) ++dump_bad;
      sphere_in_cat += info[i].ball != d::kNone;
    }
    const bool identical = sphere_bad == 0 && ball_bad == 0 && support_bad == 0 && dump_bad == 0;
    if (!identical) exit_code = 1;
    // Parties distinctes.
    u64 distinct = 0;
    {
      std::vector<u32> order_idx(np);
      std::iota(order_idx.begin(), order_idx.end(), 0u);
      std::sort(order_idx.begin(), order_idx.end(), [&](u32 a, u32 b) {
        return std::lexicographical_compare(parts[a].id.begin(), parts[a].id.begin() + k, parts[b].id.begin(),
                                            parts[b].id.begin() + k);
      });
      for (u64 i = 0; i < np; ++i)
        if (i == 0 || !std::equal(parts[order_idx[i]].id.begin(), parts[order_idx[i]].id.begin() + k,
                                  parts[order_idx[i - 1]].id.begin()))
          ++distinct;
    }
    // Chronometrage a un fil : passes entrelacees, minimum par bras ; puis par route.
    double t_ref = 1e300, t_new = 1e300;
    for (int r = 0; r < repetitions; ++r) {
      auto t0 = Clock::now();
      u64 sink = 0;
      for (const auto& f : parts) sink += reference(c, f).ball;
      t_ref = std::min(t_ref, seconds(t0));
      g_sink = g_sink + sink;
      t0 = Clock::now();
      sink = 0;
      for (u64 i = 0; i < np; ++i) {
        bool rejected = false;
        const NewOut n = path(i, rejected);
        sink += n.ball + n.sink;
      }
      t_new = std::min(t_new, seconds(t0));
      g_sink = g_sink + sink;
    }
    std::array<double, kRouteCount> route_ref{}, route_new{};
    for (u32 rt = 0; rt < kRouteCount; ++rt) {
      const auto& list = by_route[rt];
      if (list.empty()) continue;
      route_ref[rt] = best_of(repetitions, [&]() {
        u64 sink = 0;
        for (u32 i : list) sink += reference(c, parts[i]).ball;
        g_sink = g_sink + sink;
      });
      route_new[rt] = best_of(repetitions, [&]() {
        u64 sink = 0;
        for (u32 i : list) {
          bool rejected = false;
          const NewOut n = path(i, rejected);
          sink += n.ball + n.sink;
        }
        g_sink = g_sink + sink;
      });
    }
    const double nps = double(np);
    std::cout << "{\"phase\":\"ordre\",\"k\":" << k << ",\"parties\":" << np << ",\"parties_distinctes\":" << distinct
              << ",\"identite\":{\"sphere_ecarts\":" << sphere_bad << ",\"boule_ecarts\":" << ball_bad
              << ",\"support_ecarts\":" << support_bad << ",\"vidage_ecarts\":" << dump_bad
              << ",\"identiques\":" << (identical ? "true" : "false") << "},\"routes\":{";
    for (u32 rt = 0; rt < kRouteCount; ++rt) std::cout << (rt ? "," : "") << '"' << kRouteNames[rt] << "\":" << routes[rt];
    std::cout << "},\"raisons\":{";
    for (u32 w = 1; w < kWhyCount; ++w) std::cout << (w > 1 ? "," : "") << '"' << kWhyNames[w] << "\":" << whys[w];
    std::cout << "},\"parts\":{\"t1\":" << (np ? routes[kT1] / nps : 0.0)
              << ",\"sphere_au_catalogue\":" << (np ? sphere_in_cat / nps : 0.0)
              << ",\"t1_eligible_test_complet\":" << (np ? ref_table / nps : 0.0)
              << ",\"sans_arithmetique_exacte\":" << (np ? routes[kT1] / nps : 0.0) << "}"
              << ",\"comptes\":{\"sphere_au_catalogue\":" << sphere_in_cat << ",\"t1_eligible_test_complet\":" << ref_table
              << ",\"coquille_etendue_au_catalogue\":" << extended_in_table << ",\"t1_coquille_etendue\":" << t1_extended
              << ",\"proposition_non_canonique\":" << canonicalized << ",\"cospheriques_en_plus\":" << cospherical
              << ",\"presentations_reference\":" << ref_presentations
              << ",\"mere_ecartee_par_s_dans_f\":" << mother_rejected << "}"
              << ",\"temps_un_fil_s\":{\"reference\":" << t_ref << ",\"voie_nouvelle\":" << t_new
              << ",\"rapport\":" << (t_ref > 0 ? t_new / t_ref : 0.0)
              << ",\"ns_par_partie_reference\":" << (np ? 1e9 * t_ref / nps : 0.0)
              << ",\"ns_par_partie_voie_nouvelle\":" << (np ? 1e9 * t_new / nps : 0.0) << ",\"par_route\":{";
    bool first = true;
    for (u32 rt = 0; rt < kRouteCount; ++rt) {
      if (by_route[rt].empty()) continue;
      const double m = double(by_route[rt].size());
      std::cout << (first ? "" : ",") << '"' << kRouteNames[rt] << "\":{\"parties\":" << by_route[rt].size()
                << ",\"ns_reference\":" << 1e9 * route_ref[rt] / m << ",\"ns_voie_nouvelle\":" << 1e9 * route_new[rt] / m
                << "}";
      first = false;
    }
    std::cout << "}}}\n" << std::flush;
  }
  std::cout << "{\"phase\":\"fin\",\"code\":" << exit_code << "}\n";
  return exit_code;
}

}  // namespace
}  // namespace mhgp12

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--porte") return mhgp12::run_gate();
    if (argc < 2) {
      std::cerr << "usage : mhgp12_mes_m3 --porte | <dossier> [--repetitions R] [--ordres k1,k2,...]"
                   " [--repere local|absolu] [--proposition welzl|mere_puis_welzl]\n";
      return 2;
    }
    int repetitions = 3;
    bool absolute_frame = false, mother_first = false;
    std::vector<mhgp11::u32> only;
    for (int i = 2; i < argc; ++i) {
      const std::string opt = argv[i];
      if (opt == "--repetitions" && i + 1 < argc) {
        repetitions = std::max(1, std::atoi(argv[++i]));
      } else if (opt == "--repere" && i + 1 < argc) {
        const std::string frame = argv[++i];
        if (frame != "local" && frame != "absolu") {
          std::cerr << "repere : local ou absolu\n";
          return 2;
        }
        absolute_frame = frame == "absolu";
      } else if (opt == "--proposition" && i + 1 < argc) {
        const std::string kind = argv[++i];
        if (kind != "welzl" && kind != "mere_puis_welzl") {
          std::cerr << "proposition : welzl ou mere_puis_welzl\n";
          return 2;
        }
        mother_first = kind == "mere_puis_welzl";
      } else if (opt == "--ordres" && i + 1 < argc) {
        const std::string list = argv[++i];
        std::size_t at = 0;
        while (at <= list.size()) {
          const std::size_t comma = list.find(',', at);
          only.push_back(static_cast<mhgp11::u32>(std::atoi(list.substr(at, comma - at).c_str())));
          if (comma == std::string::npos) break;
          at = comma + 1;
        }
      } else {
        std::cerr << "option inconnue : " << opt << "\n";
        return 2;
      }
    }
    return mhgp12::run_bench(argv[1], repetitions, only, absolute_frame, mother_first);
  } catch (const std::exception& e) {
    std::cout << "{\"phase\":\"exception\",\"message\":\"" << e.what() << "\"}\n";
    return 3;
  }
}
