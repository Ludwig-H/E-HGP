# Mono FULL : observations et diagnostics de travail

Les [résultats sans quotas](../docs/RESULTATS_MONO_FULL_SANS_QUOTAS_20260906.md) remplacent le diagnostic de blocage par un plafond MEB : 8k/16k/32k ont terminé chacun les dix ordres horizontaux. La sonde libère les forêts ; elle ne mesure pas encore les liens inter-K ni une archive retenue. Les variantes antérieures restent des témoins historiques. public_status=not_claimed.

Le [relevé indépendant du 6 septembre](receipts_terminal_count_20260906/source_review.json) confirme le pairage P0/unlimited à 8k : mêmes données, binaire et digests, mêmes compteurs hors diagnostics MEB annoncés. K9–K10 concentrent 16,658 s des 21,114 s économisées dans FULL, soit 78,89 % sur cette paire. Cela étaye leur priorité d’optimisation ; ce n’est ni une nouvelle mesure, ni une vitesse isolée du MEB, ni une conclusion statistique. Les méthodes de mesure et les tableaux déjà décrits par le constructeur ne sont pas recopiés ici.

## Refus historique conservé

Le [diagnostic 32k/K9](receipts_followup_20260906/work_review_normal.json) identifiait correctement la MEB initiale d’un nouveau portail au plafond de quatre millions, avec P+C=M+1. Ce plafond supprimé dans la sonde ne devient ni une nouvelle réussite de ce run ni un refus courant. Ses agrégats ne prouvaient pas la répétition des labels terminaux. La [réutilisation de certification](MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise) reste une optimisation distincte des formes internes du proposeur.

## Témoin EAGER historique

La campagne `98bb6578`, header `e02d163c`, garde ses [preuves](receipts_full_mono_20260905/README.md) : trois succès relatifs 8k à s=8/10/12 et deux refus d’alias, 16k/K9 et 32k/K7. Les [résultats EAGER](../docs/RESULTATS_MONO_FULL_20260905.md) ne deviennent pas des mesures du code courant.

## Borne indépendante conservée

Pour un ordre réussi, noter L les minima, D les directes, T la somme des cardinaux de leurs supports et V les alias ajoutés par portails. L’identité EAGER est `A=L+2(K+1)D−T+V`. Les 44 lignes réussies la satisfont. Elle ne s’applique pas à un préfixe refusé ni à lazy.

À 8k/s8/K10, les 6 209 024 alias se décomposent en 600 806 minima, 5 349 726 facettes égales et 258 492 alias de portail. Les seules demandes strictes sont Q=2 534 359 : minima et cache réunis sont donc bornés par **3 135 165 clés**. Cette borne compte des clés, pas des octets de RAM.

Avec minima séparés, le cache strict est borné par 4D : 5 063 544 clés au premier ordre refusé 16k/K9, 6 856 080 à 32k/K7. Huit millions suffisent pour ces caches seuls ; les autres budgets et ordres suivants ne sont pas admis par cet argument. La [preuve mémoire](receipts_full_mono_20260905/memory_model_review.md) conserve hypothèses et propriétaires.

## Lecteurs et diagnostics conservés

Quatre [corruptions de données](receipts_full_mono_20260905/judge_review.md) exposaient les lacunes du juge v1 : minima, miroir MEB, identité d’alias et temps. Elles ne préservaient pas les sceaux historiques et n’invalident pas les runs nominaux. Le v2 et son supplément first-C ont depuis leur [contre-vérification propre](CACHE_FULL_COURANT.md).

La sonde lazy a depuis publié ses [mesures appariées](../docs/RESULTATS_MONO_FULL_LAZY_20260905.md), digest compris. Les anciens temps sans digest restent historiques. Les RSS imprimés après destruction du Builder ne mesurent pas sa résidence pendant sa vie. Ces nouvelles latences ne sont pas qualifiées indépendamment ici.

## Diagnostic des successeurs sur les captures lazy closes

La [contrelecture ciblée](successor_work_review.json), reproductible par [ce script](successor_work_review.py), raccorde cinq bruts à leurs reçus et snapshots `13c6`, puis vérifie les identités de [normalisation](CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante). Elle couvre 48 ordres réussis, identiques entre s=8/10/12 à 8k, et exclut explicitement le K9 refusé. Python normal/`-O` donnent les mêmes octets ; quatre corruptions de données sont refusées. Aucune exécution moteur, certification de latence ou extension de l’oracle géométrique.

| Ordre clos | Opérations historiques v1 | Part des seules clôtures v1 | Prévision v2, maintenant retrouvée dans les captures |
| --- | ---: | ---: | ---: |
| 8k/K10, s8 | 38 240 799 | 4,91 % | 33 607 807 (−12,12 %) |
| 16k/K10, s8 | 85 034 894 | 4,66 % | 75 223 906 (−11,54 %) |
| 32k/K8, s8 | 119 950 564 | 4,57 % | 106 373 946 (−11,32 %) |

Le dernier calcul concerne **K8 réussi**, pas K9 refusé. Les trois valeurs prédites sont désormais retrouvées dans les captures v2 contre-vérifiées, sans en déduire un temps économisé ou la fin de K9. Les profondeurs pré-lot moyennes historiques valent respectivement 4,4493, 4,7042 et 4,8201 : le volume de travail ne prouve pas une chaîne pathologique. Un effort limité à la fermeture des directes aurait visé moins de 5 % de ce compteur ; le delta qualifié concerne toutes les normalisations.

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
