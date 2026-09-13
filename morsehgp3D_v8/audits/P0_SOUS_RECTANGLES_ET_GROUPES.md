# Réduire le résidu : sous-rectangles et témoins collectifs

13 septembre 2026. Audit indépendant v8, `cpu_reference`,
`quantized_u16_input_only`, `public_status=not_claimed`. Proposeurs q2 et
certificats collectifs issus de la
[contre-fixture transverse de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md).
Les sections 6 à 8 étendent ces preuves aux groupes à moments fixes,
aux nappes et à l’addition des colonnes exactes du filtre axial en cours.
Aucun producteur général ni résultat de tour FULL n’est qualifié ici.

## 1. Un grain plus petit suffit pour q2 sur les rangées

Considérer les sites A_i=(1000,i,0), B_j=(60000,j,0), pour 0≤i,j<m,
avec s∈{8,10,12} dans cette étude.
La séparation v8 exige 59000≥s(m−1). L’autre audit a montré que les
crédits universels sur le facteur opposé entier sont tous nuls, tandis
que la profondeur diamétrale est 2 max(0,|i−j|−1).

Fixer h=Kmax, w=⌈h/2⌉ et L=w+1. Partager les rangs d’A en plages
I=[i0,i1) de longueur au plus L. Pour chacune, partitionner B en :

| Partie | Rangs de B | Rangs des témoins proposés, dans A et B |
| --- | --- | --- |
| Queue gauche | [0,max(0,i0−w)) | [i0−w,i0), seulement si la queue existe |
| Milieu conservé | [max(0,i0−w),min(m,i1+w)) | Aucun |
| Queue droite | [min(m,i1+w),m) | [i1,i1+w), seulement si la queue existe |

Chaque queue reçoit 2w **IDs distincts**. Construire la boîte Z de ces
sites, puis appeler le prédicat positif sur les boîtes U,V,Z. Sur les
rangées exactes, son minimum H vaut w>0 : les 2w sites sont intérieurs
à toutes les boules diamétrales du sous-produit. Le rejet collectif est
sûr puisque 2w≥h. Une proposition qui ne passe pas le prédicat reste
entièrement dans le résidu, même si le résultat s’appelle NoCredit.

Cette règle ne confond donc pas classement par rang et décision : sur
d’autres coordonnées, les rangs proposent et la géométrie certifie.
Les trois parties de B sont disjointes et couvrantes, comme les plages
d’A. Le rectangle WSPD initial garde la propriété de chaque paire ;
aucune nouvelle WSPD n’est nécessaire pour changer le grain du certificat.

**Coût payé.** Trier les IDs originaux coûte O(m log m). Préparer les
boîtes préfixes/suffixes de B coûte O(m) et donne les boîtes des queues
en O(1), sans les rescanner. Les boîtes d’U visitent m sites au total ;
les témoins visitent au plus 4w⌈m/L⌉ sites. Il y a au plus 3⌈m/L⌉
descripteurs et 2⌈m/L⌉ tests de boîte, mémoire O(m). La validation de
l’entrée est une préparation distincte O(m log m) dans le prototype.

Sur les rangées, les candidates sont au plus m(L+2w), donc O(hm).
Le census q2 conserve ensuite exactement la bande |i−j|≤w. À n=512,
h=10, L=6 : **4 032 candidates**, puis **2 786 paires sous seuil**,
contre 65 536 candidates des trois plans universels initiaux. La première
valeur provient de la somme des produits des plages ; la seconde de la
formule de profondeur indépendante. L’exhaustif de validation n’est pas
le consommateur produit proposé.

Le [prototype autonome](p0_rectangle_probe.cpp) appelle réellement le
prédicat v8 ; son [reçu](P0_RECTANGLE_CHECKS.json) conserve trois builds
O2/UBSan/ASan+UBSan réussis : 43 plans, 82 200 tests des sites crédités
par blocs, 1 673 156 tests du census q2 indépendant. Le mutant exécuté
du proposeur à frontière large perd une paire réelle sur une entrée
aux espacements irréguliers ; le prédicat produit n’est pas modifié.
Le juge contrôle couverture, IDs, crédits et paires finales sur petites
instances. Il garde q3/q4 entièrement
indécis sur ces rangées : le certificat Wq ponctuel ne leur donne aucun
témoin, même après restriction du facteur opposé. La partie suivante
introduit un **autre certificat**, pas une réinterprétation de Wq.

