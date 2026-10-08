# FULLN : petits nuages et priorité du catalogue CPU

Contre-extraction ciblée sur les JSONL publics du produit R1 `8a0716e74`, K5.
Aucun moteur/cloud/payload ; admission complète séparée par evidence_1704.
19 journaux et sept sources sont épinglés. Le manifeste public de 159 cas
fournit les noms, groupes et tailles ; il ne contient aucune coordonnée.

## Petits réels : 20 nuages, chacun comparé à lui-même

La Session régulière a 147 nuages (132 réels + 15 synthétiques sains), trois
tours. **20 réels de 100 à 140 sites** satisfont ≤150 ; les deux dernières
visites sont retenues, soit 40 passes par configuration. Une seule Session
par configuration, même ordre des nuages : pas d'expérience causale W4/W48.

| Configuration | Médiane des 20 médianes chaudes |
| --- | ---: |
| CPU, 4 fils | 6,118968 ms |
| CPU, 48 fils | 9,999080 ms |
| Catalogue GPU, 4 fils | 4,114733 ms |
| Catalogue GPU, 48 fils | 4,262123 ms |

Appariement par nuage : CPU48 est plus lent que CPU4 sur **20/20**, écart
médian +3,792973 ms ; GPU48 est plus lent que GPU4 sur **19/20**, écart
médian +0,160623 ms. À fils égaux, GPU est plus rapide sur ces 20 réels.
Les noms et les quatre mesures de chaque nuage sont conservés dans results.

CPU48, moyennes sur les 40 passes : FULL 9,889215 ms, C 9,056825 ms ; dans
C, parcours 3,013484, finition 2,653597, feuilles 2,413915, émission 0,962541.
Entre CPU4 et CPU48, la différence moyenne appariée du mur est +3,726053 ms,
dont C +3,597636 : parcours +2,088111 et finition +1,873353, feuilles
−0,374197. Ces différences sont calculées sur les mêmes nuages et visites,
pas par addition de médianes ; elles localisent l'écart sans en prouver la
cause (synchronisation, granularité, mémoire ou autre).

**Première piste petits réels : instrumenter parcours et finition CPU** et
juger ensuite une variation de granularité sur ce même appariement. Pas de
seuil de fils acquis ; ne pas remplacer C1, ajustement sur les 132 réels,
par ce sous-ensemble ni extrapoler ces observations à zéro site.

## ng00–02 CPU : le comptage des feuilles en premier

Sur les mêmes 36 passes chaudes que le reçu FULLN temps, feuilles est le
plus gros compteur C dans **36/36** cas. Moyennes ng00/01/02 :
**154,792 / 125,494 / 146,725 ms** ; parts des sommes feuilles/C :
**51,803 / 49,237 / 48,592 %** (feuilles/FULL : 43,644 / 41,950 / 40,709 %).
Le reste identifiable : parcours 66,931/57,463/70,387 ms, finition
53,532/48,467/57,062 ms, émission 23,467/23,415/27,697 ms.

Source `leaves.cpp` : ce timer enveloppe `parallel_for(count_body)` ; il
inclut ce travail et son exécution par le Pool, pas seulement un prédicat.
Le JSONL ne publie pas les nombres de feuilles/candidats/replis : il ne
permet pas d'attribuer ce temps à une cause interne précise. Priorité de
microbanc : cette phase de comptage, avec identité et mur FULL comme portes.

Le Pool est synchrone ; les lots sont consommés successivement. Le catalogue
soustrait ces chronos de comptage et d'émission de la durée du parcours,
puis effectue sa finition : feuilles est un mur inclus dans C, pas une somme
de temps des fils.

Sa disparition totale, **tous les autres coûts de chaque passe fixés**,
laisserait `wall - feuilles` au-dessus de 100 ms dans **36/36** cas : médianes
ng00/01/02 **199,702967 / 173,288277 / 214,271473 ms** ; minima
198,432989 / 171,797088 / 209,550099 ms. C seul resterait au moins
142,774310 / 127,999619 / 151,647629 ms. La piste census ne peut donc être
présentée comme suffisante dans ce modèle ; une vraie modification peut
aussi déplacer d'autres coûts, à mesurer séparément.

## Ne pas mélanger les familles

À ≤150 sites s'ajoutent trois sains de 100 sites (uniform, clusters8, slab)
et trois difficiles de 100 sites, séparés dans les résultats. Les difficiles
ont deux passes par processus, une seule chaude, 48 fils seulement. Leurs
murs CPU/GPU sont lattice 11,091/9,219, line 2,358/2,557, sphere 25,134/19,921
ms. Les refus des sphères plus grandes n'appartiennent pas à ce sous-ensemble.

Le synthétique uniform100 GPU48 atteint 25,518 ms, dont émission 22,235 ms,
contre environ 4 ms sur les petits réels. La taille seule ne justifie donc
pas de choisir un chemin unique ; ce compteur ne sépare pas les causes de
l'émission. Ni ces six cas ni les configurations ne sont regroupés en une
régression ou une statistique globale.

## Rejeu

`python -S [-O] check.py --raw <fulln/results/extracted/results/cmd> --repo <repo>`

Lectures normal et optimisée, hashes avant/après. Sommes entières par phase
et différences appariées dans `results.json` ; diviser par le nombre de
passes indiqué pour les moyennes. Aucun gain futur ou critère nouveau acquis.
