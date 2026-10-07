# Plan de développement de la v12

7 octobre 2026. Ordre de travail proposé. Chaque tranche a une porte d'entrée, une porte de sortie, ses sessions G4 et
son reçu. **Aucune tranche ne commence avant les décisions D1 à D7** de [`DECISIONS.md`](DECISIONS.md).

## 0. Règles qui valent pour toutes les tranches

- **Contrat avant code** : chaque tranche écrit d'abord son contrat (numérique, capacité, refus, compteurs), le fait
  relire, puis grave ses témoins et son oracle ; le natif vient après.
- **Différentiel dès la première prise** : toute sortie de la v12 se compare à celle de la v11 gelée (empreintes de
  [`MESURE.md`](MESURE.md), § 4) ; tout écart est expliqué par un témoin exact ou corrigé.
- **Une règle écrite d'avance par mesure**, jugée par le protocole de [`MESURE.md`](MESURE.md), § 5 ; jamais réécrite
  après les données ; les échecs sont publiés tels quels.
- **Trois verdicts, une seule adoption** (point 1 de l'auditeur, `AUDIT_CODEX_20261007.md`) : un levier n'est adopté
  que si son juge rend « adopté » ; un banc refusé (prise manquante, binaire non haché, isolation non certifiée,
  donnée absente) ou une preuve manquante interdisent l'adoption au même titre qu'un rejet par la mesure.
- **Local léger, G4 pour le lourd** : en local, Release et portes ciblées seulement ; sanitizers, profils, mutants,
  échelle et LiDAR sur G4, dans des sessions gardées.
- **Table de réconciliation** « décision de conception → implantation → mesure », tenue à jour à chaque tranche : la
  v11 a dérivé de sa conception sans le documenter.
- **Qualification complète à chaque jalon**, au profil produit, mutants compris, en lots de 30 minutes au plus.

## 1. T0 — socle, outils de vidage et microbancs

**Entrée** : décisions D1–D7.

**Travail** :
1. Ports explicites, épinglés à `ac081a06f` ([`PROVENANCE.md`](PROVENANCE.md)) : harnais (`cmake/run_expect.cmake`,
   `cmake/gates.cmake`, lanceur de mutants), `num`, `core` (budget étendu à l'appareil), oracle borné `reference/`, script
   de session G4 (lignée v12, avec une reprise qui rapatrie les résultats).
2. Catalogue de témoins lisible par une machine ([`OBJET_ET_CONTRAT_MATHEMATIQUE.md`](OBJET_ET_CONTRAT_MATHEMATIQUE.md),
   § 5), avec une porte native par témoin.
3. Outils de vidage, hors produit, sur la v11 gelée : feuilles (sites, boîtes), listes par niveau, parties de descente,
   graines par cellule ; sur ng00–02 à K5/16, K5/24, K10/24 et sur 8 000 / 16 000 / 32 000 sites.
4. Préparation de nouvelles trames LiDAR sans sol de plusieurs séquences (`bench/points_lidar_prepare.py` de la v11,
   Patchwork++ épinglé `3e6903a1`), empreintes au manifeste, hors dépôt.
5. Microbancs sur G4 :

| Mesure | Objet | Règle d'adoption proposée |
| --- | --- | --- |
| `MES-M0` | différentiel : un lecteur canonique relit les vidages `MHGP11FUL1` et en tire l'empreinte sémantique | identité avec les empreintes de la v11 |
| `MES-M2` | feuille sur GPU : noyau un-fil de la v11 (témoin), phases J3, forme cohérente | comptage ≤ 1/3 du témoin ; identité avec la feuille de référence. **Jugé le 7 octobre : J3 adoptée (`j3_r168`, 0,18 du témoin), cohérente rejetée** |
| `MES-M3` | plus petite boule proposée et certifiée, sur les parties de descente vidées | CPU de résolution à K10 réduit d'au moins 40 % à un fil ; résultats identiques. **Jugé le 7 octobre : adoptée définitivement** (session D, juge durci, cinq prises par cas : −45,2 à −46,1 % à K10) |
| `MES-M4` | noyau union-find sans lots et contraction, sur les graines vidées | noyau ≤ 10 ms à K5 et ≤ 35 ms à K10 à un fil ; contraction ≤ 3 ms ; forêts identiques. **Jugé le 7 octobre : tenue à K5 ; à K10, contraction 3,2 à 4,3 ms, au-dessus du seuil** (sessions B et D, preuves comprises) |
| `MES-M5` | parcours des boîtes en largeur sur GPU | même ensemble final de feuilles que le CPU. **Jugé le 7 octobre : adopté** (0,10 du temps de la v11 à K5/24, 0,07 à K10/24, transferts compris ; seuil 1/4) |
| `MES-M6` | coût de Session : contexte, modules, transferts épinglés, attente bloquante ou active | publié ; fixe le budget du régime résident. **Mesuré le 7 octobre** : contexte 116 ms, lancement synchronisé 8 µs, attente `yield` |
| `MES-M7` | profil par composante de la résolution (sonde, plus petite boule par route, census saturé et complet, saut, partie suivante), à un fil, compteur de cycles, sur la réplique v12, K5 et K10 ; la comparaison à la v10 R2 est abandonnée (précision du 7 octobre, [contrat de la tour](CONTRAT_TOUR.md) § 11) | publié ; confirme la part de la plus petite boule, du census et des sondes. **Mesuré sur G4 le 7 octobre** ([session E](../receipts/g4_t2e_20261007/README.md)) : sondes 39 à 43 %, proposition 26 %, census 14 à 19 % à K5 |
| `MES-S` | étendues locales (feuilles, supports, parties de descente) et part de chaque voie numérique ([`CONTRAT_NUMERIQUE.md`](CONTRAT_NUMERIQUE.md) § 8) | publié ; fixe les paliers |
| `MES-E` | v11 gelée sur des découpes de 1, 2, 4 et 8 millions de sites d'une scène réelle, K5 puis K10 : volumes par site, pic mémoire, temps, point de rupture | publié ; fixe les objectifs du régime (b) et la taille des lots. **Mesuré sur G4 le 7 octobre** ([session E](../receipts/g4_t2e_20261007/README.md)) : à K5, 16 à 25 µs et 7 à 17 Ko résidents par site (8 M en 155 à 203 s, 54 à 75 Gio) ; à K10, 28 à 43 Ko par site, rupture vers 4 M ; étage forêt superlinéaire sur ETH3D |
| `MES-P` | v11 gelée sur 100 à 10 000 sites, voie CPU et voie GPU, à chaud : coût fixe et coût par site | publié ; fixe le seuil de la voie CPU des petits nuages. **Mesuré sur G4 le 7 octobre** ([session G](../receipts/g4_t0g_20261007/README.md), 48 fils) : morceaux de trames réels, K5 6,9 ms + 7,4 µs par site, K10 5,5 ms + 42 µs par site ; la v11 s'effondre sur le réseau entier (étage forêt 92 à 99 %, 8,4 s à 10 000 sites à K5, prise expirée à K10) et refuse la sphère dès 3 000 sites (`wide_leaf`) ; régime à un fil non mesuré |

**Sortie** : les microbancs sont jugés ; la forme de la feuille est choisie ; les budgets de [`ARCHITECTURE.md`](ARCHITECTURE.md)
sont confirmés ou révisés **avant** tout port d'étage. Si un budget tombe, le plan se révise ici, pas en fin de tranche.

**Bilan de sortie de T0 (7 octobre 2026)**, d'après quatre sessions G4 gardées (reçus `g4_t0a` à `g4_t0d`, toutes
terminées par un arrêt certifié) :

- **Feuille** : J3 par phases, variante `j3_r168`, adoptée (`MES-M2`) ; la forme cohérente est rejetée.
- **Parcours** : en largeur sur le GPU, adopté (`MES-M5`) ; avec la feuille, environ 16 ms à K5 sur ng00 avant la fin
  d'étage : le budget de l'étage C (35 à 45 ms) est confirmé.
- **Plus petite boule** : proposée puis certifiée, adoptée définitivement (`MES-M3`, −45 à −46 % de résolution à
  K10) ; à K5 le gain n'est que de 7 % : le budget de l'étage G (25 à 30 ms) **n'est pas atteignable sans le
  recensement borné aux $k$ plus proches et le mémo de cellule** (`LEM-T3`), qui deviennent la priorité de T2.
- **Forêt sans lots** : tenue à K5 (`MES-M4`) ; à K10, contraction de 3,2 à 4,3 ms au-dessus du seuil de 3 ms,
  publiée ; le budget de l'étage T est confirmé à K5.
- **Session** : coûts fixes mesurés (`MES-M6`) ; attente `yield`, graphes pour les suites de lancements.
- **Non faits en T0**, joués depuis : `MES-M7` et `MES-E` (session E, `g4_t2e_20261007`), `MES-P` (session G,
  `g4_t0g_20261007`), sur les outils de données corrigés (`CST-0216` à `CST-0218`) ; `MES-E` a lu les découpes
  refaites par la règle exacte (par exemple `eth3d_courtyard_c1M` : 1 000 109 sites, `DONNEES.md` § 3).
- **Outils de jugement** : les juges des microbancs ont été durcis après l'audit (`CST-0213` à `0215` clos) ; des résidus
  de `CST-0018` restent à fermer avant toute nouvelle adoption.

## 2. T1 — catalogue

**Entrée** : `MES-M2` et `MES-M5` jugés.

**Travail** : référence CPU (feuille en source unique, SIMD) ; voie GPU résidente en flux ; fin d'étage sur l'appareil ;
repli `unresolved` parallèle et budgété ; départage par coordonnées. Portes : témoins, oracle borné, restriction J1,
Euler à K+2 (filet), différentiel du catalogue contre la v11 sur trames entières, mutants causaux (dont un mutant
« un fil par feuille » qui doit perdre son budget de temps).

**Sortie** : catalogue identique à celui de la v11 (au départage près, déclaré) ; budget de l'étage atteint sur G4 ou
écart publié.

**État de T1 (7 octobre 2026, soir)** : voie CPU de référence livrée et conforme à la v11 gelée (K5 et K10, profils
21 et 32, [contrat § 9](CONTRAT_CATALOGUE.md)) ; lecteur de transition livré ; voie appareil (T1-b) à écrire et à
juger sur G4 (identité à l'octet avec la voie CPU, budget de l'étage C).

## 3. T2 — tour

**Entrée** : `MES-M3`, `MES-M4`, `MES-M7` jugés ; `LEM-T1`, `LEM-T3`–`LEM-T6` contre-lus et inscrits au registre ;
contrat de la tranche et des compteurs écrit et contre-lu ([`CONTRAT_TOUR.md`](CONTRAT_TOUR.md), 7 octobre). Le port
commence sur le catalogue de la v11, lu par un adaptateur de test de ses vidages `MHGP12DP` (même objet `Catalogue`
que T1) ; la **sortie** exige le catalogue de T1 dans la chaîne mesurée.

**Travail** : résolution (plus petite boule certifiée, mémo déterministe, census borné, table de populations résidente,
leviers `G-L3` à `G-L7` jugés par `MES-G1` à `MES-G4`) ;
noyau union-find recouvert ; contraction ; verticales ; registre d'événements. Portes : oracle borné (forêt entière,
coupes ouvertes et fermées, verticales), `JUG-EMST` à l'échelle, `pipeline` W1 contre W48, mutants (plateau séquentiel,
date terminale, contraction d'hyperarête entière).

