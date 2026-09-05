# Mono F : pile locale des témoins et comparaison WSPD s=8,10,12

5 septembre 2026. Campagnes closes, liées aux sources et binaires mesurés.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Les trois paires conservent les six projections, sans gain robuste :
F est plus lent à s=8 et s=12, plus rapide à s=10.** Une seule paire froide
par s sur hôte partagé ne qualifie pas statistiquement ces différences.
Le [reçu mono clos](../receipts/witness_stack_mono_20260905/README.md)
conserve les six observations fraîches et leur reclassification.
Les deux [observations de taille](../receipts/witness_stack_scale_20260905/README.md)
sont également closes : le 16k achève la tour demandée, le 32k est refusé
sur un budget à K=9. Une observation close n'est pas nécessairement un
succès moteur.

## Même tour, mêmes plafonds

Uniforme n=8000, coordonnées u16 étendues (`coord=65536`), seed=3,
CPU logique 6, un thread demandé, fold inline sérialisé, CSR, digest inclus
et aucune archive. Chaque run calcule la **tour candidate complétée
K=1 à 10** (`--complete-incidences`, `smax=11`), pas le seul préfiltre Gabriel.
L'autorité reste `normalized_horizontal_h0_candidate` : ni verticale ni
poids ne sont livrés par cette campagne.

Chaque séparation a reçu un nouveau E puis un nouveau F, bornés à 600 s
et 26 GiB d'espace virtuel par processus, avec les mêmes plafonds de
travail et de payload partiel. Cette limite n'est pas un plafond de RSS
physique. Aucun chrono historique E n'est réutilisé.

Les trois paires closes rendent `paired_equal` : dix digests de forêt et
leur digest chaîné, cardinalités, comptes généraux, comptes silent,
plafonds silent et plafonds payload sont égaux à s fixé. La relecture
inter-s rend `objects_equal` pour les digests et cardinalités des six
runs ; les comptes de travail WSPD n'ont pas à coïncider entre séparations.
Ces contrôles conservent les projections annoncées, pas une certification
globale de l'objet.

| s | E processus | F processus | Variation F/E | E complétion silent | F complétion silent |
|---|---:|---:|---:|---:|---:|
| 8 | 187,677 s | 188,969 s | +0,688 % | 68,692 s | 68,705 s |
| 10 | 190,077 s | 185,660 s | −2,324 % | 68,400 s | 66,475 s |
| 12 | 184,878 s | 190,039 s | +2,791 % | 66,389 s | 68,925 s |

La variation est `100 × (F/E − 1)` : un signe positif indique ici une
dégradation observée. À s=8, elle correspond à environ 1,292 s. Le RSS
externe passe de 2 298 584 KiB pour E à 2 299 856 KiB pour F ; cette seule
paire n'établit aucune baisse mémoire du pipeline.

Une seule paire froide ordonnée par s ne sépare pas l'effet du delta,
celui de s et les variations de l'hôte. Les signes opposés des variations
ne justifient donc aucune accélération robuste. L'invariant local
d'allocation reste distinct de ce constat de temps. Aucun ratio D/F ou
C/F n'est reconstruit à partir de
campagnes historiques. Le défaut s=8 est inchangé, sans preuve d'optimalité.

## Ce que F change, ce que les portes qualifient

F remplace seulement le stockage de pile de `count_universal_witnesses`
par 64 entrées locales et un vector de secours, comme décrit dans
[la note de pile témoins](../docs/OPTIMISATION_PILE_TEMOINS.md).
Sous les prémisses de l'index radix u16, la frontière est bornée à 49
entrées : la pile de ce consommateur n'a pas besoin du débordement.
L'ordre du parcours, les masques, les prédicats et les comptes restent
inchangés. Cette suppression d'allocations locales n'est pas une réduction
du travail géométrique ni une garantie de baisse du temps total.

Les qualifications F closes sont distinctes des chronos : **48/48 Release
ciblées, 48/48 ASAN/UBSAN avec LeakSanitizer et 339/339 Release complètes**.
Le [reçu intégré](../receipts/witness_stack_integrated_20260905/README.md)
conserve les inventaires, JUnit, LastTest/fences, sources, binaires et
liaisons de compilation. Les onze commandes ciblées sont vérifiées ;
le contrôle de compilation full porte sur le CLI, pas sur les flags de
toutes les cibles. Une suite passée ne promeut pas l'exactitude industrielle.

F mesuré : `ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85`.
E apparié : `df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6`.
Le reçu de build F garde le nom hérité `build_D.json`, avec SHA
`522c950c70b60ca58759c4fa9b9a24ff995fe829b9aa1adf5b2f51b7b2177ac4` ;
son contenu désigne bien F. Les baselines historiques C/C/D/E restent
attribuées à leurs propres builds et qualifications.

## Travail restant et observations de taille

La paire s=8 conserve 802 125 328 supports MEB, 581 904 257 visites silent
de nœuds et 1 270 848 pas/ajouts. La tour compte 4 384 229 événements et
26 434 998 facettes cumulées ; cette somme n'est pas un pic de résidence.
Ces nombres sont recalculés depuis le run F clos, pas transférés de E
historique. Ils confirment que ce delta ne réduit pas la complétion.

