# Couverture des composantes : le catalogue suffit, pas une seule boule

29 septembre 2026. Contre-vérification indépendante de la dernière section
de la note locale `audit_independant_20260929/ANCRAGE_AMBIGUITES.md`
et de [la réponse du développeur](../../REPONSE_CLAUDE_ANCRAGE_PANELS_ET_RANGEMENT_20260929.md).
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_math_catalogue_cover`,
`public_status=not_claimed`. Contexte de lecture produit : `12aa`, avec les
déplacements de preuves annoncés dans `c5015a570` pris en compte pour les
nouveaux chemins. La note de l'autre auditeur était présente dans le
worktree mais non publiée à `origin/main=c5015a570` à la clôture ; elle
reste sous son contrôle. La preuve ci-dessous est autonome. Aucun moteur
édité ; GCP non utilisé.

## Verdict

L'équivalence proposée est correcte, avec une précision importante : le
catalogue doit conserver la coquille complète et les incidences de tous les
points contenus, pas seulement le support canonique. Un témoin peut même
être choisi avec `p + q_min ≤ K`, condition plus forte que l'admission
habituelle `p + q_min ≤ K + 1`.

Le résultat donne **toutes les composantes couvrantes à une coupe fixée**.
Il ne justifie pas de ne conserver qu'une première boule par point, ni de
fusionner les composantes qui couvrent un même point. La conversion de cette
relation de couverture en partition exclusive reste une décision distincte.

**Le renforcement ne permet pas d'amputer le catalogue de la tour FULL.**
`p+q_min≤K` suffit pour trouver un témoin de couverture dans une composante
déjà connue à la coupe R ; il ne suffit pas à représenter tous les événements
qui construisent ces composantes. Pour {−2,0,2} à K2, les deux composantes
naissent au rayon 1 et fusionnent au rayon 2. La boule de cette fusion a
centre 0, `p=1`, `q_min=2`, donc `p+q_min=K+1`. Supprimer cet événement au
prétexte du lemme confondrait recherche de témoins et construction de FULL.

## Énoncé précis

Dans cette note, `R` est un **rayon**, non un rayon carré. On écrit
`L_K(R) = {c : |X ∩ B(c,R)| ≥ K}`, avec des boules fermées ; cet ensemble
est noté `L_K(R²)` dans la convention de niveaux carrés du moteur.
X est fini, constitué d'abord de sites distincts de l'espace euclidien.
On fixe `x ∈ X`, `1 ≤ K ≤ |X|`, `R ≥ 0` et une composante C de `L_K(R)`.

Alors `dist(x,C) ≤ R` équivaut à l'existence d'une boule critique fermée
`B(c,r)` telle que :

- `r ≤ R`, `x ∈ B(c,r)` et `c ∈ C` ;
- sa population fermée est au moins K ;
- si `r > 0`, son nombre p d'intérieurs stricts et sa cardinalité minimale
  de support positif satisfont `p + q_min ≤ K`.

En dimension trois, le support positif minimal d'une boule de rayon
strictement positif possède deux, trois ou quatre sites. Ce témoin figure
donc dans le catalogue critique complet pour `Kmax ≥ K`. Les boules nulles
doivent être ajoutées par la table des sites : en non pondéré elles servent
K1 ; avec multiplicités elles peuvent aussi servir K supérieur à un.

Une boule quelconque du catalogue peut avoir une population inférieure à K.
Pour l'utiliser comme témoin natif à son propre rayon, il faut donc vérifier
la population, puis résoudre sa composante. L'appartenance `c ∈ C` ne se
déduit pas du seul rayon. Dans l'énoncé logique où `c ∈ C` est déjà fourni,
la réciproque est immédiate même sans la condition de population ; la
construction ci-dessous fournit néanmoins un témoin qui satisfait celle-ci.

## Preuve complète du sens difficile

Pour une K-partie F de X, posons
`Q_F(R) = intersection des B(z,R), pour z ∈ F`. Ces ensembles sont compacts
et convexes, et leur union finie est `L_K(R)`. Ses composantes sont compactes.

1. La distance est atteinte : il existe `y ∈ C` avec `||x−y|| ≤ R`.
   La boule `B(y,R)` contient au moins K sites et contient x. Choisissons une
   K-partie F qui contient x dans cette boule. Son centre de plus petite
   boule c_F et y appartiennent tous deux à `Q_F(R)`. Le segment qui les
   joint reste dans cet ensemble, donc `c_F ∈ C`.

2. Parmi **toutes les K-parties contenant x dont le centre de plus petite
   boule appartient à C à la coupe R**, choisissons celle dont le rayon r
   est minimal. La famille est finie et non vide. Notons c son centre,
   I les intérieurs stricts globaux et U la coquille globale complète.
   Cette plus petite boule est critique. Si r est nul, on a le témoin site.
   Sinon, notons q_min la taille minimale d'un support positif de c dans U.

3. Supposons `p = |I| ≥ K`. Si x est intérieur, K sites intérieurs incluant
   x sont déjà dans une boule strictement plus petite centrée en c. Si x est
   sur U, choisissons x et K−1 intérieurs : déplacer légèrement c vers x
   rend aussi x strictement intérieur, les autres sites restant stricts.
   Dans les deux cas, la plus petite boule de cette nouvelle K-partie G a
   un rayon strictement inférieur à r. Son centre et c sont dans `Q_G(R)` ;
   son centre appartient donc toujours à C. Cela contredit le minimum.

4. On a donc `p < K`. Supposons maintenant `p + q_min > K` et posons
   `t = K−p < q_min`. Choisissons tous les p sites de I et t sites de U,
   avec x parmi ces derniers si x est sur la coquille. C'est possible
   puisque la boule choisie contient au moins K sites.

5. Le centre c n'appartient pas à l'enveloppe convexe des t sites choisis
   sur U. Sinon une représentation convexe minimale donnerait un support
   positif affinement indépendant de taille au plus t, contredisant q_min.
   Par séparation stricte, il existe un déplacement de c qui rapproche
   strictement chacun de ces sites de coquille. Assez petit, il conserve
   tous les intérieurs stricts. La K-partie G ainsi choisie a donc une plus
   petite boule de rayon strictement inférieur à r. Comme au point 3,
   ses deux centres sont reliés dans `Q_G(R)` : contradiction.

Il reste nécessairement `p + q_min ≤ K`. Cette preuve ne suppose ni
position générale, ni unicité du support, ni absence d'égalités. Le minimum
est pris **séparément pour chaque composante C de la coupe demandée**, pas
seulement dans une composante à la date de naissance propre de la boule.

## Égalités, supports et multiplicités

Les coquilles sont fermées : retirer des égalités casse l'énoncé. Par
exemple, pour X = {−2, 0, 2}, K2 et R = 1, les deux composantes sont les
centres −1 et 1. Le point 0 est exactement sur les deux boules témoins.
Les trois coordonnées s'entendent sur un axe de l'espace tridimensionnel.

Un point couvert peut être strictement intérieur et n'appartenir à **aucun**
support de la boule témoin. Exemple K4 : les sites
`(0,0,0), (4,0,0), (2,3,0), x=(2,1,0)`.
La plus petite boule a pour centre `(2,5/6,0)` et rayon `13/6` ; x est
strictement intérieur. Elle a `p=1`, `q_min=3`, donc `p+q_min=4`.
À sa naissance, elle est l'unique centre couvrant les quatre sites.
Restreindre les incidences aux supports perdrait x. Le catalogue requis
n'est donc pas limité aux boules vides de Gabriel d'ordre un.

Sur le carré `(-1,-1,0), (-1,1,0), (1,-1,0), (1,1,0)` à K4, le centre
est l'origine, `q_min=2` et les quatre sites sont sur U. Un support canonique
ne contient qu'une diagonale : ses deux autres sites doivent aussi être
présents dans les incidences. La réduction à q_min ne réduit pas la coquille.

Pour des multiplicités entières positives, représenter chaque observation
par un ID distinct à sa position. Le même minimum porte sur K observations
et contient un représentant de x. Dans la preuve, p est alors la somme des
poids intérieurs. Un choix de t observations de coquille a au plus t
positions distinctes, ce qui conserve l'argument de séparation. Pour un
rayon positif, le renforcement `p+q_min≤K` reste donc vrai. Il est contenu
dans l'admission pondérée actuelle `p≤K−1`, qui est un sur-ensemble.

En revanche, oublier les boules nulles rend le lemme faux : sites 0 et 10,
poids 2 et 1, K2, R=1. La composante autour de 0 couvre ce site ; la seule
boule positive à support double a rayon 5. La boule site de rayon nul et
population 2 fournit le témoin manquant. Ceci est une extension mathématique,
pas une qualification de la tour pondérée du produit.

## Pourquoi toutes les composantes sont nécessaires

Pour X = {−2, 0, 4}, K2, R=2 et x=0, les composantes axiales sont
`[-2,0]` et `{2}`. Toutes deux couvrent x à distance au plus R. La première
boule couvrante de x a centre −1 et rayon 1 ; elle ne représente que la
première composante. La seconde exige la boule de centre 2 et rayon 2.
Même conserver **tous les ex æquo du minimum global** ne suffit pas ici :
ce minimum est unique.

Il faut résoudre les centres au propre rayon, puis remonter dans l'arbre
jusqu'à R et dédupliquer leurs composantes **à cette coupe**. Une cellule
locale critique n'est pas nécessairement active à K, même si sa population
fermée atteint K ; le passage par une K-partie témoin reste nécessaire.
Le mécanisme existant est à confronter à
[`cover_node`](../../../src/tower/tower.cpp), ligne 1563 du snapshot lu,
sans attribuer à ce test
mathématique une qualification nouvelle de cette routine.

Deux détails natifs sont importants pour un futur raccord. Le prédicat
`covering` des lignes 1558–1561 exige `population ≥ K + cover_extra` :
l'équivalence présente correspond à **`cover_extra=0`**. Un supplément peut
supprimer un témoin nécessaire, comme K2 sur deux sites avec supplément un.
L'option `ball_nodes` des lignes 1588–1595 résout toutes les boules couvrantes,
mais `point_node` reste défini par `first[x]`. La relation complète exige
les incidences de population et les images `ball_node`, non les seules
attaches de points à leur première boule.

## Frontière : utilité et limite pour une projection laminaire

Le catalogue donne la relation complète point→composantes, sans construire
d'abord une dilatation puis recalculer sa connexité. C'est utile pour garder
des points frontière : plusieurs boules concurrentes peuvent désigner la
**même** composante, auquel cas elles ne constituent pas une ambiguïté entre
clusters. Une politique d'attache doit comparer leurs images dans l'arbre,
pas le nombre brut de supports ou un départage par ID.

Mais la relation reste potentiellement multivoque. Dans l'exemple
{−2,0,2}, K2, R=1, les ensembles couverts sont {−2,0} et {0,2} alors que
les deux composantes sont distinctes. Les unir à cause du point 0 détruirait
la séparation HGP. Aucun choix exclusif ne peut garder simultanément les
deux appartenances comme deux blocs disjoints d'une partition.

Une piste prudente est de conserver l'attache précoce lorsque toutes les
composantes jugées compétitives ont la même image, et de traiter seulement
la véritable concurrence entre branches. Cela ne force pas tous les points
vers core : pour deux sites à distance d, K2, leur composante commune les
couvre dès d/2 alors que chacun devient core à d. En revanche, le choix de
la bande compétitive, de sa date et de la règle en cas de concurrence reste
à définir et tester. La discontinuité d'un seuil de bande ne disparaît pas
par le présent lemme ; aucune stabilité statistique ni supériorité ARI n'en
est déduite. Un rattachement fixé une fois puis suivi par ascendance donne
la laminarité ; des réaffectations indépendantes à chaque coupe ne la
garantissent pas.

## Petit oracle exact indépendant

Le [script autonome](../../../receipts/audit_continu_20260929/math_catalogue_cover/check_line.py)
ne charge aucun moteur, catalogue produit ou oracle préexistant. Il construit
les composantes directement par l'arrangement des intervalles fermés de
rayon R, en testant la profondeur aux extrémités et entre elles. Une autre
routine énumère les boules critiques diamétrales et compare les **listes
complètes de composantes** couvrant chaque site, pour l'admission ordinaire
et pour l'admission renforcée. Les boules nulles sont incluses.

Pour des sites collinéaires dans l'espace, la projection orthogonale sur
l'axe diminue toutes les distances aux sites. Elle relie chaque centre à
sa projection sans quitter `L_K(R)` ; elle préserve ainsi les composantes
et les distances minimales depuis un site de l'axe. Ce test n'assimile donc
pas arbitrairement une composante tridimensionnelle à un intervalle.

Les exécutions [normale](../../../receipts/audit_continu_20260929/math_catalogue_cover/receipt_normal.json)
et [Python −O](../../../receipts/audit_continu_20260929/math_catalogue_cover/receipt_optimized.json)
passent avec le même hash de source et les mêmes comptes :

- 273 scènes : tous les sous-ensembles de taille 1 à 6 de {0,…,6}, plus
  cinq géométries avec toutes les affectations de poids parmi {1,2,3} ;
- K1, K2 et K3 lorsqu'ils sont définis ; 7 822 coupes, aux seuils et entre
  les seuils, ainsi qu'au-delà du plus grand rayon critique ;
- 30 102 requêtes de points, dont 1 298 avec plusieurs composantes ;
- 23 385 incidences avec un témoin de rayon strictement inférieur à R ;
- les deux mutations « minimum global seulement » et « pas de boule nulle »
  sont rejetées par les contre-exemples décrits ci-dessus.

Il s'agit d'une falsification bornée, distincte de la preuve générale. Elle
ne teste pas numériquement q3/q4, les résolutions natives, ni un coût GPU.
Le nombre d'incidences point→boules, la résolution des composantes et leur
déduplication restent à mesurer ; aucune borne sous-quadratique ni garantie
statistique ne découle de ce lemme.
