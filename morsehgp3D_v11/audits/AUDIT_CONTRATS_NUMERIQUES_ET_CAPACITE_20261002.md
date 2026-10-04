# Audit courant v11 — contrats, performance et intégration

4 octobre 2026. Sources : c40f40798 (qualification FULL), b87285378
(pipeline), ab1a739d1 (banc de points), f1a53fe1c (outil d'archives G4).
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
[Audit mathématique actif](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

## Blocage nouveau à corriger

**Le préparateur et l'outil d'archives ne partagent pas le même manifeste.**
`bench/points_unpack.py` exige `labels_sha256` ; pour chaque nouvelle scène
voisine, `points_lidar_prepare.py` n'écrit que `sites_sha256`. Les archives
produites par ce chemin sont refusées, même si leurs octets sont corrects.
Ajouter le hash des étiquettes au **producteur**, garder la validation stricte,
puis une porte d'interopération ancienne/nouvelle archive. Le manifeste
historique pts3 a bien les deux hashes pour ses 64 scènes : aucun blocage de
ce lot n'est établi. **11 gardes AST normal/−O** exécutent l'expression
productrice et la boucle consommatrice figées, avec 16 octets synthétiques ;
aucune extraction d'archive réelle ni rehash de données LiDAR.
[Sources épinglées et test causal](../receipts/unpack_manifest_review_20261004/README.md).

## Ce que les mesures G4 prouvent

La tour FULL native est implémentée : catalogue, census/index, descentes MEB,
naissances et multifusions atomiques, parents et verticales. Les défauts de
propriétaire avant mémo, admission des census, tri/FENV et verdict du banc
sont corrigés et testés. Ce sont des acquis, plus des réserves courantes.

| Source et lot | Qualification propre au lot | Médianes FULL K5/u21/W48, ms |
|---|---|---|
| c40, banc clos | 4073/4073 portes, 326 mutants, 81/81 prises appariées | 489,1 /345,1 /432,4 |
| b872, claudeab7 | 666 portes, 7 TSan, 11 mutants, 36/36 prises appariées | 412,4 /351,7 /380,7 |

Trois trames **entières sans sol**, 08/000000, 000100, 000200, même séquence,
grille 1 mm, sites unitaires ; segmentation/préparation/IO hors chrono FULL.
Les profils u18/u24 et ASan/TSan de c40 ont leurs portes distinctes ; Clang
absent. Le lot b872 ne transfère pas toutes ces portes à ses autres profils.
[Qualification et compteurs](../receipts/qualification_performance_20261003/README.md),
[pipeline et lecteur strict](../receipts/developpement_20261003/pipeline_g4/README.md).
Les arrêts ciblés des sessions archivées sont certifiés.

Le gain c40 face à la **baseline v11** 895680ff8 est ×2,68–3,28 à W48,
avec sorties complètes identiques. Les phases dominantes restent génération
catalogue et descentes régulières/non terminales K5. Tri 9–13 ms et
classification 2–3 ms sont secondaires. Les médianes par phase concurrente
ne s'additionnent pas ; une somme de tâches n'est pas le temps CPU.
Pics Buffer+Cloud c40 : 346,0 /298,7 /368,3 MiB, hors RSS/piles/allocateur.

La v10 historique mesurait 204–254 ms en **u18**, non appariée à ces lots
u21. Son juge FULL acceptait des forêts erronées : pas de qualification ni
contrat de complétude hérités. Les 81 prises comparent deux versions v11,
pas v10/v11 sur LiDAR entier. [Critique v10](../docs/AUDIT_V10_SYNTHESE.md).
**200/100 ms, GPU, plusieurs séquences, massif et temps de points natifs
restent ouverts.** Aucun chrono plat nouveau dans cette contrelecture.

## Banc exact FULL → points

F/claudepts6 joue f02f91c7e, sources identiques à ab1a : **export FULL C++
CPU/u21**, consommateur rayon 457 et oracle 2f05 **Python**. Porte conforme
sur 2 854 nuages, 194 520 comparaisons, 215 974 comparaisons de sites répétées,
12 fixtures et quatre mutants **Python**, exporteur natif inchangé.
Domaine n≤9, k≤4, m≤n ; le propriétaire au plateau algébrique est désormais
exercé. Aucun port natif PointRadiusDate/K10/u24 n'est qualifié par cela.
[Lecture indépendante : 4 343 contrôles](../receipts/points_gate_qualification_20261004/README.md).
E conserve son refus de dossier manquant ; E/F sont fermées, arrêts certifiés.

F termine 205 cas : 128 synthétiques, cinq démos, 72 voisines à k2/3/5/10.
Une voisine duplique la démo02 : **71 scènes distinctes/859 observations
d'instances corrélées**, séquence08 uniquement. Tailles 32 462–126 267 sites,
sol conservé en démo04. Les 201 JSON communs D/F sont égaux hors quatre
champs de temps ; pas toutes les dates/propriétaires internes. Le lot mesure
le **meilleur bloc**, pas une sélection plate. [Archives D](../receipts/pts4_review_20261003/README.md).
m>n refuse actuellement : déclarer m≤n ou des points inactifs.

## Contrat natif encore à construire

Arbre de points N-aire, après suppression des vides/unaires : **≤2n−1 nœuds**.
Le produire depuis FULL et les attaches, sans matrice n² ni liste de membres
par ancêtre. DP : score/décision par cluster, puis un passage d'émission des
labels. Compter ensemble arbre, dates, scores, scratch, IDs/labels et FULL.
Le catalogue et les coquilles n'ont pas de borne linéaire universelle.
La construction compacte des prototypes privés reste un **plan**.

PointRadiusDate : trois rangs, égalité algébrique, ordre commun avec FULL,
coupes fermées, refus transactionnels. Majorants u18/u21/u24 : niveaux
156/116, 180/134, 204/152 bits ; produits de comparaison quatre racines
2022/2334/2646 bits. Wide2048 ne couvre pas le majorant u21 complet.
Six racines : zéro par classes carrées ; budgets suffisants conservateurs
45996/53097/60198 bits, pas une prévision de coût. 8192 bits est un budget
avec refus. Export192 ne couvre pas automatiquement le majorant u24=204.
Aucun Cloud atteignant ces extrêmes n'est revendiqué.
[Contrat détaillé Q8](../receipts/points_answers_20261003/root/Q8_CONTRAT.md).

La tête EOM exige aussi signe/égalité/refus pour ses réciproques ; l'aide
algébrique nouvelle certifie le zéro sans prouver le budget natif rapide.
[Preuve et gardes](../receipts/eom_exact_audit_20261004/README.md).
Conserver HDBSCAN officiel séparé du bras N-aire commun. Le plafond B(H)
dépend de l'univers de blocs, pas seulement du nom de l'algorithme.
[État des rapports et témoin F2](../receipts/flat_evidence_followup_20261004/README.md).
Les plans 432 scènes/6–9 sessions et les choix z/mcs ne sont pas des mesures
ni une décision finale transférable au produit.

Cette contrelecture utilise sources figées et Python borné normal/−O,
**aucun fit, build/test natif ni GCP**. Documentation de cette publication
contrôlée séparément : le contrôleur global exclut v11 et garde ses
213 liens v10 préexistants en échec. Les anciennes notes sont archivées
intactes ; aucun reçu clos ni travail d'un autre acteur n'est réécrit.
