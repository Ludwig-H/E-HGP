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

**Degré-Rips** — le nom de cet objet en analyse topologique des données
(Lesnick et Wright, 2015), décrit comme *parameter-free and density-sensitive*.
Rolle et Scoccola (JMLR 2024) : ses tranches à un paramètre sont **instables**,
l'objet multiparamètre est **stable**.

**Ultramétrique** — la distance $r_{uv}$ à laquelle deux nœuds fusionnent.
L'équivalence dendrogramme–ultramétrique est établie au chapitre 3 du
manuscrit ; c'est elle qui fonde le biais d'attention ultramétrique.

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
triangle aigu, $4$ tétraèdre. L'alphabet géométrique, et une signature de
dimension locale gratuite.

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

**Chemin dans le treillis** — la direction de grossissement : horizontal ($r$),
vertical ($K$), ou **diagonal iso-densité** ($K/r^3$ constant).

**Condensation** — l'élagage de HDBSCAN, prescrit par le § 9.1 du manuscrit :
une scission n'est une vraie scission que si les deux branches gardent assez de
masse ; sinon la branche survivante garde l'identité du parent. Le § 4.4.3
observe que c'est en réalité un **seuil de percolation**. Ici le seuil doit être
**relatif** (une fraction $\alpha$ de la masse du parent), jamais un nombre de
points.

**Masse $m_\tau$** — $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$, le poids que le
§ 9.1 impose d'utiliser à la place d'un comptage de faces dans l'arbre
condensé. Chaque point distribue une masse totale de $1$.

**Stabilité, excès de masse** — $\widehat{E}(C) \propto \sum_{x \in C} (\hat\lambda_x - \hat\lambda_{\min})$,
Def. 19 du manuscrit. Sert d'ordre de contraction pour l'échelle, et de coût de
référence pour la tête de sélection.

**Niveau de sortie $\hat\lambda_x$** — la densité à laquelle un point quitte son
nœud condensé. Ce que la condensation retire de la structure, elle le rend sous
cette forme : c'est un changement de représentation, pas une perte.

**FP, pooling de filtration** — $h_\ell = \phi(P_\ell^\top h_{\ell-1} W_\ell)$
avec $P_\ell$ l'affectation douce aux poids du § 9.1.

**MGA, attention sur graphe de fusion** — attention restreinte aux nœuds qui
fusionnent, avec **biais ultramétrique** $\varphi(\log r_{uv} - \log r_u)$.

**OM, mixage d'ordres** — fusion des représentations à plusieurs $K$ via les
cartes verticales. Trois réalisations : calendrier de $K$ selon la profondeur,
attention croisée au goulot, branches parallèles.

**PUR, lecture par partition de l'unité** — le décodeur du § 9.1.

**SEL, tête de sélection apprise** — le programme dynamique ascendant du § 5.2
(« hacker HDBSCAN ») : conserver le père si
$\mathrm{loss}(\text{père}) < \sum_i \mathrm{loss}(\text{fils}_i)$. Avec
$\mathrm{loss} = -\widehat{E}$ c'est HDBSCAN ; avec $\mathrm{loss} = -g_\theta(h_C)$
c'est une tête d'instance apprise qui rend une antichaîne par construction,
sans suppression non maximale ni appariement.

**FM, modélisation de filtration** — les cinq tâches de pré-entraînement aux
cibles exactes : fusion, persistance, profil en $K$, rétablissement, accord
inter-vues.

**Masquage par nœuds entiers** — masquer une pièce géométrique cohérente au
lieu de points au hasard, comme le masquage de mots entiers en langage.

**Raccourci géométrique** — l'effondrement de la SSL 3D sur des indices
spatiaux de bas niveau, diagnostiqué par Sonata. Les cibles de filtration n'y
sont pas sujettes parce qu'elles sont non locales.

## Mesure

**Substitution** — remplacer un seul composant, tout le reste fixé. La seule
mesure qui attribue.

**Budget apparié** — mêmes paramètres, époques, augmentations et matériel.

**Témoin négatif** — expérience qui a le pouvoir d'annuler la conclusion :
tour brouillée (T1), canal de densité seul (T2), niveaux permutés (T3), ordre
aléatoire (T4), diagnostic du raccourci (T5).

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
