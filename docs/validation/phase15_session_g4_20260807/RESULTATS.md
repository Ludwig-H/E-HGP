# Session G4 du 7 août 2026 — résultats

Machine : `ehgp-blackwell-spot-ai1a`, `g4-standard-48` SPOT, NVIDIA RTX PRO 6000
Blackwell Server Edition (97 887 MiB, capacité de calcul 12.0), CUDA 12.9, 48
cœurs, 176 Go. Dépôt au commit `31b6440`, construit **sur l'hôte** — `nvcc` est
présent nativement, donc ni image conteneur ni spécification CDI n'ont été
nécessaires, contrairement à ce que le protocole de session prévoyait. Les
mesures portent sur **50 000 points**, taille du contrat.

Session ouverte à 20:46 UTC, fermée à 21:30 UTC, deux coupe-circuits armés et
vérifiés, arrêt certifié par relecture indépendante (génération
`6118353088541925230`).

Ordre d'exécution prévu et suivi : **Q1′, Q3, Q1, Q2**.

---

## Q3 — le catalogue terminal paire tient le rang onze. Répondu : oui.

`gpu_morton_yao48_device_tiled_pair_frontier_qualification --point-count 50000
--maximum-closed-rank 11 --prune-semantics closed_rank_window`

| grandeur | valeur |
| --- | --- |
| `success` | `true` |
| `coverage_partition_complete` | `true` |
| `complete_anchor_count` | 50 000, `censored_anchor_count` 0 |
| `unresolved_pair_mass` | 0 |
| `candidate_record_count` | 7 962 604 |
| `certified_pruned_pair_mass` | 1 242 012 396 |
| `unordered_pair_universe_count` | 1 249 975 000 |
| `launcher_ns` | **2,434 s** |
| `cpu_recertification_ns` | 8,628 s |
| `every_record_validated` | `true` (7 962 604 records, 0 sauté) |
| `physical_node_visit_count` | 32 875 936 |

La partition est **exacte** : 7 962 604 + 1 242 012 396 = 1 249 975 000, soit
l'univers entier des paires non ordonnées de 50 000 points, sans reste.

Le plafond de six du consommateur était donc bien une **frontière de
qualification** et non une contrainte mathématique : l'étage paire est complet
au rang du contrat ($K=10 \Rightarrow$ rang fermé 11). Le champ d'audit
`closed_rank_window_natively_qualified` peut passer au vrai.

Réserve honnête : `every_prune_fully_recertified` reste `false` — 12 620 élagages
intégralement recertifiés et 4 096 échantillonnés sur 8 102 972 régions. La
complétude de la partition est certifiée ; la recertification exhaustive de
chaque élagage ne l'est pas.

### Le prix de $K=10$ contre $K=5$, mesuré

Balayage `--all-ranks` à 50 000 points. Les dix rangs terminent, tous
`coverage_partition_complete`, 103,3 s au total.

| rang fermé | $K$ | lanceur (s) | recertification (s) | candidats | µs / record |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1 | 0,666 | 1,557 | 1 224 232 | 1,27 |
| 3 | 2 | 1,104 | 2,463 | 2 126 603 | 1,16 |
| 4 | 3 | 1,787 | 3,272 | 2 957 204 | 1,11 |
| 5 | 4 | 2,288 | 4,073 | 3 745 191 | 1,09 |
| **6** | **5** | **3,100** | **4,834** | 4 500 332 | 1,07 |
| 7 | 6 | 3,740 | 5,605 | 5 228 201 | 1,07 |
| 8 | 7 | 4,703 | 6,399 | 5 934 480 | 1,08 |
| 9 | 8 | 5,247 | 7,142 | 6 621 512 | 1,08 |
| 10 | 9 | 6,578 | 7,910 | 7 299 513 | 1,08 |
| **11** | **10** | **7,480** | **8,756** | 7 962 604 | 1,10 |

Trois lectures.

Le lanceur croît comme $\text{rang}^{1,42}$ ; les candidats croissent presque
linéairement en rang. **$K=10$ coûte 2,03 fois $K=5$** pour l'étage paire
(16,36 s contre 8,06 s, lanceur + construction + recertification). Le contrat
n'oblige donc pas à retomber sur $K=5$ : le facteur en jeu est deux, pas un ordre
de grandeur.

La **recertification est strictement linéaire en records** — 1,07 à 1,27 µs par
record sur toute la plage, une constante. Elle est aussi **mono-thread** : le run
isolé disposait de 40 cœurs et le balayage de 28, et la recertification du rang 11
y vaut 8,628 s contre 8,756 s, soit 1,5 % d'écart. Elle représente 54 % du coût
de l'étage paire au rang 11 et n'utilise qu'un cœur sur quarante-huit.

