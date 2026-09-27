# Clôture de la qualification géométrique indépendante

27 septembre 2026. Reçu privé clos `status=passed` :
`/workspaces/E-HGP/build/v9-weighted-geometry-qualification-20260927-r1/receipt.json`.
SHA256 `ee745814ed2009724e288f3e1e8c748cda9866421ab567b39612e1b1dbfe3483`.

Les 37 commandes archivées passent : neuf gates préalables, puis 28 exports
de petits nuages jugés par l'oracle rationnel indépendant. Les 482 pins
(sources, dépendances de compilation, exécutables et reçu de build) sont
identiques avant/après. Binaire natif :
`/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export`,
SHA256 `a53f1c4d35426ad1f22a68ed6c2a4dceb7482284e52f3d5fc61c80f57cf44cdb`.

Sommes sur les 28 cas, avec répétitions d'un même nuage à plusieurs K :

| Contrôle indépendant | Compte |
|---|---:|
| Boules du catalogue comparées exactement | 774 |
| Dont coquilles non régulières | 69 |
| Sous-ensembles de K+1 points examinés exhaustivement | 715 |
| Cofaces de Gabriel identifiées | 281 |
| Masques de coquille rejetés car ne contenant pas le centre | 70 |
| Coupes strictes/fermées : Čech = Gabriel = FULL non trivial | 1 076 |

Les deux lectures LIVE suivantes passent après clôture, sans nouvelle
exécution géométrique :

```text
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/qualify_geometry.py --readback /workspaces/E-HGP/build/v9-weighted-geometry-qualification-20260927-r1
python3 -B -O morsehgp3D_v9/experiments/weighted_clustering_20260927/qualify_geometry.py --readback /workspaces/E-HGP/build/v9-weighted-geometry-qualification-20260927-r1
```

La capsule est conservée octet pour octet. Son ancien champ
`production_geometry_rerun=false` signifie **aucun grand nuage de production
ni scène de 1 200 points**. Il ne signifie pas « aucune géométrie exécutée » :
les 28 exports natifs de la grille, ainsi que les appels internes des tests
CLI et du gate, exécutent bien le moteur sur de petits nuages. Aucun GCP.

Les cas C++ du gate sont distincts de l'oracle Python `Fraction`. Les neuf
tests purs de ce dernier comprennent dix corruptions d'un JSON synthétique,
jamais assimilé à une preuve native. Les suites modèle et EOM sont archivées
en normal/−O mais ne servent pas d'oracle de miniball.

Portée : identités du catalogue, niveaux exacts, incidences et couvertures
non triviales sur ces petits cas. Ni toutes les dégénérescences possibles,
ni la qualité des futurs labels pondérés, ni une borne globale de coût,
ni une qualification GPU ne sont déduites de ce résultat.

Le [protocole de l'oracle](README_ORACLE.md) et l'[audit mathématique](AUDIT_MATH.md)
précisent l'indépendance, les conditions sur les facettes isolées et les
limites du raccord à T_K. Cette note post-capture n'est pas une source
exécutée et ne modifie aucun pin de la qualification.
