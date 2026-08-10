# Réception constructive du juge Gamma à `478cfe8`

Date : 10 août 2026 UTC.

Périmètre : audit du juge borné `mhgp3v_gamma_judge` et réponse mathématique à
la censure multiplicitaire. Aucun prototype n'a été modifié par l'auditeur.
Cadre inchangé : `phase=exploration_v3_hors_registre`, CPU de référence,
profil u16, `public_status=not_claimed`.

## Snapshot et reçus

- commit et `origin/main` : `478cfe8e42303002d63594e69ce1ab2409be7d28`;
- `oracle/gamma_forest_judge.cpp` SHA-256 :
  `295633b76f14b5a515aeb0d60006bdadee6eb2a8a6186fac124c82334b2937ae`;
- `CMakeLists.txt` SHA-256 :
  `46240129e7747ef514009192ab2120d12b0612f57e818f56605967f94cb45502`;
- binaire Release de la campagne Claude SHA-256 :
  `b553db512f39ef87323b2a18a4da1a2719d564add89d20695a098cfca3da4932`.

Un build frais indépendant du même couple source/CMake puis les six portes
Gamma passent 6/6 en 2,02 s. La suite Claude affiche 94/94, zéro échec, en
421,67 s. Sa
commande était `ctest -j2 2>&1 | tail -3`, sans `pipefail`; le code shell n'est
donc pas un reçu autonome de `ctest`. Le journal
`build/v3/Testing/Temporary/LastTest.log`, SHA-256
`9bb214a197df23a3d90adc5dacd6bc344a5376132678cf89bc0bd449ac33df55`,
contient toutefois les 94 entrées, aucune occurrence `Test Failed` ni
`Not Run`, et le résumé concordant.

Une relance auditeur sans pipeline affiche ensuite 94/94 en 462,92 s, mais elle
n'est **pas** créditée au commit : pendant son exécution, le binaire Gamma de
`build/v3` a été relinké à 09:16:42 avec le fold saturé live. La suite est donc
un run mixte, même si ses 94 tests sont verts. Le reçu stable de `478cfe8`
reste la campagne Claude corroborée par son ancien journal et le build Gamma
indépendant 6/6 ci-dessus.

## Résultat positif

Le noyau est désormais un falsificateur utile et indépendant du fold produit :

1. il énumère toutes les facettes de cardinal `k` et cofaces de cardinal
   `k+1`, avec miniboules et niveaux rationnels exacts;
2. il fige la coupe stricte, applique le lot entier puis classe seulement après
   fermeture du lot;
3. il lit les forêts sujet comme des données et refuse statut non `kOk`, forêt
   non autoritative, source, tranche de pool, parent ou cycle invalides;
4. il compare les familles canoniques de couvertures aux opérateurs `<` et
   `<=`, sur l'union des niveaux vérité et sujet;
5. il borne explicitement le claim : ni identifiants de nœuds, ni convention de
   multifusion, ni partitions de facettes, ni journal d'incidences ne sont
   déclarés égaux.

Un mutant hors dépôt remplaçant entièrement `classify` par un verdict toujours
égal est tué par la porte `reject_shifted_level` : le programme mutant retourne
0 alors que la porte exige le code 1. Le comparateur participe donc réellement
au verdict. Un parent forcé hors plage est refusé sur les 90 ordres de la
campagne générique; ses planchers rendent alors le programme rouge au code 3.

La campagne générique reçoit 78 accords de couverture hors cosphéricité aux
coupes testées. C'est un résultat positif borné, pas une équivalence de payload
Gamma complet.

## Frontière exacte du résultat

### 1. La campagne saturée ne juge pas le verrou multiplicitaire

La commande saturée décide vingt nuages et parcourt trois ordres par nuage. Ses
quarante lignes de refus sont exactement vingt refus `k=2` et vingt refus
`k=3`, tous pour `foret non autoritative`. Les vingt ordres restants sont donc
les seuls `k=1`; ils concordent. Le plancher global `--min-judged 10` est rempli
par eux seuls.

Cette porte reçoit correctement la censure, mais ne mesure aucune sortie sujet
pour les ordres multiplicataires visés par Q1. Elle ne doit être appelée ni
« carte des erreurs multiplicataires » ni fermeture A0 complète. Un prochain
reçu doit publier les compteurs jugés/refusés **par ordre** et exiger le domaine
précis attendu pour chacun.

### 2. La projection de facettes n'est pas encore confrontée

La vérité conserve `facet_partitions`, mais le sujet ne publie que des unions de
`PointId` par sous-arbre. Deux partitions de facettes distinctes peuvent avoir
les mêmes couvertures. Le juge courant certifie donc une projection utile, pas
l'état Gamma complet ni ses incidences silencieuses.

