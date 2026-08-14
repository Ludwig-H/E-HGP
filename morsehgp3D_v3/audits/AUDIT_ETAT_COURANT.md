# Audit courant de MorseHGP3D v3

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot

Le pin relu est
`HEAD=35fcea884cb93eff24db1e7c5962f8be23d4cb04`, commit
`the kill criterion of stage one fired : refinement moves the constant, not the exponent`.
Le commit ne change pas le code producteur de son parent `3c11bc8`; il ajoute
les audits et le reçu de rampe. Le worktree du reçu était propre. Le worktree
courant ne l'est pas : outre la consolidation documentaire autorisée, Claude
modifie concurremment le probe WSPD, CMake et les nouveaux probes SOC64.
L'auditeur ne touche pas ces fichiers et ne transforme jamais leur état
intermédiaire en statut du `HEAD`.

Empreintes SHA-256 du code **au HEAD mesuré avant les deltas live** :

- `prototype/ball_event.hpp` :
  `eedd8521c31fa7963506b4fc1030eb6f92491d3d79f0e8356641c7035660b24a` ;
- `prototype/ball_event_probe.cpp` :
  `4a85f6cbdd74054160266ee2bbb1dd13a1d54fb7ffb9ce522855aff93be3e793` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `cfddfc89222a9179086f99b247abf933cc24f2d22f2d2422099b86aebad8ad74` ;
- `CMakeLists.txt` :
  `3be8e878ea86f8a406e01bcf1e21e5a6986929d66420323118095c8a60c4b223`.

Les écritures de l'auditeur restent limitées à `README.md`, `PROPOSITION.md`
et `audits/`. Les suppressions documentaires déjà présentes appartiennent à la
consolidation concurrente ; elles ne sont pas attribuées à cette passe.
L'auditeur n'a pas utilisé GCP ; la session de Claude consignée au commit a
utilisé une G4 SPOT comme hôte CPU et l'a certifiée `TERMINATED`.

## Verdict

Le contrat G4 reste ouvert. Il n'existe encore ni source u16 reçue, ni stage
`0B`, ni `BenchmarkOutputContract-v1`, ni campagne p95 à 50 000 points.

Le progrès live se décompose en trois probes utiles, mais non reçus comme
chaîne produit :

1. `BallFormToBallEvent-v0` forme des sphères et des census sur petit domaine ;
2. `PointHypergraphBottleneckClosureProbe-v0` compare Kruskal et Floyd sur un
   même hypergraphe de points ;
3. le raffinement local réduit le résiduel `E4` du certificateur central, au
   prix d'un front et d'un nombre de recertifications supérieurs ;
4. la rampe à 50 000 points rejette la configuration centrale mesurée sur
   `eight_clusters`, sans réfuter les certificateurs rectangles corrélés.

Les noms « 0A fermé », « stage 0B » et « le raffinement paie » dépassent les
preuves disponibles.

## Nouvelle source candidate : `CKPairTape -> WST3 -> WST4`

L'audit reçoit la construction mathématique comme proposition exacte et
GPU-factorisable, pas comme implémentation :

- une WSPD Callahan--Kosaraju canonique partitionne toutes les paires q2 en
  `O(s^3 n)` rectangles physiques sans les développer ;
- pour chaque rectangle `R=(A,B)`, les cellules Morton d'une échelle liée à sa
  boule `B_R` et rencontrant `3B_R` couvrent tout carrier d'un support dont
  `ab` est l'arête maximale ; l'owner longueur/`EdgeKey` donne un unique
  `OwnedCK-WST3(A,B,C)` ;
- les couples non ordonnés de ces cellules donnent de même un unique
  `OwnedCK-WST4(A,B,C,D)` ; q4 recertifie directement les quatre
  barycentriques du circumcentre.

Les nombres de blocs conditionnels sont `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)`. Ils ne bornent pas les masses logiques
`|A||B|`, `|A||B||C|` et `|A||B||C||D|`. Chaque bloc reste paresseux jusqu'à un
consommateur factorisé reçu ou au preflight atomique de sa vraie sortie.

