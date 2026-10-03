# État courant du développement v11

3 octobre 2026 ; moteur jugé **c40f40798**. Les lecteurs et documents sont publiés ensuite.
Cette page décrit l'état présent ; sources, échecs et qualifications antérieurs restent dans receipts/Git.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
mode=implementation_v11_full_forests
public_status=not_claimed
```

Priorité : FULL K5 sur LiDAR sans sol, jalon200  ms puis cible100  ms sur G4.
K10, projection sur les points et comparaison à HDBSCAN/Zoltan restent ouverts.

## Moteur présent

Catalogue critique, index/census I/U global, MEB, descentes datées, naissances,
multifusions atomiques, parents et verticales fermées sont implémentés.
u21 est le défaut ; u18/u24 gardent leurs qualifications distinctes.
FULL exige des sites de poids unitaire, bien que Cloud conserve IDs et multiplicités.

Les [voies rapides](PERFORMANCE_FULL.md) comprennent les coupes certifiées de préfixes,
lignes vivantes, tri F3/F4 avec repli exact, tables de populations, ordres concurrents,
unions de racines courantes, BirthRuns et census en signes. Les nouvelles options FULL
restent inactives par défaut.16379 active ensemble graphe/populations/concurrence et
retire mémo4 ; son gain ne sépare pas chacun de ces mécanismes.

Les propriétaires sont vérifiés avant tout hit de table. L'admission couvre
`min(tâches,W−S)` census possédés, car les S espaces sont attachés aux IDs physiques.
Les portes couvrent contextes étrangers, budgets exact/−1, compteurs avec/sans mémo,
quatre arrondis, modes mixtes, FTZ/DAZ, permutations et pannes.

## Résultats clos

[Qualification et banc G4](../receipts/qualification_performance_20261003/README.md) :
**4073/4073 portes, 326 mutants tués**, GCC18/21/24, ASan24, TSan21, poison21 ;
ASan18 num/index/tower344/344, hors catalogue/FENV ; Clang absent.
Le binaire reconstruit du banc passe 94/94 portes ciblées, puis **81/81 prises**,
sorties entières identiques octet pour octet. Les premiers échecs restent préservés.

Baseline v11 895680ff8/2047 et c40/2047/16379, mêmes flags/compilateur/u21,
XYZ/IDs des trois trames entières sans sol 1mm de la séquence 08, K5, W1/W8/W48,
trois prises et rotation des producteurs. Processus/owners neufs, caches OS non vidés.
À W48/16379, médianes FULL **489,099 /345,066 /432,397  ms** ; gain **×2,68–3,28**
face à la baseline. Pics réservés Buffer+Cloud346,0 /298,7 /368,3 MiB, hors RSS.
[Analyse, valeurs et compteurs](../receipts/qualification_performance_20261003/review/analysis.md).

FULL=index+domaine+forêts. Préparation/segmentation, IO/Cloud/Pool, dumps et Python
restent séparés. Les processus natifs prennent1,046 /0,832 /1,032 s à W48/16379,
dumps/destruction inclus. Les phases par ordre se recouvrent ; utiliser les phases
globales disjointes. Naissances/publication changent de périmètre entre les voies.
Les médianes de phases sont indépendantes : ne pas les sommer.

## Prochaines priorités et limites

Génération catalogue 131–176  ms, puis résolution régulière74–131  ms ; K5 concentre
69 % des présentations MEB restantes. Le domaine seul dépasse 200  ms à chaque prise
de deux trames : les forêts seules ne suffiront pas. Tri 9–13  ms et classification 2–3  ms
sont secondaires. Chaque nouveau port exige ses propres portes et un banc contrôlé.

Les neuf prises W48/16379 passent 500  ms ; aucune des81 prises ne passe 200  ms ni 100  ms.
Aucun résultat K10/GPU/multi-millions/plusieurs séquences/points/HDBSCAN acquis.
Le différentiel canonique v10/v11 sur LiDAR entier reste ouvert ; les81 prises comparent deux v11.
Les coquilles restent complètes, contacts conservés ; les préfixes sont des comptes logiques.
La projection FULL→points devra nommer sa règle et ses garanties de frontière.

[Architecture](ARCHITECTURE.md), [provenance](PROVENANCE.md), [forêts](FULL_FORESTS.md),
[audit courant](../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md),
[invariants](../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Aucun build/test natif local. Sessions G4 gardées closes ; lectures normal/−O concordantes.
