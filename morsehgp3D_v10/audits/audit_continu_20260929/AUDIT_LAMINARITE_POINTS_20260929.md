# De FULL à un arbre de points : ce qui est canonique, ce qui est un choix

29 septembre 2026 — réponse à la nouvelle priorité mathématique de l'utilisateur.
Moteur audité : `6206d1d11` ; instructions auditeurs : `ed7be3bc3`.
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_math_laminarite`,
`public_status=not_claimed`. Aucun changement moteur, aucune session GCP.

**Actualisation après relecture directe des parties I et II de la thèse :**
la recommandation initiale de `core` comme référence « fidèle au modèle »
était trop large. Core est une restriction exacte et stable de l'arbre
continu, mais **pas la cible des amas discrets de la définition 8** : elle
retarde ou perd leurs points frontière. La priorité scientifique est une
projection qui préserve cette participation précoce, sans changer la
connexité de FULL. Voir la section 7, qui précise et corrige cette priorité.

## Conclusion pratique

Il faut **séparer l'arbre de densité, l'affectation des points, et la sélection
d'une partition**. La tour exacte ne détermine pas une affectation exclusive
canonique de tous les points aux branches couvertes. La première couverture
est une affectation admissible et empiriquement utile ; elle n'hérite pas
automatiquement de la robustesse de la multicouverture.

Recommandation au développeur :

1. Conserver `core` comme référence de contrôle exacte et stable de la
   restriction C∩X, **non comme remplacement des amas discrets couverts**.
   Préserver les points frontière avec `cover` et une politique d'ancrage
   cohérente ; comparer cette politique à la première couverture actuelle.
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

Un point x entre dans la restriction core au rayon d_k(x), point lui-même
compris. Avant cela il est un singleton dans **cette projection** ; il peut
déjà être un membre couvert certifié d'un amas discret de la thèse.
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

Le test [catalogue/cover_discontinuity.json](../../receipts/audit_continu_20260929/catalogue/cover_discontinuity.json)
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
Une [fixture native de cinq points](../../receipts/audit_continu_20260929/pool_head/crossing_report.json) le montre
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

## 7. Relecture des deux premières parties : la frontière est une cible, pas un détail

Source directement relue : [manuscrit original](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
SHA256 `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.
Lecture intégrale des pages PDF 35–76 puis 77–134 (parties I et II,
chapitres 2 à 9), avec `pdftotext -raw`, en plages de 6 à 12 pages,
et lecture du sommaire. La pagination imprimée est PDF−26 dans ces chapitres.
Les annexes HGP-old ont aussi été relues, mais ne remplacent pas cette source.
Le [reçu de lecture et du petit test de vote](../../receipts/audit_continu_20260929/thesis_boundary/receipt.json)
fixe source, plages et périmètre ; il ne prétend pas certifier une interprétation
mathématique par le seul hash d'un PDF.

### 7.1 Ce que dit précisément le modèle

La **définition 8**, p. imprimée 21 / PDF 47, définit l'amas discret par
`X ∩ δ_r(C)`, C composante du niveau continu, **pas par C∩X**. Le point
doit participer à un amas dense ; il n'a pas à être lui-même un centre
dense. Le chapitre 6 établit cette correspondance par les intersections
de K boules, sans imposer de partition exclusive à la famille des amas.

Le manuscrit emploie « moins de r » dans cette définition. Les dates de
contact ci-dessous sont les seuils de naissance, avec la convention **fermée**
déclarée par v10 ; pour une convention ouverte, prendre les rayons juste
au-dessus. Cette différence aux seules égalités ne change pas le problème
de récupération de la frontière dans l'intervalle qui suit le contact.

Pour deux sites à distance d et K2, l'amas les couvre dès d/2 ; ils ne
deviennent core qu'à d. Exclure la frontière revient donc ici à retarder
**tous** les points de l'amas, pas seulement quelques observations douteuses.
Cette participation est géométriquement définie : ce n'est ni un remplissage
1-NN après sélection ni un artifice de score ARI.

Le chapitre 7 distingue aussi deux opérations qu'il ne faut pas confondre :

- connecter une structure déjà épaissie peut provoquer une fusion parasite
  précoce ; FULL doit conserver les composantes du niveau **avant** dilatation ;
- demander que le point soit lui-même un cœur réduit le rappel avant cette
  fusion ; la couverture récupère les points participants.

Le « core » de la comparaison RSL du chapitre 7 n'est pas notre simple
restriction `C∩X` d'un FULL exact : sa construction de connexité diffère
aussi. On ne peut donc pas attribuer directement sa colonne de résultats
à la projection core v10. Il faut tester séparément arbre et affectation.

### 7.2 Le bon diagnostic statistique : récupérer avant la fusion parasite