Deux constructions plus grandes passent à s12, sans expansion ni census :

| n=2m, h10 | Descripteurs | Candidates construites | Taille de bande prouvée |
| ---: | ---: | ---: | ---: |
| 8 000 | 1 998 | 63 936 | 43 970 |
| 8 192 | 2 046 | 65 472 | 45 026 |

Ces tailles respectent u16 et la séparation ; les mêmes rangées à n16k
ou n32k ne la respecteraient plus. Aucun temps de ces tests ne qualifie
la performance du produit, et la dernière colonne n’est pas un census
exécuté sur les grandes entrées.

## 2. Un groupe peut témoigner sans témoin ponctuel universel

Pour une sphère de centre c et de rayon R, écrire la puissance
$P(z)=\Vert z-c\Vert^2-R^2$. Ses intérieurs stricts ont P(z)<0.
Supposer P(a)=P(b)=0. Si des poids α_z≥0 satisfont les trois conditions
suivantes, au moins un site du groupe est strictement intérieur :

$$\sum_z\alpha_z=1,\qquad \sum_z\alpha_z z=(1-\lambda)a+\lambda b,\qquad \sum_z\alpha_z\Vert z\Vert^2<(1-\lambda)\Vert a\Vert^2+\lambda\Vert b\Vert^2,\qquad 0<\lambda<1.$$

**Preuve.** Dans la somme pondérée des puissances, les termes linéaires
en c et constants s’annulent par les deux égalités. Il reste exactement
la différence strictement négative de la troisième condition. Des
puissances toutes positives ou nulles auraient une moyenne positive ou
nulle, contradiction. La preuve ne dépend ni du centre, ni du rayon,
ni de la régularité du nuage. La comparaison porte sur l’interpolation
des normes **carrées des extrémités**, pas sur la norme du barycentre.

Ce certificat a le quantificateur « pour chaque sphère, au moins un
site de ce groupe », et non « un même site pour toutes les sphères ».
Un groupe apporte **un** crédit garanti. Des groupes aux ensembles d’IDs
disjoints apportent autant de crédits que de groupes. Le certificat
s’applique aux boules q2/q3/q4 dont a et b sont réellement sur la frontière.
Si un membre du groupe est un autre sommet définissant, sa puissance est
nulle ; l’intérieur strict garanti est nécessairement une autre identité.
Il n’y a donc aucun retrait artificiel de q−2 à appliquer au crédit.

## 3. Certificat collectif paramétrique sur les rangées

Pour i<k<j, poser s=k−i et t=j−k. Le groupe {A_k,B_k}, avec poids
t/(s+t) et s/(s+t), vérifie pour **toute** sphère passant par A_i,B_j :

$$\frac{tP(A_k)+sP(B_k)}{s+t}=-st<0.$$

Les rangs strictement intermédiaires forment des groupes disjoints :
la profondeur de toute telle sphère est donc au moins |i−j|−1.
Les points W3/W4 individuels échouent pourtant tous : pour A_k, leur
condition exigerait respectivement 3(j−k)²>59000² ou 2(j−k)²>59000²,
impossible sous la séparation de cette famille ; argument symétrique
pour B_k. Le certificat collectif lève ce défaut du quantificateur.

Pour une voie active q, employer h_q=Kmax+2−q. Rejeter au seuil h_q
garantit toute la tour demandée. La même partition en plages, avec w=h_q
et L=w+1, donne une proposition de résidu O(h_q m) pour ces voies :
h_q rangs intermédiaires suffisent. Cette réduction est **prouvée pour
la famille**, sans implémentation du nouveau certificat dans le produit.
Les rangées sont coplanaires ; elles ne fournissent pas une mesure
non vacante de production q4. La fixture suivante vérifie cette portée
géométrique sur un vrai support de dimension trois.

## 4. Fixture q4 positive où les deux tests ponctuels échouent

Sites u16 : a=(0,4,4), b=(8,4,4), c=(4,7,9), d=(4,1,9),
z+=(4,7,4), z−=(4,1,4). Le tétraèdre abcd est non dégénéré ; ab est
l’unique arête maximale. Son centre est (4,4,29/5), son rayon carré
481/25, ses coefficients barycentriques 8/25,8/25,9/50,9/50 sont
strictement positifs. Il s’agit donc bien d’un support positif q4.

