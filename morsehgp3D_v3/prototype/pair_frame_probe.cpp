// Porte de l'ABI `PairFrame + CoreDepthLedger`.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=abi_modele_abstrait, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CETTE PORTE CERTIFIE, ET CE QU'ELLE NE CERTIFIE PAS
//
// Elle certifie l'ORDONNANCEUR, pas la geometrie. Le modele remplace le fuseau
// `W_q` par une matrice de statuts `S[p][w]` tiree d'une famille declaree ; il
// garde EXACTEMENT la structure des bornes du gateway reel :
//
//   `lower(P, F) = somme sur n dans F, si TOUS les (p,w) de P x n sont vrais, de |n|`
//   `upper(P, F) = somme sur n dans F, si UN SEUL (p,w) de P x n est vrai, de |n|`
//
// Ces deux bornes encadrent le compte vrai de chaque paire du bloc, et elles
// sont monotones sous raffinement de `P` comme de `n` — les deux proprietes
// dont depend tout le schema. Le juge est une force brute par paire.
//
// Elle NE certifie PAS : les predicats geometriques, le masque endpoint
// relationnel (un temoin `z` appartenant a `A` ou `B` n'a pas d'analogue dans
// ce modele abstrait), ni le census de circumboule. Le masque relationnel est
// la porte G5 de l'auditeur et il exige la geometrie reelle : c'est l'etape
// « recevoir q2 de bout en bout », pas celle-ci. Je le dis ici plutot que de
// laisser croire qu'une porte verte couvre plus qu'elle ne couvre.
//
// ---------------------------------------------------------------------------
// CE QUE LE CONTRE-AUDIT `a6171d` INTERDIT DE TRANSCRIRE, ET QUI NE L'EST PAS
//
//   le `std::vector::erase` au milieu         -> pile par bucket, jamais d'erase
//   la recherche lineaire du maximum          -> `--scission=buckets`, `O(1)`
//   le faux compteur de profondeur            -> `state_refine_iterations_max`
//   la masse candidate prise pour une HWM     -> deux compteurs distincts
//   la continuation reduite a un compteur     -> `CoreContinuation` serialisee,
//                                                relue, et reprise sous porte
//
// Le sequencement `classify -> reduce -> one action` est respecte : les bornes
// sont maintenues INCREMENTALEMENT, deux classifications par scission, aucune
// re-lecture de la frontiere entiere hors du selecteur vectoriel — qui n'est
// conserve que comme TERME DE COMPARAISON de la porte de coût.
//
// ---------------------------------------------------------------------------
// CODES DE SORTIE : 1 desaccord du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#include "prototype/pair_frame.hpp"

using namespace mhgp3v::pairframe;

