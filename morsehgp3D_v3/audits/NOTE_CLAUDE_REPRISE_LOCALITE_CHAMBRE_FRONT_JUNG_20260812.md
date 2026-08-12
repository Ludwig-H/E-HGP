# Note Claude — reprise de session : localité directionnelle du front de Jung

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note est une **proposition mathématique** de Claude, écrite avant toute
implémentation et avant toute mesure. Elle ne qualifie aucun snapshot, ne
revendique aucune complexité et ne reçoit aucune porte. Elle demande un
contre-audit.

## 0. Contexte de reprise

La session précédente a été interrompue. L'état repris est celui de
`AUDIT_ETAT_COURANT.md` au `HEAD=407d4d1`, plus le contre-audit
`AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md` et l'audit
`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md`.

Décision d'orientation prise par Claude, à contre-auditer : la lane
« cellules de centres » n'est plus poursuivie comme **chemin produit**. Le point
gelé `uniform,n=50 000` publie `839 582 666` occurrences pour `21 395 212`
supports; même un RLE parfait avant lift laisse un facteur de génération sans
rapport avec la cible d'une seconde. Elle reste conservée comme **comparateur
d'identités** et falsificateur borné, conformément à la matrice par tranche du
`README.md`.

La route poursuivie est celle des sections 5 à 7 de
`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md` : front de Jung
coalescé, puis enveloppe top-9 du plan médiateur, puis owner génératif
exact-once, puis fold H0.

Le verrou nommé par l'auditeur est `W_front` : produire le front sans payer
`C(n,2)`, alors que le dual-tree existant a des pentes voisines de `2,30`. La
présente note propose deux certificats exacts destinés précisément à ce verrou.

## 1. Lemme de localité directionnelle

Soit `C` un cône convexe polyédral de sommet l'origine et soit

$$\gamma_C=\max\left\lbrace\angle(u,v)\ :\ u,v\in C\setminus\lbrace 0\rbrace\right\rbrace.$$

Pour un cône simplicial, ce maximum est atteint sur un couple d'arêtes
extrêmes. Pour la chambre Yao-48 canonique `x>=y>=z>=0`, les arêtes extrêmes
sont `(1,0,0)`, `(1,1,0)` et `(1,1,1)`; le maximum vaut donc

$$\cos\gamma_{48}=\frac{1}{\sqrt{3}},\qquad\gamma_{48}=54{,}7356\ \text{degres}.$$

Soient une ancre `a`, deux points `b_i` et `b_j` tels que `u=b_i-a` et
`v=b_j-a` appartiennent à la même chambre translatée en `a`. Poser
`D_i=\lVert u\rVert` et `D_j=\lVert v\rVert`.

### Lemme 1 — témoin diamétral par chambre

Si `D_i\sqrt{3}<D_j`, alors `b_i` est strictement intérieur à la boule
diamétrale de la paire `(a,b_j)`.

Preuve. Le prédicat exact est `(b_i-a)\cdot(b_i-b_j)<0`, soit
`\lVert u\rVert^2-u\cdot v<0`. Or `u\cdot v\geq D_iD_j\cos\gamma_{48}`, donc

$$\lVert u\rVert^{2}-u\mathbin{\cdot}v\leq D_i\left(D_i-\frac{D_j}{\sqrt{3}}\right)<0.$$

Aucune hypothèse de densité, de position générale ou de bord n'intervient. La
comparaison est entière : `3D_i^2<D_j^2` suffit et se teste en `i64` sur le
profil u16.

### Corollaire 1 — rayon de coupure exact par ancre et par chambre

Soit `\rho_c(a)` la distance du dixième plus proche `PointId` de `a` dans la
chambre `c`, ou `+\infty` si la chambre contient moins de dix points. Toute
paire `(a,b)` avec `b` dans la chambre `c` et `\lVert b-a\rVert>\rho_c(a)\sqrt{3}`
possède au moins dix points strictement intérieurs à sa boule diamétrale. Elle
est donc hors de la lane q2.

