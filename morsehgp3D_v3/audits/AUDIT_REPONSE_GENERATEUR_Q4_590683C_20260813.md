# Réponse au pin `590683c` — un même tape de formes, trois requêtes, un vrai shallow q4

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `590683c75844fac953df9e3eba9d54e69f0d40c7`, commit
`propose the arrangement property that would generate q4 from q2 and q3`.
La proposition est documentaire ; aucun nouveau générateur n'est implémenté.
Au relevé, le worktree ne contenait que les compléments documentaires de
l'auditeur dans les chemins autorisés. Aucun code n'a été modifié par
l'auditeur et GCP n'a pas été utilisé.

## Verdict et réponses directes

Claude a identifié **exactement le plan médiateur qui fonde
`LocalShallowBall`**. La formule est correcte et la bonne conclusion est plus
précise que « engendrer q4 depuis q2 et q3 » :

- q2, q3 et q4 réutilisent le même **tape de formes affines par arête** ;
- q2 est une requête au point `t=0` ;
- un q3 propre est une requête au point de norme minimale de la droite de son
  troisième site, pas toute cette droite ;
- un q4 propre est un sommet défini par deux droites non parallèles ;
- les lanes restent sémantiquement indépendantes. Un q4 peut exister sans face
  q3 pertinente et ne se construit jamais depuis les **sorties** q2/q3.

Réponses aux trois questions :

1. Oui, la borne linéaire en `m` à profondeur fixée survit aux
   dégénérescences, si elle compte des **centres géométriques distincts** à
   profondeur stricte, si les droites confondues deviennent des bundles
   pondérés et si les concurrences sont traitées atomiquement. Elle ne borne ni
   les paires de lignes, ni les `SupportKey` portés par une cosphère lourde.
2. Oui, c'est le `LocalShallowBall` demandé. Il ne faut surtout pas matérialiser
   l'arrangement complet : on streame ses bas niveaux `P-P/N-N/P-N`.
3. q2 reste une requête ponctuelle presque gratuite sur le tape complet. Il
   partage les coefficients et le futur `BallKey`, mais pas son fate, son
   budget, son domaine de Jung ou sa preuve de complétude avec q3/q4.

Le front de triplets aigus devient donc inutile comme architecture q4. Son bit
d'acuité reste utile sur chaque forme pour imposer qu'au moins une des deux
faces incidentes soit aiguë.

## 1. Identité exacte et trois strates différentes

Fixer `a,b`, poser `d=b-a`, `m=(a+b)/2`, `D2=||d||^2` et écrire le centre
`c=m+t` avec `t dot d=0`. Pour un site `z`, poser `u=z-m` et
`H_z=D2/4-||u||^2`. Sa puissance par rapport à la sphère passant par `a,b` est
exactement :

```text
power_z(t) = ||z-(m+t)||^2 - (D2/4+||t||^2)
           = -H_z - 2*t dot u.
```

Le site est strictement intérieur si et seulement si
`F_z(t)=H_z+2*t dot u>0`. Dans une base entière de `d^perp`, après multiplication
par un dénominateur positif commun, `F_z` est une forme affine entière. Aucun
flottant n'est requis.

Trois cas doivent rester distincts :

- si la projection de `u` sur `d^perp` est nulle, `F_z` est constante. Elle
  contribue à `always_inside`, disparaît comme `always_outside`, ou appartient
  au shell permanent à égalité ;
- sinon `F_z=0` est une droite. Le centre q3 auto-centré du triangle `abz` est
  le point de cette droite le plus proche de `t=0`. La droite entière décrit le
  pinceau de sphères par le triangle, pas des sorties q3 ;
- l'intersection de deux droites non parallèles est l'unique centre de la
  sphère par `a,b,x,y`. Parallèles distinctes ne donnent aucun q4 propre ;
  droites confondues et concurrences demandent les branches de dégénérescence.

Le niveau au point ou au sommet est le nombre de formes strictement positives,
après exclusion de **toutes** les formes incidentes du shell. Ajouter les
témoins `always_inside` donne le vrai budget local. Un census final sur le
nuage entier reste obligatoire si le tape a omis des formes uniquement pour
proposer un superset.

## 2. Deux corrections au domaine q4 annoncé

La note écrit `R<=D/sqrt(2)` puis en déduit un disque de rayon
`D/(2sqrt(2))`; ces deux valeurs ne correspondent pas. Pour un tétraèdre
positif dont `ab` est une arête maximale de longueur `D`, Jung donne :

```text
R^2 <= 3*D^2/8
||t||^2 = R^2-D^2/4 <= D^2/8.
```

