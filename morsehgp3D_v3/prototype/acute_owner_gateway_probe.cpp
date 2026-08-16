// Microprobe autonome du gateway `A x B x C`, avec oracle exhaustif.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed. GCP non utilise.
//
// CODES DE SORTIE : 1 desaccord du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
//
// ---------------------------------------------------------------------------
// DEUX CHOSES SONT MESUREES ICI, ET IL NE FAUT PAS LES CONFONDRE
//
// 1. LA SURETE DU CLASSIFIEUR. Sur de petites boites, on enumere TOUS les
//    triplets entiers et on confronte le verdict a la verite. `DEAD_*` doit
//    n'avoir aucun porteur, `ALL_STRICT` doit n'avoir que des porteurs, et
//    `MIXED` n'est jamais faux — c'est l'aveu d'ignorance.
//
// 2. LA SPARSITE. Sur un vrai nuage, la recursion ternaire ne doit expanser
//    AUCUNE paire sur `two_lines`. Le compteur `pairid_expanded` compte les
//    couples `(a,b)` reellement materialises ; il doit valoir zero.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/acute_owner_gateway.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/wspd_wavefront.hpp"

using namespace mhgp3v::acute;
using mhgp3v::WfNode;

namespace {

struct P3 {
  short x, y, z;
};

// Le predicat POINT, avec owner canonique — la verite que le gateway
// sur-approche. `pa < pb` est impose par l'appelant pour qu'un triangle ne
// compte pas deux fois sous la meme arete lue dans les deux sens.
bool porteur_canonique(const P3& a, const P3& b, const P3& x, int pa, int pb, int px) {
  const i64 ex = x.x - a.x, ey = x.y - a.y, ez = x.z - a.z;
  const i64 tx = b.x - x.x, ty = b.y - x.y, tz = b.z - x.z;
  const i64 E = ex * ex + ey * ey + ez * ez;
  const i64 X = tx * tx + ty * ty + tz * tz;
  if (E == 0 || X == 0) return false;
  const i64 dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
  const i64 D = dx * dx + dy * dy + dz * dz;
  if (D == 0) return false;
  if (E > D || X > D) return false;
  auto cle_moins = [](int u1, int v1, int u2, int v2) {
    const int a1 = u1 < v1 ? u1 : v1, b1 = u1 < v1 ? v1 : u1;
    const int a2 = u2 < v2 ? u2 : v2, b2 = u2 < v2 ? v2 : u2;
    return a1 != a2 ? a1 < a2 : b1 < b2;
  };
  if (E == D && !cle_moins(pa, pb, pa, px)) return false;
  if (X == D && !cle_moins(pa, pb, pb, px)) return false;
  return ex * tx + ey * ty + ez * tz < 0;  // `H < 0`, STRICTE
}

// ---------------------------------------------------------------------------
// PARTIE 1 : L'ORACLE EXHAUSTIF SUR PETITES BOITES.
//
// On n'y applique PAS l'owner canonique : sur des boites, le gateway ne connait
// aucun `PointId`. La verite comparee est donc l'owner MAXIMAL FAIBLE, et
// `ALL_STRICT` exige les `Delta` strictement positifs precisement pour que les
// deux coincident sur les blocs qu'il certifie.
bool porteur_faible(const i64 a[3], const i64 b[3], const i64 x[3]) {
  i64 D = 0, E = 0, X = 0, H = 0;
  for (int k = 0; k < 3; ++k) {
    D += (a[k] - b[k]) * (a[k] - b[k]);
    E += (a[k] - x[k]) * (a[k] - x[k]);
    X += (b[k] - x[k]) * (b[k] - x[k]);
    H += (x[k] - a[k]) * (b[k] - x[k]);
  }
  if (D == 0 || E == 0 || X == 0) return false;
  return E <= D && X <= D && H < 0;
}

unsigned long long rng_etat = 0x9E3779B97F4A7C15ULL;
unsigned long long rng() {
  rng_etat += 0x9E3779B97F4A7C15ULL;
  unsigned long long z = rng_etat;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// ---- BOITES BIAISEES, POUR QUE `ALL_STRICT` SOIT REELLEMENT ATTEINT.
//
// Des boites aleatoires ne satisfont presque jamais les trois conditions
// strictes a la fois : l'oracle non biaise rend `all_strict = 0`, donc la
// branche n'est pas testee et le mutant `all-strict-lache` survit par VACUITE.
// C'est exactement le vert-par-vacuite que le protocole interdit.
//
// On fabrique donc la configuration qui la declenche : `A` autour de l'origine,
// `B` autour de `(L,0,0)`, et `C` autour de `(L/2, h, 0)` avec
// `L/2 < h < L sqrt(3)/2` — au-dessus de la boule diametrale, sous la lentille.
// Avec `L = 20`, `h = 14` tient largement, et une gigue de +/-1 laisse les
// inegalites strictes.
BoxI boite_biaisee(int quoi) {
  const i64 L = 20, h = 14;
  const i64 c[3][3] = {{0, 0, 0}, {L, 0, 0}, {L / 2, h, 0}};
  BoxI B{};
  for (int k = 0; k < 3; ++k) {
    const i64 j = (i64)(rng() % 3) - 1;  // gigue dans [-1,1]
    B.ax[k].lo = c[quoi][k] + j;
    B.ax[k].hi = c[quoi][k] + j + (i64)(rng() % 2);
  }
  return B;
}

BoxI boite_alea(int etendue) {
  BoxI B{};
  for (int k = 0; k < 3; ++k) {
    const i64 u = (i64)(rng() % (unsigned)(2 * etendue + 1)) - etendue;
    const i64 v = (i64)(rng() % (unsigned)(2 * etendue + 1)) - etendue;
    B.ax[k].lo = u < v ? u : v;
    B.ax[k].hi = u < v ? v : u;
  }
  return B;
}

struct BilanOracle {
  long long blocs = 0;
  long long dead = 0, all_strict = 0, mixed = 0;
  long long dead_faux = 0;        // un `DEAD` qui contenait un porteur : FATAL
  long long all_faux = 0;         // un `ALL_STRICT` qui contenait un non-porteur
  long long mixed_tout_porteur = 0;   // conservatisme mesure, pas une faute
  long long mixed_aucun_porteur = 0;
};

BilanOracle oracle(int tours, int etendue, GwMutant mu, bool biais) {
  BilanOracle g;
  for (int t = 0; t < tours; ++t) {
    const BoxI A = biais ? boite_biaisee(0) : boite_alea(etendue);
    const BoxI B = biais ? boite_biaisee(1) : boite_alea(etendue);
    const BoxI C = biais ? boite_biaisee(2) : boite_alea(etendue);
    const Verdict v = classifie(extrema(A, B, C), mu);
    ++g.blocs;
    long long porteurs = 0, total = 0;
    for (i64 a0 = A.ax[0].lo; a0 <= A.ax[0].hi; ++a0)
    for (i64 a1 = A.ax[1].lo; a1 <= A.ax[1].hi; ++a1)
    for (i64 a2 = A.ax[2].lo; a2 <= A.ax[2].hi; ++a2)
    for (i64 b0 = B.ax[0].lo; b0 <= B.ax[0].hi; ++b0)
    for (i64 b1 = B.ax[1].lo; b1 <= B.ax[1].hi; ++b1)
    for (i64 b2 = B.ax[2].lo; b2 <= B.ax[2].hi; ++b2)
    for (i64 c0 = C.ax[0].lo; c0 <= C.ax[0].hi; ++c0)
    for (i64 c1 = C.ax[1].lo; c1 <= C.ax[1].hi; ++c1)
    for (i64 c2 = C.ax[2].lo; c2 <= C.ax[2].hi; ++c2) {
      const i64 a[3] = {a0, a1, a2}, b[3] = {b0, b1, b2}, x[3] = {c0, c1, c2};
      ++total;
      if (porteur_faible(a, b, x)) ++porteurs;
    }
    if (v == Verdict::kDeadPhi || v == Verdict::kDeadOwnerE || v == Verdict::kDeadOwnerX) {
      ++g.dead;
      if (porteurs > 0) ++g.dead_faux;
    } else if (v == Verdict::kAllStrict) {
      ++g.all_strict;
      if (porteurs != total) ++g.all_faux;
    } else {
      ++g.mixed;
      if (porteurs == total && total > 0) ++g.mixed_tout_porteur;
      if (porteurs == 0) ++g.mixed_aucun_porteur;
    }
  }
  return g;
}

// ---------------------------------------------------------------------------
// PARTIE 2 : LA RECURSION TERNAIRE SUR UN VRAI NUAGE.
struct BilanSparse {
  long long noeuds = 0;             // triplets `(A,B,C)` classes
  long long dead_phi = 0, dead_e = 0, dead_x = 0;
  long long all_strict = 0;         // CarrierBlocks SYMBOLIQUES
  long long masse_all_strict = 0;   // leur masse logique, jamais materialisee
  long long feuilles = 0;           // triplets de feuilles testes point a point
  long long pairid_expanded = 0;    // paires `(a,b)` reellement materialisees
  long long carriers = 0;           // porteurs sous owner canonique
  long long carriers_symboliques = 0;
  long long blocs_faux = 0;  // triples d'un `ALL_STRICT` qui ne portent pas
  // ---- LEDGER CONJOINT (audit `1d9425d`, section 3).
  long long dead_w4 = 0;            // blocs morts par vivacite, avant tout aigu
  long long active_edge = 0;        // `ActiveOwnerEdgeBlock` emis
  long long seed3_emitted = 0;      // seeds ternaires reellement materialises
  long long pending = 0;            // debordements : jamais `DEAD`
  long long l4_credits = 0;         // credits `W_4` acquis en bloc
  long long frontiere_max = 0;      // plus grande frontiere indecise portee
};

}  // namespace

int main(int argc, char** argv) {
  int n = 400, tours = 4000, etendue = 2, min_dead = 0, min_all = 0;
  long long seed = 12345;
  std::string famille = "two_lines";
  bool mode_oracle = false, mode_sparse = false, mode_brute = false, biais = false;
  long long max_pairid = -1, min_carriers = -1, min_noeuds = 0;
  int r4 = 8;  // seuil de rejet q4, `h_4 = s_max - 3`
  GwMutant mu = GwMutant::kNone;

  auto entier = [&](const std::string& a, const char* cle, long long* out) {
    const size_t L = std::strlen(cle);
    if (a.rfind(cle, 0) != 0) return false;
    const std::string v = a.substr(L);
    long long r = 0;
    const auto res = std::from_chars(v.data(), v.data() + v.size(), r);
    if (res.ec != std::errc() || res.ptr != v.data() + v.size()) {
      std::fprintf(stderr, "REFUS : valeur invalide pour %s\n", cle);
      std::exit(2);
    }
    *out = r;
    return true;
  };

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    long long t = 0;
    if (a == "--oracle") { mode_oracle = true; continue; }
    if (a == "--oracle-biais") { mode_oracle = true; biais = true; continue; }
    if (a == "--sparse") { mode_sparse = true; continue; }
    if (a == "--brute") { mode_sparse = true; mode_brute = true; continue; }
    if (entier(a, "--points=", &t)) { n = (int)t; continue; }
    if (entier(a, "--tours=", &t)) { tours = (int)t; continue; }
    if (entier(a, "--etendue=", &t)) { etendue = (int)t; continue; }
    if (entier(a, "--seed=", &t)) { seed = t; continue; }
    if (entier(a, "--min-dead=", &t)) { min_dead = (int)t; continue; }
    if (entier(a, "--min-all-strict=", &t)) { min_all = (int)t; continue; }
    if (entier(a, "--max-pairid=", &t)) { max_pairid = t; continue; }
    if (entier(a, "--min-carriers=", &t)) { min_carriers = t; continue; }
    if (entier(a, "--min-noeuds=", &t)) { min_noeuds = t; continue; }
    if (entier(a, "--r4=", &t)) { r4 = (int)t; continue; }
    if (a.rfind("--family=", 0) == 0) { famille = a.substr(9); continue; }
    if (a.rfind("--inject=", 0) == 0) {
      const std::string m = a.substr(9);
      if (m == "phi-large") mu = GwMutant::kPhiLarge;
      else if (m == "dead-large") mu = GwMutant::kDeadLarge;
      else if (m == "all-strict-lache") mu = GwMutant::kAllStrictLache;
      else { std::fprintf(stderr, "REFUS : mutant inconnu %s\n", m.c_str()); return 2; }
      continue;
    }
    std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
    return 2;
  }
  if (!mode_oracle && !mode_sparse) {
    std::fprintf(stderr, "REFUS : choisir --oracle ou --sparse\n");
    return 2;
  }
  // L'oracle est CUBIQUE en la masse des boites : `(2e+1)^9` triplets au pire.
  // Une etendue de 3 donne deja 40 353 607 triplets par bloc.
  if (etendue < 0 || etendue > 3) {
    std::fprintf(stderr, "REFUS : --etendue hors [0,3], l'oracle serait non borne\n");
    return 2;
  }
  if (tours < 1 || tours > 200000) {
    std::fprintf(stderr, "REFUS : --tours hors [1,200000]\n");
    return 2;
  }
  if (n < 4 || n > 65535) {
    std::fprintf(stderr, "REFUS : n hors profil u16\n");
    return 2;
  }
  // La force brute est en `C(n,3) x n` ; elle est bornee explicitement.
  if (mode_brute && n > 400) {
    std::fprintf(stderr, "REFUS : --brute borne a 400 points, n=%d\n", n);
    return 2;
  }
  if (mu != GwMutant::kNone && !mode_oracle && !mode_sparse) {
    std::fprintf(stderr, "REFUS : un mutant sans juge ne prouve rien\n");
    return 2;
  }

