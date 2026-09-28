# Expériences de développement v9

Ces consommateurs sont distincts du moteur qualifié et de ses contrats G4.
`phase=exploration_v9_hors_registre`, `public_status=not_claimed` partout.

- [Banc synthétique calibré](synthetic_bench_20260928/README.md) (28 septembre) :
  huit familles, quatre niveaux **définis par la difficulté mesurée**, quatre
  tailles, cinq nombres de groupes. C'est le banc de référence pour toute
  comparaison à HDBSCAN ; il remplace le banc du 27 septembre, qui mesurait
  presque tout dans un régime où la référence échoue déjà.
- [Clustering pondéré à K fixé](weighted_clustering_20260927/ETAT_COURANT.md) :
  mesure du chapitre 9, raccord des facettes au FULL, condensation et vote.
  Lire cet état courant avant le README historique figé du sous-dossier.
- [Routage ponctuel emboîté](weighted_clustering_20260927/POINT_ROUTING_REFERENCE.md) :
  référence exacte sparse, séparée du vote plat et de son benchmark.
- [Campagne synthétique du 27 septembre](synthetic_clustering_20260927/) :
  première comparaison à HDBSCAN. Ses conclusions sont **suspendues** : l'audit
  du 28 septembre a établi que la marge annoncée vient de la ligne de base gelée
  et non de la machinerie pondérée, qu'elle s'inverse sur une graine sur deux, et
  que le reçu n'est pas rejouable. À refaire sur le banc calibré.
- [Dendrogrammes de points](point_dendrogram_20260927/) : matérialisation des
  dendrogrammes point à point.