namespace {

bool g_inject = false;
PfMutant g_mu = PfMutant::kNone;

int sortie(int code_normal, const char* diag) {
  if (g_inject) {
    std::printf("MUTANT TUE : %s\n", diag);
    return 4;
  }
  std::printf("FAUTE : %s\n", diag);
  return code_normal;
}

int refus(const char* diag) {
  std::printf("REFUS : %s\n", diag);
  return 2;
}

// ---------------------------------------------------------------------------
// L'ARBRE BINAIRE DES TEMOINS : un nœud couvre un intervalle contigu, comme un
// nœud LBVH couvre un intervalle de la permutation de Morton.
struct WNode { int lo = 0, hi = 0, g = -1, d = -1; };
std::vector<WNode> g_arbre;

int construire(int lo, int hi) {
  const int id = (int)g_arbre.size();
  g_arbre.push_back(WNode{lo, hi, -1, -1});
  if (hi - lo > 1) {
    const int mid = (lo + hi) / 2;
    const int a = construire(lo, mid);
    const int b = construire(mid, hi);
    g_arbre[id].g = a;
    g_arbre[id].d = b;
  }
  return id;
}

inline int taille(int n) { return g_arbre[n].hi - g_arbre[n].lo; }
inline bool feuille(int n) { return g_arbre[n].g < 0; }

// La profondeur STRUCTURALE de l'arbre, qui n'a rien a voir avec le nombre
// d'iterations de raffinement — c'est tout l'objet de la section 2 du
// contre-audit, et les deux doivent etre publiees separement.
int profondeur_interne(int n, int d) {
  if (feuille(n)) return d;
  const int a = profondeur_interne(g_arbre[n].g, d + 1);
  const int b = profondeur_interne(g_arbre[n].d, d + 1);
  return a > b ? a : b;
}

inline int bucket_de(int pop) {
  int b = 0;
  while ((1 << (b + 1)) <= pop) ++b;
  return b;  // `floor(log2(pop))`
}

// ---------------------------------------------------------------------------
// LES FAMILLES DE STATUTS. `prefixe` est celle qui compte : la paire `p` a
// exactement `k(p)` temoins en prefixe contigu, donc son compte vrai est `k(p)`
// et l'on peut FABRIQUER des paires dont le compte vaut exactement `h`. C'est
// la seule facon de mettre sous tension `>=` contre `>`.
std::vector<std::uint8_t> g_S;
int g_nP = 0, g_nW = 0;

inline bool statut(int p, int w) { return g_S[(std::size_t)p * (std::size_t)g_nW + w] != 0; }

std::uint32_t melange(std::uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}

bool remplir(const std::string& fam, unsigned seed) {
  g_S.assign((std::size_t)g_nP * (std::size_t)g_nW, 0);
  if (fam == "prefixe") {
    for (int p = 0; p < g_nP; ++p) {
      const int k = (p * 7 + (int)(seed % 5)) % (g_nW + 1);
      for (int w = 0; w < k; ++w) g_S[(std::size_t)p * g_nW + w] = 1;
    }
    return true;
  }
  if (fam == "aleatoire") {
    for (int p = 0; p < g_nP; ++p)
      for (int w = 0; w < g_nW; ++w)
        g_S[(std::size_t)p * g_nW + w] =
            (std::uint8_t)((melange((std::uint32_t)(p * 131 + w) ^ (seed * 2654435761U)) % 100u) < 55u);
    return true;
  }
  if (fam == "pleine" || fam == "vide") {
    // Les deux familles DEGENEREES : les seules ou la racine porte deja un
    // verdict. C'est la que le mutant `cap-avant-verdict` devient visible.
    g_S.assign((std::size_t)g_nP * (std::size_t)g_nW, (std::uint8_t)(fam == "pleine" ? 1 : 0));
    return true;
  }
  if (fam == "damier") {
    for (int p = 0; p < g_nP; ++p)
      for (int w = 0; w < g_nW; ++w)
        g_S[(std::size_t)p * g_nW + w] = (std::uint8_t)(((p / 4 + w / 4 + (int)seed) % 3) != 0);
    return true;
  }
  if (fam == "adversaire") {
    // CONTRE-FAMILLE DE TRAVAIL (G3 du contre-audit) : presque tout nœud
    // interne reste mixte jusqu'en bas. Un temoin sur deux est vrai, avec une
    // phase qui depend de la paire — aucun sous-arbre n'est jamais homogene
    // avant les feuilles, donc aucune fermeture precoce n'est possible.
    for (int p = 0; p < g_nP; ++p)
      for (int w = 0; w < g_nW; ++w)
        g_S[(std::size_t)p * g_nW + w] = (std::uint8_t)(((w + p) % 2) == 0);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// L'ETAT D'UN BLOC. La frontiere est SEGREGEE par classification, et les bornes
// sont maintenues incrementalement : une scission coûte deux classifications et
// une mise a jour `O(1)`, jamais une relecture de la frontiere.
//
// Les spans `NONE` sont RETIRES : ils creditent zero des deux bornes et zero du
// compte exact de chaque paire du bloc. Les garder serait du poids mort.
enum : std::uint8_t { kNone = 0, kAll = 1, kMixte = 2 };

struct Compteurs {
  long long witness_splits = 0, endpoint_splits = 0, exact_tiles = 0;
  long long child_classifications = 0, reclassifications = 0;
  long long selector_scan_items = 0, terminal_checks = 0;
  long long state_refine_iterations_max = 0;
  long long frontier_span_count_peak = 0, frontier_candidate_mass_peak = 0;
  long long pending_state_count = 0, pending_pair_mass = 0, pending_span_count = 0;
  long long resume_count = 0, etats = 0, spans_elimines = 0;
  long long witness_batches = 0, witness_batch_max = 0;
  long long pruned = 0, clear = 0;
  long long continuation_octets = 0;
};

Compteurs g_c;

std::uint8_t classifie_span(int p0, int p1, int n) {
  bool tous = true, un = false;
  for (int p = p0; p < p1 && (tous || !un); ++p)
    for (int w = g_arbre[n].lo; w < g_arbre[n].hi; ++w) {
      if (statut(p, w)) un = true; else tous = false;
    }
  if (!un) return kNone;
  return tous ? kAll : kMixte;
}

struct Etat {
  int p0 = 0, p1 = 0;
  std::vector<int> spans_all;         // portent le minorant
  std::vector<int> mixtes_feuilles;   // mixtes non scindables
  std::vector<std::vector<int>> buckets;  // mixtes non-feuilles, par `floor(log2)`
  std::uint32_t masque = 0;
  long long lower = 0, upper = 0;
  long long upper_sat_incr = -1;      // le majorant ecrete que le mutant maintient
  long long largeur = 0;              // `frontier_span_count`
  long long masse_candidate = 0;      // `frontier_candidate_mass`
  long long iterations = 0;

  Etat() : buckets(16) {}

  void ajoute(int n, std::uint8_t e) {
    if (e == kNone) { ++g_c.spans_elimines; return; }
    ++largeur;
    masse_candidate += taille(n);
    if (e == kAll) { spans_all.push_back(n); lower += taille(n); upper += taille(n); return; }
    upper += taille(n);
    if (feuille(n)) { mixtes_feuilles.push_back(n); return; }
    const int b = bucket_de(taille(n));
    buckets[(std::size_t)b].push_back(n);
    masque |= (std::uint32_t)1u << b;
  }

  bool a_mixte_non_feuille() const { return masque != 0; }
};

// ---------------------------------------------------------------------------
// LES DEUX SELECTEURS.
//
// `buckets` : plus haut bit du masque, puis sommet de pile. `O(1)`, aucun
// deplacement de vecteur. Le span choisi est a moins d'un facteur deux du
// maximum, ce qui suffit — l'ORDRE DE RAFFINAGE N'ENTRE PAS DANS LA CORRECTION,
// et c'est la porte `--politiques` qui l'atteste.
//
// `plus-gros` / `plus-petit` / `premier` : le selecteur vectoriel historique,
// conserve UNIQUEMENT comme terme de comparaison. Il rescanne la frontiere
// entiere a chaque iteration, et le compteur `selector_scan_items` le montre.
int selectionne(Etat& e, const std::string& heur, int* bucket_sortie) {
  if (heur == "buckets") {
    g_c.selector_scan_items += 1;
    int b = 15;
    while (b >= 0 && ((e.masque >> b) & 1u) == 0u) --b;
    if (b < 0) return -1;
    *bucket_sortie = b;
    return e.buckets[(std::size_t)b].back();
  }
  g_c.selector_scan_items += e.largeur;  // le rescan integral, y compris les decides
  int meilleur = -1, mb = -1;
  for (int b = 0; b < 16; ++b)
    for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
      const int n = e.buckets[(std::size_t)b][i];
      if (meilleur < 0) { meilleur = n; mb = b; if (heur == "premier") { *bucket_sortie = mb; return meilleur; } continue; }
      if (heur == "plus-petit" ? (taille(n) < taille(meilleur)) : (taille(n) > taille(meilleur))) {
        meilleur = n; mb = b;
      }
    }
  *bucket_sortie = mb;
  return meilleur;
}

void retire_du_bucket(Etat& e, int b, int n) {
  std::vector<int>& v = e.buckets[(std::size_t)b];
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i] == n) { v[i] = v.back(); v.pop_back(); break; }  // pile, jamais d'erase au milieu
  if (v.empty()) e.masque &= ~((std::uint32_t)1u << b);
  --e.largeur;
  e.masse_candidate -= taille(n);
  e.upper -= taille(n);
  // LA FAUTE DU § 3.3, EXECUTEE. Le mutant decremente un majorant DEJA ECRETE
  // au lieu de recalculer depuis la masse exacte ; c'est le seul endroit ou
  // elle peut se produire, et elle est de surete.
  if (e.upper_sat_incr >= 0) e.upper_sat_incr -= taille(n);
}

void scinde_temoin(Etat& e, int n, int b) {
  retire_du_bucket(e, b, n);
  const int g = g_arbre[n].g, d = g_arbre[n].d;
  g_c.child_classifications += 2;
  e.ajoute(g, classifie_span(e.p0, e.p1, g));
  e.ajoute(d, classifie_span(e.p0, e.p1, d));
  ++g_c.witness_splits;
}

// SPLIT_WITNESS_BATCH. Les parents choisis sont deux a deux incomparables — ils
// appartiennent a une antichaine —, donc les remplacer simultanement preserve
// l'antichaine, la partition de masse et l'invariant `L <= N <= U`, sans
// dependre d'aucun ordre. `B = 0` signifie « tous les mixtes non-feuilles ».
long long scinde_lot(Etat& e, const std::string& heur, std::uint32_t B,
                     std::uint64_t front_cap) {
  std::vector<std::pair<int, int>> lot;  // (nœud, bucket)
  if (heur == "buckets" && B == 1) {
    int b = -1;
    const int n = selectionne(e, heur, &b);
    if (n < 0) return 0;
    lot.push_back(std::make_pair(n, b));
  } else {
    g_c.selector_scan_items += (heur == "buckets") ? 1 : e.largeur;
    for (int b = 15; b >= 0; --b)
      for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
        if (B != 0 && lot.size() >= (std::size_t)B) break;
        lot.push_back(std::make_pair(e.buckets[(std::size_t)b][i], b));
      }
  }
  long long faits = 0;
  for (std::size_t i = 0; i < lot.size(); ++i) {
    if ((std::uint64_t)(e.largeur + 1) > front_cap && faits > 0) break;
    scinde_temoin(e, lot[i].first, lot[i].second);
    ++faits;
  }
  ++g_c.witness_batches;
  if (faits > g_c.witness_batch_max) g_c.witness_batch_max = faits;
  return faits;
}