F dépense encore environ 61,504 s en génération, 33,790 s dans l'étage
fold et 68,705 s en complétion silent à s=8. Les temps d'étages imbriqués
ne doivent pas être additionnés comme s'ils étaient tous disjoints.
Il reste à réduire le travail des postes dominants sans changer l'objet
ni masquer les refus pour obtenir un chrono inférieur. La
[proposition MEB par pivots](../docs/PROPOSITION_MEB_ET_BUDGETS.md)
et son budget physique ne sont pas intégrés dans F.

## Paliers F seuls : 16k achevé, 32k refusé

Deux observations indépendantes de F ont été exécutées à s=8, n=16000
puis n=32000, avec la même tour K1..10 demandée, les mêmes caps et le même
budget de 600 s/26 GiB. Elles ne sont pas mélangées aux paires 8k et ne
fournissent aucun ratio E/F.

| n | État moteur | Processus complet | Pipeline complet | RSS externe du succès |
|---|---|---:|---:|---:|
| 16000 | `engine_completed` | 413,816 s | 413,790 s | 5 361 880 KiB |
| 32000 | `engine_refused` à K=9 | — | — | — |

Le 16k conserve dix ordres, 9 178 223 événements et 55 397 230 facettes
cumulées. Ce dernier chiffre n'est ni un pic mémoire ni 55 millions de
points d'entrée. La complétion cumule 1 689 137 869 supports MEB,
1 277 647 823 visites de nœuds et 2 651 507 pas/ajouts : ce sont des sommes
sur les ordres, à ne pas confondre avec les plafonds fixés **par ordre**.

Le 32k rend le code 2, `resource_exhausted`, avec le diagnostic
`REFUS silent incidence K=9 : silent_core_record_budget`. Le temps
processus de cette tentative est **569,876 s avant refus**, pas un temps
de tour achevée. `timed_out=false` : il ne s'agit pas de la censure à
600 s. Le plafond atteint est celui de 8 000 000 core records par ordre,
pas une saturation démontrée de la RAM ou de RLIMIT_AS.

Le diagnostic `silent_refusal_K9 core=0` ne signifie pas qu'aucun record
n'a été construit. Dans le [CLI F](../cli/mhgp7.cpp), `core` affiche
`core_facets`, assigné seulement après le tri et la déduplication complets.
La [construction du cœur](../src/forest/silent_incidence.hpp) charge
`core_records` avant chaque ajout : le refus bloque ici le record brut
8 000 001, après 8 000 000 records admis, avant ce tri. Ce compteur brut
reste dans les statistiques de l'API mais n'est pas imprimé par ce format.
C'est une explication du chemin source épinglé, pas une mesure ajoutée
rétroactivement au reçu. Les deux quantités ne doivent pas être confondues.

La sortie standard de 32k est vide ; les diagnostics de travail et de
mémoire restent dans les bruts du reçu. Aucun digest de succès, préfixe
partiel de hiérarchie, RSS de tour complète ou estimation de temps futur
n'est publié à partir de ce refus. Modifier les plafonds nécessiterait
une autre campagne attribuée, sans effacer ce résultat négatif.

Les cibles [50k/1 seconde puis 100 ms](../docs/CONTRAT_PERFORMANCE.md),
d'abord toute la tour jusqu'à K=10 puis le repli jusqu'à K=5, restent non
atteintes. Les nuages de plusieurs dizaines de millions de points sur G4
ne sont ni démontrés ni extrapolés par ce jalon. Les résultats historiques
8k/16k/32k et G4 restent liés à leurs propres binaires. GCP non utilisé.

## Sources et reçus clos

Les trois paires et le build F sont publiés dans `witness_stack_mono_20260905` :
61 copies byte-exactes, 67 fichiers couverts par les listes locale et
racine, toutes deux vérifiées. `SHA256SUMS` :
`5682f6bc2362dc534722dc504646461fa3e80ec59b91a198febdd9627eaa8ee8`.
`results.json` : `7a1911e5dfc701af242208716fb99479ae678cef6ee4d4208f8587ba9c9d71b4`.
La revue rejoue chaque champ des sorties et les trois décisions,
vérifie l'accord inter-s et referme les sources/helpers/binaires.

Qualification intégrée publique : `SHA256SUMS`
`b24733a29a054814574b02ef1a6ecb28b01bfe754b53c14774b79ad579aed9e8`,
vérifié en lecture seule, sans requalification. L'export mono a été
autorisé séparément et exécuté sur CPU0 sans nouveau moteur ; aucun
moteur scale n'est relancé par la rédaction. Le reçu scale est fermé
séparément : 39 copies byte-exactes, 45 fichiers couverts et vérifiés par
les deux listes. `SHA256SUMS` :
`9e569b04de15bc9196e4973490485c4720090bbf4ac469e42ad3f88ac56306d1`.
`results.json` : `8030a22217bd24f5bb18ad8ce2c808c92190fa5f5b30f4a6306fd181ef6a3fc8`.
Les deux exports ont reclassifié les bruts et contrôlé les sources et
binaires, sans exécution nouvelle. Les trois paires et les deux paliers
prévus sont clos. Cette note est portée explicitement depuis le brouillon
privé relu ; les reçus bruts restent inchangés.
