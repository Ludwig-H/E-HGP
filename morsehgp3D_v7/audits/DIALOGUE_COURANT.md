# Dialogue actif avec le constructeur

11 septembre 2026, reprise sur **b8336ad3**. Les sources et reçus historiques restent inchangés ; la [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Travail courant

Le [second auditeur](NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md) demande une porte CTest permanente census→tour et en [chiffre le coût à partir des captures](NOTE_CLAUDE_COUT_PORTE_ET_GPU_20260911.md). Le constructeur la prépare à partir du T2 renforcé. La qualification K9/K10 acquise n’est pas remise en cause ; l’objectif est de la rejouer automatiquement aux changements suivants.

Le raccord par lots et le contexte GPU restent privés. Notre [nouveau témoin](receipts_batch_work_20260911/README.md) a reproduit un total partiel déclaré connu après débordement dans l’adaptateur, puis vérifié la correction exacte du constructeur O2/SAN. La fusion globale du Builder a sa révision et ses gates distinctes ; la qualification de leur raccord complet reste séparée.

L’[accord sur le graphe et sa réduction aux naissances](receipts_filtered_graph_20260911/README.md) est acquis : 267 cas, coupes, parents, contributions et ancres, sans raccord C++ ou gain de temps qualifié. La prochaine étape de cette piste reste la comparaison au vrai calendrier et au juge T2, sur les mêmes petits census. Les preuves détaillées ne sont plus répétées dans le dialogue.

## Demandes closes

| Point | Conclusion et preuve à conserver |
| --- | --- |
| Compteurs de l’adaptateur privé | [Ancien drapeau réfuté, correctif 993786f3 vérifié](receipts_batch_work_20260911/README.md) ; uniquement la frontière d’agrégation, données injectées, aucun kernel exécuté. |
| Semis après échange | [Intégré et qualifié](../docs/SEMIS_APRES_ECHANGE_20260911.md), header 6763a877 ; triplet mono 8k/16k/32k clos, environ 5,7 % de MEB statiques supplémentaires évitées. Aucun speedup apparié aux anciens temps. |
| Vrai census→tour K9/K10 | [T2 initial, gardes de métadonnées et rejeu actif](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md), 54 tours par build O2/SAN ; lecteurs contre-vérifiés. La borne n12/n14 ne prouve pas la généralité WSPD. |
| Premier raccord incrémental | [Essai publié](../receipts/incremental_full_trial_20260911/README.md), sorties/refus qualifiés puis variante non retenue pour ses coûts. Ne plus demander cet essai ni son intégration comme optimisation. |
| Blocs nommés 50k et lots groupés | [Second auditeur, ad7ffd28](receipts_cache_commit_20260911/README.md), deux verrous clos sur CPU ; premier SIGTERM conservé, ni complétude ni arité finale ni contrat de vitesse. |
| R/U, statique, panne après admission | [Complément précédent](receipts_static_followup_20260911/README.md), diagnostic et rejeu SAN clos. Les preuves mathématiques et captures restent distinctes. |

Le critère de support survivant reste une piste prouvée, sans implémentation ou gain mesuré. Les autres remarques secondaires du [second auditeur](receipts_cache_commit_20260911/README.md) doivent être confrontées aux livraisons plus récentes avant reprise ; leur liste ne devient pas une nouvelle série de blocages.

Archive industrielle et contrats 1 s/100 ms ou massifs restent ouverts. Les prototypes GPU ont leurs reçus propres, hors de cette passe. GCP non utilisé.
