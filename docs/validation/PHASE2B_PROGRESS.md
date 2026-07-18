# Phase 2B — progression des prédicats exacts GPU

## Statut

- phase : `2B`, terminée;
- backend : `cuda_g4`;
- profil prioritaire : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par la fermeture de la phase 2A et la qualification de la phase 3;
- porte de sortie : satisfaite par les campagnes longues, la qualification résidente et la mesure `warm_context_e2e` sur G4.

Ce document archive les incréments techniques de la phase 2B. La revue de porte ferme la phase et ouvre la phase 4 sur le backend `reference_cpu`, le profil `hgp_reduced` et le mode `certified`. Elle ne promeut aucun statut scientifique public, ne ferme pas la phase 4 et ne qualifie ni G2 ni un backend spatial GPU exact.

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

Cette écriture évite division, carrés et soustraction de deux grandes sommes de coûts. Les coordonnées de chaque label sont triées canoniquement et appariées indépendamment sur chaque axe, avec l'identifiant comme départage; l'égalité des cardinalités rend chaque somme axiale indépendante de cette bijection. Ce choix évite qu'un ordre lexicographique tridimensionnel impose de grands termes opposés sur un autre axe. Le stockage device comporte 64 champs SoA : trois numérateurs, un dénominateur et deux fois dix points tridimensionnels. Les termes sont accumulés par axe avant la somme finale afin de limiter la largeur des intervalles.

Le résultat GPU n'est connu que si l'intervalle final est strictement positif ou strictement négatif. Zéro, underflow, overflow ou intervalle contenant zéro donnent `unknown`; le témoin rationnel et les moments exacts des labels sont alors reconstruits et transmis à `decide_power_bisector_side` en multiprécision. Le mode d'audit utilise le même oracle rationnel exact pour chaque signe GPU connu. Le runner accepte désormais des lots homogènes distance, orientation ou bisecteur de puissance et refuse toute transaction qui mélange les prédicats.

## Réduction des transferts et de la mémoire froide

Les trois noyaux renvoient désormais uniquement un `FilterSign` signé d'un octet par entrée. Les identifiants de replay restent autoritatifs sur l'hôte et sont réassociés par l'index du lot; ils ne sont plus copiés vers le device ni recopiés dans une structure de sortie paddée. Cette transformation supprime un buffer, une allocation et une copie H2D par lot, puis réduit la copie D2H de 16 octets à un octet par décision, sans modifier l'API publique, les commandes de replay ni la politique de fallback.

Le chemin `summary-only` ne construit plus les chaînes `replay_command`, qui n'étaient ni émises ni contrôlées dans ce mode. Le benchmark froid ne conserve plus simultanément les trois gros payloads : il construit, mesure puis libère un prédicat avant de passer au suivant. Ces changements réduisent le pic de mémoire hôte du protocole froid et ne changent aucun signe scientifique.

## Contexte CUDA résident

Le commit `971f2df` introduit `PredicateFilterContext`, qui conserve un stream CUDA non bloquant et trois espaces de travail à capacité maximale réutilisable : coordonnées SoA, cardinalités de labels et signes de sortie. Sa construction reste purement hôte; le runtime CUDA n'est initialisé qu'au premier lot non vide. Les trois API historiques créent un contexte éphémère compatible, tandis que les nouvelles surcharges acceptent un handle move-only dont l'état partagé reste vivant jusqu'à la résolution de toutes les futures déjà planifiées.

Un mutex sérialise uniquement réservation, transferts, noyau, retour des signes et synchronisation sur un même contexte. La classification des sorties, l'audit multiprécision et le fallback CPU s'exécutent hors de cette section critique. Toute erreur CUDA ou anomalie post-GPU avant publication empoisonne ce contexte et les appels suivants échouent fermé; une validation d'entrée refusée avant le GPU ne l'empoisonne pas et un autre contexte reste indépendant. L'ordinal propriétaire est réactivé puis restauré autour de chaque section et de la destruction. Les fonctions device, les intervalles dirigés, l'ordre des opérations et la règle `unknown` vers CPU exact sont inchangés. Cette architecture a reçu sa qualification G4 de réutilisation et sa mesure chaude; celles-ci ne revendiquent aucun gain au-delà du protocole mesuré.

## Campagne de fermeture résumable