**Sortie** : empreintes FULL identiques à celles de la v11 à K5 et K10 (`MES-M0`) ; budget de la tour atteint ou écart
publié.

## 4. T3 — registre et vues

**Entrée** : T2.

**Travail** : `full` compact ; squelette (équivariant par translation) ; `points` ; `condense` ; `plat` ; selon D11, les
exports Zoltan ; écriture à SHA-NI. Chaque vue a son lecteur en bibliothèque standard et sa porte contre l'oracle.

**Sortie** : vues qualifiées ; coût de chaque vue mesuré sur G4 et publié à côté du temps de la tour.

## 5. T4 — qualification et campagne de mesure

Matrice complète au profil produit (Release, ASan/UBSan, TSan, tampons empoisonnés, mutants, échelle 8 000 / 16 000 /
32 000, LiDAR) avec des entrées u21, u24 et u32 ; campagne de temps sur plusieurs séquences, à froid et à chaud, selon le
protocole de [`MESURE.md`](MESURE.md), puis sur les scènes de plusieurs millions de sites et sur les petits nuages,
contre leurs objectifs publiés ; reçu immuable.

## 6. T5 — comparaison à HDBSCAN

Préenregistrée et menée **jusqu'au bout** (la v11 n'a jamais lancé ses tests décisifs S3b et P08) :
- trois niveaux séparés : A (tour, meilleur amas discret), B (meilleur bloc de la hiérarchie), C (partition), chacun
  avec son oracle (antichaîne) ;
- bras : `sklearn` épinglé (version, machine) ; `sklearn` à plateaux normalisés ; **la même tête sur l'arbre de
  `sklearn`** (pour séparer l'apport de la hiérarchie de celui de la tête) ; **MR$_k$-bord** (pour isoler la connexité
  d'ordre supérieur, jamais mesurée en v11) ;
- populations représentatives, jamais conditionnées au succès d'un bras ; développement sur les séquences 00–07 et
  09–10, bilan sur la 08 ; rapport par ordre, jamais en catégorie « il existe un k » ;
- métriques : PQ « things », IoU un-à-un, parts intact / fusionné / découpé / bruit ; convention de niveau déclarée.

## 7. Sessions G4 : liste de contrôle

Avant : `uptime -s` (le codespace redémarre environ toutes les 6 h, et hors calendrier) ; aucune autre session vivante
(verrou commun) ; disque libre sur `/workspaces` ; plan haché ; juge haché ; données hors de `/tmp`. Pendant : ne jamais
éditer un script en cours d'exécution ; ne jamais arrêter le processus de session. Après : `TERMINATED` certifié sur la
cible exacte ; en cas de perte du contrôleur, `--recover` aussitôt ; reçu sans identité de compte ni copie de sources.
