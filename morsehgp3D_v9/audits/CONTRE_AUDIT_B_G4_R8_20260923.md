# Contre-audit B — G4 R8, occupation q3/q4 et plancher du chemin courant

23 septembre 2026. Source : [reçu R8](../receipts/g4_tower_r8_20260923/README.md),
`SUMMARY.json`, bruts `vm/` et `host/`, paquet `515b3666`. Cadre :
`exploration_v9_hors_registre`, `reference_cpu`, grille entière 1 mm/u18,
`not_claimed`.

## Réception et portée

Les **336/336** fichiers de l'archive satisfont `SHA256SUMS`. Les 153 entrées
non-données du manifeste worker ont été recalculées depuis les blobs Git du
paquet, sans écart ; les trois charges de scène ont les SHA annoncés. Le
lecteur **épinglé à `515b3666`**, et non le WIP plus récent du worktree, relit
les bruts avec le même verdict en Python normal et sous `-O` : hôte et VM
`completed`, **20/20** cas `complete_relative`, codes 0, préflight natif non
vacant. L'arrêt ciblé de la G4 est relu `TERMINATED`. Les archives tar
transitoires non versionnées ne peuvent pas être re-hachées depuis le dépôt.

Ce sont trois trames **entières après masque sans sol**, 08/000000, 000100,
000200 (39 885, 35 551, 45 845 sites), toutes dans la **même séquence**.
Segmentation et préparation Python ne sont pas incluses. La G4 utilisait
**48 fils CPU sur 24 cœurs physiques** ; **aucun noyau GPU** n'a été exécuté.
Le digest de contrôle est exclu de `chain_s`, mais pas de l'exécution externe.
Les huit comparaisons entre nombres de fils sont des projections fortes de
l'objet (sites, catalogue, Euler, ordres, digest 64 bits), pas des comparaisons
octet par octet des structures intégrales.

## Comptes et temps (s8, W48, répétition 0)

| Trame 08/ | K | Chaîne s | q3/q4 s | Chaîne hors q3/q4 s | Tour s | Attente q3/q4 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 | 5 | 3,665 | 2,518 | **1,147** | 0,699 | 35,1 % |
| 000000 | 5 | 5,460 | 3,953 | **1,507** | 0,866 | 45,3 % |
| 000200 | 5 | 6,395 | 4,710 | **1,685** | 0,921 | 48,3 % |
| 000100 | 10 | 9,403 | 5,241 | **4,162** | 2,972 | 16,3 % |
| 000000 | 10 | 13,690 | 8,032 | **5,658** | 3,895 | 28,4 % |
| 000200 | 10 | 15,192 | 9,759 | **5,433** | 3,593 | 34,9 % |

Les répétitions W48/s8 sont doubles ; les ablations W1/W24 et s10/s12 n'ont
qu'une répétition. Sur 000100/K5, q3/q4 passe de 42,501 s à W1 à 3,570 s
à W24 puis 2,518 s à W48 : accélération W1→W48 **×16,9**, mais son CPU
cumulé augmente de 42,5 à 77,3 CPU·s. À W48, environ 35–49 % du temps des
fils q3/q4 est passé à attendre la file pour K5. Les 768/769 jobs initiaux
de front et les ~0,94–2,16 millions de plages publiées par cas montrent que
le problème n'est pas l'absence totale de tâches : la distribution de la
durée des **jobs de front et des plages** reste à mesurer. [Rectification
du code](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md) : tout rectangle
survivant **propose** sa première plage à la file, même sous 256 paires ;
256 règle le grain du découpage de A, pas l'admission de la tâche. Une
file pleine peut refuser l'offre et provoquer le développement en ligne.

Le coût n'est pas qu'un temps d'attente : sur 000100/K5, le grand-livre
compte **11,96 M** de paires développées, **193,15 M** de formes de sites
préparées par le cœur, **795,94 M** de tests uniformes, **161,45 M** de
visites de nœuds du filtre rectangle, **287,57 M** du filtre par paire et
**330,74 M** de la couverture du cœur. Ces catégories se recouvrent ou
décrivent des étapes différentes : ne pas les additionner comme des
opérations homogènes. À K10, les trois catalogues contiennent déjà
**4,38–5,51 M** de boules et les pics RSS **3,84–4,78 Gio** pour seulement
35–46 k sites. Le contrat à plusieurs dizaines de millions de points
exige donc un plan explicite de mémoire/résidence et de flux de sortie ;
une extrapolation linéaire de ces chiffres n'est pas une prévision fiable.

## Ce que ces chiffres imposent au développement

1. **Ordonnancement q3/q4 : nécessaire, non suffisant.** Réclamer les jobs
   lourds tôt, fractionner les longues descentes et publier les durées
   maximales **des jobs et des plages**, ainsi que leurs fins respectives,
   sont des essais justifiés. Mais même si q3/q4 devenait
   instantané *sans changer les autres phases*, le chemin mesuré resterait
   à **1,147–1,685 s à K5** ; le contrat de 1 s ne peut venir du seul
   rééquilibrage q3/q4. Cette soustraction est un contrefactuel du chemin
   courant, pas une borne mathématique sur une autre architecture.
2. **Réduire le travail q3/q4 avant expansion.** Le certificat exact par
   rectangle WSPD et cellules de centres, ou une autre agrégation, doit
   juger le coût total : cellules/témoins, paires ou covers restants,
   formes, clés, recensus et FULL. La [boîte LiDAR
   fixe](lidar_density_bbox_fixed_20260923/README.md) montre une croissance
   des formes effectivement préparées que la seule sortie ne force pas.
   Déplacer les mêmes milliards de formes vers une autre étape ou le GPU
   ne résoudrait pas le verrou sous-quadratique.
3. **Refondre aussi la tour aval.** À K10 elle prend déjà 2,97–3,90 s,
   dont 1,12–1,46 s de phase statique et 0,80–1,19 s de lots séquentiels
   par ordre. Une accélération q3/q4 ne suffit donc pas même idéalement.
   Tester une reconstruction de composantes/parents parallèle avec
   égalités de niveau fermées ensemble, IDs et digest inchangés.
4. **Ne pas gonfler la preuve.** Euler tient sur les ordres contrôlables
   (K1..3 pour K5, K1..8 pour K10) et les digest R8 égalent R7b, mais
   Euler est seulement nécessaire et `complete_relative` ne démontre pas
   l'absence d'une `BallKey` entièrement omise. Les s10/s12 sont plus
   lents que s8 sur cette scène K5, sans preuve universelle pour s8.

Le jalon G4 **GPU**, la trame brute entière avec sol, la diversité des
séquences et la preuve de complétude absolue restent ouverts ; aucun contrat
1 s ou 100 ms n'est acquis.
