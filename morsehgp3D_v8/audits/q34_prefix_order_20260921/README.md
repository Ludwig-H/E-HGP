# Réponse A — garder la localité des témoins sans perdre le préfixe héritable

21 septembre2026. Suite de462c29a1, chantier33 en cours. Audit indépendant,
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`public_status=not_claimed`. Écritures dans audits/ uniquement ; GCP non utilisé.

**Oui aux deux ordres proposés par le constructeur**, si l'ordre est figé
par rectangle original et conservé dans tous ses descendants. Le pivot
ne doit pas être recalculé après subdivision en gardant l'ancien compte.
Deux constructions précises sont décrites et testées ci-dessous. Elles
restent des propositions, sans port d'héritage ni gain global qualifié.

La relecture statique du brouillon33, épinglée dans [SNAPSHOT.json](SNAPSHOT.json),
ne révèle pas de défaut : extrema H et composantes Xi, signes, promotions,
comparaisons4H/16Xi, masques locaux et sommes des nouveaux compteurs sont
cohérents. Legacy n'utilise pas les nouveaux registres ; les nouvelles
options sont validées même sur une voie inactive. Le corps33 est arrivé
pendant la revue ; l'avis porte sur son hash172ae93a…, pas sur le premier
corps32 encore présent. Aucun résultat de cette relecture ne qualifie
les futurs changements de33.

## 1. Invariant commun et contexte possédé

Soit une permutation fixe pi des n rangs de Z, déterminée par le rectangle
original. Pour chaque voie q, l'état conserve le compte c_q et une position
p_q, représentée par le navigateur ci-dessous. L'invariant reste celui de
l'[audit précédent](../q34_global_contract_20260921/PREFIXES_TEMOINS.md) :

$$\forall(a,b)\in A\times B,\quad c_q=\#\{0\leq j<p_q:H(a,b,z_{\pi(j)})>0\text{ et }\alpha_qH(a,b,z_{\pi(j)})^2>\Xi(a,b,z_{\pi(j)})\},\qquad c_q<T_q.$$

Chaque site consommé a un booléen uniforme sur le produit. À feuille Z
ambiguë, scinder A/B avant consommation ; transmettre chaque état de voie
indépendamment. La restriction des facteurs conserve les booléens et donc
le préfixe. Saturation rejette la voie ; fin sans saturation la conserve.
Les crédits ne deviennent jamais un compte initial du census d'une boule.

Le contexte partagé possède l'index, le pivot et les éventuelles références
de frères. Il vit jusqu'à la dernière tâche descendante. Ne pas conserver
une vue dans le moteur ou la pile du créateur. Les références de nœuds sont
relatives au même index ; rangs spatiaux et IDs originaux restent distincts.

## 2. Ordre circulaire : deux phases et aucun nouveau découpage géométrique

Pour le rang pivot r, prendre pi=(r,…,n−1,0,…,r−1). Chaque voie conserve
`(compte, curseur, phase)` ; r et le nœud de départ sont partagés.

- Phase suffixe : partir du **plus haut nœud dont range.first=r**, puis
  suivre `escape` après consommation et `left` en cas de raffinement.
  Sa suite couvre exactement [r,n). Partir systématiquement de la feuille
  r serait sûr mais perdrait des admissions de blocs, notamment à r=0.
- Phase préfixe : revenir une fois à la racine, parcourir [0,r). Si un nœud
  traverse r, le raffiner **avant toute borne** ; si `first>=r`, terminer.
  Seuls les nœuds entiers contenus dans la phase reçoivent un certificat.
- Les phases peuvent différer entre voies et entre produits descendants.
  Le passage à la deuxième phase ne remet pas le compte à zéro.

Pendant la descente choisissant la feuille pivot, le départ du suffixe est
le nœud atteint après le dernier virage à droite, ou la racine si aucun.
Il ne faut ni un index Z supplémentaire ni des boîtes partielles : la
frontière r est traitée par les nœuds originaux. Le coût des raffinements
structurels de cette frontière est distinct du coût des bornes.

