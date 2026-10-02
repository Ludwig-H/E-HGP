# Oracle initial v11 : contrôle mathématique borné

Reçu du 2 octobre 2026, sur une **première copie figée avant tout test** de
l'oracle alors en construction. [SOURCE_BEFORE.json](SOURCE_BEFORE.json)
ferme dix fichiers de référence et trois documents ;
[SOURCE_AFTER.json](SOURCE_AFTER.json) signale les changements ultérieurs.
Neuf fichiers de référence ont évolué avant clôture (détails dans le ledger).
Ce verdict ne porte pas sur ces nouvelles versions. Aucun fichier développeur,
Git, moteur, build ou GCP modifié.

**Aucun défaut mathématique trouvé sur le périmètre contrôlé.**
Les [sorties normal](normal.json) et [−O](optimized.json) sont identiques :
**1732 gardes**, dont 616 coupes analytiques et 760 incidences verticales.
[RUN.json](RUN.json) conserve commandes, codes retour, empreintes des flux et
des sources privées après calcul. Les deux modes tournent sur une copie `/tmp`,
jamais sur le paquet développeur mutable. Aucune suite de 341 ou 5615 nuages
n'a été lancée. Aucun essai en échec dans ce lot.

## Définition, plateaux et couverture

La voie A calcule le nerf des régions témoins des k-parties avec des MEB en
Fraction et un balayage de graphe. Elle ne connecte explicitement que les
k-parties dont l'union contient k+1 points. Cela suffit pour les composantes :
si une union plus grande tient dans une boule, un chemin d'échanges d'un point
entre les deux k-parties reste dans cette union et chaque étape tient dans la
même boule. La réduction conserve donc la connexité du nerf complet.

La voie B calcule les supports critiques, coquilles entières, morceaux de
Gordan, descentes et Kruskal par plateaux. Les règles observées respectent le
centre dans l'enveloppe **fermée** pour la non-séparabilité, les intérieurs
**stricts**, les coquilles au contact, `p+q_min ≤ Kmax+1` pour les fusions,
la fenêtre basse `p+q_min−1` et le cas distinct des boules de rayon zéro.
Les entrées core et cover utilisent la coupe fermée après le plateau ; cover
conserve l'ensemble des composantes ex aequo, avec un choix canonique séparé.

Les faits géométriques ajoutés aux comparaisons sont :

- Triangle équilatéral **exact en 3D**, `(1,0,0),(0,1,0),(0,0,1)` : trois
  arêtes à β = 1/2, multifusion ternaire à β = 2/3.
- Triangle rectangle de côtés 6 et 8 : à β = 25, support antipodal q_min = 2
  et **trois** sites de coquille ; le sommet de l'angle droit n'est pas perdu.
- Carré de côté 2 : coquille de quatre sites à β = 2, quatre morceaux K2 et
  naissance K3. Les diagonales ne sont pas séparables.
- Tétraèdre régulier entier : centre `(1,1,1)`, q_min = 4, β = 3 et
  multifusion K3 à quatre parents.
- Deux triangles entiers de la fixture connue : à
  `β = 249978000484/187489`, FULL porte **ABC, CD, DEF**, puis une racine
  à trois parents à β = 3731956. Les points core entrent après cette fusion.
  Ces triangles avec h = 1732 sont légèrement isocèles ; ils ne remplacent
  pas le modèle planaire irrationnel exactement équilatéral de la thèse.

Chaque intérieur et chaque coquille des catalogues de ces cinq cas est aussi
relu par le signe rationnel direct `Σ(p_i−c_i)²−β`, indépendamment de `side_key`.
Ce contrôle utilise toutefois le centre et β publiés par B : il vérifie les
incidences, sans constituer seul un troisième calcul des centres.

## Ce qui est indépendant, et ce qui est partagé

La géométrie A/B est séparée : A utilise un système de Gram et Gauss-Jordan en
Fraction ; B utilise les formules entières de `intgeom.py`. A utilise Γ ; B
utilise morceaux, descentes et catalogue. A n'importe ni B ni intgeom.

Le modèle commun contient néanmoins `canonical_nodes` et `parents_of`, donc
une faute structurelle commune reste possible : le mot « passif » n'implique
pas une indépendance absolue de toute la construction. Les dumps partagent en
plus le catalogue, ses écritures non réduites, sa numérotation et son départage
cover, comme leur documentation l'annonce. Un accord des seuls dumps A/B ne
qualifie pas indépendamment ces conventions.

Le [troisième juge](check.py) réduit cette dépendance sur quatre droites :
`0,2,4` ; `0,1,2,6,9` ; `0,0,2,2,5` ; quatre sites confondus. Sa vérité utilise
uniquement `β(F)=(max F−min F)²/4`, **toutes** les unions de deux k-parties,
et les distances ponctuelles. Aucun solveur, support, modèle canonique ou
géométrie produit n'y décide les composantes. Il contrôle les coupes ouvertes,
fermées et intermédiaires, les core/cover, puis l'image verticale de chaque
k-partie active, dans les deux voies, avec conservation des retours doublons.

Pour associer les composantes à leurs identifiants publiés, le juge appelle
`node_at` de chaque voie et remonte les parents avec sa propre fonction ; il
compare ensuite le groupement complet des k-parties et leurs masques à la
vérité analytique. Les autres cas utilisent le juge A/B existant, renforcé
par les constantes ci-dessus : ils ne sont pas présentés comme un troisième
oracle exhaustif indépendant en dimension trois.

## Fermeture et limites

[SHA256SUMS](SHA256SUMS) ferme programme, copies, sorties et ledgers ; le
lecteur le vérifie lorsqu'il existe. Pour rejouer, copier `sources/reference/`
dans un nouveau dossier `/tmp`, puis lancer `python3 -B check.py CHEMIN/reference`
et la même commande avec `-O`.

Ce lot accompagne un oracle en construction. Il ne qualifie pas la complétude
générale de la voie constructive, les budgets de bits C++, le moteur pondéré,
les dumps natifs ni les performances. La géométrie aux multiplicités est
contrôlée ici sur les petites droites déclarées, sans transfert à une campagne.
