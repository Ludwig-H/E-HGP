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
| `MES-M3` | plus petite boule proposée et certifiée, sur les parties de descente vidées | CPU de résolution à K10 réduit d'au moins 40 % à un fil ; résultats identiques. **Jugé le 7 octobre : adoptée à titre provisoire** (−45 à −46 % à K10 ; une seule prise de résolution par cas, `CST-0213`) |
| `MES-M4` | noyau union-find sans lots et contraction, sur les graines vidées | noyau ≤ 10 ms à K5 et ≤ 35 ms à K10 à un fil ; contraction ≤ 3 ms ; forêts identiques. **Jugé le 7 octobre : tenue à K5 ; à K10, contraction 3,1 à 4,1 ms, au-dessus du seuil** |
| `MES-M5` | parcours des boîtes en largeur sur GPU | même ensemble final de feuilles que le CPU |
| `MES-M6` | coût de Session : contexte, modules, transferts épinglés, attente bloquante ou active | publié ; fixe le budget du régime résident. **Mesuré le 7 octobre** : contexte 116 ms, lancement synchronisé 8 µs, attente `yield` |
| `MES-M7` | profil par route des descentes, v11 contre v10 R2, même session, à un fil, K5 et K10 | publié ; confirme la part de la plus petite boule, du census et des coûts fixes |
| `MES-S` | étendues locales (feuilles, supports, parties de descente) et part de chaque voie numérique ([`CONTRAT_NUMERIQUE.md`](CONTRAT_NUMERIQUE.md) § 8) | publié ; fixe les paliers |
| `MES-E` | v11 gelée sur des découpes de 1, 2, 4 et 8 millions de sites d'une scène réelle, K5 puis K10 : volumes par site, pic mémoire, temps, point de rupture | publié ; fixe les objectifs du régime (b) et la taille des lots |
| `MES-P` | v11 gelée sur 100 à 10 000 sites, voie CPU et voie GPU, à chaud : coût fixe et coût par site | publié ; fixe le seuil de la voie CPU des petits nuages |

**Sortie** : les microbancs sont jugés ; la forme de la feuille est choisie ; les budgets de [`ARCHITECTURE.md`](ARCHITECTURE.md)
sont confirmés ou révisés **avant** tout port d'étage. Si un budget tombe, le plan se révise ici, pas en fin de tranche.

## 2. T1 — catalogue

**Entrée** : `MES-M2` et `MES-M5` jugés.

**Travail** : référence CPU (feuille en source unique, SIMD) ; voie GPU résidente en flux ; fin d'étage sur l'appareil ;
repli `unresolved` parallèle et budgété ; départage par coordonnées. Portes : témoins, oracle borné, restriction J1,
Euler à K+2 (filet), différentiel du catalogue contre la v11 sur trames entières, mutants causaux (dont un mutant
« un fil par feuille » qui doit perdre son budget de temps).

**Sortie** : catalogue identique à celui de la v11 (au départage près, déclaré) ; budget de l'étage atteint sur G4 ou
écart publié.

## 3. T2 — tour

**Entrée** : T1 ; `MES-M3`, `MES-M4`, `MES-M7` jugés ; `LEM-T1`, `LEM-T3`–`LEM-T6` contre-lus et inscrits au registre ;
contrat des compteurs écrit.

**Travail** : résolution (plus petite boule certifiée, mémo déterministe, census borné, table de populations résidente) ;
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
