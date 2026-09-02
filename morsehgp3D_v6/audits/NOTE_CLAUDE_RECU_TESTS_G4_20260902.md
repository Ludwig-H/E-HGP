# NOTE — lecture du reçu G4 « tests K=10 / K=5 » (`1788312873`)

Date : 2 septembre 2026. Reçu
`morsehgp3D_v6/receipts/session_g4_20260902_c8f696739b0b_1788312873/`
(commit `e66cd978`), session `g4_tests_v1` lancée au SHA d'exécution
`c8f69673` gravé par `ACCUSE_CLAUDE_GO_G4_TESTS_20260902.md`. Faits
seulement : aucun chiffre ci-dessous ne décide de quoi que ce soit ;
`public_status=not_claimed` inchangé ; l'arbre pré-enregistré du § 5.10
reste la seule voie de décision.

## 1. Ce que le reçu certifie

- **Validateur** (`validation.txt`) : `campaign_status=verifie_non_decisionnel`,
  84 runs valides — 80 murs (40 points × 2 passages aller/retour), 2
  attributions K=10, `gpuv6_build` + `gpuv6_gates` (inventaire `-N` exact,
  16 portes, `100% tests passed out of 16`, aucun pilote comme prévu au
  profil). Aucun run tronqué, aucun code non nul.
- **Cycle de vie** : `etat_cycle_vie=targeted_stopped`, `stop_rc=0`,
  génération `2026-09-01T18:34:33.420-07:00` (unique, verrouillée avant
  tout appel mutant), `issue=arret_certifie_par_le_garde` — arrêt
  `TERMINATED` certifié sur exactement cette cible, aucun second arrêt.
  Session terminée à 01:55 UTC, ~22 min après le démarrage, pour une
  enveloppe de 285 min invité / 18 000 s GCE.
- **Reprise persistante (§ 5.18.6)** : premier reçu portant la ligne
  `marques=double_guard_verified guest_guard_pending` — les deux marques
  ont été publiées par le vrai garde (`--guard-mark-dir`), `superviseur.pid`
  écrit ; aucune reprise n'a été nécessaire ; clé purgée au reçu
  (`grep private_key` vide).
- **Moteur** : aucun diff `src/`, `cli/`, `CMakeLists.txt` entre le SHA
  série C (`b97f20ea`) et ce SHA — les nombres sont comparables au reçu
  `1788293187` à binaire identique (`binaire_sha256` gravé par phase).

## 2. Fixtures d'égalité (objet)

- **K=10, n = 32000, quatre familles** : les `digest_all` des bras
  `--digest` (`c3bb0ce9…` uniform, `67a08854…` terrain, `0143c174…`
  eight_clusters, `2bc14286…` scanline_single_pass) sont **identiques** aux
  digests gravés dans `receipts/conformite_v5/<famille>_32000.txt`, dans
  les campagnes v5 du 27 août (`campagne_g4_v5_20260827*`) et dans les
  reçus v6 du 31 août / 1ᵉʳ septembre. Même objet, à 48 fils / inflight 2
  / join 0, identique entre les deux passages.
- **K=5 (`--smax=6`), n = 32000, quatre familles** : `tower_scope=prefix_k5
  smax_requested=6 smax_effective=6`, ensemble exact `K = 1..5`, digests
  identiques entre passages (`28a46163…`, `017de455…`, `8f775f32…`,
  `c3a12555…`). Aucune référence antérieure n'existait pour K=5 à cette
  taille : ces quatre `digest_all` deviennent les **premiers témoins K=5 à
  32000**, à graver comme fixture (`GPUV6_OBJET_DIGESTS`-like) avant toute
  réutilisation.
- **Propriété de préfixe, vérifiée à la main sur ce reçu** : pour chaque
  famille, `digest_forest_K1..K5` **et** `cardinalites K=1..5` du run
  `smax=6` sont égaux à ceux du run `smax=11` du même passage — le K=5 est
  bien le préfixe exact de l'objet complet, comme l'affirme la ligne
  `tower_scope`. Le validateur ne vérifiait pas encore cette égalité
  croisée entre jumeaux `smax` d'un même reçu ; elle est ajoutée au
  validateur avec sa falsification (voir § 5).

## 3. Murs CPU, K=10 contre K=5 (48 fils, inflight 2, join 0, sans digest)

Moyenne des deux passages ; dispersion entre passages ≤ 1,6 % partout
(≤ 2,8 % sur les murs sub-seconde). Mémoire = `rss_max_kb` maximal des
deux passages.

