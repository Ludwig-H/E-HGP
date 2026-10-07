// Audit L07 (2 octobre 2026) : sondes d'API de la tete publiee (libmhgp10_core.a de afb081774), sans geometrie.
// Chaque bloc imprime ce que rend mhgp10::validate / mhgp10::cluster et, a cote, la valeur exacte attendue.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

#include "head/head.hpp"
#include "l07_heads.hpp"

using namespace mhgp10;

static void show(const char* name, const PointDendrogram& d, const ClusterParams& p) {
  const Outcome v = validate(d);
  std::printf("%s : validate=%s", name, v.ok() ? "ok" : std::string(reason_name(v.reason)).c_str());
  if (!v.ok()) {
    std::printf("\n");
    return;
  }
  const Clustering V = cluster(d, p);
  const Clustering N = l07::select_x(l07::condense_x(l07::normalize(d), p, true), d.points(), p);
  std::printf(" mcs=%llu z=%g single=%d\n  publiee  : stabilites", (unsigned long long)p.min_cluster_size, p.z,
              int(p.allow_single_cluster));
  for (double s : V.tree.stability) std::printf(" %.17g", s);
  std::printf(" ; retenus");
  for (u32 c : V.selected) std::printf(" %u", c);
  std::printf(" ; etiquettes");
  for (i32 l : V.label) std::printf(" %d", l);
  std::printf("\n  cohortes : stabilites");
  for (double s : N.tree.stability) std::printf(" %.17g", s);
  std::printf(" ; retenus");
  for (u32 c : N.selected) std::printf(" %u", c);
  std::printf(" ; etiquettes");
  for (i32 l : N.label) std::printf(" %d", l);
  std::printf("\n");
}