Le commit `f1a917fd93ccf27297f654027c065cc8f2a32466` ajoute `phase2b_predicate_campaign.py`, un runner transactionnel destiné à la porte de sortie 2B. En mode `both`, il rejoue d'abord la graine gelée de phase 2A, puis une graine indépendante de phase 2B, `4257325f47505532`. Chaque graine produit 10 800 000 cas de base, répartis également entre les trois prédicats, et 1 080 000 signes métamorphiques, soit 11 880 000 signes par corpus et 23 760 000 signes agrégés lorsque les deux campagnes sont achevées.

Chaque signe est contrôlé par trois voies indépendantes : l'oracle entier-dyadique du générateur, le replay CPU forcé en multiprécision et la relation métamorphique lorsqu'elle s'applique. Le résultat GPU est le système sous test : un signe FP64 connu doit coïncider avec ces contrôles et tout `unknown` doit aboutir à une décision CPU exacte. Les racines ordonnées distinctes et la graine indépendante établissent que les deux flux canoniques ne sont pas identiques; elles ne prouvent ni ne revendiquent une disjonction cas par cas de l'ensemble complet des entrées.

Les chunks sont homogènes par prédicat, bornés à 196 608 cas de base, sérialisés en JSON canonique et chaînés par quatre racines SHA-256 pour le corpus, l'oracle, le replay CPU et le replay GPU. Un verrou POSIX exclut deux écrivains; chaque chunk et manifeste est publié par écriture temporaire, `fsync`, vérification, renommage atomique et `fsync` du répertoire. Une contradiction, un fichier modifié, une racine ou un compteur incohérent, un chunk non contigu ou une sortie non canonique échoue fermé sans avancer le checkpoint.

L'audit du premier schéma a montré que ses racines scientifiques étaient correctes mais que le certificat ne persistait pas le détail par métamorphisme et relation, ne rejetait pas tous les reçus orphelins et ne proposait pas de vérification finale en lecture seule. Le commit `4cccb0d` publie le schéma `1.1.0`, qui ferme ces défauts : il conserve les deux répartitions métamorphiques, impose les égalités exactes entre zéros et signes, entre sorties GPU connues et replis, puis entre total et replay CPU multiprécision; il rejette tout fichier temporaire, lien ou chunk non référencé. `--verify-only` relit sans écriture la chaîne complète, les certificats, l'agrégat, les exécutables et la provenance Git. Un agrégat isolé porte explicitement `single_campaign_only`; seule la paire complète peut porter `independent_seed_and_distinct_corpus_root`.

Le replay préliminaire `1.0.0` de la graine Phase 2A sur `f1a917f` a néanmoins achevé 55 chunks : 10 800 000 cas de base et 1 080 000 métamorphismes, soit 11 880 000 signes. Les racines gelées corpus `76544b0c85c2f6602e560bd007762609e1feb080f5fe13a71ec4d0d3f31f294b` et oracle `62a4f14bc4d25971d67d7c321bdea9091d45f7a17dc37d79a292fd34cd078210` sont reproduites. Le GPU certifie 11 304 000 signes et transmet 576 000 cas au CPU; aucun signe n'est erroné et aucun inconnu ne reste. L'archive complète des reçus et les deux exécutables sont conservés sous `/tmp/morsehgp3d-phase2b-f1a917f-preliminary-evidence/`; l'archive a pour SHA-256 `1addfa61f9c842faa3f828514e9a07abbf2abd024b9bdc2acabe3bafd7832ba8`. Ce replay sert de preuve préliminaire et de mesure de bout en bout, mais son schéma insuffisant interdit de l'utiliser pour fermer la porte. Les certificats finaux `1.1.0` conservent `phase_gate_closed=false` et `qualification_claimed=false` jusqu'à la revue séparée de la porte.

## Validations locales avant qualification matérielle

