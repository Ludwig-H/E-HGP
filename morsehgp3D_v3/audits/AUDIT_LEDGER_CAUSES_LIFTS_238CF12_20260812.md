# Contre-audit du ledger des causes de lifts `238cf12`

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le ledger établit un fait opérationnel utile : **l'owner est testé beaucoup
trop tard**. Sur l'observation publiée, `7 236 483` des `7 820 379` lifts, soit
`92,53 %`, sont rejetés par l'owner après construction de la géométrie. Cela
justifie de prioriser le groupement `SupportKey` avant lift ou une autre
sélection d'owner précoce.

En revanche, le ledger ne ferme pas sa partition. Il ne démontre ni « le rang
n'explique rien », ni les multiplicités `42/55/510` cellules par support. Les
conclusions causales doivent être resserrées avant de choisir l'architecture.

## 1. Provenance

Le commit audité est
`238cf1299dbbe339ed9f863f87a854584dceddf3`, intitulé
`weigh the lifts by cause and find that ownership, not positivity, dominates`.
La note est
[`NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md`](NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md),
SHA-256 `fde9419b1c174c63a9925d6c1dcaa34a66072aeb6092fd64f37a4417298399f7`.

Les octets annoncés et observés ensemble sont :

| objet | SHA-256 |
| --- | --- |
| `prototype/centre_cell_source.cpp` | `4884b29388d9617917810a03cde221430b66bc43cc320e9f06ba56be6e540793` |
| `CMakeLists.txt` | `d0738d1e3bfc103ecebc0c8e6dae8149aae3727322c34af4c3a0dcd8c12d440e` |
| ELF Release `mhgp3v_centre_cell` | `5b422644b6b461b919202f6c0257e27dc0af811110ad49fd82eca18a224f2283` |

Le commit postérieur `abcd488695c85409667d976234c3558ed8ac4d7c`, intitulé
`pin the contractual ramp with its full provenance`, a versionné
`receipts/centre_cell_scale_20260812/scale_counters_raw.txt` au SHA-256
`b9501c0a43da1e6435aa9ce68060e0b731f545f2d62597d2df555dd3cec09b86`.
Ce fichier ne contient que treize lignes de préambule et la commande 12 500;
il ne contient ni stdout, ni code de sortie, ni marque terminale, ni durée. Le
processus correspondant tournait encore après le commit. Il s'agit donc d'un
**manifeste de lancement incomplet**, pas d'une rampe pincée ni d'un reçu brut.
Il ne contient pas davantage le transcript du tableau `n=1 500` audité ici.
Les nombres de la note sont cohérents avec le format du binaire, mais ne sont
pas encore un reçu autonome reconstructible.

Aucun CTest n'a été relancé par cet audit : une exécution 12 500 points de
Claude occupait encore la machine partagée. Le registre CMake contient bien
vingt-quatre tests `centre_cell`; leur présence ne constitue pas leur résultat.

## 2. La partition par arité ne ferme pas

Dans `propose`, un lift suit exactement les décisions
`degenerate -> owner -> positive -> pending`. Ensuite `census_group` peut
rejeter **tout le groupe** dès que `interior>budget`; cette branche incrémente
seulement le compteur global `rank_rejected` puis retourne. Elle n'incrémente
ni `rank_rejected_q[q]`, ni un compteur du nombre de supports du groupe ainsi
abandonnés.

Le tableau publié laisse donc les écarts suivants :

| arité | lifts | dégénérés + owner + positivité | pending implicites | acceptés | rang final attribué | pending sans attribution |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| q2 | `1 206 409` | `1 159 553` | `46 856` | `28 808` | `0` | **`18 048`** |
| q3 | `3 479 927` | `3 318 298` | `161 629` | `63 804` | `0` | **`97 825`** |
| q4 | `3 134 043` | `3 113 740` | `20 303` | `6 140` | `3` | **`14 160`** |
| total | `7 820 379` | `7 591 591` | `228 788` | `98 752` | `3` | **`130 033`** |

Ici « pending sans attribution » désigne les occurrences de supports présentes
dans des groupes arrêtés par le rejet anticipé. L'identité reçue doit devenir,
pour chaque q :

`lifts_q = degenerate_q + owner_rejected_q + positive_rejected_q + accepted_q + final_rank_rejected_q + early_rank_rejected_supports_q`.

Un compteur séparé `early_rank_rejected_groups` est aussi nécessaire, car un
groupe et ses supports n'ont pas le même cardinal. Les `hull_pruned_q` sont des
prunes **avant** lift; ils restent hors de cette partition et ne doivent pas
être additionnés à ses issues.

Conclusion : `rank_rejected_q2=0` ne signifie pas qu'aucune paire n'est
rejetée au rang. Il signifie seulement que la branche finale par support n'en a
rejeté aucune; `18 048` occurrences q2 owner et positives appartiennent à des
groupes arrêtés plus tôt.

## 3. Ce que les pourcentages prouvent malgré tout

Les taux owner divisés par les lifts sont arithmétiquement justes :

| arité | `owner_rejected/lifts` |
| --- | ---: |
| q2 | `96,11 %` |
| q3 | `91,65 %` |
| q4 | `92,13 %` |

Ils prouvent qu'une grande majorité des occurrences paie la géométrie avant de
constater que son centre appartient à une autre cellule. Cette conclusion ne
dépend pas de la comptabilité de rang manquante. En revanche,
`positive_rejected/lifts` est une classification de sortie, pas une attribution
de coût : le code calcule les barycentriques et la positivité q3/q4 **avant**
le test owner, puis ne comptabilise `positive_rejected` que chez les survivants
owner. Le coût de positivité est donc payé aussi par presque tous les rejets
owner. Le ledger doit séparer `predicate_evaluated` de `terminal_issue`.

