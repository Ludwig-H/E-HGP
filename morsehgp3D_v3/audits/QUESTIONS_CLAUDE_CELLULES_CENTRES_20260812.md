# Questions de Claude — route par cellules de centres, vers 50 k sous une seconde

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Contexte : je reprends la route de
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
comme voie principale et j'abandonne, pour le chemin produit, le parcours
d'arrangement et la cascade duale q2. Le premier prototype
`prototype/centre_cell_source.cpp` (non suivi, non câblé) est **d'accord avec un
juge exhaustif sur les identités** à `n=200, smax=11, uniform` :
`judge_truth=41963`, `missing=0 spurious=0 mismatched=0`. Son coût est
inacceptable (`subsets_tried=61 521 353` pour 200 points) parce qu'il développe
`C(m,q)` par cellule terminale. Je le réécris. Avant, je soumets deux lemmes
que je crois nouveaux et trois questions de conception.

## L1 — Le rang `p'+q<=smax` mesuré sur la liste suffit à certifier le census

La note demande de partitionner par `beta<=R_q(C)` contre `beta>R_q(C)`. Le
test exact de `beta<=R_q(C)` est coûteux : `beta` est rationnel de dénominateur
`(2 D)^2` avec `D` un déterminant `orient3` de sites u16, et la comparaison
avec `R_q(C)` à l'échelle `2^{2 depth}` déborde `i128`.

Je prétends que ce test n'est jamais nécessaire.

**Lemme.** Soit `C` une cellule, `q` une arité, `t_q=smax-q+1<=n`, `R_q(C)` la
`t_q`-ième plus petite valeur de `u_C` et `A_q(C)` la liste associée. Soit `U`
un support minimal positif d'arité `q` de centre possédé par `C`, de boule `B`.
Soit `p'` le nombre de sites de `A_q(C)` strictement intérieurs à `B`. Alors

$$p'+q\leq smax\ \Longrightarrow\ \beta\leq R_{q}(C)\ \Longrightarrow\ I_{B}\subseteq A_{q}(C)\ \text{ et }\ U_{B}\subseteq A_{q}(C).$$

Autrement dit, le census restreint à la liste est **exact dès qu'il est
accepté**, sans jamais comparer `beta` à `R_q(C)`.

**Preuve.** Supposons `beta>R_q(C)`. Les `t_q` sites témoins de plus petit `u_C`
vérifient `l_C<=u_C<=R_q(C)`, donc ils appartiennent tous à `A_q(C)`, et
`||x-c||^2<=u_C(x)<=R_q(C)<beta`, donc ils sont tous strictement intérieurs à
`B`. Ils sont donc tous comptés dans `p'`, d'où `p'>=t_q=smax-q+1` et
`p'+q>=smax+1`. Par contraposée, `p'+q<=smax` entraîne `beta<=R_q(C)`, et le
reste est la complétude déjà prouvée dans la note. Fin.

Deux conséquences que je demande de confirmer ou de réfuter :

1. la porte de rejet correcte est `p'+q>smax` (arité du support minimal), **pas**
   `p'+s'>smax` (rang fermé). Le prototype actuel rejette sur le rang fermé :
   c'est un rejet sain mais qui confond deux classes. Le rang fermé
   `p+|U_B|<=smax` reste la classification `accepted_closed_rank`, et
   `p+q<=smax<p+|U_B|` est la classe extra-shell à quotienter ou refuser, jamais
   à effacer silencieusement ;
2. comme `A_4(C)` inclus dans `A_3(C)` inclus dans `A_2(C)`, on peut faire tous
   les census sur la seule liste `A_2(C)` en gardant la porte `p'+q<=smax` avec
   le `q` du support : les `t_q` témoins de l'arité `q` sont dans `A_q(C)`, donc
   dans `A_2(C)`, donc comptés. La restriction `l<=R_q(C)` reste nécessaire pour
   la **génération** des membres du support. Confirmez-vous que ce mélange
   liste-q2/seuils-par-arité est exact ?

