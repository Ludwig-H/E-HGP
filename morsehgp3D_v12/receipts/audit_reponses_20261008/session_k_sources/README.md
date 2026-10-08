# Session K : concordance du paquet source avec c9

Complément du 8 octobre 2026 à [la provenance K](../session_k_provenance/README.md),
déjà publiée en `039b2657e`. Ce reçu ne la modifie pas. Aucun moteur, compilation,
jeu de coordonnées ou service distant n'a été exécuté ou lu.

**Les 342 fichiers du périmètre ci-dessous sont identiques octet pour octet aux
blobs Git `c9ac60f20c741d9d493f4900fd8aec4590aaa5c5`.** Aucun fichier manquant,
supplémentaire ou modifié ; 2 435 921 octets comparés. Le périmètre inclut tous
les fichiers des répertoires, y compris leurs lecteurs Python et configurations,
pas seulement une sélection d'extensions compilables.

| Périmètre dans `morsehgp3D_v12/` | Fichiers |
| --- | ---: |
| `src/` | 128 |
| `bench/` | 41 |
| `tests/` | 168 |
| `cmake/` | 4 |
| `CMakeLists.txt` | 1 |

Paquet K : SHA256 `aa75b3162d783d6889a8d23fd7c439e39e089a962fb8e2f25704602fe847f3db`.
Manifeste dérivé, chemins triés, lignes `SHA256  chemin\n` :
`752316d452c96ff26fc3bb5d3cd4b53528a08a8e834c8b8a7632a556ce0bf128`.
Le lecteur reconstruit ce manifeste et compare directement les octets de chaque
blob ; les 342 empreintes ne sont pas dupliquées dans le dépôt. Les noms présents
sont également comparés dans les deux sens, donc un ajout ou retrait est refusé.

Les 34 entrées d'état local du snapshot ne cachent donc aucune modification dans
ce périmètre. Les pilotes FULL/D6 hors de celui-ci restent épinglés séparément
par [le reçu avant résultats](../session_k_snapshot/README.md). Ce résultat
précise le contenu expédié ; il ne change ni le grade `dev_snapshot`, ni l'absence
de hashes binaires, CMakeCache et journaux des constructions internes. Il ne
constitue pas une nouvelle qualification des tests ou de toute la chaîne de
compilation/exécution.

```sh
python -B check.py --repo DEPOT --package PAQUET_SOURCE_K_TAR_GZ
python -B -O check.py --repo DEPOT --package PAQUET_SOURCE_K_TAR_GZ
```

Normal et `-O` rendent le même résultat. Le hash du paquet est vérifié avant et
après lecture ; aucun checkout ni fichier produit n'est modifié.
