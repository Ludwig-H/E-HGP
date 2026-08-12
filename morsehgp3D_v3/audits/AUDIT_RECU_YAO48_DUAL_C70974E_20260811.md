# Audit du reçu d'échelle de la traversée duale q2

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict historique

La traversée duale persistante réduit fortement le résiduel et le coût du
classifieur, mais sa gate de travail reste **NO-GO avant G4**. Sur chacune des
trois familles structurées complètes, `dual_witness_visits` possède deux
exposants successifs strictement supérieurs à `1,35`. Le titre du commit
`c70974e`, « the output gate turns green », décrit seulement les compteurs de
sortie/classification; il ne ferme pas la gate de la source entière.

Ce résultat est néanmoins un progrès réel : survivantes, tests de boîtes et
tests ponctuels du classifieur passent sous la porte sur les trois familles.
Le verrou mesuré s'est déplacé vers la frontière témoin ambiguë. Ni CUDA, ni
G4, ni un payload consommable, ni le SLO officiel ne sont reçus.

## Pincement et provenance

| objet | SHA-256 |
| --- | --- |
| [`dual_scale_counters_raw.txt`](../receipts/yao48_dual_20260811/dual_scale_counters_raw.txt) | `a19ac56290e3262f9f1fc9b05e37952688f3a26db1f80fb989325a53292ce1b1` |
| [`dual_exponents_derived.txt`](../receipts/yao48_dual_20260811/dual_exponents_derived.txt), fichier combiné par le commit `3f2111f` | `d173160efafade5994e2c3faa2ef1fee33c93f128405cfdfc139deb0b5592b01` |
| `prototype/pair_yao48_source.cpp` | `b8d2d46c0e61ff9f75489f0468299b02fdafe6a30dd33c837762f6af18be9be2` |
| `prototype/yao48_source.hpp` | `a7d0a8ea30607028a6b7e1a12550917d0fa499082938fc05cb412aefcb80aa30` |
| binaire Release annoncé par la session | `0fce8ec7c91152d2b6b1bb4ca6e8401f2081528bbbcc42c61987a7a49260b071` |

La première section du dérivé, seule présente à `c70974e`, avait le SHA-256
`264dd91eeb96a4243558e3f84322fe1db7d004e1d3e15156ee0af5e973c8b349`.
Le commit `3f2111f` a ajouté la matrice v2 au même fichier : cet ancien hash ne
désigne plus le fichier courant.

La campagne séquentielle possède trois triplets complets
`12 500/25 000/50 000` (`terrain`, `scanline_single_pass`,
`scanline_overlap_multiecho`) et un seul cas `uniform,12 500`; les dix cas
présents rendent `rc=0` et ferment leur ledger. `uniform,25 000` ne porte
qu'un en-tête, sans sortie ni code, et `uniform,50 000` manque : aucune pente
uniforme n'est recevable.

La provenance n'est pas intrinsèque. Chaque ligne imprime faussement
`--bank-mode exact`, alors que les compteurs de banques sont nuls et les
compteurs duals non nuls; le mode dual provient du transcript de session et le
hash du frozen ELF a été mesuré extérieurement, pas embarqué dans le reçu. Les
fichiers n'embarquent ni `HEAD`, ni hashes source, compilateur, hôte, intervalle
de temps ou identifiant de run. Les secondes sont exclues : cette rampe a
coexisté avec une campagne P1a CPU lourde, des builds et des tests concurrents.

## Recalcul indépendant de la gate

Les exposants sont `log2(C(2n)/C(n))`. Les valeurs dérivées publiées sont
arithmétiquement correctes à deux décimales :

| famille | visites témoins 12,5/25/50 k | exposants | survivantes, exposants | boîtes classifieur, exposants | tests ponctuels, exposants |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain | 122 022 307 / 344 733 574 / 1 312 614 530 | 1,498 / 1,929 | 1,051 / 1,032 | 1,100 / 1,164 | 1,060 / 1,049 |
| scanline simple | 66 988 581 / 205 682 770 / 656 009 769 | 1,618 / 1,673 | 1,040 / 1,032 | 1,159 / 1,135 | 1,068 / 1,071 |
| multiecho | 101 055 157 / 333 444 048 / 1 112 715 133 | 1,722 / 1,739 | 1,116 / 1,107 | 1,245 / 1,197 | 1,175 / 1,168 |

À 50 k, la coupe duale remplace `99,54--99,72 %` des paires selon la famille
structurée et le census reste proche du linéaire. Mais elle visite encore
`656 millions--1,313 milliard` de nœuds témoins. Une masse couverte presque
quadratique n'est pas un compteur de travail; les visites le sont, et elles
refusent la route mesurée.

## Matrice v2 produite par `7e34a4a`, ajoutée au dépôt à `3f2111f`

Le second journal
[`dual_scale_v2_counters_raw.txt`](../receipts/yao48_dual_20260811/dual_scale_v2_counters_raw.txt),
SHA-256 `e79a7a1cee5b83a114c39332d5e56e4451b41d04eeca0908b23c0de75e7e592a`,
provient du binaire figé
`f92ad675499d63e83fbe8cd978dac5a82e5290167aec640739fc2be45729b4e4`.
Il pince `pair_yao48_source.cpp` à
`b8d2d46c0e61ff9f75489f0468299b02fdafe6a30dd33c837762f6af18be9be2` et
`yao48_source.hpp` à
`e0600b7c9d17d4bc2876692e5d47c821b0b6bb7b8c7392f86da31ee6ebbc5c18`,
soit le snapshot `7e34a4a` qui exploite ponctuellement les feuilles ambiguës.
Trois triplets structurés sont complets et `uniform` ne contient que
`12 500`.

Les visites témoins restent rouges deux fois : `1,50/1,92` sur `terrain`,
`1,69/1,63` sur la scanline simple et `1,69/1,70` sur multiecho. Les sorties et
compteurs du classifieur imprimés passent. Mais le nouveau
`dual_point_tests` n'est pas affiché dans le journal; il n'entre pas davantage
dans `work_done`, la fusion ou l'égalité des agrégats. Il est donc faux de
conclure que **seul** le travail témoin est rouge : c'est le seul compteur
rouge parmi ceux publiés, et la gate de travail complète demeure indécidable.
Le NO-GO reste déjà acquis par `dual_witness_visits`.

## Limite d'autorité

Ce document reçoit uniquement les journaux historiques ci-dessus. Il ne décrit
ni le cap, ni l'effacement de frontière, ni les reçus, ni les défauts du code
courant. Leur autorité est
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md), complétée par l'audit pincé de
la route sous une seconde. Modifier un fichier source ne réécrit pas ce reçu;
cela crée un nouveau candidat à auditer et à mesurer.

GCP non utilisé pour cet audit.
