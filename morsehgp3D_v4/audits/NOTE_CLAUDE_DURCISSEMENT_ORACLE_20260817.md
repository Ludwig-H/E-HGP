# Note de Claude — durcissement de l'oracle rationnel q3 exécuté

Date : 17 août 2026. Répond à
`AUDIT_ORACLE_RATIONNEL_Q3_EBC8236_20260817.md` (les cinq P0 du § 8, les
durcissements du § 4, le renommage du § 6, les mutants du § 7). Reçu complet
avec mesures : `receipts/oracle_durcissement_20260817/README.md`.

## Fait

1. **P0.1 — plus aucun `abort()`** : drapeau collant `overflow_flag()`
   fail-closed, résultat empoisonné documenté, `overflow_seen()` vérifié par
   le test avant tout verdict (drapeau ⟹ code 3 `numeric_failure`).
   `static_assert(N >= 2)`, borrow final de `sub_mag` vérifié.
2. **P0.2 — le juge du juge** : `mhgp4_obig_selftest` contre
   `boost::multiprecision::cpp_int` (tests uniquement, enregistré si Boost
   présent — précédent GMP v3). 95 valeurs × 95 = 9 025 paires add/sub/mul/
   cmp, 1 242 triplets distributifs (plancher 1 000), zéros signés,
   `INT128_MIN/MAX`, retenues et borrows multi-limbes, frontière EXACTE du
   septième limbe (`2^383` tient sans drapeau, `2^384` lève le statut,
   processus vivant). Zéro désaccord.
3. **P0.3 — fixtures u16 extrêmes + compteurs de limbes** : équilatéral
   entier à `M = 65535` (cinq cosphériques, owner à égalités), presque
   rectangle aigu (marge de Thalès 40001 sur 1,6·10^9), grande cosphère
   rayon 20000 centrée (32768,32768,32768), rejeu translaté au bord exact de
   la grille. Limbes max observés : `det` 1, `num` 1, `dist2` 2, produit du
   niveau 3 — plancher gravé `niveau >= limbe 3`.
4. **P0.4 — renommage** : `supports_with_extra_shell` (905), plus
   `ballkeys_degenerees_uniques` (335) puisque la `Q3BallKey` existe.
5. **P0.5 — mutants** : `cramer-swap`, `level-4g`, `mul-carry-lost` tués
   (codes 4), en plus de `sign-p` et `prune-ge`. Le mutant carry ne jette la
   retenue qu'aux positions `i+j >= 2` : sa mort prouve la traversée réelle
   des limbes hauts (4 635 désaccords au selftest).
6. **§ 4 — indépendance** : primitives `dot/cross/norm2/sub` réécrites
   localement dans le test ; l'oracle reçoit des `InputPoint{id, position}`
   formés, sans dépendre du renommage de `CloudIndex`. Bonus P1 : le niveau
   public du sujet (`q3_exact_level`) est désormais jugé par
   `num·det² == den·|a·det - N|²`.

Total : 39 852 triangles sur 8 nuages, 5 792 événements, zéro désaccord,
32 portes CTest vertes.

## Trois arbitrages pris, à contester si besoin

- **`OBig<5>` (§ 3.2)** : largeur 6 CONSERVÉE. La borne prouvée `2^323`
  exige le limbe 5 (`2^323 > 2^320`) : un `OBig<5>` n'est pas prouvable sûr,
  la réduction de largeur revendiquée est donc refusée. L'écart prouvé/mordu
  (limbe 5 / limbe 3) est rendu explicite par les compteurs publiés, couvert
  par l'échec fermé, et les limbes 4–5 sont exercés par le selftest. Pas de
  fixture de sharpness artificielle.
- **Mutant § 7.2 (« signe du déterminant, comparaison non quadratique »)** :
  non implémenté — l'oracle n'a aucune comparaison non quadratique où
  l'injecter honnêtement (votre § 1.2 le démontre : le carré élimine le signe
  de `det` par construction). Il sera ajouté le jour où un chemin non
  quadratique apparaît.
- **Journalisation débrayable** (`overflow_log()`) : le selftest provoque des
  milliers de débordements attendus ; seul le MESSAGE se tait, jamais le
  drapeau. Si vous préférez un message inconditionnel, c'est un one-liner.

## Question aux auditeurs (ordre de la suite)

Vos recommandations ouvertes, dans l'ordre où je les lis : (a) l'oracle
d'ÉVÉNEMENT complet (P1 § 8 : `SupportKey`, `BallKey`, `InteriorIds/
ShellIds`, `ExactCenter`, confrontés aux records de `q3_events_probe`) ;
(b) la porte torique à cible bêta incomplète (perte/duplication statistique
au bord) ; (c) l'arbre de centres version A (accélérateur JUGÉ contre la
baseline site-major, après preuve des largeurs i192 des enclos `T_x`) ;
(d) le raccord de la preuve de taille WSPD (cellules à préfixe binaire
exact) ; puis q4 axial. Je pars sur (a) puis (b) sauf avis contraire —
(a) prolonge directement ce durcissement et verrouille le contrat
transactionnel avant toute optimisation nouvelle.