Ils ne prouvent cependant pas la multiplicité moyenne d'un même
`SupportKey`. Les quotients `lifts/accepted` mélangent :

- plusieurs occurrences intercellules d'un même tuple;
- des tuples non positifs;
- des tuples owner mais trop profonds;
- des supports pertinents acceptés.

En particulier, `28 808` est le nombre de q2 acceptés pertinents, pas le nombre
de tous les `SupportKey` q2 proposés. Diviser `1 206 409` par `28 808` ne donne
donc pas qu'une paire arbitraire est vue dans quarante-deux cellules. Les ratios
`55` et `510` ont la même limitation.

Le compteur décisif demandé par la note elle-même reste à produire : après un
radix des occurrences compactes, publier
`support_occurrences`, `unique_support_keys`, puis la distribution
`occurrences_per_support` en p50/p95/max, séparée par issue
`nonpositive/no_owner/rank/relevant`. L'identité
`sum multiplicity = support_occurrences` doit fermer exactement.

## 4. Conséquence d'architecture

La proposition `SupportKey-before-lift` reste exacte et devient même mieux
motivée, sous quatre conditions :

1. le premier groupement conserve **toutes** les occurrences
   `(CellId,e0,CensusContext)` d'un tuple jusqu'au calcul unique de son centre;
2. l'occurrence owner est recherchée dans le run entier, jamais choisie comme
   premier record; plusieurs owners sont une erreur, tandis que zéro owner
   rejette un tuple arbitraire et ne devient une contradiction que si l'oracle
   prouve ce support pertinent;
3. son arène reste vivante et son contexte vérifie `b_cert>=H_run`, où
   `H_run=smax-q_min`; sinon le census est global;
4. le count/scan/radix et ses octets sont préflightés et inclus dans la gate :
   déplacer un flot combinatoire avant le lift ne le rend pas sparse.

Avant cette transformation globale, q2 offre un oracle d'ablation moins cher :
son centre doublé est `x+y`, donc l'owner peut être calculé sans lift de sphère.
Comparer cette lane à la route Yao q2 séparée donnera un signal propre sans
suspendre Yao-1 pour `k=1`.

Les pistes `i64`, carrier partagé et clé primitive réduisent le coût par
occurrence; elles ne réduisent pas la multiplicité. Le potentiel d'intervalles
et les vrais `E/T/Q` peuvent modifier la partition spatiale et donc cette
multiplicité, mais seul le futur histogramme par `SupportKey` permettra de
l'attribuer.

Le « test de rayon avant lift » de la note est une condition nécessaire mais
pas encore un prune amont : q3/q4 doivent calculer une géométrie équivalente au
lift pour connaître `beta`. Il ne devient utile qu'avec un filtre de rayon exact
strictement moins cher et compté. De même, l'owner courant est déjà testé
immédiatement après le centre complet; un pré-test employant ce centre n'évite
rien. Les vraies spécialisations amont sont le milieu entier q2, l'acuité q3,
le paramètre face--apex q4 dans l'intervalle de cellule, ou le RLE
`SupportKey` avant centre.

Enfin, `lifts_q` compte les appels à `propose`, pas toutes les primitives
géométriques quand `--axis-filter` est actif : son `TriangleLift` est
additionnel. Le point axe désactivé n'a pas ce biais; toute ablation `off/on`
doit publier séparément les constructions physiques
`pair/triangle/tetra/axis`.

Un filtre de rayon réellement amont peut employer le diamètre. Pour un support
positif de dimension affine `r=q-1`, le centre est dans `conv(U)`, sa
circumboule est donc la boule englobante minimale et Jung donne
`D2/4<=beta<=r*D2/(2*(r+1))`, où `D2` est le diamètre carré. Si `L=max l_C` et
`U=min u_C` sont dans la même échelle dyadique `S2`, rejeter lorsque
`D2*S2>4*U`; pour q3 rejeter aussi si `D2*S2<3*L`, et pour q4 si
`3*D2*S2<8*L`. Pour q2, `beta=D2/4` donne la comparaison exacte à l'intervalle
`[L,U]`. Ces produits entiers sont fail-open aux égalités; leur rentabilité
reste une ablation. Calculer `beta` exact q3/q4 sans centre reste un solve
déterminantal, pas un filtre manifestement moins cher.

## 5. Verdict pour Claude

- **Admis :** owner tardif est le premier coût observé à attaquer.
- **Non admis :** « rang nul » et multiplicités `42/55/510`.
- **Prochaine porte :** ledger fermé par arité et histogramme exact des runs
  `SupportKey`, avant toute conclusion sur le facteur cent quinze.
- **Route candidate :** `SupportKey-before-lift`, avec contextes owner et
  budget certifié; q2 midpoint-before-lift comme ablation immédiate.
- **G4 :** toujours non prêt, sans verdict de latence CUDA.

## 6. Successeur live non qualifié

Après ce pin, Claude a ajouté `early_rank_supports_q` et
`early_rank_groups`, ce qui répare en principe l'unité manquante. Le source
live observé ensuite, SHA-256 `6f46fcfacc54317bde67bb70144120d79af3a2788bdec706aea128ef8370ed69`,
n'est toutefois ni construit ni testé par cet audit et son impression contient
un défaut mécanique : une boucle `for (q=2..4)` enveloppe une seconde boucle
identique. Les trois lignes par arité et `early_rank_groups` seraient donc
imprimés trois fois. Le bloc mort `if (false)` qui suit est parasite.

