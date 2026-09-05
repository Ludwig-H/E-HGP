# Mono FULL : observations et diagnostics de travail

Campagne `98bb6578`, header `e02d163c`, `public_status=not_claimed`. Les [résultats principaux](../docs/RESULTATS_MONO_FULL_20260905.md) portent les temps détaillés. Les [preuves d’audit](receipts_full_mono_20260905/README.md) contre-vérifient trois succès relatifs 8k à s=8/10/12 et deux refus d’alias : 16k/K9 et 32k/K7. Aucun de ces refus n’est un timeout ou un temps d’achèvement.

## Borne indépendante conservée

Pour un ordre réussi, noter L les minima, D les directes, T la somme des cardinaux de leurs supports et V les alias ajoutés par portails. L’identité EAGER est `A=L+2(K+1)D−T+V`. Les 44 lignes réussies la satisfont. Elle ne s’applique pas à un préfixe refusé ni à lazy.

À 8k/s8/K10, les 6 209 024 alias se décomposent en 600 806 minima, 5 349 726 facettes égales et 258 492 alias de portail. Les seules demandes strictes sont Q=2 534 359 : minima et cache réunis sont donc bornés par **3 135 165 clés**. Cette borne compte des clés, pas des octets de RAM.

Avec minima séparés, le cache strict est borné par 4D : 5 063 544 clés au premier ordre refusé 16k/K9, 6 856 080 à 32k/K7. Huit millions suffisent pour ces caches seuls ; les autres budgets et ordres suivants ne sont pas admis par cet argument. La [preuve mémoire](receipts_full_mono_20260905/memory_model_review.md) conserve hypothèses et propriétaires.

## Juge et comparaison suivante

Quatre [corruptions de données](receipts_full_mono_20260905/judge_review.md) exposaient les lacunes du juge v1 : minima, miroir MEB, identité d’alias et temps. Elles ne préservaient pas les sceaux historiques et n’invalident pas les runs nominaux. Le v2 et son supplément first-C ont depuis leur [contre-vérification propre](CACHE_FULL_COURANT.md).

La sonde lazy a depuis publié ses [mesures appariées](../docs/RESULTATS_MONO_FULL_LAZY_20260905.md), digest compris. Les anciens temps sans digest restent historiques. Les RSS imprimés après destruction du Builder ne mesurent pas sa résidence pendant sa vie. Ces nouvelles latences ne sont pas qualifiées indépendamment ici.

## Diagnostic des successeurs sur les captures lazy closes

La [contrelecture ciblée](successor_work_review.json), reproductible par [ce script](successor_work_review.py), raccorde cinq bruts à leurs reçus et snapshots `13c6`, puis vérifie les identités de [normalisation](CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante). Elle couvre 48 ordres réussis, identiques entre s=8/10/12 à 8k, et exclut explicitement le K9 refusé. Python normal/`-O` donnent les mêmes octets ; quatre corruptions de données sont refusées. Aucune exécution moteur, certification de latence ou extension de l’oracle géométrique.

| Ordre clos | Opérations actuelles S | Part des seules clôtures | S après suppression des deux opérations redondantes par appel non trivial |
| --- | ---: | ---: | ---: |
| 8k/K10, s8 | 38 240 799 | 4,91 % | 33 607 807 (−12,12 %) |
| 16k/K10, s8 | 85 034 894 | 4,66 % | 75 223 906 (−11,54 %) |
| 32k/K8, s8 | 119 950 564 | 4,57 % | 106 373 946 (−11,32 %) |

Le dernier calcul concerne **K8 réussi**, pas K9 refusé. Il fixe exactement le travail logique économisable sur cette trace, sans prédire le temps économisé ou une nouvelle admission. Les profondeurs pré-lot moyennes valent respectivement 4,4493, 4,7042 et 4,8201 : le volume de travail ne prouve pas une chaîne pathologique. Un effort limité à la fermeture des directes viserait moins de 5 % de ce compteur ; le raccourci proposé concerne toutes les normalisations. Aucun ratio CPU n’en découle.