Le préfixe consommé est [r,t) en phase1, puis [r,n) union [0,t) en phase2.
Il est disjoint et couvre finalement tous les sites. Les tests géométriques
habituels et l'invariant s'appliquent donc. Au plus une chaîne de frontière
est raffinée lors d'une traversée complète ; après splits A/B, ce coût peut
se répéter. Ne pas convertir cette observation locale en borne globale.

## 3. DFS orienté sur le chemin du pivot

Une alternative garde les gros ancêtres testables avant de choisir les
sous-arbres. Sur le chemin racine→pivot seulement, visiter d'abord le fils
contenant r ; dans tous les autres sous-arbres, utiliser le DFS global.
Cet ordre est figé et n'est pas un tri exact par distance.

Préparer une fois la liste S_d des frères du chemin pivot. L'index u16
actuel borne sa profondeur à48 : au plus48 IDs partagés, pas48 par paire.
Le coût de recherche du pivot, de construction, d'allocation éventuelle
et de possession de ce contexte reste à payer. Chaque voie garde un
compte, un curseur et une phase/profondeur : état mutable constant.

| Phase | Descente ambiguë | Après consommation d'un nœud entier |
|---|---|---|
| Chemin pivot, profondeur d | Fils contenant r, profondeur d+1 | Commencer S_(d−1), ou finir si d=0 |
| Frère S_d | Fils gauche ordinaire | Suivre escape ; à escape(S_d), commencer S_(d−1), ou finir |

**La frontière escape(S_d) doit être traitée avant tout autre nœud.** Cet
index peut désigner le sous-arbre pivot déjà consommé : le suivre comme
une continuation ordinaire peut omettre des sites ou en visiter deux fois.
Aucun lien parent ni nouvelle descente depuis la racine n'est requis : les
frères sont pris du plus profond au moins profond, avec transitions O(1).

Chaque nœud est un intervalle contigu dans ce DFS orienté. Le chemin
initial et les frères successifs partitionnent la population ; les nœuds
admis sautent exactement leur intervalle. Cela suffit à la même preuve
de préfixe, y compris si un gros ancêtre est consommé d'un coup.
Pour huit feuilles et r=5 : ordre5,4,6,7,0,1,2,3, contre5,6,7,0,1,2,3,4
pour le cercle. Ces deux proximités dans l'arbre restent des heuristiques.

## 4. Vérification du protocole

[order_model.py](order_model.py) utilise des navigateurs sans pile Z.
Les arbres, listes de facteurs, cartes de positions servant à l'oracle et
copies JSON aux pauses sont du matériel de test, pas le format proposé
pour les tâches produit. Les décisions universelles sont obtenues par
énumération exhaustive : aucune conclusion de vitesse de ce modèle.

7 056 parcours structurels couvrent tous les pivots d'arbres équilibrés de1 à48sites
et plusieurs politiques d'admission de blocs. Le modèle de témoins vérifie
960 exécutions : huit fixtures, quatre pivots, deux ordres, cinq K et trois
quanta. Les états passent par sérialisation aux pauses. Les deux voies
rencontrent effectivement des splits après changement de phase, avec
curseurs différents et avec une voie déjà terminée. Le compteur
`pivot_path.split_after_wrap` désigne la phase des frères, sans retour
circulaire. Comparaison des paires
survivantes à l'oracle ponctuel indépendant utilisant les distances carrées.

Deux mutants de navigation sont détectés : borne avant découpe du cercle,
oubli de frontière du frère. Le premier harnais de ce second mutant
admettait directement la racine et ne l'exerçait pas : préflight échoué et
source conservés dans [MODEL_PREFLIGHT.json](MODEL_PREFLIGHT.json), puis
correction du seul scénario de test. Les quatre autres erreurs d'héritage
sont déjà couvertes par le modèle précédent, sans requalification implicite.

[Reçu normal/−O](MODEL_CHECKS.json). Les listes de sites et comptes sont
identiques pour les différents quanta ; seuls les nombres de pauses changent.

## 5. Sonde d'ordre sur LiDAR

La sonde indépendante [order_probe.cpp](order_probe.cpp) compare les ordres
sur des paires échantillonnées de rectangles réellement émis par la WSPD.
Elle conserve le pivot du rectangle original. Les certificats affines
proviennent d'une copie explicite de la primitive33, avec une seule
adaptation d'include ; ils sont identiques dans les quatre ordres.
Le front et l'index proviennent de la bibliothèque32 épinglée.

