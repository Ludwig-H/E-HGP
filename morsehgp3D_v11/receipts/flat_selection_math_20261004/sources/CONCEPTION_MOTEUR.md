## 6. Points et tête : rendre chaque perte observable

Publier d'abord les dates core, les dates cover $A_k(x)$ et les ensembles complets $E_k(x)$,
avec la relation des cellules fortes vers les nœuds vivants. Le choix d'un propriétaire exclusif est
une transformation nommée qui vient ensuite. La projection LCA publie sa date retardée ; elle ne
réécrit pas la date de première couverture. Les deux triangles, le site médian symétrique et une
continuation étendue gagnant un point sont des portes permanentes.

Chaque famille projetée est laminaire à ordre fixé. Une hiérarchie commune à plusieurs K exige un
critère supplémentaire : les descendants core peuvent se croiser, même une fois toutes les attaches
terminées (témoin à sept sites de P4 dans [MATHEMATIQUES.md](MATHEMATIQUES.md)). Conserver d'abord les
familles par ordre ; tout choix commun devra publier les groupes qu'il perd.

Comparer successivement la présence d'un groupe géométrique, sa conservation après projection, la
compatibilité simultanée de plusieurs groupes, puis ce que sélectionne la tête. Le meilleur IoU
d'une cible sur tous les nœuds borne le potentiel de cette hiérarchie pour cette cible ; plusieurs
meilleurs nœuds peuvent se chevaucher et ne forment pas nécessairement une partition réalisable.

La condensation doit balayer les cohortes de départs et tous les nœuds d'un même plateau simultanément,
y compris une attache qui entre exactement au niveau du parent. Les masses se conservent. L'option
allow_single autorise la sélection de la racine, sans l'exempter de min_cluster_size. Définir séparément
la stabilité, EOM/leaf, les égalités et le bruit ; contrôler les groupes finaux par leur masse exclusive.
Un correctif v10 par cohortes et ses preuves bornées existent : ils inspirent des portes, sans
qualifier un port v11 ni la règle vote.

Le domaine de $\lambda$, les poids, les produits poids × durée et leurs sommes doivent être traités
ensemble. Une garde « lambda finie » n'empêche pas le débordement d'une stabilité. Les cas zéro,
infini, exposant z, poids et racine exigent un contrat explicite et des refus avant publication.

La comparaison HDBSCAN utilise l'implémentation scikit-learn réelle, version et paramètres épinglés.
Les graphes témoins MR1/MR2 et leurs attaches cœur/bord sont des ablations nommées ; ils ne remplacent
pas cette référence. Le témoin exact $\{0,2,5\}$, K2, distingue déjà cover et MR2-bord avant EOM ;
des moyennes proches ne prouvent ni l'égalité des hiérarchies ni leur équivalence statistique.
