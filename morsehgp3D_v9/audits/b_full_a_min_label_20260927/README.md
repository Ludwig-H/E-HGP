# Phase A : forêt virtuelle de minima et historiques de nœuds

27 septembre 2026. Prototype isolé **séquentiel**, hors moteur. Ce dossier
implémente [la proposition auditée](../b_full_a_events_review_20260927/NEXT_MIN_LABEL.md).
Il ne calcule ni géométrie, ni populations/images B/C, ni tour FULL complète,
ni GPU. Aucun résultat de temps LiDAR ou contrat G4 n'en découle.

## Filiation et objets conservés

`min_label.hpp` est un port explicite de `b_full_a_events_20260927/events.hpp`
(SHA256 `8c1280e9082d20e69cc1c47240160c336cb54f30d88594d34722db944d6de94f`).
L'orchestration de `probe.cpp` reprend explicitement son homologue événementiel
(SHA256 `133756d8b9a64016774ff5c8378d3958f3c8bef5067f35bb2dff3ac5805c53f2`).
Le manifeste, ses validateurs, les fixtures rationnelles, la copie observée
du Builder et le rejeu chronologique sont utilisés depuis
`../b_full_a_manifest_20260927/`, sans modification. Le CMake vérifie la source
du port et l'extraction instrumentée. Une seule unité de traduction contient
les constructeurs natif/observé ; aucune bibliothèque chain supplémentaire,
fabrique de sceau ou nouvelle résolution géométrique.

L'entrée possédée décrit les vrais blocs actifs, leurs cibles et contributions
pour un ordre K. Les données sont celles de l'entrée A native capturée, jamais
un draft final reconverti en entrée. Le résultat conserve exactement le draft
daté, les ancres, les successeurs, les rangs globaux, la boule de naissance,
les racines de **toutes** les occurrences avant déduplication et les onze
compteurs sémantiques A. Les niveaux rationnels gardent leur représentation
native, issue du premier bloc du plateau entier, même silencieux.

## Algorithme en bref

1. Dans l'ordre croissant des rangs, une DSU de travail maintient le minimum
   de chaque composant. Chaque union acceptée relie le plus grand des deux
   minima au plus petit et date cette perte du statut de minimum. Il s'agit
   d'une forêt virtuelle, pas nécessairement d'arêtes du graphe initial.
2. Le parent a toujours un ordinal plus petit ; les pertes ne diminuent pas
   vers la racine. Une seule table d'ancêtres `up` suffit donc aux coupes
   ouvertes/fermées. Le dernier pas après les sauts est indispensable.
3. Les blocs sont groupés par rang et minimum fermé, puis réordonnés par
   premier ordinal du groupe, comme le natif. Les minima ouverts de leurs
   représentants donnent les composants parents avant le plateau.
4. Une naissance ou fusion (`nombre de parents != 1`) reçoit son NodeId par
   préfixe dans l'ordre canonique. Les historiques par minimum ne contiennent
   que ces créateurs. Leur prédécesseur strict donne un parent ; leur dernier
   créateur inclusif donne l'ancre. Il n'y a plus de chaîne de redirections.
5. Un groupe à un parent ne crée pas de nœud, mais sa contribution éventuelle
   reste une action datée. Un groupe entièrement inerte garde ses ancres et
   racines observées. Une fusion sans contribution reste un créateur.

Les labels sont des **ordinals de naissance**, jamais des PointId. K1 commence
par les sites ordonnés du domaine ; les nœuds de ce lot zéro conservent leurs
rangs zéro et l'absence de boule de naissance. Les rangs globaux avec trous
ne sont pas renumérotés. Deux calendriers inversent blocs et occurrences au
sein d'un même plateau, sans inverser les rangs distincts.

La forêt virtuelle conserve les composants à chaque coupe : remplacer une
union entre deux composants par une arête entre leurs minima fusionne les
mêmes ensembles. Les pertes monotones permettent le test sur la perte de
l'ancêtre suivi d'un dernier pas. L'index n'a pas de fabrique depuis des
tableaux parent/perte externes pouvant violer ce lemme.

## Travail, mémoire et chronométrage

`V` compte les sommets (sites K1 puis blocs), `E` toutes les occurrences,
`G` les groupes, `N` les nœuds créés et `C` les contributions réellement
émises. `history_entries=N`, `omitted_history_groups=G−N`, y compris les
continuations contributrices. Les trois masses de parents restent distinctes :
parents événementiels, parents du draft (avec continuations), arcs de forêt.
Le nombre total de boules du catalogue, `ball_count`, n'est pas le nombre
de blocs actifs de K : l'allocation/initialisation des ancres denses paie
aussi `O(ball_count)` en temps et mémoire.

