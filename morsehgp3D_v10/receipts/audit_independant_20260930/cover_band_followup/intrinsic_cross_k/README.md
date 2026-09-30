# Croisement inter-K et choix d’une hiérarchie de points

30 septembre 2026. Calcul autonome Fraction, normal et −O, 800 contrôles
sur 19 coupes du même nuage. Aucun moteur ou contexte développeur importé.
Les 20 coupes et l’export natif du paquet voisin sont une preuve distincte.
Aucun profil large, campagne, performance, ARI ou EOM qualifié.

## Cinq sites suffisent

Sites d’IDs0..4 aux abscisses0,1,2,6,9, autres coordonnées nulles.
Bande η=1/8, coupe fermée r=3, donc β=9 :

| Projection | Blocs de points |
| --- | --- |
| K2 | {0,1,2}, {3,4} |
| K3 | {0,1,2,3}, {4} |
| Raffinement commun | {0,1,2}, {3}, {4} |
| Coarsening commun | {0,1,2,3,4} |

{3,4} croise{0,1,2,3}. À r=3, L2 a les composantes de centres
[−2,5] et[6,9], L3 a la seule composante[−1,4]. Son image verticale
est la première composante L2, mais le point x=6 est déjà affecté à la
seconde par sa première couverture. Les objets FULL et leurs verticales
sont corrects ; les propriétaires exclusifs indépendants ne commutent pas
avec cette verticale. Le point x=6 n’a pas d’ex æquo à sa première entrée.

En dimension1 à sites distincts, une boule positive forte propre à K est
l’intervalle de K sites consécutifs : p=K−2,q_min=m=2. Son rayon est la
moitié de l’étendue ; les K+1 sites consécutifs donnent les fusions FULL.
Le script calcule ces attaches puis les confronte aux composantes Γ
exhaustives de toutes les K-parties, liées par les K+1-parties actives.
Les dates d’entrée en rayon sont1/2,1,1/2,3/2,3/2 à K2 et1,1,1,5/2,7/2
à K3. Chaque ordre demeure laminaire en rayon.

## Deux constructions possibles, deux effets différents

Soit P_K(r) la partition exclusive à K fixé, singletons avant admission.
À K fini fixé, le raffinement commun est donné par les intersections de
blocs, ou les signatures de labels. Quand r augmente, chaque P_K se
coarsen : ses signatures se coarsen aussi. Cela donne bien une hiérarchie
laminaire. Le prix est un veto de chaque ordre : ici x=6 et x=9 restent
singletons, bien que chacun soit déjà admis à K2. Un grand K peut ainsi
retarder les petits objets ou les parties peu denses recherchées.

Le coarsening commun est la fermeture transitive des liens appartenant
à au moins une partition. Ses coupes sont également emboîtées. Ici il
fusionne tous les points à β=9 alors que les deux composantes géométriques
K2 restent distinctes jusqu’à β=49/4. Cette fusion est un choix de projection.

Inclure K1 fait perdre toute la restriction de densité à cette construction.
Preuve générale : un bloc couvert à ordre K≥1 se projette dans une seule
composante L1(r), puisque L_K(r)⊆L1(r). Chaque point affecté est relié à son
centre témoin par le segment contenu dans la boule de rayon r autour de
ce point, donc dans L1(r). Les singletons différés raffinent également P1.
Ainsi P_K(r) raffine P1(r), et le coarsening commun incluant P1 vaut P1.
Le script vérifie les deux constructions et cette identité sur les19coupes.

Ces opérations prouvent l’existence de hiérarchies communes ; elles ne
choisissent pas celle qui convient statistiquement. Il faut déclarer
l’ordre utilisé, une chaîne monotone de paramètres avec transport des
propriétaires, ou une règle de combinaison, puis comparer rappel frontière,
masse différée et condensation. Recalculer des attaches indépendantes à
chaque ordre ne produit pas à lui seul la hiérarchie globale demandée.

## Retard par un témoin ancêtre : piste de comparaison

Le test unitaire développeur contient un premier témoin au niveau5 sur
node2 et un témoin au niveau12 sur son ancêtre node3, né au niveau9.
Le LCA retarde l’entrée à9 sans lignée incomparable supplémentaire.
Cela respecte la définition actuelle du bras ; un nœud unary artificiel
ne constitue pas une erreur native FULL démontrée.

Une variante à comparer garde l’antichaîne des nœuds sélectionnés minimaux
pour l’ascendance, puis leur LCA. Sous le protocole des témoins vivants à
leur propre date, un témoin plus tardif ne peut être descendant d’un nœud
antérieur déjà vivant : ce descendant serait déjà mort à cette date.
Les témoins de première couverture restent donc retenus. Agrandir η
n’ajoute que des lignées incomparables ou des ancêtres redondants, d’où
la même couverture, laminarité en rayon et monotonie avec η. La variante
éviterait ce retard particulier ; elle change le bras, demande une porte
et une comparaison, et ne supprime pas notre croisement inter-K.
