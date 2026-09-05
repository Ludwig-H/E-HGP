# Observations F seules — 16 000 et 32 000 points

Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference,
profile=quantized_u16_input_only, mode=bounded_F_scale_observation, public_status=not_claimed.

Deux observations indépendantes de F, uniforme u16 étendu coord=65536, seed=3, s=8,
tour candidate complétée K1..10 demandée, digest et incidences complètes, sans archive.
CPU6, un thread demandé, fold-inflight=1 et fold-join=1. Le runner ne mesure pas les créations de threads.

| n | État moteur | Processus achevé (s) | Pipeline achevé (ms) | RSS achevé (KiB) |
|---|---|---:|---:|---:|
| 16000 | engine_completed | 413.816374070 | 413790.2 | 5361880 |
| 32000 | engine_refused | — | — | — |

Une observation close n'est pas nécessairement un succès moteur. Refus reconnus et censures
conservent leurs sorties exactes, sans digests de succès, coûts de tour achevée ni RSS inventé.
Un fichier GNU time absent ou incomplet sous refus/censure est déclaré tel quel ; aucun octet n'est fabriqué.
Les durées brutes des tentatives restent des diagnostics, pas des temps de complétion.

Chaque processus est borné à 600 s et RLIMIT_AS=26 GiB, qui n'est pas un plafond de RSS physique.
Proxy payload partiel=16 GiB ; caps par ordre : core8M, chain2M, cofaces2M, queries1B, MEB1B.
Hôte partagé, une observation froide par taille : aucun ratio, moyenne avec les paires 8k,
gain statistique, comparaison de digests entre tailles ou extrapolation vers 50k et au-delà.

Le préflight et la frontière finale relisent les sources, helpers, CLI/build F et historiques C/C/D/E.
Toutes les sorties et tous les champs de runs.json sont reclassifiés par les autorités épinglées ;
aucun moteur, build, test, Git ou GCP n'est exécuté par l'exporteur. Un changement de HEAD documentaire
reste enregistré ; il ne remplace pas le contrôle strict des snapshots de sources et binaires.

Les copies sont byte-exactes, sans LF ajouté, réécriture ni troncature. provenance.json donne chaque
origine, destination, hash et taille. SOURCE_HASHES.json désigne les noms privés, pas leur projection.
Les snapshots de protocoles sont inertes, les dépendances historiques référencées avec leurs pins.
La préparation conserve les huit tests synthétiques normal/-O ; elle n'est pas un résultat moteur.
build_F/build_D.json conserve un nom hérité du builder mais son contenu est F. Aucun binaire n'est livré.

SHA256SUMS et SHA256SUMS.root ferment les mêmes fichiers et s'excluent mutuellement.
Un arbre partiel dépourvu des deux listes valides n'est pas publiable et n'est jamais repris ou écrasé.
Aucun SLO 50k/1 s/100 ms, résultat massif/GPU ou exactitude industrielle globale n'est démontré.
GCP non utilisé.
