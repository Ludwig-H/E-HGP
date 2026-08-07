# Bilan de contrat à 50 000 points

Document de synthèse. Aucun claim, aucune porte ouverte ou fermée, aucun statut
public. Il rassemble, étage par étage, ce qui est **mesuré**, ce que le nouvel
algorithme de génération locale change, et ce qui sépare encore le pipeline des
deux contrats. Chaque nombre est adossé à un artefact du dépôt.

## 1. Le contrat, tel qu'il est écrit

La porte de sortie de la phase 14 (`ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` l. 1848,
`implementation_status.toml` l. 974-975) fixe :

| | |
| --- | --- |
| taille | $n=50\,000$ points |
| ordre | $K_{\max}=10$ |
| **objectif principal** | p95 `warm_e2e` **< 100 ms** |
| objectif secondaire | < 1 s (ne ferme pas la porte principale) |
| mémoire | pic < 80 % de la VRAM, soit 76,8 Go sur 96 |

Deux directives normatives s'y ajoutent : la mesure ne vaut que sur la **version
sans budget** (7 août), et aucun benchmark ne promeut `public_status`
(AGENTS.md l. 52).

**Conséquence immédiate et non négociable.** Le mot `warm` n'est pas décoratif :
la seule création du contexte CUDA coûte **1 242 ms à froid** contre 18 ms à
chaud (`phase15_device_frontier_50k_kmax5_g4_3be0d42.json` contre
`_warm_g4_f39ab07.json`). Les deux contrats n'existent que dans un service
résident. Cela doit être écrit dans le contrat, pas supposé.

## 2. Ce qui est mesuré aujourd'hui, étage par étage

| étage | mesure | source | part de 100 ms |
| --- | ---: | --- | ---: |
| génération du nuage | 51,8–74,5 ms | hors périmètre `warm_e2e` | — |
| canonicalisation | 3,1–4,7 ms (CPU) | escalier R2-d, plancher produit | **3–5 %** |
| LBVH | 15,4 ms (CPU) ; 18,2 ms (CUDA chaud) | idem | **15–18 %** |
| étage paire, runner produit | **≥ 299 929 ms, censuré** à 5,78 % | escalier R2-d, cellule 50 000 | ≥ 299 929 % |
| étage paire, composant device $K{=}5$ | 1 005 ms de launcher | `phase15_product_floor_diag_g4_68f656b/full50k.json` | 1 005 % |
| étage paire, composant device $K{=}10$ | 2 979 ms de launcher (13 875 ms à froid) | qualification rang 11 | 2 979 % |
| étage higher | jamais atteint à 50 000 | l'étage paire consomme tout le délai | — |
| aval complet | **18 923,7 ms à $n=32$** pour 171 événements | escalier R2-d, cellule 32 | — |
| réducteur seul, tour synthétique | 2 792 ms | `point_hierarchy_projection_50k_local_20260803.json` | 2 792 % |
| archive durable 15L | **jamais chronométrée** | `requested=false` | inconnue |

**Le plancher incompressible mesuré** — canonicalisation plus LBVH, à chaud —
vaut **18,5 à 23,6 ms**, soit **18 à 24 % du budget de 100 ms**, avant le premier
calcul scientifique.

## 3. Ce que le nouvel algorithme change, et ce qu'il ne change pas

L'étage higher passait par une subdivision de produits dont le coût est mesuré
proportionnel à $\binom n3+\binom n4=2{,}604\cdot10^{17}$, soit de l'ordre de
2,7 millions d'années à 50 000 points. La génération locale certifiée
(`docs/math/GERMINATION_LOCALE_SUPPORTS_3_4.md`,
`docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md`) le remplace par une énumération
dont la complétude est prouvée et dont le coût est **mesuré** :

| | candidats | gain sur l'univers |
| --- | ---: | ---: |
| subdivision de produits (état antérieur) | $2{,}604\cdot10^{17}$ | 1 |
| germe seul, lemme d'angle $\kappa=0{,}2071$ | $8{,}69\cdot10^{10}$ | $3{,}0\cdot10^{6}$ |
| germe Jung $\kappa_4=\sin15^\circ$, + élagage incrémentiel | $4{,}6\cdot10^{10}$ | $5{,}73\cdot10^{6}$ |
| + J7 disque recouvert et J8 segment recouvert, **mesuré conjointement** | $\mathbf{4{,}4\cdot10^{8}}$ | $\mathbf{5{,}9\cdot10^{8}}$ |

Pour référence, la sortie utile estimée vaut $1{,}8\cdot10^{7}$ records : le
générateur examine donc **24,4 candidats par record émis**, contre 5,04 visites
de nœud par record à l'étage paire. Le générateur est désormais dans le **même
ordre de grandeur** que ce que le projet sait déjà faire tourner à 50 000 points,
et la dernière ligne est une mesure **jointe** — germe J7, énumération réelle par
lentille, test libre et segment J8 appliqués en une seule passe — non un produit
de facteurs séparés.

**Ce que cela ne change pas.** L'étage paire, le plancher amont, l'aval et
l'archive sont intacts. Le nouvel algorithme retire un mur de quatorze ordres de
grandeur ; il ne touche à aucun des autres postes.

