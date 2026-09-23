# Croissance LiDAR : secteurs capteur et densité, deux axes distincts

23 septembre 2026. Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md)
contient trois scènes 08 sans sol à grille 1 mm, K5/K10, s8/W8 : chaque
trame entière, ses deux moitiés `x<0`/`x≥0`, ses quatre quarts selon `y`,
et trois disques emboîtés. Les plans sont ceux du
[capteur](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md). Je relis ici
les six résumés, **42 cas de secteurs** au total. Une seule exécution par
cas, sur hôte CPU partagé ; les compteurs de travail sont le signal
principal, les temps CPU restent indicatifs. Statut `complete_relative` :
la complétude des clés absentes du catalogue n'est pas prouvée.

## Croissance quand le domaine spatial s'étend

Pour chaque partition en moitiés H ou en quarts Q, je calcule
`R=Σ W(morceau)/W(full)` sur les compteurs publiés et
`B=Σ (n_morceau/n_full)²`, valeur repère pour un coût quadratique homogène.
Une valeur `R<B` signale plus de travail que ce repère lors du passage
des morceaux à la scène ; `R>B` signale moins. Le repère n'est **pas** un
test asymptotique : la géométrie, les frontières, les certificats et les
sorties changent avec la coupe. `CPU` est `chain_cpu_s`, `paires` est
`expanded_pairs`, `cœur` est `core_sites`.

| 08/K | `B_H/B_Q` | `R_H` CPU / paires / cœur | `R_Q` CPU / paires / cœur | liens cœur avec `p>2` | max `p_CPU` |
| --- | --- | --- | --- | ---: | ---: |
| 000000/K5 | 0,527 / 0,265 | 0,892 / 0,856 / **0,454** | 0,772 / 0,419 / 0,296 | 2/6 | 1,430 |
| 000000/K10 | 0,527 / 0,265 | 0,913 / 0,873 / 0,559 | 0,813 / 0,462 / 0,363 | 2/6 | 1,425 |
| 000100/K5 | 0,502 / 0,256 | 0,949 / 0,884 / 0,962 | 0,811 / 0,592 / 0,480 | 2/6 | 1,281 |
| 000100/K10 | 0,502 / 0,256 | 0,940 / 0,864 / 0,952 | 0,824 / 0,657 / 0,514 | 2/6 | 1,302 |
| 000200/K5 | 0,508 / 0,286 | 0,933 / 0,853 / 0,881 | 0,855 / 0,620 / 0,715 | 3/6 | 1,650 |
| 000200/K10 | 0,508 / 0,286 | 0,938 / 0,781 / 0,899 | 0,864 / 0,591 / 0,729 | 2/6 | 1,550 |

Les six liens par ligne sont `full→2 moitiés` et `chaque moitié→ses 2
quarts`. Leur pente finie est
`p=log(W_parent/W_enfant)/log(n_parent/n_enfant)` : 0/36 pentes CPU,
8/36 pentes de paires développées et **13/36 pentes de sites de cœur**
dépasse 2. Le maximum du cœur vaut **3,940** sur
`000100/K5`, `half_x_neg→quarter_x_neg_y_nonneg`. Sur 000000/K5,
la somme des cœurs des moitiés est 45,4 % du plein, sous son repère
quadratique 52,7 %. À l'inverse, les maxima CPU restent à 1,65 ou moins.
Le temps apparent masque donc certaines masses internes défavorables.

Le déséquilibre n'est pas aléatoire : pour 000200/K10, le quart
`x≥0,y<0` contient **14 829/45 845** sites (32,3 %) mais environ
**583,0/1 069,2 M** `core_sites` (54,5 % du plein, 74,7 % de la somme
des quatre quarts). Une distribution uniforme des travaux entre quarts
serait mauvaise ici. Les trois disques emboîtés du même reçu donnent,
sur 000200 de 16k à 32k, `p_core=3,05` à K5 et `2,86` à K10 : ces
deux diagnostics changent la géométrie et ne se réfutent pas mutuellement.

## Isoler la densité à emprise fixée

Pour tester le changement de densité demandé, il faut garder **chacun
des sept secteurs capteur fixe** et sélectionner globalement, par hash
stable des IDs d'origine, des sites emboîtés `1/4⊂1/2⊂1` ; l'intersection
de cette sélection avec chaque secteur conserve le même découpage et les
mêmes coordonnées. Publier le seed, les IDs/hashes, les cardinalités
réelles et vérifier que les moitiés/quarts reconstruisent le plein à
chaque densité. Comparer dans **chaque secteur** les deux pentes
`1/4→1/2` et `1/2→1`, avec K/s/W, grille, masque sans sol et moteur
identiques. Répéter avec plusieurs seeds et d'autres scènes ; une seule
réalisation ne peut qualifier un exposant. Mesurer aussi sortie, catalogue,
paires, témoins, sites/tests du cœur, atlas, q3/q4, CPU, mur et mémoire.

Les trames brutes entières et d'autres séquences restent à chronométrer
séparément ; les six préparations float32 par plans déjà vérifiées ne sont
pas des exécutions de tour v9. Ni les ratios spatiaux ni les pentes de
densité finies ne démontrent à eux seuls une borne sous-quadratique globale
ou le contrat G4.
