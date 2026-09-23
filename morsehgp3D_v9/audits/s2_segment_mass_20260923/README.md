# Masse réelle des segments S2 avant le cœur q3/q4

23 septembre 2026. Audit CPU **hors chaîne** de la trace S2 issue du commit
`c265a5dae4dd92059fc78acc0a1d7f52de9c1435` sur la trame brute entière
SemanticKITTI 08/000000, grille isotrope commune 1 mm, 123 389 sites,
K5/s8, q3+q4. Aucune hypothèse d'alignement des points ou des passages.
Cette mesure complète les [coupes capteur et densités emboîtées](../s2_half_density_k5_20260923/README.md)
et l'[appariement des cœurs par arête](../edge_matched_core_20260923/README.md) ;
elle ne relance ni le générateur complet, ni le cœur, ni FULL et ne
constitue pas une qualification G4.

Un **segment** est la suite des arêtes S2 survivantes dans **un** rectangle
WSPD ouvert, après le filtre de paire et avant le certificat du cœur. Ce
n'est ni une composante connexe ni une sortie de tour. Le [sidecar](measure.cpp)
reconstruit le front `MidpointSamples` et le filtre de rectangle `Affine`
avec l'archive du checkout constructeur historique, de SHA `208aabb…`,
**distincte du binaire de la trace**.
Il énumère les 22 034 426 paires des rectangles
ouverts, et joint par paire non orientée d'**IDs bruts** les traces binaires
`edge_matched_core` : les rangs du front ne sont jamais traités comme des IDs.
Chaque arête tracée est retrouvée **exactement une fois**, avec son masque
inclus dans celui du rectangle. Les cinq comptes front/filtre du
[reçu de rectangles](../q34_raw_rectangle_mass_20260923/README.md) concordent
exactement : 6 175 011 rectangles, masse 238 364 135, 2 548 453 ouverts,
masse ouverte 22 034 426 et 503 488 729 visites. La jointure retrouve les
3 986 433 charges et les **559 661 741 formes réellement calculées** du reçu
S2 ; 551 688 875 formes restent après soustraction des deux extrémités par
charge. Les 19 valeurs du [sommaire mesuré](measure.stdout) et ses deux
histogrammes se recoupent dans le [lecteur](verify.py).

| Sélection connue avant le cœur | Rectangles ouverts | Arêtes S2 | Formes F | Part de F |
| --- | ---: | ---: | ---: | ---: |
| Tous | 2 548 453 | 3 986 433 | 559 661 741 | 100 % |
| Segments d'au moins 16 survivantes | **11 174** (0,44 %) | 396 481 (9,95 %) | **478 635 662** | **85,52 %** |
| Segments d'au moins 32 survivantes | 2 645 (0,10 %) | 219 079 (5,50 %) | 453 448 188 | 81,02 % |
| Rectangles de produit au moins 1 024 | 1 747 (0,069 %) | 81 089 (2,03 %) | 397 354 920 | 71,00 % |
| Rectangles de produit au moins 4 096 | 230 (0,009 %) | 56 544 (1,42 %) | 316 888 436 | 56,62 % |

Il y a **351 867 rectangles ouverts sans aucune survivante** ; 1 408 545
rectangles de produit un portent 9 824 229 formes seulement (1,76 %).
Les 2 197 segments les plus coûteux par F, soit le premier 0,1 % des
2 196 586 segments positifs, portent 462 421 824 formes (82,63 %).
Ce classement utilise des tailles de cœur **inconnues avant le cœur** ;
il décrit la concentration mais n'est pas un sélecteur exécutable. Le
nombre de survivantes du segment, lui, est connu à la fin du filtre S2.
Le seul produit n'est pas monotone : les 22 rectangles de produit au moins
32 768 n'ont ici aucune survivante.