Pour chacun des deux z, H=7 et Ξ=576 : W3 et W4 échouent. Mais leur
moyenne est le milieu d’ab et leur marge collective vaut −7. Chaque
sphère passant par a,b contient donc au moins un des deux sites ; sur
la sphère d’abcd ils sont tous deux intérieurs. À Kmax=3, h_q4=1,
le groupe suffit à éliminer ce candidat. Les deux témoins sont extérieurs
aux facteurs singleton {a},{b} : cela qualifie le certificat géométrique,
pas sa recherche automatique dans les crédits locaux actuels.

## 5. Qualification et raccord proposé

Le [juge rationnel](p0_collective_probe.py) et son [reçu](P0_COLLECTIVE_CHECKS.json)
passent en Python normal/−O : 204 groupes de rangs, 2 040 sphères,
816 puissances nulles, deux supports q4 positifs non coplanaires.
Un exercice supplémentaire fait d’un membre du groupe un sommet définissant.
Trois règles erronées sont réfutées par leurs fixtures : marge large,
condition de barycentre omise, groupe compté deux fois. Ce sont des
mutants du modèle, pas des mutations du code produit.

Objets à comparer : une tâche de sous-produit avec propriétaire et masque
de voies ; une proposition de groupe avec IDs et relation affine exacte ;
un crédit validé associé à ce groupe. Sélectionner des groupes aux
supports d’IDs disjoints, ou prouver séparément le nombre d’intérieurs
dans leur union. Dédupliquer l’union ne suffit pas : deux groupes
distincts peuvent ne contenir qu’un même site intérieur. Un ancien compte A/B ou de cœur ne
s’additionne pas sans contrôler le recouvrement des populations.

Le choix des groupes et leur recherche restent des coûts ouverts. Ne
pas énumérer tous les couples/triplets de témoins pour les trouver.
La relation générale ci-dessus porte sur une paire fixée ; sa validité
sur une boîte de paires exige une preuve supplémentaire. Sur les rangées,
l’identité paramétrique et l’ordre strict des rangs fournissent cette
preuve. Le certificat des coins de Wq ne se transfère pas automatiquement
à un groupe. La section suivante établit un transfert sous des hypothèses
précises, avec des poids fixes. Aucun gain global, ni contrat 50k ou massif annoncé.
GCP non utilisé.

## 6. Extension aux blocs généraux par les moments d’un groupe

**Nouvel apport après la publication produit 3589a2c9.** On peut relâcher
l’alignement exact du barycentre sur la corde lorsque l’on se limite aux
boules réellement visées par W3/W4 : support positif de cardinal q et
arête ab maximale. Ce sont des hypothèses indispensables du nouveau
certificat ; le théorème de la section 2 avait une portée plus forte sur
les sphères, avec une condition de barycentre plus contraignante.

Poser m=(a+b)/2, D=|a−b| et c=m+v. Par la
[variance du support positif](../../morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md),
R²≤(q−1)D²/(2q). Comme v est perpendiculaire à ab, on obtient
|v|²≤D²/12 en q3 et |v|²≤D²/8 en q4. Pour un groupe à poids fixes
α_z≥0, de somme 1, définir H̄=Σα_z(z−a)·(b−z) et
C̄=Σα_z((z−a)×(b−z)). La puissance moyenne vérifie :

$$\sum_z\alpha_zP(z)\leq-\overline{H}+\frac{\Vert\overline{C}\Vert}{\sqrt{\kappa}},\qquad \kappa=3\ (q3),\quad\kappa=2\ (q4).$$

En effet, si x=Σα_z z, la somme des puissances vaut
−H̄−2v·(x−m). La composante transverse de x−m a pour norme |C̄|/D ;
la borne précédente et Cauchy–Schwarz donnent l’inégalité. Donc
**H̄>0 et κH̄²>|C̄|²** certifient au moins un intérieur strict du groupe.
En q2, v=0 et le test H̄>0 suffit. Les carrés s’appliquent après la
moyenne ; H̄=H(x)−Σα_z|z−x|², donc remplacer le groupe par son seul
barycentre en oubliant sa dispersion serait incorrect.

