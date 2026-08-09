# Audit numérique du filtre spatial `order_k` — snapshot `4ef89a1`

> **Verdict : NO-GO pour employer la grille comme certificat de complétude.** La justification géométrique par l'union de deux boules est correcte, et aucun défaut de la marge de `outward_ball` n'a été observé sur 200 012 sphères u16. En revanche, une sphère u16 valide suffit à faire convertir des indices de cellules de l'ordre de $10^{11}$ vers `int` avant leur saturation. UBSan détecte le dépassement et le balayage ne rend que deux des quatre points situés exactement sur la sphère. La comparaison flottante de distance est en outre une décision d'exclusion, contrairement au commentaire qui affirme que le flottant ne sert qu'à sur-balayer. Le chemin sûr est une requête à intervalles qui ne rejette que sur séparation prouvée et qui bascule vers un balayage exhaustif dès que l'enveloppe ou un indice n'est pas certifiable.

## 1. Snapshot et probes

| objet | SHA-256 |
|---|---|
| `prototype/order_k_bfs.hpp` audité | `4ef89a194d2adee0e86ddd78cd15caab9af8ec76de8f6d14cca329926f9321a5` |
| probe de dépassement `grid_extreme.cpp` | `a01aa41976d7a57239582c9bb57ad70671919945d95bde4068e773c6778a07e8` |
| probe différentiel et variante à intervalles | `7876dbcc267d7edeb58f458d237236810818da52abba05c6e47619e0d53bdeb7` |
| probe multiprécision `numeric_enclosure.cpp` | `e6655a894754485765d5ebc8070abd1795914189c58ec1e4d6dd51ab939eca3d` |

Les sources et binaires auxiliaires ont été créés uniquement sous `/tmp/orderk-grid-audit.awD1l5`. Le code produit est resté en lecture seule. Les comparaisons géométriques indépendantes utilisent Boost multiprécision; le probe de cellules utilise aussi `-fsanitize=undefined,float-cast-overflow`.

## 2. La justification par deux boules est saine

Pour un pinceau fixé par trois points, la puissance d'un point $x$ par rapport à la sphère de paramètre $t$ est affine en $t$. Si $t_0<t<t_1$, alors $q_x(t)$ est une combinaison convexe de $q_x(t_0)$ et $q_x(t_1)$. Il en résulte deux propriétés utiles :

- tout événement strictement entre $t_0$ et $t_1$ change le signe de la puissance, donc son point appartient à $B(t_0)\mathbin{\triangle}B(t_1)$;
- toute boule intermédiaire vérifie $B(t)\subseteq B(t_0)\cup B(t_1)$, car une fonction affine négative en $t$ ne peut pas être positive aux deux extrémités.

Balayer extérieurement les deux boules d'extrémité suffit donc bien à trouver tout concurrent plus proche. Une nouvelle meilleure sphère découverte après ce balayage est déjà couverte par la même union; la boucle de point fixe n'a pas besoin d'une hypothèse empirique sur le nombre d'itérations.

Cette preuve est toutefois conditionnelle à un point essentiel : la requête spatiale doit réellement rendre **tous** les points des deux boules exactes. C'est précisément la condition que le snapshot ne satisfait pas.

## 3. P0 — conversion d'indice hors plage avant saturation

`Grid::ball` calcule actuellement chaque borne ainsi :

```cpp
lo[d] = (int)std::floor((centre[d] - radius - origin[d]) / cell);
hi[d] = (int)std::floor((centre[d] + radius - origin[d]) / cell);
lo[d] = std::max(0, std::min(dim[d] - 1, lo[d]));
hi[d] = std::max(0, std::min(dim[d] - 1, hi[d]));
```

La saturation intervient trop tard : une conversion flottant-vers-entier hors de la plage de `int` a déjà eu lieu. Des points u16 bornés ne bornent pas le centre de leur sphère circonscrite à la boîte du nuage. Une famille presque coplanaire peut avoir un déterminant entier égal à un et un centre arbitrairement grand à l'échelle permise par la grille.

Fixture exacte, avec $N=65535$ :

```text
a=(0,0,0)
b=(1,0,0)
c=(N,1,0)
d=(N,N,1)
```

Le déterminant affine vaut 1. Le centre exact est $c_x=\frac{1}{2}$, $c_y=\frac{N^2-N+1}{2}$ et $c_z=\frac{-N^3+3N^2-2N+1}{2}$. Pour $N=65535$, le probe obtient :

```text
centre=0.5,2147385345.5,-140724603813884.5
radius=140724617902728.89
cell=1023.9895833068423
dim=64,64,1
```