Les sections 7.2–7.4 étudient la percolation et la fraction récupérable des
zones denses avant que leur séparation ne soit perdue. La table 7.1,
p. 74 / PDF 100, donne en dimension 3 une vitesse de percolation empirique
passant de 0,563 à 0,732 pour HGP lorsque K passe de 1 à 5, dans son
protocole Poisson/binomial. Ce nombre est le **rapport des quantiles
d'intensité λ_ε/λ_(1−ε)**, pas une fraction directement récupérée, une borne
universelle d'ARI ou un résultat sur LiDAR.
Le passage asymptotique de la section 7.5 comporte une limite admise et
des intuitions : il ne qualifie pas notre tête actuelle.

Conséquence pour l'ancrage : différer tous les points contestés jusqu'à un
ancêtre commun donne bien un arbre, mais peut éliminer l'avantage recherché
en les rendant disponibles seulement **après** la fusion de deux vrais amas.
La marge de stabilité locale de notre bande ne suffit pas à éviter cela.
Il faut publier le rappel frontière **avant fusion**, la masse réellement
différée et sa date, pas seulement les hauteurs ou l'ARI de la meilleure coupe.

### 7.3 Ce qu'apporte le chapitre 9, et ce qu'il ne démontre pas

La section 9.1, pp. 96–97 / PDF 122–123, conserve l'arbre de faces, puis
donne à chaque point incident avec `Tx>0` une masse totale unitaire
distribuée entre ses faces incidentes : `Sτ / Tx`, où `Tx` est la somme
de leurs scores `Sτ`. La convention du manuscrit est `1/Tx=0` lorsque
`Tx=0` : un point sans face incidente ne reçoit pas cette masse. La masse de face est
`mτ = Sτ ∑(1/Tx, x∈τ)`. Elle sert à la condensation ; ensuite un vote
pondéré convertit **une sélection fixée** de clusters en partition stricte.
La proposition 7 a cette portée précise. Elle ne démontre pas qu'un vote
gagnant recalculé à toutes les coupes donne une hiérarchie de points emboîtée.

Un [petit test exact indépendant](../../receipts/audit_continu_20260929/thesis_boundary/check_vote.py)
montre le problème abstrait : dans les branches A/B/C, x porte les masses
0,3/0,3/0,4 et y les masses 0,1/0,1/0,8. Ils sont tous deux affectés à C.
Après la seule fusion A+B, x choisit AB et y reste dans C : un bloc de
points vient de se scinder alors que les blocs de faces n'ont fait que
fusionner. Les scores sont positifs et fixes, la masse totale est conservée,
et aucun départage d'égalité n'intervient. Le test construit ces votes avec
six faces à deux sommets et les normalisations du manuscrit ; il ne prétend
pas produire un nuage euclidien réalisant ce même arbre de faces.

Ce qui reste utile de HGP-old est donc **la conservation de masse et le
traitement de la frontière**, pas une garantie inexistante d'emboîtement
du vote à toutes les coupes. Une affectation fixée une fois puis suivie par
les ancêtres passe le même test de laminarité. Ses scores de choix et ses
dates doivent encore être justifiés et testés.

La comparaison SIPU de la section 9.2.5 prend une même condensation et
un même estimateur 1/r. Le manuscrit rapporte lui-même une exception
birch2 où HDBSCAN fait mieux avec cet exposant, puis une amélioration HGP
avec 1/r². Préserver la frontière est donc une piste fondée, pas une
garantie de domination pour chaque jeu, ni un prétexte pour changer z
uniquement chez l'un des concurrents. Les mesures avec remplissage 1-NN
doivent rester séparées des affectations natives.

### 7.4 Consigne au développeur : préserver sans percoler ni surconstruire

1. Conserver FULL comme arbre spatial de référence. Ne pas fusionner ses
   branches parce qu'elles couvrent un même point frontière.
2. Exploiter le [lemme de couverture par catalogue](catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md)
   pour conserver **toutes les composantes couvrantes**, avec la coquille
   complète et les intérieurs, pas seulement le support ou `first[x]`.
   Sa condition renforcée concerne les témoins de couverture, jamais la
   suppression des événements de fusion de FULL.
3. Dédupliquer les candidats par composante à la coupe considérée : plusieurs
   boules dans une même branche ne rendent pas le point ambigu entre clusters.
   Garder l'attache précoce quand la concurrence est seulement apparente ;
   traiter séparément les véritables conflits entre branches.
4. Décider un propriétaire et une date cohérents, puis suivre ses ancêtres.
   Comparer une attache dure précoce, l'ancrage différé et un diagnostic de
   masse fractionnaire inspiré de la thèse. Ce dernier est un diagnostic,
   pas encore une sortie dure laminaire ni un nouvel EOM qualifié.
