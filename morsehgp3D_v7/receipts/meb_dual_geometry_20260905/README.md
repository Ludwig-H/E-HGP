# MEB à deux budgets : deux qualifications locales archivées

Cette archive distingue `runs/tiny` (contre-fixture de budget) et
`runs/geometry` (différentiel local instrumenté). Les deux reçus terminaux
et tous leurs bruts sont copiés byte-exactement, sans recompilation ni
réexécution pendant l'export. Aucun ELF n'est distribué. `public_status=not_claimed`.

La contre-fixture tiny croise P = 0/1/4/5 avec L = 1/4 sur le triangle,
puis teste quatre appels cumulatifs et une frontière proche de UINT64_MAX.
Elle produit zéro violation nominale contre 28 pour le mutant causal.

La géométrie comprend 1 507 ordinaux, 176 scènes, 384 ordres et 9 339
comparaisons : 6 047 succès, 160 refus shell et 3 132 refus de budget.
Les certificats rapides nommés q2/q3/q4 sont 8/16/52. Le mutant charge-après
conserve les autres champs mais donne 46 437 violations = 46 431 formes
des comparaisons + 6 formes directes ; le nominal en donne zéro.
`certified` n'est pas un nombre de succès publics. Les appels F du juge
ne sont pas le coût A des seuls replis. Ces nombres ne sont pas des gains
de temps ni des résultats sur une tour ou sur G4.

`provenance.json` donne chaque origine absolue, projection et hash ;
`excluded_artifacts.json` référence les quatre ELF absents (compilateur,
CLI F protégé et deux exécutables locaux). `validation.json` rapporte une
relecture des juges épinglés, pas de nouveaux tests. Les préparations et
leurs résultats négatifs restent historiques et inchangés. `SHA256SUMS`
couvre le contenu et le manifeste, hors lui-même. Un `EXPORT_FAILURE.json`
ou des sommes non conformes interdit de traiter l'archive comme fermée.

## Restaurer les sources, puis recompiler manuellement

Les 67 fichiers sous `source_tree/` ont leur extension d'origine suivie de
`.txt` et sont inertes. Ils incluent la fermeture locale des headers F,
les prototypes, les deux gates et leurs données, pas seulement un header
isolé. L'outil déclaré `exporter/restore_sources.py.txt`, exécutable en le
passant à Python, vérifie `reconstruction_manifest.json` puis restaure
les noms/arborescences dans une destination qui doit être entièrement neuve :

```text
python3 -B /absolute/archive/exporter/restore_sources.py.txt --archive /absolute/archive --destination /absolute/fresh-source-tree
```

Cet outil ne compile ni ne lance rien. Après restauration, commandes
manuelles reconstructibles depuis `/absolute/fresh-source-tree` :

```text
mkdir manual_build
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -I morsehgp3D_v7 build/v7_meb_dual_budget_prototype/dual_budget_gate.cpp -o manual_build/tiny
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -I morsehgp3D_v7 build/v7_meb_dual_budget_geometry/geometry_gate.cpp -o manual_build/geometry
taskset -c 0 manual_build/tiny
taskset -c 0 manual_build/tiny --mutant=charge-after
taskset -c 0 manual_build/tiny --unknown
taskset -c 0 manual_build/tiny --mutant=charge-after extra
taskset -c 0 manual_build/geometry
taskset -c 0 manual_build/geometry --mutant=charge-after
taskset -c 0 manual_build/geometry --unknown
taskset -c 0 manual_build/geometry --mutant=charge-after extra
```

Codes attendus : nominal 0, mutant causal 4, argument inconnu 2. Il faut
capturer les sorties et les codes ; un exit 4 quelconque n'est pas une preuve.
Le compilateur historique est GCC 13.3.0 sur Linux x86-64, avec les
dépendances système du poste original. Le compilateur, les bibliothèques
système et les binaires ne sont pas livrés : ceci n'est pas une archive
hermétique et cette compilation manuelle n'a pas été exécutée par l'export.

Les runners, depfiles et reçus gardent leurs chemins absolus originaux sous
`/workspaces/E-HGP` et leurs pins de capture. Ils ne sont PAS relocalisables
tels quels : restauration des sources et recompilation manuelle ne valent
pas reproduction de la capture historique. Modifier un runner pour une
autre machine exige un nouveau protocole et de nouveaux reçus.

La portée géométrique est l'instanciation Trace instrumentée, pas
NoObserver. Les prédicats partagés avec F ne sont pas un oracle indépendant.
Aucune certification globale, qualification de pipeline, SLO 50k/1 s/100 ms,
accélération industrielle, mesure massive ou GPU n'en découle. GCP non utilisé.
