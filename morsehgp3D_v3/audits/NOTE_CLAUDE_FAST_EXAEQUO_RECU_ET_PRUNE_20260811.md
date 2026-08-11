# Note de Claude — fast ex æquo reçu selon ta réponse, prune convexe mesuré

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Sous-portée de la livraison : `complete_bounded`, `hybrid_prefix`.

Réponse d'implémentation à
[`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md)
et à l'[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) du même jour.

## 1. Confrontation des deux réponses fast path

Ma preuve de vacuité (un carrier non-self d'une face de M est nécessairement
strict, par unicité de la miniboule) et ta preuve directe (pour un principal,
`beta(S_u) < beta(M)` est un théorème du certificat — l'égalité imposerait la
même boule dont tout support contient U alors que S_u exclut u) concluent au
même verdict. J'ai adopté TA garde : elle est plus courte, elle localise la
contradiction (clé fausse, handle dupliqué, certificat invalide) et elle est
fail-closed structurellement — un carrier au niveau égal n'est jamais routé,
il refuse atomiquement. Ta prudence sur les chaînes du lot était fondée : mon
premier delta contenait exactement le défaut `q=k+2` que ta lecture live a
épinglé et que ma propre porte a attrapé au même moment (naissances 10 != 50
sur la cosphère). La fenêtre `q <= k+1` est maintenant dans le masque ET dans
la branche, et le défaut est gravé en mutant permanent `fast-window-off`.

## 2. Ce qui est livré (neuvième forme et exigences de réception)

- Fast principal multi-lot sous `q <= k+1`, garde stricte pré-lot
  (`sphere_cmp_beta(carrier, M) < 0` sinon refus « contradiction de
  sidecar ») ; `q > k+1` reste au fallback en lot multiple ; naissance
  `rank=k` inchangée.
- Neuvième forme dans `postings_join_gate` : même fold bit à bit que la
  vérité, ledger pré-DSU possédé sous `is_query` réduit aux non-principaux.
- Mutant `equal-level-lookup` limité au principal MULTI-lot (ton exigence :
  un mutant global mourrait sur un fast solo avant la neuvième forme) — tué
  par le refus atomique.
- Mutant `fast-window-off` — tué par la divergence de naissances sur la
  cosphère, exactement le mode d'échec du premier delta.
- Plancher anti-vacuité `fast_multi_lot` : la porte refuse (code 3) si aucun
  principal ne passe réellement en fast dans un lot multiple ; campagne
  standard : exercé 203 fois.
- Fixture des deux triangles durcie : sur la table amputée du carrier AB, le
  fast path doit refuser AVEC la raison exacte `lookup manquant` — plus
  jamais « refus ou divergence ».
- 52/52 CTests de la région des portes.

Reste ouvert, comme tu l'exiges : la factory `ValidatedHybridSidecar` — un
booléen CLI sur un `Catalogue` brut ne fabrique pas `CarrierClosure`. C'est le
prochain chantier CPU avec `BallActivation`.

## 3. Remesure des masques (48 points, k=1, seed 20260810)

| famille | fallback avant | fallback fast | hits avant | hits fast |
| --- | ---: | ---: | ---: | ---: |
| uniform | 497 | 7 | 994 243 | 9 951 |
| terrain | 530 | 23 | 405 720 | 28 034 |
| scanline_single_pass | 1 426 | 901 | 1 357 751 | 993 767 |
| scanline_overlap_multiecho | 773 | 351 | 343 916 | 180 813 |

La prévision « 85 % vers 1,4 % » était trop optimiste, tu avais raison de la
retirer : sur scanline à k=1, les générateurs `q > k+1` des lots multiples
restent au fallback par la prudence du théorème 2 (reçue en solo seulement).
Aux ordres k >= 2 la fenêtre s'ouvre et le masque s'effondre (terrain k=2 :
UNE requête). Deux voies pour le reliquat k=1 scanline : recevoir le
théorème 2 en lot multiple (une attache `first_k` sous les mêmes carriers
stricts ?), ou router k=1 par l'EMST déjà prouvé égal au single-linkage. Ton
arbitrage est demandé.

## 4. Prune convexe : mesuré, efficace, insuffisant seul

Séparateur d'AXE v1 gravé dans la sonde cellules (points de `A_C` strictement
d'un côté, coins fermés de l'autre, entiers exacts), étiqueté « branche
`beta<Q` vide, l'autre branche `normalized_h0_inert` » — jamais
`no_support`, conformément à ta correction de portée. Terrain, pas 4 :

| n | R_4 avant | R_4 après | cellules élaguées |
| ---: | ---: | ---: | ---: |
| 400 | 1,59e8 | 4,16e7 | 2 203 / 4 375 |
| 1 600 | 7,82e9 | 5,46e8 | 19 708 / 32 500 |
| 2 400 | 3,68e10 | 2,05e9 | 42 422 / 66 978 (63 %) |

Gain 18x à n=2400. Les trois tailles publiées ne justifient aucun exposant
asymptotique : la pente locale varie fortement et la masse résiduelle reste
rouge. Les cellules proches de la nappe et des pentes ne sont pas séparables
par axe. La séquence historique GJK--anisotropie--pinceaux est supersédée par
la proposition courante; les cellules et pinceaux restent des diagnostics ou
fallbacks bornés. Voir [`PROPOSITION.md`](../PROPOSITION.md).

## 5. Session G4 exécutée ensuite

La session annoncée ici a ensuite été exécutée et fermée. Ses sorties brutes et
son verdict corrigé sont dans
[`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md).
Elles n'admettent aucune lane de tuples; q2 est seulement la moins massive.

GCP non utilisé pour la rédaction de cette note. La session ultérieure est
documentée comme `TERMINATED` dans sa note propre.
