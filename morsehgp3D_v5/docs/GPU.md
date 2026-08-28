# MorseHGP3D v5 — plan du port GPU (point 2)

Cadre inchangé : `public_status=not_claimed`. Ce document dit **quoi** porter,
dans **quel ordre**, et **comment on le prouve** ; une mesure y reste une
mesure. **État par étape** (distinguer source présente, compilation `nvcc`,
exécution conforme, mesure, pin du reçu) :

| cible | source | compilée (`nvcc`) | exécutée conforme | mesurée | reçu (pin) |
|---|---|---|---|---|---|
| témoin device (DI128 + scan q3 warp) | `src/gpu/device_witness.cu` | oui | oui, 0 désaccord | — | `campagne_g4_v5_20260827_temoin_device` (`24b3f164`) |
| lane q3 device | `src/gpu/q3_lane_device.cuh` | oui | oui, 1200 / 1200×4 fils / 8000×8 fils | temps kernel gravés, pas de banc apparié | idem |
| lane q4 device | `src/gpu/q4_lane_device.cuh` | oui | oui, 1200 / 1200×4 fils / 8000×8 fils (157 M complétions, 0 désaccord) | temps kernel gravés | `campagne_g4_v5_20260827_lane_q4_device` (`2e75cb42`) |
| pilote `mhgp5_cuda --gpu` (contrats 50 k) | `cli/mhgp5_cuda.cu` | oui | **quatre familles** + adaptatif `eight_clusters` : `digest_balls` et `digest_all` identiques au contrat CPU au pin `8f95df2e` (égalité bornée observée ; lots bornés en seeds/sites/paires à ce pin) | GPU **plus lent partout** : `uniform` 89 s vs 78 s, `terrain` 44 vs 23, `scanline` 96 vs 38, `eight_clusters` **718 vs 246** (lane q3 : 527 s vs 94 s) ; adaptatif 256 : 713 s (il envoie 70,7 % des ancres q3 et 31,6 % des ancres q4 au device, mais 99,1 % des seeds q3 et 91,3 % des seeds q4 — le seuil par taille de cover laisse presque tout le travail coûteux au device ; mesuré sur le layout `8f95df2e` à second lot hôte, pas sur la route hôte directe de `10c46c87`) | `campagne_g4_v5_20260827_adaptatif` (partielle : 24/25 runs, journal perdu — voir le reçu) |
| mutant du témoin sur device | `--inject=witness-no-warp-correction` | oui | **tué** (code 4, run `gpu_mutant`) | — | idem |

**Verdict de mesure (sessions `2e75cb42` et `8f95df2e`)** : la lane device
par lots d'ancres présente une **égalité bornée observée au pin `8f95df2e`**
(`digest_balls` et `digest_all` identiques aux sorties CPU appariées sur les
quatre familles à 50 000 points, mutant q3 tué sur device ; campagne
partielle : 24/25 runs, journal perdu, validateur non exécuté — ce n'est pas
« exacte en général ») et **plus lente que le CPU à 48 fils sur toutes les
familles**. La cause n'est pas le kernel (111 s de kernel cumulés
sur 48 exécuteurs pour `eight_clusters`, soit ~2,3 s par fil) mais la
**matérialisation hôte** : 18,2 G seeds q3 (et 1,5 G seeds q4) à ~100 octets
chacun, plus les covers de 19,7 M ancres à 32 octets par site, copiés et
transférés — là où la lane CPU tue la plupart des seeds en quelques sites
sans rien copier. Le routage par taille de cover ne sépare pas ce coût. Ce qui est fermé comme
voie de gain est précisément le **tout-device matérialisé au pin mesuré** ;
l'adaptatif à route hôte directe (`10c46c87`, ancres hôte traitées par le
corps de production sans matérialisation) n'a pas encore été mesuré sur G4.
La lane par lots reste le banc de référence et la preuve d'égalité bornée du
chemin device. `kernel_ms` est un cumul d'événements de 48 exécuteurs, pas un
mur GPU : « matérialisation et orchestration probablement dominantes » tant
que les murs préparation / H2D / kernel / D2H ne sont pas séparés.

