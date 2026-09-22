# Contre-audit B — parallélisme q3/q4 et FULL du premier moteur v9

22 septembre 2026. Lecture seule du moteur `d2700314` (les sources citées sont
inchangées dans le worktree examiné). Cadre : `quantized_u18_input_only`,
`reference_cpu`, `not_claimed`. La [première campagne](../receipts/first_tower_20260922/README.md)
porte sur trois trames **d'une seule séquence** SemanticKITTI 08, sans sol,
à 1 mm : W8 sur un hôte local à **quatre cœurs physiques**, et FULL dans sa
voie temporelle à un fil. Ni GCP ni G4 ni GPU n'ont été utilisés. Ce ne sont
pas des chronos du profil float32 original ou du contrat principal sur brut.

## Où sont les tâches et les barrières

| Unité | Indépendance exacte possible | Verrou du code actuel |
| --- | --- | --- |
| Rectangle WSPD, plage de paires | Le front couvre des produits disjoints ; les masques q3/q4 restent séparés. | Après les témoins de rectangle, chaque paire résiduelle est **explicitement** développée. La file commune à mutex distribue des plages de rangs A, mais une arête n'est jamais interrompue ; `parallel_task_pairs=256` n'est pas un plafond si une seule rangée B est plus grande. [Pipeline](../src/gen/pipeline/wspd_q34.cpp#L395-L438), [file](../src/gen/pipeline/wspd_q34.cpp#L738-L849). |
| Arête propriétaire | Son cover et son atlas sont privés, et les arêtes peuvent être distribuées sans partager de compteur géométrique. | Une arête construit son cover, puis éventuellement **un atlas commun** aux certificats q3 et au balayage q4. Ne pas dupliquer cet atlas en séparant naïvement les deux voies. [Pipeline](../src/gen/pipeline/wspd_q34.cpp#L481-L533), [cover](../src/gen/lanes/edge_cover.cpp#L73-L115). |
| Graine q3 ; graine × cellule q4 | Après construction d'un atlas immuable complet, chaque graine q3 peut avoir son census/coquille privés ; chaque incidence q4 peut avoir ses événements et sa coquille privés, sous la règle exacte de propriété des cellules. | L'atlas se construit récursivement du parent vers quatre enfants ; la feuille inachevée est raffinée **avant** les graines. `LiveOnly` parcourt ensuite les graines de la même arête en série. Le balayage q4 trie les événements et consomme chaque groupe de racines égales avec son compte intérieur : ni événement ni fragment partiel n'est une tâche autonome. [Atlas](../src/gen/lanes/q4_local.cpp#L197-L266), [LiveOnly](../src/gen/lanes/q4_local.cpp#L570-L666), [balayage](../src/gen/lanes/q4_local.cpp#L331-L425). Les enfants/refinements recopient les frontières actives en `shared_ptr`/`vector`, source de trafic et de résidence [partition](../src/gen/lanes/q4_local_partition.cpp#L295-L309). |
| Clé de boule | Le recensus de chaque BallKey distincte est déjà parallèle avec états privés. | Les callbacks accumulent les présentations par worker, les recopient dans `all`, trient globalement par clé/support puis regroupent avant recensus ; le nombre de clés et l'ordre déterministe forment une barrière. [Chaîne](../src/chain/tower_chain.cpp#L219-L312), [recensus](../src/chain/tower_chain.cpp#L329-L407). |
| Bloc d'un niveau FULL ; ordre K | Les résolutions géométriques des blocs d'un **même niveau exact** pourraient lire l'état figé précédant ce niveau. | Le code prépare les blocs séquentiellement ; `root()` comprime en place, cache et statistiques sont mutables. La fermeture réunit par DSU **tous** les blocs reliés par parents, puis publie les ancres seulement après le lot entier. Les niveaux et les K sont chronologiques ; K+1 lit histoire et ancres de K. [Boucle](../src/tower/forest/full_ball_tower.hpp#L298-L349), [mutations](../src/tower/forest/full_ball_tower.hpp#L261-L275), [root](../src/tower/forest/full_ball_tower.hpp#L537-L545), [fermeture](../src/tower/forest/full_ball_tower.hpp#L990-L1080). |

La voie optionnelle `tower_static_threads>0` prépare, trie et matérialise
toutes les requêtes/cibles d'un ordre, puis parallélise leur résolution ;
elle ne parallélise ni la fermeture des lots ni les K et n'était pas activée
dans la campagne ([source](../src/tower/forest/full_ball_tower.hpp#L754-L849),
[défaut](../src/chain/tower_chain.hpp#L41-L47)). L'index des clés, le tri des
niveaux et les programmes par K exigent déjà le catalogue global
([source](../src/tower/forest/full_ball_tower.hpp#L443-L529)).

## Priorités dictées par le reçu, pas par un gain q2

| K | q3/q4, secondes mur | FULL, secondes mur | Paires q3/q4 développées | Covers construits | BallKeys distinctes |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5, trois trames | 120–247 | 8,9–12,9 | 12,0–23,7 M | 1,73–2,24 M | 1,10–1,41 M |
| 10, trois trames | 278–687 | 94–130 | 17,5–32,8 M | 3,67–4,93 M | 4,38–5,51 M |

Les chiffres viennent des [six JSON bruts](../receipts/first_tower_20260922/raw/)
et sont une répétition par cas. À K10, q2 dure 2,7–5,1 s, la fusion
1,35–1,53 s et le recensus 3,2–4,1 s : leur temps actuel est secondaire,
mais leur **résidence** ne l'est pas. Les présentations totales `P` dépassent
`B` de seulement 2 à 13 à K10 : ne pas compter sur la déduplication pour
réduire massivement le catalogue de ces trames. `BallData` occupe 224 octets,
soit 1,235 Go pour les 5 512 670 clés de 08/000000 K10 ; les slots et
`all` coexistent à au moins `224P` octets de capacités de présentations
([analyse de résidence](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)).
Sur cette même ligne, FULL paie 17,39 M représentants, 403,7 M nœuds
`intruder` et 1,065 milliard de tests MEB. Cela justifie un second chantier
après q3/q4 ; ce n'est pas une prédiction de temps G4.

**P0 travail q3/q4.** Mesurer puis réduire le nombre de paires survivantes,
de covers, de classifications de fragments Z et d'incidences graine-cellule.
Distribuer d'abord les graines d'une arête lourde **après** son atlas partagé,
avec parent possédé et tampons privés, plutôt que multiplier les covers.
Le certificat atlas rejette aussi des graines q3 avant leur construction de
boule ([source](../src/gen/pipeline/wspd_q34.cpp#L535-L553)) : un raccourci
« aucune graine q4 » qui supprime l'atlas doit compter les constructions q3
réintroduites, pas seulement le temps q4 économisé. Une fenêtre de tâches
borne les octets vivants, jamais le nombre de candidats ni les sorties.

**P0 résidence / P1 temps aval.** Des runs privés bornés et une fusion exacte
des clés peuvent supprimer la copie simultanée des présentations ; il faut
toujours valider toutes les incidences/profondeurs/coquilles et attribuer des
BallIds déterministes. Pour FULL, résoudre par fenêtres contre l'état
pré-niveau **figé**, avec cache/compteur privés ou consultation de racine
non mutante, puis fermer et publier le niveau entier. Le cache temporel
actuel réserve `48·nextpow2(16n)` octets et efface toutes ses cases à chaque
K>1 ([source](../src/tower/forest/full_ball_tower.hpp#L220-L250)) ; il est
facultatif pour l'exactitude, pas gratuit en mémoire/bande passante.

## Porte de décision avant G4

Le code produit est aujourd'hui **CXX seul** ([CMake](../CMakeLists.txt#L1-L8)) ;
les prédicats générateur exigent `__int128` CPU
([types](../src/gen/core/types.hpp#L31-L40)). Les `vector`, `shared_ptr`,
callbacks, récursions d'atlas, tri d'événements et sorties variables ne
constituent pas des buffers ou kernels GPU. Un port crédible emploie des
vagues à offsets disjoints, filtres conservateurs à largeur prouvée, compactage
et repli exact des cas incertains ; CPU conserve d'abord catalogue et
fermeture des lots. Aucun temps ou coût de transfert G4 n'est validé.

Le résultat `WspdQ34ParallelResult` possède déjà des compteurs détaillés
témoins, cover, q3 seeds/atlas/replis, q4 partition/frontière, cellules,
files et timings par worker ([définition](../src/gen/pipeline/wspd_q34.hpp#L47-L93),
[réduction](../src/gen/pipeline/wspd_q34.cpp#L838-L868)), mais la sonde de
chaîne n'en publie que `expanded_pairs`, `cover_builds` et les deux émissions
([source](../src/chain/tower_chain.cpp#L262-L279)). Prochaine mesure minimale :
publier ce ledger, plus temps et coût maximal/quantiles **par arête**
(cover, atlas, q3, q4), tailles de file et attente workers ; par K/niveau
FULL publier nombre de blocs, taille du plus gros lot, temps résolution/DSU,
cache slots/hits/reset et pic mémoire par phase. Comparer d'abord les mêmes
sorties exactes et le même travail W1/W8, puis une ablation de tâches
graine-cellule sur une trame K5 et K10 appariées. Exiger temps mur complet,
octets hôte/device, H2D/D2H, kernels et replis avant toute affirmation G4 ;
trois trames de la seule séquence 08 ne prouvent ni sous-quadraticité ni
contrat FULL sur plusieurs scènes.
