# Reçu — ouverture de la lane q4 : complétion exacte jugée

Date : 17 août 2026. Cadre : `phase=exploration_v4_hors_registre`,
`public_status=not_claimed`. Base : commit `f6b29e1` (U192 + fixture de
découplage). Machine : conteneur CPU de session (aucune commande GCP).

Répond à l'ordre de l'audit `bc5b05d` (§ 3 : ABI de la source q4) et au
§ 3.1 (extraction d'`AcuteSeed` en amont des census). Dossier mathématique :
`docs/MATHEMATIQUES.md` § 4.5 (nouveau, `derive_v4`) — formes, largeurs,
preuve de Jung pour le coefficient 4, question Q12 (niveau q4 hors i128).

## Ce qui est construit

1. **`src/events/acute_seed.hpp`** : `AcuteSeed` et `is_acute_seed`
   (lentille + acuité stricte `V² > D²` + owner EdgeKey) — le prédicat
   PARTAGÉ : la lane q3 le consomme pour ses porteurs (probe refactoré), la
   lane q4 pour ses seeds. Les filtres de profondeur ne sont jamais
   partagés.
2. **`src/events/edge_cover.hpp`** : le cover d'arête (traversée haute par
   rectangle + filtre exact par ancre + requête par ancre appariée) extrait
   du probe q3 et PARAMÉTRÉ par le coefficient — 3 pour q3, 4 pour q4
   (preuve § 4.5 : la circum-boule q4 est la miniball du tétraèdre, Jung 3D
   donne `R <= sqrt(3/8)·D` puis `|2z-(a+b)|² <= 4D²` pour tout intérieur).
3. **`src/events/q4_instruction.hpp`** : circumcentre par Cramer 3×3
   relatif avec canonisation d'orientation (`det > 0`, négation simultanée
   `(det, N')`) ; puissance AFFINE `P4(z) = det·|z-a|² - 2N'·(z-a)` — le
   carré est évité, tout tient en i128 (`det < 2^57`, `|N'_i| < 2^72`,
   `P4 < 2^94`) ; arité 4 STRICTE par quatre tests d'orientation homogènes
   (`< 2^112`) ; owner 6 arêtes ; `SupportKey4`.
4. **`src/events/q4_event.hpp`** : la boule q4 porte le MÊME gabarit de
   BallKey à cinq coefficients que q3 (`A = det`, `B = -2(det·a+N')`,
   `C = det|a|²+2N'·a`, pgcd/signe partagés) ; le niveau q4
   `R² = |N'|²/det²` a un numérateur `< 2^146` HORS i128 — représentant
   Q12(b) : trois mots U192 + `det²` i128, non réduits, égalité par champ.
5. **`bench/q4_events_probe.cpp`** : la chaîne complète — vague WSPD lane
   q4, histogrammes `h_a,4/h_b,4`, paquets `base_4` (collecteur généralisé
   par lane, mêmes invariants exit 3 que q3), cover coefficient 4, seeds,
   complétions dans la lentille, exact-once par carrier canonique du
   tétraèdre FORMÉ, census avec paquet en préfixe, records complets.

## Mesures (juge brut C(n,4) sur records complets)

| configuration | événements q4 | juge | invariants |
|---|---|---|---|
| fixture14 (13 points de l'audit + z) | 8 | 0 manquant / 0 en trop | tous verts |
| uniform n=120 (8,2 M sous-ensembles jugés) | 7 909 | 0 / 0 | tous verts |
| eight_clusters n=120 | 3 235 | 0 / 0 | tous verts |

La fixture bout en bout grave : l'événement `{0,1,2,3}` existe avec owner
`EdgeKey(0,1)`, profondeur 1 et intérieur `{13}` — le point
`z = (200,109,300)` est STRICTEMENT intérieur (`|z-c|² = 14641 < 14900`)
mais HORS du cover coefficient 3 (`|2z-(a+b)|² = 145924 > 3D² = 120000`) :
il n'est visible que du cover coefficient 4, et `H < 0` le garde hors de
tous les fuseaux (les survies q3-morte/q4-vivante de l'audit sont
inchangées).

## Mutants (tous code 4)

| mutant | branchement injecté | tué par |
|---|---|---|
| `seeds-from-q3-live` | une ancre ne sème que si elle est q3-VIVANTE (compte exact `n3 < h_3` — le branchement interdit par l'audit, tel quel) | la fixture : `n3 = 9`, le tétraèdre gravé disparaît |
| `cover-coef3` | census borné à `3D²` | la fixture : z perdu, profondeur 0 ≠ 1 |
| `no-canonical` | exact-once sauté | doublons de supports sur uniform n=120 |

## Compteurs remarquables

Sur uniform n=120 : 86 057 seeds → 3 349 339 complétions essayées →
2 245 742 refus « centre hors du tétraèdre » → 7 909 événements. Le refus
d'arité stricte est LE grand filtre (67 %) ; la sélection axiale § 4
(seize groupes par seed, `theoreme_v3`) reste la voie d'échelle et sera
reçue CONTRE cette baseline, comme le veut la discipline des accélérateurs
jugés.

## Addendum — l'oracle indépendant q4 (même séance)

`tests/q4_oracle_test.cpp`, mêmes standards que l'oracle q3 durci :
obigint 384 bits, primitives locales, `InputPoint`, Cramer 3×3 PLEIN
(quatre points : pas de contrainte de plan), centre-intérieur par quatre
orientations OBig confronté à `q4_center_strictly_inside`, profondeur/
coquille/intérieurs, BallKey projective, niveau par produits croisés.

| mesure (7 nuages, dont tétraèdre régulier entier à M=65535 et grande cosphère) | valeur |
|---|---|
| tétraèdres jugés | 59 825 |
| supports q4 (centre strictement intérieur) | 4 880 |
| `supports_with_extra_shell` | 2 076 |
| désaccords | 0 |
| limbes max : det / num / niveau | 0 / 1 / 3 (plancher ≥ 3) |
| mutants `cramer-swap`, `mul-carry-lost`, `sign-p4` | tués (code 4) |

Leçon de porte notable : l'identité de niveau q4 met les MÊMES valeurs
entières des deux côtés (`|N'|² = Rnum`, `det_o² = den`) — une corruption
structurelle de la multiplication frappait les deux membres à l'identique
et le mutant carry passait. Le membre gauche est désormais calculé par
l'associativité `(num·det)·det` : les paires d'opérandes ne coïncident
plus, le mutant meurt. Gravé en commentaire : deux côtés d'un juge ne
doivent jamais partager leurs paires d'opérandes.

47 portes CTest vertes. Reste : U320 pour l'ordre mixte q3/q4, sélection
axiale (contre cette baseline), forêt (macro-lots compris).
