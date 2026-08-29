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
rejoué) — pourquoi le device ne rapporte rien.** Temps *hôte* cumulés sur
48 fils (sommes de temps-exécuteur, **pas des murs** — voir l'instrument
recevable ci-dessous), `uniform` 50 k (lane q4 CPU 3,0 s, avec `--gpu` 4,3 s,
`kernel_ms` 7,5 s) : **H2D des sites 27,4 s**, **attente K1 + D2H des verdicts 42,2 s**,
boucle hôte des vivants 0,4 s, K2 1,1 s, compaction hôte 3,8 s, K3 1,5 s,
mur de `scan()` 76,4 s ; lane q3 : H2D 1,4 s, **K1 0,20 s pour 87 M seeds
(2,3 ns par seed)**, D2H 0,14 s, mur 6,9 s. Mêmes proportions sur
`eight_clusters` (q4 : H2D 38,9 s, K1 66,3 s, mur 114,7 s), `scanline` et
`terrain`. Lecture : (1) le calcul device est négligeable — le noyau q3 tient
en 0,2 s cumulés, le cœur q4 en quelques secondes ; (2) le chemin est borné
par les **copies H2D des covers** : chaque ancre envoie son cover entier, sept
tableaux i64 et un `PointId` u32 = 60 o par site, plus seeds, ancres et
lentille — « ≈ 37 Go par lane » et « ≈ 9 Go/s » étaient des *estimations*
(aucun compteur d'octets à ce pin ; l'instrument ci-dessous les grave) ;
(3) **48 fils** à exécuteurs `thread_local` se disputent un seul device
(moteur de copie, file de lancement) et chaque étape est synchrone
(`cudaStreamSynchronize` après K1, K2, K3 — et après H2D à ce seul pin, barrière
d'instrument retirée depuis) : le temps hôte d'un fil contient l'attente des 47
autres ; (4) le reste de la lane (candidats, covers, assemblage des lots,
émission) reste hôte — sa part n'est **pas** déductible de ces sommes (le calcul
« 4,3 s − 76,4/48 » de la première rédaction mélangeait un mur de lane et une
moyenne de temps-exécuteur ; retiré). Conséquence : le réglage de lots ou de
seuil déplace la charge (adaptatif : q4 cumulée 114,7 → 54,5 s sur
`eight_clusters`) sans établir un avantage contre le CPU ; le gain exige la
forme de la doctrine (§ 1) — index et
positions **résidents sur device** (96 Go), covers, tests d'ancre, grille,
seeds et cœurs calculés **sur device par rectangle** sans aller-retour, un
petit nombre de flux asynchrones à double tampon, aucune boucle hôte par seed
ou par paire — c'est-à-dire la livraison 7 conçue avec l'architecture tuilée
(`ECHELLE.md`), où une tuile est l'unité de travail device. Tant que ce n'est
pas fait, le chemin device reste un exécuteur prouvé exact et non autoritaire,
sans usage en campagne. Lecture recevable au sens de l'audit : **q3-kernel
petit, transferts/orchestration et concurrence fortement suspects, causalité
q4 encore ouverte.**

**Instrument recevable (après `ab2c2563`, réponse à la réception de
`63deda74`)** — le contrat canonique est tenu ici, ses portes dans
`PLAN_DE_TESTS.md` et ses sorties brutes dans
`../receipts/campagne_g4_v5_20260828_instrument_scale/`. Les demandes de
l'auditeur, et ce que le code fait désormais :

- « `sg3.wall` et `sg4.wall` additionnent des `scan()` concurrents ; ce ne
  sont pas des murs et ils ne se soustraient pas à `rects` » → renommés
  `executor_ms_sum` ; **toutes** les durées de `gpu::DeviceExecutorStats`
  sont des sommes de temps-exécuteur sur les fils, documentées comme telles
  dans l'en-tête ; le mur de lane (`t_rects_ms`, mesuré par le fil appelant,
  assemblage hôte compris) est imprimé explicitement `lane_wall_ms` ;
- « la barrière `cudaStreamSynchronize` après H2D change l'ordonnancement
  qu'elle attribue » → **retirée** ; H2D mesuré par événements comme en q3
  (événement avant les copies, événement après), récoltés après les
  synchronisations qui existaient au pin `82f613d3` (K1, K2, K3) ; la « sync
  fin » sur flux vide est retirée aussi ; l'instrument n'ajoute **aucune**
  synchronisation ; les événements sont créés une fois par exécuteur ;