Les bornes brutes de cellules atteignent environ `-1.37428e11` et `+1.37428e11`. UBSan arrête l'exécution sur :

```text
runtime error: -1.37428e+11 is outside the range of representable values of type 'int'
```

En compilation optimisée ordinaire, le balayage rend seulement les points `0,1`. Les points `2,3`, pourtant exactement sur la même sphère, sont absents. Le filtre de distance ajouté dans `4ef89a1` ne peut pas réparer des cellules qui n'ont jamais été parcourues.

Le défaut est sur le chemin exposé du prototype `order_k_vertices_fast`, qui n'est pas encore intégré au produit. Il peut rester silencieux : si le sous-balayage trouve un candidat plausible, le repli exhaustif conditionné par `best < 0` ne s'exécute pas. Le statut de validité numérique doit donc être propagé indépendamment de l'existence d'un candidat.

Remplacer `int` par `long long` n'est pas une preuve. La correction minimale consiste à comparer la valeur flottante aux bornes de l'intervalle de cellules **avant** toute conversion, puis à saturer ou à déclarer la requête inconclusive. Une valeur NaN, infinie ou hors plage doit élargir la requête, jamais être convertie.

## 4. `outward_ball` mesure bien le rayon d'arrangement, mais n'arrondit pas dirigément

Pour `mhgp::Sphere`, le point `base` appartient à la sphère et le centre relatif vaut $(n_x,n_y,n_z)/d$. Le vrai rayon d'arrangement est donc $R=\frac{\sqrt{n_x^2+n_y^2+n_z^2}}{d}$. `outward_ball` calcule bien cette quantité; il ne commet pas la confusion « rayon de miniboule contre rayon de sphère d'arrangement » observée dans un ancien driver de diagnostic.

En revanche, le commentaire « convertis avec arrondi vers l'extérieur » est littéralement inexact. Les conversions, divisions, additions et la racine sont faites en arrondi binaire64 ordinaire, puis une marge heuristique `1.0000001` et `1e-6` est ajoutée. Cette marge paraît très confortable sous le contrat u16 : les coordonnées entières sont exactement représentables en `double`, un support affine contient deux points distincts donc $R\geq\frac{1}{2}$, et la marge relative est très supérieure à l'epsilon machine.

Le probe multiprécision a exercé 200 000 tétraèdres u16 aléatoires et 12 familles presque singulières à déterminant 1. Il compare la boule retournée à la boule rationnelle exacte, en incluant l'erreur de centre. Résultat :

```text
checked=200012
boundary_rejected=0
min_enclosure_slack=1.0866025404507939616e-06
min_filter_r2_minus_d2=1.8820519883666620e-06
```

Le minimum d'enveloppe apparaît sur une petite fixture unitaire, pas sur les grands centres. Ce résultat crédite le choix de constante comme diagnostic sous u16; ce n'est pas un certificat logiciel. Il manque notamment une construction d'intervalle, un contrôle `den > 0`, des gardes de finitude et une hypothèse compilateur explicite interdisant les transformations qui invalident l'arithmétique d'intervalle.

## 5. La comparaison de distance peut exclure

Depuis `4ef89a1`, `Grid::ball` applique :

```cpp
if (dx * dx + dy * dy + dz * dz <= r2) visit(id);
```

Le flottant ne sert donc plus seulement à sur-balayer : il décide qu'un point n'est pas transmis. La marge actuelle a protégé tous les cas ordinaires du probe : sur 100 000 nuages u16 de 20 points, les 1 124 695 points exactement intérieurs ou sur la sphère ont tous été rendus. La fixture presque coplanaire ajoute quatre points exacts, mais la grille courante n'en rend que deux à cause des indices.

Une campagne aléatoire ne ferme pas le raisonnement. La règle sûre est de calculer un minorant dirigé $D^2_{\mathrm{lo}}$ de la distance au centre et un majorant dirigé $R^2_{\mathrm{hi}}$ du rayon. Un point peut être rejeté seulement si $D^2_{\mathrm{lo}}>R^2_{\mathrm{hi}}$. Si les intervalles se recouvrent, si une opération n'est pas finie ou si le mode d'arrondi n'est pas garanti, le point est conservé.

Une variante instrumentale sous `/tmp` applique cette règle avec `nextafter`, sature les indices avant conversion et garde tout cas ambigu. Sur le même lot, elle rend les 1 124 699 points exacts, y compris les quatre points de la fixture hostile. Cette expérience valide le sens fail-open de la conception; elle ne remplace pas une bibliothèque d'intervalles gardée et testée.

## 6. Conception fail-open recommandée

### 6.1 Enveloppe rationnel-vers-flottant

Construire un objet `SphereEnvelope` contenant :

