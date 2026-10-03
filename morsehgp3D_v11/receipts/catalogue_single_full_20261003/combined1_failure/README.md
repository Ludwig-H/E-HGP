# COMBINED1_FAILURE — échec conservé, aucune mesure FULL

Source `06d7c44b4d4a52a80a0c00f0fd926b48a92d463c`, session
`/workspaces/.ehgp-sessions/v11.20261003.combined1`.
Le lecteur retourne **code 0 pour la cohérence des preuves, avec `conforming=false`**.
Il n'exécute ni binaire natif ni commande GCP. Aucun résultat de combined3 n'est importé.

| Campagne | Sélection | Réussites | Échecs | Sans résultat fermé |
|---|---:|---:|---:|---:|
| Matrice principale | 2 799 | 2 774 | 25 | 0 |
| Supplément ASan/UBSan u18 | 209 | 205 | 4 | 0 |

Les six configurations fonctionnelles de la matrice et le supplément échouent
chacun sur les quatre mêmes portes Python. La configuration mutants échoue sur
le témoin tower ; style réussit et Clang est explicitement absent. Les huit
constructions concernées portent le statut `build_failed`, même lorsque les
autres exécutables produits permettent aux portes restantes de réussir.

Deux défauts sont isolés dans les sources et journaux épinglés :

- `bench/full_probe.cpp:142` appelle `Catalogue::incidences()`, membre inexistant.
  Chaque `build.log` contient cette seule erreur de compilation. La provenance
  des constructions est complète, mais l'exécutable `mhgp11_full_bench` y est
  absent. `mhgp11_tower_full_bench_io` et sa variante `-O` échouent ensuite par
  `FileNotFoundError` ; leurs scripts Python ont bien été lancés.
- `tests/tower/full_catalogue_collector_test.py:131` exige une égalité exacte
  entre une somme de durées converties en flottants et la conversion de leur
  somme. Ce test de borne échoue sur le Python 3.10.12 distant, avec
  `ValueError: disjoint boundary admitted`, en mode normal et `-O`. L'égalité
  entière en nanosecondes reste la propriété pertinente ; aucune incohérence
  géométrique ne découle de cet échec du test.

Le témoin non muté tower échoue **pendant sa construction** : zéro mutant tower
jugé sur les 78 prévus. Les six autres campagnes ferment 187 mutants :
185 tués par code ou ligne du juge et deux échecs de construction explicitement
attendus dans core. Zéro mutant compté par signal ou délai. Le lecteur vérifie
chaque identifiant contre le manifeste de la source, et chaque cause dans
`LastTest.log`, y compris la concordance avec les sorties JUnit tronquées.

La commande de banc ferme avec code 2 et
`full_parallel_refused: ValueError`, sans fichier de résultat. Le contrôle de
qualification précède le calendrier : **zéro tentative FULL, zéro payload,
zéro décodage réutilisé et zéro mesure**. Les 27 unités prévues par l'option
`--optimized-catalogue` (18 LiDAR, 9 synthétiques) n'ont pas été démarrées ;
`unpersisted=0` ne suggère aucune mesure perdue.

La source corrective `90dd48bd284a960ed687328e1fe2468947039069` remplace l'accès
inexistant par `population().size()` et juge la borne en nanosecondes entières,
avec vérification de chaque conversion. Sa qualification est séparée.
La copie facultative `combined2_local_preflight.json` décrit uniquement le refus
local ultérieur faute d'espace : 1 136 594 944 octets disponibles contre
1 139 263 894 exigés, avant toute mutation GCP. Elle ne remplace aucune preuve
native et n'est pas une tentative de banc.

## Conservation et fermeture

L'archive originale unique `results.tar.gz` contient 616 867 octets, SHA256
`eb4334772b5b91c5cf3f6b48c22dbd5ee57442084b34b559f783d4c03545bb6b` ;
6 609 917 octets de fichiers sont vérifiés après lecture bornée, sans extraction.
Chaque membre, le manifeste interne et l'inventaire complet sont rejugés.
`matrix.json` et `asan18.json` sont les copies octet pour octet des résumés.
Aucun paquet source, nuage KITTI ou binaire n'est recopié. `inputs.json` conserve
les identités des entrées déclarées ; elles n'ont pas été mesurées ici.

La fermeture certifie l'arrêt de la cible
`devpod-gpu-exploration/us-central1-c/ehgp-v7-3b1d496aed430749ea7e049f`,
pour la génération `2026-10-02T19:05:28.755-07:00`, observée ensuite
`TERMINATED`. `DONE=3`, worker terminé en échec, garde invité intacte,
réservation libérée, clé OS Login retirée et clé privée supprimée ; aucune
alerte ni erreur de fermeture. Les commandes d'arrêt et de retrait de clé
ont chacune code 0. Les trois groupes de commandes sont fermés, sans
troncature des flux ni débordement de collecte.

Le lecteur reste **LIVE** : le reçu brut local original, son empreinte et les
objets Git de la source sont obligatoires. Les aides historiques importées
sont vérifiées par empreinte avant chargement. Les empreintes des binaires
rapportées dans la provenance ne sont pas présentées comme recalculées depuis
des binaires archivés. Les anciens lecteurs et captures restent inchangés.
La petite trace `reader_development.json` conserve le premier défaut du
contre-test (espacement du journal) et la correction du compte prévu 19→27.

```sh
PYTHONDONTWRITEBYTECODE=1 python check.py
PYTHONDONTWRITEBYTECODE=1 python -O check.py
PYTHONDONTWRITEBYTECODE=1 python check_selftest.py
PYTHONDONTWRITEBYTECODE=1 python -O check_selftest.py
```

Lecteurs normal/−O cohérents ; contre-tests normal/−O : six positifs et
57 corruptions refusées, zéro exécution native. Voir `checks.json`.
