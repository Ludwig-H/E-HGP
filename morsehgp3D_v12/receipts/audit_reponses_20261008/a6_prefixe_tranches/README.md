# A6 — raccord abstrait du fragment aux tranches réelles

8 octobre 2026, Codex ; complément **CST-0242**, état inchangé. Source A6
`30a69104a697fa2ea2dadd8499bde6c7cb8d72c0`, neuf fichiers identiques à la
publication `ce81936fcf6fe8d2a771bebecf7a91b49bf45258` ; pins dans
[capture.json](capture.json).

**Résultat limité à une entrée T structurelle : ni catalogue HGP réalisé, ni
contre-exécution C++ complète, ni panne native observée.** Le remplissage permet
d'utiliser les vraies tranches de 256 cellules et la fenêtre d'aide de 32
tranches. Cette fixture précise, k1/cinq naissances/513 cellules, ne doit pas être
présentée comme une trame HGP : cinq sites n'offrent même qu'au plus
`C(5,1)+…+C(5,4)=30` supports de boule possibles. Un catalogue géométrique avec
ses propriétaires et sa résolution G reste donc une obligation distincte.

## Entrée et contrôles

`k=1`, cinq `birth_key=0..4`, `birth_rank=0`, numérotation canonique supposée
identique. Les 513 `cell_ball=100..612` et `cell_rank=1..513` croissent strictement.
Chaque ligne CSR a les cibles de naissance suivantes :

| Cellule | Rang | Cibles |
|---|---:|---|
| 0 | 1 | 2, 3 |
| 1 | 2 | 2, 4 |
| 2 à 510 | 3 à 511 | 2 |
| t = 511 | 512 | 0, 1 |
| u = 512 | 513 | 0, 2 |

Les offsets sont les sommes des longueurs : 517 représentants, aucune ligne
vide. Les deux gardes pertinentes sont reproduites séparément dans le modèle :
forme, domaines et monotonie de `check_forest_input` ; type/index de cible et date
strictement antérieure de `resolve_leaves`. Ici aucune cible « cellule » : le
filtre `target_index < processed` n'est pas contourné, il n'est jamais sollicité.
Tous les indices lus par le noyau appartiennent aux cinq naissances. La
numérotation identité est une précondition explicite du sous-test T ; aucune
source de boules n'est fabriquée pour les identifiants synthétiques.

Après les cellules 0 et 1, les composantes sont `{0}`, `{1}`, `{2,3,4}`. Les 509
cellules ajoutées sont inertes. Sans l'indice futur, t fusionne `{0,1}` ; avec
sa première feuille remplacée par 2, t fusionne `{1,2,3,4}`. Dans les deux cas,
u écrit **`up[0]=2`** : les tailles sont respectivement 2<3 ou 1<4. Cette valeur
future reste indépendante du choix litigieux. Les deux histoires ont exactement
quatre événements, aux cellules 0, 1, 511, 512 ; une racine finale 2 ; et des
attaches compatibles avec le contrôle d'historique. Un oracle distinct par
ensembles vérifie les partitions avant t, après t et à la fin.

## Raccord au planificateur et au fragment publié

Les trois tranches sont `[0,256)`, `[256,512)`, `[512,513)`. Le noyau est réservé
par un participant et se trouve dans la première tranche, après les deux unions
initiales. Sous les publications normales requises (`KernelOpen` terminé,
tranche 1 `Done|Leaves`, ni échec ni clôture), un second participant peut réclamer
`max(hint_next=0,kernel_slice=0+1)=1`. La tranche existe et `1≤0+32`.
`run_hint` peut acquérir `processed=0` et parcourir toute la tranche 1, dont t
est la dernière cellule ; u est la première de la tranche suivante.

Le noyau peut conserver le même job pour ces trois tranches. L'aide peut lire
`up[0]=2` pour t selon le fragment relaxed publié, puis lire l'ancien parent 1
pour son deuxième représentant. Les feuilles ajoutées valent 2 ; leurs lectures
de parent retrouvent toujours la racine 2, et leurs écritures conservent cette
valeur. Elles n'introduisent pas de synchronisation aide→consommation du noyau.
Les lectures supplémentaires de préchargement ne choisissent pas les unions ;
leurs indices restent valides. La clôture attend toujours la fin de l'aide avant
de libérer les buffers.

Le modèle réutilise, sans le modifier, le
[graphe de 47 événements](../a6_prefixe_relaxed/README.md) publié et épinglé : il
reste admis par les relations qu'examine ce vérificateur maison ; le pont
publication de feuille release/consommation acquire l'exclut. Les écritures du
noyau sont supposées ordonnées par HB, y compris ses reprises via réservation et
publication release/acquire ; ici le même participant conserve le job.

**On ne développe pas chaque accès du remplissage en un nouveau graphe C++**,
et ce vérificateur n'est pas un outil formel indépendant. Le résultat nouveau
est la compatibilité de la forme interne et des frontières du planificateur
avec le fragment. Il ne qualifie ni les données d'une Session réelle, ni la
concurrence native, ni la production de ces cibles par G. La recommandation du
pont de publication reste celle du reçu précédent ; aucun changement produit.

## Rejeu léger

```
python check.py /workspaces/E-HGP --check
python -O check.py /workspaces/E-HGP --check
```

[results.json](results.json) : neuf sources vérifiées aux deux pins, les deux
histoires, les contraintes de tranche et six contrôles négatifs (ligne vide,
date de naissance, cible hors domaine, cible cellule future, sentinelle, absence
de racine finale). Sorties normal/−O identiques ; aucune invocation native.

Pointeurs : `forest.hpp:44–61`, `forest_build.cpp:15–29`,
`forest_kernel.cpp:71–103,108–130,178–186,230–244`, `stage.hpp:12`,
`pipeline.hpp:69`, `pipeline_run.cpp:154–172,175–192,311–347`,
`pipeline_steps.cpp:122–136`. Les champs de centres, rayons, supports et
coordonnées ne sont pas validés par ce sous-test et ne sont pas lus ici.
