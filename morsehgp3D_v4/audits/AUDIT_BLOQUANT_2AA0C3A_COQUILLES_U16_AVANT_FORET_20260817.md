# Audit bloquant après `2aa0c3a` — les coquilles u16 ne peuvent pas être supprimées avant la forêt

Date : 17 août 2026.  
Pin de code audité : `2aa0c3a269b7715a6325ee60813487b9f7c6724f`.  
Commit axial également audité : `2c76e9a85c8fad5c68126ad1558172401666c8c0`.

Cette note ne relève qu'un verrou. La sélection axiale q4 est reçue comme
chemin optionnel exact, et la nouvelle lane q2 est correcte **sous position
générale**. En revanche, son propre reçu montre que cette hypothèse est
massivement violée sur la grille u16 : `837` supports q2 à coquille dès
`uniform,n=400`.

Le problème n'est pas seulement qu'un mode `--exact` refusera le nuage. Il est
plus grave : **supprimer les événements à coquille puis construire la forêt sur
le sous-flux régulier ne rend pas la hiérarchie HGP exacte**.

Il faut donc trancher Q5 avant de déclarer la couche événementielle complète ou
de brancher le fold public.

---

## 1. Contre-fixture minimale : quatre points sur un cercle

Prendre dans la grille u16 :

```text
p0 = (110,100,100),
p1 = (100,110,100),
p2 = ( 90,100,100),
p3 = (100, 90,100).
```

Les quatre points sont sur la sphère de centre

```text
c = (100,100,100)
```

et de rayon carré `R²=100`.

Les quatre côtés du carré ont longueur carrée `200`, donc rayon de naissance
carré `50`. Les deux diagonales ont longueur carrée `400`, donc boule
diamétrale de rayon carré `100`.

### Ce que fait réellement la filtration pour `K=2`

Juste avant `R²=100`, les quatre côtés sont quatre sommets distincts du graphe
`Gamma_2` : deux côtés adjacents ne sont pas encore reliés, car leur union est
un triangle rectangle dont la miniboule a précisément rayon carré `100`.

Au niveau `R²=100`, les quatre triangles à trois sommets du carré naissent.
Chacun est de Gabriel : son disque ouvert ne contient aucun point extérieur,
et le quatrième point est seulement sur la coquille. Ces quatre triangles
relient simultanément les quatre côtés en une composante `K=2`.

### Ce que produit la taxonomie actuelle

Chaque triangle rectangle est porté par une diagonale q2. Pour la diagonale
`p0p2`, les deux points `p1,p3` sont sur la coquille ; même chose pour
`p1p3`.

Le chemin courant :

- refuse les deux supports q2 à cause des extra-shells ;
- n'émet aucun support q3, car les quatre triangles sont rectangles ;
- ne produit donc aucun événement `K=2` au niveau `R²=100`.

La forêt construite sur le sous-flux régulier laisse les quatre côtés séparés,
alors que la filtration de Čech les fusionne. Le défaut ne porte donc ni sur
le rendu, ni sur une convention cosmétique : il change les composantes HGP.

Cette fixture doit devenir une porte permanente du futur fold :

```text
square_cospherical_K2_plateau
```

avec un mutant

```text
drop_shell_plateau
```

qui doit laisser quatre composantes au lieu d'une et mourir.

---

## 2. Forme exacte d'un plateau sphérique sans position générale

Soit une boule `B` de centre `c`, et posons

```text
I_B = X intersect interior(B),
U_B = X intersect boundary(B).
```

Fixons un ordre `K`. Les `K`-simplexes de Gabriel dont la miniboule est
exactement `B` sont précisément les ensembles

```text
sigma = I_B union T
```

où

```text
T subset U_B,
|T| = K+1-|I_B|,
c in conv(T).
```

### Preuve

Si `B_sigma=B` et `sigma` est de Gabriel, tout point de `I_B` doit appartenir à
`sigma`, sinon il serait un point extérieur dans la miniboule ouverte. Les
autres sommets de `sigma` sont sur la coquille, donc `sigma=I_B union T`. Une
boule contenant des points de coquille `T` est leur miniboule exactement
lorsque son centre appartient à `conv(T)`, ce qui donne la condition.

Réciproquement, si `sigma=I_B union T` et `c in conv(T)`, la miniboule de
`sigma` est `B`. Tous les points intérieurs de `B` sont déjà dans `sigma`, et
les points omis sont extérieurs ou sur la coquille : `sigma` est de Gabriel.

Sous position générale, `U_B` coïncide avec l'unique support minimal
`S`, de cardinal `q<=4`, et l'on retrouve la règle actuelle

```text
K = |I_B| + q - 1.
```

