# Réponses à Claude — localité certifiée par inversion

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ces réponses portent sur les six questions mathématiques posées pendant la
session. Elles n'admettent ni le prototype courant, ni une complexité, ni une
mesure 50 k. Le verdict logiciel lié au worktree reste
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## Q1 — théorème de localité

**Oui pour le lemme, avec deux restrictions de portée.**

Toute boule non dégénérée de centre `c`, rayon `R`, et dont `x` appartient à la
sphère possède l'antipode unique `x+Du`, avec `D=2R` et
`u=(c-x)/R`. Elle est donc bien la boule de diamètre `[x,x+Du]`, même lorsque
l'antipode n'est pas une observation. Pour `s=z-x`, l'intérieur strict est

$$\left\Vert s\right\Vert^{2}<D(u\mathbin{\cdot}s).$$

La calotte définie par cette inégalité croît lorsque `D` croît; la
contraposée du certificat global est donc exacte.

Le seuil uniforme dix est sûr pour q2, q3 et q4 : dix intérieurs donnent
`p+q>=12` pour toute arité `q>=2`. Il n'est pas optimal. Lorsque l'arité du
support propre positif est déjà certifiée, les seuils exacts d'inertie H0 sont
respectivement `10/9/8`. Une banque directionnelle peut donc conserver dans le
même ordre statistique les seuils 10, 9 et 8 sans confondre les décisions des
trois lanes.

Cette lecture suppose le domaine de support annoncé, notamment `RelevantGP` ou
un traitement explicite de la coquille dégénérée. Le rayon nul et les
coordonnées dupliquées ne sont pas couverts silencieusement par le lemme
non dégénéré.

## Q2 — triangles géodésiques

**Oui, le test des trois sommets est un certificat exact de couverture de la
cellule.**

Une calotte stricte de rayon inférieur à 90 degrés s'écrit
`a dot u>t` avec `t>0`. Si ses sommets `u_i` la vérifient, toute combinaison
sphérique courte `u=(sum lambda_i u_i)/||sum lambda_i u_i||`, avec
`lambda_i>=0`, la vérifie aussi : le numérateur est strictement supérieur à
`t sum lambda_i`, tandis que le dénominateur est au plus `sum lambda_i`.

La projection radiale d'un sous-triangle d'une face positive de l'octaèdre est
précisément cette enveloppe sphérique courte. Cela reste vrai pour `m=1` : la
face entière est le triangle sphérique de l'octant; ses angles droits ne
brisent pas la convexité de la calotte qui contient strictement ses trois
sommets.

Aucune garde séparée `||s||<r` n'est nécessaire si la comparaison quadratique
complète est utilisée. Par Cauchy--Schwarz, `||s||>=r` implique

$$ (g\mathbin{\cdot}s)^2r^2\leq\left\Vert g\right\Vert^{2}\left\Vert s\right\Vert^{2}r^2\leq\left\Vert g\right\Vert^{2}\left\Vert s\right\Vert^{4}, $$

donc l'inégalité stricte de couverture échoue. Le seul test `g dot s>0` ne
suffirait pas.

## Q3 — égalité du seuil

**Le côté fail-open proposé est correct : conserver `D^2<=r_c^2`.**

Le témoin est strict pour toute la cellule seulement lorsque `D>rho(c,z)`.
Au seuil `D=r_c`, un ou plusieurs témoins peuvent être sur la sphère et ne
comptent pas comme intérieurs. Une activation avec moins de `K` intérieurs
vérifie donc seulement `D<=r_c`, pas `D<r_c`.

Les statistiques d'ordre portent sur des `PointId` distincts avec fractions
comparées exactement et départage canonique. La cible ne peut pas se compter
dans un prune strict : pour sa propre direction, son seuil est au moins `D^2`;
si `D^2` dépasse strictement la K-ième clé, cette cible n'appartient pas aux K
clés qui certifient la coupe.

Sur une frontière entre cellules, une cellule réellement incidente suffit à
certifier une coupe. Un localisateur conservateur peut en retourner davantage,
mais il doit toujours inclure toutes les cellules réellement incidentes; en
oublier une au bord peut rendre une omission fausse.

## Q4 — cellules ouvertes

**La seule existence d'un second point de support ne donne pas de borne radiale
finie universelle.** Un point arbitrairement lointain dans une direction
sortante peut former avec l'ancre une paire q2 de faible profondeur. Pour q3 et
q4, l'antipode n'est même pas nécessairement une observation.

La bonne amélioration est de rendre le calcul cible-aware :

- en q2, seules les cellules contenant effectivement des directions de cibles
  possédées doivent être interrogées;
- une cellule sans cible ne demande aucune couverture;
- une cellule sous-pleine mais occupée conserve sa plage cible dans un
  dual-tree exact ou une requête de cône reçue;
- en q3/q4, l'espace inversé peut proposer les plans de support, mais ne ferme
  rien sans owner, ledger et fallback exacts.

