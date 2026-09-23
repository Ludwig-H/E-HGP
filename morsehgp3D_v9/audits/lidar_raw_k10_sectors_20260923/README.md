# LiDAR brut 08/000000 : K10, plans du capteur et densité

23 septembre 2026. **Matrice complète : sept secteurs physiques × trois
densités.** Dix-huit nouvelles sorties K10 gardées (deux demi-scènes et
quatre quarts aux trois densités), plus une scène entière K10 à trois
densités reprise du reçu antérieur.
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
Les six cas de demi-scène ont chacun un premier essai puis un
**rejeu autoritatif** ; les douze quarts ont un essai autoritatif.
Chaque essai autoritatif hache le binaire et le payload immédiatement
avant et après son processus. Les 24 essais sont conservés ; les six
premiers, sans garde par processus, ne servent pas au tableau.
La préparation des entrées est hors chrono. Le lecteur vérifie
octets/IDs et emboîtements, binaire,
commande, entrée FNV, statut, dix ordres, catalogue et SHA des sorties ;
il exige l'égalité des **comptes** des cinq premiers ordres avec le cas K5
de même entrée. Il conserve les tentatives échouées avant arrêt.

| secteur | sites 1/4 / 1/2 / entière | p formes cœur 1/4→1/2 / 1/2→entière | p CPU·s |
| --- | ---: | ---: | ---: |
| scène entière | 30 847 / 61 694 / 123 389 | 1,650 / 1,936 | 1,149 / 1,239 |
| demi x<0 | 15 437 / 30 644 / 61 045 | 1,446 / 1,687 | 1,189 / 1,204 |
| demi x≥0 | 15 410 / 31 050 / 62 344 | 1,915 / 1,230 | 1,194 / 1,216 |
| quart x<0,y<0 | 7 649 / 15 217 / 30 265 | 1,564 / 1,676 | 1,208 / 1,240 |
| quart x<0,y≥0 | 7 788 / 15 427 / 30 780 | 1,525 / 1,708 | 1,201 / 1,192 |
| quart x≥0,y<0 | 7 692 / 15 619 / 31 391 | 1,871 / **2,057** | 1,171 / 1,251 |
| quart x≥0,y≥0 | 7 718 / 15 431 / 30 953 | 1,505 / 1,688 | 1,193 / 1,262 |

**Un seul des 14 liens de densité** franchit p_formes=2 : le quart
x≥0,y<0 de la demi-densité à la densité entière (33 730 438 →
141 799 450 formes). Le CPU du même lien reste à p=1,251 ;
la plus grande pente CPU des 14 liens est 1,262. Le quart sensible
K5 franchissait 2 sur le premier lien, pas le second. Le
franchissement K10 n'établit aucune croissance quadratique générale,
mais interdit de déclarer l'axe des formes sous-quadratique dans tous
les secteurs de cette scène. Ce compteur correspond aux formes
effectivement préparées à chaque charge cœur, pas à un nombre de
candidats théoriques.

Les deux axes de variation ne racontent pas la même chose. À densité
entière, la pente **spatiale** des formes de la scène au demi x<0 est
2,195, et au demi x≥0 elle est 2,326 ; la pente **en densité** du plein
de 1/2 à entière est 1,936. Les deux liens du demi x≥0 au quart x≥0,y≥0
franchissent aussi 2 aux densités 1/4 et 1/2 (2,034 et 2,439).
Les coupes changent les frontières et les arêtes ; ces quatre pentes
spatiales ne sont pas des exposants asymptotiques.

| densité | morceaux | Σ formes / plein | Σ charges cœur / plein | Σ boules catalogue / plein | Σ nœuds tour / plein | Σ CPU·s / plein |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 2 demis | 0,570 | 0,958 | 0,990 | 0,990 | 0,906 |
| 1/4 | 4 quarts | 0,424 | 0,936 | 0,979 | 0,980 | 0,852 |
| 1/2 | 2 demis | 0,587 | 0,970 | 0,993 | 0,994 | 0,931 |
| 1/2 | 4 quarts | 0,413 | 0,953 | 0,985 | 0,986 | 0,878 |
| entière | 2 demis | **0,418** | 0,969 | **0,996** | **0,996** | 0,911 |
| entière | 4 quarts | **0,374** | 0,958 | **0,991** | **0,992** | 0,872 |

Le repère quadratique homogène Σ(n_morceau/n_plein)² est 0,500 pour
les demis et 0,250 pour les quarts. À densité entière, les morceaux
produisent presque autant de boules et de nœuds **en compte** que la
tour entière et déclenchent 95,8–96,9 % de ses charges cœur, mais
seulement 37,4–41,8 % des formes cœur matérialisées. Cela isole un
coût par cœur croissant quand le domaine entier est réuni ; les comptes
de sortie ne montrent pas de carré sur ces coupes finies. Les
catalogues/tours sont recalculés sur des nuages différents, donc ces
sommes ne sont pas une partition clé par clé du catalogue entier.

La scène entière K10 atteint 8,219 GiB de RSS et 905,514 CPU·s ;
les demi-scènes entières culminent à 4,753 GiB et les quarts entiers
à 2,720 GiB. Le rapport Σ CPU·s des quarts / plein vaut encore 0,872,
alors que leurs formes ne valent que 0,374 : supprimer les formes
supplémentaires du plein doit être jugé avec ses coûts de bornes,
de recherche, de catalogue et de tour, pas sur ce compteur seul.

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
le profil float32 exact (objectif secondaire de v9) ni le contrat industriel.

Lecture reproductible depuis les sources versionnées :

    python3 morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py --repo /workspaces/E-HGP --out /tmp/mhgp9-raw-k10-density-20260923
    python3 morsehgp3D_v9/audits/lidar_raw_k10_sectors_20260923/run_and_check.py verify --stage all
    cd morsehgp3D_v9/audits/lidar_raw_k10_sectors_20260923 && sha256sum -c SHA256SUMS

Le mode run --stage all rejoue uniquement les cas non encore gardés.
Aucune entrée LiDAR brute n'est copiée dans audits/.