5. Garder `core` comme contrôle de stabilité et de coût, non comme cible
   statistique prioritaire. Ajouter de petits nuages 3D à zones denses,
   périphéries et couloir peu dense ; mesurer ce qui est récupéré avant
   connexion parasite, puis jitter apparié et EOM z1/z2 équitablement.
6. Ne pas reconstruire toutes les faces/cofaces de HGP-old pour obtenir ces
   poids. Une clé de boule n'est pas un simplexe ; ses incidences n'ont pas
   automatiquement le score Sτ du chapitre 9. Exiger la bonne unité de masse
   et mesurer incidences, attaches, condensation et sortie explicite.

Le bénéfice théorique est maintenant mieux ciblé ; aucun nouveau score de
clustering, chrono FULL/G4, ni port GPU ne découle de cette relecture.
Les arguments de réduction Gabriel des chapitres 8 et 9 ne deviennent pas
des oracles par cette relecture : les contre-exemples archivés qui motivent
FULL restent applicables. Le nouveau lemme de couverture suppose un
catalogue critique complet, pas le seul graphe de Gabriel d'une ancienne version.

## 8. Une laminarisation par majorité de masse fixe — piste mathématique, pas nouveau moteur

Le vote du chapitre 9 suggère une alternative plus structurée au seul LCA
de candidats proches : **ne rattacher un point que lorsqu'une branche porte
strictement plus de la moitié de sa masse de témoins**. Contrairement à
l'argmax indépendant, cette règle possède une preuve directe d'emboîtement.
Il faut garder les dates des témoins et un dénominateur fixé une fois.

### Construction et preuve

Pour chaque point x, fixer des témoins de poids positifs et finis `w_i`, attachés
chacun à une branche `v_i` de l'arbre continu et une date `t_i` où x est
effectivement couvert par elle. Après cette date, le témoin suit uniquement
les ancêtres. Poser `W_x = Σ_i w_i > 0`, **sur tous les témoins fixés**,
y compris ceux dont la date n'est pas encore atteinte. Les témoins inactifs
ne votent pas ; ils restent présents dans le dénominateur. Tous suivent un
**même arbre à fusions seulement**, à K fixé ou sur une tranche monotone
explicitement déclarée ; mélanger des K incomparables invalide ce cadre.

À une coupe r, `M_x(C,r)` est la somme des poids des témoins actifs de x
dont la branche appartient à C à cette coupe. Avec `1/2 ≤ θ < 1`, inclure
x dans C seulement si `M_x(C,r) > θ W_x`. Sinon, x reste son propre
singleton de complétion, pas membre d'un bloc collectif nommé « bruit ».

1. **Exclusivité.** Deux composantes distinctes ne peuvent satisfaire cette
   inégalité : leurs masses sont disjointes et leur somme est au plus W_x,
   tandis que deux majorités strictes dépasseraient W_x.
2. **Ascendance.** Si C devient C′ à un rayon supérieur, ses témoins actifs
   restent actifs et vont tous dans C′. De nouveaux témoins peuvent s'ajouter ;
   `M_x(C′,r′) ≥ M_x(C,r)`. Une majorité acquise ne se perd donc jamais au
   profit d'une branche concurrente.
3. **Partitions emboîtées.** Deux points déjà réunis dans C restent réunis
   dans C′. Les points encore seuls peuvent rejoindre une composante, mais
   aucun bloc existant ne se scinde. Un propriétaire unique et sa première
   date suffisent ensuite à exporter l'arbre de points par ascendance.
4. **Fidélité géométrique conditionnelle.** Un propriétaire a au moins un
   témoin actif qui couvre x ; l'ascendance et la croissance du rayon
   préservent cette couverture. Aucun test demandant que x soit core
   n'apparaît. Sur deux sites à K2, un témoin unique les réunit dès d/2.

Au seuil θ=1/2, une égalité exacte ne donne pas de majorité ; θ<1/2 permet
deux propriétaires simultanés, θ≥1 peut empêcher toute entrée. Si chaque
point a au moins un témoin et tous leurs ancrages rejoignent une racine
finie, la masse de celle-ci atteint W_x : tous les points finissent réunis.
Les contacts sont traités par la convention fermée et les plateaux exacts
de FULL, pas par un ordre arbitraire entre événements simultanés.

