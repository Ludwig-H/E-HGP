# Résolution statique CPU : comparaison s8, s10 et s12 à n8000

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Aucune exécution CUDA, aucun contrat 1 s, 100 ms ou massif.

## Résultat clos

Uniforme u16, n8000, graine 3, coordonnées 65536, toute la tour K1..10
et ses verticales retenues ; **un thread amont et quatre threads statiques**.
Le facteur s de la WSPD, seul argument changé, vaut 8, 10 ou 12.
s8 est copié après fermeture depuis la campagne d'échelle ROOT ; s10 et
s12 sont exécutés ici en séquence. Leurs captures n'écrasent pas s8.

Le même binaire privé figé
`f71f31190f5d50ac70f8332f969c6baa50549536bd08836702e6ba01ae7fcdce`
consomme le header
`33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209`.
Sources et binaire sont épinglés avant/après chaque run. Les sources
correspondent aussi aux pins avant/après de sa compilation originale.
La sonde privée reste distincte de la sonde active, malgré le header commun.

L'entrée est
`b73744755477b18a5853084851075bb4e3e468ae7d1353c98d0991576a099639` ;
le payload complet vaut pour les trois facteurs
`cdd77e308f765d643ce6707ba0f613f94b4cf560124c0079ee8bfa58ca0c329b`.
Les 32 champs comparés explicitement d'entrée, statut, sortie et calendrier
coïncident. Cela couvre notamment 3 976 472 nœuds, 3 976 462 parents,
2 404 646 contributions et 3 960 473 références verticales. Ce n'est pas
un nouvel oracle géométrique à 8k ni un certificat général de complétude WSPD.

Les neuf lignes statiques K2..10 sont identiques terme à terme pour les
trois facteurs : R=10 396 562, U=5 176 885, semis uniques=2 396 646.
K1 est exclu des compteurs `static_orders`, pas de la tour livrée.
MEB=4 185 184 et supports testés=364 590 166, identiques pour les trois s.
Les 307 936 444 octets statiques sont des capacités retenues échantillonnées,
pas un pic de réallocation ni le RSS du processus.

| s | Candidats raw = unique | Boules après census | Total observé (s) | FULL observé (s) | RSS maximal (KiB) | CPU obtenu |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 3 144 017 | 3 113 381 | 466,761 | 150,412 | 2 137 980 | 44 % |
| 10 | 3 129 992 | 3 113 381 | 136,375 | 50,652 | 2 137 860 | 116 % |
| 12 | 3 123 497 | 3 113 381 | 169,109 | 54,033 | 2 137 952 | 106 % |

Les champs raw/unique amont n'ont pas été présumés invariants : ils baissent
de 14 025 puis 20 520 par rapport à s8. Hors temps et s lui-même, ce sont
les **seuls champs du JSON qui diffèrent**. Aucun optimum temporel de s
n'est annoncé : captures uniques sur hôte partagé, charge très différente,
chevauchement possible avec d'autres campagnes locales. La baisse du nombre
de candidats ne mesure pas tout le travail de génération par blocs WSPD.
Les temps incluent toute la tour et le digest ; `time -v` conserve séparément
temps CPU, temps externe et RSS. Aucun plafond algorithmique ni timeout
automatique n'a été ajouté. Les observations de progression sont prises
environ toutes les trente secondes.

## Paquet portable et lecture

`manifest.json` associe chaque chemin logique à un objet SHA-256 sous
`objects/`. Le paquet conserve sources figées, commandes de compilation
originales, captures brutes, progression, pins avant/après, copie close s8,
comparaisons et scripts de reproduction. Aucun ELF ni vendor Boost inclus.
Le manifeste CPU original et le paquet Boost 1.83 sont identifiés dans
`reproduction/` ; ses versions système sont des observations post-run,
pas une stabilité rétroactive inventée de l'environnement de compilation.
Une première collecte de métadonnées a cherché `manifest.json` au lieu de
`logical_manifest.json` : script et transcription de l'échec conservés,
sans effet sur les captures scientifiques ; `finish_metadata.py` clôt la suite.

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/static_s_factors_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/static_s_factors_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/static_s_factors_20260911/reader_cases.py
python3 -B -O morsehgp3D_v7/receipts/static_s_factors_20260911/reader_cases.py
```

Le lecteur ne lance aucun binaire. Il recalcule hashes, paires, domaines
des compteurs, non-vacuité, identité MEB initiales+descentes, fermeture des
dix ordres et captures de mémoire. Ses six injections à la frontière JSON
sont des tests du **lecteur seulement**, pas de nouveaux mutants du moteur
C++ : payload modifié, promotion du contrat, ordres vidés, MEB annulées,
capture non close et comparaison mensongère. Elles sont rejetées aussi
sous `python3 -O`. Les gates O2/SAN et mutants C++ du header restent celles
du [paquet CPU séparé](../static_resolution_cpu_20260911/README.md).

Pour recompiler, matérialiser les fichiers logiques `source/`, reprendre
la commande originale de `original_micro_commands.json` en remappant ses
chemins et l'include Boost 1.83, puis lancer les trois commandes conservées.
Toute recompilation constitue un nouveau binaire à épingler ; elle n'est
pas présentée comme une reproduction bit-à-bit acquise. La reconstruction
et les mesures ne sont jamais lancées par le lecteur.
