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
#include "soc64_rect.hpp"
#include "wspd_front.hpp"
#include "wspd_wavefront.hpp"

namespace {

using mhgp3v::RectLane;
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

// LES SEUILS SONT DERIVES DE `smax`, JAMAIS FIGES. `h_q = smax + 1 - q` : c'est
// ce qui permet de MESURER la dependance de `s` en `K`, au lieu de la deduire
// d'un modele volumique que l'audit a refute.
int g_need[3] = {10, 9, 8};
inline void set_smax(long long smax) {
  for (int q = 0; q < 3; ++q) g_need[q] = (int)(smax + 1 - (q + 2));
}

struct Pair { int a, b; int r = 0; };   // `r` : profondeur de RAFFINEMENT local

// Banque Morton bornee, fusionnee a l'emission : des qu'une paire est declaree
// terminale, le MEME thread calcule son `Dlo`, lit sa fenetre Morton et
// applique le masque central. Le rectangle n'est jamais materialise en memoire ;
// seuls les residuels sont compactes. C'est le gain de bande passante du
// kernel vise.
struct BankStat { long long spindle_all = 0, spindle_essais = 0, spindle_tabs = 0, reads = 0, recerts = 0, tronques = 0, juges = 0, faux = 0, v_all = 0, v_none = 0, v_descente = 0, closed[3] = {0, 0, 0}; };

// ---- `SOC64-shadow-q4` : COMPTEURS SEPARES, AUCUN FATE TOUCHE.
//
// Ils vivent dans leur propre structure et non dans `BankStat`, pour qu'aucune
// ligne de sortie existante ne bouge et qu'aucune porte en place ne les lise
// par accident. Le shadow ne consomme ni le quantum `exp`, ni la pile, ni les
// compteurs `reads`/`recerts` : il regarde exactement les taches que le
// certificat central a laissees `MIXED` en q4 et dit ce qu'il en ferait.
//
// AVERTISSEMENT PORTE PAR LE RECU. Ces nombres sont une ESTIMATION A ORDRE DE
// VISITE CONSTANT, pas un rejeu. Un vrai branchement de SOC64 arreterait la
// descente sur un nœud ferme et rendrait son budget a d'autres sous-arbres ;
// le shadow, lui, continue de descendre. Le masque `socm` empeche seulement le
// double comptage d'un nœud deja couvert par un ancetre.
// LES DEUX LEDGERS SONT SEPARES, ET C'EST LA REPARATION EXIGEE PAR L'AUDIT.
//
// Sous `--raffine`, `certifier` est appele pendant la phase de COMPTAGE sur des
// rectangles qui ne sont pas des terminaux, puis ceux-ci sont scindes et leurs
// enfants sont certifies a leur tour. Un compteur incremente dans le
// certificateur compte donc des TENTATIVES : le parent, puis ses deux enfants,
// puis leurs quatre petits-enfants. C'est exactement le defaut que
// `AUDIT_ETAT_COURANT.md` releve sur `bank.closed`, `pending_lane` et
// `mass_closed_q2`, et qui leur fait imprimer jusqu'a 380 % de masse fermee.
//
// La premiere version de ce shadow reproduisait la faute : elle publiait
// 157,67 % de masse q4 fermee sur `eight_clusters` a profondeur quatre. Le
// ledger TERMINAL ci-dessous n'est alimente qu'au seul endroit ou `fate` et
// `pend` sont ecrits, c'est-a-dire pour les paires effectivement terminales.
struct SocShadowStat {
  // --- AttemptStats : une ligne par appel du certificateur.
  long long taches = 0;        // taches (rectangle, CNode) MIXED en q4 soumises
  long long all = 0;           // SOC64 rend q4 sur toute la boite relaxee
  long long couples = 0;       // couples de coins reellement evalues
  long long early = 0;         // sorties anticipees
  long long masse_creditee = 0;    // population temoin creditee, sans double compte
  long long tentatives_fermees = 0;  // appels ou le shadow aurait franchi le seuil
  // --- TerminalLedger : une ligne par paire TERMINALE, exclusif et final.
  long long fermetures = 0;    // terminaux q4 qui passeraient de OUVERT a FERME
  long long masse_fermee = 0;  // masse de paires portee par ces terminaux
  // --- Juge independant.
  long long juges = 0;         // verdicts `ALL` effectivement enumeres
  long long juges_sautes = 0;  // verdicts trop gros pour le cap : NON juges
  long long faux = 0;          // triples reels refutant un verdict `ALL`
  long long triples = 0;       // triples enumeres par le juge
  long long invariant_viole = 0;  // ccred < cred : les ledgers ont diverge
  long long cap_refuses = 0;      // taches refusees par le cap : ledger NON final
  long long wide = 0;             // multiplications 128 bits reellement formees
  // --- LE LEDGER FAUTIF, CONSERVE COMME TEMOIN.
  //
  // C'est l'ecriture que le contre-audit a refutee : additionner `cred[2]` et
  // une somme brute de populations SOC64. Les deux ensembles ne sont pas
  // disjoints — un descendant central `ALL` recompte des temoins deja comptes
  // par un ancetre SOC `ALL`. On le calcule EN PARALLELE, sans jamais le
  // publier comme resultat, pour que l'ecart soit MESURE et garde par une
  // porte. Une faute qu'on retire sans laisser de temoin revient.
  long long fermetures_brutes = 0;
  long long masse_fermee_brute = 0;
};

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
// Le masque central est suffisant, jamais complet ; le repli est un SECOND
// certificat suffisant, non comparable. Leur disjonction reste suffisante.
bool g_fallback = false;
bool g_spindle = false;
// ---- RAFFINEMENT LOCAL : la seule voie qui reste vers le cœur exact.
//
// Le spindle sur rectangle est refute — son gain mesure est nul, parce que
// `Tabs` est le MINIMUM de `|T|` sur le bloc alors que le gain par paire vient
// des sites ou `|T|` est MAXIMAL. La seule facon de recuperer le cœur exact est
// donc de RETRECIR les boites jusqu'a ce que l'intervalle cesse d'etre domine
// par sa propre largeur. On certifie d'abord, on scinde ensuite, et seulement
// les terminaux que la lane cible laisse OUVERTS.
long long g_raffine = 0;
int g_raffine_lane = 2;
bool g_bank = false;
long long g_win = 32, g_bankl = 16;
long long g_warms = 0;
long long g_inflation = 0;
int g_infl_lane = 0;   // lane dont on echantillonne les terminaux OUVERTS
bool g_descent = false;
bool g_vwave = false;
bool g_inject_global = false;
bool g_climb = false;
bool g_judge_vwave = false;
// `SOC64-shadow-q4`, counter-only. Il n'a de sens que dans la branche VWave :
// c'est la seule qui produise des taches de BOITE `MIXED`. Les deux autres
// branches classent des points, ou SOC64 degenere en un seul couple.
bool g_soc64_shadow = false;
long long g_min_soc_taches = 0;
// JUGE DU SHADOW. Un verdict `ALL` de SOC64 affirme que TOUT point du nœud
// temoin est un temoin q4 de TOUTE paire du rectangle. Le juge le verifie par
// enumeration des vrais points, dans la representation historique `(g,Q)` de
// `spindle_cone.hpp`, qui ne partage AUCUNE quantite intermediaire avec le
// sujet : ni H, ni E, ni X, ni les differences de Minkowski. Une faute commune
// ne peut donc pas s'y compenser.
bool g_judge_soc64 = false;
// CAP DETERMINISTE DU SHADOW. Le contre-audit exige `max_soc_tasks <= 4096`
// pour le diagnostic initial et le statut `PENDING` au cap : sans lui, le claim
// « bon marche » n'est pas recevable, puisque les runs live soumettent jusqu'a
// 48,6 millions de taches. Au cap, la vue combinee cesse d'etre creditee et le
// ledger publie n'est plus final — il devient un MINORANT explicite.
long long g_soc_cap = 0;
long long g_soc_judge_cap = 4096;   // triples enumeres par verdict juge
// Echantillon scelle de porteurs aigus : le premier chiffre de `M3`/`M4`.
long long g_m4_sample = 0;
long long g_min_m4_echantillon = 0;
// `r_e` : nombre de PAIRES tirees dans chaque lentille pour l'etage interieur.
// Il borne le cout sans jamais censurer une arete de la moyenne.
long long g_apex_sample = 0;
// Oracle exhaustif de `C4`, par un parcours independant des terminaux.
long long g_porteurs_oracle = 0;
// Graine SCELLEE de l'estimateur. Elle est publiee avec le resultat : une
// estimation dont on ne peut pas rejouer le tirage n'est pas verifiable.
long long g_porteurs_seed = 1;
// Budget d'apex bien centres dont on calcule le RANG exact : le seul filtre qui
// separe une masse de candidats d'une sortie utile.
long long g_rang_budget = 0;
// MUTANT : imprimer `C4` sous le libelle `M4`. C'est exactement la faute que la
// version `v0` de ce compteur commettait, et qu'un contre-audit a du relever.
// Une faute corrigee sans mutant revient.
bool g_inject_c4_comme_m4 = false;

// ---- `EdgeWindowRangeAdd-v0` : la fenetre d'aretes, EXACTE et en `O(F+n)`.
//
// J'avais ecrit que le maximum de `E_q(a)` exigeait de developper `|A||B|` par
// terminal, donc la masse. C'est faux, et le contre-audit `ab32c9d` donne la
// raison : chaque nœud du radix tree porte une plage CONTIGUE de
// `GenerationRank`, les terminaux sont des produits de plages DISJOINTES, donc
// tout terminal `A x B` verifie exactement l'un des deux ordres totaux
// `last(A) < first(B)` ou `last(B) < first(A)`. Sous l'orientation canonique
// `b > a`, le terminal credite alors une plage CONTIGUE d'ancres d'une valeur
// CONSTANTE : c'est un range-add, deux ecritures, suivi d'un scan prefixe.
//
// Ce n'est pas une optimisation de confort. `sum_a E_4(a)` est le nombre exact
// d'aretes candidates que `LocalShallowBall` devrait traiter, et `max_a E_4(a)`
// le pire fan-out par ancre. Ces deux nombres decident si le moteur shallow est
// finançable ou non ; les mesurer sans les developper est la seule facon de les
// connaitre a `n = 50 000`.
bool g_window = false;
bool g_inject_orient = false;
bool g_inject_cote = false;

// ---- `Q3AcuteCarrierWave-v0` : LE SECOND NIVEAU, SUR LES PORTEURS.
//
// q3 n'est PAS une relation ternaire. Canonicalise par son arete maximale, un
// support q3 est une relation BINAIRE `(arete ab) x (porteur x)`. Deux faits,
// verifies sans contre-exemple, la rendent presque gratuite :
//
//  1. si `ab` est l'arete maximale, le triangle est aigu SSI l'angle en `x`
//     l'est — les deux autres angles sont automatiques ;
//  2. et cet angle est deja calcule : aigu SSI `V^2 > D^2`, avec
//     `V = ||2x-a-b||`. C'est EXACTEMENT le test du cœur q2 au signe pres, sur
//     la meme quantite entiere. Aucune algebre nouvelle.
//
// Le porteur vit donc dans `lentille(ab)` PRIVEE de la boule diametrale, region
// de diametre `sqrt(3) D`. Avec des cellules porteuses de cote `D/s`, il y en a
// `O(s^3)` par arete, donc `O(s^6 n)` blocs au total.
//
// TROIS CHOIX FONT LA BORNE.
//
//  (a) La separation de niveau deux n'est pas de meme nature que celle de la
//      WSPD : ce n'est pas « deux cellules eloignees » mais « la cellule
//      porteuse est petite devant la LONGUEUR D'ARETE », `diam(C) <= Dmin/s`.
//
//  (b) L'ACUITE NE SERT QUE DE PRUNE, jamais de certificat. C'est ce qui sauve
//      la borne : l'audit `4ce3618` note que `O(s^6 n)` ne vaut que « sans
//      raffinement des MIXED », et justement la frontiere aigu/obtus n'a jamais
//      besoin d'etre raffinee. Un bloc MIXED en acuite est conserve. Proposer
//      un triplet obtus est fail-open : son enveloppe minimale est la boule
//      diametrale de sa plus longue arete, donc c'est un support q2 et il meurt
//      au test de positivite exact en aval. Le prune est STRICT, `V^2_max <
//      D^2_min`, ce qui preserve aussi les angles droits pour la coquille q2.
//
//  (c) Ce qui FERME un bloc n'est pas l'acuite mais le rang, au niveau un, sur
//      la paire. Ce niveau-ci ne ferme rien : il enumere.
bool g_q3carriers = false;
long long g_q3s = 4;
long long g_q3oracle = 0;
long long g_min_blocs = 0;
long long g_oracle_window = 0;
long long g_min_open = 0;
long long g_min_orient = 0;
long long g_min_closed = 0;
double g_max_slope_e4 = 0.0;

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

// ---- `JungSpindleSingleton-v0` : le cœur anisotrope, et son juge independant.
//
// Le certificat en production teste `209 V2 <= 56 D2`. C'est la BOULE INSCRITE :
// le pire cas directionnel de la vraie condition, obtenu en supprimant le terme
// favorable `-4 (d.v)^2`. La condition exacte sur le disque de Jung est
//
//     L2 > V2   et   (L2-V2)^2 > 2 (V2 L2 - P^2),   P = d . v,
//
// qui redonne `V2/L2 < 2-sqrt(3)` sur le plan mediateur et la boule diametrale
// entiere sur l'axe de `ab`.
//
// PORTEE EXACTE, telle que l'audit l'exige : ce certificat est exact SUR LE
// DISQUE DE JUNG et SOUS OWNER MAXIMAL. Il n'est pas « exact pour toute sphere
// passant par la paire » : le domaine reel des centres peut etre plus petit, ce
// qui le rend suffisant, jamais complet.
struct SpindleTriple { long long a[3], b[3], z[3]; };

inline bool spindle_inside(const long long a[3], const long long b[3], const long long z[3]) {
  long long l2 = 0, v2 = 0, p = 0;
  for (int i = 0; i < 3; ++i) {
    const long long d = b[i] - a[i];
    const long long v = 2 * z[i] - a[i] - b[i];
    l2 += d * d; v2 += v * v; p += d * v;
  }
  if (l2 <= v2) return false;
  const __int128 g = (__int128)(l2 - v2) * (l2 - v2);
  return g > 2 * ((__int128)v2 * l2 - (__int128)p * p);
}

inline bool inscribed_inside(const long long a[3], const long long b[3], const long long z[3]) {
  long long l2 = 0, v2 = 0;
  for (int i = 0; i < 3; ++i) {
    const long long d = b[i] - a[i];
    const long long v = 2 * z[i] - a[i] - b[i];
    l2 += d * d; v2 += v * v;
  }
  return (__int128)209 * v2 <= (__int128)56 * l2;
}

// JUGE INDEPENDANT. Il n'emploie AUCUNE des deux algebres ci-dessus : il balaie
// des centres `c = m + t` du disque de Jung sur une grille entiere et teste
// directement `|z-c|^2 < L2/4 + |t|^2`. S'il trouve un centre qui EXCLUT `z`,
// tout certificat qui aurait credite `z` est refute. Un balayage ne peut que
// falsifier, jamais prouver l'appartenance — c'est le bon sens pour un juge.
// Rend `true` si un centre excluant a ete trouve.
inline bool jung_sweep_excludes(const long long a[3], const long long b[3],
                                const long long z[3], long long grid) {
  long long d[3], l2 = 0;
  for (int i = 0; i < 3; ++i) { d[i] = b[i] - a[i]; l2 += d[i] * d[i]; }
  // Base entiere de `d^perp`, comme `edge_shallow.hpp` : deux produits croises
  // avec les axes autres que la composante dominante de `d`.
  const long long ax = d[0] < 0 ? -d[0] : d[0];
  const long long ay = d[1] < 0 ? -d[1] : d[1];
  const long long az = d[2] < 0 ? -d[2] : d[2];
  int e1, e2;
  if (ax >= ay && ax >= az) { e1 = 1; e2 = 2; }
  else if (ay >= az) { e1 = 0; e2 = 2; }
  else { e1 = 0; e2 = 1; }
  auto cross = [&](int axis, long long out[3]) {
    long long u[3] = {0, 0, 0};
    u[axis] = 1;
    out[0] = d[1] * u[2] - d[2] * u[1];
    out[1] = d[2] * u[0] - d[0] * u[2];
    out[2] = d[0] * u[1] - d[1] * u[0];
  };
  long long q1[3], q2v[3];
  cross(e1, q1);
  cross(e2, q2v);
  // `t = (alpha q1 + beta q2)/K`. Tout est multiplie par `K` puis par `4` pour
  // absorber le demi du milieu ; aucun flottant, aucune division.
  const long long K = grid;
  for (long long alpha = -K; alpha <= K; ++alpha)
    for (long long beta = -K; beta <= K; ++beta) {
      long long w[3], t2 = 0;
      for (int i = 0; i < 3; ++i) w[i] = alpha * q1[i] + beta * q2v[i];
      for (int i = 0; i < 3; ++i) t2 += w[i] * w[i];
      // `|t|^2 = |w|^2/K^2 <= L2/8`, admissibilite du disque de Jung q4.
      if ((__int128)8 * t2 > (__int128)l2 * K * K) continue;
      // `|z - m - t|^2 < L2/4 + |t|^2`, tout multiplie par `(2K)^2`.
      __int128 lhs = 0;
      for (int i = 0; i < 3; ++i) {
        const __int128 e = (__int128)K * (2 * z[i] - a[i] - b[i]) - 2 * (__int128)w[i];
        lhs += e * e;
      }
      const __int128 rhs = (__int128)l2 * K * K + 4 * (__int128)t2;
      if (lhs >= rhs) return true;             // ce centre EXCLUT `z`
    }
  return false;
}

}  // namespace

