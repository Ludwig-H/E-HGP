# Rapport du lecteur F — La lignée v2 → v10 (et la v11) : objets, contrats, approches, réussites, échecs, leçons

7 octobre 2026. Lecteur F de l'audit géant précédant la v12. Domaine : la lignée v2 → v10, ses contrats, ses
approches, ce qui a marché ou échoué, et les leçons transversales ; la v11 en un paragraphe et dans les
vérifications demandées.

```text
phase=exploration_v11_hors_registre (audit de passation, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune compilation ni mesure ; aucune commande git qui écrit
```

**Méthode.** Dépôt lu à `e968aba8d`. Lus par moi : `AGENTS.md`, `CLAUDE.md`, la passation et l'audit final v11,
l'audit des transpositions du 4 octobre, tous les registres de pistes fermées, le registre des preuves, le registre
formel, la bibliothèque produit et la mémoire utilisateur. Quatre lectures déléguées (v2–v4, v5–v6, v7–v8, v9–v10)
ont vérifié les faits par version. J'ai recontrôlé moi-même les points critiques : code v10 des descentes, ablation
T, reçu R22, CI, blame de `CLAUDE.md` et `AGENTS.md`.

**Légende.** Sans marque : vérifié dans le fichier, le reçu ou le commit cité. **[I]** : inférence. **[M]** : mémoire
utilisateur (`~/.claude/projects/-workspaces-E-HGP/memory/`, hors dépôt, datée). **[N]** : notes de travail d'agents
(`morsehgp3D_v11/receipts/notes_hors_depot_20261007/`), reprises seulement après contrôle. « G4 » : reçu de la VM
`g4-standard-48` (EPYC 9B45, 24 cœurs × 2 SMT, RTX PRO 6000) ; « local » : codespace.

---

## 0. Synthèse

1. **Douze générations en 92 jours.** `HGP-old` (7 juillet), produit `morsehgp3d/` (14 juillet), puis v2 à v11, du
   8 août au 2 octobre. Les dix versions v2–v11 tiennent en 60 jours, soit environ 6 jours chacune, avec un dossier,
   un namespace et une base de code neufs ou reconstitués par ports.
2. **L'objet a oscillé.** La v2 calculait déjà le bon objet (arbre de fusion de d_K). Les v4 à v6 ont calculé le fold
   des cofaces de Gabriel, que la fixture E5 réfutait depuis le 14 juillet. La v7 est revenue à FULL le 5 septembre.
   E5 est revenue « pour la cinquième fois » (A9-91, origine v3 comprise).
3. **Le contrat de 100 ms date du 7 août** (50 000 points, K = 10). Il n'a jamais été tenu pour le vrai objet. Seul
   un substitut faux (`point-MST`, p95 95,8 ms), rejeté depuis, est passé sous 100 ms.
4. **Le vrai objet sur le régime final** (sans sol, 1 mm, K5, G4) :

   | Version | Temps | Remarque |
   |---|---|---|
   | v8 | 104,6 s | flux de candidats seul, local |
   | v9 | 0,76–0,98 s | GPU |
   | v10 | 204–254 ms | CPU |
   | v11 | 212–255 ms | GPU à chaud |

   Les sauts viennent de **changements d'algorithme** (chaîne v9, boîtes de centres v10), jamais d'une réécriture
   « plus propre ». À algorithme constant, les reprises n'ont rien rendu en vitesse : v4→v5, v5→v6, et v10→v11
   (×5,7–6,0 plus lente le 3 octobre, ×1,5–1,7 le 4, K10 encore ×1,5–1,6 au gel).
5. **Différentiels.** Fermés seulement pour v4 ≡ v5 ≡ v6, sur un objet faux et un digest de filtre. v10/v11 n'a été
   comparé que sur 14 petites fixtures.
6. **La qualification a mangé le calendrier.** 99 % des fichiers v7 sont des reçus ; la v8 a fait 34 tranches sans
   tour ; la v11 compte 3 695 portes. L'utilisateur est intervenu trois fois.
7. **GPU : six greffes en étage**, toutes bornées par l'hôte. Les noyaux pèsent 1 à 4 % de l'étage.
8. **Ce qui a marché** : doctrine entière exacte, oracles bornés, portes à code exact, mutants causaux, fixtures
   minimales, registre des preuves, sessions G4 gardées.
9. **Jamais tranché depuis des semaines** : froid ou résident (posé le 7 août), maximum ou médiane, GPU contractuel,
   périmètre, trame brute, vérité terrain (§ 2.3).
10. **La bibliothèque produit est figée depuis le 9 août**, sans producteur. Le registre formel est gelé en phase 15.
    `CLAUDE.md` et `AGENTS.md` sont en retard : la v11 y est encore en u18, et la v10 n'y figure pas.
11. **PASSATION § 3 et AUDIT_FINAL § 14.2 : vrais sur le fond, liste partiellement fausse** (§ 7). « T > 0 » n'agit
    pas sur LiDAR, « J3 » n'a jamais été dans la v10 mesurée, et la cause principale — le coût par unité de travail —
    manque.
12. **Le plan 100 ms du 4 octobre** a eu raison sur le plafond des transpositions (au mieux le niveau de la v10),
    mais presque tous ses leviers chiffrés ont été démentis (§ 8).

---

## 1. Chronologie version par version

### 1.0 Avant la lignée : `HGP-old` et la ligne produit `morsehgp3d/` (« v1 »)

**`HGP-old/`** (7–29 juillet, Python et Cython, licence non commerciale) est la première implémentation de la thèse.
L'utilisateur le 28 septembre : « ne doit pas être un oracle » [M].

**La ligne produit `morsehgp3d/`** (14 juillet → 9 août, 434 commits).
- Organisation : phases 0–15 du registre formel ; pivot du 29 juillet vers un catalogue exact de paires en une passe
  (`docs/HISTORIQUE.md`).
- Contrat (`docs/research/CONTRAT_50K_BILAN.md:9-28`, 7 août) : 50 000 points, K_max = 10, p95 `warm_e2e` < 100 ms.
- Le même document note que le contexte CUDA coûte **1 242 ms à froid contre 18 ms à chaud**, et que « les deux
  contrats n'existent que dans un service résident ».
- Mesures : étage des paires ≥ 299 929 ms (censuré) ; réducteur seul 2 792 ms.
- Le seul « succès » est `d69539a` : p95 de 95,8 ms à 50 000 points. Il construit un arbre sur les points, pas la
  hiérarchie des facettes, et a été reclassé `rejected_point_mst_surrogate` (registre l. 940-970).
- Survivent le réducteur exact `build_exact_point_hierarchy(CertifiedTowerInput)` (`a3d235b5b`) et le registre des
  pistes abandonnées.

### 1.1 Tableau d'ensemble

| Version | Dates, commits | Objet calculé | Architecture | Précision | Meilleur résultat mesuré, sur le contrat du moment | Fin |
|---|---|---|---|---|---|---|
| v2 | 8 août (1 jour), 6 | arbre de fusion de d_K : sphères critiques de rang fermé K (minima) et K+1 | catalogue local par point (dual inversif), forêt multifurquée | i128 + `BigInt`, grille 16 bits | n = 200, K = 10 : 92,5 s (cube de n), local 2 vCPU | « generator condemned rather than slow » (`ff519be74`) |
| v3 | 8 → 16 août, 412 | juge Γ_k exhaustif ; dix forêts jamais livrées à l'échelle | germination sur l'arête diamétrale, partition WSPD (Callahan–Kosaraju), lanes q2/q3/q4, h_q = s_max − q + 1, certificats de blocs | u16 | G4 CPU, uniform 50k : 78,8 s pour la chaîne (4 processus × 12 fils) ; autres familles en n^2,2–3,2 ; NO-GO | post-mortem WSPD, relecture de la thèse |
| v4 | 17 → 27 août, 161 | « K-MST du K-graphe de Gabriel » = fold, **faux en général (E5)**, sans verticales | un seul arbre radix de Karras (Morton 48 bits), WSPD par vagues, `ball_stream`, census I_B/U_B, fold compact, monolithe `forest_probe.cpp` | u16, i64/i128/U192/U320 | local 4 vCPU, n = 8 000 : uniform K10 343 s, K5 54 s ; eight_clusters K10 : timeout à 5 403 s ; **aucune exécution G4** | audit `bab37b9` : « référence crédible sur domaine borné » ; recommandation « garder comme oracle » non suivie |
| v5 | 27 → 31 août, 314 | même objet (`verified_events_only`) | v4 réécrite : flux par ordre K, modules, registre de mutants | u16, s ≥ 8 | G4 W48 K1..10 : uniform 50k 219 → 56,6 s ; 200k : 253 s, 72 Gio ; scanline en n^2,72 ; GPU plus lent | `linked_arcs_u16` : Θ(n²) boules critiques ; « sous-quadratique pour toute entrée » irrecevable |
| v6 | 31 août → 2 sept., 200 | même objet | génération q3/q4 « sensible à la sortie », familles stationnaires, paliers mémoire | u16, doublons refusés | G4 50k : K10 47,4 s, K5 9,07 s (uniform) ; mur de résidence ≈ 4,8·10^5 points à K10 ; GPU série C −10,4 % | v7 reprend la v6 telle quelle et change d'objet |
| v7 | 4 → 11 sept. (+1 le 22), 123 | **FULL** : minima Gabriel K, multifusions K+1, parents, portails silencieux, plateaux non réguliers | port v6 + tour FULL par boules (MEB à coquille libre, ancres, journal daté) | u16 | G4 50k uniform, FULL mono-fil : K1..10 418,9 s, K1..5 33,9 s | audit demandé le 13 sept., refonte en v8 |
| v8 | 13 → 22 sept., 170 | **aucune tour** : générateur exact q2/q3/q4 seul | 34 tranches : front WSPD réel, census q2, Pool, workers ; puis q3/q4, Local28/Window30, raccord global | u16, puis float32 (21 sept.), puis u18 (`a74e90f22`, 22 sept.) | flux seul : trames brutes K5 W48 165 / 34 / 505 s ; sans sol 1 mm, scène 0, K5, W8 local 104,6 s | audit v9 : « pas la tour » |
| v9 | 22 → 28 sept., 629 | FULL v7 portée (identique à 53 %), générateur v8 porté (identique à 84,5 %) | catalogue recoupé, juge T2, voies GPU S1–S4b, 31 leviers booléens, sondes v1 à v30 | u18 (1 mm) | G4 K5, chaîne interne : R1 18,8 / 15,1 / 29,3 s, puis R22 **0,926 / 0,760 / 0,983 s** (GPU ; moteur CPU 2,77 / 2,18 / 3,00 s ; mur du processus 1,52–1,87 s) ; K10 2,96 / 2,27 / 2,89 s | audit critique : générateur quasi quadratique sur amas (0,44·n²), cubique sur coquilles ; complexité |
| v10 | 28 sept. → 2 oct., 152 (moteur figé de fait le 29 au soir, environ 29 h de code moteur) | FULL inchangée + tête de clustering | **boîtes de centres** (lentille L13), « le WSPD et le paramètre s disparaissent » ; tour par minima, morceaux de Gordan et descentes ; `SiteTree` k-d | u18 ; grille u32 par paliers décidée le 30 sept., jamais raccordée | G4 `777406b82` K5 : **252,0 / 204,2 / 253,6 ms** ; K10 1 124,6 / 861,4 / 1 024,3 ms (CPU W48, 3ᵉ passe chaude) | utilisateur : « repartir de zéro pour avoir quelque chose de plus propre » ; raccord R2 jamais importé |
| v11 | 2 → 7 oct., 362 | FULL (K ≤ 12) + sorties `supports`, `points`, `plat` | catalogue passe unique, voie GPU feuille par fil, pipeline des forêts | u21 (u18 et u24 explicites) | voir § 1.3 | utilisateur : « tout reconstruire à neuf pour une v12 » |

