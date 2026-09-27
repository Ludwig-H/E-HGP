# Protocole ponctuel, thèse et SIPU — 27 septembre 2026

Audit de sources et proposition de protocole, **pas une nouvelle exécution
historique ni une reproduction acquise de SIPU**. Aucun benchmark, build,
appel GCP ou changement des sources/captures gelées dans ce travail.
Les expériences pertinentes des parties I–II ont été relues directement
dans le PDF, ainsi que les chemins actifs de HGP-old et le notebook public
cité par l'article. Le précédent
[audit thèse/HGP-old](RELECTURE_THESE_ET_HGP_OLD_20260927.md) a servi d'index,
pas de substitut à cette lecture.

## 1. Conditions des résultats historiques

Source : [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf).
Pages **imprimées** ci-dessous ; page PDF = page imprimée + 26.

| Passage relu | Conditions et résultat | Ce qu'il ne prouve pas |
|---|---|---|
| Partie I, §4.4.3–4, p.38–39 | Condensation selon la taille, continuation si une seule branche est admissible, puis optimisation EOM des stabilités | Une bonne géométrie ne garantit pas la meilleure sélection selon des labels externes |
| Partie I, §5.3–5.4, p.45–48 | Anomalies 3D guidées par un modèle spatial ; SemanticKITTI guidé par classes/dimensions, puis suivi temporel | Ce ne sont pas des victoires d'EOM non supervisé sur SIPU ; les a priori et post-traitements font partie des méthodes |
| Partie II, §7.4, p.73–76 | 100 répétitions, uniformes, dimensions 2/3/4, K1–5, n100000 sauf n10000 en dimension4, quantiles 3 %/97 % ; meilleure vitesse de percolation mesurée pour HGP dès K2 | Ni ARI, ni choix EOM, ni garantie pour tous les mélanges gaussiens finis |
| §9.1, p.96–97 | Scores de facettes, normalisation par point, masses fractionnaires, condensation puis vote | La proposition7 concerne une partition pour une sélection fixée, pas des coupes ponctuelles emboîtées |
| §9.2.5, p.101–103 | SIPU, K2, seuil annoncé √n, densité 1/r, EOM ; ARI, groupes trouvés et proportion classée | Ni supériorité universelle ni qualification de notre nouveau routage |

Le tableau9.3 conserve quinze jeux : `a1`, `a2`, `a3`, `aggregation`,
`birch1`, `birch2`, `d31`, `jain`, `s1`, `s2`, `s3`, `s4`, `spiral`,
`unbalance`, `worms_2`. Aux trois décimales publiées : dix ARI supérieurs,
quatre égaux, un inférieur pour HGP. Notamment `d31` : 0,835 contre 0,659 ;
`s3` : 0,418 contre 0,251. Mais **birch2 : 0,441 contre 0,996**, et
`worms_2` classe moins de points (66,4 % contre 74,2 %), malgré un ARI
supérieur (0,076 contre 0,055). Ces cas restent dans le bilan.

La p.103 indique qu'un exposant2 corrige le nombre de groupes de birch2,
sans fournir dans ce passage un nouveau tableau ARI complet. Ce diagnostic
motiverait une variante z2 préannoncée ; il ne permet pas de remplacer
après coup les seules lignes défavorables de z1.

L'[article local ANS 2026](../../docs/references/pdfs/hauseux-et-al-hgp-applied-network-science-2026.pdf),
p.18 et tableau4 p.21, précise : données **2D**, K2, HDBSCAN
`min_samples=3`, ARI sur **tous** les points, bruit inclus. Les huiles
d'olive sont un autre protocole : 572 points, 8 dimensions, colonnes
centrées/normalisées et comparaison avec Persistable ; ne pas transférer
son remplissage 1-NN au tableau SIPU.

## 2. Trois voies historiques à ne pas confondre

### Notebook K2 public associé à l'article

