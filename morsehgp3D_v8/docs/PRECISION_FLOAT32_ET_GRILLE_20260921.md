# Précision : garder les données, puis rendre les décisions exactes

Décision utilisateur du21 septembre2026 : préférer les coordonnées
float32 originales de SemanticKITTI. Une grille reste autorisée en option,
avec un paramètre de précision et un pas de **1 mm par défaut**.
Cadre : exploration v8 hors registre, CPU, `not_claimed`. Le profil du
nouveau préparateur est `lossless_float32_input_only` ou
`quantized_u32_fixed_grid_input_only` ; celui du moteur historique reste
`quantized_u16_input_only`. Aucun contrat FULL/GPU n'est acquis.

## Ce qui est maintenant écrit

Le [préparateur](../bench/prepare_lidar_precision.py) garde les sept objets
du protocole spatial : trame entière, deux moitiés et quatre quarts dans
le repère du capteur. Aucun tirage ni plafond de points.

| Option | Coordonnées publiées | Modification géométrique |
|---|---|---|
| défaut `--profile float32` | XYZ float32 little-endian,12octets/site | aucune ; `−0` normalisé en `+0` |
| `--profile grid` | XYZ u32 little-endian,12octets/site, translation entière commune publiée | arrondi isotrope au millimètre par défaut |
| `--profile grid --precision-mm 0.1` | même format entier | arrondi au dixième de millimètre |

La précision est une chaîne décimale positive convertie en rationnel
exact : pas de multiplication par une échelle flottante approximative.
L'arrondi est `floor(x / pas + 1/2)` ; les demi-entiers vont vers le côté
positif, y compris pour x négatif. L'erreur par coordonnée est au plus
la moitié du pas, donc **0,5 mm à1mm**. Ni le float32 préservé ni cette
borne ne prétendent améliorer la précision physique du capteur.

Les sites géométriquement identiques sont dédupliqués globalement ; tous
les retours conservent leur correspondance vers ces sites. Les doublons
d'origine et les fusions provoquées par la grille sont comptés séparément.
Le fichier brut haché conserve aussi les bits de réflectance, adressables
par ID de retour. Seuls XYZ doivent être finis : une réflectance NaN/Inf
n'influence pas la géométrie et n'est pas perdue. C'est une différence
explicite avec le préparateur historique20mm.

La grille est appliquée avant les coupes et reçoit **une seule translation
entière pour toute la scène**, jamais une translation par morceau. Elle
ne change ni le pas, ni les distances sur grille. Les plans capteur suivent
la même translation ; leur position encodée peut être hors de la plage
des coordonnées. Un span d'axe supérieur à2³²−1 entraîne un refus explicite,
jamais une saturation ou un changement automatique de résolution.
Les coupes float32 suivent exactement les signes x/y bruts ; la grille
peut déplacer un point de l'autre côté du plan, ce qui est publié.

Exemples, chaque destination devant être neuve :

```bash
python morsehgp3D_v8/bench/prepare_lidar_precision.py prepare --input scene.bin --output scene_float32
python morsehgp3D_v8/bench/prepare_lidar_precision.py prepare --input scene.bin --output scene_mm --profile grid
python morsehgp3D_v8/bench/prepare_lidar_precision.py prepare --input scene.bin --output scene_fin --profile grid --precision-mm 0.1
python morsehgp3D_v8/bench/prepare_lidar_precision.py read --path scene_float32
```

Le lecteur reconstruit depuis le brut, ne se contente pas de relire des
hashes. **Ces fichiers ne sont pas compatibles avec les sondes u16** :
ne pas leur transmettre un fichier f32/u32 simplement parce que sa taille
en octets serait acceptée par leur chargeur sans en-tête.

## Pourquoi changer le paramètre20mm ne suffisait pas

À1mm, u16 ne couvre que65,535m par axe, contre environ160m pour les
trames étudiées. Élargir seulement les coordonnées ne suffit pas non plus :

- le compactage48bits des triplets confondrait `(0,1,0)` et `(0,0,65536)` ;
- des carrés restent stockés en u32 : `200000²` serait tronqué de
  40000000000 à1345294336. Sur l'axe a=0,b=200000,z=40000, le test
  `|b−a|²−|2z−a−b|²` passerait de25600000000 à−13054705664 ;
- les bornes de produits q3/q4, les cellules Local28 enQ44 et les piles
  dimensionnées pour16bits ne s'étendent pas automatiquement àu32 ;
- garder tous les float32 exige aussi une représentation exacte des
  comparaisons et clés, pas un calcul en double suivi d'un epsilon.

Ces observations ne sont pas des défaillances du moteur dans son contrat
u16 actuel. Elles interdisent son élargissement implicite. La piste provisoire
2,5mm aurait gardéu16 sur ces trois scènes, mais n'est pas retenue après
la préférence utilisateur pour float32/1mm ; aucun résultat moteur n'en
a été capturé ni revendiqué.

## Première primitive numérique exacte

