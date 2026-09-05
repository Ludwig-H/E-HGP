# Reçus indépendants du 5 septembre 2026

La campagne complète porte sur D, commit source
`e6d33698e62ebecf74dff01c16d7de17149d7a4e`, en CPU de référence,
profil `quantized_u16_input_only`, exploration v7 hors registre et
`public_status=not_claimed`. Aucun résultat GPU ou GCP.

| Contrôle | Résultat et autorité |
| --- | --- |
| Reconstruction Release neuve D | 323/323 portes CPU, zéro échec/skip ; 115 sources et 37 binaires stables avant/après |
| Oracle MEB rationnel D | 89 ensembles, 431 appels, 6 176 puissances ; refus et deux mutants q3/q4 exercés |
| Prétest q2 E postérieur | Même oracle et objets locaux D/E ; mutant q2 détecté, sans suite E complète |
| Index O2 et UBSan | 237 212 nuages par binaire ; sept corruptions structurelles détectées par chaque juge |
| Arrondi D | 40 appels aux quatre modes, un/deux threads ; replis et objets contrôlés |
| Reçus constructeur | Contrelecture des huit XML, sceaux et dépendances Boost ; distincte de la nouvelle exécution |

Voir les [rapports](../README.md) pour les préconditions et planchers de
non-vacuité. Le [résumé Release](release/summary.json) conserve les commandes
exactes, l'état initial du worktree et les durées : construction 247,62 s,
CTest 607,43 s sur hôte partagé. Ces durées ne sont pas des mesures moteur.

La sélection est `ctest --output-on-failure --no-tests=error -L '^gate$'
--parallel 2`, dans le répertoire de construction privé à l'audit.
Les 46 tests d'échelle ne sont pas exécutés. Les stubs GPU sont des tests
CPU ; les sondes MEB/index sous UBSan ne sont pas une suite complète ASan.
Le SHA-256 de la CLI reconstruite est identique à celui publié pour D :
`127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`.

Le [JUnit](release/ctest.junit.xml), l'[inventaire](release/inventory.stdout),
la [sortie CTest](release/ctest.stdout) et le
[journal complet compressé](release/last_test.log.gz) sont conservés.
Le [sceau](release/receipt_manifest.json) couvre chaque fichier de ce
répertoire sauf lui-même. Le journal est compressé sans changer ses octets ;
son hash brut est dans [last_test_raw.json](release/last_test_raw.json).

## Rejouer sans écraser les preuves

Depuis la racine, sur les sources à qualifier :

```bash
MHGP7_AUDIT_RUN_NAME=replay_local_1 PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/replay_release_20260905.py
```

Choisir un nouveau nom pour chaque campagne : le runner refuse des dossiers
de travail ou de reçus préexistants. L'option de nom a été ajoutée après
l'exécution initiale ; les commandes et contrôles par défaut sont inchangés.
Tous les fichiers créés restent dans `audits/`. Un replay sur E ou une
autre révision produit ses propres pins et ne reproduit pas les octets D.

Le constructeur a commencé le prétest q2 E après la clôture du run D.
Les 323 résultats ci-dessus ne qualifient donc pas ce delta. Le manifeste
de fraîcheur conserve les sources D ; le code 1 sur ces fichiers modifiés
dans le worktree est attendu jusqu'à une actualisation explicite.

`tools/check_docs.py` exclut ce dossier. Ses Markdown sont aussi contrôlés
explicitement avec le validateur du dépôt ; le succès du contrôle canonique
seul ne les couvre pas.
