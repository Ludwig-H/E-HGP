# HGP-UNet — l'architecture

26 septembre 2026. Conception. Aucune expérience apprise n'est rapportée.

Prérequis : [`ETAT_DE_LART.md`](ETAT_DE_LART.md), qui établit ce que la tour
remplace, et [`OBJET.md`](OBJET.md), qui dit ce qu'elle fournit.

## 1. La thèse, en un paragraphe

Tout encodeur 3D contient une **échelle métrique posée à la main** — taille de
voxel, liste de rayons, $k$, taille de *patch* sur une courbe remplissante — et
c'est exactement ce qui casse quand le capteur, la portée ou le domaine
changent. Les modèles de fondation 3D de 2025–2026 ne suppriment pas cette
constante : ils en rattrapent les effets par du rééchelonnage, de l'augmentation
et du brouillage. La tour HGP fournit à la place une **structure exacte à
deux paramètres, dérivée des données**. La stabilité des coupes et affectations
choisies par le réseau est une propriété distincte à mesurer. On ne l'ajoute
donc pas à une architecture : **on la substitue aux composants qui
portent la constante.**

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

C'est **le même ordre de grandeur de jetons que PTv3 sur la même trame**.
La v9 matérialise cependant FULL avant que le réseau n'en lise $L$ coupes ;
leur export et leur mise en cache ont aussi un coût. Distinguer donc jetons
consommés, octets de FULL, compilation des coupes et latence totale. La
question architecturale devient **« quel chemin et quelles coupes lire dans
$(K,r)$, à quel coût ? »** ; voir l'[audit transversal](AUDIT_V9_ET_ARCHITECTURE_20260926.md).

## 3. Les six primitives

La tour fournit exactement six objets dont une architecture 3D a besoin et
qu'elle se procure aujourd'hui par des constantes.

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
   naturalité. **Sans équivalent dans aucune architecture existante.**
5. **Des scalaires structurels exacts** par nœud : naissance, mort,
   persistance, mélange d'arités, degré de multifusion, population. Gratuits, à
   la fois comme variables d'entrée et comme cibles de pré-entraînement.
6. **Une lecture exacte vers les points** (§ 9.1 et Proposition 7) : remplace
   l'interpolation trilinéaire ou $k$-NN du décodeur.

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

Un U-Net exige que chaque unité d'un niveau tombe dans **exactement une** unité
du niveau suivant. Cela demande que les ensembles de niveau soient emboîtés, et
la condition est immédiate :

$L_K(r) \subseteq L_{K'}(r') \quad \text{dès que} \quad r' \geq r \ \text{ et } \ K' \leq K.$