Sources du tableau :
- **v2–v4** : `morsehgp3D_v2/RESULTATS.md` § 3 bis ; `morsehgp3D_v3/README.md:7-15` ;
  `receipts/chaine_complete_g4_20260813` ; `morsehgp3D_v4/docs/MATHEMATIQUES.md:80-91, 504` ;
  `receipts/campagne_locale_n8000_20260817/RECU.md`.
- **v5–v6** : `morsehgp3D_v5/README.md:7-15` ; `audits/ETAT_COURANT.md:37-99` ; `docs/ECHELLE.md` ;
  `morsehgp3D_v6/docs/ECHELLE.md` ; sessions `d98f4729`, `b97f20ea`, `c8f69673`.
- **v7–v8** : `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md:17-36` ; `AGENTS.md:307-449` ;
  `morsehgp3D_v8/receipts/q34_spatial_20260921/README.md:96-112` ; `docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md:93-112`.
- **v9–v10** : `morsehgp3D_v9/receipts/g4_tower_r22_20260926/README.md:59-66` ; `morsehgp3D_v10/README.md:1-60` ;
  `morsehgp3D_v11/docs/AUDIT_FINAL_V11.md` § 3.2.

### 1.2 Ce que le tableau ne dit pas

- **v2.** L'objet était juste dès le départ (« il n'y a jamais de facette de cardinal K »,
  `morsehgp3D_v2/README.md:12-16`), mais le générateur était une force brute « en attendant ». Un élagage faux
  perdait 0,63 % du catalogue sans qu'aucun accord moyen ne le voie. Cause de l'échec (`b113edb47`) : les points de
  profondeur de Tukey ≤ K ne sont pas certifiables, d'où l'ancrage sur une arête. Survivent les fixtures R1–R8,
  dont R3 (rang non héréditaire).
- **v3.** Interdits posés dès l'ouverture : aucune structure de Delaunay, aucun catalogue résident, aucun tableau
  indexé par paires ou triplets. Les certificats de blocs (SOC64, BlockJungDual…) ont souvent coûté plus qu'ils
  n'économisaient. Survivent le juge Γ_k, `PISTES_FERMEES.md` et les règles du 16 août (tailles 8k/16k/32k,
  « jamais de vérification exhaustive », `46f6beca9`).
- **v4.** Le document fondateur (`f775c9881`, 07:57) adopte le K-MST de Gabriel une heure **avant** que le corpus
  de lecture ne consigne E5 (`audits/lectures_20260817/docs_autorite.md:128`, `bebdef2a4`, 08:54). Il n'a jamais été
  corrigé, et le juge construisait le même graphe que le sujet (`tests/forest_selftest.cpp:9-18`). Survit la
  doctrine des portes : `cmake/run_expect.cmake` (codes 0–4, signal refusé), planchers `--min-*`, 74 mutants,
  `--relabel-gate`, `--par-gate`, banc apparié intra-processus.
- **v5.** Conforme à la v4 au digest près 18 minutes après l'ouverture : le code était prêt hors dépôt [I].
  14 sessions G4 en 22 h, dont 3 en échec.
- **v6.**
  - Génération neuve sans gain : eight_clusters +12 %.
  - Correctif P0 du coefficient 4 du cover q4 (`381ba60b4`) : la conformité v4/v5 avait gravé le défaut.
  - Familles terrain et scanline du dépôt dilatées (hauteur ∝ √n).
  - Le 2 septembre passé en harnais suscite la directive « Ne t'égare pas avec trop de garde-fous » [M].
  - Son travail non commis (P4, C6a) a été porté octet pour octet dans la v7 (`morsehgp3D_v7/docs/V6_SOURCE_SNAPSHOT.json`).
- **v7.** Le prix du bon objet vaut environ ×9 de temps à 50k K10 (418,9 s contre 47,4 s pour l'objet réduit v6).
  Survivent la borne Ω(N²) de la sortie dès K = 2, les certificats MSF composables
  (`audits/receipts_composable_msf_20260911`), le plafond d'Amdahl ≈ 11× entre ordres
  (`NOTE_CLAUDE_DECOUPE_TOUR_20260911.md`) et le juge T2.
- **v8.**
  - Le contrat change trois fois le 21 septembre, puis la décision « entier 18 bits » annule le float32 le 22.
  - 21 tranches portent sur q2, alors que q3 domine le LiDAR (361 G tests de census q3 à 8k, `AGENTS.md:427`).
  - Croissance ×10 par doublement en G4 R3 : 8k en 614,7 s, avec 3,2 CPU occupés sur 48.
  - La tour v7 n'est pas portée.
  - Survivent le port 18 bits, Patchwork++ `3e6903a1`, le protocole de chronométrage des trames et le juge
    bilatéral d'échantillon.
- **v9.** Ce n'est pas une reprise de zéro (générateur identique à 84,5 %, tour à 53 %, A9-82).
  - La chaîne interne de 0,76–0,98 s correspond à un mur de processus de 1,52–1,87 s (A9-51).
  - Le « mode résident » a rejoué la même trame.
  - 33,5 k lignes de code pour 72,6 k lignes de Markdown.
  - Le commit « it beats HDBSCAN's oracle » (`cda636b5e`, 28 sept.) reposait sur le repli Gabriel (E5 : 18 / 88 / 416
    racines contre 1), une condensation fausse et un oracle 1D. Une fois corrigé : ARI 0,734, contre 0,738 pour
    HDBSCAN (A9-91).
- **v10.** C'est **la seule reprise algorithmique gagnante** : environ 29 h de code moteur pour ×3,7–3,9 contre la
  v9 GPU et environ ×11 contre son moteur CPU.
  - Dettes :
    - marges flottantes figées (`tower.cpp:27`) ;
    - monolithes (`tower.cpp` : 1 867 lignes) ;
    - raccord R2 `865f5e6` jamais importé ;
    - aucune section V10 au registre des preuves ;
    - dix errata, dont « 1 s tenu » qui ne portait que sur un seul ordre (`receipts/ERRATA.md`).
  - Clustering : la tête « première couverture » bat sklearn aux six K testés en synthétique. Mais à entrée et tête
    égales, la tour égale HDBSCAN « MR-bord » à 0,01 près [M `clustering-depuis-la-tour.md`]. Sur LiDAR, aucun
    avantage à K5 hors des trames choisies pour les échecs de HDBSCAN : +0,0002 [−0,005 ; +0,004] sur 299 trames
    [fork D].

### 1.3 La v11 en un paragraphe

**Calendrier et objet.** Ouverte le 2 octobre (`52687f8e5`), moteur gelé le 7 octobre (`ac081a06f` = `733912e65`),
362 commits, dont ≈ 92 d'auditeurs. Elle recalcule exactement la tour FULL (K ≤ 12, entiers à budget de bits,
profil u21) et livre quatre sorties transactionnelles.

**Exactitude.** Aucun FULL faux en au moins 126 sessions G4 ; voie GPU égale au CPU à l'octet.

**Temps au gel** (`claudeg1`, ms, ng00 / ng01 / ng02) :

| Mode | K5 à chaud | K5 à froid |
|---|---|---|
| GPU | 251 / 212 / 255 | 335 / 301 / 345 |
| CPU | 314 / 255 / 313 | 343 / 272 / 329 |

K10 GPU à chaud : 1 782 / 1 336 / 1 536 ms. Référence v10 : K5 252,0 / 204,2 / 253,6 ; K10 1 124,6 / 861,4 / 1 024,3.

**Ce que la v11 a manqué.**
- Le contrat de 100 ms.
- Le dépassement de la v10.
- Le différentiel canonique sur trames entières.

Elle porte un harnais de qualification sévère : 3 695 portes et 485/485 mutants au 5 octobre (`98a009550`).

**Clôture.** Le 7 octobre, l'utilisateur décide de tout reconstruire à neuf
(`morsehgp3D_v11/PASSATION.md` § 0-2 ; `docs/AUDIT_FINAL_V11.md` § 1-3).

---

## 2. Contrats et décisions de l'utilisateur

### 2.1 Généalogie du contrat de temps

