# Assemblage du catalogue — assembly1

Capture close, source exécutée `4b8e04be6107d27cb91fea3746d944a3c9beadb4`.
La matrice passe **2 535/2 535** portes et le supplément ASan18 **178/178**.
Le calendrier du catalogue est conforme : **36/36 succès**, aucune omission,
aucune divergence parmi les six groupes comparés.

Le reçu original LIVE reste obligatoire :
`/workspaces/.ehgp-sessions/v11.20261003.assembly1/receipt.json`.
`DONE=0`, worker0, statut `completed`, génération
`2026-10-02T17:49:46.944-07:00`. Arrêt ciblé, résultats vérifiés,
retrait OS Login, suppression de la clé privée et libération du verrou sont
certifiés ; erreurs et avertissements sont vides. Le compact conserve
l'empreinte du brut. La capsule ne se présente pas comme une archive autonome.

Une seule archive worker originale, 570 673 octets :
`d810445e5e5d8b5b915819832e7a43df32fe53c5a7ba63364a668a8edbebeb53`.
Elle contient les journaux, JUnit, inventaires et preuves de compilation,
sans paquet source ni données SemanticKITTI. `source_4b/` contient uniquement
les sept aides Python exécutées nécessaires au rejeu du verdict, épinglées
par `source_contract.json`. Aucun import des bancs WIP.

## Périmètre mesuré

Six entrées entières identiques : 39 885 / 35 551 / 45 845 sites sans sol des
trames 08/000000, 08/000100, 08/000200, et trois synthétiques 8k/16k/32k.
Ce sont trois trames d'une seule séquence. Les coordonnées et IDs sont liés
au manifeste uploadé ; domaine commun entier18 bits, mêmes coordonnées1mm
compilées en u21/u24, sans nouvelle quantification. Tous les essais sont K5,
W48, leaf16. Un processus neuf par mode/profil, sans répétition statistique.

Modes LiDAR : 3 = front fixe / assemblage série ; 11 = front fixe /
assemblage parallèle ; 7 = front adaptatif / assemblage série ; 15 = front
adaptatif / assemblage parallèle. Les synthétiques comparent seulement3/11.
Cache J2 et tri indirect exact sont actifs dans tous ces modes. Le bit8
concerne ici l'assemblage du catalogue, pas la forêt FULL.

Durées de l'API catalogue, en millisecondes :

| Entrée | Profil | Mode3 | Mode11 | Mode7 | Mode15 |
|---|---:|---:|---:|---:|---:|
| ng00 | u21 | 3352,271 | 2927,585 | 2079,055 | 1708,514 |
| ng00 | u24 | 3292,988 | 2951,379 | 2112,689 | 1671,494 |
| ng01 | u21 | 2136,906 | 1935,925 | 1708,864 | 1429,385 |
| ng01 | u24 | 2155,179 | 1860,763 | 1704,295 | 1522,654 |
| ng02 | u21 | 2882,456 | 2499,292 | 2276,187 | 1899,654 |
| ng02 | u24 | 2840,908 | 2487,473 | 2150,926 | 1780,892 |
| Uniforme8k | u21 | 450,276 | 287,413 | — | — |
| Uniforme8k | u24 | 449,683 | 283,878 | — | — |
| Uniforme16k | u21 | 935,543 | 573,790 | — | — |
| Uniforme16k | u24 | 975,947 | 579,812 | — | — |
| Uniforme32k | u21 | 2010,359 | 1218,815 | — | — |
| Uniforme32k | u24 | 1960,928 | 1221,199 | — | — |

Sur LiDAR, l'intervalle diagnostique d'assemblage passe de167,519–250,425ms
à4,004–5,219ms. Les huit intervalles publiés sont disjoints mais non
exhaustifs ; les sommes des tâches concernent la génération, pas l'assemblage.
Les variations des autres étapes entre ces essais uniques ne s'attribuent
pas automatiquement au seul port. Cloud et création Pool sont séparés de
l'API ; durée processus inclut lecture et sérialisation.

Le mur campagne est219,211170107s, dont76,436658268s de processus natifs et
136,449166051s de vérification sémantique. Les résumés sont décodés12fois et
réemployés24fois après SHA256 intégral du fichier courant. La capacité du
cache Python figé est64résumés de64KiB maximum. Son coût est hors chrono API.

La mémoire publiée mesure les réservations Buffer, Cloud vivant compris,
pas le RSS, les piles ou Python. Pics LiDAR :264 115 844–352 212 224octets.
Pics maximaux sur **toutes** les entrées :637 186 616octets en u21 et
677 768 336octets en u24, atteints par l'uniforme32k. À entrée/profil fixés,
la mémoire retenue après l'appel reste identique entre modes ; les pics
d'assemblage parallèle augmentent de4 672–19 840octets selon l'entrée.
Les comptes et ratios exacts sont dans `metrics.json`, recalculé par le lecteur.

## Preuves et limites

Les hashes sémantiques, treize compteurs géométriques, q4 et cache sont
identiques entre modes/profils ; les hashes bruts le sont à profil fixé.
Les36fichiers canoniques (8 879 621 560octets cumulés) ont été supprimés
par le runner. Cette capsule compare leurs **empreintes déclarées** et les
chaînes de réemploi ; elle ne prétend pas les rehacher. Le lecteur sait
rehacher/décoder un payload archivé, voie exercée sur une petite fixture.
Le contrôle de structure des gros catalogues ne remplace pas les oracles
géométriques indépendants bornés des portes natives.

Release481, mutants21, ASan/UBSan406, TSan406, u21406, u24406, poison407,
style2 ; Clang est absent, autorisé comme optionnel. Les logs complets et
JUnit sont recoupés : **244 mutations tuées par code/ligne et2 refus de
construction attendus**, aucune mort par signal/délai. Le supplément18
sélectionne num/index/tower ; il ne rejoue pas les portes catalogue.

Cette campagne mesure uniquement le catalogue CPU K5 sur G4. Ni FULL,
segmentation du sol, float32 original, GPU ou qualité clustering ne sont
mesurés. Aucune entrée n'atteint200ms pour le catalogue ici.

Lecture depuis ce dossier :

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B check_selftest.py
python3 -B -O check_selftest.py
```

Code0 signifie cohérence des preuves ; le champ `conforming` conserve
séparément le verdict de campagne. Les contretests sont purs, sans enfant
natif : échecs et interruptions conservés, inventaires, compteurs, temps,
réemploi, payload minuscule et transport corrompus. Ils ne constituent pas
une nouvelle qualification du moteur. `check_selftest.json` fixe leurs
sorties et les empreintes des deux scripts.
