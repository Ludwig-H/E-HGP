# Progression Phase 5 — ancre exacte $k=1$ et EMST

## Statut

- phase : `5`, en cours;
- backend : `reference_cpu`;
- profil : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; le différentiel indépendant jusqu'à $n=14$ et la voie EMST GPU scalable restent à livrer.

Les deux premiers lots de Phase 5 livrent l'ancre EMST exacte, le catalogue exhaustif des paires et la réduction rang-deux du backend CPU. Ils ne ferment ni la Phase 5, ni la porte globale G2 du catalogue, et ne publient aucun `public_status=exact` pour une tour MorseHGP3D complète.

## Ancre mathématique

Le nuage est d'abord transformé en `CanonicalPointCloud`; les sommets du graphe complet sont donc les `PointId` canoniques $0,\ldots,n-1$. Pour chaque paire distincte $(u,v)$, le noyau recalcule exactement la longueur carrée dyadique $d^2(u,v)$ et lui associe le niveau HGP $a(u,v)=d^2(u,v)/4$.

Toutes les fractions passent par `ExactRational` et tous les niveaux par `ExactLevel`. Le facteur $1/4$, les comparaisons, l'égalité des poids et le poids total sont donc rationnels exacts; aucune tolérance flottante n'intervient.

Les $n$ minima de rang un sont matérialisés comme feuilles singleton de niveau zéro. Pour $n=1$, la racine est cette feuille, le graphe et l'EMST sont vides, la coupe stricte au niveau zéro est vide et la coupe fermée contient le singleton.

## Lots égaux et multifusions

Les arêtes du graphe complet sont triées par `(longueur carrée exacte, u, v)`, puis groupées par égalité rationnelle exacte. Pour chaque lot :

1. les composantes strictement antérieures sont figées;
2. toutes les arêtes du lot sont projetées sur ces racines;
3. chaque composante connexe du graphe quotient devient une unique multifusion canonique;
4. les enfants sont ordonnés par leur couverture canonique en `PointId`;
5. Kruskal sélectionne ensuite un représentant déterministe de l'EMST;
6. les unions globales sont appliquées et l'état fermé du lot est enregistré.

La suite binaire des unions de Kruskal ne définit donc jamais la généalogie publique. Un chemin de six points séparés par la même longueur produit un seul nœud d'arité six, pas cinq fusions binaires dépendantes de l'ordre.

En présence d'ex æquo, le choix déterministe des arêtes permet le rejeu et l'audit, mais l'identité scientifique repose sur les coupes exactes, le poids total et les multifusions canoniques. Le résultat conserve aussi toutes les arêtes du graphe complet et tous les lots, y compris ceux devenus redondants après la connexion de l'arbre.

## API et audit

La cible installable `morsehgp3d::hierarchy` expose `build_exact_complete_graph_emst`. Le résultat contient :

- les arêtes complètes et les arêtes EMST avec longueur carrée et niveau HGP;
- les feuilles et nœuds de multifusion de $T_1$;
- les lots égaux avec états pré-lot et post-lot;
- les multifusions avec composantes enfants figées et union fusionnée;
- les poids totaux exacts géométrique et HGP;
- des compteurs de distances, lots, redondances, événements et arités;
- le rejeu des coupes strictes ou fermées depuis le graphe complet ou l'EMST sélectionné.

Le backend est volontairement quadratique : il matérialise le graphe euclidien complet et sert d'ancre exacte CPU. Cette portée est distincte de la future voie GPU destinée à plusieurs millions de points.

## Catalogue rang-deux et décision Gabriel

Pour chaque paire canonique $(u,v)$, une seconde voie reconstruit indépendamment son centre diamétral $c=(u+v)/2$, son rayon carré $a=left\Vert u-v\right\Vert^2/4$, puis interroge tout le nuage par `brute_force_closed_ball`. La partition globale exacte sépare intérieur strict, shell complet et extérieur; aucune liste locale, exclusion de support, tolérance ou limite de visites n'intervient.

La décision distingue trois cas :