**Livraison 7 (conception, non mesurée)** : lane device **par rectangle**.
Le candidat de cover issu de `rect_cover_handles` est un **sur-ensemble
fail-open** de chaque cover d'ancre (antichaîne de handles de ≤ 32 positions),
partagé par les |A|×|B| ancres du rectangle — mais le reçu `eight_clusters`
ne montre qu'environ 4,7 ancres device par rectangle q3 et 1,6 en q4, donc le
partage est faible et le claim « Σ covers de rectangles ≪ Σ covers d'ancres »
n'est **pas mesuré**. Variante retenue à mesurer d'abord (auditeur) : un
**tableau global O(n)** de positions et `PointId` résident sur le device,
partagé entre exécuteurs (50 000 points = 1,6 Mo ; 10 M = 320 Mo), et par
fenêtre de rectangles seulement les **plages de handles**, `|A|`, `|B|`, les
histogrammes de coin et `need` ; le device énumère lui-même les ancres,
reproduit le **filtre exact et l'ordre stable des 32 classes radiales** (les
compteurs à sortie anticipée en dépendent ; le verdict mort/vivant non), les
seeds aigus, les scans — fonctions `MHGP5_HD` déjà écrites — et ne rapatrie
que les **survivants** (ancre, seed[, complétion]) dont l'hôte forme clé et
niveau, plus les compteurs agrégés. Ni cover aplati conservé ni matrice
rectangle × ancre × point ; précomptage / prefix-sum vérifiés, tuilage
déterministe, retour des survivants borné ; conversions DI128 → binary64
certifiées bit à bit aux frontières d'arrondi si `Gd`, `Nd`, `bound` sont
formés sur device. Avant tout kernel : (1) mesurer somme et maximum des
sites par rectangle, `|A|·|B|`, seeds, complétions, survivants aux tailles
8 k à 50 k ; (2) établir sur CPU l'égalité **ensemble et ordre** de chaque
cover reconstruit avec `cover_query`, puis l'égalité de la lane shaped par
rectangle (post-RLE, verdicts, compteurs) ; (3) séparer les murs
préparation / H2D / kernel / D2H. Le fold et le reste du pipeline ne bougent
pas.

