# Gardes par rangs du catalogue — raccord CPU qualifié

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Raccord privé, moteur actif inchangé. GCP non utilisé.

## Changement qualifié

À partir du [parent CPU persistant](../parallel_birth_streaming_20260911/README.md),
trois gardes répétées utilisent les rangs déjà certifiés de l'Atlas :
semis strictement antérieur, premier consommateur non postérieur, puis
terminale admise à K et strictement antérieure. Bornes et admissions
précèdent les lectures. K1 garde les indices de points ; la branche K1
du helper terminal est testée séparément, mais n'est pas appelée par le
producteur. La [note de portée](../../docs/GARDES_RANGS_CERTIFIES_20260911.md)
distingue cette optimisation d'une suppression de contrôles géométriques.

Tri et validation exacts des rangs, comparateur du programme, MEB
initiales/intermédiaires et niveau brut transmis au resolver restent
inchangés. La géométrie, les groupes, le pool CPU, la réduction dense et
l'export FULL sont ceux du parent. Même propriétaire/census/index/Atlas
immuables et complets sont des prémisses, pas une garantie d'une simple
référence const. Ce paquet ne certifie pas la complétude générale WSPD.

Les compteurs par K sont `first_consumer_rank_checks=R`,
`initial_seed_rank_checks=Sw`, et `terminal_rank_checks=R` si K≥2,
zéro à K1. Sw compte les réponses par semis initiaux dans les fenêtres,
pas les semis stockés. Ils sont distincts des gardes du réducteur natif.
Ils décrivent les succès complets, pas le travail récupérable avant échec.
Le nombre de MEB payé ne diminue pas avec ce delta.

## Gates propres O2 et ASan/UBSan/LSan

Chaque gate FULL ferme **30 commandes**, résultats O2/SAN identiques :
114 census, 912 essais, 506 448 terminales directement comparées,
48 775 524 contrôles. Fenêtres1/7/31/4096, workers1/4, s8/10/12,
réindexages, coquilles et plateaux sont exercés. Forêts, contributions,
verticales, φ, certificats datés et travail géométrique sont comparés
aux références ; aucun tampon global de terminales ne fournit le résultat.

Tous les anciens champs scientifiques du parent sont égaux. Les 4 368
contrôles supplémentaires vérifient les trois compteurs de rang par ordre.
Totaux : 506 448 consommateurs, 297 162 semis, 484 224 terminales,
soit **1 287 834 gardes**. Le mutant supplémentaire de grand-livre est
refusé avec la cause attendue. Les autres mutations et refus, dont l'échec
après géométrie parallèle et la quiescence des workers, restent exercés.

La gate ciblée ferme **15 commandes par build**, résultats O2/SAN
byte-identiques : 18 vrais census n2/n4/n5, 36 Atlas originaux/variantes
rationnelles, 2 796 couples de boules dont 48 égaux mais distincts,
12 882 contrôles, 24 rejets de liaisons/rangs et 36 refus de débordement.
Les fractions sont comparées par produits croisés Boost indépendants.
Les 126 liaisons comparées entre catalogues ne sont pas autant de fractions
modifiées : une boule sur deux reçoit la représentation doublée.

Sept mutations réelles sont réfutées : égalité du semis, égalité de la
terminale, ordre consommateur inversé ou rendu strict, mauvais domaine
terminal, admission omise, point K1 traité comme boule. Chacune vérifie
d'abord le comportement nominal, puis exécute une faute et retrouve sa
cause attendue, code4. Unknown/missing refusent avec code2 et sorties vides.
Ces petits census ne sont pas un nouvel oracle de complétude géométrique
ni une mesure de tour. Aucun verdict TSan supplémentaire.

## Mesures

Uniforme u16/seed3, s8, K1..10, W65536. Les quatre grands processus sont
successifs, après fermeture des compilations/tests ROOT. D'autres sondes
utilisateur ont chevauché le mono et les premiers runs CPU4 ; l'hôte reste
partagé. Les comparaisons au parent sont historiques, non appariées.

| n | Threads amont/géométrie/consultations | Tour entière (s) | Atlas/géométrie/réduction (s) | RSS (KiB) | Gardes désormais entières |
| ---: | --- | ---: | ---: | ---: | ---: |
| 8 000 | 1/1/1 | 206,224207575 | 62,453518709 | 2 756 956 | 25 907 749 |
| 8 000 | 4/4/4 | 96,401057050 | 32,134279819 | 2 874 208 | 25 907 749 |
| 16 000 | 4/4/4 | 223,446536141 | 73,197374312 | 5 807 080 | 55 773 201 |
| 32 000 | 4/4/4 | 478,615724962 | 158,710478073 | 11 607 552 | 117 563 898 |

