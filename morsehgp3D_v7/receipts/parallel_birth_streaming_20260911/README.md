# Géométrie par fenêtres sur équipe CPU persistante

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Le moteur actif n'est pas remplacé. GCP non utilisé.

## Raccord qualifié

À partir de la [réduction dense mono](../birth_streaming_20260911/README.md),
le producteur groupe les facettes complètes dans chaque fenêtre. Les semis
complets répondent directement ; les groupes non semés sont distribués à
une équipe de workers persistante pour toute la tour. Scratch et compteurs
sont privés, géométrie/index/semis immuables. Après quiescence, les résultats
sont dispersés puis consommés dans l'ordre original par le réducteur natif.
Les workers ne lisent ni φ ni l'union-find. Une seule fenêtre est en cours.

O2 et ASan/UBSan/LSan passent chacun **29 commandes**, sorties identiques :
114 census, 912 essais, 506 448 terminales directement comparées et
48 771 156 contrôles. Sont exercés W1/7/31/4096, workers1/4, réindexages,
s8/10/12, plateaux, FULL/contributions/verticales et travail géométrique.
Les 456 essais à quatre workers paient 52 524 tâches géométriques.
Ces tâches ne sont pas autant de MEB : une descente peut en payer plusieurs.

Les 19 causes comprennent les mutants d'identité/compteurs, le refus de
zéro worker et un refus **après vraie géométrie K≥2**. Quatre workers
achèvent chacun au moins une résolution avant la barrière du test, puis
un callback injecté lève une exception. Quiescence et threads joints sont
vérifiés avant le catch. Un producteur neuf est comparé sur φ, certificats
datés et travail à la voie ordonnée. Ce n'est ni un nouvel export FULL
post-échec, ni une reprise d'objet partiel, ni un compte exact du travail
payé avant l'exception. La gate autonome du pool et son échec de démarrage
TSan restent dans le paquet parent, sans transfert de qualification de race.