// L'HERITAGE SOUS RAFFINEMENT D'EXTREMITE. Un span `ALL` reste `ALL` et un span
// `NONE` reste `NONE` quand `P` retrecit — c'est le theoreme de monotonie. Seuls
// les mixtes sont reclassifies, et c'est exactement l'economie que le theoreme
// autorise : redescendre depuis la racine serait du travail pur.
Etat restreint(const Etat& e, int p0, int p1) {
  Etat r;
  r.p0 = p0; r.p1 = p1;
  r.upper_sat_incr = e.upper_sat_incr;  // l'ecrete se propage : la faute aussi
  for (std::size_t i = 0; i < e.spans_all.size(); ++i) r.ajoute(e.spans_all[i], kAll);
  for (std::size_t i = 0; i < e.mixtes_feuilles.size(); ++i) {
    ++g_c.reclassifications;
    r.ajoute(e.mixtes_feuilles[i], classifie_span(p0, p1, e.mixtes_feuilles[i]));
  }
  for (int b = 0; b < 16; ++b)
    for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
      ++g_c.reclassifications;
      r.ajoute(e.buckets[(std::size_t)b][i], classifie_span(p0, p1, e.buckets[(std::size_t)b][i]));
    }
  return r;
}

// ---------------------------------------------------------------------------
// LA CONTINUATION, SERIALISEE POUR DE VRAI. Un compteur ne prouve rien ; un
// tampon d'octets relu qui redonne le meme resultat, si.
void ecrire32(std::vector<std::uint8_t>& b, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) b.push_back((std::uint8_t)((v >> (8 * i)) & 0xffu));
}
std::uint32_t lire32(const std::vector<std::uint8_t>& b, std::size_t& o) {
  std::uint32_t v = 0;
  for (int i = 0; i < 4; ++i) v |= (std::uint32_t)b[o++] << (8 * i);
  return v;
}

std::vector<std::uint8_t> serialise(const Etat& e, const CoreContinuation& k) {
  std::vector<std::uint8_t> b;
  ecrire32(b, (std::uint32_t)k.rect_id);
  ecrire32(b, (std::uint32_t)k.a_node);
  ecrire32(b, (std::uint32_t)k.b_node);
  b.push_back(k.lane); b.push_back(k.threshold);
  b.push_back(k.lower_open_sat);
  ecrire32(b, k.frontier_candidate_mass_exact);
  ecrire32(b, k.schema_version);
  ecrire32(b, k.policy_version);
  ecrire32(b, k.cloud_epoch);
  ecrire32(b, (std::uint32_t)(k.pair_mass & 0xffffffffu));
  ecrire32(b, (std::uint32_t)(k.pair_mass >> 32));
  ecrire32(b, (std::uint32_t)e.p0);
  ecrire32(b, (std::uint32_t)e.p1);
  ecrire32(b, (std::uint32_t)e.spans_all.size());
  for (std::size_t i = 0; i < e.spans_all.size(); ++i) ecrire32(b, (std::uint32_t)e.spans_all[i]);
  std::vector<int> mixtes = e.mixtes_feuilles;
  for (int t = 0; t < 16; ++t)
    for (std::size_t i = 0; i < e.buckets[(std::size_t)t].size(); ++i)
      mixtes.push_back(e.buckets[(std::size_t)t][i]);
  ecrire32(b, (std::uint32_t)mixtes.size());
  for (std::size_t i = 0; i < mixtes.size(); ++i) ecrire32(b, (std::uint32_t)mixtes[i]);
  return b;
}

Etat deserialise(const std::vector<std::uint8_t>& b, CoreContinuation& k) {
  std::size_t o = 0;
  k.rect_id = (int)lire32(b, o);
  k.a_node = (int)lire32(b, o);
  k.b_node = (int)lire32(b, o);
  k.lane = b[o++]; k.threshold = b[o++];
  k.lower_open_sat = b[o++];
  k.frontier_candidate_mass_exact = lire32(b, o);
  k.schema_version = lire32(b, o);
  k.policy_version = lire32(b, o);
  k.cloud_epoch = lire32(b, o);
  { const std::uint64_t lo = lire32(b, o); const std::uint64_t hi = lire32(b, o);
    k.pair_mass = lo | (hi << 32); }
  Etat e;
  e.p0 = (int)lire32(b, o);
  e.p1 = (int)lire32(b, o);
  const std::uint32_t na = lire32(b, o);
  for (std::uint32_t i = 0; i < na; ++i) e.ajoute((int)lire32(b, o), kAll);
  const std::uint32_t nm = lire32(b, o);
  for (std::uint32_t i = 0; i < nm; ++i) {
    const int n = (int)lire32(b, o);
    e.ajoute(n, kMixte);
  }
  return e;
}

// ---------------------------------------------------------------------------
struct Sortie {
  std::vector<int> dec;
  int faute = 0;
  const char* diag = "";
};