- « `k1+d2h`, `k2+d2h`, `k3+d2h` mêlent réserve, H2D d'offsets, kernel et
  retour » → séparés par événements `h2d | k1 | d2h1 || h2d2 | k2 | d2h2 ||
  h2d3 | k3 | d2h3` ; côté hôte, `steady_clock` entre deux points hôte :
  `reserve` (cudaMalloc transactionnels), `enfilement` (appels CUDA
  asynchrones : driver, contention entre fils), `attente` (dans les
  synchronisations existantes), `hote1` (vivants/offsets), `hote2`
  (compaction), `hote3` (émissions), `reste` non classé — le résidu q3 signalé
  (`wall` − H2D − K1 − D2H) est ainsi publié par poste ;
- « aucun octet, percentile de lots ni pic de flux » → `h2d_octets` /
  `d2h_octets` cumulés par lane (`sizeof` exact des copies enfilées :
  sites 60 o + seeds + ancres + lentille + vivants/offsets + candidates ;
  retours verdicts, un octet par paire, un octet par candidate) ; lots
  `p50`/`p95` par classe log2 (`Log2Hist` dans `BatchStats` : 65 compteurs,
  un échantillon par vidage, aucune allocation par lot ; `p50<2^k` signifie
  que la valeur de rang médian est < 2^k et ≥ 2^(k−1)) et `max` exact ;
  `flux_pic` = pic de `scan()` simultanés (compteur atomique RAII, remis à
  zéro par lane) ;
- `kernel_ms` de la ligne `gpu=1` (forme **inchangée**, validateur inchangé)
  devient la somme des **kernels seuls** (q3 : K1 ; q4 : K1 + K2 + K3),
  homogène entre lanes — il ne contient plus les retours ni les boucles hôte ;
- sortie : deux lignes `gpu_q3_etapes …` et `gpu_q4_etapes …` après la ligne
  `gpu=1` (`cli/mhgp5_cuda.cu`).

Preuve hors nvcc : porte CPU `mhgp5_gpu_instrument_gate`
(`tests/gpu_instrument_gate.cpp`) — classes log2 et quantiles gravés, fusion
`add_from`, `BatchStats` des lanes par lots hôte (un échantillon par vidage,
classe du max, plancher `--min-flushes`, un et quatre fils), pic sous barrière
(plancher ≥ 2 fils retenus), RAII sous exception, `DeviceExecutorStats` ;
mutants `log2hist-class-shift` et `gauge-no-peak` tués (code 4) ; TSan propre.
Les `.cuh`/`.cu` sont vérifiés en syntaxe C++20 par un stub CUDA ; **ni
compilation nvcc ni exécution G4** à ce commit : la prochaine session G4 doit
rejouer les contrats `--gpu` au nouveau pin et graver ces deux lignes avant
toute lecture ; aucune conclusion de débit device n'en est tirée ici.

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

## Lane résidente sur device — conception (L7, 28 août 2026)

Sources : flux de conception contradictoire
(`docs/analyses/gpu_20260828/`), reçu 12 et décisions G0–G2 intégrées dans
ce document. L'autorité courante et son pin sont tenus dans
`audits/ETAT_COURANT.md` ; les résultats et hashes restent dans `receipts/`.
Cadre inchangé, `public_status=not_claimed` ; aucun débit n'est cité sans le
compteur d'occupation qui l'accompagne. Les sommes de temps-exécuteur ne sont
jamais interprétées comme un mur.

### Décision

Le chemin device actuel (48 exécuteurs `thread_local` synchrones, covers
copiés à 32–64 o par site et par ancre) est non viable par construction (reçu
12). Trois changements bornés, **dans l'ordre des auditeurs** :

- **G0 — pool d'exécuteurs persistant et borné** (1, 2, 4 ou 8), découplé des
  producteurs CPU : file bornée de descripteurs de lots avec contre-pression,
  flux/événements/tampons créés une fois par lane, résidus soumis à la même
  file (plus jamais vidés séquentiellement après la jointure), sortie numérotée
  et fusionnée dans l'ordre déterministe actuel ; critère : mêmes digests et
  compteurs, `executors_created` en baisse, mur de lane en baisse — aucun gain
  de bout en bout exigé.
