# Reçu — le noyau de forêt HGP, jugé par miniboule indépendante

Date : 17 août 2026. Cadre : `phase=exploration_v4_hors_registre`,
`public_status=not_claimed`. Dossier : `docs/MATHEMATIQUES.md` § 5
(réécrit : § 5.1 clés, § 5.2 bras actifs et invariant des rayons de
naissance, § 5.3 macro-lots, § 5.4 le juge).

## L'objet

`src/forest/forest.hpp` : pour chaque `K`, union-find sur les `FacetKey`
(K-uplets triés de `PointId`), événements `σ = S ∪ I` (support d'arité q,
`d = K+1−q` intérieurs, niveau exact promu en `Q4Level`), traités par
**macro-lots** de niveaux sémantiquement égaux (`same_exact_level` U320,
jamais l'égalité de représentation) : racines gelées avant le lot, toutes
les unions ensemble, UN nœud de dendrogramme par racine finale ayant
absorbé plusieurs composantes pré-lot — aucune chronologie binaire.

**Invariant des rayons de naissance** (théorème, § 5.2) : une facette non
active `σ∖{z}` naît AU niveau `ρ(σ)` ; la voir née dans un lot antérieur
réfuterait la cohérence des niveaux du flux. `attach_violations` le MESURE
(porte : 0) au lieu de supposer la piste v3 « seules les actives
fusionnent ».

## Le juge (indépendant, régime oracle T2 `n <= 14`)

Énumération de TOUS les sous-ensembles σ ; miniboule par recherche de
support PROPRE (paires/triplets/quadruplets, centre strictement intérieur
à l'enveloppe relative — barycentriques et orientations en arithmétique
du juge —, plus petite boule contenante, comparaisons de distances
rationnelles en OBig 384 bits) ; Gabriel = boule ouverte vide ; un point
SUR la sphère hors support (de σ ou externe) écarte σ — le même refus
transactionnel que la production, vérifié cohérent des deux côtés (le
triangle rectangle et son hypoténuse se refusent mutuellement, par
exemple). Puis le K-graphe du manuscrit (Déf. 29, cliques COMPLÈTES) et
un Kruskal propre à lots. Ce juge valide au passage la **bijection
événement-boule** et la **complétude jointe des trois lanes** : un
simplexe de Gabriel manquant dans le flux sujet casserait les partitions.

## Comparaison (par K, par nuage)

Sommets du K-graphe, nombre de lots, **partition canonique après CHAQUE
lot**, multiensemble des nœuds `(lot, absorbées)`. Résultat : 487
événements, 1 602 fusions, **365 nœuds à >= 3 composantes absorbées**
(les plateaux multi-fusions sont MASSIFS sur grille entière — la
chronologie binaire artificielle aurait fabriqué des centaines de nœuds
fantômes), 0 désaccord, en 0,56 s.

## Fixtures gravées et mutants

- **colineaire3** (`{0,0,0},{2,0,0},{4,0,0}`, K=1) : deux arêtes de
  niveau 1 dans UN lot → un nœud TERNAIRE (3 absorbées), jamais une
  chaîne binaire. Le mutant `binary-ties` (lot forcé à un événement) le
  casse → code 4.
- **tie q4/q2** : le tétraèdre de l'audit (`R² = 14900`, représentant q4
  U192 NON réduit `14900·det²/det²`) et une arête `D² = 59600` à deux
  intérieurs (fraction canonique `14900/1`) dans la même forêt K=3 —
  même niveau sémantique, représentants différents. Le mutant
  `repr-ties` (égalité de représentation au lieu de `same_exact_level`)
  brise le lot à tort → code 4. C'est exactement le piège gravé par
  l'audit « lemme préfixe et niveau » § 2.

64 portes CTest vertes. Étape suivante déclarée : le raccord du flux
WSPD réel (les trois probes) au noyau de forêt — refactorisation des
pipelines en bibliothèque — puis le rendu § 9.1 (`F_K^render`, poids
`S_τ/T_x/m_τ`).