Sortie descente(int h, std::uint64_t cap, std::uint64_t front_cap,
                long long budget_temoins, const std::string& heur,
                bool cap_sur_masse, bool via_octets,
                std::uint32_t lot_temoins = 1, int politique_id = 1,
                const std::string& heur_reprise = std::string()) {
  // LA REPRISE PEUT CHANGER DE POLITIQUE. Une continuation capee sous une
  // politique doit se reprendre sous une AUTRE et rendre les memes identites :
  // c'est le § 4.2 de l'arbitrage, et c'est ce qui interdit a la bucketisation
  // d'entrer dans l'ABI de preuve. `heur_courante` bascule des la vague 2.
  std::string heur_courante = heur;
  Sortie s;
  s.dec.assign((std::size_t)g_nP, -1);
  std::vector<Etat> vague, suivante;
  {
    Etat r;
    r.p0 = 0; r.p1 = g_nP;
    ++g_c.child_classifications;
    r.ajoute(0, classifie_span(0, g_nP, 0));
    vague.push_back(r);
  }

  std::uint64_t cap_v = cap, front_cap_v = front_cap;
  int garde = 0;
  while (!vague.empty()) {
    if (++garde > 64) {
      s.faute = 3;
      s.diag = "plus de 64 vagues : l'escalade de ressource ne converge pas";
      return s;
    }
    long long budget = budget_temoins;
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
        if (e.masse_candidate > g_c.frontier_candidate_mass_peak)
          g_c.frontier_candidate_mass_peak = e.masse_candidate;

        CoreDepthLedger L;
        L.reject_threshold = (std::uint8_t)h;
        L.sature(e.lower, e.upper - e.lower, g_mu);
        if (g_mu == PfMutant::kUpperSatureIncr) {
          if (e.upper_sat_incr < 0)  // premiere vue : on ecrete, et c'est la faute
            e.upper_sat_incr = (e.upper > h) ? h : e.upper;
          L.upper_sat_mutant = (std::uint8_t)(e.upper_sat_incr < 0 ? 0 : e.upper_sat_incr);
        }
        L.frontier_width = (std::uint32_t)e.largeur;  // scalaire : aucun `O(F)` par vague
        PairFrame F;
        F.rect_id = 0; F.a_node = 0; F.b_node = 0;
        F.pair_mass = (std::uint64_t)(e.p1 - e.p0);
        PolitiqueV0 pol;
        pol.exact_tile_cap = cap_v;
        pol.frontier_cap = front_cap_v;
        pol.cap_sur_masse_seule = cap_sur_masse;
        pol.witness_batch = lot_temoins;
        pol.witness_budget_available = (budget > 0);

        ++g_c.terminal_checks;
        const bool endpoint_scindable = (e.p1 - e.p0) > 1;
        const Action a = choisir_action(L, F, e.a_mixte_non_feuille(),
                                        endpoint_scindable, pol, g_mu);
        const CoreFate f = fate_apres_action(a, L, g_mu);

        if (a == Action::kTerminal) {
          if (f == CoreFate::kMixedCore) {
            s.faute = 3;
            s.diag = "action terminale sans verdict : le bloc reste indecidable";
            return s;
          }
          const int v = (f == CoreFate::kPrunedUniversal) ? 1 : 0;
          if (v) ++g_c.pruned; else ++g_c.clear;
          for (int p = e.p0; p < e.p1; ++p) {
            if (s.dec[(std::size_t)p] >= 0) {
              s.faute = 3;
              s.diag = "paire decidee deux fois : la partition du travail est cassee";
              return s;
            }
            s.dec[(std::size_t)p] = v;
          }
          break;
        }

        if (a == Action::kPending) {
          // LE FATE EST CE SUR QUOI L'APPELANT AGIT, DONC LA SONDE AGIT DESSUS.
          // Si l'ABI rend un verdict sur un depassement de cap, il faut
          // l'appliquer ici pour que le juge le voie : le neutraliser
          // silencieusement rendrait le mutant `pending-pruned` inerte, et la
          // porte mesurerait alors le vide.
          if (f != CoreFate::kPendingResource) {
            const int v = (f == CoreFate::kPrunedUniversal) ? 1 : 0;
            if (v) ++g_c.pruned; else ++g_c.clear;
            for (int p = e.p0; p < e.p1; ++p) {
              if (s.dec[(std::size_t)p] >= 0) {
                s.faute = 3;
                s.diag = "paire decidee deux fois : la partition du travail est cassee";
                return s;
              }
              s.dec[(std::size_t)p] = v;
            }
            break;
          }
          ++g_c.pending_state_count;
          g_c.pending_pair_mass += (long long)F.pair_mass;
          g_c.pending_span_count += e.largeur;
          CoreContinuation k;
          k.rect_id = F.rect_id; k.a_node = F.a_node; k.b_node = F.b_node;
          k.lane = 4; k.threshold = (std::uint8_t)h;
          k.lower_open_sat = L.lower_open_sat;
          k.frontier_candidate_mass_exact = L.frontier_candidate_mass_exact;
          k.policy_version = politique_id; k.schema_version = 1;
          k.cloud_epoch = 1; k.pair_mass = F.pair_mass;
          const std::vector<std::uint8_t> octets = serialise(e, k);
          g_c.continuation_octets += (long long)octets.size();
          if (via_octets) {
            // LA REPRISE PASSE PAR LES OCTETS. Rien de l'etat en memoire ne
            // survit : seul le tampon relu redemarre le calcul.
            CoreContinuation k2;
            Etat e2 = deserialise(octets, k2);
            if (k2.threshold != k.threshold || k2.pair_mass != k.pair_mass ||
                k2.rect_id != k.rect_id || k2.policy_version != k.policy_version ||
                e2.p0 != e.p0 || e2.p1 != e.p1 ||
                e2.lower != e.lower || e2.upper != e.upper ||
                e2.largeur != e.largeur) {
              s.faute = 3;
              s.diag = "la continuation relue ne restitue pas son cadre";
              return s;
            }
            suivante.push_back(e2);
          } else {
            suivante.push_back(e);
          }
          break;
        }

        if (a == Action::kExactifyTile) {
          ++g_c.exact_tiles;
          for (int p = e.p0; p < e.p1; ++p) {
            long long n = 0;
            for (std::size_t i = 0; i < e.spans_all.size(); ++i) n += taille(e.spans_all[i]);
            for (std::size_t i = 0; i < e.mixtes_feuilles.size(); ++i)
              for (int w = g_arbre[e.mixtes_feuilles[i]].lo; w < g_arbre[e.mixtes_feuilles[i]].hi; ++w)
                if (statut(p, w)) ++n;
            for (int b = 0; b < 16; ++b)
              for (std::size_t i = 0; i < e.buckets[(std::size_t)b].size(); ++i) {
                const int nd = e.buckets[(std::size_t)b][i];
                for (int w = g_arbre[nd].lo; w < g_arbre[nd].hi; ++w)
                  if (statut(p, w)) ++n;
              }
            if (s.dec[(std::size_t)p] >= 0) {
              s.faute = 3;
              s.diag = "paire decidee deux fois : la partition du travail est cassee";
              return s;
            }
            s.dec[(std::size_t)p] = (n >= h) ? 1 : 0;
          }
          break;
        }

        if (a == Action::kSplitWitness) {
          // Terme de comparaison G1 : `feuilles` est le lot de taille infinie.
          const std::uint32_t B = (heur_courante == "feuilles") ? 0u : pol.witness_batch;
          const long long faits = scinde_lot(e, heur_courante, B, front_cap_v);
          if (faits <= 0) {
            s.faute = 3;
            s.diag = "SPLIT_WITNESS sans span mixte non-feuille : l'ABI ment sur son etat";
            return s;
          }
          budget -= faits;
          continue;
        }

        // `kSplitOneEndpoint` : UNE seule scission d'extremite, jamais les deux.
        ++g_c.endpoint_splits;
        const int mid = (e.p0 + e.p1) / 2;
        vague.push_back(restreint(e, e.p0, mid));
        vague.push_back(restreint(e, mid, e.p1));
        break;
      }
    }
    vague.swap(suivante);
    if (!vague.empty()) {
      ++g_c.resume_count;
      cap_v = cap_v * 2 + 1;
      front_cap_v = front_cap_v * 2 + 1;
      if (!heur_reprise.empty()) heur_courante = heur_reprise;
    }
  }

  for (int p = 0; p < g_nP; ++p)
    if (s.dec[(std::size_t)p] < 0) {
      s.faute = 3;
      s.diag = "paire jamais decidee : la descente ne termine pas";
      return s;
    }
  return s;
}

std::vector<int> juge(int h) {
  std::vector<int> v((std::size_t)g_nP, 0);
  for (int p = 0; p < g_nP; ++p) {
    long long n = 0;
    for (int w = 0; w < g_nW; ++w) if (statut(p, w)) ++n;
    v[(std::size_t)p] = (n >= h) ? 1 : 0;
  }
  return v;
}

