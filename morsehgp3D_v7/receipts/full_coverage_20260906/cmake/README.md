# CMake Release ciblé : couverture, quotient local et certificat v1

Qualification neuve, limitée aux trois cibles demandées. Les commandes de configuration, build séquentiel, inventaire et CTest sont toutes closes avec code0. Aucun fichier produit, CMake, audit ou entrée documentaire modifié ; GCP et Git non utilisés.

Le répertoire de compilation est `build_r1/`, les commandes et bruts sont conservés dans `run_r1/`. Configuration Release avec C++20 et options d'avertissements du projet ; Boost est fourni explicitement depuis `/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include`. Seuls `mhgp7_full_coverage_certificate_gate`, `mhgp7_local_plateau_gate` et `mhgp7_full_certificate_gate` ont été construits.

## Résultats

L'inventaire JSON est vérifié avant l'exécution : exactement les six noms ci-dessous, avec les codes attendus. Le JUnit confirme six tests exécutés, sans échec ni test ignoré.

| CTest | Code attendu du binaire | Résultat |
| --- | ---: | --- |
| `mhgp7_full_coverage_certificate` | 0 | Passed |
| `mhgp7_full_coverage_certificate_bad_argument` | 2 | Passed |
| `mhgp7_local_plateau` | 0 | Passed |
| `mhgp7_local_plateau_bad_argument` | 2 | Passed |
| `mhgp7_full_certificate` | 0 | Passed |
| `mhgp7_full_certificate_rejects` | 0 | Passed |

Le nouveau certificat de couverture rapporte710 contrôles,30 rejets,30 coupes de rejeu,34 coupes Gamma et34 rejets d'allocation. Son autorité reste `structural_only`. Le quotient local conserve18 tables,96 rangs,40 rangs réels,18 ensembles complets de supports Gram,17 raccourcis diamètre et68 rangs DSU. Les témoins v1 restent séparés :68 puis218 contrôles, aucun échec.

Il s'agit de qualification des cibles et de leur branchement CMake, pas d'un benchmark, d'un certificat de complétude géométrique, d'une intégration de la tour FULL non régulière ni d'un contrat50k.

## Traçabilité

Reçu `run_r1/receipt.json` : SHA256 `1db5f2310bfc691721251cb7bea0771b2263abadc6a3d5b151118cc7c5c5f830`.

Recorder `capture.py` : SHA256 `1243da6f3662f67108d7c1f1e392144c5fe0724b1f59297e6d76501b658f7191`.

Les inventaires candidats des sources avant et après sont identiques : SHA256 `942f12c926c34f0be8b8bbc1e3ede5c9045fc88a715a62804878c0356f9abf43`. Les dépendances produit réellement compilées sont extraites des trois fichiers `.d`, reliées à cet inventaire préalable et recontrôlées après les tests. Les empreintes ELF sont stables avant/après les tests. Les sources externes et système sont seulement observées après compilation dans `external_sources_observed_after.json` ; leurs headers ne sont pas recopiés. Les commandes de compilation, flags effectifs, fichiers `.d`, stdout/stderr, PID, codes et dates sont conservés.

Le recorder n'ajoute aucun plafond de temps, CPU, RAM ou fichier. Les deux tests v1 conservent leur timeout60s déjà déclaré dans le CMake produit ; les quatre autres n'ont pas de timeout explicite ajouté. Tous les processus lancés par cette capture sont fermés.
