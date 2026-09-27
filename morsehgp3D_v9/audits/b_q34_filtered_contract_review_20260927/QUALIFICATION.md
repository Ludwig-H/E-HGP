# Contrelecture de la qualification du raccord résident

27 septembre 2026, base `a7e80d7f9`. Complément indépendant au
[contrat de conception](README.md), sans modification des sources ni
des reçus. Les deux notes précédentes de ce dossier restent inchangées.

## Verdict

**Pas de défaut bloquant trouvé pour poursuivre par une qualification
CUDA isolée.** La capture locale qualifie la couture portable, son ordre
et ses propriétaires ; elle ne qualifie pas encore la nouvelle unité
CUDA, une performance G4, ni la tour FULL. Le moteur est inchangé.

J'ai relu `host.hpp`, `device.hpp`, les implémentations portable/CUDA,
`probe.cpp`, `run.py` et le protocole de session associé. Après clôture,
j'ai exécuté indépendamment les deux lecteurs suivants :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
python3 -B -O morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
```

Les deux retournent le même PASS, vérifient les recettes, sorties, hashes
de sources et binaires vivants. Je n'ai pas recompilé ni relancé les tests
géométriques : cette vérification est une relecture indépendante des
preuves closes, pas une seconde capture favorable.

Source de preuve : [capture r1](../../receipts/q34_filtered_resident_20260927/r1/capture.json),
[gate Release](../../receipts/q34_filtered_resident_20260927/r1/gate_release.stdout)
et [gate sanitizer](../../receipts/q34_filtered_resident_20260927/r1/gate_sanitize.stdout).
Premier essai frais clos, 50 commandes, Release et Clang ASan/UBSan avec
LeakSanitizer activé par le runner. Aucun échec de qualification antérieur
n'est annoncé ni remplacé par cette capture.

## Ce qui est effectivement comparé

Par build, le corpus comporte 85 cas et 255 passages portables : trois
configurations Qr/Q/W par cas, dont W1/W4, Q1, petits découpages et une
capacité demandée supérieure à `INT_MAX`. Cette dernière traite de petits
objets exhaustivement ; elle ne parcourt pas des milliards de paires.

Chaque passage compare au filtre CPU natif les R masques de rectangles,
toutes les survivantes ordonnées avec leurs rangs et masques, P, les
rejets logiques par voie et les visites rectangles. Le plan historique
indépendant du nouveau propriétaire fournit en plus les masses E et le
nombre de plans/fallbacks attendus. Entre configurations, le programme
compare exactement les six compteurs physiques : queries, q3, q4,
rejected3, rejected4 et visits. Les nombres de vagues sont volontairement
différents ; ce ne sont pas des invariants de la géométrie.

Les deux builds publient exactement les mêmes sorties de gate :

| Champ | Valeur par build | Portée |
| --- | ---: | --- |
| `queries` / `survivors` | 75 990 / 29 901 | Cumul des 255 passages du corpus |
| `planned` / `fallbacks` | 189 / 4 233 | Plans et replis réellement traversés |
| `pool_rejected` | 537 | Paires supprimées avant le filtre ponctuel |
| `pool_lane_reduced` | 312 | Réductions de masque dans l'oracle de couverture |
| `fallback_survivors` | 4 041 | Survivantes de cas sans plan |
| `empty` / `zero_output` | 54 / 3 | E=0 ; puis E>0 mais S=0 |
| `source_gaps` / `arena_gaps` | 1 833 / 36 | Indices source/compact/arène effectivement distincts |
| `raw_holes` | 1 833 | Produits fermés avant des compacts vivants |
| `rejections` | 276 | Refus attendus, dont Q=0 répété par passage |

Ces compteurs ne sont pas tous normalisés au même nombre de passages :
notamment `pool_lane_reduced` est collecté une fois par cas dans l'ancien
snapshot, exclusivement au gate. Ce snapshot n'entre ni dans la décision
résidente, ni dans le mode frame. Le corps de la porte d'identité ajoute
un appel valide séparé, hors `portable_runs`.

La petite mesure frame est **artificielle, 12 points**, sans prétention
LiDAR : R=65, R_live=63, P=E=S=63, zéro plan et 63 fallbacks. Elle exerce
les chronos/nettoyages et le lecteur, pas une mesure de gain Pool ou de
croissance. La couverture de l'arène vient du corpus de gate.

## Corrections de couverture avant gel

Une formule de préflight pour compter les réductions de voie utilisait
`(P3-E3)+(P4-E4)-2*(P-E)`. Elle n'était pas sûre : supprimer une paire
mono-voie peut donner −1, puis un sous-dépassement en u64. Le compteur
actuel inspecte directement le masque de chaque paire résiduelle de
l'oracle et le compare au masque de son rectangle. La correction a eu
lieu avant la première compilation de ce gate et avant la capture r1 ;
ce n'est pas un échec exécuté que l'on aurait effacé.

Autres précisions apportées avant gel : comparaison des six compteurs
physiques, publication des masses brutes par voie, fermeture explicite des
propriétaires dans le mur `adapter`, et distinction des compteurs portable
et CUDA. Les réservations POD/segments finales sont présentes dans la
source hachée qualifiée ; elles ne sont pas une optimisation mesurée à part.

## Défauts injectés et limites

Trois mutants Release compilés sont tués par une divergence sémantique
précise, avec code 2 et sans sortie de succès : masques fermés indûment
(`rectangle_masks`), mauvaise base brute (`compact_identity`), suppression
de la vérification d'origine (`refusal_missing`). La porte d'identité
rejette notamment un autre propriétaire de mêmes coordonnées/requêtes,
un autre K, un autre index et la consommation après fermeture.

Les quatre exécutions d'injection sont deux fautes **portables**, chacune
en Release et sanitizer : `resident.rectangle_stack_failure` et
`resident.pair_stack_failure`. Elles établissent l'absence de succès
partiel dans ces chemins ; elles ne simulent pas une vraie erreur CUDA,
une panne d'allocation, ni toutes les exceptions de construction.

La nouvelle source `.cu` est transportée et hachée, mais **non compilée
dans cette capture**. Aucune nouvelle gate TSan propre à ce raccord,
aucun test concurrent de Session, aucun GPU/G4, aucune nouvelle croissance
8k/16k/32k ou coupes capteur n'est acquis. Session reste synchrone et non
concurrente ; l'arène multi-CPU réutilisée reste une dépendance explicite.
Le test d'ordinal >2^32 ne qualifie que les formules de décodage.

## Protocole de la future session : rejugé hors ligne

Le [protocole résident](../b_q34_resident_session_20260927/README.md)
possède son [reçu local distinct](../b_q34_resident_session_20260927/checks/r1/receipt.json).
J'ai relu package/common/worker/session/selftest/run_local, puis exécuté
`run_local.py .../checks/r1 --readback` normal et −O, et réexécuté
`selftest.py` normal et −O. Les quatre commandes passent : 46 positifs,
117 refus, cinq scénarios d'attente/jointure et helper simulé 11/22.
Les deux selftests ont les mêmes sorties et vérifient les hashes avant/après.
Leurs subprocess/GCP sont simulés ; aucune clé ni VM n'a été utilisée.

Les recettes sont liées aux sources et au binaire réellement compilés :
gate CUDA d'abord, puis ng00 complet W4 et W48, mêmes K5/s8/Q/Qr et mêmes
P/E/S/digest exigés. W désigne la préparation d'arène, pas le front mono
ou l'oracle CPU4. Un échec arrête la séquence ; W4 seul ne devient pas une
campagne complète si W48 échoue, même si sa sortie brute reste conservée.
Les fonctions d'attente et de cycle de vie gardées sont comparées par AST
au protocole précédent épinglé. L'arrêt final exige la même génération.

Les chronos extérieurs contrôlés ne somment pas deux fois les phases
imbriquées. Le nouveau mur `adapter` inclut les destructions de Prepared,
Session et décision, mais conserve la sortie et l'amont ; `total` ajoute
notamment l'oracle CPU et n'est donc pas le coût candidat. Les métriques
mémoire par propriétaire/device ne sont pas un pic global ; l'index device
est déjà inclus dans les postes rectangle et paire.

## Identité des pièces relues

SHA-256 des sources et reçus après clôture :

```text
host.hpp       7c3b20dd88d80b5b1722201c8755f576c6004e424a2e15373aeeb2831945f3f0
device_cpu.cpp b1bdace2429499df54202d517c1e85cdb54398d5805c16e3f7410e5d3b6b62d9
device_cuda.cu 1a1e8bdbe2976b5377ed5222e06d49484b570f399cb6f8af4a53c50764758737
probe.cpp      09cf90664bae59668cc2669463e0d8d982ab131eb9b9dd9a1be4b70f57ba548f
run.py         f523fd3056ed15ead8db93b965b465817ce29b3e2c4338183a60a49f507e6f5e
capture.json   268da1049e57fed83a379ecb9d4489d1bda6676f50120e5a0ed133ad28ef9f26
gate Release et sanitizer :
               63169ba5c32e23b05e94acc7ccc3e7b304df219747f65f47ca0cf78613b8ed43
session.py     8bbbe05a85270bb91bdf3caadf0d5145cc131d01dfb66223090e39e6c0eaab54
worker.py      2f3753380a907c2a0a699e9c2e46e45c31773b190d44614a711c35c5dbb1cfff
run_local.py   dfcd7885ca33d268cfe53132122361f0d0c179b68b5add622c0362e3ef8109b5
checks/r1/receipt.json :
               643b400ec29877f095d301fadd6b9279baa213434bb36d77d01ec6950801a5dd
```

Décision suivante : compilation/gate CUDA puis diagnostic G4 de cette
couture, sans activation automatique du moteur. Avant promotion, comparer
au **filtre GPU natif apparié dans le même processus** et publier froid et
mur utile. Battre les 9,898 s de l'ancien prototype CPU ne suffit pas à
battre le filtre GPU natif, encore moins à atteindre 100 ms FULL. Les
propositions suivantes sur la sortie résidente S restent distinctes et
non mesurées : [NEXT_SURVIVORS.md](NEXT_SURVIVORS.md).
