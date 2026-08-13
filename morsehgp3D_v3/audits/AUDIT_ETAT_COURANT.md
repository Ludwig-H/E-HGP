# Audit courant de MorseHGP3D v3

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Observation live — `HEAD=471715a`, réparation P0 spindle rejouée

Au 13 août 2026, le `HEAD` observé est
`471715a68950afa9bba34edc2ac5db30724ff539`, commit documentaire
`retract three claims the auditor refuted, and take the cut at the first omitted
site`. Il répond au contre-audit de la fenêtre locale. Son parent logiciel
`519ddfbaee60007e927bb148b9fb83451d7af7bc`, commit
`a judge that shares the subject's cast is not a judge`, sépare le juge spindle
dans une unité de traduction et ajoute les refus et portes P0. Le code est
identique entre ces deux pins.

Après ce pin, Claude a ouvert un nouveau successeur non commité :
`prototype/window_source.hpp`, SHA-256
`756d2da6fa3d0288739d121b490338ac74845a6eba7f83cb7b6768b092178060` lors
de sa première lecture, et `prototype/window_source_probe.cpp`, dont les
snapshots ont encore changé pendant la lecture. Ils implémentent les primitives
et un sujet borné de fenêtre locale, mais restent en cours d'écriture. Aucun des
deux n'est raccordé au CMake commité : ils ne sont ni construits, ni jugés, ni
inclus dans le verdict `39/39` ci-dessous. Leur contre-audit live est
[`AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md`](AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md).

Le snapshot rejoué est pincé par les SHA-256 suivants :

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `39530b9444cd58655ffdf14097ea0fdb0d74ac62fac1c98b64a89091f9e1f2bd` |
| `prototype/spindle_cone.hpp` | `78037fc19d0f2dae63b28745ee8741e10bd7821a8da3278032ad2dae76db0a85` |
| `prototype/spindle_cone_probe.cpp` | `36ccfd2abdf26a4eeb821122755a85335c00793bd7b414b8ce61c8fe5b91afc3` |
| `prototype/spindle_cone_oracle.cpp` | `e6dba54e1825beab7f97f131911c829e692b08fb588cac6ccb1770f196deeca8` |
| `prototype/spindle_cone_oracle.hpp` | `f049768a45061d121f3dc9baf5beb57c2fe540d139894c82ce0e1c19ecde4d29` |
| ELF Release | `e05a2065b630475361325b22677a29db30067800a1c99f37af39dedf53a12ccd` |

La configuration est Release `-O3 -DNDEBUG`, GCC `13.3.0`, CUDA désactivé.
Après reconfiguration et construction de la seule cible, l'inventaire rend
`39` CTests `mhgp3v_cone_`. Le rejeu indépendant :

```text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_cone_'
```

rend `39/39` en `31,42 s`. Un rejeu ciblé des quatre nouvelles obligations
rend `4/4` en `3,65 s` : `LLONG_MAX` est refusé en code `2`, la cardinalité
réduite est refusée en code `2`, les trois lanes ont zéro désaccord et le
mutant `cone-ignore-inherited` est tué par le juge en code `4`. Ces temps CPU
sous charge ne qualifient aucune performance.

Cette campagne reçoit localement les quatre réparations P0 suivantes : domaine
`smax` fermé avant cast, cardinalité demandée, décisions q2/q3/q4 jugées
séparément et porte permanente du mutant d'héritage. L'oracle redérive ses
seuils et son arithmétique dans une unité distincte ; son selftest compare les
deux limbes à `BigInt`. Le faux vert historique `380/380` est donc clos sur ce
pin.

Ce vert ne reçoit toujours pas le producteur industriel :

- aucun CTest ne fait mordre les caps et les deux scalaires résiduels ne sont
  ni une partition par identité, ni un reçu rejouable ;
- les rampes mono-ELF banques 48/96 gardent deux pentes rouges sur toutes les
  familles et tous les compteurs : les dernières pentes de tests de coins
  restent `1,452` sur `uniform/96` et `1,438` sur
  `eight_clusters/96`, au-dessus de `1,35` ;
- la rampe commise jusqu'à `n=16 000` confirme des pentes dominantes rouges ;
  sa colonne `target_visits` duplique les visites k-NN et ses temps sont
  contaminés ;
- le probe reste CPU/front-only, hors payload officiel, sans producteur CUDA
  raccordé ni mesure `warm_e2e`.

Le port littéral de cette DFS par endpoint reste donc **NO-GO avant G4**. La
primitive ponctuelle devient un oracle borné reçu, pas la route 50 k. Le
contre-audit logiciel complet est
[`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md).

La réponse à la note de Claude accepte la coupure stricte au premier site omis,
mais refuse sa promotion en source globale : le census inclut `U_B`, Source S
ne borne pas le shell, l'owner vient après découverte et les candidats locaux
refusés ne couvrent pas les supports jamais proposés. Elle exige aussi un merge
global par niveau et lot :
[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md).
Le candidat suivant met en concurrence dominance dans les 432 sous-cônes,
groupes coniques et relation-tree/WSPD sur un même ledger avant CUDA :
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).

Plusieurs portes `anchor` à planchers restent contournables par
`PASS_REGULAR_EXPRESSION`. La source CUDA anchor omet en outre le
nouvel argument `density_guard` dans ses appels et son ABI ; elle
n'est pas compilable telle quelle. Aucune session GCP n'a été lancée pour ce
delta et aucun résultat device n'est reçu.

## Pin commité antérieur au delta spindle — `HEAD=2a205f3`

Au 12 août 2026 pendant la reprise d'audit, le pin commité est
`2a205f3508abc7a20ea564eef55ed8e1f0f6f67d`, commit
`compare the two ledgers, not just the supports — and kill the anchor the
moment its budget is gone`. Avant le delta spindle/cône décrit ci-dessus, le
code et le CMake du worktree étaient propres ; les seuls deltas alors observés
étaient les réponses des auditeurs dans `README.md`, `PROPOSITION.md` et
`audits/`.

Le commit reçoit le retrait de `theta` du chemin par défaut, paramètre `smax`,
compare les deux moteurs sur trente-six compteurs, rend la garde de densité
opt-in et prend la mort par budget dès que suffisamment de `Llow>0` ont été lus.
L'inventaire Release est `573` CTests, dont `56` `mhgp3v_anchor_`. Sur l'ELF
SHA-256 `f699f8d1ff17557626325b2844d77748c649e306cd0e25b324d62c7d49442d73`,
le rejeu indépendant
`ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_anchor_'` rend
`56/56` en `75,50 s`.

La garde de densité reste seulement fail-open pour Source S : elle renonce à
des tentatives de prune sans inventer de support, mais sa densité locale ne
borne pas la population ailleurs. Son ablation ne gagne aucun prune sur les
trois familles et dégrade deux temps sur trois ; l'audit recommande donc de la
sortir du chemin produit après un dernier reçu pincé. `front_mass_closed`
compte en outre tous les points d'un nœud, pas seulement les `PairId b>a`; il
ne ferme pas encore le ledger non ordonné.

La mort précoce par budget est exacte et réduit `site_evaluations`, mais elle
intervient encore après `gather_sites`. Sur `eight_clusters n=500`, les nombres
de paires q4 et de tests census restent inchangés : c'est un facteur utile, pas
la fermeture du verrou. La reprise mathématique proposée construit une banque
k-NN une fois par endpoint, puis couvre des nœuds partenaires entiers par des
cônes cibles exacts à huit coins. Elle doit opérer avant `PairId`; les
`UNKNOWN` sont envoyés par blocs au résiduel sous caps, jamais descendus
systématiquement jusqu'aux paires.

La note amas conserve par ailleurs une provenance incohérente : au même ELF du
pin précédent, `n=150/200/300` reproduit ses compteurs q4 mais publie
respectivement `1/2/40` prunes et `11 174/19 899/44 831` candidats, non
`0` et `C(n,2)`. Une boule médiane centrée dans le vide n'implique pas un
spindle universel vide. Les pentes prouvent un NO-GO empirique de la boucle
actuelle, pas une complexité cubique asymptotique.

Le nouvel
[`AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md`](AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md)
répond désormais aux six questions de Claude et fixe le classifieur spindle,
le lift bloc `A×B×C`, le cône cible exact et leurs ledgers/gates.

Le contre-audit courant répond aux cinq questions de Claude, sépare le bug
historique `smax` de sa réparation et apporte deux résultats utiles : le
classifieur collectif de relation aiguë avant `PairId`, sans retirer le second
carrier de la lentille fermée, puis la génération q4 par niveaux peu profonds
d'une ancre fixe. Le nombre de centres distincts à profondeur `k` y est
`O(m(k+1))`; cette borne ne couvre ni la masse des ancres, ni les rescans LBVH,
ni les cosphères lourdes. Voir
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).