  std::printf("AcuteOwnerGatewayReceipt-v1\n");
  std::printf("cadre phase=exploration_v3_hors_registre backend=cpu_reference "
              "profile=quantized_u16_input_only mode=diagnostic_counter_only "
              "public_status=not_claimed\n");

  if (mode_oracle) {
    rng_etat = (unsigned long long)seed * 0x9E3779B97F4A7C15ULL + 1;
    const BilanOracle g = oracle(tours, etendue, mu, biais);
    std::printf("oracle blocs=%lld dead=%lld all_strict=%lld mixed=%lld "
                "dead_faux=%lld all_faux=%lld mixed_tout_porteur=%lld "
                "mixed_aucun_porteur=%lld\n",
                g.blocs, g.dead, g.all_strict, g.mixed, g.dead_faux, g.all_faux,
                g.mixed_tout_porteur, g.mixed_aucun_porteur);
    // ---- LA SURETE EST LA SEULE CHOSE NON NEGOCIABLE.
    if (g.dead_faux > 0 || g.all_faux > 0) {
      if (mu != GwMutant::kNone) {
        std::fprintf(stderr, "MUTANT TUE : %s produit %lld DEAD faux et %lld "
                     "ALL_STRICT faux\n", mu == GwMutant::kPhiLarge ? "phi-large"
                     : (mu == GwMutant::kDeadLarge ? "dead-large" : "all-strict-lache"),
                     g.dead_faux, g.all_faux);
        return 4;
      }
      std::fprintf(stderr, "DESACCORD DU JUGE : %lld DEAD faux, %lld ALL_STRICT faux\n",
                   g.dead_faux, g.all_faux);
      return 1;
    }
    // ---- DEUX ESPECES DE MUTANTS, ET IL NE FAUT PAS LES CONFONDRE.
    //
    // `dead-large` et `all-strict-lache` rendent le classifieur FAUX : ils se
    // tuent par `dead_faux` ou `all_faux`, ci-dessus.
    //
    // `phi-large` ne ment pas — il rend `MIXED` la ou `DEAD` etait justifie,
    // donc il reste SUR et seulement plus cher. Le declarer « survivant »
    // serait une erreur de categorie. Il se tue par PERTE DE COUVERTURE : le
    // nombre de blocs certifies morts doit chuter par rapport au plancher.
    if (mu == GwMutant::kPhiLarge) {
      if (g.dead >= min_dead && min_dead > 0) {
        std::fprintf(stderr, "MUTANT SURVIVANT : phi-large garde %lld DEAD, plancher %d\n",
                     g.dead, min_dead);
        return 3;
      }
      std::fprintf(stderr, "MUTANT TUE : phi-large tombe a %lld DEAD sous le plancher %d\n",
                   g.dead, min_dead);
      return 4;
    }
    if (mu != GwMutant::kNone) {
      std::fprintf(stderr, "MUTANT SURVIVANT : il n'a pas ete vu\n");
      return 3;
    }
    // ---- LES PLANCHERS, contre le vert par vacuite.
    if (g.dead < min_dead || g.all_strict < min_all) {
      std::fprintf(stderr, "PLANCHER : dead=%lld < %d ou all_strict=%lld < %d\n",
                   g.dead, min_dead, g.all_strict, min_all);
      return 3;
    }
    std::printf("oracle verdict=SUR planchers=OK\n");
  }