Lecture de la session `2e75cb42` : le lotissement fait tomber les lancements
q3 à 8000 de 645 636 à 542 (kernel 6,6 s → 24 ms) ; l'égalité de bout en bout
à 50 000 points avec les deux lanes device est établie sur `uniform` et
`terrain` ; mais sur ces familles régulières le device **coûte** plus qu'il ne
rapporte (les seeds y meurent tôt sur CPU, tandis que le lot copie tous les
sites de toutes les ancres). Le gain attendu est sur les familles denses
(`eight_clusters` : 94 + 88 s de lanes CPU à 48 fils), précisément celles qui
ont manqué de mémoire — bornes en sites (`2^20`) et en paires (`2^24`)
ajoutées, à mesurer. Suite prévue : un **exécuteur adaptatif** (une ancre va au
device seulement si son travail estimé — seeds × sites — dépasse un seuil,
sinon elle est scannée sur l'hôte dans le même pipeline ; les deux exécuteurs
étant prouvés égaux, l'objet ne change pas) et le recouvrement des transferts.

Ce qui n'a pas de reçu n'est pas reçu.

**Recul mathématique (27 août 2026, `docs/MATHEMATIQUES.md` § 10,
`docs/analyses/seeds_20260827/`)** : les 18,2 G seeds q3 d'`eight_clusters`
50 k ne sont pas un coût inhérent — la lane q3 ne comptait pas les témoins
universels $W_3$ (la q4 le faisait) ; le compte exact plus un test de
**témoins sectoriels** (polygone circonscrit au disque des centres) tuent
54–60 % des ancres survivantes et 92–94 % des seeds sans en énumérer un seul,
objet inchangé (conformités v4 égales, fixtures F1–F3, mutants). Mesure
locale à 8000 : lane q3 `eight_clusters` 33,0 s → 13,7 s, q4 31,0 → 22,3 s.
**Conséquence pour le point 2** : la base de comparaison CPU (94 + 88 s à
50 k) est périmée ; la lane q3 CPU à 48 fils est probablement de l'ordre de
10–20 s (plancher = covers). Le gain device sur q3 n'est plus démontré et
doit être **re-mesuré sur G4 contre la nouvelle lane CPU** avant tout kernel
par rectangle ; tout étage hôte d'une lane device inclut désormais les tests
d'ancre (ne jamais envoyer au device une ancre tuable en ~22 évaluations).
Les compteurs des reçus 50 k antérieurs (seeds, `depth_killed`, `q3_cert`)
sont périmés ; leurs digests ne le sont pas.

**Session G4 n° 7 (pin `fa99b3f1`, tests d'ancre en production, 27 août
2026 — d'après le journal de session ; artefacts rapatriés par mini-session
gardée après une corruption du script de session, voir le reçu)** : 12
conformités 8 k / 16 k / 32 k à code 0 (`eight_clusters` 32 k : 83 s contre
122 s) ; contrats CPU 50 k : `uniform` 78 s (inchangé, fold), `terrain` 21 s
(23), `eight_clusters` **162 s (246)**, `scanline` **23 s (38)** ; contrats
`--gpu` : 81 / 25 / 174 / 24 s — le chemin device par lots revient à
**parité** avec le CPU (au lieu de +14 % à +192 %), parce que les tests
d'ancre tuent avant matérialisation ; adaptatif 256 : 174 / 24 s. Lecture
V8 : sur ce pin, ni le tout-device ni l'adaptatif n'ont de gain net à 50 k ;
le résidu de la lane q3 CPU est la construction des covers, pas le scan.
**Session 8 (pin `ef5abbd5`, même code de génération que la 7, reçu
`campagne_g4_v5_20260827_extension`, campagne complète, 31 runs)** —
tailles d'extension CPU 48 fils : `uniform` 100 k **166 s** (40 Go),
200 k **353 s** (65 Go) ; `eight_clusters` 100 k 440 s (gen 317 s : q3 90,
q4 207), 200 k **1457 s** (gen 1188 s : q3 383, q4 758 ; 59 Go) ;
`scanline` 100 k 89 s, 200 k 503 s (q4 379 s). Digests gravés comme
référence v5 à ces tailles. Lecture : à l'échelle, la lane **q4** domine et
croît plus vite que linéairement sur les familles denses — c'est ce que les
morceaux de corde et les prétests avant cover (pins suivants) attaquent.
Après la session 7, la lane q4 (65 s) domine : le **test de seed par
morceaux de corde** (`MATHEMATIQUES.md` § 10, théorème 10.4) est entré en
production (corps partagé, cœur shaped, kernel `k_q4_core` avec ballots par
morceau) — à 8000 `eight_clusters` (ratio local) : complétions atteignant la
profondeur 23,8 M → 5,1 M, rectangles q4 22,3 s → 18,8 s ; à recevoir sur G4.

**Session 9 (pin `5c777be3` : tests d'ancre + morceaux de corde + prétests
avant le cover sur les candidats diamétraux du rectangle ; reçu
`campagne_g4_v5_20260827_corde_pretests`, campagne complète, 31 runs,
validateur du pin rejoué)** — comparaison appariée avec la session 8 (CPU 48
fils) : `eight_clusters` 50 k **162 → 82 s** (gen 24 s : q3 5,0, q4 9,6),
100 k **440 → 185 s**, 200 k **1457 → 443 s** (gen 175 s : q3 45, q4 84) ;
`uniform` 50 k 78 s, 100 k 166 → 164 s, 200 k 353 → 346 s ; `terrain` 50 k
22 s ; `scanline` 50 k 16 s, 100 k 89 → 57 s, 200 k 503 → **499 s** (gen 465
s dont q4 438 s). Contrats `--gpu` 50 k : 81 / 24 / 85 / 18 s, adaptatif 256 :
84 / 18 s — deux digests identiques au CPU partout, toujours à parité (aucun
gain net). Trois lectures :

1. Sur `uniform` et `eight_clusters`, la génération n'est plus le poste
   dominant : à 200 k, le **fold** (`reduce` 139–144 s séquentiel par ordre,
   lié à la latence mémoire : 562 M facettes cumulées, ~1,6 µs par événement)
   et le digest (63 s) font plus de la moitié du mur ; la mémoire de pointe
   (65 Go à 200 k `uniform`) borne l'échelle avant le temps. Le device ne
   peut prendre que le corps de la lane (cœur + corde + complétions ≈ 47 % de
   la lane q4 au profil 8000, soit ~5 s sur 82 s à 50 k) : **la livraison 7
   (lane par rectangle) n'a pas de gain à démontrer à 50 k ; elle reste
   subordonnée.** Le chemin device prouvé (deux digests, mutant tué) est
   conservé comme exécuteur non autoritaire.
2. Sur `scanline`, la lane q4 croît en $n^{2{,}9}$ (10 G → 59 G → 431 G
   itérations du balayage cœur/corde de 50 k à 200 k ; ancres q4 examinées en
   $n^{1{,}86}$ ; itérations par seed mort 83 → 122 → 218) alors que l'objet
   (candidats q3/q4) est linéaire. La cause est mathématique, pas un débit :
   sur une ancre entre deux lignes de balayage, la boule diamétrale ne
   contient aucun point, donc aucun secteur (tous contiennent l'apex $v=0$)
   n'a de témoin universel et l'ancre survit alors que tous ses seeds meurent.
   C'est le chantier suivant (tests d'ancre sans apex : disque intérieur +
   secteurs annulaires, ou index dual $(\theta, t)$ des sites par ancre) —
   avant tout kernel, parce qu'un device 50× plus rapide ne change pas un
   exposant.
3. Le noyau device tel qu'écrit mesure `kernel_ms` = mur d'un lot y compris
   les boucles hôte entre les trois kernels (verdicts, offsets de paires,
   compaction des étages octet par octet) ; 13 574 lancements à 50 k
   `scanline` pour ~10 G itérations, soit ~1 G/s — deux ordres sous le débit
   attendu. Toute reprise du point 2 commence par instrumenter ces trois
   étapes séparément ; aucune conclusion de débit device n'est tirée de ce
   reçu.

**Session 10 (pin `90baa0bb` : étage B du fold concurrent par ordre,
`fold_inflight` = 2, `reduce` à état packé + prefetch ; reçu
`campagne_g4_v5_20260828_fold_inflight`, campagne complète, 31 runs,
validateur du pin rejoué)** — comparaison appariée avec la session 9, CPU 48
fils : `uniform` 50 k **78 → 57 s**, 100 k 164 → 121 s, 200 k **346 → 258 s**
(mur du fold 118 s pour 198 s cumulés de reduce + digest ; RSS 65 → 80 Go) ;
`eight_clusters` 50 k **82 → 64 s**, 100 k 185 → 144 s, 200 k **443 → 363 s**
(59 → 74 Go) ; `terrain` 22 → 20 s ; `scanline` inchangé (16 / 54 / 502 s :
lane q4). Contrats `--gpu` à parité (61 / 22 / 68 / 16 s), deux digests
identiques. Lecture : le fold n'est plus séquentiel sur le chemin critique ;
son mur reste borné par l'étage A (tri + internement + fusion ≈ 46 s à 200 k
`uniform`) plus la traîne du dernier ordre ; `fold_inflight` = 3 ou 4 (un ou
deux ordres résidents de plus) et l'internement (1,3 µs de temps-fil par
enregistrement de facette) sont les leviers suivants du fold. Sur `scanline`
et sur les familles denses, la lane q4 est le poste : c'est la **grille de
cellules** (théorème 10.5), livrée au pin suivant.