**Coins valides avec les mêmes poids.** H̄ et C̄ sont affines séparément
en a et en b. Le domaine H>0, √κH>|C| est convexe. À b fixé, les huit
coins d’U donnent donc toute sa boîte ; puis les huit coins de V donnent
tout U×V. Les 64 tests suffisent avec **un groupe et un jeu de poids
uniques pour le sous-rectangle**. Rechercher de nouveaux poids à chaque
coin ne prouve rien pour ses paires intérieures.

**Objet préparé et coût.** Avec des poids entiers w_z≥0, stocker cinq
moments : S=Σw_z>0, Z=Σw_z z et Q=Σw_z|z|², ainsi que le handle des
IDs et des poids qui les ont produits. Pour chaque paire de coins :

$$H_S=(a+b)\cdot Z-Q-S(a\cdot b),\qquad C_S=Z\times b+a\times Z-S(a\times b).$$

Les tests sont H_S>0 et κH_S²>|C_S|², sans division. Les moments se
préparent en O(g) pour g membres ; les tests aux coins ne parcourent plus
ces g sites. H_S et C_S sont partagés entre les voies, seuls leurs
comparateurs et seuils de nombre de groupes diffèrent. Les moments de
deux populations s’ajoutent, mais leur somme certifiée ne garantit
toujours **qu’un intérieur**, jamais S ou le cardinal du groupe.

Une borne arithmétique suffisante sur u16 est S≤2²⁸, poids non négatifs.
Avec M=65535, |H_S|≤3SM², chaque coordonnée de C_S est au plus 2SM²
en valeur absolue. Les expressions carrées sont bornées par 27S²M⁴<2¹²⁵
et 12S²M⁴<2¹²⁴. Des intermédiaires signés 128 bits, élargis avant
multiplication, suffisent ; cette borne est propre aux moments, sans
héritage automatique des limites du prédicat ponctuel. Un dépassement
de cette porte impose un autre calcul certifié ou le maintien de l’indécision.
Ces poids appartiennent à la preuve ; les points conservent leur comptage
unitaire. Aucun profil HGP pondéré n’est introduit.

**Contrôles rationnels.** Le même juge passe en normal/−O avec deux
nouveaux supports q4 positifs, 14 contrôles de coins de boîtes dégénérées,
75 évaluations sur des paires rationnelles dans les boîtes, 12 contrôles
de puissances et trois rejets de poids invalides. Les contre-fixtures
réfutent l’oubli de dispersion, la moyenne des carrés de H et les poids
réoptimisés aux coins. Ce dernier cas possède un véritable support q4
positif au milieu du bloc, dont les deux témoins proposés sont extérieurs.
Le modèle utilise des entiers non bornés ; la porte 128 bits est une
preuve de borne, pas un résultat de test C++.

Cet objet rend explicite ce qui peut être partagé entre tâches : une
population immutable, ses moments et son certificat de poids ; puis des
références de sous-produits et des masques de voies. Il reste à choisir
les groupes sans recherche combinatoire exhaustive, à assurer leur
comptage sûr lors de l’addition des crédits et à mesurer le résidu et
l’aval. Les poids paramétriques des rangées de la section 3 ne sont pas
des poids fixes : leur preuve propre reste nécessaire. Aucun port produit,
test de débit ou résultat GPU n’est déduit de ce lemme.

L’[autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md)
apporte deux compléments compatibles : des capacités par ID permettent
de créditer des groupes recouvrants ; préserver le barycentre et diminuer
Q réduit un certificat de moments à quatre IDs au plus. Cette compression
conserve tous les tests stricts du bloc, sans garantir le coût de recherche
ni la borne sur les nouveaux poids entiers.

## 7. Nappes q2 : une famille u16 valable aux trois tailles demandées

Pour répondre à la reprise du constructeur : prendre deux copies de la
grille entière [0,N_y)×[0,N_z), aux abscisses 1000 et 60000. Pour deux
sites de coordonnées transverses u,v, un témoin de coordonnées transverses
z dans l’un des deux plans vérifie H=|u−v|²/4−|z−(u+v)/2|².
Une fenêtre de trois coordonnées
entières par axe, dont le départ est
clip(floor((u_j+v_j)/2)−1,0,N_j−3), reste à distance carrée au plus 8
du milieu, bords inclus. **Déplacer la fenêtre entière** préserve ses
neuf sites distincts ; clamper individuellement les sites créerait des doublons.

