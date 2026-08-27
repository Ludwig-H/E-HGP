# PASSATION — état complet du chantier morsehgp3D_v4 au 18 août 2026

Document d'entrée pour la session qui reprend (Opus 5). Il relie les
trois plans — mathématique, implémentation, opérationnel — et pointe
vers les sources qui font foi. Rien ici ne remplace un document
normatif ; tout y renvoie. Pin de rédaction : `main@5c326c1`.

> **ERRATUM du 18 août (session de reprise).** La rédaction initiale
> déclarait ouverts deux chantiers déjà livrés dans `main` :
> la garde de capacité du fold (`093abed`, porte `--fold-capacity-gate`,
> trois mutants) et l'étage d'intervalles de Jung (`4df9a39`, porte
> `--q3-affine-gate` étendue, mutant `jung-swap-bounds`). Le § 2.7 et
> le § 5 ci-dessous sont corrigés en conséquence. Le compte de CTests
> était 128, pas 130.

## 0. Où commencer, dans l'ordre

1. `AGENTS.md` (racine) puis `CLAUDE.md` — règles absolues (jamais de
   branche sans accord ; VM GCP uniquement par scripts gardés ; jamais
   de vérification exhaustive ; tailles d'intérêt 8000/16000/32000).
2. Ce document (vue d'ensemble et carte).
3. `docs/MATHEMATIQUES.md` — l'objet et les statuts de preuve.
4. `docs/ARCHITECTURE.md` — le plan initial (⚠ antérieur à une grande
   partie du pipeline réel ; le présent § 3 fait la mise à jour).
5. `audits/` en ordre chronologique inverse — le dialogue avec les
   auditeurs (qui poussent sur `main`) et ses réponses EXÉCUTÉES.
6. `receipts/` — chaque avancée a son reçu, ancré à un commit.

## 1. L'objet mathématique en bref

La hiérarchie HGP (manuscrit Hauseux, Défs 20–31, Th. 2–7 — pages PDF
35–134) : pour chaque ordre $K$, une forêt de fusion sur les
$(K-1)$-simplexes, dont les événements sont portés par des BOULES : une
boule $B$ contribue si son intérieur strict $I_B$ et sa coquille $U_B$
déterminent des plateaux sphériques. Réduction v4 : trois lanes de
génération suffisent —

- **q2** : boules diamétrales (paires) ;
- **q3** : circumboules de triangles aigus (l'arête max est l'ancre) ;
- **q4** : circumboules de tétraèdres (seed aigu + complétion).

Complétude sous seuils (`derive_v4`, § en tête de
`src/pipeline/ball_stream.hpp`) : un plateau pertinent pour
$K \leq 10$ a $\vert I_B\vert \leq K_{max} + 1 - q$ ; ses témoins de
fuseau $W_q(a,b)$ sont donc sous $h_q = s_{max} - q + 1$ : les filtres
$h_{coeur}/h_a/h_b$ ne perdent AUCUN plateau pertinent (Carathéodory
garantit un support minimal d'arité 2, 3 ou 4). Le rendu § 9.1 du
manuscrit : l'arbre est une partition laminaire des facettes, la
partition de l'unité $w_{x\tau} = S_\tau/T_x$ relie points et facettes.

Doctrine d'exactitude : entrée u16 quantifiée, TOUT prédicat décidé en
entier (i64/i128/U192/U320), aucun jitter, les dégénérescences
(plateaux cosphériques) sont réelles et traitées par quotient exact.
Le flottant n'existe que comme FILTRE certifié à repli exact (§ 2.4).

## 2. Les résultats acquis (théorème → code → porte)

Chaque ligne : l'énoncé, où il vit, la porte qui le grave. Les
dérivations complètes sont dans les reçus (`receipts/forest_20260817/`)
et les contre-audits.

### 2.1 Cover coefficient 3 et bornes de site

Le cover d'ancre $(a,b)$ contient tout $z$ avec
$\vert 2z - a - b\vert^2 \leq 3 D^2$ ; donc $\vert z-a\vert^2 < 2D^2$
(contre-audit `04c71a2` § 1, dérivation $((\sqrt{3}+1)/2)^2 < 2$).
Code : `rect_cover_handles` / `anchor_cover_from_handles`
(`src/events/edge_cover.hpp`). Portes : juges des petits n +
`--depth-gate`.

### 2.2 Cœur de seed (Jung)

Tout tétraèdre accepté a $R^2 \leq 3D^2/8$ ; un site $z$ avec
$P(z) < 0$ et $2P(z)^2 > J\,B(z)^2$ (où $J = D^2(3G - 2 E X)$) est
intérieur strict de TOUTE sphère admissible du seed — comptage
fail-open à $h_4$, l'ancre entière meurt par $W_4$. Comparaison exacte
en U320 (`cmp_2p2_jb2`). Code : lane q4 de `collect_candidate_balls`.
Portes : `--axial-pair-gate` + mutant `seed-core-nonstrict`.

### 2.3 Kernel affine par ancre (le chemin chaud actuel)

Par ancre : $u_z = 2z - a - b$, $q_z = \vert u_z\vert^2 - D^2$ —
entiers $< 2^{36}$, EXACTS en binaire64, remplis paresseusement au
premier seed (`LaneScratch::fill_affine_sites`). Par seed :
$N = W - G\,d$. Identité $L(z) = G q_z - 2\,u_z \cdot N = 4 P(z)$
(preuve : $P(a) = P(b) = 0 \Rightarrow d \cdot N = 0$), donc
$P = L/4$ exact pour Jung. Porte PERMANENTE `--q3-affine-gate` :
identité + divisibilité sur ~1,94 M de triples exhaustifs, témoin de
forte annulation ± gravé ($G = 2^{67} - 12345$, $L = +216577/-45565$).
Reçu : `ADDENDUM_KERNEL_AFFINE_20260818.md`.

### 2.4 Étage flottant certifié (borne PROUVÉE)

Séquence FIGÉE `affine_l_hat` (quatre arrondis, le doublement est
exact) ; erreur $< 6u\,M(z)$ avec $u = 2^{-53}$ ; seuil par seed
$E = 2^{-48} \cdot (G\,q_{max} + 2\vert N\vert_1 u_{max}) > 31u\,M$ —
marge $\times 5$, preuve REÇUE par les deux contre-audits `879B37`.
Décision : $\hat{L} < -E$ ou $> E$ certifie le signe, sinon repli
exact i128. Garde d'exécution : filtre coupé sous `__FAST_MATH__` et
si `fegetround() != FE_TONEAREST` (porte `--float-rounding-gate`).
Jung et `cmp_mu` n'utilisent PAS ce seuil (chantiers § 5.2).

### 2.5 Fold compact

Canonique par min-fid u32 (≡ min FacetKey car les fids suivent l'ordre
des clés), tables à époque pour les racines pré/post-lot, partition
finale DENSE (`facet_keys` + `final_canon_fid`, invariants
structurels), backend FIGÉ `build_forest_legacy` comme témoin. Porte
`--fold-compact-gate` (égalité complète, mutant `canonical-is-uf-root`).
`t_fold` 56,1 → 38,3 s à n=8000 ; l'internement (~26 s) est le poste
suivant (§ 5.3).

### 2.6 Parallélisme MESURÉ, jamais déclaré

Génération : tirage dynamique par rectangle (`run_rects`), compteurs
PAR LANE alimentés au point de création des `std::thread` ; aval :
`parallel_ranges` RETOURNE le nombre créé ; affinité CPU effective
publiée par `sched_getaffinity`. Sorties bit-identiques quel que soit
le découpage (tri stable + RLE ; fusion en ordre de tranche). Portes :
`--par-gate`, `--workers-gate` (mutants `parallel-hardcodes-one-worker`,
`parallel-ranges-one-worker`, `q3-one-worker`, `wspd-one-worker`),
validateur de campagne. Depuis le 19 août, la **descente WSPD** est
elle aussi parallèle et ses ouvriers sont mesurés par lane
(`wspd_q2/q3/q4`) : elle était le poste dominant de `t_gen` (72 %) et
n'était ni parallèle ni chronométrée — § 2.12.

### 2.7 Poisson q2 et contrat de sortie

$E[N_j]/n \to 4$ par profondeur ; ~$2K(K{-}1)n$ facettes nées ;
`--q2-birth-gate` (l'égalité du second diamètre est un impossible-
théorème, donc une violation). Preflight `--output-preflight-only` +
plafond transactionnel `--max-output-bytes` (refus code 2 AVANT
allocation).

### 2.8 Garde de capacité du fold (LIVRÉE, `093abed`)

Refus `resource_exhausted/requires_tiling` AVANT tout cast et toute
allocation du fold : `evenements <= UINT32_MAX` (`FRec::e`/`ev_fid` en
u32), `Σ(q_e + d_e) <= INT32_MAX` (majorant de `nfid`, union-find i32),
`lots < UINT32_MAX` (sentinelle des tables à époque). Porte
`--fold-capacity-gate` : bases FICTIVES près des limites (jamais
d'allocation géante), 4 refus / 4, 3 cas juste sous la limite au
résultat identique, trois mutants tués (`fold-u32-event-wrap`,
`fold-i32-fid-wrap`, `fold-epoch-sentinel-collision`). Reçu :
`ADDENDUM_GARDE_CAPACITE_FOLD_20260818.md`.

Distinction exigée par l'audit `dd0d4a6` § 1, à ne pas perdre :
**DONE** = garde des index locaux u32/i32 (plus aucune troncature
silencieuse) ; **OPEN** = le tuilage/streaming qui permettrait de
DÉPASSER ces limites au lieu de refuser. La sécurité est fermée, la
capacité ne l'est pas.

### 2.9 Étage d'intervalles de Jung (LIVRÉ, `4df9a39`)

`jung_interval_sign` : sur un site certifié `P < 0`, la séparation des
intervalles flottants certifie `2P² > J B²` ou son contraire sans
i128/U320 ; les égalités tombent TOUJOURS dans le repli exact. Mesure :
les ~1,35 G de `cmp_2p2_jb2` U320 du cœur q4 tombent à 80 replis
(eight_clusters) et 145 (uniform) ; `t_gen` 127,0 → 123,1 s
(eight_clusters), 57,3 → 55,5 s (uniform). Mutant `jung-swap-bounds`
tué par un témoin GRAVÉ à cheval sur la fenêtre. Reçu :
`ADDENDUM_INTERVALLES_JUNG_20260818.md`.

### 2.10 Internement du fold en streaming (LIVRÉ, session du 18 août)

`build_forest` n'alloue plus le tableau des incidences (facette,
événement, slot) ni le tampon de fusion de `stable_sort` : chaque
facette est internée à la volée (table d'adressage ouvert, comparaison
EXACTE de clé, dimensionnée une fois sur le majorant des incidences),
puis les clés UNIQUES seules sont triées. L'invariant public — fid
croissant ⟺ FacetKey croissante, donc canonique = min-fid — ne dépend
PAS du hachage : le tri final est la seule autorité d'ordre. Le backend
FIGÉ `build_forest_legacy` garde le tri global et sert de témoin
(`--fold-compact-gate`, planchers d'incidences/facettes/lots).

Gain **mesuré par banc apparié contrebalancé** (`--fold-intern-bench`,
dix paires ABBA, échauffement, signature vérifiée) : médiane des
rapports appariés **0,8769 → ×1,14** sur l'internement du K dominant à
n=8000, dix victoires sur dix (`P = 1/1024`). Le rapport de médianes
marginales n'est PAS l'estimateur (il donne ×1,13 ici, ×1,32 sur une
autre série, ×1,08 sur les cinq paires biaisées du 18 août). Reçus :
`ADDENDUM_INTERNEMENT_STREAMING_20260818.md` (structure et mémoire, ses
facteurs temporels sont RETIRÉS) puis
`ADDENDUM_BANC_APPARIE_ET_ORDONNANCEMENT_20260819.md` (protocole
corrigé, chiffres retenus).

### 2.11 `first_batch` hors sémantique, ordonnancement à budget (LIVRÉ, 19 août)

`first_batch` n'était pas une entrée du calcul : sur un flux sans
`attach_violations`, `S_b(f) ⟹ A_b(f)`. Il est supprimé (−74,3 Mio
touchés et une écriture aléatoire de moins par sondage réussi) et
remplacé par un bit `seen` mis à jour APRÈS le lot, qui n'alimente que
les deux compteurs. Mutants `attach-detector-disabled` et
`seen-before-check`. `build_forest` reçoit ses événements par
référence et trie une permutation compacte d'indices (fin des ~400 Mo
de copies cumulées). L'ordonnancement des dix folds passe de tranches
contiguës (un ouvrier portait 69 % du travail) à `memory_budgeted_LPT` :
latence /1,40 pour +0,4 % de pic RSS, réserve sous le plafond déclaré ;
`LPT_unbounded` (/2,80) reste une BORNE, jamais un défaut — il réserve
deux fois le budget. Signature identique aux trois modes. Reçu :
`ADDENDUM_BANC_APPARIE_ET_ORDONNANCEMENT_20260819.md`.

### 2.12 Descente WSPD parallèle (LIVRÉE, 19 août) et charge q3 mesurée

`wspd_alive` produisait les rectangles vivants de chaque lane en
SÉQUENTIEL, hors de tout chrono : **52,5 s des 72,8 s de `t_gen`** à
n=8000. Le traitement d'un rectangle étant pur, la vague est découpée
en **tranches ordonnées** — chaque tranche écrit ses tampons, la
concaténation se fait en ordre de tranche, la sortie est bit-identique
au séquentiel. Mesure : descente ×3,41, **`t_gen` 72,8 → 35,3 s
(×2,06)**, `boules_uniques` et les dix cardinalités inchangées.

Les compteurs de charge par ancre (`q3_charge*`) réfutent au passage
l'index par couches convexes : le scan q3 évalue **10,3 sites par
seed** (sortie anticipée à `h_3`), un index coûterait ~830 opérations
de construction contre 351 sites scannés par ancre, et les ancres à
≥ 128 seeds ne portent que 4 % du travail. Reçu :
`ADDENDUM_DESCENTE_WSPD_ET_CHARGE_Q3_20260819.md`.

### 2.13 Préfiltre q4 par puissance équatoriale (LIVRÉ, 19 août)

Un tétraèdre est strictement bien centré **ssi** chacun de ses sommets
est strictement extérieur à la boule équatoriale de la face opposée
(réponse d'audit `5b89bc6`, lemme redéroulé au reçu). Chaque face prise
seule est donc une condition **nécessaire** — un préfiltre exact, en
seules longueurs carrées, sans centre ni division, en i128
(`< 2^105,4`, borne dérivée ici).

Seule la face `abx` est branchée. **Elle n'a pas de primitive propre** :
le contre-audit `7420355` a montré que $\mathrm{Pow}_{abx}(y) = 2 \lambda_y h^2$,
donc que son signe **est** l'un des quatre signes barycentriques que
`q4_center_strictly_inside` calcule déjà, et que `q3_power(f3s, y)` — la
forme du seed, construite une fois par seed — le donne à un facteur
$G > 0$ près. La formule en six longueurs écrite le 19 août est
**rétrogradée en oracle croisé** (`equatorial_power4` dans
`mhgp4_q4_oracle`), plus jamais sur le chemin de production.

Devant elle, un **étage zéro purement i64** (addendum `6a2bc96`) :
$2 \max(l_{ay}, l_{by}, l_{xy}) > D^2$ et
$\max(l_{ax}+l_{ay}, l_{bx}+l_{by}) > D^2$, deux conséquences nécessaires
du bien-centrage en `max/add/compare`.

Rendement à n=8000 : sur **87 499 759** paires entrantes, l'étage i64 en
retire **13 353 260** (15,3 %) avant tout `i128`, la puissance
43 921 721 de plus ; total **57 274 981** = **80,65 %** des rejets du
centre, **zéro faux rejet**.

**Deux compteurs, parce que le contrat a deux moitiés** : `faux_rejets`
voit une garde trop agressive ; seul `frontieres_manquees` voit une garde
trop **permissive**, qui n'a rien tué à tort mais a omis de tuer la
frontière $\mathrm{Pow}=0$ (« centre dans le plan de la face »). C'est ce
second compteur qui a imposé un quatrième nuage de porte — 200 points
dans $14^3$ sites, où les quadruples cosphériques sont fréquents ; sur
les trois emprises par défaut la frontière n'apparaît que 17 fois et le
mutant `nonstrict` n'était **pas** discriminé.

Quatre mutants tués en code 4 : `seed-face-power-nonstrict` (1 524
frontières manquées), `seed-face-power-sign` (657 986 faux rejets),
`seed-i64-vertex-y-drop-factor`, `seed-i64-pair-xy-min`.

**Le ×1,042 publié le 19 août est retiré** : le bras témoin du banc
calculait le préfiltre puis jetait le résultat — « court-circuité »
contre « calculé puis jeté », pas « avec » contre « sans ». Bancs
appariés corrigés (n=8000, dix paires contrebalancées) :

- **cascade complète contre aucun préfiltre** — médiane appariée
  **0,9479 → ×1,055**, 9 victoires sur 10 ($P = 0{,}011$). Le préfiltre
  vaut son coût, contre un chemin qui existerait sans lui.
- **cascade complète contre puissance seule** (l'étage i64 vaut-il son
  coût ?) — médiane appariée **1,0021**, 8 victoires sur 20 à vingt
  paires. **Aucun gain de temps mesurable.** À dix paires le même banc
  affichait ×1,013 et 8/10 : c'était du bruit. L'étage est conservé sur
  un argument *compté* et non chronométré — il retire ~40 millions de
  multiplications `i128` du chemin chaud, ressource rare du port GPU —
  et un seul mode le supprime.

Reçus : `ADDENDUM_PREFILTRE_Q4_EQUATORIAL_20260819.md` (le lemme, l'oracle
et les planchers ; son § 5 est retiré par le suivant) puis
`ADDENDUM_FRONTIERE_STRICTE_ET_ETAGE_I64_20260819.md`.

### 2.14 Plafond de sortie — corrigé DEUX fois (19 août, puis 27 août)

> **Le § ci-dessous est celui du 19 août. Il est FAUX et conservé pour
> mémoire ; lire d'abord le § 2.15.** L'audit `bab37b9` § 4 a montré que
> la borne `min(max(budget, max_K m_K), Sigma_K m_K)` est correcte pour
> la variable d'ordonnancement `reserved` mais ne l'est pas pour la
> résidence : les résultats des folds terminés restent vivants alors que
> leur $m_K$ sort du compte.

### 2.14bis Version du 19 août (rétractée)

Le plafond `--max-output-bytes` décidait sur
`evenements * sizeof(ForestEvent)` **seul**, alors que le préflight
publiait déjà des bornes par tampon cinq à six fois plus grosses. À
n=8000 : **450 Mo** de flux contre **2,60 Go** de pic — un plafond réglé
à 600 Mo passait, puis le fold réservait tout. Une garantie de mémoire
fausse d'un facteur **5,77** est pire qu'aucune garde, parce qu'elle se
lit comme une garde.

Le plafond porte désormais sur

$$B_{\text{ev}} + \min\left(\max(\text{budget}, \max_K m_K), \; \sum_K m_K\right),$$

où $m_K$ est **la même** borne que celle de l'ordonnanceur à budget,
factorisée en forme comptée `fold_bytes_upper_from_counts(E, W)` —
calculable au préflight, avant qu'un événement soit matérialisé. Les
deux bouts du `min`/`max` sortent des deux règles d'admission de
`run_folds_budgeted` (`fits` ⟹ `reserve <= budget` ; `alone` ⟹
`reserve == m_K` et plus aucune admission), et le `min` par la somme
n'est pas cosmétique : sans lui un petit nuage se verrait imputer les
2 Gio du budget et le plafond refuserait à tort — c'est le test
`max_output_passe` préexistant qui l'a montré.

Porte de **décision**, sans allocation : un plafond menteur ne plante
pas, ne change aucune sortie et n'apparaît sur aucun chrono. Comptes
gravés, relevés du préflight réel à n=8000. Quatre cas, planchers sur
les trois rôles plus un plancher sur le facteur lui-même, mutant
`budget-events-only` (le comportement d'avant) tué en code 4, et le
câblage vérifié bout en bout à n=400. Reçu :
`ADDENDUM_PLAFOND_SUR_LE_PIC_PROJETE_20260819.md`.

### 2.15 Résidence et budget de fold (LIVRÉ, 27 août — exécute `bab37b9` P0)

**La faute.** `run_folds_budgeted` fait `reserved -= bytes[idx]` au
**retour** de la tâche, mais le résultat vient d'être déplacé dans
`per_k_result[K]` et y reste jusqu'au dernier fold. Les sorties
terminées s'accumulent hors du compte. À n=8000, le seul état final
obligatoire vaut **2 599 581 876** octets contre un « pic » annoncé de
**2 597 650 400** — et les dix `ForestResult` gardés vivants pèsent
**2 548 288 512** octets mesurés, soit **400 804 864** de plus que ce que
l'ancienne formule couvrait.

**Le correctif** est le stopgap exigé au § 4.3 : la décision porte sur
$B_{\text{ev}} + \sum_K m_K$, et surtout **ne s'appelle plus un pic** —
`bytes_bound`, ligne `preflight borne_travail_cumule`, refus qui dit
lui-même « majorant conservateur, pas un pic de résidence ». **OPEN** :
la séparation en sortie persistante / sortie en construction /
temporaires actifs / amont n'est pas faite.

**Deux autorités indépendantes**, la précédente étant auto-référentielle :
`--output-budget-gate` (minorant reconstruit depuis les `sizeof` réels et
les comptes du reçu) et `--fold-residency-gate` (les dix résultats
**gardés vivants**, puis mesurés). Trois mutants tués.

**Second défaut, trouvé en préparant la G4.** Le budget de fold était
figé à 2 Gio — exactement le huitième des 16 Gio de ce conteneur. Sur la
`g4-standard-48` (48 vCPU, **180 Go**) il vaut 1,1 % de la mémoire, et la
borne d'un seul fold le dépasse bien avant n=64000 : mesuré, **3,02 Gio**
pour K=10 dès n=16000 ; à n=64000 ce sont K=5…10, qui portent **94 %**
des incidences. La phase `n64000` **aurait sérialisé le poste dominant
sur une machine à 48 cœurs**. En corrigeant, `--fold-memory-budget` s'est
révélé **mort sur le chemin de production** (il n'atteignait que le banc).

Règle : `max(2 Gio, MemAvailable/4)`, lue une fois, publiée avec sa
provenance ; portes figées au plancher. Mesure intra-processus n=8000,
**signature identique aux quatre modes** : plancher **15 593 ms** contre
budget dérivé **6 959 ms**, pic RSS inchangé. Reçu :
`ADDENDUM_RESIDENCE_ET_BUDGET_MACHINE_20260827.md`.

## 3. Carte de l'implémentation

```
src/wspd/wavefront.hpp      arbre radix + WSPD, rectangles vivants par lane
src/events/                 formes exactes : q2/q3/q4 (instruction+event),
                            acute_seed, edge_cover (cover coef 3),
                            witness_count ; U192/U320 ; uabs/mul annotés
                            MHGP4_HD (préparation GPU)
src/pipeline/ball_stream.hpp  LE CŒUR : les trois lanes génératrices,
                            LaneScratch (sites affines paresseux),
                            étage flottant + garde d'arrondi, cœur de
                            seed Jung, sweep axial 2 côtés (opt-in GPU),
                            stats/timers/workers ; en-têtes = doctrine
src/forest/forest.hpp       fold compact + build_forest_legacy (FIGÉ)
src/forest/render.hpp       rendu § 9.1 (F_K^render, multiplicités)
src/forest/sphere_plateau.hpp  quotient exact des plateaux
bench/forest_probe.cpp      pipeline aval (RLE → préfiltre → census →
                            expansion → folds), TOUTES les portes CLI,
                            digest canonique, preflight
oracle/                     juge indépendant (arithmétique propre)
tests/                      selftests unitaires (obig, tree, forest…)
CMakeLists.txt              153 CTests — dont ~la moitié de portes
                            négatives à code EXACT (1/2/3/4)
```

Codes de sortie des portes : 1 = désaccord du juge, 2 = refus avant
calcul, 3 = plancher/invariant violé, 4 = mutant tué. Un CTest à
`PASS_REGULAR_EXPRESSION` est toujours doublé d'une porte à code.

Doctrine de test (v3, conservée) : planchers `--min-*` contre le
vert-par-vacuité ; fixtures gravées aux coordonnées exactes ; mutants
causaux tués ; équivariance par permutation ; oracles exhaustifs
bornés (n ≤ 12–14) qui ÉTABLISSENT la vérité ; jamais de
re-vérification de ce qu'un théorème garantit.

## 4. Performances connues (n=8000, smax=11, 4 fils, ce conteneur)

| famille | t_gen | notes |
|---|---|---|
| uniform | ~56-62 s | stable |
| eight_clusters | ~123-130 s | 2 658 325 événements ; jamais atteint avant v4 |

⚠⚠ **Ces chiffres sont doublement périmés** : ils précèdent les
intervalles de Jung ET la parallélisation de la descente WSPD du
19 août, qui divise `t_gen` par 2,06 (72,8 → 35,3 s à n=8000). Le
profil à jour est au reçu
`ADDENDUM_DESCENTE_WSPD_ET_CHARGE_Q3_20260819.md` § 1 et § 3.

⚠ **Ces chiffres sont antérieurs aux intervalles de Jung** (`4df9a39`)
et ne doivent plus servir à désigner un poste dominant. Après les
intervalles, le cœur de seed q4 ne fait plus 8,5 G d'évaluations
exactes : il tombe à **80 replis** (eight_clusters) et **145**
(uniform) — l'essentiel est certifié en flottant. Le poste dominant de
`t_gen` est désormais le **scan de profondeur q3** (structurel), pas
l'arithmétique de Jung. ⚠ Cette dernière phrase est elle-même
RÉFUTÉE par la mesure du 19 août : le scan q3 pesait 2,6 % du mur, la
descente WSPD 72 %.

Le kernel affine est CPU-neutre (bande ±3 s) : adopté pour la
STRUCTURE (identité gravée, contrat d'erreur serré, forme GPU
warp-par-seed), pas pour la constante — reçu
`ADDENDUM_KERNEL_AFFINE` § mesures.

⚠ **Variance du conteneur.** Mesuré le 18 août : `t_fold` du MÊME
binaire varie de ±40 % d'un processus à l'autre (allocations à
l'échelle du Go), alors que `t_gen` du même run ne bouge pas. Aucune
comparaison de constante entre deux processus ne conclut ; les
comparaisons de représentation se font par alternance INTRA-processus
(`--fold-intern-bench` en est l'instrument). Profil du fold à n=8000
mesuré ce jour, pour mémoire et non comme référence :
batching ~3 s, intern ~29 s, reduce ~21 s, partition ~1 s. Depuis le
19 août, toute comparaison de representations se fait par banc APPARIE
CONTREBALANCE intra-processus (médiane des rapports par paire, jamais
rapport de médianes) — voir § 2.10.

## 5. Chantiers ouverts, par priorité

Les trois premiers de la rédaction initiale sont LIVRÉS (§ 2.8, § 2.9,
§ 2.10). Ce qui reste, dans l'ordre :

L'ordre ci-dessous est celui de l'audit `dd0d4a6` § 3, adopté tel quel.

1. ~~**Scan q3 et construction des covers**~~ — **TRANCHÉ le 19 août
   par la mesure**, et dans aucun des deux sens proposés. Les compteurs
   de charge par ancre montrent que le scan q3 évalue **10,3 sites par
   seed** (sortie anticipée à `h_3 = 9`), pas les 120 du cover : un
   index par ancre coûterait ~830 opérations de construction contre 351
   sites scannés pour l'ancre entière, et les ancres à ≥ 128 seeds ne
   portent que **4 %** du travail — l'assiette n'existe pas. Le scan
   plat reste, et c'est la forme que le GPU veut. Surtout, le scan q3
   ne pesait que **2,6 % du mur** : le poste dominant était la
   **descente WSPD**, séquentielle et hors de tout chrono (52,5 s sur
   72,8 s). Parallélisée à tranches ordonnées (sortie bit-identique),
   `t_gen` passe de **72,8 s à 35,3 s (×2,06)**. Reçu :
   `ADDENDUM_DESCENTE_WSPD_ET_CHARGE_Q3_20260819.md`.
   **OPEN restant** : la **complétion q4** est désormais le premier
   poste CPU (22,5 s) — à mesurer avant de la toucher.
2. **Internement du fold et streaming.** L'internement est LIVRÉ
   (§ 2.10) et la **comptabilité du plafond** l'est aussi (§ 2.14 : le
   contrat porte désormais sur le pic projeté, tampons du fold compris,
   et non plus sur le seul flux d'événements). **OPEN restant** : le
   **préflight par tuile de clés** — le chemin est honnêtement
   `event_expansion_preflight_after_census`, il matérialise `cands` et
   `balls` avant de brancher, et le pic ne les compte pas (il le dit,
   il ne les remplace pas). À faire en cohérence avec la borne Poisson
   de taille de sortie.
3. **Produit public 30M** : distinguer flux symbolique complet,
   hiérarchie de connectivité et requêtes/labels ciblés — c'est aussi
   ce qui lèvera le refus `resource_exhausted` de la garde de capacité
   (§ 2.8, partie OPEN) au lieu de le contourner.
4. **Port GPU exact** : compiler le témoin device sous `nvcc`
   (`src/gpu/device_compile_witness.cu`, jamais encore compilé), porter
   les filtres certifiés, puis compacter les replis exacts — plans
   `NOTE_CLAUDE_PLAN_GPU` + `NOTE_CLAUDE_PLAN_PARALLELISME_V2`
   (warp-par-seed, saturation par ballot). L'oracle et le juge ne sont
   JAMAIS portés. Le **schéma L/U à deux bornes** (audit E573888 § 5)
   appartient à ce chantier : il évite une file de repli de taille
   imprévisible sur device, pas une constante CPU.
5. **Ordre axial `cmp_mu`** (borne propre ~$2^{114}$) : seulement si le
   chemin axial (`--axial-on`) redevient actif ou devient utile sur
   GPU. Ce n'est pas un multiplicateur CPU mesuré.

Conditionnels, à ne pas promouvoir sans les compteurs de charge qui les
justifient : **deltas en CSR** (audit `57523a` § 1.3 — `t_partition`
vaut ~1 s sur ~54 s de fold, donc invisible aujourd'hui) et **boule
intérieure candidate** $B(m, R-\delta)$ (réponse d'auditeur, dormante).

**Règle de fraîcheur de cette liste** (audit `dd0d4a6` § 5) : un item
OPEN ne doit jamais nommer un reçu qui, ailleurs dans ce document, est
cité comme exécution complète ; s'il en cite un, il doit porter
explicitement la partie résiduelle encore ouverte (le § 2.8 le fait
pour la garde de capacité, le n° 2 ci-dessus pour l'internement).
`python tools/check_passation.py` refuse le contraire.

## 6. La campagne G4 « scale_threads » — protocole prêt, PAS LANÇABLE EN L'ÉTAT

> **État au 27 août 2026.** Le protocole local est prêt et sa porte
> transactionnelle est verte (`selftest_scale_threads.sh` : 15 scénarios,
> `violations=0`) ; l'enveloppe des six gardes passe sur la ligne exacte
> ci-dessous (`--check-envelope` → `EXIT=0`, aucune action GCP) ; et le
> schéma de sortie du probe satisfait toutes les expressions du
> validateur, vérifié ligne à ligne après les commits des 19 et 27 août.
>
> **Trois blocages restent, et aucun n'est levable depuis ce conteneur :**
>
> 1. `gcloud` absent (`start_and_verify.sh:503`), et le jeton présent dans
>    l'environnement fait 14 caractères — rejeté (`invalid_token`,
>    `ACCESS_TOKEN_TYPE_UNSUPPORTED`). **L'inventaire lecture seule exigé
>    avant tout geste GCP est donc impossible.**
> 2. `ssh`, `ssh-keygen`, `scp` absents (`start_and_verify.sh:505`).
> 3. Port 22 sortant bloqué — l'armement du coupe-circuit invité
>    (`start_and_verify.sh:689-719`) ne peut jamais aboutir. C'est le
>    maillon jamais franchi des six tentatives du 18 août, et il est
>    **structurel** : l'API passe, SSH jamais.
>
> La campagne doit donc partir d'un poste avec auth gcloud + OpenSSH +
> port 22 (Codespace utilisateur), ce que § 6 disait déjà.
>
> **Et deux blocages de fond, eux, sont dans le dépôt :**
>
> 4. L'audit `bab37b9` (22 août) classe la campagne G4 en **Priorité 5**,
>    après le GPU et après la décision d'objet produit, et précise que la
>    porte 50 k doit couvrir le produit décidé — « une mesure de front, de
>    composant isolé ou de préfiltre ne vaut pas `warm_e2e` ». Le
>    protocole `scale_threads` mesure la montée en fils du chemin dense,
>    ce qui n'est pas cela.
> 5. Le budget de fold était figé à 2 Gio : la phase `n64000` **aurait
>    sérialisé le poste dominant** sur la machine à 48 cœurs (§ 2.15).
>    Corrigé le 27 août — mais la correction doit être **committée et
>    poussée** avant tout pin, puisque le pin refuse un worktree sale.


Protocole (audits `9223888`/`b3a6eb4`/`66886c0`/`7d921ff`/`c9c3a48`/
`9d19ede`, tous EXÉCUTÉS) : `gcp-migration/session_scale_threads_g4.sh`
→ **pin du protocole D'ABORD** (fermeture transitive : les trois gardes
locales `set_max_run_duration_and_verify` / `start_and_verify` /
`stop_and_verify` sont dans les chemins normatifs ET matérialisées
depuis le commit ; la session n'exécute plus que
`${WORK}/pinned/gcp-migration/`) → préflight SIX gardes (dont les deux
de `start_and_verify` modélisées, constantes lues dans la copie
PINNÉE, TTL dérivé du plafond, `--check-envelope`) → inventaire
« aucune VM » → démarrage gardé →
runner `v4_scale_threads_remote.sh` (argv haché NUL, workers mesurés,
digest canonique complet K=1..10) → validateur épinglé
`validate_v4_scale_threads.py` (SEULE autorité : liaison nom→argv→
identité imprimée, affinité effective, appariement par digest) →
arrêt certifié TERMINATED. Porte transactionnelle :
`selftest_scale_threads.sh` (15 scénarios, dont
`uncommitted-local-guard` et `helper-from-worktree-after-pin`) — elle
n'est PAS câblée dans la CI GitHub (elle invoque le lanceur, et
`tools/check_gcp_workflows.py` interdit à la CI toute indirection vers
un script de cycle de vie) : elle se lance à la main avant toute
session payante. Phases : `n32000` (équivalence t1/t8/tmax), `n64000`
(échelle, non appariée), `court1h` (≤ 1 h, paires t8/tmax).

Chaîne de provenance (audit `9d19ede` § 5) : **DONE** = identité du
moteur, du runner, du validateur ET des trois gardes locales (pin +
matérialisation, fenêtre TOCTOU fermée) ; **OPEN** = rien sur ce point.

**Journal du 18 août** : six tentatives, zéro campagne exécutée ;
tentatives 2, 3 et 5 avec lecture d'arrêt certifiée, 1 et 4 refusées
avant toute VM, 6 garantie par son double coupe-circuit certifié sans
lecture finale conservée — détail dans
`receipts/campagne_scale_threads_20260818/JOURNAL_TENTATIVES.md`.
Leçons durcies dans le code : préflight 4→6 gardes ; TTL dérivé ;
`terminationTimestamp` lu sous `resourceStatus.scheduling` ;
`europe-west4-a` préempte immédiatement (2×) → repli autorisé
`europe-west4-ai1a` (capacité prouvée) ; le bac à sable CCR bloque le
port 22 sortant (l'API passe, SSH jamais) → la campagne s'exécute
depuis un poste avec SSH (Codespace de l'utilisateur) ; un jeton
d'accès vit ~60 min (gcloud le met en cache — `rm access_tokens.db`
pour en frapper un neuf).

**Ligne prête** (depuis un poste avec auth + SSH, worktree propre sur
`main ≥ 5c326c1`) :

```bash
GCP_ZONE=europe-west4-ai1a GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a \
PHASE=n64000 RUN_TIMEOUT=600 BUILD_MARGIN=480 RETRIEVE_MARGIN=300 \
MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=55 SSH_KEY_TTL_MINUTES=66 \
./gcp-migration/session_scale_threads_g4.sh
```

Dernier état observé : le boot `-ai1a` est lent à monter `sshd`
(premier boot du disque) — si l'armement de la garde invité échoue,
relancer une fois (second boot plus chaud). ⚠ Conflit tranché pour
≤ 1 h seulement : la campagne est 100 % CPU sur une machine à GPU
(règle du 9 août) — un chemin gardé CPU-only reste une décision
UTILISATEUR.

## 7. Le processus de travail

- Les AUDITEURS poussent sur `main` ; leurs audits se LISENT et
  s'EXÉCUTENT avant toute dépense ; chaque réponse est un
  `REPONSE_CLAUDE_*`/`NOTE_CLAUDE_*` dans `audits/`, chaque livraison
  un reçu ancré au commit dans `receipts/` (immuable).
- Pousser régulièrement sur `main` (autorisé explicitement) ET sur la
  branche désignée de la session.
- Canal opérationnel GCP : branche `claude/g4-relais`, fichier
  `RELAIS.md` (sections en tête, jamais de secret).
- Jamais de jeton/clé dans le dépôt, les logs committés ou les
  messages ; les refus des scripts gardés sont finaux.