**Session 11 (pin `82f613d3` : grille de cellules sans apex, théorème 10.5,
+ listes de census inline + fold concurrent ; reçu
`campagne_g4_v5_20260828_grille`, campagne complète, 31 runs, validateur du
pin rejoué)** — comparaison appariée avec la session 10, CPU 48 fils :
`scanline` 50 k 15 → 13 s, 100 k **54 → 40 s**, 200 k **502 → 268 s** (lane q4
438 → 215 s : 990 888 grilles, 687 851 ancres et 277,9 M seeds tués sans
balayer un site, itérations du cœur 431 G → 195 G) ; `uniform` 200 k 258 →
254 s (RSS 80 → 76 Go), `eight_clusters` 200 k 363 → 353 s (74 → 73 Go), 50 k
RSS 21,1 → 17,8 Go ; `terrain` 20 → 17 s. Contrats `--gpu` à parité (59 / 16 /
66 / 14 s), deux digests identiques. Paliers `rss_mb` à 200 k `uniform` :
34,2 Go après census, **66,3 Go au maximum du fold** (trois ordres résidents,
~11 Go par ordre : clés, état, internement, deltas) — la résidence du fold est
désormais le pic ; c'est le prochain poste mémoire avant le million de points.
Ce qui reste de la lane q4 `scanline` (215 s) tient aux ancres dont la grille
n'est pas construite (politique) ou n'est pas entièrement morte ; la
politique et $G$ sont des réglages sans effet sur l'objet.

