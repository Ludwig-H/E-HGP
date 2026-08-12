# Spécification mathématique — localité certifiée par inversion

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note conserve uniquement le lemme durable et l'architecture candidate.
Elle ne reçoit aucun snapshot, test, chrono, tableau de masse ou claim de
complexité. Le verdict logiciel est dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et le contre-audit pincé du
probe dans
[`AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md`](AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md).

## 1. Lemme de l'antipode

Soit `P` un nuage à positions distinctes, `x` une ancre et `B` une boule non
dégénérée de centre `c` et de rayon `R` dont `x` appartient au bord. Posons
`D=2R` et `u=(c-x)/R`. La boule est celle de diamètre
`[x,x+D u]`, même si l'antipode n'est pas une observation. Pour `z!=x`, poser
`s=z-x`, `d=||s||` et `v=s/d`. Alors

$$z\in\mathrm{int}(B)\iff\left\Vert s\right\Vert^{2}<D\,(u\mathbin{\cdot}s)\iff d<D\cos\angle(u,v).$$

Par inversion `zeta(z)=s/||s||^2`, cette condition devient l'inégalité affine
`u dot zeta(z)>1/D`. À ancre fixée, les boules passant par `x` et possédant au
plus `K-1` intérieurs correspondent donc au niveau inférieur à `K` du nuage
inversé local. Cette lecture motive une recherche output-sensitive; elle ne
fournit pas à elle seule un algorithme borné.

Le rayon nul et les `PointId` colocalisés exigent une politique séparée. Une
implémentation générique doit les conserver fail-open ou retourner
`unsupported_degeneracy`; elle ne peut pas omettre le vecteur nul.

## 2. Certificat par calottes

Pour un point `z`, la direction de l'antipode est interdite à diamètre `D` dans
la calotte

$$C_z(D)=\left\lbrace u\in S^{2}:u\mathbin{\cdot}v>d/D\right\rbrace.$$

Ces calottes croissent avec `D`.

**Théorème de localité.** Si, pour un rayon `r>0`, toute direction appartient
à au moins `K` calottes strictes `C_z(r)`, alors toute boule passant par `x` et
possédant au plus `K-1` intérieurs vérifie `diam(B)<r`.

**Preuve.** Si `D>=r`, chacun des `K` points qui couvre la direction à `r`
reste strictement intérieur à diamètre `D`, contradiction. Fin de preuve.

Pour `K_eff=10`, dix intérieurs rendent tout support propre positif d'arité au
moins deux H0-inerte dans la fenêtre demandée. Lorsque l'arité est déjà
certifiée, les seuils minimaux sont dix, neuf et huit pour q2, q3 et q4. Une
banque commune peut conserver simultanément les statistiques d'ordre 10, 9 et
8; elle ne doit pas confondre leurs verdicts.

Une couverture de toute la sphère est impossible aux ancres extrêmes de
l'enveloppe convexe et, dans une direction normale, sur un nuage coplanaire.
Le certificat global sert donc de succès partiel avec fallback, jamais de
condition `toutes les ancres certifiées` pour une source générale.

## 3. Seuil par cellule directionnelle

Discrétiser la sphère par les triangles géodésiques d'une subdivision de
l'octaèdre. Leurs sommets sont des vecteurs entiers `g` vérifiant
`|g_x|+|g_y|+|g_z|=m`. Pour une cellule `C` et `s=z-x`, définir

$$\rho_C(z)=\max_{g\in C}\frac{\left\Vert g\right\Vert\left\Vert s\right\Vert^{2}}{g\mathbin{\cdot}s},$$

avec `rho=+infini` si un sommet vérifie `g dot s<=0`. Une calotte stricte de
rayon angulaire inférieur à 90 degrés est géodésiquement convexe : si ses
trois sommets satisfont l'inégalité, toute la cellule la satisfait.

Si `r_C` est la K-ième valeur de `rho_C`, tout diamètre strictement supérieur à
`r_C` possède au moins K intérieurs pour une direction dans `C`. À l'égalité,
le K-ième témoin peut rester sur le shell; le filtre candidat doit donc garder
`D<=r_C`. Le test exact sur un sommet est

$$g\mathbin{\cdot}s>0\quad\text{et}\quad(g\mathbin{\cdot}s)^{2}r^{2}>\left\Vert g\right\Vert^{2}\left(\left\Vert s\right\Vert^{2}\right)^{2}.$$

Sur u16, ces décisions tiennent dans `i128` pour les subdivisions bornées
utilisées ici. Les comparaisons de deux seuils se font par produits croisés;
aucun flottant ni racine n'est une autorité.

Le localisateur doit rendre toutes les cellules incidentes à une direction de
bord. Une cellule sans cible ne demande aucune couverture. Une cellule
sous-pleine ou ouverte reste au résiduel exact; elle ne force pas le refus de
toute l'ancre.

## 4. Parcours LBVH candidat