Avec `L=bit_width(V)`, l'index retenu possède `V*L` ancêtres u64 et `V` pertes
u32 : `8VL+4V` octets logiques, **pas** un pic mémoire. DSU par rang/minimum :
`O(E α(V)+V)` ; index `O(V log V)` ; requêtes `O((V+E) log V)` ; groupement,
tris locaux, ancres denses et sortie explicite restent payés. Cette complexité en taille du manifeste ne prouve
pas une croissance sous-quadratique en nombre de points d'une chaîne LiDAR.

Les compteurs publient les pas DSU, entrées up/perte, requêtes/pas d'ancêtres,
prédécesseurs et capacités. Le pic observé additionne les **capacités** des
vecteurs appartenant au candidat qui coexistent aux points d'observation,
avec la sortie. La factory inclut DSU/minima/rangs et les quatre CSR initiales
de la sortie. Ce n'est ni RSS, ni une borne du pic pendant une réallocation :
allocateur, pile, temporaires de tri et manifeste d'entrée restent exclus.
Le maximum du corpus dépend de l'ABI et de la bibliothèque standard.

Les durées internes séparent validation, forêt, ancêtres, groupes, parents,
historiques et sortie. Le total inclut la libération explicite des temporaires
du candidat ; le manifeste et le résultat restent vivants. La capture native
copie encore des métadonnées par K et exécute les étapes natives aval. Elle
n'est pas un chronomètre du candidat A. Cette porte ne publie aucun benchmark.

## Porte et état

La porte compare tous les champs à trois juges : sortie A native observée,
rejeu chronologique indépendant et prototype événementiel R1 qualifié.
Un BFS indépendant du graphe original vérifie toutes les classes de coupes
ouvertes/fermées sur les petits graphes. Les coupures distinctives autour
des rangs et `UINT32_MAX−1` évitent toute boucle proportionnelle aux trous.

Fixtures ciblées : minimum dans une branche latérale, pertes 3/10 et 7/7,
dernier rang fini, singleton K1, IDs non identitaires, continuation après
longue inertie, fusion sans contribution, plateau permuté, multifusion à
32 parents, catalogue non régulier et niveaux égaux de représentations
différentes. Les graphes abstraits sont des programmes combinatoires, pas
des nuages LiDAR. Trois branches mutantes du **même binaire** sont tuées :
dernier pas omis, perte de départ utilisée pour un saut, créateur non
contributeur supprimé. Ce ne sont pas trois builds mutants indépendants.

La qualification fraîche R1 est close : **17 commandes**, 1 142 dépendances
compilées épinglées, Release et Clang ASan/UBSan/LSan passent avec des
compteurs identiques. Les lecteurs LIVE normal/−O et leurs 29 tests de
corruption passent également, y compris en contrelecture indépendante.
[Reçu R1](receipts/r1/summary.json) et
[contre-audit](../b_full_a_min_label_review_20260927/README.md).
Les builds `build/v9-a-min-label-20260927-r1_release` et
`build/v9-a-min-label-20260927-r1_sanitize` sont épinglés, à ne pas réutiliser
pour développer. Le préflight mutable ne remplace pas cette capture.
Binaire `mhgp9_full_a_min_label`, schéma
`mhgp9_full_a_min_label_v1`, option CMake `MHGP9_MIN_LABEL_SANITIZE`.
CLI `--gate` ou `--preflight` ; les autres appels refusent `minimum.usage`.

Relecture sans compiler ni relancer la géométrie, depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_min_label_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_min_label_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
```

Les 376 rejeux géométriques conservent 3 368 historiques pour 3 784 groupes
(416 entrées à un parent omises), 24 152 entrées `up` et 4 632 pertes.
Les 227 graphes de coupes comprennent aussi les petits programmes abstraits ;
ils totalisent 1 886 024 vérifications. Trois branches mutantes sont détectées.
La capacité combinée maximale observée vaut 11 884 octets sur ce corpus et
cette ABI, pas une borne portable ou une mesure sur LiDAR.

Limites : DSU et tris encore séquentiels, index `O(V log V)`, tableaux E et
sorties explicites conservés. Aucun partage de captures par K ajouté. Aucun
gain sur de vrais manifestes déduit de cette seule qualification. Les
[mesures séparées sur vrais catalogues](../b_full_a_real_20260927/RESULTATS.md)
comparent maintenant min-label et événementiel, sans modifier ces sources
figées. Aucune parallélisation, aucun transfert de qualification FULL, GPU
ou contrat 100 ms.