**Session 12 (pin `63deda74` : exécuteurs device instrumentés par étape ;
reçu `campagne_g4_v5_20260828_etapes_device`, 25 runs, validateur du pin
rejoué) — pourquoi le device ne rapporte rien.** Murs *hôte* cumulés sur 48
fils, `uniform` 50 k (lane q4 CPU 3,0 s, avec `--gpu` 4,3 s, `kernel_ms`
7,5 s) : **H2D des sites 27,4 s**, **attente K1 + D2H des verdicts 42,2 s**,
boucle hôte des vivants 0,4 s, K2 1,1 s, compaction hôte 3,8 s, K3 1,5 s,
mur de `scan()` 76,4 s ; lane q3 : H2D 1,4 s, **K1 0,20 s pour 87 M seeds
(2,3 ns par seed)**, D2H 0,14 s, mur 6,9 s. Mêmes proportions sur
`eight_clusters` (q4 : H2D 38,9 s, K1 66,3 s, mur 114,7 s), `scanline` et
`terrain`. Lecture : (1) le calcul device est négligeable — le noyau q3 tient
en 0,2 s cumulés, le cœur q4 en quelques secondes ; (2) le chemin est borné
par les **copies H2D des covers** : chaque ancre envoie son cover entier, 8
tableaux i64 = 64 o par site, soit ≈ 37 Go par lane à 50 k pour une lane CPU
de 3 s — le PCIe (≈ 9 Go/s agrégés) est le plafond ; (3) **48 fils** à
exécuteurs `thread_local` se disputent un seul device (moteur de copie, file de
lancement) et chaque étape est synchrone (`cudaStreamSynchronize` après H2D,
K1, K2, K3) : le mur hôte d'un fil est l'attente des 47 autres ; (4) le reste
de la lane (candidats, covers, assemblage des lots, émission) reste hôte, ≈
2,7 s par fil sur 4,3 s. Conséquence : aucun réglage de lots ni de seuil ne
change ce verdict ; le gain exige la forme de la doctrine (§ 1) — index et
positions **résidents sur device** (96 Go), covers, tests d'ancre, grille,
seeds et cœurs calculés **sur device par rectangle** sans aller-retour, un
petit nombre de flux asynchrones à double tampon, aucune boucle hôte par seed
ou par paire — c'est-à-dire la livraison 7 conçue avec l'architecture tuilée
(`ECHELLE.md`), où une tuile est l'unité de travail device. Tant que ce n'est
pas fait, le chemin device reste un exécuteur prouvé exact et non autoritaire,
sans usage en campagne.

## 1. Ce que la mesure G4 a désigné (27 août 2026)

Sur `g4-standard-48` à 48 fils, K = 1..10 exact (reçu
`receipts/campagne_g4_v5_20260827/`) :

| famille 50 000 | génération | dont lanes q3 / q4 | fold (avant parallélisation) |
|---|---:|---:|---:|
| uniform | 16,8 s | 2,3 / 4,1 s | 114,8 s |
| eight_clusters | **190,9 s** | **94,0 / 87,6 s** | 103,5 s |

