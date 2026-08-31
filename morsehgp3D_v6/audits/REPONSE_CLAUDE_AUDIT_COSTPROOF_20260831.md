# Réponse Claude au quatrième cycle d'audit (31 août) — contrats d'échec, capacité, validateur, WSPD, comptage

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. La requalification « E6 déclenché » → « signal de triage
pour la sonde E6 » est acceptée ; la sonde est instrumentée dans ce commit.

## P1 — invalidation centralisée : FAIT

`invalidate_provisional(rr)` vide digests (raw, compat, post-préfiltre,
all), forêts, cartes et totaux ; appelée sur CHAQUE retour non complet après
la génération (grand-livre, invariant J, capacité, census, garde de
capacité des folds, défaut A, défaut B). Vos deux probes sont couvertes :
un défaut K2 ne laisse plus ni `forest_slots` ni `cards K1`, un défaut de
census ne laisse plus le digest raw.

## P1 — capacité par statut : FAIT

`candidates_capacity_ok(n)` (helper testable, borné au selftest sur les
deux côtés de 2^32−1 sans allocation) ; `run_pipeline` refuse
`resource_exhausted` AVANT `prefilter_balls` ; la garde interne reste une
défense documentée « faute d'appelant ». Le chargeur du juge exige
l'ensemble EXACT `{1..kmax_eff}` (porte gravée
`mhgp6_juge_refus_reference_tronquee` : référence réduite à K10 → code 2) ;
le refus du narrowing a sa porte (`mhgp6_juge_refus_narrowing`, votre
reproduction exacte → 2). Les plafonds `CloudIndex` et la saturation des
agrégations u64 restent des chantiers déclarés.

## P1 — validateur : FAIT, avec sa porte Python

`pentes.py` réécrit selon votre recette : matrice exacte depuis le META,
bijection STATUS↔matrice (tuple en trop — même `code=1` — refusé),
bijection des fichiers `.txt`/`.err` (en trop ou manquants refusés),
identité recoupée avec `s/smax/threads` et le mode digest, chaque compteur
exigé (dont `P_factor_q2`), tables BUFFERISÉES et imprimées seulement après
validation complète (plus aucun stdout partiel), zéro légitime = pente `-`.
La porte `mhgp6_pentes_validateur` (tests/pentes_gate.py) grave le nominal,
DOUZE falsifications (chacune code 3 + stdout vide) et le zéro légitime.

## P1 — comptage exact du coût : FAIT

`q4_core_site_tests`/`sweep_pass2_site_tests` sont requalifiés
« évaluations éligibles » (après le saut des trois indices du seed) ; les
itérations COMPLÈTES sont comptées séparément (`iters_coeur`,
`iters_passe2`) et publiées. Le grand-livre s'enrichit de : `V_wspd`
(nœuds de témoins + coins de la descente, les deux comptages par rectangle
inclus), `H_rect` (points de handles par rectangle et par lane),
`M_anchor` (tailles de cover par ancre, q3 et q4), `V_census` (nœuds,
feuilles, range-add du préfiltre/census). Le parser lit tout, q2 compris.

**Sonde de queue E6 instrumentée** (votre triage) : distribution des ancres
q4, des seeds et des évaluations de cœur par OCTAVE de taille de cover
(`octaves_q4 ancres=… seeds=… w1=…`) — la part des ancres lourdes dans
`W_sweep1` et l'origine du pic de la graine 5 deviennent lisibles. La
campagne relancée au pin de ce commit est la première candidate à un statut
décisionnel ; ses limites restantes (kills non ventilés, comparaisons de
regroupement) restent au tableau candidat.

## P1 — wspd-drop-rect : l'annihilation est corrigée

Vous aviez raison : `lout[c].empty()` supprimait tout le chunk. Le mutant
perd désormais exactement UN rectangle vivant (drop_pending, premier du
chunk 0, flag atomique échangé une fois) ; il est tué par le grand-livre de
la route produit (porte `mhgp6_fused_mutant_droprect` via le selftest,
code 4, en plus de la boucle de conformité). La requalification complète de
la descente (ownership par paire, porte littérale n−1) reste en tête du
portage WSPD.

## P1/P2 — claims : ALIGNÉS cette fois dans le même commit

- `PLAN_DE_TESTS.md` : la ligne « barrière de sortie » est supprimée, seule
  la portée « génération/census » subsiste ; comptes exacts 60 noms /
  64 points d'injection / 28 exercés.
- `PROVENANCE.md` : la bascule résiduelle du paragraphe « Non repris » est
  réécrite (monnaies gelées).
- `REGIMES.md` : `linked_arcs_u16` = barrière de génération/census, motif
  borné jusqu'à N=34, extension forêt prévue.
- « jamais libm » est désormais VRAI : `isqrt64_pure` (bit à bit, O(32),
  aucun appel flottant) remplace `floor_sqrt` sur la frontière des familles
  stationnaires, avec fixtures des deux côtés de l'arrondi au selftest ;
  `floor_sqrt` (graine flottante corrigée, exacte) reste la voie chaude et
  est documenté comme tel.
- Labels : `oracle` posé sur `mhgp6_sweep_oracle` et `mhgp6_linked_arcs_u16`.
- Rejets sémantiques gravés en portes : narrowing, référence tronquée,
  mutant hors cible (cover), `--inject=` vide (sweep), capacité (selftest).

## Renforcements non bloquants : partiellement pris

Le golden post-préfiltre grave aussi `105076/1134/103942` et le digest
v5-compat (`2b50f878…`, identique à la v5 sur cette fixture — le cover 4
n'y change rien) ; les fixtures d'arrondi de la racine stationnaire sont au
selftest. Équivalence handles/requête du cover, pinning de la perte exacte
des mutants du sweep et événement terminal de l'API de fold restent notés.

## État

59/59 portes rapides au commit (68 + 6 nouvelles − les 15 scale non
rejouées ici). La campagne `campagne_grandlivre_20260831` est requalifiée
« baseline enrichie des champs présents » comme demandé ; une campagne au
pin de ce commit, avec le grand-livre complet et la sonde d'octaves, suit.
