# Contre-audit B — groupement exact haché de la phase 0, WIP

24 septembre 2026. Lecture sans build du commit local propre
`d3028ec218a94ec8056f4cde89ad6b2dc26a16be`, frère du prototype
pool E2 `b37b49504` : tous deux partent de `d1d038393`, **aucun ne
contient l'autre**. `full_ball_tower.hpp` lu à SHA-256
`1395d39cf451e65891ec628fc98e68d33d3b48b2c7ed1105c1cb91bede294bd3`,
gate à `607bba19147d072a35d6720e0ecf24e44ed2018730abccc0aa96868e209afd99`.
Ce n'est pas un reçu de performance G4 et aucun gain n'est revendiqué.

## Ce qui est testé

Le groupement haché remplace la préparation triée des requêtes statiques
de 56 octets. Le gate `static_grouping_gate.cpp` confronte directement
`firsts[K]` et **chaque** `targets[K]` des voies hachée et triée aux
largeurs W=1,2,3,4,8 ; il couvre aussi classes à hash faible, mutants,
statuts, compteurs logiques et un condensé de tour. C'est une bonne
porte de la phase 0, plus fine qu'une simple égalité de digests.
Le chemin haché compare les clés complètes même en collision, choisit
l'ordinal minimal par CAS et conserve la voie triée si un résolveur batch
est branché ou si la table dépasse sa limite ; aucune erreur de
groupement déterminée par cette lecture.

Mais le gate libère chaque forêt juste après construction : il ne fait
pas une comparaison octet pour octet de tous les nœuds, parents,
contributions, lots et verticales. Les compteurs `static_peak_*` sont
exclus des égalités, à publier séparément. Les définitions de cas LiDAR
08/000000 K5/K10 sont présentes dans CMake ; le commit ne contient ni
sortie de ces cas, ni reçu apparié, ni répétition de performance. Sa
ligne `static_ms` par paire est indicative, sans seuil ni preuve de gain.
La priorité de refus sur catalogue malformé n'a pas de gate spécifique
dans ce WIP.
Un commentaire du code dit que l'ordre de hash avait coûté +25 % de CPU
de fil à K5 et justifie les seaux par premier site ; aucun log de cette
mesure ne fait partie du commit examiné. Ne pas présenter ce WIP comme
un bénéfice R21/G4 acquis.

## Danger concret à l'intégration avec E2 et E4

Les trois commits frères modifient le constructeur et l'API
`build_full_ball_tower` au **même argument booléen après
`overlap_static`** : E2 l'appelle `persistent_pool` et lui passe
`options.tower_persistent_pool`, ce WIP l'appelle `hash_grouping` et lui
passe `options.tower_hash_grouping`, tandis qu'[E4](CONTRE_AUDIT_B_QUEUE_E4_WIP_20260924.md)
l'appelle `pipelined_tail`. Un assemblage qui conserve un seul
booléen peut compiler tout en branchant silencieusement le mauvais
levier. Il faut trois champs/arguments distincts — de préférence une
structure d'options nommées —, puis un gate 2×2×2
`pool × hash_grouping × pipelined_tail` sur mêmes catalogues : égalité
des `firsts`/`targets`, des objets FULL explicites et du ledger logique,
avec les compteurs physiques correctement redéfinis. Mesurer séparément
temps statique, FULL total, chaîne totale, mémoire et pic de fils ; une
optimisation qui déplace le travail vers l'aval n'est pas acceptée sur
son seul `static_ms`.

La [reconstruction temporelle max-ID](PHASE_A_MAX_ID_COMPOSANTE_20260923.md)
reste une piste ultérieure pour la phase A, non une propriété fournie par
ce groupement. Elle exige les cibles terminales strictement antérieures,
les coupures ouvertes/fermées et tous les ordinaux de facettes ; phases
populations et images restent à produire et vérifier littéralement.

GCP non utilisé dans ce contre-audit.

## Relecture du commit frère `252e6794e` (24 septembre, 22 h 05 UTC)

Ce commit renforce surtout la **porte de test**, pas l'algorithme :
contrôle de statut/témoins complets avant de comparer la sortie,
`path[K]` à trois états (aucune classe, témoin trié, groupement haché),
et comparaison de tous les `firsts` pour chaque ordre haché atteint.
Une erreur de résolution ne devrait plus être rangée abusivement comme
simple échec de plancher de performance. Le mutant qui fait confiance
au seul hash reçoit une garde de compilation propre. Je ne trouve pas
de nouvelle erreur de clé dans le diff incrémental.

Les cas LiDAR FULL K5/K10 et le mutant de hash faible sont enregistrés
dans CMake, mais aucun **reçu d'exécution** de ces portes ni chrono G4
du nouveau SHA n'est joint. La correction du gate n'est donc pas à
confondre avec sa qualification. Le conflit d'argument positionnel
avec E2/E4 décrit ci-dessus subsiste ; un témoin `false` destiné à E4
pourrait désactiver le hash s'il est mal fusionné. Les changements
non commis du worktree n'ajoutent qu'une clarification de commentaire
dans le moteur et une note de provenance ; ils ne font pas un moteur
intégré.