Le fold est traité (point 1 : partitionné, pipeliné). Le poste GPU est la
**génération sur covers denses** : par ancre survivante, un scan des sites du
cover pour chaque seed aigu (kernel affine `L = G·q − 2·u·N`, étage flottant
certifié, repli exact i128), le cœur de seed de Jung, et les complétions q4
(owner, exact-once, préfiltres, Cramer, centre strict, profondeur). Ce travail
est **régulier** (mêmes opérations sur tous les sites d'un cover) et sans
allocation : la forme voulue est *warp-par-seed* (un warp balaie les sites
d'un seed, réduction par ballot pour la profondeur), les ancres d'un rectangle
en blocs.

## 2. Contraintes non négociables

- **Même objet** : l'objet comparé est le **vecteur post-RLE** des candidats
  (trié par `ball_candidate_less`, dédoublonné par clé ; chaque élément = clé
  primitive, niveau exact, arité) produit par la lane device, qui doit être
  **égal élément par élément** à celui de la lane CPU sur les mêmes entrées ;
  à un fil, l'ordre brut d'émission est comparé aussi ; les compteurs de la
  lane (7 pour q3, 22 pour q4) doivent coïncider. C'est la porte, et la seule.
- **Exactitude** : toute décision reste entière. Arithmétique 128 bits
  **portable** (`src/core/dint.hpp`, limbes u64, prouvée égale à `__int128`
  sur CPU par `mhgp5_dint_gate` et sur device par le témoin) pour les formes
  et les scans ; `__int128` **est** employé en code device là où `wide.hpp`
  l'emploie déjà (`cmp_2p2_jb2`, cœur de seed q4), nvcc 12.9 avec GCC hôte
  le supportant — contrat de backend écrit dans `dint.hpp`. Les formes q3/q4
  sont dupliquées en variantes device (`src/lanes/device_forms.hpp`)
  prouvées égales aux formes de production sur tous les triangles/tétraèdres
  de petits nuages et sur les fixtures u16 extrêmes.
- **Le flottant reste un filtre** : la borne certifiée par seed et la séquence
  FMA figée valent sur device à condition d'imposer l'arrondi au plus proche
  et d'interdire toute contraction hors de la séquence (`-fmad=false` ou
  `__fmaf_rn`/`__fma_rn` explicites) ; le repli exact est obligatoire.
- **Ce qui reste à l'hôte** : les clés et niveaux exacts (réduction
  rationnelle, U192) des candidats émis — le device rend les verdicts (q3 :
  mort/vivant par seed ; q4 : cœur par seed, étage par complétion,
  profondeur par candidate) et l'hôte forme clé et niveau à partir de la
  forme recalculée. Le cœur de seed exact (`cmp_2p2_jb2`) s'exécute sur le
  device.
- **L'oracle n'est jamais porté.**
- **Pas de nvcc ici** : les kernels ne se compilent et ne s'exécutent que sur
  la G4, par sessions gardées ; tout ce qui peut être vérifié sur CPU
  (arithmétique, formes, logique du scan sous forme « device-shaped ») l'est
  avant chaque session.

## 3. Ordre des livraisons

1. **Arithmétique et formes portables** (CPU-vérifiables) : `dint.hpp`,
   `device_forms.hpp`, `mhgp5_dint_gate` — **livré** (0 désaccord contre
   `__int128`, mutant `dint-mulhi-dropped` tué).
2. **Scan q3 « device-shaped »** sur CPU : une fonction qui, pour un
   rectangle vivant, prend les tableaux plats (sites affines de l'ancre,
   seeds) et rend les candidats q3 exactement comme la lane q3 de
   `generate.hpp`, mais écrite comme un kernel (indices explicites, aucune
   allocation, réductions explicites) ; porte d'égalité post-RLE avec la lane
   de production à n = 400/1200 — **livré** (`src/gpu/q3_scan_shaped.hpp`,
   `mhgp5_q3_scan_shaped_gate` : 0 désaccord, mutant `q3-shaped-strict-flip`
   tué).
