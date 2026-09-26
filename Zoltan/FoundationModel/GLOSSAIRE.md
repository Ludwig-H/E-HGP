# Glossaire

Les termes du manuscrit, de la tour, de l'architecture et de la mesure.

## Objet mathématique

**Amas discret de forte densité** — les points situés à distance au plus $r$
d'une composante connexe de
$L_K(r) = \lbrace y : |B(y,r) \cap \mathcal{X}| \geq K \rbrace$. Ce que la
hiérarchie estime, exactement (Théorème 2).

**$K$-polyèdre** — l'ensemble des points d'une composante connexe du graphe
$\Gamma_K(\mathcal{X}, r)$, dont les sommets sont les $(K-1)$-simplexes et les
arêtes les $K$-simplexes. Pour $K = 1$ : une composante connexe ordinaire, donc
le Single-Linkage.

**Simplexe de Gabriel** — simplexe dont l'intérieur de la plus petite boule
englobante ne contient aucun point extérieur. Tout simplexe séparant l'est
(Théorème 4), et tout $K$-simplexe de Gabriel est porté par la mosaïque de
Delaunay d'ordre $K$ (Théorème 6).

**Rayon de naissance $\rho(\sigma)$** — rayon de la plus petite boule englobante
de $\sigma$ ; niveau d'apparition dans la filtration.

**Multifusion** — événement où trois composantes ou plus fusionnent au même
niveau exact.

**Bifiltration $(K, r)$** — l'objet à deux paramètres : $L_K(r)$ croît en $r$ et
décroît en $K$. À $K$ fixé on lit un arbre de fusion ; à $r$ fixé, une chaîne
emboîtée en $K$.

**Tour Morse HGP FULL** — forêts de fusion exactes pour les ordres K,
populations et cartes verticales, avec les niveaux de naissance et de fusion
spécifiés par le manuscrit. Une coupe consommée par le réseau est dérivée de
cet objet et requiert sa propre vérification.

**Ultramétrique de fusion** — à K fixé, rayon de fusion entre deux éléments
distincts d'une même frontière d'arbre, avec diagonale nulle. Le rayon d'une
fusion hors horizon est censuré. Le biais appris à partir de ce rayon et de
l'échelle propre d'un jeton n'est pas lui-même une ultramétrique.

**Percolation** — l'outil d'analyse du chapitre 7. $K$ contrôle la résistance
aux ponts de bruit ; le Théorème 3 chiffre la fraction récupérable avant fusion
parasite. C'est ce qui fait de $K$ un bouton sensibilité/robustesse.

**Vote pondéré § 9.1** — $w_{x\tau} = S_\tau / T_x$ avec
$S_\tau = \sum_{\sigma \supset \tau} \rho(\sigma)^{-p}$ et
$T_x = \sum_{\tau \ni x} S_\tau$ ; chaque point distribue une masse totale de
$1$. Version douce en entraînement, version dure (Proposition 7) en inférence.

## Tour

**Tour FULL** — pour chaque ordre : la forêt de fusion, les populations et les
cartes verticales.

**Catalogue** — les boules minimales exactes : clé primitive, niveau exact,
arité, intérieurs, coquille.

**Arité $q_{\min}$** — taille du support minimal : $2$ arête diamétrale, $3$
triangle aigu, $4$ tétraèdre. L'alphabet géométrique fournit une signature
d'arité des supports, à exporter ; ce n'est pas la dimension de l'objet vu.

**Carte verticale** — l'image d'un nœud d'ordre $K$ dans l'histoire d'ordre
$K-1$. **Naturelle** : l'image d'une fusion est la fusion des images.

**Niveau exact** — le rayon **au carré**, en fraction entière. Jamais un
flottant, jamais un rayon.

## Architecture

**HGP-UNet** — le squelette : un U-Net/Transformer dont le pooling, le
voisinage, le biais de position et le décodeur sont lus dans la bifiltration.
Réalisé comme une modification de PTv3, jamais comme un réseau neuf.

**HGP-FM** — le modèle pré-entraîné.

**Échelle (de recouvrements)** — la suite $A_1 \succ \cdots \succ A_L$ de
coupes emboîtées lue dans la forêt. Remplace le *grid pooling*.

**Règle de contraction** — comment on passe d'un niveau au suivant :
`E-global`, `E-rang`, `E-persistance` (défaut), `E-relative`.

**Chemin dans le treillis** — la direction de grossissement emboîtante :
horizontale ($r$ croît), verticale ($K$ décroît) ou anti-diagonale combinant
les deux. L'iso-densité compare des branches latérales ; elle ne définit pas
un pooling emboîtant.

**Condensation** — l'élagage adapté aux masses du § 9.1 du manuscrit :
dans l'adaptation aux multifusions, zéro branche lourde termine le segment,
une seule poursuit son identité, plusieurs créent une scission simultanée.
Le seuil relatif α est une variante à tester, distincte du seuil absolu du
manuscrit, et les masses écartées restent en réserve. Un seuil relatif au
parent n'impose pas une masse minimale globale : un arbre binaire équilibré
peut rester intact jusqu'aux singletons.