Une requête de cône LBVH n'a pas un coût égal au seul nombre de réponses : ses
visites, tests, octets et high-water doivent être comptés. La voie produit ne
doit pas basculer vers un scan univers complet par ancre ou par paire.

Pour remplir les seuils d'une cellule sans le scan `8m^2Mn`, un nœud AABB
témoin `W` admet le minorant rationnel suivant. Soient `d2_min` la distance
carrée minimale de l'ancre à `W` et `dot_max(g,W)` le maximum de
`g dot (w-x)` sur la boîte. Si un `dot_max<=0`, le nœud ne couvre pas la
cellule. Sinon

$$\mathrm{LB}_C(W)=\max_g\frac{\left\Vert g\right\Vert^{2}d2_{\min}^{2}}{dot_{\max}(g,W)^2}\leq\min_{w\in W}\tau_C(w).$$

Un best-first `LBVH x masque_de_cellules` peut couper `W` lorsque ce minorant
ne bat plus le seuil courant; l'égalité descend pour fermer les ex æquo et le
départage par `PointId`. C'est une proposition d'ordonnance, pas une preuve de
travail linéaire.

## Q5 — niveau inversé local et piste abandonnée

**Décision : réouverture conditionnelle recevable comme oracle/candidat local,
pas route produit admise.** Ce n'est pas un contournement de nom si chaque
objet inversé est transitoire, détruit après son ancre ou sa tuile, et si aucun
atlas, cellule, coface ou incidence globale ne persiste.

En revanche, les affirmations actuelles `taille O(M)`, `M environ 200 constant`
et « le pinceau s'applique tel quel » ne sont pas prouvées. Il faut encore :

1. établir la bijection exacte entre plans inversés, supports propres positifs,
   niveaux stricts/fermés et `BallKey`;
2. traiter coplanarités, coquilles multiples, orientation et owner exact-once;
3. borner ou mesurer le travail total `sum_x work(x)`, pas seulement `M` sur un
   nuage à 2 000 points;
4. prouver que mémoire et frontière restent locales et qu'aucun catalogue
   higher-order n'est reconstruit ancre après ancre sous un autre nom;
5. comparer l'objet produit à l'oracle exhaustif q2/q3/q4 sur petits nuages.

Jusqu'à ces portes, `order_k_bfs.hpp` est un prior art différentiel et le
niveau inversé un oracle de recherche. La nouvelle identité d'inversion
justifie l'étude; elle ne suffit pas à rouvrir un backend industriel.

## Q6 — facteur 384 et causalité

**Non. Le facteur 384 est faux comme rapport à la vraie région de témoins et ne
prouve aucune cause racine.**

Sous un modèle homogène isotrope, la vraie région diamétrale pour une cible à
distance `D` est une boule de rayon `D/2`; son volume vaut
`pi D^3/6`, soit un huitième de la boule centrée en l'ancre de rayon `D`. La
région Yao simplifiée « une chambre sur 48 et rayon D/2 » vaut un quarante-
huitième de cette boule diamétrale. Le rapport idéalement modélisé entre région
exacte et région Yao est donc 48, pas `8*48=384`; le facteur huit a été compté
deux fois relativement à l'objet pertinent.

Même le facteur 48 n'est qu'une intuition de volume sous homogénéité, pas une
borne sur les visites LBVH, les retests, les familles anisotropes ou la
latence. Surtout, le dual-tree courant utilise toutes les directions : ses
pentes rouges ne peuvent pas être attribuées à la seule chambre Yao. Les reçus
prouvent des réévaluations superlinéaires de frontière; ils ne prouvent pas que
le certificat sous-jacent est insauvable.

La cascade affine/duale, l'état adaptatif `(Q,A,F)`, le triple-tree `(P,Q,W)` et
la banque de calottes par cellule restent donc quatre expériences exactes
comparables. Aucune ne devient prioritaire par le facteur 384.

## Décision d'implémentation conseillée

1. Ne pas raccorder le probe courant à CMake avant correction du signe
   InSphere q4, de la fermeture top-M inversée, du vecteur nul et des claims de
   census/coût.
2. Conserver la banque par cellule, avec seuils `10/9/8`, reçu contenant les
   identités des témoins et remplissage best-first par masque de cellules.
3. Garder un ledger exact `certified_owner + residual_owner = universe_owner`;
   aucune cellule ouverte ni fenêtre tronquée ne rend le code zéro.
4. Ne porter le niveau inversé q2/q3/q4 qu'après bijection et différentiel
   borné; mesurer ensuite `sum work`, mémoire et sortie aux trois tailles.
5. Continuer en parallèle l'état dual adaptatif : la localité n'a pas réfuté
   cette route.
6. N'ouvrir G4 qu'après deux pentes locales admissibles et un producteur du
   payload officiel; aucune mesure CPU count-only ne qualifie le SLO.

GCP non utilisé.
