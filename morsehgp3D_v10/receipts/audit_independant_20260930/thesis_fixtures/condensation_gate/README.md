# Condensation de l'arbre cible prescrit

30 septembre 2026. Entrée prescrite : deux feuilles portant ABC et DEF,
trois observations unitaires chacune, créées à β=4/3 ; racine à β=2+√3.
Les points entrent à la création de leur triangle. **Cette porte ne produit
pas cet arbre par une règle candidate** et n'exerce aucun vote de boule FULL.

Deux binaires privés existants de la tête publiée à 33fcb53a0 sont réutilisés,
Release et UBSan non récupérant. Leur [fermeture de compilation originale](../../developer_rebound/condensation_reference/build_receipt.json)
et tous les hashes des binaires/dépendances/runtimes sont contrôlés avant
et après les deux appels. Aucun build épinglé modifié, aucune recompilation,
aucun nouvel export/générateur, GCP/GPU ou campagne statistique.

L'API native porte les niveaux en double : les deux valeurs idéales sont
converties après calcul Decimal à 80 chiffres. Les rangs d'entrée et de
création sont identiques par construction. Ce contrôle numérique ne qualifie
pas l'ordre natif d'un calendrier algébrique général.

Les **12 configurations** — deux backends, z=1/2, mcs=2/3/4, EOM et racine
exclue — donnent :

- mcs=2 et 3 : **ABC | DEF**, masses de naissance 3 et 3 ;
- mcs=4 : aucun cluster retenu, les six observations sont bruit.

Naissances, parents, masses, stabilités et sorties sont contrôlés séparément
des labels. Pour les deux feuilles admises, stabilité = 3(λ_triangle−λ_global),
et stabilité de la racine = 6λ_global. Si elles sont trop petites, tous les
points sortent à λ_global. λ=β^(−z/2), calcul indépendant à haute précision.

[execution.json](execution.json), [native.stdin](native.stdin) et
[case_identity.json](case_identity.json) relient les cas aux commandes et
flux capturés. [check.py](check.py) exige deux commandes et les longueurs
exactes, puis juge ces archives en lecture seule. Normal/−O concordent.

Relecture portable : `python3 -B check.py` et `python3 -B -O check.py`.
Hashes locaux vérifiés avant le jugement, aucun nouvel appel natif. La
production de l'arbre candidat, la condensation fractionnaire, le transport
du vote et le raccord R2 final restent des obligations distinctes.
