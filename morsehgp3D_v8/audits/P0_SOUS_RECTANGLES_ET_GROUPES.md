# Réduire le résidu : sous-rectangles et témoins collectifs

13 septembre 2026. Audit indépendant v8, `cpu_reference`,
`quantized_u16_input_only`, `public_status=not_claimed`. Deux apports
constructifs à la [contre-fixture transverse de l’autre auditeur](../../audits/morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md) :
un proposeur q2 utilisant le prédicat C++ actuel, puis une preuve de
certificat collectif qui dépasse les témoins ponctuels W3/W4. Aucun
producteur général ni résultat de tour FULL n’est qualifié ici.

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
à un groupe. Aucun gain global, ni contrat 50k ou massif annoncé.
GCP non utilisé.