**Piste concrète :** garder, pendant la validation déjà faite des survivantes
par rectangle, les offsets et longueurs des segments non vides, puis essayer
le certificat exact à sous-cellules corrélées de
[l'audit pré-cœur](../precore_cell_screen_20260923/README.md) d'abord sur les
11 174 segments de longueur au moins 16. Cela cible 85,52 % des formes avec
0,44 % des rectangles, sans axe LiDAR ni nouveau filtre approximatif.
Répartir ces segments en tuiles de paires et en blocs de cellules ; les
millions de petits segments conservent leur chemin S3. Un segment doit
conserver ses masques q3/q4, les deux seuils de garde et le repli exact.
La [borne corrélée par nœud d'index](../CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md)
donne une voie exacte pour rechercher des gardes par populations entières ;
ses visites et son coût de préparation doivent rester comptés séparément.
Cette proposition n'annonce **aucun** rejet gagné : un témoin commun peut
manquer, la recherche de gardes et le découpage ont un coût, et la couverture
des sous-cellules doit être complète avant de sauter le chargement du cœur.

Comme budget arithmétique **optimiste**, une grille initiale de huit
sous-cellules partage 27 sommets. Calculer une réduction `F_E(v)` à chacun
sur les seuls segments de longueur au moins 16 demande
`27 × 396 481 = 10 704 987` termes paire-sommet, soit 2,24 % des
478 635 662 formes que ces segments chargent aujourd'hui. Pour les
rectangles de produit au moins 1 024, ce rapport descend à
`27 × 81 089 / 397 354 920 = 0,551 %`. Leur 81 089 arêtes portent
41 117 masques q3 et 79 412 masques q4 ; séparer les réductions des deux
voies demanderait jusqu'à `27 × (41 117 + 79 412) = 3 254 283` termes,
0,819 % de F de cette classe. Ce n'est **pas** un gain temporel : deux
réductions, les gardes, les replis et les visites de couverture s'ajoutent,
et une petite valeur de `27S/F` ne prouve pas qu'une seule forme sera évitée.

Les traces épinglées sont non versionnées dans `/tmp` ; le [reçu de provenance](PROVENANCE.json)
nomme `c265a5d` comme commit **de la trace**, puis garde séparément les SHA
de la source du sidecar, de l'archive liée, de son binaire, du manifeste et
des deux reçus parents. Le lecteur statique compare ces fichiers versionnés,
les sommes des histogrammes et les comptes de référence ; `--points`,
`--raw-ids`, `--trace`, `--archive`, `--binary` ajoutent le contrôle LIVE des
octets éphémères. Capture : **41,13 s mur, 330 352 KiB RSS maximal** sur
hôte partagé, dont front et filtre de rectangle ; ce n'est pas un chrono de
la solution proposée ni du pipeline S2.

```sh
python3 -B morsehgp3D_v9/audits/s2_segment_mass_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/s2_segment_mass_20260923/verify.py
(cd morsehgp3D_v9/audits/s2_segment_mass_20260923 && sha256sum -c SHA256SUMS)
```

Pour refaire la jointure tant que les traces existent, compiler
`measure.cpp` en C++20 `-O2` avec les en-têtes du checkout constructeur
épinglé et l'archive `libmhgp9_gen.a` de SHA
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`,
puis passer les fichiers `s00_full_full.u32le`,
`s00_full_full.raw_return_ids.u32le` et le dossier `full/trace`. Les SHA
attendus des entrées sont dans `PROVENANCE.json` et ceux des huit parties
de trace dans `edge_matched_core/RESULTS.json`. Les résultats doivent être
identiques à `measure.stdout`, quelle que soit la présentation des jobs.
Les en-têtes/API du front et du filtre sont inchangés entre `c265a5d` et
le checkout lié ; l'égalité des comptes et la jointure par arête sont la
preuve expérimentale sur ce cas, **pas** une identité binaire des deux
constructions.
La distribution mesurée n'établit aucune borne sous-quadratique, pas même
pour cette famille de scènes ; elle priorise une ablation exacte et costée.
