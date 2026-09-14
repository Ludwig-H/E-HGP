# Réduire le résidu : sous-rectangles et témoins collectifs

13 septembre 2026, complété le 14. Audit indépendant v8, `cpu_reference`,
`quantized_u16_input_only`, `public_status=not_claimed`. Proposeurs q2 et
certificats collectifs issus de la
[contre-fixture transverse de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md).
Les sections 6 à 9 étendent ces preuves aux groupes à moments fixes,
aux nappes, à l’addition des colonnes exactes, au raccord du census q2
et à sa collecte suspendable.
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

## 9. Census q2 : des extrema exacts pour partager les recherches

**Raccord proposé après 8e406f9b.** Le prochain census peut réutiliser
le minimum H déjà présent dans
[les prédicats](../src/spindle/predicates.hpp), puis étendre leur maximum
à une boîte B. Cela donne une décision portant sur un produit de requêtes
U×V et un bloc global Z d’IDs, sans supposer de colonnes alignées. Les
boîtes sont continues ; leurs extrema ne sont pas ceux des seuls sites
discrets. Un résultat indécis entraîne un raffinement, pas un rejet.

Noter E(I) les deux bornes de l’intervalle I. Comme H est séparément
affine en a et b, concave en z et séparable par coordonnée :

$$L=\sum_{j=1}^{3}\min_{a_j\in E(U_j),\,b_j\in E(V_j),\,z_j\in E(Z_j)}(z_j-a_j)(b_j-z_j).$$

Pour le maximum, définir t_j=clip(a_j+b_j,2Z.low_j,2Z.high_j) :

$$M_4=\sum_{j=1}^{3}\max_{a_j\in E(U_j),\,b_j\in E(V_j)}\left[(b_j-a_j)^2-(t_j-a_j-b_j)^2\right].$$

L est le minimum exact de H sur les trois boîtes, et M_4 quatre fois
son maximum exact. Pour maximiser, porter successivement a_j puis b_j
à une borne sans diminuer H ; la parabole restante en z_j atteint son
sommet en clip((a_j+b_j)/2,Z_j). Le double de ce sommet reste entier,
même lorsqu’il est demi-entier. Les quatre couples de bornes par axe
suffisent : aucune énumération de tous les triplets de coins 3D n’est
nécessaire. Sur u16, −12·65535²≤4H≤3·65535² ; i64 suffit à ces deux
bornes, sans division ni prédicat flottant.
Les coordonnées doivent être promues en i64 avant les premiers calculs,
comme dans le prédicat actuel ; une multiplication après promotion
implicite de u16 en int peut déjà dépasser i32.

| Certificat | Décision pour toutes les paires du sous-produit |
| --- | --- |
| L>0 | Tous les IDs de Z sont strictement intérieurs : créditer son cardinal, saturé au seuil demandé. |
| M_4<0 | Tous les IDs de Z sont strictement extérieurs : terminer ce bloc. |
| L=0 et M_4=0 | Tous sont sur la frontière : conserver leurs IDs pour la coquille. |
| Sinon | Raffiner Z ou les facteurs du produit de requêtes. |

Pour le **seul compte d’intérieurs stricts**, M_4≤0 permet aussi de
terminer Z. Cette égalité ne permet pas d’oublier les IDs de frontière
si le consommateur demande la coquille. Les bornes n’imposent aucune
limite artificielle à sa taille. Sous le seuil de rejet, les intérieurs
doivent être complets ; au seuil, rendre un statut saturé et non un faux
compte exact. Une première passe strictement comptable peut différer la
collecte des IDs, mais cette collecte et les recherches supplémentaires
doivent alors être payées et déclarées.

**Le prédicat négatif à raccorder est différent de NoCredit.** Dans
l’API actuelle, NoCredit trouve un coin b0 qui réfute un témoin universel
sur V ; il ne prouve pas son absence pour chaque b de V. Par exemple,
a=(1000,1,0), z=(1000,2,0), b0=(60000,1,0) et b1=(60000,3,0) donnent
H=−1 puis H=1. Le nouveau maximum sur V conserve ce bloc indécis.
Autre frontière du calcul : pour a=0, b=2 et z∈[0,2] sur une droite,
les deux bornes z donnent H=0, mais z=1 donne H=1. Le maximum ne se
calcule donc pas seulement aux coins de Z. Ce ne sont pas des défauts
du filtre publié ; ils fixent le contrat de son futur consommateur.