- trois intervalles de centre $[c_{i,\mathrm{lo}},c_{i,\mathrm{hi}}]$ issus de `base + num/den`;
- un intervalle de rayon carré $[R^2_{\mathrm{lo}},R^2_{\mathrm{hi}}]$;
- un bit `certified` et une cause d'échec.

Chaque conversion `i128 -> double`, division, addition, multiplication et racine doit produire des bornes dirigées. Une façon simple est d'encadrer chaque conversion par les doubles adjacents et chaque opération par `nextafter` vers moins ou plus l'infini, sous `std::numeric_limits<double>::is_iec559`, mode d'arrondi contrôlé et sans `-ffast-math`. Toute valeur non finie rend l'enveloppe inconclusive.

Une alternative moins intrusive consiste à conserver le centre et le rayon scalaires actuels, puis à **prouver** avec les intervalles que la boule scalaire contient la boule rationnelle : si $E_{\mathrm{hi}}+R_{\mathrm{hi}}\leq r_{\mathrm{scalaire}}$, où $E_{\mathrm{hi}}$ majore l'erreur du centre, la requête peut continuer; sinon elle bascule vers le nuage entier.

### 6.2 Indices de cellules

Calculer d'abord les projections extérieures $L_i=c_{i,\mathrm{lo}}-R_{\mathrm{hi}}$ et $U_i=c_{i,\mathrm{hi}}+R_{\mathrm{hi}}$. Pour chaque axe :

1. si une borne ou `cell` est invalide, prendre toutes les cellules de l'axe;
2. comparer le quotient flottant à `0` et `dim-1` avant toute conversion;
3. ne convertir qu'une valeur finie déjà prouvée dans la plage de `int`;
4. en cas d'ambiguïté près d'une frontière, choisir la cellule extérieure supplémentaire.

Le sur-balayage d'une cellule est permis. L'omission d'une cellule ne l'est pas. La construction de la grille et `index_of` restent séparément bornés par u16 et `cell >= 1`; ce sont les centres de sphères, non les points stockés, qui créent le cas extrême.

### 6.3 Filtre par distance

Pour les deux boules d'extrémité du certificat, la voie la plus simple est de supprimer la décision de distance flottante : après la sur-AABB, `mhgp::sphere_side(sp, point) <= 0` décide exactement l'appartenance et conserve explicitement les égalités de coquille. Le flottant ne décide alors que d'ajouter des cellules et l'exact décide les membres de l'union.

Pour la boule agrandie de l'amorce, qui n'est plus une sphère rationnelle du pinceau, former des intervalles pour les trois différences, leurs carrés et leur somme. Rejeter uniquement sur la séparation stricte $D^2_{\mathrm{lo}}>R^2_{\mathrm{hi}}$. Une égalité, un recouvrement ou une opération invalide conserve le point. Le même préfiltre à intervalles peut être placé avant `sphere_side` si sa rentabilité est démontrée, mais il ne doit jamais remplacer la décision exacte aux extrémités certifiantes.

### 6.4 Propagation et repli

`Grid::ball` doit rendre un statut, pas seulement appeler un visiteur. Ce statut doit distinguer au moins :

- enveloppe certifiée ou inconclusive;
- axe saturé ou balayé entièrement;
- comparaison de distance ambiguë;
- repli exhaustif exécuté.

Si l'enveloppe ou la couverture des cellules n'est pas certifiée, le repli parcourt les $n$ points **même si `best >= 0`**. Le candidat déjà trouvé ne constitue pas une preuve que la requête locale était complète. Les compteurs correspondants doivent apparaître dans les reçus; sans eux, un résultat rapide peut masquer une requête numériquement invalide.

## 7. Porte de validation

Avant de réactiver le mot « exact » pour ce filtre :

1. garder la fixture à déterminant 1 ci-dessus comme test permanent sous `float-cast-overflow`;
2. exiger que les quatre points de sa coquille soient rendus par la grille;
3. comparer le filtre à un census rationnel indépendant sur petits nuages, niveaux inclus;
4. injecter NaN, infinies et bornes hors plage directement dans l'API interne de requête et vérifier le sur-balayage;
5. exécuter les mêmes fixtures avec plusieurs niveaux d'optimisation, sans autoriser `-ffast-math`;
6. conserver séparément les compteurs de cellules visitées, points testés, rejets prouvés, ambiguïtés et replis globaux.

Décision : la géométrie de l'union de deux boules peut être conservée. Le filtre `4ef89a1` doit rester `diagnostic_only` jusqu'à ce que les indices soient saturés avant conversion et que toute exclusion flottante soit prouvée ou transformée en sur-balayage.

GCP non utilisé.
