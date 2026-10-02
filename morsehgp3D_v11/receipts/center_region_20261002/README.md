# CenterRegion, J2 et FullDomain — G4 du 2 octobre 2026

**Qualification native verte ; campagne de mesures inachevée, conservée en échec.**
Source exécutée : `7f1922c7743d8682e2665a491b01d32e8f2d546c`.
Session : `v11.20261002.region1`. Les travaux parallèles ultérieurs ne font pas partie de cette capture.

La matrice principale passe **1 398 / 1 398** portes et le supplément ASan/UBSan18 **73 / 73**.
La troisième commande, `002_profiles`, atteint son délai externe de **820,003 s**, sort avec le code 124
et ferme son groupe (`group_closed=1`, `residual_group_killed=1`). Le worker sort 1 ; le contrôleur conserve
`failed_remote` et `DONE=3`. Aucun défaut produit n'est démontré par cet arrêt du banc.

Le reçu brut atteste `closure=stopped`, l'arrêt ciblé certifié, les résultats vérifiés, la suppression
de la clé privée et de la clé OS Login, ainsi que la libération de la réservation. La génération démarrée
et arrêtée est `2026-10-02T12:28:25.841-07:00` ; l'observation finale est `TERMINATED`.
Les listes d'erreurs et d'avertissements sont vides. Aucun nouvel appel cloud ni rejeu natif n'a servi à cette lecture.

## Qualification et portée

| Configuration | Profil | Portes passées |
| --- | ---: | ---: |
| GCC Release | 18 | 292 / 292 |
| GCC ASan/UBSan | 24 | 217 / 217 |
| GCC TSan | 21 | 217 / 217 |
| bits21 | 21 | 217 / 217 |
| bits24 | 24 | 217 / 217 |
| poison | 21 | 218 / 218 |
| mutants | 18, options ciblées 21/24 | 18 / 18 |
| style | sans exécution géométrique | 2 / 2 |
| Clang facultatif | — | absent |
| supplément ASan/UBSan, `num;index;tower` | 18 | 73 / 73 |

Les **154 mutants** sont tués : 78 core, 26 num, 16 cloud, 8 index, 11 catalogue et 15 tower.
Deux échecs de construction sont explicitement attendus dans core ; aucun signal ni délai n'est compté
comme mort causale. Les sorties JUnit tronquées sont recoupées avec les sections complètes de `LastTest.log`.

Les nouvelles portes CenterRegion vérifient, par profil, **371 requêtes Fraction / 865 contrôles**,
dont 55 contacts, 123 permutations, 137 intersections, 163 disjonctions, 18 dégénérescences et 53 refus.
Le modèle indépendant possède 39 faits fixes et refuse 26 corruptions par profil ; normal et `−O` concordent.
Les portes natives région comptent 29 / 28 / 154 contrôles ; le raccord catalogue 13 / 15 / 7.
Les huit groupes FullDomain et sa faute d'allocation passent également. FullDomain fournit le propriétaire
et la recherche exacte ; cette capture ne qualifie pas encore des cellules, descentes ou plateaux FULL.

## Mesures conservées

Calendrier demandé : **six entrées entières × K5/K10 × B18/B21/B24 = 36 unités**, une répétition,
feuilles16, capacité256, budget Buffer8GiB, délai individuel30s. Les six entrées ont le même domaine entier18
dans les trois builds. Les tests numériques hauts bits restent des preuves distinctes des mesures LiDAR.

Le rapport partiel conserve **29 résultats : 14 réussites et 15 délais individuels**. Ces derniers sont
13 essais K10 et les K5 uniforme32k en B21/B24. Deux K10 sont omis après l'échec de K5 du même profil.
Les **cinq unités finales sans résultat persistant** concernent ng02 : B18/K10, B21/K5/K10 et B24/K5/K10.
Une unité sans résultat peut avoir commencé avant l'interruption : elle n'est pas déclarée « non lancée ».
Le collecteur sauvegarde après capture du processus et avant décodage sémantique ; il n'enregistre pas ici
un état `running` avant lancement. Aucun résultat ni durée n'est reconstruit pour cette partie interrompue.