Ce défaut ne réfute pas les nouveaux compteurs, mais interdit de qualifier ce
transcript intermédiaire. Le successeur observé ensuite, SHA-256
`c76eaf4af307894e355371f2d2da236861fd1121b2f5584564c36c1cdcaefbb4`,
retire la double boucle et ajoute un squelette d'histogramme. Il reste non
construit et non testé; l'ELF du reçu 12 500 en cours est l'ancien
`5b422644...` et ne le qualifie jamais.

Ce nouvel histogramme n'est pas encore reçu : il enregistre `sans_owner`,
`non_positif` et `pertinent` provisoire, mais pas les lifts dégénérés ni les
rejets de rang; chaque pending est marqué `pertinent` avant census et la règle
`max(issue)` ne peut pas le reclasser vers `rang`. Sa clé `array<int,4>` encode
implicitement q par les sentinelles, ce qui reste sûr tant que les PointId sont
non négatifs, mais doit être explicité. L'agrégation courante mélange aussi les
trois arités alors que les claims `42/55/510` sont par q; elle doit publier une
matrice `q*issue` et fermer l'identité pour chaque q. Son p95 d'indice
`floor(0,95*n)` ne suit pas le nearest-rank lorsque `n` est multiple de vingt;
la convention reçue est `ceil(0,95*n)-1`. Enfin, le mode annoncé « petits
nuages » n'a ni cap ni préflight mémoire explicite. La porte minimale exige une occurrence
comptée une fois à l'entrée, une issue finale distincte, exactement une ligne
par arité, un total de groupes, `ecart=0` pour q2/q3/q4, puis une fixture de
rejet anticipé non vide.

L'issue finale ne doit pas partager le compteur d'occurrences. Un record de
diagnostic sûr sépare au moins
`{occurrences,seen_owner,geometric_status,final_rank_status}` : chaque lift
incrémente `occurrences` une fois; le census affecte ensuite `rank/relevant`
sans réincrémenter. Les deux fermetures indépendantes sont
`sum_key occurrences=lifts_built` et
`degenerate+no_owner+nonpositive+rank+relevant=unique_support_keys`. Enfin,
`std::map` reste une instrumentation bornée : l'option CLI doit refuser avant
allocation au-delà d'un cap explicite, pas seulement annoncer « petits
nuages » dans un commentaire.

## 7. Contre-audit de la note de multiplicité

La note
[`NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md`](NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md)
confirme `ecart=0` pour la nouvelle partition par arité, mais son histogramme
live ne reçoit pas encore ses conclusions.

Premièrement, les trois « issues » sont le stade maximal atteint, pas des
propriétés orthogonales. La positivité est calculée avant l'owner, mais un tuple
rejeté owner n'est jamais classé par positivité; « jamais possédé » peut donc
contenir des tuples intrinsèquement non positifs. Inversement, tout pending est
marqué `pertinent` avant census et les rangs ne le reclassent pas. Les `4 807`
dégénérés ne sont pas enregistrés du tout. Il faut des flags par clé
`valid`, `intrinsic_positive`, `owner_seen`, `rank_closed` et `relevant`, plus
une occurrence comptée une fois à l'entrée.

Deuxièmement, `52 693` est un minorant trivial du sous-ensemble pending de ce
pipeline figé, mais ni le nombre de géométries après le premier RLE ni une borne
universelle pour Source S ou H0. Le tableau contient
`144 235+66 897+52 693=263 825` clés distinctes non dégénérées. Les `4 807`
occurrences dégénérées manquantes représentent entre une et `4 807` clés
supplémentaires. Une géométrie par `SupportKey` ramènerait donc `2 220 024`
occurrences à `263 826..268 632` solves, soit un facteur diagnostique
`8,26..8,41`, pas `42`. Atteindre `52 693` suppose en plus un oracle parfait
pour owner et positivité sur toutes les autres clés.
Le RLE réduit précisément toutes les classes; il est faux d'affirmer que les
tuples possédés demandent seulement un test moins cher « pas moins nombreux ».

Troisièmement, le point `n=400` ne pince ni graine, leaf/max-depth/axe, source,
ELF ni transcript brut. Les `0,881 s` user incluent subdivision, bornes,
bitsets, enveloppes, census et `std::map`; sans compteur de cycles ni fréquence,
ils ne donnent pas « environ 1 200 cycles par occurrence ». Cette calibration
est retirée jusqu'à un profil de kernels/prédicats pincé.

La prochaine porte est donc une matrice `q*flags`, un nearest-rank p95 correct,
un cap/préflight de l'instrument et l'identité
`sum_q unique/support occurrences`. La proposition architecturale
`SupportKey-before-lift` reste renforcée, mais son gain doit être mesuré par le
nombre de toutes les clés uniques, jamais par les seules sorties pertinentes.

## 8. Contre-audit de la rampe `centre_cell_scale`

Le premier bloc `terrain,n=12 500` du transcript est isolable : lignes 1--33,
SHA-256 `7bc6ebd24f9daa83aeecf42fb995bd92e18c9a0d8a076aafed0e8383d5e357db`,
source `4884b293...`, CMake `d0738d1e...` et image exécutée
`5b422644...`. Il termine `rc=0`, `wall_s=797` sous charge, avec
`14 262 497` cellules, `756 017 485` tests bissecteurs, `561 399 279` tests
d'enveloppe, `92 531 928` lifts et `906 078` supports. Les identités agrégées
d'arbre, hull, pending, groupes et sorties ferment.

Il confirme toutefois le ledger de rang incomplet : les écarts q2/q3/q4 sont
`155 300/840 522/138 899`, soit `1 134 721` occurrences. Le global
`rank_rejected=1 134 183` compte des groupes anticipés plus 24 rejets finaux;
groupe et support ne sont pas la même unité. `rc=0` ne contrôle pas cette
partition. Les `102,124` lifts/support et `92,7221 %` d'issues owner sont des
diagnostics, pas un digest d'identité : la commande n'a pas `--judge`.

