# Obstruction locale robuste, puis raccord Palm conditionnel

Cadre : preuve mathématique sur sites distincts de poids 1, ordre k = 2,
qualification m = 3, pendaison P₁Π₃ en **rayon**. Aucun moteur, ajustement,
simulation Poisson ou test G4 n’est exécuté. Ce reçu est distinct des réponses
Q1–Q8 déjà closes. `check.py` vérifie 28 gardes exactes avec un oracle Gram/Γ
indépendant figé ; ses sorties normal et `-O` sont identiques.

## Certificat valable quel que soit l’extérieur

Le point Palm x = (0,0,0) reste fixe. Le patron est

```
x=(0,0,0), y=(10,0,0), s₂=(20,0,0), s₃=(30,0,0),
b₁=(43,0,0), b₂=(53,0,0), w₊=(-20,10,0), w₋=(-20,-10,0).
```

On prend F = 12, R = 15, et une perturbation de norme < δ = 1/10 pour
chacun des sept sites autres que x. On interdit tout autre site dans
G = B̄(x,31) ∪ B̄(w₊,31) ∪ B̄(w₋,31), les centres de G étant ceux du
patron. Une version plus forte et commode pour le raccord interdit les
autres sites dans Q = [-52,32] × [-42,42] × [-32,32], qui contient G.
Les boules prescrites autour de b₁ et b₂ sont hors de Q.

Pour r < 10 − δ, la seule paire active incidente à x peut être {x,y} :
tout autre site local est distant d’au moins 20 − 2δ de x, et tout site
extérieur est encore plus éloigné. Un premier passage de Γ₂ depuis cette
paire vers une autre paire exige une coface {x,y,z}, donc une paire active
{x,z}, impossible à ces rayons. La couverture de cette composante a donc
seulement deux sites. Une couverture de x par une paire ne contenant pas x
n’échappe pas à cet argument : l’ajout de x fournit une coface active et
une paire incidente à x dans la même composante. Ainsi t′ ≥ 10 − δ.
La coface {x,y,s₂} a rayon ≤ 10 + δ : la première qualification se produit
dans la lignée primaire, avant toute paire incidente à w₊ ou w₋ couvrant x.

Les quatre cofaces successives du patron primaire ont rayons
10, 10, 23/2, 23/2. Leurs rayons après perturbation sont au plus 23/2 + δ < F.
La primaire contient donc x et le port b₂ à F. De plus d₂(x) ≤ 10 + δ < F :
x est un site **core** de cette composante, contenant au moins six sites.

Le rival porté par S = {x,w₊,w₋} devient qualifié avant
c = 25/2 + δ < R. Jusqu’à R, sa composante Γ₂ ne contient que des paires
de S. Toute paire de S contient un w. Un premier passage vers un support
extérieur à S impose une coface contenant un w et un z hors de S.
Or leur distance est > 2R : pour les cinq sites primaires autres que x,
la distance minimale du patron vaut √1000 et √1000 − 2δ > 30 ; pour les
sites étrangers, elle est ≥ 31 − δ > 30. Même les intersections de lentilles
sans site partagé nécessitent cette union de supports, donc ne permettent
pas un passage. Le rival et la primaire restent distincts à R, indépendamment
du nombre et de la géométrie de tous les sites extérieurs.

Si p est un premier propriétaire qualifié primaire et q ce rival, leur
réunion M(p,q) est au moins R. La marge D(x) de P₁Π₃ satisfait donc

    e(x) = t′(x) + D(x)
         ≥ (10 − δ) + [R − (25/2 + δ)] = 25/2 − 2δ = 123/10 > F.

Toutes ces inégalités ont une marge stricte ; δ < 1/6 suffit à conserver
les marges utilisées ici. Les trois ajouts étrangers du programme illustrent
le raccord, sans remplacer la preuve valable pour tout extérieur.

## Probabilité Palm positive, sous hypothèse de percolation