Temps de l'API catalogue K5, en secondes, une observation par case ; les valeurs exactes sont dans `profiles.json` :

| Entrée | Sites | B18 | B21 | B24 |
| --- | ---: | ---: | ---: | ---: |
| uniforme8k | 8 000 | 5,455 | 7,042 | 7,181 |
| uniforme16k | 16 000 | 11,458 | 14,844 | 15,030 |
| uniforme32k | 32 000 | 24,125 | délai30s | délai30s |
| ng00, 08/000000 | 39 885 | 17,892 | 21,734 | 21,996 |
| ng01, 08/000100 | 35 551 | 14,217 | 17,374 | 17,509 |
| ng02, 08/000200 | 45 845 | 16,894 | sans résultat | sans résultat |

L'API inclut deux passes, le tri et les sorties en mémoire. Le temps du processus comprend aussi lecture
et écriture canonique ; le décodage Python est séparé. Les octets réservés incluent le Cloud vivant et
les Buffer natifs ; ils ne sont ni le RSS ni la mémoire du décodeur. Ces valeurs portent sur un catalogue
mono CPU, sans segmentation, FULL ni GPU. Les trois trames LiDAR viennent d'une seule séquence08,
avec masque sans sol figé et grille1mm ; elles ne qualifient pas plusieurs séquences ni le brut float32.

Quatre cases K5 possèdent trois réussites : leurs empreintes sémantiques, les treize compteurs logiques
(neuf historiques et quatre J2), ainsi que les deux compteurs q4 concordent entre profils. Aucune divergence
n'est observée parmi les résultats disponibles. Une recoupe des 13 réussites communes avec `q4levels1`
retrouve les mêmes empreintes canoniques brutes, tailles et objets sémantiques ; les compteurs de travail
ont changé avec J2 et ne sont pas assimilés à ceux de cette référence historique.

## Lecture et limites de la preuve

```sh
python3 morsehgp3D_v11/receipts/center_region_20261002/check.py
python3 -O morsehgp3D_v11/receipts/center_region_20261002/check.py
python3 morsehgp3D_v11/receipts/center_region_20261002/check_selftest.py
python3 -O morsehgp3D_v11/receipts/center_region_20261002/check_selftest.py
```

**LIVE** : le reçu original local indiqué par `raw_receipt_local` et son fichier `DONE` restent obligatoires.
`region1/` contient six pièces : reçu filtré, une archive originale, matrice principale, supplément,
rapport profils et manifeste d'entrée. Aucun payload KITTI, paquet source, binaire ou JUnit décompressé
n'est dupliqué. L'archive de343507octets est lue par `tar.extractfile`, jamais extraite.

Le lecteur compare reçu compact et brut, empreintes et tailles, clôture et commandes, inventaire exact
des configurations et sélections, JUnit et compteurs, caches/provenance, sorties des nouvelles portes,
morts causales, identité du banc, calendrier, omissions, événements et comparaisons recomputées.
`contract.json` épingle les sources Git et les sélections observées dans cette capture ; ces dernières
ne constituent pas une preuve indépendante de complétude des tests conçus. Les helpers historiques
sont importés dans des instances privées, sans modification des anciens reçus ou lecteurs.

Un code0 du lecteur signifie **cohérence des pièces**, avec `campagne=ECHEC` explicite. Un code1 signifie
refus des pièces. Les six témoins positifs et 37 corruptions de l'auto-test passent en normal et `−O` ;
les corruptions de clôture compacte sont notamment rejetées par la comparaison compact/brut, sans prétendre
qu'elles isolent causalement chaque garde de clôture. L'auto-test n'exécute aucun produit.
Les payloads canoniques du banc ont été supprimés à distance : leurs hashes déclarés sont comparés,
ils ne sont pas recalculés à partir de cette archive. La cohérence du reçu ne certifie pas à elle seule
l'authenticité externe de la machine ou de l'exécution.