  if (mode_sparse) {
    const int coord = mhgp3v::cloud_family_default_coord(
        famille == "two_lines" ? mhgp3v::CloudFamily::kTwoLines
        : famille == "terrain" ? mhgp3v::CloudFamily::kTerrain
        : famille == "eight_clusters" ? mhgp3v::CloudFamily::kEightClusters
                                      : mhgp3v::CloudFamily::kUniform, n);
    const mhgp3v::CloudFamily fam =
        famille == "two_lines" ? mhgp3v::CloudFamily::kTwoLines
        : famille == "terrain" ? mhgp3v::CloudFamily::kTerrain
        : famille == "eight_clusters" ? mhgp3v::CloudFamily::kEightClusters
        : famille == "uniform" ? mhgp3v::CloudFamily::kUniform
                               : mhgp3v::CloudFamily::kUniform;
    if (famille != "two_lines" && famille != "terrain" && famille != "uniform" &&
        famille != "eight_clusters") {
      std::fprintf(stderr, "REFUS : famille inconnue %s\n", famille.c_str());
      return 2;
    }
    const auto brut = mhgp3v::make_family_cloud(fam, n, coord, seed);
    const int m = (int)brut.size();
    if (m != n) {
      std::fprintf(stderr, "REFUS : le generateur a rendu %d points pour --points=%d\n", m, n);
      return 2;
    }
    std::vector<unsigned long long> keys(m);
    std::vector<int> order(m);
    for (int i = 0; i < m; ++i) {
      keys[i] = mhgp3v::wf_morton48(brut[i].x, brut[i].y, brut[i].z);
      order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](int i, int j) {
      return keys[i] != keys[j] ? keys[i] < keys[j] : i < j;
    });
    std::vector<unsigned long long> sk(m);
    std::vector<P3> pts(m);
    std::vector<int> pid(m);
    for (int i = 0; i < m; ++i) {
      sk[i] = keys[order[i]];
      pts[i] = P3{(short)brut[order[i]].x, (short)brut[order[i]].y, (short)brut[order[i]].z};
      pid[i] = order[i];
    }
    std::vector<WfNode> nodes = mhgp3v::wf_build(sk);
    {
      std::vector<std::array<long long, 3>> raw(m);
      for (int i = 0; i < m; ++i) raw[i] = {pts[i].x, pts[i].y, pts[i].z};
      mhgp3v::wf_tight_boxes(&nodes, raw);
    }
    auto premier = [&](int h) { return h >= 0 ? nodes[h].first : -1 - h; };
    auto dernier = [&](int h) { return h >= 0 ? nodes[h].last : -1 - h; };
    auto pop = [&](int h) { return dernier(h) - premier(h) + 1; };
    auto feuille = [&](int h) { return h < 0; };
    auto boite = [&](int h) {
      BoxI B{};
      if (h >= 0) {
        // `tlo/thi` EST LA BOITE SERREE. `lo/hi` est la CELLULE de Morton,
        // alignee sur un prefixe de cle — beaucoup plus large, et surtout
        // FAUSSE des que le nuage sort du profil u16, puisque l'encodage
        // suppose des coordonnees positives sur seize bits. Ma premiere version
        // lisait `lo/hi`, et `two_lines` — qui produit `z = -1` — rendait alors
        // deux blocs `ALL_STRICT` dont AUCUN des quatre triples n'etait porteur.
        for (int k = 0; k < 3; ++k) { B.ax[k].lo = nodes[h].tlo[k]; B.ax[k].hi = nodes[h].thi[k]; }
      } else {
        const int i = -1 - h;
        B.ax[0] = {pts[i].x, pts[i].x};
        B.ax[1] = {pts[i].y, pts[i].y};
        B.ax[2] = {pts[i].z, pts[i].z};
      }
      return B;
    };

