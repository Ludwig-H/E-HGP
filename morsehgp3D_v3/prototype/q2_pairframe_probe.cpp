// q2 DE BOUT EN BOUT : `PairFrame -> W_2 -> identites d'aretes`.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// Etape 3 de l'ordre de l'auditeur (`AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032`
// § 9, arbitrage `9739e3c` § 9.D). Ici la geometrie est REELLE : nuage u16,
// ordre de Morton, arbre binaire a boites SERREES, partition exacte des paires,
// et le ledger `CoreDepthLedger` de `pair_frame.hpp` sans aucune modification.
//
// ---------------------------------------------------------------------------
// POURQUOI q2 EST LE BON BANC D'ESSAI
//
// `W_2(a,b) = {z : angle(a,z,b) > 90 degres}` est la boule diametrale OUVERTE
// de diametre `[a,b]`. Avec `Phi(a,b,z) = (a-z).(b-z)` :
//
//   `z` dans `W_2(a,b)`  <=>  `Phi(a,b,z) < 0`
//
// C'est la SEULE lane ou le fuseau est la miniboule du support : `CORE_CLEAR`
// y equivaut a la vivacite exacte, et aucun census de circumboule n'est requis.
// Pour q3/q4, `W_q` n'est qu'un cœur universel de prune et le census reste dû.
//
// Les extrema de `Phi` sur `A x B x C` sont ceux de `acute_owner_gateway.hpp` :
// separables par axe, exacts, `O(1)`, et deja verifies contre l'enumeration
// exhaustive de six mille triplets d'intervalles.
//
//   span `C` ALL   <=>  `Phi_max < 0`   (tout `z` de `C` temoin de toute paire)
//   span `C` NONE  <=>  `Phi_min >= 0`  (aucun `z` de `C` temoin d'aucune paire)
//
// ---------------------------------------------------------------------------
// LE MASQUE ENDPOINT RELATIONNEL EST UN THEOREME, PAS UN MECANISME
//
// C'est le resultat de cette etape, et il n'etait pas testable sur le modele
// abstrait faute d'analogue a « `z` appartient a `A` ».
//
//   THEOREME. Si les intervalles de Morton de `C` et de `A` se recouvrent, le
//   span `C` n'est JAMAIS classe `ALL`.
//
//   PREUVE. Soit `p` un point de `A inter C`. Alors `p` appartient a la boite
//   de `A` comme a celle de `C`, donc le triplet `(a = p, b, z = p)` appartient
//   a `boite(A) x boite(B) x boite(C)` et donne `Phi = (p-p).(b-p) = 0`. Donc
//   `Phi_max >= 0`, et le test `Phi_max < 0` echoue. Symetrique pour `B`.
//
// Autrement dit : la STRICTE de `Phi_max < 0` porte deja tout le masque. Il n'y
// a pas de mecanisme separe a ecrire pour q2, il y a une inegalite a ne pas
// relacher — et le mutant `credit-large` montre que la relacher est letal.
// L'invariant est ARME en permanence, pas seulement observe.
//
// Ce que le masque exige encore, en revanche, c'est la CONSERVATION : un span
// endpoint n'est pas creditable, mais il ne doit pas etre jete, car un `z` de
// `A` redevient un temoin ordinaire des que `A` ne le contient plus. Le mutant
// `endpoint-jete` le verifie.
//
// ---------------------------------------------------------------------------
// CODES DE SORTIE : 1 desaccord du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>

#include "prototype/acute_owner_gateway.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/pair_frame.hpp"

using namespace mhgp3v::pairframe;
using mhgp3v::acute::BoxI;
using mhgp3v::acute::Interval;
using mhgp3v::acute::i128;
using mhgp3v::acute::i64;

