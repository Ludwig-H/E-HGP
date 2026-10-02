# État courant du développement v11

Reprise développeur sur instruction du 2 octobre 2026, avec autorisation
GCP G4. La tranche des fondations est qualifiée sur le commit publié
`a971806679a1c68519249bb28c5dac9533a43f59`. Le produit v10 et le worktree
principal chargé de travaux d'autres acteurs restent préservés.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
mode=implementation_v11_full_forests
public_status=not_claimed
```

La priorité courante est le contrat **200 ms pour FULL K1..5 sur G4**,
puis K1..10 ; hiérarchie de points et HDBSCAN/Zoltan viennent après.
L’inspiration critique de toute la v10 est explicitement autorisée.

## MEB bornée — qualification close à 25792084e

La [MEB exacte locale et son raccord au census](MEB.md) sont implémentés
dans `src/tower`. Support strict local, parties de 1 à 12 sites, refus
transactionnels et aucune allocation dans la MEB. Les nouvelles revues
indépendantes `a7a38137c` sont intégrées ; leurs pistes de bornes discrètes
restent séparées. La première campagne G4 `meb1` détecte une ancre temporaire empruntée
par la sérialisation du banc C++20 : ses deux portes IO échouent, tandis
que MEB native et Fraction passent. Le banc garde désormais une ancre
possédée ; le décodeur signé était correct. `meb3` passe **1266/1266**,
complément ASan18 **55/55**, 141 mutants et **18/18 mesures**. Les 864
requêtes donnent 216 réponses complètes et 648 saturées, sans divergence
interprofils. Sur LiDAR, 48 MEB+census prennent 1,377–1,777 ms en u21 et
1,387–1,747 ms en u24 ; ce lot artificiel ne qualifie aucune descente FULL.
[Preuves, échecs initiaux et chronos](../receipts/meb_20261002/README.md).
G4 arrêtée, clés retirées ; `meb2` est un échec de capacité sans worker,
avec vérification externe de la cible arrêtée, sans nouvelle génération.

## J2 et domaine FULL — qualification close à7f1922c77

Les [rejets J2](CENTER_REGION.md) et le [domaine FULL](FULL_DOMAIN.md)
passent1398/1398 contrôles G4, le complément ASan18 73/73 et154 mutants.
La [capture](../receipts/center_region_20261002/README.md) conserve le délai
externe820s du banc mono :14 succès,15 délais30s,2 omissions causales et
5 unités sans résultat persistant. Les13 résultats appariés au banc précédent
ont exactement les mêmes octets. Catalogue K5/u21 :21,734s sur08/0,
17,374s sur08/100 ; u24 :21,996s et17,509s. Pas de résultat u21/u24 pour08/200.
La cible a été arrêtée et les clés retirées ; aucun temps FULL n'est acquis.

Le [catalogue parallèle](CATALOGUE_PARALLELE.md), les cellules et descentes
sont qualifiés à `c1046dfc7` :1779/1779 + ASan18 107/107,189 mutants.
La [capture](../receipts/catalogue_parallel_20261002/README.md) conserve
30 succès K5 et six délais K10 à15s, sans divergence des sorties ou du
travail géométrique. Sur les trois LiDAR entiers en W48, premiers essais :
u21 4,482/3,075/3,996s ; u24 4,637/3,247/4,215s. Catalogue seulement.
Le tri coûte0,9–1,4s ; sur08/0/u21, une seule tâche prend presque tout le
mur des phases de comptage et remplissage, chacune environ1,53s.

Les [forêts et verticales](FULL_FORESTS.md) sont implémentées à80e77544e.
La première campagne `full1` échoue aux deux portes IO : le test consommait
un itérateur pour XYZ, puis produisait des IDs vides. Les autres portes
natives passent, mais cette qualification globale reste en échec. Le
correctif c6ca345e0 matérialise l'ordre une fois et conserve les diagnostics
de l'enfant. Aucun chrono ni comparaison v10 native n'a suivi cet échec.
Le rejeu corrigé est distinct ; aucun succès partiel ne lui est transféré.

La prochaine tranche prépare une [ablation cache J2/tri indirect](CATALOGUE_OPTIMISATIONS.md),
options désactivées par défaut, puis le même banc FULL avec les deux options.
Le déséquilibre des suffixes est étudié séparément, sans changer la recherche
ni confondre sommes de temps de tâches et temps mur.

## Index global — qualification et mesures courantes

L'[index global exact](INDEX.md) possède le Cloud et un arbre équilibré de
plages Morton. Les requêtes renvoient K témoins stricts ou tout I/U ; les
contacts restent exacts, même hors boîte des supports. Construction et
résultats sont budgétés, déplacements et refus transactionnels. Les poids
sont conservés, le census compte seulement les sites géométriques.
Source **`e8520481d`**, G4 : Release 251/251, ASan24/TSan21/profils21/24
176/176 chacun, poison 177/177, supplément num/index ASan18 36/36 ;
131 mutants détectés, dont deux refus de compilation attendus dans core.
Index : 845 contrôles natifs et 1 010 requêtes/36 020 contrôles Fraction par
profil normal/−O. Bornes num : 183 contrôles natifs et 391 cas Fraction.

| Trame sans sol entière | Arbre u21 | 64 census u21 | Arbre u24 | 64 census u24 |
| --- | ---: | ---: | ---: | ---: |
| 08/000000 | 0,400 ms | 0,810 ms | 0,401 ms | 0,825 ms |
| 08/000100 | 0,341 ms | 0,621 ms | 0,339 ms | 0,620 ms |
| 08/000200 | 0,418 ms | 0,703 ms | 0,409 ms | 0,700 ms |

18/18 essais conformes : six entrées entières, mêmes coordonnées 1 mm et IDs
aux trois profils, une répétition. Les 1 152 réponses sont contrôlées par
scan ; les six comparaisons interprofils ont les mêmes sorties et compteurs.
Les chronos séparent Cloud, arbre, factories, census et scan témoin. Ce sont
des requêtes choisies, sans descente MEB, catalogue ni tour FULL. Les temps
catalogue restent ceux de la section suivante ; le contrat 100 ms reste ouvert.
[Reçus et lecteur LIVE](../receipts/index_20261002/README.md). G4 arrêtée,
clés retirées ; aucun natif local. Plan : `bench/plans/index_g4.json`.

## Niveau q4 différé — qualification et mesures courantes

Source **`ffc2ff95f0ae7296bdc522df81df34c58c3fdf47`** : Release 229/229,
ASan/UBSan u24, TSan u21 et profils 21/24 154/154 chacun, poison 155/155 ;
119 mutants détectés, deux refus de compilation attendus dans core, aucun
signal/délai pris pour une détection. Complément num ASan/UBSan u18 : 14/14.
Candidate 831 contrôles ; Fraction 528 géométries et 160 entiers, 11 838 contrôles.

Le niveau q4 est calculé seulement après tous les rejets et l'admission du
support canonique. La factory publique Sphere reste complète ; q2/q3 gardent
leurs formules, sans nouvelles allocations ni changement des paramètres.
Sur LiDAR, environ 99,7 % des niveaux candidats sont évités. Les deux passes
conservent leurs comptes identiques ; les 15 sorties terminées égalent les
précédentes octet pour octet, avec mêmes neuf compteurs géométriques et mémoire.

| LiDAR entier sans sol, K5 / leaf16 / CPU mono | u18 | u21 | u24 |
| --- | ---: | ---: | ---: |
| 08/000000, 39 885 sites | 23,380 s | 24,962 s | 24,794 s |
| 08/000100, 35 551 sites | 18,457 s | 19,777 s | 19,673 s |
| 08/000200, 45 845 sites | 21,568 s | 23,101 s | 23,176 s |

Uniforme 8k : 7,180/7,743/7,818 s ; 16k : 15,104/16,345/16,404 s ; 32k/K5
expire à 30 s aux trois profils. Tous les K10 joués expirent aussi. Le banc
clos conserve 15 réussites, 18 délais et 3 omissions. Un essai par cas/profil,
temps de l'API catalogue (deux passes/tri/sorties mémoire), hors préparation,
segmentation et sérialisation. Rapports observés ancien/nouveau ×1,033–1,053,
sans preuve de gain statistique stable. Les trois trames sont d'une séquence.
Le contrat FULL de 100 ms reste ouvert ; FULL natif et GPU sont absents.

[Reçus et lecteur LIVE](../receipts/catalogue_q4_20261002/README.md) normal/−O
passent. G4 arrêtée, clés retirées, verrou libéré ; aucun natif local.
Le plan courant est `bench/plans/q4_levels_g4.json`. L'ancienne recette de
profils sans `--supplement`, conservée à `9df774947`, est retirée de l'arbre
courant après le constat de l'audit `737313a96` ; les reçus sont inchangés. Le prochain
raccord ouvre l'index global exact pour les descentes puis FULL, décrit plus bas.

## Reprise u21/u24 — qualification précédente

La source **`9df77494732b03ddf11dbcf1dcb11d96bef54a3b`** passe sa matrice G4 :
Release u18 **227/227**, profils 21/24 **152/152** chacun, ASan/UBSan u24 **152/152**,
TSan u21 **152/152**, poison u21 **153/153**, mutants 12/12 et style 2/2. Clang est
absent. Les 116 mutations sont détectées : 78 core, 13 num, 16 cloud, 9 catalogue ;
les deux refus de compilation attendus restent dans core, aucun signal/délai.
Num ajoute la vérification explicite de `side`, la certification d'arité,
207 contrôles de voies natives/larges et 7 526 contrôles Fraction par oracle.
Le complément **`d77e4b77c`** passe **13/13** portes du seul module num sous
ASan/UBSan u18, dont q3 natif : 207 contrôles de voies et les deux oracles
Fraction de 7 526 contrôles. Le code produit et les tests C++ sont identiques
à `9df774947` ; aucune mesure de performance n'est ajoutée par ce complément.
Les petites entrées aux limites 21/24 donnent 11 boules/28 incidences/4 niveaux
exactement et refusent `2^B` sans publication partielle.

Le défaut devient **u21** ; u18 et u24 restent disponibles. Les voies q1/q2/q4
emploient `i128` aux trois profils grâce à des bornes sur chaque intermédiaire.
Q3 garde `Wide` en 21/24 : l'annulation d'une puissance finale ne garantit pas
que son premier produit tienne en 128 bits. La grille du banc reste 1 mm, avec
les mêmes nuages entiers et IDs ; élargir le type ne requantifie aucune entrée.

| Entrée entière, K5 | u18 | u21 | u24 |
| --- | ---: | ---: | ---: |
| Uniforme 8k | 7,420 s | 8,066 s | 8,090 s |
| Uniforme 16k | 15,619 s | 16,920 s | 17,076 s |
| Uniforme 32k | délai 30 s | délai 30 s | délai 30 s |
| LiDAR 08/000000 sans sol, 39 885 sites | 24,524 s | 25,847 s | 25,915 s |
| LiDAR 08/000100 sans sol, 35 551 sites | 19,440 s | 20,551 s | 20,508 s |
| LiDAR 08/000200 sans sol, 45 845 sites | 22,674 s | 24,051 s | 24,094 s |

Un essai par entrée/profil, CPU mono, leaf16/max_leaf256/budget 8 GiB ; temps
de l'appel catalogue, deux passes/tri/sorties mémoire compris. Lecture,
Cloud, segmentation, sérialisation et normalisation Python sont hors de ce
chrono. Le plafond 30 s concerne le processus entier. **Tous les K10 joués
expirent** ; les trois K10/32k sont omis après l'échec K5 de leur profil.
33 tentatives sur 36 : 15 réussites, 18 délais, 3 omissions. Le banc est clos mais
non conforme à son calendrier complet ; ses échecs ne sont pas effacés.

Les cinq entrées terminées ont une empreinte sémantique et les neuf compteurs
géométriques identiques en 18/21/24. Leurs cinq sorties u18 et comptes égalent
également les sept mesures terminées de `catalogue3` (trois essais sur 8k).
Rapports de durées observés contre cette capture : ×1,06–×1,11 ; une seule
nouvelle répétition et des sessions non appariées ne qualifient pas un gain
stable. Réservations Buffer LiDAR : 235,91–297,65 Mo en 18, 259,73–326,51 Mo en 21,
276,03–346,57 Mo en 24 ; pas RSS. Ces trois trames sont d'une seule séquence.

[Reçus et lecteur LIVE](../receipts/catalogue_profiles_20261002/README.md)
passent normal/−O. La génération G4 est certifiée arrêtée, clé privée/OS Login
retirée et verrou libéré. Le préflight 4 800 s avait été refusé avant démarrage ;
le lancement réel respecte la garde existante 3 600 s. La seconde session num u18
est elle aussi certifiée arrêtée, clés retirées et verrou libéré. Aucun calcul natif local.
**Le contrat FULL de 100 ms reste non acquis ; FULL natif est toujours absent.**

## Livraison

- `num` : entiers à budget calculé, niveaux rationnels exacts, points
  validés, sphères q1–q4, puissance, orientation et convexité. Aucune
  approximation flottante ni clé canonique de boule dans cette tranche.
- `cloud` : stockage privé, vues constantes, conservation de tous les IDs
  et des multiplicités, tri radix séquentiel, allocations budgétées.
  Copie et affectations interdites ; déplacement sans allocation.
- Harnais : statut réel des signaux, sorties non UTF-8 conservées sous
  forme échappée, injection mémoire compatible avec GCC11/TSan,
  interruption globale et échéances des sondes sanitizer corrigées.
- Quatre fixtures de projection : distinction cover/MR₂-bord, coupe
  ouverte du mémo et discontinuité de l'attache premier-cover/LCA.
  Les deux étages de référence sont jugés ; aucun HDBSCAN exécuté ici.
- [Mathématiques](MATHEMATIQUES.md), [conception](CONCEPTION_MOTEUR.md)
  et [synthèse de l'audit v10](AUDIT_V10_SYNTHESE.md) consolidées.

## Qualification des fondations — historique

Source `a97180667`, session `v11.20261002.reprise3`, GCC 11.4, Python 3.10.12.
Tests **CPU sur VM G4** ; aucune donnée LiDAR et aucun calcul GPU.

| Configuration | Portes passées |
| --- | ---: |
| Release, référence complète comprise | 205/205 |
| ASan + UBSan | 130/130 |
| TSan | 130/130 |
| Profil 21 bits | 130/130 |
| Profil 24 bits | 130/130 |
| Tampons empoisonnés | 131/131 |
| Style normal/−O | 2/2 |
| Manifestes et campagnes des mutants | 9/9 |

Clang est absent, déclaré facultatif par le plan : aucune qualification
Clang annoncée. Les oracles interrogeant les binaires tournent dans chaque
configuration ; seule la référence Python indépendante du profil est
exclue des répétitions. Le contrôle nommé LiDAR n'est qu'une sentinelle
présence de dossier, pas une évaluation de données.

Chaque appel de l'oracle numérique confronte 504 géométries et 160 paires
Wide, avec 50 dégénérescences et 6 164 contrôles, en Python normal/−O.
Les six groupes natifs jouent 145 contrôles par profil. Au profil 18,
les 103 mutants sont détectés : core 78, num 9, cloud 16 ; 101 par juge
exécuté, deux par refus de compilation attendu, zéro signal ou délai.
Ces résultats qualifient les primitives et le propriétaire, pas le
catalogue, la tour FULL ni leurs performances.

[Reçu final](../receipts/developpement_20261002/reprise3/receipt.json) ;
[matrice détaillée](../receipts/developpement_20261002/reprise3/matrix.json).
Archive originale des résultats conservée avec son hash. Arrêt de la
**génération exacte** certifié à 10:42 UTC ; clés temporaires supprimées,
clé OS Login retirée, verrou libéré. Le plan garde une durée GCE de 3600 s,
un arrêt invité vérifié et une commande bornée à 1500 s.

Les premiers échecs restent explicitement conservés :
[reprise1](../receipts/developpement_20261002/reprise1/receipt.json)
(collecteur UTF-8 et construction TSan), puis
[reprise2](../receipts/developpement_20261002/reprise2/receipt.json)
(verdict 21/28, comparaison de largeurs dans un test et trois mutants
non compilables). Le succès final ne remplace pas ces captures.
Le [lecteur des reçus](../receipts/developpement_20261002/check.py) exige
les bruts locaux hachés : preuve LIVE, pas archive autonome.

## Catalogue : qualification et mesures G4

Le [catalogue séquentiel](CATALOGUE.md) est maintenant implémenté et qualifié
sur G4 à `e6fe34cb0` : listes K-certifiées, boîtes de centres T=0,
feuilles à capacité déclarée, census et coquilles complets, support canonique,
deux passes pour réserver les sorties exactes. Aucun SiteTree ni ordonnanceur
n'est requis. Son nouveau juge Gram/Fraction est indépendant des formules R2 ;
la référence constructive historique reste limitée à 21 bits.
Aucun port implicite du raffinement T=6 vers B24.

La session `catalogue3` passe Release **220/220**, ASan/UBSan, TSan, B21 et B24
**145/145** chacun, poison **146/146**, style 2/2 et mutants 12/12.
Le juge catalogue contrôle 378 requêtes, dont 358 acceptées et 20 refusées,
avec 132 505 contrôles et 127 relations métamorphiques par profil, normal/−O.
Les neuf mutants catalogue sont tués par la porte, sans construction ratée,
signal ou délai ; les 103 mutants du socle/num/cloud sont rejoués.
La coquille qmin4/m5 ferme le retour positif de la recherche canonique q4.
Le témoin inter-K passe dans les deux étages de référence ; les tentatives
malformées du banc gardent flux, durée et erreur structurée (20 cas factices,
6 échecs persistés, sans appel natif pour ces seules portes de collecteur).

`catalogue2` (`f391bf13e`) reste la qualification précédente : Release 218/218,
autres profils 143/143, poison 144/144, huit mutants catalogue.
Le premier essai `catalogue1` est conservé : le clone du lanceur de mutants
omettait `bench/` ; son témoin ne configurait pas et aucun mutant catalogue
n'y a été jugé. Les trois générations sont certifiées arrêtées, clés retirées
et verrous libérés ; la dernière clôture est consignée dans le reçu `catalogue3`.

Le banc dédié couvre trois synthétiques 8k/16k/32k et les trois trames sans sol
08/000000, 08/000100, 08/000200 entières (39 885/35 551/45 845 sites).
Coordonnées 1 mm/u18 et vrais IDs de retours ont été revérifiés contre les bruts,
masques et correspondances historiques. Aucun octet LiDAR n'est ajouté à Git.
La segmentation et la préparation de ces entrées sont hors de ce nouveau chrono.
Deux commandes worker distinctes séparent matrice et mesures ; la fermeture du
groupe de la première précède le banc. Temps de lecture, Cloud, appel catalogue
et processus sont distincts ; réservations Buffer ne signifient pas RSS.

À `leaf_size=32`, la seule mesure terminée est le synthétique 8k/K5 :
**15,478 s** de catalogue CPU, 597 998 boules, 2 895 136 incidences,
133 416 208 octets de pic réservé (Cloud compris). Une passe logique compte
144 086 254 préfixes et 4 106 480 candidats jugés ; deux passes sont payées.
8k/K10, 16k/K5, 32k/K5 et les trois trames LiDAR/K5 atteignent le plafond
processus de 30 s sans catalogue terminé. Sur les LiDAR, Cloud prend
0,857/0,977/1,151 ms pour 35 551/39 885/45 845 sites ; ce n'est pas un temps HGP.
Sept tentatives, une réussite, six délais ; 29 essais explicitement non joués.

L'ablation `leaf_size=16`, **sans changement de moteur C++**, donne :

| Entrée entière, 1 mm/u18 | Sites | Catalogue K5 | K10 |
| --- | ---: | ---: | --- |
| Synthétique uniforme 8k | 8 000 | 8,240 s, médiane de 3 | Plafond processus 30 s |
| Synthétique uniforme 16k | 16 000 | 17,310 s, un essai | Plafond processus 30 s |
| Synthétique uniforme 32k | 32 000 | Plafond processus 30 s | Non joué après le délai K5 |
| LiDAR 08/000000 sans sol | 39 885 | 26,018 s, un essai | Plafond processus 30 s |
| LiDAR 08/000100 sans sol | 35 551 | 20,741 s, un essai | Plafond processus 30 s |
| LiDAR 08/000200 sans sol | 45 845 | 24,093 s, un essai | Plafond processus 30 s |

Les temps sont ceux de l'appel catalogue CPU mono (deux passes, tri et sorties
en mémoire), hors lecture/Cloud/segmentation/sérialisation ; aucun FULL ni GPU.
Un plafond processus ne donne pas une durée finale du catalogue. Treize
tentatives : sept réussites, six délais, 23 essais non joués selon le plan.
Les trois trames proviennent d'une seule séquence. Le lot n'est pas conforme
au calendrier complet, même si sa matrice fonctionnelle passe.

Sur 8k/K5, les trois temps vont de 8,210 à 8,240 s : rapport observé ×1,88
contre l'unique essai leaf32, sans intervalle statistique apparié. Les quatre
empreintes de sortie sont identiques (`2671f84a…f74a`), ainsi que boules,
niveaux, incidences et pic réservé. Par passe, les préfixes passent de
144,09 à 85,49 millions et le census de 61,81 à 18,53 millions, mais les
filtres montent de 28,94 à 64,53 millions et les feuilles de 12 507 à 77 934.
Sur LiDAR K5/leaf16, 1,10–1,41 million de boules et 235,91–297,65 Mo réservés
sont produits. Les deux tailles synthétiques terminées ne prouvent aucune
borne générale de croissance.

**Le contrat FULL de 100 ms n'est pas acquis : le catalogue seul le dépasse
largement et FULL reste absent.** Les [reçus compacts et leur lecteur LIVE](../receipts/catalogue_20261002/README.md)
conservent échecs, entrées hachées et provenance des builds. Aucun réglage
par défaut du produit n'est changé sur la foi de cette seule ablation.

## Suite et limites actives

Après l'ablation des feuilles, les leviers isolables sont :

1. La reprise u21/u24 est close : mêmes sorties et travail, coûts propres aux
   trois profils. La voie `i128` conserve les garanties numériques mais son
   effet observé reste modeste ; aucune borne de croissance globale acquise.
2. Le niveau q4 différé est qualifié à `ffc2ff95f` : les centres exacts
   servent aux rejets, le niveau aux seules émissions canoniques. Le coût mur
   baisse peu dans cette série ; le partage des préfixes reste une autre tranche.
   La séparation ne s'étend pas à q3, qui garde sa forme réduite ; un préfixe
   q3 obtus reste disponible pour ses prolongements q4.
3. Distinguer les temps des deux passes, du tri exact et de l'assemblage
   avant de choisir une optimisation de ces phases. Les compteurs actuels
   ne permettent pas d'attribuer les 15,478 s à l'une d'elles.

Ce sont des pistes issues du code et du travail mesuré, pas des gains acquis.
Les deux passes conservent pour l'instant leur contrat de réservation exacte.

Le port q4 différé est qualifié sur G4 à **`ffc2ff95f`** :
candidat fermé ancre/N/D, cinq prédicats exacts partagés, matérialisation après
census, canonicalisation et admission. La factory publique Sphere reste complète.
Les deux passes comptent séparément centres q4 non dégénérés et niveaux matérialisés ;
aucune allocation ni formule q2/q3 modifiée. Le suivi indépendant `d40585570`
fixe l'ordre des gardes, le maintien des refus et la nécessité de conserver les
fractions non réduites. Son minorant coplanaire familial reste une autre tranche.
Les nouvelles portes confrontent ces compteurs aux nombres qmin4 de l'oracle,
les prédicats du candidat au modèle Gram/Fraction et les anciennes sorties exactes.

L'audit indépendant `e739d3c8c`, reçu après la capture, confirme la lecture
favorable et isole le [travail des coquilles nombreuses](../receipts/audit_independant_20261002/catalogue_boundary_work_review_4/README.md).
Les coquilles entières de rayons 5/15/35 donnent 30/150/270 sites : census
répété de la même boule, 20 822 900 préfixes par passe dans la feuille centrale
du deuxième cas, et refus de capacité par défaut dans le troisième. Ces faits
autonomes ne sont pas des mesures natives ni une cause démontrée des temps LiDAR.
Ils deviennent des diagnostics ciblés à porter au prochain lot utile. La piste
d'un certificat « toute la liste sur une sphère de qmin≤3 » permettrait de
couper l'arité q4 entière ; son coût et son port restent à qualifier. Ne pas
augmenter max_leaf ni tronquer la coquille pour contourner ce problème.

Le suivi `19ec9de79` précise les contrats des futurs certificats de familles :
cosphéricité de toute la liste certifiée, propriété du centre pour déduire les
populations globales, ancre minimale seulement pour qmin4. Le test positif de
puissance d'une extension q4 serait nécessaire, pas suffisant. Ces propositions
ne sont pas portées dans la reprise u21/u24 ; aucun travail discret n'en est retiré.

L'index, la MEB, le domaine commun, les cellules, la localisation et les
descentes sont maintenant qualifiés à leurs sources propres. La qualification
des plateaux N-aires et verticales fermées est en cours. Le support global dans le domaine possédé
suffit à l'identité ; une clé PGCD n'est pas nécessaire à ce raccord.
Les portes compareront toute la forêt à Definition, notamment les connexions
extérieures entre morceaux locaux et les lectures avant/après un plateau.
Un hit catalogue complet peut avoir p>=k ; une MEB hors CatK peut être valide.
Les fixtures permanentes de localisation gardent ces deux cas et la paire
globale ajoutée à la présentation locale de centre(5,5,0), niveau25.

Puis viennent core/cover ensembliste, projection exclusive, condensation et
comparaison effective à `sklearn.cluster.HDBSCAN`.

L'audit v10 est consolidé, sans prétention d'exhaustivité : les rapports
privés L09 et L11–L16 absents restent listés. Le modèle pondéré de FULL,
les cas EOM proches d'une égalité et les filtres flottants restent ouverts.
La clôture des descendants après une sortie normale reste aussi ouverte
**dans la matrice** : le worker ferme leur groupe en fin de commande,
mais les sondes portent `isolation=not_certified`. Aucun chrono de ces
campagnes fonctionnelles ne qualifie une mesure de performance isolée.

La cible reste FULL sur trames LiDAR, K5 puis K10, jalon200ms puis objectif100ms.
Aucune réussite de fondation ni estimation de cycles ne prouve ce contrat.