Le parent mono donnait 187,214349752 s, CPU4 92,963380155 /
215,380860626 / 489,601175356 s. À 32k la phase modifiée baisse de
171,384085700 à 158,710478073 s, mais aucun gain de latence reproductible
n'est établi par ces captures. L'amont non modifié varie aussi :
36,715945818→40,329536534 s à 8k CPU4 et
83,198347895→91,647889321 s à 16k. Le remplacement de prédicats est
qualifié, pas une accélération universelle de toute la tour.

Mêmes sorties et travail géométrique que les parents correspondants :
MEB 4 359 540 / 9 364 101 / 19 784 213 et nœuds FULL 3 976 472 /
8 310 399 / 17 166 975 à 8k/16k/32k. Les digests et comptes par ordre
sont vérifiés par le lecteur. Les gardes restent linéaires en occurrences
et réponses par semis ; ce n'est pas une borne linéaire en points.
Par doublement, R croît par 2,099 puis 2,071, MEB par 2,148 puis 2,113,
les temps CPU4 par 2,318 puis 2,142. Uniforme/s8 seulement, pas tous régimes.
À 32k, histoires 50,467434670 s et export 92,043522223 s restent payés.

Le temps entier inclut index, génération, census, validation, création/
jonction des workers, reconstruction, export FULL en mémoire et libérations
déclarées. Synthèse des entrées, digests diagnostiques et comparaison
physique sont exclus et chronométrés séparément. Aucune archive industrielle
n'est produite par cette sonde ; le RSS externe couvre tout le processus.
Capacités nommées et pics de phases ne s'additionnent pas en un pic global.

Les trois comparaisons physiques n800/s8/10/12 sont closes, paramètres
W65536, K1..10 et 4/4/4. Mêmes sorties FULL et travail géométrique entre
séparations et avec le parent. Elles ont chevauché les compilations de
qualification et d'autres sondes utilisateur ; pas d'optimum temporel
déduit pour s.

## Sources et reproduction

Douze captures réussies, 102 commandes : deux gates FULL, deux gates
ciblées, un build de sonde, trois micros et quatre grands runs. Les CTests
du moteur actif sont inchangés et n'ont pas été rejoués pour ce delta privé.

Pins : helper `81a32d8c…`, raccord `0e50808c…`, gate FULL `b88a0f5a…`,
sonde `805c610e…`, gate ciblée `089ddcd3…`.
Le recorder FULL/sonde `dbfe778c…` reste inchangé pour toute la campagne ;
le recorder ciblé `46314016…` est distinct. La gate FULL garde
`reducer=parallel_geometry_dense_birth_one_dsu` ; la sonde sépare
`reducer=dense_birth_one_dsu`, `geometry_backend=persistent_window_workers`
et `guard_backend=certified_catalogue_ranks`. Sa référence demeure
séquentielle avec gardes rationnelles.

Parent unique : manifeste
`759fcc21ef945803efa54c82f48b1368ec15dc95f715ad2131f140cb280688e0`.
Les sources communes sont empruntées par nom et hash, sans transfert de
qualification. Sources minimales consommées, commandes, dépendances
-M/-MD, compilateur et ELF avant/après sont liés. Les binaires sont omis
du dépôt avec leurs pins conservés ; ce n'est pas une capture complète du
sysroot de lien/runtime. Chaque capture garde son propre recorder.

Le lecteur recontrôle les fermetures et les résultats bruts O2/SAN, sépare
gate FULL et gate ciblée, puis compare au parent toutes les campagnes
effectivement appariées. Temps, RSS, répartition dynamique et scratch des
workers ne sont pas des égalités exigées ; travail géométrique et sortie
restent comparés. Aucun échec absent n'est inventé, aucun test absent n'est
présumé passé.

```bash
python3 -B morsehgp3D_v7/receipts/rank_guard_streaming_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/rank_guard_streaming_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/rank_guard_streaming_20260911/verify.py --extract build/rank_guard_replay_fresh
```

L'extraction est explicite et create-only, sans exécution implicite.
Les deux recorders extraits utilisent chacun un nouvel `--out` ; le
recordeur ciblé accepte `--san`, sans option `--kind`. Boost doit déjà être
disponible au chemin déclaré, aucune installation automatique.

Ni contrat 50k sous 1 s/100 ms, ni dizaines de millions de points sur G4,
ni borne sous-quadratique tous régimes ne sont acquis par ce paquet.