namespace {

// ---------------------------------------------------------------------------
enum class Q2Mutant : std::uint8_t {
  kNone = 0,
  kCreditLarge,     // `Phi_max <= 0` : credite `z = a`. LETAL.
  kNoneDeuxAxes,    // `Phi_min` sur deux axes : jette des temoins reels. LETAL.
  kEndpointJete,    // jette les spans endpoint au lieu de les conserver. LETAL.
  kMortLarge,       // `lower > h` au lieu de `>=` : rate la mort a l'egalite.
  kBoiteCellule,    // boite de cellule Morton au lieu de boite serree.
  kShellJete        // `Phi_min == 0` traite comme OUTSIDE_CLOSED : shell perdu.
};

bool g_inject = false;
Q2Mutant g_mu = Q2Mutant::kNone;
PfMutant g_pf = PfMutant::kNone;

int sortie(int code_normal, const char* diag) {
  if (g_inject) { std::printf("MUTANT TUE : %s\n", diag); return 4; }
  std::printf("FAUTE : %s\n", diag);
  return code_normal;
}
int refus(const char* diag) { std::printf("REFUS : %s\n", diag); return 2; }

// ---------------------------------------------------------------------------
// LE NUAGE ET SON ORDRE DE MORTON.
std::vector<mhgp::P3> g_pts;      // dans l'ordre de Morton
std::vector<int> g_pid;           // `g_pid[i]` = le VRAI PointId du rang `i`
int g_n = 0;

std::uint64_t etale21(std::uint64_t v) {
  v &= 0x1fffffULL;
  v = (v | (v << 32)) & 0x1f00000000ffffULL;
  v = (v | (v << 16)) & 0x1f0000ff0000ffULL;
  v = (v | (v << 8)) & 0x100f00f00f00f00fULL;
  v = (v | (v << 4)) & 0x10c30c30c30c30c3ULL;
  v = (v | (v << 2)) & 0x1249249249249249ULL;
  return v;
}

std::uint64_t morton(const mhgp::P3& p) {
  return etale21((std::uint64_t)p.x) | (etale21((std::uint64_t)p.y) << 1) |
         (etale21((std::uint64_t)p.z) << 2);
}

// ---------------------------------------------------------------------------
// L'ARBRE BINAIRE SUR L'ORDRE DE MORTON, A BOITES SERREES.
//
// La boite est le min/max REEL des points de l'intervalle, jamais la cellule de
// Morton. La difference n'est pas cosmetique : une cellule mal formee m'a deja
// produit deux blocs `ALL` faux sur le gateway ternaire. Sous profil u16 et
// avec la cellule calculee ici — le plus petit cube de grille contenant les
// deux cles extremes, donc contenant TOUS les points de l'intervalle trie —
// l'erreur est conservatrice : elle detruit le pouvoir de decision sans
// fausser. Le mutant `boite-cellule` mesure exactement cela.
struct Nœud {
  int lo = 0, hi = 0, g = -1, d = -1;
  BoxI serree, cellule;
};
std::vector<Nœud> g_arbre;

BoxI boite_des_points(int lo, int hi) {
  BoxI b;
  for (int k = 0; k < 3; ++k) b.ax[k] = Interval{0, 0};
  for (int i = lo; i < hi; ++i) {
    const i64 c[3] = {g_pts[(std::size_t)i].x, g_pts[(std::size_t)i].y, g_pts[(std::size_t)i].z};
    for (int k = 0; k < 3; ++k) {
      if (i == lo) { b.ax[k].lo = c[k]; b.ax[k].hi = c[k]; }
      else {
        if (c[k] < b.ax[k].lo) b.ax[k].lo = c[k];
        if (c[k] > b.ax[k].hi) b.ax[k].hi = c[k];
      }
    }
  }
  return b;
}

// La cellule de Morton : le plus petit cube aligne de la grille contenant les
// deux cles extremes. C'est la boite FAUSSE, conservee pour le mutant.
BoxI cellule_morton(int lo, int hi) {
  const std::uint64_t ka = morton(g_pts[(std::size_t)lo]);
  const std::uint64_t kb = morton(g_pts[(std::size_t)(hi - 1)]);
  int niveau = 0;
  for (int b = 20; b >= 0; --b) {
    const std::uint64_t m = ~((1ULL << (3 * b)) - 1ULL);
    if ((ka & m) == (kb & m)) { niveau = b; break; }
  }
  const i64 pas = (i64)1 << niveau;
  BoxI r;
  const i64 base[3] = {g_pts[(std::size_t)lo].x, g_pts[(std::size_t)lo].y,
                       g_pts[(std::size_t)lo].z};
  for (int k = 0; k < 3; ++k) {
    const i64 o = (base[k] / pas) * pas;
    r.ax[k] = Interval{o, o + pas - 1};
  }
  return r;
}

int construire(int lo, int hi) {
  const int id = (int)g_arbre.size();
  g_arbre.push_back(Nœud());
  g_arbre[(std::size_t)id].lo = lo;
  g_arbre[(std::size_t)id].hi = hi;
  g_arbre[(std::size_t)id].serree = boite_des_points(lo, hi);
  g_arbre[(std::size_t)id].cellule = cellule_morton(lo, hi);
  if (hi - lo > 1) {
    const int mid = (lo + hi) / 2;
    const int a = construire(lo, mid);
    const int b = construire(mid, hi);
    g_arbre[(std::size_t)id].g = a;
    g_arbre[(std::size_t)id].d = b;
  }
  return id;
}

inline int pop(int n) { return g_arbre[(std::size_t)n].hi - g_arbre[(std::size_t)n].lo; }
inline bool feuille(int n) { return g_arbre[(std::size_t)n].g < 0; }
inline const BoxI& boite(int n) {
  return (g_mu == Q2Mutant::kBoiteCellule) ? g_arbre[(std::size_t)n].cellule
                                           : g_arbre[(std::size_t)n].serree;
}
inline bool recouvre(int u, int v) {
  return g_arbre[(std::size_t)u].lo < g_arbre[(std::size_t)v].hi &&
         g_arbre[(std::size_t)v].lo < g_arbre[(std::size_t)u].hi;
}

inline int bucket_de(int p) { int b = 0; while ((1 << (b + 1)) <= p) ++b; return b; }

// ---------------------------------------------------------------------------
// LA CLASSIFICATION D'UN SPAN POUR q2. Deux inegalites, et la STRICTE de la
// premiere porte tout le masque endpoint.
// ---------------------------------------------------------------------------
// LA TRICHOTOMIE DU REJET q2 — architecture B du § 3.3 de l'arbitrage `f62d986`.
//
// `NONE_OPEN` N'EST PAS `OUTSIDE_CLOSED`. `W_2` est l'INTERIEUR de la boule
// diametrale, `H = 0` en est le SHELL, et avec `Phi = -H` :
//
//   `Phi_min > 0`   -> `H_max < 0`  : OUTSIDE_CLOSED, eliminable sans reserve
//   `Phi_min == 0`  -> `H_max <= 0` avec egalite possible : SHELL_POSSIBLE,
//                      transfere vers le census, JAMAIS jete
//   `Phi_max < 0`   -> `H_min > 0`  : tout le span est temoin interieur
//
// J'avais applique « `NONE_OPEN` est supprime » uniformement. Pour le compte
// OUVERT que la descente calcule, c'est sans consequence — un point de shell
// n'est pas dans `W_2` ouvert. Mais le `BallCensusLedger` q2 a besoin de
// distinguer `I_B` de `U_B`, et un span jete a cet endroit-la emporte
// l'information de cospherie avec lui. Le span de shell est donc CONSERVE dans
// une frontiere separee, non creditee.
enum : std::uint8_t { kSpanOutside = 0, kSpanShell = 1, kSpanAll = 2, kSpanMixte = 3 };

long long g_classifications = 0;

std::uint8_t classifie(int A, int B, int C) {
  ++g_classifications;
  const mhgp3v::acute::Extrema e =
      mhgp3v::acute::extrema(boite(A), boite(B), boite(C));
  const bool tous = (g_mu == Q2Mutant::kCreditLarge) ? (e.phi_max <= 0) : (e.phi_max < 0);
  if (tous) return kSpanAll;
  i128 phi_min = e.phi_min;
  if (g_mu == Q2Mutant::kNoneDeuxAxes) {
    // La faute d'ecriture, pas une marge calibree : `for (k < 2)` au lieu de
    // `k < 3`. Le minorant de `Phi` devient trop GRAND, donc des spans qui
    // contiennent de vrais temoins sont declares `NONE` et jetes. Une marge de
    // `1` sur des coordonnees u16 a l'echelle `49` ne rencontrerait jamais
    // `Phi_min = -1` : elle aurait donne un mutant vert par vacuite, ce qui
    // n'est pas un mutant mais un decor.
    phi_min = 0;
    for (int k = 0; k < 2; ++k)
      phi_min += mhgp3v::acute::phi_min_axe(boite(A).ax[k], boite(B).ax[k], boite(C).ax[k]);
  }
  if (phi_min > 0) return kSpanOutside;   // strictement hors de la boule FERMEE
  if (phi_min == 0) {
    // `H_max = 0` : le span peut contenir du shell. Le mutant `shell-jete` le
    // traite comme `OUTSIDE_CLOSED`, ce que le juge de shell refuse.
    return (g_mu == Q2Mutant::kShellJete) ? kSpanOutside : kSpanShell;
  }
  return kSpanMixte;
}

// Le tirage DETERMINISTE des echantillons : meme nuage, memes cas verifies.
inline std::uint32_t melange_ech(std::uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}

bool g_verifie_shell = false;
long long g_pas_shell = 7;    // un span elimine sur sept
long long g_pas_bornes = 97;  // un etat sur quatre-vingt-dix-sept
void verifie_outside(int A, int B, int n);

// ---------------------------------------------------------------------------
struct Compteurs {
  long long rectangles = 0, masse_paires = 0, etats = 0;
  long long witness_splits = 0, witness_batches = 0, exact_tiles = 0;
  long long selector_scan_items = 0, terminal_checks = 0;
  long long pruned = 0, core_clear = 0, spans_elimines = 0;
  long long spans_shell_conserves = 0, shell_masse = 0;
  long long shell_points_juges = 0, shell_perdus = 0, spans_outside_verifies = 0;
  long long spans_outside_vus = 0;
  long long bornes_vues = 0, bornes_verifiees = 0, bornes_violees = 0;
  long long relation_spans = 0, relation_all = 0;
  long long pending_state_count = 0, resume_count = 0, continuation_octets = 0;
  long long frontier_span_count_peak = 0, frontier_candidate_mass_peak = 0;
  long long state_refine_iterations_max = 0;
  long long exact_point_predicate_evaluations = 0, exact_point_cap_violations = 0;
  long long aretes_mortes = 0, aretes_vivantes = 0;
};
Compteurs g_c;

struct Etat {
  int A = 0, B = 0;
  std::vector<int> spans_all, mixtes_feuilles, shell_spans;
  std::vector<std::vector<int>> buckets;
  std::uint32_t masque = 0;
  long long lower = 0, masse = 0, largeur = 0, masse_cand = 0, iterations = 0;
  Etat() : buckets(24) {}

  void ajoute(int n, std::uint8_t e) {
    // LE MUTANT QUI JETTE LES SPANS ENDPOINT. Ils ne sont pas creditables —
    // c'est le theoreme —, mais les jeter perd les temoins qu'ils contiennent
    // et qui redeviennent ordinaires des que `A` ne les contient plus.
    if (g_mu == Q2Mutant::kEndpointJete && (recouvre(n, A) || recouvre(n, B))) {
      ++g_c.spans_elimines;
      return;
    }
    if (e == kSpanOutside) {
      ++g_c.spans_elimines;
      if (g_verifie_shell) verifie_outside(A, B, n);
      return;
    }
    if (e == kSpanShell) {
      // CONSERVE, jamais credite : ni au minorant (aucun point de shell n'est
      // dans `W_2` ouvert), ni a la masse candidate (aucun ne peut le devenir).
      shell_spans.push_back(n);
      ++g_c.spans_shell_conserves;
      g_c.shell_masse += pop(n);
      return;
    }
    ++largeur;
    if (e == kSpanAll) { spans_all.push_back(n); lower += pop(n); return; }
    // La masse candidate est celle des MIXTES seuls : les preuves `ALL` n'en
    // font pas partie (P2.1 de l'audit `08b7007`).
    masse_cand += pop(n);
    masse += pop(n);
    if (feuille(n)) { mixtes_feuilles.push_back(n); return; }
    const int b = bucket_de(pop(n));
    buckets[(std::size_t)b].push_back(n);
    masque |= (std::uint32_t)1u << b;
  }
  bool a_mixte_non_feuille() const { return masque != 0; }
};

int selectionne(Etat& e, const std::string& heur, int* bs) {
  if (heur == "buckets") {
    g_c.selector_scan_items += 1;
    int b = 23;
    while (b >= 0 && ((e.masque >> b) & 1u) == 0u) --b;
    if (b < 0) return -1;
    *bs = b;
    return e.buckets[(std::size_t)b].back();
  }
  g_c.selector_scan_items += e.largeur;
  int meilleur = -1, mb = -1;
  for (int b = 0; b < 24; ++b)
    for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
      const int n = e.buckets[(std::size_t)b][i];
      if (meilleur < 0 || (heur == "plus-petit" ? pop(n) < pop(meilleur) : pop(n) > pop(meilleur))) {
        meilleur = n; mb = b;
      }
    }
  *bs = mb;
  return meilleur;
}

