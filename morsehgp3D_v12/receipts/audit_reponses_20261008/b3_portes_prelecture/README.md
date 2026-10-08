# B3 : raccord et portée des portes locales

8 octobre 2026. B3 livré en `545ed987e`. Ce reçu ferme dix-sept fichiers pertinents et les traces déjà
produites, sans compilation ni exécution native par l'auditeur. Il ne juge pas la performance G4.

| Construction | Sélection conservée | Portée |
| --- | --- | --- |
| Prototype `b3r_lot` | 754 passés, aucun saut/échec ; 749,31 s | Table et balayage B3 ; `pipeline_run.cpp` est encore celui d'A6b `f2c106d93`, dont les portes d'aides sont sélectionnées. |
| Prototype `b3r_cles` | 82 passés, aucun saut/échec ; 214,17 s | Ablation table seule, corps de résolution distinct ; même origine A6b du pipeline. |
| Construction principale B3 | **747 sélectionnés, 746 passés, sentinelle LiDAR sautée, zéro échec ; 584,67 s** | Source déclarée `main`, pipeline R1 conservé en Git ; dix-sept fichiers capturés identiques au commit livré. Conducteur `build_ok / ctest 0`. |

Les trois caches déclarent **Release/u21/CUDA OFF**. Les noms et statuts des conducteurs sont recoupés
avec leurs journaux détaillés. Les deux premières sélections ne se transfèrent pas intégralement au
raccord R1+B3 ; la troisième est une preuve locale distincte. Deux ELF de cette troisième construction
sont hachés après exécution et relus stables ; ces empreintes ne constituent pas une fermeture continue
avant/pendant les tests. Sources intégrales, journaux et ELF restent dans le snapshot externe.

## Deux leviers, contrôles distincts

La table mémorise une clé de 128 bits par case ; le balayage teste F dans toute la population I∪U.
Les empreintes MHGP12DP/FUL1 ne sérialisent pas ce nouveau tableau de clés. Les portes `witness_square`,
`support_table` et `slices_identity` contrôlent donc les recherches elles-mêmes, en plus des sorties.
`device_test::same` recherche chaque S* et compare grand livre/niveaux ; les absences de supports voisins
sont jugées séparément par la table ordonnée indépendante de `support_table`.

Le manifeste B3 ajoute **quatre mutants catalogue** (remplissage absent, largeur, clés non transmises,
clé d'une tranche prise à la mauvaise boule) et **deux mutants tour** (garde du dernier site supprimée,
dernier site de population oublié). `table_s_etoile_queue_partielle` est adapté à la nouvelle comparaison.
Chaque substitution vise une occurrence dans les sources livrées. Ce constat et les portes nominales
réussies **ne valent pas six exécutions mutantes** ; aucun nouveau verdict causal n'est admis ici.

`support_table` a maintenant 420 sites : il exige plus de dix cas de débordement fabriqués sur des
supports q4, et leur absence. Sa largeur est neuf bits ; les clés tiennent donc dans le mot bas.
Cette porte ne couvre pas à elle seule les frontières du mot haut. Le profil géométrique u21/u32
ne change pas ce fait : la largeur de la table dépend du **nombre de sites**, pas des coordonnées.
Les témoins arithmétiques à grandes largeurs restent une qualification distincte.

## Petit complément proposé

`proposition.patch` ajoute deux contrôles ciblés aux fixtures existantes :

- pour un S* canonique q2 trouvé, ajouter le nombre de sites comme troisième « site » doit être refusé ;
  sans la garde de domaine, cette requête devient identique au remplissage de la clé q2 ;
- sur le carré ABCD et E=(40,40,40), S=AC est présent et inclus dans F={A,C,E}, mais E est hors du cercle :
  LEM-T1 doit refuser avec `table_miss=false`. Le mutant proposé qui poursuit le balayage malgré un site
  absent devient ainsi causalement observable. Le mutant existant du dernier site vise l'autre sens
  d'erreur, le refus d'une population pourtant contenante.

Le patch ne modifie pas le produit. Son application textuelle et ses trois postimages sont vérifiées ;
**aucune compilation ni réussite native du patch n'est revendiquée**. Il complète les contrôles positifs
déjà présents, sans transformer une recherche réussie dans la table en certificat de contenance.

```sh
python -B check.py DEPOT_GIT SNAPSHOT_B3
python -B -O check.py DEPOT_GIT SNAPSHOT_B3
```

Lecteur de sources/journaux, application du patch en copie temporaire uniquement. Le nombre de tests
réussis ne remplace ni la preuve mathématique des clés, ni l'admission mémoire, ni les mesures des leviers.
