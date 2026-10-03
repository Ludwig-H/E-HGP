# FULL → points : revue bornée du 3 octobre 2026

Revue en lecture seule des sources v11 au moteur `c40f40798375a0fc37917499401f16876cccbd2a`.
Les 12 dépendances lues sont identiques à ce pin avant/après. Petit modèle Python/Fraction : trois fixtures de six
sites, ordres 1..6, et quatre sites alignés aux ordres 2/3. Les deux étages de référence concordent ; les projections
sont reconstruites depuis leurs seuls enfants et entrées. Aucun moteur natif, G4, HDBSCAN, performance ou qualification
de future projection n'est exécuté. Les premières erreurs du script d'audit sont conservées dans les sorties `initial_*`.

## 1. Première attache exclusive : perte forcée, même sans égalité

Les fixtures `two_triangles_1998` et `two_triangles_1700` portent à k2 les composantes discrètes recouvrantes
`ABC | CD | DEF` entre le niveau carré `249978000484/187489` et `3728225` / `3194656`.
Pourtant C et D sont d'abord couverts **seulement** par CD aux niveaux `998001` / `722500`.
Toutes les entrées first-cover sont singleton sur ces variantes. Toute règle qui fixe définitivement leur attache
à cette première composante obtient ensuite AB, CD et EF, puis la racine ; ABC et DEF sont absents de ses descendants.
Ce n'est pas une faute de départage d'ex aequo ni de FULL. LCA est ici le même singleton et ne corrige rien.
Core k2 perd aussi les deux triangles : le pont1700 conserve CD avant la racine, les deux autres variantes n'ont
que la racine non vide en core. Source : `reference/test_ref.py:76`, `reference/hgp11_ref/families.py:85`.

À k3, cover/LCA porte ABC et DEF dès le même niveau du cercle et aux rayons 1300 et1700.
Core n'y a encore aucun site ; ses groupes ABC/DEF n'apparaissent qu'au niveau carré4000000.
Il faut donc juger la règle de points aux dates demandées, pas seulement constater que la forêt finit par porter la cible.

Le pont2000 exige une attention séparée : les coordonnées de la grille ne sont pas exactement équilatérales.
`AB²=4000000`, `AC²=BC²=3999824`, `CD²=4000000` ; AC/BC arrivent avant CD.
Sur cette fixture **seule**, first-cover LCA k2 conserve ABC/DEF. Son succès ne réfute pas l'échec des variantes courtes
et ne qualifie pas le plateau idéal équilatéral/pont égal du manuscrit. Les commentaires « équilatéraux » ne peuvent
servir d'oracle d'égalité exacte. Le modèle n'exécute pas une réalisation irrationnelle du manuscrit.

## 2. K3 seul ne remplace pas une règle commune aux ordres

Sur `{0,2,100,102}`, les projections core et first-cover LCA à k2 portent les groupes AB/CD jusqu'au parent2500
(entrées cover1 et core4). À k3, les deux naissances ABC/BCD sont à2500 ; B/C ont deux premiers propriétaires.
LCA les reporte à la racine2601 : la famille est A, D, ABCD. Core porte seulement ABCD à10000.
Il s'agit d'un contre-exemple aux **deux règles examinées**, pas d'une impossibilité universelle pour toute projection.
Prendre k3 pour sauver les triangles perd ici les paires. Réunir librement les groupes d'autres ordres ne suffit pas :
la fixture existante `{0,10,11,26,27,45,46}` croise déjà des descendants core k1/k2
(`reference/test_projection_contracts.py:122`).

## Obligations utiles avant le module points

- Publier séparément core, toutes les incidences first-cover et les populations de couverture dynamique ; garder les
  continuations fortes sans naissance, les contacts et les coupes fermées (`docs/MATHEMATIQUES.md:253–299`).
- Tester présence géométrique, conservation après projection, compatibilité laminaire et sélection finale séparément.
  Pour triangles, inclure les trois ponts et une porte du plateau idéal distincte de l'approximation entière.
- Une règle exclusive doit nommer ses pertes et ses dates ; un score sur masses recouvrantes ne garantit pas la masse
  exclusive finale. Le site médian symétrique interdit en général un singleton géométriquement équivariant ; first-cover
  LCA est en outre discontinu sous petite perturbation (`reference/test_projection_contracts.py:72`).

Reproduction : `PYTHONDONTWRITEBYTECODE=1 python3 check_points.py` et même commande avec `-O` depuis ce dossier.
Les sorties closes sont `normal.json` / `optimized.json` ; `verification.json` enregistre leur égalité et les hashes
avant/après. `SHA256SUMS` couvre tous les fichiers de cette preuve sauf lui-même. Ce dossier privé n'est pas une note
active d'audit ni une capsule produit publiée.