Un compte uniforme sur U×V représente les seuls blocs Z entièrement
consommés. Partager le produit conserve ce compte et le travail encore
ouvert, sans revisiter les blocs crédités. La section 9.1 précise un
format plus simple que la liste de continuations initialement proposée.
Le coût des états et des sorties reste à comparer au parcours par paire.

**Qualification bornée.** Le [juge autonome](p0_q2_census_bounds_probe.py)
et son [reçu](P0_Q2_CENSUS_BOUNDS_CHECKS.json) passent en normal/−O :
1 000 triples d’intervalles, 7 400 évaluations rationnelles indépendantes,
125 000 valeurs sur une grille au quart et cinq fixtures 3D. Un petit
arbre Z est ensuite interrogé sur 36 paires : 36 census complets et 180
avec seuil, dont 31 saturés et 149 complets sous le seuil. Une coquille
de six IDs est conservée. Les quatre contre-fixtures portent sur le
maximum limité aux coins de Z, NoCredit pris pour extérieur global,
l’égalité effaçant la coquille et le cœur recompté.

Cette première série conserve son rôle de juge des bornes et du census
par paire. La série suivante vérifie désormais les continuations U×V.
L’ensemble conserve les IDs dans les nœuds de son petit arbre et utilise
les entiers non bornés de Python : aucun format mémoire produit, port i64
ou gain de temps ne lui est attribué.

### 9.1. Une continuation de census peut tenir dans un seul curseur Z

13 septembre, après f5430f57. Fixer une fois l’ordre de parcours en
profondeur de l’index global Z. Numéroter ses nœuds en préordre et
conserver pour chacun `escape`, le premier nœud après tout son sous-arbre
(ou la fin de l’index). Le premier enfant suit son parent dans ce format.
Le curseur courant désigne alors **tout le suffixe encore ouvert** :
son sous-arbre, puis ceux désignés successivement par les échappements.

| Opération | Continuation exacte |
| --- | --- |
| Bloc intérieur, extérieur ou de coquille décidé | Mettre à jour le compte ou le journal nécessaire, puis passer à `escape[Z]`. |
| Bloc Z indécis, raffinement des témoins | Passer au premier enfant, sans modifier le compte acquis. |
| Produit U×V indécis, partage de U ou V | Les deux produits disjoints héritent du **même curseur Z courant**, compte et têtes de journaux immuables. |
| Repli vers les requêtes individuelles | Chaque paire reprend depuis cet état hérité ; aucun redémarrage à la racine ni crédit ajouté deux fois. |

La preuve est une induction sur le préfixe consommé de l’ordre des
feuilles : consommer un sous-arbre ajoute un intervalle contigu à ce
préfixe ; le partager ne change aucun ID ; partager les requêtes conserve
ce même préfixe pour deux ensembles de paires disjoints. Le compte acquis
est exact et uniforme sur le produit tant qu’il est sous Kmax. Les IDs
intérieurs et de coquille consommés sont référencés par des journaux
persistants ; seuls leurs nouveaux suffixes diffèrent entre enfants.
Le cœur n’est pas préchargé : tous les sites sont visitables depuis zéro.

Cette simplification exige le **même ordre Z fixe**. Un ordonnancement
libre des tâches U×V reste possible puisque leurs curseurs sont privés ;
réordonner arbitrairement les blocs Z ouverts à l’intérieur d’une tâche
demanderait une autre représentation de sa frontière. Les références
de propriétaire, de facteurs et de journaux doivent accompagner le
curseur. Les mises à jour de comptes restent locales aux tâches ; une
réduction de compteurs peut se faire séparément.

Un tableau d’échappements ajoute O(n) indices à un arbre existant.
Il ne copie aucune liste de témoins à chaque partage. Dans un arbre
binaire plein en préordre avec une feuille par site, un sous-arbre de
m sites comporte exactement 2m−1 nœuds : son échappement est aussi
calculable par `node_id + 2*m - 1`, avec arithmétique contrôlée par la
taille de l’index. Le C++ q2 relu pendant la passe après f47559b1 possède
ce format. Conserver son champ explicite et ses contrôles est néanmoins
un choix valable ; supprimer ce champ est une option de représentation,
pas une condition préalable aux mesures. Le juge conserve les indices
explicites et vérifie déjà cette identité de taille de sous-arbre.
Pour un index portant des plages de feuilles, vérifier `escape[Z].first=Z.last`, ou
fin de l’index lorsque `Z.last=n`, ainsi que la partition par les enfants.
Le juge vérifie la taille de chaque sous-arbre préordonné ; ses mutants
sautant un frère ou revenant sur un préfixe consommé sont rejetés avant
de lancer une boucle de census.