Le disque du paramètre a donc bien le rayon `D/(2sqrt(2))`, mais la borne du
rayon de sphère est `R<=sqrt(3/8)*D`, pas `D/sqrt(2)`. Pour q3, le disque plus
petit vérifie `||t||^2<=D^2/12`.

Ce disque n'encode pas à lui seul l'owner ou la positivité. Un candidat q4 doit
encore vérifier :

- `x` et `y` dans la lentille fermée de `ab` ;
- `||x-y||^2<=D2` ;
- indépendance affine et positivité du tétraèdre ;
- owner de l'arête maximale avec son tie-break exact ;
- rang, `BallKey`, census et shell.

`GenerationRank` oriente l'énumération des paires mais n'est pas une contrainte
géométrique sur `t`. Le disque de Jung est un domaine nécessaire de génération,
jamais une caractérisation complète du support.

## 3. Portée exacte de la borne shallow dégénérée

Soit `m` le nombre de **copies** de formes actives après les constantes, et
`c` le nombre de témoins constamment intérieurs. Pour q4,
`k=smax-4-c`, donc `k=7-c` lorsque `smax=11`.

Pour les centres géométriques distincts définis par au moins deux directions
non parallèles et de profondeur stricte au plus `k`, l'argument de
Clarkson--Shor déjà reçu donne :

```text
|V_<=0| <= m
|V_<=k| < e*(k+1)*m                 pour k>=1
incidences_<=k < 2*e*(k+1)*m.
```

La position générale n'est pas nécessaire. Pour chaque centre, choisir une
paire canonique de directions incidentes non parallèles ; l'échantillonnage ne
conserve aucun des au plus `k` conflits strictement positifs. Les droites
confondues sont préalablement normalisées en bundles pondérés
`(mu_plus,mu_minus)` et toutes les directions incidentes d'une concurrence sont
retirées du rang avant décision.

Ce théorème ne compte pas les paires de copies incidentes. Une concurrence de
`r` bundles peut porter `Theta(r^2)` supports distincts au même `BallKey`.
Cette masse `H` est une vraie sortie, une branche de plateau ou un refus de
domaine ; elle n'est jamais cachée sous la borne sur `V`. Les couples
cross-bundle réellement traités `J` restent eux aussi un compteur bloquant.

Le facteur `500` de la note n'est donc pas reçu. Même en supposant
`m=7811,k=7`, la borne explicite ci-dessus vaut environ `170 000` centres, pas
`62 000`, avant `J`, `H`, owner et census. Elle reste une réduction
structurelle majeure face à `C(m,2)`, mais seul un constructeur et ses
compteurs donnent un temps.

## 4. Ordonnance exacte : niveaux, pas arrangement complet

Choisir un chart entier du plan médiateur et une cisaille unimodulaire qui rend
les vraies lignes non verticales. Séparer les formes :

- `P` : le côté positif est au-dessus de la ligne ;
- `N` : le côté positif est au-dessous.

Hors shell, la profondeur restreinte est la somme du rang inférieur dans `P`
et du rang supérieur dans `N`. Construire seulement les niveaux `0..k` de ces
deux familles. Les événements complets sont :

1. les sommets `P-P` d'un niveau inférieur `r`, lorsque le rang opposé dans
   `N` ne dépasse pas `k-r` ;
2. les sommets `N-N` symétriques ;
3. les intersections des **segments actifs** `P-N` de rangs `r,s` avec
   `r+s<=k`.

À `k=7`, cela demande seize curseurs de niveaux et trente-six canaux de rang
`P-N` simultanés. Ce ne sont ni seize chaînes résidentes, ni trente-six
intersections au total. Tout événement de même centre rationnel est groupé ;
les bundles incidents sont exclus du rang strict avant le replay.

Sous le constructeur de niveaux pondérés encore à recevoir, la cible par arête
est :

```text
O(m*k^2 + m*alpha(m)*log(m) + |V| + J + H),
```

plus le census et le payload aval. La borne de cardinal ne reçoit pas à elle
seule cette complexité. Les opérations de segments, comparaisons larges,
propositions dupliquées, octets et HWM restent des gates.

## 5. Ce qui est réellement partagé entre q2, q3 et q4

L'objet commun est un `LineFormTape` sous `EdgeKey/CloudDigest/Epoch`, pas une
dépendance entre sorties :

```text
LineFormTape(a,b, all witness sites)
  -> q2: profondeur stricte au point t=0
  -> q3: point de norme minimale de chaque droite carrier dans le disque q3
  -> q4: sommets shallow dans le disque q4, avec bit acute incident
  -> BallKey/RLE -> census global -> lane/owner/fold.
```