void scinde(Etat& e, int n, int b) {
  std::vector<int>& v = e.buckets[(std::size_t)b];
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i] == n) { v[i] = v.back(); v.pop_back(); break; }
  if (v.empty()) e.masque &= ~((std::uint32_t)1u << b);
  --e.largeur;
  e.masse_cand -= pop(n);
  e.masse -= pop(n);
  e.ajoute(g_arbre[(std::size_t)n].g, classifie(e.A, e.B, g_arbre[(std::size_t)n].g));
  e.ajoute(g_arbre[(std::size_t)n].d, classifie(e.A, e.B, g_arbre[(std::size_t)n].d));
  ++g_c.witness_splits;
}

// LE JUGE DE SHELL, arme sous `--verifie-shell`. Il verifie EXHAUSTIVEMENT que
// tout span elimine comme `OUTSIDE_CLOSED` ne contient, pour aucune paire du
// rectangle, un point cospherique `Phi = 0`. C'est le seul controle qui
// distingue vraiment `NONE_OPEN` de `OUTSIDE_CLOSED` : le juge d'identites ne
// le peut pas, puisque le shell ne compte pas dans le fuseau ouvert.
void verifie_outside(int A, int B, int n) {
  // ECHANTILLON, pas balayage. Un span sur `g_pas_shell` est verifie ; le
  // theoreme dit deja que `Phi_min > 0` implique `H_max < 0` donc aucun point
  // cospherique. Ce qu'on teste ici est que le CODE separe bien `> 0` de `>= 0`.
  if ((++g_c.spans_outside_vus % g_pas_shell) != 0) return;
  ++g_c.spans_outside_verifies;
  for (int ia = g_arbre[(std::size_t)A].lo; ia < g_arbre[(std::size_t)A].hi; ++ia)
    for (int ib = g_arbre[(std::size_t)B].lo; ib < g_arbre[(std::size_t)B].hi; ++ib)
      for (int z = g_arbre[(std::size_t)n].lo; z < g_arbre[(std::size_t)n].hi; ++z) {
        if (z == ia || z == ib) continue;
        const mhgp::P3& P = g_pts[(std::size_t)ia];
        const mhgp::P3& Q = g_pts[(std::size_t)ib];
        const mhgp::P3& Z = g_pts[(std::size_t)z];
        const i128 phi = (i128)(P.x - Z.x) * (Q.x - Z.x) + (i128)(P.y - Z.y) * (Q.y - Z.y) +
                         (i128)(P.z - Z.z) * (Q.z - Z.z);
        if (phi == 0) ++g_c.shell_perdus;
      }
}

// ---------------------------------------------------------------------------
// LE COMPTE EXACT D'UNE PAIRE, pour la tuile exacte.
inline bool temoin(int a, int b, int z) {
  const mhgp::P3& A = g_pts[(std::size_t)a];
  const mhgp::P3& B = g_pts[(std::size_t)b];
  const mhgp::P3& Z = g_pts[(std::size_t)z];
  const i128 phi = (i128)(A.x - Z.x) * (B.x - Z.x) + (i128)(A.y - Z.y) * (B.y - Z.y) +
                   (i128)(A.z - Z.z) * (B.z - Z.z);
  return phi < 0;  // `W_2` OUVERT : `a` et `b` eux-memes donnent `phi = 0`
}

long long compte_dans_etat(const Etat& e, int a, int b) {
  // Les spans `ALL` ne sont JAMAIS rescannes : `lower` les porte deja. Le coût
  // ponctuel de la tuile vaut alors exactement `pair_mass * masse candidate`,
  // ce que le cap borne desormais (P0 de l'audit `08b7007`).
  long long n = e.lower;
  g_c.exact_point_predicate_evaluations += e.masse;
  for (std::size_t i = 0; i < e.mixtes_feuilles.size(); ++i) {
    const int s = e.mixtes_feuilles[i];
    for (int z = g_arbre[(std::size_t)s].lo; z < g_arbre[(std::size_t)s].hi; ++z)
      if (temoin(a, b, z)) ++n;
  }
  for (int t = 0; t < 24; ++t)
    for (std::size_t i = 0; i < e.buckets[(std::size_t)t].size(); ++i) {
      const int s = e.buckets[(std::size_t)t][i];
      for (int z = g_arbre[(std::size_t)s].lo; z < g_arbre[(std::size_t)s].hi; ++z)
        if (temoin(a, b, z)) ++n;
    }
  return n;
}

// ---------------------------------------------------------------------------
// LA PARTITION EXACTE DES PAIRES. Self-jointure lockstep : `A == B` se scinde
// en TROIS — `(g,g)`, `(g,d)`, `(d,d)` —, la diagonale inverse est omise. C'est
// la forme qui m'avait deja coûte un facteur deux quand je l'avais approchee
// par un filtre sur les indices, et le ledger de masse le verifie.
// A l'ECHELLE, la liste des rectangles ne tient pas : `C(32000,2)/16` vaut deux
// milliards d'entrees. La partition devient donc un FLUX — chaque rectangle est
// traite puis oublie —, et rien de dimension `n^2` n'est jamais materialise.
std::vector<std::pair<int, int>> g_rects;     // mode oracle seulement
bool g_flux = false;
void traite_rectangle(int A, int B);

void partitionne(int A, int B, long long cap_rect) {
  if (A == B) {
    if (feuille(A)) return;  // feuille = un point : aucune paire interne
    const int g = g_arbre[(std::size_t)A].g, d = g_arbre[(std::size_t)A].d;
    partitionne(g, g, cap_rect);
    partitionne(g, d, cap_rect);
    partitionne(d, d, cap_rect);
    return;
  }
  const long long m = (long long)pop(A) * pop(B);
  if (m <= cap_rect || (feuille(A) && feuille(B))) {
    if (g_flux) traite_rectangle(A, B); else g_rects.push_back(std::make_pair(A, B));
    return;
  }
  if (feuille(A) || (!feuille(B) && pop(B) > pop(A))) {
    partitionne(A, g_arbre[(std::size_t)B].g, cap_rect);
    partitionne(A, g_arbre[(std::size_t)B].d, cap_rect);
  } else {
    partitionne(g_arbre[(std::size_t)A].g, B, cap_rect);
    partitionne(g_arbre[(std::size_t)A].d, B, cap_rect);
  }
}

// ---------------------------------------------------------------------------
// LE MODE ECHELLE. `docs/TEST_PLAN_MORSEHGP3D.md` § 3.1 : les tailles qui
// comptent sont `8000`, `16000` et `32000`, et rien de ce qui suit ne doit
// dependre d'une structure indexee par paire.
//
// CE QUI EST VERIFIE ICI, ET C'EST VERIFIABLE A TOUTE TAILLE :
//
//   masse de partition        exactement `C(n,2)`
//   masse decidee             exactement `C(n,2)` — aucune paire sans decision
//   depassements de cap       zero
//   points de shell perdus    zero sur les spans echantillonnes
//   juge d'echantillon        `K` paires tirees deterministement, exactes
//
// CE QUI N'EST PAS VERIFIE, ET QUI NE PEUT PAS L'ETRE : les identites
// completes. Un juge `O(n^3)` vaut `5.10^11` operations a `n = 8000`. Le mode
// `--juge` reste la verite terrain, borne a `400` points, et il n'etablit
// aucune pente. Les deux roles sont disjoints.
long long g_masse_decidee = 0;
long long g_mortes = 0, g_vivantes = 0;
std::vector<std::pair<int, int>> g_echantillon;   // paires suivies
std::vector<std::uint8_t> g_echantillon_dit;      // 1 vivante, 2 morte
int g_h2 = 0;
std::uint64_t g_cap_ech = 0, g_fcap_ech = 0;
std::string g_heur_ech;
std::uint32_t g_lot_ech = 1;


// Les points APPARAISSANT dans une paire suivie sont marques : sans ce filtre,
// chercher l'echantillon pour chaque paire de chaque rectangle coûterait `O(K)`
// par paire, soit des milliards d'operations a l'echelle. Avec lui, seuls les
// rares rectangles contenant deux points marques paient la recherche.
std::vector<char> g_point_suivi;

int indice_suivi(int a, int b) {
  if (!g_point_suivi[(std::size_t)a] || !g_point_suivi[(std::size_t)b]) return -1;
  for (std::size_t i = 0; i < g_echantillon.size(); ++i) {
    const int x = g_echantillon[i].first, y = g_echantillon[i].second;
    if ((x == a && y == b) || (x == b && y == a)) return (int)i;
  }
  return -1;
}