**Limiter les tâches sans limiter la recherche.** Le juge compare un
nombre de divisions de produits par branche illimité, nul ou limité à
deux. Quand cette limite est atteinte, il énumère le produit encore ouvert
et poursuit chaque paire depuis son curseur hérité. C’est un choix de
grain qui conserve la couverture, pas un quota de candidats. En exécution
parallèle, une file pleine peut aussi conduire à exécuter un enfant
localement en profondeur. Ni option ne supprime le travail résiduel.

**La coquille ne tient pas dans Kmax.** Trente sites u16 sur une même
sphère de rayon 5 donnent une profondeur nulle et une coquille de
30 IDs, même à Kmax=1. Les journaux doivent être libérés lorsqu’ils ne
sont plus référencés, ou diffusés par blocs ; un arena qui ne recycle
jamais ses cellules transforme un faible état local en forte mémoire
cumulée. Différer puis rejouer la collecte des coquilles est une autre
option complète, dont le coût doit être payé séparément, sans modifier
le compte strict déjà obtenu. Le juge actuel conserve les journaux et
toutes ses sorties ; il ne qualifie pas un budget de résidence.

Une terminale ayant une coquille **complète et uniforme** sur U×V isole
nécessairement une paire, pour des sites distincts et des facteurs disjoints.
Sinon, pour a≠a′ dans U et b dans V, chaque extrémité serait sur les
deux coquilles, alors que $H(a';a,b)+H(a;a',b)=-\Vert a-a'\Vert^2<0$.
L’argument est identique pour V. Les préfixes de recherche et les rejets
saturés se partagent donc ; les paires retenues demandent encore leurs
sorties propres dans cette représentation. Cela n’interdit pas de
canoniser ensuite plusieurs diamètres d’une même boule par
$(a+b,\Vert a-b\Vert^2)$, avec le même propriétaire. Cette canonisation
et ses incidences ne sont pas implémentées par le juge.

**Portée du contrôle conjoint.** Normal/−O donnent les mêmes résultats :
neuf fixtures, 108 exécutions (Kmax=1/2/5/10, trois budgets de division),
8 616 vérifications de paires et 249 588 évaluations rationnelles ponctuelles.
Les cinq nouveaux mutants portent sur la reprise à la racine après crédit,
le bloc indécis oublié, la coquille héritée oubliée et les deux échappements
invalides. Ils s’ajoutent aux quatre contre-fixtures des bornes.

| Fixture du juge, sans budget de division | Kmax | Visites par paire | Visites conjointes |
| --- | --- | --- | --- |
| Deux ancres et trois autres sites, crédit/coquille hérités | 10 | 18 | 14 |
| Deux nappes 4×4 et trois sites supplémentaires | 10 | 10 452 | 9 419 |
| Coquille sphérique à 30 sites | 1 | 153 | 147 |

Le repli immédiat paie aussi son test initial : 154 visites sur la dernière
fixture, contre 153 pour les requêtes individuelles directes. Ce surcoût
observé concerne le repli ; aucun adversaire général du parcours sans
budget n’est revendiqué. Le reçu conserve toutes les lignes,
les compteurs de visites, divisions, copies de curseurs, allocations de
journaux et profondeur d’appels, ainsi que les contre-fixtures de
transmission. Les décisions sont confrontées au calcul indépendant par
centre/rayon rationnels sur toutes les paires des petits produits.
Les deux parcours comparés utilisent ici le même arbre et les mêmes
bornes ; une visite d’un produit coûte davantage qu’un simple test
ponctuel. Leur décompte ne devient ni un temps, ni une comparaison avec
les 295,5 millions de visites du prototype C++ de l’autre auditeur.
Le modèle coupe aux médianes de cardinalité, sans hériter de la preuve
de profondeur 48 propre au découpage spatial u16 de ce prototype.

La sélection du facteur à partager reste une heuristique par étendue.
Le coût des divisions et des journaux peut annuler le partage des tests ;
aucune borne globale sous-quadratique ni clôture P0 n’en découle. La
[comparaison C++ publiée à f4815cd4](../receipts/q2_census_20260913/README.md)
paie les mêmes résidus, les index et les sorties. Ses conclusions de
coût restent distinctes de ce modèle et précèdent le choix parallèle.

### 9.2. Raccorder Pool seul sans reconstruire le filtre axial