void imprime_compteurs(const char* etiq) {
  std::printf("%s witness_splits=%lld endpoint_splits=%lld exact_tiles=%lld"
              " child_classifications=%lld reclassifications=%lld\n",
              etiq, g_c.witness_splits, g_c.endpoint_splits, g_c.exact_tiles,
              g_c.child_classifications, g_c.reclassifications);
  std::printf("%s selector_scan_items=%lld terminal_checks=%lld"
              " state_refine_iterations_max=%lld etats=%lld\n",
              etiq, g_c.selector_scan_items, g_c.terminal_checks,
              g_c.state_refine_iterations_max, g_c.etats);
  std::printf("%s frontier_span_count_peak=%lld frontier_candidate_mass_peak=%lld"
              " spans_elimines=%lld lbvh_internal_depth_max=%d\n",
              etiq, g_c.frontier_span_count_peak, g_c.frontier_candidate_mass_peak,
              g_c.spans_elimines, profondeur_interne(0, 0));
  std::printf("%s pending_state_count=%lld pending_pair_mass=%lld pending_span_count=%lld"
              " resume_count=%lld continuation_octets=%lld\n",
              etiq, g_c.pending_state_count, g_c.pending_pair_mass,
              g_c.pending_span_count, g_c.resume_count, g_c.continuation_octets);
  std::printf("%s pruned=%lld clear=%lld\n", etiq, g_c.pruned, g_c.clear);
}

// ---------------------------------------------------------------------------
int mode_exhaustif(long long min_par_action) {
  long long vus[5] = {0, 0, 0, 0, 0};
  long long etats = 0;
  const std::uint64_t caps[3] = {1, 8, 64};
  for (int h = 1; h <= 12; ++h)
    for (int lo = 0; lo <= h; ++lo)
      for (int up = lo; up <= h; ++up)
        for (int ic = 0; ic < 3; ++ic)
          for (int im = 0; im < 4; ++im)
            for (int sp = 0; sp < 2; ++sp)
              for (int ep = 0; ep < 2; ++ep)
                for (int bu = 0; bu < 2; ++bu) {
                  const std::uint64_t masses[4] = {1, caps[ic] - 1, caps[ic], caps[ic] + 1};
                  CoreDepthLedger L;
                  L.reject_threshold = (std::uint8_t)h;
                  L.lower_open_sat = (std::uint8_t)lo;
                  L.frontier_candidate_mass_exact = (std::uint32_t)(up - lo);
                  L.upper_sat_mutant = (std::uint8_t)up;  // identique hors maintien
                  L.frontier_width = (std::uint32_t)(1 + (im % 3));
                  PairFrame F; F.pair_mass = masses[im];
                  PolitiqueV0 p; p.exact_tile_cap = caps[ic];
                  p.frontier_cap = (std::uint64_t)(1 + (ic % 3));
                  p.witness_budget_available = (bu != 0);
                  const Action a = choisir_action(L, F, sp != 0, ep != 0, p, g_mu);
                  const CoreFate f = fate_apres_action(a, L, g_mu);
                  ++etats;
                  vus[(int)a]++;

                  const bool verdict = (fate_terminal(L, g_mu) != CoreFate::kMixedCore);
                  if ((a == Action::kTerminal) != verdict)
                    return sortie(3, "TERMINAL et fate_terminal divergent");
                  if (a == Action::kPending && f != CoreFate::kPendingResource)
                    return sortie(3, "un depassement de cap a produit un verdict");
                  if (est_pruned(L, g_mu) && est_core_clear(L, g_mu))
                    return sortie(3, "PRUNED et CORE_CLEAR simultanes");
                  if (est_pruned(L, g_mu)) {
                    CoreDepthLedger M = L;
                    M.lower_open_sat = (std::uint8_t)h;
                    if (!est_pruned(M, g_mu))
                      return sortie(3, "la mort n'est pas monotone en lower");
                  }
                  for (int jc = 0; jc < 3; ++jc)
                    for (int jb = 0; jb < 2; ++jb) {
                      PolitiqueV0 q; q.exact_tile_cap = caps[jc];
                      q.frontier_cap = (std::uint64_t)(1 + (jc % 3));
                      q.witness_budget_available = (jb != 0);
                      const Action b = choisir_action(L, F, sp != 0, ep != 0, q, g_mu);
                      if (verdict && (b != Action::kTerminal ||
                                      fate_apres_action(b, L, g_mu) != f))
                        return sortie(3, "le verdict depend de la politique");
                    }
                }
  std::printf("abi exhaustif etats=%lld terminal=%lld exactify=%lld witness=%lld"
              " endpoint=%lld pending=%lld\n",
              etats, vus[0], vus[1], vus[2], vus[3], vus[4]);
  for (int i = 0; i < 5; ++i)
    if (vus[i] < min_par_action)
      return sortie(3, "plancher de couverture d'action non atteint");
  std::printf("abi exhaustif desaccords=0 plancher_action=%lld\n", min_par_action);
  return 0;
}

int mode_saturation(long long min_grands) {
  long long cas = 0, grands = 0;
  const long long echelles[9] = {0, 1, 7, 11, 12, 255, 256, 100000, 1000000000};
  for (int h = 1; h <= 12; ++h)
    for (int i = 0; i < 9; ++i)
      for (int j = 0; j < 9; ++j) {
        const long long lo = echelles[i], up = echelles[j];
        if (up < lo) continue;
        CoreDepthLedger L;
        L.reject_threshold = (std::uint8_t)h;
        L.sature(lo, up - lo, g_mu);           // le ledger prend la MASSE
        L.upper_sat_mutant = (std::uint8_t)(up > h ? h : up);
        ++cas;
        if (lo > 255 || up - lo > 255) ++grands;
        if (est_pruned(L, g_mu) != (lo >= h))
          return sortie(1, "la saturation a change la decision de mort");
        if (est_core_clear(L, g_mu) != (up < h))
          return sortie(1, "la saturation a change la decision de clarte");
      }
  std::printf("abi saturation cas=%lld grands=%lld desaccords=0\n", cas, grands);
  if (grands < min_grands) return sortie(3, "plancher de comptes > 255 non atteint");
  return 0;
}

struct Config { std::uint64_t cap, front_cap; long long budget; const char* heur; bool octets; };

struct Planchers {
  long long min_pending = 0, min_scissions = 0, min_tuiles = 0;
  long long min_morts = 0, min_vivantes = 0, min_pruned = 0, min_reprises = 0;
  long long max_tuiles = -1, max_scan = -1;
};