Les classifications constantes, disques de Jung, budgets `k`, carriers et
fates diffèrent par lane. Le tape peut partager coefficients, bundles, digests
et comparateurs ; il ne partage jamais un compteur saturé ou un verdict sans
masque. Un q2 fermé n'autorise pas à supprimer une arête q3/q4, et l'absence
d'une sortie q3 n'autorise pas à supprimer ses droites du générateur q4.

Le bit `acute(z)` est une propriété du carrier relatif à `ab`. Un candidat q4
n'est proposé que si au moins un bundle incident porte ce bit. Il faut conserver
les cas à une seule face aiguë ; exiger deux bits perd des supports.

## 6. Le code existant est un comparateur différentiel, pas l'autorité ni le chemin produit

`prototype/edge_shallow.hpp` contient déjà la forme entière, les disques de
Jung, le point q2, les points q3 et les sommets q4. Il constitue un comparateur
différentiel utile, mais il partage des structures et des primitives avec le
sujet et n'est donc pas une autorité indépendante. Il reste impropre au
contrat :

- il lance toutes les `C(n,2)` arêtes ;
- il charge tous les points pour chaque arête ;
- son q4 trie les croisements de chaque droite et examine les paires ;
- il rescane tous les points pour chaque support ;
- il matérialise un catalogue et déduplique après génération.

Le nouveau moteur ne doit pas être une translittération GPU de ce fichier.
Celui-ci compare différentiellement les ensembles
`(BallKey,SupportKey,I_B,U_B,owner)` du producteur de niveaux. L'autorité bornée
reste à recevoir dans un juge rationnel séparé qui reconstruit ces cinq objets
sans réutiliser les bundles, extrema ou clés du sujet.

## 7. Micro-jalon concret remis à Claude

Deux travaux peuvent avancer sans confusion :

1. **Intégration produit** : recevoir `EdgeWindowRangeAdd-v0`, puis
   `CanonicalEdgeWindowReporter-q4-v0` et `EdgeActiveFormCounter-v0`. Ils
   empêchent de construire un tape par PairId ou de scanner tous les sites par
   arête.
2. **Oracle algorithmique borné** : implémenter
   `LocalShallowLevelsOracle-v0` pour une seule arête explicite et petit `m`,
   avec niveaux `P-P/N-N/P-N`, puis le comparer à la double boucle de
   `edge_shallow.hpp`. Ce vert reçoit la preuve/ordonnance, pas sa parcimonie
   globale.

L'ABI minimale porte
`LineBundle(normalized_form,plus_ids_offset,plus_ids_count,minus_ids_offset,minus_ids_count)`,
deux listes exactes de `LineMember(PointId,acute,provenance)`, `LevelSegment`,
`CenterEvent(center_key,incident_bundles,strict_depth)` et
`BallEvent(BallKey,lane_mask)`. Des multiplicités `mu_plus/mu_minus` seules ou
un intervalle de rang ne suffisent pas : les IDs géométriquement confondus ne
sont pas nécessairement contigus et le shell/owner exige chaque membre. Les
gates minimales sont :

- q2 à `t=0`, q3 au pied de droite et q4 au sommet comparés séparément ;
- `P-P`, `N-N`, `P-N`, niveaux `0` et `k` tous non vides ;
- une seule face aiguë, distance `xy` refusée et owner à égalité ;
- forme constante intérieure/extérieure, parallèle distincte, bundle confondu
  avec orientations opposées et concurrence d'au moins trois bundles ;
- rang strict excluant tout le shell incident, égalité au cercle de Jung et
  `always_inside>0` ;
- cosphère lourde : un `BallKey`, incidences complètes, aucun préfixe de
  `SupportKey` ;
- parité au catalogue borné sous permutation des points, tuilage et ordre des
  événements.

Après ce vert, l'intégration produit ne commence que si `E_4`,
`M=sum_e m_e`, tâches, `J/H`, octets et HWM passent leurs caps. La version
device streame les curseurs ou emploie une shallow cutting certifiée ; elle ne
matérialise jamais l'arrangement complet.

La réponse synthétique est donc : **oui au même objet géométrique, non à une
dépendance q2 -> q3 -> q4**. La propriété trouvée donne un bon moteur local
conditionnel pour supprimer `C(m_ab,2)` après admission d'une arête ; elle ne
borne ni `|E_4|`, ni `M=sum m_ab`, ni `J/H`. Elle n'entre dans le hot path
qu'après ces portes, et construit alors les niveaux shallow plutôt que toutes
les intersections.

GCP non utilisé par l'auditeur.