La configuration du pin historique `9bcd137` exposait `550` CTests, dont `33` portes
`mhgp3v_anchor_`. Le premier rejeu local sur l'ELF Release SHA-256
`59425e5708251fe890b57ea271887735fe8e9ab3a30f6cb0f9951e12c514e7f3`
rend `32/33` en `503,57 s` : `mhgp3v_anchor_mutant_census` est tué par signal
après environ 62 s, et le wrapper refuse justement d'assimiler un crash à un
rejet contractuel. La relance isolée de cette porte passe `1/1` en `39,15 s` ;
le rouge complet n'est donc pas reproductible isolément et reste une anomalie
de ressource/session, pas un accord `33/33` d'un seul run. Ces portes ne
reçoivent de toute façon ni oracle rationnel indépendant, ni
`(BallKey,I_B,U_B)`, ni CUDA/G4, ni `BenchmarkOutputContract-v1`. Le contrat
50 k/1 s demeure donc ouvert.

## Pin CPU stable — producteur par ancre au `HEAD=760469d`

Le pin CPU historique contre-audité est
`760469df0320a1f081be586a0a352034b38c6a40`, commit
`run the same function on the CPU and on the GPU, then difference it`. Le
worktree était propre au pin final. Ce successeur ajoute un pipeline commun
hôte/device et une cible CUDA opt-in ; aucune compilation CUDA, exécution G4
ou mesure 50 k n'est encore reçue. Le détail pincé, les cinq réponses à Claude
et les preuves nouvelles sont dans
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).

Le propriétaire par plus petite arête maximale est mathématiquement exact
pour tout support propre positif : il couvre chaque support et en choisit une
unique occurrence. Les bornes mono-ancre `ext/4`, la face aiguë adjacente q4,
le disque q4 mutualisé pour q3 et l'égalité de shell q2 sont également sûrs
dans leur domaine. Ils ne reçoivent toutefois pas encore la source : le mode
`--verify` partage solveurs, positivité, owner et census avec le sujet et ne
compare que `SupportKey` et `p`, pas `extra`, `I_B` ni `U_B`.

Un défaut P0 reproductible réfute le contrat CLI. Le sujet accepte
`4<=smax<=24`, mais fixe son front à `10/9/8` et son enveloppe au neuvième
niveau, constantes valides seulement pour `smax=11`. Sur l'ELF Release
SHA-256 `83685f02a8c63e565177723c47efad53a7770cdc63e17f2d7241a1abfd284082` :

```text
mhgp3v_anchor_source --points=140 --family=terrain --seed=3 \
  --smax=24 --engine=pipeline --verify
```

rend code `1`, `exhaustif=24633`, `produit=24686`, `accord=NON`. Il faut soit
refuser `smax!=11`, soit employer les seuils dynamiques `smax-1`, `smax-2`,
`smax-3` et le niveau commun `smax-2`.

Les `28/28` CTests `mhgp3v_anchor_` passent localement en `229,97 s`. Ils
exercent les quatre familles, deux fixtures, les mutants, le pipeline commun,
les refus et les planchers ; ils ne couvrent ni le défaut `smax`, ni un oracle
rationnel indépendant, ni CUDA/G4, ni le payload officiel. La note de Claude
qui disait qu'aucune porte CMake n'existait est donc historiquement dépassée,
mais son claim de validation de « tous les certificats » reste trop fort.

Le modèle de coût de cette note oublie un facteur deux : `candidate_pairs` ne
conserve que `b>a`. Une boule de rayon `4,8 rho^(-1/3)` donne environ `232`
paires non ordonnées par point, pas `463`; la coalescence exacte des trois
lanes de milieu vaut `233,807309n`. Les mesures `227/351/465` ne ferment donc
pas une constante attendue. Le compteur omet aussi les paires q4 non aiguës
mais parcourues, les tris, le census et tout l'aval.

Le pipeline commun réserve `222 208` octets de scratch par slot, donc
`3 640 655 872` octets (`3,39 GiB`) au défaut de 16 384 slots avant LBVH,
sorties et workspace. Il effectue deux tris par insertion et jusqu'à
`523 776` paires q4 par ancre. Le verrou G4 demeure la matérialisation locale
quadratique `C(n_lens,2)`, pas la déduplication des supports.

Le nouveau résultat utile est le certificat collectif de **lentille aiguë**.
Pour une arête maximale `ab`, tout q3 positif a son carrier `x` dans
`||x-a||^2<=D^2`, `||x-b||^2<=D^2` et
`(x-a) dot (x-b)>0`; tout q4 positif en a au moins un. Des extrema AABB
corrélés exacts classent un produit `A*B*C` en `NONE/ALL/UNKNOWN`. `NONE`
ferme une masse d'ancres avant `PairId`; `ALL` doit rester factorisé. Ce prune
ne ferme pas seul `eight_clusters`, car un carrier commun peut servir
`Theta(n^2)` paires. La reprise prioritaire est
`carrier block -> center/rank cover -> microtuile`, avec patches half-open et
niveau top-`smax-2`, avant tout sweep q4.

Le contrat industriel reste inchangé : la seconde porte `warm_e2e<1 s` inclut
validation, transfert, index, source exacte, census, q3/q4, resolver, fold,
dix forêts, verticales, lots, certificat minimal et retour hôte. Le nouveau
kernel horizontal ne peut pas porter ce SLO isolément.

## Snapshot `407d4d1` désormais historique

`HEAD` contre-audité :
`407d4d1b2745f03a7237080a75daba1c7122ea0a`, commit
`parallelise by independent subtrees and require the receipt to be identical`.
Il ajoute des workers CPU par sous-arbres et quatre portes associées. Il ne
contient ni kernel CUDA, ni producteur du payload officiel, ni mesure G4/50 k.
Le facteur `1,44` de son message de commit n'est attaché à aucun transcript
versionné. Le worktree observé pendant ce contre-audit contient ensuite un
delta de Claude dans `prototype/centre_cell_source.cpp`, en plus de
`README.md`, `PROPOSITION.md` et des audits. Aucun code n'a été touché par les
auditeurs.

| objet courant | SHA-256 |
| --- | --- |
| `CMakeLists.txt` au `HEAD` | `3cb2d3ac4ef3e407607283e588c18682604852456029d91673f2dd928e14b87c` |
| source centre-cell au `HEAD` | `323a08489ffa4f05d9726c2515dc528483b69386e7347e21605fe9a71f81e6f0` |
| ELF Release centre-cell correspondant | `7ed9fcfcedbbce3226388fac9d1088006873b81e14a3cc3fdd315a3af4bbb608` |
| juge rationnel borné au `HEAD` | `b39d8d295f5c2edde75d6f88cb2bbf8bffb75440267b69ea677b5d93288d8658` |
| ELF Release du juge correspondant | `cfa11f3f4875b5be91b87beebce9eff7117f915aec8399fff829e5915fbe92da` |
| checker Python sujet--juge | `3671b7ab53c73f845524aca402f2779a949fc28d1a800a9214d59cef3c4912f6` |
| checker invariance workers | `86f4840a2221833558481127298aab383ad89982715737c7e31200c8cdc96fc1` |
| ELF historique immuable de la campagne gelée | `423797e9964538f42701660d8baaf492b302f801a4aeb4b0df1b183986a5a037` |
| transcript gelé complet, 254 lignes et footer | `f02b7c4c2793ef0ffbb2ac879c274ff4298bcec90ff5ea64fba3d64352e7ea59` |

La configuration Release recense `517` CTests, dont `53` préfixés
`mhgp3v_centre_cell_`; quatre portes payload sont séparées. Sur les objets du
`HEAD`, le contre-audit rend `53/53` en `238,06 s` et `4/4` payload en
`0,45 s`. Ces sorties sont observées localement et non archivées; la suite
globale `517/517` n'a pas été rejouée. Le résultat du delta non commité est
rapporté séparément ci-dessous.

Les trois portes workers normales comparent le stdout hors ligne `cloud=` entre
1/2/5 workers; la porte différée compare seulement quelques lignes agrégées.
Elles n'exigent ni `--judge`, ni identités complètes, ni plancher de tâches ou
de workers actifs. Le reçu omet `harvest_depth`, tâches récoltées, workers
actifs, octets copiés et high-water par worker. L'histogramme de profondeur
différé n'est plus un défaut live : `407d4d1` transporte et restaure la
profondeur, mais aucune porte dédiée ne reçoit encore cet histogramme.

Au `HEAD`, un défaut grave est reproductible : `--multiplicity --threads>1`
publie un histogramme partiel avec code zéro. Sur
`--points=40 --smax=4 --family=uniform --seed=11`, un worker rend
`multiplicite_total_occurrences=22535` pour `lifts_built=22543`, tandis que deux
workers rendent `7012` pour les mêmes `22543` lifts. Les moteurs workers ne
reçoivent pas la configuration et leurs maps ne sont pas fusionnées. Le delta
non commité tente cette réparation : l'eager devient invariant entre
1/2/5 workers, mais les dégénérés restent absents, les rejets de rang sont
classés trop tôt comme pertinents et le mode différé rend zéro occurrence,
`-nan` et code zéro. La combinaison n'est donc pas reçue.

### Delta live non commité de Claude

