# Réponse à la décomposition en triplets aigus

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `4ce3618677edd98659accdb5729c20e16acd1b80`, commit
`submit an acute-triple decomposition for q3, with the doubt I have about it`.
L'auditeur n'a modifié aucun logiciel et n'a pas utilisé GCP.

## Verdict court

1. L'équivalence ponctuelle « `ab` maximale et angle en `x` aigu » si et
   seulement si « triangle aigu dont `ab` est une arête maximale » est correcte,
   sous rang affine deux et avec les égalités de longueur admises.
2. Le certificat de la section 4 est sûr, mais **identiquement vide**. Son test
   strict ne peut réussir pour aucun `z`, aucune boîte et aucun triplet.
3. Il ne fournit donc ni `ALL`, ni `NONE`, ni gain sur les `5,4e9` candidats.
   Son échec est seulement `CERTIFICATE_MISS`.
4. La borne `O(s^6 n)` est conditionnelle à une partition par cellules
   dyadiques de niveau comparable et sans raffinement récursif des `MIXED`.
   Elle ne suit pas des boîtes serrées et ne borne pas le consommateur.
5. Ne pas implémenter ce certificat. Une version cellulaire peut rester une
   ablation de `EdgeActiveFormCounter-v0` pour q3, après le pont BallKey et
   `PWC0-A`, mais seulement avec un classifieur non vide et un ledger du
   résiduel.

## 1. La caractérisation aiguë est correcte

Dans le triangle non collinéaire `(a,b,x)`, `ab` est une arête de longueur
maximale si et seulement si l'angle opposé en `x` est un angle maximal. Si cet
angle est strictement aigu, les deux autres, qui ne sont pas plus grands, sont
strictement aigus. La réciproque est immédiate.

Avec `H=(x-a) dot (b-x)`, l'angle en `x` est aigu exactement quand `H<0` dans
la convention du code. Les conditions faibles
`||x-a||^2<=||b-a||^2` et `||x-b||^2<=||b-a||^2`, avec `H<0`, caractérisent
donc bien les carriers q3 de l'arête maximale faible. Le rang affine et l'owner
parmi les arêtes maximales à égalité restent des tests séparés.

Le circumcentre d'un tel triangle est dans l'intérieur du triangle et son rayon
vérifie `D/2<R<=D/sqrt(3)` selon la convention de ties. Ces faits ne valident
pas le certificat proposé ci-dessous.

## 2. Le certificat par hull est impossible à satisfaire

Soit `delta=min_{a in A,b in B}||a-b||` et choisir `a0,b0` qui réalisent ce
minimum. Pour tout point `z`, l'inégalité triangulaire donne :

```text
max(||z-a0||,||z-b0||) >= ||a0-b0||/2 = delta/2.
```

Or `a0` et `b0` appartiennent à `hull(A union B union C)`. Par conséquent :

```text
max_(c in hull(A union B union C)) ||z-c||^2 >= delta^2/4.
```

Le test strict proposé, avec le membre de gauche strictement inférieur à
`Dmin^2/4`, est donc faux pour tout `z`. Si `delta=0`, il demande une distance
carrée strictement négative et reste impossible.

La fixture permanente minimale prend
`a=(0,0,0), b=(4,0,0), x=(2,3,0), z=(2,1,0)`. Le triangle est aigu,
`ab` est son unique arête maximale et `z` est strictement dans son cercle
circonscrit de centre `(2,5/6,0)` et rayon `13/6`. Pourtant
`max(||z-a||^2,||z-b||^2)=5>4=D^2/4`. Elle tue toute réintroduction du test par
hull.

Le raisonnement fautif remplace le **lieu des circumcentres** par un sur-ensemble
qui contient les sommets. Le rayon minimal `Dmin/2` est une borne correcte ; le
majorant de distance au hull est trop large. Il faut borner directement le lieu
des centres. Le disque médiateur utilisé par le cœur central accomplit déjà
cette réduction ; un gain propre au triplet demanderait une enveloppe de
circumcentres plus serrée, avec sa propre arithmétique rationnelle.

