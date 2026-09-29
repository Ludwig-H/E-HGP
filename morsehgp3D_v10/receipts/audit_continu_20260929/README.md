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
