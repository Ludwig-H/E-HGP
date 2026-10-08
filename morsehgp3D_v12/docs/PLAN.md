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
| `MES-B` | la tour FULL de la v12 sur des scènes LiDAR réelles **entières** (captations telles quelles), K5 voie appareil, bras CPU, K10 : mur, étages, CPU·s, pics de mémoire de l'hôte et de l'appareil, refus, exposant sur les séries emboîtées de Boreas | préparé le 8 octobre ([pilote](../microbancs/mes_b_scenes/README.md), verdicts B1 à B4 écrits d'avance). **Mesuré sur G4** ([sessions L1 et L1 rejouée](../receipts/g4_mesb1r_20261008/README.md)) : non tenu ; neuf scènes entières calculées de 0,15 à 6,7 M de sites, 3,2 à 12,2 s par million à K5 ; refus de l'appareil dès 5 M de sites (8 à 25 Ko d'appareil par site) et à K10 dès 1,5 M ; T superlinéaire (3,2 µs par site à 6,2 M) ; le catalogue en flux (§ 4.6 d'ARCHITECTURE) est la condition du régime (b). [Session L2](../receipts/g4_mesb2_20261008/README.md), voie CPU : IGN Paris entier (14,6 M de sites) en 166 s et 137 Gio (G 21 s, T 35 s) ; Lyon (24 et 32 M) refusé par le pic du catalogue CPU (10 à 11 Ko par site) ; ETH3D courtyard (16,8 M) refusé `wide_leaf` : la voie large T1-c sert aussi le réel. [Session L1p](../receipts/g4_mesb1p_20261008/README.md) (produit `c648b3857`) : murs chauds 13 à 39 % plus bas qu'en L1r sur la voie appareil (Boreas 10 trames sans sol 10,0 → 7,3 s, ETH3D meadow 35,3 → 30,6 s) ; mêmes refus de mémoire de l'appareil au-delà de 5 M sites à K5 et dès 1,5 M à K10 ; la queue (noyau de l'ordre 5) dépasse G sur les grandes scènes |
| `MES-C` | la tour FULL de la v12 sur les 159 petits nuages de `g4_small`, une Session résidente par configuration (voies CPU et appareil, K5 et K10, 1, 4 et 48 fils), familles difficiles seules | **mesuré sur G4 le 8 octobre** ([session C](../receipts/g4_mesc_20261008/README.md)) : refusé (Session K10 à un fil expirée), C1 à C3 non tenus ; nuages réels à K5 et 48 fils : CPU 20 ms + 11,7 µs par site, appareil 9,7 ms + 3,6 µs par site ; le coût fixe vient de la coordination des 48 fils (6,8 ms à 4 fils contre 15,6 ms à 48 vers 150 sites) ; réseau entier calculé jusqu'à 10 000 sites (0,26–0,31 s, contre 8,4 s pour la v11) ; quasi-sphère refusée `wide_leaf` dès 3 000 sites. [Session C2](../receipts/g4_mesc2_20261008/README.md) (Pool à équipe et lot T2-d-C, 4 et 48 fils) : non tenu, aucun contrôle manquant ; vers 150 sites à 48 fils, CPU 15,6 → 11,1 ms et appareil 8,5 → 4,5 ms (comparaison descriptive, deux changements). [Session C3](../receipts/g4_mesc3_20261008/README.md) (produit `72f622a55` : Session recouverte, T2-d-B, cache de blocs) : non tenu ; appareil à 48 fils 4,3 ms vers 150 sites et 24,5 ms au-delà de 5 000 (pente 3,7 → 2,2 µs par site) ; le coût fixe vient du catalogue ; cache sans effet mesurable sur les petits nuages |
| `MES-P` | v11 gelée sur 100 à 10 000 sites, voie CPU et voie GPU, à chaud : coût fixe et coût par site | publié ; fixe le seuil de la voie CPU des petits nuages. **Mesuré sur G4 le 7 octobre** ([session G](../receipts/g4_t0g_20261007/README.md), 48 fils) : morceaux de trames réels, K5 6,9 ms + 7,4 µs par site, K10 5,5 ms + 42 µs par site ; la v11 s'effondre sur le réseau entier (étage forêt 92 à 99 %, 8,4 s à 10 000 sites à K5, prise expirée à K10) et refuse la sphère dès 3 000 sites (`wide_leaf`) ; régime à un fil non mesuré |

**Sortie** : les microbancs sont jugés ; la forme de la feuille est choisie ; les budgets de [`ARCHITECTURE.md`](ARCHITECTURE.md)
sont confirmés ou révisés **avant** tout port d'étage. Si un budget tombe, le plan se révise ici, pas en fin de tranche.

**Bilan de sortie de T0 (7 octobre 2026)**, d'après quatre sessions G4 gardées (reçus `g4_t0a` à `g4_t0d`, toutes
terminées par un arrêt certifié) :

- **Feuille** : J3 par phases, variante `j3_r168`, adoptée (`MES-M2`) ; la forme cohérente est rejetée.
- **Parcours** : en largeur sur le GPU, adopté (`MES-M5`) ; avec la feuille, environ 16 ms à K5 sur ng00 avant la fin
  d'étage. **Correction du 7 octobre (`CST-0235`)** : le budget de l'étage C (35 à 45 ms) n'en est pas confirmé ;
  seuls le parcours et les feuilles sont mesurés, l'émission, la fin d'étage et les transferts ne le sont pas, et à
  aval CPU inchangé, parcours et feuilles gratuits laisseraient encore 145 à 202 ms à K5 (reçu
  `audit_performance_20261007`) : la fin d'étage sur l'appareil (T1-b) décide du budget.
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
21 et 32, [contrat § 9](CONTRAT_CATALOGUE.md)) ; lecteur de transition livré. **Voie appareil (T1-b) adoptée sur G4
le 8 octobre** ([session I](../receipts/g4_t1bi_20261008/README.md)) : identité à l'octet avec la voie CPU sur ng00–02 à
K5 et K10 et sur les uniformes, trois mutants appareil tués, étage C à chaud **35,1 / 30,8 / 37,6 ms à K5** (maximum
des médianes par processus 35,5 / 31,0 / 37,7 ms, budget 45 ms), dont 8 à 10 ms de transferts ; 113 à 146 ms à K10.
Voie CPU après la fin d'étage partagée : 281 à 334 ms à K5 (0,71 à 0,77 de F2). **T1-d (catalogue en flux) adopté sur G4 le 8 octobre**
([sessions T1-d](../receipts/g4_t1d_20261008/README.md)) : arène rapatriée par lots quand le budget de l'appareil
ne la porte plus, fin d'étage par tranches de clés coupées aux ordres F4 certains ; catalogue identique à l'octet,
coût nul pour la trame (1,000 / 1,002 / 1,002 sur ng00–02), voie en flux tenue sur l'appareil réel.

