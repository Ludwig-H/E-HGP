// MorseHGP3D v3 — WSPD PAR VAGUES : le prototype dont le kernel GPU sera la
// transcription directe. Aucune recursion, aucune pile, aucune file par thread.
//
// Codes de sortie : 1 desaccord du juge, 2 refus avant calcul, 3 plancher ou
// invariant, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include <chrono>

#include "rect_front.hpp"
#include "wspd_front.hpp"
#include "wspd_wavefront.hpp"

namespace {

using mhgp3v::RectVerdict;
using mhgp3v::WfNode;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS: %s\n", why);
  std::exit(2);
}

long long arg_ll(const char* s, long long lo, long long hi, const char* name) {
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') refuse(name);
  if (v < lo || v > hi) refuse(name);
  return v;
}

struct Pair { int a, b; };

// Banque Morton bornee, fusionnee a l'emission : des qu'une paire est declaree
// terminale, le MEME thread calcule son `Dlo`, lit sa fenetre Morton et
// applique le masque central. Le rectangle n'est jamais materialise en memoire ;
// seuls les residuels sont compactes. C'est le gain de bande passante du
// kernel vise.
struct BankStat { long long reads = 0, recerts = 0, tronques = 0, juges = 0, faux = 0, closed[3] = {0, 0, 0}; };

// ---- PROPOSITION PAR DESCENTE, alternative a la fenetre Morton.
//
// La fenetre Morton souffre de la DISCONTINUITE de la courbe : deux points
// spatialement voisins peuvent avoir des cles tres eloignees, si bien qu'aucun
// bon temoin n'entre dans la fenetre. Une descente au meilleur d'abord dans
// l'arbre DEJA CONSTRUIT n'a pas ce defaut : elle coute `O(log n + L)`, ne
// demande aucune structure supplementaire, et reste bornee.
//
// Sur GPU c'est une pile de taille fixe en registres — pas de file dynamique,
// pas d'allocation. Le budget d'expansions est le meme que celui de la fenetre.
long long box_dist2_to(const WfNode& v, const long long m4[3], bool tight) {
  long long s = 0;
  for (int i = 0; i < 3; ++i) {
    const long long lo4 = 4 * (tight ? v.tlo[i] : v.lo[i]);
    const long long hi4 = 4 * (tight ? v.thi[i] : v.hi[i]);
    long long d = 0;
    if (m4[i] < lo4) d = lo4 - m4[i];
    else if (m4[i] > hi4) d = m4[i] - hi4;
    s += d * d;
  }
  return s;
}

// Cellule de Morton (porte la borne) ou boite serree (front bien plus petit).
bool g_tight = false;
bool g_bank = false;
long long g_win = 32, g_bankl = 16;
long long g_warms = 0;
long long g_inflation = 0;
bool g_descent = false;
bool g_vwave = false;
bool g_inject_global = false;
bool g_judge_vwave = false;

// Cellule d'un identifiant de nœud : negatif = feuille (le point lui-meme).
mhgp3v::WspdBox cell_of(const std::vector<WfNode>& nodes,
                        const std::vector<std::array<long long, 3>>& pts, int id) {
  mhgp3v::WspdBox w{};
  if (id < 0) {
    const auto& p = pts[-1 - id];
    for (int i = 0; i < 3; ++i) { w.lo[i] = p[i]; w.hi[i] = p[i]; }
  } else if (g_tight) {
    for (int i = 0; i < 3; ++i) { w.lo[i] = nodes[id].tlo[i]; w.hi[i] = nodes[id].thi[i]; }
  } else {
    for (int i = 0; i < 3; ++i) { w.lo[i] = nodes[id].lo[i]; w.hi[i] = nodes[id].hi[i]; }
  }
  return w;
}

