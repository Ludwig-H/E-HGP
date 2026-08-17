# Note de Claude — la génération est parallèle par rectangle : ×3,6 sur 4 vCPU, sorties au bit près

Date : 17 août 2026. Reçu :
`receipts/forest_20260817/ADDENDUM_PARALLELISME_20260817.md`.

Suite directe de la directive utilisateur (« Puis paralléliser et
écrire pour GPU ») et du constat des trois derniers reçus : après le
sweep à deux côtés, le cœur de seed et le kernel sans allocation, la
constante restante était l'indépendance des descentes par seed — la
parallélisation était le levier le moins risqué et le plus grand.

`collect_candidate_balls(..., num_threads)` : rectangles vivants tirés
dynamiquement (compteur atomique), ouvriers à brouillons/émissions/
stats privés, fusion additive. Exactitude par structure : multiensemble
d'émissions indépendant du découpage + tri stable + RLE ⟹ objet
post-RLE au bit près ; compteurs = sommes. Porte `--par-gate` (1 fil vs
4 fils, deux familles × deux chemins, clés ET compteurs jusqu'à
`seed_core_nodes`) ; mutant `par-drop-shard` tué. 105 CTest verts.

Chiffres (4 vCPU) : eight_clusters n=1000 `t_gen` 34,9 → **9,7 s**
(×3,6, idem baseline) ; uniform n=1600 27,0 → 10,6 s (×2,5). Cumul de
la séquence sur le cas dur : **135,9 s → 9,7 s (×14)**, sorties
identiques à chaque étape (219 653 événements).

Points d'attention pour vos contre-audits :

1. Les chronos `t_core/t_ab/t_reduce/t_emit` cumulent désormais du
   temps CPU à N fils (somme des ouvriers) — les comparer entre runs
   exige le même `--threads`. Les reçus le noteront.
2. Le protocole G4 épinglé n'est PAS amendé : ses runs restent à 1 fil
   tant que la campagne n'a pas statué — amender le protocole est un
   choix d'audit, pas un effet de bord de ce commit.
3. Le top-k sur l'arbre (b8c4a4d § 2) et le traitement groupé par ancre
   restent ouverts : le parallélisme multiplie, il ne remplace pas la
   réduction du travail par seed. Je les reprends ensuite, puis
   l'écriture GPU des deux noyaux réguliers (cœur de seed, primitive de
   sweep).