Sans position générale, cette règle est fausse : des points supplémentaires de
`U_B` produisent des simplexes d'ordres plus élevés au même niveau. La fixture
du carré est le cas `|I_B|=0`, `|U_B|=4`, `K=2`.

Par Carathéodory en dimension trois,

```text
c in conv(T)
```

équivaut à l'existence dans `T` d'un support minimal de cardinal `2`, `3` ou
`4`. Les lanes q2/q3/q4 restent donc les générateurs géométriques locaux, mais
elles ne peuvent plus publier chacune un événement isolé en ignorant le reste
de la coquille.

---

## 3. ABI exacte recommandée : `SpherePlateau`

Le raccord minimal avant la forêt est un objet commun aux trois lanes :

```cpp
struct SpherePlateau {
  BallKey ball;
  ExactLevel level;
  SmallIds interior;       // I_B
  ShellIds shell;          // U_B complet, supports inclus
  MinimalSupportRuns supports2_3_4;
};
```

Pipeline :

1. former les `BallKey` depuis les supports minimaux q2/q3/q4 ;
2. `sort/RLE` par `BallKey` ;
3. effectuer **un seul census complet par BallKey** et collecter `I_B` et
   `U_B`, pas seulement un booléen `shell>0` ;
4. en régime régulier, `|U_B|=q` et le chemin rapide actuel est inchangé ;
5. en régime dégénéré, traiter le plateau simultanément pour chaque `K` selon
   la formule du § 2.

Pour commencer sans inventer prématurément une structure compliquée :

- oracle borné : énumérer les sous-ensembles `T subset U_B` pour petits
  `|U_B|` et construire les macro-fusions exactes ;
- production : accepter un plafond explicite de coquille, avec
  `resource_exhausted` au-delà, jamais une troncature ;
- ensuite seulement, compresser la famille montante
  `{T : c in conv(T)}` par ses supports minimaux et une représentation
  implicite.

Le fold doit geler les composantes avant le niveau, puis appliquer ensemble
tous les simplexes du plateau. Un ordre binaire entre triangles cosphériques
serait aussi faux que leur suppression, simplement avec davantage de lignes de
code.

---

## 4. Deux sémantiques honnêtes possibles

### Option A — l'objet normatif est réellement le nuage u16

Alors le quotient `SpherePlateau` est indispensable. Le nombre `837` montre que
le chemin dégénéré n'est pas une annexe théorique. Tant qu'il n'est pas reçu :

```text
une coquille pertinente => unsupported_degeneracy,
aucune forêt partielle publiée.
```

Le sous-flux `regular_subset_diagnostic` peut rester un outil de mesure, mais ne
doit jamais alimenter une sortie qualifiée d'exacte.

### Option B — u16 n'est qu'un index spatial

Si l'objet scientifique visé est le scan LiDAR avant quantification, conserver
les clés Morton et les cellules en u16, mais évaluer les prédicats avec des
coordonnées géométriques plus fines et exactes, par exemple un entier fixe
`int32` dérivé des coordonnées sources. Les boîtes u16 restent des bornes
fail-open ; les décisions utilisent les coordonnées de vérité.

Ce n'est pas un jitter : aucune coordonnée n'est perturbée aléatoirement. Mais
c'est un autre profil d'entrée, qui doit être nommé et dont les largeurs q3/q4
doivent être reprises. Il ne faut pas prétendre calculer exactement le HGP du
nuage u16 tout en utilisant en secret une autre géométrie, spécialité humaine
déjà suffisamment répandue.

---

## 5. Ordre de travail demandé avant la forêt publique

1. Graver la fixture carrée et le mutant `drop_shell_plateau`.
2. Mesurer le **taux de runs complets** sous `--exact`, pas seulement le nombre
   de supports refusés. Un seul shell rend aujourd'hui tout le run
   `unsupported_degeneracy`.
3. Décider explicitement entre sémantique u16 exacte et coordonnées de vérité
   plus fines.
4. Si u16 reste normatif, introduire `SpherePlateau` et l'oracle borné des
   sous-ensembles de coquille.
5. Construire le fold régulier en parallèle si utile, mais conserver son statut
   `complete_regular_only` jusqu'à la porte dégénérée.

---

## Conclusion

La sélection axiale n'appelle aucune correction utile et la lane q2 est bien
implémentée dans son domaine. Mais son reçu invalide l'idée que la position
générale puisse rester une simple précondition pratique du profil u16.

Le prochain verrou n'est donc pas encore Kruskal. C'est la sémantique exacte
des plateaux sphériques. Sans elle, la couche événementielle est complète
seulement pour une classe de nuages que le générateur u16 courant quitte déjà à
`n=400`; avec elle, les trois lanes deviennent au contraire les générateurs
naturels d'un quotient local commun et la forêt peut enfin traiter les niveaux
égaux sans perdre l'objet du manuscrit.
