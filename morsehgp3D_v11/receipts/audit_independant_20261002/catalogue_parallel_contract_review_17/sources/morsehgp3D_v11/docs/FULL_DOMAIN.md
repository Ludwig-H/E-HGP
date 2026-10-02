# Domaine possédé pour le futur FULL

Tranche du 2 octobre 2026 : `FullDomain` réunit le `GlobalIndex`, le catalogue
construit sur `index.cloud()` et la recherche exacte de ses supports canoniques.
**Qualification native G4 à venir.** Ce contexte ne construit encore aucune
cellule, descente, forêt, verticale ou hiérarchie de points.

## Contrat et propriété

L'entrée publique est `prepare_full_domain(GlobalIndex&&, CatalogueParams, MemoryBudget&)`,
déclarée dans [full_domain.hpp](../src/tower/full_domain.hpp) et exposée par
[tower.hpp](../src/tower/tower.hpp). La factory construit le catalogue puis la table ;
elle déplace l'index uniquement au succès final. Tout refus conserve l'index source,
ses vues et les réservations préexistantes. Les réservations propres du brouillon
sont libérées. L'ordre des refus est celui du catalogue : paramètres, nuage vide,
multiplicités, ressources/calcul ; viennent ensuite l'admission et l'allocation du lookup.

Le résultat possède les trois stockages, sans référence à l'objet index appelant.
Il est déplaçable, ni copiable ni affectable ; l'objet déplacé est vide et ses vues
suivent le nouveau propriétaire. Les budgets doivent survivre aux résultats.
Les recherches ne modifient rien et n'allouent pas : le contexte est partageable en lecture.

## Identité et recherche

Le catalogue est trié par niveau exact puis support ; cet ordre ne permet pas une
dichotomie globale sur les supports. `find_support` consulte une table distincte et
compare exactement les quatre `SiteIdx`, padding `kNone` compris. Une clé arbitraire
ne sert jamais à indexer les coordonnées. Le hash choisit seulement une case.

Dans ce même Cloud, un support strict affinement indépendant détermine sa MEB unique.
L'égalité de supports canoniques identifie donc la boule, sans nouvelle clé géométrique
PGCD. Cette propriété ne s'étend pas aux mêmes indices dans deux nuages différents.
Un support **local** de MEB peut manquer alors que la boule figure au catalogue avec
un support global plus petit : le miss ne vaut jamais certificat d'absence géométrique.
Le census global et sa canonicalisation appartiennent à la prochaine tranche.

## Capacité et mémoire

Pour B boules, C=0 si B=0 ; sinon C est la plus petite puissance de deux supérieure
ou égale à 2B. La table contient `C` éléments `BallIdx`, soit exactement **4C octets**,
avec charge au plus 1/2 et au moins une case vide. `kNone` marque ces cases ; aucun
rang valide ne vaut cette sentinelle. Les sondages linéaires sont bornés par C.
Un doublon exact pendant l'insertion est un refus `catalogue_invariant`.

`B<kNone` entraîne `2B<2^33`, `C<=2^33`, `4C<=2^35`. Capacités, offsets et sondages
sont calculés en `u64` ; les bornes sont aussi vérifiées par `static_assert`.
La table est admise dans le budget avant allocation, sans tampon de tri ni copie des clés.
Pour F le pic propre de construction du catalogue et R ses réservations finales,
le pic propre de cette factory vaut `max(F,R+4C)` sur un succès. Avec U réservations
antérieures stables, le pic final est `max(ancien_pic,U+max(F,R+4C))`.
L'index/Cloud est compris dans U s'il utilise ce budget ; ses autres budgets restent distincts.
Il s'agit de réservations de buffers, pas d'une mesure RSS.

## Provenance et portes

Port explicite du principe `FlatIndex/hash_sites` de `src/tower/tower.cpp` R2,
commit `865f5e64ddd08bedf6ab8f94e8bb94812e380e79`, SHA256 du fichier source
`a23ed15546195ed75044b7d8e9ea7920c7079d3dfc1f3b23ee5851dcfe07671e`.
Le port utilise des `BallIdx` sans tag, une construction séquentielle, une charge
au plus 1/2, des sondages bornés et un budget explicite. Aucune qualification R2 héritée.

[domain.cpp](../tests/tower/domain.cpp) prépare huit portes : singleton sans boule positive,
lookup complet et collisions, distinction local/global, déplacements et vues, refus,
pic exact et résultats coexistants, lectures concurrentes et permutation des entrées.
[domain_fault.cpp](../tests/tower/domain_fault.cpp) refuse chaque allocation, y compris
la dernière réservée au lookup, puis vérifie la récupération et l'absence d'allocation en recherche.
Cinq mutants ciblent l'égalité remplacée par le hash, le padding oublié, l'arrêt sur collision,
le déplacement prématuré et la dernière boule omise.

Contrôles locaux effectués : style tower (13 fichiers), manifeste tower (15 mutants,
normal et `-O`) et `git diff --check`. Aucun test natif ni benchmark de cette tranche
n'a été exécuté localement ; les résultats attendent la session G4.
