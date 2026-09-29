# Projection dure : symétrie, bande d'ambiguïté et certificat local

29 septembre 2026. Suite du [contre-audit multi-K](CONTRE_AUDIT_PROJECTION_MULTIK_20260929.md).
Théorèmes bornés et petites fixtures sur la tour exacte du commit `6206d1d11` ;
`public_status=not_claimed`, moteur inchangé, aucun GCP. Les rayons ci-dessous ne
sont pas les rayons carrés stockés par le moteur.

## 1. Une obstruction de symétrie, indépendante des étiquettes de sortie

Prendre X = {−1,0,1}, K = 2, r = 1/2 sur une droite de R³. Le niveau L₂(r)
possède deux composantes : les centres −1/2 et 1/2. Leurs couvertures discrètes
sont {−1,0} et {0,1}. La réflexion échange ces deux composantes et fixe le point 0.

Une affectation géométrique qui donnerait 0 exclusivement à l'une des deux
composantes ne peut commuter avec cette réflexion : son image devrait à la fois
rester la même et devenir l'autre composante. Même en ignorant complètement les
numéros de clusters, les partitions `{−1,0}|{1}` et `{−1}|{0,1}` sont différentes.

Hypothèse indispensable : la méthode dépend du nuage **non étiqueté**, ou est
invariante par réindexage en plus d'être équivariante par isométrie. L'isométrie
seule sur des points portant des IDs fixes n'interdit pas une règle choisissant
l'ID minimal. Cette règle brise alors l'invariance au réindexage. Un choix aléatoire
peut être équivariant en loi, pas fournir le choix déterministe intrinsèque interdit.

Échappatoires cohérentes mais de sens différent : laisser 0 singleton ou bruit,
différer son entrée jusqu'au LCA à r = 1, ou partager sa masse entre les deux branches.
Conserver les deux couvertures n'est plus une partition. Une sélection sur les
numéros de clusters ne résout rien : ce sont les ensembles et leurs dates qui
doivent se transporter sous l'isométrie.

## 2. L'attache doit porter une date admissible

Pour une sortie attachant un point à un nœud v d'un arbre de composantes, trois
conditions sont distinctes : propriétaire unique, date d'entrée, suivi des parents.
En coupe fermée, le représentant canonique vivant à la date t satisfait
`naissance(v) ≤ t < naissance(parent(v))`, sauf racine sans borne supérieure.

Après choix d'un LCA, prendre t au moins égal à sa naissance, puis monter vers
l'ancêtre vivant à t. Dès que t **atteint** la mort d'un nœud, il faut monter ;
cela précise le mot « dépasse » du contre-audit précédent. Le validateur produit
permet aussi l'égalité avec la date du parent pour son encodage de plateaux ;
cette souplesse de stockage ne rend pas l'enfant vivant en coupe fermée.

Des singletons avant l'entrée donnent des partitions totales, sans certifier
qu'ils sont des composantes de L_K. Comparer les étiquettes numériques n'éprouve
ni cette cohérence temporelle, ni l'équivariance de la partition, ni la stabilité
de ses dates de réunion.

## 3. Une bande relative ne suffit pas à la stabilité globale

Règle examinée : pour un point x, prendre les candidats de date au plus
`(1+η) α_min(x)`, puis attacher x au LCA de ces candidats.

### 3.1 Ne pas découvrir les candidats au fil des coupes

X = {0,8,18}, K = 2, η = 1/4. Les deux branches naissent aux rayons 4 et 5 et
fusionnent au rayon 9. Pour x = 8, la bande inclut les deux branches.
À r = 4, ne voir que la première branche ferait entrer x avec 0. À r = 5,
découvrir la seconde ferait remonter son attache au LCA **futur**, de date 9.
Retirer x après l'avoir joint à 0 casserait l'emboîtement. Le placer dans ce
LCA dès r = 5 antidaterait une fusion réelle. Il faut fixer la bande complète
avant l'affectation, puis suivre une seule lignée ; ou annoncer une autre
sémantique de fusion anticipée, qui n'est plus celle de FULL.

### 3.2 Agrandir la bande ne signifie pas grossir les partitions

Sur le même arbre fixé, si les ensembles de candidats sont emboîtés quand η
augmente, leurs LCA remontent vers les ancêtres et les entrées sont retardées.
À rayon fixé, un point entré dans les deux variantes appartient à la même
composante du squelette ; un point retardé reste singleton. La partition avec
la bande plus large **raffine** donc celle avec la bande plus étroite.

Exemple à r = 6 : η = 0 donne `{0,8}|{18}`, tandis que η = 1/4 donne
`{0}|{8}|{18}` avec x = 8 différé jusqu'à 9. Le mot « ancêtre » ne doit pas
faire conclure à tort à un grossissement des partitions au même rayon.
Ces conclusions supposent arbre fixé, candidats emboîtés et affectation durable.

### 3.3 Le bord de la bande crée une nouvelle discontinuité