3. **Option CMake `MHGP5_ENABLE_CUDA`** (OFF par défaut, sm_120 fixé avant
   `enable_language`, `-fmad=false`, options GCC bornées à CXX et relayées par
   `-Xcompiler`), cible `mhgp5_device_witness` (`src/gpu/device_witness.cu`)
   — **prouvé sur G4** (session `24b3f164`, reçu
   `receipts/campagne_g4_v5_20260827_temoin_device`, RTX PRO 6000 Blackwell
   sm 12.0, CUDA 12.9.41, pilote 580.173) : 262 144 cas d'arithmétique DI128
   + bords, 0 désaccord ; scan q3 **warp-par-seed** (`__ballot_sync`/`__popc`,
   sortie anticipée à h3 reproduite jusque dans les compteurs des seeds
   mortes) sur 728 347 seeds (`uniform` 400) et 2 308 366 seeds
   (`eight_clusters` 400), 0 désaccord ; mutant `witness-no-warp-correction`
   tué. Le protocole de campagne exécute ce témoin en **phase 0** et refuse
   les phases CPU s'il n'est pas conforme ; le validateur exige les deux
   familles, leurs planchers et `desaccords=0`. Les deux sessions
   précédentes (`9762daaf`, `50fee05c`) ont été refusées — journaux non
   rapatriés, puis erreurs de compilation réelles (ponts `__int128` cachés à
   la passe device, `edge_key` hôte appelé depuis une fonction HD, sm_52 par
   défaut) — et sont gravées comme telles.
