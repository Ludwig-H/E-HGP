# Raccord MEB filtré dans FULL : preuves propres au produit

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Les [résultats](../../docs/RESULTATS_MEB_FULL_20260906.md) et le
[contrat](../../docs/CONTRAT_MEB_FULL.md) bornent l'autorité : composant
FULL horizontal relatif aux catalogues fournis, P=0 par défaut, F inchangé.
Aucun résultat CLI, archive, verticale, performance de tour ou GPU.

| Capture | Résultat | SHA-256 du `run.json` |
| --- | --- | --- |
| [CMake R3](core/run.json) | 30/30 Release et 30/30 ASan/UBSan ; 120 sources stables, dix binaires par build | `ca7c3a35b2ef8a80f202b1fe0c3dedc32f409f15604e700c5c3fb6b06b53071c` |
| [Contre-F local et oracle rationnel](extra/run.json) | 21 commandes ; 59 frontières, 9 344 comparaisons, 3 430 appels rationnels et 1 507 ordinaux par build O2/SAN | `a709d26382b55820a6ee268e0aec49098f659869ed75419ba82241ee2f456ac2` |
| [Mutations et exceptions](mutations/run.json) | 15 commandes ; quatre mutants O2 réfutés, douze injections tardives FULL par build O2/SAN | `dbbe577e7ca392b680844c098b5a4fb122122ea2ccb10cd3629694af4c71044f` |

La dernière clôture C++ est `2026-09-06T09:36:43.022417+00:00`.
Les scripts de publication/relecture ne relancent aucun moteur.
Les [six relectures initiales normal/-O](checks/) ont des sorties identiques
par paire. Les corpus se recouvrent ; ne pas additionner leurs nuages comme
s'ils étaient indépendants, ni confondre compilation et performance.

## Contenu et incidents

`core/` contient les commandes CMake, leurs sorties et pins, les sources,
les configurations et dépendances de compilation. Les dépendances système
sont identifiées après capture, pas prétendues hermétiques. Huit binaires
par build utilisent le code produit ; les deux tests historiques singleton
et successeurs gardent `MHGP7_TESTING`.

`extra/` conserve les sources effectivement compilées et les entrées/sorties
du juge rationnel indépendant. Les nouveaux contrôles A sont exécutés dans
les deux gates adaptées ; le protocole rationnel garde ses quatre anciens
champs Work. Les fonctions rationnelles copiées sont des témoins distincts
du chemin produit, jamais importées dans les headers C++.

`mutations/` conserve cinq copies explicites, les sites exacts de mutation
et les captures. Les quatre variantes fautives rendent le code 1 pour
la première cause attendue. La cinquième change seulement `before_form`
du `NoObserver` copié pour injecter les exceptions : elle ne prétend pas
que le `NoObserver` nominal puisse lever, ni interrompre l'arithmétique F.
Les fichiers auxiliaires de ces captures sont liés par le sceau final
et les lecteurs ; leur stabilité n'était pas toute revendiquée par le
seul runner initial.

`failed_attempts/` garde les deux tentatives closes : configuration sans
chemin Boost, puis build SAN limité à 64 Mio par fichier temporaire.
R3 adapte seulement la garde de compilation à 512 Mio (exécutions 64 Mio),
sur sources et plafonds HGP inchangés. Aucun fichier en échec n'est écrasé.

Aucun ELF n'est distribué. Les empreintes des binaires capturés restent
liées aux commandes ; les lecteurs portables ne les ouvrent pas et ne
prétendent pas les requalifier. Le `--judge` du contrôleur privé extra
exige encore son chemin original et ses ELF : utiliser les lecteurs
ci-dessous pour cette publication, pas ce mode privé.

## Relecture portable, sans compilation

Depuis la racine du dépôt, refaire ces commandes normalement et avec `-O` :

```bash
python3 -B morsehgp3D_v7/receipts/full_meb_product_20260906/protocol/verify_cmake.py morsehgp3D_v7/receipts/full_meb_product_20260906/core
python3 -B morsehgp3D_v7/receipts/full_meb_product_20260906/protocol/verify_extra.py morsehgp3D_v7/receipts/full_meb_product_20260906/extra
python3 -B morsehgp3D_v7/receipts/full_meb_product_20260906/protocol/verify_mutations.py morsehgp3D_v7/receipts/full_meb_product_20260906/mutations
```

Les lecteurs extra/mutations épinglent les reçus sources ; celui de CMake
vérifie les 30 terminaux et les résumés sémantiques par build. Le sceau
`SHA256SUMS` lie l'ensemble publié, y compris les échecs et les lecteurs.
Les artefacts de construction restent hors dépôt ; les sources et commandes
capturées permettent une nouvelle compilation dans un répertoire neuf,
qui constituera alors un autre reçu.
