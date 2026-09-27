# Contre-audit — minima stables pour la construction A

27 septembre 2026. Revue indépendante du
[prototype](../b_full_a_min_label_20260927/min_label.hpp), sans changement
de ses sources, du moteur ni de reçus antérieurs. Aucun GCP, aucun build
lancé par cette revue.

## Verdict

**R1 passe la contrelecture LIVE normale et `-O`, puis les 29 corruptions
du lecteur dans les deux modes.** Aucun défaut bloquant restant trouvé.
Le [reçu](../b_full_a_min_label_20260927/receipts/r1/capture.json) ferme
17 commandes et 1 142 dépendances. Release GCC et Clang ASan/UBSan/LSan
rendent les mêmes objets et compteurs ; le sanitizer est effectivement
compilé à O1 dans cette cible.

La qualification porte sur A séquentiel depuis le manifeste déclaré,
pas sur une nouvelle géométrie, B/C, l'encodage, le GPU ou la tour complète.
Les lectures restent LIVE et dépendent des builds locaux conservés.

## Contrat mathématique relu

Les sommets sont les ordinals de naissance : sites du domaine pour K1,
puis blocs du programme ; blocs seuls pour les autres ordres. Ce ne sont
ni les BallId ni les PointId. Chaque occurrence relie son bloc source à
une cible strictement antérieure, au rang de la source.

Pendant les unions croissantes, remplacer une arête admise par une arête
entre les minima de ses deux composants fusionne exactement les mêmes
composants. Une induction sur les unions donne les mêmes composantes à
chaque coupe ouverte et fermée. L'arbre virtuel n'a pas besoin d'être un
sous-graphe du graphe des représentants. Inverser les unions au sein d'un
plateau est permis ; traverser seulement une partie du plateau ne l'est pas.

Le minimum perdant cesse définitivement d'être minimum : sa case parent
est écrite une seule fois. Les parents diminuent strictement en ordinal.
Les pertes ne diminuent pas le long d'un chemin vers la racine ; la perte
d'une racine est INF, supérieure à tout rang admis. La DSU compressée de
travail est distincte de cet arbre immuable.

Dans la requête `up`, tester la perte de l'ancêtre proposé certifie toutes
les arêtes sautées par monotonie. Le dernier pas est indispensable : les
sauts recherchent le sommet le plus haut dont l'arête sortante est encore
autorisée, puis ce dernier pas franchit cette arête. Tester la perte du
point de départ à la place de celle de l'ancêtre permettrait de franchir
une barrière. INF empêche de dépasser la racine ; le test strict des parents
exclut toutes les unions du plateau actuel.

## Historiques et sortie : pourquoi la suppression est exacte

Un groupe qui rejoint un seul composant ancien ne change ni son minimum
ni son nœud HGP vivant : tous les nouveaux ordinals sont plus grands que
ceux des représentants. Les historiques par minimum peuvent donc garder
uniquement les groupes qui créent un nœud, soit zéro parent ou plusieurs
parents distincts. Une fusion sans contribution doit rester dans cet
historique. À l'inverse, une continuation avec contribution ne crée pas
d'entrée, mais garde son action et sa contribution datée dans le draft.

Les groupes sont rangés par rang puis premier ordinal de bloc. Un préfixe
des groupes créateurs assigne les IDs natifs. Les parents et les racines
par occurrence consultent le dernier créateur **strictement avant** le
rang courant ; l'owner consulte le dernier créateur **au plus** à ce rang.
Les racines historiques ne sont jamais remplacées par la racine finale.

Le réemploi de `node_ids` est sûr ici : les historiques ne référencent que
les cases créatrices, dont l'ID est vérifié inchangé pendant la résolution.
Remplacer les autres cases par leur owner ne modifie aucun résultat de
prédécesseur. Les parents sont ensuite retriés par NodeId, et non par leur
étiquette interne.

Le port conserve niveaux rationnels du premier bloc du plateau entier,
ordre des contributions, ancres des blocs inertes, racines par occurrence,
successeurs et compteurs sémantiques. K1 crée toujours ses sites de rang
zéro dans l'ordre du domaine ; leurs populations ne sont pas ball-tagged.
Les trous entre rangs et les représentations rationnelles égales ne sont
pas normalisés au passage.

## Points renforcés avant gel

Le maximum combiné de capacités initiales inclut maintenant les quatre
petits CSR de l'Output déjà construit pendant la préparation de l'index.
Le maximum reste une observation des capacités simultanément possédées,
pas le pic transitoire interne d'une réallocation, le RSS ou la mémoire
totale du processus.

