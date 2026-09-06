# Premier triplet complet sans quotas d'opérations FULL

6 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**8k, 16k et 32k terminent chacun les dix ordres horizontaux.** L'ancien
refus 32k/K9 à quatre millions de MEB ne bloque plus la mesure. Les
contrats 50k/1 s, puis 100 ms et dizaines de millions G4 restent non atteints.
Cette sonde libère chaque forêt après lecture : elle ne mesure ni une
archive FULL retenue ni les liens inter-K intégrés.

## Mesures directes

Même binaire `4938b94b3166e8c13d02b0fd9687168130d5c528702d7efcf7b7379b3adeb360`,
uniforme/seed3/u16, un thread sur CPU6, s WSPD=8, Kmax=10, cache lazy
first-C d'un million d'entrées, `P=unlimited`. La compilation et ses
[sources exactes sont conservées](../receipts/full_probe_no_quotas_20260906/README.md).
Les tests sont suivis directement ; aucun délai automatique ni admission
de format ne conditionne leur lancement. Les types et limites mémoire
de la sonde restent ceux de son [profil déclaré](CONTRAT_SONDE_FULL_MEB.md).
Les [bruts et le lecteur reproductible](../receipts/full_direct_scaling_20260906/README.md)
conservent les trois passages, sans lancer de moteur lors de leur relecture.

| n | Ordres terminés | Temps avant terminal | Pic mémoire observé | Génération | Construction FULL |
| ---: | --- | ---: | ---: | ---: | ---: |
| 8 000 | 1..10 | 133,038 s | 1 289,266 Mio | 59,640 s | 50,477 s |
| 16 000 | 1..10 | 307,643 s | 2 649,055 Mio | 132,409 s | 123,099 s |
| 32 000 | 1..10 | 684,574 s | 5 394,418 Mio | 284,484 s | 283,901 s |

Le temps inclut entrée générée, index, génération, préfiltre/census,
catalogues, construction des dix forêts, lecture, digest et libération.
Les temps externes du runner sont respectivement 133,047 / 307,656 /
684,608 s. Les étapes de la table ne prétendent pas sommer seules le
temps complet. Aucun gain de contrat n'est obtenu en les soustrayant.

| Volume cumulé des dix ordres | 8k | 16k | 32k |
| --- | ---: | ---: | ---: |
| Minima FULL | 2 404 646 | 5 026 402 | 10 380 964 |
| Nœuds FULL | 3 976 472 | 8 310 399 | 17 166 975 |
| Références de parents | 3 976 462 | 8 310 389 | 17 166 965 |
| Appels MEB du resolver | 4 305 891 | 9 241 478 | 20 239 401 |
| Formes proposées réellement testées | 24 777 382 | 53 377 876 | 117 114 021 |

Tous ces appels MEB sont certifiés par le proposeur : aucun repli F dans
ces trois captures. Leurs ordinaux de référence restent des compteurs
distincts, pas des formes physiques exécutées. À 32k, K9 paie 4 605 147
MEB et K10 en paie 5 853 547 ; les deux ordres terminent.

## Croissance observée, pas théorème universel

Pour un doublement n→2n, l'exposant rapporté est
$\alpha=\log_{2}(T_{2n}/T_n)$.

| Quantité | 8k→16k | 16k→32k |
| --- | ---: | ---: |
| Temps complet | 1,209 | 1,154 |
| Nœuds FULL | 1,063 | 1,047 |
| Minima FULL | 1,064 | 1,046 |
| Candidats bruts | 1,068 | 1,049 |
| Appels MEB | 1,102 | 1,131 |

Le comportement est sous-quadratique sur **ce triplet uniforme**, sans
répétitions statistiques ni qualification des autres géométries. Le temps
par nœud augmente encore : exposants résiduels 0,146 puis 0,107. La
[borne de sortie quadratique](CROISSANCE_ET_BORNE_DE_SORTIE.md) reste une
contrainte pour une sortie explicite sur des nuages généraux ; ces trois
mesures ne la contredisent pas et ne prédisent pas le temps sur G4.