1. `rank_two_critical` : l'intérieur est vide et le shell vaut exactement $\lbrace u,v\rbrace$;
2. `extra_shell_degeneracy` : l'intérieur est vide mais le shell contient un autre site;
3. `interior_blocked` : un site au moins appartient à l'intérieur strict.

Les deux premiers cas appartiennent au graphe de Gabriel diagnostique du contrat existant. Seul le premier entre dans la réduction rang-deux certifiée. Le second reste une décision exacte et une arête diagnostique, mais force `catalog_status=unsupported_degeneracy` et interdit `locally_supported`; il n'est ni masqué comme rejet numérique, ni promu sous l'hypothèse de position générale.

Le catalogue conserve les enregistrements des $\binom{n}{2}$ paires, leurs centres, longueurs, niveaux, intérieurs, shells et rangs fermés, ainsi que les trois projections d'arêtes : graphe complet, rang-deux critique et Gabriel diagnostique. Son coût de référence est $O(n^3)$ évaluations exactes de distance et sa mémoire est quadratique.

## Certificat EMST–rang-deux

Une arête d'un EMST ne peut avoir ni point strictement intérieur à sa boule diamétrale, ni point supplémentaire sur son shell : dans les deux cas, ce troisième sommet forme avec ses extrémités un cycle dont les deux autres arêtes sont strictement plus courtes. Toute arête EMST est donc `rank_two_critical`.

À chaque seuil exact, l'arbre témoin de Kruskal est ainsi inclus dans le graphe rang-deux, lui-même inclus dans le graphe complet. Le rejeu du certificat compare explicitement, aux niveaux réunis des trois graphes, les coupes strictes et fermées des cinq chemins suivants : graphe complet, EMST sélectionné, graphe rang-deux, arbre témoin rang-deux et graphe de Gabriel diagnostique. L'égalité des états strictement pré-lot et fermés post-lot impose alors les mêmes multifusions canoniques.

La réduction rang-deux possède son propre parcours de lots. Elle fige les composantes strictement antérieures, construit le graphe quotient du lot entier, matérialise une multifusion par composante connexe, puis seulement sélectionne et applique les arêtes témoins. Le certificat sépare l'égalité sémantique des coupes, multifusions, hiérarchie et poids de l'égalité supplémentaire du témoin déterministe. Cette dernière reste exposée comme contrôle d'implémentation mais n'entre pas dans l'agrégat `anchor_equivalence_certified`, car plusieurs EMST sont valides sous ex æquo.

## Vérifications des lots CPU

Le test C++ strict couvre le singleton, le facteur exact $3\mapsto3/4$, deux fusions disjointes dans un même lot, une multifusion d'arité six, les $24$ permutations d'un carré avec ex æquo, les poids exacts et l'accord des coupes du graphe complet avec celles de l'EMST à chaque seuil strict et fermé. Il vérifie également que les lots partitionnent le graphe complet, que chaque perte de composante égale le nombre d'arêtes sélectionnées et que la racine couvre exactement tous les `PointId`.

La compilation GCC avec avertissements transformés en erreurs, le test ciblé, le profil ASan/UBSan et l'export CMake passent. Cette vérification est une non-régression du noyau; elle ne remplace pas encore le différentiel indépendant complet jusqu'à $n=14$ exigé par la porte de sortie.

Le second test strict couvre les trois décisions de paire, une perturbation d'un ULP de chaque côté d'un shell diamétral, deux multifusions disjointes dans le même lot, un tétraèdre régulier 3D dont les six arêtes rang-deux se contractent en une multifusion d'arité quatre, les diagonales extra-shell et les $24$ permutations d'un carré, les coupes strictes et fermées exactement au seuil, ainsi que la fixture non locale où chaque extrémité possède cinq observations plus proches que la paire Gabriel recherchée. Il ferme aussi tous les compteurs du catalogue et compare nœuds, poids et témoins à l'ancre EMST.

GCP n'a pas été utilisé pour ce lot CPU.

## Suite immédiate

Le lot suivant doit exposer un dump canonique C++ et le comparer à l'oracle Python indépendant sur les fixtures, les dimensions affines un à trois, les égalités et les tailles bornées jusqu'à $n=14$. La voie EMST GPU scalable viendra ensuite; aucune promotion de phase ne précédera ces certificats.
