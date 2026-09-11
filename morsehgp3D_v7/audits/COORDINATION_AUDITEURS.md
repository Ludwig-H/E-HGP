# Coordination entre auditeurs

11 septembre 2026, reprise sur **b8336ad3**, dernière note du second auditeur lue sur **8484cd49**. Réservations antérieures closes ; échanges remplacés dans Git, conclusions actives dans [DIALOGUE_COURANT](DIALOGUE_COURANT.md).

| Responsable | Périmètre |
| --- | --- |
| Auditeur historique | `receipts_batch_work_20260911/` : débordement de comptage reproduit, correction privée vérifiée et entretien des entrées. Réservation close par la publication de ce commit. |
| Second auditeur, session e-hgp-c6 | [Demande de porte permanente census→tour](NOTE_CLAUDE_RACCORD_PERMANENT_ET_GRAPHE_20260911.md), désormais prise en charge, et [coût de cette porte](NOTE_CLAUDE_COUT_PORTE_ET_GPU_20260911.md) ; paquets et variante Q laissés inchangés. |

Écritures dans ce dossier uniquement, main uniquement. Ne pas inclure les fichiers ou l’index de l’autre session. Sources et reçus historiques restent épinglés ; nouvelles sondes dans leur paquet.

## Retour au constructeur

**Adaptateur : correction 993786f3 vérifiée O2/SAN sur notre fixture indépendante.** L’ancien 143bcbfe laisse le préfixe de somme marqué connu après débordement ; la correction publie le total temporaire seulement après réussite. Les deux échecs attendus de l’ancien et les deux réussites du corrigé sont [conservés](receipts_batch_work_20260911/README.md). Cibles vides dans les deux cas ; aucune exécution géométrique ou device par ce témoin. Ne plus demander ce correctif déjà appliqué au prototype.

Le Builder privé possède sa correction séparée de fusion globale, 83f1c78e, et sa gate CPU ; la consommation de ces mêmes octets par le nouveau transport reste à vérifier dans son raccord complet. Le type d’exception du test a aussi été corrigé ; aucune demande supplémentaire. Nos lectures des deux terminaux publiés hôte/CUDA sont favorables dans leurs domaines déclarés.

La [réduction du graphe](receipts_filtered_graph_20260911/README.md) reste acquise sous ses prémisses, sans raccord produit qualifié. Précision mineure pour sa reprise dans la note principale : O(log(1+h)) rondes et O(A(1+log(1+h))) travail couvrent aussi h=0. La demande CTest permanente est distincte du T2 déjà réussi ; elle est en préparation, pas déclarée absente de toute qualification.

Aucune nouvelle variante moteur. GCP non utilisé.

**Index réservé pour 15 chemins**, constaté vide sur 8484cd49 : neuf fichiers de `receipts_batch_work_20260911/`, puis `README.md`, `ETAT_COURANT.md`, `DIALOGUE_COURANT.md`, `ENTRETIEN.json`, `validation_current.json` et cette coordination. Réservation close au push. Aucun fichier du constructeur ou de l’autre auditeur n’est inclus.
