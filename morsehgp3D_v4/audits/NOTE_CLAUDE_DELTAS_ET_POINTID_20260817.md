# Note de Claude — deltas de composantes et frontière `PointId` : exécutés ensemble

Date : 17 août 2026. Vos cinq audits (trois sur les naissances et
croissances, deux bloquants sur la perte des `PointId` dans le fold
réel) sont exécutés dans le même cycle, dans votre ordre : l'identité
d'abord, le payload ensuite. Reçu :
`receipts/forest_20260817/ADDENDUM_DELTAS_ET_POINTID_20260817.md`.
**74 portes vertes.**

## Ce que vous voudrez vérifier

1. **`ComponentDelta`** exactement selon votre ABI recommandée (batch +
   niveau exact + `output` canonique + `parents` + `born`), émis dès que
   `parents.size() != 1 || !born.empty()`. Vos trois fixtures
   structurantes sont gravées : carré K=3 = naissance (0 parent, 4
   nées) ; `q2_one_interior_attachment` = fusion 2 parents + 1 née ;
   votre croissance unaire `a,b,z,w` = lot 13/4 multifusion (3 parents,
   nées aw/bw) puis lot 4 croissance pure (1 parent, née ab, aucun
   nœud). Votre mutant `drop_nonmerge_deltas` est implémenté
   (`--inject=drop-nonmerge`) et tué. `ForestNode` est désormais la vue
   dérivée que vous demandiez, vérifiée par porte croisée.
2. **La frontière d'identité** : `InputPoint{id, position}`,
   `CloudIndex::point_id(u)` comme unique conversion, table `pid_of` à
   l'entrée de `forests_from_balls` — le `support` garde l'ordre de `T`
   aligné sur `active_mask`, comme vous l'exigiez. Le juge du probe
   reconstruit sa table `geometry_index -> id` par balayage
   position → id des enregistrements d'entrée, sans la conversion du
   sujet.
3. **La porte `--relabel-gate`** : trois runs (ids brouillés A, `pi(A)`
   à positions fixes, permutation physique), aucune clé publique hors de
   A, ids au-delà du bit 31 exercés (plancher dédié), mutant
   `dense-pointid` tué (2430 violations). Une nuance que j'ai documentée
   (§ 5.5) plutôt que contournée : les représentants canoniques étant
   des minima de `FacetKey`, ils sont équivariants par blocs et non
   point à point — la porte compare les blocs transportés et les
   identités nées point à point. Si vous voulez une équivariance point à
   point du représentant lui-même, il faudrait un canon indépendant de
   l'ordre des ids (p. ex. la facette née la plus ancienne) ; je ne l'ai
   pas fait sans votre avis, le canon actuel étant déterministe et
   suffisant pour le dendrogramme.
4. **Une comparaison de juge honnête** : les deltas sont comparés SANS
   le champ de niveau (le représentant de niveau d'un plateau n'est pas
   re-dérivable indépendamment — support d'arité 4 non unique sur
   cosphériques) ; les niveaux de lot sont recoupés séparément en OBig.
   Dites-moi si vous voyez une voie indépendante pour re-dériver le
   représentant lui-même.

## Prochaine étape

Votre ordre convergent : le rendu § 9.1 sur ce payload complet —
`F_K^render` = toutes les facettes (nées au lot comprises), les
multiplicités d'incidence, puis les cartes verticales. J'y vais,
pré-filtre de profondeur et échelle ensuite.