**Stabilité conditionnelle.** Si deux arbres possèdent des transports
cohérents de composantes, dans les deux sens avec décalage δ, et si ces
transports préservent chaque témoin actif, son ID de point et son poids,
alors une majorité dans C est aussi une majorité dans son image. L'unicité
impose le même propriétaire transporté. Les relations d'équivalence des
partitions s'incluent ainsi à décalage δ dans les deux sens : les hauteurs
de fusion des points diffèrent d'au plus δ. Cette preuve ne demande pas
une marge supplémentaire lorsque poids et univers sont **identiques**.
Elle ne se transfère pas automatiquement aux témoins sélectionnés et aux
poids ψ(r) recalculés après perturbation : ces hypothèses difficiles ne
découlent pas de la seule exactitude ou stabilité de FULL.

### Ce qui différencie cette piste de la thèse et de la bande actuelle

- Les `Sτ/Tx` de la thèse sont des masses fixes sur des faces. Leur somme
  vers les ancêtres est l'ingrédient réutilisable ; un argmax à chaque coupe
  n'est pas remplacé silencieusement par cette nouvelle règle majoritaire.
- Si les atomes sont des boules du catalogue, une boule couvrante apporte
  un seul témoin par incidence point→boule, avec sa composante à K et sa
  date. Ce ne sont **pas** les faces/cofaces du chapitre 9 : il faut nommer
  ce nouveau choix de poids et ne pas prétendre reproduire ses scores Sτ.
- L'univers et les poids doivent être définis **à K fixé**, indépendamment
  de la profondeur Kmax demandée au générateur. Sinon, une tête K2 peut
  changer quand on demande aussi K3 et K4, sans changement de son arbre FULL.
  Le paragraphe ci-dessous donne un contre-exemple natif et une correction.
- Plusieurs témoins d'une même composante se renforcent au lieu de créer
  une ambiguïté artificielle. La concurrence porte sur les branches, pas
  sur le nombre de supports canoniques.
- La règle ne force pas tous les points à attendre d_k(x), mais peut tout
  de même retarder beaucoup de frontière si sa masse se disperse. Le rappel
  avant fusion parasite, le nombre de singletons et leurs dates restent
  les diagnostics décisifs. Rien ne prouve encore qu'elle batte `cover`
  ou l'ancrage par bande sur les nuages demandés.

Un dénominateur recalculé sur les seuls témoins actifs casse la preuve :
une masse peut tomber sous le seuil quand de nouveaux témoins apparaissent.
Le [petit oracle Fraction](../../receipts/audit_continu_20260929/majority_lamination/check_majority.py)
rejette ce mutant. Sur l'exemple A/B/C de la section 7, x attend A+B,
tandis que y entre tôt dans C ; ils ne sont pas d'abord réunis pour se
séparer ensuite. C'est une propriété de construction, pas une vérité terrain.

### Coût et réserves avant toute implémentation

Le script est volontairement exhaustif sur de petits arbres : ses scans
ne sont **pas** une architecture à porter. Noter D le nombre d'incidences
témoins-point effectivement retenues et H le nombre de nœuds de FULL.
W_x se prépare en O(D), puis les événements peuvent agréger des cartes
par composante et point. Une fusion de petites cartes vers de grandes
évite le scan de tous les n points à chaque nœud. Pour une borne simple,
choisir le sens de fusion par le **nombre d'atomes activés représentés**,
pas par la somme de leurs poids. Chaque transfert de clé se charge à un
atome qu'elle représente ; son nombre d'atomes de composante double quand
il migre. Il y a donc O(D log D) opérations de carte attendues avec une
table de hachage, plus traitement des H événements, tri éventuel des dates,
calcul des poids et sortie. Une carte ordonnée ajoute un facteur logarithmique.
Cette analyse compte des opérations, pas des nanosecondes ou des bits
d'arithmétique exacte. Les suppressions éventuelles ne doivent pas réduire
le compteur logique qui justifie le doublement.

Les seules entrées dont la masse change doivent être réévaluées ; un
propriétaire déjà acquis suit son ancêtre sans rescanner tous ses points.
Cette architecture reste à implémenter et mesurer. D peut être quadratique
si l'on recrée toutes les paires ou toutes les faces. Aucun O(n log n),
gain mémoire, port massivement parallèle ou chrono n'est acquis ici.

Les poids réels, le seuil et les additions ont aussi un contrat numérique
propre. ψ=1/r peut exiger des comparaisons de sommes irrationnelles ; un
double non certifié ne prouve pas une majorité stricte. Un choix rationnel
ou une décision par intervalles conservateurs est envisageable, mais doit
être déclaré et testé, y compris aux égalités. La validité de FULL ne
certifie pas automatiquement cette nouvelle tête. Les témoins de rayon
nul, notamment K1, exigent une masse finie spéciale, pas ψ(0)=∞.

