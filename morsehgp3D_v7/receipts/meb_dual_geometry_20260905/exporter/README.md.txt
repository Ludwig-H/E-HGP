# Préparation privée d'un export MEB à deux budgets

Statut : export non exécuté, protocole local hors produit. Aucun changement
du CLI F, aucun benchmark, aucune compilation et aucun GCP. Les seuls
tests de cet exporteur sont des simulations Python sur fichiers temporaires.

## Admission et projection

`export.py` admet exclusivement les deux reçus fermés suivants, puis
réapplique leurs juges et liaisons de dépendances épinglés en lecture seule :

- `build/v7_meb_dual_budget_prototype/run_20260905/receipt.json`, SHA-256 `a7dc00201920a678c42e75436cb09ecf8a95b63dd660e587b814cdc0b4a1ea0a` ;
- `build/v7_meb_dual_budget_geometry/run_20260905/receipt.json`, SHA-256 `b81d8e480b158710874de230c3485f79d0a42f1cb228e321c750de0f58bed49e`.

L'admission compare les 38 artefacts de chaque capture, la clôture, les
6 commandes et codes 0/0/0/4/2/2, les sorties brutes, les sources avant/après,
les macros/options, l'affinité CPU 0, les 20/21 dépendances locales et le
binaire effectivement testé. Le juge géométrique exige également les
mêmes métriques hors cause/statut/violations entre nominal et mutant, et
la causalité exacte 46 437 = 46 431 + 6. Rien n'est réexécuté.

Projection prévue, recalculée par le script avant la première écriture :

- 76 copies de captures : 37 artefacts non ELF et un reçu par qualification ;
- 67 snapshots de sources, union des 61/66 pins, dont 2 ELF exclus de l'union de 69 ;
- 6 préparations historiques supplémentaires (manifestes, sommes et deltas) ;
- 5 snapshots inertes de ce protocole d'export ;
- 7 fichiers générés : README, provenance, exclusions, validation en lecture seule, manifeste de reconstruction, manifeste de clôture et sommes.

Total : **149 copies d'entrée + 5 snapshots d'exporteur = 154 copies**,
puis **161 fichiers** en comptant les sept documents générés.
Les quatre exclusions ELF sont explicites : compilateur, CLI F protégé,
exécutable tiny et exécutable geometry. Pas de section coût dans cet export.

Les copies ne changent aucun octet ni terminaison de ligne ; les snapshots
sources et scripts portent le suffixe `.txt`. Chaque copie reçoit origine
absolue, cible, taille et SHA-256 dans `provenance.json`. Les fichiers générés
sont inventoriés dans `manifest.json`, qui ne s'inclut pas lui-même ;
`SHA256SUMS` inclut le manifeste et tous les autres fichiers, hors lui-même.

Les sources et les deux qualifications sont relues à la frontière finale.
Toutes les copies sont de nouveau confrontées à leur pin original avant
scellement ; `verify_archive` relit ensuite manifeste, inventaire exact,
sommes et provenance. Une erreur conserve les fichiers partiels ainsi que
`EXPORT_FAILURE.json`, qui interdit toute clôture ou reprise en place.
Une destination existante est refusée avant toute publication.

## Commandes et autorité d'exécution

La prévisualisation ne lit pas les captures et ne crée rien :

```text
taskset -c 0 python3 -B build/v7_meb_dual_export/export.py
```

Tests synthétiques permis hors fenêtre de mesure :

```text
taskset -c 0 python3 -B build/v7_meb_dual_export/selftest.py
taskset -c 0 python3 -B -O build/v7_meb_dual_export/selftest.py
```

La publication future exige un GO distinct et le SHA-256 exact du script :

```text
taskset -c 0 python3 -B build/v7_meb_dual_export/export.py --execute --expected-exporter-sha256 HASH_RELU
```

Destination fixe : `morsehgp3D_v7/receipts/meb_dual_geometry_20260905`.
Le script ne possède aucune voie de compilation ou d'appel de moteur ; les
autorités historiques sont chargées comme modules sans appeler leur main.

## Reconstruction et limites

Le README public incorporé au script fournit un outil déclaré de restauration
des 67 fichiers dans un arbre neuf et des commandes C++ reconstructibles.
`restore_sources.py` refuse toute fusion avec un dossier existant, toute
projection de chemin non canonique, tout ELF et tout hash divergent avant
la première écriture. Une panne de système de fichiers peut laisser un
arbre partiel, conservé sans réécriture. Il faut vérifier les sommes de
l'archive avant d'utiliser son manifeste de reconstruction.

La restauration n'exécute aucun script historique. Les chemins originaux
`/workspaces/E-HGP`, les depfiles, les sorties absolues et les pins de runners
sont conservés littéralement. **Les runners historiques ne sont pas
relocalisables et l'archive n'est pas hermétique.** La recompilation manuelle
sur GCC 13.3.0/Linux x86-64 dépend d'un compilateur et de bibliothèques système
non livrés. Elle ne reproduit pas à elle seule la capture historique et n'a
pas été exécutée par l'exporteur. Aucun ELF n'est importé dans le produit.

La géométrie concerne Trace seulement, pas NoObserver. Les prédicats sont
partagés avec F ; il ne s'agit pas d'un oracle géométrique indépendant ni
d'une validation globale q3/q4. `certified` ne signifie pas succès public
quand le plafond legacy refuse. Les comptes A et P ne sont pas des mesures
de temps. Aucun gain, SLO, pipeline complet, résultat massif ou GPU n'est
revendiqué ; l'intégration du double budget dans le produit reste absente.