- **G1 — géométrie résidente et covers par indices** : positions quantifiées
  et `PointId` du `CloudIndex` téléversés une fois (objet RAII `GpuGeometry`
  partagé en lecture seule) ; chaque cover n'envoie que ses indices u32 dans
  l'ordre du cover, les paramètres d'ancre et de petits offsets ; le kernel
  reconstruit $u = 2z - a - b$ et $q = u \cdot u - D^2$ en entiers exacts (q3 :
  ≈ 32 → 4 o/site ; q4 : ≈ 60 → 4 o/site) ; deux wires conservés en parallèle
  (SoA actuel et `site_index_u32`) jusqu'à réception CUDA bit à bit
  (verdicts, profondeurs, émissions, compteurs, digests), compteur H2D et borne
  par lot incluant le téléversement amorti ; fixtures : bornes u16, $D^2$
  maximal, cocirculaire, permutation conservant l'ordre du cover, cover vide,
  lot surdimensionné.
- **G2 — compaction q4 stable sur device** : scans/sélections stables entre
  K1, K2 et K3 (seeds vivants dans l'ordre initial, paires dans l'ordre
  (seed, lentille)), un seul retour final (émissions et compteurs agrégés) ;
  la stabilité est contractuelle et chaque tableau intermédiaire est comparé
  sur de petits lots avant le digest final. G1 précède G2.

Au-delà (L7a/L7b/L7c, conception `docs/analyses/gpu_20260828/synthese.txt`) :
index radix **résident** (40 o/point : 400 Mo à 10 M), rectangles vivants
envoyés par lots de 16 o, handles et ancres calculés sur device (K0 warp par
rectangle, K1 bloc par rectangle sous contrat $n_a^2 + n_b^2 \le 2^{17}$),
cover **paresseux** par ancre (histogramme des 32 classes radiales sans
écriture, seules les classes 0..10 et le préfixe utile matérialisés en mémoire
partagée, queue dans une arène seulement si un seed survit), tests d'ancre,
grille de cellules (deux pointeurs, `cnt[16][16]` en mémoire partagée),
seeds, cœur/corde, complétions et profondeur sur device, émission de
**candidats compacts** (`CandD` 16 o : quatre indices, arité par sentinelles)
compactés par bloc ; clé exacte, niveau, RLE, préfiltre, census, fold et
digest restent hôte (census device = second étage optionnel, L7c). Régimes
par ancre : S (cover ≤ 1024 sites, tout en mémoire partagée), M (préfixe en
mémoire partagée, queue en arène), L (cover > 350 k sites, contrat K1 ou
lentille dépassés) routé au corps hôte de production avec compteur
`routed_host` exposé.

### Exactitude et déterminisme