int mode_descente(int h, std::uint64_t cap, std::uint64_t front_cap, long long budget,
                  bool cap_sur_masse, bool octets, const std::string& heur,
                  const Planchers& pl, std::uint32_t lot) {
  g_c = Compteurs();
  Sortie s = descente(h, cap, front_cap, budget, heur, cap_sur_masse, octets, lot);
  if (s.faute) return sortie(s.faute, s.diag);
  const std::vector<int> vrai = juge(h);
  long long ecart = 0, morts = 0, vivantes = 0;
  for (int p = 0; p < g_nP; ++p) {
    if (s.dec[(std::size_t)p] != vrai[(std::size_t)p]) ++ecart;
    if (vrai[(std::size_t)p]) ++morts; else ++vivantes;
  }
  std::printf("abi descente paires=%d temoins=%d seuil=%d cap=%llu front_cap=%llu"
              " budget=%lld cap_sur=%s scission=%s reprise=%s\n",
              g_nP, g_nW, h, (unsigned long long)cap, (unsigned long long)front_cap,
              budget, cap_sur_masse ? "masse" : "tuile", heur.c_str(),
              octets ? "octets" : "memoire");
  imprime_compteurs("abi descente");
  std::printf("abi descente juge morts=%lld vivantes=%lld ecart=%lld\n",
              morts, vivantes, ecart);
  if (ecart) return sortie(1, "la descente contredit la force brute");
  if (g_c.pending_state_count < pl.min_pending)
    return sortie(3, "plancher de continuations non atteint");
  if (g_c.resume_count < pl.min_reprises) return sortie(3, "plancher de reprises non atteint");
  if (g_c.witness_splits < pl.min_scissions) return sortie(3, "plancher de scissions non atteint");
  if (g_c.exact_tiles < pl.min_tuiles) return sortie(3, "plancher de tuiles exactes non atteint");
  if (morts < pl.min_morts) return sortie(3, "plancher de paires mortes non atteint");
  if (vivantes < pl.min_vivantes) return sortie(3, "plancher de paires vivantes non atteint");
  if (g_c.pruned < pl.min_pruned) return sortie(3, "plancher de fermetures universelles non atteint");
  if (pl.max_tuiles >= 0 && g_c.exact_tiles > pl.max_tuiles)
    return sortie(3, "plafond de tuiles exactes depasse : du travail a ete developpe pour rien");
  if (pl.max_scan >= 0 && g_c.selector_scan_items > pl.max_scan)
    return sortie(3, "plafond de coût de selection depasse");
  // INVARIANT DU SELECTEUR A BUCKETS, toujours arme : une unite de scan par
  // scission, plus une par verification terminale. C'est la propriete qui
  // remplace le rescan integral du selecteur vectoriel, et elle n'a pas a etre
  // demandee par une option pour etre exigee.
  if (heur == "buckets" &&
      g_c.selector_scan_items > g_c.witness_splits + g_c.terminal_checks)
    return sortie(3, "le selecteur a buckets n'est pas en O(1) par scission");
  return 0;
}

// ---------------------------------------------------------------------------
// LA PORTE DIFFERENTIELLE : G1 (ciblee contre feuilles), G2 (invariance a la
// politique), G4 (coût du selecteur) et G5 (cap atteint, reprise par octets)
// en un seul passage. Douze politiques doivent rendre le MEME vecteur de
// decisions ; seul le travail varie.
int mode_politiques(int h, long long min_ecart_travail, long long facteur_scan) {
  const Config confs[12] = {
    {8, 64, 1000000000, "buckets", false},     // reference
    {8, 64, 1000000000, "plus-gros", false},
    {8, 64, 1000000000, "plus-petit", false},
    {8, 64, 1000000000, "premier", false},
    {8, 64, 1000000000, "feuilles", false},    // G1
    {32, 64, 1000000000, "buckets", false},
    {4096, 64, 1000000000, "buckets", false},  // la racine s'exactifie
    {8, 64, 1, "buckets", false},              // budget de temoins etrangle
    {8, 64, 2, "feuilles", false},
    {2, 3, 1000000000, "buckets", false},      // caps etrangles : continuations
    {2, 3, 1000000000, "buckets", true},       // G5 : reprise PAR LES OCTETS
    {2, 4, 1, "feuilles", true},
  };
  const int nconf = 12;
  std::vector<int> ref;
  long long travail_min = -1, travail_max = -1, pend_total = 0, reprises = 0;
  long long scan_buckets = -1, scan_vecteur = -1, splits_buckets = -1;
  long long octets_total = 0;
  for (int i = 0; i < nconf; ++i) {
    g_c = Compteurs();
    Sortie s = descente(h, confs[i].cap, confs[i].front_cap, confs[i].budget,
                        confs[i].heur, false, confs[i].octets);
    if (s.faute) return sortie(s.faute, s.diag);
    pend_total += g_c.pending_state_count;
    reprises += g_c.resume_count;
    octets_total += g_c.continuation_octets;
    const long long trav = g_c.child_classifications + g_c.reclassifications;
    if (travail_min < 0 || trav < travail_min) travail_min = trav;
    if (trav > travail_max) travail_max = trav;
    if (i == 0) { scan_buckets = g_c.selector_scan_items; splits_buckets = g_c.witness_splits; }
    if (i == 1) scan_vecteur = g_c.selector_scan_items;
    std::printf("abi politique %d cap=%llu front_cap=%llu budget=%lld scission=%s"
                " reprise=%s splits=%lld scan=%lld travail=%lld pending=%lld reprises=%lld\n",
                i, (unsigned long long)confs[i].cap,
                (unsigned long long)confs[i].front_cap, confs[i].budget, confs[i].heur,
                confs[i].octets ? "octets" : "memoire", g_c.witness_splits,
                g_c.selector_scan_items, trav, g_c.pending_state_count, g_c.resume_count);
    if (i == 0) { ref = s.dec; continue; }
    long long d = 0;
    for (int p = 0; p < g_nP; ++p) if (s.dec[(std::size_t)p] != ref[(std::size_t)p]) ++d;
    if (d) return sortie(1, "deux politiques donnent deux resultats");
  }
  const std::vector<int> vrai = juge(h);
  long long d = 0;
  for (int p = 0; p < g_nP; ++p) if (ref[(std::size_t)p] != vrai[(std::size_t)p]) ++d;
  std::printf("abi politiques nb=%d divergences=0 juge_ecart=%lld pending_total=%lld"
              " reprises_total=%lld octets_total=%lld travail_min=%lld travail_max=%lld\n",
              nconf, d, pend_total, reprises, octets_total, travail_min, travail_max);
  std::printf("abi selecteur buckets_scan=%lld buckets_splits=%lld vecteur_scan=%lld\n",
              scan_buckets, splits_buckets, scan_vecteur);
  if (d) return sortie(1, "la politique de reference contredit la force brute");
  if (pend_total <= 0) return sortie(3, "aucune continuation exercee : G2 serait vide");
  if (reprises <= 0) return sortie(3, "aucune reprise exercee : G5 serait vide");
  if (octets_total <= 0) return sortie(3, "aucune continuation serialisee");
  if (min_ecart_travail > 0 && travail_max - travail_min < min_ecart_travail)
    return sortie(3, "les politiques ne se distinguent pas en travail : porte vide");
  // G4 : le selecteur a buckets coûte UNE unite par scission ; le selecteur
  // vectoriel rescanne la frontiere entiere. La porte exige les deux faits.
  if (scan_buckets > splits_buckets + g_c.terminal_checks)
    return sortie(3, "le selecteur a buckets n'est pas en O(1) par scission");
  if (facteur_scan > 0 && scan_vecteur < scan_buckets * facteur_scan)
    return sortie(3, "le selecteur vectoriel ne coûte pas plus : la porte de coût serait vide");
  return 0;
}

