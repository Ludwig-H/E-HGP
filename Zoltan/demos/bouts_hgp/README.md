# Bouts de scène : la hiérarchie HGP retrouve les objets, celle de HDBSCAN non

```text
phase=demonstration_hors_registre
backend=reference_cpu (morsehgp3D_v11 sur G4 : tour FULL native et hiérarchie de points H^r_{k+1} ; HDBSCAN de scikit-learn 1.7.2)
profile=quantized_u21_input_only (trames SemanticKITTI, grille de 1 mm)
mode=illustration
public_status=not_claimed
```

Ce dossier rassemble des **bouts de scène** SemanticKITTI réduits aux seuls points de deux ou trois objets proches :
vélos garés côte à côte, ou vélos et piétons. Il n'y a ni sol, ni fond, ni aucun autre objet. Sur chacun, au même ordre
k, la hiérarchie de HDBSCAN ne contient **aucun** groupe qui recouvre l'un des objets à plus de 50 %, alors que la
hiérarchie de points de HGP en contient un pour **chaque** objet.

La hiérarchie HGP est celle retenue en v11 (`morsehgp3D_v11/docs/HIERARCHIE_POINTS.md`) : chaque point entre dans le
groupe de la tour FULL qui le couvre en premier, après une attente qui le rend stable, puis suit ce groupe dans ses
fusions.

## Comment ils ont été trouvés

1. **Étiquettes.** Toutes les étiquettes des séquences 00 à 10 ont été parcourues : une trame sur 10, puis une sur 2
   pour les vélos et les piétons, plus rares.
2. **Groupes candidats.** On garde les objets d'au moins 50 points, de trois familles : voitures, vélos (vélos garés et
   cyclistes), piétons. Un groupe réunit deux ou trois objets d'une même famille, séparés de moins de 1 m pour les
   voitures, de 0,6 m puis de 1 m pour les vélos et les piétons. Un groupe de la troisième famille contient au moins
   un vélo.
3. **Un bout par groupe.** On prend la trame où les objets sont le plus serrés au regard de leur propre espacement
   interne, et l'on n'y garde que les points de ces objets, au millimètre.
4. **Calcul.** 360 bouts ont été mesurés sur G4 (session gardée `claudebouts1`, commit poussé f1a53fe1c, porte de la
   hiérarchie conforme) : 263 de voitures, 81 de vélos, 16 de vélos et piétons. Les deux hiérarchies sont calculées
   aux ordres k = 2, 3, 5 et 10 ; HDBSCAN reçoit `min_samples` = k et l'on prend son arbre complet.

## Mesure et critères, fixés avant les résultats

Pour chaque objet, on prend le **meilleur IoU** d'un groupe quelconque de la hiérarchie (le recouvrement entre ce
groupe et l'objet : 1 si identiques), comme dans les autres démos de ce dossier.

- **HDBSCAN échoue à l'ordre k** : un objet au moins reste à 0,5 ou moins.
- **HGP réussit à l'ordre k** : tous les objets dépassent 0,5.
- **Un bout est retenu** s'il existe un k où les deux arrivent ensemble. La mention « (tous) » signale que HDBSCAN
  échoue aux quatre ordres mesurés.

Le cas inverse (HGP échoue là où HDBSCAN réussit) est compté et publié plus bas.

## Résultats d'ensemble

| Famille | Bouts | HDBSCAN échoue (un k au moins) | HGP réussit où HDBSCAN échoue | dont « (tous) » | HGP échoue où HDBSCAN réussit |
| --- | --- | --- | --- | --- | --- |
| Voitures | 263 | 0 | 0 | 0 | 0 |
| Vélos | 81 | 23 | 11 | 2 | 5 |
| Vélos et piétons | 16 | 4 | 2 | 1 | 0 |

IoU moyen sur **tous** les objets des bouts de la famille (HDBSCAN / HGP) :