Une fixture u16 de 64 points interdit toute cascade de rang : un q4 régulier a
rang 4 alors que ses six arêtes q2 et ses quatre faces q3 ont toutes rang 12.
`WST4` doit donc consommer les carriers aigus géométriques pré-rang, jamais les
événements q3 retenus. Le rapport et le recalcul exact sont dans
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).

Avant de multiplier les carriers, `JungDiskDepth9/8` restreint les centres au
disque imposé par Jung dans le plan médiateur. Des groupes disjoints de trois
IDs au plus peuvent y fermer q3/q4 même lorsque le LP sur tout le plan échoue.

## P0 : `0A` reste ouvert sur u16

- les constructeurs q3/q4 rabattent vers `int64` des numérateurs qui atteignent
  respectivement 81 et 67 bits sur des fixtures u16 valides ;
- les chemins de clé et de positivité forment ensuite des intermédiaires au-delà
  de 128 bits ;
- `be_level_num` et `be_level_cmp` multiplient en `i128` signé avant de tester
  l'overflow. Sous UBSan, le mutant de clé existant déclenche un comportement
  indéfini à `ball_event.hpp:290` ;
- le vert Release de ce mutant n'est pas causal : `desaccords=0`,
  `fold_desaccords=0`, puis `fautes=263`, dont 262 proviennent de
  `runs.size()` ajouté sans comparaison à une vérité et une de l'erreur
  numérique ;
- le juge partage encore des identités, calculs et filtres avec le sujet ;
- `PointId`, epoch, profil, schéma, lanes, complétude et publication
  transactionnelle ne forment pas encore une ABI reçue ;
- le census reste payé par support avant le RLE par `BallKey`.

Autorité, valeurs exactes et fixtures :
[`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## P0 : le fold live n'est pas `0B`

Le sujet écarte chaque `SphereRun` dont la disposition n'est pas `kRegular`,
puis le sujet et son juge consomment les mêmes runs, membres, niveaux et rangs.
Le nominal grid peut ainsi imprimer `refus_domaine=99` puis
`ball_event accord=OUI fold=OUI`.

Le calcul unionne `I_B union U_B` dans une seule DSU de `PointId`. Il juge une
primitive de goulot pour `K=1`, pas Morse/HGP aux ordres `1..10`. Dès `k=2`,
deux générateurs qui partagent un seul point doivent rester séparés, alors que
la DSU de points les fusionne. Manquent notamment :

- générateurs/facettes par ordre et lanes ;
- lots de niveaux égaux avec racines pré-lot gelées ;
- naissances, multifusions, coverage et continuations ;
- neuf applications verticales et payload officiel ;
- preflight complet et zéro sortie sur événement unsupported.

Le juge Floyd est en outre un oracle borné : ses deux matrices d'entiers
coûteraient environ 20 Go à 50 000 points et sa fermeture ferait
`1,25e14` triplets. Il ne peut devenir l'architecture produit.

Contre-audit et fixture `k=2` :
[`AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md).

## Raffinement local : levier reçu comme diagnostic seulement

À `n=3000`, `s=8`, `window=512`, une exécution locale a mesuré :

| famille | profondeur | masse q4 ouverte | terminaux | recertifications |
|---|---:|---:|---:|---:|
| `uniform` | 0 | 1 027 538 | 885 188 | 108 858 186 |
| `uniform` | 4 | 464 599 | 1 221 936 | 193 020 841 |
| `eight_clusters` | 0 | 4 045 644 | 363 018 | 31 538 327 |
| `eight_clusters` | 4 | 2 597 699 | 1 182 988 | 199 169 436 |

Le ledger terminal est exclusif et final dans ces six rejeux. En revanche les
anciens compteurs `bank.closed`, `pending_lane` et `mass_closed_q2` comptent des
tentatives parentes avant leur scission, puis leurs enfants : ils peuvent
dépasser 100 %. Séparer impérativement `AttemptStats` de `TerminalLedger`.