// ---------------------------------------------------------------------------
// LA PORTE DE REPRISE CROISEE, § 4.2 de l'arbitrage, dans sa forme forte :
//
//   caper sous la politique A
//   reprendre sous la politique B
//   == non cape sous la politique C
//
// Si cette porte passe, la bucketisation ne peut pas etre entree dans l'ABI de
// preuve sans qu'on s'en apercoive : un record qui dependrait de la politique
// qui l'a produit ne se reprendrait pas sous une autre.
int mode_reprise_croisee(int h) {
  struct Croisee { const char* a; const char* b; std::uint64_t cap, fcap; std::uint32_t lot; };
  const Croisee cas[8] = {
    {"buckets", "plus-gros", 2, 3, 1},
    {"buckets", "feuilles", 2, 3, 1},
    {"plus-petit", "buckets", 2, 3, 1},
    {"premier", "buckets", 2, 4, 1},
    {"buckets", "plus-gros", 2, 3, 4},   // lot de quatre a la capture
    {"feuilles", "buckets", 2, 3, 1},
    {"buckets", "plus-gros", 2, 64, 4},  // frontiere large : le lot depasse un
    {"buckets", "buckets", 2, 64, 8},
  };
  g_c = Compteurs();
  Sortie libre = descente(h, 1000000, 1000000, 1000000000, "feuilles", false, false, 0);
  if (libre.faute) return sortie(libre.faute, libre.diag);
  const std::vector<int> vrai = juge(h);
  long long d0 = 0;
  for (int p = 0; p < g_nP; ++p) if (libre.dec[(std::size_t)p] != vrai[(std::size_t)p]) ++d0;
  if (d0) return sortie(1, "la course non capee contredit deja la force brute");
  long long pend_total = 0, reprises = 0;
  long long lot_max_global = 0;
  for (int i = 0; i < 8; ++i) {
    g_c = Compteurs();
    Sortie s = descente(h, cas[i].cap, cas[i].fcap, 1000000000, cas[i].a, false, true,
                        cas[i].lot, 100 + i, cas[i].b);
    if (s.faute) return sortie(s.faute, s.diag);
    pend_total += g_c.pending_state_count;
    reprises += g_c.resume_count;
    if (g_c.witness_batch_max > lot_max_global) lot_max_global = g_c.witness_batch_max;
    long long d = 0;
    for (int p = 0; p < g_nP; ++p) if (s.dec[(std::size_t)p] != libre.dec[(std::size_t)p]) ++d;
    std::printf("abi croisee %d capture=%s reprise=%s lot=%u pending=%lld reprises=%lld"
                " lots=%lld lot_max=%lld ecart=%lld\n",
                i, cas[i].a, cas[i].b, cas[i].lot, g_c.pending_state_count,
                g_c.resume_count, g_c.witness_batches, g_c.witness_batch_max, d);
    if (d) return sortie(1, "une reprise sous une autre politique change le resultat");
  }
  std::printf("abi croisee nb=8 divergences=0 pending_total=%lld reprises_total=%lld"
              " lot_max_global=%lld\n", pend_total, reprises, lot_max_global);
  if (pend_total <= 0) return sortie(3, "aucune continuation capturee : la porte croisee serait vide");
  if (reprises <= 0) return sortie(3, "aucune reprise exercee : la porte croisee serait vide");
  // Sans lot strictement superieur a un, `SPLIT_WITNESS_BATCH` ne serait qu'un
  // champ d'ABI jamais exerce, et la porte le certifierait par vacuite.
  if (lot_max_global < 2) return sortie(3, "aucun lot de temoins au-dela de un : le batch serait vide");
  return 0;
}

