# De FULL à un arbre de points : ce qui est canonique, ce qui est un choix

29 septembre 2026 — réponse à la nouvelle priorité mathématique de l'utilisateur.
Moteur audité : `6206d1d11` ; instructions auditeurs : `ed7be3bc3`.
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_math_laminarite`,
`public_status=not_claimed`. Aucun changement moteur, aucune session GCP.

## Conclusion pratique

Il faut **séparer l'arbre de densité, l'affectation des points, et la sélection
d'une partition**. La tour exacte ne détermine pas une affectation exclusive
canonique de tous les points aux branches couvertes. La première couverture
est une affectation admissible et empiriquement utile ; elle n'hérite pas
automatiquement de la robustesse de la multicouverture.

Recommandation au développeur :

1. Conserver une référence `core` fidèle au modèle, avec les dates d'entrée,
   et ne pas supprimer la variante `cover` qui reste le témoin empirique.
2. Expérimenter une **affectation cohérente avec l'arbre et consciente de
   l'ambiguïté** : une décision par point, conservée dans tous ses ancêtres ;
   un point contesté est attaché à un ancêtre commun plutôt que forcé dans
   une branche. Les singletons complètent les partitions avant cette entrée.
3. Utiliser les autres ordres pour mesurer/corroborer la stabilité des branches,
   avant de construire une nouvelle sélection multi-K complexe. Une tranche
   monotone reste une option mathématiquement propre, déjà prototypée, mais
   pas une amélioration de score acquise.
4. Juger **la hiérarchie entière** et sa sensibilité, séparément de l'ARI de
   l'EOM et du remplissage. Les seules comparaisons de partitions plates ne
   répondent pas à la question actuelle.

Ce n'est pas l'annonce d'une nouvelle méthode statistiquement optimale. La
règle d'affectation et son seuil de confiance restent à construire et tester.

## 1. L'objet exact et sa restriction canonique

Pour un ordre fixé k, noter

`L_k(r) = {y : # (X ∩ B̄(y,r)) ≥ k}`.

Les ensembles L_k(r) sont croissants en r. Leurs composantes forment l'arbre
continu T_k. Pour une densité volumique 3D, ce sont les ensembles de niveau
de l'estimateur `fhat_k(y) = k / (n v_3 d_k(y)^3)` ; l'identité est exacte
pour l'estimateur, pas une égalité avec la densité génératrice.

La restriction **core** est la famille des `C ∩ X`, C composante de L_k(r).
Deux tels ensembles sont disjoints ou emboîtés. Ajouter comme singletons les
points extérieurs à L_k(r) donne, pour chaque r, une partition **de tous les
points**, et ces partitions sont emboîtées. Aucune affectation par vote n'est
nécessaire pour obtenir cette complétude formelle.

Un point x entre dans une composante au rayon d_k(x), point lui-même compris.
Avant cela il est un singleton, pas un membre certifié d'un autre amas.
La hauteur de fusion entre deux points distincts est

`u_k(x,y) = inf {r : x et y sont dans une même composante de L_k(r)}`.

Avec `u_k(x,x)=0`, c'est une ultramétrique : si x et y, puis y et z,
sont connectés à un rayon r, x et z le sont aussi. C'est la formulation
directe d'une hiérarchie laminaire sur les points.

**Limite réelle :** une branche spatiale de T_k peut ne contenir aucun point
du nuage avant sa fusion. La restriction core perd cette branche ; ce n'est
pas un défaut du calcul de FULL. Une branche visible dans l'espace n'impose
pas à elle seule quels points périphériques doivent lui être attribués.

## 2. Le conflit fondamental des amas couverts

La couverture discrète d'une composante est
`D_r(C) = {x ∈ X : dist(x,C) ≤ r}`.
Elle peut se recouvrir avec celle d'une autre composante au **même rayon**.

Exemple exact, embarqué en 3D sur l'axe x :

- X = {0, 2, 4}, k = 2, r = 1 ;
- L_2(1) a deux composantes, les centres {1} et {3} ;
- leurs couvertures sont {0,2} et {2,4}.

Aucune partition ne contient simultanément ces deux blocs. Trois issues :
choisir le propriétaire de 2 ; fusionner les deux branches ; ou différer
l'affectation de 2. **Laminarité, fidélité à toutes les couvertures et
affectation exclusive immédiate ne peuvent être exigées ensemble.**

La mention actuelle « cover = sémantique des amas discrets » doit donc être
précisée : les amas discrets forment un recouvrement ; la première couverture
est une *laminarisation particulière* de ce recouvrement, avec perte
d'appartenances. Elle ne restitue pas tous les amas discrets de la thèse.

### 2.1 Discontinuité confirmée dans le moteur

Le test [catalogue/cover_discontinuity.json](catalogue/cover_discontinuity.json)
compare les points 0,999,2000 et 0,1001,2000, k=2, modes core/cover :

| Mode | Fusion gauche–milieu, premier nuage | Second nuage |
| --- | ---: | ---: |
| cover | rayon 499,5 | rayon 1 000 |
| core | rayon 999 | rayon 1 001 |

Déplacer un seul point de 2 unités change donc une hauteur cover de 500,5.
Ce ne sont pas des labels EOM : **la hiérarchie elle-même** change.
Les dates rationnelles exactes et les commandes natives sont conservées.
Le test porte sur la projection avant condensation. Il ne démontre pas une
instabilité mesurée des sorties LiDAR : une condensation avec mcs > k peut
supprimer ces petites branches. Il réfute le transfert général de stabilité.

Plus généralement, sur {0,L−δ,2L} et {0,L+δ,2L}, le saut cover est
`L − (L−δ)/2 = (L+δ)/2` pour un déplacement `2δ`.
En géométrie réelle, δ→0 donne une discontinuité. Sur la grille u18 finie,
cela montre une forte amplification au pas minimal, pas une limite continue
à l'intérieur d'un ensemble fini. Le phénomène ne vient pas d'un arrondi
flottant ni seulement du départage d'une égalité exacte.

### 2.2 Borne positive pour core — preuve propre à cet audit

Supposons même nombre de points, mêmes identifiants et appariement
`||x_i−y_i|| ≤ ε` pour chaque i. Alors

`|u_k^X(i,j) − u_k^Y(i,j)| ≤ 2ε`.

Preuve : `L_k^X(r) ⊆ L_k^Y(r+ε)`, en transportant les k témoins de chaque
boule. Un chemin reliant x_i à x_j dans L_k^X(r) est donc dans L_k^Y(r+ε).
Chaque segment x_i→y_i est dans L_k^Y(r+2ε), en utilisant les k témoins
de x_i dans X. Ajouter ces deux segments relie y_i à y_j au rayon r+2ε.
Passer à l'infimum, puis échanger X et Y. La même démonstration s'applique
en toute dimension et aux niveaux de contact fermés.

La constante 2 est optimale pour cette formulation : deux sites 0,1,
k=2, puis −ε,1+ε. Le rayon de fusion core passe de 1 à 1+2ε.
La borne porte sur le **rayon**, pas sur son carré ni sur r^(−z).
Elle ne couvre ni suppression/ajout d'observations, ni changement de k,
ni EOM, ni la vérité statistique. Une tête peut choisir une autre partition
à proximité d'une égalité de scores même si l'arbre varie peu.

## 3. Affecter davantage de points sans détruire l'arbre

Sur un arbre spatial T fixé, donner à chaque point **un ancrage unique** :
une branche et une date sur cette branche, ou un nœud de fusion. Après son
entrée, le point suit uniquement les ancêtres. Avant son entrée, il reste
singleton. Cette règle produit toujours des partitions emboîtées.

L'ancrage doit exister à sa date : relever celle-ci au moins à la naissance
du nœud choisi ; si elle dépasse sa mort, remonter à son ancêtre vivant.
Les singletons de complétion ne doivent pas antidater une masse dans l'EOM.

Elle inclut core, la première couverture actuelle et une affectation
conservatrice à l'ancêtre commun de plusieurs candidats. Un simple LCA
donne cet ancêtre une fois les candidats choisis. Avec m candidats, le coût
doit être proportionnel à m (et au coût des requêtes d'ancêtres), pas à n².
Le coût d'obtention des candidats est à compter séparément.

**Proposition à tester : ancrage différé des ambiguïtés.**

- Une branche nettement soutenue reçoit le point.
- Plusieurs branches plausibles : attacher le point à leur plus proche
  ancêtre commun ; il ne vote pas artificiellement pour l'une avant leur fusion.
- Conserver une information d'incertitude à côté de l'arbre.
- Condenser ensuite avec la masse réellement attribuée, et publier le nombre
  de points différés. Pas de sélection EOM cachée dans l'affectation.

La garantie est conditionnelle : si la vraie branche est dans la liste des
candidats, le LCA ne prétend pas une appartenance plus fine que cette liste.
Ce n'est pas un théorème que la liste la contient. Un score géométrique, un
écart à la deuxième couverture ou un rééchantillonnage doivent être justifiés
et mesurés ; aucun seuil de confiance n'est encore qualifié ici.

Une affectation **molle** est également possible, avec des probabilités sur
les feuilles et addition vers les parents. Pour une sortie dure, décider une
fois au niveau fin puis prendre les ancêtres. Recalculer un vote gagnant
indépendamment à chaque coupe peut casser l'emboîtement. Les probabilités
elles-mêmes ne transforment pas un recouvrement en partition.

Le changement d'ancrage change les masses et peut changer la hiérarchie
observée sur X : ne pas qualifier la nouvelle sortie de restriction exacte
C∩X. Conserver côte à côte l'objet continu et la projection choisie.

## 4. Ce que peuvent apporter les K ordres

### 4.1 Une tranche monotone : construction propre, bénéfice non acquis

Si r(t) croît et k(t) décroît, `S(t)=L_{k(t)}(r(t))` est emboîté. Restreindre
ses composantes aux points fournit une nouvelle hiérarchie canonique de
core. Les verticales de FULL servent exactement à construire cette tranche.

Les travaux de [Rolle–Scoccola](https://www.jmlr.org/papers/v25/21-1185.html)
étudient des tranches stables de constructions multiparamètres ; leurs
hypothèses et métriques ne se transfèrent pas à une affectation cover/EOM
arbitraire. [Blumberg–Lesnick](https://arxiv.org/abs/2010.09628) établissent
la stabilité de la multicouverture vis-à-vis de perturbations de mesure :
c'est un argument pour conserver la tour, pas une certification de toute tête.

La v10 a déjà un prototype oblique. Son propre rapport ne trouve pas de
gain robuste de score sur la meilleure tranche fixe. Sa borne de retrait
devient vide dès que le nombre de points retirés atteint Kmax. Ne pas
relancer le même chantier sous un autre nom sans nouvelle hypothèse utile.

### 4.2 Mélanger les ordres ne donne pas gratuitement un arbre

Les blocs pris à des ordres et des rayons différents peuvent se croiser.
Une [fixture native de cinq points](pool_head/crossing_report.json) le montre
déjà pour core : sur {0,20,22,50,52}, l'ordre 1 au rayon 10 contient le bloc
{0,20,22}, et l'ordre 2 au rayon 15 contient {20,22,50,52}. Les paramètres
sont incomparables et les deux blocs se croisent. Toute réduction de FULL
à un arbre unique perd donc certaines composantes, même sans règle cover.

Une antichaîne dans un ordre d'inclusion n'est pas nécessairement disjointe :
{a,b} et {b,c} sont incomparables mais partagent b. A fortiori, une inclusion
à 90 % n'est pas une inclusion exacte.

Le prototype `mixq` utilise ces inclusions approximatives puis départage les
points contestés. Cela peut donner une partition utile ; ni une partition
plate ni son score ne prouvent une **hiérarchie complète laminaire** à tous
les réglages. Les gains dev restent un résultat distinct.

### 4.3 Un témoin de consensus mathématiquement sûr

Après calibration monotone de leurs hauteurs, soient u_j des ultramétriques
sur les mêmes points. Leur maximum `u=max_j u_j` est encore ultramétrique :

`u(x,z) ≤ max(u(x,y),u(y,z))`.

À chaque hauteur, c'est l'intersection des relations d'équivalence des
partitions. De plus, l'opération max n'amplifie pas leur erreur uniforme :
`||max u_j−max v_j||∞ ≤ max_j ||u_j−v_j||∞`.

**Attention aux faux gains :** en rayon brut, les restrictions core vérifient
`u_1 ≤ ... ≤ u_Kmax`, donc leur maximum est simplement u_Kmax. Pour un
consensus non trivial, il faut une autre calibration ou des répliques.
Une calibration empirique par quantiles a des plateaux et sa propre
incertitude : aucune stricte monotonie ou stabilité n'est automatique.

Le minimum et la moyenne ne conviennent pas en général. Sur trois points,
les triplets de distances (ab,bc,ac) `(1,3,3)` et `(3,1,3)` sont
ultramétriques ; leur moyenne `(2,2,3)` et minimum `(1,1,3)` ne le sont pas.
En ajoutant `(1,1,1)`, la médiane est aussi `(1,1,3)` et échoue.

Ce maximum est un témoin de sûreté, pas la nouvelle méthode recommandée
par défaut : une seule tranche trop fragmentée peut retarder les fusions.
Ne pas matérialiser une matrice n×n pour le calculer. Une éventuelle
implémentation doit exploiter les événements des arbres et démontrer son
coût ; aucun gain empirique ni coût G4 de cette variante n'est acquis.

## 5. Fiabilité statistique : bien définir la cible

Trois cibles différentes : composantes de niveaux de densité ; bassins
d'attraction de modes ; labels latents d'un mélange. Même avec la vraie
densité, ces trois partitions ne sont généralement pas identiques.
Sous un col, la densité seule ne désigne pas une branche séparée du niveau
considéré. Choisir un bassin demande une géométrie/une règle supplémentaires.
La tour des composantes seule ne contient pas le champ de gradient.

Une covariance ou des poids de mélange estimés avec les labels vrais
donnent un **diagnostic supervisé**, pas nécessairement la densité génératrice.
La règle MAP est optimale pour la perte de classification 0–1 sous son
modèle ; elle n'est pas un plafond théorique de l'ARI. Ne pas expliquer un
écart ARI par une « impossibilité pour toute méthode de densité » sans preuve.
La [lecture des références Bayes/Morse du dépôt](timeout/AUDIT_CIBLES_STATISTIQUES_20260929.md)
identifie précisément cette confusion, sans invalider leurs scores mesurés.

À K fixé égal à 5 ou 10, l'exactitude de l'estimateur empirique ne supprime
pas sa variabilité statistique. Les théorèmes asymptotiques demandant un k
croissant avec n ne qualifient pas ce petit K fixe. Inversement, les résultats
dev à plus grand K ne prouvent pas que grand K soit toujours préférable.

Pour le LiDAR, ne pas confondre échantillonnage capteur/surfaces avec une
densité volumique iid en dimension 3. La définition géométrique demeure
valable ; l'interprétation probabiliste et les perturbations pertinentes
(bruit de portée, raréfaction angulaire, suppression du sol) changent.

## 6. Prochaine expérience décisive, à budget limité

Avant toute vaste campagne : fixture trois points ci-dessus, puis quelques
petits nuages **dev** séparés/ambigus. Pas de réutilisation de `test_v10b`
pour régler la nouvelle projection. Comparer : core, cover actuel,
ancrage différé (si implémenté), tranche oblique core existante.

Mesurer séparément :

1. **Emboîtement exact**, conservation des identifiants, décisions par plateau,
   et invariance aux permutations d'entrée hors convention déclarée.
2. **Fidélité de l'arbre** : hauteurs de fusion d'un échantillon de paires
   déterminé avant les tests, branches vraies présentes/perdues, plutôt que
   seulement l'ARI de la meilleure coupe. La distance de fusion étudiée par
   [Eldridge–Belkin–Wang](https://proceedings.mlr.press/v40/Eldridge15.html)
   formalise précisément cet aspect distinct de la consistance de Hartigan.
3. **Robustesse** : jitter apparié, retraits, ajout d'isolés, variations de k,
   de mcs et de z ; ne pas résumer toutes ces opérations sous un seul mot.
4. **Allocation** : couverture, points ambigus/différés, pureté et rappel ;
   labels latents et modèle de densité évalués séparément.
5. **Sélection** : EOM aux mêmes z=1 et 2, puis réglages dev déclarés ; même
   condensation et même remplissage entre concurrents. Une victoire de la
   tête ne doit pas être attribuée à la structure FULL.
6. **Coût supplémentaire** : attaches + condensation + sortie explicite ;
   aucune matrice quadratique, aucun gradient dense caché. Les essais de
   score hors ligne ne qualifient pas le contrat FULL 100 ms.

La priorité n'est donc pas une nouvelle variante de z. C'est une définition
claire de l'appartenance des points, accompagnée d'un test qui distingue
meilleur arbre, meilleure affectation et meilleur choix de coupe.

## Contre-vérification et périmètre

Les preuves de structure, la borne core, les précautions de LCA et les
contre-exemples de consensus ont été relus indépendamment dans
[CONTRE_AUDIT_PROJECTION_MULTIK_20260929.md](pool_head/CONTRE_AUDIT_PROJECTION_MULTIK_20260929.md).
Les deux contre-exemples géométriques sont également exercés dans le moteur,
sur quelques points seulement. Ce rapport ne qualifie ni une nouvelle tête
ni un gain de score ou de temps ; il propose la cible et les tests de la suite.