## L2 — La coquille fermée `U_B` détermine la boule

Pour éviter une `BallKey` rationnelle réduite (pgcd sur `i128`, débordement), je
propose `BallKey := U_B` trié.

**Lemme.** Deux boules distinctes possédant chacune un support minimal positif
et ayant la même coquille fermée `U_B` sont égales.

**Preuve.** Le centre est équidistant de tous les points de `U_B`; l'ensemble de
ces points est un sous-espace affine `E` orthogonal à `aff(U_B)`. Le centre
appartient à `relint conv(U)` pour un support `U` inclus dans `U_B`, donc à
`aff(U_B)`. Or `E inter aff(U_B)` est réduit à un point dès que `U_B` contient
au moins deux points : c'est le circumcentre de `U_B` dans `aff(U_B)`. Les deux
boules ont donc même centre, et même rayon puisque `U_B` est sur les deux
sphères. Fin.

Question : acceptez-vous `U_B` trié comme `BallKey` exacte et entière, ou
exigez-vous quand même le centre rationnel réduit dans le reçu ?

## Q1 — Critère de subdivision

Je remplace l'énumération `C(m,q)` par une énumération de **cliques d'intervalles**.
Pour `c` dans `K_C`, tout membre `x` d'un support de rayon carré `beta` vérifie
`l_C(x)<=beta<=u_C(x)`. Les membres d'un support ont donc des intervalles
`[l_C,u_C]` d'intersection non vide. En triant par `l_C` et en descendant avec
l'intersection courante, l'énumération est exactement en `O(nombre de cliques)`.
Le test de bissecteur exact reste appliqué en plus (il est strictement plus fort
que le recouvrement d'intervalles).

Mon estimation pour un nuage volumique de pas `s0`, cellule de côté `delta` :
`|A_2(C)|` vaut environ le nombre de sites à distance `d_10+1,73 delta`, et le
travail total de paires est minimisé vers `delta` proche de `0,83 s0`, ce qui
donne de l'ordre de `1,4.10^4 n` opérations de paires. Est-ce cohérent avec vos
propres bornes ? Voyez-vous un critère de subdivision plus économique que
« largeur d'antichaîne d'intervalles » ou « cardinal de liste » ?

## Q2 — Régime LiDAR

Le contrat demandé porte « au moins sur certains régimes proches du LiDAR ».
Pour un nuage porté par une surface, les centres des boules pertinentes vivent
dans un voisinage tubulaire de la surface, donc le nombre de cellules terminales
devrait croître comme la surface et non comme le volume. Confirmez-vous que la
famille `scanline_overlap_multiecho` du dépôt est le bon régime cible pour un
premier GO, et acceptez-vous que le premier verdict soit rendu sur elle et sur
`terrain`, `uniform` restant mesurée mais non bloquante ?

## Q3 — Doublons exacts

Les familles quantifiées peuvent produire des sites de coordonnées identiques.
Un « support » `q2` de deux sites confondus a un rayon nul. Je propose de les
détecter à l'entrée, de les compter, et de refuser explicitement
(`unsupported_degeneracy`) au lieu de produire une boule de rayon nul. Est-ce la
bonne politique, ou faut-il une politique d'agrégation reçue avant la source ?

## Ce que je fais sans attendre

Je réécris le prototype avec : DFS et tampons par profondeur (plus de copie de
liste vers huit enfants), seuils `R_2/R_3/R_4`, énumération par cliques
d'intervalles, filtre de bissecteur exact, test de propriétaire half-open avant
tout census, porte `p'+q<=smax`, `BallKey=U_B`, classes `accepted_closed_rank`
et extra-shell séparées, juge exhaustif sur les identités, mutants et compteurs
complets. Puis rampe `12 500/25 000/50 000` sur les quatre familles.

GCP non utilisé à cette heure.
