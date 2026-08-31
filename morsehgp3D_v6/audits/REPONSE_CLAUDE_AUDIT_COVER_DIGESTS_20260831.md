# Réponse Claude au second état courant v6 (31 août) — cover q4, monnaies de digest, vérité des plans

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Cette réponse accompagne le commit qui exécute les P0/P1 de
la coupe `b17ca2cd` ; le statut « checkpoint J0–J2 borné » est accepté tel
quel.

## P0 — cover q4 : CORRIGÉ (coefficient 3 pour q3, 4 pour q4)

Votre contre-fixture est reçue et reproduite à l'identique : à
`(110,110,110)/(110,90,90)/(90,110,90)/(90,90,110)` + `z=(83,100,100)`,
`D2=800`, `power(z)=−11`, `|2z−a−b|²=2916` entre `3D²=2400` et `4D²=3200`.
`generate.hpp` choisit désormais le coefficient PAR LANE aux deux appels
(`rect_cover_handles`, `anchor_cover_from_handles`) ; la lentille de supports
est inchangée. La porte `mhgp6_cover_coef4` grave la fixture littérale
(vérifications `D2=800`, `U=2916`, `power=−11` incluses) : nominal
`raw=0/rle=0/survivantes=0` (mort À LA GÉNÉRATION), mutant `q4-cover-coef3`
(le défaut hérité v5, ajouté au registre) `raw=1/rle=1/survivantes=0`,
code 4 — exactement vos chiffres. La conformité d'OBJET aux digests v5 reste
verte après le correctif (petites tailles rejouées ; tailles d'intérêt
rejouées au commit, voir le reçu). `PISTES_FERMEES.md` § 7 supersède
honnêtement la fermeture v5 inverse. L'équivalence handles/requête directe
aux deux coefficients et la permutation à PointId conservés restent notées
comme extensions de cette porte.

## P0 — monnaies de digest : GELÉES, sans bascule conditionnelle

Le « bascule le jour où » est retiré partout. Trois contrats gelés :

- `digest_candidates_v5_compat` (tag v4 inchangé, candidats uniques
  post-RLE) : imprimé sous ce nom, diagnostic différentiel ; la conformité le
  RAPPORTE (note explicite en cas de divergence, attendue depuis le
  coefficient 4) et ne le juge jamais ;
- `digest_postprefilter` (tag `mhgp6-digest-v1:postprefilter-candidates`) :
  calculé pendant que `surv` existe, sur les records `cands[s.idx]` dans
  l'ordre canonique, sans profondeur ni copie. Votre golden `97be65b6…` sur
  `uniform 400` est REPRODUIT à l'identique et gravé en porte
  (`mhgp6_postprefilter_uniform_400`) ;
- la conformité d'objet v5↔v6 juge `digest_all` + chaque digest forestier.

Conséquence assumée : `mhgp6_conformity_mutant_rle` perdait son détecteur
(l'objet est réellement intact sous `rle-drop`, union-find idempotent) — il
est repointé sur la monnaie post-préfiltre, que les doublons de survivants
font diverger. Le nom « multiensemble émis » est corrigé (c'étaient les
uniques post-RLE).

## P1 — vérité des mutants : couverture exécutée ~30/60

Votre boucle des 20 mutants est câblée telle quelle
(`mhgp6_mutant_*` sur `eight_clusters 400`, divergence d'objet, 20/20 à
code 4 vérifiés), `fold-inject-b-exception-k3` exclu comme demandé (juge
d'in-flight dédié à porter). Avec les mutants dédiés (fused, sweep ×4, cover,
rle, oracle i64), la couverture exécutée passe d'environ 4 à ~30 noms sur 60.
`PLAN_DE_TESTS.md` est remis à la vérité : état réel par étage, tout le
non-câblé marqué `[PRÉVU]`, « liste tenue à jour » retiré, la fixture
familles décrite pour ce qu'elle teste. L'ordre de portage conseillé
(`wspd_gate`, `q3_oracle`, `cell_grid_oracle`, `prefix_gate`, spindle, fold
in-flight, render) est adopté pour la suite.

## P1 — cohérence documentaire : APPLIQUÉE

README (« socle v5 porté et requalifié, génération q3/q4 neuve », monnaies
correctes), bandeau de supersession en tête de la note fondatrice,
`ARCHITECTURE.md` E3 scindé (`[LIVRÉ] EndpointCredit + tueurs` /
`[PRÉVU] AnchorCredit/CoreCredit/ResidualTape`) et E4 au contrat 1 réel,
`PROVENANCE.md` avec `port_source_pending_requalification` par défaut,
`REGIMES.md`/`GRAND_LIVRE.md` requalifiés (« association diagnostique sur
ces runs », jamais « imputable »/« linéaire »).

## P1 — grand-livre et campagne

`sweep_root_comparisons` (comparaisons du tri) et `P_factor`
(auto-produits `|A|²+|B|²` des histogrammes, votre contre-famille grille +
singleton lointain est notée) sont câblés et publiés. Le tableau du
grand-livre marque `(candidat J3)` tout terme sans compteur
(`H_rect/H_scan/M_anchor/V_R/C_R/P_R/V_census/T_input/V_motif`). Le coût
quadratique de FABRICATION des familles stationnaires (chaque point contre
~n motifs) est nommé dans le tableau : il sera séparé (`T_input`) ou
rasterisé avant toute pente de bout en bout. `pentes.py` exige désormais
9/9 runs, `DONE`, codes 0, stderr vides, chaque compteur présent (échec
sinon), et publie les étendues par pas séparément.
`detail_round_isqrt_clamped` s'initialise par `floor_sqrt` (primitive
entière) — le commentaire « plus aucun libm » est maintenant vrai.

La campagne stationnaire épinglée `b17ca2cd` est requalifiée baseline
exploratoire ; elle est REJOUÉE au commit du correctif (votre exigence : le
cover change la charge mesurée), avec un `META` complet (SHA littéral,
commande, toolchain, heures, hashes des sorties). Les réserves sur
`linked_arcs` (égalité complète produit/oracle côté clés excédentaires,
multiplicités sous réétiquetage, portée « barrière de génération/census »)
et sur F1/F4 (lien causal à l'ancre visée) sont notées comme extensions
J3 ; la portée du vocabulaire est déjà corrigée dans `PLAN_DE_TESTS.md`.

## Reçus

`receipts/conformite_v5/META.txt` : champs enrichis qualifiés
`reconstruit_a_posteriori`, la revendication sur les 8 petits runs corrigée
en `not_recorded`. Le prochain reçu de campagne porte le SHA complet, la
commande, la toolchain, l'état du worktree, les heures et les hashes des
sorties.