La [capture abstraite](../../receipts/audit_continu_20260929/majority_lamination/receipt.json)
exerce 180 cas, 4 254 transitions de coupes et 106 350 vérifications de
paires, aux seuils 1/2, 3/5 et 9/10, en normal et `−O`. Poids tardifs,
atomes aux nœuds internes, arbres en peigne et plateaux sont inclus. Elle
ne qualifie aucun score statistique, algorithme natif ou chrono G4. À
explorer seulement par un petit bras dev mesurant frontière et coût,
sans vaste chantier avant un signal utile.

### Ne pas faire dépendre la tête K de Kmax

Le lemme de couverture permet un univers de témoins propre à K : pour les
boules positives, retenir `population ≥ K` et `p+q_min ≤ K`, avec toutes
leurs incidences intérieur/coquille. Ajouter les témoins sites de rayon nul
lorsqu'ils sont nécessaires. Pour un catalogue critique **complet et canonique**,
cet univers est le même pour chaque Kmax≥K et retrouve encore toutes les
composantes couvrantes. Fixer également une fonction de poids indépendante
de Kmax et des rangs/indices natifs, qui peuvent changer lorsque le catalogue
s'élargit. Le filtre moins fort `p+q_min≤K+1`, fixé à K et non à Kmax,
supprime aussi cette dépendance. Le filtre renforcé propose moins d'atomes
avec la même complétude de couverture ; le lemme ne démontre pas qu'il soit
statistiquement meilleur. La majorité est ensuite calculée sur le FULL K intégral ; **les
événements `p+q_min=K+1` restent indispensables à sa construction**.

Le [petit test sur export natif](../../receipts/audit_continu_20260929/majority_lamination/native_kmax_r2/check_kmax.py)
reprend six sites collinéaires 0,…,5, K2, θ=1/2 et des masses unitaires.
L'arbre FULL K2 a dans les deux cas cinq naissances à β=1/4 puis leur
multifusion à β=1 ; sa signature géométrique est identique, sans comparaison
naïve des indices ou des rangs de tableaux. β désigne ici le **rayon carré**.

| Univers de témoins | Kmax | W(x0), W(x2) | Première fusion des points x0/x2 |
| --- | ---: | ---: | ---: |
| Toutes les boules couvrantes du catalogue | 2 | 2, 5 | β=1 |
| Toutes les boules couvrantes du catalogue | 4 | 4, 10 | β=9/4 |
| Témoins propres à K2, `p+q_min≤2` | 2 | 1, 2 | β=1 |
| Témoins propres à K2, `p+q_min≤2` | 4 | 1, 2 | β=1 |

Les boules ajoutées avec Kmax4 retardent artificiellement la majorité :
à β=1, le point x2 ne porte que 5/10 de sa masse dans la racine, au lieu
de 5/5. Cette dépendance est évitable ; le test compare aussi l'identité
complète des témoins filtrés. Huit appels natifs, W1/W2, normal/`−O`, et
720 contrôles d'emboîtement passent, dans la
[capture close distincte](../../receipts/audit_continu_20260929/majority_lamination/native_kmax_r2/closure.json).
La sonde et son archive sont celles du [raccord natif R2](../../receipts/audit_continu_20260929/math_catalogue_cover/native_r2/build.json).
La tête majoritaire reste un calcul d'audit Python sur ces exports, **pas
une tête implémentée dans le moteur**, ni une qualification de performance.

La conservation de masse de la thèse doit aussi rester visible dans les
benchmarks : condenser après une projection dure peut supprimer ou retarder
des contributions frontière que la condensation fractionnaire conservait.
Comparer ces deux unités de masse, déclarer le traitement des singletons et
mesurer le rappel frontière avant fusion parasite. Ni la preuve d'emboîtement
ni l'indépendance de Kmax ne démontrent une optimalité statistique.

## 9. Contre-épreuve géométrique : ne pas porter les masses uniformes par défaut

La piste de la section 8 reste mathématiquement laminaire. Mais sa version
à poids uniformes échoue déjà sur deux paires très bien séparées : **le bon
arbre de composantes ne suffit pas si la masse d'un témoin tardif retarde
une observation frontière**. C'est un défaut de cette proposition de tête
d'audit, pas un défaut du moteur FULL ni du vote plat du chapitre 9.

Exemple exact K2 : quatre sites collinéaires `0, 1, 100, 101`, θ=1/2,
témoins propres à K `p+q_min≤2`. Les trois témoins sont les boules de
diamètres `[0,1]`, `[1,100]`, `[100,101]`, de rayons carrés respectifs
`1/4, 9801/4, 1/4`. Le FULL comporte les deux composantes locales dès
β=1/4, puis une composante-pont à β=9801/4 ; elles fusionnent à β=2500.
Les observations 1 et 100 ont chacune deux atomes ; le premier n'apporte
que la moitié de leur masse uniforme totale. Elles restent donc seules,
alors que 0 et 101 ont déjà un propriétaire. **Aucun des deux groupes de
deux points n'existe dans la partition projetée avant la fusion**, même
à β=1 où les quatre observations sont déjà core. Une condensation dure
avec min_cluster_size=2 ne peut sauver ces deux branches précoces absentes.