// Les deux fixtures GRAVEES du spindle, aux coordonnees exactes.
//
// A — SURETE, fixture de l'audit `b96751c` §1.2. Dix points strictement dans la
// boule diametrale de `ab` et strictement HORS de la circumsphere q4 de
// `{a,b,x,y}`. Aucun certificat q4 ne doit les crediter : « dix temoins q2 »
// n'est pas un certificat q4, et c'est precisement la confusion que je faisais.
//
// B — NON-VACUITE. Un point de l'AXE, que la boule inscrite rejette et que le
// spindle credite. Sans lui, un spindle qui n'accepterait jamais rien passerait
// la fixture A sans rien prouver.
int spindle_fixtures() {
  const long long a[3] = {100, 100, 100}, b[3] = {200, 100, 100};
  const long long x[3] = {150, 30, 120}, y[3] = {150, 30, 80};
  int fautes = 0, credites_spindle = 0, rejetes_avec_temoin = 0;

  // L'arete `ab` doit etre l'unique arete maximale du tetraedre.
  {
    auto d2 = [](const long long u[3], const long long v[3]) {
      long long s = 0;
      for (int i = 0; i < 3; ++i) { const long long w = u[i] - v[i]; s += w * w; }
      return s;
    };
    const long long ab = d2(a, b);
    const long long autres[5] = {d2(a, x), d2(a, y), d2(b, x), d2(b, y), d2(x, y)};
    for (int i = 0; i < 5; ++i)
      if (autres[i] >= ab) {
        std::fprintf(stderr, "FIXTURE: `ab`=%lld n'est pas l'unique arete maximale (%lld)\n",
                     ab, autres[i]);
        ++fautes;
      }
    if (ab != 10000) { std::fprintf(stderr, "FIXTURE: ab^2=%lld attendu 10000\n", ab); ++fautes; }
  }

  // A — les dix satellites.
  for (long long i = -4; i <= 5; ++i) {
    const long long z[3] = {150 + i, 140, 100};
    long long v2 = 0;
    for (int k = 0; k < 3; ++k) { const long long v = 2 * z[k] - a[k] - b[k]; v2 += v * v; }
    if (v2 >= 10000) {                       // doit etre DANS la boule diametrale
      std::fprintf(stderr, "FIXTURE A: z_%lld hors boule diametrale (V2=%lld)\n", i, v2);
      ++fautes;
    }
    // hors de la circumsphere `c=(150,80,100)`, `R^2=2900`
    long long dc = 0;
    const long long c[3] = {150, 80, 100};
    for (int k = 0; k < 3; ++k) { const long long w = z[k] - c[k]; dc += w * w; }
    if (dc <= 2900) {
      std::fprintf(stderr, "FIXTURE A: z_%lld dans la circumsphere (d2=%lld)\n", i, dc);
      ++fautes;
    }
    if (inscribed_inside(a, b, z)) {
      std::fprintf(stderr, "FIXTURE A: la boule inscrite credite z_%lld\n", i);
      ++fautes;
    }
    if (spindle_inside(a, b, z)) {
      std::fprintf(stderr, "FIXTURE A: le spindle credite z_%lld, refute par la"
                           " circumsphere q4\n", i);
      ++fautes;
    }
    if (jung_sweep_excludes(a, b, z, 24)) ++rejetes_avec_temoin;
  }

  // B — le point de l'axe.
  {
    const long long z[3] = {110, 100, 100};
    if (inscribed_inside(a, b, z)) {
      std::fprintf(stderr, "FIXTURE B: la boule inscrite credite deja l'axe, la"
                           " fixture ne prouve plus rien\n");
      ++fautes;
    }
    if (!spindle_inside(a, b, z)) {
      std::fprintf(stderr, "FIXTURE B: le spindle ne credite pas l'axe\n");
      ++fautes;
    } else {
      ++credites_spindle;
      if (jung_sweep_excludes(a, b, z, 24)) {
        std::fprintf(stderr, "DESACCORD DU JUGE: le balayage de Jung exclut un point"
                             " credite par le spindle\n");
        ++fautes;
      }
    }
  }

  // PLANCHERS : sans eux, un spindle qui n'accepte rien passerait la fixture A.
  if (credites_spindle < 1) {
    std::fprintf(stderr, "PLANCHER: aucun point credite par le spindle\n");
    return 3;
  }
  if (rejetes_avec_temoin < 10) {
    std::fprintf(stderr, "PLANCHER: %d rejets sur 10 justifies par un centre excluant\n",
                 rejetes_avec_temoin);
    return 3;
  }
  if (fautes) {
    std::fprintf(stderr, "DESACCORD DU JUGE: %d fautes de fixture\n", fautes);
    return 1;
  }
  std::printf("fixtures_spindle accord=OUI credites=%d rejets_justifies=%d\n",
              credites_spindle, rejetes_avec_temoin);
  return 0;
}

// FIXTURE GRAVEE — toute coupure de partenaires par RANG est refutee.
//
// C'est ma propre proposition que cette fixture ferme. J'avais demande si borner
// la liste de partenaires par un rang supprimerait le `|lens|^2` sans rien
// changer au front. L'audit `b96751c` §2 rend un contre-exemple u16 exact : un
// support q4 POSITIF, de profondeur ZERO, d'arete maximale UNIQUE, dont le
// second endpoint est au-dela du rang `4381`. Aucune coupure de rang ne peut
// donc etre exacte, quel que soit le seuil choisi.
int rang_fixture() {
  const long long c[3] = {30000, 30000, 30000};
  const long long a[3] = {5000, 40000, 30000}, b[3] = {55000, 40000, 30000};
  const long long x[3] = {30000, 5000, 40000}, y[3] = {30000, 5000, 20000};
  const long long R2 = 725000000LL;
  const long long kSat = 4381;
  int fautes = 0;
  auto d2 = [](const long long u[3], const long long v[3]) {
    long long s = 0;
    for (int i = 0; i < 3; ++i) { const long long w = u[i] - v[i]; s += w * w; }
    return s;
  };

  // Les quatre sommets sont EXACTEMENT sur la sphere.
  const long long* q[4] = {a, b, x, y};
  const char* nom[4] = {"a", "b", "x", "y"};
  for (int i = 0; i < 4; ++i)
    if (d2(q[i], c) != R2) {
      std::fprintf(stderr, "FIXTURE RANG: %s hors sphere (%lld != %lld)\n",
                   nom[i], d2(q[i], c), R2);
      ++fautes;
    }

  // `ab` est l'UNIQUE arete maximale : aucun tie-break d'owner n'est en jeu.
  const long long ab = d2(a, b);
  if (ab != 2500000000LL) {
    std::fprintf(stderr, "FIXTURE RANG: ab^2=%lld attendu 2500000000\n", ab);
    ++fautes;
  }
  const long long autres[5] = {d2(a, x), d2(a, y), d2(b, x), d2(b, y), d2(x, y)};
  for (int i = 0; i < 5; ++i)
    if (autres[i] >= ab) {
      std::fprintf(stderr, "FIXTURE RANG: arete %lld >= ab %lld\n", autres[i], ab);
      ++fautes;
    }

  // Les satellites sont STRICTEMENT hors de la circumsphere — le support est
  // donc de profondeur zero — et STRICTEMENT plus proches de `a` que `b`.
  //
  // CORRECTION de l'audit `92d0c0f` : ma premiere version ne comptait que les
  // satellites et affirmait le rang `4382`. Or `x` et `y` sont EUX AUSSI plus
  // proches de `a` que ne l'est `b` — `1,95e9` contre `2,5e9`. Le rang
  // 1-indexe de `b` vaut donc `4384`. J'avais oublie les deux autres sommets du
  // support lui-meme.
  long long plus_proches = 0;
  if (d2(a, x) < ab) ++plus_proches;
  if (d2(a, y) < ab) ++plus_proches;
  const long long attendus_hors_satellites = plus_proches;
  for (long long j = 1; j <= kSat; ++j) {
    const long long z[3] = {5000, 40000 + j, 30000};
    if (z[1] > 65535) {
      std::fprintf(stderr, "FIXTURE RANG: satellite hors u16\n");
      ++fautes;
      break;
    }
    if (d2(z, c) <= R2) {
      std::fprintf(stderr, "FIXTURE RANG: satellite j=%lld dans la circumsphere\n", j);
      ++fautes;
    }
    if (d2(a, z) < ab) ++plus_proches;
  }
  if (plus_proches != kSat + attendus_hors_satellites) {
    std::fprintf(stderr, "FIXTURE RANG: %lld voisins plus proches de `a` que `b`,"
                         " %lld attendus\n", plus_proches, kSat + attendus_hors_satellites);
    ++fautes;
  }
  if (attendus_hors_satellites != 2) {
    std::fprintf(stderr, "FIXTURE RANG: `x` et `y` devraient tous deux preceder `b`\n");
    ++fautes;
  }
  if (fautes) {
    std::fprintf(stderr, "DESACCORD DU JUGE: %d fautes de fixture de rang\n", fautes);
    return 1;
  }
  // LE RANG DE `b` VU DE `a`. `plus_proches` voisins le precedent strictement,
  // donc toute coupure au rang `plus_proches` ou en deca perd ce support.
  std::printf("fixtures_rang accord=OUI rang_du_partenaire=%lld profondeur=0"
              " arete_maximale_unique=OUI\n", plus_proches + 1);
  return 0;
}

// ---- LE TERME DIRECTIONNEL SUR UN RECTANGLE, EXACT.
//
// C'est la reponse de l'audit `b96751c` §6 a la question qui me bloquait :
// comment minorer `(d.v)^2` sur `A x B x C` alors que `d` et `v` dependent tous
// deux de `a` et `b`. L'identite qui debloque tout est
//
//     T = d . v = ||z-a||^2 - ||z-b||^2,
//
// car elle est SEPARABLE PAR AXE : `a`, `b` et `z` choisissent leurs
// coordonnees independamment dans des boites alignees, donc l'intervalle exact
// de la somme est la somme des intervalles exacts par axe.
//
// Par axe, `min_z [dist(z,A)^2 - far(z,B)^2]` et `max_z [far(z,A)^2 -
// dist(z,B)^2]`. Sur chaque morceau ces fonctions sont LINEAIRES — les termes
// en `z^2` s'annulent — sauf quand `z` est dans l'intervalle de la distance,
// ou elles deviennent concaves. Les candidats sont donc en nombre CONSTANT :
// les bornes de `C`, les ruptures de la distance, et les deux entiers voisins
// du milieu qui separe les deux endpoints les plus lointains.
//
// TESTER LES SEULS COINS EST FAUX, et l'audit en donne le contre-exemple : sur
// `A=[0,1]`, `B=[0,3]`, `C=[0,2]`, le maximum exact vaut `4` en `(0,2,2)` alors
// que les huit choix d'extremites ne donnent que `3`. C'est une fixture.
// FIXTURE ET AUDIT DE L'AUDIT. La prescription ci-dessus est ce sur quoi le
// moteur va reposer ; je ne la recois pas sur parole. Elle est donc comparee a
// une enumeration EXHAUSTIVE de tous les `(a,b,z)` sur des boites petites, et
// le contre-exemple des coins est verifie separement.
int terme_t_fixtures() {
  int fautes = 0;
  // (1) Le contre-exemple de l'audit, en une dimension.
  {
    long long lo = 0, hi = 0;
    mhgp3v::rect_t_axis(0, 1, 0, 3, 0, 2, &lo, &hi);
    long long vrai_lo = 1LL << 60, vrai_hi = -(1LL << 60), coins = -(1LL << 60);
    for (long long a = 0; a <= 1; ++a)
      for (long long b = 0; b <= 3; ++b)
        for (long long z = 0; z <= 2; ++z) {
          const long long t = (z - a) * (z - a) - (z - b) * (z - b);
          if (t < vrai_lo) vrai_lo = t;
          if (t > vrai_hi) vrai_hi = t;
          if ((a == 0 || a == 1) && (b == 0 || b == 3) && (z == 0 || z == 2))
            if (t > coins) coins = t;
        }
    if (lo != vrai_lo || hi != vrai_hi) {
      std::fprintf(stderr, "FIXTURE T: intervalle [%lld,%lld] != exact [%lld,%lld]\n",
                   lo, hi, vrai_lo, vrai_hi);
      ++fautes;
    }
    if (vrai_hi != 4 || coins != 3) {
      std::fprintf(stderr, "FIXTURE T: le contre-exemple des coins ne se reproduit pas"
                           " (exact=%lld coins=%lld, attendus 4 et 3)\n", vrai_hi, coins);
      ++fautes;
    }
  }
  // (2) Audit exhaustif en trois dimensions, sur des boites tirees.
  unsigned long long rng = 0xD1B54A32D192ED03ull;
  auto next = [&](long long mod) {
    rng = rng * 6364136223846793005ull + 1442695040888963407ull;
    return (long long)((rng >> 33) % (unsigned long long)mod);
  };
  long long cas = 0, serres = 0;
  for (int essai = 0; essai < 3000; ++essai) {
    long long al[3], ah[3], bl[3], bh[3], cl[3], ch[3];
    for (int i = 0; i < 3; ++i) {
      al[i] = next(9) - 4; ah[i] = al[i] + next(4);
      bl[i] = next(9) - 4; bh[i] = bl[i] + next(4);
      cl[i] = next(9) - 4; ch[i] = cl[i] + next(4);
    }
    long long lo = 0, hi = 0;
    for (int i = 0; i < 3; ++i) {
      long long l = 0, h = 0;
      mhgp3v::rect_t_axis(al[i], ah[i], bl[i], bh[i], cl[i], ch[i], &l, &h);
      lo += l; hi += h;
    }
    // Verite : enumeration complete de `A x B x C`.
    long long vlo = 1LL << 60, vhi = -(1LL << 60);
    for (long long ax = al[0]; ax <= ah[0]; ++ax)
     for (long long ay = al[1]; ay <= ah[1]; ++ay)
      for (long long az = al[2]; az <= ah[2]; ++az)
       for (long long bx = bl[0]; bx <= bh[0]; ++bx)
        for (long long by = bl[1]; by <= bh[1]; ++by)
         for (long long bz = bl[2]; bz <= bh[2]; ++bz)
          for (long long zx = cl[0]; zx <= ch[0]; ++zx)
           for (long long zy = cl[1]; zy <= ch[1]; ++zy)
            for (long long zz = cl[2]; zz <= ch[2]; ++zz) {
              const long long d[3] = {bx - ax, by - ay, bz - az};
              const long long v[3] = {2 * zx - ax - bx, 2 * zy - ay - by, 2 * zz - az - bz};
              const long long t = d[0] * v[0] + d[1] * v[1] + d[2] * v[2];
              if (t < vlo) vlo = t;
              if (t > vhi) vhi = t;
            }
    ++cas;
    if (lo != vlo || hi != vhi) {
      std::fprintf(stderr, "DESACCORD DU JUGE: intervalle T [%lld,%lld] != exhaustif"
                           " [%lld,%lld]\n", lo, hi, vlo, vhi);
      if (++fautes > 5) break;
    }
    if (lo == vlo && hi == vhi && vlo != vhi) ++serres;
  }
  // PLANCHER : sans lui, des boites toutes degenerees rendraient l'accord vide.
  if (cas < 3000 || serres < 2500) {
    std::fprintf(stderr, "PLANCHER: %lld cas, %lld intervalles non triviaux\n", cas, serres);
    return 3;
  }
  if (fautes) {
    std::fprintf(stderr, "DESACCORD DU JUGE: %d fautes sur le terme directionnel\n", fautes);
    return 1;
  }
  std::printf("fixtures_terme_t accord=OUI cas=%lld non_triviaux=%lld"
              " contre_exemple_coins=OUI\n", cas, serres);
  return 0;
}

// ---- L'OWNER DOIT ETRE GLOBAL, SURTOUT SUR LES EGALITES.
//
// Choisir « une arete maximale faible » ne rend pas l'emission unique : un
// triangle isocele en a deux, un equilateral trois. L'owner exact est
// (1) maximiser la longueur carree, (2) a egalite, la plus petite `EdgeKey`
// formee des deux `PointId` tries.
//
// Mon oracle de niveau deux remettait son compteur de couverture a zero pour
// CHAQUE terminal WSPD. Il recevait donc l'unicite d'un porteur relativement a
// une incidence d'arete, jamais l'unicite d'un `SupportKey` a travers les
// terminaux. L'audit `92d0c0f` §2 a raison, et la fixture est son equilateral.
// L'OWNER SE DECIDE SUR LES `PointId`, JAMAIS SUR LES POSITIONS MORTON.
//
// Ma premiere version comparait `u, v, w`, qui sont des positions dans `sp`,
// donc des `GenerationRank`. Le tableau `spid`, qui porte les vrais `PointId`,
// n'etait jamais transmis. La fixture passait parce que son ordre de vecteur
// coincide avec son ordre de labels — et surtout parce que le sujet ET la
// verite appelaient la MEME fonction, donc s'accordaient sur le meme mauvais
// tie-break. L'audit `1aa487d` le prend en flagrant delit.
//
// `lab` traduit une position en `PointId`. Le tie-break emploie l'`EdgeKey`
// formee des deux labels tries.
inline bool owner_edge(const std::vector<std::array<long long, 3>>& p,
                       const std::vector<int>& lab, int u, int v, int w) {
  auto d2 = [&](int i, int j) {
    long long s = 0;
    for (int k = 0; k < 3; ++k) { const long long d = p[i][k] - p[j][k]; s += d * d; }
    return s;
  };
  const int e[3][2] = {{u, v}, {u, w}, {v, w}};
  long long best = -1;
  int bi = -1, bj = -1;
  for (int i = 0; i < 3; ++i) {
    const long long d = d2(e[i][0], e[i][1]);
    const int la = lab[(size_t)e[i][0]], lb = lab[(size_t)e[i][1]];
    const int lo = std::min(la, lb), hi = std::max(la, lb);
    if (d > best || (d == best && (lo < bi || (lo == bi && hi < bj)))) {
      best = d; bi = lo; bj = hi;
    }
  }
  const int lu = lab[(size_t)u], lv = lab[(size_t)v];
  return bi == std::min(lu, lv) && bj == std::max(lu, lv);
}

