# Comparatif G4 des survivants — lecteur de publication

Port explicite du lecteur résident à `03decc16c`, sans modification de
l'ancien lecteur ni de ses reçus. Aucune opération cloud dans ce dossier.
Le nouveau protocole ferme séparément son build et ses sorties.

Le lecteur rejoue les validations du protocole depuis les originaux privés,
exige l'arrêt de la même génération et ne publie que les pièces textuelles
énumérées. Clés SSH, archives, coordonnées KITTI, réponses OS Login et
réponse GCE complète ne sont jamais exportées. Les journaux de garde sont
expurgés, avec les hashes des originaux conservés. Une session échouée
reste échouée, même si certaines mesures ont abouti.

Les quatre processus sont W4/ABBA, W4/BAAB, W48/ABBA et W48/BAAB. Chaque
opérateur recrée ses propriétaires, sans remise à zéro du contexte CUDA.
La synthèse distingue les premiers appels CUDA de chaque processus et les
premiers appels de chaque implémentation. Elle agrège seulement les deux
appels répétés restants par version et largeur : pas de soustraction de
l'initialisation ni d'assimilation de quatre appels à quatre départs froids.

Le temps adaptateur inclut ses destructions, mais conserve la sortie native
S et les R masques. Cette sortie est comparée puis détruite avant l'opérateur
suivant ; ajouter ce dernier temps produit une somme **non contiguë**.
Front, oracle, lecture et index sont hors adaptateur, et payés dans le
harnais. Ce comparatif S2 ne qualifie ni FULL, ni croissance, ni 100 ms.

Le lecteur LIVE dépend du snapshot et des originaux privés liés dans le
reçu. Ses tests d'agrégation, d'expurgation, de classification des échecs
et de certification de l'arrêt passent en normal et `-O` : **13 positifs
et 26 refus**. Ils utilisent des données synthétiques, pas le GPU :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_survivors_g4_readback_20260927/selftest.py
python3 -B -O morsehgp3D_v9/audits/b_q34_survivors_g4_readback_20260927/selftest.py
```

État à la création : préparation locale, aucune nouvelle session G4 exécutée.
