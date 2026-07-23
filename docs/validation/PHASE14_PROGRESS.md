# Progression Phase 14 — socle industriel Morse H0

## État

Phase `14`, backend `reference_cpu`, profil `hgp_reduced`, mode scientifique `certified`, mode de déploiement `architecture_only`, `public_status=not_claimed`.

La porte d'architecture est satisfaite par la fermeture conditionnelle de Phase 10. La porte de qualification finale reste bloquée par la Phase 11, M.1, la voie CUDA complète et le protocole p95.

## Incrément 14A livré — planification résidente et streaming

`ExactDirectMorseIndustrialPlanResult` rejoue fraîchement 10.1 et 10.2, joint leurs lots et calcule exactement les nombres de minima, simplexes du catalogue de Gabriel complet, bras et références de clés. Il certifie des bornes conservatives de nœuds et d'enfants, puis applique un modèle d'octets fourni par l'appelant et une réserve explicite de sommets de descente.

Le profil `interactive_resident_50k` exige au plus 50 000 points et un unique chunk contenant tous les lots. Le profil `massive_external_streaming` agrège des lots consécutifs sans jamais couper un couple ordre--niveau; chaque frontière de chunk devient une frontière de checkpoint et de run externe. Un lot indivisible qui dépasse un plafond rejette atomiquement tout le plan.

Le plan conserve seulement des intervalles dans le journal 10.1 et des compteurs. Il ne copie ni clé, ni facette, ni niveau exact, ne construit aucune descente ou forêt, et ne matérialise ni cellule, ni coface, ni Gamma, ni mosaïque de Delaunay d'ordre supérieur. Son prédicat recalcule en une passe les intervalles, agrégats, bornes, réserves, octets, caps et frontières. Son acceptation ne signifie ni p95 inférieur à une seconde, ni capacité démontrée à dix millions de points.

## Priorités de développement

1. remplacer la copie de tous les parents du locator à chaque lot par une transaction sparse;
2. grouper les descentes par cardinal et difficulté pour une exécution GPU;
3. rendre explicites les durées de vie des arènes du rejeu frais;
4. ajouter l'instrumentation `warm_e2e` seulement après ces réductions structurelles;
5. rendre les chunks et checkpoints durables avant toute campagne massive.

Ces priorités optimisent le chemin démontré. Elles ne réintroduisent ni les gateways historiques, ni un oracle combinatoire dans l'architecture produit.

## Validation

Les deux bibliothèques nouvelles compilent en Release avec les avertissements stricts. L'unique CTest partagé avec la fermeture de Phase 10 passe en 0,03 seconde et construit sur E5 les politiques résidente et streaming; une identité de chunk mutée invalide le prédicat structurel. Aucune campagne de performance n'est lancée.

Une falsification coordonnée pourrait encore redistribuer des compteurs entre chunks tout en conservant leurs agrégats et octets. Le payload compact ne contient volontairement ni détail ni digest par lot; un vérificateur frais contre 10.1--10.2 est donc requis avant persistance ou exécution industrielle. Cette dette P2 est compatible avec `architecture_only`, mais devra être fermée avant toute qualification.

## GCP

GCP non utilisé.