| Famille (objets) | k = 2 | k = 3 | k = 5 | k = 10 |
| --- | --- | --- | --- | --- |
| Voitures (573) | 0,980 / 0,979 | 0,979 / 0,978 | 0,978 / 0,978 | 0,973 / 0,976 |
| Vélos (178) | 0,826 / 0,831 | 0,818 / 0,835 | 0,808 / 0,837 | 0,771 / 0,831 |
| Vélos et piétons (38) | 0,884 / 0,878 | 0,880 / 0,887 | 0,877 / 0,889 | 0,836 / 0,869 |

**Aucune voiture dans ce dossier.** Quand on ne garde que leurs points, deux ou trois voitures sont toujours
retrouvées par HDBSCAN, et par HGP, même presque au contact : 45 des 263 bouts ont un écart de moins de 30 cm, le plus
petit 1 cm. Une voiture est grande et dense ; un contact entre quelques points ne suffit pas à la confondre avec sa
voisine. Dans les scènes entières, c'est le sol et le voisinage qui font échouer HDBSCAN (démos 01 à 04).

## Catalogue

| bout | trame | objets (points) | écart | k | HDBSCAN, meilleur IoU par objet | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | image |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| b00_001472_velos_40_42_59 | 00/001472 | vélo (82), vélo (139), vélo (161) | 0,07 m | 5 (tous) | 0,84 / **0,41** / 0,57 | 0,84 / 0,72 / 0,60 | 0,61 / 0,72 | [png](b00_001472_velos_40_42_59_k5.png) |
| b08_002776_velos_17_64 | 08/002776 | vélo (198), vélo (95) | 0,13 m | 5 (tous) | 0,75 / **0,36** | 0,70 / 0,53 | 0,56 / 0,61 | [png](b08_002776_velos_17_64_k5.png) |
| b08_002852_velos_6_51 | 08/002852 | vélo (157), vélo (122) | 0,05 m | 5 | 0,74 / **0,44** | 0,85 / 0,83 | 0,59 / 0,84 | [png](b08_002852_velos_6_51_k5.png) |
| b00_001470_velos_43_61 | 00/001470 | vélo (138), vélo (112) | 0,12 m | 5 | 0,82 / **0,48** | 0,96 / 0,71 | 0,65 / 0,84 | [png](b00_001470_velos_43_61_k5.png) |
| b08_001170_velos_43_57 | 08/001170 | vélo (141), vélo (100) | 0,11 m | 5 | 0,70 / **0,43** | 0,94 / 0,65 | 0,56 / 0,80 | [png](b08_001170_velos_43_57_k5.png) |
| b00_001502_velos_28_66 | 00/001502 | vélo (146), vélo (63) | 0,03 m | 5 | 0,74 / **0,40** | 0,78 / 0,60 | 0,57 / 0,69 | [png](b00_001502_velos_28_66_k5.png) |
| b06_000800_velos_8_12_13 | 06/000800 | vélo (86), vélo (101), vélo (77) | 0,04 m | 5 | **0,46** / 0,56 / 1,00 | 0,64 / 0,69 / 1,00 | 0,67 / 0,78 | [png](b06_000800_velos_8_12_13_k5.png) |
| b08_001182_velos_55_56_57 | 08/001182 | vélo (73), vélo (132), vélo (444) | 0,03 m | 10 | 0,67 / **0,37** / 0,95 | 0,77 / 0,51 / 1,00 | 0,66 / 0,76 | [png](b08_001182_velos_55_56_57_k10.png) |
| b00_001466_velos_42_59 | 00/001466 | vélo (192), vélo (118) | 0,03 m | 10 | 0,64 / **0,38** | 0,68 / 0,52 | 0,51 / 0,60 | [png](b00_001466_velos_42_59_k10.png) |
| b10_000424_velos_2_3 | 10/000424 | vélo (163), vélo (98) | 0,04 m | 5 | 0,78 / **0,50** | 0,79 / 0,51 | 0,64 / 0,65 | [png](b10_000424_velos_2_3_k5.png) |
| b06_000800_velos_pietons_2_8_12 | 06/000800 | piéton (117), vélo (86), vélo (101) | 0,04 m | 10 (tous) | 0,98 / 0,50 / **0,43** | 0,99 / 0,64 / 0,70 | 0,64 / 0,78 | [png](b06_000800_velos_pietons_2_8_12_k10.png) |
| b00_002140_velos_pietons_1_6 | 00/002140 | piéton (82), vélo (78) | 0,10 m | 10 | 0,76 / **0,49** | 0,94 / 0,87 | 0,62 / 0,90 | [png](b00_002140_velos_pietons_1_6_k10.png) |

