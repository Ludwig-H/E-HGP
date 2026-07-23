# Progression Phase 14 — socle industriel Morse H0

## État

Phase `14`, backend `reference_cpu`, profil `hgp_reduced`, mode scientifique `certified`, mode de déploiement `architecture_only`, `public_status=not_claimed`.

La porte d'architecture est satisfaite par la fermeture administrative de Phase 10 comme implémentation candidate conditionnelle. Sa fidélité globale des carriers reste une obligation de preuve; la porte de qualification finale reste en outre bloquée par la Phase 11, M.1, la voie CUDA complète et le protocole p95.

## Incrément 14A livré — planification résidente et streaming

`ExactDirectMorseIndustrialPlanResult` rejoue fraîchement 10.1 et 10.2, joint leurs lots et calcule exactement les nombres de minima, simplexes du catalogue de Gabriel complet, bras et références de clés. Il certifie des bornes conservatives de nœuds et d'enfants, puis applique un modèle d'octets fourni par l'appelant et une réserve explicite de sommets de descente.

Le profil `interactive_resident_50k` exige au plus 50 000 points et un unique chunk contenant tous les lots. Le profil `massive_external_streaming` agrège des lots consécutifs sans jamais couper un couple ordre--niveau; chaque frontière de chunk devient une frontière de checkpoint et de run externe. Un lot indivisible qui dépasse un plafond rejette atomiquement tout le plan.

Le plan conserve seulement des intervalles dans le journal 10.1 et des compteurs. Il ne copie ni clé, ni facette, ni niveau exact, ne construit aucune descente ou forêt, et ne matérialise ni cellule, ni coface, ni Gamma, ni mosaïque de Delaunay d'ordre supérieur. Son prédicat recalcule en une passe les intervalles, agrégats, bornes, réserves, octets, caps et frontières. Son acceptation ne signifie ni p95 inférieur à une seconde, ni capacité démontrée à dix millions de points.

## Incrément 14B livré — transaction locator proportionnelle aux unions

`ExactDirectSparsePositiveFacetLocator::apply_batch` ne copie plus l'arène des $H$ parents. Après toutes les requêtes pré-lot, un journal réservé à $U_b$ handles enregistre seulement les $W_b$ racines effectivement réorientées, avec $W_b\leq\min(U_b,H-1)$ pour $H>0$. La compatibilité des doublons lit cet état candidat. Un succès conserve exactement $W_b$ écritures; un rejet post-unions restaure exactement ces $W_b$ racines en ordre inverse.

Les diagnostics éphémères distinguent écritures transactionnelles, écritures de rollback et pic d'entrées du journal. Ils n'entrent ni dans les compteurs durables, ni dans le digest, de sorte que le schéma scientifique et ses vecteurs canoniques restent inchangés. Le scratch du lot devient $O(Q+KB+U_b)$ au lieu de $O(Q+KB+H)$; aucune nouvelle structure globale n'est construite.

## Incrément 14C livré — lanes de descentes par structure exacte

Le nouveau planificateur rejoue 14A sous un plafond explicite de chunks, contrôlé avant chaque rétention avec abandon du préfixe transitoire en cas de dépassement, puis classe les familles de chaque lot exact par cardinal de support deux, trois ou quatre, soit au plus trois lanes. Une lane reste dans un seul chunk et un seul lot; elle conserve seulement les intervalles candidats, les cardinaux et les comptes de graines, lancements et examens locaux. Elle n'alloue ni clés, ni facettes, ni permutation durable de bras.

Le plan impose un snapshot locator pré-lot commun, une sélection stable unique, une fermeture 10.5c et une mémoïsation partagées par lot, puis une jointure par `arm_seed_index`. La tuile de lancement est bornée avant allocation. Le pire coût initial autonome publié vaut quatre passages sur au plus 385 supports par graine à $K=10$; la fixture frontière à support deux porte donc deux graines et 3080 examens. La fermeture complète, la difficulté LBVH ou rationnelle et les temps GPU ne sont pas bornés.

`complete_architecture_plan()` est explicitement un contrôle de forme. `fresh_reconstruction_required_before_execution=true` exige le vérificateur frais avant exécution ou persistance. Le plan reste `architecture_only`, `public_status=not_claimed`, sans revendication 50 k ou 10 M+.

## Priorités de développement

1. exécuter la sélection, la fermeture partagée et la jointure des lanes 14C dans une tranche commune aux deux profils;
2. séparer les propositions GPU de leur rejeu CPU exact et ajouter les classes de difficulté observée;
3. rendre explicites les durées de vie des arènes du rejeu frais;
4. ajouter l'instrumentation `warm_e2e` seulement après ces réductions structurelles;
5. rendre les chunks et checkpoints durables avant toute campagne massive.

Ces priorités optimisent le chemin démontré. Elles ne réintroduisent ni les gateways historiques, ni un oracle combinatoire dans l'architecture produit.

## Validation

Les deux bibliothèques nouvelles compilent en Release avec les avertissements stricts. L'unique CTest partagé avec la fermeture de Phase 10 passe en 0,03 seconde et construit sur E5 les politiques résidente et streaming; une identité de chunk mutée invalide le prédicat structurel. Aucune campagne de performance n'est lancée.

Pour 14B, la cible locator compile avec les avertissements stricts et son unique CTest ciblé passe en 0,01 seconde. Les deux fixtures déjà discriminantes vérifient respectivement une écriture conservée et une écriture restaurée; le checker statique interdit le clone dense et impose la réservation puis l'écriture journalisée. Aucun benchmark, GPU ou test long n'est lancé.

Pour 14C, les deux CTests ciblés passent sous GCC Release strict en 0,16 seconde et sous Clang Release strict en 0,14 seconde. Ils couvrent le plan vide sans selle, les trois lanes du tétraèdre, les plafonds de chunks, lanes, lancements et examens juste insuffisants, l'absence de chunk source partiel après rejet, la tuile nulle, plusieurs chunks, deux lanes de supports distincts dans un même batch avec barrière commune, une lane falsifiée et le bord collinéaire $K=10$ à 3080 examens initiaux. Le CTest forêt séparé exerce la vraie fixture E5 tridimensionnelle, sa liaison `AC` vers la composante de `DE`, l'identité causale de cette racine avec la continuation exacte `ABC` et le rejeu frais. L'installation, l'export de la cible et le consumer externe passent 1/1. Aucun benchmark, GPU, sanitizer ou oracle long n'est lancé.

Une falsification coordonnée pourrait encore redistribuer des compteurs entre chunks 14A ou lanes 14C tout en conservant leurs agrégats. Les payloads compacts ne contiennent volontairement ni détail ni digest par lot; un vérificateur frais contre 10.1--10.2 est donc requis avant persistance ou exécution industrielle. Cette dette P2 est compatible avec `architecture_only`, mais devra être fermée avant toute qualification.

## GCP

GCP non utilisé.
