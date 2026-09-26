# HGP-UNet — l'architecture

26 septembre 2026. Conception. Aucune expérience apprise n'est rapportée.

Prérequis : [`ETAT_DE_LART.md`](ETAT_DE_LART.md), qui établit ce que la tour
remplace, et [`OBJET.md`](OBJET.md), qui dit ce qu'elle fournit.

Le [contrat des coupes et masses](CONTRAT_COUPES_ET_MASSES_20260926.md)
fixe les interfaces du pilote : états datés, branches K autonomes, univers
pondéré gelé, réserves et composition vérifiée.

Le [réaudit global](REAUDIT_GLOBAL_20260926.md) précise les choix encore
ouverts : réalisation géométrique canonique, graphe d'événements, condensation,
perte de guidage et objectif de sélection. Les variantes sont des hypothèses
à mesurer ; le pilote ne les active pas toutes ensemble.

## 1. La thèse, en un paragraphe

Les encodeurs choisissent une granularité et des voisinages, par paramètres
métriques ou budgets cardinaux : voxel, rayon, k voisins, taille de fenêtre.
Ces choix peuvent mal transférer entre acquisitions. FULL fournit une
structure exacte à deux paramètres dont on peut dériver une partie de ces
choix ; grille, Kmax, coupes et budgets restent à fixer. Deux usages sont
étudiés : substituer un opérateur du réseau et guider l'apprentissage depuis
un enseignant. La stabilité et le gain de chacun se mesurent séparément.

Le modèle qui en découle n'est pas un nouveau réseau exotique. C'est un U-Net /
Transformer ordinaire dont **le pooling, le voisinage, l'encodage de position
relative et le décodeur** sont tous lus dans la bifiltration $(K, r)$. La
nouveauté est dans ce qui structure le calcul, pas dans les couches.

## 2. Correction d'une erreur de cadrage

Une première lecture des chiffres de la tour (1,3 M à 16,3 M nœuds par trame)
conduit naturellement à dire : « il faut sélectionner quelques milliers de
jetons ». **C'est le mauvais cadre, et il faut l'abandonner.**

Un U-Net ne consomme pas l'ensemble des nœuds : il consomme **$L$ coupes**,
c'est-à-dire $L$ recouvrements emboîtés de l'ensemble des points. Avec un
rapport de $4$ par niveau sur une trame brute :

```text
  points   120 000
  niveau 1  30 000
  niveau 2   7 500
  niveau 3   1 900
  niveau 4     470
  niveau 5     120
  total    ~160 000 unités, tous niveaux confondus
```

Ce budget illustratif concerne **une seule branche** et reste à rendre
réalisable. Avec un niveau point partagé et quatre branches ayant chacune
ces cinq niveaux, il devient environ 280 000 unités ; dupliquer aussi le
niveau point le porterait à environ 640 000. Les réserves s'y ajoutent.
Rapporter la somme des jetons par K et niveau, et les arêtes réellement
consommées : un budget par branche n'est pas le budget du modèle.
La v9 matérialise cependant FULL avant que le réseau n'en lise $L$ coupes ;
leur export et leur mise en cache ont aussi un coût. Distinguer donc jetons
consommés, octets de FULL, compilation des coupes et latence totale. La
question architecturale devient **« quel chemin et quelles coupes lire dans
$(K,r)$, à quel coût ? »** ; voir l'[audit transversal](AUDIT_V9_ET_ARCHITECTURE_20260926.md).

## 3. Les six primitives

Le projet construit six opérateurs à partir de FULL et de ses suppléments
d'export. Ils ne sont pas tous des champs déjà disponibles dans la tour.

1. **Une échelle de recouvrements** $A_1 \succ A_2 \succ \cdots \succ A_L$ :
   remplace le *grid pooling*, le FPS et les niveaux de superpoints.
2. **Des matrices d'affectation douces** $P_\ell$, aux poids du § 9.1 :
   remplacent le *pooling* de cellule et conservent la masse. Elles sont
   linéaires dans les activations ; leurs incidences discrètes ne sont pas
   apprises par rétropropagation.
3. **Un graphe épars dérivé des événements de fusion** par niveau, avec
   budget d'arêtes et rayon de fusion. Remplace le graphe
   $k$-NN et la fenêtre sur la sérialisation.
4. **Un axe d'ordre $K$**, avec une carte verticale dont la v9 vérifie la
   naturalité, pour comparer les ordres sur des états datés.
5. **Des scalaires structurels exacts** par nœud : naissance, mort,
   persistance, mélange d'arités, degré de multifusion, population.
   Leur export/agrégation est payé ; les variables enseignantes qui révèlent
   une cible ne sont pas des entrées du forward élève.
6. **Une lecture exacte vers les points** (§ 9.1 et Proposition 7) : remplace
   la lecture standard du décodeur ; pour PTv3, indices inverses et connexion
   de saut, à conserver comme témoin.

