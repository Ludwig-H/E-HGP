# Mono FULL EAGER : observations et borne utile

Campagne `98bb6578`, header `e02d163c`, `public_status=not_claimed`. Les [résultats principaux](../docs/RESULTATS_MONO_FULL_20260905.md) portent les temps détaillés. Les [preuves d’audit](receipts_full_mono_20260905/README.md) contre-vérifient trois succès relatifs 8k à s=8/10/12 et deux refus d’alias : 16k/K9 et 32k/K7. Aucun de ces refus n’est un timeout ou un temps d’achèvement.

## Borne indépendante conservée

Pour un ordre réussi, noter L les minima, D les directes, T la somme des cardinaux de leurs supports et V les alias ajoutés par portails. L’identité EAGER est `A=L+2(K+1)D−T+V`. Les 44 lignes réussies la satisfont. Elle ne s’applique pas à un préfixe refusé ni à lazy.

À 8k/s8/K10, les 6 209 024 alias se décomposent en 600 806 minima, 5 349 726 facettes égales et 258 492 alias de portail. Les seules demandes strictes sont Q=2 534 359 : minima et cache réunis sont donc bornés par **3 135 165 clés**. Cette borne compte des clés, pas des octets de RAM.

Avec minima séparés, le cache strict est borné par 4D : 5 063 544 clés au premier ordre refusé 16k/K9, 6 856 080 à 32k/K7. Huit millions suffisent pour ces caches seuls ; les autres budgets et ordres suivants ne sont pas admis par cet argument. La [preuve mémoire](receipts_full_mono_20260905/memory_model_review.md) conserve hypothèses et propriétaires.

## Juge et comparaison suivante

Quatre [corruptions de données](receipts_full_mono_20260905/judge_review.md) exposaient les lacunes du juge v1 : minima, miroir MEB, identité d’alias et temps. Elles ne préservaient pas les sceaux historiques et n’invalident pas les runs nominaux. Le v2 et son supplément first-C ont depuis leur [contre-vérification propre](CACHE_FULL_COURANT.md).

La nouvelle sonde doit comparer les deux bras avec digest compris. Les anciens temps sans digest restent historiques. À 8k/s8, la génération occupait déjà 61,434 s sur 150,776 s ; supprimer les alias ne résout pas ce coût. Les RSS imprimés après destruction du Builder ne mesurent pas sa résidence pendant sa vie. Aucune mesure lazy massive n’est ajoutée ici.
