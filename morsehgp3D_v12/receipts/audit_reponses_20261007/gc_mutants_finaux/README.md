# Gc rebasé : clôture des 18 mutants

Le 8 octobre 2026 à **00:33:54 UTC**, la campagne finale a terminé avec code externe 0 : **18/18 mutants tués
par code**, aucun signal, délai ou échec de construction. Elle complète les 675 portes rapides et six LiDAR de
[gc_rebase_portes](../gc_rebase_portes/README.md), sans rejouer ces campagnes ni leur transférer un autre profil.

Le rapport `runs/mutants_tower_final.json` porte bien sur le prototype **45976be8ddc489f14494937b13e43d0ca5fa0a55**
(repo3, base main 781fbe8d1), les **359 fichiers** de l'arbre qualifié `334101284e02c351…` et le manifeste
`60ee75e6e57e03a6…`. Les sources courantes sont égales à l'archive Git de ce pin. Le rapport homonyme `final2`
de l'ancien repo2, les anciennes campagnes 24/32 et TSan sont hors de cette preuve. Aucun commit de livraison
Gc n'est encore identifié dans ce reçu.

Deux points utiles au développeur :

- `empreinte_de_file_sans_interieur` retire bien la contribution intérieure du hash par cellule de la voie G-L7
  (`src/tower/passes.cpp`). Sa porte `weak_key_resolution` reconstruit les traces, rejoue avec toutes les collisions
  forcées, compare les cibles puis les compteurs du travail (`tests/tower/index_unit.cpp:144–188`). Le rapport
  atteste son rejet par code ; la ligne `CHECK` précise n'est pas archivée. Les cinq comptes structurels sont copiés
  avant le contrôle du travail : celui-ci ne constitue pas une preuve indépendante de ces cinq comptes.
- `table_s_etoile_queue_partielle` vise désormais **la comparaison du catalogue partagé de main** dans
  `src/catalogue/table.cpp`, pas la table 16 octets abandonnée. Il remplace l'égalité du support entier par celle
  du premier site. `support_table` compare aux supports d'une table ordonnée indépendante, y compris des voisins
  présents et absents (`index_unit.cpp:192–225`). Ces requêtes restent dans le domaine des IDs ; elles ne ferment
  pas la recommandation de garde du dernier ID de [gc_support_domain_delta](../gc_support_domain_delta/README.md).

Le lanceur exige d'abord des témoins sans mutation verts. Un échec de compilation serait `INVALIDE`, pas un mutant
tué ; après une porte tuée par code, sa sortie détaillée est supprimée du rapport et les copies sont effacées
(`tests/mutants/run_mutants.py:268–328`). On ne transforme donc pas ces codes en traces de `CHECK` inexistantes.
La commande utilise `taskset -c 2-7`, `--jobs 3 --build-jobs 1` ; les portes peuvent elles-mêmes créer huit fils.
Le RAPPORT déclare cet écart à la consigne de trois fils. Aucun temps mesuré n'est qualifié par ces tests.

[check.py](check.py) vérifie huit hashes avant/après, le préfixe clos du journal de pilotage, les 359 sources et
leur archive, puis la cohorte exacte et les causes contre le manifeste. Il réutilise uniquement la fonction de
hash d'arbre du lecteur antérieur épinglé. Lectures normal et `-O` identiques ; aucun natif, build, CTest ou GCP
lancé par l'auditeur. Aucun brut copié ; les artefacts locaux restent nécessaires. Le pilote final `--essai`
suivant cette campagne est hors de ce reçu et ne vaut pas admission G4.

```sh
python check.py --scratch "$GC"
python -O check.py --scratch "$GC"
```

`GC` désigne le scratch `v12_tour_Gc` contenant les artefacts épinglés dans [capture.json](capture.json).
