# Dialogue actif avec le constructeur

Le [cache FULL lazy](CACHE_FULL_COURANT.md), le digest et le supplément first-C sont qualifiés dans leur domaine annoncé. J1, cache nul/saturé, ancres muettes, lots simultanés et plafonds ne sont plus des demandes ouvertes. `public_status=not_claimed`.

## Travail courant : coût des normalisations

**La dernière relecture/écriture de compression est supprimable sans changer l’état final des successeurs.** Retenir le dernier nœud avant la racine lors de la première passe, puis arrêter la compression avant lui. La [preuve](CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante) donne 1 opération si d=0, 3d−1 sinon ; le calendrier budgétaire doit être versionné si ces opérations cessent d’être facturées.

Le [diagnostic indépendant](MONO_FULL_COURANT.md#diagnostic-des-successeurs-sur-les-captures-lazy-closes) vérifie 48 ordres lazy clos. À 32k/K8, supprimer cette paire dans tous les appels non triviaux ferait passer les opérations de 119 950 564 à 106 373 946 (−11,32 %). Les clôtures des directes ne pèsent seules que 4,57 %. Ces comptes ne prédisent ni un gain de temps ni la réussite du K9 refusé à 128 millions. Son préfixe est explicitement exclu des égalités de succès.

Le constructeur implante séparément le lot à une directe. La qualification indépendante reste attachée au header `13c6cc72` ; le nouveau code en préparation ne lui est pas substitué dans le manifeste. Aucun moteur/build/CTest d’audit n’est occupé ; seules des lectures et de courts calculs Python CPU0 sont effectués.

## Avis antérieurs retenus

La [preuve du lot à une directe](CACHE_FULL_COURANT.md#lot-contenant-une-seule-directe--avis-statique) et la [contre-fixture q4](S1_COURANT.md#7-rejet-précoce-dun-bloc-q4--frontière-de-loptimisation) restent acquises statiquement. Toutes les résolutions, leur ordre et les naissances simultanées doivent être conservés ; q4 ne saute que le bloc profond et poursuit le balayage. Le développeur a retenu ces conditions. Les qualifications et mesures des futurs deltas restent distinctes.

## Suite utile

La [PASSATION constructeur](../PASSATION.md) porte les campagnes closes 8k/16k/32k et leurs limites. Le présent contrôle porte seulement sur les compteurs de normalisation des ordres réussis, pas sur toute leur qualification. L’interruption sans terminal reste distincte des tentatives terminées.

L’export FULL doit lier les arènes à l’entrée, aux ordres, à l’horizon, à la convention de coupe et au succès terminal. Les ancres inter-K se rattachent à l’état inférieur fermé ; le supplément pondéré reste distinct. Aucun catalogue Gamma exhaustif ni nouvelle qualification des anciens jalons clos n’est demandé.

## Entretien et coordination

À la demande de l’utilisateur, les notes dépassées et les reprises de documentation principale sont retirées ; les preuves encore déléguées à l’audit sont condensées. Les questions secondaires tiennent dans [un seul fichier](QUESTIONS_SECONDAIRES.md). Le [registre](ENTRETIEN.json) conserve les remplacements et hashes historiques.

Le commit d’audit `aafe7d93` est publié et sa réservation d’index est close. Index constaté vide : réservation pour publier le seul diagnostic ci-dessus et les notes actualisées, jusqu’au commit et retour à un index vide. Aucun fichier constructeur inclus. GCP non utilisé.
