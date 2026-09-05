# Qualification locale du certificat FULL structurel

5 septembre 2026, base `94a3513b081bd61a8276c3e73e7d91ca5aa42abe`, worktree
modifié et capturé. `authority=structural_only`, `public_status=not_claimed`.
Voir le [contrat](../../docs/CONTRAT_CERTIFICAT_FULL.md).

Deux répertoires neufs, GNU 13.3.0, C++20 avec
`-Wall -Wextra -Wpedantic -Werror` : **2/2 CTests Release et 2/2 CTests
ASan/UBSan réussis**, aucun saut. Le second build utilise Debug, les
sanitizers adresse/comportements indéfinis, détection de fuites et arrêt
au premier diagnostic. Les anciens binaires C et F ne sont pas reconstruits.

| Mode, identique dans les deux builds | Contrôles | Positifs | Refus construction | Refus lecture | Pannes allocation |
| --- | ---: | ---: | ---: | ---: | ---: |
| `--selftest` | 68 | 11 | 0 | 3 | 0 |
| `--rejects` | 218 | 13 | 45 | 19 | 15 |

Chaque mode exige un code de sortie exactement nul et son plancher interne.
Le mode rejets répète les positifs : ces lignes ne comptent pas 286 tests
indépendants. Les refus inspectent le statut, la raison et l'absence de
payload partiel. Le balayage des allocations vérifie que chaque panne
injectée a réellement été atteinte.

Les [commandes et résultats](qualification.json),
[journal Release](release_last_test.log),
[journal ASan/UBSan](sanitized_last_test.log),
[options Release](release_flags.txt), [options sanitizer](sanitized_flags.txt),
[pins des sources et dépendances locales](source_sha256.txt),
[pins binaires](binary_sha256.txt) et
[état du worktree](worktree_at_qualification.txt) sont conservés.
Les dépendances locales correspondent au fichier `.o.d` produit par le
compilateur ; le reçu ne capture pas une image hermétique de la toolchain.
Les fichiers `.o.d` et commandes de liaison sont également capturés :
[dépendances](release_dependencies.d), [liaison Release](release_link.txt),
[liaison sanitizer](sanitized_link.txt). Les [empreintes du paquet](SHA256SUMS)
se vérifient depuis ce répertoire. Les quatre captures de journaux/options
conservent aussi les lignes vides terminales des fichiers de build.

Limites de provenance : les commandes configure/build et leurs codes sont
rapportés depuis les appels d'exécution, mais leurs journaux bruts ne sont
pas archivés. Les pins sources ont été pris après les tests, pas dans un
double inventaire avant/après build. Les sources du module et du gate ont
été figées par la contrelecture avant les builds ; cela ne constitue pas
un moniteur de mutation de tous les fichiers. Les variables des sanitizers
sont dans la commande déclarée du JSON, pas dans le journal CTest lui-même.

Depuis la racine, les sources se contrôlent par :

```bash
sha256sum -c morsehgp3D_v7/receipts/full_certificate_20260905/source_sha256.txt
ctest --test-dir build/v7_full_certificate_20260905 --output-on-failure -R '^mhgp7_full_certificate'
env ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir build/v7_full_certificate_sanitized_20260905 --output-on-failure -R '^mhgp7_full_certificate'
```

Une première compilation directe de l'agent a produit un gate à 216
contrôles, zéro erreur mais 44 refus pour un plancher de 45 : code 3,
donc **essai non conforme conservé**, pas succès produit. Le cas explicite
9/1→18/2 a été ajouté, sans abaisser le plancher. Le binaire de ce premier
essai est conservé localement ; le JSON en épingle le hash et indique que
son résultat provient du runner délégué, sans journal brut complet archivé.

La contrelecture interne a également identifié, avant cette qualification,
le risque d'une affectation copiée partiellement interrompue. Le certificat
est désormais move-only, ses déplacements sont sans allocation et invalident
la source. Ces propriétés sont testées sous panne persistante. Cette
contrelecture C++ interne ne remplace pas celle de l'auditeur externe.

Portée : forêt encodée, coupes et couvertures. Aucun oracle de géométrie
nouveau, aucun constructeur de portails, aucune tour FULL produit,
aucune verticale ou masse, aucun format d'archive ni budget RAM certifié.
Les branches défensives de dépassement de taille irréalisables sur cet
hôte ne sont pas annoncées exercées. La suite complète F historique n'a
pas été relancée ; seules les deux nouvelles portes par build qualifient
ce composant autonome. Ce reçu n'est pas un benchmark. GCP non utilisé.