// ---------------------------------------------------------------------------
int mode_fixtures() {
  long long n = 0;
  auto pose = [&](int h, int lo, int up) {
    CoreDepthLedger L;
    L.reject_threshold = (std::uint8_t)h;
    L.lower_open_sat = (std::uint8_t)lo;
    L.frontier_candidate_mass_exact = (std::uint32_t)(up - lo);
    L.upper_sat_mutant = (std::uint8_t)up;
    ++n;
    return L;
  };
  // `lower == h` : la mort est atteinte A L'EGALITE. `h_q` est le compte auquel
  // l'ancre meurt, pas celui au-dela duquel elle meurt.
  if (fate_terminal(pose(10, 10, 10), g_mu) != CoreFate::kPrunedUniversal)
    return sortie(3, "fixture lower==h : la mort a l'egalite est perdue");
  if (est_pruned(pose(10, 9, 10), g_mu))
    return sortie(3, "fixture lower==h-1 : une mort est inventee");
  if (fate_terminal(pose(10, 0, 9), g_mu) != CoreFate::kCoreClear)
    return sortie(3, "fixture upper==h-1 : la clarte est perdue");
  if (fate_terminal(pose(10, 0, 10), g_mu) != CoreFate::kMixedCore)
    return sortie(3, "fixture upper==h : une clarte est inventee");
  // Le cap porte sur `pair_mass * |frontiere|` : frontiere 4, bascule a 16.
  {
    CoreDepthLedger L = pose(10, 0, 10);
    L.frontier_width = 4;
    PolitiqueV0 p; p.exact_tile_cap = 64; p.frontier_cap = 8;
    PairFrame F; F.pair_mass = 16;
    if (cout_tuile(L, F, p) != 64)
      return sortie(3, "fixture coût de tuile : le produit masse x frontiere est faux");
    if (choisir_action(L, F, true, true, p, g_mu) != Action::kExactifyTile)
      return sortie(3, "fixture coût==cap : la tuile exacte n'est pas prise");
    F.pair_mass = 17;
    if (choisir_action(L, F, true, true, p, g_mu) != Action::kSplitWitness)
      return sortie(3, "fixture coût==cap+4 : la tuile exacte est prise a tort");
    p.frontier_cap = 4;
    if (choisir_action(L, F, true, true, p, g_mu) != Action::kSplitOneEndpoint)
      return sortie(3, "fixture frontiere saturee : un temoin est scinde au-dela du cap");
    if (choisir_action(L, F, true, false, p, g_mu) != Action::kPending)
      return sortie(3, "fixture sans recours : la continuation n'est pas rendue");
    if (fate_apres_action(Action::kPending, L, g_mu) != CoreFate::kPendingResource)
      return sortie(3, "fixture continuation : un verdict a ete rendu sur un cap");
    PolitiqueV0 v0 = p; v0.cap_sur_masse_seule = true; v0.exact_tile_cap = 64;
    PairFrame G; G.pair_mass = 1;
    if (choisir_action(L, G, true, false, v0, g_mu) != Action::kExactifyTile)
      return sortie(3, "fixture section 9 : le cap sur la masse seule devrait toujours exactifier");
    ++n;
  }
  {
    CoreDepthLedger L;
    L.reject_threshold = 10;
    L.sature(256, 100000, g_mu);
    if (!est_pruned(L, g_mu))
      return sortie(3, "fixture lower=256 : un bloc archi-mort est revenu a la vie");
    ++n;
  }
  // Le bucket est `floor(log2(pop))` : la frontiere entre deux buckets est une
  // puissance de deux exacte, donc elle se grave.
  if (bucket_de(1) != 0 || bucket_de(2) != 1 || bucket_de(3) != 1 ||
      bucket_de(4) != 2 || bucket_de(7) != 2 || bucket_de(8) != 3)
    return sortie(3, "fixture bucket : floor(log2) est faux a la puissance de deux");
  ++n;
  std::printf("abi fixtures gravees=%lld desaccords=0\n", n);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string mode, fam = "prefixe", heur = "buckets";
  int h = 10;
  unsigned seed = 1;
  std::uint64_t cap = 4096, front_cap = 64;
  long long budget = 1000000000;
  bool cap_sur_masse = false, octets = false;
  long long min_par_action = 1, min_grands = 1, min_ecart_travail = 0, facteur_scan = 0;
  std::uint32_t lot = 1;
  Planchers pl;
  g_nP = 64; g_nW = 16;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* pref) { return a.substr(std::strlen(pref)); };
    if (a == "--exhaustif" || a == "--saturation" || a == "--descente" ||
        a == "--politiques" || a == "--fixtures" || a == "--reprise-croisee")
      mode = a.substr(2);
    else if (a == "--reprise=octets") octets = true;
    else if (a == "--reprise=memoire") octets = false;
    else if (a == "--cap-sur=masse") cap_sur_masse = true;
    else if (a == "--cap-sur=tuile") cap_sur_masse = false;
    else if (a.rfind("--paires=", 0) == 0) g_nP = std::atoi(val("--paires=").c_str());
    else if (a.rfind("--temoins=", 0) == 0) g_nW = std::atoi(val("--temoins=").c_str());
    else if (a.rfind("--seuil=", 0) == 0) h = std::atoi(val("--seuil=").c_str());
    else if (a.rfind("--cap=", 0) == 0) cap = (std::uint64_t)std::atoll(val("--cap=").c_str());
    else if (a.rfind("--front-cap=", 0) == 0) front_cap = (std::uint64_t)std::atoll(val("--front-cap=").c_str());
    else if (a.rfind("--budget-temoins=", 0) == 0) budget = std::atoll(val("--budget-temoins=").c_str());
    else if (a.rfind("--famille=", 0) == 0) fam = val("--famille=");
    else if (a.rfind("--scission=", 0) == 0) heur = val("--scission=");
    else if (a.rfind("--seed=", 0) == 0) seed = (unsigned)std::atoi(val("--seed=").c_str());
    else if (a.rfind("--min-par-action=", 0) == 0) min_par_action = std::atoll(val("--min-par-action=").c_str());
    else if (a.rfind("--min-grands=", 0) == 0) min_grands = std::atoll(val("--min-grands=").c_str());
    else if (a.rfind("--min-pending=", 0) == 0) pl.min_pending = std::atoll(val("--min-pending=").c_str());
    else if (a.rfind("--min-reprises=", 0) == 0) pl.min_reprises = std::atoll(val("--min-reprises=").c_str());
    else if (a.rfind("--min-scissions=", 0) == 0) pl.min_scissions = std::atoll(val("--min-scissions=").c_str());
    else if (a.rfind("--min-tuiles=", 0) == 0) pl.min_tuiles = std::atoll(val("--min-tuiles=").c_str());
    else if (a.rfind("--min-morts=", 0) == 0) pl.min_morts = std::atoll(val("--min-morts=").c_str());
    else if (a.rfind("--min-vivantes=", 0) == 0) pl.min_vivantes = std::atoll(val("--min-vivantes=").c_str());
    else if (a.rfind("--min-pruned=", 0) == 0) pl.min_pruned = std::atoll(val("--min-pruned=").c_str());
    else if (a.rfind("--max-tuiles=", 0) == 0) pl.max_tuiles = std::atoll(val("--max-tuiles=").c_str());
    else if (a.rfind("--max-scan=", 0) == 0) pl.max_scan = std::atoll(val("--max-scan=").c_str());
    else if (a.rfind("--min-ecart-travail=", 0) == 0) min_ecart_travail = std::atoll(val("--min-ecart-travail=").c_str());
    else if (a.rfind("--facteur-scan=", 0) == 0) facteur_scan = std::atoll(val("--facteur-scan=").c_str());
    else if (a.rfind("--lot=", 0) == 0) lot = (std::uint32_t)std::atoll(val("--lot=").c_str());
    else if (a.rfind("--inject=", 0) == 0) {
      const std::string m = val("--inject=");
      g_inject = true;
      if (m == "sature-tronque") g_mu = PfMutant::kSatureTronque;
      else if (m == "clear-large") g_mu = PfMutant::kClearLarge;
      else if (m == "pruned-large") g_mu = PfMutant::kPrunedLarge;
      else if (m == "pending-pruned") g_mu = PfMutant::kPendingPruned;
      else if (m == "cap-avant-verdict") g_mu = PfMutant::kCapAvantVerdict;
      else if (m == "upper-sature-incremental") g_mu = PfMutant::kUpperSatureIncr;
      else return refus("mutant inconnu");
    } else return refus("argument inconnu");
  }

  if (mode.empty())
    return refus("aucun mode : --exhaustif --saturation --descente --politiques"
                 " --fixtures --reprise-croisee");
  if (lot > 4096) return refus("lot de temoins hors de [0,4096]");
  if (h < 1 || h > 60) return refus("seuil hors de [1,60] : h_q=0 tuerait tout par construction");
  if (cap < 1) return refus("cap nul : aucune tuile exacte ne pourrait terminer la descente");
  if (front_cap < 1) return refus("front_cap nul : la frontiere ne pourrait jamais exister");

  if (mode == "exhaustif") return mode_exhaustif(min_par_action);
  if (mode == "saturation") return mode_saturation(min_grands);

  if (g_nP < 2 || g_nP > 4096) return refus("paires hors de [2,4096]");
  if (g_nW < 2 || g_nW > 256) return refus("temoins hors de [2,256]");
  if ((g_nW & (g_nW - 1)) != 0) return refus("temoins doit etre une puissance de deux");
  if (h > g_nW) return refus("seuil au-dessus du nombre de temoins : tout serait vivant");
  if (!remplir(fam, seed)) return refus("famille inconnue");
  if (heur != "buckets" && heur != "plus-gros" && heur != "plus-petit" &&
      heur != "premier" && heur != "feuilles")
    return refus("heuristique de scission inconnue");
  g_arbre.clear();
  construire(0, g_nW);

  if (mode == "fixtures") return mode_fixtures();
  if (mode == "politiques") return mode_politiques(h, min_ecart_travail, facteur_scan);
  if (mode == "reprise-croisee") return mode_reprise_croisee(h);
  return mode_descente(h, cap, front_cap, budget, cap_sur_masse, octets, heur, pl, lot);
}
