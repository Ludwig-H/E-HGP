# Glossaire

Les termes du manuscrit, de la v9 et du modèle, dans un seul endroit.

## Objet mathématique

**Amas discret de forte densité** — ensemble des points de $\mathcal{X}$ situés
à distance au plus $r$ d'une composante connexe de
$L_K(r) = \lbrace y : |B(y,r) \cap \mathcal{X}| \geq K \rbrace$. C'est ce que la
hiérarchie estime, exactement (Théorème 2).

**Complexe de Čech $\check{C}(\mathcal{X}, r)$** — $\sigma$ en est un simplexe
ssi les boules de rayon $r$ centrées sur ses sommets ont un point commun.

**$K$-polyèdre** — ensemble des points apparaissant dans une composante connexe
du graphe $\Gamma_K(\mathcal{X}, r)$, dont les sommets sont les
$(K-1)$-simplexes et les arêtes les $K$-simplexes. Pour $K = 1$, c'est une
composante connexe ordinaire : le Single-Linkage.

**HGP-Clusterer** — Hypergraphe-Percol', l'algorithme qui suit l'évolution des
$K$-polyèdres le long de la filtration (Def. 22).

**Rayon de naissance $\rho(\sigma)$** — rayon de la plus petite boule englobante
(*miniball*) de $\sigma$ ; niveau d'apparition de $\sigma$ dans la filtration.

**Simplexe de Gabriel** — simplexe dont l'intérieur de la miniball ne contient
aucun point extérieur. Tout simplexe séparant est de Gabriel (Théorème 4), et
tout $K$-simplexe de Gabriel est porté par la mosaïque de Delaunay d'ordre $K$
(Théorème 6).

**Simplexe $K$-séparant** — simplexe qui provoque effectivement une fusion de
deux $K$-polyèdres à sa naissance. Généralise l'arête de l'arbre minimum
couvrant.

**Multifusion** — événement où trois composantes ou plus fusionnent au même
niveau exact. La v9 les publie explicitement (deux parents ou plus).

**Bifiltration $(K, r)$** — la tour est indexée par deux paramètres : l'ordre
$K$ et le rayon $r$. À $K$ fixé on lit un arbre de fusion ; à $r$ fixé, une
chaîne emboîtée en $K$, puisque $L_K(r) \subseteq L_{K-1}(r)$.

## Tour v9

**Tour FULL** — pour chaque ordre $K = 1 \ldots K_{\max}$ : la forêt de fusion
(minima de Gabriel, multifusions, parents), les populations, et les verticales.

**Catalogue** — l'ensemble des boules minimales exactes. Une boule porte sa clé
primitive, son niveau exact, son **arité**, ses intérieurs stricts et sa
coquille.

**Arité $q_{\min}$** — taille du support minimal d'une boule : $2$ (arête
diamétrale), $3$ (triangle aigu), $4$ (tétraèdre). L'alphabet géométrique du
projet.

**Support / coquille / intérieur** — le support est le sous-ensemble du nuage
qui détermine la miniball ; la coquille est l'ensemble des sites sur la sphère ;
l'intérieur, ceux strictement dedans. Plafonds v9 : intérieurs $\leq 9$,
coquille $\leq 12$, au-delà **refus de domaine** explicite.

**Verticale (`lower_nodes`)** — image d'un nœud d'ordre $K$ dans l'histoire
d'ordre $K-1$, prise au niveau de création fermé du nœud.

**Niveau exact (`ExactLevel`)** — le rayon **au carré**, comme fraction non
réduite en U192/i128. Jamais un flottant, jamais un rayon.

**`complete_relative`** — statut publié par la chaîne : complet *relativement*
au catalogue recoupé et scellé. Ce n'est pas une preuve de complétude Gabriel
absolue.

**`tower_digest`** — condensé FNV-64 canonique de toute la tour. C'est la preuve
de reproductibilité du tokenizer.

## Modèle

**HGP-Tok** — le tokenizer : tour FULL, condensation § 9.1, sélection par excès
de masse, réalisation géométrique, descripteur. Exact, déterministe, non appris.

**HGP-FM** — le modèle complet : HGP-Tok, encodeur de jeton, backbone à trois
canaux d'attention, lecture au point par § 9.1.

**Bande $L$** — l'antichaîne d'excès de masse plus ses $L$ ancêtres et $L$
descendants condensés ; le paramètre qui décide du budget de jetons.

**Canaux d'attention** — latérale (voisinage spatial), verticale
(parents/enfants d'un même ordre), d'ordre (cartes $K \leftrightarrow K-1$).

**Modélisation de filtration** — objectif de pré-entraînement propre au projet :
prédire des grandeurs masquées de la tour (niveau de mort, partenaire de
fusion, ordre d'apparition, arité, degré de multifusion). Cibles géométriques
exactes et gratuites.

**Vote pondéré § 9.1** — $w_{x\tau} = S_\tau / T_x$ avec
$S_\tau = \sum_{\sigma \supset \tau} \rho(\sigma)^{-p}$ et
$T_x = \sum_{\tau \ni x} S_\tau$ ; chaque point distribue une masse totale de 1
entre les facettes qui le contiennent. Version douce pour l'entraînement,
version dure (Proposition 7) pour l'évaluation en partition.

## Protocole

**Porte** — une question, un coût, un critère de promotion et un critère de
**réfutation** écrit avant l'expérience.

**Porte de réfutation** — porte qui ne peut que disqualifier, jamais promouvoir.
L'oracle d'instances (G2) en est une.

**Reçu** — capture immuable ancrée au commit : condensés, sha256 du jeu de
données, configuration, graines, matériel, sorties brutes. Une mesure sans reçu
n'existe pas.

**Témoin apparié** — comparaison à même nombre de paramètres, mêmes époques,
mêmes augmentations, même matériel.

**Régime strict** — protocole de comparaison sur SemanticKITTI où l'on se
compare à l'état de l'art et jamais au « scratch ».
