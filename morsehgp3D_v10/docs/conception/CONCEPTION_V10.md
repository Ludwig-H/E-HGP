# CONCEPTION_V10 — morsehgp3D_v10, conception intégrée

28 septembre 2026. Document d'intégration des cinq conceptions finales de sous-systèmes
(`GEN_v2.md`, `TOWER_v2.md`, `CLUSTER_v2.md`, `EVAL_v2.md`, `ARCH_v2.md`, même dossier). Il fixe le cadre,
les contrats, les interfaces exactes entre générateur, tour, points, tête et banc, les arbitrages entre
documents, le registre des preuves, les portes minimales et le plan de phases. Il ne modifie aucun fichier suivi du
dépôt. GCP non utilisé.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference              (cuda_g4 subordonné : pistes V10-G et V10-4b)
profile=quantized_u18_input_only   (multiplicités natives dans l'objet)
mode=conception_integree
public_status=not_claimed
```

---

## 0. Préséance, directives, état du dépôt

### 0.1 Qui prévaut sur quoi

| Document | Prévaut pour | Cède sur |
| --- | --- | --- |
| **ce document** | interfaces entre couches, types, formats, statuts, responsabilités, arbitrages du § 5, plan de phases, questions ouvertes | l'intérieur des sous-systèmes |
| `GEN_v2.md` | algorithme du générateur (boîtes de centres), prédicats et bornes du générateur, `LeafOracle`, portes du catalogue | § 5 (digest, rang 0, format typé) |
| `TOWER_v2.md` | étages P/G/M/T1/T2/Q de la tour, invariants I1–I10, juges J-CENSUS, J-DESC, J-MF, T2 | § 5 (modes de contrôle, statuts, index des boules nulles) |
| `CLUSTER_v2.md` | hiérarchie C∩X, graphe-chemin, condensation N-aire, λ, ẑ, politiques de bruit, fixtures de la tête | § 5 (producteur MR, H3, clés de lot publiées par la tour) |
| `EVAL_v2.md` | banc synthétique, métriques, statistique, préenregistrement, protocole de performance LiDAR, go/no-go | § 5 (adversaires HDBSCAN, témoin E1, λ en zéro, remplissage) |
| `ARCH_v2.md` | couches, `core`, budget logique, parallélisme R1–R7, digests SHA-256, CMake, CI, reçus, protocole G4, disque | § 5 (admission, T2, multiplicités, quantification LiDAR) |

Les directives datées de l'utilisateur prévalent sur tous les documents :

| Date | Directive | Traduction dans la v10 |
| --- | --- | --- |
| 28 sept., ≈ 09:20 UTC | auditer la v9, développer la v10 (même objet), puis le choix le plus pertinent de clustering hiérarchique pour battre HDBSCAN sur les bancs synthétiques ; feu vert G4 | objectifs § 2.3 ; sessions G4 gardées § 12.3 |
| 28 sept., ≈ 10:30 UTC | « Pour la complexité, c'est le régime LiDAR qui nous intéresse le plus » | contrats C(K, T) sur trames sans sol ; synthétiques = pentes et qualité |
| 28 sept., ≈ 14:30 UTC | « Le but n'est pas de recréer HDBSCAN ; tu peux faire confiance à une implémentation récente Python » | adversaires = `sklearn.cluster.HDBSCAN` tel quel ; aucun producteur MR en C++ (D-V10-17) |
| 28 sept. | z = dimension intrinsèque ; z = 1 pour l'équité face à HDBSCAN, z = 2 pour les surfaces LiDAR | § 4.4 |
| 28 sept. | la thèse est une source, pas une autorité ; toute divergence est explicite | divergences déclarées : C∩X partition (CLUSTER § 3.2), plancher λ, condensation N-aire |
| 22 sept. | un livrable qui fonctionne ; trancher les détails soi-même | § 15 : 25 questions de sous-systèmes tranchées ici |
| 21 sept., 20:10 UTC (`AGENTS.md`, § Ouverture v9) | contrats de temps « principalement » sur trames sans sol | trames brutes = régime secondaire publié et jugé |
| permanentes | `main` sans branche ; aucun octet KITTI versionné ; HGP-old en lecture seule ; reçus hors `/tmp` | § 12.4 |

### 0.2 État du dépôt : une implémentation V10-α existe déjà

Le développement a commencé en parallèle de la conception. `origin/main` porte douze commits v10, de `d7a1459dd` à
`5bf66793e` (≈ 4,5 k lignes hors audits) :

- fondations (`src/core`, `src/sched`, `src/arith/wide.hpp`, `cmake/run_expect.cmake`) ;
- nuage (`src/cloud`, avec un `SiteTree` qui répond aussi aux centres rationnels) ;
- générateur par boîtes de centres, feuille v2 (`src/catalogue/generator.cpp`), égal à la v9 sur 08/000200 K5
  (1 407 885 boules) et K10 (5 483 320) ;
- tour par descente (`src/tower/tower.cpp`), jugée par `mhgp10_tower_oracle` (7 600 coupes), qui refuse les
  multiplicités ;
- tête (condensation, EOM, feuilles), égale à sklearn à K = 1 et 2 ;
- banc Python (`bench/synthetic`) contre `sklearn.cluster.HDBSCAN` tel quel, reçu de développement
  `receipts/bench_dev_20260928` ;
- référence exacte Python `reference/hgp10_ref.py`.

On appelle **V10-α** cet état. Ce document est **normatif pour la suite** : chaque composant de V10-α est gardé s'il
passe la porte de sa phase, migré sinon (§ 12.0). Deux constats de la passation de V10-α fixent l'ordre des travaux.
Ce sont des mesures locales sans reçu, donc des diagnostics seulement :

- la tour est le goulot (4 fils, hôte chargé) : K5 4–6 s de tour contre 3–4 s de catalogue ; K10 32–56 s contre
  11–15 s ; RSS 2,4 Go. C'est 6 à 24 fois l'estimation de TOWER_v2 (§ 9.4) ;
- le banc de développement donne la **parité** entre la meilleure configuration unique de la tour (K = 1, √n, ẑ, EOM,
  remplissage) et le meilleur HDBSCAN réglé sur les mêmes graines : ARI_s 0,7444 contre 0,7444. La victoire nette
  n'existe que contre HDBSCAN par défaut (0,5563).

---

## 1. Résumé exécutif

1. **L'objet ne change pas** : pour K = 1..Kmax (Kmax ≤ 10), la tour FULL de π0(L_K(a)) sur le multiensemble des
   sites, niveaux = rayons carrés rationnels exacts, naissances, multifusions N-aires, continuations datées, plateaux,
   extension non régulière, verticales K → K−1.
2. **Générateur** : boîtes de centres à listes K-certifiées (GEN_v2). Linéaire en la sortie sur toutes les familles
   mesurées (exposants de travail ≤ 1,126), 3,7 à 5,8 µs CPU par boule en sonde. Le WSPD et le paramètre `s`
   disparaissent. Admission unique `p_w + q_min ≤ K + 1`.
3. **Tour** : résolveur pur par saut K-NN (λ strictement décroissant), minima par sauts de pointeurs, hyperarêtes en
   étoile, puis forêt par ordre en trois temps : noyau union-find séquentiel **sans lots** (6,6–9,2 ms mesurés à K5,
   15,9–22,3 ms à K10), classement de liste parallèle, matérialisation cartésienne parallèle des multifusions
   (PO-T18). L'étage G, parallèle, porte 65 à 75 % du travail.
4. **Points et tête** : hiérarchie C∩X lue sur l'index d'intervalles de la tour par graphe-chemin (théorème PG),
   condensation N-aire, λ = r^(−z) en binary64 pur, ẑ au 1/16. La tête coûte ≈ 3–8 ms sur G4.
5. **HDBSCAN n'est pas réimplémenté** : les adversaires sont sklearn 1.9.1 tel quel (`kd_tree` épinglé, α explicite).
   Le témoin géométrique E1 est l'arbre d'atteignabilité mutuelle de sklearn à α = 2, réatomisé exactement (lemme
   V10-I1, § 10).
6. **Contrat LiDAR** C(K, T) sur le mur résident B2, 30 trames de test sans sol, digest égal à une référence certifiée
   (`kcat` = Kmax + 2), cohérence de préfixe K5 ⊂ K10, juge des clés absentes.
7. **Prévisions chiffrées sur G4, CPU seul** (sonde GEN + estimation TOWER, non mesurées sur le produit) : B2 ≈
   0,19–0,37 s à K5 sur 08/000200 et 0,27–0,53 s sur la pire trame de 60 k sites, donc **C(K5, 1 s) probable** ;
   B2 ≈ 0,7–1,4 s à K10 et 1,05–2,1 s au pire, donc **C(K10, 1 s) improbable sans GPU**. Sur GPU : K5 ≈ 45–100 ms
   (recherche), K10 ≈ 0,1–0,3 s. **100 ms à K10 : hors d'atteinte connue.**
8. **Clustering, sans complaisance** : la prévision écrite est que la géométrie exacte de la tour n'apporte rien au
   clustering plat (issue N). À K = 1, la tour **est** HDBSCAN(ms = 1, α = 2). Le gain sur HDBSCAN standard viendra de
   la tête (ẑ, mcs = √n, bruit). `decide.py` écrit la phrase, quel que soit le signe.
9. **Vérification** : invariants O(sortie) dans le produit ; oracles bornés (T2 Γ_K n ≤ 14, catalogue brut, C∩X
   brut) ; juges d'échelle indépendants (J-KM2, boîtes, J-CENSUS, J-DESC, J-MF, EMST, BFS local, cohérence
   points–verticales, différentiel v9) ; mutants par copies mutées ; aucun juge O(n³), aucune vérification exhaustive
   à l'échelle.
10. **Plan** : V10-0 à V10-5, plus V10-G (catalogue GPU, ouverte par la décision D-K5). Chemin critique LiDAR :
    V10-1 → V10-2 (réécriture de la tour selon TOWER_v2) → V10-4. Environ 47 à 66 jours-agent, 3 à 4 semaines de mur
    à 4–5 agents.
11. **Trois questions** restent à l'utilisateur (§ 14) : données KITTI, fichiers normatifs `AGENTS.md`/`CLAUDE.md`,
    nettoyage du disque (3,3 Go libres).

---

## 2. Objet, objectifs, contrats

### 2.1 Objet (inchangé)

- Entrée : points u18 (grille de 1 mm pour le LiDAR), `PointId` u32 arbitraires ; points de même position = un
  **site** de poids `w_s ≥ 1`. `W = Σ w_s`, `K_eff = min(Kmax, W)`.
- `L_K(a) = { y : Σ_{s : |y − s|² ≤ a} w_s ≥ K }`. Sortie : le foncteur π0 sur `{1..K_eff}^op × ℝ` avec inclusions
  horizontales (a ≤ b) et verticales (`L_{K+1} ⊆ L_K`), représenté par la tour FULL de la v7 : pour chaque K, forêt
  de fusion à naissances et multifusions N-aires, continuations à contributions datées, plateaux atomiques par niveau
  exact, quotient local de Gordan des coquilles étendues ou pondérées, verticales à la coupe fermée, une racine
  finale par K.
- Modèle discret : Γ_K (Th. 2 du manuscrit, `theorem_external`), étendu aux multiensembles (PO-T16).
- Hiérarchie de points C∩X d'ordre K : x entre au niveau `D_K(x)` (K-ième voisin du multiensemble, x compris) et
  appartient à la composante de `L_K(D_K(x))` qui le contient. C'est une partition emboîtée en a et en K ; divergence
  déclarée avec les amas discrets recouvrants de la thèse (Déf. 8).
- **Mosaïque de Delaunay d'ordre supérieur** : non matérialisée. Le catalogue de boules critiques n'est ni un
  catalogue de cellules ni de cofaces : ce sont des sphères sans adjacence, en nombre mesuré n·Θ(1) sur toutes les
  familles (31–33 par site à K5 et 120–138 à K10 sur LiDAR, ≈ 420 à K10 en uniforme), suffisantes pour π0 (P2) et
  complètes par théorème (V10-GEN-1 à 7), avec fixtures. Les facettes de Γ_K ne sont jamais matérialisées : les
  représentants de l'étage G sont transitoires.

### 2.2 Hérité de la v9

| Élément v9 | Statut en v10 | Raison |
| --- | --- | --- |
| Tour FULL v7/v9, T2 exhaustive n ≤ 14 (0 désaccord sur 13 696 coupes multi-K) | **gardé**, étendu aux multiensembles et aux descentes longues | c'est l'objet et son seul juge complet |
| Profil u18, prédicats entiers, aucun jitter, flottant = filtre certifié à repli exact | gardé | doctrine du dépôt |
| Trames SemanticKITTI sans sol 30–60 k sites, K5 puis K10, 1 s puis 100 ms sur G4 | gardé (contrat § 2.4) | directive LiDAR |
| R22 (G4, GPU) : chaîne K5 0,93 / 0,76 / 0,98 s ; K10 2,96 / 2,27 / 2,89 s ; mur processus K5 1,5–1,9 s | **référence à battre**, pas un contrat | meilleur chiffre v9 |
| 12 épingles L13/v9 du catalogue (uniforme, huit amas, terrain 8k/16k/32k K5 ; huit amas 8k K10 ; 08/000200 K5 et K10) | gardées, épinglées | continuité |
| Statuts transactionnels, publication tout ou rien, `complete_relative` | gardés sous 5 statuts (§ 6.7) | un seul vocabulaire |
| WSPD, séparation s ≥ 8 | **supprimés** : il n'y a plus de WSPD | générateur par centres |
| Six chemins « même objet », 38 leviers, 3 index, triple recensement, mutants `#if` | supprimés | L12 |
| Clustering du 28 sept. (repli Gabriel E5, condensation fausse, oracle HDBSCAN 1D) | **jeté**, conservé comme fixtures de réfutation | L08, L14 |

### 2.3 Objectifs de la v10

| Objectif | Mesure | Verdict | Phase |
| --- | --- | --- | --- |
| Catalogue exact et sensible à la sortie | exposant de travail ≤ 1,15 sur 12 familles à doublement pur ; LiDAR en absolu (COST-C) | porte | V10-1 |
| Tour exacte à coût linéaire, parallèle | T2 + juges ; compteurs par boule ≤ 1,15 par doublement ; µs CPU par boule publiés | porte | V10-2 |
| C(K5, 1 s) sur G4 | contrat § 2.4, bras `cpu48` d'abord | verdict publié quel qu'il soit | V10-4a |
| C(K10, 1 s) sur G4 | idem, bras `gpu` attendu | verdict publié | V10-4b |
| C(K5, 100 ms) | décomposition mesurée contre le budget par étage (§ 9.1) | cible de recherche, sans promesse | V10-4b |
| Mémoire | RSS ≤ 0,4 Go (K5) et ≤ 1,2 Go (K10) sur trame sans sol ; pic tour ≤ 100 o par boule | porte | V10-2, V10-4 |
| Clustering hiérarchique contre HDBSCAN | revendications R1, ATT, R3, ARB (§ 4.5) sur banc préenregistré 8k/16k/32k | phrase générée par `decide.py` | V10-5 |
| Multiplicités natives | T2 pondérée, Euler pondéré | porte ; lève `duplicate_positions` | V10-2 |
| Déterminisme | digests égaux à W ∈ {1, 2, 8, 48}, sous permutation, entre backends | porte | toutes |
| Taille de code | ≈ 12 k lignes produit CPU, ≈ 2,5 k GPU, ≈ 8 k tests (v9 : 33,5 k + 33,8 k) | diagnostic | toutes |
| CI | verte, `fast` ≤ 5 min, job ≤ 15 min mesuré | porte | V10-0 |

### 2.4 Contrat LiDAR C(K, T) (EVAL § 12, adopté)

- Entrée : 30 trames de test sans sol (3 par séquence 00–07, 09, 10, tirées par SHA-256, gelées dans
  `perf/FRAMES_v2.toml`), quantifiées par la règle v8 (§ 6.2). Trames de conception : 08/000000, 000100, 000200.
  Trames dev : 20, deux par séquence hors 08, écart ≥ 20 scans avec le test.
- Chronomètre **B2** (`wall_resident`) : des sites en mémoire hôte à la tour FULL canonique en mémoire hôte,
  transferts, allocations et synchronisations compris. B0 (processus froid) et C (sol + quantification + B2 + tête)
  sont publiés dans la même phrase.
- Satisfait si : p95 des médianes par trame ≤ T ; maximum des médianes ≤ 1,2 T ; 100 % des passes ont un
  `merkle_prefix(Kmax)` égal à celui de la référence certifiée ; contrôles EP1 (cohérence de préfixe), EP3 (clés
  absentes), invariants globaux et EMST verts ; aucune trame refusée.
- Porte **EP5 (go/no-go)** avant toute session de contrat : `B2_pred = balls · c_ball · κ / (24 · η) + t_fixed`
  extrapolé à 60 k sites doit valoir ≤ 0,8 T (§ 9.1). Un NO-GO se publie.
- Trames brutes (avec sol) : régime secondaire, B2, B0 et RSS publiés, correction jugée à l'identique, pas de porte de
  phase (budgets RSS ≤ 0,7 Go à K5 et ≤ 2,5 Go à K10).

### 2.5 Ce que la v10 ne promet pas

- `public_status=exact` : jamais. La tour est exacte **relativement** au catalogue ; le catalogue est complet par
  théorème et jugé par échantillon.
- 100 ms à K10 ; une borne de pire cas (la sortie peut être Ω(n²), v7) ; l'échelle de 10^6 points (hors v10.0) ;
  la segmentation sémantique LiDAR ; une supériorité de la géométrie exacte sur l'atteignabilité mutuelle.

---

## 3. Décisions d'architecture

Chaque décision cite sa raison et le document qui la détaille. Les numéros sont ceux de ce document ; l'annexe A
donne la correspondance avec les identifiants des sous-systèmes.

| Id | Décision | Raison | Détail |
| --- | --- | --- | --- |
| D-V10-01 | Base neuve ; la v9 est un sujet différentiel épinglé (`ce8a649dd`), jamais une base de code ; tout port est listé dans `docs/PROVENANCE.md` | L12 : la v9 juxtapose trois portages | ARCH D-01 |
| D-V10-02 | Générateur par boîtes de centres à listes K-certifiées ; filtre d'arbre = dominance seule (G ⊂ D), sans branchement ni multiplication ; pas de règle de stagnation ; racine fixe `[0, 2^18)³` ; T = 6 ; M(K) = 16 / 24 / 32 ; `M_hard` = 128 | linéaire sur les familles adverses ; 0 refus sur 98 exécutions ; la stagnation v1 faisait refuser amas et grilles | GEN § 1, § 6, § 7 |
| D-V10-03 | Admission unique et positionnelle `p_w + q_min ≤ K + 1` (q_min en positions, p_w pondéré), pour toutes les coquilles | lemme W prouvé ; la clause « p ≤ K − 1 » sur coquille pondérée contredisait la tour et gonflait le catalogue | GEN § 2.3, § 3.8 |
| D-V10-04 | Deux repères arithmétiques : sites (centres, recensement, niveaux) et boîte en unités 2^−6 (dominance, lemme Z) ; pont unique « centre ∈ boîte » en i128 | mettre les sites à l'échelle 2^T casse l'i128 (mutant M13 observé) | GEN § 5 |
| D-V10-05 | Aucun flottant ne décide. Seule la clé de tri des niveaux est flottante (bande 2^−49 réparée en U320). Le filtre certifié O-F1 n'est adopté que sur preuve écrite et gain ≥ ×1,5 de la feuille K10 | doctrine du dépôt ; la feuille i128 fait 85 % du temps à K10 | GEN § 3.11, § 5.4 |
| D-V10-06 | Identité d'une boule = S\* ; ordre canonique (rang, S\*) ; **rang 0 = niveau nul**, rangs du catalogue ≥ 1 | S\* détermine la boule (P16) ; un seul espace de rangs pour catalogue, tour et points | GEN § 6.4 ; § 5 ligne 18 |
| D-V10-07 | Deux index à propriétaire unique : `LeafOracle` (requêtes à centre rationnel, propriété de `catalogue/`, utilisé par `tower/`) et `SiteTree` (requêtes en un site entier : ẑ, remplissage, juges) | lemme O : recensement exact sur ≤ 24 sites sans élagage ; plus de pavé contre boule à 2^191 | ARCH D-03\*, GEN § 9 |
| D-V10-08 | Tour : étages P, G, M, T1, T2a (noyau sans lots), T2b (classement de liste), T2c (règle cartésienne), Q ; pas de préfiltre MSF ni de renumérotation Morton ; option D&C (d ≤ 2) seulement sur mesure G4 | noyau mesuré 3 à 6 fois plus rapide que le Kruskal par lots, sortie identique ; PO-T18 | TOWER § 6.5 |
| D-V10-09 | Résolveur pur par saut K-NN ; I3 strict (λ décroît à chaque saut) ; un seul résolveur dans le dépôt (`tower/`) | PO-T4 corrigé ; la tête ne calcule aucune MEB | TOWER § 6.2, § 9.1 |
| D-V10-10 | Multiplicités natives dans l'objet et le catalogue dès V10-1 ; dans la tour, refus explicite `unsupported_degeneracy/duplicate_positions` jusqu'à la T2 pondérée verte, levé dans le même commit à la sortie de V10-2 ; Euler pondéré (PO-T19) actif | livraison par étapes jugée ; aucune trame LiDAR mesurée n'a de doublon à 1 mm | ARCH D-05\*, TOWER D-T14 |
| D-V10-11 | Coquilles : quotient circulaire exact pour tout u, table 3D pour u ≤ 16, classes pondérées en O(u·2^u) ; refus par boule (boule publiée) au-delà | PO-T20, PO-T21 ; entrées z = 0 entièrement couvertes | TOWER D-T13 |
| D-V10-12 | Produit : validation structurelle de toutes les boules, géométrique 1/64, naturalité 1/16 (sel publié). Validation complète, J-KM2, J-CENSUS, J-DESC, J-MF, boîtes, EMST, BFS local et différentiel v9 : dans `tests/` seulement. Pas de mode `verify` public | la structure protège la mémoire ; la géométrie re-vérifie un théorème (plan de tests § 3.2) | § 5 ligne 14 |
| D-V10-13 | `kcat` = Kmax en produit ; références certifiées à `kcat` = Kmax + 2, hors chronomètre ; code de tour dimensionné pour K ≤ 12, API publique bornée à 10 | Euler ne juge l'ordre K qu'avec `kcat ≥ K + 2` ; la référence K10 peut alors être la restriction d'une tour K12 | § 5 ligne 15 |
| D-V10-14 | Cinq statuts produit ; `catalogue_incomplete/*` de TOWER devient `invariant_violated/*` ; succès = `ok` avec `exactness=relative_to_catalogue` | une faute de générateur n'est pas une propriété de l'entrée | ARCH § 4.5 |
| D-V10-15 | Budget mémoire logique déterministe, frais F(W) hors budget, aucune annulation dans un étage (P19) | statut indépendant de W | ARCH § 4.4 |
| D-V10-16 | Digests SHA-256 hors chronomètre : `engine_digest` (Merkle), `catalogue_digest` (sans valeur de niveau ni numéro de rang, avec bit d'égalité), `tower_digest_v10`, `tower_merkle_v10`, `id_digest` | FNV sur mots était cassable ; J-KM2 compare des catalogues de rangs différents | § 6.8 |
| D-V10-17 | HDBSCAN n'est jamais réimplémenté. Adversaires = sklearn 1.9.1 tel quel, `algorithm="kd_tree"`, α explicite. Arbres MR_α = `_single_linkage_tree_` de sklearn, réatomisés par égalité exacte des poids (V10-I1) ; aucun `mreach` C++ dans le produit | directive du 28 sept. ; kd_tree rend α sémantique (PO-E13) ; l'atomisation est exacte en u18 | § 5 lignes 8–9 |
| D-V10-18 | Hiérarchie de points par graphe-chemin sur (`leaf_lo`, `jrank`, `entry_node`) de la tour ; clés de lot u64 unifiées entre ordres ; `entry_pos` et `entry_eq` publiés par l'étage Q | O(n log n + L_K), indépendant de l'algorithme de T2 ; théorème PG | CLUSTER § 4.1 |
| D-V10-19 | Tête : condensation N-aire (sémantique sklearn étendue), EOM avec égalité → parent, racine exclue par défaut, λ = r^(−z) en binary64 pur (sans FMA, sans libm), plancher λ(0) = λ(r_floor), ẑ MLE sur sites distincts arrondi au 1/16, remplissage en abstention sur égalité, étiquettes numérotées par plus petit `SiteIdx` | canonique, bit-identique entre plateformes, équivariant | CLUSTER § 5–8 |
| D-V10-20 | Revendications calculées par `decide.py` : R1 (IUT sur cinq adversaires sklearn), ATT (H3 : tour contre `mr2_P*`), R3 (oracles), ARB (qualité d'arbre) ; niveau inférieur R1-std publié si R1 échoue | EVAL D5, D12 ; pas de phrase sans reçu | § 4.5 |
| D-V10-21 | Livrable de clustering = la tête sur la hiérarchie C∩X de la tour, dans tous les cas ; si ATT n'est pas établie, la phrase dit que le gain vient de la tête et que la même tête sur l'arbre MR α = 2 de sklearn fait aussi bien | la tour est calculée de toute façon pour le LiDAR ; pas de second producteur à maintenir | § 5 ligne 10 |
| D-V10-22 | Contrat B2 résident sur 30 trames de test sans sol ; B0 et C publiés ; trames brutes en régime secondaire | décision du 21 sept. ; continuité avec la « chaîne » R22 | EVAL § 12.2 |
| D-V10-23 | EP5 (go/no-go) et D-K5 (ouverture de V10-G) utilisent la même formule, alimentée par les sessions C0/C1 ; la première session G4 est une calibration | décisions sur mesures, pas sur déclarations | § 9.1, § 12 |
| D-V10-24 | Code : C++20, `-Wall -Wextra -Wpedantic -Werror`, en-tête contrat par couche, un `sched::Pool` par session, aucune directive conditionnelle hors liste, mutants par copies mutées (code 4 réservé à l'enveloppe), fichier ≤ 600 lignes, liste blanche de paramètres publics | L12 : 38 leviers, 12 sites de création de fils, 21 mutants morts dans le produit | ARCH § 9 |
| D-V10-25 | CI v10 dédiée, Python 3.12 par `setup-python`, g++ 11 et CMake 3.22.1 comme la VM ; rouge = arrêt des poussées | CI v9 rouge 40 fois | ARCH § 13 |
| D-V10-26 | Sessions G4 depuis un commit poussé, répertoire de session persistant, reprise sans redémarrage ; les trois bloquants de la revue adverse de `v10_session.py` sont corrigés avant toute session | incident du 27 août ; revue du 28 sept. | ARCH § 15 ; § 12.3 |
| D-V10-27 | GPU subordonné : catalogue (V10-G) puis étages G, M, T1, T2b, T2c, Q de la tour (V10-4b) ; T2a reste sur l'hôte ; `__int128` natif ; aucun FP64 décisionnel ; un étage ne passe sur GPU que s'il dépasse 30 % du profil `cpu48` ; égalité de digest CPU/GPU | la v9 a montré des écarts ×10 entre projection et mur | § 8.4 |
| D-V10-28 | Travail en parallèle par rôles A–E dans des worktrees clairsemés, porte disque, propriété par dossier, juges écrits sans lire l'implémentation | disque à 95 % ; indépendance des juges | ARCH § 20 |

---

## 4. Chaîne et sous-systèmes

### 4.1 Chaîne de calcul

```text
entrée u32 (xyz u18, PointId)                                                     [api]   validation, invalid_input
 S0  cloud      tri Morton, sites pondérés, CSR site -> PointId, SiteTree           [cloud] parallèle
 S1  catalogue  arbre de centres (dominance D, k-DOP), phase en largeur 64W        [A]     parallèle ; GPU V10-G
 S2  catalogue  feuilles : paires, triplets (lemme Z), quadruplets, recensement    [A]     une tâche par feuille ; GPU V10-G
 S3  catalogue  clé double fidèle, tri, bandes réparées en U320, rangs >= 1         [A]     parallèle
 --> Catalogue + LeafOracle (vit jusqu'à la fin de Q)
 S4  tower P    structure (toutes), géométrie 1/64, atlas, quotients locaux, H_K, Euler I2       [B]
 S5  tower G    représentants -> resolve (H_K, MEB exacte, lookup, saut K-NN) ; K décroissant   [B] GPU V10-4b
 S6  tower M    minima fixes par sauts de pointeurs, par ordre                                  [B]
 S7  tower T1   hyperarêtes en étoile, triées par clé (rang, cellule, i)                        [B]
 S8  tower T2a  noyau union-find + listes, arête par arête, sans lots (un fil par ordre ; Kmax en tête)
 S9  tower T2b/T2c  (pi, J) par classement de liste ; nœuds, multifusions, numérotation canonique  [B]
 S10 tower Q    jblock, WA : ancres, contributions, verticales, naturalité 1/16, entrées C∩X      [B]
 --> Tower (fin du chronomètre B2) ; digests hors chronomètre (io)
 S11 points     graphe-chemin par ordre -> PointDendrogram                                       [C]
 S12 head       condensation N-aire, sélection, remplissage, étiquettes                          [C]
```

Rôles : A géométrie, B tour, C clustering, D vérification, E infrastructure puis GPU (ARCH § 20.1).

### 4.2 GEN — générateur (détail : `GEN_v2.md`)

- Théorie : lemmes D, D-loc, G ⊂ D, proposition L, théorème C, lemmes M, K, S, Z (forme fermée i64), U (la coquille
  détermine la boule), W (admission pondérée), O (oracle K-NN), F (listes larges inhérentes).
- Nœud : une passe calcule `A(x) = |x'|²` et `δ(x) = |2x' − s·1|²` ; tampon d'insertion des 3K plus proches du centre ;
  `Y` = plus court préfixe de poids ≥ 3K ; noyau `cnt(x) += w(y) · [A(x) − A(y) > (Σ max(0, x'_i − y'_i)) << (log2 s + 1)]`
  ; `L(Q) = {x : cnt(x) < K}` ; découpe si `|L| > M(K)` et `s > 1`.
- Feuille : matrice de dominance, paires, triplets (lemme Z), quadruplets, recensement avec sortie anticipée
  `p_w > θ(T) = K + 1 − |T|`, mémo des coquilles étendues par masque U, raccourci q4 avant le calcul du centre.
- Sortie : `Catalogue` SoA (§ 6.3) et `LeafOracle`. Coût mesuré (sonde) : 3,7 à 5,8 µs CPU par boule, arbre 15 à 30 %,
  feuille i128 le reste (85 % à K10).

### 4.3 TOWER — tour FULL (détail : `TOWER_v2.md`)

- Fenêtre d'une boule : `lo(b) = max(1, p_w + q_min − 1)`, `hi(b) = min(Kmax, W, p_w + u_w)` ; boules de rayon nul,
  une par site, fenêtre `[1, min(Kmax, w_s)]`.
- Quotient local à l'ordre K (`t = K − p_w`) : t-multiensembles stricts de U (c hors de l'enveloppe fermée) ; aucune
  composante → naissance de population I ∪ U ; sinon un représentant `I ∪ A` par composante, contribution
  `D = U ∖ ⋃ supp(sommets stricts)`.
- Lots : cellules d'un ordre et d'un niveau exact ; groupes par racine pré-lot ; 0 racine = naissance, 1 = continuation
  ou bloc inerte, ≥ 2 = **une** multifusion.
- Algorithme : § 4.1 ; propriété (P) sur (π, J) ; règle cartésienne (PO-T18) ; requête unique `WA(v, seuil)` sur
  `jblock` ; numérotation canonique (rang, plus petite feuille du sous-arbre).
- Coût estimé (G4, W48, CPU) : K5 LiDAR 33–53 ms (optimiste) à 70–110 ms (pessimiste) ; K10 0,19–0,28 s à
  0,40–0,58 s ; ≈ 75 o persistants par boule, pic ≤ 100 o.

### 4.4 POINTS et HEAD — hiérarchie C∩X et têtes (détail : `CLUSTER_v2.md`)

- Producteur tour → points par graphe-chemin : `p[x] = leaf_lo[entry_node[x]]` ; tri radix par (p, `SiteIdx`) ;
  `w_m = max jrank[p_{x_m} .. p_{x_{m+1}} − 1]` (⊥ si positions égales) ; Kruskal par lots sur n − 1 arêtes et n
  activations ; invariants I-H1 à I-H3.
- Têtes candidates préenregistrées : **T1** EOM-densité à K fixe (K ∈ {1, 2, 3} sur `dev`, 5 en variante), **T2**
  tranche γ multi-ordres (K_hi = 5, composition des dendrogrammes P_k, jamais d'arêtes-chemins d'un autre ordre),
  **T3** § 9.1 pondéré sur facettes (tour seule, **différée**, § 5 ligne 31).
- Paramètres communs : mcs = round(√N) en masse ; EOM, égalité → parent ; z ∈ {1, ẑ} sur le banc (z = 1 = ligne
  d'équité), **z = 2 fixé sur LiDAR** avec ẑ publié ; politique de bruit `none | b<ρ> | full` choisie sur `dev`.
- Coût : ≈ 3–5 ms (K5) et 4–8 ms (K10) de mur G4 W48, tous ordres compris ; ≤ 4 % du total.
- Prévision écrite (P1a–P1c) : H3 non rejetée ; issue N ; gain sur HDBSCAN standard = effet de tête.

### 4.5 EVAL — banc et performance (détail : `EVAL_v2.md`, amendé par le § 5)

- Données : 16 familles 3D + 3 témoins nuls, 4 niveaux géométriques calibrés, bruit ν ∈ {0 ; 0,1 ; 0,3}, tailles
  500 / 2 000 / 8 000 / 16 000 / 32 000 (strate décisive 8k/16k/32k), graines par SHA-256, espaces `calib`, `cost`,
  `dev`, `test`, `test2` disjoints ; suites publiques descriptives.
- Métrique primaire ARI_s (bruit en singletons, O(n)) ; gardes F1_H et AMI_s.
- **Adversaires HDBSCAN (tous sklearn tel quel)** : `hdb_dev` (réglé sur `dev` : ms ∈ {1, 2, 3, 4, 5, 6, 8, 10, 12, 16,
  20, 32}, mcs, EOM ou feuilles, eps, asc, α ∈ {1, 2}, remplissage appliqué après coup par la fonction commune),
  `hdb_match` (ms = K_ref(P), α = 2, paramètres de P), `hdb_match_fill`, `hdb_lib` (défauts), `hdb_these_K1`
  (mcs = √N, ms = 3, α = 1). Si P utilise une sélection sans étiquettes par scène (DBCV), `hdb_dev` y a droit sur sa
  propre grille.
- **Revendications** (Holm entre les quatre) : R1 = IUT « `tw_P` bat chacun des cinq adversaires », δ_min = +0,02
  d'ARI_s, gardes sans perte, refus ≤ 1 % ; **R1-std** = même IUT sans `hdb_dev` (niveau inférieur, publié s'il est
  atteint) ; ATT = H3 `tw_P` contre `mr2_P*` ; R3 = `tw_oracle_full` contre `hdb_oracle` (grille sklearn complète) ;
  ARB = DP et BNF1 de `tw` contre `mr2` à K_ref.
- Statistique : scène = unité ; retournement de signe stratifié (Rademacher, 10^5 tirages) ; IC par bootstrap de
  McCarthy–Snowden ; calibrage EG3.
- Performance : § 2.4 ; références certifiées ; mutants EP-M1 (strate de fin de fenêtre omise) et EP-M2 (omission
  éparse 10^−3).

### 4.6 ARCH — infrastructure (détail : `ARCH_v2.md`)

Couches strictes L0 `core`/`sched` → L1 `arith` → L2 `cloud` → L3 `catalogue` → L4 `tower` → L5 `points` → L6 `head`
→ L7 `api` ; `io` (digests, sérialisations) séparé ; `gpu` backend de L3/L4 ; `alloc` choisi à l'édition de liens
(empoisonnement 0xA5 en test). `tools/check_v10.py` vérifie mécaniquement : graphe d'inclusion, une seule définition
de `resolve`, aucune requête rationnelle hors de `leaf_oracle`, aucun fil hors de `sched`, aucun `getenv`, fichiers
≤ 600 lignes, liste blanche des paramètres, extraits de mutants présents, README ≤ 80 lignes, PASSATION ≤ 300.

---

## 5. Arbitrages entre sous-systèmes

Chaque ligne tranche une incohérence constatée entre documents (ou entre un document et V10-α).

| # | Sujet | Positions | Décision | Raison |
| --- | --- | --- | --- | --- |
| 1 | Admission pondérée | GEN_v2 : unique `p_w + q_min ≤ K + 1` ; ARCH_v2 § 5.2 et V10-α : « p ≤ K − 1 » si la coquille est pondérée | **GEN_v2** | lemme W prouvé et validé (600 nuages pondérés, J-KM2 pondéré) ; TOWER utilise déjà la fenêtre positionnelle ; P2b (monotonie) reste vrai |
| 2 | `catalogue_digest` | GEN : sans niveau ni rang ; ARCH § 12.5 : niveau réduit inclus | **GEN, plus un bit d'égalité** avec la boule précédente | le niveau est déterminé par S\* ; le pgcd coûte ≈ 5,5 CPU·s à K10 ; le bit d'égalité (calculé sur les rangs de chaque catalogue) survit à la restriction et détecte une faute de regroupement des rangs |
| 3 | Vocabulaire des statuts | TOWER : `complete_relative`, `catalogue_incomplete/*` ; ARCH et V10-α : 5 statuts ; EVAL : `complete`, `refused_domain`, `numeric_failure`… | **5 statuts ARCH** ; correspondance § 6.7 | un vocabulaire ; `numeric_failure` ne peut pas se produire (aucune décision flottante) |
| 4 | Algorithme de T2 | ARCH § 5.3/§ 6.1 : préfiltre MSF (Borůvka) puis Kruskal par plateaux ; TOWER_v2 : noyau sans lots + classement de liste + règle cartésienne | **TOWER_v2** | mesuré sur la structure réelle LiDAR ; ARCH P13 (balayage intra-ordre) devient PO-T18/PO-T22 |
| 5 | Multiplicités dans la tour | ARCH D-05\* : refus jusqu'à PO-T16 ; TOWER_v2 : natives ; V10-α : refus | **natives visées, refus transitoire** jusqu'à la T2 pondérée verte (sortie de V10-2) | on ne publie pas un calcul non jugé ; coût nul pour les contrats |
| 6 | Euler pondéré | GEN (O-W2) et ARCH (P17) : ouvert ; TOWER : PO-T19 prouvée (340 nuages, 3 270 contrôles) | **proved_here à inscrire** ; I2 et l'étape Euler de J-KM2 actifs sur entrée pondérée | preuve écrite par valuation d'Euler sur les copies, mutant tué 40/40 |
| 7 | Requêtes à centre rationnel | V10-α : `SiteTree` ; GEN, ARCH, TOWER : `LeafOracle` | **LeafOracle**, migration en V10-2 | l'étage G fait 65–75 % de la tour ; recensement sur ≤ 24 sites sans élagage (lemme O) |
| 8 | Producteur MR_α | CLUSTER § 4.3 et EVAL § 2.3 : Borůvka exact en C++ ; directive utilisateur : pas de réimplémentation | **sklearn `kd_tree` + réatomisation exacte** (V10-I1) ; `tests/head/mreach.cpp` de V10-α est retiré du chemin E1 | directive du 28 sept. ; la réatomisation est exacte en u18 (§ 10) ; porte EG-ATOM |
| 9 | Définition de `hdb_dev` | EVAL : notre tête sur les sources `mr1`/`mr2` | **sklearn tel quel réglé sur `dev`** ; notre tête sur MR = famille témoin `mr2_P*` (ATT) | un adversaire nommé HDBSCAN doit être HDBSCAN ; le témoin géométrique reste mesuré |
| 10 | H3 | CLUSTER : IUT sur α ∈ {1, 2}, K ∈ {2, 3} ; EVAL : `tw_P` contre `mr2_P*` | **EVAL** ; E1-dev publié à K ∈ {1, 2, 3, 5}, `mr1_P*` en diagnostic | α = 1 est déjà couvert par `hdb_dev` dans R1 ; un test unique garde la puissance |
| 11 | λ au rayon nul | EVAL § 2.5 : +∞ comme sklearn ; CLUSTER : plancher λ(r_floor) | **plancher** pour notre tête sur toutes les sources ; les adversaires sklearn gardent leur +∞ | une stabilité infinie rend EOM dégénérée ; seules les suites publiques à doublons sont touchées ; EG7 exclut les scènes à niveau nul |
| 12 | Égalités du remplissage | EVAL : ordre lexicographique des coordonnées ; CLUSTER : abstention | **abstention**, comptée (`fill_ties`) | aucun ordre arbitraire, pas même l'orientation du repère ; CLUSTER possède la tête |
| 13 | Noms des politiques de bruit | CLUSTER `bounded(ρ)`, EVAL `b2`, ARCH `abstain/bounded_fill/full_fill` | **`none`, `b<ρ>`, `full`** partout (CLI `--fill`) | un seul nom |
| 14 | Modes `checks=full/sampled` | TOWER : paramètre ; ARCH D-13 : pas de mode `verify` | **produit = échantillonné seulement** ; la validation complète est un juge de `tests/` qui appelle les mêmes fonctions | pas de mode caché ; sortie identique par construction |
| 15 | Mode certifié `kcat = Kmax + 2` | TOWER : option de produit ; ARCH : juge J-KM2 ; EVAL : références, tour K12 si possible | **références hors chronomètre** ; tour interne jusqu'à K = 12 | la certification se transfère aux passes chronométrées par égalité de digest (P20) |
| 16 | Drapeau « coquille circulaire » | TOWER § 3.1 : dans le catalogue ; GEN : format sans ce drapeau | **calculé par l'étage P de la tour** (`LocalTables`) | le générateur ne calcule aucun prédicat de tour (GEN § 2.4) |
| 17 | Index des boules de rayon nul | TOWER § 5.1 « avant le catalogue » contre § 5.2 « réelles puis virtuelles » | **virtuelles d'abord** : index atlas `s` pour le site s < n_sites, `n_sites + b` pour la boule b | cohérent avec l'ordre canonique (rang 0 en tête) |
| 18 | Rang 0 | TOWER, CLUSTER, V10-α : niveau nul ; GEN : rangs non précisés | **catalogue à partir de 1**, `level_rep[0] = kNone` | un espace de rangs commun ; clé ⊥ = 0 de CLUSTER |
| 19 | `level_double` | CLUSTER : exigence de bit-identité ; GEN : clé de tri ; TOWER : recalcul | **une seule fonction** `arith/level.hpp` : 64 bits de tête avec bit collant pour numérateur et dénominateur, conversions u64 → double, une division, `-ffp-contract=off` | même valeur pour le tri, la tête et le GPU |
| 20 | Hiérarchie de points | ARCH : `point_node`/`point_rank` ; CLUSTER : `AtomDendrogram` ; EVAL : `tree.bin` | **forme CLUSTER** en mémoire, sérialisée en `tree.bin` v2 (EVAL) | les atomes sont les feuilles ; un seul format pour toutes les sources |
| 21 | Budget d'octets | EVAL : ≤ 32 o par boule ; ARCH et TOWER : catalogue 30–48 o + tour ≤ 100 o au pic | **ARCH § 5.6** | une sortie FULL (ancres, index, contributions) ne tient pas en 32 o |
| 22 | Constante du catalogue | EVAL/ARCH : 14,6 µs (cble) ; GEN_v2 : 3,7–5,8 µs (sonde ingénieriée) ; V10-α : ≈ 8,5–11 µs (sans reçu) | publier les trois avec leur statut ; **EP5 et D-K5 n'utilisent que la constante du produit mesurée en C1** | une décision sur une sonde a déjà coûté ×10 en v9 |
| 23 | Coût de la tour | TOWER_v2 : 1,7–2,6 CPU·s à K5 ; V10-α : 4–6 s de mur à 4 fils | **l'estimation est une cible** ; sortie de V10-2 sur compteurs + µs CPU par boule publiés ; verdict sur G4 | les temps locaux sont des diagnostics (hôte chargé) |
| 24 | Qui calcule `D_K(x)` | TOWER : étage Q par `LeafOracle` ; CLUSTER/ARCH : `SiteTree::kth_distance` | **tour** (`entry_level`) ; `SiteTree` seulement pour ẑ, remplissage et juges ; porte EG9 d'égalité | un seul producteur de l'entrée C∩X |
| 25 | `nearest_strict_intruder` | GEN et ARCH : dans l'API du `LeafOracle` | **retiré** | inutilisé en produit ; J-DESC utilise sa propre grille |
| 26 | Représentation des sphères | TOWER § 7 : clés (A, B, C) globales ; GEN : forme ancrée `c = x(a) + N/D` | **forme ancrée** ; égalité de sphères par produit croisé réancré en i192 signé (≤ 2^173) | bornes plus petites ; une seule forme dans `arith/sphere.hpp` |
| 27 | Juges d'Euler | GEN J4, TOWER I2, ARCH J-KM2 | **I2 en produit** (K ≤ kcat − 2) ; **J-KM2** en juge (Euler sur `kcat` + 2 et restriction) ; J4 = étape 3 de J-KM2 | pas de doublon |
| 28 | Portes de coût | GEN COST-A..E ; EVAL `p_travail − p_boules ≤ 0,15` ; ARCH `p_boules ≤ 1,15` ; TOWER rapports ≤ 1,15 | **une règle** : exposant du travail par rapport à la sortie ≤ 1,15 (équivalent à `p_travail − p_boules ≤ 0,15`) ; arbre contre n ≤ 1,15 ; `p_boules` publié comme propriété des données ; LiDAR jugé en absolu (COST-C) | les formulations sont équivalentes sauf ARCH, qui jugeait les données |
| 29 | Quantification LiDAR | ARCH § 5.1 : arrondi au pair, origine capteur − 2^17 ; EVAL D9 : `floor(x/pas + 1/2)`, translation entière commune v8 | **EVAL = règle v8** (`PRECISION_FLOAT32_ET_GRILLE_20260921.md`) | continuité bit à bit avec R22 et L13 (EP6 : sha `a4bbc86d…` de 08/000200) |
| 30 | Trame brute « 00/000000 » de GEN | ARCH : 08/000000 brute, 123 389 sites | **même fichier** (scène v8 « 00 » = 08/000000) ; nommage corrigé en 08/000000 | chemin `scene_00_000000_grid/full.u32le` du reçu v8 |
| 31 | Tête T3 (§ 9.1 sur facettes) | CLUSTER : candidate ; EVAL : source `tf` | **différée** : hors chemin critique, réalisée seulement si V10-3 a de la marge | prévision T3 ≤ T1, coût (dichotomie U320, catalogue `gabriel`) ; fidélité à la thèse seule |
| 32 | Sélection DBCV par scène (V10-α) | absente d'EVAL | **permise** comme variante de tête sans étiquettes, offerte symétriquement à `hdb_dev` | règle de budget symétrique d'EVAL § 4.3 |
| 33 | `max_leaf` | V10-α : 256 ; GEN : `M_hard` = 128 | **128** | F11 ; arène bornée ; refus de la tour au-delà de u = 16 de toute façon |
| 34 | Clé d'offsets du catalogue | V10-α : u64 ; ARCH : u32 avec garde | **u32**, dépassement détecté avant écriture (`index_overflow_u32`) | marge ≥ ×30 au pire cas publié (brute K10 ≈ 92 M identifiants) |

---

## 6. Interfaces exactes

### 6.1 Types (`src/core/ids.hpp`)

```cpp
namespace mhgp10 {
using u8 = std::uint8_t; using u16 = std::uint16_t; using u32 = std::uint32_t; using u64 = std::uint64_t;
using i32 = std::int32_t; using i64 = std::int64_t;
__extension__ typedef __int128 i128; __extension__ typedef unsigned __int128 u128;
enum class PointId   : u32 {};  // identité externe ; seule api/ la lit
enum class SiteIdx   : u32 {};  // rang de Morton 54 bits des positions distinctes
enum class BallIdx   : u32 {};  // rang dans l'ordre canonique (rang de niveau, S*) du catalogue
enum class LevelRank : u32 {};  // rang dense des niveaux exacts ; 0 = niveau nul, catalogue >= 1
enum class CellIdx   : u32 {};  // cellule (boule d'atlas, K)
enum class LeafIdx   : u32 {};  // naissance d'ordre K, dense par K
enum class NodeIdx   : u32 {};  // nœud d'une forêt d'ordre K (numérotation canonique)
using Order = u8;
inline constexpr u32   kNone = 0xFFFFFFFFu;
inline constexpr Order kKmaxPublic = 10;  // API publique
inline constexpr Order kKcatMax = 12;     // catalogue et tour internes (références certifiées)
}
```

Tout index interne est u32 ; un dépassement est détecté avant l'écriture (`resource_exhausted/index_overflow_u32`).

### 6.2 Entrée et `SiteTable` (`cloud/`)

- Domaine : `0 ≤ x < 2^18` par axe ; hors domaine → `invalid_input/coordinate_out_of_domain` ; `PointId` en double →
  `invalid_input/duplicate_point_id`.
- LiDAR (banc de performance, hors bibliothèque) : entrée = retours du `.bin` ; sans sol = retours non classés
  « ground » par Patchwork++ épinglé (recette v8) ; `PointId` = indice du retour ; quantification v8
  `q = floor(x/pas + 1/2)` en rationnels exacts depuis le float32, pas = 1 mm, une translation entière commune ; un
  retour hors domaine refuse la trame (compté).
- Banc synthétique et suites publiques : `q = floor((X − o)/h + 1/2)` en `Fraction`, h = étendue maximale / (2^18 − 1)
  ; scènes générées retirées jusqu'à 0 doublon ; suites publiques avec multiplicités.

```cpp
struct SiteTable {               // ordre de Morton = SiteIdx
  Buffer<u32> x, y, z;           // u18
  Buffer<u32> weight;            // >= 1
  Csr<PointId> ids;              // triés par PointId croissant ; tous conservés
  u64 n_points;                  // W = somme des poids
  bool weighted;                 // un poids >= 2 existe
};
class SiteTree {                 // requêtes en un site ENTIER seulement ; d² < 2^38, exact en i64
 public:
  u64  kth_distance(SiteIdx q, u32 k) const;                  // D_k(q), poids et q compris
  void knn(SiteIdx q, u32 k, std::span<SiteIdx> out) const;   // départage (d², SiteIdx)
  void within(SiteIdx q, u64 r2, Buffer<SiteIdx>& out) const;
};
```

### 6.3 Catalogue, niveaux, `LeafOracle` (`catalogue/`, `arith/`)

```cpp
namespace mhgp10 {
struct Catalogue {                   // SoA, ordre canonique (rang, S*)
  Buffer<LevelRank> rank;            // >= 1 ................................................ 4 o
  Buffer<u32> ids_off;               // CSR dans ids : I trié puis U trié (u32, garde) ........ 4 o
  Buffer<u8>  n_interior;            // |I| en positions (<= kcat - 1) ........................ 1 o
  Buffer<u8>  n_shell;               // |U| en positions (<= 128) ............................. 1 o
  Buffer<u8>  qflags;                // bits 0-1 : q_min - 2 ; bit 2 : étendue ; bit 3 : coquille pondérée ;
                                     // bit 4 : intérieur pondéré ............................. 1 o
  Buffer<SiteIdx> ids;               // ........................................... 4 o par identifiant
  Buffer<BallIdx> level_rep;         // par rang ; level_rep[0] = kNone .................. <= 4 o par boule
  ExtendedSide ext;                  // boules étendues triées : positions de S* dans U (4 x u8)
  Order kcat; CatalogueLedger ledger;  // grand-livre : ≈ 25 compteurs (GEN § 12.8)
};
struct ExactCenter { SiteIdx a; i128 n[3]; i128 d; };   // c = x(a) + n/d, d > 0, non réduit (repère de sites)
struct ExactSphere { ExactCenter c; };                 // x(a) est SUR la sphère : r² = |n|²/d²
struct Level { U192 num; u128 den; };                  // r² non réduit, depuis S* (Gram en q3, Cramer en q4)
Level  level_of_support(const SiteTable&, std::span<const SiteIdx> support);  // 2 à 4 positions
int    compare(const Level&, const Level&);             // produits croisés U320
int    compare(const Level&, u64 d2);                   // num contre den · d2, U192 (entry_pos, entry_eq)
double level_double(const Level&);                      // suite IEEE fixe (§ 5 ligne 19), erreur < 2^-51
bool   sphere_equal(const SiteTable&, const ExactSphere&, const ExactSphere&);  // i192 signé, voie rare

class LeafOracle {                   // nœuds terminaux (énumérés ET ignorés), triés par Morton fin du coin
 public:
  u32  locate(const ExactCenter&) const;                              // feuille demi-ouverte de c (i128)
  void knn_closed(const ExactCenter&, u32 k, KnnOut&) const;          // N_k(c) fermé, k <= kcat ; ordre
                                                                      // (puissance exacte, SiteIdx), copies
                                                                      // consécutives, ex aequo compris
  std::optional<BallIdx> lookup(const ExactSphere&) const;            // boule du catalogue de cette sphère
  std::span<const SiteIdx> list(u32 leaf) const;                      // L(Q), pour les juges
};
Result<std::pair<Catalogue, LeafOracle>> build_catalogue(Session&, const SiteTable&, Order kcat);
}
```

- Un site x se passe comme `ExactCenter{x, {0, 0, 0}, 1}` : les entrées C∩X utilisent le même oracle.
- Octets par boule : 11 fixes + 4 par identifiant + ≤ 4 de table des niveaux ; mesuré 4,63 identifiants par boule à
  K5 et 8,10 à K10 (08/000200), soit 42–47 Mo à K5 et 238–260 Mo à K10.
- `LeafOracle` : Σm ≈ 12 M identifiants (≈ 49 Mo) à K5, ≈ 26 M (≈ 106 Mo) à K10 ; vit de la fin de S3 à la fin de Q.
- Transaction : count → fill → validate → publish ; aucun préfixe publié.

### 6.4 Tour (`tower/`)

```cpp
namespace mhgp10::tower {
struct OrderForest {
  Order K;
  // nœuds en ordre canonique (rang, plus petite feuille du sous-arbre)
  Buffer<LevelRank> rank;          Csr<NodeIdx> parents;      // parents croissants
  Buffer<NodeIdx>   successor;     Buffer<NodeIdx> image;     // image : nœud d'ordre K-1 à la coupe fermée
  Buffer<u32>       leaf_lo, leaf_n;                          // intervalle du sous-arbre dans pi
  // index d'intervalles (déterministe ; hors digest linéaire)
  Buffer<LeafIdx>   pi;            Buffer<NodeIdx> leaf_node;  Buffer<CellIdx> leaf_cell;
  Buffer<LevelRank> jrank;         Buffer<NodeIdx> jnode;      Buffer<LevelRank> jblock;  // blocs de 16
  // marques
  Buffer<NodeIdx> anchor;          // par cellule d'ordre K
  Buffer<Contribution> contrib;    // {segment, rang, cellule, masque de coquille}, rares
  // entrées C∩X par site
  Buffer<NodeIdx> entry_node;      // composante de L_K(D_K(x)) contenant x
  Buffer<u64> entry_level;         // D_K(x) exact (x et poids compris)
  Buffer<LevelRank> entry_pos;     // plus grand rang de niveau <= D_K(x)
  Buffer<u8> entry_eq;             // 1 si D_K(x) = niveau(entry_pos)
};
struct Tower {
  std::shared_ptr<const Catalogue> cat; std::shared_ptr<const SiteTable> sites;
  Atlas atlas; LocalTables local;              // LocalTables porte le drapeau circulaire (§ 5 ligne 16)
  std::array<OrderForest, kKcatMax + 1> orders; Order keff;
  TowerStatus status;                          // sel, euler_checked_up_to, circle_balls, table_balls,
};                                             // weighted_balls, shell_max_3d, jumps_hist, meb_calls, t2_algo
}
```

- `WA(v, s)` : composante de l'ordre K à la coupe `≤ s` contenant v (définie si `rank(v) ≤ s`) ; coupe ouverte à r =
  seuil r − 1 ; ≈ 71–75 ns par requête mesurés.
- Localisateur de facettes : `locate_facet(K, F) → (β(F), cellule terminale, feuille)` et
  `component(K, F, a) = WA(leaf_node[Min(term)], a)` ; représentants exportables sur demande (parade E5).
- `TowerOrderView` (consommé par `points/`) = sous-ensemble en lecture seule : `rank`, `successor`, `leaf_lo`,
  `leaf_n`, `jrank`, `entry_node`, `entry_level`, `entry_pos`, `entry_eq`, `parents` (différentiel G11 seulement),
  plus `level_double(rank)` et `compare(rank, u64)`. Une vue incomplète rend
  `invariant_violated/tower_view_incomplete`.
- Invariants produit : I1 (structure, toutes les boules), I1g (géométrie, 1/64), I2 (Euler, pondéré compris,
  K ≤ min(W, kcat − 2)), I3 (λ strictement décroissant), I3b (K ≤ hi), I4 (pointeur strict), I5 (clé admissible
  présente), I6 (une racine par ordre), I7 (identité de forêt), I8 (naturalité, 1/16), I9, I10.

### 6.5 Points et `tree.bin` (`points/`, `io/`)

```cpp
namespace mhgp10::points {
struct LotKey { u64 v; };  // tour : rang r -> r << 32 ; entrée D -> (pos(D) << 32) | (eq ? 0 : 1 + s) ; ⊥ -> 0
                           // s = rang dense de D parmi les entrées de TOUS les ordres entre deux rangs
struct PointDendrogram {   // N-aire, plateaux atomiques
  Order K; u8 source;      // 0 tw, 1 mr1, 2 mr2, 3 sk1, 4 sk2
  u32 n_atoms, n_nodes;    // atomes 0..n_atoms-1 = sites en ordre de SiteIdx ; nœuds ensuite, ordre des lots
  Buffer<LotKey> key;      // égalité <=> même niveau exact (comparables entre ordres pour tw)
  Buffer<double> radius;   // unités du producteur, brut (0 permis) ; le plancher est appliqué par la tête
  Buffer<u32> parent;      // kNone à la racine
  Csr<u32> children;       // triés par identifiant
  Buffer<u32> mass;        // poids des sites
};
Result<PointDendrogram> from_tower(Session&, const tower::Tower&, Order K);   // graphe-chemin (PG)
}
```

Sérialisation `tree.bin` version 2 (EVAL § 2.4, inchangée) : `magic "MHGP10TR"`, version 2, `n_leaves`, `n_nodes`,
`source`, `K`, `parent[]`, `level_rank[]` (rang dense des clés **dans ce dendrogramme**, commun aux feuilles et aux
internes), `level_radius[]` (f64), `weight[]`, SHA-256 en pied. `r_floor` se déduit de `source` : 1/2 pour `tw`,
1/α pour `mr_α` et `sk_α`. Pour `sk1`/`sk2` (arbres binarisés de sklearn), l'égalité de rang est l'égalité float64,
déclarée. `mr1`/`mr2` sont les mêmes arbres réatomisés (V10-I1).

### 6.6 Tête (`head/`)

```cpp
namespace mhgp10::head {
enum class Selection : u8 { eom, leaf };
enum class Fill : u8 { none, bounded, full };
struct ClusterParams {                 // liste blanche ; hyperparamètres préenregistrés
  u32 min_cluster_size;                // en masse (lignes) ; mcs = round(sqrt(N)) par défaut du banc
  double z;                            // > 0 ; la valeur ẑ est calculée par estimate_intrinsic_dimension
  Selection selection; bool allow_single_cluster;
  double eps_quantile;                 // < 0 : aucun ; sinon quantile des rayons de fusion propres à la source
  Fill fill; double fill_rho;          // b<rho> : core_K(p) <= rho * Q95 du groupe
};
struct Clustering {
  Buffer<i32> site_label;              // -1 bruit ; groupes numérotés par plus petit SiteIdx
  Buffer<double> probability;          // sémantique sklearn
  u32 eom_near_ties, fill_ties, vote_near_ties; double z_used;
};
Result<Clustering> cluster(Session&, const points::PointDendrogram&, const ClusterParams&);
Result<double> estimate_intrinsic_dimension(Session&, const SiteTable&, u32 k = 10);  // arrondi au 1/16
}
```

- Condensation : un nœud u du cluster c se scinde à λ(r_u) ; B = enfants de masse ≥ mcs ; |B| ≥ 2 : chaque enfant de
  B naît ; |B| = 1 : continuation ; |B| = 0 : **c se termine** ; un plateau N-aire est une seule action ; un atome de
  masse ≥ mcs entré à 0 sort à λ(max(r, r_floor)).
- λ : `z ∈ {1, 2, 3}` par suites fixes de multiplications et divisions ; z réel par `det_exp2(−z · det_log2(r))`
  (polynômes maison, ≤ 8 ulp) ; stabilités sommées par Neumaier dans l'ordre de l'arbre.
- Sélection eps : sémantique exacte d'`epsilon_search` et `traverse_upwards` de sklearn (inégalités strictes).
- Les étiquettes par site sont diffusées aux points (ordre d'entrée de l'API) par `api/`, et aux lignes par le lanceur
  du banc (`row_site`).

### 6.7 Statuts, raisons, codes de sortie

```cpp
enum class Status : u8 { ok, invalid_input, unsupported_degeneracy, resource_exhausted, invariant_violated };
```

| Statut | Raisons (table X-macro unique `reasons.def`) |
| --- | --- |
| `invalid_input` (api/ seulement) | `empty_input`, `size_mismatch`, `coordinate_out_of_domain`, `duplicate_point_id`, `kmax_out_of_range`, `k_out_of_range`, `parameter_out_of_range`, `fp_environment` |
| `unsupported_degeneracy` | `shell_quotient_budget` (boule publiée), `duplicate_positions` (transitoire) |
| `resource_exhausted` | `memory_budget`, `scratch_arena`, `session_overhead`, `allocation_failed` (non déterministe, exclu de R1), `index_overflow_u32`, `wide_leaf`, `euler_overflow`, `device_unavailable` |
| `invariant_violated` | `arith_guard`, `census_mismatch`, `canonical_order_duplicate`, `rank_order`, `csr_bounds`, `euler_mismatch`, `missing_key`, `root_count`, `forest_identity`, `descent_not_decreasing`, `window_above`, `pointer_not_strict`, `window_empty`, `vertical_naturality`, `vertical_closed_cut`, `tower_view_incomplete`, `nested_parallelism`, `leaf_unsplittable` |

- Issue d'un échec : étage le plus tôt ; dans l'étage, `memory_budget` d'abord, puis la faute de plus petit (K,
  ordinal), puis la plus petite raison. Rien n'est publié.
- CLI produit : 0 = `ok`, 2 = refus (`invalid_input`, `unsupported_degeneracy`, `resource_exhausted`), 3 =
  `invariant_violated` ; une ligne JSON `{"status", "reason", "stage", "order"}` sur la sortie standard.
- Portes (`run_expect.cmake`, code **exact**, signal = échec) : 0 conforme, 1 désaccord d'un juge, 2 refus attendu,
  3 invariant ou plancher violé, 4 mutant tué (enveloppe de mutant seulement).
- Statut du banc (`run.json`) : le statut produit tel quel (`ok` s'écrit `complete`), plus `timeout`, `crash`,
  `invalid_output` posés par le lanceur. `refused_domain` et `numeric_failure` d'EVAL sont retirés.

### 6.8 Digests (`io/`, hors chronomètre)

| Digest | Contenu | Usage |
| --- | --- | --- |
| `engine_digest` | Merkle SHA-256 des tableaux du moteur (catalogue et tour) en ordre canonique, tranches fixes de 1 Mio hachées en parallèle ; niveaux identifiés par (rang, coordonnées de S\* de `level_rep`) ; sans `PointId` | déterminisme (W, backend, permutation) ; honnêteté du chronomètre (mutant `skip_stage_resolve`) |
| `catalogue_digest` | SHA-256 de la suite canonique : par boule, bit « même niveau que la précédente dans cette suite », q_min, drapeaux, \|I\|, \|U\|, coordonnées de S\*, coordonnées et poids de I puis de U (ordre de Morton) ; ni valeur de niveau, ni numéro de rang, ni `kcat` | J-KM2 (restriction), différentiel v9 |
| `tower_digest_v10` | sérialisation linéaire canonique : niveaux réduits gros-boutistes, parents, successeurs, images, feuilles par population en `PointId` triés, contributions, ancres ; sans π, J, `leaf_lo`, `jblock` | épingles, reçus |
| `tower_merkle_v10` | `merkle_prefix(k0) = H(k0, H(racine_1), …, H(racine_k0))`, sans numérotation, sans Kmax ni `kcat` (PO-E15) | références certifiées, EP1, comparaisons v9 et GPU |
| `id_digest` | listes de `PointId` par site | R1-c |

SHA-256 portable plus voie SHA-NI choisie à l'exécution (sans `#if`) ; vecteurs NIST ; égalité des deux voies sur
10^4 tampons.

### 6.9 Schémas de mesure et de reçu

- `mhgp10_run_v1` (EVAL § 12.4), une ligne JSON par passe : entrée, grand-livre du générateur, catalogue, tour par K,
  sortie, appareil, hôte (`cpu_s` par phase, `CLOCK_THREAD_CPUTIME_ID`), temps B0/B1/B2/B3/B4/G/Q/H/C, exactitude
  (`merkle_prefix[k]`, `digest_ref`, `equal`) ; ajouts additifs d'ARCH (budget logique, F(W), statut, digests, juges,
  calibration `rho`, `s24`, `s48`, `c1_us_per_ball`). Au plus 3 versions sur la vie de la v10.
- `mhgp10_receipt_v1` (ARCH annexe C) : commit, arbre propre, recette, binaires et sha256, hôte, entrées par sha256,
  commandes, labels exécutés, statut `public_status: "not_claimed"`, bloc `gcp` (session, génération, `TERMINATED`
  certifié). ≤ 2 Mo, aucun octet de nuage, jamais sous `/tmp`.

### 6.10 API publique, CLI, liaison Python

```cpp
namespace mhgp10 {
enum class Backend : u8 { cpu, cuda };
struct SessionConfig { unsigned threads = 0; Backend backend = Backend::cpu; u64 data_budget_bytes = 0; };
class Session { public: static Result<Session> open(const SessionConfig&, Ledger* = nullptr); };  // publie F(W)
struct InputView { std::span<const u32> xyz; std::span<const u32> ids; };
struct TowerParams { Order kmax; };                                                  // 1..10
Result<tower::Tower>           build_tower(Session&, InputView, TowerParams, Ledger* = nullptr);
Result<points::PointDendrogram> point_hierarchy(Session&, const tower::Tower&, Order K);
Result<points::PointDendrogram> read_tree_bin(std::span<const std::byte>);           // sources mr, sk du banc
Result<double>                 estimate_intrinsic_dimension(Session&, InputView, u32 k = 10);
Result<head::Clustering>       cluster(Session&, const points::PointDendrogram&, const head::ClusterParams&);
}
```

- `mreach_hierarchy` d'ARCH est **supprimée** (D-V10-17).
- CLI unique `mhgp10` : `tower` (`--in`, `--ids`, `--kmax`, `--threads`, `--backend`, `--data-budget`, `--out`),
  `cluster` (`--in` avec `--K`, ou `--tree tree.bin` ; `--mcs`, `--z`, `--selection`, `--eps-quantile`, `--asc`,
  `--fill none|b<ρ>|full`, `--out`), `digest`. Les trois exécutables de V10-α sont fusionnés.
- ABI C (≈ 12 fonctions, compter puis remplir) et `python/mhgp10` en ctypes + numpy ; pas de pybind11.

### 6.11 Responsabilités

| Quantité | Calculée par | Lue par | Jamais calculée par |
| --- | --- | --- | --- |
| sites, poids, `SiteIdx`, CSR des `PointId` | `cloud` (S0) | tous | — |
| boules critiques (I, U, S\*, q_min, drapeaux), rangs, `level_rep` | `catalogue` (S1–S3) | `tower` | `tower`, `points`, `head` (aucun second recensement) |
| niveau exact, `level_double`, comparaisons | `arith/level.hpp` | `catalogue`, `tower`, `points` | ailleurs |
| requêtes à centre rationnel | `catalogue/leaf_oracle` | `tower` | `cloud`, `points`, `head` |
| MEB exacte, saut K-NN, `resolve` | `tower` (étage G) | — | `points`, `head` |
| fenêtres, quotients locaux, drapeau circulaire, pw/uw saturés, Euler I2 | `tower` (étage P) | `tower` | `catalogue` |
| forêts, ancres, contributions, verticales, (π, J) | `tower` (T1–Q) | `points`, `api`, `io` | — |
| `D_K(x)`, `entry_node/level/pos/eq` | `tower` (étage Q) | `points` | `head` ; `SiteTree` seulement en juge |
| dendrogramme de points `tw` | `points/from_tower` | `head` | — |
| dendrogrammes `mr_α`, `sk_α` | banc Python (sklearn `kd_tree` + réatomisation) → `tree.bin` | `head` (CLI) | C++ produit |
| ẑ | `points/zhat` (C++), une fois par scène sur les sites pondérés | `head`, banc (commun à toutes les sources) | Python |
| condensation, sélection, remplissage, étiquettes | `head` | `api`, banc | Python (les références Python sont des juges) |
| ARI_s, AMI_s, F1_H, DP, BNF1, statistiques, décision | `eval/` (Python) | reçus | C++ produit |
| digests | `io` | portes, reçus | chronomètre |
| statut agrégé | `api` | CLI, banc | — |

---

## 7. Arithmétique consolidée (profil u18, B = 18, T = 6)

Bornes dans le repère **local** de GEN (plus petites que le repère global d'ARCH, qui reste la borne de sûreté des
`static_assert`). Chaque ligne est un `static_assert` paramétré par (B, T) dans `arith/widths.hpp`, et un test contre
BigInt aux coins u18 (0 et 262 143), dont la collision (0,1,0)/(0,0,65536).

| Quantité | Repère | Borne | Type |
| --- | --- | --- | --- |
| dominance, bissectrice, sélection de Y, k-DOP | boîte (2^−6 mm) | < 2^51,6 | i64 (B + T ≤ 29) |
| lemme Z (droite q4 contre boîte) | boîte | 72E'³ | i64 si E' ≤ 2^18 (97,5 % des feuilles LiDAR), sinon i128 |
| test de droite par paramètres | — | 2^133,1 | **interdit** |
| centre q3 (Gram) N / D | sites | < 2^95,2 / < 2^76,6 | i128 |
| centre q4 (Cramer) N / D | sites | < 2^76,2 / < 2^57,6 | i128 |
| centre ∈ boîte (pont) | pont | < 2^101,2 | i128 |
| intérieur strict du tétraèdre | sites | < 2^115,5 | i128 |
| recensement q3 / q4 | sites | 288E^6 < 2^116,2 / < 2^97,2 | i128 (exact jusqu'à B = 19) |
| q_min et S\* d'une coquille étendue, Gordan à centre q3 (voie rare) | sites | < 2^134,3 | U256 signé |
| orientation circulaire `det2(v_j, v_k)` | sites homogènes | < 2^196 | i256 (filtre double prouvé, repli exact) |
| coplanarité au centre `det3` (entrée non plane) | sites homogènes | < 2^293 | i320 signé |
| niveau r² q3 / q4 | sites | num < 2^153,9 ; dén < 2^115,2 | U192 / u128 |
| comparaison de niveaux | — | < 2^269,1 | U320 |
| niveau contre entier `D_K(x)` | — | < 2^160 | U192 |
| égalité de sphères réancrée | sites | < 2^173 | i192 signé |
| `D_K(x)`, d² entre sites | sites | < 2^38 | u64 |
| poids MR `α² D_K` et d² | sites | < 2^40 | u64 |
| Euler, forme close circulaire / table 3D | — | \|coef\| ≤ u + 1 / non borné finement | i64 / i128 avec `__builtin_*_overflow` (`resource_exhausted/euler_overflow`) |
| clé de tri des niveaux | flottant | erreur relative < 2^−51, bande 2^−49 | double puis U320 |

- Voie large i256 pour B ≥ 20 seulement (aucun cas en u18), jugée contre `cpp_int` si Boost est présent.
- `Session::open` exige `FE_TONEAREST` ; `-ffast-math` refusé au configure ; `__FAST_MATH__` → `#error`.
- Sur l'appareil : `nvcc --fmad=false` pour toute unité de filtre (P14) ; entier pur ailleurs.

---

## 8. Parallélisme, déterminisme, mémoire, GPU

### 8.1 Règles (ARCH § 7.2)

- **R1** : toute sortie indexée par un identifiant interne et tout digest sont invariants par permutation de l'entrée,
  par W et par backend ; les tableaux par point sont équivariants ; un renommage des `PointId` ne change que
  `id_digest` et les sorties indexées par `PointId` ; l'issue suit P19.
- **R2** : écritures en cases indexées par ordinal déterministe, concaténation par préfixes (GEN : ordre de Morton des
  tâches ; TOWER : ordre des cellules). **R3** : aucune réduction flottante dépendant de l'ordonnancement. **R4** :
  hachage pour l'appartenance seulement. **R5** : départages par `SiteIdx`, jamais par `PointId`. **R6** : pas de
  parallélisme imbriqué. **R7** : aucune réservation logique en région parallèle hors du `ChunkPool`.

### 8.2 Par étage

| Étage | Grain | Point séquentiel | Mesure de mise à l'échelle |
| --- | --- | --- | --- |
| S1 arbre | phase en largeur jusqu'à 64W nœuds ou tous terminaux, puis une tâche par nœud, par taille décroissante | aucun (le découpage fixe du prototype dégénère avec la racine fixe) | P-SCALE W1 → W24 → W48 |
| S2 feuilles | une tâche par feuille, arène par ouvrier (1 Mio) | aucun | idem |
| S3 rangs | tri par échantillonnage, bandes réparées en parallèle | balayage des bandes | ≈ 20–40 ms à 5,5 M boules (estimé) |
| S4–S7 | 256 cellules (P, G), 4 096 (M, T1) ; G émis par K décroissant | aucun | — |
| S8 T2a | une tâche par ordre ; l'ordre Kmax démarre en premier et se recouvre avec G des ordres inférieurs | **noyau** : 6,6–9,2 ms (K5), 15,9–22,3 ms (K10) en local, arêtes semées | visible ≈ 0–1 ms sur CPU ; ≈ 4,5–12 ms (K5) sur GPU |
| S9–S10 | 16 384 éléments ; classement de liste par règles tous les 64 ; potentiels en repli | préfixe sur L/64 | décision D-T5 sur G4 (T-0) |
| S11–S12 | une tâche par ordre ; max segmenté par tranches à couture fixe | balayage par ordre | ≈ +1 ms par ordre |

Porte transversale : `engine_digest` identique pour W ∈ {1, 2, 8} en local et 48 sur G4, sous permutation de l'entrée ;
statut identique à W1, W2 et W8 sous budget serré (P19).

### 8.3 Mémoire (normatif, ARCH § 5.6)

| Poste | K5 sans sol (1,31–1,41 M boules) | K10 sans sol (5,5 M) | K5 brute (2,82 M) | K10 brute (11,4 M) |
| --- | ---: | ---: | ---: | ---: |
| catalogue | 34 o — 45 Mo | 48 o — 265 Mo | 34 o — 96 Mo | 48 o — 550 Mo |
| `LeafOracle` | 44 o — 58 Mo | 21 o — 115 Mo | ≈ 27 o — 75 Mo | ≈ 20 o — 230 Mo |
| tour, pic | ≤ 100 o — 131 Mo | ≤ 100 o — 551 Mo | ≤ 100 o — 282 Mo | ≤ 100 o — 1,14 Go |
| **RSS budgété** (porte) | **≤ 0,4 Go** | **≤ 1,2 Go** | **≤ 0,7 Go** | **≤ 2,5 Go** |
| v9 mesuré / V10-α (sans reçu) | — | 4,5–6,4 Go / 2,4 Go | — | ≈ 10 Go |

Porte d'honnêteté : pic logique compté ≥ 85 % du pic RSS au-delà du socle et de F(W) (F(48) ≈ 60 Mo).

### 8.4 Voie GPU (RTX PRO 6000, sm_120)

- **V10-G, catalogue** : noyau de dominance (i64, décalages, sans branchement) en BFS par niveau, un warp par nœud si
  la liste parente a ≤ 1 024 sites ; feuilles à un warp par feuille (m ≤ 32, masques u32, H en mémoire partagée,
  4 Ko par warp) ; feuilles m > 32 rendues au CPU et comptées ; tri canonique sur l'appareil. Coût des multiplications
  64 et 128 bits émulées par IMAD compté.
- **V10-4b, tour** : G (file de représentants à fils persistants, H_K trié, MEB proposée en double puis vérifiée en
  entier, `LeafOracle` résident), M, T1, T2b, T2c, Q sur l'appareil ; **T2a sur l'hôte** (arêtes 12 o, ≈ 22 Mo à K10,
  ≈ 1–2 ms PCIe 5 ; listes NJ remontées) ; les ordres inférieurs continuent sur l'appareil pendant le noyau de Kmax.
- Processus **résident** obligatoire pour tout contrat sous 1 s (contexte CUDA 121–166 ms).
- Porte : CPU = GPU au `tower_merkle_v10` sur toutes les trames et à 8k/16k/32k ; un étage ne migre que s'il dépasse
  30 % du profil B3 du bras `cpu48` ; gain mesuré en A/B sur B2, jamais sur un noyau isolé.

---

## 9. Coûts attendus

Conventions : « mesuré » cite un reçu ou une sonde de conception ; « estimé » est un modèle ; « projection » GPU n'a
aucune valeur avant reçu G4 (écarts ×10 en v9). Scénarios G4 : ρ (coût par fil G4 / codespace) ∈ [0,5 ; 1] et
S48 ∈ [24 ; 34], mesurés par la session C0.

### 9.1 LiDAR (priorité)

Volumes mesurés, 08/000200 sans sol, 45 845 sites : 1 407 885 boules à K5, 5 483 320 à K10. Pire trame du contrat,
60 k sites : ≈ 2,0 M et ≈ 8,3 M boules (EVAL § 12.6).

| Poste, G4 W48 | K5, 08/000200 | K10, 08/000200 | Source |
| --- | ---: | ---: | --- |
| catalogue, CPU | 0,15–0,25 s | 0,5–0,8 s | GEN § 11.9 (sonde, ± 30 %) |
| tour, CPU (optimiste / pessimiste) | 33–53 / 70–110 ms | 0,19–0,28 / 0,40–0,58 s | TOWER § 12.2 (estimé) |
| tête (tous ordres) | 3–5 ms | 4–8 ms | CLUSTER § 11.1 (estimé) |
| **B2 prévu, CPU** | **0,19–0,37 s** | **0,7–1,4 s** | somme |
| B2 prévu, pire trame 60 k (×1,42 ; ×1,51) | 0,27–0,53 s | 1,05–2,1 s | extrapolation linéaire en boules |
| projection GPU (catalogue + tour + fixe) | 45–100 ms ; pire trame 64–140 ms | 0,1–0,2 s ; pire trame 0,15–0,30 s | GEN § 10.2, TOWER § 14 |
| référence v9 R22 (chaîne GPU ; mur processus) | 0,76–0,98 s ; 1,5–1,9 s | 2,27–2,96 s ; 4,8–6,0 s | reçu R22 |
| V10-α en local, 4 fils (sans reçu) | catalogue 3–4 s + tour 4–6 s | 11–15 s + 32–56 s | PASSATION `5bf66793e` |

**Budget EP5 par boule** (tout compris, 24 cœurs physiques, 1 s, pire trame) : ≤ 12,1 µs CPU à K5 [15,2 avec 25 % de
gain SMT] et ≤ 2,9 µs à K10 [3,6]. Constantes de conception : catalogue 3,7–5,8 µs + tour 1,2–1,9 µs (K5) ou
1,7–2,6 µs (K10), soit **4,9–7,7 µs à K5 (GO prévu, marge ×1,6 à ×3)** et **7,0–8,4 µs à K10 (NO-GO prévu en CPU,
écart ×2 à ×3)**. D'où D-K5 (§ 12.2) : C(K5, 1 s) poursuivi en CPU ; V10-G ouverte pour K10 dès la sortie de V10-1,
sauf surprise mesurée.

**100 ms à K5** (budget à tenir simultanément, ARCH § 6.8) : catalogue ≤ 40 ms (≥ 35 M boules/s), étage G ≤ 20 ms,
T2 ≤ 15 ms (T2a visible 4,5–12 ms sur GPU), Q et reste ≤ 25 ms, processus résident. Cible de recherche publiée comme
décomposition mesurée. **100 ms à K10** : 5,5 à 8,3 M boules à produire et à consommer, soit ≤ 12 ns de mur par
boule ; aucun algorithme connu.

### 9.2 Tailles d'intérêt 8k / 16k / 32k (G4 W48, CPU ; compteurs en local, temps sur G4)

Catalogue projeté par la formule de GEN § 11.9 : `boules × (3,7–5,8 µs) / 48 × 1,15 + 25–45 ms` ; tour : estimations
pessimistes de TOWER § 12.2.

| Entrée | Boules | Catalogue | Tour (pessimiste) | Source |
| --- | ---: | ---: | ---: | --- |
| uniforme K5 8k / 16k / 32k | 0,59 / 1,23 / 2,53 M | ≈ 0,08–0,13 / 0,13–0,22 / 0,25–0,40 s | 30–42 / 62–87 / 125–180 ms | GEN § 11.3, TOWER § 12.2 |
| uniforme K10 8k / 32k | 3,09 / 13,49 M | ≈ 0,30–0,47 / 1,2–1,9 s | 0,26–0,32 / 1,1–1,4 s | idem |
| huit amas K5 8k / 16k / 32k | 0,51 / 1,10 / 2,31 M | exposant de travail total 0,999 | — | GEN § 11.3 |
| coquilles K5 8k / 16k / 32k | 0,18 / 0,35 / 0,70 M | 1,2 / 2,3 / 4,6 s CPU en sonde locale (v9 : 57,5 / 179,9 / 1 438,5 s pour q3/q4 seul) | — | GEN § 11.3 |
| banc de clustering (K ≤ 3, 32k, ≈ 0,8 M boules) | — | ≈ 3–5 CPU·s, soit ≈ 0,10–0,16 s | ≈ 25–35 ms | CLUSTER § 11.3 |
| grilles entières 20³ à 32³ (dégénérescence massive) | 0,23–1,02 M (K5) | 14,5–14,7 µs par boule (3 à 5 fois le LiDAR) | — | GEN § 11.4 |

Portes de coût : exposant du travail par rapport à la sortie ≤ 1,15 sur les familles à doublement pur (mesuré ≤ 1,126
pour le générateur) ; LiDAR emboîté 8k ⊂ 16k ⊂ 32k : exposant total ≤ 1,15, exposant de l'arbre publié (1,21 sur s02,
alarme non bloquante au-delà de 1,30) ; LiDAR en absolu (COST-C : ≤ 1,1 × les références de 08/000200).

### 9.3 Campagnes

- Banc de test (EVAL § 11.1, Kmax_run = 10) : ≈ 155 h CPU au modèle d'EVAL (10 µs par boule) ; ≈ 110–130 h avec les
  constantes de conception ; 5 à 6 h de mur sur G4 en CPU, deux sessions gardées de ≤ 4 h ; la phase E-c0 (coûts
  sans étiquettes) peut réduire symétriquement Kmax_run ou les répétitions avant le choix de P.
- Passe `dev` complète : ≈ 30 h CPU ; arbres mis en cache par (commit moteur, scène, K, source) hors Git.
- Références certifiées LiDAR : ≈ 30 s par (trame, K), ≈ 30 min pour 30 trames × 2 K ; juge des clés absentes
  ≈ 10 min ; différentiel v9 (session P9) ≈ 30 min plus le build.

### 9.4 Écart entre V10-α et la conception

| Composant | Conception | V10-α (local, 4 fils, sans reçu) | Écart | Action |
| --- | --- | --- | --- | --- |
| catalogue K5 | 3,7–5,8 µs CPU par boule | ≈ 8,5–11 µs | ×1,5–3 | arbre ingénierié, tampon d'insertion, feuille par masques (V10-1) |
| tour K5 | 1,2–1,9 µs | ≈ 11–17 µs | ×6–14 | réécriture selon TOWER_v2 (V10-2) |
| tour K10 | 1,7–2,6 µs | ≈ 23–41 µs | ×9–24 | idem |
| RSS K10 | ≤ 1,2 Go | 2,4 Go | ×2 | `Buffer` comptés, SoA, libération par ordre |

---

## 10. Obligations de preuve (section v10 du registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`)

À inscrire **avant** le code qui en dépend. Statuts du registre : `theorem_external`, `proved_here`,
`conditional_theorem`, `proof_obligation`, `experimental_target`, `measured_negative`, `false_in_general`, `open`.

| Registre | Énoncé | Statut visé | Source | Porte ou fixture | Avant |
| --- | --- | --- | --- | --- | --- |
| V10-R01 | composantes de Γ_K = π0(L_K(a)) | theorem_external (Th. 2) | ARCH annexe D | T2 | V10-0 |
| V10-R02 | lemmes D, D-loc, corollaire G ⊂ D, proposition L | proved_here | V10-GEN-1 | O1/O2, M1, M2 | V10-1 |
| V10-R03 | théorème C (recensement local exact), corollaire A | proved_here | V10-GEN-2 | O1/O2, F4, F12 | V10-1 |
| V10-R04 | feuilles demi-ouvertes, identité par S\*, émission unique ; ordre (rang, S\*) injectif | proved_here | V10-GEN-3, P16 | M3, M4, F9, J6 | V10-1 |
| V10-R05 | bissectrice = non-dominance mutuelle ; lemmes Z (forme fermée), M, K (suffisant), S | proved_here | V10-GEN-4, P3a | M6–M8, M10 ; 10^6 cas contre BigRat | V10-1 |
| V10-R06 | lemme O (oracle K-NN) | proved_here | V10-GEN-5, P18 | J3 | V10-1 |
| V10-R07 | lemme W (admission positionnelle sûre avec poids) | proved_here | V10-GEN-6 | F8a, F8b, F8c, M14 | V10-1 |
| V10-R08 | lemme U (la coquille détermine la boule critique) | proved_here | V10-GEN-7 | M4, M15 | V10-1 |
| V10-R09 | « la fenêtre pondérée de v1 est un intervalle » | false_in_general | V10-GEN-10 | F8a {1, 4}, F8b {2, 4, 5} | V10-1 |
| V10-R10 | suffisance du catalogue (P2) ; transfert de restriction `restrict(cat_{k'}, adm_k) = cat_k` (P2b) | proved_here | ARCH P2, P2b | J-KM2 ; mutant `dominator_threshold_k_minus_1` | V10-1 |
| V10-R11 | multiensembles : Th. 2 sur copies, fenêtre basse `p_w + q − 1` en positions, lots pondérés, continuation a×2 + b | proof_obligation → proved_here | PO-T16, P2w, P5w, P7w | T2 pondérée (≥ 200 sites de poids ≥ 2), fixtures pondérées | V10-2 |
| V10-R12 | table des largeurs, voie U256, élagage exact d'un pavé (P3c) | proved_here + `static_assert` | P3, P3b, P3c | selftests, `lattice_prune_check` | V10-0 |
| V10-R13 | aucun prédicat flottant dans le générateur ; si O-F1 : borne a priori écrite avant le code | proved_here ; O-F1 : proof_obligation | P4, GEN O-F1 | mutant de signe du lemme Z | V10-1 |
| V10-R14 | PO-T1 à PO-T3, PO-T5, PO-T6, PO-T8 (équivalence v9 des ancres), PO-T13, PO-T15 | proved_here | TOWER § 9 | T2, fixtures | V10-2 |
| V10-R15 | PO-T4 : chaque saut K-NN fait décroître λ strictement ; refus seulement si une sphère admissible manque | proved_here (corrigé) | TOWER § 9.1 | I3 ; fixtures de descente à 3–4 sauts ; mutant `knn_from_support_site` | V10-2 |
| V10-R16 | verticales et naturalité (PO-T9) ; quotient local par tables (PO-T12) | conditional_theorem → proved_here | TOWER | `inert_ball`, carré, triangle rectangle | V10-2 |
| V10-R17 | règle cartésienne sur (π, J) (PO-T18) ; WA par intervalles (PO-T10) | proved_here ; **remplace** la ligne « contraction des plateaux » (`proof_obligation`) | TOWER § 9.2 | carré K2 (4 parents), E5 ; mutant `mat_no_plateau_grouping` | V10-2 |
| V10-R18 | Euler par le nerf, sites distincts (PO-T17) ; **Euler pondéré** (PO-T19) | proved_here ; lève P17, O-W2, V10-GEN-8 | TOWER § 9.3 | I2 ; 340 nuages ; mutant `euler_shell_ignores_weight` | V10-2 |
| V10-R19 | coquilles circulaires : quotient par fenêtres et Euler clos (PO-T20) ; classes pondérées en O(u·2^u) (PO-T21) | proved_here | TOWER § 9.4–9.5 | `plane_lattice_*`, 150 nuages pondérés | V10-2 |
| V10-R20 | option D&C (PO-T22) ; classement de liste = potentiels (PO-T23) ; numérotation canonique indépendante de l'algorithme (PO-T24) | proved_here | TOWER | variantes `dc1`, `dc2`, `pot` | V10-2 |
| V10-R21 | déterminisme de l'issue sous budget logique (P19) ; transfert du digest (P20) | proved_here | ARCH | porte W1/W2/W8 à budget serré ; `skip_stage_resolve` | V10-0 |
| V10-R22 | C∩X : C1 (rattachement par N_K(x) multiensemble), C2, C3 (emboîtements) | proved_here | CLUSTER OP1–OP2, PO-T11 | G3 | V10-3 |
| V10-R23 | théorème PG (graphe-chemin) ; balayage exact et égal (OP7b) | proved_here | CLUSTER OP7 | G3, G11 | V10-3 |
| V10-R24 | entrelacements : C5 (α = 1), C5bis (α = 2), `m2/2 ≤ t ≤ 2·m2` et `m1/2 ≤ t ≤ (3/2)·m1` ; identité tour = MR_2 à K = 1 (L1, PO-E5) ; MR_1 à K = 2 = liaison simple (L2) | proved_here | CLUSTER OP3, OP3b, OP4 ; EVAL PO-E5, PO-E6 | G1, G3, EG8 | V10-3 |
| V10-R25 | condensation N-aire ≡ ascendante de HGP-old (OP5) ; plateaux sûrs ⇒ égalité avec toute binarisation (OP5b) | proved_here | CLUSTER | G5, G7 | V10-3 |
| V10-R26 | « les étiquettes de sklearn HDBSCAN sont équivariantes par permutation » | false_in_general | CLUSTER OP5c | `line5` | V10-3 |
| V10-R27 | composition des dendrogrammes P_k exacte (OP8) ; transport des unions-chemins entre ordres | proved_here ; false_in_general | CLUSTER OP8, OP8b | `phantom_transfer_n5` | V10-3 |
| V10-R28 | repli par les seules cofaces de Gabriel préserve C∩X ; raccourci de première couverture | false_in_general | CLUSTER OP11, OP12 | `cx_fold_k2_n6` (190/7 contre 55/2), `cx_firstcov_k3_n6` | V10-3 |
| V10-R29 | **V10-I1 (nouveau)** : sur une entrée u18, les poids float64 de l'arbre couvrant de sklearn 1.9.1 (`kd_tree`, α ∈ {1, 2}) valent `RN(sqrt(W))` avec `W = max(D_K(x), D_K(y), d²/α²) ∈ (1/4)·ℕ`, `W < 2^38` ; `RN∘sqrt` y est injective (écart des racines ≥ 2^−22 > ulp(2^19) = 2^−33) et monotone, donc tout arbre couvrant minimal flottant est un arbre couvrant minimal exact et le regroupement des poids égaux donne le dendrogramme atomisé exact | proof_obligation → proved_here | ce document (D-V10-17) | porte EG-ATOM | V10-3 |
| V10-R30 | plancher λ(0) = λ(r_floor) | convention déclarée | CLUSTER OP15 | `heavy_atom` | V10-3 |
| V10-R31 | EOM intégrée sur K | measured_negative | CLUSTER OP14 | reçu `h2_proxy` | V10-3 |
| V10-R32 | T2-MR = γ-linkage publiée | open | CLUSTER OP13 | — | — |
| V10-R33 | métriques et statistique : ARI_s en O(n), DP par petite-dans-grande, core_K commun (lignes et sites pondérés), quantification ≤ h/2, juge des clés absentes, McCarthy–Snowden, objet d'ordre k indépendant de Kmax et `kcat` | proved_here | EVAL PO-E1, E2, E4, E11, E15–E18 | EG1, EG2, EG9, EG6, EP3, EG3, EP1 | V10-3 |
| V10-R34 | retournement de signe stratifié valide sous moyenne nulle hétéroscédastique ; IUT sans correction | argument + simulation ; theorem_external (Berger 1982) | EVAL PO-E7, PO-E14 | EG3, EG4 | V10-3 |
| V10-R35 | sémantique d'α dans sklearn 1.9.1 (`brute` = changement d'échelle ; `kd_tree` = paire seule) | lecture de source épinglée + fixture | EVAL PO-E13 | EG7c | V10-3 |
| V10-R36 | filtres sur l'appareil certifiés sous `--fmad=false`, ou entier pur | proof_obligation → proved_here | ARCH P14 | porte `device` | V10-G |
| V10-R37 | coût linéaire en la sortie (générateur et tour) sur familles adverses et LiDAR | experimental_target | V10-GEN-9 | COST-A à E, portes de la tour | V10-1 |
| V10-R38 | C(K5, 1 s) sur G4 en CPU ; C(K10, 1 s) sur G4 ; C(K5, 100 ms) | experimental_target | EVAL § 12.6 | EP5, contrats | V10-4 |
| V10-R39 | la tête sur la tour bat la même tête sur MR α = 2 (ATT) ; la v10 bat HDBSCAN (R1) | experimental_target | EVAL § 6.3 | reçus E1, V10-5 | V10-5 |

**Réouverture formelle** de la piste v3 « cellules de centres » (`morsehgp3D_v3/audits/PISTES_FERMEES.md`) : fermée
pour raisons d'ingénierie, pas par contre-exemple ; rouverte par V10-R02 à R08, fixtures et une porte de coût
distincte. La note est écrite avant V10-1.

---

## 11. Portes minimales

Toutes à code exact (`mhgp10_gate … CODE <0..4>`), planchers `--min-*` contre le vert par vacuité, labels `fast`,
`gate`, `oracle`, `judge`, `mutant`, `scale8000`, `scale16000`, `scale32000`, `lidar`, `device`. Tout `ctest` d'un
plan de session porte `--no-tests=error`.

### 11.1 Fondations (V10-0)

| Porte | Contenu | Plancher |
| --- | --- | --- |
| F-ARITH | entiers larges contre BigInt ; lemme Z et P3c contre BigRat ou force brute ; coins u18 ; refus de 262 144 | 10^6 opérations ; 10^6 couples (triplet, boîte) |
| F-SCHED | sorties bit-identiques W ∈ {1, 2, 8} ; P19 (budget serré, statut identique) | 3 entrées |
| F-SHA | vecteurs NIST ; SHA-NI = portable ; Merkle indépendant de W | 10^4 tampons |
| F-G4 | `v10_selftest.py` (faux gcloud), SIGKILL à six moments ; bloquants de la revue corrigés : pas d'arrêt sur une génération non démarrée (blocage 71), aucun `log()` avant l'arrêt, `--no-tests=error` | 6 moments |
| F-CI | trois exécutions mesurées du job ; `check_v10.py`, `check_docs.py` | budgets gelés + 30 % |

### 11.2 Catalogue (V10-1)

| Porte | Contenu | Plancher |
| --- | --- | --- |
| C-ORACLE | produit = O1 (Python/Fraction) = O2 (C++/`cpp_int`) enregistrement par enregistrement | ≥ 1 000 nuages, dont 300 à coquilles étendues et 300 pondérés ; K jusqu'à 12 |
| C-PINS | 12 épingles L13/v9 ; `catalogue_digest` égal à l'exporteur v9 sur uniforme, terrain, huit amas 8k et les 3 trames de conception K5/K10 | 100 % |
| C-JKM2 | Euler sur `kcat` + 2 (pondéré compris) et restriction canonique égale au produit | 8k/16k/32k K5 (6 familles), 8k K10, 3 trames + brute K5/K10 |
| C-BOXES | juge des boîtes : énumération brute dans `X ∩ B(q0, d_K(q0) + diam Q)`, strates (feuilles, ignorés, 1 % de plus grand rayon, vides, coquilles étendues) | ≥ 200 boîtes par entrée, ≥ 30 par strate |
| C-COST | COST-A à COST-E (§ 9.2) | 12 familles, LiDAR emboîté, 3 trames |
| C-DET | P-THREADS, P-PERM (mutant M16) | W ∈ {1, 2, 8, 48} |
| C-MUT | M1 à M16 tués par la porte nommée | 16/16 |

### 11.3 Tour (V10-2)

| Porte | Contenu | Plancher |
| --- | --- | --- |
| T-T2 | T2 Γ_K multi-K dégénérée, coupes ouvertes et fermées, verticales, C∩X | ≥ 5 000 nuages n ≤ 14 (CI : 200, n ≤ 9) ; ≥ 10^6 contrôles ; ≥ 500 coquilles non régulières (≥ 100 circulaires à u ≥ 5, ≥ 100 par table 3D) ; ≥ 200 multifusions à ≥ 3 parents ; ≥ 200 sites de poids ≥ 2 ; ≥ 300 nuages plans ; ≥ 1 000 K-parties à ≥ 3 sauts, ≥ 50 à ≥ 4 |
| T-FIX | fixtures de TOWER § 13.2 avec `tower_digest_v10` épinglé ; catalogues amputés (refus attendu, ou égalité comptée, ou fixture de zone aveugle) | toutes |
| T-EQ | liste canonique des nœuds = Kruskal par lots de référence (`tests/`) ; `dc1`, `pot`, W neutres | toutes les entrées d'échelle |
| T-JUDGES | EMST (K = 1) ; Euler `kcat` + 2 ; J-CENSUS ; J-DESC ; J-MF ; cohérence points–verticales (tous les sites) ; BFS local | J-CENSUS ≥ 20 000 boules par (entrée, K) ; J-DESC trames K5 ≥ 2 000 / 500 / 200 à ≥ 1 / 2 / 3 sauts, K10 + ≥ 20 à ≥ 5 ; J-MF ≥ 1 000 fusions par ordre ; BFS ≥ 1 000 graines, ≥ 60 % terminées |
| T-V9 | différentiel v9 (`tower_merkle_v10`), tour seule puis chaîne | uniforme, terrain, huit amas 8k/16k/32k K5, 8k/16k K10, trames de conception |
| T-BUDGET | pic ≤ 100 o par boule ; compté ≥ 85 % du RSS | 8k/16k/32k, trames |
| T-MUT | 22 mutants de la tour + 2 de recensement | 24/24 |
| T-COST | compteurs par boule (`meb_calls`, `jumps_hist`, `hk_hits`, `pointer_rounds`, `edges`, `wa_queries`) ≤ 1,15 par doublement ; µs CPU par boule publiés sur 08/000200 | 3 familles × 3 tailles |

### 11.4 Points, tête, banc (V10-3)

| Porte | Contenu | Plancher |
| --- | --- | --- |
| H-G0 | validateurs O(n + C), niveaux strictement croissants, I-H1 à I-H3 | `--min-atoms`, `--min-clusters`, `--min-multifusions` |
| H-G1 / EG8 | K = 1 : `tw` = `mr2` (structure, rangs, poids) ; étiquettes = sklearn(ms = 1, α = 2, `kd_tree`) sur les scènes à plateaux sûrs | ≥ 10 scènes sans plateau ; n ∈ {500, 2 000, 8 000} sur toute l'étendue 2^18 |
| H-G3 | graphe-chemin C++ sur la tour C++ = oracle C∩X Fraction à chaque niveau critique ; C1, C3, C5, C5bis à l'union des niveaux | 400 nuages, n ≤ 10, K ≤ 4 |
| H-G5 | condensation dorée : `cond_toy8`, `plateau_3tri`, `parent_split`, `line5`, `heavy_atom` | toutes |
| H-G4 / EG10 | une racine par K ; fixtures E5, L11, `cx_fold_*` sur tout consommateur | 100 % des scènes |
| H-G9 / EG14 | équivariance (permutation, renommage) et W ∈ {1, 2, 7, 16} | 50 scènes × 3 permutations |
| H-G11 | graphe-chemin = balayage ; max segmenté = requêtes `jblock` | 8k/16k/32k, trames, K5 et K10 |
| **EG-ATOM** (nouvelle) | pour chaque arête de l'arbre sklearn, `α²·w²` arrondi égale le niveau entier exact recalculé ; regroupement = dendrogramme atomisé | 20 scènes, K ∈ {1, 2, 5, 10}, α ∈ {1, 2} |
| EG1–EG6, EG7/EG7c, EG9, EG11–EG13, EG16, EG18, EG-F, EG-L | métriques, statistique, décision, graines, quantification, rejeu sklearn et `kd_tree` imposé, core_K commun, survivants, symétrie des paramètres, rejeu des argmax, absence de fuite, multiplicités, géométrie des familles, gel des niveaux | planchers d'EVAL § 9.3 |

EG17 (Borůvka = Prim) et G2 (MR C++ = référence N-aire) d'EVAL et de CLUSTER sont **retirées** avec le producteur
MR C++ (D-V10-17).

### 11.5 Performance (V10-4) et campagne (V10-5)

| Porte | Contenu |
| --- | --- |
| EP1 | `merkle_prefix(5)` de K10 = celui de K5 ; K10 = référence K10 certifiée ; K5 = référence K5 certifiée |
| EP3 | juge des clés absentes, strates de fin de fenêtre (q2 à p = Kmax − 1, q3 à Kmax − 2, q4 à Kmax − 3), 29 956 clés vraies par (K, strate), ≥ 1 000 par trame ; couverture du cadre publiée |
| EP4 | écart ≥ 20 scans entre trames dev et test |
| EP5 | go/no-go écrit dans le reçu avant toute session de contrat |
| EP6 | sha256 des sites des trames de conception = entrées R22/L13 |
| EP-M1, EP-M2 | mutants causaux tués (code 4) |
| P-COST-CPU | mur du générateur W48 (≤ 0,25 s K5, ≤ 0,8 s K10) et µs CPU par boule à W24 (≤ 4 µs) publiés |
| B2-DIGEST | 100 % des passes chronométrées égales à la référence ; aucune relance silencieuse |
| PREREG | `PREREG_EVAL_V10_<date>.toml` commis avant toute lecture des vérités `test` (EG16) ; réexécution seulement par une porte listée dans `gates_at_prereg` |

---

## 12. Plan de phases

### 12.0 Point de départ : migrations de V10-α

| Composant V10-α | Sort | Phase |
| --- | --- | --- |
| `core`, `sched`, `wide.hpp`, `run_expect.cmake`, statuts | gardés ; complétés (budget logique, `ChunkPool`, `ids.hpp` typé) | V10-0 |
| `cloud` (`SiteTree` rationnel) | `SiteTree` réduit aux requêtes entières ; la partie rationnelle passe au `LeafOracle` | V10-1/2 |
| `catalogue/generator.cpp` (feuille v2, stagnation, clause pondérée, `max_leaf` 256, vecteurs, offsets u64) | migré en place selon GEN_v2 ; le catalogue doit rester égal aux épingles à chaque commit | V10-1 |
| `tower/tower.cpp` (descente, mémo, refus des multiplicités) | déplacé dans `tests/reference/tower_alpha/` comme différentiel pendant la réécriture, **supprimé** à la sortie de V10-2 | V10-2 |
| `head/`, `points/dendrogram` | gardés ; alignés (N-aire, plancher, abstention, `det_log2`) | V10-3 |
| `tests/head/mreach.cpp` | retiré du chemin E1 ; peut devenir le juge EMST K = 1 s'il passe T-JUDGES | V10-2 |
| `bench/synthetic` (8 familles, DBCV) | étendu au plan d'EVAL (16 familles + nuls, statistique, `decide.py`) ; le reçu `bench_dev_20260928` reste un reçu `dev` | V10-3 |
| `reference/hgp10_ref.py` | gardé comme second langage (oracle Γ_k Fraction) | toutes |
| `gcp-migration/v10_*` (non commis, 3 bloquants) | corrigés puis commis | V10-0 |

### 12.1 Vue d'ensemble

```text
            J0     J3         J8              J14            J20            J28
V10-0  [fondations + C0]
V10-1        [catalogue GEN_v2 + juges + C1] -> D-K5
V10-G                    [catalogue GPU, ouverte par D-K5 pour K10] ···············>
V10-2        [tour TOWER_v2 : T-0 (G4) .. T-4] ·······> [T2 pondérée, lève duplicate_positions]
V10-3  [tête + banc + E-a/E-b dès J0] ··········> [branchement tour, E-c0, E-c (E1-dev), PREREG]
V10-4a                                        [P9, étalonnage, contrat CPU K5 (et K10)]
V10-4b                                                        [GPU tour + chaîne, 100 ms K5]
V10-5                                                    [campagne de test préenregistrée]
```

Chemin critique LiDAR : V10-0 → V10-1 → V10-2 → V10-4a → V10-4b. Chemin clustering : V10-0 → V10-3 (tête et banc sur
V10-α et arbres sklearn dès J0) → branchement de la nouvelle tour quand T-T2 est vert → V10-5.

### 12.2 Phases

**V10-0 — Fondations et calibration** (≈ 3–4 jours-agent ; rôles E, A, D).

- Entrée : cette conception acceptée. L'accord sur `AGENTS.md`/`CLAUDE.md` (question 2) ne bloque que l'édition de
  ces fichiers, pas le code.
- Travail : budget logique, `ChunkPool`, `ids.hpp` typé ; `arith` (U192/U256/U320, `level.hpp`, `sphere.hpp`, lemme Z,
  P3c, `widths.hpp`) ; `io` (SHA-256, Merkle, `tree.bin`) ; `alloc/` et empoisonnement ; `check_v10.py`,
  `receipt.py`, `run_schema.py`, `disk_gate.sh`, `sync_push.sh` ; workflow CI ; scripts G4 corrigés et commis ;
  `bench/proto/cble.cpp` et `bench/proto/pdendro.cpp` commis ; squelettes `SPEC_V10`, `PREUVES_V10`, `PROVENANCE`.
- Sortie : portes F-\* vertes ; V10-R01, R12, R21 écrits ; **reçu C0** sur G4 (cble à K5/K10 sur les 3 trames de
  conception et la brute, à W1, W24, W48 ; moteur v9 K5 à W1 et W48 ; `pdendro` T-0 ; même binaire en local) donnant
  ρ, S24, S48 et la décision D-T5/D-T6 écrite.

**V10-1 — Catalogue exact, oracles, J-KM2** (≈ 5–7 jours-agent ; A, D).

- Entrée : V10-0 close ; V10-R02 à R10 écrits ; note de réouverture de la piste v3.
- Travail : GEN_v2 tranches G0–G5 sur la base de V10-α : dominance seule sans branchement, tampon d'insertion, phase
  en largeur, feuille par matrice de dominance et mémo par masques, admission positionnelle, sortie SoA, rangs à clé
  fidèle, digests, `LeafOracle` ; oracle O2 (rôle D) ; juges J1, J3, J6, J-KM2, boîtes ; portes de coût.
- Sortie : portes C-\* ; **reçu C1** (catalogue produit à W1/W48 sur G4, P-CAL de M(K), P-COST-CPU, J-KM2 sur les
  4 trames) ; **décision D-K5** écrite dans la passation : `B2_pred(K) = c1_G4 · N / S48 + T_tour + 0,05 s` sur la
  pire trame, avec T_tour de TOWER ; si la borne haute est ≤ 0,8 s, CPU seul ; si la borne basse est > 1,0 s, V10-G
  s'ouvre immédiatement ; entre les deux, les deux pistes, V10-G en second. Pour K10, V10-G s'ouvre sauf borne haute
  ≤ 0,8 s.

**V10-G — Catalogue GPU** (conditionnelle ; ≈ 7–10 jours-agent ; E).

- Entrée : D-K5 ; V10-R36 écrit.
- Sortie : égalité des multiensembles d'enregistrements (S\*, I, U, rang) contre le CPU sur les trames et à
  8k/16k/32k (24/24) ; feuilles m > 32 comptées ; reçu G4.

**V10-2 — Tour FULL selon TOWER_v2** (≈ 9–12 jours-agent ; B, D).

- Entrée : V10-0 close pour démarrer (doublure : catalogue brut borné et catalogues V10-α) ; V10-1 close pour sortir ;
  V10-R11, R14 à R20 écrits.
- Travail : jalons TOWER T-1 (atlas, tables circulaire/3D/classes, validation, Euler pondéré, H_K), T-2 (MEB,
  résolveur sur `LeafOracle`, pointeurs, minima ; histogramme des sauts sur 08/000200 à ± 5 % de la critique ;
  J-DESC local), T-3 (T1, T2a/b/c, Q, marques, verticales, entrées C∩X, digests), T-4 (échelle) ; V10-α tour en
  différentiel dans `tests/`.
- Sortie : portes T-\* ; **T2 pondérée verte et refus `duplicate_positions` levé dans le même commit** ; µs CPU par
  boule de la tour publiés (locaux, diagnostic) ; la passation écrit si V10-4a commence par des leviers de tour
  (cible : ≤ 3 µs à K5 et ≤ 4 µs à K10 en local, soit ≈ 1,6 × l'estimation haute).

**V10-3 — Points, têtes, banc, E1** (≈ 8–11 jours-agent ; C, D ; démarre à J0).

- Entrée : piste tête et banc dès V10-0 (sur V10-α et les arbres sklearn) ; branchement sur la nouvelle tour quand
  T-T2 est vert ; V10-R22 à R35 écrits.
- Travail : graphe-chemin, clés unifiées, tête N-aire (plancher, abstention, `det_log2`/`det_exp2`, ẑ) ; banc EVAL
  (familles, quantification, lanceur à un fork par couple, métriques, statistique, `decide.py`, adversaires sklearn,
  sources `mr` réatomisées) ; phases E-a (harnais), E-b (calibration), E-c0 (coûts sans étiquettes), E-c (dev :
  E0–E6, choix de P, `hdb_dev`, `mr2_P*`, `mr1_P*`) ; E-d (préenregistrement).
- Sortie : portes H-\*, EG-\* ; reçus `dev` ; E1-dev publié avec la prévision G/N/D ; `PREREG` commis.

**V10-4a — Performance CPU sur G4** (≈ 4–6 jours-agent + 3 sessions ; B, E).

- Entrée : V10-2 close ; D-K5 écrite ; EP5 écrit.
- Travail : session P9 (épingles v9 sur chaque trame mesurée, sans sol et brute, K5 et K10) ; session d'étalonnage
  (κ, η, bras `cpu48`, balayage Kmax = 1..10 sur les trames de conception) ; leviers en A/B ABBA sur conception + dev
  (adoption si digests égaux, borne haute de l'IC < 0, gain médian ≥ 2 % ; sinon `FAUSSES_PISTES.md`) ; contrat.
- Sortie : **si les 30 trames de test sont présentes**, verdict C(K5, 1 s) (et C(K10, 1 s)) en `cpu48`, publié quel
  qu'il soit, avec B0, C, RSS et trames brutes ; **sinon**, diagnostic sur les 3 trames de conception (maximum, jamais
  un p95) et contrat OPEN pour raison de données.

**V10-4b — GPU** (≈ 8–12 jours-agent + 2 sessions ; E, B).

- Entrée : V10-4a close (ou V10-G avancée) ; profil `cpu48` montrant un étage > 30 % de B2 ; V10-R36.
- Sortie : CPU = GPU au digest sur toutes les trames et à 8k/16k/32k ; verdicts C(K10, 1 s) et C(K5, 1 s) en bras
  `gpu` ; décomposition mesurée contre le budget 100 ms à K5, note de décision.

**V10-5 — Campagne contre HDBSCAN** (≈ 3–4 jours-agent + 2 sessions de ≤ 4 h ; C, D).

- Entrée : V10-3 close (E1-dev faite, P gelée) ; `PREREG` dans HEAD, hash vérifié.
- Travail : exécution unique sur l'espace `test` (8 438 scènes), `DECISION.json` par `decide.py`, reçu
  `receipts/eval_v10_<date>/`.
- Sortie : phrase générée publiée **quel qu'en soit le signe** ; lignes V10-R39 mises à jour ; README formulé selon la
  phrase.

Total : ≈ 47–66 jours-agent, dont 15–22 conditionnels au GPU ; 3 à 4 semaines de mur à 4–5 agents.

### 12.3 Sessions G4 (une seule session SPOT utile à la fois)

| Ordre | Session | Contenu | `maxRunDuration` |
| --- | --- | --- | --- |
| 1 | C0 calibration | cble, moteur v9, `pdendro` ; ρ, S24, S48 | 3 600 s |
| 2 | C1 catalogue | produit V10-1, M(K), P-COST-CPU, J-KM2 | 3 600 s |
| 3 | P9 épingles v9 | chaîne v9 exportée sur chaque trame mesurée | ≤ 2 h |
| 4 | étalonnage LiDAR | κ, η, bras `cpu48` (et `gpu` si prêt) sur conception + dev | ≈ 1 h |
| 5 | contrat CPU | références certifiées, juge des clés absentes, résident, froid | ≈ 1 h 50 par K |
| 6 | qualité `dev` 16k/32k et E-c0 | § 9.3 | ≈ 1 h 30 |
| 7 | contrat GPU | bras `gpu` et `cpu48` | ≤ 3 h |
| 8–9 | campagne de test | n ≤ 8k, puis 16k et 32k | ≤ 4 h chacune |

Avant toute session : `ctest -L gate` vert au commit, compilation CUDA locale (si utile), `v10_selftest.py` vert,
`disk_gate.sh 3000`, EP5 écrit pour une session de contrat, dépôts de l'auditeur lus. Après : `TERMINATED` certifié
sur exactement la cible (`start_and_verify.sh` / `stop_and_verify.sh`), écrit dans le reçu.

### 12.4 Règles communes à toutes les phases

- Un commit par tranche sur `main` (code, fixtures, reçu, passation ensemble) ; jamais de branche ; jamais
  `git add -A` ; `git diff --cached --quiet` avant `git add` ; jamais `worktree prune` ni `gc --prune=now`.
- Toute contradiction mathématique devient une fixture minimale permanente et une ligne du registre **avant** de
  continuer.
- Un chiffre publié cite un reçu ; les temps locaux sont des diagnostics ; les compteurs déterministes et les exposants
  font foi en local.
- `docs/implementation_status.toml` n'est pas touché (exploration hors registre).
- PASSATION réécrite à chaque sortie de phase (≤ 300 lignes) ; `check_docs.py` vert.

---

## 13. Risques et plan de repli

| # | Risque | Probabilité | Parade | Repli |
| --- | --- | --- | --- | --- |
| 1 | La tour réécrite n'atteint pas le coût de conception (V10-α est à ×6–24) | moyenne | T-0 sur G4 avant le code ; étage G mesuré dès T-2 ; jointure triée pour H_K ; `LeafOracle` | tour V10-α gardée comme référence ; GPU de l'étage G avancé ; NO-GO K5 CPU publié |
| 2 | Constante du générateur produit > sonde (C1) | moyenne | microbancs à sorties identiques ; O-F1 si ×1,5 | V10-G ouverte pour K5 aussi |
| 3 | C(K10, 1 s) inaccessible en CPU | élevée | V10-G, V10-4b | verdict NO-GO CPU publié ; contrat GPU seul |
| 4 | 100 ms à K5 hors d'atteinte | élevée | budget par étage, processus résident | décomposition publiée, aucune promesse |
| 5 | Données de test KITTI absentes | élevée tant que la question 1 n'a pas de réponse | § 14 | diagnostics sur 3 trames ; contrat OPEN |
| 6 | E1 sans gain géométrique (P1a) | élevée | règle écrite d'avance | phrase « le gain vient de la tête » ; valeur de la tour = topologie exacte multi-ordres |
| 7 | R1 échoue contre `hdb_dev` (parité mesurée en `dev`) | élevée | R1-std publié comme niveau inférieur | « bat HDBSCAN par défaut, apparié et protocole de la thèse » seulement |
| 8 | Disque (3,3 Go libres) | certaine | worktrees clairsemés, porte disque, transitoires en scratchpad | un rôle actif à la fois ; question 3 |
| 9 | Zone aveugle de fin de fenêtre (Kmax − 1, Kmax) | faible | J-KM2, EP1, EP3, boîtes, différentiel v9 | omission trouvée → fixture + registre, référence invalidée |
| 10 | Pire cas non borné (sortie Ω(n²), grilles, sphères de réseau) | faible sur les données visées | budget logique, `wide_leaf`, refus typés, portes de coût adverses | refus explicite publié |
| 11 | Dépendance aux internes de sklearn (`_single_linkage_tree_`, α) | faible | version et sources épinglées, EG7c, EG-ATOM | arbres `sk` binarisés publiés sans réatomisation, déclarés |
| 12 | Projection GPU fausse (×10 en v9) | moyenne | aucune porte sur projection ; A/B sur B2 | calendrier V10-4b repoussé, verdict CPU seul publié |
| 13 | Rechute de méthode (leviers, modes, juges dans le produit) | moyenne | `check_v10.py`, liste blanche, ablation obligatoire | revert du commit fautif |
| 14 | Multiplicités tardives (T2 pondérée) | faible | fixtures prêtes, prototype pondéré vérifié | suites publiques à doublons refusées et comptées dans S-pub |

---

## 14. Questions ouvertes pour l'utilisateur

1. **Données KITTI.** Pour les 20 trames `dev` et les 30 trames de test (séquences 00–07, 09 et 10), pouvez-vous les
   fournir hors Git, ou autoriser le téléchargement temporaire de l'archive velodyne de KITTI odometry (≈ 80 Go dans
   `/tmp`, volatil) pour en extraire ces scans ? Leur envoi privé vers la VM G4 pendant les sessions gardées est-il
   conforme à votre usage de la licence ? Sans réponse, le contrat reste OPEN et seuls des diagnostics sur les trois
   trames de conception sont publiés.
2. **Fichiers normatifs.** Autorisez-vous une section « Ouverture v10 » dans `AGENTS.md` et le passage de la cible de
   `CLAUDE.md` de la v5 à la v10 ? La v10 est déjà développée sur `main` sans que ces fichiers la désignent. La
   section v10 du registre des preuves est, elle, tenue comme travail ordinaire.
3. **Disque.** `/workspaces` n'a plus que 3,3 Go libres. Le travail à 4–5 rôles demande ≈ 3,5 Go au pic. Autorisez-vous
   le nettoyage de `build/` (≈ 22 Go : arbres v4 à v8 et builds d'audit v9 de plus de sept jours, non cités), selon
   les critères du 23 septembre ?

---

## 15. Questions des sous-systèmes tranchées ici

| Origine | Question | Décision | Raison |
| --- | --- | --- | --- |
| GEN 1, ARCH 1 | C(K10, 1 s) dépendant de la feuille GPU ou d'O-F1 ; trames brutes contractuelles ? | oui, déclaré ; trames brutes = régime secondaire | EP5 prévoit NO-GO CPU à K10 ; décision du 21 sept. |
| GEN 2 | autoriser O-F1 | oui, sous preuve écrite et gain ≥ ×1,5 de la feuille K10 | doctrine du dépôt (filtre certifié à repli exact) |
| GEN 3 | admission positionnelle et digest sans niveau | oui (§ 5 lignes 1–2) | lemme W ; J-KM2 |
| GEN 4 | COST-C bloquante, exposant d'arbre LiDAR en diagnostic | oui | la série emboîtée n'est pas un doublement pur |
| TOWER 1 | mode certifié `kcat` = Kmax + 2 par défaut ? | références et reçus de qualification seulement | coût ×1,35–1,9 ; transfert par digest |
| TOWER 2 | `checks=sampled` par défaut | oui, et c'est le seul mode du produit | § 5 ligne 14 |
| TOWER 3 | planifier D&C ou PANDORA pour K10 à 100 ms | non ; K10 à 100 ms déclaré hors d'atteinte | aucun budget crédible (≤ 12 ns par boule) |
| TOWER 4 | numérotation K1 | `SiteIdx` ; l'exporteur rend l'ordre v9 | R5 : aucun `PointId` dans le moteur |
| TOWER 5 | portes sur trames locales non versionnées | oui, empreintes dans le reçu | précédent v8/v9 ; aucun octet versionné |
| CLUSTER 1, ARCH 4 | cas N : producteur du clustering | la tête sur la tour ; la phrase dit l'équivalence avec MR α = 2 | D-V10-21 |
| CLUSTER 2, EVAL 1 | « battre HDBSCAN » doit-il survivre à α et au réglage sur `dev` ? | oui pour R1 ; R1-std publié à côté | α est un paramètre public de sklearn |
| CLUSTER 3, EVAL 2 | politique de bruit visée | choisie sur `dev` par J (ARI_s neutre) ; LiDAR : `none` publié à côté | aucune préférence produit n'est déclarée |
| CLUSTER 4 | α dans `hdb_dev` | oui ; `hdb_dev` est sklearn tel quel | § 5 ligne 9 |
| CLUSTER 5 | ordre de la tête LiDAR | K_P du banc ; qualité LiDAR exploratoire (E8, dev) | pas de vérité terrain utilisée en porte |
| CLUSTER 6 | mode produit « clustering seul » à Kmax = K_P | non ; le temps de la tour à `kmax` = 2 ou 3 est publié en E6 | `kmax` est déjà un paramètre ; pas de mode spécial |
| EVAL 3, ARCH 2 | données KITTI | **question 1** | décision de l'utilisateur |
| EVAL 4 | 155 h CPU de campagne, 30 h par `dev` | accepté dans le feu vert G4 ; réduction symétrique décidée en E-c0 | coûts sans étiquettes d'abord |
| EVAL 5 | première session G4 = étalonnage | oui (C0) | mesurer ρ et S48 avant tout contrat |
| EVAL 6, ARCH 3 | contrat B2 ou chaîne C | B2, avec B0 et C dans la même phrase | continuité R22 ; la tête et le sol sont mesurés à part |
| EVAL 7 | suites publiques larges, piste qualité LiDAR | gardées, descriptives ; LiDAR sur trames `dev` seulement | coût modéré, aucune revendication |
| ARCH 5 | `AGENTS.md`, `CLAUDE.md` | **question 2** | fichiers normatifs |
| ARCH 6 | nettoyage de `build/` | **question 3** | action destructive |
| ARCH 7 | refus `duplicate_positions` jusqu'à V10-2 | oui | § 5 ligne 5 |

---

## Annexe A — Correspondance des identifiants

| Ce document | GEN_v2 | TOWER_v2 | CLUSTER_v2 | EVAL_v2 | ARCH_v2 |
| --- | --- | --- | --- | --- | --- |
| V10-R02–R09 | V10-GEN-1 à 7, 10 | — | — | — | P1, P3a, P16, P18 |
| V10-R10 | — | — | — | — | P2, P2b |
| V10-R11 | lemme W (partie tour) | PO-T16 | — | — | P2w, P5w, P7w |
| V10-R14–R20 | — | PO-T1 à T24 | OP6 | PO-E12, PO-E15 | P5–P9, P13, P17 |
| V10-R22–R28, R30–R32 | — | PO-T11 | OP1 à OP15 | PO-E5, PO-E6 | P10–P12 |
| V10-R29 | — | — | (remplace § 4.3) | (remplace § 2.3) | (supprime `mreach.cpp`) |
| V10-R33–R35 | — | — | — | PO-E1 à E18 | P15 |
| V10-R36 | § 10.2 | § 14 | — | § 12.11 | P14 |
| V10-R37–R39 | V10-GEN-9 | — | P1a–P6 | R1, ATT, R3, ARB | annexe D |
| D-V10-02–05 | § 1, § 5–7 | — | — | — | D-02, D-44 |
| D-V10-07 | § 9 | § 5.6 | — | — | D-03\* |
| D-V10-08–13 | — | D-T1 à D-T20 | — | — | D-05\*, D-06, D-13, D-29 |
| D-V10-14–16 | § 6.5 | § 6.7, § 8.3 | — | — | D-07\*, D-10\*, D-20\*, D-31\* |
| D-V10-17–21 | — | — | § 4–9, § 13 | D5–D7, D20 | D-27, D-28, D-35 |
| D-V10-22–23 | § 11.9 | § 11.3 | § 11 | D13, D22 | D-24, D-37\*, D-42 |
| D-V10-24–28 | § 10 | § 11, § 14 | § 10 | § 9 | D-09 à D-23, D-32, D-33\* |
