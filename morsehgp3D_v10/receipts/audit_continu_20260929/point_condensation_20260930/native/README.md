# Condensation des sorties de points directement attachés

Capture close du 30 septembre 2026, 10:11:37–10:11:55 UTC.
Sources Git figées à
8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047.
Cadre : audit CPU de la tête/API, moteur partagé inchangé, GCP non utilisé.
Le statut EXPECTED_API_DIFFERENCE_CONFIRMED signifie que le désaccord
attendu avec la condensation à seuil de masse a été confirmé, pas que le
code courant est correct.

## Défaut exercé

[head.cpp](source/head/head.cpp) paie chaque point directement attaché à
sa propre date, puis poursuit sans tester si la masse restante tombe sous
min_cluster_size. Une feuille géométrique n'a pas de nouvelle fusion
susceptible de déclencher ce contrôle. La condensation de l'arbre de points
doit au contraire terminer la branche quand une cohorte laisse moins de
min_cluster_size unités, et faire sortir les derniers points au même niveau.

Le [premier oracle](check.py) suit les ensembles de points et les sorties
par événements exacts. Le [second](check_under_root.py) recoupe les scores
par formules fermées. Les sorties à même niveau forment une cohorte atomique.
Les niveaux sont des carrés entiers positifs, donc
lambda=beta^(-z/2) est rationnel pour z1 et z2.
Aucun zéro, infini, cap de score ou assert n'est nécessaire.

## Témoin API5, racine sélectionnable

A naît à beta1 et porte trois points entrant à beta1,4,9.
B naît à beta16 et porte deux points entrant à beta16.
Leur parent R naît à beta25. Les multiplicités valent un.

À mcs2, après les sorties de la cohorte beta4, A ne conserve qu'un point :
ce dernier doit lui aussi sortir à beta4, non beta1.

| Score z1 | Tête figée | Oracle à seuil de masse |
| --- | ---: | ---: |
| S_A | 37/30 | 11/15 |
| S_B | 1/10 | 1/10 |
| S_R | 1 | 1 |
| S_A+S_B | 4/3 | 5/6 |
| EOM, racine permise | A/B | R |

## Racine exclue et taille minimale usuelle

[probe_under_root.cpp](probe_under_root.cpp) ajoute C, deux points à beta100,
et une racine globale à beta1600. R porte cinq points, C en porte deux.
Avec allow_single_cluster=false, R naît à lambda1/40 et possède
S_R=7/8. Les enfants donnent toujours 4/3 dans le code, contre 5/6 dans
l'oracle. EOM choisit donc A/B/C au lieu de R/C : le défaut ne dépend pas
de la sélection de la racine globale.

[probe_tripled.cpp](probe_tripled.cpp) répète chaque observation trois fois
comme un point API distinct de poids un : 21 points, mcs5.
Les cohortes de A passent 9→6→3 ; le dernier passage franchit le seuil5.
Tous les scores sont multipliés par trois :
R=21/8, A+B courant=4, A+B exact=5/2. Le même renversement se produit.

Pour z2, les scores de A restent erronés dans ces cas, mais les sélections
observées ne changent pas. Ne pas généraliser ce maintien à d'autres arbres.

## Exécutions et contrôles

[receipt.json](receipt.json) conserve les commandes, codes, dates, sorties,
compiler pin et empreintes avant/après de treize sources locales. Les sept
sources produit privées sont vérifiées exactement contre les blobs Git
avant compilation. G++13.3 normal strict et UBSan non récupérant produisent
des sorties identiques et aucun diagnostic.

Six invocations natives : trois sondes × normal/UBSan, 24 configurations
API distinctes par mode, 48 lignes capturées au total.
Douze jugements Python normal/−O réutilisent ces captures.
Par mode natif : seize configurations concordent avec l'oracle,
huit ont des scores/sorties différant, quatre changent la décision EOM.
Les témoins comprennent les cohortes simultanées, sorties toutes à même
date et masses minimales deux. mcs1 sert uniquement de contrôle API positif :
ce n'est pas un paramètre accepté par sklearn HDBSCAN.

Les binaires ne sont pas embarqués. Leurs hashes avant/après exécution sont
conservés ; les chemins externes décrivent les exécutions historiques.
Rejuger les sorties ne recompile pas et n'est pas une qualification LIVE.
Les dépendances système ne sont pas archivées.
Une erreur de préparation du README, avant exécution de la cellule outil,
est conservée dans [preparation_failure.txt](preparation_failure.txt).
Elle n'a changé ni les sources ni la capture native close.

## Portée et suite

Toutes les sondes passent validate(PointDendrogram). Ceci ne prouve PAS
qu'un nuage 3D, un catalogue fort complet et la projection cover/bande
produisent exactement ces arbres et dates. Aucun test MEB de réalisabilité,
FULL, LiDAR, croissance, ARI, GPU ou contrat100ms n'est acquis ici.
Le cas triplié conserve les observations distinctes dans l'API seulement ;
ce n'est pas une entrée pondérée nouvellement supportée par la tour.

La réparation doit traiter les dates d'entrée par cohortes, maintenir la
masse restante et clore la branche au seuil, sans repayer les points déjà
sortis ni supprimer leurs continuations FULL. Son coût total et les
égalités avec les fusions spatiales doivent être jugés séparément.
La comparaison réelle sklearn est un reçu distinct détenu par la racine ;
ce paquet n'exécute et n'embarque aucune réimplémentation de HDBSCAN.