La [note d'architecture](../../docs/CONTRACTION_NAISSANCES_ET_WORKERS_20260911.md)
distingue ces garanties de celles du census complet immuable. La preuve
de complétude générale du producteur WSPD n'est pas fournie par cette gate.

## Comparaison des configurations à 8 000 points

Uniforme u16/seed3, s8, K1..10, W65536. Les trois processus ci-dessous
sont successifs, sans autre compilation ou benchmark ROOT concurrent.
Hôte virtuel partagé AMD EPYC 9V74, huit CPU logiques exposés ; aucune
exclusivité matérielle ni qualification statistique de speedup revendiquée.
L'équipe persiste pendant la géométrie, pas entre processus.

| Threads amont / géométrie / consultations | Tour entière (s) | Atlas/géométrie/réduction (s) | Histoires (s) | Export (s) | RSS (KiB) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 / 1 / 1 | 187,214349752 | 58,649085565 | 7,731458310 | 16,184593413 | 2 756 768 |
| 1 / 4 / 1 | 164,702915024 | 34,976048990 | 8,052506604 | 16,649185751 | 2 757 048 |
| 4 / 4 / 4 | 92,963380155 | 32,986995649 | 7,717746988 | 15,500843690 | 2 874 428 |

Même digest dense
`a19a83fa4d646e4e0505ba2968ed9120fe8cc80aacccbf0fec61e53ad09b331f`,
mêmes 3 976 472 nœuds, 10 456 312 occurrences, 4 359 540 MEB et
3 127 185 tâches géométriques pour 164 fenêtres. Les quatre workers du
deuxième run paient respectivement 786 996 / 786 878 / 790 469 / 762 842
tâches : la répartition est une observation, pas une identité déterministe.
Le lecteur compare les totaux géométriques et le travail logique entre
un/quatre workers à autres contrôles constants, sans imposer la répartition.

Le gain observé de la seule géométrie parallèle est limité par les opérations
encore séquentielles de cette phase : atlas, tri, scatter, réduction.
Le paramètre consultations ne parallélise pas la construction des histoires.
L'amont existant profite déjà de quatre threads ; ce résultat n'est pas
attribué au seul nouveau pool. Validation, reconstruction et export restent
dans le temps total, ainsi que les créations/jonctions et libérations.
Synthèse/digests/comparaison sont chronométrés séparément, hors contrat.

## Triplet 8k/16k/32k à quatre threads

Les trois tailles sont closes, avec 4/4/4 et les mêmes paramètres ci-dessus.
Même nombre de MEB, mêmes digests et comptes FULL que les témoins ordonnés
respectifs, sans attribuer leurs anciens temps à ces nouveaux exécutables.

| n | Tour entière (s) | Occurrences R | Tâches géométriques | MEB | Nœuds FULL | RSS (KiB) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 92,963380155 | 10 456 312 | 3 127 185 | 4 359 540 | 3 976 472 | 2 874 428 |
| 16 000 | 215,380860626 | 21 948 186 | 6 762 501 | 9 364 101 | 8 310 399 | 5 803 964 |
| 32 000 | 489,601175356 | 45 453 599 | 14 308 225 | 19 784 213 | 17 166 975 | 11 608 968 |

Par doublement, R augmente par facteurs 2,099 puis 2,071 ; les MEB par
2,148 puis 2,113. Les temps augmentent par 2,317 puis 2,273. Ce sont
trois observations sur uniforme/s8, pas une borne sous-quadratique tous
régimes. Le nombre de nœuds et la mémoire de sortie restent à compter.
À 32k, atlas/géométrie/réduction prennent 171,384085700 s, construction
des histoires 50,145306644 s et export 91,704433127 s. La validation
commune seule prend 44,206991671 s. Ces postes restent distincts ; les
consultations parallèles ne rendent pas les préparations gratuites.

Les microcomparaisons physiques n800/s8/10/12 utilisent également 4/4/4.
Elles vérifient la tour et les comptes géométriques, sans choix d'optimum
chronométrique pour s. Elles suivent le triplet et chevauchent seulement
une petite reconstruction du test autonome du pool, pas les grands runs.

## Mémoire et protocole

Le run 1/4/1 mesure 5 505 032 octets de capacité pour les cinq vecteurs
de fenêtre, sentinelle des groupes incluse. Les quatre WorkerState ont
4 320 octets de capacité, hors scratch. Le scratch retenu est échantillonné
après quiescence ; ce n'est pas son maximum interne pendant une descente.
Runtime des threads et piles ne sont pas mesurés par ces capacités.
Le RSS externe comprend toutes les allocations du processus, sortie incluse.
Les pics et capacités nommés ne s'additionnent pas en un pic global.

Les modes `compare` font une comparaison physique linéaire dans le même
processus. Les grands processus séparés sont comparés par digest dense,
comptes FULL et travail logique, pas par un comparateur linéaire entre
leurs mémoires. Les refus CLI n'ont ni payload ni temps de complétion.

Le parent unique est le paquet dense mono, manifeste `e613ff0f…`.
Le paquet contient quinze captures réussies et 75 commandes : deux gates,
un build de sonde, neuf mesures et trois refus CLI. L'échec TSan est
conservé dans le parent, pas recompté comme capture locale de ce paquet.
Chaque capture conserve son propre recorder : `b02ea3d2…` pour O2 gate
(benchmark volontairement non admis à cette étape), `c736438f…` pour
SAN et benchmark. Le C++ du raccord est le même dans ces deux gates.
La gate nomme son raccord `parallel_geometry_dense_birth_one_dsu` ; la
sonde sépare `reducer=dense_birth_one_dsu` et le backend géométrique
`persistent_window_workers`. Les deux identités ne sont pas confondues.

Sources minimales et snapshots, compilateur, dépendances -M/-MD, ELF,
arguments et sorties sont liés par hash. Les exécutables sont omis du
dépôt, sans effacer leurs pins ; aucune installation automatique.
Ce protocole ne fige pas tout le sysroot de lien/runtime.

```bash
python3 -B morsehgp3D_v7/receipts/parallel_birth_streaming_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/parallel_birth_streaming_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/parallel_birth_streaming_20260911/verify.py --extract build/parallel_birth_replay_fresh
```

L'extraction est create-only, sans exécution implicite. Le recorder extrait
reconstruit avec un nouveau `--out`; la gate utilise le Boost déjà disponible
au chemin déclaré. Ni GPU ni contrat 50k sous 1 s/100 ms, ni dizaines de
millions de points sur G4 ne sont qualifiés par ce paquet CPU.