Le second bloc `terrain,n=25 000` termine ensuite `rc=0`, `wall_s=2 191` sur le
même ancien inode `5b422644...`. Il annonce `46 745 417` cellules,
`2 561 898 157` tests bissecteurs, `220 298 378` lifts et `1 872 528` supports :
`117,648` lifts/support et `92,874 %` de rejets owner. Les occurrences de rang
anticipé omises valent q2/q3/q4 `332 617/1 848 421/335 606`, soit `2 516 644`;
le compteur global compte des groupes et reste d'une autre unité. Ce point
renforce le NO-GO du port eager, sans identité juge ni temps qualifiable.

La campagne complète est **irrecevable comme rampe mono-binaire**. Le cas
25 000 a fini sur l'image supprimée `/proc/.../exe=5b422644...`, puis le cas
50 000 a effectivement démarré sur l'ELF différent `8fdfc8af...`, toujours sous
l'en-tête unique qui annonce `5b422644...`. Le script
temporaire n'est pas archivé, emploie `>>`, omet `multiecho`, le digest des
entrées/sorties, la liste des quatre fichiers dirty, les flags de build et la
mémoire. Ce fichier doit rester la trace d'une campagne mixte réfutée, jamais
être réécrit en reçu vert.

Le commit `02e709bf` l'a finalement supprimé au lieu de le renommer. C'est une
régression de provenance : l'objet `64cf6fe` ne conserve que 34 lignes arrêtées
après le 12 500, et la sortie brute 25 000 observée ensuite n'est plus dans Git.
Le nouveau `scale_counters_frozen.txt` a été commité ouvert à douze lignes,
avant même la sortie 12 500. Son driver temporaire ne couvre que trois familles,
reste fail-open et non archivé; il ne devient pas contractuel par le seul gel de
l'ELF. Conserver tout échec sous un nom `invalid_*`, finaliser atomiquement le
successeur, puis seulement indexer son hash terminal.

Une future rampe utilise un ELF immuable adressé par contenu, un en-tête et des
hashes avant/après **chaque** cas, la matrice contractuelle de six familles avec
Poisson uniforme et mélange équilibré bloquants, un fichier
temporaire finalisé atomiquement, RSS/workspaces, digests d'entrée et de
supports, puis refuse tout `ecart!=0` ou code non nul.

## 9. Filtre de diamètre du successeur

Le lemme ajouté au source live est exact. Si `c` appartient à la cellule, tous
les membres du support sont à distance carrée `beta` de `c`, donc son diamètre
carré `D2` vérifie `D2<=4*beta`. Pour chaque membre `x`,
`beta<=u_C(x)`; ainsi `D2*S2<=4*min u_C` dans l'échelle dyadique. Lorsque le
support s'étend, `D2` ne peut qu'augmenter et `min u_C` diminuer : une violation
est un prune monotone de tout sous-arbre.

Le live observé ne coupe pourtant rien : il incrémente seulement
`diameter_pruned`, puis continue. Son propre diagnostic annonce `0,64 %` de
violations à `n=1 500` et le classe non rentable. Ce compteur est donc une
ablation négative, pas un « prune ajouté ». Avant toute activation, exiger une
fixture d'égalité `D2*S2=4*U` conservée, le mutant `>` vers `>=`, un plancher
non vide, un accord des identités et les bornes u16/profondeur 26. Son coût
inclut plusieurs distances par triangle/q4 et doit être comparé aux lifts
réellement évités.

## 10. Le RLE par sous-arbre n'abolit pas encore le besoin d'une frontière de lots

Le commit `64cf6febafc4a80a48b4103667be0c69cf794e9d`, source SHA-256
`4d09080860ab949fda65d12f84e6249677e785b1e03db09807832393b7946720`,
ajoute une instrumentation `(SupportKey,batch_depth)`. Son titre conclut que le
RLE n'a pas besoin d'être global, mais le commit ne contient ni transcript, ni
reçu, ni borne. Des exécutions éphémères ont été observées sur un ELF ensuite
remplacé; leurs sorties ne sont pas auditables.

Pour une antichaîne **fixée** de sous-arbres, le RLE local reste exact : le lot
qui contient la feuille owner conserve le support pertinent; les autres lots
peuvent recalculer la même clé puis constater zéro owner. Mais exactitude ne
signifie pas parcimonie. La somme des clés distinctes par lot mesure bien les
solves non dégénérés d'un tel RLE; elle ne prouve aucune borne uniforme et peut
redevenir le nombre d'occurrences si une clé traverse de nombreux lots.

Les exécutions éphémères qui ont motivé le titre donnent seulement un point de
laboratoire non reçu. Sur `terrain,n=400`, elles annonçaient `N=2 215 217`
occurrences et `U=263 825` clés non dégénérées. Aux profondeurs de lot
`1/2/3`, les sommes locales étaient `311 158/410 803/604 962`, soit une
inflation `1,179/1,557/2,293` par rapport au RLE global et des gains locaux
`7,119/5,392/3,662`. Ces sorties n'ont ni transcript archivé ni ELF encore
disponible : elles illustrent les métriques, elles ne qualifient aucun chemin.

Il n'existe pas de petite borne géométrique cachée. Avec cinq sites u16, un
support diamétral joignant deux coins opposés a pour bissecteur un plan qui
traverse un nombre quadratique de cubes par niveau dyadique; lorsque
`n<smax-1`, les listes ne le retirent pas. Une exécution différentielle
historique `uniform,n=5,leaf=4,work_cap=1,max_depth=4` donnait `900`
occurrences, `19` clés globales et `894` couples clé--lot à la profondeur quatre,
soit un gain local `1,007`, avec accord juge `15/15`. Une autre, sur
`uniform,n=25,seed=11,leaf=4,work_cap=20000`, donnait un gain global `3,960`
mais seulement `2,276/1,575/1,130` aux profondeurs `1/2/3`. Elles sont elles
aussi historiques et non archivées, mais réfutent toute extrapolation de
localité depuis le seul terrain `n=400`.

