# Qualification de l'équipe persistante front+census q2

15 septembre2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Flux q2 complet de supports avec intérieurs stricts et coquilles complètes.
**Ni q3/q4, ni HGP FULL, ni qualification G4. GCP non utilisé.**

## Captures propres

| Capture | Périmètre | État |
|---|---|---|
| [qualification_glu80_4s](qualification_glu80_4s/COMPLETION.json) |69 CTests Release + gate coopérative | PASS |
| `qualification_ifknl_sm` | Premier essai ASan interrompu par redémarrage | Incomplet, conservé |
| [qualification_tkoq1k82](qualification_tkoq1k82/COMPLETION.json) | Reprise distincte69 CTests ASan/UBSan + gate | PASS |
| [tsan_lxvwelac](tsan_lxvwelac/COMPLETION.json) | Gate coopérative Clang ThreadSanitizer | PASS |
| [smoke_ax6m8ojy](smoke_ax6m8ojy/COMPLETION.json) |4 familles n32, référence W4 et coopération W1/W4 |12 mesures closes |
| [scale_5359addn](scale_5359addn/COMPLETION.json) |4 familles n8k/16k/32k, K10/s8, Pool64/minB16 |36 mesures closes |
| [separations_3yoggku9](separations_3yoggku9/COMPLETION.json) |4 familles n8k, K5/10, s8/10/12, Pool64/minB16 |72 mesures closes |
| [rows_large_de3y8rig](rows_large_de3y8rig/COMPLETION.json) |Rangées n8k/16k/32k, K5/10, s8/10/12, Pool0/minB64 |54 mesures closes |

Total :174 mesures, dont58 références Coarse à4workers et116 appels
coopératifs à1/4workers. Chaque triplet partage les mêmes coordonnées,
K, s et seuil Pool. Les sorties et tous les compteurs géométriques,
de callback et de Pool concordent. Les queues et temps ne sont pas
contraints à l'égalité. Les fichiers de l'auditeur B restent indépendants.

Les16 commandes de lecture/analyse normal/−O sont closes dans
[analysis_4gtbcwpi](analysis_4gtbcwpi/COMPLETION.json), avec
[synthèse déterministe](analysis_4gtbcwpi/SUMMARY.json) identique pour les
deux modes. Les sources, binaires/caches et entrées, y compris le dossier
interrompu, sont recontrôlés à la fermeture de cette analyse.

La gate C++ confronte235 appels coopératifs à91 appels Coarse et à un
oracle scalaire exhaustif :8nuages,5 744 paires,396 286 tests de sites,
50 438 supports vérifiés, coquille maximale30. Sont exercés les états
avec crédits et phases, Pool64 filtrant et repli sans rejet, W>jobs,
plusieurs callbacks sur de vrais threads, erreur de callback suivie de
la sortie des threads observés, reset de l'index et contexte imbriqué.
Les14 invalides et5 mutants ciblés sont rejetés. Les nombres de dons
varient avec l'ordonnancement ; ils sont positifs dans les captures, sans
promesse d'un nombre fixe ni preuve qu'un worker dormait lors du throw.

Chaque gate de reçus normal/−O exécute24 sondes réelles,12 comparaisons
W1/W4,26 CLI invalides et plus de4 100 mutants. Douze mutations ciblent
les inventaires de pins et le JSON strict. Les mutants de maxima comme
sommes varient avec la répartition observée ; l'analyse normal/−O doit
être identique sur **les mêmes reçus**, pas sur deux nouveaux schedulings.

Les builds désormais épinglés sont `build/v8_cooperative_20260915`,
`build/v8_cooperative_sanitize_20260915` et
`build/v8_cooperative_tsan_clang_20260915`. Les
[préflights](PREFLIGHT.md) et l'[interruption](qualification_ifknl_sm/INTERRUPTED.md)
restent documentés. Aucun échec ou interruption n'est transformé en PASS.

## Résultat de performance : pas de changement du défaut

Les observations sont uniques et sous charge concurrente de tests/audits,
avec affinité0,2,4,6 sur quatre cœurs physiques. Elles ne qualifient pas
un gain stable. La nouvelle entrée reste une option explicite ; le chemin
Coarse existant n'est pas remplacé.