- workflows `cpu-release` et `sanitizer` : 31 tests sur 31 réussis dans chacun, contrôle statique 2B, test du contexte résident, harness chaud hôte et smoke de reprise de campagne inclus;
- contrôle statique 2B : 63 mutations négatives rejetées, notamment les inversions des directions `rd/ru`, des extrémités de produit, des signes de cofacteurs, du facteur `q-r`, de la réduction homogène, de la sortie device compacte, de la mémoire `summary-only`, de l'affinité CUDA, de la réutilisation résidentielle et du chemin `warm_context_e2e`;
- compilation syntaxique stricte des unités hôtes : réussie;
- différentiel distance renforcé avec lanceur GPU simulé : 2 064 cas, dont douze cas non entiers ciblant séparément soustraction, multiplication et addition dirigées; 2 061 signes connus et trois replis CPU, zéro `unknown` terminal;
- corpus orientation : 2 070 cas relus par un oracle rationnel indépendant et par une émulation bit-à-bit des intervalles dirigés; 2 067 signes GPU obligatoires, zéro mauvais signe et zéro `unknown` terminal après replay CPU;
- corpus bisecteur de puissance : 2 079 cas construits par un oracle direct `Fraction`, dont 2 067 signes GPU obligatoires, six cas adversariaux facultativement filtrables, six replis obligatoires et quatre zéros exacts; l'identité affine secondaire est vérifiée indépendamment pour chaque cas et une paire de fixtures impose le tri axial face à un overflow créé par l'ordre lexicographique;
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

## Qualification matérielle de l'état `f1a917f` du 17 juillet 2026

Le worker G4 a compilé et exécuté le commit propre et poussé `f1a917fd93ccf27297f654027c065cc8f2a32466` sur la même cible `g4-standard-48` Spot. Les profils CUDA release et audit, l'AOT `sm_120` sans PTX et les trois exécutions de `compute-sanitizer --tool memcheck --leak-check full` réussissent.

Le différentiel distance traite 2 064 cas : 2 061 signes GPU connus et trois `unknown` transmis au CPU. Le différentiel orientation traite 2 070 cas : 2 067 signes GPU connus et trois replis. Le différentiel de puissance traite 2 079 cas : 2 073 signes GPU connus et six replis, dont quatre zéros exacts. Tous les signes GPU connus sont audités, aucun signe ne contredit l'oracle et les trois campagnes finissent avec `remaining_unknown=0`.

Le benchmark froid de processus complet traite 262 144 cas par prédicat sur trois répétitions. Les médianes sont de 313 195 distances par seconde, 271 907 orientations par seconde et 129 528 bisecteurs de puissance par seconde. Les temps médians correspondants sont 0,837 s, 0,964 s et 2,024 s. Ce protocole inclut toujours parsing, création du contexte, allocations, transferts, noyau et sérialisation; il ne mesure pas un chemin résident.

L'artefact worker hors dépôt est `/tmp/phase2b-f1a917fd93ccf27297f654027c065cc8f2a32466.json`, de 2 423 octets et de SHA-256 `0628657deab59c38c0b896dbe8da909e81214f76140577b2536f0b91ce5c865b`. Il conserve immuablement `public_status=null`, `scientific_result_claimed=false` et `status=worker_passed_pending_shutdown`. Après l'archivage du replay préliminaire, la génération ciblée `2026-07-17T14:08:37.248-07:00` a été arrêtée et relue avec `lastStopTimestamp=2026-07-17T14:48:27.452-07:00` : état final `TERMINATED`. Aucune autre VM `project=e-hgp` active n'a été observée; la clé OS Login éphémère a été révoquée, vérifiée absente puis supprimée. Cette qualification ne ferme pas la phase 2B.

## Qualification de fermeture du 17 au 18 juillet 2026

Le commit propre `39a011c92d67b6de9ba38d60dc884b926cc36c5f` a été qualifié avec le schéma de campagne `1.1.0`. La première session a rejoué le corpus gelé Phase 2A; la seconde a repris son archive qualifiée puis produit le corpus Phase 2B avec la graine indépendante `4257325f47505532`. Chaque certificat contient 10 800 000 cas de base et 1 080 000 signes métamorphiques. L'agrégat vérifié en lecture seule contient 23 760 000 signes, zéro `wrong_sign`, zéro `remaining_unknown` et deux racines de corpus distinctes.

Chaque corpus compte 11 304 000 signes GPU FP64 certifiés et 576 000 `unknown` transmis puis décidés par le replay CPU multiprécision. Les quatre racines ordonnées de chaque campagne, les hashes de la configuration, du générateur, du runner, du replay CPU, du replay GPU et du commit propre sont conservés. Les certificats versionnés se trouvent sous `morsehgp3d/tests/campaigns/phase2b_predicates_v1/`; l'agrégat vaut `98b40b566cefc63b3edf37ce98e1b13749c4b6f76b3f6c332a77fdf7898cfbda`.

