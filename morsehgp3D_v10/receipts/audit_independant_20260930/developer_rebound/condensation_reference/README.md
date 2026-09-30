# Référence de condensation par cohortes et plateaux

30 septembre 2026. Aide au raccord courant, distincte de la nouvelle sélection
fractionnaire. Aucun fichier produit ou build existant modifié. Sources
publiées de tête/validation figées à **33fcb53a0**, deux petits builds privés.
La tête reste identique à celle des constats précédents. Aucun générateur,
sklearn, campagne statistique, GPU/GCP ou allocation massive exécuté.

## Réparation représentable sans modifier FULL

Un PointDendrogram porte deux calendriers : créations géométriques des
composantes et entrées des observations. Dans la descente en densité, une
cohorte de points quitte la branche à son rang d'entrée. Si la masse
survivante passe sous mcs, les survivants partent **au même événement**.
Tester seulement les enfants géométriques avec leur masse statique omet
cette fin. Le [constat K3 antérieur](../../point_condensation_followup/README.md)
reste la motivation, pas un nouveau calcul de géométrie.

[reference.py](reference.py) fournit une adaptation de référence :

1. Contracter les arêtes géométriques à rang égal, par représentants calculés
   de la racine aux feuilles. Le plateau devient une multifusion atomique.
2. Pour chaque composante, grouper ses observations directement attachées
   par rang d'entrée et ajouter une chaîne d'événements de cohortes.
3. Aplatir la dernière cohorte d'un enfant lorsqu'elle coïncide avec le rang
   du parent. Tous ses départs sont alors retirés avant le test des masses
   des branches restantes. Chaque observation est attachée à un événement
   de son propre rang ; toutes les autres dates restent inchangées.

Les arêtes géométriques restantes sont strictes. Une cohorte égale au rang
du parent est donc la dernière de cet enfant : son aplatissement ne traverse
pas répétitivement une chaîne géométrique de rang égal. Au plus V+n nœuds
d'événements sont produits ; chaque observation appartient à un seul
événement. Regrouper les cohortes coûte aussi leur tri. Cette borne concerne
la représentation, pas le temps du prototype récursif ou du sweep exact,
ni la condensation C++ à grande arité. Une implémentation native devra
prévoir ses types, refus et capacités avant construction.

L'adaptation préserve toutes les partitions de points avant/après chaque
date, y compris singletons avant entrée. Entre deux dates aucun événement
ne se produit, donc ces contrôles couvrent toutes les coupes. FULL, ses
incidences et ses dates ne sont pas changés. **Le transport de node_cluster
pour le vote des boules reste à définir** : ce reçu ne qualifie pas ce vote.

## Oracle indépendant et contrôles natifs

L'oracle balaye les dates décroissantes avec des ensembles d'observations
explicites : départ de toute la cohorte, fermeture si masse<mcs, puis
partition géométrique strictement sous le plateau. Il n'utilise ni
l'adaptation ni les masses statiques de sous-arbres. Comparaisons Fraction,
z=1 sur carrés rationnels, z=2 sur tous les cas ; niveaux strictement
positifs. Le domaine zéro/infini de la garde numérique R2 reste distinct.

**536 condensations exactes, 1 092 coupes ouvertes/fermées, 2 144 sélections
EOM/leaf avec/sans racine** concordent après adaptation. Les 96 arbres API
aléatoires, graine fixée, comprennent multifusions, rangs égaux, départs
différés et poids entiers positifs. Ce sont des contrôles du contrat API,
sans réalisation MEB revendiquée. Les trois cas ciblés sont staggers,
plateau géométrique nul et cohorte au split ; le quatrième est l'export
géométrique K3 historique à six sites, non régénéré. Parmi les 536 cas,
389 diffèrent de la tête statique d'origine : ce nombre mesure ce panel
API, pas une fréquence d'erreur sur les données. Sur K3/mcs6/z2, la tête
adaptée retrouve la stabilité **6/25** et les six sorties à β=25.

Le [probe.cpp](probe.cpp) appelle la tête C++ publiée inchangée sur les
arbres originaux/adaptés, après validate. **1 120 appels Release et 1 120
UBSan**, avec topologie, masses, naissances, stabilités et sorties jugées
contre l'arithmétique exacte. Les 64 sélections ciblées par backend
concordent également ; aucun verdict de labels natifs sur les EOM
aléatoires proches d'une égalité flottante n'est revendiqué.

Contrôle causal supplémentaire : six observations, deux branches de
masse 3, quatre sorties au plateau commun β=25. Ne pas aplatir la cohorte
avant la séparation fait retourner au probe UBSan **code 0**, avec deux
clusters {0,1,2}/{3,4,5}. Le sweep exact donne les six points bruit quand
la racine est exclue. Ce contrôle a une valeur incorrecte, sans crash ni
défaut de compilation interprété comme preuve. L'adaptation correcte
supprime ces clusters artificiels.

## Clôture et reproduction

[record.py](record.py) conserve les sept sources Git dans observed/, les
arguments, dépendances exactes préparées avec les flags de chaque variante,
binaires et runtimes hachés avant/après. Les builds sont privés hors du
moteur ; [build_receipt.json](build_receipt.json) donne leurs chemins et
leur fermeture. native.stdin/case_identity.json lient commandes et cas.
Les deux lecteurs normal/−O ont le même résultat.

Un premier lecteur réutilisait à sa dernière comparaison le nom de variable
de la boucle native au lieu de la fixture du contrôle plateau. Ce lecteur
et ses deux sorties restent dans preflight_reader_binding/. Le lecteur
corrigé exige les deux partitions exactes attendues ; il rejuge les mêmes
sorties natives, sans recompilation ni nouvel appel. Aucune preuve produit
n'est déduite de cette correction de lecteur.

Relecture autonome : `python3 -B check.py` et `python3 -B -O check.py`.
Hashes locaux vérifiés avant les oracles, aucune écriture ni invocation
native. Les sources historiques et leur pin interne restent inchangés.
Le ledger inclut aussi le premier lecteur ; une capture ancienne ne devient
pas la qualification de la version corrigée.
