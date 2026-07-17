# Phase 2B — progression des prédicats exacts GPU

## Statut

- phase : `2B`, en cours;
- backend : `cuda_g4`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par la fermeture de la phase 2A et la qualification de la phase 3;
- porte de sortie : non satisfaite.

Ce document suit les incréments techniques de la phase 2B. Il ne promeut aucun statut scientifique public et ne ferme pas la phase.

## Premier incrément : comparaison de distances carrées

Le premier portage GPU cible `compare_squared_distances`. Il conserve les neuf mots binary64 d'entrée et un identifiant de replay, puis produit un tri-état distinct : `negative`, `positive` ou `unknown`. La valeur `unknown` ne représente jamais une égalité exacte.

Le noyau CUDA calcule des intervalles avec les primitives d'arrondi dirigé `__dsub_rd/ru`, `__dmul_rd/ru` et `__dadd_rd/ru`. Tout mot non fini, tout intermédiaire non fini et tout intervalle contenant zéro donnent `unknown`. La cible reste AOT `sm_120`, sans fast math, FMA ni FTZ selon la politique CUDA fermée de la phase 3.

Le runner hôte conserve l'ordre et les identifiants. Il transmet chaque `unknown` à `decide_squared_distance_order` avec la politique CPU adaptative dans une tâche asynchrone. En mode d'audit, chaque signe connu du GPU est recalculé séparément avec la politique CPU `multiprecision_only`; une contradiction fait échouer le lot. Chaque décision publiée contient la commande canonique acceptée par l'outil de replay CPU.

## Deuxième incrément : orientation exacte en dimension trois

Le deuxième portage cible `orientation_3d` avec la même convention que le prédicat CPU :

$$\det\left([b-a,c-a,d-a]\right)=u_0(v_1w_2-v_2w_1)-u_1(v_0w_2-v_2w_0)+u_2(v_0w_1-v_1w_0).$$

Les douze mots binary64 et l'identifiant de replay restent intacts. Les primitives d'intervalles CUDA sont désormais mutualisées entre distance et orientation. La multiplication traite les quatre quadrants de signe avec deux produits dirigés; le chemin général à huit produits n'est utilisé que lorsqu'un intervalle traverse zéro. Les deux noyaux utilisent un stockage SoA par champ, afin que chaque warp lise des coordonnées contiguës plutôt que des structures espacées.

Le GPU ne publie toujours qu'un signe strictement séparé de zéro. Coplanarité, underflow, overflow ou intervalle indécis produisent `unknown`, puis `decide_orientation_3d` résout le cas sur le CPU. Le mode d'audit recalcule chaque signe GPU connu avec l'oracle multiprécision.

## Troisième incrément : bisecteur de puissance rationnel

Le troisième portage cible `power_bisector_side` pour deux labels `R` et `Q` de même cardinalité, bornée à dix. Le témoin exact est sérialisé sous la forme homogène réduite `y=A/D`, avec `D>0`; les quatre entiers doivent être exactement représentables en binary64. Une valeur entière non représentable est refusée au lieu d'être arrondie et doit être orientée vers le prédicat CPU exact avant cet appel GPU.

Pour tout appariement des points des deux labels, la quadratique commune s'annule et le noyau évalue la factorisation suivante, de même signe que le prédicat public :

$$G=D H_{R,Q}(A/D)=\sum_{i=1}^{K}\sum_{a=1}^{3}(q_{i,a}-r_{i,a})[(A_a-D r_{i,a})+(A_a-D q_{i,a})].$$

Cette écriture évite division, carrés et soustraction de deux grandes sommes de coûts. Les points de chaque label sont triés canoniquement par coordonnées puis identifiant avant l'appariement; l'égalité des cardinalités rend la somme indépendante de ce choix. Le stockage device comporte 64 champs SoA : trois numérateurs, un dénominateur et deux fois dix points tridimensionnels. Les termes sont accumulés par axe avant la somme finale afin de limiter la largeur des intervalles.

Le résultat GPU n'est connu que si l'intervalle final est strictement positif ou strictement négatif. Zéro, underflow, overflow ou intervalle contenant zéro donnent `unknown`; le témoin rationnel et les moments exacts des labels sont alors reconstruits et transmis à `decide_power_bisector_side` en multiprécision. Le mode d'audit utilise le même oracle rationnel exact pour chaque signe GPU connu. Le runner accepte désormais des lots homogènes distance, orientation ou bisecteur de puissance et refuse toute transaction qui mélange les prédicats.

## Validations locales avant qualification matérielle

