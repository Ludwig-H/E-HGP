# Durée couverte : réserves vérifiées, et correction pour K2

30 septembre 2026. Audit mathématique de petits nuages euclidiens en 3D,
arithmétique Fraction. Aucun moteur invoqué ou modifié, aucune compilation,
aucun GCP. Ces contrôles portent sur la structure et les incidences, pas sur
des scores statistiques, EOM ou le contrat FULL 100 ms.

## Résultat principal

La réserve « certains points ne sont jamais couverts par une feuille de
naissance » doit être **corrigée pour K2** : elle est impossible pour des
sites distincts. Elle est en revanche réelle dès K3. Un nuage de six sites
de dimension affine trois en donne un contre-exemple. Un second exemple
de sept sites le confirme à K5, ordre du contrat prioritaire.

### Preuve K2, indépendante des fixtures

Pour x, choisir un plus proche voisin a et poser d=|a−x|>0. La boule fermée
de diamètre xa ne contient aucun autre site. En effet, z dans cette boule
vérifie `|z−x|²≤(a−x)·(z−x)≤d|z−x|`. Donc `|z−x|≤d`, et l'égalité d>0
impose z=a. Tout z distinct de x,a y serait strictement plus proche de x
que a, contradiction. Cela couvre aussi les égalités entre voisins les
plus proches : un autre voisin à distance d n'est pas sur cette coquille.

La 2-partie `{x,a}` naît dans Γ2 à β=d²/4. Toute 3-partie la contenant a
une MEB de rayon strictement supérieur : une boule de rayon d/2 contenant
x et a a nécessairement leur milieu pour centre, mais ne contient pas le
troisième site. Le sommet Γ2 est donc isolé à sa naissance. Il crée une
vraie feuille couvrant x, avec une durée positive avant toute fusion.
Chaque point a ainsi au moins une incidence de feuille. Cette preuve ne
qualifie ni la majorité de masse choisie, ni sa stabilité statistique.

Le script contrôle ce raisonnement sur 18 points de trois fixtures,
dont un nuage à égalités symétriques de distances. La preuve générale
repose sur l'argument précédent, pas sur la seule réussite de ces contrôles.

## Contre-exemple K3 : six sites u18, dimension affine trois

IDs dans l'ordre du fichier :

`x=(15,4,0), a=(5,4,0), b=(7,8,0), c=(7,0,0), e=(1,4,0), f=(0,4,1)`.

C'est la translation (+10,+4,0) du nuage de construction. Les quatre
feuilles de Γ3 naissent avec les couvertures suivantes :

- β=13/2 : `{a,e,f}` ;
- β=13 : `{a,b,e}` et `{a,c,e}` ;
- β=16 : `{a,b,c}`.

Les trois premières fusionnent à β=33/2 ; leur branche fusionne avec la
dernière à β=169/9. **Toutes les feuilles sont mortes avant que x soit
couvert.** À β=25, x entre dans cette branche interne, sans naissance
d'une nouvelle feuille. Les coupes Γ complètes et les premières dates de
couverture de chaque nœud sont dans les reçus.

Le catalogue critique fort couvrant x contient exactement une boule :
β=25, intérieur vide, coquille `{x,a,b,c}`, q_min=2. À K3, sa structure
locale est un `join`, représenté par `{a,b,c}`, pas une naissance.
Les grandes boules impliquant e ou f sont trop profondes pour créer une
feuille couvrant x. La vérification ne suppose pas cette dernière phrase :
elle examine tous les supports critiques et toutes les coupes Γ.

## Contre-exemple K5 : sept sites, dimension affine trois

Avant la translation (+325,+325,+325) :

`x=(0,0,325)` et les six sites
`(195,0,-260),(-125,0,-300),(0,91,-312),`
`(0,-195,-260),(117,156,-260),(-75,-100,-300)`.

Tous sont sur la sphère de rayon 325. Les trois feuilles sur les six sites
inférieurs naissent à β=6865625/261,1795625/66,68445/2. Elles fusionnent
à β=116715625/3409. x est couvert seulement à β=105625, sur cette branche
interne. Il n'a aucune incidence de feuille, même en permettant qu'une
feuille acquière de nouveaux points après sa naissance et avant sa mort.

L'unique boule critique forte couvrant x est la sphère commune, p=0,
q_min=3, coquille de sept sites. À K5, sa structure locale est un `join`.
Ces deux contre-exemples imposent donc un traitement des couvertures de
branches internes : supprimer tous les ancêtres du dénominateur ne donne
pas une projection complète pour K≥3.

## Branches fantômes : segmentation réelle d'un arc long

Reprendre le nuage K5, multiplier les coordonnées par N=64,128,256,
translater par (+325N,+325N,+325N), puis déplacer seulement x d'une unité
vers le bas. Les deux nuages sont dans u18 ; ils ont une dimension affine
trois. Toutes les K-parties et cofaces K+1 restent dans l'oracle Γ : aucune
fusion n'est supprimée par le filtre fort de couverture.

Le nuage exact avait un arc racine long, né à
β/N²=116715625/3409 et continuant jusqu'à l'infini. Après déplacement,
plusieurs petites feuilles proches de la sphère commune apparaissent puis
fusionnent avec cet arc. Celui-ci meurt maintenant à un niveau normalisé
105620,260 / 105622,630 / 105623,815 selon N. De nouveaux ancêtres prennent
la continuation. Le découpage en arcs a changé, pas le fait que cette
composante continue dans ses ancêtres.

Avec φ=1/β, la somme des nouvelles persistances de feuilles, dans les
coordonnées normalisées, vaut environ 2,601e−11 / 1,301e−11 / 6,502e−12.
En revanche, oublier la continuation ancestrale retire à l'ancien arc une
masse d'environ 9,468e−6 dans les trois cas. Les valeurs rationnelles
exactes et les traces avant/après figurent dans les reçus.

Pour le modèle réel, les rayons MEB des parties contenant x varient
continûment et tendent tous vers 325 quand le déplacement tend vers zéro.
Les naissances/morts des nouveaux petits arcs ont donc cette même limite :
leur persistance tend vers zéro. La nouvelle mort de l'ancien arc tend
aussi vers 325, donc la masse de continuation qu'on oublierait tend vers
φ(β=105625)=1/105625>0 pour ce choix φ=1/β. Sur la grille finie,
les trois N sont seulement trois contrôles
d'amplification, pas une limite continue dans u18.

**Cela ne réfute pas toute pondération par persistance de branche.**
Sommer correctement les segments d'une même continuation télescope leurs
persistances. Le défaut est de traiter chaque arc comme une unité nouvelle,
puis d'exclure des ancêtres ou de doublonner leur masse sans règle cohérente.
Ce contrôle ghost porte sur un arc interne long ; ce n'est pas une preuve
d'instabilité d'une tête durationleaf complète, qui est déjà indéfinie sur
x dans le nuage original.

## Capture et limites de preuve

Le constructeur Γ du script utilise toutes les K-parties et K+1-parties,
traite les plateaux en un seul événement, exporte chaque nœud et toutes
ses premières couvertures. Ses couvertures sont contre-vérifiées avec
`gamma_cuts` du référentiel figé. Les deux chemins partagent les primitives
Fraction/MEB : ce n'est pas un second oracle arithmétique indépendant.

Normal et `python3 -O` terminent avec code 0 et les mêmes résultats
sémantiques. Sources et référence sont hachées avant/après. Le code 0
signifie que ces preuves et contre-exemples sont reproduits, pas qu'un
nouveau clusterer, score EOM ou coût de production est qualifié.

Rejeu : `python3 -B check_reserves.py frozen_reference.py`.