| objet | valeur |
| --- | --- |
| source centre-cell | `72e490932e5553796de0f3322f8d43d2ddfb7c5d720e04e4a0d5c81578aa862e` |
| diff contre le `HEAD` | `+205/-8` |
| ELF Release correspondant | `772069ff1891fb0f36a2aa2d4851c42d22e41cbdd1b19b5295c0d5a269c16dc8` |
| inventaire CTest | `517`, aucune nouvelle porte `unique-keys`/multiplicité |

`UniqueKeyReceipt-v1` ferme correctement le nombre d'occurrences collectées
sur les petits cas rejoués et donne à `terrain,n=400` : `1 768 790`
occurrences, `246 263` clés uniques, facteur `7,1825`. Il reste un diagnostic
CPU : toutes les arités sont stockées sur huit octets, le tri q4 est colex et
non préfixé par face, le cap par moteur peut alterner code 0/3 sous la même
commande selon le scheduling, ne borne pas le HWM réel et `cap=0` est illimité.
Le champ reçu `flux` varie aussi entre runs identiques. Aucun résultat 50 k,
CUDA ou G4 n'est attaché à ce delta.

Après reconstruction de ce delta, les `53/53` CTests centre-cell passent en
`271,21 s`; les quatre workers passent aussi isolément en `70,93 s`. Aucun de
ces tests n'invoque `--unique-keys` ni `--multiplicity`, donc ce vert ne couvre
aucun des défauts ci-dessus. Les commandes bornées avec `--judge` restent
d'accord en eager/différé et avec 1/5 workers.

Le détail des reproductions, de la correction top-`(12-q)` et des gates G4 est
dans
[`AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md`](AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md).