- workflow `cpu-release` : 28 tests sur 28 réussis, contrôle statique 2B inclus;
- contrôle statique 2B : 36 mutations négatives rejetées, notamment les inversions des directions `rd/ru`, des extrémités de produit, des signes de cofacteurs, du facteur `q-r` et de la réduction homogène;
- compilation syntaxique stricte des unités hôtes : réussie;
- différentiel distance renforcé avec lanceur GPU simulé : 2 064 cas, dont douze cas non entiers ciblant séparément soustraction, multiplication et addition dirigées; 2 061 signes connus et trois replis CPU, zéro `unknown` terminal;
- corpus orientation : 2 070 cas relus par un oracle rationnel indépendant et par une émulation bit-à-bit des intervalles dirigés; 2 065 signes GPU obligatoires, zéro mauvais signe et zéro `unknown` terminal après replay CPU;
- corpus bisecteur de puissance : 2 077 cas construits par un oracle direct `Fraction`, dont 2 065 signes GPU obligatoires, six cas adversariaux facultativement filtrables, six replis obligatoires et quatre zéros exacts; l'identité affine secondaire est vérifiée indépendamment pour chaque cas;
- test mathématique indépendant de la multiplication d'intervalles : 100 000 paires aléatoires, tous les quadrants couverts, 65 559 enveloppes finies exactes et 34 441 overflows rejetés de façon conservatrice;
- replay CPU multiprécision de toutes les commandes distance, orientation et bisecteur de puissance : réussi.

Le lanceur simulé et l'émulation mathématique vérifient l'orchestration, les schémas, les compteurs, le replay et les formules. La qualification matérielle combinée ci-dessous vérifie ensuite le code device réel et mesure le chemin complet sur G4.

## Qualification matérielle du 17 juillet 2026