**T1-c — voie large (ouverte le 7 octobre, `CST-0237`)**. L'objectif des petits nuages « aucune famille dégénérée
refusée ni expirée » ([`MESURE.md`](MESURE.md) § 2) n'est pas couvert par T1 : feuilles d'au plus 256 sites,
coquilles d'au plus 64 sites au census, $m$ sur un octet, masques de traces sur 64 bits dans la tour ; et
$q_{\min}\leq 4$ ne borne ni la coquille ni la liste candidate. Relever une constante ne suffit pas. Protocole
retenu (proposé par l'auditeur, reçu `audit_performance_20261007`, complément de la session G) : (1) publier, sur le
générateur épinglé, maximum de liste candidate, profondeur, largeur au refus et histogramme des coquilles exactes,
en distinguant quasi-sphère, sphère entière exacte et réseau ; (2) pour le réseau, ventiler préparation,
résolution, noyau et contraction, avec cellules, représentants, boules, incidences et tailles de coquilles ;
(3) écrire le contrat d'une voie large (masques et CSR extensibles, budgets, traitement certifié des listes
larges, canonisation par boule, quotient de traces `LEM-T7` s'il est adopté ; ni écrêtage, ni jitter, ni
préfixe) ; (4) graver les petites versions contre l'oracle, les frontières 32/33, 64/65 et 255/256/257 et les
plateaux à supports minimaux différents, puis mesurer sur G4 avec sorties complètes et refus explicites.

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

