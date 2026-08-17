# Audit ciblé après `48e446` — le sweep est exact, mais son chemin chaud alloue par seed

Date : 17 août 2026.  
Pin audité : `48e44675413c1760827dec6123e54a33775ba562`.

## Verdict

Le sweep axial à deux côtés est mathématiquement reçu.

Pour un seed fixé, le code calcule correctement

```text
d_cover(mu) = p + #{B > 0, mu_z < mu} + #{B < 0, mu_z > mu},
```

conserve les égalités de frontière, fusionne les deux signes d'une même racine et choisit le représentant canonique minimal. La suppression de `depth_dead` dans le chemin axial est donc justifiée ; le préfiltre global par `BallKey` reste ensuite l'autorité sur les points hors du cover.

Je ne vois pas de nouvelle faute géométrique dans ce commit.

Le verrou immédiat est maintenant plus prosaïque et probablement coûteux sur le cas mesuré : le nouveau sweep effectue plusieurs allocations dynamiques pour chacun des millions de seeds.

---

## 1. Le code alloue plusieurs fois par seed

Le chemin courant construit localement :

```cpp
struct MuGroup {
  AxialSite head;
  u64 npos = 0, nneg = 0;
  std::vector<i32> members;
};

std::vector<MuGroup> groups;
std::vector<u64> pos_before(ng), neg_after(ng);
```

Chaque groupe créé alloue en outre son propre `members`.

Sur `eight_clusters,n=1000`, le reçu annonce environ `4,4 M` seeds. Même avec un seul groupe non vide par seed, le chemin peut donc provoquer plusieurs millions d'allocations et de libérations pour :

```text
groups,
members,
pos_before,
neg_after.
```

Avec plusieurs groupes, le nombre de blocs alloués devient facilement de l'ordre de plusieurs dizaines de millions. Ce layout est également l'inverse du futur kernel GPU : petits objets imbriqués, pointeurs et allocations indépendantes.

Avant d'ouvrir une nouvelle structure spatiale, il faut retirer ce coût certain. Les humains construisent volontiers un nouvel arbre pour éviter de remarquer le tas mémoire déjà planté dans la boucle interne.

---

## 2. Remplacement exact sans allocation par seed

Le nombre de groupes distincts est borné par

```text
ng <= 2k <= 2h_4 <= 16.
```

Il doit donc être représenté par des tableaux fixes :

```cpp
struct FixedMuGroup {
  AxialSite head;
  u32 npos;
  u32 nneg;
  bool alive;
  bool have_best;
  BallCandidate best;
  Q4Form best_f4;
};

std::array<FixedMuGroup, 16> groups;
std::array<u8, 16> order;
std::array<u8, 16> rank;
std::array<u64, 16> pos_before;
std::array<u64, 16> neg_after;
size_t ng = 0;
```

Pour les membres, utiliser un seul buffer plat réutilisé entre seeds :

```cpp
std::vector<u8> axial_gid;  // déclaré hors des boucles de seeds
axial_gid.assign(axial.size(), 0xff);
```

Pendant le passage de groupement :

```text
site hors [L,U]  -> compteur de rejet ;
site dans [L,U]  -> recherche parmi au plus 16 groupes,
                    axial_gid[i] = group_id,
                    incrément npos/nneg.
```

Trier seulement `order[0..ng)` par `mu`, sans déplacer les groupes ni invalider `axial_gid`. Calculer les préfixes et suffixes dans les tableaux fixes, puis marquer `groups[g].alive` selon `d_cover`.

Enfin, effectuer un unique passage sur `axial` :

```cpp
for (size_t i = 0; i < axial.size(); ++i) {
  const u8 g = axial_gid[i];
  if (g == 0xff || !groups[g].alive) continue;
  // valid_completion, puis minimum canonique du groupe
}
```

Cette version :

- examine exactement les mêmes membres ;
- conserve exactement les mêmes groupes et représentants ;
- n'ajoute aucun calcul rationnel ;
- supprime tous les `vector` imbriqués et les allocations par seed ;
- produit directement un layout SoA transposable sur GPU.

Le buffer `axial_gid` garde sa capacité maximale et ne réalloue qu'en cas de nouveau plus grand cover.

---

## 3. Le compteur `morts_bilat` mélange actuellement deux unités

`axial_groups_killed_two_sided` est incrémenté :

1. une fois par **site-racine** positif sous `L` ou négatif au-dessus de `U` ;
2. une fois par **groupe distinct** tué ensuite par `d_cover`.

La valeur publiée sous

```text
morts_bilat
```

n'est donc ni un nombre de groupes, ni un nombre de candidates, ni un nombre d'appels à `valid_completion`. Le chiffre annoncé de plusieurs centaines de millions ne peut pas être comparé directement aux `groupes` émis.

Séparer au minimum :

```text
axial_root_sites_pruned_opposite_window,
axial_mu_groups_killed_exact_depth,
axial_valid_completion_calls.
```

Ce n'est pas une question de vocabulaire. Ces trois nombres décideront si le prochain gain vient :

- du cœur universel seed-local ;
- de la recherche top-k sur l'arbre ;
- ou simplement de la suppression des allocations et des appels résiduels à `valid_completion`.

---

## 4. Portes et mesure utiles

Conserver les portes actuelles et ajouter seulement les invariants structurels :

```text
ng <= 2 * (h_4 - p),
flat et nested produisent les mêmes BallCandidates post-RLE,
le compte d_cover reste égal au scan de réception.
```

Pendant une courte phase, garder l'ancienne implantation derrière :

```text
--axial-groups=nested | flat
```

et comparer sur les trois nuages de la porte ainsi que sur `eight_clusters,n=1000`.

Publier séparément :

```text
t_axial_grouping,
t_valid_completion,
max_groups_per_seed,
root_sites_in_window,
valid_completion_calls.
```

Une porte CTest sur le temps serait inutile ; l'égalité de sortie et l'absence d'allocation dans la boucle sont les invariants. Le reçu de performance décidera du gain.

## Ordre recommandé

1. Remplacer les groupes imbriqués par la table fixe et le buffer `axial_gid` réutilisé.
2. Corriger les unités des compteurs.
3. Mesurer à nouveau `eight_clusters,n=1000`.
4. Implémenter ensuite le cœur universel seed-local et le top-k sur l'arbre si le calcul `(A,B)` reste dominant.

Le sweep à deux côtés a fermé la redondance mathématique. Il faut maintenant éviter que sa réalisation paie un petit allocateur généraliste plusieurs millions de fois avant d'accuser la géométrie.