En effet, si $|B(y, r) \cap \mathcal{X}| \geq K$ et $r' \geq r$, alors
$|B(y, r') \cap \mathcal{X}| \geq K \geq K'$. Et la condition sur $K$ est
essentielle : un point qui a exactement $K$ voisins dans $B(y, r')$ appartient à
$L_K(r')$ mais pas à $L_{K'}(r')$ pour $K' > K$. **Grossir, c'est donc augmenter
$r$ et décroître $K$, jamais l'inverse.**

Trois familles de chemins sont donc des échelles valides :

| chemin | mouvement | effet |
| --- | --- | --- |
| **horizontal** | $r \uparrow$, $K$ fixé | grossir en espace, à exigence de densité constante |
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
à des échelles spatiales différentes, reliées par les cartes verticales et non
par du pooling. C'est exactement ce que consomme le mixage d'ordres (OM).

### 4.2 bis Pourquoi OM devient structurel, et non facultatif

La monotonie a une conséquence qui change le statut du mixage d'ordres.

Le long d'une échelle, $K$ **décroît** avec la profondeur : les niveaux fins
travaillent à $K$ élevé (exigence forte, beaucoup de petites composantes), les
niveaux grossiers à $K$ faible (exigence faible, peu de grandes composantes).
C'est l'inverse de ce qu'une première version de ce document affirmait.

Or la sémantique tire dans l'autre sens. Un objet mince et lointain — un poteau
à quarante mètres, quinze retours sur deux mètres — naît **tard** à $K$ élevé et
**tôt** à $K$ faible. Il vit donc dans le coin « $K$ petit, $r$ petit » du
treillis. Une échelle monotone unique traverse ce coin au mieux en diagonale :
aux niveaux fins elle est à $K$ élevé, où le poteau n'existe pas encore ; aux
niveaux grossiers elle est à $K$ faible, où il a déjà fusionné avec le sol ou la
végétation.

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

Avant de contracter, il faut **condenser**, au sens exact de HDBSCAN. Ce n'est
pas un ajout emprunté à l'extérieur : le § 9.1 du manuscrit le prescrit, avec
le poids qu'il faut.

> « La masse d'une face dans l'arbre condensé est alors
> $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$ […] C'est ce poids $m_\tau$, et non
> le simple comptage des faces, qui est utilisé par le seuil
> `min_cluster_size` dans l'arbre condensé (mêmes idées algorithmiques que
> HDBSCAN). »

**Pourquoi c'est nécessaire et non optionnel.** La forêt brute est dominée par
des événements triviaux. À $K = 1$ sur une trame sans sol de 39 885 sites, il y
a 39 796 fusions, c'est-à-dire le dendrogramme complet du Single-Linkage : la
quasi-totalité sont des « un point rejoint une grosse composante ». Le § 4.4.3
du manuscrit les nomme pour ce qu'ils sont — du bruit de micro-composantes —
et note que l'élagage de HDBSCAN est en réalité **un seuil de percolation** non
formulé comme tel. Une échelle construite en comptant les fusions brutes
dépenserait ses premiers niveaux à absorber des singletons.

**Ce que la condensation rend.** Quatre choses, dont trois sont des entrées
directes de l'architecture :

1. le **squelette** : les vraies scissions, sans les continuations ;
2. la **stabilité** de chaque nœud, $\widehat{E}(C) \propto \sum_{x \in C} (\hat\lambda_x - \hat\lambda_{\min})$,
   c'est-à-dire l'ordre dans lequel contracter ;
3. le niveau de sortie $\hat\lambda_x$ de chaque point, **rendu comme variable
   par point**. La condensation est donc un *changement de représentation* et
   non une perte : ce qu'elle retire de la structure, elle le rend en scalaire ;
4. la **tête de sélection** du modèle, par le mécanisme du § 5.2 du manuscrit (voir le composant SEL, § 5).

**Ce qu'elle doit rendre stable, et c'est une prédiction.** L'arbre brut bouge
beaucoup sous décimation : chaque micro-fusion se déplace. L'arbre condensé ne
garde que les événements qui ont de la masse, donc précisément ceux qui
survivent à une perte de points. **La condensation devrait donc améliorer
l'invariance en portée**, et c'est mesurable sans apprentissage (prédiction P7
de [`MESURE.md`](MESURE.md)).

### 4.4 Le piège : `min_cluster_size` est exactement la constante que l'on voulait supprimer

Il faut nommer la tension au lieu de la contourner. Toute la thèse de ce
dossier est de retirer les constantes posées à la main ; or la condensation de
HDBSCAN en introduit une, `min_cluster_size`, et un seuil en *nombre de points*
ne transfère ni d'un capteur à l'autre, ni du champ proche au champ lointain.

**La règle qui résout la tension : un rapport transfère, une longueur non.**
Le seuil doit être **relatif** — une scission n'est validée que si chaque
branche conserve au moins une fraction $\alpha$ de la masse $m_\tau$ du parent.
$\alpha$ est sans dimension, mais reste un hyperparamètre dont le transfert
doit être vérifié ; un compte absolu est plus sensible à la densité acquise.
C'est la première forme de condensation à évaluer ici.

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
| **E-global** | couper à des rayons globaux $r_1 < \cdots < r_L$ | témoin, doit échouer sur la variation de portée |
| **E-rang** | contracter les fusions dans l'ordre de $r$ jusqu'à une cible de compte | équilibré, mais encore globalement ordonné |
| **E-persistance** | contracter d'abord les fusions de plus faible persistance | **défaut à tester** : contracte selon la persistance, avec rayons locaux variables |
| **E-relative** | contracter dans l'échelle normalisée $r / r_K(x)$, avec convention explicite si $r_K(x)=0$ | invariance sous homothétie commune ; robustesse en portée à tester |

Les quatre règles s'appliquent **sur l'arbre condensé**, jamais sur la forêt
brute. `E-persistance` est un défaut **à éprouver** : il ordonne les contractions
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

*Le retrait du sol, et son ironie.* C'est la parade pratique, universelle dans
le domaine — ALPINE atteint $\mathrm{PQ} = 64{,}2$ après elle, et le corpus v9
maintient des trames sans sol pour cette raison. Mais il faut reconnaître ce
qu'elle est : **un prétraitement à seuil posé à la main**, c'est-à-dire
exactement le genre de constante que tout ce dossier prétend supprimer. On ne
peut donc pas la présenter comme une solution ; on la garde comme un régime de
comparaison, et on mesure ce que $K$ fait **sans elle**. Si $K$ remplace le
retrait du sol, c'est un résultat en soi.

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

$h_\ell = \phi\!\left(P_\ell^{\top} h_{\ell-1} W_\ell\right)$, où $P_\ell$ est
l'affectation du niveau $\ell-1$ vers le niveau $\ell$, normalisée en lignes
par les poids $w_{x\tau} = S_\tau / T_x$ du § 9.1. Le dépliage est $P_\ell$
appliqué en sens inverse, avec connexion de saut, comme dans tout U-Net.

Trois propriétés qu'un *grid pooling* n'a pas : l'affectation est **canonique**
(aucune grille, aucune graine), **conservative** (la masse est préservée sur
toute antichaîne), et douce là où il le faut — voir ci-dessous.

**Un seul étage est doux, et c'est une simplification importante.** Il faut
distinguer deux interfaces, ce qu'une première version de ce document
confondait.

- **Niveau 0 vers niveau 1, points vers nœuds : doux.** L'arbre est un arbre de
  **facettes** ; une antichaîne de cet arbre **partitionne** les facettes ; et
  les poids du § 9.1 poussent cette partition en une **partition de l'unité sur
  les points** : $w_{xv} = \sum_{\tau \in v} S_\tau / T_x$, de somme $1$ sur
  l'antichaîne, sous réserve de couvrir tous les retours, y compris ceux sans
  facette après condensation. C'est ici que le recouvrement pour $K \geq 2$
  se manifeste. Compter non-zéros, lignes orphelines et coût de fabrication
  de $P_1$ (porte 0.5).
- **Niveau $\ell$ vers niveau $\ell+1$ : dur.** Deux antichaînes emboîtées du
  même arbre condensé donnent à chaque nœud du niveau fin **exactement un**
  ancêtre au niveau grossier. $P_{\ell+1}$ est une matrice $0/1$ à une entrée
  par ligne : un pooling ordinaire, creux, sans recouvrement.
- **Changement d'ordre, $K$ vers $K-1$ : conditionnel.** La carte verticale
  envoie un nœud FULL à son niveau de création, mais son quotient entre les
  antichaînes retenues doit être construit et vérifié. Si les coupes sont
  incompatibles, OM consomme une incidence éparse entre branches.

Autrement dit, la crainte d'un $P_\ell$ dense à tous les étages était infondée :
**le recouvrement des retours coûte à l'entrée** ; à K fixé, les étages
emboîtés deviennent des contractions d'arbre. Les croisements entre K ont
un coût distinct à publier.

### MGA — Attention sur le graphe de fusion, à biais ultramétrique

À chaque niveau, l'attention est restreinte au **graphe de fusion** : les nœuds
voisins sont ceux qui fusionneront, et la distance naturelle entre deux nœuds
$u$ et $v$ est le niveau $r_{uv}$ auquel ils se rejoignent.

Ce $r_{uv}$ n'est pas une distance quelconque : c'est une **ultramétrique**.
Le chapitre 3 du manuscrit établit l'équivalence entre dendrogrammes et
ultramétriques ; la hiérarchie *est* une ultramétrique sur ses feuilles. On
pose donc le biais d'attention

$b_{uv} = \varphi\!\left(\log r_{uv} - \log r_u\right)$,

normalisé par l'échelle propre du nœud. C'est un **encodage de position
relative par la hiérarchie et non par la métrique**. Sous une homothétie
commune des rayons positifs, le rapport est invariant. Une raréfaction des
retours avec la portée peut modifier différemment
$r_{uv}$ et $r_u$ : cette robustesse doit être mesurée. Définir un code
séparé si $r_u=0$.

L'encodage relatif $xyz$ ordinaire reste disponible comme **canal séparé, à
ablater** : on veut savoir lequel des deux porte l'information.

### OM — Mixage d'ordres

À un niveau donné, la même région possède une représentation pour chaque $K$,
et les cartes verticales fournissent la correspondance. On fusionne par
attention croisée ou par porte apprise.

Ce n'est pas un enrichissement décoratif. Le chapitre 7 du manuscrit fait de
$K$ le paramètre de **résistance à la percolation du bruit** : $K = 1$ est le
Single-Linkage, sensible et sujet au chaînage ; $K$ grand résiste aux ponts de
bruit mais retarde la naissance des structures minces. Le Théorème 3 chiffre la
fraction récupérable avant fusion parasite. **$K$ est donc littéralement un
bouton sensibilité/robustesse, et OM le rend apprenable par région.**

Un poteau veut $K$ petit ; séparer une voiture du sol qui la touche veut $K$
grand. Aucune architecture à un seul graphe de voisinage ne peut offrir ce
choix.

Trois réalisations, par coût croissant : **calendrier de $K$ selon la
profondeur** — et le sens est imposé par la monotonie du § 4.2, $K$ **élevé aux
niveaux fins**, **faible aux niveaux grossiers** —, **attention croisée entre
ordres au goulot**, et **branches parallèles par $K$**. Le § 4.2 bis montre que
la troisième n'est pas la borne supérieure coûteuse qu'on croyait mais la
**seule** qui rende visibles les objets minces et lointains : c'est donc elle
qu'il faut mesurer en premier, pas en dernier.

### PUR — Lecture par partition de l'unité

Le décodeur ne réinvente aucune interpolation : $p(x) = \sum_{\tau \ni x} w_{x\tau} \, p_\tau$
en entraînement, argmax de la Proposition 7 en inférence, qui garantit une
partition stricte des points. Les $w_{x\tau}$ somment à $1$, donc c'est une
relaxation différentiable propre et la masse est conservée.

### FM — Modélisation de filtration

Voir § 7.

### SEL — Tête de sélection apprise, ou « hacker HDBSCAN »

C'est la conséquence la plus productive de la condensation, et elle vient
encore du manuscrit. Le § 5.2 du manuscrit observe que l'extraction d'un partitionnement à
plat depuis l'arbre condensé est un **programme dynamique ascendant** à
fonction de coût **remplaçable** :

$\text{si } \mathrm{loss}(C_{\text{père}}) < \sum_i \mathrm{loss}(C_{\text{fils},i}) \implies \text{conserver le père, sinon continuer d'explorer.}$

Avec $\mathrm{loss}(C) = -\widehat{E}(C)$ on retrouve exactement l'excès de
masse de HDBSCAN. Et le manuscrit en tire la remarque décisive : **« L'excès de
masse est un critère purement statistique, aveugle à la géométrie »** ; en
substituant une évaluation propre au problème — alignement d'une structure,
volume attendu, conformité à un modèle 3D — « l'algorithme se transforme en un
extracteur guidé géométriquement ». C'est ce que font les deux applications du
§ 5.3 et du § 5.4 du manuscrit, avec des coûts écrits à la main.

**Le mouvement du modèle de fondation est donc évident : apprendre ce coût.**
On pose $\mathrm{loss}(C) = -g_\theta(h_C)$, où $h_C$ est l'état du nœud produit
par HGP-UNet, et l'on garde le programme dynamique tel quel.

Quatre propriétés font de cette tête un bon objet, et non un gadget :

1. **le programme dynamique est exact et coûte une passe ascendante** sur
   l'arbre condensé, soit quelques milliers de nœuds ;
2. **il rend une antichaîne par construction.** Les nœuds choisis ne se
   recouvrent donc jamais dans l'arbre : ni suppression non maximale, ni
   appariement hongrois, ni seuil de recouvrement à régler. La Proposition 7
   convertit ensuite en partition stricte des points. Une tête d'instance sans
   aucune machinerie de propositions, c'est rare ;
3. **il se supervise simplement** : cible de $g_\theta$ = l'IoU du nœud avec la
   meilleure instance annotée, programme dynamique exact à l'inférence. Une
   relaxation continue n'est utile qu'ensuite, si la discrétisation coûte ;
4. **il généralise les deux applications du manuscrit** au lieu de les
   concurrencer : le coût guidé par un modèle 3D de la détection d'anomalies
   devient un cas particulier à coût figé.

Deux limites à énoncer tout de suite. Le meilleur IoU d'un nœud isolé de la porte 0.1 mesure la qualité des
propositions, mais ne borne pas à lui seul une antichaîne suivie du vote § 9.1
ni une sortie par point. Le plafond propre à SEL doit exécuter la sélection
et le vote réellement permis. Et le témoin qui compte n'est pas
un détecteur à boîtes, c'est **l'excès de masse sur le même arbre** : si le
coût appris ne bat pas $-\widehat{E}(C)$, il n'apporte rien. À quoi s'ajoute
ALPINE, qui atteint $\mathrm{PQ} = 64{,}2$ par regroupement géométrique sans
aucune étiquette d'instance.


## 6. Réalisation : une modification de PTv3, pas un nouveau réseau

**C'est la décision d'ingénierie la plus importante du projet.** HGP-UNet doit
être écrit *dans* la base de code PTv3, en remplaçant :

- `GridPool` par `FiltrationPool` ;
- le regroupement en *patches* sur la sérialisation par les *patches* du graphe
  de fusion (en conservant la sérialisation **à l'intérieur** d'un nœud, ce qui
  garde l'attention par fenêtre efficace) ;
- l'encodage relatif par le biais ultramétrique ;
- le décodeur d'interpolation par PUR ;

**et rien d'autre.** Même nombre de couches, mêmes largeurs, même optimiseur,
même recette.

Deux raisons, et elles pèsent plus que l'élégance :

1. **la substitution devient exacte.** Chaque composant se mesure contre son
   équivalent standard, toutes choses égales par ailleurs. Un réseau écrit de
   zéro rendrait tout écart inintelligible ;
2. **le résultat devient lisible par la communauté.** « Nous remplaçons le
   *grid pooling* de PTv3 par une échelle de densité canonique et nous gagnons
   $x$ points » est une phrase vérifiable. « Nous proposons une nouvelle
   architecture » ne l'est pas.

## 7. Pré-entraînement

### 7.1 Pourquoi la tour répond au raccourci géométrique

Sonata a établi le diagnostic central de la SSL 3D : les représentations
s'effondrent sur des indices spatiaux de bas niveau, **parce que la géométrie
est l'entrée**. Prédire une coordonnée masquée se résout par interpolation
locale. Sonata *atténue* : bruit gaussien sur les coordonnées masquées,
ordonnanceur de masque, suppression du décodeur hiérarchique.

Les cibles de la tour permettent des prétextes structurels exacts, mais
l'absence de raccourci géométrique reste à établir. À K=1, deux points
fusionnent au rayon égal à la moitié de leur distance : une cible FM-1 peut
être locale. Pour chaque tâche, masquer les variables qui révèlent la cible,
puis comparer à un prédicteur fondé sur la géométrie locale et à un contexte
plus large.

### 7.2 Les six tâches

| tâche | cible | pourquoi elle n'est pas locale |
| --- | --- | --- |
| **FM-1 fusion** | $\log r_{uv}$ pour deux nœuds adjacents | goulot de densité entre eux |
| **FM-2 persistance** | mort $-$ naissance d'un nœud | dépend de tout le voisinage jusqu'à la fusion |
| **FM-3 profil en $K$** | l'ordre auquel un nœud cesse d'exister | mesure la densité locale relative au reste |
| **FM-4 rétablissement** | masquer une région, prédire la **structure de fusion** qu'elle aurait | reconstruction topologique, pas géométrique |
| **FM-5 accord inter-vues** | même structure de fusion sous décimation en portée et occultation | c'est l'hypothèse d'invariance, posée en fonction de perte |
| **FM-6 distillation d'agrégat** | depuis une trame isolée, la structure de fusion de la tour de l'agrégat multi-trames | § 7.6 : une **cible dense** au lieu d'un simple accord entre deux vues pauvres |

Ces cibles sont **exactes une fois la tour et les incidences nécessaires
calculées**, sans annotation ; leur préparation, stockage et alignement entre
vues doivent entrer dans le coût total.

### 7.3 Masquage par nœuds entiers

En traitement du langage, masquer un mot entier bat le masquage de sous-mots.
La tour donne l'unité correspondante en 3D : **on masque un nœud entier, pas
des points au hasard.** Un nœud fournit un masque structurel cohérent ;
vérifier par témoin local si son contenu reste prévisible par interpolation. C'est le levier le plus
simple à essayer, et il se greffe sur n'importe quelle recette d'auto-
distillation existante sans changer le reste.

### 7.4 Auto-distillation

En parallèle, une auto-distillation enseignant/élève de la famille Sonata, avec
les augmentations que le domaine impose : décimation en portée selon le modèle
de balayage, retrait d'anneaux, occultation par secteur. L'invariant demandé à
l'élève est que **la structure**, et non les coordonnées, soit préservée.

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

1. **Recalculer, ne pas supposer.** Le moteur est assez rapide pour des
   centaines de milliers de trames ; on pré-calcule donc une **banque de vues
   augmentées** — rotations, décimations en portée, retraits d'anneaux,
   occultations — chacune avec sa tour. C'est un coût de préparation, pas une
   difficulté.
2. **Mesurer l'écart, parce qu'il est informatif.** La dérive du condensé de la
   tour sous rotation pure est une **mesure directe de la stabilité de l'objet
   au niveau de quantification choisi**. Si elle est forte, la grille de 1 mm
   est trop grossière pour la géométrie observée, et c'est un fait utile bien
   au-delà de ce dossier. Porte 0.7 de [`MESURE.md`](MESURE.md).
3. **La rotation devient une augmentation informative.** Puisqu'elle n'est pas
   gratuite, elle teste quelque chose : l'invariance du modèle à une
   perturbation de l'ordre du millimètre. C'est exactement le régime où les
   prédicats exacts peuvent basculer, donc le pire cas honnête.

### 7.6 Le temps, et la meilleure cible d'apprentissage du projet

Un LiDAR automobile produit une séquence, pas une trame. La tour, elle, est
définie par trame. Deux usages, et le second est le plus intéressant.

**Usage en ligne.** Une tour par trame, plus une attention temporelle entre
nœuds de trames voisines, recalée par l'odométrie. Classique.

**Usage en apprentissage, et c'est là que se trouve le vrai levier.** On
agrège $N$ trames consécutives en compensant le mouvement propre, et on calcule
**une tour sur l'agrégat**. Le nuage agrégé apporte davantage de retours
observés, avec ses propres erreurs d'alignement, occultations et objets
mobiles. La trame isolée en est un échantillon plus clairsemé.

On tient alors une cible que le poster demandait sans pouvoir la produire :

> **FM-6 — distillation d'agrégat.** Depuis une trame isolée, prédire la
> structure de fusion que la tour de l'agrégat multi-trames possède.

C'est l'hypothèse centrale du projet transformée en fonction de perte
**supervisée par la géométrie elle-même**. Là où FM-5 demande seulement que
deux vues s'accordent — ce qui peut être satisfait par une représentation
triviale —, FM-6 propose une cible dense. Son calcul exige l'alignement,
la tour de l'agrégat et un contrôle de visibilité ; mesurer ces coûts.

Deux précautions honnêtes. Les objets **mobiles** se traînent dans l'agrégat ;
on se limite donc à des fenêtres courtes, et l'on rapporte séparément les
classes dynamiques. Et l'agrégat n'est pas la vérité : c'est un meilleur
échantillon, pas la surface. On ne doit donc jamais parler de « vérité
géométrique » mais de **cible dense**.

## 8. Variables de nœud

Le détail est dans [`JETON.md`](JETON.md). Le principe tient en une règle :
**une seule famille est normalisée**, la forme ; les grandeurs physiques, les
canaux de filtration et les canaux d'acquisition gardent leurs unités, dans des
canaux séparés et ablatables.

Point de conception important pour le transfert inter-capteurs : on veut une
**équivariance d'échelle, pas une invariance**. Une voiture mesure quatre
mètres, et c'est une information réelle. On donne donc $\log r$ comme canal
explicite, tandis que la *structure* (pooling, voisinage, biais) reste sans
échelle. Le modèle peut ainsi utiliser l'échelle absolue là où elle aide et
l'ignorer là où elle nuit — et l'ablation du canal $\log r$ dit lequel des deux
régimes domine.

## 9. Ce que l'architecture n'est pas

| conception écartée | raison |
| --- | --- |
| Sélectionner quelques milliers de jetons dans la tour et les donner à un Transformer plat | perd la hiérarchie, qui est la contribution ; et c'était un mauvais cadrage du coût (§ 2) |
| Vectoriser la persistance (code-barres, paysages, images de persistance) en variables d'entrée d'un réseau standard | c'est la voie TDA classique : elle jette la structure pour n'en garder qu'un résumé, et elle est largement explorée |
| Une seule tranche ou un seul K comme architecture finale | perd les événements et alternatives entre ordres que FULL publie ; garder comme témoin d'ablation |
| Apprendre la partition (superpoints appris, $k$-moyennes différentiables) | on reperd la canonicité et le déterminisme, et l'ablation redevient inintelligible |
| Écrire un réseau nouveau de zéro | rend la substitution impossible à interpréter et le résultat invérifiable |
| Forcer une partition stricte des points à l'entrée | pour $K \geq 2$, le recouvrement **est** l'information d'ordre supérieur (§ 9.1) |
| Remplacer un nœud par l'enveloppe convexe de sa géométrie, ou par sa seule fonction support | $h_P = h_{\mathrm{conv}(P)}$ : aveugle à la non-convexité et aux trous, qui sont l'essentiel d'une surface LiDAR |

## 10. Ce qui est revendicable

Aucune brique n'est nouvelle isolément : U-Net 3D, attention éparse
hiérarchique, partitions multi-échelles, auto-distillation, persistance
multiparamètre. Ce qui peut l'être :

1. **remplacer l'échelle métrique posée à la main par une échelle canonique
   dérivée des données**, et montrer par substitution ce que cela vaut ;
2. **utiliser les forêts exactes et les cartes entre K** comme structure de
   calcul, et établir expérimentalement la valeur de l'axe K ;
3. **un encodage de position relative ultramétrique**, dont la robustesse
   à la raréfaction des retours se mesure ;
4. **une famille de prétextes structurels exacts**, avec raccourcis
   et coût de préparation mesurés ;
5. **un décodeur fondé sur le vote démontré** (Proposition 7), sous réserve
   d'exporter les incidences et poids nécessaires.

Chacun de ces cinq points correspond à une ligne de la table de substitution de
[`ETAT_DE_LART.md`](ETAT_DE_LART.md) § 4, donc à une expérience contrôlée. Si
une ligne ne se paie pas, on la retire et on le dit. C'est l'objet de
[`MESURE.md`](MESURE.md).
