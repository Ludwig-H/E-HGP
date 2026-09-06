# Journal FULL à couvertures datées

6 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [composant C++](../src/forest/full_coverage_certificate.hpp) encode
la topologie et les couvertures d'une hiérarchie déjà calculée. Son
schéma `full_dated_coverage_forest_v2` est **distinct** du certificat
régulier `full_minima_merge_forest_v1`. Ce n'est ni le constructeur
géométrique FULL, ni une archive, ni la qualification du catalogue amont.
La sonde ne l'appelle pas encore et conserve son refus des plateaux.

## Objet encodé

Le [résultat mathématique de l'auditeur](../audits/receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md)
autorise des contributions locales potentiellement redondantes. Pour une
boule B de population S=I∪U, la contribution D est S moins l'union des
couvertures strictes **locales**. Avec un parent strict, seuls des points
de U peuvent manquer ; sans parent, la naissance porte tout S. D n'est
pas le delta minimal de nouveaux points par rapport aux parents globaux.

| Structure | Contenu et invariant |
| --- | --- |
| `FullCoveragePopulations` | Banque immuable du domaine et des populations I/U, partagée par pointeur entre ordres ; aucun ensemble complet par racine |
| `FullCoverageBatch` | Un niveau exact, toutes les actions déjà regroupées par parents pré-lot |
| Action sans parent | Naissance d'un nœud avec une référence à la population entière, de cardinal au moins K |
| Action à un parent | Contributions sur le même segment ; **aucun nouveau nœud** |
| Action à plusieurs parents | Une vraie multifusion, parents distincts et vivants avant le lot |
| `FullDatedContribution` | Niveau, segment post-lot, référence de population, masque de U et indicateur d'inclusion de I |
| Successeur historique | Arc vers la multifusion suivante ; jamais remplacé par la racine finale |

Les identifiants sont denses, dans l'ordre des actions, en sautant les
continuations. Deux naissances au même niveau ne sont pas identifiées
par leurs listes de parents vides. Les recouvrements de points entre
racines sont conservés. Les listes de parents sont strictement triées ;
un parent utilisé dans deux actions du même lot est rejeté : le
regroupement géométrique des boules aurait dû les réunir en amont.

Une naissance utilise une seule population entière. En géométrie, des
boules distinctes de même rayon ne partagent pas de facette nouvellement
née, par unicité de sa MEB ; sans parent ancien partagé, ce sont des
naissances distinctes. Le composant contrôle le format, **pas** cette
propriété géométrique sur les populations fournies.

Un D vide ne crée pas de contribution. Une continuation sans contribution
n'est pas une action de journal ; cela n'autorise jamais à omettre son
ancre dans le futur constructeur. Une multifusion sans contribution est
valide et conserve toute son information topologique.

## Lectures et atomicité

`full_coverage_root_at(segment, cut, closed)` suit seulement les arcs dont
la date est admise : strictement avant la coupe ouverte, jusqu'à la coupe
fermée incluse. Il refuse un segment qui n'existait pas encore. Cela
reste vrai après construction complète de l'ordre ; c'est nécessaire
pour les futures images verticales vers un ordre inférieur déjà terminé.

`full_coverage_at(root, cut, closed)` exige une racine vivante à cette
coupe et restitue l'union triée de ses contributions admises, héritage
par multifusion compris. La date de chaque contribution est contrôlée
séparément de celle du nœud : une continuation ne réécrit pas la naissance.
Les doublons sont éliminés **dans la racine lue**, jamais entre racines.

Les constructeurs ne publient leurs arènes qu'après réussite entière.
Une entrée invalide, `bad_alloc` ou `length_error` rend un objet vide,
illisible ; un déplacement invalide aussi l'objet source. La banque
supprime copie, déplacement et affectations publiques : un alias mutable
ne peut pas vider une banque déjà référencée par un certificat. Les
contrôles de domaine et de références ne remplacent pas un census exact.

K1 impose les singletons du domaine, dans leur ordre, au niveau zéro,
sans naissance tardive. K>1 exige des niveaux positifs. Le cas terminal
K=n est représentable sans coface n+1 ; une fixture géométrique K=n=4
le vérifie. Le domaine des ordres reste celui de la représentation
existante, 1..10. La banque accepte jusqu'à 16 points de coquille, largeur
de son masque u16 ; cela n'étend pas le domaine géométrique u≤12 du
quotient local. Aucun plafond de taille du nuage, d'intérieurs, de travail
ou de durée n'est ajouté.

## Coûts et structures évitées

La banque coûte O(n+P), P étant le nombre total de références de points
des populations distinctes stockées, et se partage entre ordres. Sa
validation recherche les identifiants dans le domaine trié : O(P log n).
Pour un ordre, N nœuds, E références de parents et C contributions,
la construction coûte O(N+E+C), hors validation initiale de la banque ;
les lots d'entrée et arènes de sortie restent de cette taille. Aucune
facette Gamma, cellule de Delaunay supérieure ou copie des couvertures
complètes par racine n'est construite.

Une requête de racine coûte la profondeur parcourue. Le lecteur de
couverture matérialise un tableau de N racines, parcourt les contributions
admises et trie les références de points qu'il développe. Ce coût de
reconstruction explicite appartient au lecteur, pas à la production des
contributions. Ni ces bornes ni le masque ne promettent que N, P ou C
soient linéaires en n pour tout nuage 3D.

## Qualification du composant

La [gate](../tests/full_coverage_certificate_gate.cpp) et ses
[captures propres](../receipts/full_coverage_20260906/README.md) passent
710 contrôles O2 et ASan/UBSan, mêmes sorties, détection de fuites active :

- 34 coupes ouvertes/fermées contre l'oracle Gram/Gamma indépendant :
  carré K3, terminal K4, croissance ABCZ à K3 et K1=single-linkage à deux points ;
- 30 coupes contre un autre algorithme de rejeu, avec ensembles de points
  explicites seulement dans le juge : identités, recouvrements, fusions,
  contributions répétées, racines anciennes et futures ;
- 30 rejets d'entrée et 34 injections de panne d'allocation, chaque refus
  laissant une sortie vide ; cas de masque 16/17 et 5 000 intérieurs ;
- trois mutants causaux rejetés : omission des continuations, fuite d'une
  contribution future, normalisation vers une fusion future.

Six CTests ciblés frais comparent également ce delta, le quotient local
et l'ancien certificat structurel v1. Ces témoins ne certifient ni les
parents d'un nuage arbitraire, ni la complétude d'un producteur WSPD.
Les populations sont des identifiants, sans coordonnées ou BallKey dans
ce format ; l'association à une géométrie certifiée reste au constructeur.
Les poids du manuscrit et les datations de toutes les facettes ne sont
pas reconstructibles depuis ce seul journal.

## Prochain raccord

Consommer le census partagé, résoudre un représentant par composante
stricte dans l'état pré-lot, assembler les parents globaux, fermer le lot
du journal, puis installer ses ancres `(K, BallKey)`. Les ancres sont
obligatoires et séparées des nœuds de sortie et des caches facultatifs.

Les MEB F actuelles refusent une coquille sélectionnée plus grande que
leur support. Il faut une entrée exacte **à coquille libre**, pas réutiliser
le résultat d'un appel déclaré échoué. La descente garde K sites et peut
conserver le rayon si le nombre de sites sélectionnés sur la coquille
baisse strictement. Le hit d'ancre précède la recherche d'un intrus strict.
Ces étapes restent à implémenter et qualifier ; ni la garde générale
50k, ni l'autorité F ne changent avec ce journal. GCP non utilisé.