La sonde traite les voies séparément, avec des piles locales, sans héritage
A/B ni production de boules. Ses nombres de visites comparent l'ordre ;
ils ne sont ni le coût du filtre partagé33 ni celui du futur état compact.
Le census ponctuel indépendant contrôle chaque décision par paire.
La sélection comporte deux strates, produits singleton et multiples ;
ces petits échantillons ne fournissent pas un estimateur pondéré du travail
global. Les résultats clos sont ci-dessous.


### Résultat clos :18 cas,3 800 requêtes de voie

Scans08/000000,000100,000200 préparés séparément, n8k/16k/32k, K5/10,
s8. Les fichiers déjà présents sont lus sans modification et hachés.
Aucune correspondance entre points, grille axiale ou identité inter-scan
n'est utilisée. La campagne ne teste pas ici leur superposition.

Les32 rectangles retenus par strate et par cas donnent1 152 contextes
originaux et3 800 requêtes de voie ; chaque paire est jugée sur le nuage
entier par l'oracle, soit70 560 000 tests de site cumulés. Tous les ordres
rendent le même compte saturé. Six petites fixtures ×K3/10 passent aussi
sur Release et Clang ASan/UBSan/LSan, soit24 appels et700 requêtes de voie
cumulées ; leur oracle Python, par produit vectoriel, vérifie séparément
l'oracle C++ utilisant les distances. Aucun test TSan dans cette campagne.

| Ordre, sur les3 800 requêtes LiDAR | Bornes H | Bornes Xi | Découpes/écarts de phase | Distances de boîtes locales | Tests de rang |
|---|---:|---:|---:|---:|---:|
| DFS global | 206 033 | 150 228 | 0 | 0 | 0 |
| Proche du milieu de la paire | 136 183 | 107 253 | 0 | 171 596 | 0 |
| Circulaire au pivot parental | 114 575 | 73 441 | 35 952 | 0 | 0 |
| Chemin du pivot parental | 141 538 | 111 249 | 0 | 0 | 87 575 |

Pour les deux variantes au pivot, ajouter40 014 tests de distance de
préparation, une fois par rectangle original. Ils ne sont pas imputés
aux ordres global/proche-du-milieu. Le départ circulaire est préparé au
même passage, sans redescente par paire. Ces catégories ont des coûts CPU
différents : ne pas les additionner pour annoncer un facteur de temps.
Le registre contient aussi les admissions, feuilles, pics de pile et le
front complet utilisé pour sélectionner l'échantillon.

Le cercle peut afficher moins de bornes même quand la recherche épuise Z :
ses découpes remplacent alors les tests géométriques de certains ancêtres.
Cela ne prouve pas que tout le travail disparaît. En revanche, pour les
ordres global, proche-du-milieu et chemin-pivot, les comptes géométriques
sont identiques sur chaque requête non saturée, contrôle effectué par
l'analyseur. L'ordre ne change alors que les transitions et la préparation.

Pour les2 344 requêtes saturées issues de **rectangles multiples**, les
rapports de bornes H à l'ordre proche-du-milieu sont :

| Ordre | Rapport des sommes | Minimum / médiane / maximum des18 cas |
|---|---:|---:|
| Global | 2,094 | 1,774 /2,148 /2,472 |
| Circulaire | 0,945 | 0,675 /0,890 /1,283 |
| Chemin du pivot | 1,078 | 1,017 /1,078 /1,142 |

**Suite conseillée : conserver le DFS global comme référence et essayer
l'héritage avec un ordre parental proche du pivot.** Le chemin-pivot garde
ici une localité plus régulière que le cercle ; le cercle reste une
alternative simple avec moins de bornes sur l'ensemble de cet échantillon.
Le port doit mesurer le coût total avec les contextes possédés, transitions,
recherches conjointes et aval q3/q4. Cette sonde ne choisit pas un défaut,
ne mesure aucun gain de temps stable et n'établit aucune croissance globale.

