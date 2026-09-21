# q3 : préparer une enveloppe de centres par bloc de seeds

Audit mathématique du 21 septembre 2026, réponse à la question constructeur
dans `audits/COORDINATION_MORSEHGP3D_V8.md`. Proposition indépendante,
**sans port ni mesure de gain**. Elle concerne le census q3, pas le filtre
de témoins citron ni la qualification de la tranche 33.

La formule proposée est correcte. Une alternative évite de réévaluer un
intervalle cubique en x pour chaque couple X×Z : préparer une boîte de
centres conditionnelle par X, puis borner six paraboles en z par nœud Z.
La préparation demande seulement les boîtes et l'arête fixe, sans scan
des seeds. Une fixture ci-dessous prouve un gain de précision, pas de temps.

## Formule et domaine exacts

On conserve les notations du constructeur : m=(a+b)/2, d=b−a, D=|d|²>0,
w=2x−a−b, v=2z−a−b, t=d·w, J=D|w|²−t², P=Dw−td,
Q=|w|²−D et qz=|v|²−D.

$$ c=m+\frac{QP}{4J},\qquad \mathrm{pow}_x(z)=\frac{Jq_z-Q(P\cdot v)}{4J}. $$

En effet P est orthogonal à d, P·w=J et |P|²=DJ ; la puissance de x
et des deux endpoints est nulle. **J>0 signifie non-colinéarité**, pas
positivité du support. Les trois angles sont strictement aigus exactement
si Q>0 et |t|<D. L'arête ab est de longueur maximale exactement si
Q+2|t|≤2D. Les égalités de longueur restent possibles ; le départage par
IDs est une restriction supplémentaire qui ne fragilise pas les bornes.

Poser λ=DQ/J. Pour les seules seeds aiguës possédées par cette arête,
0<Q≤2D et |t|≤D−Q/2. Ainsi :

$$ J=D^2+DQ-t^2\ge 2DQ-Q^2/4\ge 3DQ/2,\qquad 0<\lambda\le 2/3,\qquad B(x)=12D(c-m)=3\lambda P. $$

La borne 2/3 est atteinte par le triangle équilatéral entier
a=(30,30,30), b=(36,36,30), x=(36,30,36). Sans acuité, λ peut être
négatif ; sans propriété de l'arête, il peut dépasser 2/3. Ces deux
hypothèses ne doivent pas être supprimées.

## Deux préparations comparées

Les trois coordonnées de P sont affines en x : leur intervalle exact sur
X se prépare avec la matrice D·I−ddᵀ appliquée à w. On prépare aussi les
intervalles de Q, t et J=|d×w|² ; pour J, sommer les intervalles carrés des
trois composantes affines donne une enveloppe sûre.

- **Universelle** : λ∈[0,2/3], puis produit d'intervalles 3λP_i.
- **Resserrée** : si J_min>0, intersecter l'intervalle précédent avec DQ/J.
  Conserver aussi la relation J=DQ+R, R=D²−t² : λ est croissant en Q
  et décroissant en R. Si J_min=0, reprendre la préparation universelle.

Les entrées u16 étant entières, Q>0 et |t|<D autorisent Q_min≥1,
Q_max≤2D et t∈[−D+1,D−1] pour cette sous-population conditionnelle.
Après ces intersections, R_min>0 et les bornes supplémentaires sont :

$$ \frac{DQ_{\min}}{DQ_{\min}+R_{\max}}\le\lambda\le\frac{DQ_{\max}}{DQ_{\max}+R_{\min}}. $$

Ce maintien de la relation J=DQ+R est plus précis que le quotient
d'intervalles naïf. Une intersection vide prouve l'absence de seed valide.
Les coordonnées finales de B sont arrondies vers l'extérieur, par floor
et ceil rationnels exacts. Les six bornes entières sont immuables et
réutilisables pour tous les Z traités avec ce X.

**La boîte préparée n'enveloppe que les centres des seeds valides.**
Il n'est pas nécessaire que tous les points de X soient aigus ou de bonne
propriété ; les autres seront ignorés par le générateur. En revanche,
aucun de ces points ne doit être retiré de la population globale Z des
témoins : une seed invalide peut être un témoin intérieur d'une autre.

## Requête Z, comptes et contacts

Pour s_i=a_i+b_i, la puissance mise à l'échelle s'écrit :

$$ 12D\mathrm{pow}_x(z)=\sum_{i=1}^3\bigl(3D(2z_i-s_i)^2-B_i(2z_i-s_i)\bigr)-3D^2. $$

