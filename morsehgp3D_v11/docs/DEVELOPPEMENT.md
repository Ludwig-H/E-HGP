# État courant du développement v11

Reprise développeur sur instruction du 2 octobre 2026, avec autorisation
GCP G4. La tranche des fondations est qualifiée sur le commit publié
`a971806679a1c68519249bb28c5dac9533a43f59`. Le produit v10 et le worktree
principal chargé de travaux d'autres acteurs restent préservés.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
mode=implementation_v11_catalogue
public_status=not_claimed
```

## Reprise u21/u24 — qualification et mesures closes

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
2. Retarder le calcul du niveau q4 jusqu'à l'émission, en conservant les
   centres exacts pour propriété et census, et partager les coefficients des
   préfixes. Limiter cette séparation à q4 : le niveau q3 utilise sa forme
   réduite, alors que le carré générique de N3 atteindrait un degré 10 hors
   du budget de Level. Ne pas écarter un q3 obtus avant ses prolongements q4.
3. Distinguer les temps des deux passes, du tri exact et de l'assemblage
   avant de choisir une optimisation de ces phases. Les compteurs actuels
   ne permettent pas d'attribuer les 15,478 s à l'une d'elles.

Ce sont des pistes issues du code et du travail mesuré, pas des gains acquis.
Les deux passes conservent pour l'instant leur contrat de réservation exacte.

Pour le levier q4, la séparation proposée est un candidat fermé ancre/N/D,
sans niveau, matérialisé après census, canonicalisation et admission. Les
fabriques publiques de sphères garderaient leur résultat complet ; les deux
passes paieraient chacune le niveau des seules émissions qmin4. Le port devra
confronter les sorties canoniques et compter séparément ces constructions de
niveaux, sans changer les refus ni les allocations. Cette conception n'est
pas encore implémentée et ne donne aucun gain acquis.

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

Ensuite viennent l'index et FULL, core/cover ensembliste, projection exclusive,
condensation et comparaison effective à `sklearn.cluster.HDBSCAN`.

L'audit v10 est consolidé, sans prétention d'exhaustivité : les rapports
privés L09 et L11–L16 absents restent listés. Le modèle pondéré de FULL,
les cas EOM proches d'une égalité et les filtres flottants restent ouverts.
La clôture des descendants après une sortie normale reste aussi ouverte
**dans la matrice** : le worker ferme leur groupe en fin de commande,
mais les sondes portent `isolation=not_certified`. Aucun chrono de ces
campagnes fonctionnelles ne qualifie une mesure de performance isolée.

La cible reste FULL sur trames LiDAR, K5 puis K10, avec objectif 100 ms.
Aucune réussite de fondation ni estimation de cycles ne prouve ce contrat.