**État de T2 (7 octobre 2026, soir)** : **étage G livré** dans `src/tower/` (résolution du § 4.1 du contrat, politique
`v12_indices` ; [rapport](../receipts/developpement_20261007/tour_G_RAPPORT.md),
[interface](../receipts/developpement_20261007/tour_G_INTERFACE.md)) : oracle borné (21 225 cibles égales à
`resolve_v12` sur 342 nuages), différentiel de la v11 sur ng00–02 à K5 (graine identique pour 100 % des
représentants), déterminisme 1 contre 8 fils aux trois tailles d'intérêt et aux profils 21, 24 et 32, 7 mutants tués.
**Étages T, M, V, R et export `MHGP11FUL1` livrés le 8 octobre** ([rapport](../receipts/developpement_20261007/tour_TMVR_RAPPORT.md),
[interface](../receipts/developpement_20261007/tour_TMVR_INTERFACE.md)) : noyau union-find par taille sans lots après une
pré-passe parallèle des cibles, contraction `LEM-T4`, verticales `LEM-T5`/`LEM-T6`, registre avec les branches ouvertes
des hyperarêtes ; `MES-M0` à l'octet sur les neuf cas avec les graines de la v11 et **sémantique sur la chaîne complète
du produit** (catalogue de T1 → G → T, M, V, R → export), 1 contre 8 fils identiques, 27 mutants de la tour tués : critère
de sortie de T2 tenu localement. Temps locaux à un fil sur ng00 K5 : T 100, M 50, V 67, R 167 ms, à mesurer sur G4. **Première mesure du contrat FULL le 8 octobre** ([session K](../receipts/g4_fullk_20261008/README.md),
Session résidente, catalogue sur l'appareil) : **non tenu** — 159 / 128 / 163 ms à K5 sur ng00–02, médiane 241 ms et
maximum 468 ms sur les 37 trames `v12set` (six séquences, médiane 64 740 sites) ; vers 65 000 sites : C 54 (dont 15
de transferts), G 79, T 63, M 16, R 23 ms. Suite : tranche T2-d (recouvrement et parallélisme de T, M, V, R ; coût de
G ; transferts de C), budgets d'étage repris à la taille médiane réelle. **T2-d-C adopté sur G4 le 8 octobre**
([session T2-d-C](../receipts/g4_t2dc_20261008/README.md), `02b735d6b`) : sorties du catalogue en flux, double tampon,
sorties anticipées, repli compact ; étage C **26,9 / 23,8 / 26,8 ms à K5** (rapport 0,74 à 0,80), 98 à 103 ms à K10 ;
mur FULL 154 / 123 / 156 ms (−4 à −6 %), G et T, M, V, R restant en série. **T2-d-A adopté sur G4 le 8 octobre**
([session T2-d-A](../receipts/g4_t2da_20261008/README.md), bras avant = `27eca166b`) : Session recouverte, mur FULL K5
**99,7 / 81,0 / 97,8 ms** sur ng00–02 (rapport 0,65 à 0,69) ; 37 trames `v12set` : médiane 160,6 ms, maximum 327,3 ms ;
queue de 5 à 14 ms, G devient le chemin critique ; la Session recouverte est depuis la voie par défaut de la sonde
FULL et de ses pilotes (`--sequentiel` garde l'ancienne voie comme témoin,
[bascule](../receipts/developpement_20261008/reponse_audit_t2d_bascule.md)). **T2-d-B qualifié sur G4 le 8 octobre** ([session T2-d-B](../receipts/g4_t2db_20261008/README.md), bras reconstruits sur `902041f66`, `REGLE_T2D_B` écrite d'avance) : étage G **48,6 / 38,9 / 45,0 ms** à K5 sur ng00–02 avec le lot (rapport 0,92 à 0,93) ; census à témoins (L3) et report des compteurs (L2) adoptés, garde seule (L1) et proposition entière (L4) rejetées seules ; L4 retirée du produit, qui est le bras « census » (L1 + L2 + L3), mesuré et adopté (0,92 à 0,94). **Contrat mesuré sur ce produit le 8 octobre**
([session M](../receipts/g4_fullm_20261008/README.md)) : **non tenu** ; ng00–02 sous 100 ms (94,8 / 78,1 / 94,8 ms),
mais 37 trames `v12set` à 160,6 ms de médiane et 358,9 ms de maximum ; sur les grandes trames, le noyau séquentiel de
l'ordre 5 ferme la tour (A6). Cache de blocs de la Session (8 Gio) adopté par le pilote apparié (mur FULL 0,92 à 0,93)
et mis par défaut ; voie `--sequentiel` 1,47 à 1,53 fois plus lente sur le même binaire ; adoption du cache reproduite
avec le pilote apparié fermé ([session cache2](../receipts/g4_cache2_20261008/README.md), 0,912 à 0,925).
**Étude « G sur l'appareil »** ([note](../receipts/developpement_20261008/etude_g_appareil.md),
[session G-APP](../receipts/g4_gapp_20261008/README.md)) : **rejetée** par sa règle sur les propositions DWelzl en
binaire64 (rapport 0,74 à 0,76) ; census ×9 à ×11 et sondes ×19 à ×22 sur l'appareil, identité complète ; le census
« à plat » de la même source coûte 0,57 à 0,59 fois celui du produit sur l'hôte (levier CPU transmis à T2-d-B2).
[Étape 2](../receipts/g4_gapp2_20261008/README.md) (propositions sur l'appareil) : D2 (voie entière puis binaire32) et D1
(voie entière puis binaire64, partagée) **rejetés** par leur règle ; aucune tranche « G sur l'appareil » sur cette base. **A6 (chaîne de l'ordre K) rejeté sur G4 le 8 octobre**
([sessions A6](../receipts/g4_t2da6_20261008/README.md)) : grandes trames 0,914 (queue de 27–72 à 12–20 ms), mais ng00 et ng01
ralenties de 3 à 4 % (seuil 1,02) ; retiré de `main`, une A6b sans cette perte reste à concevoir. **Lot T2-d-B2 adopté sur G4 le 8 octobre**
([session T2-d-B2](../receipts/g4_t2db2_20261008/README.md), bras sur `72f622a55`) : mur FULL 0,944 / 0,948 / 0,963 sur ng00–02, 0,982 et 0,984
sur deux trames moyennes ; B2-T (tables) adopté seul, B2-S et B2-C rejetés seuls ; fin de G −15 % sur ng00, −7 % à la
médiane. Profil de G à 48 fils sur G4 : LEM-T1 27 %, traces 22 %, sonde 14 %, census 15 %, proposition 10 % ; LEM-T1 est
le levier suivant. Suites déclarées (T2-c) : classification des cellules et table de populations
reconstruites à chaque appel (37 ms à K5, 385 ms à K10 à 3 fils) à rendre résidentes ou parallèles ; coût par unité
de travail au-dessus de la réplique de `MES-M7` (2,67 s contre 2,34 s à un fil sur ng00 K5), sonde de table d'abord ;
leviers `G-L4` à `G-L7` non implantés. **Premiers temps G4** ([session H](../receipts/g4_t2h_20261007/README.md), 48 fils) : étage G 80 / 63 / 76 ms à K5 sur ng00–02 (tables 15 à 19 ms, à peine parallèles ; résolution 37 à 49 ms, ×30 de 1 à 48 fils), 450 à 634 ms à K10 : 2,5 à 3 fois le budget à K5 ; chantier T2-c en cours (tables, `G-L5`, recherche des supports). **T2-c adopté sur G4 le 8 octobre** ([session J](../receipts/g4_t2cj_20261008/README.md)) : index des naissances parallèle et file de sondes `G-L7`, étage G **53,1 / 42,1 / 48,2 ms à K5** (rapport 0,63 à 0,67, IC 95 % serré, A/A ≈ 1), 443 ms à K10 sur ng00 ; `G-L5` rejeté. Reste 1,6 à 1,8 fois le budget : census gardé, puis `LEM-T1` et proposition.

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
