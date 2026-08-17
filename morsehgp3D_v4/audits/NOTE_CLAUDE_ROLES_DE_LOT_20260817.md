# Note de Claude — les enfants nés dans le lot sont corrigés (les deux audits de forêt sont exécutés)

Date : 17 août 2026. Répond à
`AUDIT_CIBLE_1310B21_FACETTES_NEES_DANS_LE_LOT` — exécuté en même temps
que l'audit bloquant des coquilles (voir `NOTE_CLAUDE_PLATEAUX`), les
deux corrections vivant dans le même noyau.

## La correction (votre § 3, à la lettre)

`build_forest` classifie désormais les rôles PAR FACETTE sur tout le lot
AVANT toute création d'ID : `existed_before_batch` mémorisé d'abord,
`active` / `attachment` agrégés depuis les événements, racines pré-lot
= `active ∨ existed`, unions sur TOUTES les facettes comme avant,
`absorbed` compté sur les seules racines pré-lot. Les deux invariants
gratuits sont gravés en portes (`attach_violations`,
`birth_violations` = 0) et `new_attachments` est compté et COMPARÉ au
juge.

Votre § 5 est aussi implémenté : sur un plateau `σ = I_B ∪ T`, le rôle
d'un retrait `v ∈ T` est calculé par `c ∈ conv(T∖{v})` (la boule
conservée ⟹ attachement) — le producteur grave un `active_mask` par
événement, et la formule coïncide avec la règle régulière quand `T` est
le support minimal. Conséquence mesurée sur la fixture carrée : au K=2
le nœud n'a plus que QUATRE enfants (les côtés actifs — les deux
diagonales naissent au lot), et au K=3 il n'y a AUCUN nœud (la
composante naît entière) ; les valeurs gravées ont été corrigées en
conséquence. `plateaux_multi` passe de 369 à 144 : un tiers des arités
étaient gonflées par des enfants fantômes — votre diagnostic était
quantitativement lourd, pas cosmétique.

## Le juge est corrigé par une voie distincte (votre § 4)

Il ne réutilise ni le code ni la convention du sujet : le rayon de
naissance de CHAQUE facette est recalculé par SA propre miniboule
(recherche de support du juge, OBig) et comparé au niveau du lot —
`active ⟺ ρ(facette) < ρ(σ)`, la définition même. Racines pré-lot,
`new_attachments` et violations comptés de son côté, comparés champ à
champ.

## Fixture et mutant (votre § 2)

`q2_one_interior_attachment` (`{(0,0,0), (4,0,0), (2,1,0)}`) est gravée
en ATTENTE MATHÉMATIQUE explicite — nœud unique, `absorbed = 2`,
`new_attachments = 1` — pas en simple accord sujet-juge. Le mutant
`attach-prebatch` (l'ancienne convention) meurt (code 4), et les trois
autres mutants de forêt restent tués. Un point d'exécution : sous
mutant `binary-ties`, c'est désormais l'invariant `attach_violations`
qui se déclenche EN PREMIER (scinder un lot rend « préexistant » un
attachement du même niveau) — le détecteur qui tire est compté comme
mise à mort du mutant, ce qui est sa fonction.

503 événements, 0 désaccord, 66 portes CTest vertes, tout est poussé.
Votre ordre § « utile » est donc épuisé jusqu'au point 4 : le prochain
chantier est le raccord des flux WSPD réels au fold (avec le
`SpherePlateau` d'échelle), puis le rendu § 9.1 dont vous notez
justement que ces rôles déterminent `F_K^conn`.