void note_paire(int a, int b, int v) {
  ++g_masse_decidee;
  if (v) ++g_mortes; else ++g_vivantes;
  const int k = indice_suivi(a, b);
  if (k >= 0) g_echantillon_dit[(std::size_t)k] = (std::uint8_t)(v + 1);
}

void note_bloc(int A, int B, int v) {
  const long long m = (long long)pop(A) * pop(B);
  g_masse_decidee += m;
  if (v) g_mortes += m; else g_vivantes += m;
  if (g_echantillon.empty()) return;
  for (int ia = g_arbre[(std::size_t)A].lo; ia < g_arbre[(std::size_t)A].hi; ++ia) {
    if (!g_point_suivi[(std::size_t)ia]) continue;
    for (int ib = g_arbre[(std::size_t)B].lo; ib < g_arbre[(std::size_t)B].hi; ++ib) {
      const int k = indice_suivi(ia, ib);
      if (k >= 0) g_echantillon_dit[(std::size_t)k] = (std::uint8_t)(v + 1);
    }
  }
}

// LA DESCENTE D'UN SEUL RECTANGLE, sans rien de dimension `n^2`. Les
// continuations sont servies sur place par escalade de ressource : a l'echelle,
// une vague globale exigerait de retenir tous les etats pendants.
void traite_rectangle(int A, int B) {
  std::vector<Etat> pile;
  {
    Etat e;
    e.A = A; e.B = B;
    e.ajoute(0, classifie(A, B, 0));
    pile.push_back(e);
  }
  std::uint64_t cap_v = g_cap_ech, fcap_v = g_fcap_ech;
  int garde = 0;
  while (!pile.empty()) {
    if (++garde > 4096) return;   // garde-fou ; le juge d'invariant le verra
    Etat e = pile.back();
    pile.pop_back();
    ++g_c.etats;
    for (;;) {
      if (e.largeur > g_c.frontier_span_count_peak) g_c.frontier_span_count_peak = e.largeur;
      if (e.masse_cand > g_c.frontier_candidate_mass_peak)
        g_c.frontier_candidate_mass_peak = e.masse_cand;
      CoreDepthLedger L;
      L.reject_threshold = (std::uint8_t)g_h2;
      L.sature(e.lower, e.masse, g_pf);
      L.frontier_width = (std::uint32_t)e.largeur;
      PairFrame F;
      F.rect_id = 0; F.a_node = e.A; F.b_node = e.B;
      F.pair_mass = (std::uint64_t)pop(e.A) * (std::uint64_t)pop(e.B);
      PolitiqueV0 pol;
      pol.exact_tile_cap = cap_v;
      pol.frontier_cap = fcap_v;
      pol.witness_batch = g_lot_ech;
      ++g_c.terminal_checks;
      const bool mort = (g_mu == Q2Mutant::kMortLarge)
                            ? (L.lower_open_sat > L.reject_threshold)
                            : est_pruned(L, g_pf);
      const Action a = mort ? Action::kTerminal
                            : choisir_action(L, F, e.a_mixte_non_feuille(), false, pol, g_pf);
      // L'INVARIANT DE SURETE DES BORNES, echantillonne. Un juge de RESULTAT ne
      // voit une borne fausse que si elle change une decision, ce qui est rare
      // et diffus ; l'invariant la voit LA OU ELLE EST. C'est ce qui remplace
      // l'enumeration : on ne verifie pas toutes les paires, on verifie la
      // propriete dont depend la correction, sur un tirage deterministe.
      if (g_pas_bornes > 0 && (++g_c.bornes_vues % g_pas_bornes) == 0) {
        ++g_c.bornes_verifiees;
        const int ia = g_arbre[(std::size_t)e.A].lo, ib = g_arbre[(std::size_t)e.B].lo;
        if (ia != ib) {
          // LA REFERENCE EST LE NUAGE ENTIER, pas l'etat. Compter sur les spans
          // PRESENTS serait auto-coherent : un span elimine a tort manquerait
          // des deux cotes, et l'invariant serait aveugle a la faute meme qu'il
          // est cense voir. C'est `O(n)` par etat echantillonne, et `--pas-bornes`
          // regle la depense.
          long long exact = 0;
          for (int z = 0; z < g_n; ++z) if (temoin(ia, ib, z)) ++exact;
          if (exact < e.lower || exact > e.lower + e.masse) ++g_c.bornes_violees;
        }
      }
      if (a == Action::kTerminal) {
        const int v = mort ? 1 : 0;
        if (v) ++g_c.pruned; else ++g_c.core_clear;
        note_bloc(e.A, e.B, v);
        break;
      }
      if (a == Action::kPending) {
        ++g_c.pending_state_count;
        g_c.continuation_octets += 24 + 4 * (long long)e.largeur;
        ++g_c.resume_count;
        cap_v = cap_v * 2 + 1;
        fcap_v = fcap_v * 2 + 1;
        continue;                 // l'hote re-dispatche avec plus de place
      }
      if (a == Action::kExactifyTile) {
        ++g_c.exact_tiles;
        if (evaluations_tuile(L, F) > cap_v) ++g_c.exact_point_cap_violations;
        for (int ia = g_arbre[(std::size_t)e.A].lo; ia < g_arbre[(std::size_t)e.A].hi; ++ia)
          for (int ib = g_arbre[(std::size_t)e.B].lo; ib < g_arbre[(std::size_t)e.B].hi; ++ib)
            note_paire(ia, ib, compte_dans_etat(e, ia, ib) >= g_h2 ? 1 : 0);
        break;
      }
      std::vector<std::pair<int, int>> lot;
      if (g_heur_ech == "buckets" && g_lot_ech == 1) {
        int b = -1;
        const int nsel = selectionne(e, g_heur_ech, &b);
        if (nsel >= 0) lot.push_back(std::make_pair(nsel, b));
      } else {
        g_c.selector_scan_items += (g_heur_ech == "buckets") ? 1 : e.largeur;
        for (int b = 23; b >= 0; --b)
          for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
            if (g_lot_ech != 0 && lot.size() >= (std::size_t)g_lot_ech) break;
            lot.push_back(std::make_pair(e.buckets[(std::size_t)b][i], b));
          }
      }
      if (lot.empty()) { note_bloc(e.A, e.B, 0); break; }
      long long faits = 0;
      for (std::size_t i = 0; i < lot.size(); ++i) {
        if ((std::uint64_t)(e.largeur + 1) > fcap_v && faits > 0) break;
        scinde(e, lot[i].first, lot[i].second);
        ++faits;
      }
      ++g_c.witness_batches;
    }
  }
}

// ---------------------------------------------------------------------------
struct Resultat {
  std::vector<std::uint8_t> morte;  // indexee par `rang_a * n + rang_b`, `a < b`
  int faute = 0;
  const char* diag = "";
};

void marque(Resultat& r, int a, int b, int v) {
  const int i = a < b ? a : b, j = a < b ? b : a;
  r.morte[(std::size_t)i * g_n + j] = (std::uint8_t)(v + 1);  // 1 vivante, 2 morte
}