Pour chaque axe, prendre les deux endpoints de l'intervalle B_i.
Les extrema sur l'intervalle entier z_i se trouvent aux endpoints pour
le maximum ; au floor/ceil du sommet, bornés à l'intervalle, pour le
minimum. Le sommet est s_i/2+B_i/(12D). Cela donne des extrema exacts de
l'enveloppe séparable B×Z, avec six paraboles, sans former B_i².
L'arrondi floor/ceil du sommet exploite **le profil u16** ; ce n'est pas
un minimum sur un nuage de coordonnées réelles quelconques.

Un majorant strictement négatif crédite tous les sites du bloc Z ; une
borne inférieure ≥0 les exclut du **compte strict de ce bloc q3 seulement**.
Le seuil de rejet est K−1. Une exclusion n'apporte pas de crédit et ne
rejette pas la seed. Une puissance nulle est un contact à conserver pour
la coquille d'une boule finalement acceptée.

Deux objets restent séparés : la préparation immuable de centres et
l'état exclusif `(compte, curseur Z, ordre original)` de la tâche. Un
préfixe ne s'hérite qu'après classification exhaustive pour toutes les
seeds descendantes. Devant une feuille Z ambiguë, scinder X **avant** de
la consommer. Au relais singleton, transmettre ensemble compte et curseur,
ou recommencer le census à zéro. Ne jamais ajouter un crédit au census
complet recommencé ; ne transmettre aucun de ces comptes à q4.

Ne pas exclure tout X comme s'il était la seed unique : ses autres sites
peuvent être intérieurs. Pour une boule acceptée, la collecte complète
de coquille repart sur l'index global ; les exclusions ≥0 du compte ne
constituent pas un payload de coquille.

## Fixture de précision et gate

a=(20,20,20), b=(40,20,20), X={(30,y,20) : 32≤y≤37},
z=(30,31,20). Toutes ces seeds sont aiguës et ab est leur arête maximale.

| Enveloppe | Intervalle obtenu | Décision commune |
|---|---:|---|
| Intervalle direct de F | [−206 841 600 ; 1 670 400] | Indécise |
| Centres universels, échelle 12D | [−497 600 ; 100 800] | Indécise |
| Centres resserrés, échelle 12D | [−486 226 ; −92 800] | Strictement intérieur |

Les deux échelles diffèrent ; seul le signe de leur borne importe ici.
La boîte resserrée donne B_y∈[8800,26683]. La rotation entière de matrice
((2,−2,1),(1,2,2),(−2,−1,2)), suivie d'une translation, exerce aussi des
arêtes non axiales ; elle multiplie toutes les distances par trois.

[math_checks.py](math_checks.py) résout les centres indépendamment par
trois équations cartésiennes en `Fraction`. Les 14 cas couvrent les
rotations, les bornes u16, les contacts, les seeds invalides et le repli
J_min=0 : 16 164 identités de puissance, 15 148 inclusions dans les bornes
et 260 contrôles de centre, ces derniers incluant les deux préparations.
Une fixture K3 a profondeur1 et trois sites de coquille : son crédit de
préfixe vaut1 ; le relais correct reste1, le redémarrage avec crédit donne
faussement2. Les sorties normal/−O et le préflight du harnais sont
conservés dans [CHECKS.json](CHECKS.json).

## i128 et coût restant à mesurer

Pour M=65535, |P_i|≤12M³, |Q|,|qz|≤12M² et J≤36M⁴ donnent
|F|≤1296M⁶<2¹⁰⁷. Dans la préparation resserrée, les numérateurs de λ
sont ≤18M⁴ ; les produits 3P_i·numérateur sont ≤648M⁷<2¹²².
**Ne pas comparer deux fractions par produits croisés** : ceux-ci peuvent
atteindre l'ordre M⁸, au-delà d'i128. Le modèle utilise des quotients/restes
successifs, sans grossissement des opérandes ; les extrêmes de 3λP_i sont
choisis directement selon le signe de P_i. Promouvoir avant multiplication.
Les dénominateurs sont positifs ; les
divisions signées doivent arrondir extérieurement. B_i tient en i64
(|B_i|≤24M³+1), mais les évaluations de puissance utilisent i128.

Ces calculs bornés ne donnent aucune borne globale de travail. La sonde
ne mesure ni LiDAR ni temps CPU : le coût des préparations X, des visites
X×Z, des descendants ambigus, des relais, du census et des coquilles reste
à payer. La boîte de centres peut être trop large ou se préparer plus
cher que l'intervalle direct. Préparer une fois par X, réutiliser l'index
global et garder les petites seeds sur le chemin scalaire sont des
conditions du futur comparatif, pas des gains déjà acquis.