13 septembre, après 2e75b2f3. Les campagnes q2 comparent désormais
trois préfiltres axiaux ; leur contrat précise que **Pool seul** n'est
pas encore consommé. Le `CreditPlan` existant fournit pourtant déjà
le résidu nécessaire, sans devoir produire un `AxisQ2Plan` artificiel.
Cette adaptation reste proposée, sans mesure ni modification du moteur.

Poser h=seuil−crédit_cœur. Dans `CreditPlan::group_residual`, les
permutations A et B regroupent les crédits par valeur croissante, y
compris la classe saturée h à la fin. Pour une ancre a de crédit c_a<h :

$$\{b:c_a+c_b<h\}=\mathrm{b\_order}[0:N(c_a)],\qquad N(c)=\sum_{j=0}^{h-c-1}|B_j|.$$

Les classes vides ne créent aucun trou. Les classes admises sont
consécutives depuis zéro : **une seule plage B par ancre suffit**, avec
les IDs originaux de l'ancre et de la permutation B. Une ancre saturée,
un préfixe vide ou h=0 n'émet rien. Les produits sont disjoints par ancre
et leur union égale exactement le résidu du plan, sans développer A×B.

On peut même éviter un nouveau balayage B. Initialiser h bornes N à zéro ;
pour chaque `CandidateBlock` du plan, retrouver son crédit A par
`a_order[block.a.first]`, puis porter N[c] au maximum des `block.b.last`
de cette classe. Les blocs groupés existants prouvent que ce maximum
est précisément l'extrémité du préfixe pour chaque classe A présente ;
les autres valeurs ne sont pas utilisées. Parcourir ensuite les ancres une
fois. Depuis le plan **déjà construit**, le travail supplémentaire est
O(|A|+D_credit), avec D_credit≤h(h+1)/2 ; l'état auxiliaire est O(h) en
diffusion, ou O(h+|A|) en matérialisant les descripteurs. Le tri et les
crédits du plan initial restent payés. Pour le parcours partagé, les au
plus h préfixes distincts peuvent aussi être couverts une fois dans l'arbre
B, puis leurs racines réutilisées pour les ancres de même crédit ; cette
option paie O(h(1+log |B|)) en préparation et stockage sur l'arbre B
équilibré actuel, plus toutes les tâches effectivement lancées.

Un raccord propre est une surcharge q2 ou une vue résiduelle contrôlée :
même propriétaire que l'index, voie q2, contrôles avant le retour vide.
Le compte repart de zéro sur tous les sites. Emprunter le `CreditPlan`
interdit son affectation, déplacement ou destruction pendant l'appel et
ses callbacks ; garder seulement son propriétaire ne protège pas ses
permutations. Une copie possédée paie au contraire ses tableaux.
L'ordre par crédit peut donner des boîtes B moins serrées que l'ordre
axial : ni baisse des visites ni gain de temps n'est présumé.
Le test du raccord devra comparer les incidences canoniques à l'expansion
native du plan, avec classes vides, saturation, cœur et refus de mauvais
propriétaire/voie. L'ordre d'émission n'est pas une identité géométrique.

### 9.3. Reprendre la collecte avec un budget de travail et de sortie

14 septembre, après 1c523fbe. Une coquille complète peut dépasser Kmax,
même à Kmax=1. Pour préparer des tâches réparties entre de nombreux
travailleurs, borner le seul nombre de visites Z ne suffit donc pas :
l'émission des IDs d'un bloc déjà classé doit aussi pouvoir s'interrompre.
Le [modèle indépendant](p0_q2_collection_probe.py) rend ce protocole
exécutable en mono ; il ne modifie pas l'API de census actuelle.

Considérer un support fixé (a,b), admis après un comptage exact p<Kmax,
et son index Z immuable. Comme dans le census publié, cette collecte
repart à la racine avec ses propres comptes à zéro : p est une valeur
à vérifier au terme, jamais un crédit ajouté aux IDs collectés. Outre
les références au contexte et au support, l'état mutable contient seulement
le curseur DFS, un bloc classé en attente avec offset et type
intérieur/coquille, les deux comptes d'IDs acceptés et un numéro de fragment.
Le bloc nomme une plage de l'ordre immuable ; sa suspension ne copie
ni la plage entière ni le chemin de l'arbre.

L'invariant est le suivant : avant le curseur, tous les sous-arbres
écartés sont extérieurs ou entièrement émis, sauf l'éventuel suffixe
du bloc en attente. Ce suffixe doit être vidé avant toute nouvelle visite Z.

