# NOTE_CLAUDE — lecture du reçu G4 série C `session_g4_20260901_b97f20ea4b8f_1788293187`

Date : 1er septembre 2026 (soir). Reçu immuable commité `852ca703`, SHA
d'exécution `b97f20ea4b8f0932761c4719b32bb14b8c1f0395`, génération VM
`2026-09-01T13:06:27.081-07:00`, arrêt certifié `TERMINATED` (tentative 1),
validateur épinglé : `campaign_status=verifie_non_decisionnel` — **58 runs
valides / 58** (48 matrice, 4 attributions, build CUDA, inventaire + 16
portes gpu, 4 pilotes). `public_status=not_claimed` inchangé ; rien ici
n'est une décision, l'arbre § 5.10 est appliqué en lecture et reste
suspendu à votre audit du reçu.

## 0. Contrôles manuels du § 5.16 (exigés avant réception) — CONFORMES

- (a) `commande=` des quatre attributions : égalité EXACTE avec le vecteur
  `taskset -c <liste demandée> ./build-v6/mhgp6_profile --family --n --s=8
  --smax=11 --seed=3 --threads --fold-inflight --fold-join` (12 jetons),
  aucun argument supplémentaire ni dupliqué (script de contrôle rejoué sur
  les quatre statuts).
- (b) `arch_compilees=120` dans les quatre en-têtes pilote ; `device` et
  `sm=12.0` identiques à l'identité du build (RTX PRO 6000 Blackwell Server
  Edition, `GPU-e62a3510…`, CC 12.0, driver 580.173.02, nvcc 12.9).

## 1. Pilote série C (50 000 points, 48 fils, 4 répétitions ABBA retenues)

Murs (ms, MÉDIANES homogènes des 4 répétitions retenues ; étendue < 1,5 %) — rectifié après § 5.17 (la première version mélangeait médianes et premier record) :

| famille | `mur_cpu` | `mur_route_device` | gain | étage CPU prefiltre+census | étage device | dont wire (hôte) | H2D | kernels | D2H | rebuild (hôte) | lots |
|---|---|---|---|---|---|---|---|---|---|---|---|
| uniform | 59 011 | 52 891 | −10,4 % | 14 017 | 7 717 | ~2 640 | ~172 | 154 | ~77 | ~4 110 | 11 |
| eight_clusters | 70 462 | 63 866 | −9,4 % | 13 799 | 7 295 | ~2 500 | ~162 | 154 | ~71 | ~4 000 | 10 |
| terrain | 18 616 | 17 896 | −3,9 % | 2 021 | 1 277 | — | 33 | 23,5 | 14,6 | — | 2 |
| scanline_single_pass | 15 048 | 14 627 | −2,8 % | 1 625 | 1 186 | — | 30,7 | 17,9 | 13,2 | — | 2 |

Faits (uniform, record 1) : `nb_total=21 622 341` boules, H2D = 1,9 Mo
d'index + 2 421 702 192 o de boules + 2 162 234 100 o de sentinelles,
D2H = 2 162 234 100 o (formules 112/100/100 recalculées par le juge et le
validateur), `lot_effectif=2^21`, 11 lots — la frontière multi-lots est
exercée. Parité recalculée (signatures) à chaque record, juge embarqué
conforme après chaque famille.

Lecture : **les kernels ne sont pas le coût** — 154 ms pour 21,6 M de boules
(1,1 % de l'étage CPU qu'ils remplacent), et les transferts tiennent en
250 ms (≈ 27 Go/s en H2D). L'étage device de 7,7 s est dominé par l'HÔTE :
sérialisation du wire 2,6 s + reconstruction 4,1 s = 6,75 s (88 %). Le gain
net sur le mur (−10 %) est donc plafonné par du code hôte de la couture C5,
pas par le device. Sur les familles peu denses (terrain, scanline) l'étage
est déjà petit et le gain marginal.

## 2. Matrice CPU (sous-plan pré-enregistré, statut `verifie_non_decisionnel` ; uniform, une graine, trois blocs d'ordre déterministes, étendue par point ≤ 1,58 %)

| point | mur (s) | contraste |
|---|---|---|
| 16000 · T16 · i2 · j0 | 21,6 | référence T16 |
| 16000 · T48 · i2 · j0 | 13,6 | T48/T16 = 1,59× (T16 = 16 cœurs physiques, T48 = 24 cœurs + SMT sur cette topologie : le facteur ne démontre pas à lui seul une part sérielle) |
| 16000 · T48 · i1 · j0 | 15,8 | inflight 1→2 : −14 % |
| 16000 · T48 · i4 · j0 | 13,5 | inflight 2→4 : 0 |
| 16000 · T48 · i2 · **j1** | 18,4 | join=1 : +35 % |
| 16000 · T48 · i2 · j0 · **avec digest** | 16,9 | digest : +3,2 s (+24 %) |
| 50000 · T48 · i2 · j0 | 48,0 | contrat 50k |
| 50000 · T48 · i2 · j1 | 64,8 | join=1 : +35 % |
| 50000 · T48 · i2 · j0 · avec | 58,9 | digest : +10,9 s (+23 %) |
| 50000 · T48 · i2 · j1 · avec | 83,0 | cumul |

