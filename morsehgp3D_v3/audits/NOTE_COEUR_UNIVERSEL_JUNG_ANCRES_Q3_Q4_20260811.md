# Note mathématique — cœur universel de Jung pour les ancres q3/q4

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le certificat ci-dessous est exact. Il donne, pour une paire qui est réellement
une arête de longueur maximale d'un support q3 ou q4 positif, un sous-ensemble
universel de points strictement intérieurs à toutes les sphères admissibles
ancrées par cette paire. Neuf témoins pour q3, ou huit pour q4, certifient alors
l'inertie du seul quotient horizontal jusqu'à `K=10`.

Le lemme complémentaire de profondeur fermée de demi-boule peut supprimer
d'autres paires sans hypothèse de diamètre. Le cœur universel et la profondeur
sont deux certificats suffisants lorsqu'ils mordent et ils sont incomparables.
Ils ne sont pas les seuls prunes exacts : le center-cover par 64 patches est
une troisième voie déjà documentée.

Ce résultat ferme une question mathématique, pas la porte industrielle : il ne
prouve ni que les ancres survivantes sont peu nombreuses, ni qu'on peut trouver
les témoins en temps sous-quadratique, ni que la source q3/q4 est complète. Il
ne concerne ni Gamma exhaustif ni les verticales.

## Provenance : reformulation, pas nouveau filtre

Les prédicats polynomiaux ci-dessous sont exactement les filtres P0 de
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md)
§6.1 pour `JungChordCsrTile`. Dans cette autorité, `B=||U||^2-D^2=-g` et
`G=||d cross U||^2=Q`; les conditions `B<0`, `4G-3B^2<0` et
`2G-B^2<0` sont donc les mêmes tests. La présente note fournit une preuve
compacte par minimisation sur le disque et précise leur emploi comme prune
d'ancres H0. Elle ne revendique ni découverte nouvelle, ni qualification de
performance.

Le recouvrement exact de §5.5 est plus général : sa banque de témoins dépend
du patch de centres. Il ne doit pas être décrit comme une liste fixe de
témoins universels par paire.

## Hypothèses obligatoires

Soient deux points distincts `a,b`, avec `d=b-a`, milieu `m` et
`D^2=d dot d>0`. La paire `ab` doit être une arête du support considéré et sa
longueur doit être maximale parmi toutes les arêtes de ce support. Les ex æquo
de diamètre demandent un ownership canonique exact.

Le support q3 ou q4 est affinement indépendant et propre positif : le centre
appartient à l'intérieur relatif de son enveloppe convexe. Les supports
collinéaires, coplanaires en q4, redondants ou non positifs prennent une autre
branche fail-open.

