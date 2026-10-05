# Preuve G4 partielle — 5 octobre 2026

`phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Chaque configuration ci-dessous conserve son profil exécuté propre.

Source exécutée : `00bd979ac2c24aff422421525d484488dff0c993`.
Les deux sessions `v11.20261005.claudequalmatrice` et `v11.20261005.claudequalA`
ont une fermeture ciblée certifiée et des archives locales dont les empreintes
correspondent aux reçus. Leurs matrices restent non conformes : le contrôleur
a interrompu les tests au délai global, avec `worker_exit_code=1`.
`matrix.complete=true` signifie que le résumé est finalisé, pas que toutes les
portes ont réussi. Aucun contrat de temps de la tour n'est acquis ici.

| Session | Configuration | Passed / sélectionnés | Sans résultat |
| --- | --- | ---: | ---: |
| matrice | gcc_release, u18 | 929 / 941 | 12 |
| matrice | mutants | 33 / 33 | 0 |
| matrice | gcc_asan_ubsan, u24 | 751 / 834 | 83 |
| matrice | gcc_tsan, u21 | 747 / 834 | 87 |
| matrice | bits21 | 757 / 834 | 77 |
| matrice | bits24 | 757 / 834 | 77 |
| matrice | poison, u21 | 758 / 835 | 77 |
| A | gcc_release, u18 | 929 / 941 | 12 |
| A | bits21 | 822 / 834 | 12 |
| A | bits24 | 822 / 834 | 12 |
| A | poison, u21 | 823 / 835 | 12 |

La première session donne aussi style 2/2 ; clang++ absent, configuration
facultative. Aucun résultat individuel Failed n'est consigné. Les douze portes
sans résultat sont identiques dans les quatre profils A, dont scale32000 K10,
LiDAR ng00 K10 et les six portes supports scale/LiDAR sous Python `-O`.
Leurs noms exacts figurent dans `summary.json`.

Dans chacun des quatre profils A, les **27 portes `api_*`, 24 portes `io_*`
et `cli_contract` normal/`-O` ont un verdict Passed explicite**. Le rejeu lit les
lignes CTest terminées, les rapproche de l'inventaire et des compteurs du
résultat ; il ne déduit pas un succès de l'absence de failure. Les verdicts et
durées individuels sont conservés dans `critical_verdicts`. En particulier :

- `mhgp11_api_session_session_identity` couvre refus de la mauvaise Session
  avant fichiers/rapport, puis publication par la Session déplacée
  (`tests/api/publish_test.cpp:231`, source ci-dessus) ;
- `mhgp11_api_session_provenance` et `mhgp11_api_publish_reader` normal/`-O`
  couvrent les refus de provenance et l'aller-retour au lecteur officiel ;
- `mhgp11_cli_contract` normal/`-O` inclut les pannes du crochet I/O dans les
  profils A sans sanitizer (`tests/cli/tests.cmake:25`, `cli_contract.py:400`).
  Son préchargement limite explicitement la lecture variadique à renameat2.

Les onze verdicts mutants numériques exacts donnent **459/459 tués** :
457 par code de retour attendu et deux refus de construction prévus dans core,
zéro signal et zéro délai. Supports 15/15, API 22/22, CLI 28/28. Les catégories
par module figurent dans `summary.json`, les lignes originales dans le manifeste.

`manifest.json` contient les empreintes des reçus, paquets, plans et archives,
les seuls champs sûrs sélectionnés des reçus/résumés et les huit plans exacts
figés le 5 octobre à 14:46:50 UTC, avant mutation ultérieure. Aucun nom de compte,
clé, nuage ou log brut n'est recopié. Le plan de mesure figé porte b319efc84 et
adopte les contrôles W48 ng02/ng00 (`--fils 1,4,48`) ; leur exécution n'est pas
prouvée par ces deux archives. La session A2 active est explicitement exclue.

Rejeu depuis n'importe quel répertoire :

```sh
python3 /chemin/de/cette/capsule/replay.py
python3 -O /chemin/de/cette/capsule/replay.py
```

Le rejeu est stdlib, en lecture seule et sans réseau, build ou appel natif.
Il dépend des deux archives, reçus, paquets et plans locaux aux chemins épinglés
dans le manifeste, sous `/workspaces/.ehgp-sessions/`. Cette capsule est une
contre-lecture vérifiable de ces preuves locales, **pas un reçu autonome**.
[Identité des sources](source_identity.json) : produit, portes, références,
bancs et CMake sont inchangés entre 00bd979ac et b319efc84 ; ce dernier
sépare les labels `long` dans la matrice, sans nouvelle qualification.
`SHA256SUMS` ferme les fichiers de la capsule. Les étapes de copie et
de fermeture des empreintes figurent dans [packaged_checks.json](packaged_checks.json).