Le [nouveau header](../src/core/float32_predicates.hpp) reçoit directement
les trois mots binaires float32 finis. Pour q2, il décide le signe exact
de `(z−a)·(z−b)` : négatif à l'intérieur strict de la boule de diamètreab,
nul sur la coquille, positif à l'extérieur.

Le chemin filtré calcule un intervalle en double, élargi vers l'extérieur
après les opérations. Il ne décide que si cet intervalle exclut zéro.
Sinon, le repli entier tranche, **y compris les égalités exactes**. Aucun
epsilon, déplacement de point ou arrondi de décision n'est utilisé.

Preuve de capacité du repli : chaque float32 fini est une mantisse entière
de moins de24bits multipliée par2^e, avec−149≤e≤104. En unités de2^-298,
chacun des douze produits du développement `z²−za−zb+ab` est inférieur
à2^554. Douze termes restent strictement sous2^558. Deux accumulateurs
non signés de18mots32bits, soit576bits chacun, séparent termes positifs
et négatifs ; ils sont comparés sans soustraction signée débordante.
Leurs tableaux occupent144octets au total, sans allocation dynamique.

La primitive exige une compilation sans fast-math et sans contraction
flottante. Les tests couvrent les quatre modes d'arrondi usuels ; FTZ/DAZ
n'est pas qualifié expérimentalement ici. Le mode `ExactOnly` n'effectue
aucune arithmétique flottante. Les compteurs mutables restent privés à
l'appel ; les points immuables peuvent être partagés. Les tableaux fixes
ne constituent pas à eux seuls un port GPU.

## Observations et vérifications

Les trois trames disponibles n'ont pas de doublonXYZ exact ; à1mm, aucune
fusion supplémentaire n'est observée. Cela ne signifie pas que la grille
conserve leur topologie ou que d'autres scènes ne fusionneront rien.

| Trame séquence08 | Retours = sites float32 = sites grille1mm | Changements de quart dus à1mm |
|---|---:|---:|
| 000000 | 123389 | 3 |
| 000100 | 124479 | 1 |
| 000200 | 125526 | 4 |

Les15tests du préparateur couvrent profils/défauts, arrondis exacts,
correspondances, sous-normaux et extrêmes float32, valeurs invalides,
limitesu32, corruptions et conservation des échecs. L'oracle q2 indépendant
emploie des fractions rationnelles Python :3923requêtes, dont916intérieurs,
1208contacts et1799extérieurs. Le filtre tranche2715cas ; les1208autres
passent au calcul entier. Ce taux provient de fixtures avec beaucoup de
contacts imposés : **ce n'est pas un taux de repli mesuré sur LiDAR**.
Cinq mutations des résultats sont rejetées ; ce ne sont pas des mutants
compilés. La sondeC++ ajoute49contrôles, dont18rejets NaN/Inf et les
quatre modes d'arrondi.

Voir les [reçus et commandes de qualification](../receipts/float32_precision_20260921/README.md).
Entrées et sorties natives complètes sont conservées, puis rejugées par
l'oracle à la lecture. Les captures initiales et corrigées restent séparées.
Aucun test du moteur u16 n'est présenté comme un test du nouveau moteur
float32 ; aucune campagne GCP n'est engagée pour cette brique locale.

## Suite de développement, sans perdre la parallélisation

Mise à jour après cette capture : le [propriétaire/index natif float32](INDEX_FLOAT32_ET_SUITE_Q34_20260921.md)
de l'étape1 ci-dessous est maintenant réalisé, avec boîtes exactes,
préparation O(n log n), oracles et mesures séparés. Les étapes numériques
q3/q4 et le raccord à la tour restent ouverts.

Mise à jour suivante : [supports et puissances q3/q4](BOULES_FLOAT32_Q3_Q4_20260921.md)
également portés et qualifiés, sans clés globales, bornes de blocs ou
raccord census natif. Ne pas présenter ces primitives comme la tour.

1. Introduire un propriétaire/index qui conserve les float32, avec
   unicité et boîtes conservatrices adaptées. Ne pas émuler ce profil
   par une grille qui supprime silencieusement des bits.
2. Porter les bornes de blocs et les prédicats q3/q4 avec filtres certifiés
   et replis exacts. Les contacts doivent rester indécis jusqu'au calcul
   exact ; garder les comparateurs réduits, sans produits croisés inutiles.
3. Porter les clés de boules, égalités et ordre des événements : des signes
   exacts seuls ne suffisent pas à construire un catalogue exact.
4. Conserver parents immuables et petites tâches partageables q3/q4.
   Qualifier le partage du census entre graines avec le nouveau contrat
   numérique, puis la distribution fine CPU/GPU. Le travail par blocs,
   le résidu et les sorties restent tous à payer.
5. Refaire les mesures entière/moitiés/quarts, K5/K10 et s8/10/12, sous
   chaque profil déclaré. Les chronos20mm ne se transfèrent pas.

La préparation est séparée des décisions géométriques ; cette tranche
n'apporte aucun nouveau résultat de croissance de la chaîne, ni tour
FULL, ni contrat1s/100ms, ni qualification de dizaines de millions de points.
