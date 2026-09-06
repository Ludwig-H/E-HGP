# Qualification indépendante du raccord MEB à FULL

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé.

**Le raccord `a946e31d` / helper `f922544b`, maintenant publié par `20b28b1d`, passe un nouveau rejeu indépendant O2 et ASan/UBSan.** Aucun résultat privé n’est transféré. Les 33 champs historiques, les forêts et les préfixes de refus restent identiques à la référence L sur son corpus. Les deux nouveaux ordres K9/K10 sont jugés directement par Gamma rationnel. Cette qualification concerne le composant horizontal relatif à des catalogues complets, exacts et réguliers fournis ; elle ne produit pas ces catalogues à grande échelle.

| Contrôle | Résultat par build nominal |
| --- | --- |
| Composition | 116 ordres, deux représentations, eager et lazy C=0/1/100000, P=0/1/1000000 : 2 784 sorties et 214 704 coupes strictes/fermées |
| Budgets historiques | 48 succès aux caps exacts, 540 refus cap−1 et 36 conflits API d’alias ; statuts, raisons, forêts purgées et 33 compteurs de chaque préfixe comparés à L |
| Work persistant | 94 sorties certifiées à P1, dont 78 avec certification puis repli ; 134 sorties certifiées à grand P ; bornes, partition des appels et exclusion des ordinaux virtuels de A |
| Extension K9/K10 | 48 sorties, dont 12 C0, sur n=14 ; catalogue et coupes calculés indépendamment par Fraction/Gram et Gamma exhaustif |
| Calendrier local | Neuf appels du helper produit dans trois séquences persistantes P3/P6/P12 ; support entier, clé et niveau q4 brut vérifiés avant Work |

Le [juge FULL normal](normal.json) et son [rejeu sous `-O`](optimized.json) sont identiques : SHA `895f3ee15a90c2bc007dac40bf488e6d5727ea50881f4a5ccb2ae7c4fa13f741`. Leurs totaux additionnent les deux builds, sans multiplier de nouveau par les lectures Python. Les [28 commandes C++](closure.json), six compilations et vingt-deux exécutions, sont closes au plus tard à `2026-09-06T09:46:09.323911+00:00`. CPU1, une commande C++ à la fois, délai maximal 180 s ; LeakSanitizer actif, UBSan sans récupération. Ce ne sont pas des mesures de performance.

## Deux réfutations causales

La copie privée [reset_work](reset_work/source/morsehgp3D_v7/src/forest/full_gabriel.hpp) remet Work à zéro à chaque appel. À P1, 90 sorties violent `certified + fallback == meb_calls` alors que leurs forêts et tous leurs préfixes historiques restent identiques. Cette réfutation vise l’état entier partagé ; elle ne prétend pas isoler la seule remise à zéro de p.

Le [mutant q4-first](local_calendar_q4_first.json) permute seulement les deux boucles de candidats du helper. Sur le tétraèdre régulier, le [juge local](local_calendar_judge.py) vérifie d’abord les dix-huit terminaux F/proposé, puis réfute neuf calendriers Work. À la première MEB, P3 doit replier avec p=3, A=11, aucun certificat ; P6 certifie avec p=6, A=0. Le mutant certifie déjà à P3, avec exactement les mêmes support et niveau : le rejet porte bien sur l’admission P. Le [calcul rationnel](local_calendar_expected.json) vérifie centre, rayons des faces, contenance, ordinal 11 et niveau brut 49152/16384. Chaque mutant a son build O2 propre ; aucun n’est injecté dans les binaires nominaux.

## Ce qu’ajoutent K9 et K10

La [fixture n=14](higher_order_fixtures.json), calculée par le [préparateur rationnel](higher_order_fixture.py), vérifie 290 supports positifs sans coquille globale supplémentaire. K9 évalue 2 002 facettes / 1 001 cofaces ; K10, 1 001 / 364. Toutes ces cellules alimentent le juge Gamma, y compris les non-Gabriel. Naissances puis multifusions sont numérotées depuis ses composantes, sans appeler le producteur ni son oracle C++.