La porte `mhgp3v_centre_cell_independant_voit_le_mutant` reste vacueuse : le
sujet refuse `--inject=rank-closed` sans `--judge`, le driver rend code 2, puis
`WILL_FAIL` transforme ce refus en vert. Le rejeu courant affiche exactement
`REFUS : le sujet rend 2` et `REFUS : un mutant sans juge ne prouve rien`. Le
juge lui-même est sensible : alimenté directement par le stdout mutant, il rend
code 1 avec `510` vérités, `504` identités et `6` q2 manquantes. La porte doit
exiger ce code et `DESACCORD`, tandis qu'un refus code 2 reste rouge. Voir
[`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md).

Le transcript historique du commit `e6f1ef3` contient neuf commandes, neuf `rc=0`, un ELF identique
avant/après chaque cas et `RAMPE TERMINEE`. Il ne couvre que `terrain`,
`uniform` et `scanline_single_pass`; son driver temporaire n'est pas archivé,
l'en-tête associe `git_commit=64cf6fe` à la source postérieure `dbaa2e0`, et les
temps ont subi de la charge concurrente. Il s'agit donc d'un diagnostic
count-only fermé, pas d'une rampe contractuelle ni d'un benchmark.

Sur `uniform`, les deux pentes de cellules/lifts/supports valent
`0,913/1,078/1,058`, puis `1,159/1,032/1,042`. À 50 000 points, le diagnostic
publie `21 395 212` supports pour `839 582 666` lifts, soit `39,242` géométries
par support; `684 722 232` propositions, soit `81,555 %`, meurent à l'owner.
Le verrou GPU visé est l'amplification avant `SupportKey`, pas la sortie. Les
lanes q2/q3/q4 contiennent `96 241 855 / 352 786 093 / 390 554 718`
occurrences : elles occupent environ `6,33 Go` en `u32/u64/u64` seulement avec
un `DensePointIndex:u16` lié par `cloud_epoch` aux `PointId` durables. La table
de remap, le workspace, les listes et les sorties sont hors compte. Le nombre
de `SupportKey` uniques et le high-water complet ne sont pas mesurés.

Les défauts live non couverts par ce vert restent : description inexacte de la
direction d'adjugée comme normale des moindres carrés hors rang deux; commentaire
strict de `rank_cell` alors que seule la version non stricte est vraie; overflow
signé possible avant la saturation de `work`; terminal stall sans hard-cap
d'adjacence/K4; paramètres stall et normale absents du reçu. Le théorème `rank_cell` lui-même est sûr sous
`U subset mine`; le stall est sémantiquement sûr seulement si son exhaustif
termine.

### Pins historiques

L'ancien registre recensait `488` CTests, dont `28` préfixés
`mhgp3v_centre_cell_`. Ces 28 passent sur le couple historique
`dbaa2e0.../423797e...` en `202,12 s`; sortie SHA-256
`ac8063615912a8272c1e781f3b1baf8381ecc056180abbb2cd9c266d7861cd58`,
`LastTest.log`
`ac5774d57f40e1e785f62baf666f477a34388b0ac1723f1cb35c1c8c6e61e750`.
Le temps est contaminé par la campagne concurrente, mais le résultat fonctionnel
est reçu dans sa portée bornée. Il n'inclut ni mutant différé sémantique, ni
`owner_multiple` fail-close, ni HWM d'octets. L'ancien inventaire `484/24`
reste historique.
Les pins ci-dessous restent les observations historiques antérieures.

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` à 13:50 UTC | `f663ada0ecbedb63a5bb651915bb41dcf3f12da4a96b34f7be5b806c9b4029cd` |
| `prototype/certified_locality_probe.cpp` | `e687b62787631d31c2fd5c4211e21fee808ac4f53edbaf72bfb0b9669dd4f20f` |
| `oracle/locality_census_judge.cpp` | `a7812b3959a2a0752a7ac6413c26947eec2e763546c979a6695439786de7ac65` |
| `prototype/ball_front.hpp` | `221356332743af11481a5387d65f6d27e0ec2b0ce0e10e2118f3796bb763d490` |
| `prototype/ball_front_probe.cpp` | `0ebe3388084c70a933df60fe9ef2209217f5962acc8042ae3f4d77ef211901ce` |
| binaire Release front q4 | `2e471ab830fa9347e48c96149a2cddce4c292c5e4acf60ff55b3a3c3ca8215dc` |
| `prototype/order_k_flats.hpp` | `a70f990adfff9bec9b810059c32ba9ec62aef95a3b06e679a3fb6f06b5af8bc8` |
| `prototype/flats_scale_probe.cpp` | `b3ecf5db981bab9741a97e828a6a00db996dab1f2be2678ddb5f50375e793a2d` |
| binaire Release sonde flats | `4f8d7da3d41b2a368ce18d1007c0544e90b22077f516253ea5c93463fb20f396` |
| `prototype/centre_cell_source.cpp` testé à 13:49--13:50 UTC | `343718804b0ada609a2f08f318c81e4cd19b1f13c0ac181f86e0ee35a25da7a8` |
| binaire Release de ce snapshot | `f927e47b4e19d5c49c1032e0d0993b2af523470a87b8895add601613294dd3a6` |
| `prototype/centre_cell_source.cpp` live à 14:05 UTC après le pin, non qualifié | `c07ce5019358910d907d0f80440ddbe0337a1e17ceb67cdc9d94d3824f18785e` |
| binaire Release local postérieur, non qualifié par 22/22 | `dcf9eef3ddbd58173bd8347de724ea620a6a0de5d903b56d9e47ee91f5b1e0ff` |
| binaire Release localité | `27c984e29c1e6a53171adf03c557669c7ff3dda392004691301e6b797757ece9` |
| binaire Release juge rationnel | `989150541dfb7a04241f5c8d9929f394eaffc1066083b916b719f6d6d25c9d75` |
| `prototype/morton_lbvh.hpp` | `23ffc797c35e24823cf346be934643b0447f8d69a5c0843b4fd090ddc548b267` |
| `prototype/pair_yao48_source.cpp` | `1af80a793058da2b69996035901c67050888be96b99b513f03c65542242a46d9` |
| `prototype/yao48_source.hpp` | `8f42e4cbaccaa8a943664b2264108ccf2765cae6bdc3938d8aa31d7581aabb3e` |
| binaire Release q2 | `f783f442b54f97f21ffa1ca1e760f041c3a58a4477ab452e8e3f66159d2a307a` |
| `prototype/warm_e2e_h0_diagnostic.cpp` | `5b442db51067a360b325237e58a1a5449a31e1396f01a18bf87ab0964d4d8208` |
| binaire Release diagnostic | `4a118dfc2cc1db718941ad20335ccc880f61d6d40b7f03a407f111942d8ee0b1` |
| `prototype/emst_boruvka.hpp` | `0e2ca1276fb5b53f9e43c7186021fca9258bf91ceee4c85679179a6d5f9e68f4` |
| `prototype/emst_boruvka_probe.cpp` | `ea56b5d75635bbb600bb899ce8c91ef6ac1c04b2f411260aa86d84500134d07f` |
| binaire Release EMST | `4cf4731df27b9ccaefcc831d06c44cdfe88fa5dad97788f3cc144d97347277ab` |
| `prototype/center_cover_mass_probe.cpp` | `fc4001b3a198ae9c095c0c563cc9500357b9b5e2fe20a8678f88a117225aada9` |
| binaire Release P1a | `9c130163a92a243c30f25157f5a817fa734b7b66fa47ec84d477bbe54155fbab` |

Toute modification d'un de ces objets rend seulement historiques les tests et
mesures qui lui sont attachés. Les snapshots plus anciens restent pincés dans
leur audit daté; ils ne sont pas reproductibles à partir du dépôt courant s'ils
n'ont pas été archivés.

## Verdict industriel

Le contrat industriel n'est pas rempli. Les seuils officiel principal
`p95 warm_e2e<100 ms` et secondaire `p95 warm_e2e<1 s` portent sur
une famille volumique favorable dont le certificat reste sparse. Le calcul
chronométré inclut validation, transfert, index, source exacte, census, q3/q4,
resolver et fold. Seul le payload `BenchmarkOutputContract-v1` — dix forêts,
applications verticales, lots et certificat minimal — doit être copié en
mémoire hôte épinglée avant l'arrêt du chronomètre.

Le seul harness horizontal nommé reste un diagnostic CPU partiel. Il construit
un LBVH, un EMST et des comptes q2, mais ne matérialise ni census, ni q3/q4, ni
hiérarchie, ni verticales, ni payload officiel. Son `partial_h0_wall` n'est
jamais une mesure du SLO.

Trois ordonnances ont été falsifiées avant G4 :

- le q2 par chambres du snapshot `2e49dcf` a plusieurs compteurs de travail
  rouges deux fois sur trois familles structurées;
- le q2 dual persistant réduit fortement le résiduel, mais ses visites témoins
  restent rouges deux fois sur chacune des trois familles structurées reçues;
- le probe P1a `b312638` recommence sa recherche témoin par bloc et présente
  des pentes presque quadratiques.

Ces refus portent sur les ordonnances mesurées, pas sur les certificats
mathématiques. Le front inverse concurrent reste un témoin q4 matérialisant,
pas une autorité de source q2/q3/q4. Sa nouvelle transition mono-requête vise le
premier croisement entrant ou sortant et transporte les lots; l'accord final est
encourageant, mais aucune porte transitionnelle rationnelle ni borne
sortie-sensible n'existe. Aucune session G4 n'est recommandée avant une source
q3/q4 complète et une réduction locale mesurée du travail q2/P1a.

Il n'existe actuellement **aucun** échantillon admissible au SLO et aucun
producteur de `BenchmarkOutputContract-v1`. Le seul reçu G4 v3 est CPU,
mass-only et s'est terminé sans GPU; les rampes q2 et P1a ultérieures sont des
diagnostics CPU count-only.

## Tests sur les octets pincés

La configuration Release `f663ada0...` observée à 13:50 UTC enregistre `482`
CTests : `34` préfixés `mhgp3v_locality_`, `10` préfixés
`mhgp3v_ball_front_`, `4` préfixés `mhgp3v_flats_scale_` et `22` préfixés
`mhgp3v_centre_cell_`. Après configuration et reconstruction ciblée, la
commande

```bash
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_centre_cell_'
```

rend `22/22` en `106,22 s` sur source `34371880...` et ELF `f927e47b...`. Les
empreintes sont identiques après la porte. Ce vert reçoit les fixtures et
mutants raccordés; il ne reçoit ni une complexité sparse, ni CUDA, ni le
payload officiel.

La configuration Release historique observée à 13:05 UTC enregistrait `468` CTests : `34`
préfixés `mhgp3v_locality_`, `10` préfixés `mhgp3v_ball_front_`, `4` préfixés
`mhgp3v_flats_scale_` et `8` préfixés `mhgp3v_centre_cell_`. Après
reconstruction des snapshots correspondants, les portes anciennes restent
historiques dès que leurs sources changent. La commande

```bash
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_locality_'
```

rend `33/34`. Le seul rouge est
`mhgp3v_locality_gravees_mutant_signe`. Le binaire rend bien le code 4, mais le
mutant est tué plus tôt par la fixture d'orientation q4 et imprime
`mutant tue par la fixture d'orientation q4`; le harness attend
`mutant tue par les valeurs gravees`. Cette porte ne prouve donc pas que les
valeurs gravées atteignent et tuent le mutant.

Le juge rationnel emploie une arithmétique indépendante des prédicats du sujet.
Il imprime trois comptes de supports et trois comptes de records portant une
extra-shell, mais les portes ne comparent que les trois premiers. Elles ne
comparent ni identités de supports, ni `I_B/U_B`, ni `BallKey`, ni owner. Il énumère
`Theta(n^4)` supports q4 puis balaie jusqu'à `n` points par support : le pire
cas est `Theta(n^5)`, contrairement au commentaire `O(n^4)`. Ses exécutions
partagées à `n=50` ont pris des dizaines de secondes; le cap `n<=400` n'est pas
une enveloppe de ressource praticable.

Aucun passage global `468/468` n'est revendiqué. Un ancien filtre
`^mhgp3v_ball_front_` a rendu `8/9`: le test de refus du juge à 500 points était
intercepté par le défaut `smax=3` et n'atteignait plus le diagnostic attendu.
Cette mesure appartient à une configuration antérieure à celle qui recense dix
tests `ball_front`; elle ne qualifie donc pas le snapshot CMake ci-dessus. Un
filtre historique `^mhgp3v_flats_scale_` a rendu `4/4`. Leur portée bornée est
auditée plus bas. Ils ne réparent ni la suite localité rouge ni le pipeline
produit. Les contrôles documentaires sont rapportés seulement après la
consolidation finale.

Le snapshot historique `centre_cell` `fd734092...` et son binaire
`b2b430bb...` avaient été reconstruits ensemble. Leur filtre CTest rendait
`8/8` en `3,18 s`; `--fixtures` exerçait égalité, owner et les deux
contre-fixtures inter-arités. Ces octets ont été remplacés et ne sont plus
reconstructibles depuis le worktree courant.

Le successeur `34371880...` raccorde désormais sept mutants, dont `drop-ties`
et `arity-cascade`; ils passent dans les 22 portes. `strata-stop`, qui arrête la
promotion au premier bucket, reste seulement enregistré dans le CLI et sans
CTest. L'égalité de fermeture découle du flot : la garde initiale établit
`r>=e0`, puis chaque promotion pose `h=r` et les scans suivants ne peuvent
qu'augmenter `r`; sortir sur `r<=h` donne donc `r=h`. Une assertion explicite
serait défensive, mais le trou reçu est le mutant non raccordé.

## État q2 Yao48/LBVH

### Preuves et mesures durables

Le prédicat dual est exact. Pour une ancre `p`, une cible `q` et un témoin `w`,
`A(p;q,w)=(q-p) dot (w-p)-||w-p||^2=-Phi`; le minimum sur deux AABB est la
somme des trois minima axiaux, chacun pris sur les quatre couples
d'extrémités. `L_p(Q,W)>0` certifie strictement tout `Q` contre tout `W`. Le
majorant utilisé pour `U<=0` est sûr, mais son arrondi supérieur n'est pas un
maximum entier exact; il ne peut produire que des faux négatifs de prune.

Le reçu historique `c70974e` contient trois triplets structurés complets. Les
survivantes, boîtes et tests du classifieur passent sous `1,35`, mais
`dual_witness_visits` donne respectivement `1,498/1,929`, `1,618/1,673` et
`1,722/1,739`. Le successeur pointwise v2 reste rouge sur les six pentes de ce
compteur et n'imprime pas `dual_point_tests`; sa série `uniform` versionnée est
incomplète. Les secondes étaient contaminées et sont exclues. Voir
[`AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md`](AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md).

### Trous du code courant

1. La table de banque partage un tableau mutable `engaged[10]` entre plusieurs
   reçus. Un reçu tardif peut réécrire le masque d'un reçu antérieur; la voie
   radiale calcule son propre masque puis ne le sérialise pas. La table doit
   être immuable et chaque reçu porter un masque de onze bits dont exactement
   dix sont levés pour un réservoir arbitraire. Une banque certifiée des dix
   plus proches reste à dix : si la cible est dans ce top-10, dix témoins
   strictement plus proches sont impossibles et le onzième n'apporte aucun
   prune. Pour un reçu de **région**, un masque commun de dix pris dans un
   réservoir de onze n'existe que si la région contient au plus un identifiant
   de la banque; avec deux intersections il faut scinder, choisir une banque
   disjointe ou échouer ouvert.
2. Aucun `DualReceipt` ne lie epoch, ancre, plage cible, plages témoins,
   sous-ensembles ponctuels, masses et bornes `L/U`. Le juge compare les sorts
   bornés sans rejouer ce transcript.
3. `work_done()` omet tout le travail dual. `merge_receipts` et
   `receipts_equal` omettent ses neuf champs, dont `dual_point_tests` et les
   abandons de frontière. Le harness shardé exécute maintenant la voie duale,
   mais perd donc précisément sa télémétrie dominante à la fusion.
4. Le plafond de frontière est fail-open pour les sorties, pas reçu comme
   politique déterministe ni comme borne mémoire globale. Il borne un segment
   courant d'une arène append-only, son `nth_element` n'a pas de tie-break
   canonique, son option est convertie en `int` avant bornage et aucune porte
   permanente nommée ne l'exerce.
5. L'effacement annoncé « exact, sans perdre un prune » est réfuté. Une feuille
   partiellement créditée est retirée entière; ses points non crédités peuvent
   devenir témoins sur un enfant `Q'`. En une dimension, pour `p=0`,
   `Q=[10,20]` et une feuille `W={5,15}`, le point 5 est universel sur `Q`,
   tandis que 15 ne devient témoin que sur `Q'={20}`. Avec huit autres crédits
   hérités, retirer toute la feuille empêche donc un prune à dix. La sortie
   reste exacte car le classifieur rattrape la paire, mais un prune est perdu.
6. Le mode normal reste count-only. Le LBVH est un `std::sort` CPU suivi d'une
   construction récursive avec rescans AABB; aucune construction Karras device,
   aucune résidence CUDA et aucun census consommable ne sont reçus.

### Réduction recommandée

- remplacer le majorant continu arrondi par le maximum entier exact. Pour
  chaque axe et chaque extrémité entière `u` de l'intervalle cible, évaluer
  `u*v-v^2` aux bords témoins et aux deux entiers bornés voisins de `u/2`;
  prendre le maximum sur les deux extrémités `u`, puis sommer les trois axes.
  Le `ceil(u^2/4)` courant surestime d'une unité quand `u` est impair et peut
  donc manquer des rejets `U<=0` sans jamais créer de faux prune;
- conserver séparément, dans toute feuille partielle, les identifiants ou le
  masque des points acceptés, rejetés et ambigus; hériter seulement le résidu
  ambigu. Le majorant de masse future somme crédits, résidus et nœuds
  antichaîne sans double compte;
- représenter la frontière par une arène immuable à partage structurel. Elle
  conserve les domaines qui chevauchent `Q`; après le split, les points du
  sibling déjà représentés deviennent admissibles pour l'enfant. Les verdicts
  `L>0` et `U<=0` s'héritent, le résidu seul est raffiné;
- ne pas recopier dans l'arène le suffixe de travail lorsque la masse dix vient
  d'être atteinte : le nœud cible est immédiatement pruné et ce slice est mort.
  Restaurer aussi le checkpoint après une feuille cible et entre deux siblings
  réduit le stockage à la profondeur utile plutôt qu'au nombre de boîtes;
- publier un `DualReceipt` compact et rejouable, puis compter visites, tests
  ponctuels, copies, scans d'overlap, opérations d'arène, octets et high-water
  dans `work_done`, la fusion shardée et les reçus d'échelle;
- faire précéder le dual résiduel par une cascade exacte. Les banques Yao
  ferment les chambres faciles; sur chaque plage cible compatible, dix
  témoins ponctuels immuables peuvent appliquer directement la forme affine
  `h_w(q)=(q-p) dot (w-p)-||w-p||^2`, dont le minimum sur une boîte se choisit
  coordonnée par coordonnée. Dix minima strictement positifs dominent la coupe
  Yao pour cette même banque. Les dix identifiants doivent être distincts,
  différents de l'ancre et disjoints de toute la plage cible; le résiduel seul
  entre dans le dual-tree puis le classifieur;
- grouper un microtile de nœuds cibles dans un masque de bits contre chaque
  nœud témoin. Les décisions `L>0`, `U<=0` et ambiguës produisent trois masques;
  un état n'est dupliqué qu'à un vrai split, au lieu de recopier une frontier
  entière pour chaque `Q`;
- produire dans la même traversée le transcript Yao-1 exact : chaque slot de
  chambre termine en premier voisin canonique ou vide certifié, puis au plus
  `48n` arêtes sont dédupliquées et réduites par un Kruskal/Borůvka sparse;
- seulement après deux pentes complètes acceptables, porter cette machine vers
  le prior art device de la ligne enregistrée : ownership exact-once,
  epochs/leases, tuiles, `count--scan--fill` et offsets 64 bits, avec nouveaux
  prédicats u16 et sans reprendre ses décisions binary64.

## Lane `k=1` et diagnostic horizontal

Le commit `e6f1ef3` a introduit un read-off k1 depuis les supports q2 à zéro intérieur.
Il est exact pour le multiensemble des poids MST, mais son vocabulaire et sa
portée sont trop forts. Zéro intérieur signifie boule diamétrale ouverte vide;
une extra-shell peut subsister. Les lignes `K1 d2` publient `d2=4 beta`, pas le
niveau rationnel nommé. Le Kruskal mute son DSU séquentiellement puis jette les
endpoints; le RLE des poids ne publie ni racines pré-lot, ni composantes
quotient, ni multifusions. Le juge trie les scalaires et ne peut donc valider
leur ordre, les lots ou le catalogue Gabriel complet.

Cette lecture ne réduit surtout pas le chemin source. Sur `terrain,n=400`, elle
paie les mêmes `30 265` cellules, `1 768 790` lifts et `52 665` census que la
source normale, puis filtre `832` arêtes vers 399 poids. Les cinq verts k1 sont
des différentiels bornés de poids MST, pas une source sparse 50 000. Yao-1
reste un candidat architectural sparse indépendant. Les preuves, la fixture d'extra-shell et
la reconstruction correcte des multifusions sont dans
[`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md).

Le Borůvka CPU courant reste un oracle borné : son parcours dit best-first est
en réalité un DFS near-first et son API cœur publie les arêtes dans l'ordre des
rondes, pas dans l'ordre final `(distance_squared,min_PointId,max_PointId)`.
Une requête point--LBVH par sommet et par ronde garde une borne simple
`O(n^2 log n)`.

Son juge laisse en outre passer une classe d'incidences métriques :
`check_spanning` ne vérifie pas que le niveau publié égale la distance carrée
des deux endpoints. Sur les octets pincés,
`--points 5000 --family uniform --inject level-off-by-one` rend le code 0 et
affiche `MUTANT SURVIVANT`, car l'oracle borné n'est plus actif. Il faut rejouer
chaque distance en arithmétique indépendante et graver ce cas sans dépendre de
l'oracle Prim.

Le prior art enregistré fournit déjà le levier exact : le voisin canonique le
plus proche dans chacune des 48 chambres contient un EMST sur des positions 3D
deux à deux distinctes. La route candidate produit au plus `48n` arêtes,
déduplique, applique Kruskal/Borůvka sparse, trie les `n-1` arêtes et groupe
atomiquement les niveaux égaux. Chaque slot doit finir en
`exact_first_neighbor` ou `certified_empty`; budget, cap ou frontier abandonnée
signifient `incomplete`, jamais vide.

Le diagnostic horizontal active désormais la voie duale par défaut. Ses
shards ont des états privés et ferment les masses, mais la fusion omet les
neuf compteurs duals. Il ne compare ni sorts ni transcript entre 1/2/N threads,
et sa provenance reste incomplète. Son `p95` interpole cinq répétitions sans
warmup au lieu d'appliquer le nearest-rank contractuel, et
`--threads 4294967297` se replie sur `1` avant validation puis rend le code 0.
Sa ligne de portée annonce encore des
« tombstones Yao48/radial » alors que le défaut `--dual=1` contourne ces voies;
les smokes ne contrôlent pas cette description. Il demeure `DiagnosticHorizontalReceipt-v1`,
`backend=cpu`, `slo_eligible=false`.

## P1a q4 center-cover

Le probe `b312638` n'a montré aucune fausse coupe dans sa portée bornée. La
condition q4, les 64 patches fermés, les tests stricts, les exclusions et le
ledger sont sûrs sous leurs hypothèses. Le juge déterminantal est
arithmétiquement indépendant pour les sphères et la positivité propre, mais il
n'authentifie ni toute la bijection ni tous les champs structurels du reçu.

L'ordonnance est refusée. Sur `terrain` à 2/4/8 k, les visites témoins valent
`11 342 326 / 48 755 505 / 181 460 408`, soit des pentes `2,104/1,896`; les
tests point--patch donnent `1,968/1,786`. `uniform` ne termine que 2/4 k puis
expire à 8 k. Le collecteur repart de la racine pour chaque bloc et
`max_states` ne borne pas ce travail. Ce diagnostic n'ajoute pas une gate de
performance au protocole P1a; il interdit seulement de porter ce probe tel
quel. Voir
[`AUDIT_P1A_CENTER_COVER_B312638_20260811.md`](AUDIT_P1A_CENTER_COVER_B312638_20260811.md).

Le successeur doit reprendre du prior art `95dd803` le scheduler, la partition,
les antichaînes, ledgers et arènes, pas son arithmétique binary64. Avant les 64
patches, il applique le cœur universel de Jung aux blocs de cibles; le résiduel
seul entre dans une wavefront témoin persistante `(pair_block,W,patch_mask)`.
Il lui faut les bornes dirigées `L/U`, les masques accepté/rejeté/ambigu par
patch, une borne de masse encore atteignable et la recertification complète des
huit coins après chaque split. Après le
différentiel hôte `n=32`, la même session G4 gardée doit fermer parité native,
rejeu `n=32` et Compute Sanitizer, puis seulement les deux profils 50 k directs,
sans palier de performance ni retry.

## Prototype de localité, source sparse et porte mathématique

Le lemme full-sphere reste seulement partiel : une ancre extrême possède une
direction sortante qu'aucune calotte stricte ne couvre. Le mode `directional`
courant ne l'emploie plus comme condition globale. Il calcule des rayons par
cellule; lorsque la fenêtre top-M ne ferme pas une cellule, la voie `scan`
bascule sur l'univers et la voie `cone` interroge le LBVH. La condition de
débordement est maintenant dans le bon sens : `within_rho(d_M^2,r)` déclenche
la fermeture, égalité comprise. Le juge compare les identités des paires et le
signe q4 est corrigé avec une fixture centre/extérieur et un mutant dédié.

Le raccord local est réel mais borné. La suite CMake enregistre 34 portes
`locality`; aucune n'exerce `--closure=cone` et le filtre complet est rouge
`33/34` pour la raison donnée plus haut. Les nombres d'une ancienne campagne
manuelle cône ne disposent pas ici d'un reçu brut pincé : ils ne sont donc pas
repris comme preuve live. Structurellement, une requête AABB peut visiter tout
l'arbre par cellule et les classifications de boule conservent un pire cas
cubique. Une gate cône durable avec CLI, graine, empreintes, log brut et
compteurs complets reste requise avant tout verdict de coût.

Les modes `directional` et `arity` comptent des supports retenus, jamais le
payload. Ils n'émettent ni `BallKey`, ni census fermé `I_B/U_B`, ni owner
exact-once ni hiérarchie. Le nouveau juge rationnel évite le partage des
primitives de décision, mais ne ferme encore que des cardinalités. Les
fractions publiées à `n=1 500` mesurent des records émis avec au moins une
extra-shell, pas des supports minimaux multiples, des boules ou des cofaces;
elles proviennent d'une graine et d'une fenêtre q3/q4 empirique. Elles ne
permettent aucune extrapolation au régime 50 k.

Le nouveau mode `sparse` dimensionne les bras et les branches `J_F`, mais ne
réalise pas la route : sa fenêtre fixe de 48 est aussi utilisée par q2, il
recalcule un kNN complet par ancre, balaie le nuage pour `J_F` et ne construit
ni gateway, ni resolver, ni MSF, ni fold. Sur la commande diagnostique
`n=600, terrain, K=10, support-window=48`, il imprime 38 641 supports qualifiés
« cofaces », 108 226 bras, 54 900 facettes uniques et les proportions
`62,04/27,97/9,99 %` pour `|J_F|=0/1/>=2`; ce sont des préfixes de
dimensionnement, pas des populations complètes ni un benchmark.

Mathématiquement, `(S,B)` ne suffit pas dès que le shell global `U_B` diffère du
support minimal `S`: les cofaces directes portées par la boule sont
`I_B union A` pour les sous-ensembles admissibles `A` de `U_B`. La route
régulière exige donc census `I_B/U_B`, `BallKey`, owner et porte `U_B=S`. Les
supports multiples ne se réparent pas par un pivot dans leur union. Au-dessus
de la fenêtre, le théorème 4.2 autorise une tombstone H0; dans la fenêtre,
quotient de plateau reçu ou refus fermé. La route conditionnelle complète est
`directes + facettes du cœur + premières incidences + gateways + resolver +
MSF/fold atomique`; elle est détaillée dans
[`AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md`](AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md)
et
[`NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md`](NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md).

Le front inverse concurrent ne répare pas encore cette fermeture. Son en-tête
reconnaît que `|I_B|<=K` n'est pas un rang fermé. Son objet principal et son juge
exhaustif restent les sphères ayant quatre labels non coplanaires;
`record` ne vérifie pas l'auto-centrage. La récolte optionnelle q2/q3 décrite
ci-dessous ne transforme donc pas le front en Source S certifiée.

La transition par défaut vise désormais le premier croisement strict dans les
deux sens, collecte les ex æquo et transporte l'intérieur par lots. C'est la
réparation mathématique attendue du front q4, mais le juge final ne contrôle pas
chaque `(sommet,flat,sens,lot)` et partage encore conventions, générateur et
prédicats entiers avec le sujet. Le compteur `transitions` compte des directions
tentées, pas seulement des arêtes suivies. Le germe reste certainement
incomplet : `uniform n=4 seed=1 coord=15`, affine-3 et non coplanaire, est
refusé parce que la direction opposée n'est pas rejouée.

Le comparateur de croisement du snapshot pincé est algébriquement correct et sa
formulation par signes de puissance évite le produit croisé large du quotient.
La recherche reste toutefois une pile DFS dont la distance au barycentre
n'ordonne les enfants qu'heuristiquement; elle garde un pire cas linéaire par
requête, puis paie un second parcours `collect_shell`. Un comparateur antérieur
inversé passait encore les accords globaux : il faut un oracle **local** du
premier successeur et un mutant d'ordre. `tie_mass` compte le shell fermé, pas le
lot entrant; les tests de feuilles du second parcours et plusieurs octets de
scratch manquent au ledger. Le préflight accepte aussi `coord>65536`, hors de la
preuve i128 u16.

L'option concurrente `--harvest` énumère désormais des sous-ensembles q2/q3/q4
des shells et recense leurs miniboules, mais aucune porte CMake ni aucun juge de
supports ne la reçoit. `--harvest --judge` compare encore seulement les sommets
q4. Le CLI permet en outre `cap<smax-2`; à `uniform n=20 seed=1 smax=11`,
`cap=3` publie 554 supports contre 807 à `cap=9`, tous deux avec code nul. Le
mode perd aussi les supports pertinents par `p+q<=smax` dès que leur rang fermé
`p+|U_B|` dépasse `smax`, conserve seulement `ids -> |I_B|`, et ne groupe ni
`BallKey`, ni `I_B/U_B`, ni tous les supports, ni owner ou plateau. Il reste
donc une récolte prototype, pas la Source S.

La baisse du rayon peut faire passer le niveau de 1 à 3, et une requête LBVH de
sortie intérieure vide peut visiter `Theta(n)` nœuds. Le raccourci de plateau
« une facette par coface » perd déjà le carré cosphérique à quatre points. Les
fixtures, preuves et alternatives exactes sont dans
[`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md).

Un témoin q4 historique compilait et, sur `uniform n=20 K=3 seed=1`, retrouvait
les 799 shells/intérieurs de son oracle. Sa suite à neuf tests était rouge
`8/9`; la configuration courante en recense dix et n'hérite pas de ce verdict.
Les deux fixtures de projection passaient manuellement, mais n'étaient pas
raccordées. Une campagne de l'ancien pivot avait trouvé
2 672 accords et 1 328 refus de germe; elle est historique et ne reçoit pas la
nouvelle transition. Aucun de ces résultats ne reçoit `BallActivation`.

Les rampes des snapshots pivot antérieurs sont historiques depuis le raccord
du nouveau pinceau. La sonde `order_k_flats`, elle, confirme que l'ordonnance
arrangement reste rouge : à `smax=11`, les sommets et points touchés croissent
fortement dès les petites tailles, et `n=200` compte 207 216 sommets pour
214 847 238 touches. Cela ne prouve ni une loi asymptotique ni un minorant pour
toute source exacte; cela interdit un port littéral sans nouvelle rampe du
successeur et réduction structurelle.

Le refus est désormais mathématique, pas seulement empirique. La famille u16
`A_i=(1+i,0,0)`, `B_j=(0,1+j,1)` possède, à `n=50 000`,
`34 364 000 715` sommets relevés à shell quatre jusqu'au niveau neuf, tous
transits non positifs, mais seulement `499 945` supports q2--q4 de Source S.
Même les plafonds huit et sept laissent respectivement `28 116 750 495` et
`22 494 000 330` transits. L'arrangement n'est donc ni la sortie ni un minorant
que toute source exacte doit payer. La preuve complète, le census deux passes et
le blueprint device sont dans
[`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md).

Le théorème global de listes de cellules de centres imbriquées est exact lorsque
les domaines actifs enfant--parent sont emboîtés. Le resserrement live par
`tight`, suivi d'enfants dyadiques qui peuvent en déborder, ne conserve pas
l'identité globale de `R_p/A_p`. Il conserve néanmoins la complétude sous
l'invariant pool-relative : tout pool hérité qui contient `I_B union U_B`
continue de les contenir après le filtre, parce que la statistique relative ne
peut placer `p+1` témoins strictement dans une boule de profondeur `p`. Les
reçus doivent distinguer domaine actif, cellule owner et digest du pool.

La première génération proposée n'était pas complète : q3 depuis les q2
pertinents et q4 depuis les q3 pertinents perdent deux contre-fixtures u16
explicites. Les lanes doivent rester indépendantes. La stratification corrigée
par budgets d'intérieurs, `e0` immuable et promotion est dans
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
et son audit complet dans
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).

Le successeur historique `fd734092...` a réparé le CLI, les lanes inter-arités,
le shell dynamique et le groupement avant census; le snapshot actuel conserve
ces réparations et ajoute des portes. L'ordonnance reste combinatoire. Sur
`uniform,seed=3,smax=11`, `n=100/200/400`, les créations de cellules, lectures
parentes, IDs candidats et census ont deux pentes successives supérieures à
1,35. À `n=400`, 7 240 129 lifts produisent 103 978 supports et 85,7 % des
lifts meurent à l'owner. L'extrapolation strictement linéaire — diagnostic, pas
preuve asymptotique — donne environ 905 millions de lifts à 50 k. Cette
ordonnance reste `NO-GO` avant G4; le source live postérieur doit être repincé
avant tout nouveau verdict.

Le ledger ajouté au commit `238cf12` confirme sur
`terrain,n=1 500,work_cap=20 000` que `7 236 483` des `7 820 379` lifts, soit
`92,53 %`, meurent à l'owner. Il justifie une décision `SupportKey` avant lift,
mais sa partition de rang ne ferme pas : après dégénérescence, owner et
positivité, `130 033` occurrences pending ne sont imputées ni aux acceptations
ni à `rank_rejected_q`. La branche `interior>budget` retourne au niveau du
groupe et n'incrémente que le compteur global. Ainsi « rang nul » et les
multiplicités `42/55/510` ne sont pas reçus. Le prochain ledger doit compter
`early_rank_rejected_supports_q`, fermer l'identité par arité et publier les
runs uniques `SupportKey`. Voir
[`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md).

Le premier point du transcript de taille en cours, sur l'ancien couple pincé
`4884b293.../5b422644...`, termine `terrain,n=12 500,work_cap=20 000` avec
`rc=0`, `14 262 497` cellules, `92 531 928` lifts, `85 797 521` rejets owner et
`906 078` supports, soit `102,124` lifts par support et `92,7221 %` de rejets
owner. Son `wall_s=797` sous charge n'est pas qualifiable. Le
ledger ancien ne ferme toujours pas ses rangs par arité : le global annonce
`1 134 183` rejets de rang, les lignes q2/q3/q4 seulement `0/7/17`. Ce point
n'est ni la variante de l'ancienne note (`8 338 753` cellules et
`104 352 433` lifts), ni une famille SLO volumique. Le 25 000 a depuis terminé,
mais la campagne a changé d'ELF avant le 50 000 : aucune pente mono-binaire
n'est donc publiable.

Le transcript gelé `dbaa2e0.../423797e...` est maintenant clos sur neuf cas et
un footer. Les ledgers par arité rendent tous `ecart=0`; l'ELF est identique
avant/après chaque cas. `uniform,n=50 000` ferme notamment `7 773 329`
cellules, `839 582 666` lifts et `21 395 212` supports. C'est un diagnostic
count-only reproductible de ces compteurs; sans `--judge`, digest d'identités,
mémoire, `eight_clusters` ou pipeline, il ne reçoit pas l'exactitude générale
ni le SLO. `probe_tests=0` confirme aussi que les comptes `E2/T3/T4/Q4` ne sont
pas exercés au défaut. Tous les `wall_s` sont contaminés et non qualifiables.

Le successeur `005b786` ferme ensuite la partition par arité sur l'observation
`n=1 500` : `130 033` occurrences de rang anticipé et `3` finales. Son
histogramme `n=400`, encore sans transcript/pins autonomes, contient `263 825`
clés non dégénérées pour `2 215 217` occurrences et omet `4 807`
occurrences dégénérées. Un RLE seul implique donc `263 826..268 632`
géométries, facteur `8,26..8,41`, non `52 693` et facteur 42. Les classes sont
des stades maximaux par clé; elles ne séparent pas encore rang et pertinence.

Le commit `64cf6fe` mesure ensuite des clés par sous-arbre, sans transcript ni
borne d'octets. Une RLE `SupportKey` locale est exacte grâce à l'unique feuille
owner, mais paie un solve par clé et par lot; le titre du commit ne reçoit donc
pas l'abandon d'une agrégation globale streamée. Le worktree postérieur tente
un vrai lot différé. L'audit statique relève déjà un cap en records qui ne borne
ni listes ni scratch, `owner_multiple` non fail-closed, l'instrument
`--multiplicity` déconnecté en combinaison avec `--deferred-lift`, et le mutant
`arity-cascade` affaibli parce que `record_tuple()` retourne toujours vrai dans
cette voie. Le successeur la place derrière `--deferred-lift`. Les quatre
accords différés passent sur le couple historique pincé, y compris le petit cap,
mais ils ne couvrent pas ces invariants et ne qualifient aucun débit. Le cas
`terrain,n=100` ferme `4 693/4 693` supports aux trois caps testés, tandis que
le gain tombe de `8,718` à `1,410` lorsque le cap passe de `1 048 576` à
`1 024`; il faut donc publier la courbe cap--réplication--octets, pas seulement
un facteur favorable. La combinaison différé--multiplicité imprime des tables
vides et des facteurs `NaN` avec code zéro. Les commentaires « lifts divisés
par cinq » et « treize pour cent plus lent » n'ont toujours ni reçu autonome ni
portée performance.

Le contre-audit corrige ici l'autre auditeur : sous arbre/epoch communs et lots
spatiaux de feuilles atomiques, tous les supports d'une même boule ont le même
centre et donc la même feuille half-open owner. Cette feuille est un
`BallOwner` exact; le second RLE/census par boule peut rester local avec un
contexte `b_cert>=H_run`. Cela ne vaut pas automatiquement pour des shards
hashés par `SupportKey` : des supports distincts d'une boule peuvent tomber
dans des shards distincts et doivent être redistribués par
`GeometricBallKey/OwnerCellId` avant le census.

Le snapshot `fd043fe...` raccordait une sonde du vrai graphe bissecteur dans
une bande autour du cap. La réutilisation de l'adjacence au terminal est réelle,
mais `topp` vaut toujours la taille du pool q2, `E+9T` omet les K4 et la matrice
dense n'a aucun préflight d'octets. Dans `K_24`, le critère accepte
`E+9T=18 492` sous un cap `20 000`, alors que
`E+3T+6Q=70 104`. Ce critère reste donc une heuristique de split, pas une borne
de travail ou de mémoire. L'alternative exacte proposée est la borne duale de
Kruskal--Katona `Q4<=Q_KK(T4)`, puis la gate
`E2+3T3+6Q_KK(T4)<=work_cap`, calculée en u128 saturé; une CSR forward sparse
remplace la matrice dense `Theta(m^2/64)` mots. Aucun test n'est transféré à ce
successeur.

Le snapshot `dbaa2e0...` corrige les deux défauts combinatoires de ce snapshot :
quand les seuils existent, cuts `D_(smax-2)/D_(smax-3)/D_(smax-4)`
(`D_9/D_8/D_7` à `smax=11`), compte exact `E2/T3/T4/Q4`, admission
`E2+3T3+6Q4`, et sonde bornée à `top<=96`. Le `28/28` reçoit le chemin par
défaut, mais pas cette sonde. En effet,
`probe_factor=1` rend sa branche de sonde inatteignable par défaut, puisque
`!terminal` implique déjà `work>work_cap`; aucune porte CTest ne passe un
facteur supérieur. Le contrôle d'incidence autorise à tort `Q=bound+1` au lieu
de tester directement `4Q<=(m4-3)T4`. Hors sonde, `generate()` conserve son
bitset dense sans préflight. Sous `have_thresholds` et sans overflow, le partage
de `R_top` empêche une liste arbitrairement longue d'avoir un potentiel quasi
nul; les brèches restantes sont une terminalisation forcée par `max_depth`, par
un `leaf` CLI relevé, ou un overflow hors profil. Le cap ne borne toujours ni
contextes, ni enfants, ni scratch.
Les potentiels i64 ne sont pas saturés sur tout le domaine CLI, et l'en-tête
n'imprime ni `probe_factor`, ni `probe_top_cap`, ni `batch_records`.
Si `have_thresholds` est faux, le code prend les trois cuts égaux au pool entier
: ce sont alors des supersets fail-open, pas littéralement les trois `D_h`.

Le `HEAD=3ffff85`, source `d2039ba...`, ajoute un rejet des enfants dont la fermeture est
strictement disjointe du `tight` parent, puis réemploie des scratch vectors.
Le prune est mathématiquement sûr : pour un support positif, `c_B` appartient
à `relint conv(U)`, donc à `bbox(mine_parent) intersection cell=tight`; les
comparaisons à l'échelle enfant sont exactement `child` contre `2*tight`, et
les inégalités strictes conservent la tangence. Le réemploi est sémantiquement
neutre dans cet Engine mono-thread. Il corrige aussi le contrôle d'incidence par
produits i128 exacts, ajoute `min_probes` et un mutant multipliant `Q4` par
quatre. La suite ciblée 30/30 observée sur l'ELF correspondant qualifie ces
chemins dans la portée de ses fixtures; sa sortie brute reste en `/tmp` et son
`LastTest.log` a été écrasé après le pin, donc elle n'est pas un reçu durable.
`min_probes>0` ne garantit pas à lui seul la mort du mutant si
`Q4=0` ou si l'inégalité a du slack : la porte exige une clique complète
saturant `4Q4=(m4-3)T4`, par exemple `K_24`, ou un plancher q4 dédié. Il
exige fixtures centre sur plan de split et face haute racine, mutants `<` vers
`<=` et `>` vers `>=`, prune strict non vide, et compteur des lectures
candidates évitées.

Un smoke `terrain,n=200,probe_factor=64` exerce `1 706` sondes; le mutant rend
le code 3, mais continue après la première faute, imprime plus de mille lignes
et emploie le `Q4` muté dans l'admission, modifiant le parcours (`1 785`
sondes). Pour une porte causale, conserver `Q4_real` dans la décision, tester
une copie mutée, échouer vite ou n'imprimer qu'une fois, et séparer le flag
d'incidence du message final actuellement libellé « lemme de profondeur ».
Le mutant s'appelle `incidence-off-by-one` mais réalise `Q4*=4`; il vérifie la
sensibilité de la garde, pas l'exactitude de l'orientation de `real_counts`.
Contrairement au commentaire du commit, un vrai mutant `Q4+1` est tué sur tout
graphe complet `K_m`, où `4Q4=(m-3)T4`; c'est la fixture terrain qui ne garantit
pas cette égalité. Une fixture synthétique `K4/K5/K24`, plus un graphe non
complet, doit comparer `E2/T3/T4/Q4` à des constantes indépendantes.