La campagne de croissance Pool64 ne produit **aucun don**. Seul le terrain
crée des continuations (111 à16k,36 à32k) ; toutes ses offres rencontrent
des workers occupés. À8k, aucune famille ne possède de B dans la fenêtre
16<=|B|<64. Uniforme32k a un facteur maximal de10 : le partage nouveau
ne touche pas ses11,084millions de racines et1,001milliard de visites census.

Les rangées sans Pool exercent réellement la file. Sur les174 mesures,
81 042 dons sont terminés, dont80 629 après attribution de toutes les
seeds. Malgré cela,17 des18 comparaisons rangées/Pool0 sont moins rapides
que Coarse dans ces observations. Exemple K10/s8, temps du pipeline q2
et callbacks (pas toute la tour, ni la génération/indexation) :

| n | Coarse W4 | Coopératif W1 | Coopératif W4 |
|---|---:|---:|---:|
|8 000 |119,51 ms |288,79 ms |207,66 ms |
|16 000 |170,63 ms |579,93 ms |224,53 ms |
|32 000 |317,78 ms |1 162,51 ms |398,03 ms |

Il y a donc un acquis d'architecture et d'exactitude, **pas une optimisation
de vitesse acquise**. La création de continuations complètes et les
consultations de file restent payées. Avec Pool activé, un gros repli
synchrone peut encore concentrer le travail : à8k rangées, un worker porte
78,8% des visites, aussi bien dans Coarse que dans cette nouvelle entrée.

## Croissance : distinguer opérations, populations et régimes

Pour la campagne Pool64/K10/s8, visites census du flux q2 complet :

| Famille |8k |16k |32k | Ratios |
|---|---:|---:|---:|---|
| Uniforme |171 895 354 |413 553 244 |1 001 201 993 |×2,406 / ×2,421 |
| Terrain |20 472 635 |42 798 408 |95 128 515 |×2,091 / ×2,223 |
| Amas |81 112 664 |239 954 275 |648 207 562 |×2,958 / ×2,701 |
| Rangées |5 742 485 |11 886 273 |24 566 835 |×2,070 / ×2,067 |

Les six postes suivis (produits front, descentes témoins, candidates,
visites census, divisions structurelles et supports) restent sous×3 sur
ces quatre séries. Ce n'est pas « tous les compteurs sont sous×3 » :

- Le volume F de préparation Pool des rangées vaut8 000/32 000/80 000,
  donc×4 puis×2,5. Les insertions passent22/8 066/24 154 ; le changement
  de décomposition est visible, sans prouver une puissance asymptotique.
- Les paires effectivement envoyées au census Pairwise après Pool sur
  amas font11 329/29 688/102 336, soit×2,621 puis×3,447. Ce coût aval
  reste à surveiller même si les visites totales progressent mieux.
- Sans Pool, les rangées gardent une masse candidate presque quadruplée
  au premier doublement (jusqu'à×3,995 selon K/s). Les groupes permettent
  pourtant des visites census autour de×2,1 : une population cartésienne
  représentée n'est pas un nombre de paires effectivement développées.
- Les populations de témoins consommées par blocs peuvent dépasser×4 ;
  ce ne sont pas des visites. Onze ratios des compteurs de scheduling
  dépassent×4 ; ils sont publiés, y compris les départs de zéro.

L'analyse garde tous les compteurs Pool et du scheduler, et sépare les
populations créditées. **Aucune borne générale sous-quadratique ni contrat
de tour n'est fermé.** Les coûts nouveaux d'offre restent bornés par les
pauses ; cette relation ne borne pas à elle seule le travail géométrique.

## Rejouer et suite

Le [runner](../../bench/run_wspd_q2_cooperative_checks.py) propose `read`
sur chacun des dossiers clos et `run --campaign qualification|tsan|smoke|scale|separations|rows_large`
avec un build neuf et un dossier de sortie. Chaque capture possède son
MANIFEST, ses records bruts/décodés et sa fermeture. `analyze.py` lit les
captures explicitement nommées ; `record_analysis.py` conserve ses lectures
normal/−O, hashes d'entrée et éventuels échecs dans un nouveau dossier.

Suite prioritaire dans la [note d'architecture](../../docs/P0_EQUIPE_PERSISTANTE_Q2.md) :
curseur de plages d'ancres avec moteur privé réutilisé, parent Pool possédé
une seule fois et bandes partageables, puis lots compacts de singletons.
Ne pas généraliser une pile49cadres par petite paire. Les stratégies
q3/q4 restent séparées et leurs moteurs ne sont pas encore implémentés.
