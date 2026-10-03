# Réponse indépendante aux questions Q6 et Q7 — 3 octobre 2026

Q6, formulée avec fidélité, équivariance et stabilité lipschitzienne à cardinalité constante, admet un contre-exemple constructif à l'impossibilité proposée. Q7/S est correcte pour ER0h ; son argument en niveaux carrés peut être complété par une famille géométrique en rayon, à paramètres fixes.

Il s'agit d'une vérification mathématique bornée, sans moteur exécuté, compilation, fit ni GCP. Aucun produit ni audit actif n'est modifié. Les sources du workflow sont figées par empreintes ; la question développeur porte SHA-256 `9a1003e1b768934c7abd782e2020c4572dfe7b1178d575a2e827429e210bfbbf`. Le HEAD développeur à la capture est `8df2025ab0074403f9dd94a303d5f367424b8561`.

## Q6 : une construction, sans recommandation de modèle

Sur des sites distincts, de poids un, comparés par un appariement conservant N et les identifiants, définir une seule règle :

* k=1 : liaison simple, dans l'unité rayon = demi-distance.
* k≥2, N=6 : P₁ sur le profil couvrant qualifié Π₃.
* k≥2, N=4 : P₂ sur le profil couvrant brut Π₁.
* Autres N, k≥2 : P₁ sur Π₁.
* k>N : tous les sites inactifs.

Pκ choisit un premier point p du profil, à hauteur t, puis e=max(t, sup_q[m(p,q)−κ(h(q)−t)]) ; le propriétaire est Up_e(p). Les premiers points ex æquo donnent le même point transporté : le sup impose e au moins égal à leurs rencontres. Cette convention ne départage donc pas des composantes symétriques par leurs indices.

Chaque bras est une pendaison dans le profil couvrant : fidélité et laminarité. Le choix du bras n'utilise que N, donc renumérotations, isométries et homothéties le conservent. Sous l'entrelacement transportant les profils qualifiés, la borne de Pκ est (1+2κ)ε sur les dates **et** hauteurs de réunion, y compris les plateaux fermés. N étant constant dans cette distance, les comparaisons ne changent jamais de bras : la constante uniforme de la règle entière est donc max(3,5)=5, indépendante de N, de k et de l'échelle. La preuve de transport Π_m est une hypothèse géométrique explicite de cette conclusion ; le script ne remplace pas cette preuve par des essais.

Vérification exacte par `Definition` figée, Fraction et un comparateur de radicaux indépendant :

| Cible | N | Dates / résultat |
| --- | ---: | --- |
| T0 équilatéral entier | 6 | Toutes les entrées à √(2/3), ABC\|DEF à r=1, avant la racine √(3/2). |
| Q1bis, pont 1700 | 6 | Entrées à √(249978000484/187489) ≈1154,6836 ; ABC\|DEF à 1155, 1400, 1600, 1787, avant la racine √3194656. |
| Q2/S17 | 4 | eₓ=100−(3/5)√(90625/16)≈54,84402033, eₐ=50, e_b₁=e_b₂=√257/2 ; {x,a}\|{b₁,b₂} sur [61,75]. |

Pour Q2, la base et les quatre variantes primaires sont vérifiées sur leurs intervalles entiers déclarés : toutes les activations sont antérieures à la borne inférieure et toutes les coupes FULL intermédiaires sont relues. Pour les trois cibles, les dates et partitions sont également vérifiées après inversion des indices, homothétie 2 et translation commune.

Provenance des cibles : `primary_sources/QUESTIONS_UTILISATEUR.md` contient les quatre coordonnées de Q2 ; `REPONSES_UTILISATEUR_20261001.md:7` impose les triangles aussi à mcs=2. L'ancien catalogue v2 conserve une cellule **dérivée antérieure** AB\|CD\|EF à mcs=2 pour Q1 ; elle est supersédée par cette réponse datée. Nous ne revendiquons pas réussite sur ce catalogue entier. Son seul extrait Q2, sans ambiguïté, est conservé avec le hash du parent de 13,48 Mo.

Cette construction est volontairement ad hoc. Elle n'est ni une proposition de modèle naturel, ni une preuve d'optimalité, ni stable sous insertion/suppression ou changement de poids. Elle ne vérifie pas A5_glob sur le profil **brut** : Π₃ peut retarder une unique remontée brute jusqu'à sa qualification. Q6 ne demande pas cet axiome. Si l'on ajoute A5_glob brut, une localité stricte au profil, une compatibilité à l'ajout de bruit, ou une distance autorisant N à changer, la construction ne clôt plus la question ainsi renforcée.

En particulier, le théorème E exclut Loc+A4^prof+Mon sur le profil brut ; le théorème F exclut la famille première-lignée qualifiée à seuil constant. Aucun des deux n'exclut toutes les règles des trois axiomes de Q6.

## Q7/S : preuve en niveaux carrés

Fixer η=1, 0<κ≤31 et ρ≥2 entier. Pour X={0,(L,h,0),(L,−h,0)}, Y remplaçant le dernier z par 1, L=8ρ², h=8ρ :

o=16ρ²(ρ²+1), b=16(ρ²+1)², s=b−o=16(ρ²+1).

Le triangle est aigu ; ses trois paires sont des naissances FULL₂, puis elles fusionnent à sa MEB. Héron donne b'=b+a avec

a=(ρ²+1)(192ρ²−63) / [4(256ρ⁴+ρ²+1)], 0<a≤1/4.

