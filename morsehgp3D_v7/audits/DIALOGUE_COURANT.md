# Dialogue actif avec le constructeur

11 septembre 2026, lecture de **dc5a36ba**. Les sources et reçus historiques restent inchangés ; la [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Réponse sur le graphe filtré

Avis favorable à la [proposition](../docs/GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md), sous ses prémisses. Notre [complément indépendant](receipts_filtered_graph_20260911/README.md) prouve une réduction supplémentaire : choisir un terminal strict pivot par hub, suivre ces pointeurs jusqu’à une naissance, puis transférer les autres arêtes à leurs naissances représentantes, au niveau du bloc consommateur.

Le graphe a L sommets et R−A+L arêtes avant suppression des boucles/doublons. Une forêt minimale en conserve les coupes. La carte des hubs, les dates de contributions et l’admission historique des ancres restent nécessaires. Une référence inférieure par naissance suffit à la verticale interne, sous naturalité ; l’export physique actuel demande encore sa reconstruction canonique.

Le modèle indépendant vérifie 267 cas, les deux côtés des coupes, parents depuis la seule forêt, contributions et ancres ; sept transformations fautives et deux contre-fixtures sont distinguées. Aucun raccord C++ ni gain de temps n’est acquis. Le prochain jalon utile est une comparaison du graphe réduit au vrai calendrier et au juge T2 sur les mêmes petits census, avant toute mesure d’échelle.

## Demandes closes

| Point | Conclusion et preuve à conserver |
| --- | --- |
| Semis après échange | [Intégré et qualifié](../docs/SEMIS_APRES_ECHANGE_20260911.md), header 6763a877 ; triplet mono 8k/16k/32k clos, environ 5,7 % de MEB statiques supplémentaires évitées. Aucun speedup apparié aux anciens temps. |
| Vrai census→tour K9/K10 | [T2 initial, gardes de métadonnées et rejeu actif](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md), 54 tours par build O2/SAN ; lecteurs contre-vérifiés. La borne n12/n14 ne prouve pas la généralité WSPD. |
| Premier raccord incrémental | [Essai publié](../receipts/incremental_full_trial_20260911/README.md), sorties/refus qualifiés puis variante non retenue pour ses coûts. Ne plus demander cet essai ni son intégration comme optimisation. |
| Blocs nommés 50k et lots groupés | [Second auditeur, ad7ffd28](receipts_cache_commit_20260911/README.md), deux verrous clos sur CPU ; premier SIGTERM conservé, ni complétude ni arité finale ni contrat de vitesse. |
| R/U, statique, panne après admission | [Complément précédent](receipts_static_followup_20260911/README.md), diagnostic et rejeu SAN clos. Les preuves mathématiques et captures restent distinctes. |

Le critère de support survivant reste une piste prouvée, sans implémentation ou gain mesuré. Les autres remarques secondaires du [second auditeur](receipts_cache_commit_20260911/README.md) doivent être confrontées aux livraisons plus récentes avant reprise ; leur liste ne devient pas une nouvelle série de blocages.

Archive industrielle et contrats 1 s/100 ms ou massifs restent ouverts. Les prototypes GPU ont leurs reçus propres, hors de cette passe. GCP non utilisé.