| Date | Énoncé | Source | Objet réellement calculé |
|---|---|---|---|
| 7 août | 50 000 points, K_max 10, p95 `warm_e2e` < 100 ms (secondaire 1 s) ; « warm » : contexte CUDA hors chronomètre | `docs/research/CONTRAT_50K_BILAN.md:9-28` | substitut point-MST (rejeté) |
| 8 août (v3) | même chose, « 100 ms principal » | `morsehgp3D_v3/PROPOSITION.md:19-24` | dix forêts (jamais à l'échelle) |
| 17 août (v4) | K1..10 < 100 ms sur G4 ; secondaire K5 < 1 s ; « dizaines de millions » | `morsehgp3D_v4/README.md:17-20` | fold (faux) |
| 27–28 août (v5) | contrat 50k **mesuré** (aucun seuil) ; cible 10–30 M points, K10 principal, K5 secondaire | `morsehgp3D_v5/docs/PLAN_DE_TESTS.md:103`, `docs/ECHELLE.md:18-31` ; [M `cible-echelle-g4.md`] | fold |
| 2 sept. | « minimum de garde-fous », multi-CPU/GPU de 10^4 à 10^7 points, feu vert G4 | [M `directive-echelle-multicpu-gpu.md`] | fold |
| 4–5 sept. (v7) | 50k : **toute la tour K1..10 < 1 s** sur G4, repli K1..5, puis 100 ms ; mono → multi-CPU → GPU | `morsehgp3D_v7/docs/CONTRAT_PERFORMANCE.md:3-9` ; `docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md:3` | FULL |
| 21 sept. | **trame SemanticKITTI entière** (aucun sous-échantillonnage), K1..10 < 1 s, repli K5, puis 100 ms ; float32 d'origine par défaut | `AGENTS.md:116-138` ; `morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` | flux seul |
| 21 sept., 20:10 | régime prioritaire : **LiDAR sans sol 30 000–60 000 points**, 1 s puis 100 ms, K5 ou K10 | `AGENTS.md:52-57` | flux seul |
| 22 sept. | « continuons en entier 18 bits » ; « ne développe pas plus pour le float32 » ; multi-millions secondaire | `AGENTS.md:56-61, 71-72` | — |
| 28 sept. (v10) | même objet ; complexité jugée « d'abord sur le régime LiDAR » ; clustering contre `sklearn.cluster.HDBSCAN` | [M `ouverture-v10.md`] ; `morsehgp3D_v10/README.md` | FULL |
| 2 oct. (v11) | « 100 ms sur nuages LiDAR sans sol (éventuellement avec sol), K = 5 et si possible K = 10 » ; comparer à HDBSCAN sur synthétique et réel | `AGENTS.md:7-13` | FULL |
| 3 oct. | « essaie de respecter 200 ms à K5 sans sol » (jalon) | [M `ouverture-v11.md`] | FULL |
| 4 oct. | « l'objectif du contrat est toujours 100 ms » | `…/audit_transpositions/CONTEXTE.md` | FULL |

[I] Le chiffre de 100 ms a traversé trois objets (substitut, fold, FULL), deux régimes (synthétique 50k, puis LiDAR
sans sol 30–60 k) et un changement de taille de référence. **Aucune mesure n'a jamais fondé sa faisabilité pour
FULL.** Les jugements de faisabilité sont restés des estimations :
- PISTES_DE_RUPTURE (2 oct.) : « atteignable en CPU seul à K = 5 (65 à 91 ms), sans marge » ;
- plan du 4 octobre : « non démontré, pas exclu ».

### 2.2 Décisions datées, et leur statut pour la v12

| Date | Décision | Source | Statut proposé pour la v12 |
|---|---|---|---|
| 16 août | tailles 8k/16k/32k ; jamais de vérification exhaustive | `docs/TEST_PLAN_MORSEHGP3D.md` § 3.1–3.2 (`46f6beca9`) | en vigueur pour les pentes et l'oracle ; **subordonnée** au LiDAR réel pour toute décision (6 oct.) |
| 14 sept. | « ne prends jamais s en-dessous de 8 » | `morsehgp3D_v8/audits/SEPARATION_20260914.md:9` | **en vigueur si une WSPD revient** ; sans objet depuis la v10 (« le WSPD et le paramètre s disparaissent ») |
| 21 sept. | « la croissance sur les régimes considérés prime sur une borne sous-quadratique universelle » | `AGENTS.md:425` | en vigueur |
| 21 sept. | « pas de confrontation à la vérité terrain » | `AGENTS.md:64-66` | **renversée pour le clustering** le 1ᵉʳ oct. (voir plus bas) ; garder la règle « les étiquettes ne bloquent pas les chronos » ; à réénoncer dans `AGENTS.md` |
| 21 sept. | trame entière avec sol = contrat principal | `AGENTS.md:116` | **ambiguë** : non retirée selon `AUDIT_V8_SYNTHESE.md`, mais « éventuellement avec sol » le 2 oct. ; à trancher |
| 22 sept. | livrable qui fonctionne, trancher les détails ; multi-millions secondaire | [M `livrable-qui-fonctionne.md`] ; `AGENTS.md:71-72` | en vigueur |
| 28 sept. | HDBSCAN = sklearn tel quel, jamais réimplémenté ; comparer à K = `min_samples` | [M `hdbscan-sklearn-reference.md`] | en vigueur |
| 28 sept. | « HDBSCAN ne peut pas battre la tour » : concevoir la tête depuis le modèle ; ni HGP-old, ni la thèse, ni Γ_k ne sont des oracles du *bon* clustering | [M `clustering-depuis-la-tour.md`, `these-source-pas-autorite.md`] | en vigueur |
| 29 sept. | « Envoie le lot C ou tout calcul trop coûteux/long sur G4 ; on n'a pas toute la vie ! » | [M, cité par le fork D] | en vigueur (G4 pour le lourd) |
| 30 sept. | convention de niveau de la thèse dans toute comparaison ; deux triangles (§ 6.1) | [M `hdbscan-echoue-deja-k2.md`] | en vigueur (exception déclarée pour les vidéos, 5 oct.) |
| 30 sept. | grille u32 par paliers (u24 puis u32) | [M `precision-grille-u32-v10.md`] | **remplacée** par u21 par défaut (2 oct., `AGENTS.md:19-20`), confirmée le 6 oct. [M `profil-u21-par-defaut.md`] ; aucun palier u32 abouti |
| 30 sept. | « Le grand verrou mathématique » : passer de la tour FULL à la hiérarchie laminaire sur les points | [M `verrou-full-vers-points.md`] | ouvert (la v11 a retenu H^r_{k+1}, sans stabilité par insertion) |
| 1ᵉʳ oct. | « Priorité aux tests synthétiques et au bon découpage (précision, rappel) par rapport à la ground truth » ; « Il est impératif d'être meilleur que HDBSCAN » | [M, cité par le fork D] | **renverse** pour le clustering la consigne du 21 sept. ; en vigueur |
| 1ᵉʳ oct. | ordre : la tour contient-elle l'information, puis la hiérarchie, puis seulement z | [M `ordre-tour-hierarchie-puis-z.md`] | en vigueur |
| 2 oct. | « Fais les tests sur G4 » ; un worktree par acteur ; push sur `main`, sans branche | `AGENTS.md:31-37` | en vigueur |
| 3 oct. | « Q2 ou Q3 ne sont que de peu d'importance par rapport au modèle mathématique » | [M `modele-avant-cibles-q2-q3.md`] | en vigueur |
| 5 oct. | workflows légers en local, lourd sur G4, un workflow à la fois | [M `workflows-legers-g4-lourd.md`] | en vigueur |
| 6 oct. | « Il vaut mieux toujours tester sur données LiDAR réelles » | [M `tester-sur-lidar-reel.md`] | en vigueur, en tension avec la règle absolue des tailles 8k/16k/32k de `CLAUDE.md:15` |
| 7 oct. | v12 « reconstruite à neuf » | `morsehgp3D_v11/PASSATION.md` § 0 | — |

### 2.3 Ce qui n'a jamais été tranché

1. **Froid ou résident.** Question posée le 7 août (`CONTRAT_50K_BILAN.md:24-28`), reposée le 4 octobre (plan,
   § 4.5) et encore le 7 octobre (`PASSATION.md` § 6.1, décision 1).
2. **Maximum ou médiane de la plage.**
   - 85 des 127 trames c08 sans sol dépassent 60 000 sites [N, plan § 4.2] : la plage déclarée exclut la majorité
     des trames de cet échantillon.
   - La cible effective au pire de la plage est contestée : 54–77 ms selon le verdict, 62–100 ms selon la critique
     P4 [N].
3. **GPU dans le chemin contractuel**, et périmètre des 100 ms : FULL seul, ou aussi les sorties ? Le vidage FULL
   pèse 255 à 329 Mo [N, critique P10].
4. **K10** : contrat ou objectif ? Le plan du 4 octobre proposait d'annoncer 0,3–0,4 s.

---

## 3. Registre des pistes fermées de la lignée

Règle commune (`docs/archive/abandoned/README.md` ; `morsehgp3D_v3/audits/PISTES_FERMEES.md`, « Réouverture ») : une
piste ne se rouvre qu'avec un nouveau théorème de complétude, une fixture qui falsifie le motif d'abandon, une
architecture sans structure interdite et une porte de coût distincte. La v9 a ajouté une distinction essentielle
(`morsehgp3D_v9/docs/FAUSSES_PISTES.md:9-17`) : une fermeture par **preuve ou fixture** est définitive ; une
fermeture par **mesure**, **modèle de coût** ou **consigne** ne vaut que pour son régime.

### 3.1 Invariants d'architecture (définitifs)

- **Pas de mosaïque de Delaunay d'ordre supérieur, pas de Γ global, pas de catalogue cellulaire en C(n,k).** La
  raison : on matérialiserait exactement ce qu'il faut éviter. Ce qui survit : les oracles bornés de `reference/`
  (`docs/archive/abandoned/README.md` ; v3 Q14 interdit même Delaunay d'ordre 1). E-HGP l'a démontré en grande
  dimension : les naissances de la tour y valent exactement C(n,k) en d = 100 [M `ouverture-e-hgp.md`].
- **Pas de tour globale de boules saturées énumérée exhaustivement.** Elle reste un repli de preuve borné.
- **Pas de grille, DTM, lissage ou ANN comme définition exacte** : ils changent la filtration.
- **Pas de fenêtre Morton fixe, ni de préfixe kNN, comme autorité exhaustive.** Deux amas colinéaires de M+1 points
  suffisent (registre l. 88) ; fixture v3 « 50 000 IDs à 12 499 distracteurs ». Morton ne sert que de clé de tri.
- **Pas de MST d'atteignabilité mutuelle sur les points pour K ≥ 2** : il oublie l'identité des facettes. Le
  « surrogate point-MST » du produit a été rejeté.
- **Un seul arbre spatial ; pas de plafond de population dans un critère terminal**, qui forcerait ≥ C(n,2)/C²
  rectangles (v5 `PISTES_FERMEES.md:7-30`).

### 3.2 Objet et mathématiques (définitifs, par preuve ou fixture)

| Piste | Cause | Ce qui survit |
|---|---|---|
| Graphe brut des cofaces Gabriel / « fold » v4 / K-MST élagué (Prop. 6, Th. 5 de la thèse) | E5 : une attache silencieuse non-Gabriel ; fausse naissance, puis fausse fusion à 24 au lieu de 83886/3563 ; minima isolés et K = n absents (registre l. 40-41, 129) | FULL v7 sous régularité (`conditional_theorem`, registre l. 129-135) ; attaches datées |
| Minima Gabriel avec leurs seules adjacences induites | fixtures à 4 points (fusion 477/34 manquée) | transfert des chemins vers les minima |
| q−1 bras pour une multifusion | contre-exemple n4 | q ≤ 4 bras essentiels en 3D |
| Publier tous les niveaux Γ | suffisance prouvée des minima et des multifusions | portails internes au calcul |
| Identifier une composante par sa couverture de points, ou une boule par son support, son arité ou son rayon | deux identités de même couverture | identités FULL |
| p + u ≤ s_max pour les coquilles non régulières | coquille de 7 points donnant une naissance à K5 | p + q_min ≤ s_max |
| Rang héréditaire, fermeture par les paires de rang utile | triangle aigu de rang 3 dont les trois côtés sont de rang 4 (registre l. 72-74 ; v2 R3) | énumération directe des triangles et tétraèdres |
| Wedges Delaunay, étoile fermée, carré, fans pour Γ₂ | fixtures à 6 et 8 points | preuve bornée |
| Arrondir un rayon d'une unité vers le haut | deux fixtures q3/q4 à norme irrationnelle | arithmétique dirigée stricte |
| Décomposition ternaire symétrique en O(n) blocs pour q3 | famille cercle–axe en Ω(n²) (Th. 4) | WSSD approchées, sources asymétriques |
| Sous-quadratique pour toute entrée | `linked_arcs_u16` (v5) ; borne Ω(N²) de la sortie dès K = 2 (v7) | contrat « sensible à la sortie », sur les régimes visés |
| Accès q3/q4 conditionné par q2/q3 acceptés | six contre-fixtures rationnelles (v8 tranche 15) | voies indépendantes |
| α3 = 3 sur q4 ; cover q4 au coefficient 3 | tétraèdre entier u16 ; tétraèdre régulier + z (v6) | coefficient 4 pour q4 |
| Distance-à-la-mesure = D_k ; naissance par intérieur strict | encadrement seulement multiplicatif ; fixture F1 (E-HGP) | — |

### 3.3 Générateur : la lignée WSPD (v3 → v9), close par la v10

La v10 a remplacé le générateur WSPD par les boîtes de centres, linéaires mesurées, contre un front v9 quasi
quadratique sur amas et cubique sur coquilles (`morsehgp3D_v10/README.md:28-35`). C'est une fermeture par mesure et
nouvel algorithme. Les pistes ci-dessous sont donc caduques tant qu'aucune WSPD ne revient [I].

| Version | Pistes fermées |
|---|---|
| v3 (`PISTES_FERMEES.md:14-58`) | front de Jung coalescé (pentes 2,30) ; chambres Yao-48 (54,74° pour 35,26° exigés) ; cônes locaux (faux vert) ; sentinelle ; lentille aiguë (aigu sur 300/300 paires) ; Helly (≈ 180 bits) ; cœur de Jung universel ; fenêtre top-M (1 277 supports jamais proposés) ; arrangement relevé ; apex unique pour h_a |
| v4 | sélection axiale (+7 %) ; couches convexes pour q3 ; étage i64 du préfiltre q4 (médiane 1,0021) |
| v5 | raffinement après séparation (−47 % d'ancres, +34 % de mur) |
| v7–v8 (`morsehgp3D_v9/docs/FAUSSES_PISTES.md:86-150`) | histogrammes locaux O(\|A\|²+\|B\|²) (2,04 → 8,39 → 31,08 s) ; témoins universels (deux rangées parallèles n'ont aucun témoin) ; Tubes (0 crédit) ; Pool (amas seulement) ; lots de singletons (×1,005–1,225) ; Donate, continuations, équipe persistante (pas de gain stable) ; micro-variantes q2 (fermées par **consigne**) ; Window30 (médiane 1,153 plus lente) ; couches duales (×10,27 sur l'adversaire) |
| v9 | crédit par nœuds (CPU +27–32 %) ; index des selles (10,2 M entrées pour 1,01 M MEB évitées) ; 16 plus proches voisins (+3,7 % à K10) ; raffinement des rectangles (+12–42 %) |

Deux pistes restent explicitement **différées, non fermées** (v9 `FAUSSES_PISTES.md`) : les blocs Z le long de la
descente, et les couches duales comme filtre des graines q3. Elles ne valent que si une WSPD revient.

### 3.4 Numérique

| Piste | Cause | Ce qui survit |
|---|---|---|
| Float32 natif, repli de 1 728 bits | cinq tranches qualifiées le 21 sept., abandonnées le 22 par l'utilisateur | fermeture par consigne |
| Réduire le pas u16, élargir implicitement | collision (0,1,0)/(0,0,65536) ; 200 000² tronqué en u32 ; 2^137 > i128 | profils portés par paliers, bornes certifiées |
| Marges flottantes figées v10 (`kMargin`, `kApproxMargin`) | prouvées pour u18 seulement, arrondi supposé, contraires à F1–F6 | filtre certifié par expression |
| Filtre F6 des signes de puissance (v11) | environ 1 % : le coût est le parcours | — |
| `-march` global | dans le bruit ; essai sous-dimensionné [N] | à rejouer à W1 si besoin |
| Enveloppe empirique de 64 ULP (produit) | `heuristic_only` (registre l. 950) | — |

### 3.5 Tour et forêts

| Piste | Cause | Source |
|---|---|---|
| Copier dix fois le constructeur ; alias eager de toutes les facettes | plafonds à 16k/K9 et 32k/K7 | v7 `FAUSSES_PISTES.md` |
| Fenêtres de résolution indépendantes en mono | 188,6 → 250,4 s | idem |
| Journal incrémental possédant | +30 % de mémoire retenue | idem |
| Publication par Borůvka à étiquette minimale (v9) | 1 264,7 ms à K5 contre 36–52 ms ; perd le recouvrement | `morsehgp3D_v9/audits/b_full_a_real_20260927/RESULTATS.md` |
| Partager la géométrie entre les K forêts | une boule régulière ne travaille qu'aux ordres m−1 et m | v9 `FULL_PARTAGE_INTER_ORDRES_20260926.md` |
| Saut par orthants | 1,32 pas par trace | v10 |
| Frontière en largeur à barrières, grain de 19 482 tâches (v10) | ×2,1 seulement de 1 à 48 fils | reçu S4 v10 [N] |
| Coquilles étendues par énumération brute (v10) | 24 points cosphériques : 49–57 s à K5, refus après 82 s à K10 | budget a priori (fork D) |
| Transport q4 des intérieurs ; encodeur FULL général (v9) | même en supprimant tout le census tardif, il reste 816 ms de chaîne ; environ ×2 contre le natif | v9 `FAUSSES_PISTES.md` |
| Kruskal par lots de la v10 | plancher séquentiel de 33–47 ms à K5 ; ×2,6–3,2 plus lent qu'un noyau sans lots | à garder comme **référence de porte** [N] |
| Mémos de lane ; lots ordre par ordre (v11) | 2,6 % de succès ; environ 600 barrières | `PASSATION.md` § 4 |
| Euler comme certificat | omissions qui se compensent (fixture D/T à 13 points) | garder comme filet |

### 3.6 GPU : un motif plus qu'une piste

| Tentative | Mesure | Source |
|---|---|---|
| v3, balayage axial | exact ; ×4–6 seulement contre 48 cœurs | `morsehgp3D_v3/receipts/axis_cuda_g4_20260815` |
| Produit, frontière Morton–Yao48 | 2,4 s pour les seules paires ; 7,0–7,5 s à chaud, dont 4,8–5,4 s de recertification CPU | `docs/validation/phase15_*` [N] |
| v5, tout sur le device, matérialisé | plus lent partout : eight_clusters 718 s contre 246 s ; 40–95 Go de copies vers le device | `morsehgp3D_v5/docs/GPU.md:13, 549-565` |
| v6, série C | 154 ms de noyaux sur 7 717 ms d'étage ; −10,4 % de bout en bout ; plafond ×1,31 | `morsehgp3D_v6/docs/GPU.md:217-222` |
| v7, census | 29,8 ms de noyaux, 846 ms d'étage (419,5 ms de reconstruction hôte) | `morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910` |
| v9, S1–S4b | gains réels (R12 −15 à −25 %) mais attachés au CPU ; contexte CUDA par processus ; débits S7 de la v10 invalides | [M] ; `morsehgp3D_v10/receipts/ERRATA.md` |
| v11, feuille par fil | catalogue ×0,84–0,88 contre la v10 CPU ; environ 3 fils actifs sur 32 ; N1, L4 et feuille coopérative rejetés | `AUDIT_FINAL_V11.md` § 1, 6 |

**Doctrines écartées** :
- (v6 C6) lire les structures hôte dans leur ABI ;
- (v6 C6) partager une table CPU/GPU qui modifierait la route CPU ;
- un DSU ou un `reduce` sur le device sans théorème d'équivalence.

**Survit** (contrat R7 de la v11) :
- exact sur le device, ou `unresolved` repris sur CPU avant admission ;
- replis comptés ;
- capacités comptées en count–scan–emit ;
- contexte chaud déclaré.

### 3.7 Points et clustering

- **Routage descendant par vote (produit), routage médian, vote du § 9.1, ER0h.** Discontinus, sans constante de
  stabilité uniforme (v11 `HIERARCHIE_POINTS.md` § 5-6) [N].
- **Marge en niveau carré (Q₁ de la v10, ER0h).** Aucune constante uniforme (`PASSATION.md` § 4).
- **Supports v1 (tous les Q_b), `kparties_reliees`.** Instables, Hausdorff ≥ 1/4 (idem).
- **Seuil de condensation relatif au parent.** Ne garde que 9 objets sur 19, et aucun vélo contre un mur
  (`AUDIT_FINAL_V11.md:663-665`).
- **z < 1** : réfuté. ẑ s'effondre (coquilles, K10). Les têtes multi-K ne gagnent pas 0,02. Rétrécir les amas EOM
  ne répare rien. Il faut publier toujours z = 1, avec z = 3 canonique [M ; fork D].
- **« Géométrie exacte ⇒ meilleur clustering » (v10).** Parité avec MR-bord à entrée et tête égales.
  - Ce qui survit : publier toujours les témoins MR₁-bord et MR₂-bord à côté de la tour.
  - Sur les 7 échecs de HDBSCAN des démos Zoltan, la tour échoue aussi à K5 et K10 (rapport L03 [M `ouverture-v11.md`]).
- **Entrée `cover` dépendante du rang Morton (v10).** 47 sites changent de classe sous un échange d'axes.
  Ce qui survit : publier l'ensemble cover.
- **Le réducteur produit et la tête v10 comme base de code.** `cpp_int`, `std::map`, monofil (2,79 s à 50 000
  points d'ordre 1) ; condensation fausse aux cohortes [N, plan § 5.5].

### 3.8 Méthode et processus (fermetures par incident)

**Mesure.**
- Constantes comparées entre processus : ±40 % (v5).
- Médianes de cinq prises à W48 : il faut 29 à 44 paires pour voir 5 % [N].
- Projection en CPU·s/48.
- « Impossible » prononcé sans borne inférieure (verdicts v9 réfutés par la v10).
- Gain local supposé valoir sur G4 : THP −17 % en local, +6,4 % sur G4 [M].
- Gain déduit des instructions : V3 ×0,565 en instructions mais ×0,925 en temps.

**Portes.**
- Portes vacueuses (`WILL_FAIL`, regex) en v3.
- Digest de candidats pris pour une conformité (v5).
- Juge partageant la proposition du sujet (v4).

**Infrastructure.**
- Sentinelles périmées.
- `ctest` dans un build épinglé.
- Index Git partagé : `4c3cdb0c` a emporté 300 fichiers.
- Script édité en cours d'exécution.
- Diagnostics SSH superposés à la collecte G4 (v9 `FAUSSES_PISTES.md:190-203`).

### 3.9 Pistes rouvertes à tort et fermetures fragiles

1. **E5, « pour la cinquième fois »** (`AUDIT_V9_CRITIQUE_20260928.md:800-811`, constat A9-91, origine comprise) :
   - fermée en v3 (`PISTES_FERMEES.md:53`) ;
   - rouverte comme objet par v4/v5/v6 ;
   - recommandée par l'audit de reprise v8 (`AUDIT_REPRISE_DEVELOPPEUR_20260921.md:196-197`) ;
   - rechute v9 le 27 septembre (26 racines contre 1) ;
   - rechute v9 le 28 septembre, en 15 h, E5 absente des tests.

   La v11 l'a enfin gravée dans sa référence (`morsehgp3D_v11/reference/README.md:201`, `hgp11_ref/families.py:60`).
   [I] Il faut l'imposer à **tout** producteur et à tout lecteur de hiérarchie, y compris les sorties.
2. **Deux recommandations de `PASSATION.md` § 3 sans fondement mesuré.**
   - La « partition T > 0 » : le plan du 4 octobre la classait parmi les fausses bonnes idées pour le LiDAR
     (`PLAN_VITESSE_100MS.md` § 5) [N], et la v10 avait mesuré l'ablation (§ 7).
   - La « feuille J3 complète » est présentée comme mécanisme de la v10. Elle n'a jamais été dans la v10 mesurée, et
     son port partiel n'a rien rendu sur G4.
3. **Le GPU greffé en étage.** Fermé à chaque version, refait à la suivante (§ 3.6).
4. **Reconstruire plutôt que garder l'ancienne version comme oracle.** L'audit v4 (`bab37b9`) recommandait de garder
   la v4 comme oracle et de fermer la tour ; la v5 a réécrit le même objet.
5. **Leviers retirés de justesse en v11.** G1 AVX2, frontière par tranches, préchargements, annonces : −3 à −17 %
   sur leur phase. Ce sont des fermetures par seuil mal calibré, **pas des réfutations** (`PASSATION.md` § 4).
6. **Fermetures par consigne, à ne pas confondre avec des preuves** : micro-variantes q2, float32, lots de
   singletons « sans hypothèse nouvelle ».

---

## 4. Motifs récurrents

### 4.1 Motifs d'échec, preuves à l'appui

**M1. Reconstruire au lieu de capitaliser.**
- Dix bases de code en 60 jours, chaque fondation refaite : cœur, ordonnanceur, numérique, CLI, outillage G4 [I pour
  v2–v9].
- La v11 a lancé son « workflow de fondations » le jour de son ouverture [M `ouverture-v11.md`] ; la v10 aussi
  (« V10-0 », 28 sept.) [M].
- Trois reconstructions à algorithme constant (v4→v5, v5→v6, v10→v11) n'ont jamais apporté de vitesse (§ 4.3).

**M2. Perdre les acquis de vitesse ou d'objet de la version précédente.**
- La v8 reconstruit l'amont et laisse la tour v7 (`AUDIT_V8_SYNTHESE.md:31-43`).
- La v11 démarre sans les mécanismes de la v10 :
  - le 3 octobre, elle est ×5,7–6,0 plus lente (1,15–1,51 s contre 0,20–0,25 s) ;
  - au 4 octobre, le pas de descente est encore 3,2 fois plus cher à K5 (188 ns contre 597 ns à W1 sur ng02 [N,
    vérifié par le juge des transpositions]) ;
  - au gel, la tour K10 reste ×2,6–2,8 plus lente.
- Elle en reprend une partie **en relisant la v10** : semis par population, q3 différé (§ 7).
- La v10 elle-même n'a jamais importé son raccord R2.

**M3. Ne pas fermer le différentiel.**
- v10/v11 : le banc `tests/tower/full_v10_diff.py` ne compare que 14 fixtures nommées (`singleton` … `random1`),
  soit 42 égalités sur 3 profils (`receipts/full_20261002/README.md:18-23`).
- Les ancres Merkle de la v10 sur trames entières n'existent que hors dépôt.
- Le reçu `ecart_v10_v11` est un diagnostic de performance, pas un différentiel d'objet.
- Les seuls différentiels fermés (v4 ≡ v5 ≡ v6) portaient sur un objet faux et sur un digest de filtre.

**M4. La qualification dévore le temps.**
- v7 : 26 537 des 26 780 fichiers sont des reçus ou des audits.
- v8 : 109 répertoires de build épinglés sont cités dans `AGENTS.md`, aucun ne subsiste. La tranche 19 a coûté
  75 CTests × 2 builds, TSan, 595 appels d'oracle et 172 mesures, pour un résultat négatif.
- v11 : une tranche locale de 4 h 30 [M `workflows-legers-g4-lourd.md`] ; qualification complète seulement au
  5 octobre.
- Trois interventions de l'utilisateur : 2 sept. « trop de garde-fous », 22 sept. « pas de pinaillage », 5 oct.
  « pourquoi si long ? ».

**M5. Objet surqualifié ou mal déclaré.**
- Fold v4 adopté avant la fin de la lecture.
- README v5 jugé « surqualifié » par l'auditeur (`ETAT_COURANT.md:66-99`).
- « Trois v7 » à ne pas confondre (CLI F, sonde FULL, prototypes ; `AUDIT_V7_SYNTHESE.md`).
- « Beats HDBSCAN's oracle » (`cda636b5e`) avant requalification.

**M6. Mesure.**
- Chronos de composant présentés comme contrats :
  - 189 ms de noyaux pour une tour de 419 s (v7) ;
  - 63,8 ms de filtre v9 sans le front ;
  - `chain_total` v9 à 0,76–0,98 s pour un mur de processus de 1,52–1,87 s (A9-51) ;
  - « 1 s tenu » en v10 pour un seul ordre (ERRATA).
- Comparaisons entre protocoles différents : v10 en 3ᵉ passe chaude, v11 en processus neufs.
- Seuils sans calcul de puissance (`AUDIT_FINAL_V11.md` § 1, point 8).
- Bimodalité des temps W48 à binaire identique jamais expliquée (critique P1 [N]).

**M7. Le GPU greffé en étage**, borné par l'hôte (§ 3.6) : six fois.

**M8. Contrat ambigu jamais tranché** (§ 2.3). La même question froid/chaud court du 7 août au 7 octobre.

**M9. Mauvaise priorité.**
- v8 : q2 pendant 5 jours alors que q3 domine.
- v11 : les sorties avant la vitesse ; la vitesse ne revient au premier plan que le 4 octobre (`AUDIT_FINAL_V11.md`
  § 14.2).

**M10. Hygiène du dépôt et des documents.**
- Index partagé.
- Scans KITTI bruts versionnés : `morsehgp3D_v8/receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz`
  contient `data/scan000000/RAW.bin` et deux autres scans (noms seuls listés). La question est ouverte depuis le
  21 septembre (`AUDIT_V8_SYNTHESE.md:88, 206-216`).
- Profil OS Login dans les archives de sessions.
- `CLAUDE.md` et `AGENTS.md` en retard d'une ou plusieurs versions : la v10 n'y a jamais été déclarée (§ 6.3).
- Reçus qui dépendent de `/tmp` ou de `build/` : 44 des 78 dossiers de reçus v9 (A9-57) ; ancres Merkle et rapports
  L01–L10 de l'audit v10 hors dépôt.

### 4.2 Motifs de réussite

| Motif | Preuve |
|---|---|
| S1. Doctrine entière exacte, flottant seulement en filtre certifié à repli exact | aucun FULL faux en v11 sur 126 sessions ; GPU égal à l'octet ; chaque défaut grave vu à une borne exacte (2^127−1, 2^20±1 ; `AUDIT_FINAL_V11.md` § 15) |
| S2. Oracles bornés qui établissent la vérité, à arithmétique autre | juge Γ_k (v3) ; `obigint.hpp` (v4) ; T2 (v7 : 54 tours, 8,1 M vérifications verticales, 4 mutants) ; `hgp10_ref.py` ; référence à deux étages (v11) |
| S3. Fixtures minimales gravées | E5, deux triangles de la thèse, minima à 4 points, coquille de 7, carré cocyclique, `linked_arcs_u16`, peigne Morton de profondeur 49 |
| S4. Portes à code exact et mutants causaux, nés des portes vacueuses de la v3 | `run_expect.cmake` (v4) ; 485/485 mutants (v11) |
| S5. Registre des preuves typé | seul document vivant à travers toutes les versions (dernier commit `1fbeea5b8`, 7 oct.) |
| S6. Les audits ont porté les deux vrais sauts de vitesse | v9 issue de l'audit v8 ; générateur v10 issu de la lentille L13 de l'audit v9 |
| S7. Mesure appariée dans une même session | banc intra-processus (v4, v5) ; `recu_local.sh` [M] ; `ab_summary.py` (v11) |
| S8. Sessions G4 gardées | 124 arrêts certifiés sur 126 en v11 (`PASSATION.md` § 3) ; scripts nourris des incidents v5 et v9 |

### 4.3 Les reprises de zéro : combien, à quel coût, pour quel apport

**Combien.**
- Toutes les versions v2 à v11 sont des bases de code neuves : nouveau dossier, nouveau namespace.
- Cinq reprises sont **déclarées « de zéro »** : v4 (`f775c9881`), v5 (« base de code neuve »), v6 (« repartir de
  zéro », paraphrase de Claude, mots de l'utilisateur non retrouvés), v10 (« la v9 … jamais une base de code »),
  v11. La v12 sera la sixième.
- Les autres sont des **compositions par ports** (v7 depuis la v6, v9 depuis v8 + v7) ou des **refontes après
  réfutation** (v2, v3, v8).

| Passage | Motif | Durée de la nouvelle version | Apport | Coût |
|---|---|---|---|---|
| produit → v2 | Γ exhaustif en repli dans le v1 | 1 j | objet juste, R1–R8 | force brute condamnée le soir même |
| v2 → v3 | générateur non certifiable (Tukey) | 9 j | WSPD + lanes, juge Γ_k, registre des pistes | NO-GO G4 ; payload jamais livré |
| v3 → v4 | post-mortem WSPD | 11 j | doctrine des portes | objet faux (E5 connue) ; pas de G4 |
| v4 → v5 | propreté, mémoire | 5 j | flux par K, différentiel fermé, 50k en 56,6 s sur G4 | même objet faux ; recommandation d'audit ignorée |
| v5 → v6 | Θ(n²) universel | 3 j | familles stationnaires, coefficient 4, murs mesurés | aucun gain de temps |
| v6 → v7 | changement d'objet | 8 j | FULL, Ω(N²), MSF composables, T2 | ≈ ×9 de temps |
| v7 → v8 | P0 du générateur | 10 j | 18 bits, Patchwork++, protocoles | aucune tour ; générateur remplacé ensuite |
| v8 → v9 | « pas la tour » | 7 j | K5 < 1 s (GPU) | complexité ; E5 ×2 |
| v9 → v10 | audit critique | 5 j | boîtes de centres : **×3,7–3,9** | marges flottantes, monolithes, R2 non importé |
| v10 → v11 | « plus propre » | 6 j | exactitude démontrée, sorties, harnais, GPU exact | vitesse : ×5,7–6,0 le 3 oct., ×1,5–1,7 le 4 ; tour K10 ×2,6–2,8 au gel ; différentiel non fermé |

[I] **Bilan.**
- Les seules reprises qui ont fait progresser le contrat apportaient un **nouvel objet** (v7) ou un **nouvel
  algorithme** (v9 par assemblage, v10 par boîtes de centres), tirés d'une réfutation ou d'un audit.
- Celles qui ne visaient que la propreté ou la mémoire (v5, v6, v11) n'ont rien rendu en vitesse.
- La v4 et la v8 ont dépensé leur temps sur un objet faux, ou sur un amont sans tour.

Une v12 « à neuf » ne doit pas redevenir une reprise de propreté. Elle doit fixer d'avance quel algorithme change,
puis porter d'emblée tout le reste et le mesurer dans la même session.

---

## 5. Ce que la v12 doit reprendre d'avant la v10, et ce qu'il ne faut pas refaire

### 5.1 À reprendre

**Énoncés et théorèmes.**
- L'énoncé v2 de l'objet : arbre de fusion de d_K, avec un seul catalogue de rang ≤ K+1 pour toute la tour.
- La FULL de la v7 sous régularité, avec son extension non régulière (contributions datées) : registre l. 129-135.
- La borne Ω(N²) : la cible est un surcoût sur la sortie, pas une borne absolue.
- Les **certificats MSF composables** de la v7. Base naturelle d'une forêt parallèle, jamais faite en v11
  (`AUDIT_FINAL_V11.md:570-580`). Leur obligation de preuve (contraction des plateaux, registre l. 240) est à
  fermer d'abord.
- Le plafond ≈ 11× d'Amdahl entre ordres indépendants, comme donnée de conception.
- La propriété de préfixe Kmax (v6) : les ordres 1..5 de K10 sont égaux à K5.

**Oracles et juges.**
- Le juge Γ_k de la v3 (coupes ouvertes et fermées).
- `obigint` de la v4 : arithmétique autre que celle du sujet.
- T2 de la v7, avec ses leçons de métadonnées.
- Le juge bilatéral d'échantillon de la v8 : une porte de complétude à l'échelle d'une trame, non adoptée en
  v11 [I].
- Les juges d'échantillon de la v9 : « zone aveugle » (boules de fusion seule à Kmax, 26–29 % du catalogue à K5) et
  « clés jamais émises » (77 k boules admissibles à 2k et 8k). Non portés en v11 [fork D, grep négatif].
- Le juge K = 1 contre l'EMST.
- L'invariant d'Euler de la v9, comme filet seulement.
- Le protocole de l'audit v9 (A9-51) : boucle résidente de **trames distinctes** (p50/p95), mur froid publié,
  vérification hors chronomètre.

**Fixtures.**
- E5.
- R1–R8 (v2).
- v3 : Q2X, F16, CRUX, cosphère de 24, 50 000 IDs à distracteurs, `plateau_carre_multifusion`, δ² = 100 = 4R².
- v5 : `linked_arcs_u16`.
- v6 : tétraèdre régulier + z.
- v7 : minima à 4 points, n4, MEB K7, peigne de profondeur 49, extra-shell 50k, contre-exemple α3.
- v8 : six contre-fixtures q3/q4, 278 points dyadiques.

**Outils.**
- `run_expect.cmake` (codes 0–4).
- Les drapeaux `--relabel-gate`, `--par-gate`, `--workers-gate`, `--min-*`.
- Le registre de mutants.
- `recu_local.sh` et le banc apparié intra-processus.
- La comptabilité mémoire par rôle, avec `bad_alloc` rendu en refus typé (v6).
- `AxisBounds` de la v6, devenu V3 en v11.
- Les familles stationnaires.

**LiDAR.**
- Le contrat de chronométrage des trames de la v8 : froid et chaud distingués, frontière du chronomètre écrite
  avant la mesure, chaque scène publiée.
- Patchwork++ `3e6903a1` sur IDs (déjà porté en v11).
- Le protocole spatial par plans du capteur.

**Faits de coût.**
- Contexte CUDA : 1 242 ms à froid contre 18 ms à chaud (7 août).
- Les noyaux GPU ne pèsent que 1 à 4 % d'un étage greffé.
- Les compteurs déterministes sont identiques en local (8 fils) et sur G4 (48 fils) [M `mesures-locales-vs-g4.md`].

### 5.2 À ne pas refaire

Tout ce que le § 3 ferme par preuve ou fixture, notamment :
- les réductions sans attaches silencieuses (E5) ;
- les substituts rapides d'un autre objet (point-MST) ;
- le générateur WSPD et ses micro-variantes ;
- le float32 natif et les marges flottantes figées ;
- le GPU greffé en étage (exécuteurs éphémères, matérialisation par ancre, DSU sur le device sans preuve) ;
- les promesses sous-quadratiques universelles et les plafonds mémoire « projetés ».

Et, côté organisation et mesure :
- déclarer l'objet avant la fin de la lecture ;
- une force brute « en attendant » ;
- une semaine de tranches sur une voie qui ne domine pas le régime visé ;
- reconstruire alors que l'audit recommande de garder l'ancienne version comme oracle ;
- laisser le chemin produit différer du chemin mesuré (v9, A9-83 ; API v11 à feuilles de 16 sans GPU ni cache) ;
- des leviers booléens sans ablation, des prototypes dans `audits/` ;
- comparer des constantes entre processus, ou présenter un digest de candidats comme preuve de conformité ;
- des pentes tirées de familles dilatées ou mesurées sous charge ;
- ouvrir une version sans mettre à jour `AGENTS.md` et `CLAUDE.md`, ou laisser une section du registre en attente (v10) ;
- un index Git partagé, des reçus qui dépendent de `/tmp` ou de `build/`, des octets KITTI dans le dépôt.

---

## 6. Bibliothèque produit, registre formel, incohérences documentaires

### 6.1 `morsehgp3d/` et le chaînon manquant

**État.**
- La bibliothèque est figée depuis le 9 août (`95dd8036a`). Seule cible publique : `build_exact_point_hierarchy`.
- Elle prend un `CertifiedTowerInput` : nœuds, arêtes horizontales et verticales datées, simplexes projetables avec
  les rayons de leurs cofaces, et quatre identifiants d'autorité amont liés par `tower_payload_id`
  (`include/morsehgp3d/api/point_hierarchy.hpp:44-106`).
- Elle n'authentifie pas la vérité amont : son README dit « ne construit pas encore la tour depuis un nuage brut ».

**Aucun producteur.**
- `CertifiedTowerInput` n'apparaît hors de `morsehgp3d/` que dans des documents : `CLAUDE.md:134`,
  `morsehgp3D_v4/audits/ETAT_COURANT.md:31` (« API producteur … absente »), `morsehgp3D_v9/docs/PLAN_V9.md:125`,
  `morsehgp3D_v9/PASSATION.md:1044`, `AUDIT_V8_SYNTHESE.md:318`, `Zoltan/FoundationModel/OBJET.md:136`.
- Le `ChainResult` cité par `CLAUDE.md` est le type de chaîne de la v9 (`src/chain/tower_chain.hpp`), absent des
  v10 et v11.
- La v11 a reconstruit nativement la hiérarchie de points et la tête plate exacte, sans réutiliser le réducteur.

**Le réducteur produit est inadapté comme base.**
- Routage dur irréversible (Zoltan `OBJET.md:136`).
- `cpp_int`, `std::map`, monofil.
- Cascade flottante qui suppose l'arrondi au plus proche [N].

**La CI.** `ci.yml` construit et teste encore cette bibliothèque en GCC, Clang et ASan/UBSan (l. 30-57), mais ni la
v10 ni la v11.

[I] Pour la v12, deux options sont à soumettre à l'utilisateur :
- (a) archiver `morsehgp3d/` comme « ligne enregistrée historique » et sortir le registre et la roadmap de la
  chaîne d'autorité active ;
- (b) définir un `CertifiedTowerInput` v2 comme **vue** du registre d'événements de la v12, conformément à la
  leçon 11 de la passation.

Le statu quo laisse une API publique sans producteur, que la CI principale continue pourtant de qualifier.

### 6.2 Registre formel et roadmap

**`docs/implementation_status.toml`.**
- `updated_at = "2026-08-08"`, `current_phase = "15"` (phase 15 `in_progress`).
- Jalons `v1_correctness` et `v1_interactive_scalable` à `blocked`.
- Phases 5, 6, 8, 14 et 17 `ready` ; 12, 13, 16 et 18 `blocked`.
- Aucune des versions v2 à v11 n'y figure. C'est conforme à la règle « hors registre », mais deux mois de travail
  n'ont aucune trace formelle.

**`docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md:3`.** Annonce toujours la « Mission actuelle — v7 FULL,
5 septembre 2026 » (dernier commit `f4c0734c5`).

**Registre des preuves.** C'est le seul registre vivant. Il porte les sections V9-S4 et V11 (l. 1268-1376), mais
**aucune section V10**. Le développeur v10 la donnait pourtant comme « seul bloquant » le 30 septembre (`9ca8e4f6e`,
fork D).

### 6.3 Incohérences entre `CLAUDE.md`, `AGENTS.md` et l'état réel

1. **Profil de la v11.** `CLAUDE.md:28` dit u18, alors que `AGENTS.md:18-20`, la passation et la décision du 6 octobre
   disent u21. `AGENTS.md` a été mis à jour en `ffc2ff95f` (2 oct.), `CLAUDE.md` est resté à `f2ebb7a08`.
2. **« Chantier actif » partout.**
   - v11 dans `CLAUDE.md:21-23`, volontairement, en attendant l'accord de l'utilisateur.
   - v9 dans `CLAUDE.md:36`, sous un titre « chantier précédent ».
   - v5 dans `CLAUDE.md:50`.
   - v4 dans `CLAUDE.md:130`.
3. **Versions absentes.** Aucune section v10 dans `CLAUDE.md` ni dans `AGENTS.md`, ni v6 nulle part. Les décisions
   du 28 septembre au 1ᵉʳ octobre ne vivent que dans la mémoire et les documents v10.
4. **CI.** `CLAUDE.md` dit « la CI ne construit ni la v5, ni la v4, ni la v3 ». En fait, des workflows existent pour
   la v7, l'audit LiDAR v8 et la v9, mais aucun pour la v10 ni la v11. La qualification finale de la v11 note
   « Clang est absent de la VM » (`receipts/developpement_20261005/qualification_finale/README.md:33`), alors que
   `docs/ARCHITECTURE.md:33` exige GCC **et** Clang.
5. **`AGENTS.md` obsolète par endroits.**
   - l. 116-138 garde « Contrat principal actif — trames entières, float32 par défaut », remplacé depuis.
   - l. 488 : « la v5 ne possède pas encore de manifeste CMake », faux dès le commit qui l'a écrit (`8600c53b9`).
   - l. 526-591 : « v5 prioritaire ».
   - l. 500 : Zoltan « cible la v9 ».
   - l. 87-449 citent 109 builds épinglés disparus, donc non rejouables.
6. **`CLAUDE.md:134` (Zoltan).** « À seuil relatif » : fermé par la v11 (`AUDIT_FINAL_V11.md:663-665`). Le chaînon
   cité, `ChainResult`, est un type de la v9.
7. **Règle des tailles.** La règle absolue 8k/16k/32k (`CLAUDE.md:15`) est en tension avec la consigne du 6 octobre et
   avec la plage contractuelle de 30–60 k.

---

## 7. Vérification de `AUDIT_FINAL_V11.md` § 14.2 et `PASSATION.md` § 3–4

**Ce qui est vrai.**
- La v11 n'a pas porté d'emblée la vitesse de la v10. Le 3 octobre à 12 h 07, à la demande de l'utilisateur, le
  développeur mesure la v11 (`ae817d09e`) à **1,15–1,51 s contre 0,20–0,25 s pour la v10 (×5,7 à 6,0)**.
- Il identifie cinq causes : frontière déséquilibrée, forêts ordre par ordre (environ 600 barrières), « pas
  d'équivalent du semis H_K », pas de `live2`, arithmétique sans filtre
  (`receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md.snapshot` ; correctifs
  `ef75dafac`).
- Le non-port est en partie **délibéré** : « Ni les masques de triplets, ni le mémo partagé par cellule, ni les
  filtres flottants des MEB de la v10 ne sont repris » (`morsehgp3D_v11/docs/PROVENANCE.md:164-176`).
- Le différentiel v10/v11 n'a jamais été fermé sur trames. Seules 14 fixtures × 3 profils = 42 égalités ont été
  comparées à la v10 `c764e121a` (`receipts/full_20261002/README.md:18-23`), alors que `ARCHITECTURE.md:179-184` et
  `AGENTS.md:27-30` exigeaient des sorties identiques octet pour octet, trames comprises.

**Les cinq mécanismes cités, un à un.**

| Mécanisme (PASSATION § 3) | Dans la v10 | Effet attribué | Dans la v11 au gel | Verdict |
|---|---|---|---|---|
| Choix des k plus proches dans les descentes | oui, `tower.cpp:905-931` : sélection en double à marge figée `kApproxMargin`, repli exact `side_key` | jamais isolé sur G4 ; mutant local `JUMP_ANY` : +3 % de MEB, +7 % de sauts [N, vérifié] | absent : `descent.cpp:138-140` prend `located.interior().first(k)` | vrai mais mineur à K5 ; K10 non mesuré. À porter avec un comparateur **exact** : la marge flottante viole F1–F6 |
| Mémo cellulaire daté | oui, `tower.cpp:951-991` : valeur par cellule dans `atlas.val`, `atomic_ref` relaxé, écrit en fin de descente | 307 177 arrêts, soit 7 % des pas à K5 ; 2,14 M à K10 | absent du chemin par défaut : `DescentMemo` à clé de k-uplet, capacité 0 ; mémos de lane retirés (2,6 %) | vrai ; mais à K5 les succès de la table de populations v11 compensent presque tout (écart de pas +8 %) |
| Partition T > 0 (sous-maille 1/64) | oui, `generator.cpp:22` (`kT = 6`) | **aucun sur LiDAR** : ablation T = 6/3/0 sur ng02, dumps identiques, nœuds 734 083 / 734 261 / 735 601, `t_boxes` 2,84 / 2,80 / 2,74 s (`build/v11-persist/audit_v10/preuves_l05_code_catalogue/ablation_kT.txt`, relu) ; effet seulement sur grille 16³ | absent | **faux comme mécanisme de vitesse**. `AUDIT_FINAL_V11.md` § 3.2 et § 14.2 sont contredits par la source. Utile seulement contre les grilles synthétiques denses |
| Feuille J3 complète | **non** : l'arbre v10 mesuré (`777406b82`, 24 fichiers `src`) ne contient aucune feuille J3. C'est un prototype privé dans `receipts/audit_continu_20260929/…/feuille/` | ×1,371 à K5, ×1,548 à K10 : mesure **locale** sous charge, étage des boîtes seul | partiel : lemme R nul sur G4 (`claudeab8`) ; M3/E4 **+1,2 à 1,4 %** sur la voie CPU (`AUDIT_FINAL_V11.md` § 5.3) | « mécanisme de la v10 » inexact ; gain jamais reproduit |
| Table M(K) (12/16/24/28) | oui, `generator.cpp:639-641` | K10 sur ng02 : feuille 16 → 11,06 M nœuds, feuille 24 → 1,07 M ; `t_boxes` 32,8 → 18,6 s local (`ablation_M.txt`) | partiel : le banc joue 24 à K10 et sur la voie GPU K5, l'API reste à 16 à tout K (`src/api/compute.cpp:26`) | utile au produit, mais **n'explique pas** l'écart K10, mesuré déjà en feuille 24 |

**« Redécouvertes ».**
- Semis par population et q3 différé : ce sont des **lectures de la v10** faites le 3 et le 4 octobre (`PROVENANCE.md`
  l. 164-170 ; `56216392e`), pas des redécouvertes indépendantes. La nuance compte pour la leçon.
- Effet du q3 différé sur G4 : −1,5 à −1,8 % de passe unique à W1, ambigu à W48 (critique P3 [N]).

**Causes mesurées absentes du § 14.2.**
- **À travail logique égal**, le coût par unité est plus élevé (cartes et verdict du 4 octobre [N], recoupés avec la
  table de vérité v10) :
  - passe unique : ×1,27 à W1, et passage de 1 à 48 fils ×23–28 contre ×34 ;
  - descentes : ×3,2–3,4 par pas (2 918,5 contre 851,7 ms à W1 sur ng02) ;
  - recensement : ≈ 3,1–3,9 µs par appel contre ≈ 1,06 µs pour la boule fermée du `SiteTree` v10 (estimation par
    parts de profil). Le nouvel index remplace délibérément le k-d à marge flottante, et le banc M5 qui devait
    trancher n'a jamais été joué ;
  - compteurs vérifiés dans la boucle chaude (≈ 0,5 milliard d'additions par trame, corrigé le 4 octobre) ;
  - CAS partagés par nœud dans `Buffer::allocate` ;
  - table des triplets vivants absente.
- Les décisions de conception du 2 octobre n'ont jamais été implantées : D-F1 (forêt sans lots), D-G1 (arrêt à la
  première cellule), D-G4 (recensement aux k plus proches), D-M2 (ancres Merkle v10). Au gel, la forêt parallèle
  (V4/O7) et le mémo de cellule daté (V7) sont toujours « jamais faits » (`AUDIT_FINAL_V11.md` § 13).

**`PASSATION.md` § 4 (« à ne pas refaire »).** Les entrées vérifiées sont exactes : commits de rejet `c1675e4c9`
(N1), `830473218` (L4), `d4228f5e5` (feuille coopérative) ; leviers retirés `b6fd3796d`, `3e6f88c7f`, `1950c3727`,
`e35db29c3`, `04b00810d`.
- Elle omet, parmi les choses à ne pas refaire de la **lignée** :
  - le fold et le graphe de Gabriel (E5) ;
  - le générateur WSPD ;
  - le GPU greffé en étage ;
  - les marges flottantes figées de la v10.
- Corrections à apporter à la passation :
  1. retirer la « partition T > 0 » de la liste des ports, ou la requalifier « robustesse sur grilles synthétiques » ;
  2. requalifier J3 en « prototype privé v10, à rejuger » ;
  3. ajouter en tête de liste le **coût par unité** (recensement, compteurs, allocations, triplets) et le banc M5.

---

## 8. L'audit des transpositions du 4 octobre : le plan 100 ms face à la mesure

**Ce que disait l'audit** [N] : `AUDIT_TRANSPOSITIONS_V11.md` (4 oct., 13 h 57 UTC), `CRITIQUE_COMPLETUDE.md`
(14 h 23), `PLAN_VITESSE_100MS.md` (13 h 43), `conception/PISTES_DE_RUPTURE.md` (2 oct.).
- À même objet et même travail (56,6 candidats testés par boule), la v11 payait plus cher chaque unité de travail :
  passe unique +58 à +88 ms, descentes +64 à +88 ms (§ 7).
- Verdict : aucune transposition, ni toutes ensemble, ne mène à 100 ms à K5 ; à K10, viser 0,3–0,4 s.
- Erreurs connues des notes : « meilleure prise 320,4 ms » fausse (305,3 ms dans `claudeab4`) ; débits S7 de la v10
  invalides ; « États » périmés une demi-heure plus tard (critique, § 5).

**Prédictions et démentis.**

| Prédiction | Mesure ultérieure |
|---|---|
| 2 oct. : « 100 ms atteignable en CPU seul à K = 5 (65–91 ms) » | au gel, K5 CPU à chaud 255–314 ms : **démenti** |
| 4 oct. : transpositions → ≈ 215–350 ms, « au mieux le niveau de la v10 » | au gel, 212–255 ms GPU et 255–314 ms CPU : **confirmé** |
| V1, feuille J3 : −25 à −55 ms | tranche (a), lemme R : −0,2 à +0,4 % à W1 (`claudeab8`) ; (b) M3/E4 portée (`ec55578d9`) : **+1,2 à 1,4 %** sur la voie CPU (`AUDIT_FINAL_V11.md` § 5.3) ; (d) et (e) jamais faites |
| V3, bornes sur réseau et arbre radix : tests ×0,26–0,28 | V3 livrée : tests ×0,26 et instructions ×0,565, mais **temps des forêts ×0,925** seulement |
| V4, forêt sans lots ; V7, mémo de cellule | jamais faits (`AUDIT_FINAL_V11.md` § 13) |
| V6, filtre G1 sans branche : −5 à −20 ms | `claudecat1` : préambule ×2,1 plus lent ; G1 AVX2 retiré le 7 oct. (gm 0,895 pour un seuil de 0,85) |
| V8, feuille 24 à K10 : catalogue ×1,6–1,8 | adoptée sur la voie GPU ; l'API reste à 16 à tout K (`src/api/compute.cpp`) |
| V12, catalogue GPU par sous-arbres (« seule route ») | non faite ; à sa place, une feuille par fil, puis N1, L4 et feuille coopérative rejetés |
| Plan GPU du 6 oct. : N1 (rang 1), L4 (rang 3) | rejetés sur G4 (`c1675e4c9`, `830473218`, `d4228f5e5`) |
| K10 : ≈ 0,6–1,0 s après transpositions, cible annoncée 0,3–0,4 s | 1,34–1,78 s au gel : **démenti** |

**Ce qui reste valable.**
- La décomposition des causes de l'écart avec la v10.
- Le protocole statistique : bras A/A, médiane des rapports, nombre de paires fixé par σ.
- Les questions à l'utilisateur.
- Les filets avant réécriture (Euler K+2, différentiel v10 sur trames, ordre 1 contre l'arbre couvrant minimal).
- La critique P1 (bimodalité W48 à binaire identique) n'a reçu aucune explication publiée [I : aucun reçu cité
  par l'audit final ne la traite].

---

## 9. Recommandations pour l'ouverture de la v12

1. **Trancher d'abord le contrat**, par écrit et une fois pour toutes :
   - froid ou résident ;
   - maximum ou médiane ;
   - plage de tailles (30–60 k exclut 85/127 trames c08) ;
   - GPU contractuel ou non ;
   - périmètre (FULL seul ou sorties) ;
   - K10 cible ou objectif ;
   - sort du contrat « trame brute » et de la consigne « pas de vérité terrain ».
2. **Écrire la liste des changements d'algorithme** que la v12 apporte (catalogue résident sur GPU ? forêt parallèle
   par MSF composables ? descentes au coût v10 ?). Porter **d'emblée** et mesurer en même session tout le reste :
   mécanismes v10, acquis v11.
3. **Fermer le différentiel v11 (et v10) sur trames entières dès la première tranche**, avec les empreintes de
   `AUDIT_FINAL_V11.md` § 3.4.
4. **Imposer à tout producteur et à tout lecteur la bibliothèque de fixtures « objet »** : E5, deux triangles,
   minima à 4 points, coquille de 7, K = 1, K = n. Inclure un mutant « Gabriel seul » qui doit être tué.
5. **Un registre unique des pistes fermées**, typé par force (preuve, mesure, modèle, consigne), avec la règle de
   réouverture. Le corriger tout de suite sur la « partition T > 0 ».
6. **Mettre `CLAUDE.md` et `AGENTS.md` à jour à l'ouverture**, avec l'accord de l'utilisateur.
   - Retirer les cadres obsolètes (v4, v5, v9 « actif », contrat float32, u18).
   - Consigner les décisions v10.
   - Statuer sur `morsehgp3d/`, le registre formel et les données KITTI versionnées.
7. **Une règle de rentabilité unique et calibrée** (puissance statistique, effet attendu, A/A), pour éviter à la fois
   le sur-jugement de la v8 et la perte de leviers de la v11.

---

## Annexe : sources principales

**Normatives.** `AGENTS.md`, `CLAUDE.md`, `docs/HISTORIQUE.md`, `docs/research/CONTRAT_50K_BILAN.md`,
`docs/archive/abandoned/README.md`, `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, `docs/implementation_status.toml`,
`docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md`, `.github/workflows/`.

**Par version.**
- Registres de pistes : `PISTES_FERMEES.md` (v3, v5, v6) et `FAUSSES_PISTES.md` (v7, v8, v9).
- `morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md`.
- `morsehgp3D_v10/receipts/audit_v9_20260928/AUDIT_V9_CRITIQUE_20260928.md`.
- `morsehgp3D_v10/src/tower/tower.cpp:897-991`.
- v11 : `PASSATION.md`, `docs/AUDIT_FINAL_V11.md`, `docs/PROVENANCE.md`.
- Notes du 4 octobre : `morsehgp3D_v11/receipts/notes_hors_depot_20261007/`.

**Hors dépôt.**
- `build/v11-persist/audit_v10/preuves_l05_code_catalogue/ablation_kT.txt`.
- Mémoire utilisateur.
- Notes des lectures déléguées : `scratchpad/agent_F/fork_{A,B,C,D}/NOTES.md`.