**Masse $m_\tau$** — $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$, le poids que le
§ 9.1 impose d'utiliser à la place d'un comptage de faces dans l'arbre
condensé. Chaque point distribue une masse totale de $1$.

**Stabilité, excès de masse** — intégrale de la masse du segment dans la
coordonnée de densité, calculée par durées de présence des incidences à poids
gelés. Sert de variante d'ordre de contraction et de coût de référence pour SEL.

**Niveau de sortie** — coordonnée de densité à laquelle une incidence ou une
branche quitte son segment condensé. Un point peut participer à plusieurs
branches avec des sorties différentes ; un scalaire par point est une
agrégation déclarée, qui ne reconstitue pas la structure retirée.

**État de coupe** — segment FULL avec niveau carré exact et côté ouvert/fermé.
Une continuation peut changer la population sans changer l'identifiant du
segment.

**Univers pondéré figé** — atomes représentés, poids et réserves conservés
entre les niveaux d'une branche. Le quotient atomique permet alors de vérifier
$P_g=P_fQ$. Une lecture indépendante de FULL à chaque rayon peut avoir un autre
univers actif ; les deux opérateurs sont distingués dans le
[contrat des coupes](CONTRAT_COUPES_ET_MASSES_20260926.md).

**FP, pooling de filtration** — moyenne pondérée avec transport de la masse :
$M_\ell=P_\ell^\top M_{\ell-1}$ et $h_\ell=\phi(D_{M_\ell}^{-1}P_\ell^\top D_{M_{\ell-1}}h_{\ell-1}W_\ell)$.
Les inverses ne portent que sur les masses positives ; affectation douce à
l'entrée et quotient dur entre niveaux compatibles. La somme brute est une
variante distincte.

**MGA, attention sur graphe de fusion** — attention sur une relation choisie
dans l'histoire des fusions, avec biais $\varphi(\log r_{uv}-\log r_u)$.
Le candidat épars utilise des jetons auxiliaires d'événements ; sa diffusion
en deux passages ne se confond pas avec une attention complète entre frères.

**OM, mixage d'ordres** — fusion des représentations à plusieurs $K$ via les
cartes verticales. Trois réalisations : calendrier de $K$ selon la profondeur,
attention croisée au goulot, branches parallèles.

**PUR, lecture par partition de l'unité** — mélange des probabilités des
jetons avec P, suivi d'un argmax déterministe. Extension linéaire du vote
de labels du § 9.1 ; ce n'est pas un inverse du pooling.

**SEL, sélection dans l'arbre** — programme dynamique à score additif :
$V(C)=\max(g(C),\sum_i V(C_i))$, avec cas terminaux et départage déclarés.
Comparer à l'optimum des sous-arbres, pas aux seuls scores bruts des enfants.
Le choix $g=\widehat{E}$ retrouve le critère d'excès de masse sur le même
arbre. Une antichaîne partitionne les atomes de facettes couverts ; elle ne
garantit ni une instance par objet ni l'absence de fragments après le vote
aux points. Apprendre le meilleur IoU isolé de chaque nœud ne suffit pas à
définir un score additif de bonne segmentation.

**FM, modélisation de filtration** — les six tâches proposées aux
cibles exactes : fusion, persistance, profil en $K$, rétablissement, accord
inter-vues et distillation d'agrégat. Cette dernière est une extension
temporelle, hors régime primaire mono-scan. La cible exacte sur l'enseignant
peut rester incertaine depuis la seule vue élève.

**Masquage par nœuds entiers** — masquer une pièce géométrique cohérente au
lieu de points au hasard, comme le masquage de mots entiers en langage.

**Raccourci géométrique** — l'effondrement de la SSL 3D sur des indices
spatiaux de bas niveau, diagnostiqué par Sonata. Une cible FULL peut être
locale ou révélée par le graphe fourni au modèle. Le contrôle du forward
élève et les témoins locaux sont donc nécessaires, même avec une cible
structurelle.

## Mesure

**Substitution** — remplacer un composant à recette et données appariées
pour attribuer son effet ; déclarer les différences de coût et mesurer ensuite
les interactions.

**Budget apparié** — comparer à exposition aux scans égale puis à coût total
égal, en comptant préparation, export, cache et apprentissage. Paramètres,
époques, augmentations et matériel identiques ne garantissent pas le même
travail.

**Témoin négatif** — expérience qui a le pouvoir d'annuler la conclusion :
tour brouillée (T1), canal de densité seul (T2), niveaux permutés (T3), branches K1 répétées à budget égal (T4), diagnostic du raccourci (T5).

**Prédiction pré-enregistrée** — où le mécanisme dit que l'on doit gagner, et
surtout où il dit que l'on **ne doit pas** gagner (P6).

**Porte de réfutation** — porte qui ne peut que disqualifier, jamais promouvoir.
L'oracle d'instances en est une.

**Sonde linéaire** — squelette gelé, tête `BatchNorm` + linéaire. Le protocole
de référence pour juger une représentation.

**Efficacité en étiquettes** — mIoU à $0{,}1$, $1$, $10$, $50$, $100\,\%$ des
annotations.

**Reçu** — capture immuable ancrée au commit : condensés, sha256 du corpus,
configuration, graines, matériel, sorties brutes, coût. Une mesure sans reçu
n'existe pas.