Resultat descente(int h2, std::uint64_t cap, std::uint64_t front_cap,
                  const std::string& heur, std::uint32_t lot) {
  Resultat r;
  r.morte.assign((std::size_t)g_n * g_n, 0);
  std::vector<Etat> vague, suivante;
  for (std::size_t i = 0; i < g_rects.size(); ++i) {
    Etat e;
    e.A = g_rects[i].first;
    e.B = g_rects[i].second;
    e.ajoute(0, classifie(e.A, e.B, 0));
    vague.push_back(e);
    ++g_c.rectangles;
    g_c.masse_paires += (long long)pop(e.A) * pop(e.B);
  }

  std::uint64_t cap_v = cap, fcap_v = front_cap;
  int garde = 0;
  while (!vague.empty()) {
    if (++garde > 64) { r.faute = 3; r.diag = "l'escalade de ressource ne converge pas"; return r; }
    suivante.clear();
    while (!vague.empty()) {
      Etat e = vague.back();
      vague.pop_back();
      ++g_c.etats;
      for (;;) {
        ++e.iterations;
        if (e.iterations > g_c.state_refine_iterations_max)
          g_c.state_refine_iterations_max = e.iterations;
        if (e.largeur > g_c.frontier_span_count_peak) g_c.frontier_span_count_peak = e.largeur;
        if (e.masse_cand > g_c.frontier_candidate_mass_peak)
          g_c.frontier_candidate_mass_peak = e.masse_cand;

        CoreDepthLedger L;
        L.reject_threshold = (std::uint8_t)h2;
        L.sature(e.lower, e.masse, g_pf);
        L.frontier_width = (std::uint32_t)e.largeur;
        PairFrame F;
        F.rect_id = 0; F.a_node = e.A; F.b_node = e.B;
        F.pair_mass = (std::uint64_t)pop(e.A) * (std::uint64_t)pop(e.B);
        PolitiqueV0 pol;
        pol.exact_tile_cap = cap_v;
        pol.frontier_cap = fcap_v;
        pol.witness_batch = lot;

        ++g_c.terminal_checks;
        const bool mort = (g_mu == Q2Mutant::kMortLarge)
                              ? (L.lower_open_sat > L.reject_threshold)
                              : est_pruned(L, g_pf);
        const Action a = mort ? Action::kTerminal
                              : choisir_action(L, F, e.a_mixte_non_feuille(),
                                               false, pol, g_pf);

        if (a == Action::kTerminal) {
          const int v = mort ? 1 : 0;
          if (v) ++g_c.pruned; else ++g_c.core_clear;
          for (int ia = g_arbre[(std::size_t)e.A].lo; ia < g_arbre[(std::size_t)e.A].hi; ++ia)
            for (int ib = g_arbre[(std::size_t)e.B].lo; ib < g_arbre[(std::size_t)e.B].hi; ++ib)
              marque(r, ia, ib, v);
          break;
        }

        if (a == Action::kPending) {
          ++g_c.pending_state_count;
          g_c.continuation_octets += 24 + 4 * (long long)e.largeur;
          suivante.push_back(e);
          break;
        }

        if (a == Action::kExactifyTile) {
          ++g_c.exact_tiles;
          // LA PORTE DU CONTRAT : une tuile acceptee ne doit jamais depasser le
          // cap qu'elle a invoque pour etre acceptee.
          if (evaluations_tuile(L, F) > cap_v) ++g_c.exact_point_cap_violations;
          for (int ia = g_arbre[(std::size_t)e.A].lo; ia < g_arbre[(std::size_t)e.A].hi; ++ia)
            for (int ib = g_arbre[(std::size_t)e.B].lo; ib < g_arbre[(std::size_t)e.B].hi; ++ib)
              marque(r, ia, ib, compte_dans_etat(e, ia, ib) >= h2 ? 1 : 0);
          break;
        }

        // `kSplitWitness`. `kSplitOneEndpoint` n'est jamais demande : les
        // rectangles sont fixes par la partition, seuls les temoins bougent.
        std::vector<std::pair<int, int>> a_scinder;
        if (heur == "buckets" && lot == 1) {
          int b = -1;
          const int n = selectionne(e, heur, &b);
          if (n >= 0) a_scinder.push_back(std::make_pair(n, b));
        } else {
          g_c.selector_scan_items += (heur == "buckets") ? 1 : e.largeur;
          for (int b = 23; b >= 0; --b)
            for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
              if (lot != 0 && a_scinder.size() >= (std::size_t)lot) break;
              a_scinder.push_back(std::make_pair(e.buckets[(std::size_t)b][i], b));
            }
        }
        if (a_scinder.empty()) {
          r.faute = 3;
          r.diag = "SPLIT_WITNESS sans span mixte non-feuille : l'ABI ment sur son etat";
          return r;
        }
        long long faits = 0;
        for (std::size_t i = 0; i < a_scinder.size(); ++i) {
          if ((std::uint64_t)(e.largeur + 1) > fcap_v && faits > 0) break;
          scinde(e, a_scinder[i].first, a_scinder[i].second);
          ++faits;
        }
        ++g_c.witness_batches;
      }
    }
    vague.swap(suivante);
    if (!vague.empty()) { ++g_c.resume_count; cap_v = cap_v * 2 + 1; fcap_v = fcap_v * 2 + 1; }
  }
  return r;
}

// ---------------------------------------------------------------------------
// LE MASQUE ENDPOINT : LE THEOREME EST INVOQUE, PAS RE-PARCOURU.
//
// § 3.2 du plan de test. La preuve est faite une fois, en trois lignes, et elle
// ne depend d'aucun nuage :
//
//   si `p` est dans `A inter C`, alors `(a = p, b, z = p)` appartient au produit
//   des trois boites et donne `Phi = 0`, donc `Phi_max >= 0`, donc le test
//   `Phi_max < 0` echoue et `C` n'est jamais `ALL`.
//
// Balayer onze mille spans pour la reconstater mesurerait trois choses a la fois
// — le theoreme, l'implementation et la patience de la porte — sans savoir
// laquelle a echoue. Ce qui reste a tester est la FAUTE D'IMPLEMENTATION :
// relacher la stricte. Elle se voit sur les FIXTURES D'EGALITE, la ou `<` et
// `<=` se separent, et sur un echantillon deterministe de spans endpoint.
int verifie_masque_endpoint(long long min_echantillon) {
  g_c.relation_spans = 0;
  g_c.relation_all = 0;

  // ---- LES FIXTURES D'EGALITE. `z = a` donne exactement `Phi = 0` : c'est le
  // seul point ou la preuve se casse si elle se casse.
  for (int a = 0; a < g_n && a < 8; ++a)
    for (int b = a + 1; b < g_n && b < a + 4; ++b) {
      const mhgp::P3& P = g_pts[(std::size_t)a];
      const mhgp::P3& Q = g_pts[(std::size_t)b];
      const i128 phi_en_a = (i128)(P.x - P.x) * (Q.x - P.x) + (i128)(P.y - P.y) * (Q.y - P.y) +
                            (i128)(P.z - P.z) * (Q.z - P.z);
      if (phi_en_a != 0)
        return sortie(3, "fixture d'egalite : Phi(a,b,a) ne vaut pas zero");
      if (temoin(a, b, a) || temoin(a, b, b))
        return sortie(3, "fixture d'egalite : une extremite est comptee comme temoin");
    }

  // ---- L'ECHANTILLON DETERMINISTE. Il ne prouve rien que le theoreme ne dise
  // deja ; il atteste que le CODE applique bien la stricte.
  // Le tirage est DIRIGE vers la branche a tester : un tirage uniforme de
  // triplets touche un recouvrement une fois sur trois cents a `n = 8000`, donc
  // il mesurerait surtout sa propre malchance. `C` est construit en descendant
  // depuis la racine VERS `A` — il recouvre `A` par construction — puis en
  // s'arretant a une profondeur tiree.
  const int nb = (int)g_arbre.size();
  for (long long t = 0; t < min_echantillon; ++t) {
    const int A = (int)(melange_ech((std::uint32_t)(3 * t + 11)) % (std::uint32_t)nb);
    const int B = (int)(melange_ech((std::uint32_t)(3 * t + 22)) % (std::uint32_t)nb);
    const int prof = (int)(melange_ech((std::uint32_t)(3 * t + 33)) % 40u);
    int C = 0;
    for (int d = 0; d < prof && !feuille(C); ++d) {
      const int gg = g_arbre[(std::size_t)C].g;
      C = recouvre(gg, A) ? gg : g_arbre[(std::size_t)C].d;
    }
    if (!recouvre(C, A) && !recouvre(C, B)) continue;
    ++g_c.relation_spans;
    if (classifie(A, B, C) == kSpanAll) ++g_c.relation_all;
  }
  std::printf("q2 masque theoreme=invoque fixtures_egalite=ok echantillon=%lld"
              " relation_spans=%lld relation_all=%lld\n",
              min_echantillon, g_c.relation_spans, g_c.relation_all);
  if (g_c.relation_all)
    return sortie(3, "un span endpoint a ete credite : la stricte de Phi_max a ete relachee");
  if (g_c.relation_spans <= 0)
    return sortie(3, "aucun span endpoint tire : l'echantillon serait vide");
  return 0;
}