La simple appartenance individuelle des futurs points du support à une lentille
ne suffit pas à prouver que `ab` est une arête maximale. La contre-fixture
permanente reste
[`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md).

## Disque de centres

Tout centre admissible s'écrit `c=m+t`, avec `t dot d=0`. Comme `a` et `b` sont
sur la sphère, son rayon vérifie :

$$r^2=\frac{D^2}{4}+\lVert t\rVert^2.$$

Le théorème de Jung, appliqué avec l'arête diamètre certifiée, donne les bornes
suivantes :

$$\text{q3: }\lVert t\rVert^2\leq\frac{D^2}{12},\qquad\text{q4: }\lVert t\rVert^2\leq\frac{D^2}{8}.$$

Ces disques sont des sur-ensembles des centres réellement réalisables. Un
certificat valide sur tout le disque est donc valide pour tout support
admissible; l'inverse n'est pas requis.

## Certificat polynomial exact

Pour un point témoin `w`, poser `U=2w-a-b`, puis :

$$g=D^2-\lVert U\rVert^2,\qquad Q=D^2\lVert U\rVert^2-(U\mathbin{\cdot}d)^2=\lVert d\mathbin{\times}U\rVert^2.$$

Le point `w` est strictement intérieur à la sphère de centre `m+t` si et
seulement si :

$$g+4U\mathbin{\cdot}t>0.$$

Sur le disque `t dot d=0`, `||t||<=H`, le minimum du membre gauche vaut :

$$g-4H\sqrt{\lVert U\rVert^2-\frac{(U\mathbin{\cdot}d)^2}{D^2}}.$$

Il est donc strictement positif sur tout le disque lorsque les prédicats
entiers suivants sont satisfaits :

$$\text{q3: }g>0\ \text{ et }\ 3g^2>4Q,$$

$$\text{q4: }g>0\ \text{ et }\ g^2>2Q.$$

La garde `g>0` doit être évaluée avant la mise au carré. Toute égalité est
conservée : elle peut désigner un point de coquille et ne compte jamais comme
témoin strict.

### Preuve

Écrire `w-m=U/2`. Après annulation de `||t||^2` entre la distance au centre et
le rayon, l'inégalité stricte intérieure devient exactement
`g+4 U dot t>0`. La minimisation d'une forme linéaire sur le disque orthogonal
à `d` donne le terme négatif ci-dessus. Enfin,
`||Proj U||^2=Q/D^2`; substituer `H^2=D^2/12` ou `H^2=D^2/8`, puis mettre au
carré sous la garde `g>0`, donne respectivement `3g^2>4Q` et `g^2>2Q`.

## Seuils d'inertie à K=10

Les témoins sont des `PointId` distincts. Ils doivent être distincts des
extrémités et, lors d'un certificat de bloc, leur disjonction des plages des
extrémités doit être rejouable.

- q3 : neuf témoins universels donnent `p>=9`, donc `p+q>=12`;
- q4 : huit témoins universels donnent `p>=8`, donc `p+q>=12`.

Le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
autorise alors une tombstone du quotient `hgp_reduced_normalized_h0_v3` pour
les ordres au plus 10. Il n'autorise jamais à annoncer l'absence d'un support,
d'une incidence Gamma, d'une grande coquille ou d'une application verticale.

## Arithmétique u16

Le test ne comporte aucune division. Pour le profil u16, `D^2`, `||U||^2`,
`U dot d`, `g`, `Q` et les deux côtés des comparaisons tiennent largement dans
un entier signé de 128 bits; les produits les plus larges restent sous environ
`2^74`. Il faut promouvoir avant `2*w` et avant tout produit.

Les deux tests doivent rester des prédicats stricts, avec une branche fail-open
sur tout domaine ABI non reçu.

## Deux niveaux de prune par blocs

Le spindle universel contient une boule centrée au milieu de la paire. Cela
donne deux tests plus faibles mais très bon marché :

$$\text{q3: }3\lVert U\rVert^2<D^2,$$

$$\text{q4: }15\lVert U\rVert^2\leq4D^2.$$

Le second utilise le rayon rationnel sûr `D/sqrt(15)`, strictement inférieur à
`D sin(15 degrés)`; son égalité reste donc strictement dans le vrai spindle.
Pour un produit de boîtes d'extrémités et une boîte témoin,
on peut borner par intervalles `max ||U||^2` et `min D^2`. Si l'une des
inégalités reste stricte avec ces bornes, tous les identifiants de la boîte
témoin sont universels pour toutes les paires du produit. Sinon on descend;
aucune paire n'est supprimée.

Aux blocs terminaux, le prédicat polynomial complet est plus fort. Dans les
deux cas, le reçu de prune conserve les deux plages d'extrémités et neuf ou
huit identifiants ou plages témoins disjoints, afin qu'un juge indépendant
rejoue le certificat.

## Conséquence de couverture

Supposons qu'un self-join canonique partitionne toutes les paires non ordonnées
et ne supprime un bloc q3 ou q4 qu'avec le certificat universel correspondant.
Tout support non inerte possède une arête diamètre canonique. Cette ancre ne
peut appartenir à un bloc supprimé, car les neuf ou huit témoins certifieraient
alors son inertie. Les ancres émises forment donc un sur-ensemble complet des
ancres des supports non inertes.

Cette conséquence est plus forte que la lane q2 résiduelle : une paire prunée
par dix témoins dans sa seule boule diamétrale peut encore ancrer une sphère q3
ou q4 décalée. Les certificats q3/q4 ci-dessus portent sur tout le disque de
centres et autorisent ce prune supérieur sans construire la sphère.

## Complément exact : profondeur fermée de demi-boule

Pour une paire distincte `a,b`, poser `m=(a+b)/2` et
`P={z:(z-a) dot (z-b)<0}`. Pour chaque direction non nulle `nu` du plan
médiateur, définir :

$$h(\nu)=\left\lvert\left\lbrace z\in P:(z-m)\mathbin{\cdot}\nu\geq0\right\rbrace\right\rvert,\qquad\delta(a,b)=\min_{\nu}h(\nu).$$

Toute sphère passant par `a,b` contient strictement au moins `delta(a,b)`
points. En effet, son centre s'écrit `m+t`. Si `t` est non nul, chaque point de
`P` dans le demi-plan fermé orienté par `t` reste strictement intérieur; si
`t=0`, tous les points de `P` sont intérieurs. La statistique q2 est donc le
total `|P|`, jamais `delta`; q3/q4 peuvent employer `delta`.

Les seuils `delta>=9` pour q3 et `delta>=8` pour q4 certifient l'inertie de
toute sphère de coquille contenant la paire. Cette preuve n'exige ni que la
paire soit diamètre ni une borne de Jung. Elle est complémentaire du cœur :
des témoins répartis autour de l'axe peuvent donner une grande profondeur sans
qu'aucun soit universel; des témoins proches du milieu peuvent rendre le cœur
immédiat.

Les résiduels des lanes ne sont pas emboîtés. La fixture q2 contre q3 possède
dix intérieurs diamétraux d'un seul côté : q2 supprime et `delta=0` conserve
q3. Inversement, neuf `PointId` distincts sur le segment ouvert `(a,b)` donnent
`|P|=9` et `delta=9` : q2 conserve et q3 supprime.

Le calcul ponctuel exact de `delta` est un balayage angulaire. Les projections
nulles sont créditées dans une banque `always`; rayons confondus, antipodes et
demi-plans fermés doivent être traités exactement. Le relèvement sectoriel à
un produit AABB n'est pas encore prouvé et reste fail-open. Cette profondeur
est donc d'abord un filtre terminal, après le cœur universel par blocs.

## Contacts exacts à graver

La fixture q3 est le triangle équilatéral propre
`a=(10,10,10)`, `b=(13,13,10)`, `z=(13,10,13)`, de centre
`(12,11,11)` et rayon carré 6. Le point extra-shell `w=(11,12,9)` satisfait
simultanément `3g^2=4Q` et `3||U||^2=D^2`. Remplacer une inégalité stricte par
une large classerait donc un point de coquille comme intérieur.

La fixture q4 est le tétraèdre régulier propre
`a=(13,13,13)`, `b=(13,7,7)`, `z=(7,13,7)`, `t=(7,7,13)`, de centre
`(10,10,10)` et rayon carré 27. Le point extra-shell `w=(15,11,11)` satisfait
`g^2=2Q`. Là encore, l'égalité doit rester fail-open.

## Coût : ce qui reste à prouver

Le pire cas demeure potentiellement quadratique en paires et cubique si chaque
paire rescane le nuage. Le certificat est suffisant et fail-open; il ne fournit
aucune borne déterministe sur le nombre d'ancres survivantes ou sur la taille
des frontières de témoins.

Sous un processus de Poisson homogène stationnaire sans bord en dimension
trois, le seul test de boule inscrite donne, pour un point typique, un degré
dirigé moyen `t/kappa^3`. Pour q3, cela vaut environ `374,1`. Pour q4, le cœur
exact `kappa=sin(15 degrés)` donne environ `461,4`, tandis que le test rationnel
effectivement proposé avec `kappa=1/sqrt(15)` donne
`8*15^(3/2)=464,76`. Ce sont des calculs de modèle infini, pas des bornes
produit : bords, surfaces, LiDAR, amas et entrées adversariales peuvent
conserver beaucoup plus de paires. Ils justifient une sonde falsifiable,
jamais une extrapolation à 50 k.

## Porte d'implémentation proposée

1. Implémenter d'abord un falsificateur count-only sans catalogue global de
   paires : self-join canonique, test de boule par blocs, test polynomial puis
   profondeur fermée aux terminaux, et sortie des seules ancres survivantes.
2. Sur `n<=32`, tenir un sort par paire et rejouer chaque certificat; comparer
   ensuite toutes les ancres de supports q3/q4 non inertes de l'oracle
   exhaustif au sur-ensemble émis.
3. Tuer les mutants `>` vers `>=`, seuil `9/8` diminué d'un, paire non maximale,
   witness dupliqué, plage recouvrant une extrémité, ancre ex æquo mal possédée,
   support non positif et dernier bloc omis; ajouter pour la profondeur zéro,
   rayons confondus, antipodes, permutation et demi-plan ouvert à la place du
   fermé.
4. Publier par famille les états, visites, certificats boule/spindle, ancres
   survivantes, octets et high-water à `12 500/25 000/50 000` points. Le seuil
   de deux exposants consécutifs supérieurs à `1,35` est une règle de gate
   opérationnelle choisie pour falsifier tôt la route; ce n'est pas un
   théorème de complexité.
5. Une seule machine de blocs peut partager l'arbre et la partition des paires,
   mais q2, q3 et q4 gardent trois sorts et trois ledgers indépendants. Mesurer
   d'abord `cœur seul`, `profondeur seule` et `combiné` avec compteurs séparés;
   un résiduel d'une lane ne sert jamais d'univers à une autre.
6. Ne porter sur G4 que si les masses passent, puis mesurer le pipeline complet
   avec q3/q4 shallow, census, resolver, fold et payload.

GCP non utilisé.