Le source worktree `fbf34da...`, postérieur aux 30 tests, ajoute
`--ablate=0..5` et une copie contiguë `cell_pts` par cellule. Il n'est ni
construit ni testé par CMake. La copie paraît sémantiquement neutre dans cet
Engine mono-thread, mais ajoute un coût `O(top)` sans préflight et ne rend pas à
elle seule `build_adjacency` contigu; « quelques dizaines » n'est garanti que
sous le plafond de sonde, pas pour tous les terminaux. Son gain doit être
mesuré. Le successeur refuse désormais juge, mutants, lift différé et
planchers. Deux défauts interdisent encore d'en tirer des coûts causaux : avec
`probe_factor>1`, `real_counts` exécute
encore adjacence et cliques avant les retours `ablate>=3/4/5`; enfin une sortie
fausse peut conserver le schéma `CentreCellReceipt-v3` et rendre le code zéro.
L'ablation doit avoir un contrat diagnostic distinct, refuser toute porte,
neutraliser ou nommer la sonde, et ne jamais être consommable comme sortie
exacte. Les différences de préfixes restent des coûts marginaux sous états de
cache différents, pas une attribution causale absolue.
Le CMake worktree `38d4b14...` définit deux fois chacun de cinq mêmes noms de
tests : rejets juge, mutant, lift différé et plancher, plus
`ablation_annoncee`. Une configuration Release temporaire échoue avec cinq
erreurs `add_test ... already exists`; le premier bloc, placé avant la cible,
doit disparaître et le second rester unique. Une éventuelle exécution de
l'ancien ELF `fc2eb10...` ne qualifierait jamais cette ablation.

