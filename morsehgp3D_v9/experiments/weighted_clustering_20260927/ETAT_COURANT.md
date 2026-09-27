# Développement : mesure pondérée et rattachements exacts à T_K

27 septembre 2026. Cette entrée prévaut sur le README initial, conservé à
l'identique parce qu'il fait partie des entrées hachées du premier essai.

Suite dans un **nouveau dossier**, sans modification des sources/captures
gelées de celui-ci : [dendrogramme explicite, condensation ponctuelle et
pilote K5](../point_dendrogram_20260927/RESULTATS.md).
Le routage désormais évalué n'est pas le vote plat mesuré ci-dessous.
La [lecture complémentaire old/3D](../../audits/LECTURE_HGP_OLD_CLUSTERER3D_20260927.md)
confirme aussi un écart de catalogue contributif : les formules S/T/m du
chapitre9 ne rendent pas Gabriel et ordre-Voronoï interchangeables pour
les poids. Les présentes mesures restent valides pour leur objet déclaré.

Cadre : `exploration_v9_hors_registre / cpu_reference /
quantized_u18_input_only / weighted_fixed_k_reference / not_claimed`.
Aucun fichier du moteur, registre, ancien benchmark ou build épinglé n'est
modifié. Aucune dépense GCP. Ce traitement de clustering est **après FULL** :
ses temps ne remplacent pas les mesures de construction de la tour.

La campagne de qualité corrigée et son contre-audit sont maintenant clos :
**364 lignes, 140 agrégats, 104 sorties pondérées**. Le
[bilan détaillé](RESULTATS.md) est mixte, sans dominance sur HDBSCAN ni sur
la première couverture. La publication conserve tous les réglages prévus.

## Ce qui est implémenté

1. Export du catalogue de boules complet et des cofaces Gabriel à K fixé,
   y compris les cofaces qui n'entraînent pas de fusion.
2. Calcul des scores Sτ, normalisateurs T_x et masses mτ **avant** toute
   réduction du graphe. ExpZ1 et 2 modifient ces masses, puis les densités EOM.
3. Rattachement de chaque facette à sa vraie composante FULL, à la date
   exacte de sa miniball. Les ancres sans nouveau point couvert sont gardées.
4. Arbre pondéré augmenté, puis condensation par masse minimale, EOM,
   et vote des points. Aucun remplissage 1-NN, aucune racine sélectionnée.

Le vote donne une partition pour la sélection choisie. Il ne qualifie pas
encore une famille de partitions ponctuelles emboîtées pour toutes les coupes.
Le seuil de masse n'est pas une garantie de cardinalité après vote.

## Défaut trouvé et corrigé avant publication des scores

Le premier port construisait la connexité directement avec les seules
cofaces Gabriel. Sur la première gaussienne de 1 200 points, K5, il donnait
26 composantes au lieu de l'unique racine FULL. L'essai a échoué avant tout
score : `v9-weighted-gaussian-pilot-20260927-r1`, reçu SHA256
`663f50c559f98496fa62ba5d03a294fd567c4d950a04fc3e7e539b825b3557c7`.

