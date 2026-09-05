# S1 : du propriétaire au représentant d'arité minimale

Cette preuve fournit l'autorité géométrique citée par la [composition constructeur](../docs/PREUVE_HORIZONTALE_COMPOSITION.md#21-ce-que-la-wspd-permet-de-prouver-et-ce-quelle-ne-suffit-pas-à-prouver). Ses [sources épinglées](receipts_20260904/s1_sources.json) et les preuves locales ci-dessous restent distinctes des reçus d'exécution. Elle concerne le catalogue de boules, sans réattribuer la route réduite F à FULL. `public_status=not_claimed`.

On considère une boule B de support minimal strictement positif U, de cardinal q∈{2,3,4}, rayon R et arête maximale AB de longueur D. Positions et PointId sont distincts. Le propriétaire est l'arête maximale départagée par la plus petite EdgeKey ; ce propriétaire l'est aussi dans ses faces incidentes.

## 1. Un seed aigu q4 existe sur chaque arête maximale

Si les deux autres sommets d'un tétraèdre positif appartenaient à la boule diamétrale AB, cette boule contiendrait le support. Sa circumboule étant sa miniball par positivité, son rayon serait ≤D/2, donc exactement D/2 puisque A/B sont à distance D. Son centre serait le milieu d'AB, incompatible avec un centre strictement intérieur au tétraèdre. Un sommet X est donc strictement extérieur à cette boule : ABX est aigu en X.

Il l'est aussi en A/B, puisque AB est maximale et les sommets distincts :

$$2\langle B-A,X-A\rangle=AB^2+AX^2-BX^2\geq AX^2>0.$$

Le plus petit PointId parmi les seeds admissibles fixe un représentant. Lors du rejet canonique `uy<ux`, la lentille et le propriétaire q4 sont déjà vérifiés : un Y extérieur à la boule diamétrale est lui aussi un seed accessible. Le plus petit seed ne peut donc être rejeté par cette règle ; l'ordre Morton ne change pas le départage.

## 2. Le cover contient les sommets et tous les intérieurs

Les poids barycentriques positifs du centre donnent :

$$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j|u_i-u_j|^2\leq\frac{D^2}{2}\left(1-\sum_i\lambda_i^2\right)\leq\frac{q-1}{2q}D^2.$$

Pour m=(A+B)/2, |c−m|²=R²−D²/4 et tout z∈B satisfait |z−m|≤R+√(R²−D²/4). Il en résulte 4|z−m|²≤3D² en q3 et ≤(2+√3)D²<4D² en q4 : les coefficients 3/4 réellement utilisés couvrent toute la boule fermée.

