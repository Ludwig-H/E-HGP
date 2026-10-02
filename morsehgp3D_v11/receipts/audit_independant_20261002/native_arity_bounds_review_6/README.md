# Bornes de puissance native par arité de présentation

Verdict mathématique : les nouvelles bornes i128 q1/q2/q4 pour B18/21/24
et q3 pour B18 sont valides sur les Point certifiés, **sans supposer un
centre critique ou intérieur au hull**. Aucun contre-exemple trouvé à ces
domaines. Les fichiers demandés comme WIP étaient déjà publiés, propres,
à `9d639e1460e7a75b9af5340bc1fc436ea00c1d07` lors de leur copie.
Les neuf copies et les observations après lecture restent distinctes.

Le périmètre est la preuve des intermédiaires de
[native_power](source/morsehgp3D_v11/src/num/predicates.cpp#L19), depuis les
coefficients des factories de [Sphere](source/morsehgp3D_v11/src/num/sphere.cpp).
Aucune compilation, exécution native, GCP ou qualification de performance.

## Preuve q4 à 24 bits

Poser M=2^B et L=M−1. Les coordonnées des supports et requêtes sont dans
[0,L]. Pour trois Point a,b,c, une composante de `(b−a)×(c−a)` est l'aire
orientée d'un triangle dans le **même carré** [0,L]². L'expression développée
est affine séparément en chacune des six coordonnées ; ses extrema sont
donc atteints aux coins. Là, ses valeurs sont 0 et ±L². Ainsi chaque
composante a une magnitude au plus L², donc strictement inférieure à M².
Cela ne vaut pas pour deux Vec arbitraires, qui peuvent donner 2L².

Pour u=b−a, v=c−a, s=d−a : les normes carrées sont <3M² ; chacune de
leurs sommes partielles tient en i64. Les composantes de `v×s`, `s×u`
et `u×v` sont <M² en valeur absolue. Les trois termes du déterminant
sont chacun <M³, donc chaque somme partielle est <3M³ ; le dénominateur
normalisé D=2|det| est <6M³. Chaque terme de Cramer pour N_j est <3M⁴,
donc chaque somme partielle et |N_j| sont <9M⁴. La normalisation des signes
est sûre avant l'emploi de ces coefficients.

Avec w=z−a, `D||w||²` est <18M⁵, et chacun des trois produits
`N_j(−2w_j)` est <18M⁵ en valeur absolue. La somme des quatre magnitudes
est **<72M⁵** : elle majore tous les produits et toutes les sommes
partielles dans l'ordre du code, sans annulation supposée. À B24,
`72M⁵ = 72·2^120 < 2^127`. Les valeurs entières sont donc dans le domaine
signé i128, y compris pendant chaque addition. Les conversions de dot
vers i128 et le facteur `−2w_j` sont exacts. Le petit déterminant d'un
support presque dépendant peut éloigner le centre ; il ne change pas
cette borne sur N et D.

q1 a N=0, D=1 et une borne 3M². q2 a D=2, |N_j|<M, et une somme de
magnitudes <12M². Pour q3, les bornes générales |N_j|<24M⁵ et D<24M⁴
donnent une somme de magnitudes <216M⁶. Cela tient en i128 à B18 ; la
sélection conserve Wide à B21/B24. Les autres calculs de niveau et leurs
budgets ne sont pas requalifiés par ce reçu.

## Garde de portée : présentation q3, boule qmin2

Soit T={(0,0,0),(L,0,0),(0,L,0)} et z=(L,L,L). La factory q3 donne
N=(L⁵,L⁵,0), D=2L⁴. Le premier produit vaut 6L⁶ et le total vaut 2L⁶.
À B21, le premier produit a **129 bits**, dépasse 2^127, alors que le
total a 127 bits et reste strictement inférieur à 2^127. Une preuve
portant seulement sur le résultat final serait donc insuffisante.
À B24, le total lui-même a 145 bits.

Cette boule possède qmin2 : les deux extrémités de l'hypoténuse forment
un support de milieu égal au centre. Pourtant la **présentation q3**
requiert Wide. Le tag privé de présentation, fixé par les factories,
est le bon critère ; qmin ne peut le remplacer. Le code capturé respecte
cette distinction et conserve la conversion contrôlée de `power` vers
SideInt, tandis que `side` prend le signe exact de l'intermédiaire natif.

## Contrôles exacts bornés

[check.py](check.py) utilise les entiers Python et un solveur de Gram
Fraction indépendant. **499 gardes** passent en normal et -O, avec
sorties identiques, sans importer aucun code produit. Les coins du carré
vérifient les extrema aux trois profils ; la preuve multiaffine ci-dessus
justifie leur extension à toutes les coordonnées du domaine.

Un témoin q4 non critique utilise a=floor(L/2) et les quatre sites
`(0,0,0),(a,a−1,1),(1,a,1),(a,2a−1,2)`. Son déterminant est 1, D=2,
son centre est hors du cube et ses poids ne sont pas tous positifs.
Le solveur rationnel confirme le centre de Cramer et l'identité entre
le polynôme de puissance et D fois la différence géométrique des rayons
carrés. Les quatre supports sont exactement sur la coquille. À B24,
les produits et sommes partiels de ce témoin atteignent 118 bits ;
ce maximum observé ne remplace pas la borne générale de 127 bits.

[RUN.json](RUN.json) conserve les commandes et premières sorties ;
`SOURCE_BEFORE.json` et `SOURCE_AFTER.json` ferment les copies et suivent
séparément toute évolution du checkout développeur. Le manifeste est
vérifié par le lecteur lorsqu'il est présent. Les portes natives/G4,
leur provenance et leurs mutants appartiennent à d'autres reçus.