    // ---- LA RECURSION TERNAIRE.
    //
    // On descend sur les TROIS boites. Un bloc mort n'expanse rien ; un bloc
    // `ALL_STRICT` est emis SYMBOLIQUEMENT — sa masse est comptee, ses triplets
    // ne sont jamais formes ; seuls les blocs indecis descendent.
    //
    // `pairid_expanded` ne s'incremente qu'au niveau des FEUILLES, c'est-a-dire
    // au seul endroit ou une paire `(a,b)` existe reellement en memoire. C'est
    // le compteur que `two_lines` doit laisser a zero.
    BilanSparse g;
    // ---- LA TACHE PORTE SON LEDGER, ET C'EST CE QUI EVITE DE TOUT RECALCULER.
    //
    // `L4` est le nombre d'IDs universellement `W_4`-interieurs pour TOUTE paire
    // du rectangle `A x B`. Raffiner `A` ou `B` AFFAIBLIT le « pour toute
    // paire », donc `L4` ne peut que CROITRE : le credit acquis est definitif et
    // s'herite. Seule la FRONTIERE indecise se re-teste chez les enfants.
    //
    // `U4 = L4 + masse de la frontiere` majore le compte vrai. D'ou les deux
    // seuils de la section 3.2 de l'audit :
    //
    //   `L4 >= r4`  -> toutes les paires du bloc sont mortes ;
    //   `U4 <  r4`  -> toutes les paires du bloc sont `W_4`-vivantes.
    //
    // La frontiere est une liste de handles de nœuds, jamais de points : c'est
    // elle qui remplace le CSR de sites par seed que l'audit refuse.
    struct Tache {
      int A, B, C;
      long long L4;
      std::vector<int> frontiere;
    };
    std::vector<Tache> st;
    const int racine = nodes.empty() ? -1 : 0;
    {
      Tache t0{racine, racine, racine, 0, {}};
      t0.frontiere.push_back(racine);
      st.push_back(std::move(t0));
    }
    // Cap de frontiere : un depassement rend `PENDING`, jamais `DEAD`.
    const int kCapFrontiere = 64;
    // ---- L'AUTO-JOINTURE EN PAS CADENCE, ET POURQUOI ELLE EST NECESSAIRE.
    //
    // Partir de `(racine, racine, racine)` sans precaution fait visiter chaque
    // paire de blocs DEUX FOIS — `(A,B,C)` et `(B,A,C)`. Le juge l'a vu tout de
    // suite : `sparse` valait exactement `2 x brute` sur les trois familles
    // denses. Au niveau des feuilles l'orientation `pid` reglait le cas, mais
    // les blocs `ALL_STRICT` comptaient leur masse deux fois.
    //
    // J'ai d'abord essaye d'elaguer la moitie mal ordonnee. C'etait FAUX, et le
    // juge l'a vu aussi : on passe alors de `+2 x` a `-13 %`, parce qu'un nœud
    // peut etre l'ANCETRE de l'autre, et couper sur `premier(A) > premier(B)`
    // supprime des descendants legitimes.
    //
    // La regle correcte maintient l'INVARIANT « `A == B`, ou `A` et `B`
    // disjoints avec `A` avant `B` ». Les nœuds d'un octree etant laminaires,
    // scinder `A == B` en TROIS — `(A.g,A.g)`, `(A.g,A.d)`, `(A.d,A.d)`, la
    // diagonale inverse etant omise — le preserve, et chaque paire non ordonnee
    // n'est alors visitee qu'une fois.
    while (!st.empty()) {
      const Tache t = std::move(st.back());
      st.pop_back();
      ++g.noeuds;
      const BoxI BA = boite(t.A), BB = boite(t.B), BC = boite(t.C);

      // ---- PASSE LEDGER : on rafraichit la frontiere pour CE rectangle.
      long long L4 = t.L4;
      std::vector<int> frontiere;
      {
        std::vector<int> pile = t.frontiere;
        while (!pile.empty() && (int)frontiere.size() <= kCapFrontiere) {
          const int h = pile.back();
          pile.pop_back();
          const int pf = premier(h), pl = dernier(h);
          // Disjonction : un point de `A` ou de `B` n'est jamais temoin du cœur.
          if (!(pl < premier(t.A) || pf > dernier(t.A))) {
            if (!feuille(h)) { pile.push_back(nodes[h].left); pile.push_back(nodes[h].right); }
            continue;
          }
          if (!(pl < premier(t.B) || pf > dernier(t.B))) {
            if (!feuille(h)) { pile.push_back(nodes[h].left); pile.push_back(nodes[h].right); }
            continue;
          }
          const BoxI BZ = boite(h);
          if (bloc_tout_w4(BA, BB, BZ, &g.noeuds)) { L4 += pl - pf + 1; ++g.l4_credits; continue; }
          if (bloc_aucun_w2(extrema(BA, BB, BZ))) continue;  // hors `W_2`, donc hors `W_4`
          if (feuille(h)) { frontiere.push_back(h); continue; }
          pile.push_back(nodes[h].left);
          pile.push_back(nodes[h].right);
        }
        if (!pile.empty()) {
          // Frontiere trop large : on rend la tache PENDING plutot que de
          // decider a tort. `PENDING` n'est jamais `DEAD` — c'est la regle.
          ++g.pending;
          continue;
        }
      }
      if ((long long)frontiere.size() > g.frontiere_max)
        g.frontiere_max = (long long)frontiere.size();
      const long long U4 = L4 + (long long)frontiere.size();

      const Extrema ex = extrema(BA, BB, BC);
      const VerdictConjoint vc = classifie_conjoint(ex, L4, U4, r4, mu);
      if (vc == VerdictConjoint::kDeadW4) { ++g.dead_w4; continue; }
      const Verdict v = classifie(ex, mu);
      if (v == Verdict::kDeadPhi) { ++g.dead_phi; continue; }
      if (v == Verdict::kDeadOwnerE) { ++g.dead_e; continue; }
      if (v == Verdict::kDeadOwnerX) { ++g.dead_x; continue; }
      if (vc == VerdictConjoint::kActiveAll) {
        // `ActiveOwnerEdgeBlock` : ni `PairId`, ni face materialisee.
        ++g.active_edge;
      }
      if (v == Verdict::kAllStrict) {
        // CARRIER BLOCK SYMBOLIQUE : on retient `(A,B,C)`, pas ses triplets.
        ++g.all_strict;
        // La masse compte des paires NON ORDONNEES : `A == B` donne
        // `C(|A|,2)`, pas `|A|^2`.
        const long long masse =
            (t.A == t.B) ? (long long)pop(t.A) * (pop(t.A) - 1) / 2 * pop(t.C)
                         : (long long)pop(t.A) * pop(t.B) * pop(t.C);
        g.masse_all_strict += masse;
        g.carriers_symboliques += masse;
        if (mode_brute) {
          // VERIFICATION DU BLOC, point par point. Elle ne sert qu'au juge : un
          // bloc `ALL_STRICT` doit etre integralement porteur, et un ecart ici
          // LOCALISE la faute bien mieux qu'un total qui ne tombe pas.
          for (int ia = premier(t.A); ia <= dernier(t.A); ++ia)
            for (int ib = premier(t.B); ib <= dernier(t.B); ++ib) {
              if (t.A == t.B && ia >= ib) continue;  // une seule fois par paire
              for (int ic = premier(t.C); ic <= dernier(t.C); ++ic) {
                const bool ok =
                    pid[ia] < pid[ib]
                        ? porteur_canonique(pts[ia], pts[ib], pts[ic], pid[ia], pid[ib], pid[ic])
                        : porteur_canonique(pts[ib], pts[ia], pts[ic], pid[ib], pid[ia], pid[ic]);
                if (!ok) {
                  ++g.blocs_faux;
                  if (g.blocs_faux <= 4)
                    std::fprintf(stderr,
                        "BLOC FAUX a=(%d,%d,%d)#%d b=(%d,%d,%d)#%d x=(%d,%d,%d)#%d\n",
                        pts[ia].x, pts[ia].y, pts[ia].z, pid[ia],
                        pts[ib].x, pts[ib].y, pts[ib].z, pid[ib],
                        pts[ic].x, pts[ic].y, pts[ic].z, pid[ic]);
                }
              }
            }
        }
        continue;
      }
      const bool fa = feuille(t.A), fb = feuille(t.B), fc = feuille(t.C);
      if (fa && fb && fc) {
        ++g.feuilles;
        const int ia = premier(t.A), ib = premier(t.B), ic = premier(t.C);
        if (ia == ib || ia == ic || ib == ic) continue;
        // PAS DE SECOND FILTRE D'ORIENTATION ICI. L'invariant du pas cadence
        // garantit deja que la paire non ordonnee `{A,B}` n'est visitee qu'une
        // fois. J'avais laisse un `pid[ia] >= pid[ib] -> continue` en plus, et
        // il coupait les paires dont l'ordre Morton et l'ordre `pid` divergent :
        // le juge a lu `-196`, `-157` et `-45` porteurs manquants. On ORDONNE
        // l'appel au lieu de filtrer.
        ++g.pairid_expanded;
        ++g.seed3_emitted;
        const bool porte =
            pid[ia] < pid[ib]
                ? porteur_canonique(pts[ia], pts[ib], pts[ic], pid[ia], pid[ib], pid[ic])
                : porteur_canonique(pts[ib], pts[ia], pts[ic], pid[ib], pid[ia], pid[ic]);
        if (porte) ++g.carriers;
        continue;
      }
      // On coupe la PLUS GROSSE des trois, ce qui fait decroitre le produit des
      // populations et garantit la terminaison.
      const int pa = fa ? 1 : pop(t.A), pb = fb ? 1 : pop(t.B), pc = fc ? 1 : pop(t.C);
      if (t.A == t.B && !fa) {
        // LA DIAGONALE, DEPLIEE EN TROIS. `(A.d, A.g)` est omise : c'est la
        // meme paire non ordonnee que `(A.g, A.d)`.
        const int g1 = nodes[t.A].left, d1 = nodes[t.A].right;
        st.push_back({g1, g1, t.C, L4, frontiere});
        st.push_back({g1, d1, t.C, L4, frontiere});
        st.push_back({d1, d1, t.C, L4, frontiere});
      } else if (t.A == t.B) {
        // `A == B` feuille : la seule paire possible est `(a,a)`, degeneree.
        // Il ne reste qu'a descendre `C`, ou a s'arreter.
        if (!fc) {
          st.push_back({t.A, t.B, nodes[t.C].left, L4, frontiere});
          st.push_back({t.A, t.B, nodes[t.C].right, L4, frontiere});
        }
      } else if (!fa && pa >= pb && pa >= pc) {
        st.push_back({nodes[t.A].left, t.B, t.C, L4, frontiere});
        st.push_back({nodes[t.A].right, t.B, t.C, L4, frontiere});
      } else if (!fb && pb >= pc) {
        st.push_back({t.A, nodes[t.B].left, t.C, L4, frontiere});
        st.push_back({t.A, nodes[t.B].right, t.C, L4, frontiere});
      } else if (!fc) {
        st.push_back({t.A, t.B, nodes[t.C].left, L4, frontiere});
        st.push_back({t.A, t.B, nodes[t.C].right, L4, frontiere});
      }
    }
    // ---- L'ORACLE AU NIVEAU NUAGE, qui juge la recursion entiere.
    //
    // La sûrete du classifieur est etablie sur des boites ; elle ne dit rien de
    // la RECURSION — un bug de descente, de coupe ou de double comptage y
    // echapperait entierement. On enumere donc tous les triples, une fois, sous
    // owner canonique, et on exige l'egalite A L'UNITE avec
    // `carriers + carriers_symboliques`.
    //
    // Coût `C(n,3)`, donc borne a petit `n` : c'est un juge, pas un chemin.
    long long ref_brute = -1;
    if (mode_brute) {
      ref_brute = 0;
      for (int i = 0; i < m; ++i)
        for (int j = i + 1; j < m; ++j)
          for (int k = 0; k < m; ++k) {
            if (k == i || k == j) continue;
            // Meme convention d'orientation que les feuilles : `pid` croissant.
            if (pid[i] < pid[j]) {
              if (porteur_canonique(pts[i], pts[j], pts[k], pid[i], pid[j], pid[k])) ++ref_brute;
            } else {
              if (porteur_canonique(pts[j], pts[i], pts[k], pid[j], pid[i], pid[k])) ++ref_brute;
            }
          }
    }
    std::printf("sparse famille=%s n=%d noeuds=%lld dead_phi=%lld dead_owner_e=%lld "
                "dead_owner_x=%lld all_strict=%lld masse_all_strict=%lld "
                "feuilles=%lld pairid_expanded=%lld carriers=%lld "
                "carriers_symboliques=%lld blocs_faux=%lld dead_w4=%lld active_edge=%lld "
                "seed3_emitted=%lld pending=%lld l4_credits=%lld frontiere_max=%lld\n",
                famille.c_str(), n, g.noeuds, g.dead_phi, g.dead_e, g.dead_x,
                g.all_strict, g.masse_all_strict, g.feuilles, g.pairid_expanded,
                g.carriers, g.carriers_symboliques, g.blocs_faux, g.dead_w4,
                g.active_edge, g.seed3_emitted, g.pending, g.l4_credits,
                g.frontiere_max);
    if (mode_brute) {
      const long long total = g.carriers + g.carriers_symboliques;
      std::printf("juge brute=%lld sparse=%lld ecart=%lld\n", ref_brute, total, total - ref_brute);
      if (total != ref_brute || g.blocs_faux > 0) {
        // CONVENTION v3 : un ecart SOUS MUTANT est un mutant tue (4), un ecart
        // sans mutant est un desaccord du juge (1).
        if (mu != GwMutant::kNone) {
          std::fprintf(stderr, "MUTANT TUE : ecart %lld, blocs_faux %lld\n",
                       total - ref_brute, g.blocs_faux);
          return 4;
        }
        std::fprintf(stderr, "DESACCORD DU JUGE : recursion %lld contre force brute %lld,"
                     " blocs_faux %lld\n", total, ref_brute, g.blocs_faux);
        return 1;
      }
      if (mu != GwMutant::kNone && mu != GwMutant::kPhiLarge) {
        std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a pas ete vu\n",
                     mu == GwMutant::kDeadLarge ? "dead-large" : "all-strict-lache");
        return 3;
      }
      if (ref_brute == 0 && min_carriers > 0) {
        std::fprintf(stderr, "PLANCHER : la force brute ne trouve aucun porteur\n");
        return 3;
      }
    }
    if (max_pairid >= 0 && g.pairid_expanded > max_pairid) {
      std::fprintf(stderr, "PLANCHER : pairid_expanded=%lld depasse %lld\n",
                   g.pairid_expanded, max_pairid);
      return 3;
    }
    if (min_carriers >= 0 && g.carriers + g.carriers_symboliques < min_carriers) {
      std::fprintf(stderr, "PLANCHER : carriers=%lld sous %lld\n",
                   g.carriers + g.carriers_symboliques, min_carriers);
      return 3;
    }
    if (g.noeuds < min_noeuds) {
      std::fprintf(stderr, "PLANCHER : noeuds=%lld sous %lld\n", g.noeuds, min_noeuds);
      return 3;
    }
  }
  std::printf("OK : gateway aigu mesure\n");
  return 0;
}
