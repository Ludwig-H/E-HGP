# Journal incrémental : prototype structurel privé

11 septembre 2026. Source de départ : `ad7ffd28b35e153a20bd8cf42534d1cd29160bcd`.
Cadre `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Ce prototype est une collection de journaux structurels partageant une
banque, non raccordée au producteur FULL.** Il ne certifie ni la géométrie,
ni la complétude, ni une tour de K consécutifs, ni les cartes verticales.
Aucune mesure de performance/RSS, aucun contrat 50k ou GPU n'est revendiqué.
GCP non utilisé ; les headers actifs, les audits et Git n'ont pas été modifiés
par ce travail. Les erreurs de montage des juges et le premier échec SAN
restent dans ce paquet, sans être comptés comme des défauts nominaux du moteur.

## Delta privé et qualifications

Le [patch non appliqué](delta.patch) factorise les gardes whole-lot dans
[le journal privé](sources/full_coverage_certificate.hpp). La façade actuelle
conserve son API, son précomptage et ses réservations exactes.
[L'assembleur privé](sources/full_coverage_incremental.hpp) emprunte les vues
parents/contributions d'un lot complet ; il écrit les arènes sans conserver
ces vues ou leurs buffers. Un propriétaire append-only interne donne leurs
identifiants aux populations et fournit seul la banque immuable au scellement.
Une erreur invalide toute la collection, y compris ses ordres déjà terminés.

| Qualification close | Résultat O2 et ASan/UBSan |
| --- | --- |
| Façade baseline contre façade factorisée | Sorties identiques : 823 contrôles, 30 rejets, 30 coupes de rejeu, 40 coupes Gamma, 20 pannes d'allocation |
| Fixtures de l'auditeur du journal | Sorties identiques : 42 contrôles, 10 cas valides, 9 rejets, 6 lectures, 5/5 pannes d'allocation |
| [Gate incrémentale](sources/incremental_gate.cpp) | 491 contrôles, 14 rejets, toutes les 49 allocations de la construction mixte refusées séparément |
| [Comparaison directe et stabilité des coupes](sources/paired_baseline_gate.cpp) | 104 cas, 17 436 contrôles ; égalité physique de 2 991 nœuds, 2 480 parents et 4 595 contributions |
| Extensions du journal de l'auditeur | 60 anciennes coupes stables ; six écritures de successeurs exactement ABSENT vers une nouvelle multifusion |
| Mutants, O2 seulement | Quatre compilations réussies, puis quatre refus causaux : continuation restaurée trop tôt, propriétaire ignoré, réservation à chaque lot, parent écrit 0 |

Les 104 cas directs comprennent 100 journaux structurels déterministes pour
K=1..10 et quatre préfixes de `structural_mixed` ; leurs domaines comptent au
plus 16 points. Les tableaux sont comparés champ par champ, sans assimiler
les capacités ou les octets de padding au contenu. Les préfixes de la gate
métamorphique sont des constructions distinctes entièrement scellées : aucune
API ne publie un préfixe pending. `successors` n'est pas append-only ; seules
ses cases ABSENT nouvellement consommées sont renseignées.

La porte contre `reserve(size+delta)` observe 44 allocations des arènes pour
1 024 lots de naissance, avec un plafond de test de 48 dans le STL testé.
C'est une régression algorithmique ciblée, **pas un gain de temps ou de RSS**.
L'accounting de la voie privée est `amortized_vector_growth_v1`, distinct des
réservations exactes de la façade actuelle. Le coût amorti O(N+P+C) porte sur
les tailles de sortie, pas sur une borne sous-quadratique universelle en n.

## Échecs conservés et replay SAN

| Capture d'origine | Statut conservé |
| --- | --- |
| `o2_r1` | Runner incorrect : oubli de `--selftest`, code 2 ; runner et gate originaux conservés |
| `o2_r2`, `o2_r3` | PASS ; r3 renforce seulement les attentes causales du juge incrémental |
| `san_r1` | LeakSanitizer échoue sous ptrace après compilation du baseline |
| `san_r2` | PASS hors bac à sable, autorisation explicite et `detect_leaks=1` maintenu |
| `additional_r1` | Montage du namespace baseline incomplet face à la déduplication `#pragma once` de GCC |
| `additional_r2` | Affectation/comparaison erronée de tableaux C dans le juge, rejetée par compilation stricte |
| `additional_r3` | Comparaison directe et métamorphique PASS O2/SAN, puis quatre mutants causaux O2 |

Les deux headers privés n'ont pas changé entre leurs qualifications closes.
La première tentative manuelle de compilation sans le chemin Boost a échoué
avant ces captures ; elle n'est pas présentée comme une commande enregistrée
par leur runner. Le rejeu utilise le Boost déjà extrait localement, sans
installation ni ajout de ses fichiers au paquet.

## Paquet portable compact et lecture

[Le manifeste de capture original](capture_manifest.json) conserve les 2 840
entrées logiques scellées, soit 43 711 888 octets textuels avant déduplication.
Il omettait les extensions `.cu`, `.cuh` et `.txt`, pourtant épinglées par les
captures de sources avant/après. Le premier lecteur portable a détecté cette
lacune ; ses échecs normal et `-O`, son code et son manifeste sont conservés
sous `reader_review/`. [closure_sources.json](closure_sources.json) complète
126 chemins logiques, représentant neuf contenus uniques, **uniquement depuis
les SHA déjà enregistrés dans les reçus originaux**. Le lecteur vérifie ce lien
de provenance ; aucun reçu, source historique ou manifeste original n'est réécrit.
Le paquet stocke chaque contenu une seule fois :
[storage_map.json](storage_map.json) associe chaque chemin logique à son fichier
physique, sa taille et son SHA-256. Les sources principales et le patch restent
directement lisibles ; les autres sources, logs, reçus, runners et captures
échouées sont dans `objects/`, avec extension `.source`. Il n'y a ni ELF, ni
archive vendor, ni copie du toolkit. Le mapping conserve les octets historiques,
sans réécrire les reçus ou leurs chemins de compilation d'origine.

[Le lecteur](verify.py) vérifie le manifeste portable et le mapping, reconstitue
la capture dans un répertoire temporaire dédié, puis appelle le lecteur
original épinglé. Il ne compile rien et ne lance aucun moteur ou appel GCP.
Il reste effectif sous Python `-O`. Le SHA du manifeste portable est le sceau
à conserver en dehors de ce paquet.

```bash
python3 -B morsehgp3D_v7/receipts/incremental_journal_prototype_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/incremental_journal_prototype_20260911/verify.py
```

Avant toute promotion : raccorder en privé `close_lot` et les populations
paresseuses, repasser les 28 nuages Gram/Gamma, les plateaux groupés et les
verticales, puis seulement mesurer les allocations et la résidence du
producteur complet. Les arènes finales résident plus tôt : la suppression des
brouillons ne se convertit pas automatiquement en économie de pic mémoire.