L'instrument courant a cinq limites matérielles :

1. il omet encore les `4 807` occurrences dégénérées de l'exemple `n=400`;
2. `batch_depth` définit un sous-arbre de profondeur fixe, pas un lot borné en
   octets; aucune masse maximale, p95, RSS ou workspace par lot n'est publiée;
3. le lot implicite zéro absorbe les terminaux situés au-dessus de la profondeur
   choisie; `depth=0` ou une profondeur trop grande peut donc reproduire
   artificiellement le RLE global tout en annonçant `lots=0`;
4. `batch_counter` compte les racines créées, pas les lots non vides, et le CLI
   ne borne pas la profondeur relativement à `max_depth`;
5. les deux `std::map` sont des instruments CPU globaux; ils ne donnent aucun
   layout, trafic, radix, scratch, occupation ou HWM GPU.

La porte industrielle doit construire une antichaîne adaptative par `count`
exact : subdiviser jusqu'à `bytes<=B`, empaqueter les petits sous-arbres sans
couper une cellule terminale, et router un terminal trop gros vers split,
fallback exact ou `resource_exhausted`. Un run global de `SupportKey` peut
traverser plusieurs lots sans perte d'exactitude, mais paie alors un solve par
lot. Pour chaque cap `B`, publier par arité les occurrences
`O`, uniques globales `U`, somme locale `sum U_b`, réplication interlots
`sum U_b/U`, gain `O/(sum U_b)`, lots non vides, max/p50/p95
occurrences--uniques--octets--scratch, puis les digests d'identité. Poisson
uniforme et huit amas équilibrés sont les deux familles bloquantes; terrain
seul ne reçoit pas la décision.

Au point historique 12 500, les `92 531 928` occurrences représentent déjà
`1 480 510 848` octets pour quatre identifiants u32 seuls, avant cellule, `e0`,
contexte, count/fill et scratch radix. Le lot local peut borner le workspace,
mais ne réduit pas ce trafic d'émission; la gate doit compter les deux.

## 11. Audit précoce du prototype de lot différé vivant

Après `64cf6fe`, Claude a commencé un producteur borné par
`batch_rec_cap`. Cette revue est statique sur un worktree mouvant, complétée
par de petites portes épinglées qui ne transfèrent aucun résultat au source
live postérieur.

Le successeur suivant place cette voie derrière `--deferred-lift` et conserve
le chemin eager par défaut. Cela protège provisoirement les anciennes portes,
mais les vingt-quatre CTests antérieurs n'exerçaient pas le producteur différé.
Sur le couple source
`d47ed7ebe39013f82f6bd6991ad39de56a52fffa312b32cd8cb3c7d601c6f804`,
ELF Release
`8fdfc8af75639137ec3bd9974c6c5486d0d246b119ce9f59b41f74caccc46c32`,
les quatre nouveaux CTests différés ont passé `4/4` en `32,70 s`. Un recheck
sur le même ELF et le CMake
`0f64c1c60afbf4af51339807b758e49ec0312d4be69f7dcda8303d251616c865`
a repassé `4/4` en `50,11 s`; son `LastTest.log` avait le SHA-256
`e0c140085046eaf81e50560616468d1aef50f7cd29316b32c47a13748c22a8a3`
avant d'être écrasé par une exécution concurrente. Les temps sous charge ne
sont pas des mesures de performance. Les tests couvrent trois familles bornées
et un cap `1024`, mais ni mutant différé, ni frontière owner multi-support et
inter-arités, ni `--multiplicity`, ni HWM. Le cas grille exerce toutefois
`2 556` boules multi-supports, et sa variante cap `1024` garde le même payload
sur `1 879` flushes; il manque encore la distribution multi-arité dans une même
boule pour recevoir `qmin/H_run`. Le source live a changé après ces runs. Le
commentaire CMake annonçant un facteur cinq et un surcoût de treize pour cent
reste une observation sans
source--ELF--commande--transcript propre, pas un reçu.

Sur le même couple pincé, la commande bornée
`terrain,n=100,seed=11,smax=11,work_cap=20000,--judge` ferme exactement
`4 693/4 693` supports avec zéro absent, parasite, mismatch ou doublon en eager
et en différé aux caps `1024/4096/1048576`. Les lifts sont respectivement
`366 907/260 188/181 621/42 084`, les gains RLE
`1,000/1,410/2,020/8,718` et les lots `0/327/88/1`. C'est une petite gate de
correction et une courbe cap--réplication; elle ne mesure ni octets, ni HWM, ni
temps qualifiable et ne soutient pas qu'un petit lot capte l'essentiel.

Le snapshot intermédiaire `b9b90cf...` ajoutait `real_edges_triangles()` et un
état `adj_ready` sans les appeler. Le successeur historique `fd043fe...` les raccorde
bien : dans une bande `work<=work_cap*probe_factor`, il construit le graphe de
bissecteurs, compte ses vrais `E/T`, puis réutilise l'adjacence si la cellule
devient terminale. L'ancien constat « non raccordé » est donc historique.

Cette seconde étape n'est cependant pas encore une enveloppe de travail :

- `topp=max(mine.size(),m3p,m4p)` vaut toujours `mine.size()`, puisque les deux
  derniers termes sont des préfixes de `mine`; la sonde compte ainsi triangles
  et arêtes sur le pool q2 `D_9`, pas sur les cuts q3 `D_8` et q4 `D_7`;