Le scan `cellules * points` n'est qu'un oracle borné. Pour un nœud témoin `W`
et une cellule `C`, poser `d2_min=dist^2(x,AABB(W))` et, pour chaque sommet
`g`, `dot_max(g,W)=max_{z in W} g dot (z-x)`. Si un `dot_max<=0`, aucun point
du nœud ne couvre toute la cellule. Sinon le minorant

$$LB_C(W)=\max_{g\in C}\frac{\left\Vert g\right\Vert^{2}d2_{min}^{2}}{dot_{max}(g,W)^{2}}$$

borne inférieurement la meilleure clé `rho_C^2` du nœud. Un parcours best-first
sur `(W,cell_mask)` peut couper le nœud lorsque ce minorant ne bat plus la
K-ième clé courante; l'égalité descend pour conserver le tie-break canonique.
Les masques des cellules encore sous-pleines ou améliorables évitent de
repartir indépendamment pour chaque cellule.

Cette borne est exacte. Elle ne prouve aucune complexité sous-linéaire : un
cône peut contenir tout le nuage et une requête peut visiter tout le LBVH. Le
reçu doit compter visites de nœuds, feuilles, tests de plans et de points,
opérations de heap, allocations, octets et high-water.

## 5. Jung ferme l'intériorité, pas les supports

Pour un support propre positif `S`, sa boule circonscrite est sa miniboule. En
dimension trois, Jung donne

$$R\leq\mathrm{diam}(S)\sqrt{\frac{3}{8}},\qquad D^{2}\leq\frac{3}{2}\mathrm{diam}(S)^{2}.$$

Cette borne ferme un balayage d'intériorité : au-delà de
`2 d_z^2>3 diam(S)^2`, aucun nouveau point ne peut être intérieur. Elle ne
borne pas l'univers des partenaires de support. En particulier, la boule
diamétrale d'une corde n'est généralement pas incluse dans la boule
circonscrite qui contient cette corde.

La contre-fixture u16 complète à graver est :

- `A=(100,100,100)`, `B=(200,100,100)`, `C=(150,180,100)` ;
- `W_j=(80,140,96+j)` pour `0<=j<=9`.

Le triangle `ABC` est aigu, de centre `(150,995/8,100)` et de rayon carré
`198025/64`. Chaque `W_j` est strictement extérieur, avec un excès de distance
carrée `2050+(j-4)^2`. En revanche

$$(W_j-A)\mathbin{\cdot}(W_j-C)=-200+(j-4)^{2}<0.$$

Les dix témoins sont donc strictement intérieurs à la boule diamétrale de
`AC`, qui est rejetée en q2, tandis que `ABC` reste une activation q3 sans
intérieur. Une fenêtre q3/q4 ne se déduit jamais de la lane q2. Tant que cette
fixture n'est pas exécutée par une porte nommée, elle reste une obligation,
pas une fixture logicielle reçue.

## 6. De la mesure à une source directe

Un compteur de supports n'est pas une source. Pour qu'un support `U` devienne
une coface directe `Q=U union I`, le producteur doit publier et faire rejouer :

- le support minimal propre positif et sa provenance ;
- tous les intérieurs stricts `I`, avec fermeture terminale ;
- le shell global complet et la politique d'extra-shell ;
- la `BallKey` canonique et le niveau exact ;
- un owner exact-once indépendant du scheduling ;
- le statut de fenêtre certifiée ou résiduelle.

Un juge de cardinalité ne suffit pas : l'oracle compare les identités de
`BallKey`, supports, intérieurs et shell. Des supports multiples nommant la
même boule sont dédupliqués par `BallKey`, puis leur plateau est quotienté
atomiquement ou refusé dans la fenêtre pertinente.

La fermeture d'une facette `F` pour la requête `J_F` exige un nouveau
certificat sur sa propre miniboule. L'inégalité `beta(F)<beta(Q)` entre deux
boules de centres différents n'implique aucune inclusion et ne ferme pas
`J_F` dans le voisinage de la coface parente.

## 7. Portes industrielles

Avant tout port G4 de cette voie :

1. exécuter la contre-fixture q3 ci-dessus et les cas colocalisé, cellule de
   bord, cellule ouverte, égalité de seuil et support multiple ;
2. comparer les records complets à un oracle rationnel indépendant, avec
   mutants omission, doublon, shell perdu et owner dupliqué ;
3. rendre toute fenêtre non certifiée `incomplete`, jamais succès avec simple
   avertissement ;
4. mesurer les compteurs de travail et la mémoire à `12 500/25 000/50 000`
   sur les quatre familles, avec deux pentes successives ;
5. composer seulement le résidu avec les autres certificats q2/q3/q4, le
   resolver et le fold atomique ;
6. produire le payload contractuel complet avant toute mesure du SLO.

La voie évite par construction toute mosaïque de Delaunay d'ordre supérieur et
tout catalogue global de paires, cellules, cofaces ou incidences. Elle ne
prouve pas encore que son état intermédiaire reste compatible avec 50 k.

GCP non utilisé.