Le cache est fixé à un million d'entrées **par ordre**, pas dimensionné
proportionnellement à n. Aucun insert n'est omis à 8k ; à 16k et 32k,
respectivement 996 236 et 6 717 054 insertions facultatives sont sautées
après saturation. Cette politique intervient donc dans la croissance
observée. Les résolutions correspondantes restent calculées ; il ne
s'agit pas de minima ou de connexions perdus.

## Génération : deux postes comparables

| Poste | 8k | 16k | 32k |
| --- | ---: | ---: | ---: |
| Front WSPD et ses témoins | 29,567 s | 69,283 s | 151,786 s |
| Traitement des rectangles survivants | 30,073 s | 63,126 s | 132,697 s |

La priorité utilisateur suivante porte sur les rejets par blocs avec
h_cœur, h_a et h_b. Renforcer ces certificats et éviter les comptages ou
tests répétés sont deux leviers distincts. Les classes de scores des
histogrammes peuvent éviter d'énumérer les paires rejetées, mais ne
suppriment pas leur coût de construction actuel en A²+B². Un nouveau
cœur de sous-rectangle ne s'additionne pas aveuglément aux crédits des
facteurs parentaux : certains témoins seraient comptés deux fois.
La consultation de l'auditeur et le premier test de réutilisation du
compte terminal q2 ne constituent pas encore un gain mesuré dans ce lot.

## Comparaison P0 / unlimited sur le même binaire

La [paire directe 8k/s8](../receipts/full_direct_p0_comparison_20260906/README.md)
est maintenant close sur le binaire `4938b94b…`. P0 prend 154,837 s
externes contre 133,047 s avec `unlimited`, soit −14,07 % sur ces deux
passages. La phase FULL passe de 71,590 à 50,477 s (−29,49 %), sans
chronométrage MEB isolé. Les dix digests, le digest final et les champs non
mesurés hors configuration P et cinq diagnostics MEB sont identiques.
La génération ne dépend pas de ce MEB local ; sa petite variation de
60,253 à 59,640 s n'est pas un effet du proposeur.

Les deux bras comptent 4 305 891 appels MEB. P0 paie 503 231 458 supports F ;
`unlimited` paie 24 777 382 formes proposées et aucun support F. Leur ratio
20,310 n'est ni un facteur de vitesse ni un rapport de coûts homogènes.
Un seul passage par bras ne donne pas d'intervalle de confiance. Aucune
activation générale par défaut n'en est déduite. L'ancien P0 de 159,160 s
reste un témoin historique, pas le bras frais de cette paire.

## Réemploi q2 et séparation WSPD

Les [comparaisons s=8/10/12](../receipts/full_wspd_q2_separation_20260906/README.md)
du nouveau binaire `23646a32…` portant le réemploi q2 sont closes à 8k :

| Séparation | Temps avant terminal | Front WSPD | Rectangles | Candidats bruts |
| ---: | ---: | ---: | ---: | ---: |
| 8 | 131,482 s | 28,938 s | 29,761 s | 3 144 017 |
| 10 | 132,138 s | 31,358 s | 28,680 s | 3 129 992 |
| 12 | 137,247 s | 32,890 s | 29,135 s | 3 123 497 |

Les dix digests et compteurs de construction des forêts sont identiques,
ainsi que les 3 113 381 boules après census. Augmenter s réduit les coins
mais augmente les visites de témoins : la génération totale augmente de
2,28 % puis 5,67 % par rapport à s8. Ces observations ne déterminent pas
un meilleur s universel. Le réemploi q2 à s8 garde aussi les mêmes comptes
de visites et de coins que l'ancien code, car q3/q4 imposent encore les
parcours fusionnés ; le recul observé de 1,17 % au total ne constitue pas
un gain robuste. Les trois mesures ne sont pas réattribuées au triplet
initial et ne remplacent pas de futures comparaisons sur d'autres régimes.

Les digests de la table sont ceux de forêts horizontales relatives aux
catalogues fournis, non des preuves de complétude géométrique. Les
[contrats 50k et G4](CONTRAT_PERFORMANCE.md) restent ouverts. GCP non utilisé.