Le harness résident G4 traite 1 984 entrées, 16 lots, deux contextes, trois lots vides et les trois prédicats; il reproduit exactement le chemin éphémère. Compute Sanitizer termine avec zéro erreur et zéro fuite. Les profils CUDA release et audit compilent avec NVCC 12.9.86 pour `sm_120`, sans spill rapporté. Les archives qualifiées Phase 2A et Phase 2B valent respectivement `8fab713397e0dd5fc8462529b6f0a008ffbffaa4b9d7bfc511c88ca02a13b270` et `702e64cc2194c6e01d1faf24696e6fdee87752c1787b9a18db969380b86dd905`.

La génération GCP `2026-07-17T16:47:58.724-07:00` a été arrêtée et relue `TERMINATED` avec `lastStopTimestamp=2026-07-17T17:18:25.306-07:00`. Aucune autre VM E-HGP active n'a été observée et la clé OS Login a été révoquée. Les deadlines de travail précèdent les bornes GCE sûres d'environ 15 minutes 27 secondes; elles relèvent de l'exception transactionnelle documentée, car chaque unité est bornée à 240 secondes, chaque chunk est checkpointé atomiquement et vérification, copie et nettoyage restent bornés.

## Qualification `warm_context_e2e` du 18 juillet 2026

Le commit propre et poussé `9ad398b8b83c5477fb52ab7d357aff3309931d4f` a été compilé avec NVCC 12.9.86 dans l'image `sha256:e3d96c187ca405790227e02aef1a66ca47df0820bb6b2a86b097359105956d58`. La cible réelle est une NVIDIA RTX PRO 6000 Blackwell Server Edition, capacité de calcul 12.0, 97 887 MiB et pilote 580.159.03. Le binaire du harness vaut `3df18bfa8ee05d92b07acca759e276d90f2be7d33eced1eec9c577f18744b47b`.

Pour chacun des trois prédicats, le protocole exécute un warmup puis 31 répétitions mesurées de 65 536 cas sur un contexte résident. Le chronomètre entoure l'API asynchrone jusqu'à `.get()` et inclut validation, packing, transferts, noyau, repli exact et synchronisation, mais exclut la génération des entrées.

| prédicat | min (ns) | p50 (ns) | p95 (ns) | max (ns) | MAD (ns) |
|---|---:|---:|---:|---:|---:|
| distance | 105 935 481 | 107 268 960 | 108 741 671 | 108 933 220 | 676 940 |
| orientation | 156 700 046 | 158 774 816 | 162 199 595 | 162 203 986 | 1 351 680 |
| puissance | 179 281 734 | 180 766 484 | 182 756 654 | 184 231 713 | 527 879 |

Chaque prédicat agrège exactement 2 031 616 entrées, 1 354 421 signes GPU connus et 677 195 `unknown` transmis en 31 lots au CPU. Les 677 195 replis sont aussi 677 195 zéros exacts; aucun inconnu ne reste. Distance et orientation ferment par expansions, puissance par multiprécision. Le worker brut hors dépôt a une taille de 3 392 octets et le SHA-256 `9cc5fb5673ff7dbeccd9c39fd931085a8178da09f8418163a1a7c9983c13550c`.

La session utile `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` était `SPOT`, avec `instanceTerminationAction=STOP`, `maxRunDuration=3600` et arrêt invité armé à 45 minutes. Elle a démarré à `2026-07-18T00:25:40.606-07:00`, puis l'arrêt ciblé de cette génération a été certifié à `2026-07-18T00:31:09.515-07:00`. La relecture indépendante du 18 juillet à `07:34:06Z` confirme `TERMINATED`; la clé OS Login est révoquée et absente, et aucune autre VM `project=e-hgp` active n'est observée.

La preuve finale canonique `morsehgp3d/tests/campaigns/phase2b_predicates_v1/warm-context-qualification.json` fait 4 215 octets et vaut `ac59ffa0a1fde5ca14a79a62bc8579571324ab2848b5c832617ddd0fb89fe1c6`. Elle imbrique le worker et son hash, le lifecycle ciblé et le statut final, tout en conservant `public_status=null` et `scientific_result_claimed=false`. Avec la revue `docs/validation/PHASE2B_GATE_REVIEW.md`, elle ferme la Phase 2B et ouvre uniquement la Phase 4; G2 et la sortie de la Phase 4 restent ouverts.