Le [notebook des auteurs](https://github.com/Ayana-Inria/HypergraphPercol_K2/blob/main/HypergraphPercol_K2_Colab.ipynb)
a été lu, **pas exécuté**. SHA256 des octets lus :
`8ec05a957c5828fbcedc77c9a5abd75c6fdf852ac98d092d01e818d070cf9a18`.
Le lien `main` reste mutable ; ce hash identifie la lecture, pas une capsule
d'exécution de l'article.

- Cellule10 : paquet `hdbscan` (contrib), `min_samples=3`, seuil
  `int(round(sqrt(n)))`, vérité `labels[0]`, ARI avant recoloriage supervisé.
- Cellule9 annonce `ceil`, mais la cellule exécutante utilise `round`.
- Cellule6 : plus grande composante de facettes, puis première incidence
  `(rayon, identifiant de facette)` de chaque point ; construction d'un
  arbre de points et appel à `_tree_to_labels`, racine exclue.
- Le seuil effectif de cet appel HGP est **seuil−1**, contre seuil pour
  HDBSCAN. Ce décalage historique ne sera pas repris comme avantage caché.
- `expZ=False` saute la transformation de Z : les rayons restent r,
  donc la sélection standard travaille avec 1/r. Ce n'est pas l'exposant0.
  Aucun calcul de masses de facettes ni vote pondéré dans cette voie.

Il faut donc distinguer le code public K2 du §9.1 et de HGP-old. Cette
lecture ne certifie ni ses décisions numériques ni l'identité de son
environnement avec celui du tableau publié.

### HGP-old actif

Dans [core.py](../../HGP-old/src/hgp_clusterer/core.py), lignes202–215,
les scores Sτ donnent T_x puis mτ ; lignes255–263, la condensation consomme
ces masses ; lignes297–324, un vote sur les facettes sélectionnées produit
les labels. Les défauts sont K2, z2, sans propagation1-NN et sans
sous-échantillonnage (lignes41–59), m arrondi√n et `min_samples=K+1`
(lignes139–149). `estimator.py` réexporte cette classe.

[hypergraph.py](../../HGP-old/src/hgp_clusterer/hypergraph.py), lignes119–128,
transforme les rayons carrés en r^z ; [_cython.pyx](../../HGP-old/src/hgp_clusterer/_cython.pyx),
lignes838–848, additionne leurs inverses, plafonnés à 10^12 pour les très
petites valeurs. La masse et λ changent donc avec z. Float32, repli Rips
possible et perturbation Geogram ne constituent pas un oracle géométrique
exact du pipeline FULL actuel.

L'EOM historique peut sélectionner une racine
([clustering.py](../../HGP-old/src/hgp_clusterer/clustering.py), lignes157–227).
Son option `whole_tree` distribue les points entre enfants, mais démarre
séparément des racines sélectionnées, utilise `argmax` pour les égalités
et une matrice points×enfants (lignes404–561). Elle n'est pas appelée par
le `fit()` usuel ; ce n'est pas notre contrat exclusif global et daté.

### HGP-Clusterer3D et catalogue contributif

La [lecture indépendante de HGP-old/Clusterer3D](LECTURE_HGP_OLD_CLUSTERER3D_20260927.md)
distingue aussi le catalogue d'ordre-Voronoï du catalogue Gabriel strict,
la racine admissible et les conventions numériques. **Même squelette FULL
ne signifie pas mêmes scores Sτ**. Avant de prétendre reproduire une ancienne
victoire, il faut nommer et qualifier son catalogue, pas uniquement copier K
ou la règle EOM. L'effet catalogue et l'effet projection restent deux
expériences distinctes à prévoir, sans modifier les preuves existantes.

## 3. Protocole annoncé pour le nouveau dendrogramme de points

La [référence de routage](../experiments/weighted_clustering_20260927/POINT_ROUTING_REFERENCE.md)
fixe une attache datée par point, traite les plateaux atomiquement et garde
les extérieurs comme singletons. Le dendrogramme compact doit en conserver
toutes les coupes, avec **une masse unitaire par point**. Condenser ensuite
selon m points est une nouvelle méthode, distincte du seuil massique des
facettes suivi d'un vote plat. Aucune dominance ne découle de l'emboîtement.

Plan convenu avec le responsable **avant évaluation des nouveaux labels** :

- Les treize scènes 3D existantes restent un **diagnostic connu**, pas un
  lot tenu à l'écart. Nuages complets et identités précédentes conservés.
- K5 seulement ; m20/50 et z1/2, soit 52 sélections ponctuelles. Profil
  principal **m20, z1**. Toutes les autres lignes publiées, aucun meilleur
  z choisi séparément pour chaque scène.
- Scores des 26 mesures z1/z2 déjà closes : conversion exacte des binary64
  en Fraction, sans recalcul des poids. Routage exact **sur ces arrondis**,
  pas sur les poids réels géométriques. Attaches et β hérités exacts ; EOM
  binary64, conversion sqrt(β) et collisions de niveaux à contrôler/refuser.
- Racine EOM exclue, seuil m commun en points, aucun remplissage1-NN,
  aucune scission guidée par labels ; bruit et points extérieurs conservés.
- Comparateurs figés : vote pondéré FULL, première couverture, arbre
  HDBSCAN avec EOM commun et sortie HDBSCAN standard distincte. Déclarer
  pour chaque méthode la mesure, λ et la convention de voisinage.
- ARI sur tous les points en mesure principale, bruit −1 conservé ;
  ARI avec bruit individualisé, couverture, nombre de groupes et F1 macro
  apparié en diagnostics. Les oracles de récupérabilité de l'arbre utilisent
  les labels seulement **après** construction, jamais pour choisir EOM.
- Conserver échecs, sorties, commandes et hashes avant/après. Séparer temps
  d'export géométrique, routage, condensation et capture ; pas de gain GPU
  déduit de ce prototype Python.

## 4. Retour à SIPU, puis confirmation indépendante

SIPU n'est pas un lot tenu à l'écart : ses résultats sont déjà connus.
La reproduction documentaire doit conserver les quinze jeux du tableau,
**dont birch2 et worms_2**. Les exclusions historiques sont `compound`,
`flame`, `pathbased`, `r15` (plusieurs références), et `worms_64` (dimension64).
Une extension peut rapporter toutes les références ambiguës, sans retenir
par méthode celle qui la favorise.

Les sources primaires sont le [site SIPU](https://cs.uef.fi/sipu/datasets/)
et la [suite ClustBench versionnée v1.1.0](https://github.com/gagolews/clustering-data-v1/tree/v1.1.0/sipu).
Aucun jeu SIPU nommé n'a été trouvé dans les fichiers locaux versionnés
inspectés. Aucun téléchargement de données ni fit effectué ici.
Avant capture : figer version, fichiers, labels et hashes. Le
[prétraitement ClustBench](https://clustering-benchmarks.gagolewski.com/clustbench-documentation.html)
peut ajouter un petit bruit : il doit être désactivé ou explicitement
reproduit, jamais hérité silencieusement.

Pour une **nouvelle comparaison commune**, préannoncer K2, m=round√n,
z1 principal/z2 secondaire, même m effectif pour HGP et HDBSCAN, racine
exclue, sans propagation. La convention historique contrib `min_samples=3`
correspond à sklearn `min_samples=4` ; sklearn compte le point lui-même,
contrairement à contrib ([documentation officielle](https://scikit-learn.org/stable/modules/generated/sklearn.cluster.HDBSCAN.html)).
Ne pas transformer cette correspondance logicielle en théorème K-HDBSCAN.
Les réglages historiques m−1 et ceux du nouveau protocole doivent porter
des noms différents. Aucun ajustement aux labels par jeu.

L'inclusion 2D→3D `(x,y,0)` conserve les distances ; un jitter ou une
quantification les modifie. Si l'export natif ne prend pas les données
originales sans perte, annoncer ce blocage ou ce nouveau profil, sans
appeler le résultat reproduction exacte SIPU. Les jeux100k ne deviennent
pas des jeux réduits : en cas de budget insuffisant, publier leur statut
non exécuté et la couverture incomplète du benchmark.

Après gel de la méthode, proposer de **nouvelles graines préenregistrées**
des cinq régimes 3D, évaluées une seule fois sans sélection sur les scores.
Une amélioration moyenne doit être annoncée comme telle : deltas appariés
par scène, victoires/égalités/défaites, couverture et incertitude entre
répétitions ; jamais un gain universel déduit des seuls cas favorables.
Ce lot de confirmation reste à définir et à capturer, pas acquis ici.