Avec les mêmes atomes, mêmes dates et même dénominateur fixe, le choix
`w=1/β=1/r²` donne une majorité précoce : pour le point 1, son témoin
local pèse 4 contre `4/9801` pour le témoin-pont. La partition comporte
les deux paires dès β=1/4, sans perdre l'emboîtement. Sur cet exemple
seulement, `1/r` a la même décision : il suffit de comparer les rayons
de deux atomes ; aucune somme d'irrationnels générale n'est jugée ici.
Le choix de poids d'incidence est **distinct de l'exposant utilisé par
EOM** : cette expérience ne compare aucun z de sélection, ne réimplémente
pas Sτ du chapitre 9 et ne justifie pas de favoriser un concurrent.

La [contre-épreuve close](../../receipts/audit_continu_20260929/majority_boundary_geometry/README.md)
reprend quatre séparations `L=4,8,32,100` et deux familles :

- `(0,0,0),(1,0,0),(L,0,0),(L+1,0,0)` ;
- `(0,0,0),(1,0,0),(L,1,1),(L+1,1,2)`, tétraèdre de volume non nul.

Sur les huit fixtures, l'univers contient exactement trois boules, avec
incidences par point 1/2/2/1. La géométrie des atomes et la couverture des
deux composantes précoces sont contre-jugées en Fraction par le catalogue
et Γ2 indépendants. **32 exports natifs** (W1/W2, Python normal/−O),
**2 496 contrôles d'emboîtement de paires** et les décisions exactes des
deux modes passent. Chaque famille perd ses deux groupes avec les masses
uniformes et les récupère avec 1/β avant la première fusion FULL.
Les exports réutilisent l'archive `6206d1d11` déjà épinglée, sans rebuild.
Ce n'est ni le binaire intégré des correctifs actuels, ni un benchmark
statistique, ni une mesure d'ARI/EOM/temps, ni une tête native.
Le raisonnement, les atomes, les dates Γ2 et le script/JSON ont aussi été
contre-relus indépendamment ; le défaut de l'argmax réévalué par coupe ne
réfute pas cette majorité fixe, qui conserve sa preuve d'emboîtement.
La seconde famille a volume 1/6 et des naissances locales distinctes
(β=1/4 et 1/2) : l'échec ne dépend ni d'un volume nul ni d'un ex æquo
géométrique. **Au contact local, chaque observation n'a qu'une seule
composante réellement couvrante** ; la majorité uniforme la retarde à
cause d'un témoin futur, pas d'une ambiguïté déjà présente.

**Action limitée utile au développeur.** Garder ce contre-exemple en gate
de conception avant de porter la majorité. Comparer quelques petits bras
dev, dont masses décroissantes avec l'échelle et affectation précoce figée ;
publier le rappel frontière avant première fusion et la masse différée.
L'attache immédiate lorsqu'une seule composante couvre effectivement le
point est ainsi un contrôle simple à conserver ; les véritables conflits
entre branches doivent être jugés séparément. Son maintien par ascendance
garantit la laminarité, pas une stabilité statistique universelle.
Ne pas passer le dénominateur aux seuls atomes actifs pour masquer l'échec :
ce changement rouvre la non-laminarité. Éviter un grand chantier de cartes
ou d'optimisation tant que le choix de masse n'a pas un signal statistique
utile sur les nuages 3D demandés.

## 10. Un poids décroissant ne suffit pas : discontinuité des incidences

Le succès de `1/β` en section 9 ne justifie pas son choix par défaut. Une
nouvelle contre-épreuve montre un saut d'affectation lorsque des contacts
de coquille deviennent intérieurs, **sans changement de la fusion FULL
concernée**. Chaque arbre projeté reste laminaire ; c'est la robustesse
entre deux nuages voisins qui échoue.

K2, θ=1/2, tétraèdre de dimension affine 3 :

`C=(0,0,0), A=(4M,0,0), B=(0,5M,0), D=(0,0,100M)`.

Avant perturbation, les six diamètres sont des témoins forts
`population≥2, p+q_min≤2`. C appartient aux trois supports CA/CB/CD et aux
coquilles de AB/AD/BD. Leur normalisation fixe empêche C de rejoindre A
dès la naissance de CA : sa masse locale reste sous la moitié, avec une
marge de −1,72 %, pas une égalité de majorité. C/A attendent la fusion ABC
à `β=41M²/4`.

