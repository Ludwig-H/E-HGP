# Coût local MEB à deux budgets : captures historiques distinctes

L'export est clos ; cela ne transforme pas le premier build en succès.
`runs/failed_v1` conserve l'échec de compilation rc1 (macro objet main).
`runs/build_v2` conserve le build corrigé, sans mesure à ce stade.
`runs/measure_v2` conserve la seule campagne autorisée ensuite : deux chauffes,
sept passages, 9 347 jobs, 9 351 états, 58 491 appels top-level par bras/passage.
Les égalités F/Trace/NoObserver avant/après et les captures chronométrées ont
été rejugées depuis les bruts ; pas de nouvelle compilation ni mesure à l'export.
Aucune future campagne de répétitions différentes n'est incluse.

Deux fichiers seulement sont compressés : `disassembly.stdout.gz` et
`measurement.stdout.gz`. Gzip level9, mtime0, filename vide ; aucun changement
de ligne. `provenance.json` distingue hashes/tailles bruts et compressés.
La décompression a été confrontée byte-exactement au brut d'origine et à
son pin. Tous les autres fichiers copiés sont byte-exacts.

La note `review/disassembly_review_root.md.txt` est la lecture root préalable
du désassemblage, pas une preuve d'isolation du matériel. Le temps comprend
resets, enveloppes, copies, barrières et captures : ce n'est pas le helper nu
ni le CLI Release. Des lots sont trop courts selon le seuil du protocole ;
tous les temps et strates restent archivés, sans sélection d'un meilleur P.
Aucun gain global, SLO 50k/1 s/100 ms, résultat massif ou GPU n'est revendiqué.

La qualification géométrique Trace est référencée dans l'archive sœur
`../meb_dual_geometry_20260905` (manifeste 571b565e, sommes 2abbc213 ; pins
complets dans validation.json), sans copier à nouveau ses 161 fichiers.
La qualification native NoObserver appartient à la capture coût v2,
pas rétroactivement à cette géométrie historique. `public_status=not_claimed`.

## Reconstruction manuelle, non hermétique

Les 73 snapshots inertes `source_tree/**/*.txt` conservent une fermeture
locale autonome des sources F, prototypes, corpus et harnais v1/v2.
Après contrôle de SHA256SUMS, l'outil déclaré restaure ces sources dans
un répertoire entièrement neuf, sans exécuter les runners :

```text
python3 -B /absolute/archive/exporter/restore_sources.py.txt --archive /absolute/archive --destination /absolute/fresh-source-tree
```

Depuis ce nouvel arbre, commandes manuelles reconstructibles v2 :

```text
mkdir manual_build
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -fno-lto -I morsehgp3D_v7 build/v7_meb_dual_budget_cost_v2/cost_harness.cpp -o manual_build/cost_harness
objdump -d -C manual_build/cost_harness
```

Le compilateur historique est GCC 13.3.0 sur Linux x86-64. Compilateur,
objdump, CLI F protégé et binaire du harnais ne sont pas distribués ; leurs
pins figurent dans excluded_artifacts.json. Les bibliothèques système ne
sont pas livrées. Cette recompilation manuelle n'a pas été exécutée par
l'exporteur et ne reproduit pas la capture historique.

Les runners/depfiles/reçus gardent leurs chemins absolus `/workspaces/E-HGP`,
sorties et pins originaux : ils ne sont PAS relocalisables ni hermétiques.
Une nouvelle mesure exige un protocole réadmis, une revue du nouveau binaire
et un GO distinct ; un appel brut du harnais n'est pas une capture certifiée.
Les sorties gzip se lisent par décompression standard sans conversion LF.

`manifest.json` inventorie le contenu hors lui-même et SHA256SUMS ; les
sommes couvrent tout, manifeste inclus, hors elles-mêmes. EXPORT_FAILURE.json,
inventaire divergent ou sommes invalides interdisent toute clôture.
Le double budget n'est pas intégré au produit. GCP non utilisé.