- le verdict `E+9T<=work_cap` ne compte aucun K4. Dans une clique de taille
  `m`, `Q/T=(m-3)/4`; aucune constante neuf ne borne donc les quadruplets quand
  `m` croît. Le commentaire « rapport voisin de l'unité observé » est une
  heuristique de famille, pas un certificat de cap;
- la matrice dense réserve `top*ceil(top/64)` mots de 64 bits avant tout
  préflight d'octets. La bande multiplicative peut donc elle-même déclencher un
  gros workspace; il faut bitset seulement sous borne reçue, CSR sparse sinon;
- les sommes combinatoires et `E+9T` sont des `i64` non saturés, alors que le
  CLI accepte jusqu'à cent millions de points. Elles sont sûres au seul profil
  nominal 50 000, pas sur le domaine déclaré du binaire;
- `terminal_overlaps` continue de recevoir le potentiel d'intervalles `pot_e`,
  même lorsque la décision terminale vient de `real_e`; le nom du compteur ne
  décrit donc plus le travail effectivement accepté.

Cette sonde est un choix adaptatif fail-open pour l'exactitude scientifique,
pas un `work_cap` industriel. Il faut compter au minimum les vrais K4 ou un
majorant prouvé lane-specific, les octets de l'adjacence et la réplication des
enfants, puis refuser/splitter avant allocation si l'enveloppe est dépassée.

La réfutation tient déjà au cap par défaut. Pour `K_24`, `E=276`, `T=2024` et
`Q=10626`; ce snapshot acceptait car `E+9T=18492<=20000`, alors que son ancien modèle
`E+3T+6Q` vaut `70104`. Vingt-quatre points entiers d'une même coquille dont le
centre appartient à la cellule réalisent ce graphe bissecteur complet; la
fixture `coquille` en possède déjà trente. La porte permanente doit imposer que
ce cas ne soit jamais qualifié « sous cap » et tuer le mutant `Q_upper=T`.

Un majorant GPU peu coûteux évite d'énumérer les K4. Pour chaque arête orientée
`i<j`, poser `c_ij=popcount(N+(i) intersection N+(j))`. Alors
`T=sum c_ij` et `Q<=sum C(c_ij,2)`: tout K4 est compté par l'arête formée de ses
deux plus petits sommets; une paire de voisins communs non adjacents ne fait que
surcompter. Masquer le cut q4 resserre cette borne. Les sommes sont saturées à
`work_cap+1`; si `adj_bytes=8*top*ceil(top/64)` ou le budget de popcounts est
dépassé avant allocation, la branche subdivise ou rend `resource_exhausted`.
À `top=50000`, la matrice dense seule vaut environ `313` Mo.

Le schéma `occurrence compacte -> tri SupportKey -> un solve -> recherche owner`
est le bon ordre. Une régression de porte est certaine dans les octets observés :

- `record_tuple()` retourne toujours `true`. Le mutant `arity-cascade` consulte
  donc `pair_kept/tri_kept` avant owner, positivité et rang; il ne simule plus
  « engendrer q3/q4 seulement depuis un support inférieur retenu ». Une fixture
  peut encore le tuer par un filtre hull ou par la coupure de lane, mais ce vert
  ne reçoit plus l'ancienne mutation d'admission. Il faut conserver une voie
  mutante sémantique ou une fixture dédiée `--deferred-lift` qui l'exerce après
  décision; les portes eager et les quatre accords différés sans mutant ne
  suffisent pas.

Le census cellule par cellule n'est pas, à lui seul, une faute du snapshot :
avec un arbre terminal commun, deux supports de la même sphère ont le même
centre et donc la même feuille half-open owner; comme une cellule terminale
n'est jamais coupée entre lots spatiaux, leurs pending owner sont co-localisés
dans le même `BatchCell`. Cette cellule est déjà un `BallOwner` exact et le RLE
par boule peut rester local exact-once. C'est la correction matérielle du
diagnostic antérieur qui exigeait à tort un second RLE global dans ce layout.
La propriété doit être exercée par une cosphère multi-supports et disparaît si
une feuille est coupée ou si arités, backends ou epochs ont des partitions
distinctes.

Elle ne se transfère pas non plus automatiquement aux shards hashés par
`SupportKey` : deux supports distincts de la même boule atteignent généralement
deux shards. Le `BallOwner` fournit alors la destination exacte, mais il faut
router les pending par `(owner_cell,GeometricBallKey)` avant le census, ou
accepter des census répétés puis une réduction aval reçue. `b_cert>=H_run` et le
contexte owner entier restent obligatoires dans les deux layouts.

Autres portes avant réception : le cap porte sur le nombre d'occurrences et est
testé seulement **après** une cellule terminale, donc le HWM peut dépasser le cap
de toute la production d'une cellule. `BatchCell` est en outre créé avant de
savoir si la cellule émettra un seul record et copie chaque CSR `cands` et
`bucket_end`, dont les octets ne sont pas comptés. Une suite de feuilles à zéro
ou peu de records peut donc accumuler des contextes sans approcher
`batch_rec_cap`; plusieurs occurrences owner
du même tuple dans une même feuille doivent être RLE sur leur contexte ou
signalées, pas injectées plusieurs fois. Or `owner_multiple` est seulement
compté : chaque occurrence owner reçoit encore un pending et aucune erreur
d'invariant n'est levée. Le vieux `propose()` duplique la géométrie dans le
source, créant une dette de parité entre les deux voies. Dans le seul mode
différé, `--multiplicity` ne remplit plus ses tables et ses facteurs peuvent
diviser `0/0`, car `record_tuple()` n'appelle pas `note_occurrence()`.
`batch_records` n'est ni préflighté ni imprimé dans l'en-tête; le flush
final vide gonfle aussi le nombre de lots. Les fixtures reconstruisent leurs
`Options` et n'héritent pas nécessairement `--deferred-lift` ou
`--batch-records`; une porte fixture doit donc vérifier le cap effectif dans son
propre reçu.

