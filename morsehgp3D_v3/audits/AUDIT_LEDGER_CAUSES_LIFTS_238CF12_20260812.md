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

GCP non utilisé. Aucun fichier de code ou de reçu n'a été modifié.

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

Deuxièmement, `52 693` n'est ni une borne inférieure de cette architecture ni
le nombre de géométries après le premier RLE. Le tableau contient
`144 235+66 897+52 693=263 825` clés distinctes non dégénérées. Une géométrie
par `SupportKey` ramènerait les `2 215 217` occurrences enregistrées à au plus
`263 825` solves, soit un facteur diagnostique `8,40`, pas `42`. Atteindre
`52 693` suppose en plus des prunes parfaits pour les `211 132` autres clés.
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

La campagne complète est **irrecevable comme rampe mono-binaire**. Pendant que
le cas 25 000 continuait sur l'image supprimée `/proc/.../exe=5b422644...`, le
fichier exécutable sur disque a été remplacé par `49c8a508...`; les lancements
suivants peuvent donc changer de binaire sous un en-tête unique. Le script
temporaire n'est pas archivé, emploie `>>`, omet `multiecho`, le digest des
entrées/sorties, la liste des quatre fichiers dirty, les flags de build et la
mémoire. Ce fichier doit rester la trace d'une campagne mixte réfutée, jamais
être réécrit en reçu vert.

Une future rampe utilise un ELF immuable adressé par contenu, un en-tête et des
hashes avant/après **chaque** cas, les quatre familles prévues, un fichier
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
