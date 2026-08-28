# MorseHGP3D v5 — Passage à l'échelle : dizaines de millions de points sur une G4

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`. Ce document
est une **conception chiffrée**, pas un claim : chaque chiffre cite un reçu ou
une mesure locale (ratios), et les théorèmes à prouver sont listés avant le
code (§ 6). Rédigé le 28 août 2026 à partir des reçus G4 n° 9–11, des mesures
locales du 28 août et des audits v3/v4 cités au § 2.

## 1. Les deux objectifs

1. **Principal — objet complet, `smax = 11`, K = 1..10** (dix forêts, profil
   d'entrée u16 inchangé), sur des nuages de 10 M à 30 M points.
2. **Secondaire — K = 1..5** (`smax = 6`, $h_q = 7 - q$), même code, profil
   **nommé** dans les reçus (jamais confondu avec l'objet complet ; un préfixe
   exact de la tour, § 3.3).

Machine cible (lecture seule du 28 août 2026) : `g4-standard-48` = 48 vCPU,
**180 Gio** de RAM, 1 × RTX PRO 6000 (96 Go), un disque persistant NVMe de
**100 Go** ; aucun SSD local attaché. Sessions gardées de **8 h au plus**
(`maxRunDuration`), donc tout calcul plus long doit être **reprenable**
(statut `incomplete_continuation`, flux sur disque persistant survivant à
l'arrêt).

## 2. Ce que la v3 et la v4 avaient déjà établi

- **v4, audit bloquant `C829` (17 août 2026)** — « la prochaine limite est la
  taille de l'objet, pas le prédicat » : 391 événements et 247 nœuds de fusion
  par point à 8000, ~11,7 G événements à 30 M ; il faut **nommer le produit**
  (`full_symbolic_stream` / `connectivity_hierarchy` / `query_or_labels`),
  préflight de cardinalité avant allocation, index 64 bits, plus de `ev_k`
  résident, séparer connectivité et rendu, garder `SpherePlateau` comme unité.
- **v4, contre-audit `772A8D`** — bornes de Poisson fermées : q2 seule impose
  en espérance $2K_{\max}(K_{\max}-1)$ facettes nées par point, soit **180 par
  point pour K = 10** (5,4 G à 30 M) et **40 pour K = 5** (1,2 G à 30 M) ;
  produit recommandé `facet_hierarchy_stream` (K-MST exact au niveau des
  facettes, streamé, output-sensitive), et un produit orienté requêtes (Th. 2
  du manuscrit : composantes de $L_K(r)$) comme seul candidat à un SLO court.
- **v3, réponses « route 50 k puis 10 M » (13 août 2026)** — un halo k-NN ne
  rend pas les tuiles indépendantes ; une coquille peut avoir $\Theta(n)$
  labels ; les événements inter-tuiles et les lots de même niveau exigent une
  **fusion globale** ; peuvent rester résidents : points, index, état du fold
  par ordre, fronts bornés ; les supports se streament.
- **doctrine produit** (`docs/GPU_G4_ARCHITECTURE.md`) : à 10–30 M, nuage et
  arbre résidents seulement si le préflight le démontre ; ancres, frontières,
  candidats et sorties streamés par plages Morton.

La v5 reprend ces conclusions telles quelles ; ce document ne les contredit
sur aucun point et y ajoute des mesures qui fixent les constantes.

## 3. Lois d'échelle mesurées

### 3.1 Par ordre et par point (reçu G4 n° 11, `uniform` 200 000, 48 fils)

| grandeur | total | par point | dont K = 10 |
|---|---:|---:|---:|
| boules uniques survivantes | 90 M | 450 | — |
| événements (K = 1..10) | 89,8 M | 449 | 105 |
| facettes (K = 1..10) | 562 M | 2811 | 912 |
| deltas | 62,8 M | 314 | 83 |
| attachements (naissances) | 468 M | 2340 | 787 |

Mémoire (paliers `rss_mb`) : 14 Go après génération, 34 Go après census
(boules 224 o), **66 Go au maximum du fold** (trois ordres résidents, ~11 Go
par ordre). Temps : 254 s (génération 69, préfiltre 31, census 19, mur du
fold 115 dont reduce séquentiel 1,4 µs par événement et digest 66 s cumul).

### 3.2 En fonction de l'ordre maximal (`uniform` 32 000, 8 fils, ratios)

| `smax` | K max | boules/pt | mur | pic du fold |
|---:|---:|---:|---:|---:|
| 5 | 4 | 49 | 25 s | 1,0 Go |
| 7 | 6 | 122 | 50 s | 2,4 Go |
| 9 | 8 | 243 | 96 s | 5,1 Go |
| 11 | 10 | 425 | 211 s | 8,8 Go |

Les cardinalités d'un ordre K sont **identiques** quel que soit `smax` ≥ K + 1
(vérifié à 32 000 sur `uniform` et `scanline` : K = 4 → 703 116 événements et
1 943 993 facettes dans les quatre runs) : la restriction est un préfixe exact
de la tour (§ 3.3). Le coût croît en ≈ K².

### 3.3 Le préfixe exact

La mort par profondeur $\left\vert I_B \right\vert \ge h_q = s_{\max} - q + 1$
ne tue que des boules dont tous les événements ont $K = q + d - 1 \ge s_{\max}$ ;
les ordres $K \le s_{\max} - 1$ sont donc inchangés. Un profil K = 5 est
l'objet complet **restreint** aux cinq premiers ordres, avec les mêmes
digests par ordre (`digest_forest[K]`) — la conformité du profil se prouve par
égalité de ces digests avec ceux d'un run `smax = 11` à 8000–32 000.

### 3.4 Facettes vivantes dans le fold (instrument `profil_vivantes`, 28 août)

Dans le fold d'un ordre trié par niveau, une facette n'est vivante (première
incidence passée, dernière à venir) que brièvement. Pic de facettes vivantes
par point, K = 10 : `uniform` 8000 16,8, 16 000 18,1 (+8 % par doublement) ;
`eight_clusters` 8000 13,3 ; `scanline` 8000 2,5. Fraction du total : K = 10
2,2 %, K = 4 10 %, K = 1 93 % (les singletons). **C'est la mesure qui rend un
fold à état borné possible** : à 10 M, ≈ 40 facettes vivantes par point
(marge ×2,2) contre 912 au total pour K = 10.

### 3.5 Rayons des boules survivantes (32 000, `smax = 11`)

Rayon maximal : `uniform` 22 unités (2,2 pas moyens), `scanline` 84 (2,4 pas),
`eight_clusters` **149 (7,4 pas, 23 % du domaine — les vides inter-amas)**. Un
halo par copie est donc impraticable sur les familles à vides ; la conception
laisse l'index **résident** et ne tuile que le travail (§ 4).

## 4. Architecture retenue (à prouver puis livrer, § 6–8)

### 4.1 Produit nommé

- `product = facet_hierarchy_stream` (le B de `C829`/`772A8D`) : par ordre K,
  le flux des deltas (fusions, naissances, croissances) avec leurs clés de
  facettes, dans l'ordre des lots de niveau ; c'est l'objet des applications
  (rendu § 9.1, segmentation) et il est **output-sensitive** : à 10 M et K = 10,
  ≈ 2340 naissances/pt × 45 o ≈ **1 To** de clés nées ; K = 5 : ≈ 0,1 To.
- Le digest v4 (`facet_keys` triées + `final_canon_fid` par facette + deltas)
  reste la **porte de conformité** jusqu'à 200 k–1 M (calculable en résident)
  ; à l'échelle, un **digest v5 de flux** versionné (`mhgp5-digest-v1:stream`)
  signe le flux des deltas et des naissances par ordre, et son égalité avec le
  digest v4 recalculé est prouvée aux tailles où les deux existent.
- Statut public : jamais `exact` ; `complete_regular` pour le produit nommé,
  `resource_exhausted/<rôle>` avant toute allocation qui dépasserait le
  préflight, `incomplete_continuation` à la coupure de session.

### 4.2 Résident, streamé, externe

| rôle | 10 M, K = 10 | 30 M, K = 10 | 10 M, K = 5 |
|---|---:|---:|---:|
| index radix + positions (résident) | 1,7 Go | 5,1 Go | 1,7 Go |
| rectangles WSPD vivants (résident ou par vague) | ~4 Go | ~12 Go | ~1 Go |
| tuile de travail (boules 224 o, événements, tri) × 2 | ~2 × 10 Go | idem | idem |
| flux d'événements par (K, niveau) sur disque (≈ 52 o/évt) | ~230 Go | ~700 Go | ~40 Go |
| fold K = 10 : état vivant (76 o) | ~30 Go | ~90 Go | K = 5 : ~10 Go |
| fold K = 10 : table des comptes de dernière incidence (7 o × facettes) | ~64 Go | ~190 Go ✗ | ~4 Go |
| sortie `facet_hierarchy_stream` (disque) | ~1 To | ~3 To | ~0,1 To |

Lecture : **K = 5 à 10 M tient dans 180 Go et sur un disque de ~150 Go** ;
**K = 10 à 10 M tient en RAM (pic ≈ 100 Go) mais exige ≈ 1,3 To de disque**
(flux + sortie) ; **K = 10 à 30 M** exige en plus un comptage externe des
dernières incidences (la table ne tient plus) et ≈ 4 To de disque — hors de
portée de la VM actuelle sans changement de configuration.

### 4.3 Les trois étages

1. **Génération tuilée par plages Morton du centre des boules.** L'index reste
   résident ; les rectangles WSPD vivants sont attribués à la tuile de leur
   boîte de milieux ; chaque boule émise est **routée vers la tuile de son
   centre exact** ($c = -B/(2A)$, cellule Morton calculée en i128 : fonction
   pure de la `BallKey`), ce qui rend le RLE par tuile **globalement exact**
   (deux émissions d'une même boule se rencontrent dans la même tuile). Par
   tuile : RLE, préfiltre, census contre l'index global, expansion en
   événements par K, tri par niveau exact, écriture de *runs* sur disque.
   Mémoire par tuile bornée (≈ 200 k boules), indépendante de $n$.
2. **Fold streamé par ordre, état borné.** Pour chaque K : fusion k-aire des
   runs de niveau (lecture séquentielle), lots de niveau exact, union-find sur
   les seules facettes **vivantes** (identifiant attribué à la première
   incidence, clé canonique portée par la racine, deltas émis avec les clés
   recalculées depuis l'événement), **oubli** d'une facette à sa dernière
   incidence (table des comptes construite pendant l'expansion : empreinte
   64 bits → compte u8), compaction périodique de l'état. Sortie : flux de
   deltas de l'ordre K sur disque + digest de flux.
3. **Reprise.** Les runs et les sorties sont sur le disque persistant ; un
   manifeste (pin, tailles, sha256 des runs, ordre courant, lot courant) permet
   de reprendre un fold interrompu au lot suivant : `incomplete_continuation`
   n'est jamais un préfixe publié comme résultat.

## 5. Modèle de temps (constantes du reçu 200 k, 48 fils)

| poste | 200 k mesuré | 10 M K = 10 (estimation) | 10 M K = 5 |
|---|---:|---:|---:|
| génération (lanes q2/q3/q4) | 69 s | ~75–100 min (×50, +30 % tuilage) | ~15 min |
| RLE + préfiltre + census | 58 s | ~60 min | ~8 min |
| expansion + tri + écriture des runs | ~15 s + E/S | ~30 min (230 Go à ≥ 1 Go/s) | ~5 min |
| fold (reduce 1,4 µs/évt, digest) | 115 s mur | ~3–4 h (4,5 G évts séquentiels par ordre, ordres en parallèle bornés par la RAM) | ~25 min |
| **total** | 254 s | **≈ 6–7 h** (une session de 8 h, sans marge) | **≈ 1 h** |

Les hypothèses à mesurer avant tout engagement : débit du disque persistant
(1–2 Go/s en séquentiel pour un PD-SSD de taille suffisante ; le disque de
100 Go actuel est trop petit et trop lent), constante du reduce à état borné
(l'état compact pourrait la faire baisser sous 1 µs), surgénération aux
frontières de tuiles.

## 6. Théorèmes à prouver avant le code

1. **Routage par centre.** Deux émissions d'une même boule (même `BallKey`)
   ont le même centre exact, donc la même tuile : le RLE par tuile est
   équivalent au RLE global ; la cellule Morton du centre écrêté est une
   fonction pure de la clé (dénominateur $2A$, pgcd, i128 sans débordement).
2. **Complétude par tuile.** Toute boule dont le centre est dans la tuile est
   émise par un rectangle attribué à la tuile ou à une tuile voisine à
   distance $\le R_{\max}$ (borne exacte du rayon admissible d'une ancre :
   $R \le \sqrt{3/8}\,D$) ; les rectangles sont attribués avec ce recouvrement
   et l'exact-once des seeds q4 est conservé.
3. **Dernière incidence.** Le nombre d'incidences d'une facette dans le flux
   d'un ordre est exactement le compte accumulé pendant l'expansion (même
   fonction `facet_minus`, même règle de rôle) ; à la dernière incidence, la
   facette n'est plus jamais référencée : elle peut être compactée si elle
   n'est pas racine d'une composante à membres vivants (les racines gardent la
   clé canonique).
4. **Ordre du fold.** Les lots de niveau et l'ordre des ex aequo (tri stable
   par rang d'entrée) sont reproduits par la fusion k-aire des runs avec une
   clé d'ex aequo explicite (niveau exact, rang de boule, sous-index) : les
   deltas et leur ordre par racine sont bit-identiques au fold résident.
5. **Digest de flux.** Le digest v5 est une fonction du flux des deltas et
   des naissances par ordre ; son égalité avec le digest v4 recalculé à
   200 k–1 M (facettes triées, partition finale) prouve l'objet ; au-delà,
   c'est le digest v5 qui fait foi, avec le produit nommé.

## 7. Portes

- égalité **tuilé / non tuilé** (boules uniques, `digest_balls`, `digest_all`)
  à 8000–32 000 localement (cinq familles + contre-familles) et à 200 k puis
  1 M sur G4 ; mutants `route-by-anchor`, `no-clamp`, `forget-early` (oubli
  avant la dernière incidence), `tie-order` (ex aequo) tués ;
- préflight par rôle (majorants publiés, `MemAvailable`, `statvfs`) et refus
  `resource_exhausted/<rôle>` avant allocation ; porte à budget artificiel ;
- reprise : coupure injectée à un lot, reprise, digest identique ;
- invariants globaux à l'échelle (sommes de cardinalités par ordre, comptes
  de naissances contre la borne de Poisson q2, juge d'échantillon sur des
  boules tirées au sort) — jamais un juge $O(n^3)$.

## 8. Ordre des livraisons

1. **Livré (28 août, `profil=` dans la sortie, `tests/prefix_gate.cpp`)** :
   profil nommé K = 5 (`smax = 6`) ; porte de préfixe (digests et
   cardinalités par ordre égaux à ceux de `smax = 11` sur trois familles,
   K = 4 aussi ; mutant `anchor-kill-h-minus-one` tué). À 1200 `uniform` :
   78 k boules au lieu de 382 k.
2. Instrument de réception : pic de facettes vivantes et comptes d'incidence
   par ordre (32 000 local, 200 k G4).
3. Routage par centre + RLE par seau (théorème 1) ; portes d'égalité.
4. Génération tuilée + runs de niveau sur disque (théorème 2) ; préflight par
   rôle ; égalité tuilé / non tuilé à 200 k sur G4.
5. Fold streamé à état borné (théorèmes 3–4) ; digest de flux (théorème 5) ;
   reprise.
6. Session G4 à 1 M (K = 5 puis K = 10) avec disque dimensionné ; puis 10 M
   K = 5 ; puis 10 M K = 10 (deux sessions avec reprise si nécessaire).

## 9. Questions à trancher par l'utilisateur

1. **Disque** : ajouter un disque persistant SSD (≥ 500 Go pour 10 M K = 5 ;
   ≥ 2 To pour 10 M K = 10) à la VM — c'est une mutation hors des scripts
   gardés actuels, à ajouter à `gcp-migration/` avec double coupe-circuit.
2. **Produit** : le flux des deltas par ordre (`facet_hierarchy_stream`)
   est-il la sortie attendue à l'échelle, ou faut-il aussi le rendu § 9.1 pour
   un $\psi$ fixé (accumulé en flux, sans série symbolique) ?
3. **30 M, K = 10** : accepter que cela exige un comptage externe et ≈ 4 To,
   ou viser d'abord 10 M pour K = 10 et 30 M pour K = 5.