Si |u−v|²>32, la boîte des deux fenêtres contient 18 témoins stricts,
suffisants pour h_q2≤10. Le terme axial de H est positif ou nul à
l’intérieur du segment entre les deux plans. Ce certificat est donc
aussi valable pour toute la boîte de témoins, pas seulement ses sites.
L’égalité 32 reste indécise. Il existe 101 offsets entiers de norme
carrée au plus 32 ; sommer leurs placements possibles donne :

| n total | Grille par facteur | Produit initial | Candidates du disque d’offsets |
| ---: | --- | ---: | ---: |
| 8 000 | 50×80 | 16 000 000 | 373 060 |
| 16 000 | 100×80 | 64 000 000 | 764 960 |
| 32 000 | 125×128 | 256 000 000 | 1 555 294 |

Comptes fermés, sans nouvelle exécution C++ ni census :
101N_yN_z−242(N_y+N_z)+520. Pour la plus grande grille, le diamètre
carré vaut 31505 ; 59000²>12²·31505, donc s8/10/12 passent et u16
suffit. Ces nombres ne sont ni les survivantes exactes q2 ni la sortie FULL.

La formule d’offsets est une issue propre à la grille, pas un algorithme
générique déjà acquis. Dans un arbre de boîtes, proposer une fenêtre Z
d’IDs réellement présents pour U×V puis demander H_min(U,V,Z)>0 permet
un rejet sans développer le produit. Compter les sites trouvés, jamais
les 18 positions attendues sur une nappe incomplète. Un échec conserve
le sous-produit ou le raffine ; seul le masque q2 est éliminé. Les
visites d’index et de raffinements sont précisément le travail à mesurer,
pour ne pas recréer m² recherches derrière le choix des témoins.

Le [prototype de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_NAPPES_2D.md)
traite déjà les nappes tronquées réellement utilisées par le constructeur,
avec certificats de boîtes et caches de queues. La présente grille entière
fournit une preuve complémentaire ; ses comptes ne sont pas une comparaison
appariée avec ce prototype.

## 8. Raffinement axial en cours : additionner les colonnes exactes

**Apport à la version non publiée après 7f4ba045.** Le nouveau filtre
`axis_q2.cpp` conserve les paires lorsque chaque axe fournit moins de h
témoins, où h est le seuil q2 diminué du crédit de cœur. Ce rejet est sûr,
mais les colonnes exactes permettent une addition plus forte. Contrairement
à des tubes épais ou des groupes arbitraires, deux droites coordonnées
distinctes passant par a se rencontrent seulement en a. Ce site est exclu ;
les autres IDs sont donc disjoints entre les trois axes.

Pour l’axe j, soit C_j(a) l’ensemble des sites de A autres que a qui
partagent avec a leurs deux autres coordonnées. Définir :

$$c_j(t)=\#\left\lbrace z\in C_j(a):\min(a_j,t)<z_j<\max(a_j,t)\right\rbrace.$$

Chaque site compté vérifie exactement H=(z_j−a_j)(b_j−z_j)>0 pour t=b_j,
quelles que soient les deux autres coordonnées de b. Par disjonction,
**c_x(b_x)+c_y(b_y)+c_z(b_z)≥h suffit au rejet**. Le cœur reste extérieur
à A∪B. Une saturation individuelle des comptes à h préserve cette décision.
Les égalités avec une coordonnée de témoin restent exclues du compte strict.

Fixture minimale, Kmax=2 et sans cœur : a=(1000,1000,1000),
z_y=(1000,1001,1000), z_z=(1000,1000,1001) dans A et
b=(60000,1002,1002) dans B. y et z fournissent chacun un témoin, x aucun,
donc le filtre actuel conserve (a,b) ; la somme en certifie deux et le
rejette. Les deux valeurs de H valent 1, les sites sont distincts et la
séparation s12 est satisfaite. C’est une possibilité de réduire le résidu,
pas une erreur de sûreté du filtre conservateur.

