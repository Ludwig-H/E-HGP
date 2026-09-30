# Qκ : pénalité quadratique de cohorte — audit conditionnel

Nouvelle règle, distincte de Pκ ; aucun moteur, statistique, GCP ou natif qualifié.

## Hypothèses nécessaires

Un point est fixé dans chaque filtration X/Y en correspondance. a=α_X≥0,b=α_Y≥0,|a−b|≤ε. Pour r≥a, M_X(r) est la hauteur en RAYON du LCA de la cohorte jusqu'à r (plateaux fermés/atomiques), ou une hauteur normalisée équivalente. Les deux hypothèses utilisées sont :
M_X(r)≤r+a (L6), et M_X(r)≤M_Y(r+ε)+ε ; symétriquement.
Aucun minorant M_X(r)≥r ou M_X(r)≥a n'est nécessaire ici.
L'interleaving géométrique et l'exhaustivité de l'univers restent des obligations distinctes de cette preuve scalaire.

Pour κ entier≥2, Q_X=max(a²,sup_{r≥a}{M_X(r)²−κ(r²−a²)}) et T_X=√Q_X.
Le propriétaire est la première cohorte à a, remontée à T_X après normalisation du plateau fermé. Ne pas choisir le propriétaire d'une cohorte maximisante tardive.

## Borne et cutoff

f(r)=(r+a)²−κ(r²−a²)=−(κ−1)r²+2ar+(κ+1)a².
f'(r)≤0 pour r≥a,κ≥2 ; f(a)=4a². Donc a≤T_X≤2a.
Un candidat ne peut battre a² dès que (κ−1)r²−2ar−κa²≥0.
Sa racine positive est qa, q=(1+√(κ²−κ+1))/(κ−1).
Le seuil est exact en rayons ; les candidats à égalité ne changent pas Q.

On peut appliquer ce cutoff SANS racines : β=r²,A=a² et v=(κ−1)β−κA.
Le rayon est ≤qa exactement lorsque v≤0 ou v²≤4Aβ (r≥a). Ce test rationnel doit être fait avec largeur certifiée ; il ne justifie pas de réutiliser un comparateur huit mots.

Sur une cohorte de M constant, le candidat décroît avec r² ; il suffit d'examiner le début de chaque plateau/cohorte, dont β est un niveau exact. Le maximum inclut toujours A ; aucun arrondi en rayon ne décide la sortie.

## Stabilité améliorée : Cε, non seulement 2Cε

C=(κ+1)(q+1). Si Q_X=a², T_X≤T_Y+ε.
Si ε>a, T_X≤2a<2ε≤Cε≤T_Y+Cε.
Sinon, prendre un candidat gagnant Q_X>a² : m=M_X(r)>a≥ε (le minorant vient du candidat gagnant, pas d'une hypothèse sur toutes les cohortes).
L'interleaving donne Q_Y≥(m−ε)²−κ((r+ε)²−b²), et b≥a−ε.
D'où Q_X−Q_Y≤2(m+κr+κa)ε−ε²≤2Caε−ε².
Or a≤b+ε≤T_Y+ε ; ainsi
Q_X≤T_Y²+2CT_Yε+(2C−1)ε²≤(T_Y+Cε)²,
car C²−(2C−1)=(C−1)²≥0.
Par symétrie |T_X−T_Y|≤Cε. Les suprema non atteints se traitent par approximation ; une tour/cohorte finie atteint le maximum.

Une constante rationnelle conservatrice est Cbar=2κ(κ+1)/(κ−1), car q≤(κ+1)/(κ−1). Pour κ2, C=6+3√3≈11,1962 ; Cbar12. Les nombres approchés sont explicatifs, pas des décisions algorithmiques.

## Compatibilité du propriétaire

La cohorte initiale X est transportée à la cohorte Y au rayon a+ε (≥b).
Q_Y≥M_Y(a+ε)²−κ((a+ε)²−b²).
Comme a≤b+ε et T_Y≥b,
M_Y(a+ε)²≤T_Y²+4κbε+4κε²≤(T_Y+2κε)².
Ainsi M_Y(a+ε)≤T_Y+2κε pour TOUT ε≥0, sans séparation en petits/grands déplacements.
Sous les mêmes maps de composantes de l'interleaving, l'image du propriétaire X à T_X+ε et le propriétaire Y deviennent compatibles au plus à max(T_X+ε,T_Y+2κε), soit un décalage≤(C+2κ)ε depuis T_X ; symétriquement.
Cette conclusion requiert que la première cohorte, son transport et les ascendants soient définis dans des arbres compatibles, sans fabriquer de racine commune pour une forêt partielle.

## Distinction de Pκ

Si Pκ=max(a,sup{m−κ(r−a)}), Qκ est au moins aussi retardante aux mêmes κ.
Pour d=r−a≥0 et p=m−κd>a,
q_cand−p²=κd[2(m−a)−(κ+1)d]≥κ(κ−1)d²≥0.
Les candidats p≤a n'affectent pas Pκ.
Exemple abstrait : a1, première cohorte m1 ; nouvelle cohorte r3/2,m5/2 ; κ2.
Pκ=3/2, Qκ²=15/4. Ce n'est pas une équivalence ni une correction du mutant β de Pκ.
Une règle plus retardante ne garantit ni robustesse statistique ni non-percolation ni ARI/EOM supérieur ; ces propriétés doivent être étudiées séparément.

## Largeurs futures : démonstration de capacité seulement

Avec niveaux génériques N<2^266,D<2^200,D>0 et 2≤κ≤8,
B−κ(C−A) possède une représentation sur D_A D_B D_C<2^600,
avec numérateur signé de magnitude<(2κ+1)2^666<2^671.
Comparer deux de ces dates carrées par produits croisés nécessite des magnitudes<2^1271 : vingt mots64 suffisent (1280 bits).
Comparer à un ancien niveau générique peut nécessiter<2^871 : quatorze mots64.
Ces majorants conservateurs ne constituent aucune primitive native/constructor/tri qualifiés.
Le cutoff rationnel v²≤4Aβ peut lui aussi dépasser huit mots ; vérifier chaque expression/intermédiaire avant implémentation.
Une réduction par PGCD peut aider, mais ne justifie aucune largeur nominale tant que les cas non réduits restent admis.

## Portée complexité

Si l'univers de D incidences est déjà disponible, la cohorte peut être balayée et les candidats maximisés avec comparaisons exactes ; cela évite la comparaison des SOMMES de racines de Pκ.
Les racines restent utiles pour interpréter T, pas pour choisir Q. L'ordre rationalisé peut toutefois être plus large.
Aucune borne sous-quadratique de D, aucun coût de production des cohortes, LCA ni qualité statistique établi ici.