4. **Kernel q3** (`src/gpu/q3_scan_kernel.cuh`, partagé par le témoin et la
   lane) + **lane q3 device** (`src/gpu/q3_lane_device.cuh` : exécuteur par
   fil, tampons croissants, flux, un lancement par lot) — **prouvée égale à
   la production sur G4** (`mhgp5_q3_lane_device_gate`, run `gpu_lane` du
   même reçu) : `uniform` 1200 (176 245 candidats, 3,37 M seeds),
   `eight_clusters` 1200 à 4 fils (25,65 M seeds), `uniform` 8000 à 8 fils
   (1 425 821 candidats, 35,4 M seeds) — 0 désaccord vectoriel post-RLE, 0
   désaccord de compteurs (`q3_cert`). **Pas encore rapide** : un lot = un
   rectangle, soit 645 636 lancements pour 35,4 M seeds à 8000 (55 seeds par
   lancement, 6,6 s de kernel) ; le lotissement multi-rectangles est la
   livraison 5.
   Étage hôte (CPU, prouvé) : `q3_lane_batched.hpp`
   (`generate_q3_batched_with<Scan>`), `mhgp5_q3_lane_batched_gate`
   (égalité vecteur à vecteur, ordre brut à un fil, post-RLE à plusieurs).
   **q4 en forme de kernel, prouvé sur CPU** : `q4_core_shaped.hpp` (cœur de
   seed de Jung : `mhgp5_q4_core_shaped_gate`, 0 désaccord par seed sur
   508 979 / 936 824 / 326 836 seeds, replis exacts exercés par la famille
   cocirculaire), `q4_completion_shaped.hpp` (complétions + Cramer +
   bien-centrage + profondeur avec `Q4FormD` : `mhgp5_q4_completion_shaped_gate`,
   0 désaccord d'étage sur 3,99 M / 6,25 M / 2,61 M paires, formes `det`/`N'`
   identiques i128 ↔ DI128), `q4_lane_batched.hpp` (étage hôte complet :
   `mhgp5_q4_lane_batched_gate`, égalité post-RLE et **dix-neuf compteurs**
   avec la production, 1200 × trois familles, cocirculaire, ordre brut à un
   fil, 8000 à 8 fils). Mutants tués : `q4-shaped-jung-skip-kills`,
   `q4-shaped-once-flip`, `q4-batched-emit-deep`. Reste : l'exécuteur device
   q4 (kernel cœur warp-par-seed ; complétions thread-par-paire avec
   compaction ; profondeur warp-par-candidat) et sa porte sur G4.
5. **Lotissement, kernels q4, pilote CUDA et mesure** :
   - 5a **livré (CPU)** : `generate_q3_batched_with` / `generate_q4_batched_with`
     accumulent les rectangles de chaque ouvrier dans un lot vidé dès que le
     seuil `kSeedsPerLaunch = 2^16` seeds est atteint — testé **après chaque
     ancre** (l'unité atomique), donc **borne dure** d'un lot = seuil + seeds
     de la plus grosse ancre, mesurée et exigée par les portes
     (`vidages`, `max_lot_seeds`, `max_ancre_seeds`, `--min-flushes`) ; un
     seuil < 1 est refusé (code 2) ; une ancre q4 sans seed n'est pas
     matérialisée. **Ordre** : l'ordre *local* de chaque ouvrier (rectangles,
     ancres, seeds) est préservé ; à plusieurs fils, l'ordre brut global
     n'est pas spécifié (tirage dynamique, fusion par ouvrier) — seule la
     sortie post-RLE l'est, et c'est elle que les portes exigent (l'ordre
     brut n'est comparé qu'à un fil). Contrat structurel des lots
     (`validate_q3_batch_view`, `validate_q4_batch_view`,
     `validate_q4_results_view` : SoA de même taille, tranches dans les
     tableaux, indices de lentille, `x_site`/`skip` dans la tranche,
     émissions ordonnées/distinctes/de seeds vivants, somme des étages =
     complétions, limite `UINT32_MAX` gravée par vue synthétique) vérifié
     avant tout scan et toute émission (`mhgp5_batch_contract_gate`) ; les
     primitives parallèles capturent la première exception, joignent tous
     les fils puis relancent (`mhgp5_parallel_exception_gate`, quatre fils,
     vidage au seuil et vidage final) ; réserves device transactionnelles
     (temporaires puis échange, instance inutilisable après échec) ;
     comptage des paires q4 en `u64` refusé au-delà du domaine `u32` des
     kernels ; `lots` et `kernels` comptés séparément.
   - **Arithmétique device** : contrat retenu — `DI128` (`dint.hpp`) pour
     les formes et les scans ; `__int128` autorisé en code device **là où
     `wide.hpp` l'emploie déjà** (`cmp_2p2_jb2` via `mul_192x128_320`,
     pont `di_to_i128_hd`), nvcc 12.9 le supportant avec GCC hôte. Ce choix
     de backend est documenté et mesuré, jamais un claim ; la session
     `24b3f164` compile ce chemin (témoin q3) et la session `2e75cb42` le
     chemin q4.
   - 5b **écrit, en attente de G4** (session `2e75cb42`) :
     `src/gpu/q4_kernels.cuh` — `k_q4_core` (warp par seed, six compteurs par
     ballot, correction intra-warp au h4-ième témoin), `k_q4_complete` (bloc
     par seed vivant, étage `Q4Stage` par paire de la lentille), `k_q4_depth`
     (warp par paire candidate, `q4_power_d` par ballot) ;
     `src/gpu/q4_lane_device.cuh` — `Q4DeviceExecutor` (l'hôte calcule les
     offsets de paires des seeds vivants, compte les étages, compacte les
     candidates dans l'ordre de la production entre les kernels) ;
     `tests/q4_lane_device_gate.cu` (même contrat que la porte hôte : post-RLE
     et dix-neuf compteurs). Pilote `cli/mhgp5_cuda.cu` (`--gpu` : hooks
     `RunOptions::q3_override` / `q4_override`, sans mutants ; sans `--gpu` :
     témoin CPU du même binaire). La campagne construit et exécute les portes
     q4 device dans `gpu_lane` puis, en **phase 3**, les quatre contrats
     50 000 par `mhgp5_cuda --gpu` dont `digest_all` doit être **identique**
     au contrat CPU de la même famille — l'égalité de bout en bout à 50 k est
     ainsi jugée par le validateur, et les temps GPU/CPU sont gravés dans le
     même reçu (mesure, jamais un claim).
   - Reste après la session : lecture des temps (le lotissement et les
     transferts par lot sont la première marge), banc apparié CPU 48 fils /
     GPU sur `eight_clusters` et `scanline_single_pass`, et le port du fold
     n'est **pas** prévu (séquentiel par nature, déjà recouvert).