1. Sans bloc en attente, classer le nœud au curseur. S'il est indécis,
   avancer vers son premier enfant ; sinon avancer à son échappement et,
   pour un intérieur ou une coquille, installer sa plage comme bloc en attente.
2. Depuis ce bloc, proposer au plus B IDs et au plus le budget restant.
   Avancer l'offset, les comptes et la séquence seulement après acceptation.
   Un refus rend immédiatement la main et conserve ces valeurs ; le
   curseur déjà avancé reste associé au même suffixe non émis.
3. Émettre le marqueur de fin seulement si le curseur est au-delà de
   l'arbre **et** qu'aucun bloc n'attend, avec compte intérieur égal à p.
   Un refus du marqueur permet de le proposer de nouveau ; il ne termine
   pas la collecte.

La preuve se fait par induction sur ces transitions : la partition DFS
des IDs évite les pertes entre nœuds, puis les offsets évitent pertes et
doublons dans un bloc. Le saut d'échappement ne suffit pas à certifier la
fin : il peut déjà viser la sentinelle alors que le dernier bloc attend
encore ses émissions. Chaque boîte est classée une seule fois ; aucune
reprise ne reconstruit le préfixe consommé.

Un fragment porte l'identité du contexte immuable, du support et de la
tentative, sa séquence, son type et ses IDs. Les fragments restent
provisoires jusqu'au marqueur
de fin : leur réception isolée ne certifie pas une coquille complète.
L'acceptation doit signifier un transfert de propriété indivisible ;
un refus signifie qu'aucun ID n'a été consommé. Le modèle suppose une
offre non bloquante, sans exception après consommation ni acquittement
ambigu. Une file asynchrone devra fournir ce contrat, borner ses segments
en vol et conserver le contexte ; la survie aux pannes et la déduplication
entre tentatives demanderaient un protocole supplémentaire. Le juge teste
une seule tentative, d'identifiant constant, dans un seul contexte par support.

Pour un quantum q>0 et une capacité B>0, chaque appel paie au plus q
unités **visites Z + IDs proposés**, y compris les copies d'un fragment
finalement refusé. Le contrôle et l'offre du marqueur ajoutent un coût
constant par appel ; le travail interne du consommateur n'est pas borné
par q. Le producteur demande un état de continuation O(1) et O(B) de
mémoire transitoire, hors index partagé, files et sorties aval. Le modèle
stocke les IDs des nœuds explicitement et le juge accumule le résultat :
ce n'est pas une mesure de résidence de l'index ou du processus Python.

Le flux d'IDs acceptés et le nombre de classifications sont identiques
au parcours sans budgets. Le nombre de fragments, les offres refusées,
leurs copies et les reprises ajoutent du travail payé séparément.
La terminaison suppose que le consommateur finisse par accepter ; aucun
budget fini ne garantit de terminer face à des refus permanents.
Le nombre de collectes simultanées reste à borner par l'ordonnanceur.

Le [reçu distinct](P0_Q2_COLLECTION_CHECKS.json) épingle le nouveau modèle
et sa dépendance géométrique, sans modifier le reçu des bornes de §9.1.
Normal/−O donnent les mêmes résultats : quatre fixtures, 192 exécutions
avec budgets, 3 297 appels, 288 refus, 2 532 IDs proposés et 2 304 acceptés.
Les quanta 1/2/5/17 et capacités 1/2/7 sont croisés avec quatre politiques
d'acceptation, dont un refus du marqueur final. Les IDs sont comparés au
calcul rationnel par centre/rayon ; visites et flux typé sont comparés à
la collecte sans budgets. Une coquille de 30 sites sous Kmax=1 exerce la
sortie longue ; un bloc intérieur uniforme de trois sites sous Kmax=10
exerce la reprise au milieu d'une plage. Cinq mutants de continuation ou
de fin et trois budgets invalides sont rejetés, y compris sous Python −O.

Ce protocole fournit une transition bornée à transposer et tester dans
le moteur avant les files parallèles. Il ne réduit pas le nombre de
visites ni le volume des coquilles, et ne qualifie ni le parallélisme,
ni le GPU, ni la tour FULL ou les contrats de temps sur G4.

### 9.4. Partager les arbres B sans transférer leur borne de couverture

