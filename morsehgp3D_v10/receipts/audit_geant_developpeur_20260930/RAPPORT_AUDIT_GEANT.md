# Audit géant v10 — synthèse du développeur

30 septembre 2026. Reprise du développement après la tranche de l'auditeur-développeur.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_geant_developpeur
public_status=not_claimed
GCP non utilisé
```

Ce rapport ne qualifie rien et ne promeut aucun statut. Suivi détaillé :
[TRACKER.md](TRACKER.md). Plan des tranches : [PLAN_DEVELOPPEMENT.md](PLAN_DEVELOPPEMENT.md).

## 0. Ancre, périmètre, méthode

- **Ancre** : `8bb4618e5`. L'intervalle audité est `bdc0b8f08..8bb4618e5` : un commit de
  développement (`4b7d70422`) et une dizaine de commits d'audit.
- **Code produit stable jusqu'au dernier commit** : `src`, `cli`, `tests`, `CMakeLists.txt`,
  `bench`, `reference`, `cmake`, `tools` et `docs/math` sont identiques de `8bb4618e5` à
  `33fcb53a0` (`git diff --quiet` : code 0).
- **Après l'ancre** : `408d1ffe4` (note du développeur, décision de précision) puis quatre
  commits d'audit (`d192637a7`, `df0890977`, `a8252527e`, `33fcb53a0`). Ils n'ont pas été
  audités par les lentilles. Un seul constat produit en est repris : TT1 (condensation de la
  tête), marqué « non rejoué ».
- **Build de référence** : `build/v10-giant-audit/build` (Release HEAD). 13 portes sur 13 :
  11 hors oracles en 44 s ; oracles catalogue et tour en 163 s et 183 s.
- **Sept lentilles**, chacune suivie d'un vérificateur adverse :

| Lentille | Rapport | Vérification |
| --- | --- | --- |
| Code livré au moteur | `moteur_livre/RAPPORT.md` | `verif_moteur_livre/RAPPORT_VERIF_MOTEUR_LIVRE_20260930.md` |
| Frontière FULL → points | `frontiere/RAPPORT_FRONTIERE_20260930.md` | `verif_frontiere/RAPPORT_VERIF_FRONTIERE_20260930.md` |
| Précision au-delà de u18 | `precision/PLAN_PRECISION.md`, `precision/INVENTAIRE_PORT.md` | `verif_precision/VERDICTS.txt` |
| Raccord R2 | `raccord_r2/RAPPORT_RACCORD_R2.md`, `raccord_r2/PLAN_INTEGRATION_R2.md` | `verif_raccord_r2/VERIF_RACCORD_R2.md` |
| Constats ouverts | `constats/TRACKER.md` | `verif_constats/INDEX_PREUVES.txt` |
| Santé, build, CI | `sante/RAPPORT_SANTE.md` | `verif_sante/preuves/` |
| Cardinalités, mémoire | `cardinalites/RAPPORT_CARDINALITES.md` | `verif_cardinalites/INDEX_PREUVES.txt` |

**Règles de consolidation.** Un constat confirmé ou incertain est gardé. La gravité est celle
du vérificateur. Les manques trouvés par les vérificateurs sont ajoutés. Les doublons entre
lentilles sont fusionnés. Les constats réfutés sont listés à part. Une ligne du suivi des
auditeurs que le vérificateur n'a pas examinée est gardée et marquée « non contre-vérifié ».

## 1. Verdict

1. **Le code livré par l'auditeur-développeur est exact et ne change aucune sortie.** Les
   modèles indépendants, les sanitizers, les mutants et 216 paires de dumps pré/HEAD le
   confirment.
2. **Mais un trou de couverture est ouvert à l'égalité des attaches** (AT1). Une régression
   de `level_at_most` (core) ou du rang d'entrée cover passe toutes les portes, alors qu'elle
   change des attaches et des étiquettes. Or le port de précision doit réécrire cette fonction.
3. **Le produit u18 est sain** sous ASan+UBSan, TSan, Clang, valgrind et la chaîne de la VM
   G4. Ses sorties sont déterministes.
4. **Les défauts sont autour du moteur** : CLI qui meurent par signal ou écrasent des
   fichiers à code 0, décision de banc sans schéma validé, mémoire non admise, registre des
   preuves sans section v10 (seul bloquant), CI aveugle.
5. **Aucun correctif R2 n'est dans HEAD.** Le raccord est en cours. Il demande cinq
   préalables (SiteTree réparé, `OutputSet`, frontière de tête unique, portes `fast`,
   drapeaux des juges) et une décision que les essais n'avaient pas vue (RC6).
6. **Précision** : le refus u18 protège le profil actuel. Les échecs au-delà sont reproduits
   dès B21. Aucun palier au-delà de u18 n'est qualifié. Le chemin est décidé depuis le
   30 septembre : grille u32 par paliers, u24 puis u32.
7. **Frontière** : aucune campagne n'a tourné. La bande HEAD est laminaire mais pas robuste.
   L'antichaîne des témoins est le meilleur candidat. La tête doit d'abord être corrigée (TT1).
8. **Totaux du suivi** : 98 lignes — 1 bloquant, 17 majeurs, 51 mineurs, 29 infos.

## 2. Ce que l'auditeur-développeur a livré, et ce qui tient

### 2.1 Tranche `4b7d70422`

Le build HEAD refait depuis `git archive` est identique octet pour octet au build fourni
(`mhgp10_tower` `3d55b869…`, `mhgp10_cluster` `5df023bd…`).

| Élément | Nature | Verdict | Preuve |
| --- | --- | --- | --- |
| `src/tower/rank_search.hpp` | recherche de rang : milieu par différence, produit élargi avant la borne | exact ; même suite de sondes que l'ancien code jusqu'à 2^31 | Deux modèles Python non bornés : 45 451 cas exhaustifs et 19 539 (ou 19 500) cas virtuels jusqu'à 2^32−1, 0 écart, au plus 33 sondes. Normal, `-O`, ASan+UBSan, clang `-fsanitize=integer`. L'ancien code était faux sur 11 314 des 19 500 requêtes virtuelles. |
| raccord `RankIndex` (`tower.cpp:1004-1017`) | délégation au helper | aucune sortie changée | 216 paires de dumps pré-tranche/HEAD égales (31 entrées, K5/K10, core/cover, 1 et 4 fils, 87,2 Go hachés). 184 paires 1/4 fils égales. `head_diff` 18/18. `sample.size() = ⌈size/64⌉` sur 1 113 tailles ; 2 514 442 requêtes égales au comptage. |
| `src/cloud/grid32_primitives.hpp` | distance u128, Morton96, décodeur | exact ; non raccordé au moteur | Oracle non borné : 28 072 distances (3 926 au-dessus de 2^64), 23 008 clés, 10 002 décodages, 2 098 refus, 0 écart. Mutants tués : 13/13 (lentille), 12 non équivalents sur 13 (vérificateur). Porte sous ASan+UBSan : 212 684 contrôles. |
| `tests/unit/rank_search.cpp`, `grid32_primitives.cpp` | portes C++ | vertes | Section 3 non bornée (PT1) ; pas de label `fast` (PT2). |
| `bench/frontier/cover_band.py`, `dev_quotas.py` et leurs tests | banc exact, hors tête de production | propriétés annoncées vraies ; robustesse non acquise | Juge Γ indépendant : 700 couples (nuage, K), 1 070 483 contrôles, 0 écart. Quotas : 642 allocations, 51 886 contrôles, 19 refus. Voir FR2–FR4. |
| `tests/points/test_cover_band_native.py` | test natif borné | PASS, 815 contrôles, 6 nuages, normal et `-O` | Non enregistré (PT6). Ne tue pas le mutant strict d'AT1. |
| `CMakeLists.txt` | 4 portes | 2 portes Python hors `run_expect` | Casse `mhgp10_fast_targets` après raccord (RC5). |

Revendications du suivi `docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md`, vérifiées :

| Revendication | Verdict |
| --- | --- |
| Milieu par différence, produit élargi avant la borne | confirmé |
| « Sans allocation supplémentaire ni changement d'ordonnancement » | confirmé (suites de sondes identiques) |
| 36 047 contrôles ; les deux erreurs réintroduites sont tuées | confirmé sur la porte enregistrée, code 1 |
| « Pas une correction de toutes les conversions de cardinalité » | exact : garde du catalogue toujours absente (MM2) |
| grid32 : 212 684 contrôles, deux troncatures tuées ; « pas encore raccordées » | confirmé |
| Test natif : 815 contrôles, normal et `-O` | confirmé, mais hors CTest |

### 2.2 Ce qui est solide ailleurs

| Sujet | Preuve |
| --- | --- |
| Produit u18 sous outils | ASan+UBSan : 13/13 portes, dont les 11 hors oracles en options strictes. TSan : `mhgp10_unit` ×3, trois CLI à 4 fils, K5/K10 core/cover. Clang 18 `-Werror`. Clang `-fsanitize=integer` : 20 sites, tous modulaires et voulus, 0 UB. `MHGP10_POISON` et valgrind : 0 erreur. |
| Déterminisme | Empreintes catalogue `4939fead…`, tour `d00a1d6f…`, étiquettes `06091b42…`, arbre `a55607ea…` identiques dans six configurations. |
| Chaîne de la VM G4 | GCC 11.4, CMake 3.22.1, Python 3.10.12 : build `-Werror` propre ; sorties K5/K10 core/cover identiques au Release GCC 13. |
| Refus u18 | Coordonnée 2^18 : code 2. `prepare_cloud` refuse bits > 21. Bornes prouvées jusqu'à B19 composante par composante, et jusqu'à B20 par bornes euclidiennes pour le côté q3 et le dénominateur q4. |
| Raccord R2, essai A | HEAD + six groupes sans `entrees_cli` : 57/57 portes (avec deux reçus archivés). Tour 8/8, tête 18/18, catalogue 10/10 identiques à HEAD, à 1 et 4 fils. |
| Réparation SiteTree (`82b49a9`) | 34/34 mutants tués. ASan/UBSan et TSan à 0. +0,0008 % d'instructions. |
| Groupe `faits_math` | S'applique sur `8bb4618e5`. Portes vertes contre les binaires HEAD : faits 5/5 et 10/10 mutants ; registre 5/5 et 8/8. |
| Fondations frontière | Porte de conception : 282 502 contrôles, 14/14 mutants. Validation : 1 961 564 contrôles, 0 écart. Références core 36/36 identiques avec le binaire HEAD. |
| Reçus rejoués | `inverse_beta_shell`, `duration_*`, `intrinsic_cross_k`, `math_review` : sorties identiques en normal et `-O`. |
| Liens | 42/42 dans les fichiers de navigation, 260/260 dans `audits/`. Cinq déplacements R100. `SHA256SUMS` 12/12 et 7/7. |
| Bornes de précision de l'auditeur | 60 comparaisons, 6 écarts d'un bit (trop larges), aucune borne trop petite. |

## 3. État par chantier

### 3.1 Moteur et portes

- 13 portes vertes. `ctest -L fast` n'en couvre que 4.
- Mutants du raccord `RankIndex` : rang −1 tué par 4 portes (via `validate`), mais T2 reste
  vert ; rang +1 et échantillon lu dans `level[i]` tués par 5 portes ; `level_at_most` strict
  tué par **aucune**.
- Le mutant strict change 389 attaches sur 229 225 (lidar02 K5, core), 776 à K10, et les
  étiquettes d'une fixture de 5 points. Le mutant cover sans `+1` passe 11/11 portes et change
  les étiquettes d'une fixture de 9 points. HEAD respecte l'invariant exact dans tous les cas
  mesurés. C'est AT1.
- Le contrat est ambigu à l'égalité : le témoin mreach produit légitimement l'égalité
  (feuilles de durée nulle). L'invariant fermé vit donc dans le juge exact de la tour, pas dans
  `validate()` (AT2).
- Conventions de portes : RC5 (portes hors `run_expect`), PT1 à PT7.

### 3.2 Frontière (FULL → partitions de points)

**Ce qui existe.** La bande B_η est un banc Python exact, hors tête de production. Ses
propriétés annoncées sont vraies : couverture à l'entrée, emboîtement, monotonie en η,
η=0 = A5, théorème de l'univers fort.

**Ce qui ne tient pas.**

| Bras | Contre-exemple | Effet mesuré | Statut |
| --- | --- | --- | --- |
| A0 core | — | ≤ 2ε | contrôle stable, retarde la frontière |
| A1 cover | F2 `{0,999,2000}` | saut de 500,5 pour 2 unités | contrôle de participation précoce |
| A2 bande de paires (K2) | G6 (η=1/4) | discontinuité de bord | affaibli |
| A3 majorité uniforme | paires `0,1,L,L+1` | 0 groupe sur 2 avant la première fusion | réfuté, témoin négatif |
| A4 majorité 1/β | cinq sites ; contact coquille/intérieur | LiDAR K5 : 99 % différés, 26 % après D_K | réfuté comme bras principal |
| A5, A6 | quasi-égalité K3 | saut de 1 068 et 2 135 pour 1 unité ; égaux à A1 hors 0 à 9 points | aucun gain sur A1 |
| B_η (HEAD) | bord de bande ; témoin ancêtre ; univers | saut 1,16·S et 1,54·S ; 36–88 % différés à K5 | affaibli ; D2 non mesuré |
| B_η^min (antichaîne) | bord de bande | invariant par l'univers, jamais plus tardif | meilleur candidat, non qualifié |

**Mesures à l'échelle** (hors préenregistrement ; un crop LiDAR de 8 000 et des nuages
synthétiques de 8 000 à 32 000). À K5, la moitié des points a une lignée concurrente à moins
de 3,7 à 5,2 % de α. Sous 1 mm de jitter, 22 à 35 % des points LiDAR K5 restent à risque,
quel que soit le bras de couverture.

**Campagne.** Le préenregistrement est scellé hors dépôt (`build/v10-frontiere`). Aucun bras
n'a tourné. Les fondations sont réutilisables. L'export complet à 32 000 points K5 est
faisable : 532,6 Mo, 57 s de chargement, pic de 5,6 Go.

**Après l'ancre, côté auditeur.** Nouveau verrou de tête (TT1) : à corriger avant toute
comparaison frontière. Le LCA de l'antichaîne se calcule par deux extrêmes d'Euler, sans tri
(`antichain_counterreview_20260930`). Le chantier de recherche « FULL → hiérarchie
laminaire » (`build/v10-verrou-points`) est séparé ; ses bras entreront par amendement.

### 3.3 Précision au-delà de u18

**Décision en vigueur.** 30 septembre (`408d1ffe4`) : grille u32 à pas décimal exact,
moteur porté par paliers u24 puis u32 complet, refus explicite hors domaine ; float32 natif
sans perte non développé pour l'instant.

**Protection actuelle.** `generator.cpp:636` et `tower.cpp:1156` refusent bits > 18.
`prepare_cloud` refuse bits > 21. Toutes les bornes tiennent jusqu'à B19 composante par
composante ; B20 est prouvé pour le côté q3 et le dénominateur q4 par bornes euclidiennes.
Le refus u18 a donc probablement 2 bits de marge.

**Échecs reproduits hors contrat** (corps HEAD appelés directement, normal = UBSan) :

| B | Échec | Diagnostic |
| --- | --- | --- |
| 21 | dénominateur q4 = D² mod 2^128 (vrai : 130 bits) | aucun |
| 21 | côté q3 : −1 au lieu de +1 | UBSan `geometry.hpp:99-100` |
| 21 | orientation q4 en UB (signe juste par repli) | UBSan `geometry.hpp:120` |
| 21–22 | marge 0,02 non prouvée ; 0,0215 à B22 sous arrondi dirigé | aucun |
| 22 | dénominateur q4 nul (D = 2^65), niveau infini | aucun |
| 22 | `strictly_inside_tetra` inversé : boule q4 perdue | aucun |
| 22 | Morton64 : (0,0,0) et (2^21,0,0) même clé | aucun |
| 24 | numérateur q4 nul : niveau (rayon carré) 0 au lieu de 844424829468675/4 | aucun |
| 24 | marge 0,02 : 64,9 % des sphères q3 et 81,0 % des sphères q4 fausses | aucun |
| 24 | `level_at_most` d'un port naïf : vrai sur un vrai niveau (e = 2^45) | aucun |
| 31–32 | distance K-NN i64 : UB puis valeur fausse | UBSan `site_tree.cpp:252` |

**Domaines KITTI** (séquence 08, trames 000000, 000100, 000200) :

| Pas | Domaine | Fusions |
| --- | --- | --- |
| 1 mm | B18 | 0 |
| 0,1 mm | B21 | 0 |
| 0,01 mm | B24 (marge 4,7 % sur x) | 0 |
| 1 µm | B28 | 0 |
| float32 sans perte | grille dyadique 2^−37 à 2^−41 m, B45 à B49 | — |

**Voies courtes.** Extrapolées d'histogrammes à 1 mm : 100 % à 0,1 mm, 96 à 99 % à
0,01 mm, 33 à 92 % à 1 µm. Ces parts ne sont pas mesurées au pas fin ; le « 100 % » est
optimiste pour certaines feuilles de lidar00 K10.

**Briques isolées de l'auditeur** : Morton96 et distance u128 (212 684 contrôles), filtre
relatif certifié (3 600 requêtes, 196 contacts, 40 translations), comparateur de niveaux
266/200 bits (2 444 requêtes). Aucune n'est raccordée.

**Désaccord de plan (PR1, incertain).** La lentille propose B21 comme premier palier. Cela
contredit la décision u24 puis u32. Le plan garde le contenu B21 comme étape interne du
palier u24.

### 3.4 Raccord R2

| Groupe | Patch | Verdict du flux R2 | Ouvert |
| --- | --- | --- | --- |
| pool | `db5e04e4` | accepté avec réserves | PL2 |
| sitetree | `1f6038d6` (r2) | **refusé** ; réparation `82b49a9` vérifiée ici (34/34) | RC3 : patch et reçu à publier |
| entrees_cli | `b1c0b530` | jamais jugé | SO2, RC4, RC6, 10 mutants survivants |
| oracles | `ea79324e` | accepté avec réserves | JG2, JG3, RC6 |
| tete | `af48faf8` | jamais jugé | RC4 ; G12, G17, R01 survivants |
| bancs | `e47ab923` | accepté avec réserves | BN1 (schéma), BN3 |
| faits_math | `65a86d2b` | jamais jugé | lacunes V1/V2, P5/P9 (RC2) |

- **Application.** Sur HEAD, pool, `entrees_cli` et `tete` échouent en application exacte
  mais passent en `-C1`. Le vrai conflit est entre séries : environ 24 zones dans 8 fichiers.
- **Essais.** A (six groupes sans `entrees_cli`) : tout vert, sorties identiques à HEAD.
  B (HEAD + `entrees_cli`) : `mhgp10_fast_targets` rend 3 (RC5). D (HEAD + pool +
  `entrees_cli`) : 23/24, puis vert une fois RC5 corrigé.
- **Nouveaux défauts de copie.** `OutputSet` détruit un fichier préexistant ou l'entrée sur
  refus (SO2). Le juge catalogue r2 et la règle M ≥ K+3 se contredisent (RC6).
- **État.** Le raccord tourne dans `build/v10-integration-r2` (base `a8252527e`) ;
  `faits_math` y est appliqué.

### 3.5 Constats ouverts des auditeurs et promesses

Le suivi des auditeurs compte 73 lignes. Elles sont reprises dans le suivi unique,
fusionnées avec les constats des lentilles, sauf DOC-09 (adressée à l'auditeur
indépendant). Les lignes closes y figurent à part. Les promesses du développeur :

| Promesse | Où | État | Ligne |
| --- | --- | --- | --- |
| Raccord commun en extraction figée | RR0929 § 1 | non tenue ; en cours | RC1 |
| Contre-exemples en fixtures, section v10 du registre | N0929, RR0929 | non tenue | RG1 |
| Correctifs I1, I2, P1, H1–H4, E1, G1, juges | N0929, RA0929 § 2 | copies seulement | CL1–CL3, TT2–TT5, PL1, BN1, BN2, ST1, JG1 |
| Budget mémoire logique | RA0929 § 3 | conditionnelle (« avant toute annonce de capacité »), non échue ; en-tête faux | MM1 |
| Rang exact publié | RA0929 § 3 | non tenue | TT6 |
| Limites statistiques dans la passation | RA0929 § 3 | non tenue | DC2 |
| Correction « plafond de Bayes » | N0929 | tenue en partie | DC3 |
| Sonde CUDA à la prochaine session G4 | RA0929 § 1 | en attente (aucune session) | CU2 |
| Reçus avec dumps et empreintes complets | N0929 | non tenue pour les différentiels R2 | RC8 |
| Expérience frontière | six réponses | non tenue | FR1 |
| Lemme de couverture gravé avant emploi | RP0929 § 3 | partielle | PT6 |
| Pool : TSan, création partielle, reçu | RP0929 § 4 | preuves en copie | PL2 |
| Bornes CSR avant intégration | RR0929 § 3.2 | conditionnelle, non échue | PF1 |
| `REPONSE_CLAUDE` après la synthèse | NR0930 § 4 | attendue | DC5 |

Tenues : relocalisation et liens, lecteur CUDA strict (deux réserves en CU1), corrections
d'échelle et errata, aucune campagne coûteuse avant les corrections.

### 3.6 Build, CI et dépôt

- Produit : propre sous tous les outils essayés (§ 2.2).
- `check_docs` rouge : 154 liens morts, tous dans des copies d'auditeurs sous `receipts/`.
  Il cache déjà de vrais liens morts (CI1).
- CI `main` rouge depuis le 19 juillet, pour des causes hors v10 ; `check_docs` y est sauté
  (CI2).
- Aucun workflow ne construit la v10 (CI3).
- 30 preuves hachées sont exclues par `.gitignore` (CI4).
- Hygiène : 8 portes Python sans `-B`, labels `fast` incomplets, vert par vacuité possible
  dans `test_level_collision`, Python non requis au configure.

### 3.7 Cardinalités et mémoire

Murs (scénarios arithmétiques à partir des ratios mesurés, pas des prédictions) :

| Mur | Refus dans HEAD | K=10 : sites LiDAR | K=5 : sites LiDAR |
| --- | --- | ---: | ---: |
| RAM 176 Gio (303,6–343,2 o/boule en RSS) | aucun : SIGABRT | 4,0–5,2 M | 16,8–20,3 M |
| Cellules d'atlas, tour complète | oui, tardif | 17–20 M | 77–87 M |
| Boules < 2^32 | aucun (copie R2 seulement) | 31–36 M | 131–140 M |

- Sur le chemin `mhgp10_cluster` (un seul ordre), la garde de l'atlas tombe après le mur des
  boules : la troncature silencieuse (MM2) y est le premier mur u32.
- La mémoire n'est pas admise (MM1). L'en-tête `buffer.hpp` affirme le contraire.
- Les forêts sont bornées : K ≥ 2 par la garde des cellules, K=1 par la garde des
  représentants par ordre.
- Élargir les identités en u64 sur place coûterait +109 à +129 o/boule, sans gain avant le
  mur RAM. Il vaut mieux un u32 local dans des segments gardés, et u64 aux frontières.

## 4. Défauts et manques consolidés

Portée : HEAD = produit au commit ; copie = copies R2 ; futur = garde pour un domaine refusé
ou non atteint ; doc/CI = documentation, registre, dépôt. Détail et preuves : suivi.

### 4.1 Bloquant

| ID | Portée | Constat | Tranche |
| --- | --- | --- | --- |
| RG1 | doc HEAD | Registre des preuves sans section v10 ; contre-exemples non gravés en fixtures | T0, T1 |

### 4.2 Majeurs (17)

| ID | Portée | Constat | Tranche |
| --- | --- | --- | --- |
| AT1 | HEAD tests | Attaches à l'égalité (core et cover) non gardées : un raccord faux passe toutes les portes | T1 |
| CL1 | HEAD | CLI : SIGSEGV et SIGABRT sur entrées ou options banales | T0 |
| SO1 | HEAD | Sorties écrasées ou non écrites à code 0 (collision, entrée = sortie, `/dev/full`, préfixe) | T0 |
| SO2 | copie | `OutputSet` détruit un fichier préexistant ou l'entrée sur refus | T0 |
| RC1 | process | Raccord R2 promis non fait (en cours) | T0 |
| RC2 | copie | Trois groupes jamais jugés ; mutants survivants | T0 |
| RC3 | copie | Réparation SiteTree non publiée ; le patch sauvegardé est l'état refusé | T0 |
| RC4 | copie | Frontières de tête incompatibles entre `entrees_cli` et `tete` | T0 |
| RC5 | HEAD tests | Deux portes `fast` hors `run_expect` : `fast_targets` rend 3 après raccord | T0 |
| RC6 | copie | Juge catalogue r2 contre règle M ≥ K+3 : oracle rouge sur l'arbre intégré | T0 |
| BN1 | HEAD bancs | Décision de banc sans schéma validé (lot incomplet, ARI 1,25, alpha=2) | T0 |
| MM1 | HEAD | Mémoire non admise ; SIGABRT au lieu d'un refus ; en-tête faux | T0, T1, T6 |
| MM2 | futur | Nombre de boules sans refus : catalogue tronqué publié « ok » | T0, T1 |
| CI1 | CI/dépôt | `check_docs` rouge, masque de vrais liens morts | T1 |
| CI2 | CI/dépôt | CI `main` rouge depuis le 19 juillet (hors v10) | T1 |
| TT1 | HEAD | Condensation : départs de points hors contrôle `min_cluster_size` (non rejoué ici) | T2 |
| PR1 | plan | Ordre des paliers de précision : B21 contre décision u24 (incertain) | T3 |

### 4.3 Mineurs (51), par portée

| Portée | Lignes |
| --- | --- |
| HEAD produit | CL2 options admises à tort ; CL3 lecteur u32le ; TT2 domaine de la tête ; TT3 `validate()` ; TT4 `allow_single_cluster` ; TT5 coût quadratique ; TT6 rang exact non publié ; PL1 exceptions du pool ; ST1 centre lointain SiteTree ; ST2 SiteTree à 19–21 bits ; FP1 gardes flottantes |
| HEAD tests et bancs | PT1 section 3 non bornée ; PT2 labels `fast` ; PT3 pas de `-O` en CTest ; PT4 vert par vacuité ; PT5 Python non requis ; PT6 test natif non enregistré ; JG1 juges faibles ; BN2 enfant orphelin ; BN4 runner S5 ; CU1 `cuda_probe.py` |
| Copies R2 | PL2 réserves pool ; JG2 décisions du juge de tour et lecteur en flux ; JG3 drapeaux des juges ; BN3 réserves bancs ; RC7 conflits entre séries ; RC8 différentiels privés |
| Prototypes et banc frontière | PF1 CSR parallèle ; FR1 campagne jamais exécutée ; FR2 bord de bande ; FR3 dépendance à l'univers ; FR4 différés à K5 ; FR5 bras réfutés ; FR6 provenance codée en dur ; FR7 croisement K2/K3 |
| Futur (précision) | PR2 `level4` ; PR3 côté q3 ; PR4 orientation q4 ; PR5 marge et arrondi ; PR6 retours ignorés ; PR7 unité de la garde de tête |
| Futur (échelle) | MM3 refus tardif des cellules ; MM4 note de décision massive ; MM5 masques de coquilles |
| Doc, CI, reçus | DC1 « amas discrets » ; DC2 limites statistiques ; DC3 Bayes ; DC4 PASSATION ; DC5 réponses aux audits ; CI3 workflow v10 ; CI4 preuves ignorées |

### 4.4 Infos (29)

AT2 contrat à l'égalité ; PT7 `-B` ; PT8 code 1 des sanitizers ; PT9 `log_path` et UBSan ;
PR8 raccourci mort de `level_at_most` ; PR9 Morton ; PR10 K-NN ; PR11 `cloud.bits` ;
PR12 manifeste ; PR13 similitude sans plancher ; PR14 faits établis ; PR15 commentaires ;
FR8 témoins invalides ; FR9 marge ; FR10 générateur privé ; FR11 préreg ; FR12 ε en mm ;
MM6 formules ; MM7 `--repeat` ; MM8 gardes d'indices ; MM9 coût d'un élargissement u64 ;
MM10 certificat Q×Z ; RC9 ordre des raisons ; RC10 relances de mutants ; CI5 manifestes ;
CU2 sonde CUDA ; PF2 FULL K5 en 204–254 ms ; DC6 navigation ; ER1 errata des rapports.

### 4.5 Arbitrages de gravité

Même défaut, gravités différentes selon les vérificateurs :

| Ligne | Gravités proposées | Choix | Raison |
| --- | --- | --- | --- |
| MM2 (boules) | CARD-01 majeur ; ML-3 et MAS-01 mineur ; MAS-03 info | majeur | Un catalogue tronqué sort « ok » ; premier mur u32 du chemin `mhgp10_cluster`. Reste une garde future : la RAM tombe avant. |
| MM1 (mémoire) | CARD-02 majeur ; MAS-04 mineur | majeur | Premier mur réel ; SIGABRT au lieu d'un refus ; l'en-tête affirme un budget honnête. |
| SO1 (sorties) | CLI-01 et CLI-09 majeur ; R2-05 mineur | majeur | La collision seule exige deux fois le même chemin ; mais entrée = sortie et `/dev/full` à code 0 sont plus larges. |
| CI1 (`check_docs`) | S1 majeur ; DOC-03 mineur | majeur | Il cache déjà un vrai lien mort ; nul pour le produit. |
| FR7 (croisement K2/K3) | FRO-03 mineur ; F12 info | mineur | Une phrase de politique est due même à K fixé. |
| TT6 (rang exact) | TET-06 mineur ; PREC-10 info | mineur | Comportement voulu, mais la promesse n'est pas tenue. |
| RC5 (portes `fast`) | R2-03 majeur ; ML-4 mineur ; F9 info | majeur | L'effet sur l'arbre intégré est un échec de `ctest -L fast`. |

Rétrogradations notables par les vérificateurs : les trois bloquants de la lentille R2
deviennent majeurs ; les constats de précision deviennent mineurs ou infos (gardes
futures), sauf PR1 qui reste majeur et incertain (plan) ; les six majeurs de la lentille
frontière deviennent mineurs, infos ou réfutés (banc expérimental, bras privés).

### 4.6 Constats écartés

F5 (export JSON à 32 000 K5 « ne tient pas ») : réfuté par mesure. S10 (« seul
`check_docs` lit la v10 ») : faux, `check_scope` aussi. CARD-08, partie K=1 : la garde des
représentants par ordre borne la forêt K=1. Détail : suivi, section « Constats écartés ».

## 5. Décisions de détail prises par le développeur

Consigne du 22 septembre : trancher les détails soi-même, avec une raison écrite.

| Sujet | Décision | Raison |
| --- | --- | --- |
| Feuille du juge catalogue (RC6) | `max(8, K+3)` | Suit la règle produit ; inchangé pour K ≤ 5 ; évite le mode diagnostic dans un oracle. |
| Convention d'attache à l'égalité (AT1, AT2, JG2) | fermée, `niveau(v) <= e < niveau(parent(v))`, jugée en exact dans le juge de la tour ; `validate()` inchangée | HEAD la respecte partout ; mreach et les collisions de doubles produisent l'égalité légitimement. |
| Naissances de durée nulle (JG2) | juge strict : une fusion a au moins deux enfants nés strictement avant | Aucune famille de la campagne des oracles n'est touchée. |
| Atomicité de `mhgp10_cluster` multi-K | tout ou rien sur tous les ordres | Doctrine : jamais un préfixe de sortie publié. |
| Ordre des raisons (RC9) | `input_unreadable`, `node_budget`, `output_unwritable`, `numeric_domain` | Proposition du plan R2 ; départage seulement à K égal. |
| Portes `fast` de HEAD (RC5) | sous `run_expect.cmake` | Recette validée dans l'essai D. |
| `check_docs` (CI1) | exclure les copies de provenance sous `receipts/`, sans réécrire les reçus ; périmètre de `check_scope` inchangé | Les reçus clos ne se réécrivent pas ; `check_scope` doit continuer à voir tout le corpus. |
| Racine singleton au niveau 0 | refusée (`numeric_domain`, `"k":1`) | Choix de la copie `tete`, accepté par le coordinateur ; sklearn exige `min_cluster_size >= 2`. |
| Candidat frontière | B_η^min (antichaîne), η ∈ {1/32, 1/8} ; B_η en contrôle ; A2 aux η préenregistrés | Invariance par l'univers ; jamais plus tardif. |
| Premier palier de précision | u24, avec le contenu B21 en étape interne P1a | Décision du 30 septembre ; un palier public B21 demande un accord. |

## 6. Questions pour l'utilisateur

**Chemin de précision.** La décision du 30 septembre (grille u32 par paliers u24 puis u32,
float32 natif non développé) remplace la préférence du 21 septembre (float32 sans perte,
grille en option). L'audit s'y conforme et ne rouvre pas ce choix. Faits utiles si vous
voulez le revoir : le float32 sans perte demanderait sur ces trames une grille dyadique de
45 à 49 bits, donc hors du chemin u32 ; aucune chaîne FULL float32 n'existe en v8 ; la grille
u32 couvre 1 µm pour une trame (B28) ; le capteur HDL-64E est précis à environ 2 cm.

Questions ouvertes :

1. **Palier public B21.** Faut-il livrer d'abord un palier public B21 (0,1 mm pour une
   trame, refus levé à 21 bits), ou garder B21 comme étape interne du palier u24 ?
2. **Pas par défaut.** Une fois u24 qualifié, garder 1 mm par défaut et offrir 0,1 et
   0,01 mm, ou passer le défaut à 0,1 mm ?
3. **Cible du palier u24.** Qualifier d'abord une trame à 0,01 mm, ou des cartes de plusieurs
   trames à 0,1 mm (jusqu'à 1,68 km par axe) ?
4. **Ordre des tranches.** Le plan fait : raccord, gardes, correctif de tête, palier u24, puis
   campagne frontière. La campagne frontière (le « grand verrou ») peut passer avant le palier
   u24 : elle n'en dépend pas. Quel ordre préférez-vous ?
5. **Régime massif.** Les 10–50 M points restent-ils secondaires ? Si oui, le plan se limite
   aux gardes (refus propres) sans segmentation. Sinon, quelle RAM cible (176 Gio de la G4 ou
   plus) ?
6. **Hiérarchie de points.** Confirmez-vous la cible « arbre d'un seul K » ? Une combinaison
   multi-K demanderait une politique déclarée (croisement K2/K3 démontré).

## Annexe — reproduction

Chaque lentille donne ses commandes (sections « Reproduction » de ses rapports). Preuves
courtes : `*/preuves/`. Builds et brouillons lourds : `/tmp/mhgp10-audit-geant/<lentille>/`
(non durables). Aucune graine `test` ni `test_v10b`. Aucun fichier du worktree partagé
modifié. GCP non utilisé.