Le prochain jalon utile est `ProofCarryingLocalRefinement-v0` : un enfant
hérite les spans `ALL/NONE` du parent et ne rejoue que sa frontière `MIXED`.
Mesurer ensuite le coût transitif `E4 -> M4 -> BallKeys -> census -> fold`, pas
la seule pente de `E4`.

Audit complet et défauts de la recette G4 :
[`AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md).

## Rampe 50 000 : no-go borné, pas pivot automatique vers les cages

La session G4 CPU du commit publie quarante `code=0` et une provenance
substantielle. Les pentes q4 post-calculées sont arithmétiquement correctes.
Pour `eight_clusters/r4`, elles valent `1.898146 / 1.909154 / 1.911063`, avec
`pending=0` aux quatre tailles. Le seuil numérique préannoncé `1,7` justifie
donc l'arrêt d'ingénierie de
`CentralBall209 + DVT scalaire + s=8 + window=512 + raffinement r=4` sur cette
famille et cette graine. Le protocole plus large demandait plusieurs graines ;
ce n'est pas une réception asymptotique.

La portée s'arrête là :

- le script lance une taille par processus et ne calcule aucune pente ;
- `--max-slope=9` est inerte et aucune gate `sum_E4` n'est armée ;
- la complétude compte les lignes `code=` sans tester explicitement leur
  valeur. Un code non nul fait actuellement disparaître la ligne sous
  `errexit`, donc le fichier est rejeté, mais le diagnostic promis est perdu ;
- `terrain` contient du pending q4 à 25 000 et 50 000 ;
- le filtre retire recertifications, temps et HWM, donc le « prix exact » du
  front n'est pas mesuré.

`uniform` seule ne peut qualifier le SLO : `TEST_PLAN` §14.5 et G6 exigent
Poisson uniforme et le mélange équilibré de huit amas. Le prochain mouvement
est un diagnostic `SOC64+LP`, puis seulement les cages si la perte mesurée le
justifie.

Réponses complètes aux trois questions de Claude :
[`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md).
Le détail contractuel du reçu et des continuations est dans
[`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## Déblocage mathématique prioritaire

Le faible gain du classifieur scalaire `D/V/T` ne réfute que des extrema
décorrélés. Il ne réfute ni le spindle ponctuel, ni les rectangles corrélés, ni
le disque de Jung.

La prochaine ablation doit être `SOC64-shadow-q4`, avant davantage de
raffinement. Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q4
exige `H>0` et `3H^2>EX`. Si les 64 couples de coins de
`(C-A) times (B-C)` passent, tout le rectangle est `ALL`; le premier échec est
seulement `UNKNOWN`. La fixture axiale où toutes les différences sont
colinéaires positives ferme q4 par `SOC64` alors que la borne scalaire échoue.

Ordre expérimental recommandé :

```text
central q4 MIXED
  -> SOC64 shadow, cap propre, union de preuves disjointe
  -> mesurer masse créditable et early exits
  -> brancher SOC64 avant le raffinement
  -> CORNER512 seulement si son amortissement est plausible
  -> JungDiskDepth8 sur le résiduel avant WST4
  -> LP global seulement comme oracle comparatif
```

Le delta live `SOC64` a reçu `16/16` portes isolées. Son premier raccord WSPD
additionnait à tort le crédit SOC d'un ancêtre aux crédits centraux du même
sous-arbre. Le contre-audit a forcé sa réécriture : `cred` conserve la baseline,
`ccred` rejoue l'union, SOC vient après les fallbacks, et `cmask` arrête une
branche combinée à son premier `ALL`. Sur le replay borné `uniform,n=120`, les
`624` verdicts SOC-`ALL` et `3873` triples ont zéro faux ; l'union ferme `41`
terminaux de masse `95`, contre `127` et `316` avec l'ancienne somme fautive.
La contradiction est donc observée, mais aucune CTest ne grave encore ce replay,
le surcompte strict ni un mutant `sum_instead_of_union` : le raccord reste non
reçu.

Le raccord n'a pas non plus le cap de 4096 tâches annoncé. Un diagnostic local
à `n=1000` en a soumis environ `988000` et `3,69` millions de couples ; ce
shadow doit être échantillonné hors chrono ou rendre un statut tronqué explicite.
Ces chiffres ne sont pas une extrapolation recevable vers le SLO.

`JungDiskDepth8` restreint le plan des centres au disque
`||y-d||^2<=D/2`. Une fixture à huit groupes disjoints ferme ce disque alors
que le LP global échoue même à profondeur un. Le LP à 3280 appels reste donc un
oracle pairwise, jamais le hot path ni le diagnostic d'une « vraie pénurie »
sur le domaine Morse.

Preuves, largeurs, fixtures et cascade :
[`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Contre-audit de l'autre auditeur

L'autre auditeur a correctement découvert l'owner décidé sur
`GenerationRank` plutôt que `PointId`; Claude l'a réparé localement au commit
`f516198`. Il a aussi correctement relevé l'incomplétude de `0A` et la nécessité
des lots/verticales.

Ses propositions LP/cages sont recevables après les restrictions intégrées
dans la proposition consolidée : complétude LP seulement sur le pool mondial,
largeur du constructeur distincte du vérificateur `F`, traitement des bases de
rang inférieur, et recalcul de la fleur après réduction d'une cage. Les comptes
`32/36` ne valent que pour des tétra-cages : les maxima six-sites deviennent
`64/72`; `P=48` est une capacité q4, pas une existence. Pour une base positive
minimale 3D, le seuil angulaire suffisant est `delta>=4h-3`, soit `29` pour
huit cages, et n'est jamais une condition nécessaire. Son oracle de profondeur
ne prouve pas l'absence d'un support source et son arbre à 3280 LP ne doit pas
être présenté comme GPU/hot path.

Sa restriction « WSSD compacte ne borne pas la sortie » est correcte. Sa
conclusion « WSSD seulement broad phase » est trop étroite : avec partition
CK, owner total, certificats `[L,U]` et blocs paresseux, `OwnedCK-WST` devient
une source factorisée exacte. Elle ne devient toujours pas une liste sparse.
De même, le théorème des faces aiguës q4 est utile uniquement avant le rang ;
la fixture 64 points interdit toute cascade depuis les événements q3 retenus.

Le contre-audit détaillé corrige aussi les anciennes confusions entre q2 et q4,
cutoff kNN, taux empirique et borne structurelle.

## Rejeux ponctuels

```text
ball_event Release ciblé : 10/10 verts
suite ciblée ball_event/WSPD/Gamma/postings/saturated : 87/87 verts
faces q4 exactes : 5/5 ; q3 côtés hors rang : 6/6
fixtures centre_cell arité 3/4 + mutant cascade : 3/3
SOC64 isolé : 16/16 ; raccord WSPD shadow : non reçu
suite complète : interrompue après 28/734 terminés, tous verts
grid n=16 : refus_domaine=99 puis fold=OUI
clusters n=5, coord=4 : timeout après 2 s, capacité non preflightée
UBSan mutant clé : overflow signé à ball_event.hpp:290
```

Ces rejeux falsifient des claims précis ; ils ne qualifient ni exactitude
publique, ni performance.

## Ordre bloquant

```text
réparer 0A u16 et isoler les juges de mutants
  -> raccorder BallEvent aux autorités Gamma par ordres/lots/verticales
  -> recevoir 0B et le payload borné
  -> recevoir CKPairTape q2 et ses certificats [L,U]
  -> SOC64 union-disjointe + JungDiskDepth9/8 avant expansion
  -> OwnedCK-WST3 puis WST4 pré-rang, avec fixture de non-cascade
  -> raffinement porteur de preuves sur les tâches encore MIXED
  -> mesurer F2/F3/F4, M3/M4, BallKeys, census, H et coût transitif
  -> seulement alors portage device et campagne G4 50k
```

La proposition consolidée est
[`../PROPOSITION.md`](../PROPOSITION.md). L'index court des audits est
[`README.md`](README.md).

GCP non utilisé par l'auditeur ; la session reçue de Claude est arrêtée.
