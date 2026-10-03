# Contrelecture de claudepts4 close — 3 octobre 2026

Lecture indépendante des métadonnées existantes, sans moteur, fit, build ni GCP.
Les deux sessions sont `worktree_snapshot` de développement : le HEAD indiqué
est e26b48055, pas un pin Git reproduisant leurs fichiers non suivis.

`claudepts4` finit `failed_remote`, worker 1 : prepare, ancienne porte,
synthétique et lidar-a passent ; lidar-b est coupé par l'échéance globale
(`deadline_cut`, 124, 807,004 s, délai nominal 1 500 s). Aucun stderr diagnostique
natif. Arrêt ciblé certifié, `TERMINATED`, génération
2026-10-03T15:44:04.798−07:00. Archive `0d99afad…`, 433 541 octets.

L'archive finale contient **260 résultats OK : 128 synthétiques, 64 lidar-a,
68 voisines**, contre 258 = 128 + 64 + 66 pour claudepts3. Il reste quatre
voisines sans résultat persistant : 001191, 001193, 001194, 001195. Leur statut
de lancement est inconnu. Les comptes 65/67 de la note développeur ne couvrent
pas toutes les pièces des archives closes.

Les **258 cas communs ont exactement les mêmes JSON** après retrait récursif
des seules clés `seconds`, `wall_seconds`, `full_ns`, `export_ns`. Cela porte
sur les observables persistés : notamment les meilleurs IoU arrondis à six
décimales et les nombres de blocs/nœuds/incidences. Aucun dump complet des
dates/propriétaires ou FULL canonique ne prouve toutes les décisions internes.
Les deux cas ajoutés sont c08_001189 et c08_001190.

Après dédoublonnage par XYZ **et labels**, avec priorité aux démos :
5 démos/72 instances, 37 criblage/425, 20 témoins/204, 68 voisines/824.
Les instances voisines sont des observations corrélées par trame. Toutes les
entrées ne sont pas sans sol à 30–60k : lidar-a a 32 462–126 267 sites
(démo04 avec sol), voisines 36 752–81 451. Les moyennes exactes des scores
persistés et sauvetages/pertes, par cohorte et ordre, sont dans
[result_normal.json](result_normal.json).

| Voisines, ordre | H / HDBSCAN | Sauvetages / pertes |
| --- | --- | --- |
| 2 | 0,883863528 / 0,879130150 | 16 / 8 |
| 3 | 0,882952962 / 0,875355869 | 21 / 8 |
| 5 | 0,882195642 / 0,870423988 | 26 / 4 |
| 10 | 0,878733075 / 0,852835358 | 33 / 2 |

Sources jouées : radius f023f6d0 dans pts3, 58952a8b dans pts4 ; même
hierarchy 0da8fce4, ancienne gate eb467b91 dans pts4. Les sources natives
sélectionnées et le hash ELF exportateur déclaré cfdc5450… sont identiques
entre ces prises. Le moteur est CPU sur G4, compilé u21 ; aucune exécution GPU
ni qualification de tout le domaine u21. La porte stricte et ses nouveaux
mutants publiés dans 6c88fe0e ne sont pas qualifiés par pts4. La seule différence
de radius LIVE 457b997f avec 58952a8b est sa docstring (AST contrôlé).

Les 336/338 payloads des archives ont été rehachés avec inventaires exacts
avant extraction. Les captures contiennent seulement sources et métadonnées
de sortie, inventaires/hashes, aucun XYZ, masque, label brut ou binaire KITTI.
[SOURCE_BEFORE.json](SOURCE_BEFORE.json) et [SOURCE_AFTER.json](SOURCE_AFTER.json)
préservent le changement de HEAD et de documentation pendant cette lecture.

`python3 -B check.py` et `python3 -B -O check.py` : 6 997 contrôles, sorties
identiques. Rejeu portable des copies, sans dépendance aux chemins vivants.
Les checks valident les métadonnées ; ils ne relancent pas la campagne et ne
qualifient ni condensation/sélection ni supériorité générale de la projection.