Le modèle de descente C0, indépendant du juge des parents, prédit deux pas à K9 et un à K10. Les binaires retrouvent exactement ces comptes dans les deux représentations et aux trois P. Au K10, ce pas appelle donc la MEB sur onze sites, puisque la clé de chaîne a cardinal K+1. Les forêts restent les mêmes : 35 nœuds à K9, 24 à K10.

| Témoin C0 | Appels FULL | Formes physiques F à P0 | Formes proposées à grand P | Formes physiques F à grand P |
| --- | ---: | ---: | ---: | ---: |
| n14/K9 | 14 | 1 634 | 61 | 0 |
| n14/K10 | 11 | 1 471 | 50 | 0 |

Le nombre d’appels reste inchangé. Ces petites scènes démontrent du travail supprimé dans le raccord effectivement consommé ; elles ne mesurent ni la recherche de diamètre, ni les puissances, ni une latence de tour. Les quatre millions d’appels du refus 32k restent un verrou distinct. Le q4 du modèle K9 et les compteurs globaux de certificats ne sont pas une télémétrie des supports internes du natif.

## Captures constructeur et reproductibilité

La [contrelecture du paquet constructeur](constructor_capture_review.md) ferme séparément 30+30 CTests, quatre mutants et douze injections tardives par build. Les contrôles compilés et leurs résumés sont attribués précisément ; les douze états individuels d’exception ne sont pas publiés. Le paquet `extra/` est scellé, sans rejugement scientifique supplémentaire dans cette lecture. Ses résultats restent ceux du [rapport constructeur](../../docs/RESULTATS_MEB_FULL_20260906.md).

L’[adaptateur](../full_meb_bridge.cpp) porte explicitement le bridge successeurs L et expose les cinq Work. Le [runner et juge](../full_meb_run.py) réutilise les anciens cas uniquement comme entrées/attendus ; les deux nouveaux headers sont [capturés](source_binding.json), et les dépendances L inchangées sont consommées sur leurs copies scellées. Les fichiers `.d` de chaque build prouvent ces chemins ; aucun header vivant du produit n’est compilé. `-MMD` exclut les en-têtes système : aucune prétention de build hermétique. Les sources initiales du préparateur et de l’adaptateur sont conservées ; le juge final ajoute des vérifications de compteurs et de non-interférence du mutant.

Le [contrôle indépendant de liaison](capture_binding_review.py) vérifie les trente pins initiaux, les dépendances des six builds, les intentions et les trente-quatre commandes capturées, ainsi que l’absence de chevauchement C++. Son mode par défaut se rejoue sans ELF et ne juge pas la géométrie ; les en-têtes système et l’environnement hérité complet restent exclus. Les stdout sont conservés en gzip sans perte, avec hash des octets décompressés et du fichier gzip dans chaque reçu. Aucun flux brut n’est dupliqué en clair. Les six ELF ont été rehashés puis [retirés avec leur seul répertoire temporaire](owned_build_cleanup.json) ; leurs identités et toutes les preuves sont conservées. Le diagnostic tuples/listes du préparateur de fixture est documenté dans son JSON, sans être attribué au produit. Les variantes historiques et captures précédentes restent intactes.

Depuis la racine, rejuger sans moteur :

```bash
python3 -B morsehgp3D_v7/audits/full_meb_run.py judge
python3 -B -O morsehgp3D_v7/audits/full_meb_run.py judge
python3 -B morsehgp3D_v7/audits/receipts_full_meb_20260906/constructor_capture_review.py
python3 -B -O morsehgp3D_v7/audits/receipts_full_meb_20260906/capture_binding_review.py
```

Pour le calendrier local, `local_calendar_judge.py` consomme le stdout JSONL décompressé ; `--expect-q4-first` vérifie le mutant après toute la géométrie. Les noms de commandes C++ sont immuables : une nouvelle compilation demande un nouveau reçu, jamais l’écrasement de ces sorties. P=0 reste le défaut ; CLI, archive, verticale, mémo terminal, producteur complet de catalogues, contrats 50k et G4 restent hors qualification.
