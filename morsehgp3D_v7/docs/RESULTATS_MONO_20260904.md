# Première comparaison mono B/C, s=8/10/12

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [reçu terminal](../receipts/mono_bc_20260904/summary.json) contient six
exécutions achevées, trois paires et deux comparaisons entre séparations.
Tous les digests et toutes les cardinalités publiés sont identiques.
Les sources et binaires sont restés stables. La campagne est achevée,
sans censure ni refus ; ce n'est pas une qualification de performance.

## Résultats observés

Une entrée uniforme, n=8000, coordonnées générées dans le domaine 200,
seed=3 ; tour entière K1..K10, CSR, `--digest`, `--threads=1`,
`--fold-inflight=1`, `--fold-join=1`. Affinité CPU 6 sur l'EPYC 7763
virtualisé local. Ordre des bras : B/C, C/B, B/C. Aucune compilation
ou campagne lourde concurrente demandée pendant ces mesures.

| Séparation s | B, secondes | C, secondes | Baisse observée | RSS max B, Mio | RSS max C, Mio |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 127,997 | 105,932 | 17,24 % | 1893,43 | 1852,60 |
| 10 | 125,524 | 105,594 | 15,88 % | 1894,18 | 1851,18 |
| 12 | 126,527 | 105,112 | 16,93 % | 1893,50 | 1851,44 |

Temps externe du processus, génération de l'entrée et digest compris,
sans archive sur disque. Chaque valeur vient d'un seul processus ; il
n'y a ni échauffements contractuels ni répétitions suffisantes pour un
intervalle de confiance ou un p95. L'hôte n'est pas une machine dédiée.
Le faible écart entre s=8,10,12 ne justifie pas de changer le défaut s=8.

À s=8, préfiltre + census passent de 38,7905 s à 17,2970 s. La génération
reste à environ 60 s et le fold à environ 23 s ; ils sont désormais les
premiers postes à examiner. Les compteurs de parcours du census ne sont
pas supprimés pour gagner du temps. L'argmin réduit le calcul par boîte.
L'écart de RSS est mesuré, sans attribution causale à une allocation
précise ni promesse de réduction universelle de mémoire.

Les compteurs et temps de C montrent le compromis de séparation sur
cette entrée, sans imposer leur égalité entre s :

| s | WSPD, secondes | Traitement des rectangles, secondes | Rectangles visités, compteur fusionné | Candidats émis |
| ---: | ---: | ---: | ---: | ---: |
| 8 | 31,440 | 28,496 | 5 053 015 | 3 135 204 |
| 10 | 32,447 | 27,367 | 5 844 749 | 3 121 244 |
| 12 | 33,923 | 26,537 | 6 346 957 | 3 114 237 |

Les 3 103 251 boules survivantes et les 292 952 871 visites du census
complet restent identiques. Ici, augmenter s réduit les candidats q2
superflus mais augmente le travail du front WSPD ; les temps des deux
parties de la génération se compensent largement. Ce constat local ne
détermine pas le meilleur s pour une autre famille ou cardinalité.

## Objet et autorité

Le payload de ce diagnostic est **`verified_events_only`**, commun à B et
C ; ce n'est pas Gamma complet. Chaque tour contient 3 126 158 événements
et 19 466 907 facettes cumulées sur les dix ordres. Cette somme n'est pas
le nombre de facettes simultanément résidentes. Le digest complet commun
est `f57157901b2a20282f5c418e48a376f3e33dab0047328020eb3f9846408974c7`.

B conserve l'ancien fold sur thread auxiliaire mais sérialisé. C exécute
le fold sur l'appelant et utilise le nouvel argmin. Les
[dix portes combinées](../receipts/mono_axis_combined_20260904/README.md)
qualifient ces transformations sur leurs fixtures ; le runner de mesure
ne prétend pas prouver lui-même l'absence de thread auxiliaire.

| Binaire mesuré | SHA-256 | Reçu de rattachement |
| --- | --- | --- |
| B conservé | `fa917eefd8198d8ee676585dd99401f74594dd33a4bf77e1265ef397f439e200` | [Delta B2](../receipts/release_delta2_20260904/summary.json) |
| C | `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2` | [Construction C](../receipts/mono_c_build_20260904/summary.json), puis [Release fraîche 292/292](../receipts/release_c_20260904/summary.json) |

Les [données par run](../receipts/mono_bc_20260904/runs.json),
[métadonnées](../receipts/mono_bc_20260904/metadata.json) et
[hashes des sorties brutes](../receipts/mono_bc_20260904/hashes.json)
restent inchangés après scellement. B n'est pas présenté comme compilé
depuis les sources C qui étaient courantes pendant la comparaison.
Les commits documentaires intervenus pendant le run n'ont changé aucun
octet consommé. La suite Release C fraîche a un reçu distinct, postérieur
à cette campagne : les 292 portes ont été exécutées sans échec ni skip,
et le CLI reconstruit isolément est identique octet pour octet à C mesuré.

## Suite imposée par le contrat

Ni cette mesure à 8k, ni le gain local du census ne satisfait le contrat
exact 50k / tour 1..10 / une seconde. La tour 1..5 reste le repli explicite,
et 100 ms vient après qualification de la seconde. Les étapes silencieuses,
la verticale et les poids ne doivent pas être retirés du périmètre requis
pour annoncer le délai. Aucune extrapolation vers plusieurs dizaines de
millions sur G4 n'est validée par cette comparaison.

GCP non utilisé par cette campagne locale.