Le juge de coupes emploie des coupures distinctives autour des rangs,
plutôt que d'énumérer tous les entiers jusqu'au dernier rang. Cela permet
une arête à `absent32−1` sans boucle géante ni débordement du compteur.
Les graphes et tableaux du candidat restent séparés du parcours BFS qui
calcule les minima attendus sur le graphe original.

La factory remet aussi ses compteurs et durées à zéro avant chaque appel.
Les réutilisations de Work dans les petites fixtures ne cumulent donc pas
des arêtes de forêt d'index distincts dans un compte de composants local.

## Couverture fermée et non-vacuité

Les 22 variantes géométriques produisent 44 captures, 188 manifestes et
376 appels du candidat, dans les deux ordres d'union des plateaux. Sur ce
corpus : 4 632 sommets, 5 416 occurrences, 3 784 groupes et 3 368 historiques
créateurs. Les 416 entrées omises correspondent aux 384 groupes inertes
et 32 continuations : ces dernières restent dans les drafts. La fusion
à 32 parents demeure couverte. Les incidences sont 3 408 entre événements,
3 024 dans les drafts et 2 992 dans les forêts de nœuds.

S'ajoutent 74 appels abstraits et quatre appels de frontière, hors des
totaux géométriques précédents. Le juge BFS vérifie 227 graphes et
1 886 024 réponses de coupe, ouvertes/fermées et dans deux calendriers.
Les 1 699 comparaisons d'objets combinent natif observé, chronique et
ancien événementiel. Les 682 refus regroupent trois contrôles par graphe
(domaine de sommet, rang INF, CSR invalide) et le refus du créateur supprimé.
Ce ne sont pas 682 branches différentes du moteur.

Les trois mutations sont des branches du même binaire, pas trois builds
indépendants. Les deux mauvaises requêtes donnent explicitement 2 puis 0
au lieu de 1 sur la chaîne de pertes 3/10 ; supprimer le créateur d'une
fusion sans contribution échoue sur `minimum.creator_history_matches`.
La longue inertie conserve une seule entrée d'historique et deux actions,
dont la continuation contributrice. Le dernier rang fini est testé ouvert
et fermé sans énumération de milliards de rangs.

## Coûts et portée

La représentation supprime réellement le CSR/DFS d'enracinement, la table
des maxima, les historiques à un parent et les redirections de l'ancien
prototype. Elle ne supprime pas les groupes temporaires inertes, leurs
ancres, les occurrences E ni les sorties explicites. L'index courant garde
`8 V bit_width(V) + 4 V` octets logiques de tables, hors capacités et autres
objets coexistants. Il reste O(V log V), pas O(V).

Sur le seul corpus géométrique, 24 152 entrées `up` et 4 632 pertes sont
comptées ; les 10 048 requêtes demandent 64 080 étapes de table/dernier pas.
Le maximum observé de capacités combinées vaut 11 884 octets sur cette ABI,
contre 13 004 pour l'ancienne porte événementielle sur le même sous-corpus.
Cette comparaison de capacités bornée n'est ni un pic global ni un gain
chronométré, et n'inclut pas les plus grands cas abstraits supplémentaires.

DSU et plusieurs scans demeurent séquentiels. Les tris de groupes et de
parents, la validation et l'initialisation des sorties restent à compter.
Une borne en V/E du manifeste ne constitue pas une borne sous-quadratique
du générateur en nombre de points LiDAR. Ni gain chronométré, ni GPU,
ni contrat FULL 100 ms ne découle de ce seul port A.

La [proposition heavy-light](NEXT_HEAVY_LIGHT.md) pourrait supprimer la
table `up` au profit d'un index O(V), mais reste une tâche ultérieure
distincte, sans code ni qualification dans la présente revue.

## Reproduction et identités

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_min_label_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_min_label_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
python3 -B morsehgp3D_v9/audits/b_full_a_min_label_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_min_label_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_min_label_20260927/receipts/r1
```

- `min_label.hpp` : `6cd05606f03fe41da5ae1bfb8f53b810dcf53aa88e97607b7374219bc9519c09`.
- `probe.cpp` : `cc9e42ebfd0a44cd179172ee63384a6af82b9d963924e7c10ecf27c3f472bb24`.
- `run.py` : `78816029de56200d8b09526d1afeb931708e71154aad25206ee39983ff2313d1`.
- `selftest.py` : `4a7ec1d2618a4795a50adb3739fabdb09a87b8b15294ec81b88d1b81068558ce`.
- R1 `capture.json` : `52bdec163ecf26c7de325acf96fb1f21f4ae3bfdbc283b096e4d3becc9497a87`.
- R1 `summary.json` : `eb0036392221d59cadd6f3216cab66c5568b27eb2913bf7a1d0a61a8aaf0203e`.

Note close ; aucun handle actif, aucune preuve d'autrui modifiée.