La campagne de taille ne peut plus devenir un reçu mono-binaire : le 25 000 a
terminé sur l'ancien inode supprimé `5b422644...`, puis le 50 000 a démarré sur
l'ELF distinct `8fdfc8af...` sous le même en-tête. Le processus a disparu sans
sortie ni code pour ce dernier cas et sans marqueur de fin. Le mélange et la
troncature sont donc des faits observés. Le bloc 25 000 annonce `46 745 417` cellules,
`220 298 378` lifts et `1 872 528` supports, soit `117,648` lifts/support et
`92,874 %` de rejets owner; son `wall_s=2 191` sous charge n'est pas une mesure
SLO. Voir le contre-audit du ledger pour la reprise par ELF immuable et
enveloppe par cas.

Les commentaires et sorties du code courant ne sont pas reçus lorsqu'ils
annoncent `EXACT`, « cofaces directes », `O(sum M*)`, causalité du facteur 384
ou travail proportionnel à la sortie. Le parseur du probe de localité accepte
encore `--points=5junk` comme `n=5` et rend le code 0; le juge rationnel accepte
de même `--points=50junk`. Ces surclaims et parseurs doivent être corrigés par
Claude; l'audit ne modifie pas ses sources.

Les ancres q3/q4, Jung, Helly, localité et profondeur restent des certificats
locaux ou falsificateurs bornés. Aucun ne constitue encore une source 50 k de
`BallActivation`. Aucun atlas global ou persistant de paires, tuples, cellules
d'arrangement, faces, cofaces ou incidences ne doit entrer dans le chemin
produit. Une CSR transitoire de cellules de centres reste autorisée seulement
si son coût complet est compté et passe la gate.