Chaque sommet X vérifie également AX,BX≤D, donc la lentille et |2X−A−B|²=2AX²+2BX²−D²≤3D². La boîte des sommes contient A+B et Dmax² majore D² : le rejet strict des handles ne perd aucun tel site. Les plages conservées sont parcourues entièrement, puis le test d'ancre garde l'égalité ; le tri radial est une permutation. Ce [raccord d'index et de cover](AUDIT_INDEX_20260905.md) donne l'accès effectif au seed et à sa complétion dès que l'ancre survit.

## 3. La complétion q4 est dans le sweep fermé

Pour la face aiguë ABX, noter G son Gram positif, n=(B−A)×(X−A), c0 son centre et R0 son rayon. Tous les centres équidistants sont c=c0+μn/(2G). La puissance q3 est P(z)=G(|z−c0|²−R0²), et l'incidence s'écrit P(z)−μBz=0 avec Bz=n·(z−A).

Une complétion affinement indépendante Y a BY≠0 et racine μ=P(Y)/BY. Son rayon vérifie R²=R0²+μ²/(4G). Le cover de centres q4 donne exactement le test fermé 2P(Y)²≤J BY², où J=D²(3G−2AX²BX²). De R0²≤D²/3 découle J≥GD²/3>0. Y ne peut donc être écarté hors corde. L'identité d·W=GD² prouve `l_exact=4*q3_power` : la division par quatre est exacte.

Au groupe de racines, le sweep compte les témoins constants, les entrées strictement antérieures et les sorties strictement postérieures. Il retire les sorties du groupe avant lecture et ajoute ses entrées après. Les sites du shell ne sont pas comptés. Le cover contient tous les intérieurs et aucune identité dupliquée ; le compte lu est donc le vrai p, sans réaddition des crédits d'amont.

## 4. Les préfiltres q4 préservent les supports positifs

Pour tout sommet Y, la variance donne Σλj|Y−uj|²=2R². Si M2=max(AY²,BY²,XY²), alors 2R²≤(1−λY)M2<M2. Comme R≥D/2, on obtient 2M2>D², premier préfiltre i64.

Supposons ensuite AX²+AY²≤D² et BX²+BY²≤D². Le contrôle de propriété a déjà imposé XY²≤D². Les identités de variance en X/Y donnent :

$$4R^2=\lambda_A(AX^2+AY^2)+\lambda_B(BX^2+BY^2)+(\lambda_X+\lambda_Y)XY^2\leq D^2.$$

Cela imposerait R=D/2 et le centre sur AB, contradiction. Le maximum des deux sommes doit dépasser D², second préfiltre. Enfin, le centre positif est du même côté du plan ABX que Y : sa puissance dans la boule de face, proportionnelle à 2t BY, est strictement positive. Le filtre de puissance de face est donc nécessaire. Les tests de déterminant et de centre confirment ensuite rang et positivité.

## 5. Chaque élimination conserve une boule pertinente

Une mort doit impliquer p(B)≥hq=rmax+1−q pour toutes les boules représentées par la tâche. Une boule pertinente vérifie p<hq. L'[index et les piles](AUDIT_INDEX_20260905.md) partitionnent les paires ; le [front et ses témoins](FRONT_ET_TEMOINS_COURANT.md) justifient fuseaux, coins, boules de cœur et histogrammes. Le cœur exclut A∪B ; les crédits d'extrémités appartiennent à A sans a et B sans b, donc leur addition conserve des identités disjointes.

Les [secteurs et cordes](PREUVE_CHORD_SECTOR_COURANTE.md) fournissent des minorants sur tous les centres admissibles ; leurs comptes susceptibles de recouvrement se combinent par OU. Les [cellules](CELLULES_COURANT.md) couvrent les centres et toute la corde, avec comptes stricts sur cellules fermées et surcouverture du localisateur. Les [marges flottantes](FILTRES_FLOTTANTS_COURANTS.md) incluent les arrondis des bornes finales ; une égalité ne devient pas intérieure. Aucune de ces morts ne peut supprimer une boule pertinente.

## 6. Théorème géométrique conditionnel et RLE

Supposons l'index valide, les opérations entières et comparaisons larges conformes sans débordement, les parcours/tris conformes, le domaine binaire64 nommé par les preuves, et une exécution terminale réussie sans mutant ni cap interrompant le flux. Alors chaque boule minimale B de rang p(B)+q(B)≤rmax possède après RLE un représentant de bonne géométrie et d'arité minimale q(B).

**Preuve.** Choisir un support positif de cardinal minimal et son ancre propriétaire. La partition atteint son rectangle ; toute mort contredirait p<hq. En q2, la paire survivante est émise immédiatement. En q3, le troisième sommet est dans le cover et satisfait le seed propriétaire. En q4, le plus petit seed aigu et sa complétion sont accessibles ; les certificats préservent ce seed, sa racine entre dans le sweep et les préfiltres préservent sa présentation. Le compte strict p<hq autorise l'émission. Les [identités des lanes](ARITHMETIQUE_LANES_COURANTE.md) donnent la vraie boule et son niveau.

Toutes les émissions ont un support positif ; aucune ne peut donc représenter B avec une arité inférieure à q(B). Les formes entières de B sont des multiples rationnels positifs ; leur réduction par PGCD, coefficient quadratique positif, donne la même clé primitive. Le tri par clé puis arité croissante et le dédoublonnage conservant le premier gardent exactement q(B), puisqu'un tel représentant a été émis. Le départage par représentation de niveau ne change pas sa valeur géométrique. Il n'est pas nécessaire d'émettre tous les supports alternatifs.

Les preuves des [primitives entières](../docs/ARITHMETIQUE_PRIMITIVES.md), de l'[arithmétique des témoins](ARITHMETIQUE_SPINDLE_COURANTE.md), des secteurs et cellules déchargent les bornes locales sur le [domaine CPU](DOMAINE_CPU_COURANT.md). Les [reçus compilés](receipts_front_compiled_20260905/README.md) sont des vérifications bornées distinctes. La frontière de fenêtre, jugée jusqu'à 24 points avec refus pertinent et mutants, reste dans son [reçu indépendant](receipts_20260904/math_window_repro.json) et son [snapshot](receipts_20260904/math_window_source_snapshot.json) ; sa composition est documentée chez le constructeur. Aucun nouveau run ni qualification FULL industrielle dans cette condensation.