L'égalité finale `interior==h` est vraie, même sans assertion dédiée. Au départ,
le membre de support de rang d'entrée maximal donne `r_(e0)>=e0`; après une
promotion `h'=r_h`, le nesting donne `r_(h')>=r_h=h'`; au premier arrêt la
condition de boucle donne `r_h<=h`, donc `r_h=h`. Seule la justification du
commentaire par un membre toujours absent de `D_(h-1)` cesse de valoir après la
première promotion. La vraie porte manquante est un mutant `strata-stop` reçu,
avec éventuellement une assertion redondante au point fixe.

Sur le couple `d47ed.../8fdf...`, combiner `--deferred-lift`, cap `1024` et
`--multiplicity` imprime cinq classes vides, `total=0` contre `260 188` lifts,
puis deux facteurs `NaN`, tout en rendant le code zéro. Tant que l'instrument
n'est pas recâblé au flot différé, cette combinaison doit refuser explicitement
ou rester hors des reçus.

Exiger au minimum `owner_multiple=0` comme assertion fail-closed, identité
séparée occurrences/solves/issues par arité, parité du payload avec le snapshot
juge, mutants tous tués, cap transactionnel et HWM total en octets avant toute
mesure de gain. L'axe q4 conserve un `TriangleLift` physique par occurrence
hors de `lifts_distincts`; son coût doit rester dans un ledger séparé. Le
libellé `lifts_distincts` doit aussi être conditionné : en mode eager il compte
les occurrences, pas des clés distinctes. Enfin, l'ordre des causes diffère —
owner avant positivité en eager, positivité avant owner en différé — donc leurs
pourcentages ne sont pas directement comparables.

## 12. Le probe historique `E+9T` ne bornait ni les quadruplets ni la mémoire

Le snapshot historique `fd043fe...` ajoutait une terminalisation sur le graphe
réel de bissecteurs : il comptait ses arêtes `E` et triangles `T`, puis
acceptait si `E+9T<=work_cap`. Cette quantité est un modèle heuristique, pas un
cap combinatoire, car elle ignore le nombre `Q` de cliques de taille quatre.
Dans le graphe complet `K_24`, `E=276`, `T=2 024` et `Q=10 626` : pour un cap
`20 000`, `E+9T=18 492` accepte tandis que la métrique pondérée déjà déclarée
par le prototype vaut `E+3T+6Q=70 104`. Plus généralement,
`Q/T=(m-3)/4` dans `K_m`; le facteur `probe_factor` borne seulement la bande où
la sonde est appelée, pas cette sous-estimation. L'exactitude de la sortie reste
fail-open, mais le contrat de ressource ne ferme pas.

La forme duale clique-count/upper-shadow de Kruskal--Katona fournit ici une
borne entière GPU-friendly. Pour `T>0`, écrire l'unique développement canonique
`T=C(a3,3)+C(a2,2)+C(a1,1)`, avec `a3>a2>a1>=1` et termes nuls omis : choisir
gloutonnement `a3`, puis `a2`, et poser `a1` exactement égal au dernier reste.
Alors tout graphe ayant `T` triangles et `Q` copies de `K4` vérifie
`Q<=C(a3,4)+C(a2,3)+C(a1,2)`; pour `T=0`, poser `Q_KK=0`. Une admission sûre
pour la même métrique déclarée est donc
`E2+3*T3+6*Q_KK(T4)<=work_cap`, avec les triangles du préfixe q4. Une version
calculée entièrement sur le surgraphe q2 reste sûre mais plus lâche. La
recherche et les binomiales emploient u128 et saturent à `cap+1`; sous
`m<=50 000`, le score maximal reste inférieur à `2^61`, mais le CLI à cent
millions de points n'est pas couvert par i64. La borne plus simple
`4Q<=T(m-3)` est aussi sûre mais plus lâche. Ni l'une ni l'autre ne remplace un
cap séparé sur les octets, le scratch et le temps.

La représentation dense actuelle alloue
`m*ceil(m/64)` mots u64. Son compte exact emploie, pour
`W=ceil(m/64)`, au moins
`Theta(c_2 W+E_2+E_3 W+T_3+T_4 W+Q_4)` opérations; le terme
`T_4 W` atteint `Theta(m^4/64)` dans une clique. Contre-correction de l'autre
audit : à `m=50 000`, cela représente `39 100 000` u64, donc `312 800 000`
octets ou environ `298,3 MiB`, et non `2,5 GB`; le défaut reste matériel. Le
layout sparse candidat est une CSR forward orientée par `(degre,PointId)` ou
par dégénérescence, avec intersections merge/galloping; réserver les bitsets
tuilés aux seuls sommets de fort degré sous cap exact. `T` alimente alors la
borne de Kruskal--Katona sans énumérer `Q` avant la décision de terminalisation.

Une enveloppe exacte plus précoce vient des degrés forward. Dans toute
orientation donnée par un ordre total, chaque triangle ou K4 a un unique plus
petit sommet. Si `d_q^+(v)` est le degré forward du cut q, alors
`T3<=sum_v C(d_3^+(v),2)` et `Q4<=sum_v C(d_4^+(v),3)`. Le `count` CSR sature
ces sommes à `cap+1` sans intersection. Une orientation de dégénérescence
garantit `max d^+<=d` et donc les bornes `n*C(d,2)` et `n*C(d,3)`; elle n'est
pas prétendue meilleure que tout autre ordre sur chaque graphe. Le pipeline GPU
essaie donc enveloppe de degrés, `T3/T4` exacts avec
`Q_KK(T4)`, puis `Q4` exact sous petite largeur et préflight. Cela ne borne pas
encore le temps ni la mémoire totale.