// MUTANT : decider l'owner sur les positions Morton au lieu des `PointId`.
bool g_inject_owner_rank = false;
inline bool owner_edge_id(const std::vector<std::array<long long, 3>>& p,
                          const std::vector<int>& lab, int u, int v, int w) {
  if (!g_inject_owner_rank) return owner_edge(p, lab, u, v, w);
  std::vector<int> pos(lab.size());
  for (size_t i = 0; i < lab.size(); ++i) pos[i] = (int)i;
  return owner_edge(p, pos, u, v, w);
}

int owner_fixtures() {
  int fautes = 0;
  auto d2 = [](const std::vector<std::array<long long, 3>>& p, int i, int j) {
    long long s = 0;
    for (int k = 0; k < 3; ++k) { const long long d = p[i][k] - p[j][k]; s += d * d; }
    return s;
  };
  const int e[3][2] = {{0, 1}, {0, 2}, {1, 2}};

  // --- EQUILATERAL ENTIER DE `Z^3`, avec RELABELING.
  //
  // La contre-fixture de l'audit `1aa487d` : memes coordonnees, labels permutes.
  // L'ordre Morton devient `1,2,0`. L'owner SCIENTIFIQUE par `PointId` est
  // l'arete `(0,1)`, alors qu'un code qui deciderait sur les positions
  // choisirait les deux premieres, donc `(1,2)`.
  const std::array<long long, 3> P0 = {101, 100, 101};   // PointId 0
  const std::array<long long, 3> P1 = {100, 100, 100};   // PointId 1
  const std::array<long long, 3> P2 = {101, 101, 100};   // PointId 2
  // Trois relabelings : l'owner par `PointId` doit etre INVARIANT.
  const int perms[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}};
  for (int pi = 0; pi < 3; ++pi) {
    std::vector<std::array<long long, 3>> p(3);
    std::vector<int> lab(3);
    for (int i = 0; i < 3; ++i) {
      const int id = perms[pi][i];                       // position `i` porte ce `PointId`
      p[(size_t)i] = (id == 0) ? P0 : (id == 1) ? P1 : P2;
      lab[(size_t)i] = id;
    }
    if (d2(p, 0, 1) != 2 || d2(p, 0, 2) != 2 || d2(p, 1, 2) != 2) {
      std::fprintf(stderr, "FIXTURE OWNER: le triangle n'est pas equilateral\n");
      ++fautes;
    }
    int admissibles = 0, owners = 0, owner_lo = -1, owner_hi = -1;
    for (int i = 0; i < 3; ++i) {
      const int w = 3 - e[i][0] - e[i][1];
      const long long D = d2(p, e[i][0], e[i][1]);
      if (d2(p, e[i][0], w) <= D && d2(p, e[i][1], w) <= D) ++admissibles;
      if (owner_edge(p, lab, e[i][0], e[i][1], w)) {
        ++owners;
        owner_lo = std::min(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
        owner_hi = std::max(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
      }
    }
    if (admissibles != 3) {
      std::fprintf(stderr, "FIXTURE OWNER: %d incidences admissibles, trois attendues\n",
                   admissibles);
      ++fautes;
    }
    // L'OWNER PAR `PointId` EST `(0,1)`, quelle que soit la permutation.
    if (owners != 1 || owner_lo != 0 || owner_hi != 1) {
      std::fprintf(stderr, "FIXTURE OWNER: relabeling %d rend %d owners, arete (%d,%d),"
                           " un seul et (0,1) attendus\n", pi, owners, owner_lo, owner_hi);
      ++fautes;
    }
  }

  // --- ISOCELE : deux aretes maximales, un seul owner.
  {
    std::vector<std::array<long long, 3>> q = {{0, 0, 0}, {4, 0, 0}, {2, 4, 0}};
    std::vector<int> lab = {0, 1, 2};
    int adm = 0, own = 0;
    for (int i = 0; i < 3; ++i) {
      const int w = 3 - e[i][0] - e[i][1];
      const long long D = d2(q, e[i][0], e[i][1]);
      if (d2(q, e[i][0], w) <= D && d2(q, e[i][1], w) <= D) ++adm;
      if (owner_edge(q, lab, e[i][0], e[i][1], w)) ++own;
    }
    if (adm != 2 || own != 1) {
      std::fprintf(stderr, "FIXTURE OWNER: isocele rend %d admissibles et %d owners,"
                           " deux et un attendus\n", adm, own);
      ++fautes;
    }
  }

  // --- LE MUTANT DOIT MORDRE : decider sur les positions choisit `(1,2)`.
  {
    std::vector<std::array<long long, 3>> p = {P1, P2, P0};   // ordre Morton
    std::vector<int> lab = {1, 2, 0};
    std::vector<int> pos = {0, 1, 2};
    int lo_id = -1, hi_id = -1, lo_pos = -1, hi_pos = -1;
    for (int i = 0; i < 3; ++i) {
      const int w = 3 - e[i][0] - e[i][1];
      if (owner_edge(p, lab, e[i][0], e[i][1], w)) {
        lo_id = std::min(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
        hi_id = std::max(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
      }
      if (owner_edge(p, pos, e[i][0], e[i][1], w)) {
        lo_pos = std::min(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
        hi_pos = std::max(lab[(size_t)e[i][0]], lab[(size_t)e[i][1]]);
      }
    }
    if (lo_id != 0 || hi_id != 1) {
      std::fprintf(stderr, "FIXTURE OWNER: owner par PointId = (%d,%d), (0,1) attendu\n",
                   lo_id, hi_id);
      ++fautes;
    }
    if (lo_pos == lo_id && hi_pos == hi_id) {
      std::fprintf(stderr, "MUTANT SURVIVANT: decider sur les positions Morton rend le"
                           " meme owner que sur les PointId\n");
      return 3;
    }
    std::printf("mutant_owner_positions arete=(%d,%d) au lieu de (%d,%d)\n",
                lo_pos, hi_pos, lo_id, hi_id);
  }

  if (fautes) {
    std::fprintf(stderr, "DESACCORD DU JUGE: %d fautes d'owner\n", fautes);
    return 1;
  }
  std::printf("fixtures_owner accord=OUI relabelings=3 owner_pointid=(0,1)"
              " isocele admissibles=2 owners=1\n");
  return 0;
}

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
    else if (a == "--climb") { g_bank = true; g_vwave = true; g_climb = true; }
    else if (a.rfind("--smax=", 0) == 0) set_smax(arg_ll(val("--smax=").c_str(), 4, 34, "smax"));
    else if (a == "--fallback") g_fallback = true;
    else if (a == "--spindle") g_spindle = true;
    else if (a.rfind("--raffine=", 0) == 0) g_raffine = arg_ll(val("--raffine=").c_str(), 1, 12, "raffine");
    else if (a.rfind("--raffine-lane=", 0) == 0) g_raffine_lane = (int)arg_ll(val("--raffine-lane=").c_str(), 0, 2, "raffine-lane");
    else if (a == "--vwave") { g_bank = true; g_vwave = true; }
    else if (a == "--descent") { g_bank = true; g_descent = true; }
    else if (a.rfind("--window=", 0) == 0) { g_win = arg_ll(val("--window=").c_str(), 2, 1024, "window"); g_bank = true; }
    else if (a.rfind("--bank-l=", 0) == 0) { g_bankl = arg_ll(val("--bank-l=").c_str(), 1, 64, "bank-l"); g_bank = true; }
    else if (a.rfind("--inflation=", 0) == 0) g_inflation = arg_ll(val("--inflation=").c_str(), 1, 20000, "inflation");
    else if (a.rfind("--inflation-lane=", 0) == 0) g_infl_lane = (int)arg_ll(val("--inflation-lane=").c_str(), 0, 2, "inflation-lane");
    else if (a.rfind("--warms=", 0) == 0) g_warms = arg_ll(val("--warms=").c_str(), 1, 200, "warms");
    else if (a == "--fixtures-spindle") return spindle_fixtures();
    else if (a == "--fixtures-rang") return rang_fixture();
    else if (a == "--fixtures-owner") return owner_fixtures();
    else if (a == "--inject=owner-generationrank") { g_inject_owner_rank = true; g_q3carriers = true; }
    else if (a == "--fixtures-terme-t") return terme_t_fixtures();
    else if (a == "--q3-carriers") g_q3carriers = true;
    else if (a.rfind("--q3-sep=", 0) == 0) { g_q3carriers = true; g_q3s = arg_ll(val("--q3-sep=").c_str(), 1, 64, "q3-sep"); }
    else if (a.rfind("--q3-oracle=", 0) == 0) { g_q3carriers = true; g_q3oracle = arg_ll(val("--q3-oracle=").c_str(), 8, 400, "q3-oracle"); }
    else if (a.rfind("--min-blocs=", 0) == 0) g_min_blocs = arg_ll(val("--min-blocs=").c_str(), 1, (1LL << 40), "min-blocs");
    else if (a == "--window-ledger") g_window = true;
    else if (a == "--inject=orientation-pointid") { g_window = true; g_inject_orient = true; }
    else if (a == "--inject=orientation-cote") { g_window = true; g_inject_cote = true; }
    else if (a.rfind("--oracle-window=", 0) == 0) {
      g_window = true;
      g_oracle_window = arg_ll(val("--oracle-window=").c_str(), 8, 8000, "oracle-window");
    }
    else if (a.rfind("--min-ouverts=", 0) == 0) g_min_open = arg_ll(val("--min-ouverts=").c_str(), 1, (1LL << 40), "min-ouverts");
    else if (a.rfind("--min-orientations=", 0) == 0) g_min_orient = arg_ll(val("--min-orientations=").c_str(), 1, (1LL << 40), "min-orientations");
    else if (a.rfind("--min-fermes=", 0) == 0) g_min_closed = arg_ll(val("--min-fermes=").c_str(), 1, (1LL << 40), "min-fermes");
    else if (a.rfind("--max-slope-e4=", 0) == 0) g_max_slope_e4 = std::atof(val("--max-slope-e4=").c_str());
    else if (a == "--soc64-shadow") g_soc64_shadow = true;
    else if (a == "--judge-soc64") { g_soc64_shadow = true; g_judge_soc64 = true; }
    else if (a.rfind("--soc-cap=", 0) == 0) { g_soc64_shadow = true; g_soc_cap = arg_ll(val("--soc-cap=").c_str(), 1, (1LL << 40), "soc-cap"); }
    else if (a.rfind("--porteurs=", 0) == 0) { g_window = true; g_m4_sample = arg_ll(val("--porteurs=").c_str(), 1, (1LL << 22), "porteurs"); }
    else if (a.rfind("--min-masse-porteurs=", 0) == 0) g_min_m4_echantillon = arg_ll(val("--min-masse-porteurs=").c_str(), 1, (1LL << 40), "min-masse-porteurs");
    else if (a.rfind("--apex=", 0) == 0) g_apex_sample = arg_ll(val("--apex=").c_str(), 1, (1LL << 22), "apex");
    else if (a.rfind("--porteurs-seed=", 0) == 0) g_porteurs_seed = arg_ll(val("--porteurs-seed=").c_str(), 0, (1LL << 40), "porteurs-seed");
    else if (a.rfind("--rang=", 0) == 0) g_rang_budget = arg_ll(val("--rang=").c_str(), 1, (1LL << 22), "rang");
    else if (a.rfind("--porteurs-oracle=", 0) == 0) { g_window = true; g_porteurs_oracle = arg_ll(val("--porteurs-oracle=").c_str(), 4, 4000, "porteurs-oracle"); }
    else if (a == "--inject=porteurs-c4-comme-m4") g_inject_c4_comme_m4 = true;
    else if (a.rfind("--soc-judge-cap=", 0) == 0) { g_judge_soc64 = true; g_soc64_shadow = true; g_soc_judge_cap = arg_ll(val("--soc-judge-cap=").c_str(), 1, (1LL << 24), "soc-judge-cap"); }
    else if (a.rfind("--min-soc-taches=", 0) == 0) g_min_soc_taches = arg_ll(val("--min-soc-taches=").c_str(), 1, (1LL << 40), "min-soc-taches");
    else refuse("option inconnue");
  }
  // LE SHADOW EXIGE LA BRANCHE QUI PRODUIT SES TACHES. Sans `--vwave` il ne
  // verrait jamais une boite temoin, et publierait zero tache avec un code
  // zero : un vert par vacuite. Refus AVANT tout calcul.
  if (g_soc64_shadow && !g_vwave) refuse("--soc64-shadow exige --vwave");
  if (g_min_soc_taches > 0 && !g_soc64_shadow) refuse("--min-soc-taches exige --soc64-shadow");
  if (g_min_m4_echantillon > 0 && g_m4_sample == 0)
    refuse("--min-masse-porteurs exige --porteurs");
  if (g_apex_sample > 0 && g_m4_sample == 0) refuse("--apex exige --porteurs");
  if (g_porteurs_oracle > 0 && g_m4_sample == 0) refuse("--porteurs-oracle exige --porteurs");
  // Un mutant sans son juge est un vert par vacuite : il faut l'oracle.
  if (g_inject_c4_comme_m4 && g_porteurs_oracle == 0)
    refuse("--inject=porteurs-c4-comme-m4 exige --porteurs-oracle");
  if (ns.empty()) ns = {4000, 16000};

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho") fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  // CONTRE-FAMILLE, pas un regime du contrat : masse universelle quadratique,
  // source q3/q4 vide. Elle sert a refuter, jamais a qualifier.
  else if (family == "two_lines") fam = mhgp3v::CloudFamily::kTwoLines;
  else refuse("famille inconnue");
  if (oracle && ns.back() > 64) refuse("l'oracle exige n <= 64");

  std::vector<double> fronts, fenetres, e4_sums, e4_maxs;
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

    // Parent de chaque feuille, calcule UNE fois : la remontee ne peut pas se
    // permettre une recherche lineaire par rectangle.
    std::vector<int> leaf_parent(sp.size(), -1);
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (nodes[i].left < 0) leaf_parent[-1 - nodes[i].left] = (int)i;
      if (nodes[i].right < 0) leaf_parent[-1 - nodes[i].right] = (int)i;
    }

    // ---- GRAINES : le cas diagonal DEROULE. Un thread par nœud interne.
    std::vector<Pair> wave;
    wave.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) wave.push_back({nodes[i].left, nodes[i].right, 0});

    // ---- VAGUES : `count -> scan -> fill`, aucune pile.
    std::vector<Pair> terms;
    BankStat bank;
    SocShadowStat soc;
    // PAR TERMINAL, UN SORT PAR LANE — pas un seul bit q2. Le contre-audit le
    // demande explicitement : sans `closed_mask` par lane, q3 et q4 ne sont que
    // des agregats et leur fenetre ne peut pas etre calculee. `pend` marque les
    // terminaux dont la certification a ete TRONQUEE : leur lane reste ouverte
    // par surete, mais la fenetre publiee n'est alors qu'un SURENSEMBLE.
    std::vector<unsigned char> fate;   // bit `lane` : la banque a-t-elle ferme cette lane ?
    std::vector<unsigned char> pend;   // bit `lane` : certification tronquee, sort inconnu
    long long pending_lane[3] = {0, 0, 0};
    // LA FRACTION DE RECORDS N'EST PAS LA FRACTION DE MASSE, et j'ai publie
    // l'une pour l'autre. On compte donc la masse fermee explicitement.
    long long mass_closed_q2 = 0;
    long long raffines = 0;
    long long tests = 0, levels = 0, wave_hwm = (long long)wave.size();
    // ---- LA CERTIFICATION, EXTRAITE. Elle etait inline dans la phase de
    // remplissage, donc un terminal etait decide AVANT d'etre certifie. Le
    // raffinement local exige l'inverse : certifier, puis decider si l'on
    // scinde. C'est le levier qui reste apres la refutation du spindle sur
    // rectangle, et il ne peut pas s'ecrire sans cette extraction.
      auto certifier = [&](int wa, int wb, unsigned char* out_f, unsigned char* out_p,
                           unsigned char* out_soc) {
        const mhgp3v::WspdBox ba = cell_of(nodes, sp, wa);
        const mhgp3v::WspdBox bb = cell_of(nodes, sp, wb);
        mhgp3v::RectBox qa{}, qb{};
        long long m4[3];
        for (int d = 0; d < 3; ++d) {
          qa.lo[d] = ba.lo[d]; qa.hi[d] = ba.hi[d];
          qb.lo[d] = bb.lo[d]; qb.hi[d] = bb.hi[d];
          m4[d] = ba.lo[d] + ba.hi[d] + bb.lo[d] + bb.hi[d];
        }
        const long long dlo = mhgp3v::rect_minsq(qa, qb);   // UNE fois
        long long cred[3] = {0, 0, 0};
        // LEDGER COMBINE : les credits qu'un algorithme disposant AUSSI de
        // SOC64 aurait accumules, sur le meme parcours. Il n'est jamais
        // additionne a `cred` ; il le REMPLACE dans la vue combinee.
        long long ccred[3] = {0, 0, 0};
        // Somme brute des populations SOC64-`ALL`, sans aucun masque : le
        // ledger fautif, conserve comme temoin de l'ecart.
        long long soc_cred_brut = 0;
        long long taken = 0;
        bool tronque = false;
        if (g_vwave) {
          // `Central-VWave`. LA TACHE EST `(CNode, lane_mask)`, jamais un
          // nœud seul avec un masque global : sinon un parent `ALL` en q2
          // mais `MIXED` en q3 pousse ses enfants, dont la population est
          // CREDITEE UNE SECONDE FOIS en q2 — une fausse fermeture. Seuls
          // les bits `MIXED` du parent passent aux enfants ; les bits
          // `ALL`/`NONE` y sont consommes exactement une fois.
          // ---- DEUX LEDGERS, UN SEUL PARCOURS : LE REPLAY VIRTUEL COMBINE.
          //
          // La premiere version de ce shadow additionnait `cred[2]` et un
          // credit SOC64 separe, avec un masque empechant SOC -> SOC. Le
          // contre-audit `AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`
          // section 10 a montre que cela ne suffit pas : SOC64 ferme un nœud,
          // le shadow continue quand meme de descendre, et un DESCENDANT
          // central `ALL` recredite alors une population deja comptee par
          // l'ancetre SOC. Les deux ensembles ne sont pas disjoints, et la
          // somme annonce un gain inexistant. Separer les structures de
          // compteurs ne separe pas les ensembles.
          //
          // La reparation est celle que l'audit prescrit. On maintient DEUX
          // masques de lanes vivantes sur la MEME pile :
          //   `mask`  : vue BASELINE, exactement le parcours historique ;
          //   `cmask` : vue COMBINEE, ou SOC64 est un disjonctif de plus.
          // Chaque vue credite au plus une fois par lane et par branche, et la
          // vue combinee ETEINT sa lane des qu'elle ferme — donc ses
          // descendants ne la recreditent jamais.
          //
          // Le parcours reste celui de la baseline, bit pour bit : aucune
          // visite, aucun `exp`, aucun `bank.reads` ne change. C'est licite
          // parce que `cmask` est toujours inclus dans `mask` — SOC64 ne fait
          // que promouvoir `MIXED` en `ALL`, donc la vue combinee sature plus
          // tot, jamais plus tard.
          struct Task { int node; unsigned mask; unsigned cmask; };
          Task st[96];
          int sn = 0;
          // REPERAGE PUIS REMONTEE. Descendre depuis la RACINE pour chaque
          // rectangle depense 42,7 % du travail en descente pure — des
          // nœuds qui ne creditent rien et n'elaguent rien, et n'existent
          // que pour atteindre la region utile. Or les nœuds crediteurs
          // sont tous autour de `m_0`. On repere donc la feuille de `m_0`
          // par la cle de Morton, puis on REMONTE : a chaque ancetre, le
          // sous-arbre FRERE est un candidat, et on s'arrete des que les
          // seuils sont atteints. Ni les 51,7 % de `NONE` lointains, ni la
          // descente initiale ne sont alors payes.
          if (g_climb) {
            const unsigned long long qk0 =
                mhgp3v::wf_morton48(m4[0] / 4, m4[1] / 4, m4[2] / 4);
            size_t pos0 =
                (size_t)(std::lower_bound(keys.begin(), keys.end(), qk0) - keys.begin());
            if (pos0 >= keys.size()) pos0 = keys.size() - 1;
            // Remonter depuis la feuille `pos0` : trouver le nœud interne
            // dont elle est un enfant, puis empiler les freres successifs.
            int cur = -1 - (int)pos0;
            for (size_t up = 0; up < nodes.size() && sn + 2 <= 96; ++up) {
              const int par = (cur < 0) ? leaf_parent[-1 - cur] : nodes[cur].parent;
              if (par < 0) break;
              const int frere = (nodes[par].left == cur) ? nodes[par].right : nodes[par].left;
              st[sn++] = {frere, 7u, 7u};
              cur = par;
            }
            if (sn == 0) st[sn++] = {0, 7u, 7u};
          } else {
            st[sn++] = {0, 7u, 7u};
          }
          long long exp = 0;
          bool abandonne = false;
          const int* need = g_need;
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
            // Vue combinee : meme regle de saturation, sur SES propres credits.
            // L'intersection avec `m` materialise l'inclusion `cmask` dans
            // `mask` — si elle etait fausse, la vue combinee serait creditee
            // sur une visite que la baseline ne fait pas, et le shadow ne
            // serait plus un shadow.
            unsigned cm = tk.cmask & m;
            for (int lane = 0; lane < 3; ++lane)
              if (ccred[lane] >= need[lane]) cm &= ~(1u << lane);
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
            unsigned cmixed = 0;
            bool eut_all = false, eut_none = false;
            for (int lane = 0; lane < 3; ++lane) {
              if (!(m & (1u << lane))) continue;
              RectVerdict v = mhgp3v::rect_central_verdict(dlo, smn, smx, lane);
              // Le masque central est SUFFISANT, jamais complet. Sous
              // `--fallback`, un `MIXED` central est repris par le
              // classifieur complet — `Hmin` et les deux maxima de
              // distance —, qui n'est pas comparable et peut mordre la ou
              // le central renonce. La disjonction de deux certificats
              // suffisants reste suffisante.
              // `JungSpindleRect-v0` en DISJONCTION. Deux certificats
              // suffisants et non comparables restent suffisants. Le central
              // teste la boule INSCRITE ; le spindle rend le terme
              // directionnel que la reduction jetait.
              if (v == RectVerdict::kMixed && g_spindle) {
                const long long dhi = mhgp3v::rect_maxsq(qa, qb);
                const long long tabs = mhgp3v::rect_t_abs(qa, qb, cb2);
                ++bank.spindle_essais;
                if (tabs > 0) ++bank.spindle_tabs;
                if (mhgp3v::rect_spindle_all(dlo, dhi, smx, tabs, lane)) {
                  v = RectVerdict::kAll;
                  ++bank.spindle_all;
                }
              }
              if (v == RectVerdict::kMixed && g_fallback) {
                long long mxk = 0;
                const RectVerdict w =
                    mhgp3v::rect_classify(qa, qb, cb2, (RectLane)lane, &mxk);
                if (w == RectVerdict::kAll) v = RectVerdict::kAll;
              }
              // ---- LEDGER BASELINE. Inchange, bit pour bit.
              if (v == RectVerdict::kAll) { cred[lane] += pop; eut_all = true; }
              else if (v == RectVerdict::kMixed) mixed |= 1u << lane;
              else eut_none = true;

              // ---- LEDGER COMBINE. `SOC64` est un disjonctif DE PLUS, place
              // APRES le spindle et le fallback : le contre-audit relevait a
              // juste titre qu'un SOC place avant eux pouvait crediter un nœud
              // que le fallback aurait de toute facon ferme, et compter deux
              // fois la meme population.
              //
              // La vue combinee ne credite QUE si sa propre lane est encore
              // vivante, et l'eteint des qu'elle ferme : ses descendants ne la
              // recreditent jamais. C'est ce qui rend `ccred` comparable a
              // `cred` sans jamais les additionner.
              if (!(cm & (1u << lane))) continue;
              RectVerdict w = v;
              if (w == RectVerdict::kMixed && g_soc64_shadow && lane == 2) {
                mhgp3v::soc::Box sa{}, sb{}, sc{};
                for (int d = 0; d < 3; ++d) {
                  sa.lo[d] = qa.lo[d]; sa.hi[d] = qa.hi[d];
                  sb.lo[d] = qb.lo[d]; sb.hi[d] = qb.hi[d];
                  sc.lo[d] = cb2.lo[d]; sc.hi[d] = cb2.hi[d];
                }
                if (g_soc_cap > 0 && soc.taches >= g_soc_cap) {
                  ++soc.cap_refuses;
                } else {
                mhgp3v::soc::SocStats sst{};
                ++soc.taches;
                const int sl = mhgp3v::soc::soc64_all_lane(
                    sa, sb, sc, mhgp3v::soc::SocMutant::kNone, mhgp3v::cone::kLaneQ4, &sst);
                soc.couples += sst.pairs;
                soc.early += sst.early;
                soc.wide += sst.wide;
                if (sl >= mhgp3v::cone::kLaneQ4) {
                  ++soc.all;
                  w = RectVerdict::kAll;
                  soc.masse_creditee += pop;
                  // Le temoin fautif. Il est ici un MINORANT de l'ancienne
                  // ecriture : celle-ci appelait SOC64 sur des nœuds que la
                  // vue combinee a deja eteints, donc elle surcomptait encore
                  // davantage. L'ecart mesure est donc conservateur.
                  soc_cred_brut += pop;
                  // ---- LE JUGE. Il enumere les VRAIS points, pas les coins.
                  //
                  // SOC64 travaille sur des boites ; le juge prend les
                  // `PointId` reellement stockes dans les trois nœuds et evalue
                  // chaque triple dans l'ecriture `(g,Q)` de `spindle_cone.hpp` :
                  // g = D2 - |U|^2 avec U = 2z-a-b, Q = D2|U|^2 - (U.d)^2, et
                  // q4 <=> g > 0 et g^2 > 2Q. Ni H, ni E, ni X, ni aucune
                  // difference de Minkowski n'y apparait : une faute commune ne
                  // peut pas s'y compenser.
                  if (g_judge_soc64) {
                    const int fa = (wa < 0) ? (-1 - wa) : nodes[wa].first;
                    const int la2 = (wa < 0) ? (-1 - wa) : nodes[wa].last;
                    const int fb = (wb < 0) ? (-1 - wb) : nodes[wb].first;
                    const int lb2 = (wb < 0) ? (-1 - wb) : nodes[wb].last;
                    const int fc = (tk.node < 0) ? (-1 - tk.node) : nodes[tk.node].first;
                    const int lc = (tk.node < 0) ? (-1 - tk.node) : nodes[tk.node].last;
                    const long long na = (long long)(la2 - fa + 1);
                    const long long nb = (long long)(lb2 - fb + 1);
                    const long long nc = (long long)(lc - fc + 1);
                    // Le cap est un REFUS DE JUGER, jamais un accord implicite.
                    if (na > 0 && nb > 0 && nc > 0 && na <= g_soc_judge_cap &&
                        na * nb <= g_soc_judge_cap && na * nb * nc <= g_soc_judge_cap) {
                      ++soc.juges;
                      bool refute = false;
                      for (int ia = fa; ia <= la2 && !refute; ++ia)
                        for (int ib = fb; ib <= lb2 && !refute; ++ib)
                          for (int ic = fc; ic <= lc; ++ic) {
                            ++soc.triples;
                            const int lj = mhgp3v::cone::lane_of_target_gq(
                                sp[ia][0], sp[ia][1], sp[ia][2],
                                sp[ic][0], sp[ic][1], sp[ic][2],
                                sp[ib][0], sp[ib][1], sp[ib][2]);
                            if (lj < mhgp3v::cone::kLaneQ4) { refute = true; break; }
                          }
                      if (refute) ++soc.faux;
                    } else {
                      ++soc.juges_sautes;
                    }
                  }
                }
                }
              }
              if (w == RectVerdict::kAll) ccred[lane] += pop;
              else if (w == RectVerdict::kMixed) cmixed |= 1u << lane;
            }
            // OU PASSE LE TRAVAIL ? Un `MIXED` pur est une descente pure :
            // il ne credite rien, n'elague rien, et ne sert qu'a atteindre
            // les nœuds utiles. C'est la part compressible.
            if (eut_all) ++bank.v_all;
            else if (mixed && !eut_none) ++bank.v_descente;
            else ++bank.v_none;
            if (mixed && tk.node >= 0) {
              if (sn + 2 > 96) { abandonne = true; break; }       // jamais en silence
              st[sn++] = {nodes[tk.node].left, mixed, cmixed};
              st[sn++] = {nodes[tk.node].right, mixed, cmixed};
            }
          }
          // Une pile pleine ou un quantum epuise laisse des taches VIVANTES.
          // ATTENTION : ce compteur les DENOMBRE, il ne les SERIALISE pas.
          // Aucune tache, aucun masque, aucun curseur n'est encore ecrit —
          // la continuation reste a faire, et l'audit `dfa9e1b` a raison de
          // refuser le mot. Les credits deja acquis restent valides ; seule
          // la COMPLETUDE est perdue, jamais la surete.
          if (abandonne || (sn > 0 && exp >= g_win)) { ++bank.tronques; tronque = true; }
          taken = exp;
        } else if (g_descent) {
          // Descente au meilleur d'abord vers `m_0`, pile bornee.
          std::pair<long long, int> heap[64];
          int hn = 0;
          bool deborde = false;
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
              if (hn >= 62) { deborde = true; break; }
              const long long dd = (ch < 0) ? 0 : box_dist2_to(nodes[ch], m4, g_tight);
              heap[hn++] = {dd, ch};
            }
          }
          // UN TAS NON VIDE, UN DEBORDEMENT OU UN CAP SONT DES ABANDONS.
          // Le contre-audit releve que ces branches annonçaient
          // `fenetre_finale=OUI` apres avoir abandonne.
          if (hn > 0 || deborde) { ++bank.tronques; tronque = true; }
        } else {
          const unsigned long long qk =
              mhgp3v::wf_morton48(m4[0] / 4, m4[1] / 4, m4[2] / 4);
          size_t pos = (size_t)(std::lower_bound(keys.begin(), keys.end(), qk) - keys.begin());
          const size_t beg = (pos > (size_t)(g_win / 2)) ? pos - g_win / 2 : 0;
          const size_t end = std::min(keys.size(), beg + (size_t)g_win);
          size_t r = beg;
          for (; r < end && taken < g_bankl; ++r) {
            ++bank.reads;
            mhgp3v::RectBox zb{};
            for (int d = 0; d < 3; ++d) { zb.lo[d] = sp[r][d]; zb.hi[d] = sp[r][d]; }
            ++taken; ++bank.recerts;
            const unsigned got = mhgp3v::rect_central_mask_dlo(dlo, qa, qb, zb);
            for (int lane = 0; lane < 3; ++lane)
              if (got & (1u << lane)) ++cred[lane];
          }
          // LA FENETRE MORTON N'EXAMINE JAMAIS TOUT LE NUAGE : c'est une
          // PROPOSITION bornee autour d'une cle, jamais une preuve
          // d'absence. Elle n'est complete que si elle a couvert l'ordre
          // entier sans buter sur son cap — cas qui n'arrive qu'a tres
          // petit `n`. Sinon le sort de chaque lane non fermee est INCONNU.
          if (beg != 0 || end != keys.size() || r < end) { ++bank.tronques; tronque = true; }
        }
        const int* need = g_need;
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
        const long long msz = count_of(nodes, wa) * count_of(nodes, wb);
        // LE SEUL NOMBRE QUI DECIDE : ce rectangle, q4 OUVERT pour le
        // certificat central, serait-il FERME par le certificat correle ? Un
        // gain en credits qui ne franchit jamais `need[2]` ne ferme rien et ne
        // vaut rien.
        //
        // Le verdict est RENDU A L'APPELANT, il n'est pas comptabilise ici :
        // sous `--raffine` cet appel peut porter sur un rectangle qui sera
        // scinde, et seul l'appelant sait s'il est terminal.
        if (out_soc != nullptr) {
          // `flip` compare DEUX LEDGERS, il n'additionne pas deux ensembles.
          // C'est la difference exacte entre l'ancienne version fautive et
          // celle-ci : `ccred[2]` est le nombre de temoins qu'un algorithme
          // disposant de SOC64 aurait credites, chacun compte une seule fois.
          const bool flip = g_soc64_shadow && cred[2] < need[2] && ccred[2] >= need[2];
          if (flip) ++soc.tentatives_fermees;
          // Le meme test, ecrit comme la version refutee : une SOMME de deux
          // ensembles qui ne sont pas disjoints.
          const bool flip_brut =
              g_soc64_shadow && cred[2] < need[2] && cred[2] + soc_cred_brut >= need[2];
          *out_soc = (unsigned char)((flip ? 1u : 0u) | (flip_brut ? 2u : 0u));
        }
        // ---- INVARIANT DU REPLAY, ET SA FORME EXACTE.
        //
        // La forme naive `ccred >= cred` est FAUSSE, et le compteur ci-dessous
        // l'a refutee sur 419 rectangles au premier essai. La raison est la
        // saturation : la vue combinee ferme plus tot, donc elle ARRETE de
        // crediter, pendant que la baseline continue d'empiler des temoins
        // dont elle n'a plus besoin. Avec `need=8`, un SOC64 qui credite 10 au
        // premier nœud s'arrete a 10, tandis que la baseline peut atteindre 12
        // en trois nœuds. Le total combine est alors plus petit, alors que la
        // vue combinee est strictement meilleure.
        //
        // La forme exacte est donc `ccred >= min(cred, need)`. Elle dit ce qui
        // compte : sur chaque sous-arbre ou la vue combinee repond `ALL`, la
        // population du nœud majore la somme des populations `ALL` DISJOINTES
        // que la baseline y aurait creditees — jusqu'a saturation. Sa violation
        // signalerait que les deux ledgers ne partagent plus le meme parcours.
        if (g_soc64_shadow) {
          for (int lane = 0; lane < 3; ++lane) {
            const long long plancher = (cred[lane] < need[lane]) ? cred[lane] : need[lane];
            if (ccred[lane] < plancher) ++soc.invariant_viole;
          }
        }
        unsigned char f = 0, pn = 0;
        for (int lane = 0; lane < 3; ++lane) {
          if (cred[lane] >= need[lane]) { f |= (unsigned char)(1u << lane); continue; }
          // UNE LANE NON FERMEE APRES TRONCATURE N'EST PAS UNE LANE OUVERTE :
          // c'est une lane dont le sort est INCONNU. On la compte ouverte —
          // fail-open, donc sur — mais on le DIT, et la fenetre publiee est
          // alors un surensemble, jamais la fenetre finale.
          if (tronque) { pn |= (unsigned char)(1u << lane); ++pending_lane[lane]; }
        }
        *out_f = f;
        *out_p = pn;
        if (f & 1u) mass_closed_q2 += msz;
      };
    while (!wave.empty()) {
      ++levels;
      std::vector<int> cnt(wave.size());
      std::vector<char> sep(wave.size());
      std::vector<unsigned char> fpre(wave.size(), 0), ppre(wave.size(), 0), done(wave.size(), 0);
      std::vector<unsigned char> spre(wave.size(), 0);   // verdict shadow de la tentative
      for (size_t i = 0; i < wave.size(); ++i) {
        ++tests;
        const mhgp3v::WspdBox ca = cell_of(nodes, sp, wave[i].a);
        const mhgp3v::WspdBox cb = cell_of(nodes, sp, wave[i].b);
        sep[i] = mhgp3v::wspd_separated_euclid(ca, cb, p, q) ? 1 : 0;
        const bool la = wave[i].a < 0, lb = wave[i].b < 0;
        if (sep[i]) {
          if (!g_bank || g_raffine == 0) { cnt[i] = 0; continue; }
          // RAFFINEMENT LOCAL : certifier AVANT de decider. Un terminal dont la
          // lane cible reste ouverte est scinde, tant qu'il reste du budget et
          // qu'il n'est pas deja une paire de feuilles.
          certifier(wave[i].a, wave[i].b, &fpre[i], &ppre[i], &spre[i]);
          done[i] = 1;
          const bool ferme = (fpre[i] & (unsigned char)(1u << g_raffine_lane)) != 0;
          if (ferme || wave[i].r >= g_raffine || (la && lb)) { cnt[i] = 0; continue; }
          cnt[i] = 2; ++raffines;
          continue;
        }
        cnt[i] = (la && lb) ? 0 : 2;    // deux feuilles non separables : terminal force
      }
      std::vector<int> off(wave.size() + 1, 0);
      for (size_t i = 0; i < wave.size(); ++i) off[i + 1] = off[i] + cnt[i];
      std::vector<Pair> next(off.back());
      for (size_t i = 0; i < wave.size(); ++i) {
        if (cnt[i] == 0) {
          terms.push_back(wave[i]);
          if (!g_bank) { fate.push_back(0); pend.push_back(0); }
          if (g_bank) {
            unsigned char f = 0, pn = 0, sc = 0;
            if (done[i]) { f = fpre[i]; pn = ppre[i]; sc = spre[i]; }   // deja certifie
            else certifier(wave[i].a, wave[i].b, &f, &pn, &sc);
            fate.push_back(f);
            pend.push_back(pn);
            // LEDGER TERMINAL DU SHADOW. Ce point du programme est le SEUL ou
            // une paire est definitivement terminale : `cnt[i]==0` et son sort
            // est ecrit dans `fate`/`pend`. Compter ici, et seulement ici,
            // interdit qu'un parent scinde et ses enfants soient additionnes.
            if (sc != 0u) {
              const long long mm = count_of(nodes, wave[i].a) * count_of(nodes, wave[i].b);
              if (sc & 1u) { ++soc.fermetures; soc.masse_fermee += mm; }
              if (sc & 2u) { ++soc.fermetures_brutes; soc.masse_fermee_brute += mm; }
            }
          }
          continue;
        }
        const int ia = wave[i].a, ib = wave[i].b;
        const long long ra = (ia < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ia));
        const long long rb = (ib < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ib));
        int o = off[i];
        const int rr = wave[i].r + (sep[i] ? 1 : 0);   // scission de RAFFINEMENT
        if (ia >= 0 && (ib < 0 || ra >= rb)) {
          next[o++] = {nodes[ia].left, ib, rr};
          next[o++] = {nodes[ia].right, ib, rr};
        } else {
          next[o++] = {ia, nodes[ib].left, rr};
          next[o++] = {ia, nodes[ib].right, rr};
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
    // ---- DEGRE RESIDUEL PAR POINT — nomme correctement cette fois.
    //
    // CORRECTION (contre-audit `736f5bc`). Ce compteur ne contient AUCUN credit
    // projectif : l'appeler `ProjectiveWindowCounter` etait un abus, et son
    // identite exacte est `somme_a deg(a) = 2 x masse residuelle`, parce qu'il
    // additionne les DEUX degres de chaque paire non ordonnee. Le facteur deux
    // est desormais IMPRIME plutot que cache.
    //
    // La quantite qui se brancherait sur `anchor_source` est `E_q(a)`, les
    // seconds endpoints `b > a` sous orientation canonique, dont la somme vaut
    // la masse residuelle SANS facteur deux. Je ne la calcule pas ici : son
    // parcours coute `O(|A| |B|)` par rectangle, donc la masse elle-meme. Sa
    // SOMME est en revanche gratuite — c'est exactement `masse_residuelle` —,
    // et seul son maximum demanderait le parcours.
    //
    // CE QUE CE COMPTEUR N'EST PAS. Ce n'est pas le `kept` du moteur. `kept`
    // est un ensemble de SITES `z` dependant de `(a,b)`, publie en MAXIMUM par
    // paire ; le degre residuel est un nombre d'ENDPOINTS par point, publie en
    // moyenne. Les rapprocher etait une coincidence de scalaires — leur rejeu
    // donne une moyenne de 82,5 la ou je citais 446 — et je ne le fais plus.
    std::vector<long long> deg_res(sp.size(), 0);
    std::vector<long long> diff_sym(sp.size() + 1, 0);
    long long masse_res = 0;
    for (size_t i = 0; i < terms.size(); ++i) {
      if (i < fate.size() && (fate[i] & 1u)) continue;
      const Pair& t = terms[i];
      const int fa = (t.a < 0) ? (-1 - t.a) : nodes[t.a].first;
      const int la = (t.a < 0) ? (-1 - t.a) : nodes[t.a].last;
      const int fb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].first;
      const int lb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].last;
      const long long ka = la - fa + 1, kb = lb - fb + 1;
      masse_res += ka * kb;
      // CE COMPTEUR AUSSI EST UN RANGE-ADD, et il ne l'etait pas. Le
      // contre-audit releve que le degre symetrique parcourait encore ses deux
      // plages, donc que le wall du probe n'etait pas `O(F+n)` malgre le
      // nouveau ledger. Les deux boucles deviennent quatre ecritures : le
      // degre symetrique est la SOMME des deux orientations, donc exactement
      // deux range-adds au lieu d'un.
      diff_sym[(size_t)fa] += kb; diff_sym[(size_t)(la + 1)] -= kb;
      diff_sym[(size_t)fb] += ka; diff_sym[(size_t)(lb + 1)] -= ka;
    }
    long long nsum = 0, nmax = 0;
    {
      long long run = 0;
      for (size_t r = 0; r < sp.size(); ++r) {
        run += diff_sym[r];
        deg_res[r] = run;
        nsum += run;
        nmax = std::max(nmax, run);
      }
    }

    // ---- `EdgeWindowRangeAdd-v0` — LA FENETRE D'ARETES, EXACTE, EN `O(F+n)`.
    //
    // Ce que j'ai ecrit et qui etait faux : « le maximum de `E_q(a)` exige de
    // developper `|A||B|` par terminal, donc la masse ». Le contre-audit donne
    // la raison exacte du contraire. Chaque nœud du radix tree porte une plage
    // CONTIGUE de `GenerationRank`. Les graines sont des plages sœurs
    // disjointes, les splits les remplacent par des sous-plages, donc tout
    // terminal `A x B` satisfait exactement l'un des deux ordres TOTAUX
    // `last(A) < first(B)` ou `last(B) < first(A)`.
    //
    // Sous l'orientation canonique « second endpoint `b > a` », un terminal
    // credite alors la plage INFERIEURE toute entiere d'une valeur CONSTANTE,
    // le cardinal de la plage superieure. Deux ecritures dans un tableau de
    // differences signe, un scan prefixe, et tous les `E_q(a)` sont exacts.
    //
    // Les deux nombres que cela rend sont ceux qui decident l'architecture :
    // `sum_a E_4(a)` est le nombre d'aretes candidates que `LocalShallowBall`
    // devrait traiter, et `max_a E_4(a)` le pire fan-out par ancre. Aucun
    // `PairId` n'est developpe pour les obtenir.
    long long win_sum[3] = {0, 0, 0}, win_max[3] = {0, 0, 0};
    long long open_terms[3] = {0, 0, 0}, mass_open[3] = {0, 0, 0};
    long long mass_closed[3] = {0, 0, 0}, mass_pending[3] = {0, 0, 0};
    long long mass_strict_open[3] = {0, 0, 0}, closed_terms[3] = {0, 0, 0};
    long long win_p50[3] = {0, 0, 0}, win_p95[3] = {0, 0, 0}, win_p99[3] = {0, 0, 0};
    long long orient_ab = 0, orient_ba = 0;
    long long oracle_pairs = 0, oracle_desaccords = 0;
    // Portes MORDUES par le mutant, comptees separement : domaine des degres,
    // identite de somme, oracle exhaustif. Trois juges independants, et le recu
    // dit lequel a vu quoi.
    long long mut_domaine = 0, mut_somme = 0;
    if (g_window) {
      const long long total_pairs = m * (m - 1) / 2;
      // ---- LEDGER MASSIQUE EXCLUSIF : `entree = ferme + ouvert + pendant`.
      //
      // Le contre-audit le demande explicitement : `mass_open` incluait les
      // pendants, `pend` n'etait pas consomme, et trois etats se recouvraient.
      // Un terminal dont la certification a ete tronquee n'est ni ferme ni
      // ouvert : son sort est INCONNU. Il est traite comme ouvert par surete —
      // la fenetre reste un surensemble — mais il est COMPTE a part, et
      // l'identite des trois masses est gatee.
      for (int lane = 0; lane < 3; ++lane) {
        for (size_t i = 0; i < terms.size(); ++i) {
          const Pair& t = terms[i];
          const long long ka = count_of(nodes, t.a), kb = count_of(nodes, t.b);
          const long long msz = ka * kb;
          const bool ferme = (i < fate.size()) && (fate[i] & (1u << lane));
          const bool pendant = (i < pend.size()) && (pend[i] & (1u << lane));
          if (ferme) { mass_closed[lane] += msz; ++closed_terms[lane]; }
          else if (pendant) { mass_pending[lane] += msz; }
          else { mass_strict_open[lane] += msz; }
        }
        if (mass_closed[lane] + mass_pending[lane] + mass_strict_open[lane] != total_pairs) {
          std::fprintf(stderr, "INVARIANT VIOLE: ledger q%d non exclusif :"
                               " ferme %lld + pendant %lld + ouvert %lld != C(n,2)=%lld\n",
                       lane + 2, mass_closed[lane], mass_pending[lane],
                       mass_strict_open[lane], total_pairs);
          return 3;
        }
      }
      // EQUIVARIANCE PAR ECHANGE DES COTES. Le producteur emet structurellement
      // la plage basse en premier : mesure faite, `A<B` vaut `17 444` et `B<A`
      // vaut ZERO. La seconde branche de l'ordre total serait donc du code mort,
      // et un ledger vert ne dirait rien d'elle. On calcule donc CHAQUE fenetre
      // DEUX fois, la seconde sur les terminaux dont les deux cotes sont
      // echanges : la fenetre ne depend pas de l'ordre de stockage, donc les
      // deux vecteurs doivent etre IDENTIQUES, et la branche `B<A` est exercee
      // a chaque mesure au lieu de rester vacante.
      auto range_add = [&](int lane, bool swap, std::vector<long long>* deg_out) -> int {
        std::vector<long long> diff((size_t)m + 1, 0);
        for (size_t i = 0; i < terms.size(); ++i) {
          if (i < fate.size() && (fate[i] & (1u << lane))) continue;
          const int ta = swap ? terms[i].b : terms[i].a;
          const int tb = swap ? terms[i].a : terms[i].b;
          const long long fa = (ta < 0) ? (-1 - ta) : nodes[ta].first;
          const long long la = (ta < 0) ? (-1 - ta) : nodes[ta].last;
          const long long fb = (tb < 0) ? (-1 - tb) : nodes[tb].first;
          const long long lb = (tb < 0) ? (-1 - tb) : nodes[tb].last;
          // PLAGES VALIDES. Une plage vide ou inversee rendrait le range-add
          // silencieusement faux ; on la refuse au lieu de la subir.
          if (fa > la || fb > lb || fa < 0 || fb < 0 || la >= m || lb >= m) {
            std::fprintf(stderr, "INVARIANT VIOLE: plage de terminal invalide"
                                 " [%lld,%lld]x[%lld,%lld] n=%lld\n", fa, la, fb, lb, m);
            return 3;
          }
          const long long ka = la - fa + 1, kb = lb - fb + 1;
          if (!swap) { ++open_terms[lane]; mass_open[lane] += ka * kb; }
          // MUTANT `orientation-pointid`. Le contre-audit autorise un SCATTER
          // par `spid[rank]` APRES le scan, si le consommateur indexe par
          // `PointId`. Il n'autorise pas le range-add sur des intervalles de
          // `PointId` : ceux-la ne sont pas contigus, et la plage ecrite n'a
          // alors ni la bonne longueur ni meme forcement une longueur positive.
          const long long ia = g_inject_orient ? spid[fa] : fa;
          const long long ja = g_inject_orient ? spid[la] : la;
          const long long ib = g_inject_orient ? spid[fb] : fb;
          const long long jb = g_inject_orient ? spid[lb] : lb;
          // MUTANT `orientation-cote`. Crediter TOUJOURS le cote stocke en
          // premier, sans tester lequel des deux est reellement inferieur.
          // L'identite de somme y survit intacte — les deux plages ont la meme
          // masse —, et l'oracle ne le voit que si l'orientation canonique est
          // deja fixee. Seule l'equivariance par echange des cotes le mord.
          if (g_inject_cote || la < fb) {
            diff[(size_t)ia] += kb; diff[(size_t)(ja + 1)] -= kb;
            if (lane == 2) ++orient_ab;
          } else if (lb < fa) {
            diff[(size_t)ib] += ka; diff[(size_t)(jb + 1)] -= ka;
            if (lane == 2) ++orient_ba;
          } else {
            // PLAGES NON DISJOINTES : l'ordre total suppose par le range-add
            // n'existe pas. C'est une refutation de la structure, pas un cas
            // a arrondir.
            std::fprintf(stderr, "INVARIANT VIOLE: plages de terminal non totalement"
                                 " ordonnees [%lld,%lld] et [%lld,%lld]\n", fa, la, fb, lb);
            return 3;
          }
        }
        deg_out->assign((size_t)m, 0);
        long long run = 0;
        for (long long r = 0; r < m; ++r) { run += diff[(size_t)r]; (*deg_out)[(size_t)r] = run; }
        return 0;
      };
      for (int lane = 0; lane < 3; ++lane) {
        std::vector<long long> deg, deg_swap;
        if (int rc = range_add(lane, false, &deg)) return rc;
        if (int rc = range_add(lane, true, &deg_swap)) return rc;
        if (deg != deg_swap) {
          std::fprintf(stderr, "INVARIANT VIOLE: la fenetre q%d depend de l'ordre de"
                               " stockage des cotes du terminal\n", lane + 2);
          return 3;
        }
        for (long long r = 0; r < m; ++r) {
          const long long d = deg[(size_t)r];
          if (d < 0 || d > m - 1) {
            // SOUS INJECTION, ON NE SORT PAS ICI. Un mutant tue par la premiere
            // porte ne dit pas si les autres l'auraient vu ; on le laisse donc
            // traverser tout le juge et on compte CHAQUE porte qui le mord.
            if (!g_inject_orient) {
              std::fprintf(stderr, "INVARIANT VIOLE: E_%d(rang %lld)=%lld hors [0,%lld]\n",
                           lane + 2, r, d, m - 1);
              return 3;
            }
            ++mut_domaine;
          }
          win_sum[lane] += d;
          win_max[lane] = std::max(win_max[lane], d);
        }
        // L'IDENTITE QUI JUGE LE LEDGER : la somme des degres ORIENTES vaut
        // exactement la masse ouverte, SANS facteur deux. Si elle est fausse,
        // le range-add ne represente pas la relation.
        if (win_sum[lane] != mass_open[lane]) {
          if (!g_inject_orient) {
            std::fprintf(stderr, "INVARIANT VIOLE: sum E_%d=%lld != masse ouverte %lld\n",
                         lane + 2, win_sum[lane], mass_open[lane]);
            return 3;
          }
          ++mut_somme;
        }
        if (mass_open[lane] > total_pairs) {
          std::fprintf(stderr, "INVARIANT VIOLE: masse ouverte %lld > C(n,2)=%lld\n",
                       mass_open[lane], total_pairs);
          return 3;
        }
        std::vector<long long> srt = deg;
        std::sort(srt.begin(), srt.end());
        auto qtl = [&](double f) { return srt[std::min(srt.size() - 1, (size_t)(f * (double)srt.size()))]; };
        win_p50[lane] = qtl(0.50); win_p95[lane] = qtl(0.95); win_p99[lane] = qtl(0.99);

        // ORACLE : developper CHAQUE `PairId` du residuel EXACTEMENT UNE FOIS,
        // orienter par `GenerationRank` et comparer TOUT le vecteur de degres.
        // C'est le seul juge qui tue le mutant d'orientation : l'identite de
        // somme, elle, peut survivre a une plage de mauvaise longueur.
        if (g_oracle_window > 0 && m <= g_oracle_window) {
          std::vector<long long> oracle_deg((size_t)m, 0);
          std::vector<unsigned char> vu((size_t)(m * (m - 1) / 2), 0);
          for (size_t i = 0; i < terms.size(); ++i) {
            if (i < fate.size() && (fate[i] & (1u << lane))) continue;
            const Pair& t = terms[i];
            const long long fa = (t.a < 0) ? (-1 - t.a) : nodes[t.a].first;
            const long long la = (t.a < 0) ? (-1 - t.a) : nodes[t.a].last;
            const long long fb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].first;
            const long long lb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].last;
            for (long long u = fa; u <= la; ++u)
              for (long long v = fb; v <= lb; ++v) {
                if (u == v) {
                  std::fprintf(stderr, "INVARIANT VIOLE: paire diagonale au rang %lld\n", u);
                  return 3;
                }
                const long long lo = std::min(u, v), hi = std::max(u, v);
                const long long idx = lo * (2 * m - lo - 1) / 2 + (hi - lo - 1);
                if (vu[(size_t)idx]) {
                  std::fprintf(stderr, "INVARIANT VIOLE: PairId (%lld,%lld) developpe deux fois\n",
                               lo, hi);
                  return 3;
                }
                vu[(size_t)idx] = 1;
                ++oracle_deg[(size_t)lo];       // orientation canonique : `b > a`
                if (lane == 2) ++oracle_pairs;
              }
          }
          for (long long r = 0; r < m; ++r)
            if (oracle_deg[(size_t)r] != deg[(size_t)r]) ++oracle_desaccords;
        }
      }
      // COHERENCE AVEC LE COMPTEUR SYMETRIQUE DEJA PUBLIE. Le degre residuel q2
      // additionne les DEUX endpoints de chaque paire ; la fenetre orientee n'en
      // additionne qu'un. Leur rapport doit valoir exactement deux. Deux
      // compteurs ecrits separement qui se recoupent valent mieux qu'un seul.
      if (win_sum[0] != masse_res || nsum != 2 * masse_res) {
        if (!g_inject_orient) {
          std::fprintf(stderr, "INVARIANT VIOLE: fenetre q2 %lld, masse residuelle %lld,"
                               " degre symetrique %lld\n", win_sum[0], masse_res, nsum);
          return 3;
        }
        ++mut_somme;
      }
    }

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
    // ---- LE CŒUR CENTRAL, MESURE PAR PAIRE — sans aucun jeu de rectangle.
    //
    // Il faut separer deux causes que j'avais confondues. Ou bien la paire
    // possede bien ses temoins universels et c'est la FACTORISATION qui les
    // perd — le cœur commun a `A x B` est plus petit que celui de la paire —,
    // ou bien la paire n'en a AUCUN et alors aucun certificat central, factorise
    // ou non, ne pourra jamais la fermer.
    //
    // Le cœur universel q4 d'une paire est la boule de rayon
    // `sqrt(2-sqrt(3)) D / 2 = 0,2588 D` autour du milieu ; les deux extremites,
    // elles, sont a `D/2`. Un cœur q4 vide est donc le signe qu'il n'y a rien
    // entre `a` et `b` — et c'est une propriete de la PAIRE, pas du certificat.
    long long ech = 0, faux_resid = 0, som_temoins = 0, max_temoins = 0;
    long long som_coeur4 = 0, coeur4_vide = 0, coeur4_suffisant = 0;
    // ---- LE CŒUR UNIVERSEL EXACT, contre la boule INSCRITE qu'on teste.
    //
    // Le test `209 V^2 <= 56 D^2` vient d'une reduction qui SUPPRIME le terme
    // `-4 (d.v)^2` — le commentaire de `rect_front.hpp` le dit explicitement.
    // Or ce terme est favorable, et il est maximal SUR L'AXE de l'arete.
    //
    // Le cœur universel exact se derive directement. Avec `u = z-m`, le site est
    // interieur a TOUTE sphere admissible ssi, pour tout `t` du disque de Jung
    // de rayon `D/(2 sqrt 2)` orthogonal a `d`, `||u-t||^2 < D^2/4 + ||t||^2`,
    // c'est-a-dire `||u||^2 + (D/sqrt 2) ||u_perp|| < D^2/4`. En elevant au
    // carre et avec `4H = D^2 - V^2` :
    //
    //     (D2 - V2)^2 > 2 (V2 D2 - (d.v)^2)   et   D2 > V2.
    //
    // A `d.v = 0` cela redonne EXACTEMENT `V2/D2 < 2 - sqrt(3)` : le test
    // implemente est le pire cas directionnel, donc la boule inscrite. Sur
    // l'axe, au contraire, `(d.v)^2 = V2 D2` et la condition redevient `H > 0` :
    // le vrai cœur atteint la boule diametrale entiere. C'est precisement la
    // region ou vivent les temoins d'une paire inter-amas.
    long long som_exact4 = 0, exact4_vide = 0, exact4_suffisant = 0;
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
        if (i >= fate.size() || !(fate[i] & (1u << g_infl_lane))) {
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
        long long d2 = 0;
        for (int d = 0; d < 3; ++d) {
          const long long u = sp[bi][d] - sp[ai][d];
          d2 += u * u;
        }
        long long cnt = 0, cnt4 = 0, cntE = 0;
        for (size_t z = 0; z < sp.size(); ++z) {
          if ((int)z == ai || (int)z == bi) continue;
          long long h = 0, v2 = 0;
          for (int d = 0; d < 3; ++d) {
            h += (sp[z][d] - sp[ai][d]) * (sp[bi][d] - sp[z][d]);
            const long long w = 2 * sp[z][d] - sp[ai][d] - sp[bi][d];
            v2 += w * w;
          }
          if (h > 0) ++cnt;                                   // cœur q2 : `V2 < D2`
          if ((__int128)209 * v2 <= (__int128)56 * d2) ++cnt4;  // boule INSCRITE
          if (v2 < d2) {                                        // cœur EXACT
            long long dv = 0;
            for (int d = 0; d < 3; ++d)
              dv += (sp[bi][d] - sp[ai][d]) * (2 * sp[z][d] - sp[ai][d] - sp[bi][d]);
            const __int128 g = (__int128)(d2 - v2) * (d2 - v2);
            const __int128 rhs = 2 * ((__int128)v2 * d2 - (__int128)dv * dv);
            if (g > rhs) ++cntE;
          }
        }
        ++ech; som_temoins += cnt; max_temoins = std::max(max_temoins, cnt);
        som_coeur4 += cnt4;
        if (cnt4 == 0) ++coeur4_vide;
        if (cnt4 >= g_need[2]) ++coeur4_suffisant;
        som_exact4 += cntE;
        if (cntE == 0) ++exact4_vide;
        if (cntE >= g_need[2]) ++exact4_suffisant;
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
                " juges=%lld faux=%lld | verdicts ALL=%lld NONE=%lld descente_pure=%lld spindle_ALL=%lld essais=%lld tabs_non_nul=%lld"
                " raffinements=%lld | seuils=%d/%d/%d degre_residuel somme=%lld (= 2 x masse_res %lld) max=%lld moyen=%.1f"
                " | partenaires max=%lld moyen=%.2f"
                " | residuel : %lld paires tirees DANS LA MASSE ouverte,"
                " temoins_moyen=%.1f max=%lld, deja >=10 temoins : %lld (%.1f%%)"
                " donc supports q2 estimes %.1f%% du residuel"
                " | coeur q4 par paire (lane echantillonnee %d) : moyen=%.2f,"
                " vide : %lld (%.1f%%), au moins %d temoins : %lld (%.1f%%)"
                " | coeur EXACT : moyen=%.2f, vide : %lld (%.1f%%),"
                " au moins %d temoins : %lld (%.1f%%)\n",
                m, family.c_str(), g_tight ? "serree" : "cellule", p, q, terms.size(), (double)terms.size() / (double)m,
                levels, tests, (double)tests / (double)terms.size(), wave_hwm, mass, total,
                pct(t_tree, 0.5), pct(t_tree, 0.95), t_wave.back(),
                bank.reads, bank.recerts, bank.closed[0], bank.closed[1], bank.closed[2],
                100.0 * (double)mass_closed_q2 / (double)total,
                100.0 * (double)bank.closed[0] / (double)std::max<size_t>(1, terms.size()),
                bank.tronques, bank.juges, bank.faux, bank.v_all, bank.v_none, bank.v_descente, bank.spindle_all, bank.spindle_essais, bank.spindle_tabs, raffines,
                g_need[0], g_need[1], g_need[2], nsum, masse_res, nmax, (double)nsum / (double)m,
                dmax, (double)dsum / (double)std::max(1LL, dnz),
                ech, (double)som_temoins / (double)std::max(1LL, ech), max_temoins,
                faux_resid, 100.0 * (double)faux_resid / (double)std::max(1LL, ech),
                100.0 * (double)(ech - faux_resid) / (double)std::max(1LL, ech),
                g_infl_lane, (double)som_coeur4 / (double)std::max(1LL, ech),
                coeur4_vide, 100.0 * (double)coeur4_vide / (double)std::max(1LL, ech),
                g_need[2], coeur4_suffisant,
                100.0 * (double)coeur4_suffisant / (double)std::max(1LL, ech),
                (double)som_exact4 / (double)std::max(1LL, ech),
                exact4_vide, 100.0 * (double)exact4_vide / (double)std::max(1LL, ech),
                g_need[2], exact4_suffisant,
                100.0 * (double)exact4_suffisant / (double)std::max(1LL, ech));
    // LIGNE AUTONOME DU SHADOW. Elle n'entre dans aucune chaine de format
    // existante : les portes en place lisent des lignes entieres.
    if (g_soc64_shadow) {
      std::printf("soc64_shadow q4 : tentatives taches=%lld all=%lld (%.3f%%) couples=%lld"
                  " couples/tache=%.2f early=%lld larges=%lld cap_refuses=%lld"
                  " ledger=%s masse_creditee=%lld fermetures=%lld"
                  " | terminaux fermetures=%lld masse_fermee=%lld seuil=%d\n",
                  soc.taches, soc.all,
                  100.0 * (double)soc.all / (double)std::max(1LL, soc.taches), soc.couples,
                  (double)soc.couples / (double)std::max(1LL, soc.taches), soc.early,
                  soc.wide, soc.cap_refuses,
                  soc.cap_refuses == 0 ? "FINAL" : "MINORANT_CAP",
                  soc.masse_creditee, soc.tentatives_fermees, soc.fermetures, soc.masse_fermee,
                  g_need[2]);
      // LE TEMOIN DE LA FAUTE, PUBLIE A COTE DU RESULTAT. Il n'est jamais le
      // resultat : il mesure de combien l'ecriture refutee se serait trompee.
      std::printf("soc64_somme_brute q4 : fermetures=%lld masse_fermee=%lld"
                  " | surcompte fermetures=%lld masse=%lld\n",
                  soc.fermetures_brutes, soc.masse_fermee_brute,
                  soc.fermetures_brutes - soc.fermetures,
                  soc.masse_fermee_brute - soc.masse_fermee);
      if (soc.invariant_viole != 0) {
        std::fprintf(stderr,
                     "INVARIANT VIOLE: %lld rectangles ou le ledger combine credite MOINS que "
                     "la baseline\n",
                     soc.invariant_viole);
        return 3;
      }
      if (g_judge_soc64) {
        std::printf("soc64_juge accord=%s verdicts_juges=%lld sautes=%lld triples=%lld faux=%lld\n",
                    soc.faux == 0 ? "OUI" : "NON", soc.juges, soc.juges_sautes, soc.triples,
                    soc.faux);
        // UN SEUL TRIPLE REFUTANT SUFFIT. Le verdict `ALL` est universel : il
        // n'a pas de taux d'erreur admissible.
        if (soc.faux != 0) {
          std::fprintf(stderr,
                       "DESACCORD: %lld verdicts SOC64 `ALL` refutes par enumeration des points\n",
                       soc.faux);
          return 1;
        }
        // Un juge qui n'a rien juge n'est pas un accord.
        if (soc.juges == 0) {
          std::fprintf(stderr, "PLANCHER: le juge SOC64 n'a enumere aucun verdict\n");
          return 3;
        }
      }
      // PLANCHER DE COUVERTURE. Une campagne qui ne soumet aucune tache au
      // certificat correle ne mesure rien ; sans ce plancher elle serait verte.
      if (g_min_soc_taches > 0 && soc.taches < g_min_soc_taches) {
        std::fprintf(stderr, "PLANCHER: %lld taches SOC64 soumises, %lld exigees\n", soc.taches,
                     g_min_soc_taches);
        return 3;
      }
    }
    if (g_window) {
      for (int lane = 0; lane < 3; ++lane)
        std::printf("fenetre q%d : terminaux_ouverts=%lld masse_ouverte=%lld (%.3f%% de C(n,2))"
                    " sum_E=%lld max_E=%lld p50=%lld p95=%lld p99=%lld moyen=%.2f"
                    " | masses : fermee=%lld pendante=%lld ouverte=%lld"
                    " terminaux fermes=%lld | pending=%lld%s\n",
                    lane + 2, open_terms[lane], mass_open[lane],
                    100.0 * (double)mass_open[lane] / (double)(m * (m - 1) / 2),
                    win_sum[lane], win_max[lane], win_p50[lane], win_p95[lane], win_p99[lane],
                    (double)win_sum[lane] / (double)m,
                    mass_closed[lane], mass_pending[lane], mass_strict_open[lane],
                    closed_terms[lane], pending_lane[lane],
                    pending_lane[lane] ? " SURENSEMBLE" : "");
      std::printf("orientations q4 : A<B=%lld B<A=%lld | oracle paires=%lld desaccords=%lld\n",
                  orient_ab, orient_ba, oracle_pairs, oracle_desaccords);

      // ---------------------------------------------------------------------
      // `CarrierApexEstimator-v2` : ESTIMATEUR EMBOITE, SANS CENSURE.
      //
      // Les versions precedentes de ce compteur ont ete refutees deux fois, et
      // les deux fois pour la meme raison de fond : un nombre publie sans sa
      // loi n'est pas une mesure.
      //
      //   v0 imprimait `E4 * moyenne(|A_e|)` sous le nom `M4`. C'etait `C4`,
      //      sans tie-break d'owner, donc meme pas `C4` exactement : un
      //      triangle a egalite y etait credite sous plusieurs aretes.
      //   v1 reparait l'owner et ajoutait l'apex, mais RETIRAIT de la moyenne
      //      toute arete dont `binom(|L_e|,2)` depassait le cap. Cette censure
      //      porte precisement sur la variable qui rend le cout lourd : elle
      //      biaise vers le BAS, et d'autant plus que la vraie valeur est
      //      grande. `AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`
      //      section 2.2 a raison de la refuser.
      //
      // La v2 suit le schema emboite de la section 3.1 de ce meme audit.
      //
      // ETAGE EXTERIEUR. `N` est la masse d'aretes q4 OUVERTES ET FINALES —
      // les terminaux `PENDING` sont exclus et leur masse est publiee a part,
      // car un terminal tronque n'a pas de sort connu. On tire `K` rangs
      // uniformes independants dans `[0,N)` par un generateur counter-based
      // seede, jamais par des milieux de quantiles : la quadrature
      // deterministe peut s'aligner sur une periodicite de Morton et n'a
      // aucune barre d'erreur. La bijection rang -> paire passe par la masse
      // cumulee des terminaux. Les estimateurs sont Hansen--Hurwitz.
      //
      // ETAGE INTERIEUR. Pour eviter `binom(l_e,2)` sur une grosse lentille,
      // on tire `r_e` rangs uniformes de PAIRES dans la lentille, decodes par
      // unranking triangulaire EXACT — recherche binaire sur les prefixes
      // entiers, aucun flottant —, puis on evalue l'indicatrice exacte de
      // `Q_e`. L'estimateur local est `binom(l_e,2) * moyenne`. AUCUNE arete
      // n'est jamais retiree de la moyenne : un cap reduit `r_e`, il ne censure
      // pas la population.
      //
      // CE QUI EST PUBLIE. Graine, `K`, digest des paires tirees, doublons,
      // variance empirique, demi-largeur a deux sigma. Les noms sont
      // `C4_carrier_quadrature` et `M4_apex_quadrature` : ce sont des
      // ESTIMATIONS et le nom le dit. Le nom nu `M4` est interdit.
      if (g_m4_sample > 0) {
        // `EdgeKey` sur les vrais `PointId`, jamais sur le rang de tri.
        auto edge_key_moins = [&](long long r1, long long r2, long long s1,
                                  long long s2) -> bool {
          long long a1 = spid[(size_t)r1], a2 = spid[(size_t)r2];
          if (a1 > a2) std::swap(a1, a2);
          long long b1 = spid[(size_t)s1], b2 = spid[(size_t)s2];
          if (b1 > b2) std::swap(b1, b2);
          return (a1 != b1) ? (a1 < b1) : (a2 < b2);
        };
        auto d2 = [&](long long r1, long long r2) -> long long {
          const long long dx = sp[(size_t)r1][0] - sp[(size_t)r2][0];
          const long long dy = sp[(size_t)r1][1] - sp[(size_t)r2][1];
          const long long dz = sp[(size_t)r1][2] - sp[(size_t)r2][2];
          return dx * dx + dy * dy + dz * dz;
        };
        // `e` reste-t-il owner face a l'arete `(r1,r2)` de longueur carree `l` ?
        // Strictement plus longue : non. Egale : le tie-break `EdgeKey` decide.
        auto owner_tient = [&](long long dd, long long ra, long long rb, long long l,
                               long long r1, long long r2) -> bool {
          if (l > dd) return false;
          if (l < dd) return true;
          return edge_key_moins(ra, rb, r1, r2);
        };
        auto dans_ae = [&](long long dd, long long ra, long long rb,
                           long long rx) -> bool {
          const long long ee = d2(rx, ra), xx = d2(rx, rb);
          if (ee > dd || xx > dd) return false;
          if (!owner_tient(dd, ra, rb, ee, ra, rx)) return false;
          if (!owner_tient(dd, ra, rb, xx, rb, rx)) return false;
          return ee + xx > dd;   // `ab` faiblement maximale : aigu <=> E + X > D
        };
        // Indicatrice exacte de `Q_e` sur un couple ordonne `(rx, ry)`.
        auto dans_qe = [&](long long dd, long long ra, long long rb, long long rx,
                           long long ry, bool* positif) -> bool {
          if (!owner_tient(dd, ra, rb, d2(rx, ry), rx, ry)) return false;
          if (!owner_tient(dd, ra, rb, d2(rx, ra), ra, rx)) return false;
          if (!owner_tient(dd, ra, rb, d2(rx, rb), rb, rx)) return false;
          if (!owner_tient(dd, ra, rb, d2(ry, ra), ra, ry)) return false;
          if (!owner_tient(dd, ra, rb, d2(ry, rb), rb, ry)) return false;
          if (!dans_ae(dd, ra, rb, rx) && !dans_ae(dd, ra, rb, ry)) return false;
          const long long ux = sp[(size_t)rx][0] - sp[(size_t)ra][0];
          const long long uy = sp[(size_t)rx][1] - sp[(size_t)ra][1];
          const long long uz = sp[(size_t)rx][2] - sp[(size_t)ra][2];
          const long long vx = sp[(size_t)ry][0] - sp[(size_t)ra][0];
          const long long vy = sp[(size_t)ry][1] - sp[(size_t)ra][1];
          const long long vz = sp[(size_t)ry][2] - sp[(size_t)ra][2];
          const long long wx = sp[(size_t)rb][0] - sp[(size_t)ra][0];
          const long long wy = sp[(size_t)rb][1] - sp[(size_t)ra][1];
          const long long wz = sp[(size_t)rb][2] - sp[(size_t)ra][2];
          if (wx * (uy * vz - uz * vy) - wy * (ux * vz - uz * vx) +
              wz * (ux * vy - uy * vx) == 0) return false;
          // Bien centre : les quatre barycentriques du circumcentre strictement
          // positives, decidees sans division. `G_ij = ui.uj`, `r_i = |ui|^2`,
          // `beta_i = det(G_i)/(2 det G)` : il suffit de `det G > 0`,
          // `det G_i > 0` et `somme det G_i < 2 det G`. Sous u16, un
          // determinant 3x3 d'entrees 34 bits tient sur 104 bits.
          *positif = false;
          const long long g11 = wx * wx + wy * wy + wz * wz;
          const long long g22 = ux * ux + uy * uy + uz * uz;
          const long long g33 = vx * vx + vy * vy + vz * vz;
          const long long g12 = wx * ux + wy * uy + wz * uz;
          const long long g13 = wx * vx + wy * vy + wz * vz;
          const long long g23 = ux * vx + uy * vy + uz * vz;
          auto det3 = [](__int128 a11, __int128 a12, __int128 a13, __int128 a21,
                         __int128 a22, __int128 a23, __int128 a31, __int128 a32,
                         __int128 a33) -> __int128 {
            return a11 * (a22 * a33 - a23 * a32) - a12 * (a21 * a33 - a23 * a31) +
                   a13 * (a21 * a32 - a22 * a31);
          };
          const __int128 dg = det3(g11, g12, g13, g12, g22, g23, g13, g23, g33);
          if (dg > 0) {
            const __int128 e1 = det3(g11, g12, g13, g22, g22, g23, g33, g23, g33);
            const __int128 e2 = det3(g11, g11, g13, g12, g22, g23, g13, g33, g33);
            const __int128 e3 = det3(g11, g12, g11, g12, g22, g22, g13, g23, g33);
            if (e1 > 0 && e2 > 0 && e3 > 0 && e1 + e2 + e3 < 2 * dg) *positif = true;
          }
          return true;
        };
        // Generateur counter-based : la graine est SCELLEE et le j-ieme tirage
        // ne depend que de `(graine, j)`. Aucun etat, donc aucune dependance a
        // l'ordre d'evaluation.
        auto splitmix = [](unsigned long long x) -> unsigned long long {
          x += 0x9E3779B97F4A7C15ULL;
          x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
          x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
          return x ^ (x >> 31);
        };
        // Unranking triangulaire EXACT : rang `r` dans `[0, C(l,2))` -> couple
        // `i < j`. Recherche binaire sur `prefixe(i) = i*(2l-i-1)/2`, en
        // entiers, sans jamais former une racine carree.
        auto unrank = [](long long l, long long r, long long* oi, long long* oj) {
          long long lo = 0, hi = l - 2;
          while (lo < hi) {
            const long long mid = (lo + hi + 1) / 2;
            const long long pref = mid * (2 * l - mid - 1) / 2;
            if (pref <= r) lo = mid; else hi = mid - 1;
          }
          *oi = lo;
          *oj = r - lo * (2 * l - lo - 1) / 2 + lo + 1;
        };

        for (int lane = 1; lane <= 2; ++lane) {
          // POPULATION : ouverte ET FINALE. Un terminal `PENDING` a un sort
          // inconnu ; l'inclure ferait viser un surensemble sans le dire.
          std::vector<long long> cum;
          std::vector<size_t> idx;
          cum.reserve(terms.size() + 1);
          long long acc = 0, masse_pending = 0;
          cum.push_back(0);
          for (size_t i = 0; i < terms.size(); ++i) {
            if (i < fate.size() && (fate[i] & (1u << lane))) continue;
            const int ta = terms[i].a, tb = terms[i].b;
            const long long fa = (ta < 0) ? (-1 - ta) : nodes[ta].first;
            const long long la = (ta < 0) ? (-1 - ta) : nodes[ta].last;
            const long long fb = (tb < 0) ? (-1 - tb) : nodes[tb].first;
            const long long lb = (tb < 0) ? (-1 - tb) : nodes[tb].last;
            const long long msz = (la - fa + 1) * (lb - fb + 1);
            if (i < pend.size() && (pend[i] & (1u << lane))) { masse_pending += msz; continue; }
            acc += msz;
            idx.push_back(i);
            cum.push_back(acc);
          }
          if (acc <= 0) {
            std::printf("porteurs q%d : population_finale=0 masse_pending=%lld\n", lane + 2,
                        masse_pending);
            continue;
          }
          const long long k = g_m4_sample;
          long long somme_a = 0;
          double somme_a2 = 0.0;
          double somme_q = 0.0, somme_q2 = 0.0, somme_p = 0.0;
          long long maxi = 0, sans = 0, doublons = 0, tires = 0;
          long long lens_max = 0, paires_internes = 0;
          long long rang_faits = 0, rang_retenus = 0, rang_max = 0;
          double somme_rang = 0.0;
          unsigned long long digest = 1469598103934665603ULL;   // FNV-1a 64
          unsigned long long precedent = ~0ULL;
          std::vector<long long> le;
          for (long long j = 0; j < k; ++j) {
            // Rang uniforme dans `[0, acc)`. Le produit est forme en 128 bits :
            // `acc` atteint `C(n,2)` et un `long long` deborderait au-dela du
            // domaine 50k. Le contre-audit avait raison de le relever.
            const unsigned long long h =
                splitmix((unsigned long long)g_porteurs_seed * 0x100000001B3ULL +
                         (unsigned long long)j);
            const long long rang = (long long)(((unsigned __int128)h * (unsigned __int128)acc) >> 64);
            const size_t pos = (size_t)(std::upper_bound(cum.begin(), cum.end(), rang) -
                                        cum.begin() - 1);
            if (pos >= idx.size()) continue;
            const size_t ti = idx[pos];
            const int ta = terms[ti].a, tb = terms[ti].b;
            const long long fa = (ta < 0) ? (-1 - ta) : nodes[ta].first;
            const long long fb = (tb < 0) ? (-1 - tb) : nodes[tb].first;
            const long long lb = (tb < 0) ? (-1 - tb) : nodes[tb].last;
            const long long kb = lb - fb + 1;
            const long long dans = rang - cum[pos];
            const long long ra = fa + dans / kb;
            const long long rb = fb + dans % kb;
            ++tires;
            {
              unsigned long long p1 = (unsigned long long)spid[(size_t)ra];
              unsigned long long p2 = (unsigned long long)spid[(size_t)rb];
              if (p1 > p2) std::swap(p1, p2);
              const unsigned long long cle = p1 * 1000003ULL + p2;
              if (cle == precedent) ++doublons;
              precedent = cle;
              digest ^= p1; digest *= 1099511628211ULL;
              digest ^= p2; digest *= 1099511628211ULL;
            }
            const long long dd = d2(ra, rb);
            long long porteurs = 0;
            le.clear();
            for (long long r = 0; r < m; ++r) {
              if (r == ra || r == rb) continue;
              if (d2(r, ra) > dd || d2(r, rb) > dd) continue;   // hors de `L_e`
              le.push_back(r);
              if (dans_ae(dd, ra, rb, r)) ++porteurs;
            }
            somme_a += porteurs;
            somme_a2 += (double)porteurs * (double)porteurs;
            if (porteurs > maxi) maxi = porteurs;
            if (porteurs == 0) ++sans;

            // ---- ETAGE INTERIEUR. Jamais de censure : un cap reduit `r_e`.
            const long long l = (long long)le.size();
            if (l > lens_max) lens_max = l;
            const long long total_paires = (l < 2) ? 0 : l * (l - 1) / 2;
            double qhat = 0.0, phat = 0.0;
            if (total_paires > 0) {
              const long long re = std::min<long long>(g_apex_sample, total_paires);
              long long touches = 0, touches_pos = 0;
              for (long long t = 0; t < re; ++t) {
                const unsigned long long h2 =
                    splitmix((unsigned long long)g_porteurs_seed * 0x9E3779B9ULL +
                             (unsigned long long)j * 0x100000001B3ULL +
                             (unsigned long long)t + 0xABCDEF01ULL);
                const long long rp =
                    (long long)(((unsigned __int128)h2 * (unsigned __int128)total_paires) >> 64);
                long long i1 = 0, i2 = 0;
                unrank(l, rp, &i1, &i2);
                if (i1 < 0 || i2 >= l) continue;
                bool positif = false;
                if (dans_qe(dd, ra, rb, le[(size_t)i1], le[(size_t)i2], &positif)) {
                  ++touches;
                  if (positif) ++touches_pos;
                }
                ++paires_internes;
              }
              if (re > 0) {
                qhat = (double)total_paires * (double)touches / (double)re;
                phat = (double)total_paires * (double)touches_pos / (double)re;
              }
            }
            somme_q += qhat;
            somme_q2 += qhat * qhat;
            somme_p += phat;

            // ---- LE RANG DU CANDIDAT, ET L'ECART CANDIDATS -> SORTIE.
            //
            // `M4_apex` et `W4_positive` comptent des CANDIDATS. Un support q4
            // n'est retenu que si sa propre sphere circonscrite contient au
            // plus `smax - 4` points interieurs. C'est ce filtre, et lui seul,
            // qui separe une masse quartique d'une sortie utile — et il ne
            // s'applique qu'apres avoir connu la sphere.
            //
            // Le predicat est le determinant in-sphere 4x4, translate en `a` :
            // lignes `(p-a, |p-a|^2)` pour `p = b, x, y, z`, son signe corrige
            // par l'orientation `det3(b-a,x-a,y-a)`. Sous u16 il tient sur
            // environ 87 bits — trois coordonnees a 65535 et une norme carree a
            // 1,3e10 donnent 8,8e25 apres les 24 permutations — donc `i128`.
            if (g_rang_budget > 0 && rang_faits < g_rang_budget && total_paires > 0) {
              const long long re2 = std::min<long long>(8, total_paires);
              for (long long t = 0; t < re2 && rang_faits < g_rang_budget; ++t) {
                const unsigned long long h3 =
                    splitmix((unsigned long long)g_porteurs_seed * 0x2545F491ULL +
                             (unsigned long long)j * 0x9E3779B9ULL +
                             (unsigned long long)t + 0x5DEECE66DULL);
                const long long rp =
                    (long long)(((unsigned __int128)h3 * (unsigned __int128)total_paires) >> 64);
                long long i1 = 0, i2 = 0;
                unrank(l, rp, &i1, &i2);
                if (i1 < 0 || i2 >= l) continue;
                bool positif = false;
                if (!dans_qe(dd, ra, rb, le[(size_t)i1], le[(size_t)i2], &positif)) continue;
                if (!positif) continue;                 // seuls les bien centres
                const long long rx = le[(size_t)i1], ry = le[(size_t)i2];
                const __int128 ux = sp[(size_t)rx][0] - sp[(size_t)ra][0];
                const __int128 uy = sp[(size_t)rx][1] - sp[(size_t)ra][1];
                const __int128 uz = sp[(size_t)rx][2] - sp[(size_t)ra][2];
                const __int128 vx = sp[(size_t)ry][0] - sp[(size_t)ra][0];
                const __int128 vy = sp[(size_t)ry][1] - sp[(size_t)ra][1];
                const __int128 vz = sp[(size_t)ry][2] - sp[(size_t)ra][2];
                const __int128 wx = sp[(size_t)rb][0] - sp[(size_t)ra][0];
                const __int128 wy = sp[(size_t)rb][1] - sp[(size_t)ra][1];
                const __int128 wz = sp[(size_t)rb][2] - sp[(size_t)ra][2];
                const __int128 orient = wx * (uy * vz - uz * vy) - wy * (ux * vz - uz * vx) +
                                        wz * (ux * vy - uy * vx);
                const __int128 nw = wx * wx + wy * wy + wz * wz;
                const __int128 nu = ux * ux + uy * uy + uz * uz;
                const __int128 nv = vx * vx + vy * vy + vz * vz;
                const __int128 c0 = uy * (vz * nw - nv * wz) - uz * (vy * nw - nv * wy) +
                                    nu * (vy * wz - vz * wy);
                const __int128 c1 = ux * (vz * nw - nv * wz) - uz * (vx * nw - nv * wx) +
                                    nu * (vx * wz - vz * wx);
                const __int128 c2 = ux * (vy * nw - nv * wy) - uy * (vx * nw - nv * wx) +
                                    nu * (vx * wy - vy * wx);
                const __int128 c3 = ux * (vy * wz - vz * wy) - uy * (vx * wz - vz * wx) +
                                    uz * (vx * wy - vy * wx);
                long long interieurs = 0;
                for (long long z = 0; z < m; ++z) {
                  if (z == ra || z == rb || z == rx || z == ry) continue;
                  const __int128 zx = sp[(size_t)z][0] - sp[(size_t)ra][0];
                  const __int128 zy = sp[(size_t)z][1] - sp[(size_t)ra][1];
                  const __int128 zz = sp[(size_t)z][2] - sp[(size_t)ra][2];
                  const __int128 nz = zx * zx + zy * zy + zz * zz;
                  const __int128 det4 = -zx * c0 + zy * c1 - zz * c2 + nz * c3;
                  if ((orient > 0) ? (det4 < 0) : (det4 > 0)) ++interieurs;
                }
                ++rang_faits;
                somme_rang += (double)interieurs;
                if (interieurs <= (long long)g_need[2] - 1) ++rang_retenus;
                if (interieurs > rang_max) rang_max = interieurs;
              }
            }
          }
          if (tires == 0) {
            std::fprintf(stderr, "REFUS : aucun tirage valide sur %lld demandes\n", k);
            return 2;
          }
          const double moy_a = (double)somme_a / (double)tires;
          const double var_a = std::max(0.0, somme_a2 / (double)tires - moy_a * moy_a);
          const double moy_q = somme_q / (double)tires;
          const double var_q = std::max(0.0, somme_q2 / (double)tires - moy_q * moy_q);
          const double moy_p = somme_p / (double)tires;
          const double demi_a = 2.0 * std::sqrt(var_a / (double)tires) * (double)acc;
          const double demi_q = 2.0 * std::sqrt(var_q / (double)tires) * (double)acc;
          std::printf("porteurs q%d : population_finale=%lld masse_pending=%lld tires=%lld"
                      " graine=%lld doublons=%lld digest=%016llx | A_e moyen=%.3f max=%lld"
                      " sans=%lld (%.3f%%) | C4_carrier_quadrature=%.10g +/-%.4g\n",
                      lane + 2, acc, masse_pending, tires, g_porteurs_seed, doublons,
                      (unsigned long long)digest, moy_a, maxi, sans,
                      100.0 * (double)sans / (double)tires, moy_a * (double)acc, demi_a);
          if (g_rang_budget > 0) {
            std::printf("rang q%d : bien_centres_juges=%lld interieurs_moyen=%.1f max=%lld"
                        " | retenus_a_rang_max_%d : %lld (%.6f%%)\n",
                        lane + 2, rang_faits,
                        (rang_faits == 0) ? 0.0 : somme_rang / (double)rang_faits, rang_max,
                        g_need[2] - 1, rang_retenus,
                        100.0 * (double)rang_retenus / (double)std::max(1LL, rang_faits));
          }
          if (g_apex_sample > 0) {
            std::printf("apex q%d : L_e max=%lld paires_internes=%lld r_e=%lld"
                        " | Q_e moyen=%.3f M4_apex_quadrature=%.10g +/-%.4g"
                        " | W4_positive_quadrature=%.10g ratio=%.6f\n",
                        lane + 2, lens_max, paires_internes, g_apex_sample, moy_q,
                        moy_q * (double)acc, demi_q, moy_p * (double)acc,
                        (moy_q > 0.0) ? moy_p / moy_q : 0.0);
          }
        }

        // -------------------------------------------------------------------
        // CONTROLE D'INDEX EXHAUSTIF. Ce n'est PAS un oracle independant : il
        // reutilise les memes lambdas geometriques, les memes terminaux et les
        // memes fates, et le contre-audit a raison de le dire. Il verifie une
        // seule chose, mais il la verifie exactement : que le decodage
        // `rang -> (terminal, ra, rb)` de l'echantillonneur enumere la meme
        // population que le parcours direct des plages. C'est le defaut le plus
        // silencieux possible de ce compteur, et ce controle le ferme.
        //
        // Il COMPARE les totaux dans le processus et rend 1 en cas d'ecart ; il
        // ne se contente pas d'imprimer deux lignes qu'un regex confronterait.
        if (g_porteurs_oracle > 0) {
          if (m > g_porteurs_oracle) {
            // UN JUGE SAUTE NE VAUT JAMAIS ACCORD. Le contre-audit a reproduit
            // la vacuite : `points=100 --porteurs-oracle=4` sortait zero sans
            // ligne oracle ni mutant.
            std::fprintf(stderr,
                         "REFUS : --porteurs-oracle=%lld < n=%lld, le controle serait saute\n",
                         g_porteurs_oracle, m);
            return 2;
          }
          for (int lane = 1; lane <= 2; ++lane) {
            long long c4 = 0, masse = 0, m4o = 0;
            for (size_t i = 0; i < terms.size(); ++i) {
              if (i < fate.size() && (fate[i] & (1u << lane))) continue;
              if (i < pend.size() && (pend[i] & (1u << lane))) continue;
              const int ta = terms[i].a, tb = terms[i].b;
              const long long fa = (ta < 0) ? (-1 - ta) : nodes[ta].first;
              const long long la = (ta < 0) ? (-1 - ta) : nodes[ta].last;
              const long long fb = (tb < 0) ? (-1 - tb) : nodes[tb].first;
              const long long lb = (tb < 0) ? (-1 - tb) : nodes[tb].last;
              for (long long ra = fa; ra <= la; ++ra)
                for (long long rb = fb; rb <= lb; ++rb) {
                  ++masse;
                  const long long dd = d2(ra, rb);
                  std::vector<long long> lo;
                  for (long long r = 0; r < m; ++r) {
                    if (r == ra || r == rb) continue;
                    if (d2(r, ra) <= dd && d2(r, rb) <= dd) lo.push_back(r);
                    if (dans_ae(dd, ra, rb, r)) ++c4;
                  }
                  for (size_t i1 = 0; i1 < lo.size(); ++i1)
                    for (size_t i2 = i1 + 1; i2 < lo.size(); ++i2) {
                      bool pos = false;
                      if (dans_qe(dd, ra, rb, lo[i1], lo[i2], &pos)) ++m4o;
                    }
                }
            }
            std::printf("porteurs_controle q%d : population_finale=%lld C4_carrier=%lld"
                        " M4_apex=%lld\n",
                        lane + 2, masse, c4, g_inject_c4_comme_m4 ? c4 : m4o);
            if (g_inject_c4_comme_m4 && m4o != c4) {
              std::printf("mutant_killed=1 porteurs-c4-comme-m4 : C4_carrier=%lld mais"
                          " M4_apex=%lld\n", c4, m4o);
              return 4;
            }
          }
        }

        if (g_min_m4_echantillon > 0) {
          if (mass_open[2] < g_min_m4_echantillon) {
            std::fprintf(stderr, "PLANCHER: masse q4 ouverte %lld, %lld exigee pour"
                                 " l'echantillon de porteurs\n",
                         mass_open[2], g_min_m4_echantillon);
            return 3;
          }
        }
      }

      // PLANCHERS DE COUVERTURE. Sans eux, un ledger vert ne dit rien : zero
      // terminal ouvert ou une seule orientation exercee rendraient le
      // range-add trivialement correct et le mutant invisible.
      if (g_min_open > 0 && open_terms[2] < g_min_open) {
        std::fprintf(stderr, "PLANCHER: %lld terminaux ouverts en q4, %lld exiges\n",
                     open_terms[2], g_min_open);
        return 3;
      }
      // PLANCHER DE FERMETURE. Sans banque, tous les terminaux sont ouverts et
      // l'oracle ne compare le range-add qu'a la relation triviale `C(n,2)`. Un
      // nominal utile exige des sorts FERMES ET OUVERTS non vides en q4.
      if (g_min_closed > 0 && closed_terms[2] < g_min_closed) {
        std::fprintf(stderr, "PLANCHER: %lld terminaux fermes en q4, %lld exiges\n",
                     closed_terms[2], g_min_closed);
        return 3;
      }
      if (g_min_orient > 0 && (orient_ab < g_min_orient || orient_ba < g_min_orient)) {
        std::fprintf(stderr, "PLANCHER: orientations A<B=%lld B<A=%lld, %lld exigees de chaque\n",
                     orient_ab, orient_ba, g_min_orient);
        return 3;
      }
      if (g_oracle_window > 0 && m <= g_oracle_window) {
        if (g_inject_orient) {
          // LE MUTANT DOIT MOURIR SOUS L'ORACLE, pas seulement sous une porte
          // arithmetique. Un signe negatif est un accident heureux ; le juge
          // exhaustif, lui, est la raison pour laquelle on peut croire le
          // ledger a `n = 50 000` ou aucun oracle ne tourne.
          if (oracle_desaccords == 0) {
            std::fprintf(stderr, "MUTANT SURVIVANT: le range-add par PointId rend le meme"
                                 " vecteur de degres que l'oracle (domaine=%lld somme=%lld)\n",
                         mut_domaine, mut_somme);
            return 3;
          }
          std::printf("mutant_killed=1 raison=orientation_pointid desaccords=%lld"
                      " portes_mordues : domaine=%lld somme=%lld oracle=1\n",
                      oracle_desaccords, mut_domaine, mut_somme);
          return 4;
        }
        if (oracle_desaccords != 0) {
          std::fprintf(stderr, "DESACCORD DU JUGE: %lld rangs ou la fenetre range-add"
                               " differe du developpement exact\n", oracle_desaccords);
          return 1;
        }
        std::printf("oracle_fenetre accord=OUI paires=%lld\n", oracle_pairs);
      } else if (g_inject_orient) {
        refuse("le mutant d'orientation exige --oracle-window couvrant la taille");
      }
      // LA FENETRE FINALE N'EXISTE QUE SI RIEN N'EST PENDANT. Une certification
      // tronquee laisse un sort inconnu ; ce qui est publie est alors un
      // surensemble fail-open, et le dire est la seule facon de ne pas le
      // confondre plus tard avec la fenetre.
      if (pending_lane[0] || pending_lane[1] || pending_lane[2])
        std::printf("fenetre_finale=NON raison=continuations_pendantes\n");
      else
        std::printf("fenetre_finale=OUI\n");
      e4_sums.push_back((double)win_sum[2]);
      e4_maxs.push_back((double)std::max(1LL, win_max[2]));
    }
    // ---- `Q3AcuteCarrierWave-v0` : la vague de niveau DEUX.
    if (g_q3carriers) {
      long long blocs = 0, prune_obtus = 0, prune_nonmax = 0;
      __int128 masse_bloc = 0;
      long long splits = 0, visites = 0, hwm = 0, feuilles = 0;
      // Couverture par porteur, pour l'oracle : `couv[x]` compte les blocs
      // emis qui contiennent `x` pour le terminal courant.
      std::vector<unsigned char> couv;
      std::vector<long long> ownk;
      long long owners_faux = 0;
      long long triples_aigus = 0, manques = 0, doublons = 0;
      const bool fait_oracle = (g_q3oracle > 0 && m <= g_q3oracle);
      if (fait_oracle) {
        couv.assign((size_t)m * (size_t)m * (size_t)m, 0);
        ownk.assign((size_t)m * (size_t)m * (size_t)m, -1);
      }
      for (size_t i = 0; i < terms.size(); ++i) {
        if (i < fate.size() && (fate[i] & 2u)) continue;      // lane q3 deja fermee
        const mhgp3v::WspdBox ba = cell_of(nodes, sp, terms[i].a);
        const mhgp3v::WspdBox bb = cell_of(nodes, sp, terms[i].b);
        mhgp3v::RectBox qa{}, qb{};
        for (int d = 0; d < 3; ++d) {
          qa.lo[d] = ba.lo[d]; qa.hi[d] = ba.hi[d];
          qb.lo[d] = bb.lo[d]; qb.hi[d] = bb.hi[d];
        }
        const long long dmin = mhgp3v::rect_minsq(qa, qb);
        const long long dmax = mhgp3v::rect_maxsq(qa, qb);
        const long long ka = count_of(nodes, terms[i].a), kb = count_of(nodes, terms[i].b);
        std::vector<int> emis;
        int st[256];
        int sn = 0;
        st[sn++] = 0;                                          // racine
        while (sn > 0) {
          const int cid = st[--sn];
          ++visites;
          const mhgp3v::WspdBox bc = cell_of(nodes, sp, cid);
          mhgp3v::RectBox qc{};
          for (int d = 0; d < 3; ++d) { qc.lo[d] = bc.lo[d]; qc.hi[d] = bc.hi[d]; }
          // PRUNE 1 — `ab` ne peut pas etre l'arete maximale.
          if (mhgp3v::rect_minsq(qa, qc) > dmax || mhgp3v::rect_minsq(qb, qc) > dmax) {
            ++prune_nonmax;
            continue;
          }
          long long smn = 0, smx = 0;
          mhgp3v::rect_s_interval(qa, qb, qc, &smn, &smx);
          // PRUNE 2 — TOUT le bloc est STRICTEMENT obtus, donc aucun q3 positif
          // et aucun angle droit. Strict par construction : `<`, pas `<=`.
          if (smx < dmin) { ++prune_obtus; continue; }
          // ARRET — la cellule porteuse est petite devant la LONGUEUR d'arete.
          const long long w2 = mhgp3v::wspd_w2(bc);
          const bool feuille = (cid < 0);
          if (feuille || (__int128)g_q3s * g_q3s * w2 <= (__int128)dmin) {
            ++blocs;
            if (feuille) ++feuilles;
            const long long kc = count_of(nodes, cid);
            // UN WRAP NE PEUT JAMAIS RENDRE UNE GATE VERTE (audit `92d0c0f` §5).
            // Le produit `ka*kb*kc` deborde un `i64` bien avant les tailles
            // visees ; on accumule en `i128` et on sature explicitement.
            masse_bloc += (__int128)ka * kb * kc;
            if (masse_bloc > (__int128)1 << 100) {
              std::fprintf(stderr, "INVARIANT VIOLE: masse de blocs saturee\n");
              return 3;
            }
            if (fait_oracle) emis.push_back(cid);
            continue;
          }
          if (sn + 2 > 256) {
            std::fprintf(stderr, "INVARIANT VIOLE: pile de porteurs saturee\n");
            return 3;
          }
          ++splits;
          st[sn++] = nodes[cid].left;
          st[sn++] = nodes[cid].right;
          if (sn > hwm) hwm = sn;
        }
        if (!fait_oracle) continue;
        // ORACLE GLOBAL, EXACT-ONCE SUR LE SUPPORT — pas seulement sur le
        // porteur d'une incidence. L'audit `92d0c0f` §2 releve que je remettais
        // la couverture a zero par terminal : je recevais l'unicite d'un porteur
        // dans la partition d'une arete, jamais l'unicite d'un `SupportKey` a
        // travers les terminaux. Un isocele a deux aretes maximales, un
        // equilateral trois ; seul l'owner global tranche.
        for (int cid : emis) {
          const int cf = (cid < 0) ? (-1 - cid) : nodes[cid].first;
          const int cl = (cid < 0) ? (-1 - cid) : nodes[cid].last;
          const int fa2 = (terms[i].a < 0) ? (-1 - terms[i].a) : nodes[terms[i].a].first;
          const int la2 = (terms[i].a < 0) ? (-1 - terms[i].a) : nodes[terms[i].a].last;
          const int fb2 = (terms[i].b < 0) ? (-1 - terms[i].b) : nodes[terms[i].b].first;
          const int lb2 = (terms[i].b < 0) ? (-1 - terms[i].b) : nodes[terms[i].b].last;
          for (int u = fa2; u <= la2; ++u)
            for (int v = fb2; v <= lb2; ++v)
              for (int x = cf; x <= cl; ++x) {
                if (x == u || x == v) continue;
                if (!owner_edge_id(sp, spid, u, v, x)) continue;   // OWNER sur `PointId`
                long long D = 0, V = 0;
                for (int d = 0; d < 3; ++d) {
                  const long long w = sp[v][d] - sp[u][d];
                  const long long z = 2 * sp[x][d] - sp[u][d] - sp[v][d];
                  D += w * w; V += z * z;
                }
                if (V <= D) continue;                        // pas strictement aigu
                int t[3] = {spid[u], spid[v], spid[x]};   // `PointId`, pas positions
                std::sort(t, t + 3);
                const long long key = ((long long)t[0] * m + t[1]) * m + t[2];
                if (couv[(size_t)key] < 255) ++couv[(size_t)key];
                // ET L'IDENTITE DE L'ARETE QUI POSSEDE, pas seulement le compte.
                // Un oracle qui ne verifie que la multiplicite ne voit pas un
                // tie-break faux : les trois aretes d'un equilateral sont toutes
                // emises, donc le compte vaut un quelle que soit celle qu'on
                // declare owner. Il faut comparer l'ARETE.
                ownk[(size_t)key] = ((long long)std::min(spid[u], spid[v]) << 32)
                                  | (long long)std::max(spid[u], spid[v]);
              }
        }
      }
      if (fait_oracle) {
        // Verite : tout triple AIGU, vu depuis son OWNER, doit avoir ete emis
        // EXACTEMENT UNE FOIS sur l'ensemble des terminaux.
        for (int u = 0; u < (int)m; ++u)
          for (int v = u + 1; v < (int)m; ++v)
            for (int x = v + 1; x < (int)m; ++x) {
              // arete owner du triple, puis acuite relativement a elle
              int oa = u, ob = v, ox = x;
              // LA VERITE N'EST JAMAIS MUTEE. Le sujet emploie `owner_edge_id`,
              // qui peut etre injecte ; la verite emploie toujours l'owner par
              // `PointId`. Sans cette separation, sujet et juge s'accordent sur
              // le meme mauvais tie-break — c'est le defaut que l'audit
              // `1aa487d` a pris en flagrant delit.
              if (owner_edge(sp, spid, u, x, v)) { oa = u; ob = x; ox = v; }
              else if (owner_edge(sp, spid, v, x, u)) { oa = v; ob = x; ox = u; }
              long long D = 0, V = 0;
              for (int d = 0; d < 3; ++d) {
                const long long w = sp[ob][d] - sp[oa][d];
                const long long z = 2 * sp[ox][d] - sp[oa][d] - sp[ob][d];
                D += w * w; V += z * z;
              }
              if (V <= D) continue;                          // pas un q3 positif
              ++triples_aigus;
              int tk[3] = {spid[u], spid[v], spid[x]};
              std::sort(tk, tk + 3);
              const long long key = ((long long)tk[0] * m + tk[1]) * m + tk[2];
              const int c = couv[(size_t)key];
              if (c == 0) { ++manques; continue; }
              if (c > 1) ++doublons;
              const long long attendu = ((long long)std::min(spid[oa], spid[ob]) << 32)
                                      | (long long)std::max(spid[oa], spid[ob]);
              if (ownk[(size_t)key] != attendu) ++owners_faux;
            }
      }
      std::printf("q3_porteurs s=%lld : blocs=%lld masse=%.6g feuilles=%lld"
                  " | prunes obtus=%lld non_maximale=%lld | splits=%lld visites=%lld"
                  " pile_max=%lld | blocs/pt=%.3f\n",
                  g_q3s, blocs, (double)masse_bloc, feuilles, prune_obtus, prune_nonmax,
                  splits, visites, hwm, (double)blocs / (double)m);
      if (fait_oracle) {
        std::printf("q3_oracle triples_aigus=%lld manques=%lld doublons=%lld"
                    " owners_faux=%lld\n", triples_aigus, manques, doublons, owners_faux);
        if (triples_aigus < 100) {
          std::fprintf(stderr, "PLANCHER: %lld triples aigus canoniques seulement\n",
                       triples_aigus);
          return 3;
        }
        if (g_inject_owner_rank) {
          if (owners_faux == 0) {
            std::fprintf(stderr, "MUTANT SURVIVANT: decider l'owner sur les positions"
                                 " Morton rend les memes aretes que sur les PointId\n");
            return 3;
          }
          std::printf("mutant_killed=1 raison=owner_generationrank owners_faux=%lld\n",
                      owners_faux);
          return 4;
        }
        if (manques || doublons || owners_faux) {
          std::fprintf(stderr, "DESACCORD DU JUGE: %lld triples hors bloc, %lld en double,"
                               " %lld owners faux\n", manques, doublons, owners_faux);
          return 1;
        }
        std::printf("q3_oracle accord=OUI\n");
      }
      if (g_min_blocs > 0 && blocs < g_min_blocs) {
        std::fprintf(stderr, "PLANCHER: %lld blocs porteurs, %lld exiges\n", blocs, g_min_blocs);
        return 3;
      }
    }
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
    fenetres.push_back((double)nsum);
  }
  if (oracle) { std::printf("oracle accord=OUI\n"); return 0; }
  // LA GATE JUGE LES DEUX COMPTEURS, PAS UN SEUL.
  //
  // Le contre-audit `5dc65c7` releve que cette gate imprimait `OK` sur
  // `eight_clusters` alors que les trois pentes du degre residuel etaient
  // rouges : elle ne refusait que sur `front_records`. Publier une pente sans
  // la juger, c'est publier une decoration.
  // TOUTES LES MESURES SONT PUBLIEES AVANT TOUT VERDICT.
  //
  // Le contre-audit a rejoue mes deux portes de pente et montre que la porte
  // `sum_E4` n'etait jamais atteinte : l'ancienne porte `front_records` rendait
  // `3` AVANT l'impression, et le CTest `pente_mord` pouvait etre satisfait par
  // cette porte-la au lieu de la sienne. Une porte rouge ne doit pas masquer
  // celle que le test pretend exercer. On imprime donc tout, puis on refuse, et
  // chaque refus porte un motif DISTINCT que le CTest doit nommer.
  int bad_front = 0, bad_deg = 0, bad_e4 = 0;
  int refus_front = 0, refus_deg = 0, refus_e4 = 0;
  for (size_t k = 1; k < fronts.size(); ++k) {
    const double sf = std::log2(fronts[k] / fronts[k - 1]) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    const double sd = std::log2(fenetres[k] / fenetres[k - 1]) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente front_records=%.3f pente degre_residuel=%.3f (%lld->%lld)\n",
                sf, sd, ns[k - 1], ns[k]);
    bad_front = (sf >= max_slope) ? bad_front + 1 : 0;
    bad_deg = (sd >= max_slope) ? bad_deg + 1 : 0;
    if (bad_front >= 2 && !refus_front) refus_front = 1;
    if (bad_deg >= 2 && !refus_deg) refus_deg = 1;
  }
  for (size_t k = 1; k < e4_sums.size(); ++k) {
    const double ss = std::log2(std::max(1.0, e4_sums[k]) / std::max(1.0, e4_sums[k - 1])) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    const double sm = std::log2(e4_maxs[k] / e4_maxs[k - 1]) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente sum_E4=%.3f pente max_E4=%.3f (%lld->%lld)\n", ss, sm, ns[k - 1], ns[k]);
    if (g_max_slope_e4 > 0.0) {
      bad_e4 = (ss >= g_max_slope_e4) ? bad_e4 + 1 : 0;
      if (bad_e4 >= 2 && !refus_e4) refus_e4 = 1;
    }
  }
  // ORDRE DES VERDICTS : le plus specifique d'abord, pour qu'un test qui
  // desarme les portes anterieures obtienne bien le motif qu'il nomme.
  if (refus_e4) {
    std::fprintf(stderr, "REFUS DE PENTE sum_E4: deux pentes sum_E4 >= %.2f\n", g_max_slope_e4);
    return 3;
  }
  if (refus_front) {
    std::fprintf(stderr, "REFUS DE PENTE front_records: deux pentes >= %.2f\n", max_slope);
    return 3;
  }
  if (refus_deg) {
    std::fprintf(stderr, "REFUS DE PENTE degre_residuel: deux pentes >= %.2f\n", max_slope);
    return 3;
  }
  // LA PENTE QUI DECIDE REELLEMENT L'ARCHITECTURE. `sum_a E_4(a)` est le nombre
  // d'aretes candidates que le moteur shallow devrait traiter. Si elle croit
  // comme `n^2`, aucun cout par arete, si petit soit-il, ne tient le contrat ;
  // si elle croit comme `n`, `LocalShallowBall` devient finançable. Cette pente
  // est donc une porte, pas une decoration — la lecon de `0eb65f1`.
  std::printf("OK famille=%s sep=%lld/%lld\n", family.c_str(), p, q);
  return 0;
}
