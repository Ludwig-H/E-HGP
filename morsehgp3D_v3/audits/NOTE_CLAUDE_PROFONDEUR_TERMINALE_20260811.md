# Note de livraison Claude — filtre terminal de profondeur fermée (tranche 2 du falsificateur)

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Réponse à la porte 4 de [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md)
(« prototyper indépendamment cœur de Jung, profondeur fermée … avec ledger
de sort et oracle exhaustif borné ; mesurer les deux filtres séparément et
combinés »). Réception aux auditeurs.

## Le sweep exact (`exact_closed_depth`)

`min fermé = always + m − max ouvert`, gravé tel que reçu : banque `always`
des projections nulles (`d × V = 0`), réduction 2D EXACTE sans
normalisation (base entière `e1 ⊥ d`, `e2 = d × e1` ;
`V·(αe1+βe2) = α(V·e1)+β(V·e2)` — les comptes de demi-plans de `V` sont
ceux des `w = (V·e1, V·e2)`), max ouvert atteint sur l'arc semi-ouvert
`[θᵢ, θᵢ+π)` porté par un vecteur : rayon confondu (cross 0, dot > 0)
DANS l'arc, antipode (cross 0, dot < 0) EXCLU. Bornes : `|w₂| < 2^56`
(i64), cross/dot en i128 (~2^93). O(m²), permutation-invariant par
construction. Les témoins sont collectés par l'arbre avec le sup ET
l'INFIMUM exacts par axe (`min de V²−d²` vaut EXACTEMENT `−d²` quand
l'intervalle contient zéro — une première version surestimait l'infimum et
pouvait perdre des témoins, corrigée avant tout enregistrement).

## Les trois modes isolés, mesurés (terrain 400, lane q3, un thread)

| mode | prunées | résiduelles | phase locale | note |
| --- | ---: | ---: | ---: | --- |
| `core` | 67 707 | 12 093 (15,15 %) | 0,142 s | blocs + coins |
| `depth` | 42 230 | 37 570 (47,08 %) | 5,391 s | aucun prune de bloc, sweep pour chaque paire (7,53 M témoins, m max 395) |
| `combined` | 67 718 | 12 082 (15,14 %) | 0,186 s | sweep sur les 12 093 survivantes du cœur |

**Le fait à retenir : après le cœur universel, la profondeur ne tue que 11
paires de plus** (12 093 → 12 082) sur cette famille et cette taille, pour
un sweep 37× plus cher en mode isolé. Sur ces campagnes bornées, le cœur
domine ; la profondeur reste le certificat exact de réserve que la réponse
d'audit décrivait — aucune décision d'architecture n'est prononcée sur une
seule taille.

## Portes

- Fixtures `depth-rays` (rayons confondus + antipodes + banque always :
  delta == 3 dérivé à la main, permutation vérifiée) et `scope-depth` (le
  nuage de portée de l'audit : les dix témoins projetés sur UN rayon,
  delta == 0, ab RESIDUELLE en q3 dans les deux formalismes).
- Mutants `depth-open-boundary` (l'antipode compte dans l'arc) et
  `depth-always-dropped` (banque perdue), tués à code 4 par delta ≠ 3.
- Oracle exhaustif `n<=32` vert en modes `depth` et `combined` ; les prunes
  de profondeur portent un certificat `(paire, delta)` que le juge REJOUE
  par balayage complet au prédicat nominal, égalité exacte exigée.
- Sous-ensemble ciblé : 75/75.

## Restes de la porte 4

Le troisième mécanisme (center-cover 64 patches) n'est pas encore comparé
sur les mêmes paires ; la fixture tueuse d'`oracle-accept-nonpositive`
reste due (géométrie dérivée : tuple q4 à support triangulaire, quatrième
point à ~90 degrés, boule petite devant la corde) ; le sweep O(m²) devra
passer au deux-pointeurs trié si la profondeur entre un jour dans un chemin
mesuré à l'échelle.

GCP non utilisé pour cette livraison.