C'est un certificat de **coupure radiale par chambre**, calculable une fois par
ancre, qui borne l'univers des partenaires q2 sans énumérer les paires. Il est
strictement plus fort qu'une borne par densité : il reste exact sur une ancre du
bord convexe (sa chambre sortante a moins de dix points, le rayon vaut
`+\infty`, et aucune paire n'y est prunée à tort).

## 2. Ce que le Lemme 1 ne donne pas pour q3 et q4

Un témoin universel de Jung n'est pas un simple intérieur diamétral. Les
prédicats norm-only déjà reçus donnent les boules de milieu

$$B\left(m,\frac{D}{\sqrt{12}}\right)\subset W_3(a,b),\qquad B\left(m,\frac{D}{\sqrt{15}}\right)\subset W_4(a,b),$$

avec `m` le milieu de `(a,b)`. En posant `t=D_i/D_j` et `\theta` l'angle entre
`u` et `v`,

$$\frac{\left\Vert b_i-m\right\Vert^{2}}{D_j^{2}}=t^{2}-t\cos\theta+\frac{1}{4}.$$

La condition de témoin q3 devient `t^2-t\cos\theta+1/6<0`, dont le discriminant
`\cos^2\theta-2/3` est positif seulement si `\theta<35{,}264` degrés. La
condition q4 devient `t^2-t\cos\theta+11/60<0`, de discriminant
`\cos^2\theta-11/15`, positif seulement si `\theta<31{,}134` degrés.

Conséquence directe, et c'est le point à retenir : **les 48 chambres Yao ne
peuvent pas certifier un témoin de Jung**, puisque `\gamma_{48}=54{,}74` degrés.
Une banque directionnelle utilisable pour q3/q4 exige un raffinement angulaire
de diamètre strictement inférieur à `31{,}134` degrés, par exemple une
subdivision barycentrique de chaque triangle sphérique de l'octaèdre. Le nombre
de chambres, le tie-break de frontière et le coût de remplissage deviennent
alors des paramètres à mesurer, pas des constantes admises.

Pour une chambre de diamètre `\gamma` avec `\cos\gamma>\sqrt{11/15}`, les
témoins q4 certifiés sont exactement les `b_i` dont le rapport `t` appartient à
l'intervalle ouvert

$$\left(\frac{\cos\gamma-\sqrt{\cos^{2}\gamma-11/15}}{2},\ \frac{\cos\gamma+\sqrt{\cos^{2}\gamma-11/15}}{2}\right).$$

Ce n'est donc pas un préfixe radial mais un **anneau** : le témoin doit être
près du milieu, pas près de l'ancre. La coupure q4 par chambre n'est donc pas
monotone en `D` et ne se réduit pas à un rayon unique. C'est la raison pour
laquelle la section suivante propose un producteur par blocs plutôt qu'une
banque par ancre.

## 3. Producteur candidat du front : boule témoin commune par produit de nœuds

Soit un LBVH sur les points. Pour deux nœuds `A` et `B` de centres `c_A,c_B` et
de rayons englobants `r_A,r_B`, poser `s=r_A+r_B`, `d=\lVert c_B-c_A\rVert` et
`D_{\min}=d-s`. Pour tout `a\in A` et tout `b\in B`, le milieu `m` appartient à
la boule de centre `m_0=(c_A+c_B)/2` et de rayon `s/2`, et
`\lVert b-a\rVert\geq D_{\min}`.

### Lemme 2 — boule témoin commune

Si `D_{\min}>0`, alors pour toute paire `(a,b)` du produit `A\times B`,

$$B\left(m_0,\ \frac{D_{\min}}{\sqrt{15}}-\frac{s}{2}\right)\subset B\left(m,\frac{\lVert b-a\rVert}{\sqrt{15}}\right)\subset W_4(a,b),$$

et la même inclusion vaut avec `\sqrt{12}` pour `W_3` et avec le rayon
`D_{\min}/2-s/2` pour la boule diamétrale.

Le rayon témoin q4 est strictement positif dès que

$$d>s\left(1+\frac{\sqrt{15}}{2}\right)\simeq 2{,}9365\ s.$$

Ainsi, dès qu'un produit de nœuds est séparé d'environ trois fois sa propre
taille, **un unique range-count** décide tout le produit : huit `PointId`
distincts dans la boule témoin ferment la lane q4 du bloc entier, neuf ferment
q3, dix ferment q2. Le bloc n'est ouvert que si les trois lanes échouent.

Deux obligations d'exactitude, à graver comme fixtures avant toute mesure :

1. les points de `A` et de `B` eux-mêmes ne doivent jamais être comptés comme
   témoins. Pour q3/q4 la séparation les exclut automatiquement dès que
   `d>2{,}9365\,s`, mais la vérification doit être un test entier explicite,
   pas un argument de marge; pour la lane diamétrale la borne est serrée et
   l'exclusion doit être écrite;
2. le range-count doit compter des `PointId` distincts, jamais des visites, et
   toute égalité au bord de la boule témoin reste fail-open — les boules
   témoins sont lues ouvertes, donc un contact exact ne crédite pas.

Le range-count exact se fait sur le même LBVH avec des bornes entières : un
nœud entièrement inclus crédite sa masse, un nœud disjoint est rejeté, un nœud
ambigu descend. Il s'arrête dès le seuil atteint, donc son coût est borné par le
seuil et non par la population.

### Pourquoi ce producteur peut ne pas avoir la pente du dual-tree courant

Le dual-tree réfuté repart de la racine pour chercher des témoins par bloc. Ici
le témoin n'est plus cherché par paire ni par bloc de paires : c'est une seule
requête de comptage, dont le rayon décroît avec la séparation. Un bloc lointain
est fermé en `O(\text{seuil})` visites au lieu d'un parcours complet. La
récursion ne descend que sur les produits proches, dont le nombre est ce que
`W_front` doit mesurer.

Aucune borne n'est revendiquée. Les compteurs obligatoires sont : produits de
nœuds visités, range-counts lancés, nœuds visités par range-count, microtuiles
ouvertes, tests ponctuels, paires émises, et la fermeture
`pair_mass_pruned+pair_mass_microtiles=C(n,2)` par lane. La gate reste deux
pentes au plus `1,35` sur `uniform` **et** `eight_clusters` à
`12 500/25 000/50 000`.

## 4. Falsification attendue

`eight_clusters` est la famille adverse annoncée : une paire inter-amas a un
spindle vide et survit à tous les certificats de témoins. Le nombre de telles
paires peut être quadratique. Le Lemme 1 ne les sauve pas, puisqu'il ne conclut
que lorsque la chambre est peuplée. Deux issues sont possibles et doivent être
mesurées, pas devinées :

- ces paires survivent au front mais meurent immédiatement à l'extension, car
  leur enveloppe top-9 ne fournit aucun carrier admissible; le coût est alors
  celui de leur énumération, qui reste quadratique et refuse la route;
- un certificat collectif supplémentaire — center-cover par patches, ou
  séparation par plans de coupe entre amas — les ferme avant émission de
  `PairId`. C'est l'obligation nommée par l'auditeur.

Claude ne dispose d'aucune preuve pour la seconde issue. La mesure passe donc
`eight_clusters` en premier, avant toute optimisation du régime volumique.

## 5. Questions à l'auditeur

1. Le Lemme 1 et son corollaire sont-ils déjà couverts par un énoncé du dépôt
   que Claude aurait manqué, notamment dans les notes Yao48 ou dans
   `NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md` ?
2. Le raffinement angulaire sous `31{,}134` degrés est-il jugé praticable comme
   banque, ou faut-il abandonner la voie « banque directionnelle » pour q3/q4 et
   ne garder que le producteur par blocs de la section 3 ?
3. Le Lemme 2 a-t-il un défaut d'exclusion que Claude n'aurait pas vu, en
   particulier pour la lane diamétrale où la marge est serrée ?
4. Sur `eight_clusters`, existe-t-il déjà un certificat reçu qui ferme les
   paires inter-amas avant émission, ou faut-il le construire ?
5. Le contrat visé reste-t-il `hgp_reduced_normalized_h0_v3`, ou faut-il viser
   directement `BenchmarkOutputContract-v1` — auquel cas la cible d'une seconde
   demandée par l'utilisateur inclut dix forêts, verticales, lots et certificat
   minimal ?

## 6. Ce que cette note ne dit pas

Elle ne mesure rien, ne compile rien et ne reçoit aucune porte. Les Lemmes 1
et 2 sont des énoncés géométriques élémentaires; leur exactitude ne dit rien du
coût du producteur, ni de la taille du front sur les familles du dépôt, ni du
SLO. Aucun statut public n'est modifié.

GCP non utilisé pour cette note.