Le commit propre et poussé `6f27a68177d21efb19f7ba4cb35ed4855ec73a90` a ensuite été compilé et exécuté sur `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, une `g4-standard-48` Spot. Le coupe-circuit GCE était `instanceTerminationAction=STOP` avec `maxRunDuration=3600`; l'arrêt invité était armé à 45 minutes.

Les workflows CUDA release et audit ont compilé le runner avec NVCC 12.9.86. Le différentiel réel a traité 2 052 cas : 2 049 signes FP64 GPU ont été certifiés puis revérifiés par l'oracle CPU multiprécision; les trois cas attendus — égalité exacte, sous-normal et overflow — ont produit `unknown`, ont été transmis au CPU et n'ont laissé aucun `unknown` terminal. Toutes les commandes émises ont été rejouées par l'outil CPU.

`cuobjdump` a trouvé uniquement l'ELF AOT `sm_120` et aucune entrée PTX. `compute-sanitizer --tool memcheck --leak-check full --error-exitcode=86` a réussi sur un lot contenant une décision GPU connue et un repli CPU. L'image réutilisée est `sha256:e3d96c187ca405790227e02aef1a66ca47df0820bb6b2a86b097359105956d58`, déjà qualifiée par la phase 3.

L'artefact hors dépôt est `/tmp/morsehgp3d-phase2b-6f27a68177d21efb19f7ba4cb35ed4855ec73a90-20260717T172423Z/phase2b-6f27a68177d21efb19f7ba4cb35ed4855ec73a90.json`, de SHA-256 `65bc7175ed922a2515ecbf15a1a3b3c80445581c473232bc233d0d370a36cff1`. Il ne revendique aucun résultat scientifique public.

Après le succès, l'arrêt ciblé de la génération `2026-07-17T10:24:56.396-07:00` a été exécuté et relu indépendamment : état final `TERMINATED` à `2026-07-17T17:27:51.338224Z`. Aucune autre VM `project=e-hgp` active n'a été observée et la clé OS Login de session a été révoquée puis supprimée. Deux tentatives préparatoires avaient échoué avant compilation sur l'accès Docker et son point de montage; chacune avait déjà été arrêtée et certifiée `TERMINATED` avant la tentative suivante.

## Qualification matérielle combinée du 17 juillet 2026

Le commit propre et poussé `16f7f5975eff6233c73cc617730e608da3bfff69` a été compilé en profils CUDA release et audit avec NVCC 12.9.86 sur la même cible G4 Spot gardée. Le noyau distance utilise 39 registres et le noyau orientation 69 registres; `ptxas` ne signale aucun spill pour les deux. `cuobjdump` ne trouve que l'ELF AOT `sm_120` et aucune entrée PTX.

Le différentiel distance traite 2 064 cas : 2 061 signes GPU connus, tous audités par l'oracle CPU multiprécision, puis trois replis attendus. Le différentiel orientation traite 2 070 cas : 2 067 signes GPU connus audités et trois replis attendus pour coplanarité, underflow et overflow. Les deux campagnes finissent avec zéro contradiction et zéro `unknown` terminal. Le replay CPU multiprécision et `compute-sanitizer --tool memcheck --leak-check full` réussissent séparément pour les deux prédicats.

Le benchmark reproductible mesure un nouveau processus complet, y compris parsing, création du contexte CUDA, allocations, copies, noyau, replis éventuels et sérialisation; il ne représente donc pas un débit de noyau résident. Sur 262 144 cas et trois répétitions, la médiane atteint 313 022 comparaisons de distances par seconde en 0,837 s et 274 559 orientations par seconde en 0,955 s. Les minima chronométrés sont respectivement 0,832 s et 0,949 s. Tous les cas du benchmark sont certifiés sur le GPU sans repli.

L'artefact hors dépôt est `/tmp/morsehgp3d-phase2b-16f7f5975eff6233c73cc617730e608da3bfff69-20260717T184315Z/phase2b-16f7f5975eff6233c73cc617730e608da3bfff69.json`, de 2 184 octets et de SHA-256 `d15bd4e057f5d308c5479647cda409eea1f735ab35b5c964f0228ce4269d9fc5`. Son schéma est `morsehgp3d.phase2b.predicates.qualification.v1`; il conserve `public_status=null` et `scientific_result_claimed=false`.

La génération ciblée `2026-07-17T11:43:47.723-07:00` a été arrêtée puis relue indépendamment : état `TERMINATED`, dernier arrêt GCE `2026-07-17T11:46:22.897-07:00`. Aucune autre VM `project=e-hgp` active n'a été observée et la clé OS Login de session a été révoquée puis supprimée.

## Qualification matérielle des trois prédicats du 17 juillet 2026

Le commit propre et poussé `dfd9c0c72ca79baaa38d4889c2a8af564debdb08` a été compilé et exécuté sur `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`. La cible était toujours une `g4-standard-48` Spot, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et un arrêt invité vérifié à 50 minutes. Le worker conservait une réserve de 1 800 secondes avant cet arrêt invité.

Les workflows CUDA release et audit réussissent pour les trois noyaux. `cuobjdump` ne trouve que l'ELF AOT `sm_120` et aucune entrée PTX. Trois exécutions séparées de `compute-sanitizer --tool memcheck --leak-check full --error-exitcode=86`, une par prédicat, finissent avec zéro erreur.

Le différentiel distance traite 2 064 cas : 2 061 signes GPU connus audités et trois replis. Le différentiel orientation traite 2 070 cas : 2 067 signes GPU connus audités et trois replis. Le nouveau différentiel de puissance traite 2 077 cas : 2 071 signes GPU connus sont tous revérifiés par l'oracle rationnel multiprécision et six cas sont transmis au CPU, dont quatre zéros exacts. Les trois campagnes finissent sans contradiction et avec `remaining_unknown=0`.

Le benchmark froid de processus complet traite 262 144 cas par prédicat sur trois répétitions. Les médianes sont de 282 909 distances par seconde en 0,927 s, 246 380 orientations par seconde en 1,064 s et 118 933 bisecteurs de puissance par seconde en 2,204 s. Les meilleurs temps sont respectivement 0,921 s, 1,034 s et 2,186 s. Tous les cas de ces lots bien conditionnés sont certifiés par le GPU sans repli; ces mesures incluent parsing, contexte CUDA, allocations, transferts et sérialisation et ne représentent pas encore un débit résident.

L'artefact final hors dépôt est `/tmp/morsehgp3d-phase2b-dfd9c0c72ca79baaa38d4889c2a8af564debdb08-20260717T201837Z/phase2b-dfd9c0c72ca79baaa38d4889c2a8af564debdb08.json`, de 2 773 octets et de SHA-256 `8385559d8bca8dd0a8ab12e3bf0da0ac86199800fab575b0145ea355269919df`. Il conserve `public_status=null` et `scientific_result_claimed=false`.

La génération ciblée `2026-07-17T13:19:39.826-07:00` a été arrêtée puis relue indépendamment : état final `TERMINATED` certifié à `2026-07-17T20:24:18Z`. Aucune autre VM `project=e-hgp` active n'a été observée et la clé OS Login éphémère a été révoquée. Deux essais préparatoires, arrêtés et certifiés séparément, ont permis de corriger la remontée des journaux puis le point de sous-montage Docker avant cette exécution réussie.

## Travaux restant avant la porte de sortie

- étendre le différentiel au corpus distance de la phase 2A et à la campagne supplémentaire requise;
- porter uniquement les expansions GPU dont le gain est mesuré et conserver les autres replis exacts sur CPU;
- introduire un contexte résident et des espaces de travail réutilisables afin de séparer le coût du noyau de celui du processus froid;
- mesurer et publier les taux de chaque étage sans en faire une condition de correction;
- vérifier que tout `unknown` GPU est transmis au CPU sur l'ensemble des prédicats portés.

La porte de sortie 2B exigera zéro différence CPU/GPU sur le corpus de phase 2A et sur au moins dix millions de signes supplémentaires. Elle reste donc ouverte après ce premier incrément.
