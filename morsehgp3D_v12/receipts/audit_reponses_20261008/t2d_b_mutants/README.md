# T2-d B — clôture des mutants num et tour

Supplément du 8 octobre 2026 au [reçu partiel](../t2d_b_traces/README.md),
qui reste inchangé. Lecture seule des rapports/journaux du développeur ;
aucune compilation, sonde native, donnée LiDAR ou campagne GCP par l'auditeur.

Les deux campagnes ciblées sont closes : **huit mutants num à 05:31:12 UTC,
neuf mutants tour à 05:44:39 UTC**, tous `TUE` par `code`, témoins verts.
Le conducteur se termine à 05:44:39. Les manifestes, sélections `--only`,
portes et empreintes source concordent avec les rapports. Avec les 22
mutants index déjà clos, cela donne 39 mutants détectés, sans mort déclarée
par signal, délai ou construction.

L'arbre de 385 fichiers `845776efa2f4ed75…` est identique à celui du reçu
partiel et aux empreintes inscrites dans les deux rapports. Il est relu et
comparé à la source vivante à la nouvelle capture. Les dates et pins exacts
figurent dans `capture.json`. La retouche du pilote, hors de `COPIED`,
n'étend pas cette qualification à son admission de campagne.

À 05:43:53.927 UTC, les sources de trois dernières copies mutées encore
présentes ont aussi été comparées aux substitutions attendues :
`entiere_triangle_au_dela_de_trois`, `proposition_non_triee` et
`proposition_indices_de_partie`. Leurs trois corps correspondaient exactement
au manifeste. Le reçu conserve les empreintes observées ; son lecteur
reproduit ces substitutions sur la source figée. Cette vérification de
source ne remplace pas un relevé du binaire de chaque mutant.

Le runner supprime les constructions et sorties individuelles des mutants
tués ; nous recoupons ses rapports finaux, journaux globaux, codes extérieurs
et sources, sans prétendre rejuger des sorties natives désormais absentes.
Aucun temps G4, gain des leviers, profil large ou comportement CUDA n'est
qualifié ici.

Rejeu normal et optimisé (sorties égales au champ `result`) :

```sh
python check.py --evidence /capture/mutants --source /capture/traces/source
python -O check.py --evidence /capture/mutants --source /capture/traces/source
```

Les captures extérieures sont indiquées dans la passation. Le lecteur ne
lance que des lectures et hashes ; les reçus restent légers et ne copient
ni sources complètes ni journaux volumineux dans le dépôt.
