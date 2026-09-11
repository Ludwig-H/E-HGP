# Mono FULL : mesures courantes et preuves de travail

Le [triplet mono après échange](../receipts/post_exchange_scale_20260911/README.md), header 6763a877, conserve les sorties et évite 237 557 / 501 258 / 1 045 620 MEB supplémentaires à 8k/16k/32k : environ 5,7 % du travail statique précédent. Les lecteurs sont [contre-vérifiés](receipts_filtered_graph_20260911/README.md), sans nouveau run moteur. Les anciens temps statique4 ne sont pas une paire causale avec ce triplet mono ; aucun gain de latence ou RSS ne lui est attribué.

La [voie statique initiale](../docs/RESOLUTION_STATIQUE_CPU_20260911.md) et sa [décomposition](receipts_static_followup_20260911/README.md) restent historiques : environ 98 % de sa baisse MEB venait des résolutions initiales évitées. Le semis après échange, alors proposé, est désormais intégré ; le critère de support survivant reste non mesuré.

Les [tours 50k cache CPU/hybride, ad7ffd28](../docs/RESULTATS_TOUR_CACHE_G4_20260910.md) restent distinctes. Contrats 1 s/100 ms et régime massif ouverts. Notre [relecture de résidence](receipts_tower_cost_review_20260910/README.md) et le [calcul net incrémental](receipts_incremental_review_20260911/README.md) conservent leurs bornes historiques, sans nouveau gain RSS.

Les [anciens passages sans quotas](../docs/RESULTATS_MONO_FULL_SANS_QUOTAS_20260906.md) libéraient chaque ordre horizontal ; ils ne sont pas une baseline appariée de cette tour retenue. Le [refus G4 CPU48 à 50k](../docs/RESULTATS_G4_FULL_20260906.md) est également historique : la garde extra-shell de cet ancien instrument ne décrit pas le nouveau raccord. Les échecs restent scellés, sans requalification rétroactive. GCP non utilisé par l’auditeur.

## Refus historique conservé

Le [préfixe refusé 32k/K9](receipts_followup_20260906/work_review_normal.json) attestait P+C=M+1 au plafond de quatre millions ; il ne devient pas une réussite après suppression de ce plafond. La [réutilisation terminale](MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise) concerne la lignée lazy régulière, distincte du nouveau resolver à ancres.

## Preuves historiques indépendantes

Les chronologies et tableaux de mesures déjà documentés par le constructeur sont retirés de cette note. Les preuves encore utiles restent consultables :

| Objet historique | Preuve conservée et portée |
| --- | --- |
| Alias EAGER | [Modèle mémoire](receipts_full_mono_20260905/memory_model_review.md) : A=L+2(K+1)D−T+V ; 44 lignes réussies. Borne des clés, pas octets de RAM ni identité du moteur courant. |
| Juges de mesures | [Quatre corruptions v1](receipts_full_mono_20260905/judge_review.md) ; résultats négatifs conservés, lecteurs successeurs qualifiés séparément. |
| Normalisation lazy | [Recalcul reproductible](successor_work_review.json), [script](successor_work_review.py) : 48 ordres clos et quatre corruptions réfutées. Les comptes prévus de v2 sont retrouvés sans en déduire un temps économisé. |
| Plafonds MEB supprimés | [Pairage P0/unlimited](receipts_terminal_count_20260906/source_review.json) : K9–K10 concentrent 78,89 % de la différence FULL observée à 8k ; une paire, pas une statistique ni un temps MEB isolé. |

## Borne des supports MEB q4 sur les six passages singleton

La [campagne singleton historique](../docs/RESULTATS_MONO_FULL_SINGLETON_20260905.md) est close, sans accélération robuste retenue. Le [contrôle indépendant](meb_full_work_review.py) raccorde ses six bruts, reçus et snapshots de source ; son [résultat](meb_full_work_review.json) retrouve les mêmes comptes MEB sur 60 ordres réussis. Python normal et `-O` concordent ; quatre corruptions de données sont refusées. Ce contrôle porte sur les identités de travail, pas sur toute la qualification de la campagne ni sur ses temps.

En lazy réussi, P=`portal_requests` appelle une MEB sur K sites ; chacune des C=`chain_steps` appelle ensuite une MEB sur K+1 sites. Donc M=P+C. Chaque appel F énumère au plus B(n) supports q2/q3, puis au plus $\binom{n}{4}$ supports q4, où B(n) est la somme des nombres de paires et de triples. Avec S=`meb_supports` :

$$B(n)=\binom{n}{2}+\binom{n}{3},\qquad S_{q4}\geq\max\left(0,S-PB(K)-CB(K+1)\right).$$

| Ordre 8k, identique sur les six bras | Appels MEB | Supports totaux | Supports q4 essayés, minimum | Appels terminant q4, minimum |
| --- | ---: | ---: | ---: | ---: |
| K9 | 956 321 | 134 645 682 | 3 977 502 | 18 941 |
| K10 | 1 202 962 | 250 854 612 | 27 267 677 | 82 630 |

À K10, P=746 631 et C=456 331 donnent au plus 223 586 935 essais q2/q3 ; **au moins 10,87 % des supports essayés sont donc q4**. Un appel sur au plus onze sites essaie au plus 330 quadruplets, d’où la seconde borne par division et arrondi supérieur. En succès, un appel atteignant cette boucle termine sur un support q4. Cela ne compte pas seulement des tétraèdres valides : les formes singulières ou rejetées restent des essais facturés. Aucun coût CPU ne découle de ces fractions.

Aux ordres inférieurs, une borne nulle ne signifie pas absence de q4. Ces agrégats ne donnent ni l’histogramme des ordinaux ni le gain du [filtre de pivot](MEB_DOUBLE_BUDGET_COURANT.md#réduction-démontrée-des-formes-de-pivot). Ils suffisent à réfuter l’idée que le K10 serait constitué exclusivement de MEB à support q2/q3. Les préfixes refusés restent exclus.

La [borne sur les minima FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md#taille-des-feuilles--le-pire-cas-porte-sur-full) ferme maintenant la distinction entre sortie et candidats : le pire cas est quadratique dès K2 et pour chaque K fixé≥2 à précision croissante. Les observations uniformes 8k/16k/32k restent un régime séparé ; un préfixe refusé ne fournit pas un temps de tour complète.
