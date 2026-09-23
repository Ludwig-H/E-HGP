# LiDAR brut 08/000000 : K10, plans du capteur et densité

23 septembre 2026. **Palier fermé : deux demi-scènes × trois densités,
soit six nouvelles sorties K10 gardées et une scène entière K10 à trois densités
reprises du reçu antérieur. Les quatre quarts restent à mesurer.**
Ce reçu d'audit exploratoire prolonge la
[matrice K5](../lidar_raw_physical_scaling_20260923/README.md) et les
[trois densités K10 de la scène entière](../lidar_raw_k10_density_20260923/README.md).
Les deux plans physiques x=0 et y=0 passent par le capteur dans le
repère float32 d'origine ; le calcul HGP utilise ensuite les coordonnées
entières 1 mm u18 de la trame entière, sans translation par morceau.
Les sous-échantillons globaux emboîtés 1/4 ⊂ 1/2 ⊂ 1 sont intersectés avec
chaque secteur. Les mêmes IDs originaux et les mêmes octets d'entrée que
K5 sont vérifiés par le manifeste SHA-256
6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095.
Les trois entrées de la scène entière sont reprises du reçu K10 antérieur,
sans nouveau calcul.

Binaire v12 épinglé SHA-256
e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80,
K10/s8/W8/static8, six leviers activés, nice 19 sur CPU local partagé.
Chaque cas a un premier essai puis un **rejeu autoritatif** qui
hache le binaire et le payload immédiatement avant et après le
processus. Les douze essais sont conservés ; les six premiers, sans
garde par processus, ne servent pas au tableau. La préparation des entrées
est hors chrono. Le lecteur vérifie octets/IDs et emboîtements, binaire,
commande, entrée FNV, statut, dix ordres, catalogue et SHA des sorties ;
il exige l'égalité des **comptes** des cinq premiers ordres avec le cas K5
de même entrée. Il conserve les tentatives échouées avant arrêt.

| secteur | sites 1/4 / 1/2 / entière | p formes cœur 1/4→1/2 / 1/2→entière | p CPU·s |
| --- | ---: | ---: | ---: |
| scène entière | 30 847 / 61 694 / 123 389 | 1,650 / 1,936 | 1,149 / 1,239 |
| demi x<0 | 15 437 / 30 644 / 61 045 | 1,446 / 1,687 | 1,189 / 1,204 |
| demi x≥0 | 15 410 / 31 050 / 62 344 | 1,915 / 1,230 | 1,194 / 1,216 |

Les deux axes de variation ne racontent pas la même chose. À densité
entière, la pente **spatiale** des formes de la scène au demi x<0 est
2,195, et au demi x≥0 elle est 2,326 ; la pente **en densité** du plein
de 1/2 à entière est 1,936. Les coupes changent les frontières et les
arêtes ; ces pentes spatiales ne sont pas des exposants asymptotiques.

| densité | Σ formes des deux demis / plein | Σ charges cœur / plein | Σ boules catalogue / plein | Σ nœuds tour / plein | Σ CPU·s / plein |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 0,570 | 0,958 | 0,990 | 0,990 | 0,906 |
| 1/2 | 0,587 | 0,970 | 0,993 | 0,994 | 0,931 |
| entière | **0,418** | 0,969 | **0,996** | **0,996** | 0,911 |

Le repère quadratique homogène Σ(n_demi/n_plein)² est 0,500 à chaque
densité. Au plein, les deux tours de demi-scènes produisent presque
autant de boules et de nœuds **en compte** que la tour entière, et
déclenchent 96,9 % de ses charges cœur, mais seulement 41,8 % des formes
cœur matérialisées. Cela isole un coût par cœur croissant quand le
domaine entier est réuni ; la sortie observée ne croît pas de façon
quadratique sur cette coupe. Les catalogues/tours sont recalculés sur
des nuages différents, donc ces sommes ne sont pas une partition
clé par clé du catalogue entier.

Les six rejeux ont exactement les mêmes formes, charges cœur, paires
développées, visites et tests core+cover, catalogues, nœuds, dix ordres,
digests et compteurs de tour que les six premiers essais. Les compteurs
de cache/témoins du ledger varient avec l'ordonnancement W8
(les clés concernées sont listées par cas dans SUMMARY.json). La
variation maximale des CPU·s entre essais est 0,51 %, celle du RSS
3,47 %. La variation maximale du mur chaîne atteint 107 %, celle du
mur q3/q4 155 % : la contention de l'hôte rend ces derniers impropres
à l'inférence de croissance. Les pentes de formes se fondent sur les
compteurs identiques des deux essais.

« complete_relative » signifie que les clés émises sont recoupées, mais
ne certifie pas l'absence de clés jamais émises. Les pentes finies
p=log(W₂/W₁)/log(n₂/n₁) changent la géométrie lorsque la densité ou le
secteur change : elles ne prouvent aucune borne asymptotique. La somme
des tours de morceaux ne reconstruit pas la tour de la scène entière.
Le CPU local partagé et une seule scène ne qualifient pas G4, GPU,
float32 exact (objectif secondaire de v9) ni le contrat industriel.

Lecture reproductible depuis les sources versionnées : régénérer les
entrées avec generate.py du reçu K5, puis lancer run_and_check.py verify
avec --stage halves ; contrôler SHA256SUMS. La commande run --stage halves
rejoue uniquement les cas non encore gardés. Le futur mode --stage all
prolongera cette capture aux douze quarts. Aucune entrée LiDAR brute
n'est copiée dans audits/.