C'est la régression E5 déjà connue en v7, malheureusement absente des
28 premières fixtures de ce port. Ces petits tests restent réellement passés,
mais ne justifient pas la généralisation. Voir
[l'explication et la contre-fixture](AUDIT_SILENT_ATTACHMENTS.md).
Les anciennes sources, captures et leur échec sont préservés ; le graphe
de `weighted_model.py` n'est **pas** utilisé comme topologie dans le pilote corrigé.
Ce module fournit seulement les masses et le vote au nouveau raccord.

Le raccourci « retrouver toute ancre dans les contributions de points »
est également faux : les diagonales d'un carré rejoignent une fusion sans
nouvelle contribution. Le nouveau consommateur réutilise l'observation
explicite des ancres FULL ; il ne fabrique pas une racine artificielle.

## Qualification du raccord corrigé

Release **et** GCC ASan/UBSan : 46 commandes par capture, dont 33 petits
nuages/ordres confrontés à un oracle rationnel indépendant. Chaque capture
contrôle 831 boules, dont 70 non régulières, 298 cofaces Gabriel,
464 facettes et 1 166 coupes ouvertes/fermées.

La comparaison porte sur les **partitions de facettes**, leurs naissances,
leurs rattachements et l'arbre pondéré augmenté, pas seulement les unions
de points. Les facettes de masse nulle restent disponibles pour transmettre
les chemins dans l'oracle Čech. E5 réfute le graphe brut à deux coupes.
Carré, octaèdre, facettes isolées, K10 et catalogue pondéré vide K=n inclus.
Lecteurs vivants normal/−O passés pour les deux captures.

| Capture privée, sous `build/` | SHA256 du reçu |
|---|---|
| `v9-weighted-full-qualification-20260927-r1/receipt.json` | `35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c` |
| `v9-weighted-full-attachment-sanitize-qualification-20260927-r1/receipt.json` | `16b8c85a95554db8cae88ca060529466cd57bd450fc807110619e35187b6b543` |
| `v9-weighted-attachment-sanitize-checks-20260927-r1/receipt.json` (environnement) | `3513fdb45bf99d518c713e20eb754f18da28fe79c35e0675b6f5c40d714981bb` |

ASan/UBSan ont été exécutés avec `detect_leaks=0` : **aucune qualification
LeakSanitizer**. Deux essais V1 précédents restent conservés : échec du lien
Clang++ résolu par erreur vers Clang, puis refus LSan sous ptrace. Aucun
défaut géométrique n'est inféré de ces erreurs d'environnement.

Les masses et décisions EOM des grands cas restent en binary64. Les sommes
de masses d'entrée arrondies sont conservées exactement comme nombres
dyadiques ; ce n'est pas une certification des décisions par rapport aux
poids réels. Le mode rationnel z2 sert aux petits contrôles.

## Fichiers actifs

- [Export et rattachements natifs](ATTACHMENTS.md),
  `native_attachment_export.cpp`, `build_attachments.py`.
- `weighted_model.py` pour les scores, masses et votes seulement ;
  `full_weighted_tree.py` pour la topologie corrigée.
- `weighted_eom.py` : condensation et sélection avec masses pondérées.
- `full_attachment_oracle.py`, `qualify_full_attachments.py` et leurs tests :
  contrôles indépendants, domaine petit explicitement borné.
- `benchmark_full_weighted_parallel_r2.py` : pilote clos, reprise explicite
  de dix unités et seize nouveaux workers, avec fermeture de leurs groupes.
  `benchmark_full_weighted.py` conserve l'essai séquentiel interrompu ;
  `benchmark_full_weighted_parallel.py` la première version d'orchestration ;
  `benchmark_weighted.py` le premier essai géométrique réfuté.
- [Résultats](RESULTATS.md),
  [publication close r2](../../receipts/weighted_full_gaussian_20260927/r2/TABLES.md),
  `post_audit_parallel.py`, `report_full_weighted_parallel.py` et leurs tests :
  lecteurs distincts, sans promotion du reçu interrompu. La r2 enlève
  seulement un espace final Markdown ; la r1 scellée est conservée en privé,
  les CSV restent identiques octet par octet et aucun score n'est recalculé.
- [Dépôts datés](DEPOTS_DATES.md) : preuve de regroupement exact pour la
  condensation et le vote plat, sous conditions ; ce n'est pas encore un
  moteur compact implémenté.
- [Comptages des dépôts](DEPOTS_MESURES.md) : captures distinctes K5/K10
  sur le premier cas clos, avec conservation des attaches et des dates.
- [Routage ponctuel exact](POINT_ROUTING_REFERENCE.md) : référence sparse
  séparée qui produit des partitions de points emboîtées ; elle n'est pas
  la méthode de vote plat évaluée dans le pilote gaussien.

## Limites et coût

Pour z fixé, les incidences peuvent être agrégées avant le calcul des masses.
Cela ne dispense pas de payer leur production ni les attaches géométriques.
Le pilote Python conserve des objets explicites volumineux et observe une
seconde construction FULL CPU pour qualifier ses ancres : ce n'est pas une
voie industrielle, ni un chrono mono-thread (l'observateur utilise au moins
deux workers, séparément déclarés). Il ne donne aucune nouvelle borne de
croissance sur LiDAR ou de performance G4/100 ms.

La campagne de qualité corrigée est close : 13 scènes gaussiennes complètes
n1200, K5/10, masses minimales20/50, expZ1/2, comparateurs précédents conservés
par hash. Dix unités complètes ont été reprises après interruption ; seize
ont été calculées par les nouveaux workers. L'ancien reçu reste en échec et
sa clôture absente n'est pas reconstituée rétroactivement. Le contre-audit
normal/−O identique recoupe 52 mesures, 39 135 752 sorties de facettes et
1 175 pins, avec ARI exacts indépendants ; solveur Hungarian partagé,
NMI non recalculé. Sous-ensemble diagnostique de régimes déjà vus, pas test
statistique aveugle de supériorité universelle.

Les [tableaux principaux](RESULTATS.md) montrent des résultats mixtes :
le pondéré z2 progresse sur G8/δ4 sphérique, mais reste derrière HDBSCAN
commun en anisotrope et déséquilibré z2. Les scores de première couverture
restent aussi visibles. ExpZ change masses **et** λ chez HGP pondéré,
contre λ seulement pour les comparateurs à masses ponctuelles unitaires.
Ces résultats n'évaluent pas la nouvelle référence de routage emboîté.

## Piste suivante, sans gain de temps revendiqué

En dimension trois, les facettes d'une coface ont au plus cinq événements
distincts `(segment FULL fermé, naissance MEB)`. Après connaissance des
attaches, les contributions peuvent ainsi être accumulées en O(K) opérations
par coface sans développer tous les triplets coface–facette–point.
Cela ne borne ni le nombre de cofaces ni le travail de recherche des attaches.

Sur G2/δ8/graine1, le comptage donne 108 909 → 55 915 objets à K5 et
689 531 → 174 331 à K10, en passant des facettes aux événements. Les incidences
point–objet deviennent respectivement 309 138 et 1 828 667. Ce sont des
comptages, pas une mesure de vitesse ni de mémoire totale. En particulier,
le sous-total illustratif IDs/scores/masses augmente légèrement à K5.
La référence calcule déjà les scores de façon factorisée : le comparer à
une expansion naïve O(K²C) ne serait pas une mesure honnête de son gain.

Les dépôts doivent rester des **sorties de masse datées**, jamais de gros
enfants susceptibles d'être sélectionnés par EOM. L'équivalence exacte
n'hérite pas d'une égalité bit à bit des décisions binary64. Le
[routage ponctuel emboîté](POINT_ROUTING_REFERENCE.md) possède désormais
une référence distincte du vote plat, pas encore un benchmark de qualité.

## Partitions de points emboîtées : référence maintenant implémentée

`point_routing_reference.py` choisit un chemin unique par point, une fois
pour K,z fixés. Les égalités s'arrêtent au parent ; les plateaux sont
atomisés ; les points pas encore attachés restent des singletons distincts.
Les dates de feuilles sont leurs vraies naissances MEB, pas les zéros
virtuels de l'adaptateur EOM. Le score accepté est entier/Fraction positif.

La référence utilise les incidences et de petits arbres virtuels par point,
avec un index d'ancêtres commun. Pas de tableau n×V, ni de remontée de
chaque incidence sur toute la profondeur. Son coût dépend de V et I=KF,
pas d'une borne globale nouvellement acquise en n ; le coût binaire des
grands rationnels reste à payer.

Capture normal/−O close : **133 fixtures, 2 160 coupes, 14 refus**, dont
trois intégrations aux objets FULL qualifiés (E5 K2, carré K2 et K1).
Les deux lectures donnent les mêmes résultats, avec 668 pins inchangés.
Le reçu et les commandes sont décrits dans la note dédiée. Ni EOM ni ARI,
ni nouveau calcul natif/GPU n'ont été exécutés par cette qualification.