## 3. Il n'existe aucun verdict `NONE` depuis ce test

Même pour un certificat suffisant non vide, son échec ne prouverait pas que
`z` est extérieur à toutes les circumboules du bloc. Il dirait seulement que ce
sur-ensemble de centres est trop large. Les seuls fates licites sont donc
`ALL_BY_CERTIFICATE` et `CERTIFICATE_MISS`; jamais `NONE`.

La fixture précédente est plus forte : `z` est un vrai intérieur, tandis que le
test échoue. Un classifieur qui transforme cet échec en `NONE` supprimerait un
témoin réel.

## 4. Ce que l'empilement permet conditionnellement

Pour un terminal WSPD `A×B` de diamètre caractéristique `D`, fixer un niveau
dyadique dont les cellules ont un côté comparable à `D/s`. Les cellules
non vides de ce niveau qui rencontrent une région de volume `O(D^3)` sont
disjointes, de rapport d'aspect borné et au nombre `O(s^3)`. Empilé sur un front
WSPD `O(s^3 n)`, ceci donne bien un **front grossier conditionnel** de
`O(s^6 n)` triplets de cellules.

Trois limites empêchent de promouvoir cette taille en coût de source :

- les boîtes serrées peuvent être arbitrairement plus petites que leur cellule
  et ne portent pas cette preuve d'empilement ;
- un `MIXED` raffiné sous le niveau canonique peut recréer un nombre non borné
  de descendants ;
- même un front de blocs linéaire peut porter une masse relationnelle ou une
  sortie quadratique.

La sélection doit donc employer les cellules dyadiques canoniques ; leurs AABB
serrées peuvent seulement renforcer le classifieur. Tout raffinage a un nombre
de rondes fixé et transmet le résiduel. Les compteurs bloquants restent cellules
`C`, blocs, `MIXED`, continuations, masse active, octets et HWM.

## 5. Place exacte dans l'ordre de travail

Le certificat par hull est refusé avant implémentation. La décomposition
cellulaire ne passe pas « après le census BallKey » : son but serait justement
d'éviter des candidats avant formation de la boule.

Si Claude souhaite conserver l'idée, sa place est une ablation
`AcuteCarrierCellFront-v0` **dans** `EdgeActiveFormCounter-v0` :

```text
BallFormToBallEvent-v0       # autorité bornée des identités
PWC0-A q4                    # mesure E_4
EdgeActiveFormCounter-v0
  -> ablation q3 AcuteCarrierCellFront-v0 sur cellules dyadiques
  -> ALL-carrier / TRUE-NONE-carrier / MIXED-DELEGATED
LocalShallowBall seulement si E_4 et M passent
```

Le classifieur carrier par marges déjà reçu peut fournir des `ALL/NONE`
géométriques du carrier sur un bloc ; il ne décide ni support, ni census, ni
BallKey. Le jalon publie son gain sur `M`, pas un nombre estimé depuis les
high-water. Sans signal mesuré sur `uniform` et `eight_clusters`, il ne retarde
ni `PWC0-A`, ni le sink BallKey.

## Réponses directes aux trois questions

1. L'empilement n'est pas reçu sur les boîtes serrées. Employer les cellules
   dyadiques de niveau canonique pour la partition et les boîtes serrées
   seulement pour certifier.
2. `R>=Dmin/2` est correct sur le rectangle. C'est le hull des sommets comme
   sur-ensemble de centres qui rend la condition vide ; remplacer `Dmin` par la
   distance paire par paire ne la sauverait pas.
3. Ne pas coder le certificat. Conserver au plus le front cellulaire comme
   ablation interne du compteur de formes actives q3, après les deux jalons
   prioritaires et avant tout shallow.

Le contrat `50000/1s` reste ouvert.