X₋ = {0,8000,17999}, X₊ = {0,8000,18001}, K = 2, η = 1/4.
Pour le point 8000, α_min = 4000 et le bord de bande vaut 5000 dans les deux cas.
La seconde branche naît respectivement à 4999,5 et 5000,5. Elle est incluse
dans le premier cas et exclue dans le second. Le point est donc attaché au LCA
à 8999,5 dans X₋, mais à la première branche à 4000 dans X₊.

Déplacement maximal des données : **2** ; saut de date d'attache et de réunion
des points 0/8000 : **4999,5**. Sur une grille millimétrique, cela oppose
2 mm de perturbation à 4,9995 m de saut. La famille rationnelle obtenue en
remplaçant ±1 par ±ε démontre la discontinuité quand ε tend vers zéro.

Contre-exemple reproduit sur les deux tours natives exactes :
[`band_report.json`](../../../receipts/audit_continu_20260929/pool_head/band_report.json), [`minus.txt`](../../../receipts/audit_continu_20260929/pool_head/minus.txt), [`plus.txt`](../../../receipts/audit_continu_20260929/pool_head/plus.txt).
Rejeu : `probe_band.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY`.
Cette sonde applique la règle proposée hors moteur ; elle ne prétend pas
que cette règle serait déjà implémentée dans le produit.

## 4. Un certificat local q2 est néanmoins démontrable

Supposons des nuages X et Y en correspondance d'IDs, chaque point déplacé
d'au plus ε. Pour chaque point i, fixer un univers U_i de paires d'IDs.
Pour la paire j, noter c_j son milieu et α_j sa demi-distance ; poser
α_min = minimum des α_j et g_j = α_j − (1+η) α_min.

Les milieux changent d'au plus ε. Chaque α_j et leur minimum changent d'au
plus ε. Par inégalité triangulaire, chaque g_j change d'au plus `(2+η) ε`.
Si **tous** les g_j ont une marge absolue strictement supérieure à cette borne,
la sélection `g_j ≤ 0` conserve exactement les mêmes identités de paires.
Ce certificat est suffisant, pas nécessaire ; à η = 0 le minimiseur a g = 0,
donc cette forme de marge ne peut pas certifier la sélection du minimiseur.
Sur la fixture du § 3.3, le candidat frontalier a `|g| = 1/2`, contre une
borne `(2+1/4) × 2 = 9/2` : le certificat refuse précisément le cas instable.

Deux précautions empêchent de surinterpréter ce résultat :

- Si U_i signifie « les supports q2 présents au catalogue courant », il n'est
  pas fixe sous perturbation. Il faut certifier également cette admissibilité,
  ou définir U_i par les paires d'IDs indépendamment du catalogue. Les paires
  proches non Gabriel restent des ancrages valides, mais il faut les raccorder
  à FULL et payer la recherche ; le simple lookup de boule peut échouer.
- Pour une paire non Gabriel, son milieu peut appartenir à L₂ avant α_j si
  deux autres points sont déjà plus proches. « Même composante dès que les
  centres y sont » et « LCA des ancrages activés à α_j » ne sont donc pas
  strictement la même règle. Il faut préciser les dates d'activation.

## 5. Stabilité conditionnelle des ancrages datés, preuve

Soit A_i l'ensemble non vide des paires sélectionnées pour le point i, dont
les identités sont désormais certifiées inchangées. Définir u(i,j) comme le
premier rayon r tel que **tous** les ancrages de A_i et A_j soient activés
(`α_a ≤ r`) et leurs milieux appartiennent à une seule composante de L₂(r).
La diagonale naturelle donne une date d'entrée ; on peut la stocker à part
et mettre la diagonale de l'ultramétrique conventionnelle à zéro.

**Emboîtement.** Si i,j sont réunis et j,k sont réunis à un même rayon, les
deux composantes contiennent les milieux de l'ensemble non vide A_j. Ce
sont donc la même composante, qui réunit aussi i,k. Quand le rayon croît,
activations et composantes ne font que croître. L'inégalité ultramétrique
et la laminarité en découlent.

**Stabilité locale.** À centre spatial fixé, la deuxième distance du nuage
change d'au plus ε. Un chemin dans L₂ de X au rayon r demeure donc dans
L₂ de Y au rayon r+ε. Chaque ancien milieu est à distance au plus ε du
nouveau ; le segment qui les relie reste dans L₂ de Y au rayon r+2ε.
Tous les nouveaux milieux sont ainsi reliés à ce dernier rayon. Leurs
activations n'ont augmenté que d'au plus ε. Par symétrie entre X et Y :
`|u_X(i,j) − u_Y(i,j)| ≤ 2ε`, sous l'hypothèse d'identités sélectionnées fixes.
La même preuve vaut pour les dates d'entrée.

Ce résultat n'est **pas** une garantie globale de la bande, réfutée au § 3.3.
Il ne couvre ni changements d'effectif, ni outliers arbitraires, ni EOM,
ni candidats q3/q4 dont les centres n'ont pas la borne de déplacement des
milieux. Il ne démontre pas le coût du raccord, un avantage statistique,
ou le contrat 100 ms. C'est une possibilité de garantie locale clairement
conditionnée, à comparer au squelette core plus simple avant d'ajouter
une nouvelle voie produit.