Soit un Poisson homogène d’intensité λ > 0 dans ℝ³, sous sa loi Palm
Πλ ∪ {x}. On prescrit un point dans chacune des sept boules ouvertes
B(site,δ), aucune autre présence dans D = Q ∪ ⋃ B(site,δ). Les boules sont
disjointes. Si vδ est leur volume, cet événement a probabilité

    exp(−λ |D|) λ⁷ vδ⁷ > 0.

Il ne reste pas à supposer une composante infinie **unique**. On suppose
qu’à ces λ et F fixés, FULL₂ possède une composante non bornée avec
probabilité positive. Son nerf Γ₂ est localement fini presque sûrement.
Retirer les points de D retire un nombre fini de sommets et de voisins
de ce nerf. Dans un graphe connexe infini localement fini, retirer un
ensemble fini laisse au moins une composante infinie. Une composante
infinie persiste donc dans l’extérieur, dont la loi est indépendante de
l’événement local prescrit.

Le port b₂ est hors de Q. On le relie à une paire active de cette composante
par une chaîne finie dans l’extérieur de Q et des boules prescrites, avec
des cofaces de rayon strictement < F. Le complément de cette boîte et de
ces boules disjointes est connexe dans ℝ³. Des pas assez petits suffisent ;
la première coface utilise b₁,b₂ et le premier ajout extérieur. Pour le
dernier raccord, une paire active {a,b} de la composante infinie a
|a−b| < 2F presque sûrement ; un ajout assez proche de a donne une coface
{a,b,c} de rayon < F. Tous les ajouts sont hors de G, donc ne brisent aucun
certificat de retard.

Des suites finies de boules à données rationnelles forment une famille
dénombrable de raccords robustes possibles. Sur l’événement de composante
infinie extérieure, l’une de ces suites convient ; au moins une suite a
donc une probabilité de compatibilité strictement positive. Insérer des
points uniformes dans ses boules disjointes est absolument continu par
rapport à la loi Poisson extérieure : pour une boule B, la densité de
Πλ + U_B relativement à Πλ vaut N(B)/(λ|B|), et l’argument s’itère.
Cela transfère la probabilité positive du raccord inséré vers un événement
du Poisson lui-même ; une insertion déterministe, singulière, ne suffirait
pas. On obtient une probabilité Palm strictement positive d’avoir x core
d’une composante FULL₂ non bornée à F mais e(x) > F.

## Conséquence et limites

**Si** H∞ est défini, mesurable et une pendaison qualifiée fidèle respectant
ce certificat local, notons Θpoly la probabilité Palm d’être couvert par
une composante FULL₂ non bornée à F, et ΘH celle d’être actif et attaché
à une telle composante. La fidélité donne ΘH ≤ Θpoly, et l’événement
construit donne strictement **ΘH < Θpoly**, aux mêmes λ et F fixés.
Si ΘH compte plutôt les blocs infinis de points, la même borne supérieure
s’applique : un bloc fidèle infini ne peut être porté par une composante
FULL bornée d’un processus localement fini.

Ce n’est pas une construction ou une preuve de mesurabilité de H∞, ni
un théorème de convergence en fenêtre finie. Aucun déficit uniforme lorsque
λ varie n’est établi : le coût probabiliste du vide dépend de λ. Ce résultat
ne compare pas les méthodes avant leurs **propres** seuils critiques ou
dates de fusion, et ne transfère donc pas automatiquement le théorème 3
du chapitre 7 de la thèse à H∞.

La source primaire locale est épinglée dans `SOURCE_PINS.json` : définition
23, page imprimée 65 / PDF 91 (appartenance à une composante infinie, sans
unicité supposée) ; poly et union des composantes infinies, page 66 / PDF 92 ;
théorème 3, pages 71–72 / PDF 97–98 (seuil critique propre et hypothèses de
continuité). L’extrait conservé est `PRIMARY_CH7_EXCERPT.txt`. La contrelecture
adverse de geometry_catalogue confirme les passages Γ₂, la protection de S
et le raccord, avec les mêmes réserves de régime fixé et de limite infinie.

Rejeu : `python -B check.py` et `python -B -O check.py`. Les hashes de
fermeture couvrent tous les fichiers, sauf le manifeste racine lui-même.