// ---------------------------------------------------------------------------
// LA DISJONCTION CŒUR / CARRIER, mesuree plutot qu'affirmee.
//
// Contre-audit `f471945` : « un `NONE_OPEN` peut etre elide du sous-etat
// `CoreDepth` ; il ne peut pas l'etre du domaine FUTUR de la lane ». Pour q3/q4
// les carriers vivent du cote OPPOSE au cœur universel, et reutiliser la
// frontiere residuelle du cœur pour les enumerer perdrait non pas quelques cas
// limites mais potentiellement TOUS les supports.
//
// Avec `Phi = -H`, c'est immediat et exact :
//
//   fuseau `W_2`      = `{Phi < 0}`   (angle en `z` obtus)
//   porteur aigu      = `{Phi > 0}` ET lentille   (angle en `x` aigu)
//   shell             = `{Phi = 0}`  (angle droit)
//
// Les deux regions sont DISJOINTES, separees par le shell. Et mes spans
// `OUTSIDE_CLOSED` — ceux que le cœur elimine, `Phi_min > 0` — sont exactement
// ceux qui sont entierement du cote carrier.
//
// Ce mode compte, pour chaque paire, les porteurs aigus reels, et regarde ou
// ils tombent. Si la quasi-totalite est dans la region que le cœur elimine, le
// point de l'auditeur n'est plus une precaution : c'est un chiffre.
int mode_carriers(int h2, long long min_carriers, long long K) {
  (void)h2;
  long long carriers = 0, dans_fuseau = 0, dans_shell = 0, hors_lentille = 0;
  long long paires_avec_carrier = 0, paires_vues = 0;
  // ECHANTILLON DETERMINISTE de paires, pas les `C(n,2)`. La DISJONCTION est un
  // theoreme — `Phi > 0` contre `Phi < 0` — et ne se mesure pas ; ce qui se
  // mesure est la FREQUENCE, et une frequence s'estime sur un tirage.
  for (long long t = 0; t < K; ++t) {
    const int a = (int)(melange_ech((std::uint32_t)(5 * t + 3)) % (std::uint32_t)g_n);
    int b = (int)(melange_ech((std::uint32_t)(5 * t + 7)) % (std::uint32_t)g_n);
    if (b == a) b = (a + 1) % g_n;
    {
      ++paires_vues;
      const mhgp::P3& A = g_pts[(std::size_t)a];
      const mhgp::P3& B = g_pts[(std::size_t)b];
      const i128 dab = (i128)(A.x - B.x) * (A.x - B.x) + (i128)(A.y - B.y) * (A.y - B.y) +
                       (i128)(A.z - B.z) * (A.z - B.z);
      long long ici = 0;
      for (int x = 0; x < g_n; ++x) {
        if (x == a || x == b) continue;
        const mhgp::P3& X = g_pts[(std::size_t)x];
        const i128 dax = (i128)(A.x - X.x) * (A.x - X.x) + (i128)(A.y - X.y) * (A.y - X.y) +
                         (i128)(A.z - X.z) * (A.z - X.z);
        const i128 dbx = (i128)(B.x - X.x) * (B.x - X.x) + (i128)(B.y - X.y) * (B.y - X.y) +
                         (i128)(B.z - X.z) * (B.z - X.z);
        const i128 phi = (i128)(A.x - X.x) * (B.x - X.x) + (i128)(A.y - X.y) * (B.y - X.y) +
                         (i128)(A.z - X.z) * (B.z - X.z);
        // Lentille : `(a,b)` est l'arete MAXIMALE du triangle.
        if (dax > dab || dbx > dab) { if (phi > 0) ++hors_lentille; continue; }
        if (phi > 0) { ++carriers; ++ici; }
        else if (phi == 0) ++dans_shell;
        else ++dans_fuseau;
      }
      if (ici) ++paires_avec_carrier;
    }
  }
  std::printf("q2 carriers paires_vues=%lld total=%lld paires_avec_carrier=%lld"
              " dans_fuseau=%lld dans_shell=%lld hors_lentille=%lld\n",
              paires_vues, carriers, paires_avec_carrier, dans_fuseau, dans_shell,
              hors_lentille);
  // L'INVARIANT : aucun porteur aigu n'est dans le fuseau. Ce n'est pas une
  // mesure statistique, c'est `Phi > 0` contre `Phi < 0`.
  std::printf("q2 carriers intersection_fuseau=0 separation=exacte\n");
  if (carriers < min_carriers)
    return sortie(3, "plancher de porteurs aigus non atteint : la mesure serait vide");
  return 0;
}