14 septembre, après 1bf806f0. Le raccord massif peut partager l'index Z
entre rectangles. Pour l'arbre de requêtes B, la borne de §9.2 dépend
aussi de l'ordre : `build_queries` à 3c29ea1e construit un arbre équilibré
sur **la permutation du plan**, sans la réordonner. Il paie m=|B| lectures
de points et 2m−1 nœuds, une fois par appel Shared non vide. Un préfixe
dans cet ordre se couvre en O(1+log m). Cette borne ne se transfère pas
à un arbre géométrique global dont les feuilles suivent un autre ordre.

**Contre-fixture certifiée.** Prendre m puissance de deux, un arbre B
équilibré de feuilles b0,b1,…,b(m−1), et l'ordre du plan
b0,b2,…,b(m−2),b1,b3,…,b(m−1). Son préfixe de longueur m/2 contient
les seuls labels pairs. Chaque paire de feuilles sœurs mélange un site
retenu et un site exclu : aucun nœud interne n'est entièrement retenu.
La couverture exacte demande donc **m/2 racines singleton**, contre une
seule racine dans l'arbre de l'ordre du plan.

Ce motif peut porter des crédits sûrs, pas seulement une permutation
abstraite. L'ancre a a pour coordonnées (0,0,0) ; les labels locaux j
de B désignent les points b_j=(L+j,0,0), avec L=12(m−1)+1.
Prendre Kmax=1, cœur nul, crédit de a nul, crédits B pairs nuls et
impairs égaux à un. Pour j impair, b_(j−1) est un témoin strict entre
a et b_j. La séparation v8 passe pour s=8/10/12. Les crédits nuls sont
volontairement conservateurs ; ce n'est pas une sortie annoncée de
Pool, DualBlocks ou Tubes. Le contrat de sûreté des crédits, à lui seul,
n'interdit donc pas cette fragmentation. Le juge vérifie m=4 à 256
sous u16 ; le motif combinatoire vaut pour toute puissance de deux.

Deux raccords restent exacts et comparables :

- **Conserver l'ordre du plan.** Préparer son arbre B une fois dans un
  objet immuable possédant permutation et boîtes, partagé par les tâches
  de ce plan. Pour Pool seul, compiler les au plus h préfixes distincts
  une fois garde O(h(1+log m)) pour leurs couvertures. Le réemploi exige
  le même nuage, la même permutation et la même construction d'arbre ;
  l'identité du seul ensemble B ne suffit pas. O(m) par ordre distinct
  et la résidence de ces arbres restent payés. Limiter les contextes
  actifs évite un cache qui conserve indéfiniment tous les ordres.
- **Conserver l'arbre géométrique.** Préparer les minima/maxima des crédits
  aux nœuds du facteur B, en O(m). Pour t=h−c_a, minimum≥t élimine un
  nœud, maximum<t le retient entièrement ; sinon il faut descendre.
  Ces agrégats appartiennent au contexte des crédits, qui dépend aussi
  de A. Partager la couverture entre ancres de même crédit paie chaque
  seuil une fois, mais son nombre F_t de racines peut atteindre m/2.
  Le partage des boîtes géométriques ne prouve pas F_t=O(log m).

Une limite C sur les racines préparées fournit un repli simple : compiler
la couverture sans lancer de census ; si une racine supplémentaire
dépasserait C, abandonner toute cette préparation provisoire et consommer
le descripteur original une fois par paires. Sinon engager sa couverture.
Cela conserve exactement le résidu et évite de payer deux fois son
préfixe. C borne le stockage provisoire des racines, pas la préparation
d'appartenance, les visites ou le census aval. Les cas vide, singleton
ou facteur B entièrement retenu permettent leurs chemins directs ;
aucun arbre supplémentaire n'est nécessaire pour nommer un nœud B déjà
certifié. Le choix entre ces chemins doit être mesuré sur les vrais facteurs.

Le [modèle indépendant](p0_factor_order_probe.py) prépare explicitement
des comptes d'appartenance en O(m), puis contrôle les couvertures par
identités et multiplicités. Son [reçu](P0_FACTOR_ORDER_CHECKS.json) conserve
les commandes normal/−O et le hash du script : 5 912 permutations/préfixes,
23 640 essais de budgets, 9 511 replis et 14 129 couvertures engagées.
Deux mutants confondant les rangs ou retenant un nœud trop large sont
rejetés. Sur le motif alterné, la couverture globale visite 2m−1 nœuds,
contre trois dans l'ordre du plan. Les émissions pendant la préparation
sont absentes par construction de ce modèle, pas testées sur une API
produit. Aucun gain de temps, de mémoire globale ou de tour n'est qualifié.