[Manifeste et hashes](capture/MANIFEST.json), [fermeture](capture/COMPLETION.json),
[analyse détaillée](ANALYSIS.json), [lectures normal/−O](READ_CHECKS.json.gz)
et [analyses normal/−O](ANALYSIS_CHECKS.json.gz). Les42 appels C++ (24 petits,
18 LiDAR) et trois commandes de préparation restent conservés avec leurs
sorties brutes. Les14 dépendances épinglent les bibliothèques et en-têtes
réellement utilisés, sans copier implicitement le moteur constructeur.

Depuis la racine, les relectures et le modèle sont rejouables :

```sh
python3 -B morsehgp3D_v8/audits/q34_prefix_order_20260921/campaign.py read
python3 -B -O morsehgp3D_v8/audits/q34_prefix_order_20260921/analyze.py
python3 -B -O morsehgp3D_v8/audits/q34_prefix_order_20260921/order_model.py
```

Le lecteur requiert les entrées et binaires locaux épinglés ; le dossier
`.inputs` contient seulement les six fixtures du modèle précédent, les
LiDAR restent dans leur dossier d'origine. `campaign.py run` construit
dans `.build` et refuse d'écraser `capture` ; les captures closes ne se
reconstruisent pas sur place. Ni le modèle ni cette sonde ne produisent
le catalogue, FULL ou une qualification GPU/50k.

## 6. Réponse à la nouvelle question q4 : produits seeds × cellules

**Piste sûre pour q4, avec une primitive déjà présente.**
[Q4LocalGeometry::node_bounds](../../src/lanes/q4_local_partition.cpp)
calcule précisément la borne sur boîte spatiale X × cellule fermée C.
La forme est affine dans le centre : prendre les quatre coins de C suffit.
À centre fixé, elle est quadratique séparable en x : maximum aux extrémités,
minimum au sommet rabattu dans l'intervalle, avec arrondi inférieur sûr.
Tester seulement les coins de X ne donne pas un minorant valide.
[Sources relues et hashes](Q4_REVIEW.json), pas de nouvelle gate exécutée.

`minimum>0 || maximum<0` rejette une **incidence X×C**. Rejeter X entier
requiert l'absence d'incidence restante dans toute l'union des feuilles
potentiellement admissibles. Ignorer les cellules `Outside` ou `Deep`
est sûr selon leur certificat existant. Un budget épuisé conserve les
produits indécis. Ni une borne ratée ni un fragment hors d'une cellule
ne justifient la disparition de X pour toutes les autres.

Les contacts zéro et les cellules fermées restent actifs dans la recherche.
`owns_right/owns_top` servent seulement à départager l'émission. Reprendre
la [fixture du q4 isolé à huit sites](../q4_kernel_composition_20260920/README.md),
au paramètre(0,−1), parmi les contre-tests du futur port : pas de suppression
sous prétexte qu'une droite ne traverse aucun intérieur de cellule.

Le domaine `Positive` de l'arête garde **toutes les complétions de sa
lentille**, même obtuses. Il contient les centres q4 positifs propriétaires ;
le reconstruire à partir des seules seeds du bloc X pourrait en perdre.
Ce rejet ne dit rien sur q3 et ne retire aucun témoin du census.
Les contrôles d'acuité, propriété, positivité et seed canonique restent
nécessaires lors du relais vers le balayage.

Un relais utile prend directement le fragment de feuille déjà trouvé.
L'entrée actuelle `run_q4_local_seed_candidates` relance la visite à la
racine : l'appeler pour chaque incidence annulerait du partage et risquerait
des émissions répétées. Le futur moteur devrait accepter la famille/ligne
préparée et le fragment certifié, en conservant les règles d'émission de28.
Compter les constructions effectives de famille, y compris si la même seed
est rencontrée dans plusieurs cellules ; ne pas les masquer par le seul
nombre d'IDs de seeds distincts.

Le vecteur de coût à mesurer comprend préparation de l'atlas et des
constantes par cellule, couples X×C visités, bornes, constructions de seeds,
incidences seed–feuille, scans actifs, tris, sorties et stockage simultané.
Le pire cas peut atteindre le produit des tailles des deux arbres ; les
contacts sur frontières peuvent multiplier les incidences. Un parcours
conjoint promet du partage, sans fournir à lui seul une borne globale ni
un gain LiDAR. Ce complément est une réponse de conception, pas un port.
