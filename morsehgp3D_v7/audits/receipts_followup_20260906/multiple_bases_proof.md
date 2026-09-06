# Base positive unique à chaque pivot strict

6 septembre 2026. Preuve et fixtures rationnelles locales ; aucune compilation, exécution C++, trajectoire native qualifiée ou mesure. `public_status=not_claimed`. GCP non utilisé.

**La fixture demandée avec deux bases positives acceptables est impossible dans le domaine du pivot natif.** Notre ancienne justification de la stabilité par plusieurs bases possibles sur une coquille intermédiaire était trop générale. Une coquille supplémentaire peut exister ; sa base positive reste unique ici.

Soit Q une base positive affinement indépendante, de MEB de centre c et rayon r. Soit z strictement extérieur, et T=Q∪{z}, de MEB de centre b et rayon R. La positivité fait de la boule ancienne la MEB de Q. L'unicité de cette MEB et la violation stricte donnent R>r. Toute base positive dont la boule contient T est une base de cette même MEB ; elle contient z, puisque les supports inclus dans Q ont un rayon au plus r.

Soit S l'ensemble des points de Q sur la coquille de la MEB de T. Cet ensemble est non vide : R>0, donc une base positive ne peut se réduire au seul site z. Tous ces points se trouvent sur les deux sphères. Comme R>r, l'existence d'un point commun impose aussi b≠c. Leur hyperplan radical H vérifie :

$$H:\quad 2(b-c)\cdot x=\lVert b\rVert^{2}-\lVert c\rVert^{2}+r^{2}-R^{2}.$$

Le résidu au centre b est strictement positif :

$$2(b-c)\cdot b-\bigl(\lVert b\rVert^{2}-\lVert c\rVert^{2}+r^{2}-R^{2}\bigr)=\lVert b-c\rVert^{2}+R^{2}-r^{2}>0.$$

Ainsi b n'appartient pas à H. Une base positive acceptable donne b dans l'enveloppe convexe de S∪{z}. Si z appartenait à l'espace affine de S, b appartiendrait à H, contradiction. Or S est affinement indépendant, comme sous-ensemble de Q. Donc **la coquille entière S∪{z} est affinement indépendante**. Ses coordonnées barycentriques de b sont uniques ; les coefficients strictement positifs déterminent exactement l'unique base positive. Les autres points de coquille ont un coefficient nul. La preuve ne dépend ni de la dimension trois, ni de la borne r≥D(T)/2.

Le [juge rationnel](multiple_bases_review.py) vérifie cinq scènes et les 62 permutations de Q déjà présentes dans le corpus des filtres : augmentation stricte, équation radicale, indépendance de la coquille entière et unique base positive contenant z. La scène de coquille supplémentaire conserve ses quatre points actifs et son unique base q2. Ce corpus accompagne la preuve ; son succès fini ne la remplace pas.

## Limite du domaine

Avec Q={(2,9,5),(1,2,5),(9,2,5)} et z=(8,9,5), le centre est (5,5,5) et le rayon au carré vaut 25. Q est positif, mais z est seulement sur la coquille. Les supports (0,1,2) et (1,2,3), indices dans Q suivi de z, sont deux triangles positifs acceptables. Cette fixture réfute l'extension de la conclusion à un violateur non strict ; elle ne constitue pas un pivot admissible.

## Ce que l'ordre contrôle encore

L'ordre historique reste pertinent pour le travail facturé, les préfixes sous cap, les exceptions et les observateurs. Il n'est pas nécessaire à l'identité du support accepté lorsque tous les candidats nécessaires sont essayés dans ce domaine strict.

Le pivot local Q={(0,0,0),(2,2,0),(2,0,2)}, z=(0,2,2) vérifie toutes les préconditions demandées : Q positif, z strict, r²=8/3 et D(T)²=8, donc r²≥D(T)²/4. Sa MEB finale a R²=3 et son unique support q4 comprend les quatre sites. Le calendrier filtré historique essaie les trois q3 contenant z puis ce q4 : quatre formes. L'ordre entièrement inversé essaie le q4 d'abord : une forme. À marge P=1, le premier calendrier épuise son cap tandis que le second accepte. Ces comptes concernent seulement ce pivot local, sans charge d'initialisation ni autre MEB.

Cette fixture fournit un motif causal pour réfuter un mutant d'ordre via **P ou son admission**. Elle ne doit pas être présentée comme une différence de support à budget non limitant, ni comme une trajectoire C++ effectivement observée.

La même entrée, dans l'ordre des quatre sites ci-dessus, admet aussi une trace native **démontrée rationnellement** : toutes les distances au carré valent 8, donc le départage strict retient d'abord la paire (0,1). Le premier violateur est le site 2, de puissance 4 ; le premier pivot propose son unique q3 et donne exactement Q. Le violateur suivant est le site 3, de puissance 8/3. Deux formes ont donc été chargées avant le second pivot : la paire initiale et le premier triangle. La terminaison demande P=6 dans l'ordre historique filtré, contre P=3 avec l'inversion considérée. À P global 3, le second pivot retrouve précisément le différentiel d'admission local précédent. Ce calcul suit les règles natives de départage et de sélection ; il ne remplace pas leur future vérification C++.

Le [reçu normal](multiple_bases_normal.json) et le [reçu optimisé](multiple_bases_optimized.json) portent les mêmes résultats exacts.