int main() {
  ClusterParams p;
  // 1. Plus petite erreur de stabilite : une composante, deux points entres aux rayons carres 1 et 4, mcs = 2.
  //    Exact (z = 1) : apres le depart du point entre a 4, il reste un point < mcs : il sort aussi a lambda = 1/2.
  //    Stabilite exacte 2 * (1/2 - 0) = 1 ; la tete publiee paie 1/2 + 1 = 3/2.
  {
    PointDendrogram d;
    d.level = {1, 4};
    d.node_rank = {0};
    d.child_off = {0, 0};
    d.parent = {kNone};
    d.point_node = {0, 0};
    d.point_rank = {0, 1};
    d.point_weight = {1, 1};
    p = ClusterParams{};
    p.min_cluster_size = 2;
    p.z = 1;
    p.allow_single_cluster = true;
    show("1_un_noeud_deux_points (exact : stabilite 1)", d, p);
  }
  // 2. Plus petit renversement EOM : A = {1, 4}, B = {16, 16}, fusion R a 25, mcs = 2, z = 1, racine permise.
  //    Exact : S_A = 2 (1/2 - 1/5) = 3/5, S_B = 2 (1/4 - 1/5) = 1/10, S_R = 4/5 >= 7/10 : R retenu.
  //    Publiee : S_A = (1 - 1/5) + (1/2 - 1/5) = 11/10, somme 6/5 > 4/5 : A et B retenus.
  {
    PointDendrogram d;
    d.level = {1, 4, 16, 25};
    d.node_rank = {0, 2, 3};
    d.child_off = {0, 0, 0, 2};
    d.child_val = {0, 1};
    d.parent = {2, 2, kNone};
    d.point_node = {0, 0, 1, 1};
    d.point_rank = {0, 1, 2, 2};
    d.point_weight = {1, 1, 1, 1};
    p = ClusterParams{};
    p.min_cluster_size = 2;
    p.z = 1;
    p.allow_single_cluster = true;
    show("2_quatre_points_racine_permise (exact : un cluster, 0 0 0 0)", d, p);
  }
  // 3. Racine exclue (configuration par defaut) : on ajoute C = {100, 100} et une racine globale a 1600.
  //    R nait a 1/40 : S_R = 4 (1/5 - 1/40) = 7/10 ; exact S_A + S_B = 2 (1/2 - 1/5) + 2 (1/4 - 1/5) = 7/10 : egalite,
  //    on prend plutot B = {9, 9} : S_B = 2 (1/3 - 1/5) = 4/15, somme exacte 13/15 > 7/10. Pour un vrai renversement
  //    on garde B = {16, 16} et on avance la fusion de R a 20,25 (rayon 4,5) : S_R = 4 (1/4.5 - 1/40) = 71/90,
  //    exact S_A + S_B = 2 (1/2 - 2/9) + 2 (1/4 - 2/9) = 5/9 + 1/18 = 11/18 < 71/90 : R et C retenus ;
  //    publiee S_A = (1 - 2/9) + (1/2 - 2/9) = 19/18, somme 10/9 > 71/90 : A, B et C retenus.
  {
    PointDendrogram d;
    d.level = {1, 4, 16, 20.25, 100, 1600};
    d.node_rank = {0, 2, 3, 4, 5};
    d.child_off = {0, 0, 0, 2, 2, 4};
    d.child_val = {0, 1, 2, 3};
    d.parent = {2, 2, 4, 4, kNone};
    d.point_node = {0, 0, 1, 1, 3, 3};
    d.point_rank = {0, 1, 2, 2, 4, 4};
    d.point_weight = {1, 1, 1, 1, 1, 1};
    p = ClusterParams{};
    p.min_cluster_size = 2;
    p.z = 1;
    p.allow_single_cluster = false;
    show("3_six_points_racine_exclue (exact : deux clusters, 0 0 0 0 1 1)", d, p);
  }
  // 4. Racine trop petite retenue : trois points, mcs = 5, racine permise.
  {
    PointDendrogram d;
    d.level = {1};
    d.node_rank = {0};
    d.child_off = {0, 0};
    d.parent = {kNone};
    d.point_node = {0, 0, 0};
    d.point_rank = {0, 0, 0};
    d.point_weight = {1, 1, 1};
    p = ClusterParams{};
    p.min_cluster_size = 5;
    p.z = 1;
    p.allow_single_cluster = true;
    show("4_racine_de_masse_3_sous_mcs_5 (attendu : tout en bruit)", d, p);
  }
  // 5. Trous du validateur (on ne lance PAS la tete sur ces objets : lecture hors bornes ou masses fausses).
  {
    PointDendrogram d;
    d.level = {1};
    d.node_rank = {0};
    d.child_off = {0, 0};
    d.parent = {kNone};
    d.point_node = {0};
    d.point_rank = {7};  // rang d'entree hors de la table des niveaux, point attache a la racine
    d.point_weight = {1};
    const Outcome v = validate(d);
    std::printf("5a_point_rank_hors_table (racine) : validate=%s (level.size()=1, point_rank=7)\n",
                v.ok() ? "ok" : std::string(reason_name(v.reason)).c_str());
    PointDendrogram e;
    e.level = {1, 4};
    e.node_rank = {0, 1};
    e.child_off = {0, 0, 2};
    e.child_val = {0, 0};  // le meme enfant deux fois
    e.parent = {1, kNone};
    e.point_node = {0, 0};
    e.point_rank = {0, 0};
    e.point_weight = {1, 1};
    const Outcome w = validate(e);
    std::printf("5b_enfant_en_double : validate=%s\n", w.ok() ? "ok" : std::string(reason_name(w.reason)).c_str());
    PointDendrogram f;
    f.level = {1, 4};
    f.node_rank = {0, 0, 1};
    f.child_off = {0, 0, 0, 1};
    f.child_val = {0};  // le noeud 1 a pour parent 2 mais n'est pas dans sa liste d'enfants
    f.parent = {2, 2, kNone};
    f.point_node = {0, 0, 1, 1};
    f.point_rank = {0, 0, 0, 0};
    f.point_weight = {1, 1, 1, 1};
    const Outcome u = validate(f);
    std::printf("5c_noeud_orphelin_du_csr : validate=%s", u.ok() ? "ok" : std::string(reason_name(u.reason)).c_str());
    if (u.ok()) {
      p = ClusterParams{};
      p.min_cluster_size = 2;
      p.allow_single_cluster = true;
      const Clustering V = cluster(f, p);
      std::printf(" ; etiquettes publiees");
      for (i32 l : V.label) std::printf(" %d", l);
      std::printf(" ; masse de la racine %llu (4 points)", (unsigned long long)V.tree.mass[0]);
    }
    std::printf("\n");
    PointDendrogram g;
    g.level = {-4, -1};  // niveaux negatifs croissants
    g.node_rank = {0};
    g.child_off = {0, 0};
    g.parent = {kNone};
    g.point_node = {0, 0};
    g.point_rank = {0, 1};
    g.point_weight = {1, 1};
    const Outcome t = validate(g);
    std::printf("5d_niveaux_negatifs : validate=%s\n", t.ok() ? "ok" : std::string(reason_name(t.reason)).c_str());
  }
  // 6. Egalites exactes EOM tranchees par l'arrondi (z = 2, lambda = 1 / niveau, racine permise) :
  //    A = mA points au niveau a, B = mB points au niveau b, fusion au niveau s. Egalite exacte ssi
  //    (mA b + mB a) s == 2 (mA + mB) a b ; la regle (sous-arbre > stabilite) donne alors le parent.
  {
    u64 ties = 0, parent = 0, children = 0;
    int shown = 0;
    for (u32 a = 1; a <= 40; ++a)
      for (u32 b = a; b <= 40; ++b)
        for (u32 s = b + 1; s <= 80; ++s)
          for (u32 mA = 2; mA <= 5; ++mA)
            for (u32 mB = 2; mB <= 5; ++mB) {
              if (u64(mA * b + mB * a) * s != u64(2) * (mA + mB) * a * b) continue;
              ++ties;
              PointDendrogram d;
              if (a == b) d.level = {double(a), double(s)};
              else d.level = {double(a), double(b), double(s)};
              const u32 rb = a == b ? 0 : 1, rs = a == b ? 1 : 2;
              d.node_rank = {0, rb, rs};
              d.child_off = {0, 0, 0, 2};
              d.child_val = {0, 1};
              d.parent = {2, 2, kNone};
              for (u32 i = 0; i < mA; ++i) {
                d.point_node.push_back(0);
                d.point_rank.push_back(0);
                d.point_weight.push_back(1);
              }
              for (u32 i = 0; i < mB; ++i) {
                d.point_node.push_back(1);
                d.point_rank.push_back(rb);
                d.point_weight.push_back(1);
              }
              if (!validate(d).ok()) continue;
              p = ClusterParams{};
              p.min_cluster_size = 2;
              p.z = 2;
              p.allow_single_cluster = true;
              const Clustering V = cluster(d, p);
              if (V.selected.size() == 1) ++parent;
              else {
                ++children;
                if (shown++ < 3)
                  std::printf("6_egalite_exacte a=%u b=%u s=%u mA=%u mB=%u : la tete retient les enfants ; S_parent=%.17g "
                              "S_A=%.17g S_B=%.17g\n",
                              a, b, s, mA, mB, V.tree.stability[0], V.tree.stability[1], V.tree.stability[2]);
              }
            }
    std::printf("6_egalites_exactes_EOM : %llu egalites construites ; parent retenu %llu fois, enfants %llu fois\n",
                (unsigned long long)ties, (unsigned long long)parent, (unsigned long long)children);
  }
  // 7. Peigne : m scissions en chaine, chacune detache une feuille de 5 points (mcs = 5, z = 2, racine exclue,
  //    pas de lambda uniforme : le parent l'emporte a chaque niveau, seuls les deux enfants de la racine restent).
  //    La structure est celle d'un arbre condense de profondeur m ; on mesure les pas de remontee et le temps.
  {
    for (u32 m : {2000u, 4000u, 8000u, 16000u}) {
      std::vector<std::pair<double, int>> lv;  // (niveau, cle) : cle 2 i = scission i, 2 i + 1 = feuille i
      for (u32 i = 0; i < m; ++i) lv.push_back({1.0 / (i + 1.0), int(2 * i)});
      for (u32 i = 0; i <= m; ++i) lv.push_back({1.0 / (i + 1.5), int(2 * i + 1)});
      std::sort(lv.begin(), lv.end());
      std::vector<u32> rank_of_key(2 * m + 2, 0);
      PointDendrogram d;
      for (u32 r = 0; r < lv.size(); ++r) {
        d.level.push_back(lv[r].first);
        rank_of_key[lv[r].second] = r;
      }
      // noeuds : feuille m, puis pour i = m - 1 .. 0 : feuille i, chaine i
      std::vector<u32> leaf_id(m + 1), chain_id(m);
      auto add_node = [&](u32 rank) {
        d.node_rank.push_back(rank);
        d.parent.push_back(kNone);
        return static_cast<u32>(d.node_rank.size() - 1);
      };
      leaf_id[m] = add_node(rank_of_key[2 * m + 1]);
      for (u32 i = m; i-- > 0;) {
        leaf_id[i] = add_node(rank_of_key[2 * i + 1]);
        chain_id[i] = add_node(rank_of_key[2 * i]);
        d.parent[leaf_id[i]] = chain_id[i];
        d.parent[i == m - 1 ? leaf_id[m] : chain_id[i + 1]] = chain_id[i];
      }
      const u32 nn = d.nodes();
      d.child_off.assign(nn + 1, 0);
      for (u32 v = 0; v < nn; ++v)
        if (d.parent[v] != kNone) ++d.child_off[d.parent[v] + 1];
      for (u32 v = 0; v < nn; ++v) d.child_off[v + 1] += d.child_off[v];
      d.child_val.resize(d.child_off[nn]);
      {
        std::vector<u32> fill(d.child_off.begin(), d.child_off.end() - 1);
        for (u32 v = 0; v < nn; ++v)
          if (d.parent[v] != kNone) d.child_val[fill[d.parent[v]]++] = v;
      }
      for (u32 i = 0; i <= m; ++i)
        for (int q = 0; q < 5; ++q) {
          d.point_node.push_back(leaf_id[i]);
          d.point_rank.push_back(d.node_rank[leaf_id[i]]);
          d.point_weight.push_back(1);
        }
      if (!validate(d).ok()) {
        std::printf("7_peigne m=%u : dendrogramme invalide\n", m);
        continue;
      }
      p = ClusterParams{};
      p.min_cluster_size = 5;
      p.z = 2;
      const auto t0 = std::chrono::steady_clock::now();
      const Clustering V = cluster(d, p);
      const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      l07::Walk w;
      const Clustering P = l07::select_x(l07::condense_x(d, p, false), d.points(), p, &w);
      std::printf("7_peigne m=%u points=%u clusters=%zu retenus=%zu profondeur=%u pas_etiquettes_clusters=%llu "
                  "pas_etiquettes_points=%llu pas_desactivation=%llu temps_cluster=%.3f s port_identique=%d\n",
                  m, d.points(), V.tree.parent.size(), V.selected.size(), w.max_depth, (unsigned long long)w.cluster_label,
                  (unsigned long long)w.point_label, (unsigned long long)w.deactivate, dt, int(P.label == V.label));
    }
  }
  return 0;
}