Les naissances des deux branches couvrant x sont o,o dans X, o,o+1/4 dans Y. Les naissances de la troisième paire sont h², h²+1/4. Les premiers instants de toutes les couvertures appariées changent au plus de 1/4 ; δ_β=1/4 exactement. Les applications « même branche, puis ancêtre vivant après +δ_β » donnent un entrelacement du FULL décoré en niveaux carrés ; δ_β n'est **pas** le déplacement géométrique en rayon.

Dans X, l'héritage proportionnel du vote parent répartit tout W par moitiés : aucune majorité stricte avant b. Dans Y, les deux masses héritées sont proportionnelles à s₁=s+a, s₂=s+a−1/4. La marge est

μ=(1/4)/(2s+2a−1/4)=1/(8s+8a−1).

La date en rayon est t_Y=max(√o, √b'−κμ√o), contre t_X=√b. On a μ≤1/[124(ρ²+1)], donc κμ√o≤κ/31≤1, tandis que √b'−√o>2 ; le cône est actif. De plus μ≥1/[129(ρ²+1)]. Par conséquent

t_X²−t_Y² = −a + κμ√o(2√b'−κμ√o)
≥ −1/4 + κμo
≥ 16κρ²/129−1/4.

Le minorant de S, |Δu(x,x)|/δ_β≥64κρ²/129−1, est donc correct si u(x,x) désigne la date d'entrée **en niveau carré**. Les mêmes états FULL et couvertures sont indépendamment recoupés par `Definition`. Les gardes couvrent ρ=2..40 et κ=.01,.5,1,12,31, ainsi que les valeurs ρ=80,160 pour κ=12. Les nombres 304,5839968 et 307204,640569 aux ρ=5,160 sont des présentations décimales ; les décisions et minorants sont certifiés rationnellement.

La cause n'est pas une discontinuité démontrée à nuage fixe : un parent massif redistribue son crédit à deux enfants de courte durée s ; une variation d'un début change leur rapport avec sensibilité 1/s, puis le cône multiplie cette marge par l'échelle absolue √o.

## Complément : absence de constante géométrique en rayon

Pour **chaque κ>0 fixé**, choisir un entier A≥max(48,4κ), puis ρ≥2, L=Aρ⁴, h=L/ρ. Reprendre X et déplacer uniquement y₂ vers (L+1,−h,0). Ainsi ε=max_i‖x_i−y_i‖=1. Les deux triangles restent aigus et η=1 reste fixé.

Noter o=(L²+h²)/4, b=(L²+h²)²/(4L²), s=b−o=L²(1+ρ⁻²)/(4ρ²), d=(2L+1)/4 et a=b'−b≥0. La MEB est 1-lipschitzienne en rayon, donc √b'−√b≤1 et a≤2√b+1≤3L/2≤3d. On a aussi 5d≤s et d≤s/10. Les mêmes votes donnent

μ=d/(2s+2a−d) ≥ d/(3s) ≥ 8ρ²/(15L).

Comme √o≤3L/5 et d≤3L/5,

κμ√o ≤ (72κ/95)ρ² < (A/4)ρ² ≤ √b'−√o.

Le cône est donc actif. En utilisant √o≥L/2,

t_X−t_Y = √b−√b' + κμ√o ≥ −1 + 4κρ²/15 ≥ κρ²/4−1 →∞.

Cela exclut une borne uniforme des **dates d'entrée en rayon** d'ER0h, déjà sur trois sites, η et κ fixes. Pour κ=12 : saut 146,849667 à ρ=5, 596,775429 à ρ=10, 2396,756390 à ρ=20 et 9596,751600 à ρ=40, pour déplacement unité. Les deux premières configurations se traduisent dans u18 et u21 respectivement. Ces calculs ne qualifient aucun export natif ER0h.

La suite non bornée finit par dépasser toute largeur B fixée. Sur une grille u21 finie à pas fixé, on ne peut affirmer absence de **toute** constante : la borne peut dépendre de l'étendue/quantification. S réfute une constante de stabilité indépendante de ces paramètres. Avec la convention ultramétrique ordinaire u(i,i)=0, le seul témoin à trois sites ne suffit pas à réfuter une borne sur les seules réunions hors diagonale ; les quatre valeurs de la selle à cinq sites du rapport sont des témoins supplémentaires, pas ici une nouvelle preuve asymptotique hors diagonale.

Le rapport `majorite_vote/RAPPORT.md:47,203` transforme parfois le seul maximum fini 69,5 d'ER0hr en absence de borne uniforme. Cette implication n'est pas prouvée : sa section 5.3:335–336 dit correctement « conjecture » et « aucune constante uniforme revendiquée ». S concerne ER0h au cône **absolu**, pas ER0hr au cône relatif. Le vérificateur adverse formule précisément « sans petite constante » à sa ligne 52.

## Rejeu et fermeture

Depuis ce répertoire :

```sh
python3 -B check_answers.py > normal.json 2> normal.stderr
python3 -B -O check_answers.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Le script importe seulement la copie figée du petit oracle Python `Definition`. La géométrie, les votes de cette famille et Pκ sont reconstruits ici ; aucun code de règle du workflow n'est importé. Les tests booléens explicites restent actifs sous −O. Les intervalles de racines utilisent des bornes rationnelles avec isqrt, l'égalité est certifiée par classes de carrés, et un éventuel manque de précision serait un refus explicite. Decimal sert uniquement à afficher les dates. `source_manifest_after.json` consigne les empreintes LIVE après lecture et les éventuelles dérives séparément des sources exécutées.