## 4. Ce que le treillis permet, et ce qu'il ne permettra jamais

### 4.1 Pourquoi mesurer les limites d'un rayon global

Dans un scan automobile, la densité des retours varie avec la portée,
l'incidence et l'occultation. À rayon global égal, des régions proches et
lointaines peuvent donc être regroupées à des granularités très différentes.
La tour Morse HGP donne les historiques exacts à plusieurs K et r ; elle
permet de comparer une coupe globale à des antichaînes adaptatives sur le
**même** nuage. La valeur d'une échelle adaptative et de plusieurs ordres est
l'hypothèse centrale à tester, pas une conclusion acquise par l'exactitude
de FULL.

### 4.2 Quels chemins sont des échelles, et lesquels n'en sont pas

*Section corrigée. Une première version recommandait un chemin diagonal
iso-densité comme défaut ; il n'est pas emboîtant, donc il ne peut pas être une
échelle de pooling. La correction est plus instructive que l'erreur.*

Notre variante de pooling dur exige une image unique pour chaque bloc fin.
Ce n'est pas une obligation de tout U-Net. Une condition suffisante
d'inclusion des ensembles géométriques est :

$L_K(r) \subseteq L_{K'}(r') \quad \text{dès que} \quad r' \geq r \ \text{ et } \ K' \leq K.$

