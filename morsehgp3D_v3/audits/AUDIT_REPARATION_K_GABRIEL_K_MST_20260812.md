# Audit mathématique — réparation du K-graphe de Gabriel et du K-MST

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Références : proposition 6 et théorème 5, pages imprimées 90--91 de
[`MANUSCRIT_THESE_HAUSEUX.pdf`](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
la preuve de référence
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
et la fixture permanente
[`gabriel_point_set_counterexample.json`](../../tests/fixtures/regressions/gabriel_point_set_counterexample.json).

Le `HEAD` observé au début de cet audit est
`154c107cbccffcc1129b8ed9adf2860f95b693c1`. Le résultat ci-dessous est un
audit du théorème et ne reçoit aucun état logiciel.

## Verdict

Oui, une retouche locale conserve l'essentiel de la conclusion du manuscrit :
il suffit de remplacer le K-graphe de Gabriel brut par le **K-graphe de
Gabriel complété par les incidences silencieuses**. Pour chaque coface
non-Gabriel, ce graphe ajoute une étoile de `|I|` arêtes, et non la clique
complète. Sous la porte régulière du manuscrit, il possède exactement les
mêmes composantes de facettes que Gamma à toute coupe ouverte ou fermée. Une
forêt couvrante minimale de ce graphe complété donne alors le théorème 5
corrigé.

La modification est légère mathématiquement mais pas industriellement : sa
construction littérale doit encore examiner toutes les cofaces. Elle doit
servir d'oracle exact et de preuve de référence. La route sparse pertinente
est une seconde réduction, conditionnelle, fondée sur les cofaces directes,
tous les co-minimiseurs de première incidence requis et, dans la seule branche
à au moins deux intrus réguliers, une attache résolue par facette du cœur. Elle
préserve le quotient horizontal normalisé, pas le transcript Gamma complet.

| variante | statut exact | intérêt produit |
| --- | --- | --- |
| K-graphe de Gabriel brut | faux en général | proposition ou connectivité positive seulement |
| Gabriel complété `G_k^+` | théorème exact sous la porte régulière globale | oracle; exhaustif en cofaces |
| directes + premières incidences + carriers | théorème conditionnel pour le seul H0 horizontal normalisé | meilleur candidat sparse actuel |

## 1. Où casse exactement la proposition 6

Le théorème 4 du manuscrit reste utile : une coface qui fusionne deux
composantes antérieures doit être de Gabriel. L'erreur intervient à l'étape
suivante. « Ne produit aucune fusion dans son propre lot » n'implique pas
« peut être retirée de toute la filtration ».

Une coface non-Gabriel peut rattacher au lot une facette née au même niveau,
sans modifier l'union de points de la composante. Une coface Gabriel plus
tardive peut réutiliser cette facette. Si l'incidence antérieure a été jetée,
la fusion est retardée artificiellement.

La fixture E5 le réalise en position générale, à l'ordre deux :

- `ACD` et `ACE`, non-Gabriel au niveau carré `33/2`, rattachent la facette
  `AC` à la composante contenant `AD`, `AE`, `CD`, `CE`, `DE` ;
- leur effet immédiat est une continuation sans nouveau `PointId` ;
- `ABC`, Gabriel au niveau carré `83886/3563`, réutilise ensuite `AC` ;
- Gamma possède alors une composante sur les cinq points, tandis que le graphe
  brut conserve deux composantes jusqu'au niveau `24`.

L'induction de la proposition 6 mémorise les unions de points, mais oublie la
facette `AC`. Le théorème 5 applique ensuite correctement le fait standard sur
les MST à un graphe qui n'a pas les bonnes composantes : sa seconde flèche est
donc invalide, pas le fait MST lui-même.

## 2. Porte régulière utilisée par la réparation

Fixons `1<=k<n`. Une facette contient `k` labels et une coface `Q` en contient
`k+1`. On note `beta(S)` le rayon carré de la miniboule de `S` et
`a=beta(Q)`.

La réparation littérale suppose, pour chaque coface non-Gabriel pertinente :

1. un support minimal `U(Q)` unique et essentiel ;
2. tout label de `I(Q)=Q\U(Q)` strictement intérieur à la miniboule ;
3. aucun label extérieur à `Q` exactement sur sa frontière ;
4. au moins un intrus `z` extérieur à `Q` strictement intérieur.

La Définition 26 globale du manuscrit suffit à cette porte. Le contrat
`RelevantGP` v3 est plus faible : il quantifie les activations utiles/proper et
ne ferme pas, à lui seul, les extra-shells de toutes les facettes silencieuses
avec plusieurs intrus. Un reçu `relevant_gp_complete` isolé ne suffit donc pas;
les égalités extérieures et supports non uniques des objets visités exigent des
certificats locaux séparés ou une autorité globale plus forte. Hors de cette
porte, la compression ci-dessous n'est pas admise : il faut quotienter le
plateau par une preuve dédiée, conserver la coface Gamma entière, ou retourner
`unsupported_degeneracy`.

## 3. Lemme local : l'étoile silencieuse

Pour `u` dans `U(Q)`, la facette `F_u=Q\{u}` est stricte :

$$\beta(F_u)<a.$$

Pour `x` dans `I(Q)`, la facette `E_x=Q\{x}` contient encore tout le support et
naît simultanément :

$$\beta(E_x)=a.$$

Les facettes strictes sont déjà toutes dans une même composante avant `a`.
En effet, pour un intrus strict `z` et deux éléments `u,v` du support, les
cofaces obtenues en remplaçant `u` ou `v` par `z` ont un niveau strictement
inférieur à `a` et partagent la facette
`(Q\{u,v}) union {z}`. Elles donnent un chemin Gamma entre `F_u` et `F_v`.

Choisir alors un `u_0` canonique dans `U(Q)` et ajouter uniquement

$$F_{u_0}\longleftrightarrow E_x\quad\text{au poids }a\quad\text{pour chaque }x\in I(Q).$$

Cette étoile induit exactement la même équivalence que l'hyperarête complète
de `Q` : ses facettes strictes sont déjà ensemble et chaque facette
simultanée est rattachée une fois. Si `I(Q)` est vide, aucune arête n'est
nécessaire.

## 4. Proposition 6 corrigée

Définir `G_k^+` ainsi :

- ses sommets sont les mêmes facettes pondérées que dans Gamma, avec naissance
  à `beta(F)` ;
- une coface Gabriel conserve son hyperarête, ou toute étoile canonique qui
  relie ses `k+1` facettes au poids `beta(Q)` ;
- une coface non-Gabriel émet seulement l'étoile silencieuse de la section 3.

**Proposition 6+.** Sous la porte de la section 2 pour toutes les cofaces
non-Gabriel, `G_k^+` et Gamma ont les mêmes composantes de facettes à toute
coupe ouverte ou fermée. Ils ont donc aussi les mêmes K-polyèdres lorsque
ceux-ci sont l'union des labels de chaque composante non triviale.

**Preuve.** Procéder par induction sur les niveaux exacts. Les facettes
pondérées et les hyperarêtes Gabriel coïncident. Avant un lot `a`, l'hypothèse
d'induction place les facettes strictes de chaque coface non-Gabriel dans la
même composante des deux objets. Son étoile silencieuse rattache alors toutes
ses facettes simultanées et induit la même relation que son hyperarête Gamma.
Prendre l'union de toutes ces relations puis contracter le lot en une seule
opération rend la conclusion indépendante de l'ordre d'énumération. Les
coupes ouvertes suivent de l'état pré-lot et les coupes fermées de l'état
post-lot. Fin de preuve.

Cette conclusion facette par facette est plus forte que l'invariant par unions
de points utilisé dans la preuve originale.

## 5. Théorème 5 corrigé

Développer les hyperarêtes Gabriel de `G_k^+` en étoiles canoniques de même
poids et prendre une forêt couvrante minimale `T_k^+`. Une MST convient si le
graphe global est connecté.

**Théorème 5+.** Pour tout seuil `a`, les composantes non triviales de
`T_k^+` restreinte aux arêtes de poids `<a`, respectivement `<=a`, sont celles
de Gamma à la même coupe, sous la bijection de la proposition 6+.

Le fait standard est exact : dans tout graphe pondéré, une forêt couvrante
minimale préserve les composantes de chaque sous-graphe de seuil. La
proposition 6+ fournit précisément la prémisse qui manquait au manuscrit.

Deux précisions empêchent un nouveau sur-claim :

- filtrer aussi les sommets par leur poids de naissance `beta(F)` ; un MSF
  d'arêtes ne sérialise pas à lui seul la naissance des facettes isolées ;
- un MSF suffit à reproduire les partitions et les niveaux horizontaux, pas le
  payload exhaustif des facettes, cofaces, lots Gamma ou applications
  verticales.

À `k=1`, pour des sites géométriques distincts, la réparation n'ajoute aucune
attache : une paire possède ses deux extrémités comme support et `I(Q)` est
vide. On retrouve le théorème classique
Gabriel--EMST/single linkage, qui dispose en outre d'une preuve indépendante
plus générale pour des sites distincts. Le cas terminal `k=n>1` n'a aucune
coface : sa naissance isolée est explicite dans `full_pi0`, tandis que
`hgp_reduced` garde seulement son carrier latent et publie une forêt vide.

## 6. Pourquoi ce correctif n'est pas encore industriel

La complétion littérale doit décider Gabriel/non-Gabriel et construire le
support de chacune des `C(n,k+1)` cofaces. Elle compresse seulement l'incidence
d'une coface connue : au plus `|I(Q)|<=k-1` attaches silencieuses au lieu d'une
clique. Elle ne réduit pas l'univers de recherche.

Il serait donc faux de renommer cette construction « source sparse 50 k ».
Elle est utile comme :

- oracle borné de `G_k^+` contre Gamma ;
- preuve exacte de ce qu'une source sparse doit reproduire ;
- juge des lots égaux, des naissances de facettes et des carriers ;
- source d'une fixture permanente E5.

Si le graphe brut est conservé sans attache, le seul affaiblissement universel
est unilatéral : sur ses facettes, chaque composante Gabriel est incluse dans
une composante Gamma. Son niveau de connexion éventuel est une borne
supérieure du vrai niveau. E5 interdit toute réciproque, toute bijection et
toute sortie HGP exacte fondée sur ce seul fait.

Il existe aussi une caractérisation exacte, mais non constructive, de
l'hypothèse qui sauverait l'égalité **forte des partitions de facettes** : pour
chaque coface non-Gabriel `Q`, toutes ses facettes doivent déjà appartenir à
une même composante du graphe Gabriel brut dans le lot `beta(Q)`, atomicité
comprise. La condition est nécessaire et suffisante pour cette égalité forte,
car elle dit exactement que chaque hyperarête omise est redondante à son
seuil. Elle est suffisante, mais pas nécessaire, pour la seule égalité plus
faible des collections d'unions de points affirmée par la proposition 6 : une
incidence peut modifier une partition de facettes sans jamais modifier une
union de labels. Le critère nécessaire et suffisant de cette version faible
est global et n'offre aucun test local coface par coface plus simple que le
quotient à comparer.

Sous la porte régulière, une condition géométrique suffisante plus lisible est
`I(Q)=empty` pour toute coface non-Gabriel : aucune facette simultanée n'est
alors créée et les facettes strictes sont reliées par la descente du lemme.
Cette condition est automatique à `k=1`, mais elle est prohibitive aux ordres
supérieurs. En dimension trois, un support essentiel possède au plus quatre
points; dès que `k+1>4`, elle exclut toute coface non-Gabriel de cette taille.
Ce n'est donc pas une hypothèse produit crédible.

## 7. Réparation sparse : premières incidences et gateways du cœur

La réduction industriellement pertinente ne cherche pas toutes les cofaces
non-Gabriel. Elle part d'une source complète de cofaces directes/Gabriel et
déduplique leurs facettes, appelées ici **facettes du cœur**. Cette notation
évite la collision entre le cœur parfois noté `D_k` et le champ continu top-k
également noté `D_k(y)` dans les preuves existantes.

Pour une facette du cœur `F`, soit `B_F` sa miniboule, `U_F` son support et

$$J_F=(B_F^{\circ}\cap X)\setminus F.$$

Sous la même porte régulière, ses premières incidences se classent exactement
ainsi :

| cas | première incidence | action sparse exacte |
| --- | --- | --- |
| `|J_F|=0` | minimum des cofaces directes incidentes à `F` | conserver tous les co-minimiseurs exacts et leur lot atomique |
| `|J_F|=1` | `F union {z}` au niveau `beta(F)` | cette coface est directe et suffit |
| `|J_F|>=2` | toutes les `F union {z}`, `z` dans `J_F`, au niveau `beta(F)` | une seule attache canonique vers leur composante stricte commune suffit pour H0 |

Dans le troisième cas, choisir deux intrus distincts canoniques `z_F,w_F` et
un support essentiel canonique `u_F`, puis former

$$T_F=(F\setminus\lbrace u_F\rbrace)\cup\lbrace z_F\rbrace.$$

On a `beta(T_F)<beta(F)`. Le second intrus prouve que `F union {z_F}` est
non-Gabriel; le lemme des remplacements place toutes les premières incidences
dans le même apex strict. Après résolution de `T_F` dans le snapshot strict,
une unique arête `carrier(T_F)--F` au poids `beta(F)` remplace donc tous les
co-minimiseurs silencieux pour la partition horizontale du cœur.

Ce lemme local est exact. Sa composition globale reste conditionnelle à :

- une source directe complète et terminale ;
- une autorité de régularité couvrant aussi les cofaces omises pertinentes,
  ou une preuve séparée de leur inertie de haut rang ;
- des requêtes top-k et de census fermées, sans préfixe publié comme minimum ;
- un resolver strict complet, y compris lorsque `T_F` n'est pas une facette du
  cœur ;
- tous les co-minimiseurs dans la branche `|J_F|=0` ;
- un quotient atomique de chaque niveau exact et des carriers latents.

Sous ces hypothèses, le théorème de rétraction existant justifie un **MSF de
carriers** ou, plus directement, un fold DSU du stream
`directes + gateways`. Il ne faut pas appeler cet objet « K-MST de Gabriel
brut » : il préserve le H0 horizontal normalisé, pas les identités ni le
payload Gamma.

Ce H0 normalisé est un système persistant indexé de classes de carriers
non triviales, décoré par leurs couvertures en `PointId`. Ces couvertures ne
forment pas nécessairement une partition du nuage. Le payload minimal est un
journal de niveaux, racines antérieures, `q_R`, parents et deltas de couverture,
pas une copie des facettes à chaque coupe. Les identités de facettes restent
des clés de preuve et de résolution internes. La définition opérationnelle et
les réponses sur le fold sont détaillées dans
[`AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md`](AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md).

La porte régulière n'est pas une conséquence de la quantification u16. Avec
supports multiples ou extra-shell pertinent, choisir un pivot dans l'union
des supports est faux : sa suppression peut conserver le même niveau. Il faut
alors un quotient de plateau démontré, le générateur de boule saturée comme
référence exacte, ou `unsupported_degeneracy`. L'inertie saturée au-dessus de
la fenêtre H0 reste utilisable sans rebaptiser ces boules régulières.

## 8. Conséquence d'implémentation en dimension trois

Pour une coface directe `Q`, supprimer un label intérieur de sa miniboule
laisse exactement ce label comme unique intrus de la facette : `Q` traite déjà
ce bras. Seules les facettes obtenues en supprimant un élément du support sont
des bras stricts à résoudre. Un support minimal positif en dimension trois a
au plus quatre éléments. Il y a donc au plus quatre bras stricts proposés par
événement direct, avant déduplication globale des facettes.

Cette borne ne prouve ni que le nombre d'événements directs est linéaire, ni
que les recherches de census terminent sous une seconde. Elle indique
néanmoins l'interface à optimiser :

1. streamer les événements directs sans catalogue Gamma et dédupliquer les
   clés des seuls bras stricts ;
2. lancer pour chaque facette unique une requête exacte de `J_F` ;
3. fermer complètement les cas zéro et un, mais permettre un succès positif
   dès que deux intrus canoniques sont certifiés ;
4. émettre un reçu immutable
   `(F, beta(F), u_F, z_F, w_F, T_F, terminal_carrier, descent_receipt)` ;
5. mettre en cache les descentes strictes par clé de facette et refuser tout
   arrêt sans carrier terminal ;
6. grouper directes et gateways par niveau exact avant toute mutation DSU ;
7. conserver des ledgers disjoints `direct_owner` et `gateway_owner`.

Le choix canonique des deux intrus ne doit pas dépendre du scheduling. Une
recherche LBVH peut ordonner les nœuds par leur plus petit `PointId` et fermer
la recherche des deux plus petits témoins seulement lorsque les bornes des
nœuds restants excluent un identifiant plus petit. Scanner tout le nuage est
un oracle, pas le chemin produit.

Deux intrus prouvent la branche `|J_F|>=2` et la décroissance du carrier; ils
ne prouvent pas à eux seuls que les autres intrus, co-minimiseurs ou facettes
ont été couverts. L'attache unique n'est donc consommable que si le
`terminal_carrier` rejoué représente déjà la composante Gamma stricte et sa
couverture complète sous l'autorité globale de rétraction. Sans cette
autorité, la facette reste `unresolved`.

Le MSF est surtout un théorème de certification. Le chemin chaud peut streamer
les mêmes équivalences directement dans le fold atomique et éviter de
matérialiser le graphe global, ses cofaces ou une mosaïque de Delaunay d'ordre
supérieur.

## 9. Portes permanentes exigées

1. E5 : `AC` doit être rattachée au niveau `33/2`, avec
   `added_point_ids=[]`, puis `ABC` doit être une continuation de la même
   composante au niveau `83886/3563`.
2. Comparer `G_k^+` à Gamma facette par facette aux coupes ouvertes et
   fermées, pas seulement les unions de points.
3. Tuer les mutants `drop-silent-star`, `attach-at-next-gabriel`,
   `attach-before-level` et `sequentialize-equal-batch`.
4. Couvrir `I(Q)=empty`, une facette simultanée, plusieurs facettes
   simultanées et plusieurs cofaces silencieuses dans le même lot.
5. Couvrir les trois branches `|J_F|=0/1/>=2`, les co-minimiseurs ex aequo et
   un `T_F` extérieur au cœur qui exige plusieurs descentes.
6. Rejouer `k=1` contre l'EMST; conserver la naissance terminale `k=n` dans
   `full_pi0` et son carrier latent sans nœud public dans `hgp_reduced`.
7. Refuser support multiple, extra-shell pertinent, census tronqué, carrier
   absent et descente non strictement décroissante.
8. Permuter shards, threads et ordres d'arrivée tout en conservant les mêmes
   gateways, lots et partitions.

## Validation logicielle ciblée

Après reconfiguration Release au `HEAD` observé pendant cet audit, la commande

```bash
cmake --build build/v3 --parallel --target mhgp3v_first_incidence
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_first_incidence_'
```

passe `10/10` en `24,08 s` sur la machine partagée. Elle couvre les branches
extra-shell, nœuds internes, ordre élevé, régulière et six refus/planchers.
Ces portes valident le probe borné de premières incidences; elles ne prouvent
ni la source directe globale, ni la porte de régularité, ni la complétude du
resolver, ni une borne 50 k.

## Décision

- Garder `Proposition 6` et `Théorème 5` du graphe brut au statut
  `false_in_general`.
- Admettre `Proposition 6+` et `Théorème 5+` comme réparation mathématique
  sous porte régulière, avec `G_k^+` exhaustif comme oracle.
- Orienter le produit vers `directes + gateways + resolver + fold atomique`,
  en nommant sa portée `normalized_horizontal_h0` et en laissant verticales,
  identités Gamma, statut public et SLO non revendiqués.

GCP non utilisé.
