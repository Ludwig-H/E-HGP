# Captures de l'audit continu v10

Sources, captures et contre-exemples du 29 septembre 2026, relocalisés depuis
`audits/audit_continu_20260929/` à la demande de rangement de l'utilisateur.
Les 34 fichiers déplacés sont conservés **octet pour octet** ; le
[manifest](RELOCALISATION.json) donne ancien chemin, nouveau chemin et SHA256.
Les rapports courants restent dans [audits/](../../audits/AUDIT_ETAT_COURANT.md).

Les commandes, diagnostics et chemins absolus inscrits dans les JSON décrivent
leur exécution historique ; ils ne sont pas réécrits pour paraître récents.
Cette relocalisation n'est ni un rejeu des campagnes ni une qualification
supplémentaire. Les essais interrompus restent identifiés comme tels.

- `catalogue/` : contre-oracle, feuilles, entrée tronquée, discontinuité cover.
- `pool_head/` : sécurité du pool, coût de la tête, verticales, multi-K et bande.
- `projection_band/` : petit oracle Fraction autonome, normal et `-O`.
- `timeout/` : enfant survivant au délai et réplique hôte CUDA historique.
- `RELECTURE_COORDINATION.json` : relecture fermée sur les chemins de l'époque.

Les scripts catalogue peuvent écrire des sorties : les rejouer uniquement
depuis une **copie dans un nouveau répertoire temporaire**, avec leurs chemins
de capture adaptés. Ne jamais lancer un script écrivain sur cette archive.
La profondeur relative au dossier v10 est inchangée ; les imports locaux
restent groupés. `timeout/reproduce.py` retrouve donc encore le même moteur.
Une nouvelle exécution doit produire un reçu distinct, avec sa propre version.

## Captures ajoutées après la relocalisation

- `math_catalogue_cover/` : oracle exact autonome de toutes les composantes
  couvrantes, K1/K2/K3, normal et `-O` ; ni générateur ni tour native.
- `thesis_boundary/` : provenance de la relecture des parties I et II et
  petit contre-exemple abstrait du vote recalculé à chaque coupe.
- `pool_corrected/` : refus réel de création partielle de fils sur copie
  corrigée ; ne qualifie pas les consommateurs ou l'intégration.
- `timeout/cuda_corrected/` : contrôle arithmétique hôte UBSan de la sonde
  unsigned ; aucun chrono ou reçu GPU nouveau.

Ces nouveaux fichiers ne font pas partie des 34 déplacements historiques
du manifeste. Les comptes, sources et limites sont dans leurs reçus propres
et dans les rapports pointés par l'état courant. GCP non utilisé.