En effet, si $|B(y, r) \cap \mathcal{X}| \geq K$ et $r' \geq r$, alors
$|B(y, r') \cap \mathcal{X}| \geq K \geq K'$. Et la condition sur $K$ est
essentielle : un point qui a exactement $K$ voisins dans $B(y, r')$ appartient à
$L_K(r')$ mais pas à $L_{K'}(r')$ pour $K' > K$. Le contrat universel retenu
grossit donc par r croissant et K décroissant ; une inclusion fortuite sur
une scène ne remplace pas ce contrat.

Trois familles de chemins sont donc des échelles valides :

| chemin | mouvement | effet |
| --- | --- | --- |
| **horizontal** | $r \uparrow$, $K$ fixé | grossir en espace, avec seuil de densité K-NN décroissant |
| **vertical** | $r$ fixé, $K \downarrow$ | grossir en relâchant l'exigence de densité, à échelle fixée |
| **anti-diagonal** | $r \uparrow$ et $K \downarrow$ | grossissement rapide sur les deux axes |

Et une famille n'en est **pas** une : le chemin **iso-densité**
$\hat f_K \propto K/r^{3}$ constant, qui fait croître $K$ avec $r$. Il est
séduisant — « grossir en espace à densité constante » — mais il est
**sans garantie de monotonie** : ses ensembles peuvent ne pas s'emboîter,
donc aucune application de pooling dure n'est garantie entre ses niveaux.
Une trame particulière peut présenter un emboîtement fortuit, sans fournir
un contrat valable pour le tokenizer.

**Ce que l'iso-densité est vraiment, et où il sert.** C'est une famille
**latérale** : plusieurs lectures de la même scène au même niveau de densité et
à des échelles spatiales différentes. Les cartes verticales s'évaluent à
rayon commun ; relier deux lectures incomparables demande de composer avec
l'histoire horizontale ou de définir une incidence latérale. Aucune carte
unique dure n'en découle automatiquement. OM doit déclarer ce raccord.

### 4.2 bis Pourquoi comparer des branches K autonomes

La monotonie a une conséquence qui change le statut du mixage d'ordres.

Sur un chemin garanti, K est non croissant et peut rester fixé. Un chemin
K1 horizontal parcourt déjà le coin sensible aux structures clairsemées ;
OM n'est donc pas une nécessité mathématique. Il permet de conserver
simultanément plusieurs exigences de couverture.

Or la sémantique tire dans l'autre sens. Un objet mince et lointain — un poteau
à quarante mètres, quinze retours sur deux mètres — naît **tard** à $K$ élevé et
**tôt** à $K$ faible. Il vit donc dans le coin « $K$ petit, $r$ petit » du
treillis. Un calendrier qui commence à K élevé peut le manquer : le poteau
n'est pas encore né à ses niveaux fins et peut déjà avoir fusionné aux niveaux
grossiers. Ce scénario motive des branches autonomes ; il ne garantit pas
qu'un ordre donné isolera l'objet.

**Un chemin monotone unique peut manquer cet objet à ses niveaux retenus.**
Faire tourner **plusieurs branches à des $K$ différents** et les fusionner
latéralement est l'hypothèse OM à comparer à un calendrier unique, notamment
sur les classes filiformes. La prédiction P5 devient donc un test du dispositif
entier, pas d'un module.

Bonne nouvelle d'ingénierie : les cartes verticales de la v9 vont de $K$ vers
$K-1$, c'est-à-dire **exactement dans le sens du grossissement**. Le moteur
publie une application de référence pour changer d'ordre ; le quotient sur
les antichaînes effectivement retenues reste à construire et à vérifier.

### 4.3 La condensation : l'étape que le manuscrit prescrit déjà

La condensation est une variante structurante prescrite dans l'Algorithme 1
du manuscrit, avec une masse adaptée au recouvrement. Le pilote de raccord
vérifie d'abord les coupes globales brutes ; la condensation se compare ensuite
à cette référence, avec le poids prévu par le §9.1.

> « La masse d'une face dans l'arbre condensé est alors
> $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$ […] C'est ce poids $m_\tau$, et non
> le simple comptage des faces, qui est utilisé par le seuil
> `min_cluster_size` dans l'arbre condensé (mêmes idées algorithmiques que
> HDBSCAN). »

**Pourquoi la mesurer.** La capture K1 citée compte 39 796 fusions pour
39 885 sites. Ces comptes ne disent pas si elles sont équilibrées ou dominées
par des rattachements de singletons : un arbre binaire équilibré a aussi
presque autant de fusions que de feuilles. Mesurer masses relatives des
enfants et profondeurs avant de conclure que les premières coupes dépensent
leur budget sur des micro-composantes.

**Ce que la condensation rend.** Quatre choses, dont trois sont des entrées
directes de l'architecture :

1. le **squelette** : les vraies scissions, sans les continuations ;
2. la **stabilité** de chaque nœud, calculée par durées de présence des
   incidences avec poids gelés, comme variante d'ordre de contraction ;
3. les niveaux de sortie **par incidence ou branche**, éventuellement résumés
   en variable par point. Un point peut avoir plusieurs sorties ; le scalaire
   agrégé ne reconstitue pas la structure retirée ;
4. la **tête de sélection** du modèle, par le mécanisme du § 5.2 du manuscrit (voir le composant SEL, § 5).

**Hypothèse de stabilité.** Retirer des branches peut atténuer des variations
du scan, mais peut aussi perdre une structure utile. Mesurer conjointement
compression, couverture et stabilité après décimation ; la masse seule ne
garantit pas la survie. Voir P7 dans [MESURE](MESURE.md).

### 4.4 Ce que mesure réellement la condensation relative

L'objectif est de dériver des opérateurs utiles de FULL et de mesurer leur
transfert. Un seuil absolu ou relatif reste un choix expérimental ; aucun
n'est disqualifié uniquement par son unité.

**La règle relative à tester.** Une branche est lourde si elle conserve au
moins une fraction α de la masse du parent. Zéro branche lourde termine
le segment, une seule poursuit son identité, plusieurs créent une scission
simultanée ; les autres masses restent en réserve.
$\alpha$ est sans dimension, mais reste un hyperparamètre dont le transfert
doit être vérifié ; un compte absolu est plus sensible à la densité acquise.
C'est une variante à comparer aux coupes brutes.

**Elle filtre le déséquilibre, sans fixer une taille minimale.** Dans un
arbre binaire équilibré, α≤1/2 conserve chaque scission jusqu'aux singletons.
Pour α>1/2, aucune scission ne peut avoir deux branches lourdes. Une feuille
de masse relative $2^{-d}$ à la racine peut donc survivre à profondeur d.
Comparer ce seuil local à une référence de masse gelée et/ou à la durée de
vie, en déclarant ces variantes ; leur efficacité reste à mesurer.

Deux garde-fous s'y ajoutent :

- **coupler $\alpha$ entre les ordres.** Condenser chaque $K$ avec le même
  $\alpha$, et **revérifier la naturalité des cartes verticales après
  condensation** : si une fusion est retirée à l'ordre $K$ mais conservée à
  $K-1$, le carré peut cesser de commuter. Ce n'est pas automatique, c'est
  vérifiable, et il faut le vérifier ;
- **ne jamais binariser les multifusions.** La condensation de HDBSCAN parcourt
  des scissions binaires ; la tour publie des événements à trois parents ou
  plus au même niveau exact. Binariser inventerait un ordre qui n'existe pas et
  détruirait la canonicité. La règle doit être généralisée aux événements
  $k$-aires : à une multifusion, plusieurs branches peuvent tomber à la fois.

### 4.5 Quatre règles de construction de l'échelle

Le chemin fixe la direction ; il reste à fixer **comment on contracte**.

| règle | définition | attendu |
| --- | --- | --- |
| **E-global** | couper à des rayons globaux $r_1 < \cdots < r_L$ | référence simple ; variation de granularité à mesurer |
| **E-rang** | contracter les fusions dans l'ordre de $r$ jusqu'à une cible de compte | équilibré, mais encore globalement ordonné |
| **E-persistance** | contracter d'abord les fusions de plus faible persistance | **défaut à tester** : contracte selon la persistance, avec rayons locaux variables |
| **E-relative** | contracter dans l'échelle normalisée $r / r_K(x)$, avec convention explicite si $r_K(x)=0$ | invariance sous homothétie commune ; robustesse en portée à tester |

Les quatre règles se compareront sur l'arbre condensé, après validation des
coupes globales brutes du pilote. `E-persistance` est un défaut **à éprouver** : il ordonne les contractions
par une quantité structurale et contrôle le nombre d'unités, ce qui aide à
former les lots. Il produit des rayons locaux variables, mais la stabilité
des antichaînes ainsi choisies reste à mesurer sous perturbation du scan. La
condensation et les niveaux de sortie $\hat\lambda_x$ ont aussi un coût
de construction distinct de FULL.

Le rayon effectif de chaque nœud varie alors d'un bout à l'autre d'un même
niveau. **C'est le but** : c'est précisément ce qu'un voxel ne peut pas faire,
et c'est la mesure de l'adaptativité. Il faut le publier (histogramme de $r$
par niveau et par tranche de portée), car c'est l'observable qui montre que
l'architecture fait ce qu'elle prétend.

### 4.6 La limite de fond : la densité ne sépare pas ce qui se touche

Cette section manquait, et c'est la limitation la plus importante du projet.

**L'énoncé.** La tour sépare par la **densité**, jamais par la géométrie
différentielle. Deux objets dont les retours forment un continuum — une voiture
posée sur l'asphalte, un poteau dans l'herbe, un piéton sur la chaussée — sont
reliés dans certaines coupes. Il faut mesurer si **l'un des ordres et
rayons disponibles** rompt ce pont avant que l'objet disparaisse ; FULL ne le
garantit pas pour un scan donné. Un superpoint muni de normales, de planéité
ou d'élévation dispose de signaux supplémentaires au contact entre surfaces.

C'est une différence de nature et il faut la dire avant qu'un relecteur ne la
trouve. Elle borne directement le plafond d'oracle de la porte 0.1, et elle le
borne **là où sont les classes qui comptent**.

**Trois parades, d'inégale valeur.**

*L'axe $K$, et c'est la parade de principe.* Un contact entre une voiture et le
sol passe le plus souvent par une poignée de retours. Dans le vocabulaire du
chapitre 7 du manuscrit, c'est **un pont de bruit**, et $K$ est exactement le
paramètre qui y résiste : à $K$ élevé il faut $K$ retours simultanés dans une
même boule pour propager la connexité, ce qu'un contact ténu ne fournit pas. Le
Théorème 3 chiffre même la fraction récupérable avant fusion parasite. **La
limite et son remède sont donc tous deux dans la théorie**, et la question
devient quantitative : à quel $K$ le pont casse-t-il, sur de vraies scènes ?
C'est mesurable sans apprentissage.

*Une branche non-sol avec contexte.* Le sans-sol est un régime prioritaire du
moteur et une variante légitime du réseau. Comparer FULL sur brut à FULL sur
non-sol avec une branche sol/contexte, puis restituer les prédictions vers
**tous les retours**. Le masque géométrique est produit sur la trame entière
avant les vues et son coût est payé ; il ne devient pas une classe sémantique.
Mesurer erreurs de retrait et perte de contexte. Kmax, α et les coupes
comportent déjà des choix : l'existence de paramètres ne justifie pas
d'exclure cette variante. Si K permet de s'en passer, ce sera un résultat.

*Le relèvement métrique, et sa dette.* La tour n'est pas attachée à
$\mathbb{R}^{3}$ : elle se calcule dans tout espace euclidien. On peut donc la
construire sur $(x, y, z, \lambda n)$ avec $n$ la normale estimée, où deux
surfaces en contact mais d'orientations différentes deviennent distantes. C'est
séduisant et il faut être prudent : le chantier `E-HGP/` du dépôt a **mesuré**
que les naissances de la tour passent de $O(n)$ à $\binom{n}{k}$ quand la
dimension monte, l'obstruction étant gouvernée par la dimension **intrinsèque**
et ramenée par le bruit ambiant. Avant tout port, il faut donc simplement
compter les naissances d'une trame relevée en dimension 6. Si elles explosent,
la piste se ferme d'elle-même, et c'est une expérience d'une journée.

**Ce qu'on en fait.** On mesure le plafond d'oracle **stratifié par contact** :
objets en contact avec le sol contre objets isolés, en fonction de $K$, avec et
sans retrait du sol. C'est la porte 0.9. Tant que cette courbe n'existe pas,
toute discussion sur le potentiel du projet en segmentation d'instance est de
la spéculation.

## 5. Les six composants

### FP — Pooling de filtration

$M_\ell=P_\ell^\top M_{\ell-1}$ et $h_\ell=\phi(D_{M_\ell}^{-1}P_\ell^\top D_{M_{\ell-1}}h_{\ell-1}W_\ell)$.

P est normalisée en lignes, les colonnes de masse nulle sont masquées et la
mesure initiale est déclarée par site ou par retour. Les moyennes transportent
leurs masses à chaque étage. La conservation porte sur le transport linéaire
avant φ ; la somme brute est une autre agrégation à ablater.

**Trois interfaces distinctes** doivent être définies.

- **Niveau 0 vers niveau 1, points vers nœuds : doux.** L'arbre est un arbre de
  **facettes** ; une frontière couvrante partitionne les atomes retenus ; et
  les poids du § 9.1 poussent cette partition en une **partition de l'unité sur
  les points** : $w_{xv} = \sum_{\tau \in v,\ x\in\tau} S_\tau / T_x$, de somme 1
  lorsque les réserves **partielles** sont incluses. C'est ici que le recouvrement pour $K \geq 2$
  se manifeste. Compter non-zéros, lignes orphelines et coût de fabrication
  de $P_1$ (porte 0.5).
- **Niveau $\ell$ vers niveau $\ell+1$ : dur.** Deux antichaînes emboîtées du
  même univers pondéré gelé donnent à chaque bloc fin une image grossière
  unique. Vérifier $P_g=P_fQ$ et le quotient sur les atomes ; les naissances
  ultérieures sont lues latéralement, avec les réserves persistantes.
- **Changement d'ordre, $K$ vers $K-1$ : conditionnel.** La carte verticale
  envoie un nœud FULL à son niveau de création, mais son quotient entre les
  antichaînes retenues doit être construit et vérifié. Si les coupes sont
  incompatibles, OM consomme une incidence éparse entre branches. Même un
  quotient géométrique valide ne prouve pas la commutation des poids entre K ;
  les branches autonomes constituent le pilote.

Autrement dit, la crainte d'un $P_\ell$ dense à tous les étages était infondée :
**le recouvrement des retours coûte à l'entrée** ; à K fixé, les étages
emboîtés deviennent des contractions d'arbre. Les croisements entre K ont
un coût distinct à publier.

### MGA — Attention sur le graphe d'événements

La relation « fusionnent un jour » serait dense. Le pilote de la
[SPECIFICATION](SPECIFICATION.md) propose des **hubs d'événement** : les
états de la coupe sont reliés à leur prochain événement retenu, traité
atomiquement. Une multifusion de m branches demande m incidences, plutôt que
m(m−1)/2 arêtes entre frères. Deux échanges, vers le hub puis vers les états,
créent un goulot qui n'est pas équivalent à une attention de clique.

À K fixé, sur une même antichaîne géométrique, le niveau de fusion r_uv
donne une ultramétrique avec diagonale définie à zéro. Les paires non réunies
dans l'horizon restent censurées. La garantie ne s'étend pas aux paires
inter-K ni à un ensemble contenant simultanément ancêtres et descendants.

Le biais proposé $b_{uv}=\varphi(\log(r_{uv}/r_u))$ est invariant sous
homothétie commune pour rayons positifs mais **en général asymétrique** ; le biais appris
n'est pas une ultramétrique. L'inégalité du niveau brut ne justifie aucun
élagage des logits d'attention. Déclarer les codes zéro/censure, le rayon
propre et les réserves. La robustesse à la raréfaction reste à mesurer.

Conserver un canal de déplacement physique et comparer voisinage standard,
hubs, puis union des deux à budget d'arêtes apparié. Le graphe HGP ne
remplace pas automatiquement toute géométrie locale. Voir la contrainte de
noyau FlashAttention en §6 avant d'ajouter un biais pair-à-pair.

### OM — Mixage d'ordres

Lorsque deux branches K représentent la région, leurs cartes et incidences
fournissent une correspondance à construire sur les états retenus. À K élevé,
une région peut manquer : prévoir masque ou réserve, sans inventer un token.
Les échanges se font par attention croisée ou porte apprise.

Ce n'est pas un enrichissement décoratif. Le chapitre 7 du manuscrit fait de
$K$ le paramètre de **résistance à la percolation du bruit** : $K = 1$ est le
Single-Linkage, sensible et sujet au chaînage ; $K$ grand résiste aux ponts de
bruit mais retarde la naissance des structures minces. Le Théorème 3 chiffre la
fraction récupérable avant fusion parasite. **$K$ est donc littéralement un
bouton sensibilité/robustesse, et OM le rend apprenable par région.**

Un petit K peut préserver un poteau ; un K plus élevé peut éliminer un pont,
mais aussi faire disparaître l'objet. Comparer cette lecture à des
voisinages locaux multiples et aux canaux de normales/visibilité.

Trois réalisations, par coût croissant : **calendrier de $K$ selon la
profondeur** — et le sens est imposé par la monotonie du § 4.2, $K$ **élevé aux
niveaux fins**, **faible aux niveaux grossiers** —, **attention croisée entre
ordres au goulot**, et **branches parallèles par $K$**. Le pilote retient les
branches autonomes, qui gardent les mesures de chaque K explicites. Les
calendriers changeant K exigent en plus la compatibilité pondérée ; la valeur
relative des trois options sera mesurée sur les classes filiformes.

### PUR — Lecture par partition de l'unité

PUR étend linéairement le vote de labels du §9.1 : il mélange les
**probabilités** des jetons avec P, puis applique l'argmax. Il retrouve le
vote de la Proposition 7 lorsque les jetons portent des labels certains.
Softmax après mélange de logits définit un autre opérateur. La lecture
conserve les constantes, mais n'inverse pas le pooling ; les connexions de
saut maintiennent une représentation fine.

### FM — Modélisation de filtration

Voir § 7.

### SEL — Sélection apprise d'une antichaîne, extension instance

Le §5.2 du manuscrit permet de remplacer le coût statistique par un coût
adapté au problème. Pour un score additif g et une frontière couvrante,
la bonne récurrence compare les **optima des sous-arbres** :

$$V(C)=\max\left(g(C),\sum_{D\in\mathrm{enfants}(C)}V(D)\right).$$

Aux feuilles, V=g ; déclarer le départage. Les atomes attachés à un événement
et les réserves doivent avoir une option de couverture avant d'utiliser la
somme des enfants. Une option « aucun objet » exige son coût propre et ne
peut supprimer silencieusement la couverture sémantique.

Comparer g(parent) à la somme des seuls g(enfants) est faux : racine de
score 9, deux enfants de score 4, quatre petits-enfants de score 3 donnent
un optimum 12, pas 9. La DP est linéaire dans la taille de l'arbre pour cet
objectif additif, sans garantie sur une métrique d'instance non additive.

**Retirer la cible naïve somme des meilleurs IoU.** Un nœud égal à une
instance G a IoU=1 ; des enfants qui partitionnent G ont aussi une somme
d'IoU égale à 1. Descendre aux égalités rend tous les fragments. La bonne
définition de l'objectif est une tâche de recherche, pas un remplacement
automatique de l'excès de masse.

Pistes après le pilote sémantique : coût additif avec terme explicite par
objet et option fond/rejet, ou apprentissage structuré sur des sélections
complètes après projection. Une pénalité de cardinalité tranche la
fragmentation mais peut favoriser une fusion erronée. L'appariement avec
les instances annotées peut rester nécessaire pendant la supervision ou
l'évaluation. Un coût global/non additif n'hérite pas de cette DP exacte.

L'antichaîne rend les facettes disjointes, **pas leurs supports en retours**.
À K supérieur, le gagnant du vote pour un point partagé dépend des autres
nœuds sélectionnés : les scores après PUR ne sont généralement pas additifs.
Le vote §9.1 produit une partition finale après choix des labels, sans
garantir une instance par objet. L'oracle propre à SEL doit exécuter
sélection et vote ; le meilleur nœud isolé reste un diagnostic séparé.
Comparer à l'excès de masse sur le même arbre. ALPINE est un témoin
d'instances conditionné par des prédictions sémantiques et des priors ;
apparier ce régime. SEL reste hors du pilote sémantique.

## 6. Réalisation : partir du véritable PTv3

Épingler version, configuration et recette. La [table de substitution](ETAT_DE_LART.md)
décrit les classes officielles : `SerializedPooling`,
`SerializedUnpooling`, `SerializedAttention` et l'encodage de position
convolutionnel. Le retour standard est une remontée par indices inverses
avec connexion de saut, pas une interpolation K-NN.

Ordre d'intégration :

1. enseignant FULL et tête auxiliaire sur encodeur standard ;
2. FP/PUR vérifié, même représentation fine et budget global ;
3. graphe d'événements puis échanges inter-K, chacun ablaté ;
4. biais relationnel sur le même noyau d'attention que ses témoins.

Le chemin FlashAttention du code de référence refuse le RPE explicite ;
ajouter un biais HGP arbitraire n'est pas un remplacement gratuit.
Pour S4, comparer sans biais, biais XYZ et biais HGP dans une configuration
acceptant les trois, puis mesurer séparément le prix du changement de noyau.
Un hub peut utiliser des messages épars sans ce biais ; c'est un autre bras.

Sérialisation et convolutions de position utilisent encore des
coordonnées/grilles. Remplacer le pooling ne rend donc pas tout le réseau
« sans échelle métrique ». Garder la base de code rend les interventions
lisibles, mais ni des paramètres égaux ni un nom de module commun
n'égalisent automatiquement les opérations et le réceptif.

## 7. Pré-entraînement

### 7.1 Deux rôles de FULL à distinguer

Le [contrat de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md) définit
deux interventions : FULL comme **enseignant dans la perte**, avec un PTv3
ordinaire à l'inférence, et FULL comme **structure de calcul** dans HGP-UNet.
Le plan croisé A0G0/A0G1/A1G0/A1G1 distingue leurs gains et leur interaction.
Commencer par l'enseignant K1 demande seulement les coupes de couverture ;
FP/PUR à K supérieur attendent le supplément pondéré.

[Sonata](https://arxiv.org/html/2503.16429v1) motive le diagnostic des
raccourcis spatiaux. Dans notre cas, retirer un scalaire cible ne suffit pas
si parents, graphes ou biais calculés sur la vue complète le révèlent.
L'élève ne reçoit que sa vue V et, dans le bras architectural, FULL(V)
recalculé. FULL(T), sur la vue enseignante, reste dans la perte.

### 7.2 Les six tâches

| tâche | cible proposée | limite à traiter |
| --- | --- | --- |
| **FM-1 fusion** | connexité K1 d'une paire de retours à un rayon interrogé | témoins distance, voisinage local et MST sur V ; horizon déclaré |
| **FM-2 persistance** | survie ou rayon d'un état identifié | naissances et censure, variables et ancêtres enseignants cachés |
| **FM-3 profil en K** | lectures des mêmes retours dans plusieurs ordres | pas d'identité de nœud supposée ; couverture et recouvrement explicites |
| **FM-4 rétablissement** | relations enseignantes après suppression d'une région | structure cachée non identifiable en général ; prédiction conditionnelle |
| **FM-5 accord inter-vues** | features sur les mêmes IDs et supports pondérés dans la perte | les deux arbres recalculés peuvent différer ; accord seul trivial |
| **FM-6 distillation d'agrégat** | relations d'une cible temporelle plus informée | régime secondaire avec alignement, visibilité et mouvement |

Commencer avec FM-1 et la recette SSL de référence. Ajouter un prétexte à la
fois. Les relations FULL sont exactes sur l'enseignant ; leur prédiction
depuis une vue appauvrie et leur intérêt sémantique restent à mesurer.
Préparation, requêtes et cache entrent dans le coût total.

### 7.3 Masquage par nœuds entiers

Un nœud peut définir un masque structurel cohérent. Sa sélection depuis la
tour complète peut aussi renseigner sur la cible : c'est une intervention à
comparer à des masques géométriques de mêmes volume et cardinal.
Le premier pilote utilise des masques indépendants des réponses FULL(T).
Les features, parents et poids de la région cachée ne passent pas dans le
forward. Fixer V puis changer la seule partie cachée de T doit laisser ce
forward inchangé.

### 7.4 Auto-distillation

Conserver une recette enseignant/élève de référence, avec décimation,
retrait d'anneaux et occultation. **Ne pas imposer la même structure de fusion
entre vues.** À K1, {0,1,2} fusionne au rayon 1/2, mais {0,2} au rayon 1.
Un pont supprimé change correctement la tour.

Le guidage compare les mêmes IDs sur le domaine commun observable. Pour les
features régionales, agréger enseignant et élève avec les mêmes poids
enseignants, dans la perte seulement. Le produit de deux matrices
d'affectation douces fournit un mélange, pas une identité de tokens, même
pour une vue inchangée. Les formules, réserves et contre-exemples sont dans
le [contrat de guidage](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md).
Conserver le mécanisme de diversité de la SSL de base : une cible dense
n'interdit pas à elle seule une représentation constante.

### 7.5 Augmentations : ce qui commute, et ce que la quantification casse

*Section corrigée. Une première version affirmait que les rotations, les
translations et les homothéties commutent exactement avec la tour et que ces
augmentations étaient donc gratuites. C'est vrai de l'objet mathématique et
**faux du moteur**.*

L'objet mathématique est bien équivariant : $\rho(\sigma)$, la condition de
Gabriel et l'ordre des niveaux sont invariants par isométrie et covariants par
homothétie. Mais le moteur consomme un nuage **quantifié sur une grille de
1 mm**. Le chemin réel est `tourner → quantifier → tour`, et une rotation change
l'accrochage à la grille : le nuage quantifié n'est pas l'image du nuage
quantifié. Les prédicats étant exacts, un accrochage différent peut de surcroît
**faire basculer une égalité** — un plateau cosphérique se scinde, une
multifusion devient deux fusions.

Trois conséquences pratiques :

1. **Recalculer, ne pas supposer.** Dimensionner une banque de vues
   augmentées — rotations, décimations, retraits d'anneaux, occultations —
   avec coût de compilation, stockage, diversité et taux de réutilisation.
   La conformité de FULL ne suffit pas à rendre cette banque négligeable.
2. **Mesurer l'écart.** Rapporter changements de FULL, du condensé et des
   affectations consommées. Si ces changements pénalisent le réseau,
   comparer les précisions et leurs coûts. Une égalité exacte qui se scinde
   peut modifier la combinatoire sans dégrader la représentation utile.
   Porte 0.7 de [`MESURE.md`](MESURE.md).
3. **La rotation devient une augmentation informative.** Puisqu'elle n'est pas
   gratuite, elle teste quelque chose : l'invariance du modèle à une
   perturbation de l'ordre du millimètre. C'est exactement le régime où les
   prédicats exacts peuvent basculer, donc le pire cas honnête.

### 7.6 Le temps : une extension avec information supplémentaire

La cible primaire reste le mono-scan sans historique. Le temporel est une
extension distincte, même quand seule l'étape de pré-entraînement y accède.
Deux usages peuvent être évalués.

**Usage en ligne.** Une tour par trame, plus une attention temporelle entre
nœuds de trames voisines, recalée par l'odométrie. Classique.

**Usage en apprentissage.** On
agrège $N$ trames consécutives en compensant le mouvement propre, et on calcule
**une tour sur l'agrégat**. Le nuage agrégé apporte davantage de retours
observés, avec ses propres erreurs d'alignement, occultations et objets
mobiles. La trame isolée en est un échantillon plus clairsemé.

On tient alors une cible que le poster demandait sans pouvoir la produire :

> **FM-6 — distillation d'agrégat.** Depuis une trame isolée, prédire la
> structure de fusion que la tour de l'agrégat multi-trames possède.

La cible est plus informée, sans être identifiable depuis toute trame isolée.
Comparer les variantes à même accès aux trames et à l'odométrie, avec fenêtres
et frontières de corpus déclarées. La recette anti-effondrement reste
nécessaire. Mesurer alignement, tour de l'agrégat et contrôle de visibilité.

Les objets mobiles et erreurs de recalage déforment l'agrégat ; une fenêtre
courte ne les supprime pas. Déclarer confiance, visibilité et pondération des
observations répétées. L'agrégat est une cible plus dense, pas une surface
physique vraie ni nécessairement un meilleur enseignant sur chaque région.

## 8. Variables de nœud

Le détail est dans [`JETON.md`](JETON.md). Le principe tient en une règle :
**une seule famille est normalisée**, la forme ; les grandeurs physiques, les
canaux de filtration et les canaux d'acquisition gardent leurs unités, dans des
canaux séparés et ablatables.

Conserver la taille physique comme information, avec unité de référence
explicite pour les logarithmes. Ce canal permet au modèle d'utiliser la
métrique ; il ne rend pas le réseau équivariant par construction. Mesurer
séparément covariance de l'objet, transformation des features et comportement
appris sous homothétie, décimation et changement de capteur.

## 9. Ce que l'architecture n'est pas

| conception écartée | raison |
| --- | --- |
| Sélectionner quelques milliers de jetons dans la tour et les donner à un Transformer plat | perd la hiérarchie, qui est la contribution ; et c'était un mauvais cadrage du coût (§ 2) |
| Vectoriser la persistance (code-barres, paysages, images de persistance) en variables d'entrée d'un réseau standard | c'est la voie TDA classique : elle jette la structure pour n'en garder qu'un résumé, et elle est largement explorée |
| Une seule tranche ou un seul K comme architecture finale | perd les événements et alternatives entre ordres que FULL publie ; garder comme témoin d'ablation |
| Apprendre la partition | hors du premier pilote ; témoin possible à budget égal. Un tokenizer appris peut être déterministe, mais sa partition dépend des poids appris |
| Écrire un réseau nouveau de zéro | augmente les facteurs de confusion et le travail de reproduction ; réutiliser PTv3 au premier pilote |
| Forcer une partition stricte des points à l'entrée | pour $K \geq 2$, le recouvrement **est** l'information d'ordre supérieur (§ 9.1) |
| Employer uniquement l'enveloppe convexe ou la fonction support | perd la non-convexité de la réalisation choisie ; témoin géométrique possible, sans l'identifier au squelette HGP ou à une surface physique |

## 10. Ce qui est revendicable

Aucune brique n'est nouvelle isolément : U-Net 3D, attention éparse
hiérarchique, partitions multi-échelles, auto-distillation, persistance
multiparamètre. Ce qui peut l'être :

1. **dériver des coupes de la tour exacte**, avec règles et budgets
   déclarés, puis mesurer leur transfert ;
2. **utiliser les forêts exactes et les cartes entre K** comme structure de
   calcul, et établir expérimentalement la valeur de l'axe K ;
3. **un biais dérivé des rayons de fusion**, dont la robustesse
   à la raréfaction se mesure ; sa version normalisée n'est pas ultramétrique ;
4. **une famille de prétextes structurels exacts**, avec raccourcis
   et coût de préparation mesurés ;
5. **un décodeur fondé sur le vote démontré** (Proposition 7), sous réserve
   d'exporter les incidences et poids nécessaires.

Chacun de ces cinq points correspond à une ligne de la table de substitution de
[`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 4, donc à une expérience contrôlée. Si
une ligne ne se paie pas, on la retire et on le dit. C'est l'objet de
[`MESURE.md`](MESURE.md).