## 13. Contre-audit du successeur à K4 exacts

Le snapshot historique `HEAD=02e709bf`, source
`dbaa2e0128c5be30e2f7c75784e38758a45c7bb938fba5d8ab4a87c71d5ad764`,
et son ELF Release
`423797e9964538f42701660d8baaf492b302f801a4aeb4b0df1b183986a5a037`
absorbent la réfutation précédente : la sonde est limitée à `top<=96`, compte
exactement `E2` sur `D_(smax-2)`, `T3` sur `D_(smax-3)` et `T4/Q4` sur
`D_(smax-4)` — donc `D_9/D_8/D_7` à `smax=11` — puis décide avec
`E2+3T3+6Q4`. L'orientation supérieure des bitsets fait compter chaque triangle
et chaque K4 une fois. Ce couple passe les 28 CTests `centre_cell`; la branche
de sonde n'est toutefois pas exercée au facteur par défaut.

Cette identité de cuts suppose `have_thresholds`. Quand le pool commun contient
moins de `smax-1` sites, le code pose `c2=c3=c4=|P|`; le compte reste exact sur
les lanes réellement parcourues, mais celles-ci sont des supersets fail-open et
ne sont plus littéralement `D_(smax-q)`.

Quatre réserves empêchent encore de parler de cap industriel :

1. le défaut `probe_factor=1` désactive algébriquement la sonde : elle exige
   simultanément `!terminal`, donc `work>work_cap`, et
   `work<=work_cap*probe_factor`. Les CTests enregistrés ne passent pas
   `--probe-factor>1`; le nouveau flot `E2/T3/T4/Q4` n'est donc pas exercé;
2. le diagnostic d'incidence calcule bien
   `bound=floor((m4-3)T4/4)`, mais échoue seulement pour `Q4>bound+1`. L'inégalité
   entière exacte est `Q4<=bound`, équivalente à `4Q4<=(m4-3)T4`; le `+1`
   masque précisément une erreur d'une unité;
3. le plafond 96 borne la matrice de **la sonde**, pas celle de `generate()`.
   Sous `have_thresholds` et sans overflow, le partage du seuil `R_top` empêche
   toutefois une liste arbitrairement longue d'avoir un potentiel quasi nul :
   la vraie brèche restante est une cellule terminalisée par `max_depth`, par un
   `leaf` CLI relevé, ou après overflow hors profil. Elle alloue encore le
   bitset dense sans préflight. Le cap ne borne pas davantage `BatchCell`,
   occurrences, enfants, pending ou scratch;
4. les potentiels combinatoires et compteurs cumulés restent en i64 sans
   saturation sur le domaine CLI allant jusqu'à cent millions de points. Le
   reçu n'imprime ni `probe_factor`, ni `probe_top_cap`, ni `batch_records`.

La gate minimale ajoute un test A/B explicite avec facteur supérieur à un,
`probe_tests>0`, identité de payload, fixture `K_24`, mutant d'incidence d'une
unité et HWM total en octets. L'énumération exacte de `Q4` est acceptable sous
le plafond 96; sur device, le majorant
`sum C(popcount(N+(i) intersection N+(j)),2)` permet d'arrêter plus tôt dès le
cap dépassé.

Le successeur `HEAD=3ffff85`, source `d2039ba...`, ajoute un prune
enfant--`tight` exact, le réemploi de scratch vectors et le contrôle i128 exact
de l'incidence. La suite ciblée 30/30 observée sur son ELF Release exerce deux
portes de sonde; elle n'est pas archivée comme reçu durable et la porte saine
n'emploie ni `--judge`, ni vérité indépendante pour `E2/T3/T4/Q4`. Le stdout
publie `T3` sous `probe_triangles`, mais la garde utilise le `T4` caché : le
lecteur ne peut pas recalculer `4Q4<=(m4-3)T4` depuis le reçu. Publier
`probe_triangles_q4` et les deux membres maximaux de la garde est obligatoire.

Son mutant `Q4*=4`, malgré son nom `incidence-off-by-one`, teste la sensibilité
de la garde et non l'orientation de l'énumérateur. `min_probes>0` ne garantit
pas à lui seul sa mort si la sonde ne voit aucun K4 ou si la borne a du slack;
la fixture `K_24` saturée reste nécessaire. Un vrai mutant `Q4+1` serait au
contraire tué sur toute clique complète, puisque la borne d'incidence y est une
égalité; le défaut du test terrain est l'absence de cette fixture, pas une borne
intrinsèquement non serrée.

Le smoke terrain tue empiriquement ce mutant, mais celui-ci modifie ensuite le
score d'admission et donc le parcours, puis journalise chaque violation. La
porte durable garde le compte réel pour toute décision, teste séparément la
copie mutée et échoue vite avec un diagnostic propre à l'incidence.

Le worktree postérieur `1b6ca68...` ajoute une ablation et `cell_pts`, non reçus.
Il accepte encore les planchers malgré son commentaire; en différé,
`ablate=2` exécute encore les lifts et `ablate=3` peut accumuler les contextes
sans flush; la sonde calcule des cliques avant certains retours; `cell_pts`
ajoute une copie `Theta(top)` non préflightée. Une sortie volontairement fausse
peut encore employer le schéma exact et le code zéro. Le CMake worktree
`c9a7386...` duplique les trois noms de tests d'ablation. Il faut un contrat
diagnostic distinct et des portes dédupliquées avant toute mesure de coût.

GCP non utilisé. Aucun fichier de code ou de reçu n'a été modifié.