long long count_of(const std::vector<WfNode>& nodes, int id) {
  return (id < 0) ? 1 : (nodes[id].last - nodes[id].first + 1);
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  std::vector<long long> ns;
  long long coord = 65535, p = 2, q = 1, seed = 12345;
  bool oracle = false;
  double max_slope = 1.35;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* pre) { return a.substr(std::strlen(pre)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) {
      std::string s = val("--points="); size_t o = 0;
      while (o < s.size()) { size_t c = s.find(',', o);
        if (c == std::string::npos) c = s.size();
        ns.push_back(arg_ll(s.substr(o, c - o).c_str(), 8, 100000000LL, "points")); o = c + 1; }
    }
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 8, 65535, "coord");
    else if (a.rfind("--sep-euclid=", 0) == 0) {
      const std::string v = val("--sep-euclid=");
      const size_t sl = v.find('/');
      if (sl == std::string::npos) refuse("sep-euclid attend p/q");
      p = arg_ll(v.substr(0, sl).c_str(), 1, 64, "sep p");
      q = arg_ll(v.substr(sl + 1).c_str(), 1, 16, "sep q");
    }
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, (1LL << 40), "seed");
    else if (a.rfind("--max-slope=", 0) == 0) max_slope = std::atof(val("--max-slope=").c_str());
    else if (a == "--oracle") oracle = true;
    else if (a == "--tight") g_tight = true;
    else if (a == "--bank") g_bank = true;
    else if (a == "--inject=masque-global") { g_bank = true; g_vwave = true; g_inject_global = true; g_judge_vwave = true; }
    else if (a == "--judge-vwave") { g_bank = true; g_vwave = true; g_judge_vwave = true; }
    else if (a == "--vwave") { g_bank = true; g_vwave = true; }
    else if (a == "--descent") { g_bank = true; g_descent = true; }
    else if (a.rfind("--window=", 0) == 0) { g_win = arg_ll(val("--window=").c_str(), 2, 1024, "window"); g_bank = true; }
    else if (a.rfind("--bank-l=", 0) == 0) { g_bankl = arg_ll(val("--bank-l=").c_str(), 1, 64, "bank-l"); g_bank = true; }
    else if (a.rfind("--inflation=", 0) == 0) g_inflation = arg_ll(val("--inflation=").c_str(), 1, 20000, "inflation");
    else if (a.rfind("--warms=", 0) == 0) g_warms = arg_ll(val("--warms=").c_str(), 1, 200, "warms");
    else refuse("option inconnue");
  }
  if (ns.empty()) ns = {4000, 16000};

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho") fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue");
  if (oracle && ns.back() > 64) refuse("l'oracle exige n <= 64");

  std::vector<double> fronts;
  for (size_t k = 0; k < ns.size(); ++k) {
    const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, (int)ns[k], (int)coord, seed);
    std::vector<std::array<long long, 3>> pts;
    for (const mhgp::P3& pt : cloud) pts.push_back({pt.x, pt.y, pt.z});
    // Tri Morton avec depart par PointId : l'ordre est UNIQUE, donc l'arbre
    // aussi, donc les `RectId` sont identiques sur CPU et sur device.
    std::vector<int> pid(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) pid[i] = (int)i;
    std::sort(pid.begin(), pid.end(), [&](int u, int v) {
      const unsigned long long ku = mhgp3v::wf_morton48(pts[u][0], pts[u][1], pts[u][2]);
      const unsigned long long kv = mhgp3v::wf_morton48(pts[v][0], pts[v][1], pts[v][2]);
      if (ku != kv) return ku < kv;
      return u < v;
    });
    std::vector<std::array<long long, 3>> sp(pts.size());
    std::vector<int> spid(pts.size());
    for (size_t i = 0; i < pid.size(); ++i) { sp[i] = pts[pid[i]]; spid[i] = pid[i]; }
    std::vector<unsigned long long> keys(sp.size());
    for (size_t i = 0; i < sp.size(); ++i) keys[i] = mhgp3v::wf_morton48(sp[i][0], sp[i][1], sp[i][2]);
    {
      std::vector<unsigned long long> s2 = keys;
      if (std::adjacent_find(s2.begin(), s2.end()) != s2.end())
        refuse("codes de Morton coincidents : positions dupliquees");
    }
    // ---- CHRONO. Le contrat ne se decide pas sur des comptages. On mesure
    // l'arbre puis la vague fusionnee, sur `warms` repetitions, et on publie la
    // MEDIANE et le p95 — jamais la meilleure.
    using clk = std::chrono::steady_clock;
    std::vector<double> t_tree, t_wave;
    // L'arbre est repete `warms` fois ; la vague, une fois par execution du
    // probe. Le `p95` de la vague s'obtient en repetant le PROBE, ce qui mesure
    // aussi le cout froid — c'est plus honnete qu'une boucle chaude interne.
    const long long warms = std::max(1LL, g_warms);
    std::vector<WfNode> nodes;
    for (long long w = 0; w < warms; ++w) {
      const auto a0 = clk::now();
      nodes = mhgp3v::wf_build(keys);
      mhgp3v::wf_tight_boxes(&nodes, sp);
      t_tree.push_back(std::chrono::duration<double, std::milli>(clk::now() - a0).count());
    }
    const auto w0 = clk::now();
    const long long m = (long long)sp.size();

    // ---- GRAINES : le cas diagonal DEROULE. Un thread par nœud interne.
    std::vector<Pair> wave;
    wave.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) wave.push_back({nodes[i].left, nodes[i].right});

    // ---- VAGUES : `count -> scan -> fill`, aucune pile.
    std::vector<Pair> terms;
    BankStat bank;
    std::vector<char> closed_q2;   // par terminal : la banque a-t-elle ferme q2 ?
    // LA FRACTION DE RECORDS N'EST PAS LA FRACTION DE MASSE, et j'ai publie
    // l'une pour l'autre. On compte donc la masse fermee explicitement.
    long long mass_closed_q2 = 0;
    long long tests = 0, levels = 0, wave_hwm = (long long)wave.size();
    while (!wave.empty()) {
      ++levels;
      std::vector<int> cnt(wave.size());
      std::vector<char> sep(wave.size());
      for (size_t i = 0; i < wave.size(); ++i) {
        ++tests;
        const mhgp3v::WspdBox ca = cell_of(nodes, sp, wave[i].a);
        const mhgp3v::WspdBox cb = cell_of(nodes, sp, wave[i].b);
        sep[i] = mhgp3v::wspd_separated_euclid(ca, cb, p, q) ? 1 : 0;
        if (sep[i]) { cnt[i] = 0; continue; }
        const bool la = wave[i].a < 0, lb = wave[i].b < 0;
        cnt[i] = (la && lb) ? 0 : 2;    // deux feuilles non separables : terminal force
      }
      std::vector<int> off(wave.size() + 1, 0);
      for (size_t i = 0; i < wave.size(); ++i) off[i + 1] = off[i] + cnt[i];
      std::vector<Pair> next(off.back());
      for (size_t i = 0; i < wave.size(); ++i) {
        if (cnt[i] == 0) {
          terms.push_back(wave[i]);
          if (!g_bank) closed_q2.push_back(0);
          if (g_bank) {
            const mhgp3v::WspdBox ba = cell_of(nodes, sp, wave[i].a);
            const mhgp3v::WspdBox bb = cell_of(nodes, sp, wave[i].b);
            mhgp3v::RectBox qa{}, qb{};
            long long m4[3];
            for (int d = 0; d < 3; ++d) {
              qa.lo[d] = ba.lo[d]; qa.hi[d] = ba.hi[d];
              qb.lo[d] = bb.lo[d]; qb.hi[d] = bb.hi[d];
              m4[d] = ba.lo[d] + ba.hi[d] + bb.lo[d] + bb.hi[d];
            }
            const long long dlo = mhgp3v::rect_minsq(qa, qb);   // UNE fois
            long long cred[3] = {0, 0, 0};
            long long taken = 0;
            if (g_vwave) {
              // `Central-VWave`. LA TACHE EST `(CNode, lane_mask)`, jamais un
              // nœud seul avec un masque global : sinon un parent `ALL` en q2
              // mais `MIXED` en q3 pousse ses enfants, dont la population est
              // CREDITEE UNE SECONDE FOIS en q2 — une fausse fermeture. Seuls
              // les bits `MIXED` du parent passent aux enfants ; les bits
              // `ALL`/`NONE` y sont consommes exactement une fois.
              struct Task { int node; unsigned mask; };
              Task st[96];
              int sn = 0;
              st[sn++] = {0, 7u};
              long long exp = 0;
              bool abandonne = false;
              const int need[3] = {10, 9, 8};
              while (sn > 0 && exp < g_win) {
                const Task tk = st[--sn];
                // MUTANT `masque-global` : rendre au parent un masque complet
                // fait redescendre une lane deja `ALL` dans les enfants, dont
                // la population est alors creditee DEUX fois. C'est la faute
                // que j'avais ecrite, et le juge doit la tuer.
                unsigned m = g_inject_global ? 7u : tk.mask;
                for (int lane = 0; lane < 3; ++lane)
                  if (cred[lane] >= need[lane]) m &= ~(1u << lane);   // lane saturee
                if (!m) continue;
                ++exp; ++bank.reads; ++bank.recerts;
                mhgp3v::RectBox cb2{};
                long long pop = 1;
                if (tk.node < 0) {
                  const int r = -1 - tk.node;
                  for (int d = 0; d < 3; ++d) { cb2.lo[d] = sp[r][d]; cb2.hi[d] = sp[r][d]; }
                } else {
                  pop = nodes[tk.node].last - nodes[tk.node].first + 1;
                  for (int d = 0; d < 3; ++d) {
                    cb2.lo[d] = g_tight ? nodes[tk.node].tlo[d] : nodes[tk.node].lo[d];
                    cb2.hi[d] = g_tight ? nodes[tk.node].thi[d] : nodes[tk.node].hi[d];
                  }
                }
                long long smn = 0, smx = 0;
                mhgp3v::rect_s_interval(qa, qb, cb2, &smn, &smx);
                unsigned mixed = 0;
                for (int lane = 0; lane < 3; ++lane) {
                  if (!(m & (1u << lane))) continue;
                  const RectVerdict v = mhgp3v::rect_central_verdict(dlo, smn, smx, lane);
                  if (v == RectVerdict::kAll) cred[lane] += pop;      // consomme ICI
                  else if (v == RectVerdict::kMixed) mixed |= 1u << lane;
                }
                if (mixed && tk.node >= 0) {
                  if (sn + 2 > 96) { abandonne = true; break; }       // jamais en silence
                  st[sn++] = {nodes[tk.node].left, mixed};
                  st[sn++] = {nodes[tk.node].right, mixed};
                }
              }
              // Une pile pleine ou un quantum epuise laisse des taches VIVANTES.
              // ATTENTION : ce compteur les DENOMBRE, il ne les SERIALISE pas.
              // Aucune tache, aucun masque, aucun curseur n'est encore ecrit —
              // la continuation reste a faire, et l'audit `dfa9e1b` a raison de
              // refuser le mot. Les credits deja acquis restent valides ; seule
              // la COMPLETUDE est perdue, jamais la surete.
              if (abandonne || (sn > 0 && exp >= g_win)) ++bank.tronques;
              taken = exp;
            } else if (g_descent) {
              // Descente au meilleur d'abord vers `m_0`, pile bornee.
              std::pair<long long, int> heap[64];
              int hn = 0;
              heap[hn++] = {0, 0};
              long long exp = 0;
              while (hn > 0 && taken < g_bankl && exp < g_win) {
                int best = 0;
                for (int u = 1; u < hn; ++u) if (heap[u].first < heap[best].first) best = u;
                const int id = heap[best].second;
                heap[best] = heap[--hn];
                ++exp; ++bank.reads;
                if (id < 0) {
                  const int r = -1 - id;
                  mhgp3v::RectBox zb{};
                  for (int d = 0; d < 3; ++d) { zb.lo[d] = sp[r][d]; zb.hi[d] = sp[r][d]; }
                  ++taken; ++bank.recerts;
                  const unsigned got = mhgp3v::rect_central_mask_dlo(dlo, qa, qb, zb);
                  for (int lane = 0; lane < 3; ++lane) if (got & (1u << lane)) ++cred[lane];
                  continue;
                }
                for (int side = 0; side < 2; ++side) {
                  const int ch = side ? nodes[id].right : nodes[id].left;
                  if (hn >= 62) break;
                  const long long dd = (ch < 0) ? 0 : box_dist2_to(nodes[ch], m4, g_tight);
                  heap[hn++] = {dd, ch};
                }
              }
            } else {
              const unsigned long long qk =
                  mhgp3v::wf_morton48(m4[0] / 4, m4[1] / 4, m4[2] / 4);
              size_t pos = (size_t)(std::lower_bound(keys.begin(), keys.end(), qk) - keys.begin());
              const size_t beg = (pos > (size_t)(g_win / 2)) ? pos - g_win / 2 : 0;
              const size_t end = std::min(keys.size(), beg + (size_t)g_win);
              for (size_t r = beg; r < end && taken < g_bankl; ++r) {
                ++bank.reads;
                mhgp3v::RectBox zb{};
                for (int d = 0; d < 3; ++d) { zb.lo[d] = sp[r][d]; zb.hi[d] = sp[r][d]; }
                ++taken; ++bank.recerts;
                const unsigned got = mhgp3v::rect_central_mask_dlo(dlo, qa, qb, zb);
                for (int lane = 0; lane < 3; ++lane)
                  if (got & (1u << lane)) ++cred[lane];
              }
            }
            const int need[3] = {10, 9, 8};
            for (int lane = 0; lane < 3; ++lane)
              if (cred[lane] >= need[lane]) ++bank.closed[lane];
            // JUGE DE LA VAGUE. Toute fermeture affirme qu'il existe `need`
            // `PointId` DISTINCTS satisfaisant le masque central sur tout
            // `A x B`. On le verifie par balayage exhaustif du nuage, dans une
            // ecriture qui n'emprunte ni l'intervalle du score, ni l'antichaine.
            if (g_judge_vwave) {
              for (int lane = 0; lane < 3; ++lane) {
                if (cred[lane] < need[lane]) continue;
                long long vrai = 0;
                for (size_t z = 0; z < sp.size(); ++z) {
                  mhgp3v::RectBox zb{};
                  for (int d = 0; d < 3; ++d) { zb.lo[d] = sp[z][d]; zb.hi[d] = sp[z][d]; }
                  if (mhgp3v::rect_central_mask_dlo(dlo, qa, qb, zb) & (1u << lane)) ++vrai;
                }
                ++bank.juges;
                if (vrai < need[lane]) ++bank.faux;
              }
            }
            const long long msz = count_of(nodes, wave[i].a) * count_of(nodes, wave[i].b);
            if (cred[0] >= need[0]) { closed_q2.push_back(1); mass_closed_q2 += msz; }
            else closed_q2.push_back(0);
          }
          continue;
        }
        const int ia = wave[i].a, ib = wave[i].b;
        const long long ra = (ia < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ia));
        const long long rb = (ib < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ib));
        int o = off[i];
        if (ia >= 0 && (ib < 0 || ra >= rb)) {
          next[o++] = {nodes[ia].left, ib};
          next[o++] = {nodes[ia].right, ib};
        } else {
          next[o++] = {ia, nodes[ib].left};
          next[o++] = {ia, nodes[ib].right};
        }
      }
      wave.swap(next);
      wave_hwm = std::max(wave_hwm, (long long)wave.size());
    }

    t_wave.push_back(std::chrono::duration<double, std::milli>(clk::now() - w0).count());
    auto pct = [](std::vector<double> v, double f) {
      std::sort(v.begin(), v.end());
      return v[std::min(v.size() - 1, (size_t)(f * (double)v.size()))];
    };
    // ---- L'ARGUMENT D'EMPILEMENT, MESURE PLUTOT QU'ESPERE.
    //
    // La borne `O(s^3 n)` repose sur un argument de PACKING : un nœud donne ne
    // peut avoir qu'un nombre BORNE de partenaires, parce que ceux-ci sont des
    // boites disjointes de taille comparable dans une region bornee. Ma crainte
    // etait que la compression de l'octree — qui saute les niveaux vides — le
    // viole. C'est une affirmation directement testable : si le nombre maximal
    // de partenaires par nœud reste borne quand `n` croit, l'empilement tient.
    // S'il croit avec `n`, la borne est fausse et il faut le savoir.
    std::vector<int> deg(nodes.size() + sp.size(), 0);
    auto slot = [&](int id) { return (id < 0) ? (int)nodes.size() + (-1 - id) : id; };
    for (const Pair& t : terms) { ++deg[slot(t.a)]; ++deg[slot(t.b)]; }
    long long dmax = 0, dsum = 0, dnz = 0;
    for (int d : deg) { dmax = std::max(dmax, (long long)d); dsum += d; if (d) ++dnz; }

    // ---- L'INFLATION DU SEUIL, MESUREE.
    //
    // Le certificat de rectangle ne ferme une paire que si elle possede
    // `K lambda(s)` temoins, non `K` : le cœur commun a toutes les paires du
    // rectangle est plus petit que la boule d'une paire donnee, d'un facteur de
    // volume `lambda(s) = ((1+u)/(1-2u))^3` avec `u = 2/(s+2)`. Les paires dont
    // le compte tombe entre `K` et `K lambda` sont FAUSSEMENT residuelles.
    //
    // On echantillonne donc des rectangles NON fermes, on y prend une paire, et
    // on compte ses VRAIS temoins universels q2 par balayage exhaustif du nuage.
    // Une paire faussement residuelle est une paire qui en a deja `K`.
    long long ech = 0, faux_resid = 0, som_temoins = 0, max_temoins = 0;
    if (g_inflation > 0) {
      unsigned long long rng = 0x9E3779B97F4A7C15ull;
      // On n'echantillonne QUE les rectangles laisses OUVERTS par la banque :
      // ce sont eux qui partiraient a la source, et eux seuls dont il faut
      // savoir s'ils sont FAUSSEMENT residuels.
      // ECHANTILLONNAGE PONDERE PAR LA MASSE, et paire TIREE AU HASARD dans le
      // rectangle. Ma premiere version tirait un rectangle uniformement puis en
      // prenait la PREMIERE paire : elle sur-representait les petits rectangles
      // et prenait une paire arbitraire dans les grands. Pour estimer la
      // densite de supports du RESIDUEL, il faut tirer une PAIRE uniformement
      // dans la masse residuelle.
      std::vector<size_t> ouverts;
      std::vector<long long> cum;
      long long acc = 0;
      for (size_t i = 0; i < terms.size(); ++i)
        if (i >= closed_q2.size() || !closed_q2[i]) {
          ouverts.push_back(i);
          acc += count_of(nodes, terms[i].a) * count_of(nodes, terms[i].b);
          cum.push_back(acc);
        }
      for (long long e = 0; e < g_inflation && !ouverts.empty(); ++e) {
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        const long long pick = (long long)((rng >> 11) % (unsigned long long)acc);
        const size_t idx =
            (size_t)(std::upper_bound(cum.begin(), cum.end(), pick) - cum.begin());
        const Pair& t = terms[ouverts[idx]];
        const int fa = (t.a < 0) ? (-1 - t.a) : nodes[t.a].first;
        const int la = (t.a < 0) ? (-1 - t.a) : nodes[t.a].last;
        const int fb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].first;
        const int lb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].last;
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        const int ai = fa + (int)((rng >> 33) % (unsigned long long)(la - fa + 1));
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        const int bi = fb + (int)((rng >> 33) % (unsigned long long)(lb - fb + 1));
        if (ai == bi) continue;
        long long cnt = 0;
        for (size_t z = 0; z < sp.size(); ++z) {
          if ((int)z == ai || (int)z == bi) continue;
          long long h = 0;
          for (int d = 0; d < 3; ++d) h += (sp[z][d] - sp[ai][d]) * (sp[bi][d] - sp[z][d]);
          if (h > 0) ++cnt;
        }
        ++ech; som_temoins += cnt; max_temoins = std::max(max_temoins, cnt);
        if (cnt >= 10) ++faux_resid;
      }
    }

    long long mass = 0;
    for (const Pair& t : terms) mass += count_of(nodes, t.a) * count_of(nodes, t.b);
    const long long total = m * (m - 1) / 2;

    if (oracle) {
      std::map<std::pair<int, int>, int> mult;
      long long diag = 0;
      for (const Pair& t : terms) {
        const int fa = (t.a < 0) ? (-1 - t.a) : nodes[t.a].first;
        const int la = (t.a < 0) ? (-1 - t.a) : nodes[t.a].last;
        const int fb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].first;
        const int lb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].last;
        for (int u = fa; u <= la; ++u)
          for (int v = fb; v <= lb; ++v) {
            if (spid[u] == spid[v]) { ++diag; continue; }
            ++mult[{std::min(spid[u], spid[v]), std::max(spid[u], spid[v])}];
          }
      }
      long long dup = 0, miss = 0;
      for (const auto& e : mult) if (e.second != 1) ++dup;
      for (int x = 0; x < m; ++x)
        for (int y = x + 1; y < m; ++y) if (!mult.count({x, y})) ++miss;
      std::printf("oracle n=%lld attendu=%lld cles=%zu diagonales=%lld doublons=%lld manquantes=%lld\n",
                  m, total, mult.size(), diag, dup, miss);
      if (diag || (long long)mult.size() != total || dup || miss) {
        std::fprintf(stderr, "INVARIANT VIOLE: la vague ne partitionne pas les paires\n");
        return 3;
      }
      continue;
    }

    std::printf("n=%lld famille=%s boite=%s sep=%lld/%lld | front=%zu (%.3f/pt) | vagues=%lld"
                " tests=%lld tests/front=%.2f vague_max=%lld | masse=%lld/%lld"
                " | arbre_med=%.1f ms arbre_p95=%.1f ms vague=%.1f ms"
                " | banque lectures=%lld recert=%lld ferme q2=%lld q3=%lld q4=%lld"
                " | masse fermee q2=%.2f%% records fermes q2=%.2f%% tronques=%lld"
                " juges=%lld faux=%lld"
                " | partenaires max=%lld moyen=%.2f"
                " | residuel : %lld paires tirees DANS LA MASSE ouverte,"
                " temoins_moyen=%.1f max=%lld, deja >=10 temoins : %lld (%.1f%%)"
                " donc supports q2 estimes %.1f%% du residuel\n",
                m, family.c_str(), g_tight ? "serree" : "cellule", p, q, terms.size(), (double)terms.size() / (double)m,
                levels, tests, (double)tests / (double)terms.size(), wave_hwm, mass, total,
                pct(t_tree, 0.5), pct(t_tree, 0.95), t_wave.back(),
                bank.reads, bank.recerts, bank.closed[0], bank.closed[1], bank.closed[2],
                100.0 * (double)mass_closed_q2 / (double)total,
                100.0 * (double)bank.closed[0] / (double)std::max<size_t>(1, terms.size()),
                bank.tronques, bank.juges, bank.faux,
                dmax, (double)dsum / (double)std::max(1LL, dnz),
                ech, (double)som_temoins / (double)std::max(1LL, ech), max_temoins,
                faux_resid, 100.0 * (double)faux_resid / (double)std::max(1LL, ech),
                100.0 * (double)(ech - faux_resid) / (double)std::max(1LL, ech));
    if (g_judge_vwave) {
      if (bank.juges < 1000) {
        std::fprintf(stderr, "PLANCHER JUGE VAGUE: %lld fermetures jugees\n", bank.juges);
        return 3;
      }
      if (g_inject_global) {
        if (bank.faux == 0) {
          std::fprintf(stderr, "MUTANT SURVIVANT: le masque global n'a produit aucune"
                               " fausse fermeture\n");
          return 3;
        }
        std::printf("mutant_killed=1 raison=juge_vague faux=%lld\n", bank.faux);
        return 4;
      }
      if (bank.faux != 0) {
        std::fprintf(stderr, "DESACCORD DU JUGE: %lld fermetures sans %d PointId distincts\n",
                     bank.faux, 10);
        return 1;
      }
      std::printf("juge_vague accord=OUI jugees=%lld\n", bank.juges);
    }
    if (mass != total) {
      std::fprintf(stderr, "INVARIANT VIOLE: masse %lld != %lld\n", mass, total);
      return 3;
    }
    fronts.push_back((double)terms.size());
  }
  if (oracle) { std::printf("oracle accord=OUI\n"); return 0; }
  int bad = 0;
  for (size_t k = 1; k < fronts.size(); ++k) {
    const double s = std::log2(fronts[k] / fronts[k - 1]) / std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente front_records=%.3f (%lld->%lld)\n", s, ns[k - 1], ns[k]);
    if (s >= max_slope) ++bad; else bad = 0;
    if (bad >= 2) { std::fprintf(stderr, "REFUS DE PENTE: deux pentes >= %.2f\n", max_slope); return 3; }
  }
  std::printf("OK famille=%s sep=%lld/%lld\n", family.c_str(), p, q);
  return 0;
}