---

## Q1′ — le pipeline atteint-il l'aval à 50 000 points avec $K=2$ ? Répondu : non.

`morsehgp3d_direct_morse_product_runner --point-count 50000 --maximum-order 2
--mode complete_resident_diagnostic --budget-profile unbudgeted_industrial
--operational-deadline-ms 900000`

| étage (ms) | 900 s | 60 s |
| --- | ---: | ---: |
| `generation` | 45,1 | 45,7 |
| `lbvh` | 15,3 | 15,1 |
| `canonicalization` | 3,1 | 3,1 |
| **`pair_support`** | **899 936,5** | **59 936,1** |
| `higher_support` | 0,0 | 0,0 |
| `batch_plan`, `reducer_setup`, `reducer_stream` | 0,0 | 0,0 |

`stop_category = operational_deadline`, arrêt **dans l'étage paire**. L'aval n'est
pas approché.

### La sonde réfute l'hypothèse qui la motivait

Le pari était qu'à $K=2$ — rang fermé 3 — les candidats admis s'effondrent, donc
l'étage paire passe. La mesure dit le contraire.

| instant | paires certifiées | couverture de l'univers |
| --- | ---: | ---: |
| 60 s | 19 000 000 | 1,52 % |
| 900 s | 159 550 000 | 12,76 % |

Quinze fois plus de temps ne rend que 8,40 fois plus de paires : le débit se
dégrade, les paires croissent comme $t^{0,786}$. Extrapolée, la partition
complète sur l'hôte demande **12 359 s, soit 3 h 26**.

Interpolée à 299,9 s, cette courbe donne 5,38 % de couverture ; la mesure scellée
au rang 6 en donnait 5,78 % dans le même délai. **Le mur de l'étage paire hôte
est indépendant de l'ordre** : descendre $K$ de 5 à 2 ne déplace rien. La sonde a
coûté un run et a supprimé une voie.

---

## Ce que Q1′ et Q3 disent ensemble : chercher et certifier

Les deux étages paires calculent la même partition du même univers de
1 249 975 000 paires à 50 000 points.

| | lanceur natif (device) | session paire (hôte) |
| --- | ---: | ---: |
| partition complète, rang 11 | **2,434 s** | 12 359 s extrapolés (rang 3) |
| visites de nœud | 32 875 936 | 38 750 239 (à 12,76 %) |
| coût par visite | **74 ns** | 23,2 µs |
| rapport | | **5 078 ×** |

Le coût hôte se décompose : 2 209 991 917 prédicats exacts en 900 s, soit
**407 ns par prédicat** et **57 prédicats par visite de nœud**. L'hôte paie de
l'arithmétique rationnelle non bornée à chaque visite ; le device n'en paie
aucune.

Mais l'hôte n'est pas lent en soi. Dans le run Q3, c'est **l'hôte** qui
recertifie les 7 962 604 records du device, en 8,628 s, soit **1,08 µs par
record**. L'hôte est lent à **chercher**, pas à **certifier**.

D'où le chiffre qui compte :

> **étage paire complet et certifié à 50 000 points au rang du contrat =
> 2,434 s (device) + 8,628 s (recertification hôte mono-thread) = 11,06 s**,
> contre 3 h 26 pour l'hôte seul.

Le contrat de 1 s reste dépassé, mais l'écart est passé de quatre ordres de
grandeur à un seul, et 78 % du reste est une boucle linéaire mono-thread sur
quarante-huit cœurs.

---

## Q2 — calcul ou transport ? Répondu par accident, et dans le sens du transport.

Le profil natif que Q2 réclamait n'est pas outillé :
`gpu_exact_higher_support_product_cuda_qualification` n'accepte aucune option et
scelle `kernel_launch_count == 3`, c'est une qualification de composant à taille
fixe. Le plan interdisait de construire l'instrument sur une VM facturée.

Mais la comparaison du rang 11 **isolé** et du rang 11 **comme dixième d'un
balayage** répond à la question sans instrument dédié :

| compteur | isolé | dans le balayage |
| --- | ---: | ---: |
| `physical_node_visit_count` | 32 875 936 | 32 875 936 |
| `candidate_record_count` | 7 962 604 | 7 962 604 |
| `certified_pruned_pair_mass` | 1 242 012 396 | 1 242 012 396 |
| `tile_count` / `chunk_count` | 13 / 27 | 13 / 27 |
| `cpu_recertification_ns` | 8,628 s | 8,756 s |
| **`launcher_ns`** | **2,434 s** | **7,480 s** |