Le domaine produit exact n'est pas encore fermé. Les preuves Yao-1 supposent
des positions distinctes; la spécification exige en outre un `RelevantGP`
certifié, un quotient exact des plateaux pertinents ou un refus explicite de la
dégénérescence. `smax=11` ne borne pas une coquille fermée arbitraire : il borne
seulement une activation admise de rang fermé au plus onze sous ce contrat. Une
coquille plus grande doit être diagnostiquée complètement puis traitée par un
générateur saturé reçu ou refusée, jamais tronquée. Enfin, le cas terminal
`k=n` appartient au contrat mais n'est pas encore produit par le candidat v3;
le fold ne peut donc pas être déclaré complet.

## Ordre de travail

1. Installer immédiatement le squelette de `BenchmarkOutputContract-v1`, son
   payload et l'interface verticale avec producteurs explicitement
   `incomplete`; taguer chaque chantier `slo_critical_path=yes/no`. Réparer en
   parallèle la porte locality rouge pour qu'elle atteigne réellement les
   valeurs gravées, refuser les suffixes CLI, corriger les complexités et
   comparer des identités `(BallKey,support,I_B,U_B)` avec l'oracle rationnel.
2. Graver une porte locale du front inverse : pour chaque
   `(cellule,flat ferme,sens)`, comparer premier croisement, lot et intérieur à
   un oracle rationnel; tuer le mutant d'ordre et couvrir `lambda=0`, ex æquo,
   cap `K+1`, fallback pivot et germe opposé. Garder le front comme témoin q4;
   ses accords globaux et son `--harvest` sans juge ne reçoivent pas Source S.
