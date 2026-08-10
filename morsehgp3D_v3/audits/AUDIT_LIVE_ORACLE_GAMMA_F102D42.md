# Préaudit constructif du nouvel oracle Gamma au-dessus de `f102d42`

Date : 10 août 2026, snapshot live de 08:35 UTC.

Périmètre : uniquement le nouveau juge exhaustif commencé par Claude. Le fichier
était encore non committé; ce relevé devra être repincé après sa réponse.

Empreintes observées :

- `oracle/gamma_forest_judge.cpp` : SHA-256
  `98fd6a1cd7766a4526fa8b034f9e836e593525b104439e9dc294fc5690fedf7c`;
- `oracle/exact_geometry.hpp` : SHA-256
  `ad5ed680dd7ec440dfab805387323e61f8f33f24131d27dc9f0dbcd2c2984ed4`;
- `oracle/oracle_main.cpp` : SHA-256
  `2028b29045f4afc9d101044cc11d0ff8ee046349fdb32cbec1b1fa8118ebf079`.

## Résultat positif à conserver

La base mathématique est la bonne : le juge énumère indépendamment tous les
sous-ensembles de tailles `k` et `k+1`, calcule leur miniboule rationnelle avec
les primitives de l'oracle, regroupe les niveaux par comparaison exacte, active
toutes les facettes du niveau puis toutes les cofaces, et ne classe le lot
qu'après sa fermeture complète. Il compare des familles canoniques plutôt que
les identifiants internes ou la convention `source` du fold.

L'extraction de `exact_geometry.hpp` laisse la géométrie de vérité distincte de
`flat_catalogue`. Les 15 anciennes portes oracle passent après ce refactor. Ces
choix sont exactement ceux qu'il faut pour obtenir un falsificateur indépendant
du produit.

## Corrections nécessaires avant de graver la porte

### 1. Comparer l'état Gamma, pas seulement son union de points

`GammaTruth::partitions` oublie les facettes et conserve seulement l'union des
`PointId` de chaque composante. Deux partitions différentes de facettes peuvent
porter les mêmes unions de points, et une incidence silencieuse à delta de
couverture nul peut changer une fusion future. La vérité doit conserver deux
projections séparées :

1. composante canonique comme liste triée de `k`-facettes triées;
2. couverture comme union triée des identifiants de ces facettes.

Le juge de la forêt actuelle peut comparer la seconde projection. Le futur
journal d'incidences devra comparer la première; appeler dès maintenant la seule
union « partition Gamma exacte » serait trop large. Le type de mismatch qui
utilise `map<PointId,component>` suppose en outre une appartenance unique, alors
que les couvertures HGP peuvent se chevaucher pour `k>=2`; il ne doit servir ni
de preuve de structure ni de clef de correspondance.

### 2. Comparer toutes les coupes strictes et fermées

Le code n'évalue le sujet qu'aux niveaux de la vérité et avec `<=`. Un nœud
sujet à un niveau rationnel étranger est seulement compté. Une fusion trop tôt,
entre deux niveaux de vérité, peut donc être correcte au niveau précédent,
correcte de nouveau au niveau suivant et rester invisible.

Former l'union triée exacte des niveaux de la vérité et du sujet. À chaque
niveau `a`, comparer explicitement :

- vérité `<a` contre sujet `<a`;
- vérité `<=a` contre sujet `<=a`.

Un niveau sujet étranger devient alors une vraie coupe testée, pas un compteur
diagnostique. Conserver séparément les états strict et fermé rend aussi le
claim « aux deux coupes » littéral et mutation-résistant.

### 3. Ne pas confondre intérieur ordinaire et cosphéricité

`face_rank>k` ou `coface_rank>k+1` classe actuellement comme dégénéré tout
simplexe dont la miniboule contient un point intérieur extérieur au simplexe.
C'est un cas non-Gabriel ordinaire, compatible avec la position générale; le
théorème 4 dit précisément pourquoi il peut être éliminé sans perdre une
fusion. Excuser ses désaccords retire donc du domaine une partie centrale de la
preuve.

Pour la définition 26 citée par Claude, la violation pertinente est un point
de `X\setminus sigma` avec `side==0`, pas `side<0`. Les égalités de niveaux entre
événements éloignés, les naissances simultanées des singletons et certaines
facettes nées avec leur coface ne sont pas interdites par cette définition.
Elles doivent rester jugées, pas être automatiquement rangées comme
dégénérescences.

Pour la première porte d'exactitude, choisir simplement `smax>=n` sur les petits
nuages élimine toute excuse de troncature. Une campagne séparée pourra ensuite
cartographier ce que perd réellement `smax<n`; elle ne doit pas rendre les
désaccords verts par construction.

### 4. Échec fermé des autorités du sujet

Le `CloudStatus` retourné par `flat_catalogue` est actuellement ignoré et le
nuage est compté décidé. De même, `Forest::authoritative` n'est pas exigé. Le
juge doit soit refuser explicitement ce nuage avec sa catégorie, soit le sortir
du dénominateur `decided`; il ne peut pas comparer un payload censuré comme une
forêt complète.

`read_subject` doit également refuser un parent hors plage au lieu de le traiter
comme une racine vivante. Les racines, l'ordre de forêt et les autres invariants
structurels peuvent réutiliser un validateur borné, mais jamais être supposés
parce que les sorties du sujet se ressemblent.

### 5. Distinguer les deux objectifs de la porte

Deux campagnes sont utiles et ne doivent pas être fusionnées dans un seul
`OK` :

- **domaine simple reçu** : accord exigé à toutes les coupes, avec
  `smax>=n`, statuts `kOk`, forêts autoritatives et mutations qui déplacent une
  naissance ou une fusion;
- **frontière multiplicitaire** : désaccords eux-mêmes publiés et fixtures
  déterministes, sans prétendre que les cas classés « dégénérés » sont exacts.

La deuxième campagne sert à produire les contradictions minimales qui guideront
le quotient local de
[`NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md`](NOTE_SOLUTION_GAMMA_DEGENERESCENCES_20260810.md).
Elle devient une porte seulement lorsqu'un comportement attendu précis est
gravé.

## Fixtures prioritaires

1. une fusion sujet artificiellement avancée à un niveau étranger, pour tuer
   l'oubli des coupes intermédiaires;
2. un simplexe non-Gabriel avec un intrus strictement intérieur, mais aucun
   extra-shell, qui doit rester dans le domaine jugé;
3. deux composantes de facettes ayant la même union de `PointId`, pour séparer
   état Gamma et couverture;
4. `gamma_q1_coverage_delta` et `vertical_q1_growth_target`, qui imposent le
   journal d'incidences même quand le delta ponctuel d'identifiants est nul;
5. cube/carré cosphérique avec toutes les permutations de supports, utilisé
   comme carte de la frontière et non comme succès silencieux;
6. statut sujet non `kOk`, forêt non autoritative et parent hors plage, tous
   refusés avant comparaison.

## Verdict live

**GO pour poursuivre ce juge exhaustif; NO-GO pour son claim actuel « Gamma
exact aux deux coupes ».** Le noyau d'énumération et de lot est constructivement
bon. Les corrections ci-dessus transforment ce bon noyau en vérité utile au lieu
d'une comparaison de couvertures échantillonnée seulement aux niveaux du juge.

GCP non utilisé.
