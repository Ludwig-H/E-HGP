# Réponse Claude à l'état courant v6 du 31 août — exécution des P0/P1

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Cette réponse accompagne le commit qui versionne le lot J0–J2
complet ; l'audit portait sur `HEAD=d9cb45db` plus le lot non suivi.

## P0 — porte de descente fusionnée : FERMÉ

Deux détecteurs causaux, cumulés dans `--fused-descent` :

1. invariant structurel : une lane émise a `core[q] < h_q` — sous
   `fused-mask-stuck`, des lanes mortes sont émises avec `core >= h_q` ;
2. votre plancher causal : sur `uniform n=700`, la masse tuée nominale est non
   nulle dans les trois lanes ; sous le mutant elle tombe à zéro partout
   (tout est versé aux émis, le grand-livre ferme quand même — constat reçu).

`mhgp6_fused_mutant_mask` rend 4. Le remplacement du bras singleton par une
descente de référence réellement indépendante reste noté comme renforcement
ultérieur ; les deux détecteurs actuels ne comparent plus deux bras co-mués.

## P0 — ce que le sweep économise : REQUALIFIÉ partout

La revendication « la boucle C×D est morte » est retirée de `generate.hpp`,
`MATHEMATIQUES.md` (C1), `ARCHITECTURE.md` et `PROVENANCE.md`. La forme
retenue est la vôtre : le **rescan de profondeur par candidat** disparaît,
l'incidence seed–complétion reste matérialisée et payée ; le coût par seed
survivant passe de `O(p_e·m_e)` à `O(m_e log m_e + p_e)`. Compteurs publiés :
`sweep_pass2_seeds`, `sweep_roots_onchord` (observable, jamais « ×log »),
`sweep_root_groups` (blocs d'égalité), `sweep_roots_offchord` (rejets exacts
par la borne de Jung — jamais énumérés), `sweep_const_interior`, et
`q4_completions` reste le `P_role` payé de cascade. `GRAND_LIVRE.md` remplace
`W_sweep2` par ces observables disjoints.

Premier chiffre indicatif (diagnostic, un run, `terrain_stationnaire`
n=2000) : 101 055 seeds en passe 2, 1,40 M racines sur corde soumises à la
cascade, **2,30 M racines hors corde rejetées exactement** — des complétions
que la route v5 aurait énumérées puis rejetées après Cramer/centre.

## P0 — contrat de profondeur q4 : CHOISI (contrat 1)

`MATHEMATIQUES.md` C3 nomme désormais les deux contrats et grave le choix
courant : le sweep balaie le cover complet et le verdict est
`depth_at(μ_d) >= h4` **sans aucun crédit ajouté** (vos témoins y figurent
déjà). Le contrat 2 (`ResidualTape`, `compose(depth_residual_at)`) est le
chantier J3, avec la formule sectorielle qui suit le même choix.

## P0 — frontière de digest : ALIGNÉE sur le code

Constat exact : le lot signe `cands` post-RLE avant préfiltre — c'est le
digest v5, et c'est précisément pourquoi la conformité passe. Les documents
sont réécrits en ce sens (`ARCHITECTURE.md`, `PROVENANCE.md`) : tant que le
multiensemble émis reste identique à la v5 (fait de construction du sweep,
vérifié par les portes), `digest_balls` sert de conformité ; le jour où un
tueur plus fort rompt légitimement cette égalité, bascule sur
`mhgp6-postprefilter-balls-v1` (votre identifiant, avec contrat de tri,
d'unicité, profil et statut encodés), et la conformité v5↔v6 reste sur
`digest_all` + forêts. Aucun commentaire n'affirme plus que la frontière a
déjà changé.

## P1 — provenance : catégorie `port_source_requalified` CRÉÉE

`PROVENANCE.md` reclasse honnêtement : ports contractuels (sha256, familles,
digest) ; **ports de source requalifiés** (toutes les transcriptions
mécaniques, diffs de renommage vérifiés à la livraison, pin `3bad233d`) ;
re-dérivés réels (`run.hpp`, `cli/mhgp6.cpp`) ; neufs (descente fusionnée,
sweep, familles stationnaires, grand-livre). Les étages `[LIVRÉ]`
d'`ARCHITECTURE.md` correspondent au commit qui porte cette réponse ;
`src/credit/`, `src/carrier/`, la route M restent `[PRÉVU]`.

Les reçus v5 sont renommés en esprit `baseline_v5_capture` : `META.txt` porte
désormais commande exacte, toolchain, date, état du worktree à la capture et
le constat stderr.

## P1 — familles stationnaires : les quatre bornes APPLIQUÉES

- `c0 = 566` partout (`REGIMES.md` aligné sur le code ; 565 était une faute
  du document) ;
- l'emprise emploie une racine carrée **entière** à règle d'arrondi explicite
  (au plus proche, milieu vers le haut), plus aucun `libm` sur la frontière du
  domaine (`detail_round_isqrt_clamped`) ;
- qualification « régimes synthétiques stationnaires, physiquement motivés »
  gravée, avec la réserve corpus capteur ;
- `scanline_stationnaire` est documentée comme hybride neuf (échos dès la
  passe principale, pas de passe de recouvrement) ; « capteur inchangé » est
  restreint aux paramètres de balayage ; les lois historiques sont corrigées
  dans `REGIMES.md` (canopée `[1, coord/8]`, échos `[2, coord/10]`).

Les statistiques demandées (cardinalité après déduplication, densité de
motifs, recouvrement) seront publiées avec la campagne stationnaire J3.

## Réponses reçues

V6-Q1 à V6-Q4 : reçues et intégrées (identifiant du digest post-préfiltre ;
ordre contrat écrit → oracle → raccord pour le sweep, avec vos fixtures de
frontière à graver en J3 ; qualification des régimes ; contre-fixture
calotte–lentille à graver littérale u16 avec marges OBig et mutant de
troncature i64).

## État des portes au commit de cette réponse

`cmake --build build/v6` : 0. Portes rapides (`-LE scale*`) : 23/23, dont
`mhgp6_fused_mutant_mask` désormais à 4. Conformité v5≡v6 aux tailles
d'intérêt : 15/15 au lot audité ; rejouée sur le binaire final au commit (voir
`receipts/conformite_v6_j2_20260831/`). L'oracle du sweep (énumération
exhaustive, sept cas dont `two_lines` et `collinear_seven`) passe ; vos
fixtures de frontière (signes de dénominateur, racines égales, extrémités de
Jung, rôles, réétiquetage) restent à graver en J3 comme demandé.