Étages du run 50k T48 j0 (cumuls par étage, mur 48,0 s) : gen 11,8 s (wspd
7,1 + rects 4,7), rle 1,9, prefiltre 7,0, census 6,9, expansion 1,9, fold
cumulé 38,5 (mur du fold en vol 20,3 s, inflight 2). Affinité demandée =
effective = cpuset `0-47`, masque recalculé par le validateur.

## 3. Attribution (mhgp6_profile, jamais un mur)

| run | mur | Σ reduce | part | matérialisation | part du reduce | pre | touch | post | partition | unite |
|---|---|---|---|---|---|---|---|---|---|---|
| 50000 T48 i2 j1 | 67,5 s | 25,8 s | 38 % | 8,1 s | 31 % | 5,0 | 4,5 | 3,6 | 2,4 | 1,2 |
| 16000 T48 i2 j0 | 13,9 s | 8,6 s | 62 % | 3,1 s | 36 % | 1,4 | 1,5 | 1,2 | 0,7 | 0,4 |
| 16000 T16 i2 j0 | 22,0 s | 8,0 s | 36 % | 2,7 s | 34 % | 1,4 | 1,4 | 1,1 | 0,7 | 0,3 |
| 16000 T48 i2 j1 | 19,1 s | 7,2 s | 38 % | 2,4 s | 34 % | 1,1 | 1,3 | 1,1 | 0,6 | 0,3 |

`somme = mur_reduce_interne` à 1 ms près sur tous les K (fermeture
exacte). Rectifié après § 5.17 : sous `join=0` la somme des réductions
RECOUVRE plusieurs B et n'est pas une fraction du mur (16k/T48 : 8,585 s de
réductions pour ~5,957 s de mur du fold) ; `materialisation_tri_copie` est
le premier poste interne (31–36 %) mais pas une majorité — `touch`, `pre`,
`post_remplissage` et `partition` pèsent ensemble davantage ; `init` et
`liberation` sont plus petits que `unite`. Le reduce varie peu entre T16
et T48 (8,0 vs 8,6 s à 16000). Confirme la direction de la matrice locale
(62cd2e28) sur la machine cible.

## 4. Application en lecture de l'arbre § 5.10

1. **Réglage de débit à conserver : 48 fils, inflight=2, join=0** (inflight
   1→2 : −14 % ; 2→4 : rien ; join=1 : +35 % de mur mais RSS médian 17,99 →
   13,99 Gio à 50k — `join=1` reste le mode de pression mémoire documenté,
   pas un réglage à proscrire).
2. **`materialisation_tri_copie` premier poste interne** (31–36 % du reduce,
   ~8 s cumulés à 50k) → l'arbre sélectionne UNE SONDE **CompactDelta**
   (palier synchrone, `ForestResult` complet comparé, temps et octets
   mesurés séparément sur CPU), pas encore « le » levier CPU prouvé.
3. **Série C** : le device fait le travail en 0,15 s ; le gain visible (−10 %)
   est bridé par le wire hôte (2,6 s) et le rebuild hôte (4,1 s). Plafond
   descriptif (§ 5.17) : même un étage device ramené à zéro ne donnerait que
   1,12–1,31× de bout en bout ; un simple chevauchement wire/rebuild laisse
   ≥ 4,1 s. Levier C6 utile : supprimer les matérialisations globales
   (2,42 Go d'entrée, 2,16 Go de sorties) par un traitement PAR LOT dans des
   temporaires privés avec publication transactionnelle finale — précédé de
   deux contre-sondes locales (réserve gardée `cands.size()*112`, réserve
   exacte ou deux passes de `lsurv`/`lballs`).
4. Le digest (+23 % du mur) reste un coût de mesure, hors chemin produit.

Aucune de ces lignes n'est une conclusion d'accélération publiable : ce
sont des faits de reçu et les branches de l'arbre qu'ils sélectionnent.

## 5. Chronologie GCP et réserves

- Départ 1 `5d886db1` : refusé fail-closed par la garde invitée (armement
  16 s après l'échéance GCE), reçu `…1788286152`, ~6 min SPOT.
- Départ 2 `b97f20ea` : interrompu par le redémarrage du conteneur, aucun
  reçu durable, VM arrêtée et certifiée à la reprise (~30 min SPOT).
- Départ 3 `b97f20ea` : complet, reçu ci-dessus, arrêt certifié ; écart au
  protocole d'accusé (accusé consommé par le départ 2) reconnu — tout
  quatrième départ demandera un nouvel accusé explicite.
- Correctifs § 5.16 à porter AVANT tout nouveau pin : égalité d'argv
  normalisée (attribution, matrice, pilote), `arch_compilees` comparé par
  le juge et le validateur avec mutant 86, frontières 600/601 et 480/481 s
  du budget d'armement au selftest (900 s est l'écart brut, 600 s le budget
  certifiable après la réserve GCE, 480 s avec la tolérance systemd —
  commentaire du profil à corriger), et la reprise persistante du
  superviseur (WORK, clé, artefacts hors `/tmp`, selftest de reprise).
- Dette séparée : 5 tests `tests/gcp/test_phase5_*`/`test_phase15_*`
  rouges AVANT ce chantier (rejoués identiques au parent `b1e5463e`),
  hors CI et hors série C.

GCP non utilisé par cette note.
