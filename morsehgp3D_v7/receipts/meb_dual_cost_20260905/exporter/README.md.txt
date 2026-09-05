# Export coût privé : préparation seule

Destination future : `morsehgp3D_v7/receipts/meb_dual_cost_20260905`.
Pas de publication avant GO distinct avec pin exact de `export.py` ; aucun
compilateur, moteur, Git ou GCP n'est lancé par cet exporteur.

Les trois seules captures admises sont le build v1 failed `247c952c`, le
build v2 completed `de6de29f` et la mesure v2 completed `874f100f`.
Tous les pins complets sont littéraux dans le script. Le code relu du runner
v2 `8b9ae71e` est réutilisé sans modification pour les captures fermées,
les commandes, la géométrie, le build, les dépendances et le jugement des
temps. Le runner v1 ne diffère que par le pin C++ ; son préflight source
est conservé. L'échec v1 nécessite un juge explicite séparé : status failed,
compile returncode 1 mais expected_rc historique 0, erreur terminale exacte,
aucun binaire ni désassemblage. Il ne passe jamais par une admission completed.

## Inventaire borné et conservation

- 56 copies de captures : 17 pour failed v1, 25 pour build v2 hors ELF, 14 pour mesure v2.
- 73 snapshots sources inertes : union cohérente de 76 pins, moins compilateur/objdump/CLI F.
- 22 copies complémentaires : préparations et tous bruts selftests v1/v2, sommes, delta v2 et note root de désassemblage.
- 6 snapshots d'exporteur : export, selftests, README, préparation, helper fermé de l'export géométrique et outil de restauration fermé.
- 7 documents générés : README, provenance, exclusions, validation, manifeste de reconstruction, manifeste de clôture, SHA256SUMS.

Soit **151 artefacts d'entrée + 6 snapshots = 157 copies**, et **164 fichiers**
au total. Le binaire coût est la quatrième exclusion ELF. L'archive géométrique
est référencée par ses pins publics `571b565e`/`2abbc213` et relue, pas dupliquée.
Aucun dossier de campagne future, notamment un éventuel changement de répétitions,
n'est découvert ou intégré par scan récursif.

Deux transformations seulement : `measurement.stdout` (28 972 744 octets) et
`disassembly.stdout` (2 248 162 octets) deviennent `.gz` par gzip level9,
mtime 0, filename vide. Chaque provenance distingue SHA/taille du brut et
SHA/taille de l'archive. La décompression doit égaler exactement les octets
originaux, avant/après copie et au scellement ; aucun LF n'est réécrit.
Tous les autres fichiers sont copiés byte-exactement. La note root est
épinglée séparément à `964d5ea703be17335e58307e6c22176778c12053f87273745cff6610890a6dd6`.

Relectures terminales : sources F et privées, trois captures, juges,
provenances, décompressions, inventaire exact, manifeste et sommes.
Une erreur après création laisse les artefacts et EXPORT_FAILURE.json,
sans reprise ni promotion. Une destination existante est refusée.

## Préparation et exécution future

```text
taskset -c 0 python3 -B build/v7_meb_dual_cost_export/export.py
taskset -c 0 python3 -B build/v7_meb_dual_cost_export/selftest.py
taskset -c 0 python3 -B -O build/v7_meb_dual_cost_export/selftest.py
```

Ces mocks interdisent tous les points d'entrée subprocess ; seuls des
fichiers synthétiques temporaires sont créés. Les portes ne dépendent pas
d'instructions Python `assert` supprimées sous `-O`.

Commande future, après lecture et GO explicite seulement :

```text
taskset -c 0 python3 -B build/v7_meb_dual_cost_export/export.py --execute --expected-exporter-sha256 HASH_RELU
```

## Portée

Une fermeture d'export n'est pas une campagne uniformément passée : le
failed v1 reste un échec de compilation, sans mesure. Les préparations
historiques gardent leur statut antérieur non exécuté. La mesure native
NoObserver v2 reste distincte de la géométrie instrumentée Trace.
Le jugement conserve tous les lots courts, strates, chauffes et passages,
sans sélectionner un meilleur plafond P et sans soustraire les captures.
Les comptes physiques ne sont pas convertis en gain de temps supposé.

Les 73 snapshots permettent une reconstruction locale des sources dans
un arbre neuf via l'outil déclaré, après contrôle des sommes de l'archive.
Les runners historiques restent absolus et non relocalisables ; compilateur,
objdump, bibliothèques système et binaires sont absents. La recompilation
manuelle proposée n'a pas été exécutée et n'est pas une reproduction
hermétique de la capture. Une mesure supplémentaire exige un nouveau GO.
Pas d'intégration produit, de claim SLO/tour, de résultat massif ni GPU.