**Requête sur l’index B, sans développer les paires.** Pour une boîte V,
poser l_j(V)=0 si son intervalle j contient a_j, sinon le compte c_j au
bord le plus proche de a_j ; poser u_j(V)=max(c_j(V.low_j),c_j(V.high_j)).
Le compte décroît vers a_j et croît en s’en éloignant. Ainsi la somme des
l_j est le minimum du compte axial sur V et la somme des u_j son maximum.

- Si la somme des minima atteint h, rejeter tout le nœud.
- Si la somme des maxima reste sous h, émettre sa plage dans la permutation B.
- Sinon, visiter ses deux enfants ; à une feuille les deux sommes coïncident.

Les plages émises sont disjointes et représentent exactement le résidu
du certificat axial additif. Il n’est pas nécessaire de construire les
O(h³) cellules d’une grille de seuils, ni de parcourir toutes les paires
pour choisir le rejet. Les tâches d’ancres peuvent partager le même index
B immutable et adresser leurs résultats par préfixes de tailles.

**Préparation et coût proposé.** Conserver les trois permutations par
colonne déjà calculées, ainsi que rang et limites de colonne de chaque
ancre, coûte O(m) mémoire pour m sites de A. Une vue sur au plus h voisins
de chaque côté suffit pour un compte saturé ; une recherche binaire y
coûte O(log(h+1)). Aucun tableau de h copies par ancre n’est obligatoire.
Avec J visites de nœuds et D fragments émis, le coût proposé, préparation
de l’index B incluse, est O(m log m+48|B|+J log(h+1)+D), avant census.
La borne u16 limite la profondeur de l’index ; elle ne borne ni J ni D
linéairement. Une nappe sans colonnes exactes peut toujours garder tout A×B.

Sur les **mêmes grilles entières** que la section 7, le compte axial vaut
(|Δy|−1)_+ + (|Δz|−1)_+. À h10, 261 décalages le laissent sous le seuil,
contre 441 pour les tests d’axes isolés. Leur somme de placements donne
261N_yN_z−990(N_y+N_z)+2860 :

| n total | Axes isolés, compte fermé | Somme des axes, compte fermé |
| ---: | ---: | ---: |
| 8 000 | 1 475 800 | 918 160 |
| 16 000 | 3 124 300 | 1 912 660 |
| 32 000 | 6 483 670 | 3 928 390 |

Ces nombres ne sont pas des mesures de l’implémentation C++, ni le census
des survivantes. Les queues A/B de l’autre auditeur et les fenêtres de la
section 7 utilisent d’autres témoins et peuvent mieux réduire ce résidu.
Cette amélioration se juge sur son travail total et se raccorde au filtre
actuel sans imposer un gagnant général. Elle reste propre à q2.

Ne pas transférer l’addition à des tubes voisins, ni ajouter ces comptes
aux crédits Tubes/Pool/Dual sans leurs IDs : le même site peut y être
compté de nouveau. Deux filtres sûrs restent combinables en intersectant
leurs résidus ; la somme de leurs crédits exige une preuve supplémentaire.

**Contrôle indépendant.** Le [modèle entier](p0_axis_union_probe.py) et son
[reçu](P0_AXIS_UNION_CHECKS.json) passent en normal/−O : 80 petits plans,
11 060 paires jugées par census, 660 valeurs pour les bornes d’intervalles
et 148 rejets supplémentaires permis par l’addition. Trois contre-modèles
produisent un faux rejet : frontière incluse, colonne inexacte, maximum
utilisé pour rejeter une boîte entière. Le maximum des crédits d’axes
reste sûr mais plus faible ; sa contre-fixture mesure cette perte seulement.

Le modèle construit aussi les plages aux tailles 8k/16k/32k, puis compare
leur masse à la formule d’offsets, sans développer les grandes paires.
Il utilise les dimensions 50×80, 80×100 et **100×160**, celles du gate
axial lu ; la dernière diffère de la grille 125×128 du tableau. À 32k :
3 921 460 candidates, 3 632 760 visites de nœuds et 769 506 fragments.
Le coût des requêtes et des fragments est donc réel, avant tout census.
Ce Python recherche dans les colonnes entières, en O(log m) ; les vues
limitées à h voisins et leur O(log(h+1)) restent un raccord proposé.
Aucun temps ni résultat C++ ne sont qualifiés par ces exécutions.
