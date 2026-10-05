# Projection du nouveau perimetre G4 — 5 octobre 2026

Source figee `c97776ea8b8730bc41c8da4b137879cb8b544c2c`, comparee aux inventaires effectivement recueillis dans la session B fermee (`b319efc8477fec234afc0b31e86f8a43e3023641`). Lecture seule ; aucune construction, aucun test natif, aucun appel cloud.

Les nouveaux filtres sanitaires retirent 82 portes de chacun des deux anciens inventaires de 824, dont toutes les 50 portes sans resultat ASan/UBSan et toutes les 65 TSan. Les 742 retenues sont une projection sur l'inventaire historique B : ce nombre n'est pas l'inventaire courant, qui comprend notamment S9. Le retrait concerne catalogue (6), CLI (26), support (1), supports (29), tower (20). Cela reduit le perimetre propose ; cela ne complete pas les anciennes captures partielles.

Les declarations CMake de S9 donnent une projection statique de 28 portes (dont CLI points). Le nouveau filtre sanitizer en garde 12 : six unites/inventaire, fixtures normal/-O, oracle standard normal/-O, CLI points normal/-O. Il exclut les douze portes echelle/LiDAR et les quatre differentiels longs. Les configurations Release ordinaires en gardent 24 ; release_long n'en garde aucune, car les quatre longs sont tous exclus par `_vs_python`. Les portes longues n'ont pas de jumelle -O.

L'oracle independant en bibliotheque standard reste present : definitions exactes, nuages bornes, K=1..4. Les quatre differentiels `_vs_python` comparent la totalite des dates, proprietaires et arbre de points avec la chaine Python qualifiee, jusqu'a K5 et sur les trois trames LiDAR. Les portes CLI echelle/LiDAR ont une lecture structurelle complete et des conditions exactes echantillonnees ; elles ne remplacent pas ces quatre comparaisons completes.

Le plan auditeur `points_differential_plan.json` propose ailleurs appelle directement chacun de ces quatre CTests avec une expression ancree, sans filtres de matrice, et demande Python epingle. A cette source, la construction par defaut enregistre tous les modules presents ; les deux cibles requises `mhgp11_points_probe` et `mhgp11_points_export` existent. Cette relecture etablit la faisabilite statique, pas un resultat de session.

Rejeu normal et optimise :

```sh
python3 replay.py /workspaces/E-HGP
python3 -O replay.py /workspaces/E-HGP
```

Le rejeu depend du commit Git fige et de l'archive locale B dont l'empreinte est controlee. Il peut recevoir un autre chemin d'archive en deuxieme argument. Les seuls membres lus sont les deux couples `tests.json`/`result.json` ; aucune identite cloud, aucun brut ni secret n'est copie. `summary.json` conserve les noms exacts exclus et ceux des anciennes portes manquantes. Les anciennes capsules restent immuables.