Tableau copié de `catalogue.md`, écrit par `tools/choisir_bouts.py`. « Écart » : plus courte distance entre les points de deux objets du bout. En gras : l'objet que HDBSCAN manque. La
colonne k donne l'ordre montré par l'image ; `bouts.json` donne tous les ordres où le bout est retenu.

## Lire une image

Vue de dessus, tournée selon l'axe principal du bout, trois panneaux :

1. **vérité** : chaque objet dans sa couleur (bleu, orange, violet, dans l'ordre du tableau) ;
2. **HDBSCAN** : son meilleur groupe pour l'objet qu'il manque ;
3. **HGP** : son meilleur groupe pour le même objet.

Dans les panneaux 2 et 3 : vert = point de l'objet pris dans le groupe ; rouge = point d'un autre objet pris dans le
groupe ; bleu = point de l'objet laissé hors du groupe ; gris = autres points.

## Ce que ces exemples ne montrent pas

- **Le meilleur groupe par objet est optimiste.** On choisit, objet par objet, le meilleur niveau de la hiérarchie ;
  ce n'est pas encore un découpage automatique en clusters, qui reste à mesurer.
- **Plusieurs exemples sont corrélés.** Les quatre bouts de la séquence 00, trames 1466 à 1502, montrent la même
  rangée de vélos vue à des instants voisins ; la trame 06/000800 fournit deux groupes.
- **Deux cas sont limites :** b10_000424 (0,50 contre 0,51) et b08_001182 (HGP à 0,51).
- **L'inverse existe.** HGP échoue là où HDBSCAN réussit dans 5 couples (bout, k), tous des vélos :
  b00_001466_velos_42_59 (k = 2), b06_000016_velos_10_18 (k = 5), b06_000774_velos_14_15 (k = 3),
  b06_000774_velos_6_14_15 (k = 3), b08_001182_velos_55_56_57 (k = 2). Les deux échouent dans 52 couples de vélos et
  8 de vélos et piétons. Le détail de chaque bout mesuré est dans `evalues.json`.
- **Les bouts sont artificiels.** Retirer le sol et le voisinage supprime une partie de la difficulté réelle, et les
  échecs dépendent de l'annotation SemanticKITTI.

## Reproduire

```sh
# 1. Recherche des bouts (étiquettes en local, nuages par requêtes partielles ; sorties hors du dépôt)
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out SORTIE1 --step 10
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out SORTIE2 --step 2 --gap-velo 1.0 --familles deux_roues
# 2. Hiérarchies sur G4 : session gardée gcp-migration/v11_session.py, plan et reçu dans
#    morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/
# 3. Choix, images et catalogue
python3 Zoltan/demos/tools/choisir_bouts.py --bouts LOT/bouts.json --data LOT/data --results RESULTATS/lidar \
    --out Zoltan/demos/bouts_hgp --par-famille 12
# Refaire les points des bouts de ce dossier depuis les archives officielles (dans data/, ignoré par git)
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/bouts_hgp --rebuild Zoltan/demos/bouts_hgp/bouts.json
```

`bouts.json` épingle, pour chaque bout, l'empreinte sha256 de la trame, des étiquettes et des fichiers du bout.

## Licences

Comme pour le reste de ce dossier : code et texte sous MIT ; les **images** sont des œuvres dérivées de KITTI
(CC BY-NC-SA 3.0) et de SemanticKITTI (CC BY-NC-SA 4.0), non commerciales, partagées dans les mêmes conditions, avec
attribution : Geiger et al., CVPR 2012 ; Behley et al., ICCV 2019. Aucun octet KITTI ni aucune coordonnée de point
n'est versionné : `data/` est ignoré par git.
