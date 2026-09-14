# Passation v8 — front réel implémenté, raccord census prioritaire

14 septembre 2026. Cadre actif : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Aucun contrat de tour n'est encore acquis.

## À reprendre maintenant

Le [premier front WSPD réel](docs/P0_FRONT_REEL.md) manipule les nœuds du
même index Z et leur permutation. Aucun plan Pool/Axis ni parcours de
facteur n'est préparé par produit. La convention de séparation est
`box_gap_diameter_v1`, la comparaison `Pure`/`MidpointSamples` conserve
les entrées, pas nécessairement les descripteurs. Les rejets sont propres
à chaque voie et hérités ; les comptes partiels ne sont pas hérités.
Les preuves et coûts sont dans le [nouveau reçu](receipts/wspd_front_20260914/README.md).

40 CTests Release/Clang ASan/UBSan passent. La mesure uniforme32k/s8
réduit le front de 56,8 à 20,9 millions de rectangles, mais passe de
4,87 s à 37,4 s : le proposeur est trop cher (954 millions de pas
d'index). Ne pas promouvoir `MidpointSamples` comme gain de temps
acquis, ni retirer Pure de la comparaison du futur chemin consommé.
Les 72 mesures sont closes : quatre familles, n8k/16k/32k, Kmax10,
s8/10/12, deux modes. Les tâches croissent sous ×4 dans ces mesures,
mais les résidus Samples des amas font presque ×4, comme q3/q4 sur les
rangées. P0 n'est pas clos. Les deux builds `v8_front_20260914` et
`v8_front_sanitize_20260914` sont épinglés ; reprendre dans des builds neufs.

**Priorité immédiate : consommer directement le front par le census q2.**
Ne pas reconstruire une factory de rectangle, Axis ou un arbre B par
descripteur. Garder les nœuds et leur ordre spatial, factoriser l'état du
census global et réutiliser ses tampons. Comparer à l'exhaustif les vrais
supports, intérieurs et coquilles, puis mesurer le coût complet aux trois
tailles. Le proposeur recommence encore depuis la racine : test préalable
de lentille et certificats partiels distincts sont à comparer sur ce chemin.

q3/q4 nécessitent aussi une réduction collective : deux rangées parallèles
laissent m² candidates transversales même avec tous les témoins ponctuels.
Ne pas développer ce résidu par défaut, ni conclure qu'il s'agit de m²
supports utiles. La génération canonique par plus longue arête et la
rétention par boule restent requises ; un rejet q4 ne tue pas q2.
Les tâches par handles permettent une distribution future, mais le moteur
courant est mono-thread. FULL, multi-CPU, GPU et contrats G4 restent ouverts.

L'auditeur A prépare le régime LiDAR simple et superposé. Le `terrain`
actuel est un slab aléatoire mince, pas un simulateur de capteur : ne pas
choisir une variante pour le LiDAR sur cette seule mesure. Ajouter un
corpus déclaré, poses/quantification et collisions explicites, en séparant
densification de la même zone et extension du trajet. Ses captures en
préparation restent indépendantes de la présente qualification.

## Sixième tranche publiée à 85015a8c — historique

La sixième tranche partage [le nuage et l'index Z](docs/P0_NUAGE_ET_INDEX_PARTAGES.md)
entre rectangles et seuils. 37 CTests passent en Release et Clang ASan/UBSan.
Les copies globales répétées et le scan des facteurs pour leurs boîtes
sont retirés du nouvel appel partagé. L'ancien adaptateur reste coûteux
par construction et ne doit pas alimenter un front WSPD. Les builds
`v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont réservés à ces preuves.

Les [66 essais clos](receipts/cloud_reuse_20260914/README.md) couvrent
8k/16k/32k, Kmax10, s8/10/12 à R32, puis R32/64/128 sur grille/déséquilibré.
À 32k/R32/s8, partage : grille 457–465 ms, déséquilibré 87–95 ms,
nappe environ 16 s. Les mêmes sorties et compteurs locaux sont vérifiés,
les préparations globales sont divisées exactement par R. En revanche,
les copies de restrictions grilles font ×3,94 puis ×3,97 quand R et n
doublent : ce verrou n'est pas résolu. La subdivision garde 35,6 millions
de candidates sur nappe32k. Tous les chronomètres, y compris un doublement
grille à ×5,62, sont conservés ; les temps restent bruités. Les builds sont
désormais épinglés. Aucun résultat ancien n'est réattribué à ce moteur.

**Priorité immédiate : pilote de front réel avec coût cumulé des facteurs.**
Ne pas prendre le partage du nuage pour une solution complète : Pool/Axis
gardent Ω(R|B|) sur une partition A_i×B et le découpage peut perdre des
témoins utiles aux minorants. R croissant avec n doit rester dans les
tests. Faire porter permutation et boîtes par les nœuds spatiaux certifiés ;
l'arbre de plages originales n'est qu'un raccord de compatibilité.
Tester les rejets sur produits ancêtres avant séparation complète et un
chemin sans tris/allocations lourds pour petits facteurs. Le nouvel audit
du front pur v4 montre leur nombre élevé, sans qualifier une WSPD v8.
Fixer explicitement la convention de s avant la comparaison s8/10/12.

Le seuil du census vient du rectangle du plan, l'index n'en possède plus.
Les restrictions restent liées au même rectangle exact. Les ordres B/Z
ne sont pas interchangeables ; l'auditeur précise en §9.4 pourquoi une
couverture compacte dans un ordre peut devenir linéaire dans un autre.
Raccorder Pool sans axe imposé, puis suspension mono, workers CPU et GPU.
Les continuations doivent suspendre sur saturation, jamais tronquer.
q3/q4 et FULL restent ouverts ; génération par plus longue arête et
rétention par boule (union des voies qui la conservent) sont inscrites au
plan. GCP non utilisé ; aucun contrat de tour n'est acquis.

## Cinquième tranche publiée à 3c29ea1e — historique

La cinquième tranche est qualifiée : [bornes préparées](docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md)
de 48 octets, 34 CTests Release/Clang ASan/UBSan, oracle de 1 424 cas et
comparaison des révisions sur [124 mesures](receipts/q2_prepared_bounds_20260914/README.md).
Compteurs et sorties inchangés ; baisse exploratoire du total Shared
de 4–10 % sur les médianes répétées grille/nappe32k/K10/s8. Les temps
doublent par 1,58–2,41 sur ces familles, sans borne générale ni gain
garanti sur tout nuage. Les deux builds `v8_prepared_bounds_20260913`
et `v8_prepared_bounds_sanitize_20260913` sont épinglés.

**Prochain chantier : séparer nuage/index global et contexte de rectangle.**
Ne pas recopier et revalider n sites pour chaque rectangle WSPD. Préserver
les trois compatibilités : même nuage pour index/requête, même contexte
rectangle/seuil pour crédits, même permutation B et index Z précis pour
plages/continuations. Porter les contre-fixtures de l'auditeur avant de
relâcher les contrôles de propriétaires actuels. Raccorder Pool seul par
les préfixes de classes (§9.2 de son audit), sans forcer le filtre axial.
Puis expliciter les transitions du curseur pour suspension mono testée,
workers CPU et enfin GPU ; des files pleines suspendent, jamais ne tronquent.
Partager l'index ne borne pas la somme des tailles de facteurs ni les sorties.

La déduplication des boules doit précéder les collectes répétées tout en
conservant les incidences des supports. q3/q4 et la tranche FULL minimale
restent prioritaires : ne pas repartir dans une série de micro-variantes
de la seule borne q2. Les contrats 50k portent sur la tour entière sur
G4 ; mono local, G4 et massif sont des qualifications distinctes.
GCP non utilisé.

## Quatrième tranche publiée à f4815cd4 — historique

La quatrième tranche implémente le [census q2](docs/P0_CENSUS_Q2_PARTAGE.md)
sur les résidus compacts. Le parcours partagé emploie les échappements
DFS proposés par l'auditeur : compte uniforme et curseur, sans arena de
continuations. Il est comparé au parcours individuel sur le même index de
tous les sites. La seconde collecte émet réellement intérieurs, coquille
et support/clé entière ; son temps et son callback sont inclus. Le gate
géométrique passe 277 cas ; les 31 CTests Release/ASan/UBSan et les lecteurs
normal/−O passent. Les [204 mesures](receipts/q2_census_20260913/README.md)
ferment quatre campagnes, avec 164 configurations distinctes. Les builds
`v8_census_20260913` et `v8_census_sanitize_20260913` sont épinglés.

Résultat : l'intersection paie son coût sur les grilles (environ 131 ms
à n32k/K10 avec census individuel, contre 3,74–3,79 s après Additive seul).
Sur nappes32k, le census partagé réduit les visites mais reste plus lent.
À 50k/K10, individuel après intersection : 176–186 ms sur grilles,
4,05–4,09 s sur nappes ; K5 sur nappes reste à environ 1,55 s. Ce ne sont
pas des tours FULL. Les doublements 8k/16k/32k sont inférieurs à ×4
dans ces familles ; aucune borne globale n'en découle.

Suite mono bornée : préparer les constantes de bornes `(a,B)` une fois
par tâche, vérifier les mêmes décisions/visites/sorties puis mesurer le
temps complet. Éviter de choisir automatiquement Shared sur les petits
résidus. Le coût Pool seul doit encore être comparé via une interface de
résidu non liée à l'axe. Ne pas prolonger ces variantes au détriment du
propriétaire global WSPD et de la tranche q3/q4/FULL minimale.

La déduplication globale des boules, q3/q4, le partage du propriétaire
sur une vraie WSPD et les parents FULL restent distincts. Ne pas reprendre
un compte saturé à un seuil supérieur comme s'il était exact. L'ancien
parcours à listes n'est pas conservé comme moteur parallèle. GCP non utilisé.

## Troisième tranche publiée à f5430f57

La troisième tranche ajoute le mode `Additive`, son intersection intégrée
avec un plan local q2 et la suppression des allocations actives aussitôt
remplacées. Lire le [contrat](docs/P0_ADDITION_ET_INTERSECTION.md) et les
[648 mesures](receipts/additive_q2_20260913/README.md). 26 CTests Release
et Clang ASan/UBSan passent, incluant 650 plans confrontés au nouveau juge,
frontières, cœur, copie de restriction, 7 contre-modèles et 15 rejets d'API.

Résultat précis : la nappe complète n32k/K10 garde 3 928 390 candidates
contre 6 483 670 auparavant, mais la sélection additive seule coûte
environ 238 ms contre 57 ms. Sur la grille, l'intersection Pool garde
114 716 candidates en 34–35 ms ; Pool seul garde 378 840 candidates en
2,4–2,6 ms. Ne pas confondre moins de candidates et moins de temps total.
La croissance mesurée à 8k/16k/32k est inférieure au quadratique dans
ces familles, pas une qualification générale. Aucun choix automatique
de filtre ni changement du mode `Independent` par défaut n'est installé.

**Prochain jalon : payer le census q2 et partager ses requêtes par blocs.**
Il doit indexer tous les sites, distinguer intérieurs stricts et coquille,
conserver les IDs et canoniser les boules. Comparer une référence de
requêtes individuelles avec le traitement partagé, coût total inclus.
L'auditeur a livré les [bornes sur trois boîtes](audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
et un [prototype de consommation indexée](../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md).
Ce dernier épingle `8e406f9b`, pas les nouvelles sources : ses grandes
mesures sont exploratoires. Ne pas précharger le cœur puis revoir ses IDs.

Les rangs filtrés B proposés pour composer deux plans déjà construits
restent une alternative au parcours intégré. Les fenêtres A/B peuvent
encore réduire le résidu des nappes ; ne pas retarder le census et la
tranche FULL minimale derrière une optimisation sans fin du seul cas axial.
Le vrai partage du propriétaire/index sur la WSPD, q3/q4, parents FULL,
parallélisation et contrats 50k/massif restent ouverts. GCP non utilisé.
Les builds `v8_additive_20260913` et `v8_additive_sanitize_20260913` sont
épinglés ; repartir dans un nouveau répertoire de construction.

## Deuxième tranche publiée à 8e406f9b

La deuxième tranche implémente `CreditBatch` (trois voies géométriques,
pas une tour K) et `AxisQ2Plan` (colonnes exactes + plages de l'index B).
Vingt et un CTests passent en Release GCC et Clang ASan/UBSan ;
[594 mesures appariées](receipts/shared_axis_20260913/README.md) couvrent
8k/16k/32k, Kmax5/10, s8/10/12 et deux ordres sur le même propriétaire.
Lire le [contrat expliqué](docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).

Acquis bornés : préparation Tubes divisée par trois à plans identiques ;
résidu q2 des nappes complètes passant de 256 millions à 6,48 millions
de paires à n32k. Le filtre axial coûte environ 57 ms, hors propriétaire,
sans expansion/census. Il peut faire moins bien que Pool sur les cubes,
et une rotation peut lui faire conserver toutes les paires. Aucun
algorithme général ni contrat de tour n'est acquis.

Suite prioritaire : additionner les colonnes exactes disjointes dans les
requêtes d'index, supprimer les tableaux initialisés puis remplacés,
comparer queues/fenêtres A/B et intersections de résidus. Puis implémenter
le consommateur q2 exact avec un index sur tous les sites, pas un scan
de n sites pour chaque paire ; ne pas recompter le cœur implicitement.
Le partage de validation sur toute la WSPD reste à faire. Les groupes
collectifs q3/q4 restent une piste distincte à qualifier avec l'auditeur.

Les pannes d'affectation de plans/batches sont désormais injectées et
couvertes par la garantie forte. Les reçus appariés vérifient sources,
matrices, identités, convention de checksum et provenances déclarées.
Les captures préliminaires exclues sont conservées avec leur motif.
Les nouveaux builds `v8_shared_axis_20260913` et sa variante sanitizer
sont épinglés ; ne pas les écraser. Aucun fichier v6/v7 ni statut formel
n'est modifié par cette tranche. GCP non utilisé.

## Première tranche publiée : historique à 3589a2c9

La bibliothèque `mhgp8_p0`, ses deux gates C++ et sa sonde sont implémentées.
Pool/DualBlocks/Tubes produisent des crédits sûrs et un résidu compact
sur un seul rectangle séparé. Huit CTests passent en Release GCC et en
Debug Clang ASan/UBSan ; 729 mesures mono sont conservées avec sources,
commandes, entrée et compteurs. Voir le
[contrat](docs/P0_CREDITS_LOCAUX.md) et les
[résultats](receipts/p0_local_credits_20260913/README.md).

Trois défauts d'audit sont corrigés avant livraison : le propriétaire
n'est plus copiable/réaffectable derrière un plan, ses coordonnées sont
copiées dans un stockage privé sans alias mutable hérité, et les reçus sont
confrontés strictement aux commandes avec sorties brutes et hashes de
fermeture. La première capture, antérieure aux correctifs, reste dans
`first_pass_pre_owner_fix/`, la deuxième dans `second_pass_pre_alias_fix/` ;
seules les nouvelles captures r3, copie des coordonnées incluse, font autorité.

La préparation quadratique systématique a une alternative effective :
les tubes coûtent O(m log m) par facteur. Mais **ni cette borne locale,
ni les petites latences ne ferment P0**. Les nappes gardent toutes les
paires ; sur les grilles q3/q4, les tubes sont rapides mais créditent
moins que DualBlocks. Le pool global peut rater les régions utiles :
contre-fixture des rails à 2 718 sites, q4, 1 846 881 paires contre 2 916
pour DualBlocks et Tubes. Ne pas généraliser un gagnant unique.

Suite prioritaire : partager le tri des tubes entre voies, partager la
validation du nuage, puis raffiner les produits résiduels difficiles en
préservant couverture et IDs. Il faut tester notamment les nappes, où
l'universalité sur toute la boîte opposée est trop restrictive. Une
combinaison de crédits utilise leur maximum, jamais leur somme sans
disjonction. Le cœur n'a pas encore ses IDs exportés : ne pas le recompter
implicitement dans le census. Mesurer ensuite le consommateur exact q2
minimal avant d'étendre census/FULL et la parallélisation.

Le vrai constructeur WSPD et sa comparaison s8/10/12, les voies complètes
q3/q4, census, parents et tour FULL restent à implémenter. Les benchmarks
actuels n'effectuent pas ce travail. Les builds v8 datés du 13 septembre
et les reçus sont épinglés. Aucun statut formel ni fichier v6/v7 modifié
par cette tranche ; aucun GPU/GCP utilisé.

## Historique de l'audit d'ouverture

La demande remplace l'optimisation incrémentale v7 par un audit complet
avant reconstruction. La base publiée examinée est main `dc57ffd5`.
Les modifications v6/v7 déjà présentes sont conservées, sans les inclure
dans cette ouverture v8. Le delta privé fused_history du 11 septembre
reste distinct des résultats publiés ; sa publication n'est pas poursuivie
dans ce changement de cap. Aucun processus de benchmark restant constaté
à l'ouverture du 13 septembre ; GCP non utilisé.

L'audit d'ouverture est rédigé : [synthèse](docs/AUDIT_V7_SYNTHESE.md),
[exposé pédagogique](docs/ALGORITHME_EXPLIQUE.md), quatre rapports détaillés
et [plan de refonte](docs/PLAN_DE_REFONTE.md). Il couvre WSPD et témoins,
supports q2/q3/q4, census, rattachements, histoires, verticales,
parallélisation, mémoire, tests et livraison. Sa portée n'est pas une
certification ligne par ligne de tout le corpus ni une réexécution C++.

Décisions principales : conserver le contrat FULL avec vrais parents,
partager les objets géométriques, préparer les rattachements indépendamment
de l'histoire, découper l'intérieur des gros rectangles, reconstruire
les histoires par contractions parallèles, puis réemployer les marques et
index. Le détail distingue explicitement régularité, coquilles, sorties
quadratiques et propositions encore sans qualification.

Les contrats restent ouverts : 50k 1..10/1..5 en environ 419/34 s sur les
dernières complétions publiées, pas de nouvelle capture 50k des prototypes
récents, pas de chaîne intégralement GPU, pas de dizaines de millions.

L'inventaire épingle la base publiée et les deltas locaux. Six lecteurs
de reçus clos et deux recalculs rationnels ont passé ; les contrôles de
livraison sont dans [PUBLICATION_CHECKS](receipts/audit_v7_20260913/PUBLICATION_CHECKS.json).
Le contrôleur documentaire couvre désormais la v8 et possède un test
positif/négatif de découverte, sans inclure les futurs audits indépendants.
Aucun statut formel modifié, aucun moteur compilé, GCP non utilisé.

Suite réordonnée sur demande explicite du 13 septembre : **P0 d'abord,
supprimer la préparation systématique O(|A|²+|B|²)** après l'échec des
témoins universels. Comparer minorants issus de petits ensembles,
parcours conjoints de blocs, requêtes géométriques groupées et sélection
directe de sous-produits ; aucune solution n'est imposée. Le
[plan](docs/PLAN_DE_REFONTE.md) fixe les preuves et mesures de travail
total, coût aval compris, avant les campagnes CPU/GPU. P0 reste ouverte.

Avant tout port, lire aussi [VERROUS_ARCHITECTURE](docs/VERROUS_ARCHITECTURE.md).
La note conserve les cinq obstacles suivants : recherches spatiales
répétées, triangles de départ × voisinages q3/q4, MEB/descentes répétées,
histoire/export centraux, puis résidence et échanges CPU/GPU. Elle fixe
les sources, nuances et critères de validation pour le futur développeur.
P0 garde son premier rang ; B1–B5 restent ouverts et ne sont pas des
impossibilités intrinsèques de HGP. Aucun nouveau résultat n'est revendiqué.

La tranche FULL minimale et les petits juges servent cette comparaison ;
ne pas la repousser derrière un port général de la v7. Garder les tailles
8k/16k/32k et s8/10/12. Aucun moteur n'est modifié par cette décision.
Ne pas reprendre le chantier fused v7 comme si le changement de cap
n'avait pas eu lieu. Le
[journal v8](../audits/COORDINATION_MORSEHGP3D_V8.md) porte les questions
à l'auditeur indépendant et la coordination d'index.
