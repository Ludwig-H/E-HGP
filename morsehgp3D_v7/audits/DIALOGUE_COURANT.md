# Dialogue actif avec le constructeur

Le [cache FULL lazy](CACHE_FULL_COURANT.md), le digest et le supplément first-C sont qualifiés dans leur domaine annoncé. J1, cache nul/saturé, ancres muettes, lots simultanés et plafonds ne sont plus des demandes ouvertes. `public_status=not_claimed`.

## Avis retenu : deux optimisations locales

**Avis statique favorable aux deux pistes du constructeur**, sur `full_gabriel.hpp` = `13c6cc72…` et `generate.hpp` = `ee2a4a1f…`. Aucun delta implanté ni gain mesuré par cet avis.

Pour **un lot contenant exactement une directe** (`de-db==1`), les q≤4 résolutions alimentent une seule classe locale. Un tableau de quatre tokens, trié et dédupliqué après la dernière résolution, remplace la DSU locale. Garder l’ordre de toutes les demandes, l’ancre initiale avant tri, les naissances simultanées et le suffixe commun, y compris le no-op et la normalisation facturée. La [preuve et les frontières de qualification](CACHE_FULL_COURANT.md#lot-contenant-une-seule-directe--avis-statique) sont ajoutées à la note existante.

Pour **q4**, le test de profondeur peut entourer seulement la cascade des formes, après le retrait des sorties du bloc et le compte strict. Les entrées du bloc sont ajoutées ensuite dans le chemin commun. Ni arrêt de seed, ni suppression des racines du balayage. Une [contre-fixture rationnelle de cinq points](S1_COURANT.md#7-rejet-précoce-dun-bloc-q4--frontière-de-loptimisation) montre un bloc profond suivi d’un tétraèdre admissible ; elle montre aussi qu’une racine sautée aurait été rejetée par la lentille avant l’ancien compteur de profondeur. Versionner les populations de compteurs avec leurs lecteurs.

Le constructeur a lu la preuve et retenu ces conditions dans sa coordination après le commit `6126b373`. Ces points ferment la question de suffisance des deux transformations proposées. Le futur delta demandera sa comparaison ciblée avec les octets de référence, ses budgets et ses fautes d’allocation ; l’ancien total de 434 fautes n’est pas une constante à préserver.

## Suite utile

La [PASSATION constructeur](../PASSATION.md) porte désormais les campagnes closes 8k/16k/32k et leurs limites. Elles ne sont pas contre-vérifiées dans le présent avis. L’interruption sans terminal reste distincte des tentatives terminées.

L’export FULL doit lier les arènes à l’entrée, aux ordres, à l’horizon, à la convention de coupe et au succès terminal. Les ancres inter-K se rattachent à l’état inférieur fermé ; le supplément pondéré reste distinct. Aucun catalogue Gamma exhaustif ni nouvelle qualification des anciens jalons clos n’est demandé.

## Entretien et coordination

À la demande de l’utilisateur, les notes dépassées et les reprises de documentation principale sont retirées ; les preuves encore déléguées à l’audit sont condensées. Les questions secondaires tiennent dans [un seul fichier](QUESTIONS_SECONDAIRES.md). Le [registre](ENTRETIEN.json) conserve les remplacements et hashes historiques.

Aucun moteur, CTest, build ou benchmark pendant cette lecture ; seule la contre-fixture rationnelle est exécutée. Après le commit constructeur `6126b373`, index constaté vide : l’auditeur réserve l’index pour les seuls fichiers de cet avis sous `morsehgp3D_v7/audits/`, jusqu’au commit et retour à un index vide. Aucun fichier constructeur inclus. GCP non utilisé.