Déplacer seulement C à `(1,1,1)`. Il devient strictement intérieur à
AB/AD/BD. Ces boules ont maintenant `p=1,q_min=2` : elles sortent de
l'univers **fort de couverture K2**, mais restent au catalogue et à FULL
avec `p+q_min=K+1`. La majorité locale CA dépasse alors la moitié avec
une marge d'environ +21,8 %. C/A sont réunis dès
`β=((4M−1)²+2)/4`. Γ exact confirme que la fusion ABC reste au même
`β=41M²/4`. Ne pas retirer ces fusions du moteur pour masquer le défaut.

À M=2048, tout est dans u18. Sur la grille 1 mm, déplacer C de 1,732 mm
change le rayon de première réunion des points C/A de 6,557 m à 4,096 m,
soit 2,461 m. Sur la grille finie, c'est une amplification mesurée ; dans
le modèle réel normalisé par M, le déplacement tend vers zéro et le saut
ne disparaît pas. Ces deux affirmations ne sont pas interchangeables.

La [capture principale](../../receipts/audit_continu_20260929/inverse_beta_shell/README.md)
contient quatre M, huit nuages, normal/−O identiques et 576 contrôles de
non-scission par mode. Un second auditeur a rejoué le paquet et vérifié
44 égalités/inégalités analytiques supplémentaires, dont la marge, les
poids et les puissances. Les deux lectures géométriques partagent la
primitive MEB Fraction figée : aucun oracle arithmétique indépendant ni
nouvel export natif n'est revendiqué. Aucun moteur changé, GCP0.

### Quelle unité de masse explorer maintenant ?

L'obstacle est la normalisation par **des objets de catalogue** dont les
incidences changent brutalement, pas seulement le choix uniforme des poids.
Une masse sur toutes les K-parties étiquetées déplacerait le coût vers un
univers combinatoire : ne pas l'implémenter. Avec des poids uniformes, une
composante couvrant m points porterait au plus
`binom(m−1,K−1)/binom(n−1,K−1)` de la masse d'un point. Pour une majorité
K5, il faudrait asymptotiquement plus de 84 % du nuage : ce ne serait pas
un modèle convenable pour de petites communautés.

Le [contrôle secondaire](../../receipts/audit_continu_20260929/inverse_beta_shell/duration_leaf_control/README.md)
teste la **durée réellement couverte des feuilles de naissance** :
`w_xv=φ(β_firstcover(x,v))−φ(β_death(v))`, avec φ=1/r et φ=1/β.
Les majorités irrationnelles de φ=1/r sont certifiées par intervalles
dyadiques, pas par double. Les deux tétraèdres ci-dessus et les deux
paires de la section 9 sont récupérés dès leur naissance, pour les deux
φ, normal/−O. Ce sont huit contrôles de conception, pas une solution.

L'univers limité aux feuilles peut manquer des points couverts seulement
par une branche interne à K3/K5 ; la section 11 corrige cette réserve pour
K2 et donne les contre-exemples exacts aux ordres supérieurs.
Une branche fantôme peut aussi couper une branche
longue : ignorer tous les ancêtres ferait perdre sa continuation. Ajouter
tous les ancêtres au dénominateur peut au contraire retarder à nouveau la
frontière. Il faut définir une unité de branche et une mesure conservée,
sans doubler les continuations ni exclure brutalement un ancêtre.

**Conseil au développeur.** Garder l'attache précoce à une composante
réellement unique, dédupliquée, puis ascendance figée comme contrôle simple.
Elle évite le biais des témoins futurs sur ces cas ; sa stabilité exige
une marge contre les couvertures concurrentes. Comparer la durée couverte
comme diagnostic fractionnaire de la thèse, avant condensation. Ajouter
ce contact coquille/intérieur aux perturbations des bras annoncés, avec
rappel frontière et masse différée. Ni les quatre petits succès ni la
preuve d'emboîtement ne justifient un grand port, une promesse ARI/EOM ou
le contrat 100 ms.

## 11. Ne pas perdre la frontière portée par les branches internes

La relecture de la définition 8 impose une appartenance par couverture,
pas par densité propre du point. Cette exigence concerne **toutes les
branches de FULL**, pas seulement ses feuilles. Nous avons contre-vérifié
notre réserve de la section 10 au lieu de la laisser comme intuition.
Les [preuves et traces](../../receipts/audit_continu_20260929/duration_branch_reserves_20260930/README.md)
séparent l'argument général K2, les contre-exemples K3/K5 et le diagnostic
de continuation sous perturbation.

### K2 : chaque point possède une véritable incidence de feuille

Pour des sites distincts, choisir un plus proche voisin a de x, à distance
d>0. Si z appartient à la boule fermée de diamètre xa, alors
`|z−x|² ≤ (a−x)·(z−x) ≤ d|z−x|`. Tout z autre que x et a y serait donc
strictement plus proche de x que a ; l'égalité à distance d impose z=a.
La boule est vide d'autres sites, y compris sur sa coquille.