// ---------------------------------------------------------------------------
// LA CAMPAGNE D'ECHELLE. `docs/TEST_PLAN_MORSEHGP3D.md` § 3.1.
int mode_echelle(int h2, std::uint64_t cap, std::uint64_t fcap,
                 const std::string& heur, std::uint32_t lot, long long K,
                 long long min_ech, bool exige_echelle) {
  // Le refus est un CODE DE SORTIE, pas un commentaire : une campagne d'echelle
  // qui n'atteint pas 8000 points doit dire pourquoi et echouer.
  if (exige_echelle && g_n < 8000)
    return refus("campagne d'echelle sous 8000 points : les tailles d'interet"
                 " sont 8000, 16000 et 32000");
  g_h2 = h2; g_cap_ech = cap; g_fcap_ech = fcap; g_heur_ech = heur; g_lot_ech = lot;
  g_masse_decidee = 0; g_mortes = 0; g_vivantes = 0;

  // L'echantillon est DETERMINISTE : meme nuage, memes paires suivies.
  g_point_suivi.assign((std::size_t)g_n, 0);
  g_echantillon.clear();
  for (long long i = 0; i < K; ++i) {
    const int a = (int)(melange_ech((std::uint32_t)(2 * i + 1)) % (std::uint32_t)g_n);
    int b = (int)(melange_ech((std::uint32_t)(2 * i + 77777)) % (std::uint32_t)g_n);
    if (b == a) b = (a + 1) % g_n;
    const std::pair<int, int> pr(a < b ? a : b, a < b ? b : a);
    // DEDUPLIQUER : `indice_suivi` rend le premier index correspondant, donc un
    // doublon de tirage laisserait la seconde entree eternellement indecise et
    // ferait echouer la porte pour une raison qui n'est pas la bonne.
    bool deja = false;
    for (std::size_t j = 0; j < g_echantillon.size(); ++j)
      if (g_echantillon[j] == pr) { deja = true; break; }
    if (deja) continue;
    g_echantillon.push_back(pr);
    g_point_suivi[(std::size_t)pr.first] = 1;
    g_point_suivi[(std::size_t)pr.second] = 1;
  }
  g_echantillon_dit.assign(g_echantillon.size(), 0);

  g_flux = true;
  partitionne(0, 0, 64);
  g_flux = false;

  const long long attendue = (long long)g_n * (g_n - 1) / 2;
  std::printf("q2 echelle points=%d h2=%d cap=%llu front_cap=%llu scission=%s lot=%u\n",
              g_n, h2, (unsigned long long)cap, (unsigned long long)fcap,
              heur.c_str(), lot);
  std::printf("q2 echelle masse_decidee=%lld attendue=%lld ecart=%lld mortes=%lld"
              " vivantes=%lld\n",
              g_masse_decidee, attendue, g_masse_decidee - attendue, g_mortes, g_vivantes);
  std::printf("q2 echelle etats=%lld pruned=%lld core_clear=%lld exact_tiles=%lld"
              " witness_splits=%lld classifications=%lld\n",
              g_c.etats, g_c.pruned, g_c.core_clear, g_c.exact_tiles,
              g_c.witness_splits, g_classifications);
  std::printf("q2 echelle frontier_span_count_peak=%lld frontier_candidate_mass_peak=%lld"
              " pending=%lld escalades=%lld cap_violations=%lld\n",
              g_c.frontier_span_count_peak, g_c.frontier_candidate_mass_peak,
              g_c.pending_state_count, g_c.resume_count, g_c.exact_point_cap_violations);

  std::printf("q2 echelle bornes_verifiees=%lld bornes_violees=%lld\n",
              g_c.bornes_verifiees, g_c.bornes_violees);
  std::printf("q2 echelle shell_spans=%lld shell_masse=%lld outside_verifies=%lld"
              " shell_perdus=%lld\n",
              g_c.spans_shell_conserves, g_c.shell_masse, g_c.spans_outside_verifies,
              g_c.shell_perdus);

  // LE JUGE D'ECHANTILLON : `K` paires verifiees EXACTEMENT, en `O(Kn)`.
  long long verifies = 0, ecarts = 0, non_couvertes = 0;
  for (std::size_t i = 0; i < g_echantillon.size(); ++i) {
    if (g_echantillon_dit[i] == 0) { ++non_couvertes; continue; }
    const int a = g_echantillon[i].first, b = g_echantillon[i].second;
    long long cnt = 0;
    for (int z = 0; z < g_n; ++z) if (temoin(a, b, z)) ++cnt;
    const int vrai = (cnt >= h2) ? 1 : 0;
    const int rendu = (g_echantillon_dit[i] == 2) ? 1 : 0;
    ++verifies;
    if (vrai != rendu) ++ecarts;
  }
  std::printf("q2 echelle juge_echantillon K=%zu verifies=%lld ecarts=%lld"
              " non_couvertes=%lld\n",
              g_echantillon.size(), verifies, ecarts, non_couvertes);

  if (g_c.shell_perdus)
    return sortie(1, "un span elimine contenait des points cospheriques : le shell est perdu");
  if (g_verifie_shell && g_c.spans_outside_verifies <= 0)
    return sortie(3, "aucun span OUTSIDE echantillonne : la porte de shell serait vide");
  if (g_c.bornes_violees)
    return sortie(1, "lower <= compte <= upper est viole : une borne est fausse");
  if (g_c.bornes_verifiees <= 0)
    return sortie(3, "aucune borne verifiee : l'invariant serait vide");
  if (g_masse_decidee != attendue)
    return sortie(3, "la masse decidee ne vaut pas C(n,2) : des paires manquent ou doublent");
  if (g_c.exact_point_cap_violations)
    return sortie(3, "une tuile acceptee a depasse le cap d'evaluations ponctuelles");
  if (ecarts) return sortie(1, "le juge d'echantillon contredit la descente");
  if (non_couvertes) return sortie(3, "une paire suivie n'a recu aucune decision");
  if (verifies < min_ech) return sortie(3, "plancher de paires verifiees non atteint");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string fam = "uniform", heur = "buckets", mode = "juge";
  int n = 120, smax = 11, coord = 0;
  long long seed = 3, cap_rect = 64;
  std::uint64_t cap = 4096, front_cap = 64;
  std::uint32_t lot = 1;
  long long min_mortes = 0, min_vivantes = 0, min_pruned = 0, min_clear = 0;
  long long min_relation = 0, min_tuiles = 0, min_scissions = 0, min_shell = 0;
  long long min_carriers = 0, ech = 64, min_ech = 1;
  bool exige_echelle = false;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto v = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--juge" || a == "--masque" || a == "--politiques" ||
        a == "--carriers" || a == "--echelle")
      mode = a.substr(2);
    else if (a == "--verifie-shell") g_verifie_shell = true;
    else if (a.rfind("--points=", 0) == 0) n = std::atoi(v("--points=").c_str());
    else if (a.rfind("--famille=", 0) == 0) fam = v("--famille=");
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(v("--seed=").c_str());
    else if (a.rfind("--smax=", 0) == 0) smax = std::atoi(v("--smax=").c_str());
    else if (a.rfind("--cap-rect=", 0) == 0) cap_rect = std::atoll(v("--cap-rect=").c_str());
    else if (a.rfind("--cap=", 0) == 0) cap = (std::uint64_t)std::atoll(v("--cap=").c_str());
    else if (a.rfind("--front-cap=", 0) == 0) front_cap = (std::uint64_t)std::atoll(v("--front-cap=").c_str());
    else if (a.rfind("--scission=", 0) == 0) heur = v("--scission=");
    else if (a.rfind("--lot=", 0) == 0) lot = (std::uint32_t)std::atoll(v("--lot=").c_str());
    else if (a.rfind("--min-mortes=", 0) == 0) min_mortes = std::atoll(v("--min-mortes=").c_str());
    else if (a.rfind("--min-vivantes=", 0) == 0) min_vivantes = std::atoll(v("--min-vivantes=").c_str());
    else if (a.rfind("--min-pruned=", 0) == 0) min_pruned = std::atoll(v("--min-pruned=").c_str());
    else if (a.rfind("--min-clear=", 0) == 0) min_clear = std::atoll(v("--min-clear=").c_str());
    else if (a.rfind("--min-relation=", 0) == 0) min_relation = std::atoll(v("--min-relation=").c_str());
    else if (a.rfind("--min-tuiles=", 0) == 0) min_tuiles = std::atoll(v("--min-tuiles=").c_str());
    else if (a.rfind("--min-scissions=", 0) == 0) min_scissions = std::atoll(v("--min-scissions=").c_str());
    else if (a.rfind("--min-shell=", 0) == 0) min_shell = std::atoll(v("--min-shell=").c_str());
    else if (a.rfind("--min-carriers=", 0) == 0) min_carriers = std::atoll(v("--min-carriers=").c_str());
    else if (a.rfind("--echantillon=", 0) == 0) ech = std::atoll(v("--echantillon=").c_str());
    else if (a.rfind("--min-echantillon=", 0) == 0) min_ech = std::atoll(v("--min-echantillon=").c_str());
    else if (a == "--exige-echelle") exige_echelle = true;
    else if (a.rfind("--pas-bornes=", 0) == 0) g_pas_bornes = std::atoll(v("--pas-bornes=").c_str());
    else if (a.rfind("--pas-shell=", 0) == 0) g_pas_shell = std::atoll(v("--pas-shell=").c_str());
    else if (a.rfind("--inject=", 0) == 0) {
      const std::string m = v("--inject=");
      g_inject = true;
      if (m == "credit-large") g_mu = Q2Mutant::kCreditLarge;
      else if (m == "none-deux-axes") g_mu = Q2Mutant::kNoneDeuxAxes;
      else if (m == "endpoint-jete") g_mu = Q2Mutant::kEndpointJete;
      else if (m == "mort-large") g_mu = Q2Mutant::kMortLarge;
      else if (m == "boite-cellule") g_mu = Q2Mutant::kBoiteCellule;
      else if (m == "shell-jete") g_mu = Q2Mutant::kShellJete;
      else if (m == "cout-tuile-largeur") g_pf = PfMutant::kCoutTuileLargeur;
      else if (m == "upper-sature-incremental") g_pf = PfMutant::kUpperSatureIncr;
      else if (m == "clear-large") g_pf = PfMutant::kClearLarge;
      else return refus("mutant inconnu");
    } else return refus("argument inconnu");
  }

  // Le mode `--echelle` n'a PAS cette borne : c'est le mode `--juge` qui est
  // borne, parce qu'un oracle exhaustif en `O(n^3)` vaut `5.10^11` operations a
  // `n = 8000`. La borne appartient a l'oracle, pas au produit.
  if (n < 4 || n > 4000000) return refus("points hors de [4, 4000000]");
  if (smax < 3 || smax > 40) return refus("smax hors de [3,40]");
  if (cap_rect < 1) return refus("cap-rect nul : aucun rectangle ne serait accepte");
  if (cap < 1) return refus("cap nul");
  if (front_cap < 1) return refus("front_cap nul");
  if (heur != "buckets" && heur != "plus-gros" && heur != "plus-petit")
    return refus("heuristique de scission inconnue");

  mhgp3v::CloudFamily cf;
  if (fam == "uniform") cf = mhgp3v::CloudFamily::kUniform;
  else if (fam == "terrain") cf = mhgp3v::CloudFamily::kTerrain;
  else if (fam == "eight_clusters") cf = mhgp3v::CloudFamily::kEightClusters;
  else if (fam == "two_lines") cf = mhgp3v::CloudFamily::kTwoLines;
  else if (fam == "scanline_single_pass") cf = mhgp3v::CloudFamily::kScanlineSinglePass;
  else return refus("famille inconnue");

  coord = mhgp3v::cloud_family_default_coord(cf, n);
  g_pts = mhgp3v::make_family_cloud(cf, n, coord, seed);
  g_n = (int)g_pts.size();
  if (g_n < 4) return refus("nuage trop petit");

  // Le PROFIL u16 est une porte d'entree, pas une supposition. Aucune des
  // familles declarees ne le viole aujourd'hui — je l'ai verifie plutot que de
  // l'affirmer —, mais le controle reste : c'est lui qui garantit que la
  // cellule de Morton contient bien la boite serree, donc que le mutant
  // `boite-cellule` est conservateur et non letal.
  for (int i = 0; i < g_n; ++i) {
    const mhgp::P3& p = g_pts[(std::size_t)i];
    if (p.x < 0 || p.y < 0 || p.z < 0 || p.x > 65535 || p.y > 65535 || p.z > 65535)
      return refus("coordonnee hors du profil u16 quantifie");
  }

  std::vector<int> ordre((std::size_t)g_n);
  for (int i = 0; i < g_n; ++i) ordre[(std::size_t)i] = i;
  std::sort(ordre.begin(), ordre.end(), [&](int a, int b) {
    const std::uint64_t ka = morton(g_pts[(std::size_t)a]), kb = morton(g_pts[(std::size_t)b]);
    return ka != kb ? ka < kb : a < b;
  });
  std::vector<mhgp::P3> tries((std::size_t)g_n);
  g_pid.assign((std::size_t)g_n, 0);
  for (int i = 0; i < g_n; ++i) {
    tries[(std::size_t)i] = g_pts[(std::size_t)ordre[(std::size_t)i]];
    g_pid[(std::size_t)i] = ordre[(std::size_t)i];
  }
  g_pts.swap(tries);

  g_arbre.clear();
  construire(0, g_n);

  // EN MODE ECHELLE ON NE MATERIALISE PAS LES RECTANGLES. `C(32000,2)/16` vaut
  // deux milliards d'entrees : la partition est un FLUX, et la faire une
  // premiere fois pour compter la masse doublerait le coût tout en faisant
  // exploser la memoire pour rien.
  if (mode == "echelle")
    return mode_echelle(smax - 1, cap, front_cap, heur, lot, ech, min_ech, exige_echelle);

  g_rects.clear();
  partitionne(0, 0, cap_rect);

  const long long attendue = (long long)g_n * (g_n - 1) / 2;
  long long masse = 0;
  for (std::size_t i = 0; i < g_rects.size(); ++i)
    masse += (long long)pop(g_rects[i].first) * pop(g_rects[i].second);
  std::printf("q2 cadre famille=%s points=%d coord=%d smax=%d h2=%d rectangles=%zu"
              " masse=%lld attendue=%lld ecart=%lld\n",
              fam.c_str(), g_n, coord, smax, smax - 1, g_rects.size(), masse, attendue,
              masse - attendue);
  if (masse != attendue)
    return sortie(3, "la partition des paires ne couvre pas exactement C(n,2)");

  if (mode == "masque") return verifie_masque_endpoint(min_relation);
  if (mode == "carriers") return mode_carriers(smax - 1, min_carriers, ech);
  if (g_n > 400)
    return refus("le juge exhaustif est en O(n^3) : utiliser --echelle au-dela de 400 points");

  const int h2 = smax - 1;
  if (mode == "politiques") {
    // Le resultat ne depend d'aucune politique : trois heuristiques, trois
    // caps, deux lots, et les caps etrangles qui forcent des continuations.
    struct Cfg { const char* h; std::uint64_t cap, fcap; std::uint32_t lot; };
    // Les caps sont SERRES des la reference : avec un cap large, tout part en
    // tuile exacte, aucune scission n'a lieu, et la porte comparerait six fois
    // la meme execution triviale.
    const Cfg cfg[6] = {{"buckets", 8, 64, 1}, {"plus-gros", 8, 64, 1},
                        {"plus-petit", 8, 64, 1}, {"buckets", 8, 64, 4},
                        {"buckets", 4, 8, 1}, {"buckets", 4, 3, 2}};
    std::vector<std::uint8_t> ref;
    long long pend = 0, scan_buckets = -1, scan_vecteur = -1, splits_buckets = -1;
    for (int i = 0; i < 6; ++i) {
      g_c = Compteurs();
      g_classifications = 0;
      Resultat r = descente(h2, cfg[i].cap, cfg[i].fcap, cfg[i].h, cfg[i].lot);
      if (r.faute) return sortie(r.faute, r.diag);
      pend += g_c.pending_state_count;
      std::printf("q2 politique %d scission=%s cap=%llu front_cap=%llu lot=%u"
                  " splits=%lld scan=%lld classifications=%lld pending=%lld\n",
                  i, cfg[i].h, (unsigned long long)cfg[i].cap,
                  (unsigned long long)cfg[i].fcap, cfg[i].lot, g_c.witness_splits,
                  g_c.selector_scan_items, g_classifications, g_c.pending_state_count);
      if (i == 0) { scan_buckets = g_c.selector_scan_items; splits_buckets = g_c.witness_splits; }
      if (i == 1) scan_vecteur = g_c.selector_scan_items;
      if (i == 0) { ref = r.morte; continue; }
      long long d = 0;
      for (std::size_t k = 0; k < ref.size(); ++k) if (ref[k] != r.morte[k]) ++d;
      if (d) return sortie(1, "deux politiques donnent deux ensembles d'aretes");
    }
    std::printf("q2 politiques nb=6 divergences=0 pending_total=%lld"
                " buckets_scan=%lld vecteur_scan=%lld splits=%lld\n",
                pend, scan_buckets, scan_vecteur, splits_buckets);
    if (pend <= 0) return sortie(3, "aucune continuation exercee : la porte serait vide");
    if (splits_buckets <= 0) return sortie(3, "aucune scission : la porte serait vide");
    if (scan_buckets > splits_buckets + g_c.terminal_checks)
      return sortie(3, "le selecteur a buckets n'est pas en O(1) par scission");
    if (scan_vecteur < scan_buckets * 4)
      return sortie(3, "le selecteur vectoriel ne coûte pas plus : la porte de coût serait vide");
    return 0;
  }

  g_c = Compteurs();
  g_classifications = 0;
  Resultat r = descente(h2, cap, front_cap, heur, lot);
  if (r.faute) return sortie(r.faute, r.diag);

  // LE JUGE : force brute `O(n^3)`, sans jamais passer par le ledger.
  long long manquantes = 0, fausses = 0, mortes = 0, vivantes = 0, indecises = 0;
  for (int a = 0; a < g_n; ++a)
    for (int b = a + 1; b < g_n; ++b) {
      long long cnt = 0;
      for (int z = 0; z < g_n; ++z) {
        if (temoin(a, b, z)) ++cnt;
        else if (z != a && z != b) {
          const mhgp::P3& P = g_pts[(std::size_t)a];
          const mhgp::P3& Q = g_pts[(std::size_t)b];
          const mhgp::P3& Z = g_pts[(std::size_t)z];
          const i128 phi = (i128)(P.x - Z.x) * (Q.x - Z.x) + (i128)(P.y - Z.y) * (Q.y - Z.y) +
                           (i128)(P.z - Z.z) * (Q.z - Z.z);
          if (phi == 0) ++g_c.shell_points_juges;   // cospherique EXACT
        }
      }
      const int vrai = (cnt >= h2) ? 1 : 0;
      const std::uint8_t dit = r.morte[(std::size_t)a * g_n + b];
      if (dit == 0) { ++indecises; continue; }
      const int rendu = (dit == 2) ? 1 : 0;
      if (vrai) ++mortes; else ++vivantes;
      if (rendu && !vrai) ++fausses;      // arete tuee a tort : support perdu
      if (!rendu && vrai) ++manquantes;   // arete gardee a tort
    }

  std::printf("q2 descente rectangles=%lld etats=%lld witness_splits=%lld lots=%lld"
              " exact_tiles=%lld classifications=%lld\n",
              g_c.rectangles, g_c.etats, g_c.witness_splits, g_c.witness_batches,
              g_c.exact_tiles, g_classifications);
  std::printf("q2 descente pruned=%lld core_clear=%lld spans_elimines=%lld"
              " selector_scan_items=%lld state_refine_iterations_max=%lld\n",
              g_c.pruned, g_c.core_clear, g_c.spans_elimines, g_c.selector_scan_items,
              g_c.state_refine_iterations_max);
  std::printf("q2 descente frontier_span_count_peak=%lld frontier_candidate_mass_peak=%lld"
              " pending=%lld reprises=%lld octets=%lld\n",
              g_c.frontier_span_count_peak, g_c.frontier_candidate_mass_peak,
              g_c.pending_state_count, g_c.resume_count, g_c.continuation_octets);
  std::printf("q2 descente exact_point_predicate_evaluations=%lld"
              " exact_point_cap_violations=%lld\n",
              g_c.exact_point_predicate_evaluations, g_c.exact_point_cap_violations);
  std::printf("q2 shell spans_conserves=%lld masse=%lld outside_verifies=%lld"
              " perdus=%lld total_cospheriques=%lld\n",
              g_c.spans_shell_conserves, g_c.shell_masse, g_c.spans_outside_verifies,
              g_c.shell_perdus, g_c.shell_points_juges);
  std::printf("q2 juge mortes=%lld vivantes=%lld indecises=%lld manquantes=%lld"
              " fausses=%lld\n", mortes, vivantes, indecises, manquantes, fausses);

  if (g_c.shell_perdus)
    return sortie(1, "un span elimine contenait des points cospheriques : le shell est perdu");
  if (g_verifie_shell && g_c.shell_points_juges < min_shell)
    return sortie(3, "plancher de points cospheriques non atteint : la porte de shell serait vide");
  if (g_c.exact_point_cap_violations)
    return sortie(3, "une tuile acceptee a depasse le cap d'evaluations ponctuelles");
  if (indecises) return sortie(3, "des paires n'ont recu aucune decision");
  if (manquantes || fausses) return sortie(1, "la descente contredit la force brute");
  if (mortes < min_mortes) return sortie(3, "plancher d'aretes mortes non atteint");
  if (vivantes < min_vivantes) return sortie(3, "plancher d'aretes vivantes non atteint");
  if (g_c.pruned < min_pruned) return sortie(3, "plancher de fermetures universelles non atteint");
  if (g_c.core_clear < min_clear) return sortie(3, "plancher de CORE_CLEAR non atteint");
  if (g_c.exact_tiles < min_tuiles) return sortie(3, "plancher de tuiles exactes non atteint");
  if (g_c.witness_splits < min_scissions) return sortie(3, "plancher de scissions non atteint");
  return 0;
}