| famille | n | mur K=10 (ms) | mur K=5 (ms) | ratio | rss K=10 (Mo) | rss K=5 (Mo) | ratio |
|---|---|---|---|---|---|---|---|
| uniform | 8000 | 6196 | 1236 | 5,01 | 3261 | 661 | 4,93 |
| uniform | 16000 | 13342 | 2573 | 5,19 | 6514 | 1219 | 5,35 |
| uniform | 32000 | 29249 | 5571 | 5,25 | 11920 | 2369 | 5,03 |
| uniform | 50000 | 47677 | 9083 | 5,25 | 18515 | 3891 | 4,76 |
| terrain | 8000 | 1148 | 368 | 3,12 | 681 | 199 | 3,43 |
| terrain | 16000 | 2771 | 847 | 3,27 | 1376 | 388 | 3,55 |
| terrain | 32000 | 7748 | 2246 | 3,45 | 2340 | 766 | 3,06 |
| terrain | 50000 | 16987 | 4867 | 3,49 | 3894 | 1212 | 3,21 |
| eight_clusters | 8000 | 5968 | 1303 | 4,58 | 2783 | 597 | 4,66 |
| eight_clusters | 16000 | 14252 | 3058 | 4,66 | 5911 | 1203 | 4,91 |
| eight_clusters | 32000 | 34649 | 7359 | 4,71 | 10689 | 2275 | 4,70 |
| eight_clusters | 50000 | 60920 | 12889 | 4,73 | 16214 | 3794 | 4,27 |
| scanline_single_pass | 8000 | 1094 | 349 | 3,14 | 686 | 204 | 3,36 |
| scanline_single_pass | 16000 | 2405 | 735 | 3,27 | 1267 | 369 | 3,43 |
| scanline_single_pass | 32000 | 6545 | 1960 | 3,34 | 2435 | 790 | 3,08 |
| scanline_single_pass | 50000 | 13336 | 4203 | 3,17 | 3751 | 1114 | 3,37 |

Lecture factuelle :

- Le passage de K=10 à K=5 divise le mur par **≈ 5** sur les familles
  denses (uniform, eight_clusters) et par **≈ 3,2–3,5** sur les familles
  minces (terrain, scanline) ; le rapport croît lentement avec `n`. La
  mémoire suit le même rapport — la résidence est dominée par les forêts
  hautes, pas par la génération.
- **Pentes log-log 8000 → 50000** (mur) : uniform 1,11 / 1,09 (K=10 /
  K=5), eight_clusters 1,27 / 1,25, scanline 1,36 / 1,36, terrain 1,47 /
  1,41. Les pentes sont **indépendantes de K** à 0,06 près : réduire la
  hauteur de la tour change la constante, pas l'exposant. Le segment
  32000 → 50000 est plus raide sur les familles minces (terrain 1,76,
  scanline 1,60–1,71) — quatre tailles ne suffisent pas à trancher entre
  une pente asymptotique et un effet de cache ; à mesurer, pas à conclure.
- Pentes mémoire 0,93–1,01 : linéaires en `n` pour les deux K.
- Surcoût du `--digest` à 32000 : +14 à +23 % du mur à K=10, +11 à +19 %
  à K=5 (le digest est un cumul par étage, hors des murs « sans »).

## 4. Attribution K=10 à 32000 et reproductibilité inter-sessions

- `attrib_uniform_n32000` : reduce séquentiel cumulé 18 235 ms pour un mur
  de 29 836 ms (61 %) ; `materialisation_tri_copie` 6 314 ms = **35 % du
  reduce**, puis `pre` 3 384, `touch` 3 096, `post_remplissage` 2 330,
  `partition` 1 615, `unite` 761. `attrib_eight_clusters_n32000` : reduce
  16 059 ms / 35 216 ms (46 %), `materialisation_tri_copie` 5 604 ms
  (35 %). Mêmes proportions qu'au reçu série C (31–36 %) : la cible
  CompactDelta du § 5.10 est confirmée à cette taille, sur deux familles.
- Uniform K=10 à binaire identique, même réglage : 16000 → série C
  13 642 / 13 580 / 13 560 ms, tests 13 234 / 13 450 ms ; 50000 → série C
  48 050 / 48 056 / 48 216 ms, tests 47 382 / 47 972 ms. Écart entre
  sessions ≤ 3 %, inférieur au contraste le plus fin de l'arbre § 5.10.

## 5. Suites (aucune décision)

1. Graver les quatre `digest_all` K=5 à 32000 comme fixture d'égalité et
   ajouter au validateur l'égalité croisée `digest_forest_K1..K(smax−1)` +
   `cardinalites` entre jumeaux `smax` d'un même reçu, avec un mutant du
   faux pilote qui la casse (refus attendu) — livré dans le commit qui
   accompagne cette note.
2. Sonde CompactDelta (palier synchrone, § 5.10 / § 5.17) : conception en
   cours ; comparaison complète du `ForestResult` à 1 et T fils avant tout
   chiffre.
3. Le segment 32000 → 50000 des familles minces (pente 1,6–1,8) mérite une
   taille de plus (64000) dans un profil ultérieur, jamais une conclusion
   sur quatre points.

GCP non utilisé par cette note (session déjà arrêtée et certifiée).