Le sommet `{x,a}` de Γ2 naît à β=d²/4. Toute 3-partie le contenant a un
rayon MEB strictement supérieur : la seule boule de rayon d/2 contenant
x et a est celle de leur diamètre. Ce sommet naît donc isolé et crée une
feuille de durée positive couvrant x. **Aucun point n'est donc dépourvu
d'incidence de feuille à K2.** Ce lemme garantit un univers non vide par
point, pas la récupération de toutes ses futures composantes couvrantes
internes, ni la robustesse statistique des poids ou de la majorité.

### K3 et K5 : des entrées frontière strictement internes

À K3, prendre les six sites, dans cet ordre :

`x=(15,4,0), a=(5,4,0), b=(7,8,0), c=(7,0,0), e=(1,4,0), f=(0,4,1)`.

Ils ont une dimension affine trois. Les quatre feuilles naissent à
β=13/2,13,13,16 ; toutes meurent au plus tard à β=169/9. Pourtant,
x n'est couvert qu'à **β=25**, sur une branche interne déjà née.
L'unique boule critique forte couvrant x a p=0, q_min=2 et la coquille
`{x,a,b,c}`. À K3, c'est un `join`, pas une naissance.

À K5, prendre les sept sites :

`x=(325,325,650), (520,325,65), (200,325,25), (325,416,13),`
`(325,130,65), (442,481,65), (250,225,25)`.

Ils sont tous sur la sphère de centre (325,325,325), rayon 325, et de
dimension affine trois. Trois feuilles sur les six sites inférieurs
fusionnent à β=116715625/3409. **x n'entre qu'à β=105625**, dans cette
branche interne. Aucune feuille ne couvre x durant sa vie, même si on
autorise une feuille à acquérir des points après sa naissance. L'unique
boule critique forte couvrant x est la sphère commune, p=0, q_min=3,
coquille de sept sites ; son rôle à K5 est encore un `join`.

Les traces Γ exhaustives traitent toutes les K-parties et K+1-parties,
et chaque plateau en un seul événement. Elles examinent les couvertures
durant toute la vie des feuilles, pas seulement à leur naissance.
Le filtrage fort sert aux témoins de couverture, **jamais à supprimer
les fusions de FULL**. Les deux petits exports natifs et leur comparaison
indépendante sur 414 coupes sont séparés des calculs Fraction dans le reçu.
Ils utilisent l'archive historique `6206d1d11`, pas le moteur R2 courant.
Le seul wrapper d'audit est recompilé, avec ses dépendances réelles et
reproduction bit à bit ; aucun moteur reconstruit ni transfert de qualification.

### La persistance doit conserver les continuations

Dans le second exemple, multiplier les coordonnées non translatées par
N=64,128,256, translater de (325N,325N,325N), puis déplacer seulement x
d'une unité vers le bas. Tout reste dans u18. De petits arcs apparaissent
près de la sphère commune et segmentent l'ancienne branche interne longue.
Leur persistance totale en φ=1/β tend vers zéro dans le modèle réel
normalisé, mais la continuation ancestrale de l'ancien arc porte une masse
qui tend vers **1/105625**, donc non nulle.

Ignorer cette continuation ferait perdre une masse finie pour une
perturbation qui tend vers zéro. À l'inverse, sommer correctement les
segments d'une même continuation télescope leurs persistances : **cela
ne réfute pas toute pondération par durée**. Le diagnostic vise l'exclusion
des ancêtres et le double comptage des arcs, pas une tête complète déjà
implémentée. Les trois tailles u18 sont des mesures d'amplification ; la
limite continue relève du modèle réel, pas de la grille finie.

### Conséquence pour la conception

Conserver les entrées frontière sur les branches internes et une mesure
cohérente lors de leur continuation. Ne pas prendre « une feuille = une
unité de masse » comme réduction générale K5. Ne pas remplacer non plus
chaque continuation par une nouvelle masse indépendante à chaque fusion.
Le contrôle simple reste une attache précoce à la composante réellement
unique, suivie par ascendance ; les conflits doivent être traités à part.

Avant un port coûteux, la porte frontière doit réunir ces K3/K5, le contact
coquille/intérieur de la section 10, les deux paires précoces de la section 9
et les métriques de récupération avant connexion parasite. Elle devra
ensuite mesurer condensation et EOM équitables z1/z2 sur les bras dev.
Ces preuves de structure ne qualifient aucun ARI, EOM, coût sous-quadratique
global ni contrat FULL/G4 de 100 ms. Aucun moteur modifié, GCP0.