3. Produire le transcript Yao-1 exact et le Kruskal sparse; cette preuve retire
   q2 profond du chemin critique `k=1` sans énumérer Gabriel.
4. Pour q2 supérieur, garder la cascade Yao--affine--dual comme comparateur
   suspendu. Ne rouvrir sa construction que si une comparaison avec la lane
   cellules `D_9` le justifie; elle exige alors masque régional,
   `frontier-clear`, états immuables, `DualReceipt`, maximum entier, fusion,
   télémétrie et rampe `12 500/25 000/50 000`.
5. Garder P1a actuel comme falsificateur; construire le front de Jung coalescé,
   puis la lentille fermée avec bit aigu et une cutting signée half-open. Le
   top-`(smax-2)` ne sert qu'à tuer un patch : sur tout patch vivant, son rejet
   est redondant avec `U_z<0`. Générer les centres q4 par niveaux pondérés
   `P-P/N-N/P-N`, grouper les concurrences, puis traiter leur masse `J/H` par
   dominance ou terminal borné. Recevoir l'arête maximale canonique avec
   `occurrences=SupportKey_unique`; reconstruire `(I_B,U_B)` depuis les
   identités `always_inside` authentifiées, le support et les conflits au
   centre, puis comparer l'enregistrement complet à l'oracle rationnel.
6. Garder les lanes cellulaires `D_9/D_8/D_7` comme comparateur exact, avec
   partition terminale commune, `e0` immuable et promotion. Après génération,
   faire un premier RLE `SupportKey` avant lift et choisir le contexte owner.
   Comparer deux ordonnances : BallKey-first choisit le support d'arité minimale
   et top-`(12-q_min)` dans `X minus U_star`; SupportKey-first emploie d'abord
   le census producteur puis top-`(12-q)` hors `U` en fallback. Seuls
   `delta>beta` et `E=U` publient directement; toute extra-shell, toute égalité
   et toute demande Gamma rejoint la side queue `GeometricBallKey`. Le census
   pool reste l'autre backend exact et `U_B` un certificat aval. Pour Gamma, garder
   la provenance requise; pour le H0 normalisé, employer un support canonique
   et le token de fermeture Johnson au lieu d'énumérer tous les supports d'une
   cosphère. Graver les deux contre-fixtures inter-arités, l'invariant
   pool-relative, les arbres de budgets indépendants et le shell 30. Deux
   pentes rouges de candidats, listes, census, postings ou octets ferment cette
   route avant CUDA. En parallèle, le front de Jung coalescé n'entre en
   extension que si `W_front` ferme ses visites; le dual-tree actuel, de pente
   proche de `2,3`, est une baseline réfutée.
7. Fermer le terminal `k=n`, `BallActivation`, premières incidences, gateways,
   resolver, MSF/fold, dix forêts, verticales et payload hôte. Seul ce pipeline
   complet peut mesurer le SLO.

GCP utilisé uniquement pour la fermeture demandée :
`devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`, génération
`2026-08-12T11:50:53.892-07:00`, a été arrêtée par le script gardé
`stop_and_verify.sh --yes` et certifiée `TERMINATED`. Aucun benchmark GPU n'a
été lancé par ce contre-audit.