Prédicats existants réutilisés tels quels (`lanes/device_forms.hpp`,
`gpu/*_shaped.hpp` : DI128/U192, formes q3/q4, puissances, centre strict,
`in_spindle_d`) ; à écrire avec fixture d'égalité hôte/device : `bin_of_d`,
`spindle_sector_d`, `cell_grid_d` (deux pointeurs en DI128), `di_to_double_d`
(décalage à bit collant, un seul arrondi, `ldexp` exact — preuve à inscrire
dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`), `seed_affine_d`
(séquence FMA figée, `-fmad=false`), `isqrt128_floor_d` (boucle bornée
prouvée, jamais un « ±1 » sur `sqrt(double)`). Filtre flottant seulement là
où un repli exact existe. Objet : multiensemble de candidats → RLE →
`digest_balls` identique CPU/GPU (cinq familles, $s$ = 6/8/10, 8000–50 000) ;
ordre brut jugé à un fil seulement (`raw_order_gate` avec rangs de seed et de
lentille) ; compteurs en trois classes (exacts, de politique, de mesure) ;
verdict fonction pure du rectangle (porte à deux découpages de lots) ; mutants
device résolus en index constant dans `kMutants` avec masque `__constant__`
par lot.

### Ce qui n'est pas promis

Aucun « GPU 10× » : à 50 k `uniform` le plafond d'Amdahl est 1,10× tant que
fold, digest, préfiltre et census restent hôte ; la valeur de G0–G1 est la
viabilité du device (fin des 48 exécuteurs synchrones et des 37 Go de bus).
`k_q4_core` mesuré à 7,5 s de « kernel_ms » (ancienne sémantique : tout le
lot après H2D) pour 3,0 s de lane CPU n'est pas encore décomposé sous nvcc :
la session de réception de l'instrument (kernels seuls, événements, octets,
pic de flux, cycle de vie) précède tout budget L7b ; la requalification V29
des auditeurs (941 M seeds tués à ~10 µs ≈ 196 s contre 214,5 s mesurées à
`scanline` 200 k) montre que le modèle par seed et le reçu 11 concordent. À 10 M, le test de profondeur à la génération est
le poste dominant identifié et non résolu (V24) ; le pic mémoire hôte des
`BallCandidate` relève d'`ECHELLE.md`, pas de L7. Occupation, spills, ratio
visites/cover, fraction routée hôte : mesurés dans chaque reçu, jamais
déclarés.

### Session 13 (pin `c95cfa95`, instrument recevable + pilote `SCALE_THREADS`) — les nombres qui décident

Reçu `campagne_g4_v5_20260828_instrument_scale` (39 runs, validateur du pin
rejoué, digests CPU/GPU identiques). Sommes d'exécuteur sur 48 fils, 50 k :

| lane, famille | mur de lane | kernels (K1 + K2 + K3) | H2D | enfilement | réservations | cycle de vie (98 exécuteurs) |
|---|---:|---:|---:|---:|---:|---:|
| q4 `uniform` | 3,3 s | **0,85 s** | 17,2 + 13,7 s, **40 Go** | 33,2 s | 12,6 s | 172 s |
| q4 `eight_clusters` | 9,4 s | 1,27 s | 30,8 + 16,7 s, **95 Go** | 49,7 s | 14,7 s | 655 s |
| q3 `uniform` | 2,3 s | 0,23 s | 3,9 s, 20 Go | 4,3 s | 2,1 s | (idem) |
| q3 `scanline` | 2,2 s | 0,09 s | 15,0 s, 28 Go | 15,2 s | 11,9 s | (idem) |

Le calcul device est de l'ordre du pour cent du temps d'exécuteur ; les
postes sont, dans l'ordre, les **copies** (64 o par site et par ancre, et la
copie des candidats avant K3), l'**enfilement** des appels CUDA par 48 fils
sur un seul pilote, les **réservations** par exécuteur et le **cycle de vie**
de 98 exécuteurs éphémères — exactement G0 (pool persistant, livré au pin
suivant : `ExecutorPool`, `--gpu-executors`, porte hôte et mutants) et G1
(indices u32 : 64 → 4 o par site). `kernel_ms` de la ligne `gpu=1` est
désormais la somme des kernels seuls : les valeurs des sessions ≤ 12 ne sont
pas comparables.

**Pilote de scaling CPU** (protocole A des auditeurs : `eight_clusters`
16 000, `fold_inflight = 1`, une répétition, `time -v` gravé) :

| fils | 1 | 2 | 4 | 8 | 16 | 32 | 48 |
|---|---:|---:|---:|---:|---:|---:|---:|
| mur, digest OFF | 215 s | 123 s | 65 s | 38 s | 22 s | 17 s | 15 s |
| accélération | 1 | 1,75 | 3,3 | 5,7 | 9,6 | 12,8 | 14,1 |
| mur, digest ON | 217 s | 124 s | 68 s | 41 s | 26 s | 21 s | 19 s |

Lecture bornée (un passage, non contrebalancé) : le gain est quasi linéaire
jusqu'à 8 fils puis sature vers 14× à 48 vCPU ; le digest coûte 2–4 s fixes
(séquentiel par ordre). La prochaine marche multi-CPU est celle décrite par
les auditeurs (pool persistant, subdivision des rectangles lourds, fusions par
offsets, fold vivant), pas `threads = 48` seul.

### G0 et G1 livrés côté hôte (28 août, réception CUDA en attente)

- **G0 — pool persistant** (`src/gpu/executor_pool.hpp`, `--gpu-executors=N`,
  N ∈ [1, 8], défaut 4) : exécuteurs créés une fois par lane et possédés par
  des fils de pool, file bornée à contre-pression, producteurs CPU bloquants
  (ordre d'émission par ouvrier inchangé), résidus dans la même file,
  exceptions relancées dans le producteur soumettant.
  **Quatre corrections de sûreté** demandées par l'audit du 28 août et livrées
  le même jour :
  (1) *ticket* — la notification se fait sous `Ticket::mu` et plus rien n'est
  touché du ticket ensuite ; auparavant un réveil spurieux laissait le
  producteur détruire son ticket de pile avant `notify_all()`. **ThreadSanitizer
  voit la différence** : l'ancien pool sort une course
  `pthread_cond_destroy` ↔ `~Ticket` (code 66), le nouveau sort 0 avertissement ;
  (2) *démarrage transactionnel* — l'`Executor` est construit sous capture
  d'exception, le constructeur du pool n'ouvre qu'une fois les N fils **prêts**
  et, en cas d'échec (exécuteur ou lancement de fil après démarrage partiel),
  ferme, réveille, joint puis relance ; auparavant l'exception s'échappait du
  corps du fil (`std::terminate`) ;
  (3) *réentrance* — une soumission depuis un travail du même pool est refusée
  immédiatement (pile de pools `thread_local`), au lieu de bloquer (certain à
  N = 1) ;
  (4) *domaine* — 0 et 9 sont **refusés**, jamais clampés ; tout entier de 1 à
  8 est accepté, 3 compris.
  Porte hôte `mhgp5_executor_pool` : **pic d'activité déterministe** par latch
  (N travaux retenus jusqu'à ce que les N soient actifs, donc pic **exactement**
  N) à N = 1, 2, 4, 8 — l'ancienne porte, dépendante de l'ordonnanceur, échouait
  78 fois sur 100 sous `taskset -c 0` ; la nouvelle y sort 0 **30 fois sur 30**.
  Elle exige aussi l'ordre par producteur, N exécuteurs construits, l'exception
  propagée, le refus de réentrance, le refus de domaine, et la contre-pression
  à `queue_cap = 1` (pic de file ≤ 1) ; plancher 1 000 travaux, `TIMEOUT`
  explicite, propre sous ASan/UBSan **et** TSan. Mutants `pool-serial`,
  `pool-drop-exception` (code 4).
  **Primitive hôte de fermeture fatale, raccord device encore ouvert** :
  `close_fatal(exception_ptr)` rend le pool à usage unique — l'admission ferme,
  la première erreur est mémorisée,
  les tickets **en file sont annulés** avec elle (jamais laissés en attente),
  toutes les attentes sont réveillées et les exécuteurs des travaux actifs ne
  sont plus réutilisés ; une exception de travail *ordinaire* reste, elle,
  récupérable et le pool continue. La comptabilité de fermeture est vérifiée :
  `soumis = réussis + échoués + annulés` et `actifs = en file = 0` — la porte
  bloque les deux exécuteurs pour que l'annulation ne soit pas vide (mesuré :
  soumis 6 = réussis 2 + annulés 4, six producteurs reçoivent l'erreur, aucun
  n'est laissé en attente). Ce scénario reçoit le mécanisme hôte explicite,
  pas encore le confinement d'une panne CUDA : les wrappers q3/q4 propagent
  actuellement une exception ordinaire via `submit_and_wait(ex.scan)` sans la
  convertir en erreur typée ni appeler `close_fatal` avant la prise d'un autre
  lot. Les quatre dents restantes sont tenues dans
  `audits/ETAT_COURANT.md` et `audits/QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`.
- **G1 — géométrie résidente et covers par indices (q3)** : `GpuGeometry`
  (positions uniques i32 téléversées une fois par lane, RAII) ; lot q3 porteur
  de `site_index` (u32 par site) et d'une géométrie par ancre
  ($a + b$, $D^2$) ; kernel `k_scan_idx` (corps commun `k_scan_body` avec
  fournisseur de sites SoA ou indices) reconstruisant $u = 2z - (a + b)$ et
  $q = u \cdot u - D^2$ en i64 exacts — les mêmes entiers que
  `fill_affine_sites`, donc le même filtre flottant certifié ; **32 → 4 o par
  site** ; deux wires conservés (`--gpu-wire=soa|index`, défaut SoA jusqu'à
  réception), portes device `mhgp5_q3_lane_device_*_wire_index` (label
  `gpu`) exigeant le même vecteur de verdicts et les mêmes compteurs que la
  production ; octets H2D gravés wire par wire. Réception : prochaine session
  G4 (compilation nvcc, portes `gpu`, contrats `--gpu --gpu-wire=index`,
  digests identiques). La lane **q4** a le même wire (`Q4SitesDev` à
  accesseurs : indices + géométrie et `PointId` résidents, **60 → 4 o par
  site**, portes `mhgp5_q4_lane_device_*_wire_index`) ; reste G2.

### Ordre de commits (auditeurs)

1. Finir l'instrument (parseur au format réel, mutants et CMake, cycle de vie
   CUDA dans la décomposition, timelines non additives) — sans GCP.
2. G0 pool persistant : 1/2/4/8 exécuteurs, résidus dans la file, égalité des
   sorties.
3. G1 wire indices : `GpuGeometry` + `site_index_u32` en parallèle du wire
   actuel ; réception CUDA à 50 k.
4. G2 compaction q4 device : portes des intermédiaires, puis ablation du D2H.
5. Pilote CPU sous cpuset (protocole A/B, ≤ 24 runs) seulement après
   réception du protocole local et de son budget de session.

Verrous à poser aux auditeurs : `audits/QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`
(V17–V30).