Tous les compteurs de travail sont **bit-à-bit identiques**, le temps hôte l'est
à 1,5 % près, et le temps du lanceur varie d'un **facteur 3,07**. Le temps de
lanceur n'est donc pas déterminé par le travail qu'il effectue : à travail
rigoureusement égal il triple. C'est l'indication que cherchait Q2 — le coût vit
dans l'occupation et les allers-retours, non dans l'arithmétique — et elle
confirme rétrospectivement pourquoi la réfutation du filtre fp64 était le bon
verdict.

La cause exacte (fréquences soutenues, réutilisation d'arène, ordonnancement des
lancements) reste à attribuer, et cela demande l'instrument absent.

---

## L'étage higher ne termine pas, à aucune des tailles essayées

Deux exécutions du runner avec `--higher-backend device_tiled_session
--higher-verification-basis tile_certified`, délai 240 s, profil sans budget :

| $n$ | `pair_support` | `higher_support` | événements acceptés | `stop_category` |
| ---: | ---: | ---: | ---: | --- |
| 400 | 666 ms | 239 369 ms | **0** | `operational_deadline` |
| 1 000 | 2 603 ms | 237 411 ms | **0** | `operational_deadline` |

À quatre cents points, l'étage paire finit en deux tiers de seconde et l'étage
higher consomme quatre minutes sans produire **un seul** événement.

Une contre-épreuve sur le backend hôte à $n=100$ et $n=400$, délai annoncé de
60 s, n'a pas rendu la main en 420 s : l'étage higher ne termine pas davantage
sur l'hôte.

### Un défaut du garde-fou opérationnel, confirmé localement

La contre-épreuve a été reprise sur la machine de développement, hors VM :

| $n$ | $K$ | délai annoncé | arrêt réel | dépassement | `stop_category` |
| ---: | ---: | ---: | ---: | ---: | --- |
| 40 | 3 | 5 000 ms | **45 354 ms** (dont 45 309 dans `higher_support`) | **9,07 ×** | `operational_deadline` |
| 60 | 3 | 5 000 ms | > 120 000 ms, tué | > 24 × | — |

Le délai **est** armé dans l'étage higher — la catégorie d'arrêt le prouve — mais
son **quantum n'est pas borné** : la boucle ne reprend la main qu'à la fin d'une
unité de travail dont rien ne limite la durée. Dans l'étage paire, au contraire,
les arrêts tombent à 60 000 ms et 900 000 ms exactement.

Un délai qui déborde d'un facteur non borné n'est pas un délai. L'étage higher
doit subdiviser son unité de travail pour que le garde-fou reprenne la main —
c'est un correctif à part entière, et il conditionne toute mesure future bornée
sur cet étage.

---

## Ce que la session réordonne

Avant cette session, le verrou dominant déclaré était la **fermeture de descente
de facette** en aval, quadratique en événements. Les mesures le déplacent.

1. **L'étage paire n'est plus le mur** — à condition que le runner emprunte le
   chemin natif. Q3 le qualifie au rang du contrat ; la substitution n'est pas
   câblée. Tant qu'elle ne l'est pas, aucune question sur l'aval n'est
   atteignable à 50 000 points, et Q1 reste sans réponse pour cette raison.

2. **Le nouveau verrou dominant est l'étage higher**, et c'est un mur de
   **volume de travail exploré**, pas d'implémentation : il ne termine ni sur le
   device ni sur l'hôte à $n=400$, où le nombre de quadruples est déjà de
   $\binom{400}{4} \approx 1{,}05 \cdot 10^{9}$. C'est exactement l'objet que la
   germination locale certifiée a été construite pour attaquer — univers
   $2{,}604 \cdot 10^{17}$ ramené à $4{,}4 \cdot 10^{8}$ candidats — et sa
   restriction certifiée n'est pas encore câblée dans le chemin device.

3. **La fermeture de descente de facette n'est pas le prochain travail.** Elle
   reste quadratique et sa constante croît, mais elle ne se paie que sur des
   événements qui n'existent pas encore.

4. **Deux gains d'ingénierie sont chiffrés et disponibles** : paralléliser la
   recertification (54 % de l'étage paire au rang 11, mono-thread, linéaire à
   1,1 µs par record, sur 48 cœurs) et comprendre le facteur 3 du lanceur à
   travail constant.

---

## Ce que la session ne prétend pas

Aucun de ces runs n'est une exécution de bout en bout du produit. Q3 se déclare
`component_only`, `public_status: not_claimed`, `deployment_status:
component_only`. Le contrat de 50 000 points en 1 s n'est pas approché. Ce qui
est établi est que l'étage paire cesse d'être le mur dès que le runner emprunte
le chemin natif, et que le mur suivant est l'étage higher.