## 4. Conversion en secondes, et le vrai verrou

Un candidat coûte, au pire, un comptage de boule fermée. La seule mesure de débit
disponible sur ce matériel est celle du launcher paire : 22 697 584 visites de
nœud physiques en 1 005 ms, soit **44,3 ns par visite**, à raison de 5,04 visites
par record produit.

| hypothèse de débit | $4{,}4\cdot10^{8}$ candidats, 5 visites chacun | verdict |
| --- | ---: | --- |
| 44,3 ns/visite (mesuré aujourd'hui) | $\approx 97$ s | hors d'atteinte |
| 5 ns/visite | $\approx 11$ s | hors d'atteinte |
| 1 ns/visite | $\approx 2{,}2$ s | proche |
| **0,5 ns/visite** (soit $2\cdot10^{9}$ visites/s) | $\approx \mathbf{1{,}1}$ **s** | **contrat 1 s essentiellement tenu** |

Autrement dit : **le verrou n'est plus l'algorithme, c'est le coût unitaire.**
Et le débit de référence est lui-même suspect — 44,3 ns par visite de nœud sur
une Blackwell est deux ordres de grandeur au-dessus de ce que le matériel permet.

C'est exactement ce que la réfutation du filtre fp64 avait désigné : retirer du
travail arithmétique aux portes du moteur higher n'a rien déplacé, ce qui plaide
pour un coût dominé par l'**aller-retour de lancement et de drainage** plutôt que
par le calcul. Tant que ce point n'est pas instrumenté nativement, toute
conversion candidats → secondes reste une hypothèse.

## 5. Le bilan honnête des deux contrats

**Contrat à 1 seconde.** Trois conditions, toutes nécessaires :
1. l'étage paire ramené de 5 190 s à environ 1 s — c'est B1, conçu, non
   implémenté ;
2. l'étage higher porté sur la génération locale certifiée — mathématique
   acquise, implémentation à faire ;
3. un débit de visite de nœud amélioré d'environ **cent fois** par rapport aux
   44,3 ns mesurés, soit $2\cdot10^{9}$ visites par seconde — cause du déficit
   non encore identifiée, à instrumenter. Pour situer : les 44,3 ns agrégés du
   launcher paire représentent $2{,}3\cdot10^{7}$ visites par seconde au total,
   c'est-à-dire quelques centaines de visites par seconde et par thread. Aucune
   opération de traversée ne justifie un tel chiffre ; c'est l'argument le plus
   fort en faveur d'un coût de transport plutôt que de calcul.

**Contrat à 100 ms.** Les trois conditions ci-dessus, **plus** :
4. un facteur dix supplémentaire sur le coût unitaire ;
5. le portage device de la canonicalisation et du LBVH, qui consomment à eux
   seuls 18 à 24 % du budget ;
6. la parallélisation de tout l'aval, dont aucune mesure n'existe à l'échelle —
   son seul point de comparaison est 18 923,7 ms pour 171 événements, et son
   entrée passerait à $1{,}8\cdot10^{7}$ records ;
7. un service résident chaud, sans lequel le seul contexte CUDA dépasse le budget
   d'un facteur douze.

**La condition qui ne dépendait d'aucune optimisation est levée.** Jusqu'au
7 août, aucun instrument du dépôt ne pouvait produire le nombre contractuel :
`warm_e2e_protocol_executed` était un littéral `false`, `p95_ms` un littéral
`null`, et le runner ne portait aucune instrumentation mémoire alors que la porte
exige un pic sous 80 % de la VRAM. Le protocole existe désormais
(`--warm-e2e-repetitions N`) et publie des faits, pas des littéraux. Il reste
qu'il mesure aujourd'hui le **résident hôte** ; le pic **VRAM** exigé par la
porte demande la même instrumentation côté device, et elle n'existe pas encore.

## 6. Ordre de travail

1. ~~**Instrumenter `warm_e2e` et le pic mémoire** dans le runner.~~ **Fait.**
   `--warm-e2e-repetitions N` répète tout le pipeline dans un seul processus,
   écarte la première itération comme échauffement et publie p95 au rang le plus
   proche, minimum, médiane, le compte d'échantillons et le pic de résident hôte
   lu dans `VmHWM`. `warm_e2e_slo_claimed` reste faux en toutes circonstances :
   le protocole observe, il ne revendique jamais. Vérifié sur $n=6$, $K=3$,
   vingt échantillons : p95 123,023 ms, médiane 78,035 ms, minimum 57,692 ms.
2. **B1** — le consommateur exact derrière la frontière paire device
   (`PAIR_STAGE_DEVICE_CONSUMER_DESIGN.md`, six incréments). Sans lui, l'étage
   higher n'est pas mesurable à la taille contractuelle.
3. **Le générateur de germination locale**, avec la refondation de la
   comptabilité sur la complétude déjà décidée.
4. **Le profil natif lancement/drainage**, qui décide si le coût unitaire est de
   l'arithmétique ou du transport. Une seule session G4, mais elle ne vaut la
   dépense qu'après 1 et 2.
5. **L'aval**, jamais mesuré, et le portage device du plancher amont.

Les postes 1 et 3 n'exigent aucun GPU et peuvent avancer immédiatement.
