# Resolver statique CPU : qualification causale du prototype

11 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Les routes nominale et statiques 1/4 passent ; six mutants physiques compilent
puis sont réfutés.** Ce reçu qualifie le prototype privé CPU épinglé
`33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209`, pas une
implémentation choisie implicitement dans le worktree courant. La route statique
est destinée à rester opt-in, le défaut nominal inchangé. Le reçu ne constitue
ni une certification globale du producteur WSPD, ni un résultat GPU, ni un
contrat 50k/1 s ou grand nuage.

## Résultats et indépendance

Le [driver C++20 sans Boost](capture/driver.cpp) consomme des census rationnels
figés de six nuages de 2 à 8 points, chacun avec deux remappages d'entrée et de
PointIds. Les [réponses attendues](capture/expected.txt) sont calculées par
l'oracle rationnel Gamma, **pas copiées d'une exécution nominale du moteur**.
Les comparaisons portent sur les chaînes entières représentant naissances,
multifusions et parents, puis couvertures datées et images verticales.

| Route | Ordres | Coupes d'ordres | Contrôles verticaux | Signatures/coupes exactes |
| --- | ---: | ---: | ---: | ---: |
| Nominale (`static_threads=0`) | 46 | 1 544 | 1 506 | 2 524 |
| Statique, 1 lane | 46 | 1 544 | 1 506 | 2 524 |
| Statique, 4 lanes demandées | 46 | 1 544 | 1 506 | 2 524 |

Les deux routes statiques exercent 354 demandes, 219 facettes uniques, 176 hits
de semis et 51 MEB ; le nominal paie 59 MEB. Les répétitions, seeds et MEB
résiduelles ont des planchers exécutables. Aucun gain de temps ni borne
sous-quadratique générale ne découle de ces comptes sur petites fixtures.

L'oracle énumère les facettes sur ces entrées bornées : ce n'est pas une
architecture produit. Les signatures de nœuds sont vérifiées uniques dans les
fixtures ; la couverture seule n'est jamais présumée identifier une composante
en général. Le cercle de niveau 4225 est explicitement présent à Kmax6 mais
inadmissible à K4 dans la fixture `window` ; les nominales la jugent entièrement.
Le mutant d'admission est déjà réfuté sur E5, première fixture : son arrêt n'est
pas une observation du bloc 4225.

Les [18 commandes fermées](capture/commands.json) comprennent sept compilations
`-O1 -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING`, trois
routes nominales, une injection de panne nominale et six exécutions mutantes.
Le define de test permet l'injection de panne du pool ; aucun mutant du registre
n'est activé dans le nominal. Aucun échec de compilation n'est compté comme un
mutant tué. **Ce paquet ne porte pas de run ASan/UBSan** ; les reçus ROOT de
sanitizers sont distincts.

## Six mutations réfutées

| Mutation physique | Compilation / exécution | Première raison exacte |
| --- | --- | --- |
| [Admettre une clé sans sa fenêtre K](capture/mutations/admit_global_key.diff) | 0 / 1 | `tower_status:E5:full_ball_static_closed_anchor_missing` |
| [Consulter les ancres pendant la géométrie](capture/mutations/consult_anchors.diff) | 0 / 1 | `tower_status:E5:full_ball_static_missing_weak_terminal` |
| [Écrire selon l'index trié](capture/mutations/scatter_sorted_index.diff) | 0 / 1 | `tower_status:E5:full_ball_final_component_count` |
| [Oublier les occurrences répétées](capture/mutations/scatter_holes.diff) | 0 / 1 | `tower_status:E5:full_ball_static_target_not_strict` |
| [Ne pas normaliser l'ancre pré-lot](capture/mutations/unnormalized_anchor.diff) | 0 / 1 | `tower_status:E5:full_ball_final_component_count` |
| [Admettre un lancement partiel](capture/mutations/partial_launch.diff) | 0 / 1 | `partial_launch_admitted_geometry` |

Le test de lancement fait échouer la création du deuxième thread. Le nominal
ne publie aucun ordre, ne paie aucune MEB de résolution et joint tous les
workers. Le mutant est réfuté parce qu'il admet effectivement de la géométrie
avant création de tous les workers ; vérifier seulement son statut final
d'échec aurait été insuffisant.

## Contrelecture et corrections r2

La [conception inspectée](review/CONCEPTION.md) et la
[contrelecture initiale](review/INITIAL.md) sont conservées sans changement.
La seconde porte sur l'ancien prototype `c9be538c…`, pas sur r2. Ses trois
remarques de comptabilité sont corrigées dans le
[header r2 réellement compilé](capture/full_ball_tower.hpp.source) :

- `static_lanes_used` distingue la lane séquentielle des threads effectivement
  créés ; le compteur de threads n'augmente que pour plusieurs lanes.
- Les capacités retenues comprennent requests, targets, seeds, groupes, workers
  et piles, avec une somme simultanée. Il ne s'agit pas d'un pic RSS ni d'un
  suivi des doubles buffers pendant réallocation.
- La réduction des stats après jointure conserve aussi les hits de seed en
  échec, sans publier de tour partielle.

Ces corrections ont été relues dans r2. La qualification de mutants ci-dessus
porte exactement sur r2 ; elle n'hérite pas silencieusement d'une exécution
du prototype initial.

## Paquet portable et lecture

Les [pins de provenance](source_pins.json) et le
[manifeste privé original](capture/MANIFEST.json) identifient les 544 fichiers
sources/captures originaux. Le [mapping de stockage](storage_map.json) les
associe à des fichiers lisibles ou à des objets dédupliqués par SHA-256.
Chaque octet original est conservé, y compris les commandes avec leurs chemins
historiques. Les anciens Markdown destinés seulement à la provenance portent
`.source`, afin de ne pas exposer de liens historiques comme documentation
actuelle. Les ELF sont exclus ; leurs empreintes de capture sont conservées.

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/static_resolver_mutants_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/static_resolver_mutants_20260911/verify.py
```

Le lecteur vérifie les sceaux et le mapping, reconstitue les octets vérifiés dans
un répertoire temporaire neuf, puis exécute uniquement le lecteur privé épinglé.
Il ne compile rien, ne lance aucun ELF, oracle, benchmark ou commande réseau,
et ne dépend pas du dossier `build/` d'origine. Le répertoire temporaire est
nettoyé automatiquement. Le paquet publié lui-même reste en lecture seule et
les gardes restent effectives sous `-O`.

La publication de ce reçu ne modifie ni le moteur, ni les fichiers des
auditeurs, ni l'index Git. GCP non utilisé.