### 3. Le mutant de niveau ne sépare pas les deux nouveaux contrats

La porte shifted tue bien le comparateur mort. Elle ne prouve toutefois pas
spécifiquement l'union des niveaux **et** les coupes strictes. Deux mutants
orthogonaux passent encore chacun 6/6 : supprimer uniquement la boucle de coupe
stricte; supprimer uniquement l'ajout des niveaux sujet à `cut_levels`. Le
mutant shifted actuel est déjà visible par d'autres coupes. Le commentaire
« seule l'union aux deux coupes peut le voir » est donc trop fort.

Deux auto-tests algébriques, indépendants de la géométrie aléatoire, rendent les
deux obligations littérales :

- **coupe stricte** : une injection `--force-strict-as-closed` sur le cas
  déterministe `n=4`, `k=1`, graine 7 rend le code 1 avec un désaccord étiqueté
  `coupe STRICTE`. Si la boucle stricte est supprimée, l'injection n'est plus
  exercée et le programme rend 0;
- **niveau sujet étranger** : ajouter après lecture une racine fantôme de
  couverture vide au niveau `b=2*a`, où `a` est le dernier niveau vérité. Le cas
  déterministe rend le code 1 à la coupe fermée de `b`; si l'ajout des niveaux
  sujet est supprimé, il rend 0. Une option
  `--force-foreign-root-after-truth` en fait une porte comparator-only.

La sonde révèle aussi un compteur trompeur : `foreign_levels` est incrémenté à
la découverte avant l'insertion dans `cut_levels`. Avec le mutant qui omet
l'insertion, la sortie annonce encore `niveaux etrangers testes=1` alors que le
niveau n'a jamais été comparé. Séparer `foreign_levels_seen` et
`foreign_cut_levels_compared`, puis plancher le second dans la fixture dédiée.

### 4. Le refus structurel dépend encore des planchers de campagne

`read_subject` refuse bien un parent hors plage avant comparaison. Une commande
directe sans plancher peut néanmoins finir au code 0 avec zéro ordre jugé et le
libellé `CARTE DE FRONTIERE`. Ce comportement convient à un outil diagnostique,
mais une porte fail-closed permanente doit injecter séparément parent, cycle,
source, tranche et autorité, puis exiger leur code contractuel sans dépendre de
l'épuisement fortuit d'un plancher global.

## Solution du verrou mathématique, pas seulement son constat

La proposition « lire tous les sous-ensembles de la seule coquille » est fausse.
Le générateur exact est le saturé complet `M=I union U`. À une coupe, Gamma est
l'union des graphes de Johnson portés par les saturés actifs. Deux générateurs
`M,N` appartiennent au même graphe de générateurs dès que la cardinalité de leur
intersection atteint `k`. Ce théorème et ses contre-exemples minimaux sont dans
[`REPONSE_COMPLEMENT_CLAUDE_GAMMA_20260810.md`](REPONSE_COMPLEMENT_CLAUDE_GAMMA_20260810.md).

La prochaine expérience utile est donc une troisième vérité bornée, pas une
modification spéculative de `build_forest` :

1. énumérer les supports bien centrés de tailles un à quatre;
2. classifier exactement leur saturé complet et dédupliquer les générateurs;
3. construire leurs intersections pondérées;
4. dériver simultanément les composantes pour chaque seuil `k`;
5. comparer `Gamma exhaustif == tour saturée == sujet` aux coupes stricte et
   fermée.

Cette troisième vérité doit énumérer elle-même les supports avec les primitives
de l'oracle. Un `saturated_fold` qui consomme `flat_catalogue` est un candidat
produit utile, mais pas une autorité indépendante : `smax>=n` retire la censure
de rang sans certifier que la navigation source a émis tous les générateurs.
La porte doit donc comparer séparément `tour exhaustive == catalogue` puis
`tour exhaustive == fold`.

Pour ne pas développer les graphes de Johnson, un index inversé
`PointId -> générateurs` suffit au premier prototype : à l'arrivée d'un saturé
`M`, scanner les listes de ses membres et accumuler exactement `|M intersection
N|` pour chaque générateur candidat `N`. Une paire de poids `w` alimente les
DSU de tous les ordres `k<=min(K,w)`. Tous les générateurs d'un même niveau sont
mis en staging, leurs unions sont calculées, puis le lot est committé
atomiquement. Cette route ne matérialise ni mosaïque de Delaunay d'ordre
supérieur, ni sous-simplexes de `M`.

Elle sépare honnêtement le prochain verrou : l'oracle borné peut énumérer
`O(n^4)` supports, mais le produit doit encore certifier qu'une source
output-sensitive n'a omis aucun saturé utile, y compris lorsque `|M|>smax`.
Un arrêt budgétaire est une censure déclarée, jamais une preuve de complétude.

GCP non utilisé.
