# Audit courant de MorseHGP3D v3

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> **Alerte de worktree au `HEAD=783a789`.** Le contre-audit live détaillé du
> delta mouvant de Claude est
> [`AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`](AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md).
> La primitive `BlockJungDual64` à poids fixes est sûre, mais son premier
> raccord additionnait des groupes à des singletons sans union de `PointId` ;
> le raccord courant répare ce packing et trois CTests causaux passent. Il
> publie encore `OK` lorsque son juge est partiel, accepte des options vacuaires
> et agit après la descente complète. Le chemin recommandé conserve les bases
> comme hyperarêtes et ferme sur `tau(F)>=8` **avant** cette descente. La dernière
> session G4 documentée a échoué avant sa rampe et a bien été arrêtée
> `TERMINATED` : elle ne fournit aucune mesure 50k.

## Snapshot

Le dernier commit stable relu est
`HEAD=783a78934fef97b2c2836ee49c6f03ddd18d2e08`, commit
`BlockJungDual64 : une base ferme un rectangle entier en 64 coins`. Depuis le
pin `cec4a4f`, deux commits ont ajouté le branchement optionnel de `SOC64`, sa
recette G4, puis la primitive `BlockJungDual64`.

Le delta logiciel observé de Claude porte sur
`M morsehgp3D_v3/CMakeLists.txt` et
`M morsehgp3D_v3/prototype/wspd_wavefront_probe.cpp`, hors présentes écritures
documentaires de l'auditeur. Il tente le raccord de
groupes BJD au ledger WSPD, avec reçus d'identité, mutants et juge primal. Ce
delta mobile n'est pas reçu et doit être repinné, compilé et rejoué après
stabilisation. L'auditeur n'a modifié aucun logiciel.

Empreintes SHA-256 au commit `783a789` :

- `CMakeLists.txt` :
  `2532db5f1befa1520e3aa0fd20d1dc2788a2ca62004a40d15b0d4839063c60f5` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `64936944ef2f190299a2f7d67b0387b5002aab9c9019c6e830db6492e0189dfb` ;
- `prototype/jung_dual_probe.cpp` :
  `d05a997c1c10e1e02918f3a585e7cdee45fda6814edf4930ccbae9b4030fd4e6` ;
- `prototype/block_jung_dual.hpp` :
  `99d960eb42e452a7cec126428945c1a56bf8984c7b138d18d3ea837f9a56ae5d` ;
- `receipts/soc64_actif_g4_20260814/transcript.txt` :
  `18e6dd0f1aeafca51639805761a15855acbd08f4446ef9b4cbe7132db64977eb`.

Le commit stable reçoit l'identité algébrique ponctuelle de BJD et son header
CPU. Il ne reçoit ni source CK--WST, ni profondeur factorisée, ni ledger de
`PointId` combinant groupes et singletons, ni primitive device, ni payload.
Les titres de commits restent des claims, jamais des verdicts reçus. GCP non
utilisé par le présent auditeur.

## Verdict

Le contrat G4 reste ouvert. Il n'existe encore ni source u16 reçue, ni stage
`0B` produit, ni `BenchmarkOutputContract-v1` complet et éligible, ni campagne
p95 à 50 000 points. Le squelette du contrat de benchmark existe, mais ses
étages critiques restent incomplets ou absents.

`SOC64 --actif` et `BlockJungDual64` sont des diagnostics amont. Aucun ne
traverse `BallEvent -> 0B -> payload`; ils ne qualifient donc ni exactitude
industrielle ni SLO.

Le snapshot contient cinq probes/campagnes utiles, mais aucune chaîne produit
reçue :

1. `BallFormToBallEvent-v0` forme des sphères et des census sur petit domaine ;
2. `PointHypergraphBottleneckClosureProbe-v0` compare Kruskal et Floyd sur un
   même hypergraphe de points ;
3. le raffinement local réduit le résiduel `E4` du certificateur central, au
   prix d'un front et d'un nombre de recertifications supérieurs ;
4. la rampe à 50 000 points rejette la configuration centrale mesurée sur
   `eight_clusters`, sans réfuter les certificateurs rectangles corrélés ;
5. `q4_brute_oracle` énumère les 4-sous-ensembles à petit `n`, avec un census
   et des prédicats encore corrélés/incomplets sur le shell.

Les noms « 0A fermé », « stage 0B » et « le raffinement paie » dépassent les
preuves disponibles.

## Nouvelle source candidate : `CKPairTape -> WST3 -> WST4`

L'audit reçoit la construction mathématique comme proposition exacte et
GPU-factorisable, pas comme implémentation :

- une WSPD Callahan--Kosaraju canonique partitionne toutes les paires en
  `O(s^3 n)` rectangles physiques sans les développer ; après filtrage des
  seules paires endpoint `D=0`, elle source tous les q2 propres. Un bucket de
  positions dupliquées conserve obligatoirement la multiplicité et les vrais
  `PointId` dans les pools témoins et les produits ; les paires vers toute
  troisième position gardent cette multiplicité ;
- pour chaque rectangle `R=(A,B)`, les cellules Morton d'une échelle liée à sa
  boule `B_R` et rencontrant `2B_R` couvrent tout carrier d'un support dont
  `ab` est l'arête maximale ; l'intersection avec les deux enveloppes endpoint
  resserre encore ce domaine. L'owner longueur/`EdgeKey` donne un unique
  `OwnedCK-WST3(A,B,C)` ;
- les couples non ordonnés de ces cellules donnent de même un unique
  `OwnedCK-WST4(A,B,C,D)` ; q4 recertifie directement les quatre
  barycentriques du circumcentre.

Les nombres de blocs **initiaux** conditionnels sont `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` sous une vraie propriété
fair/compressed-split et `0<eta<=1`. Ils ne bornent ni les raffinements `MIXED`,
ni les masses logiques `|A||B|`, `|A||B||C|` et `|A||B||C||D|`. Chaque bloc
reste paresseux jusqu'à un consommateur factorisé reçu ou au preflight atomique
de sa vraie sortie.

Une fixture u16 de 64 points interdit toute cascade de rang : un q4 régulier a
rang 4 alors que ses six arêtes q2 et ses quatre faces q3 ont toutes rang 12.
`WST4` doit donc consommer les carriers aigus géométriques pré-rang, jamais les
événements q3 retenus. La relation est construite lorsque
`q3_open || q4_open`, même si la lane de rang q3 est déjà fermée. Le rapport et le recalcul exact sont dans
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).

Pour une paire ponctuelle, `JungDiskDepth9/8` restreint les centres au disque
imposé par Jung dans le plan médiateur. Des groupes disjoints de trois IDs au
plus peuvent y fermer q3/q4 même lorsque le LP sur tout le plan échoue. Cette
preuve ne ferme pas un rectangle CK : le plan et les demi-plans varient avec
`a,b`. Une contre-fixture `2×2` est désormais documentée ; il faut scinder vers
des microtiles rejoués ou prouver un `BlockJungDiskDepth` uniforme.

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
Mesurer ensuite le coût transitif `E4 -> F3/C4_carrier -> F4/M4_apex ->
BallKeys -> census -> fold`, pas la seule pente de `E4`.

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
Poisson uniforme et le mélange équilibré de huit amas. `sum E4` reste la gate
du certificateur universel mesuré, mais la contre-famille `two_lines` interdit
d'en faire une gate de la source : elle conserve une masse quadratique avec
zéro carrier aigu et zéro q4.

Réponses complètes aux trois questions de Claude :
[`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md).
Le détail contractuel du reçu et des continuations est dans
[`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## `C4/M4/H4` : le mur avant rang est réel, les claims du HEAD ne le sont pas

Le schéma courant sépare `C4_carrier` pour les faces aiguës,
`M4_apex` pour les quadruplets canoniques avant barycentriques,
`W4_positive` après positivité et `H4_rank` après census. L'owner maximal doit
rester dans la route proposée : changer seulement l'owner ne réduit ni les
vrais supports ni les 4-ensembles à attribuer. L'arête maximale fournit un
diamètre qui borne immédiatement les cinq autres distances. Une autre broad
phase exigerait une nouvelle preuve de couverture ; une `BallKey` est aval et
serait un owner de génération circulaire.

Le sampler v2 améliore v1 : il exclut `PENDING` et ne censure plus les grosses
lentilles. Le HEAD a remplacé la fausse barre `2 sigma` par une demi-largeur
Hoeffding correcte **conditionnellement** à des tirages indépendants et
uniformes. Cette loi n'est pas reçue : multiply-high reste sans rejet exact,
les deux streams SplitMix à seed fixe n'ont pas de contrat d'indépendance et le
delta n'est pas réparti sur les décisions simultanées. `W4` n'a pas d'intervalle
propre. Le champ est honnêtement renommé `repetitions_consecutives`, mais le
contrôle direct ne compare toujours pas le décodeur rang--`PairId` à un mode
exhaustif déterministe. La vue combinée SOC reste absente.

`--rang` peut sortir code zéro sans `--porteurs` et sans aucune ligne rang. Sur
`two_lines,n=40`, il peut aussi juger zéro bien-centré puis réussir. Son taux
est conditionné par arête et par positif, sans poids `binom(|L_e|,2)` : ce
n'est pas `H4/W4`. Surtout, il compte seulement `I_B`. Le rang fermé vaut
`|I_B|+|U_B|`; sous le régime régulier q4 il faut `I<=7` **et** `U=4`, ou un
refus des extra-shells pertinents. Le libellé `rang<=7` est donc faux.

Le nouvel énumérateur brut confirme seulement des comptes exhaustifs à petit
`n`. Il recopie Gram--Cramer et in-sphere du sujet : l'énumération est
indépendante, les prédicats ne le sont pas. Son claim
`M4=Theta(n^4)` « pour n'importe quel nuage » est réfuté par sa propre famille
`two_lines`, où `M4=0`. Les tailles `60/90/120` ne prouvent ni `H4` linéaire ni
une marge de deux ordres ; à `eight_clusters,n=120`, le rapport moyen publié au
seuil sept est seulement `6,6`. Les cas `n<4` produisent encore `NaN`, et aucun
cap n'empêche le coût `O(n^5)`.

Le mur mathématique existe néanmoins. Le support strict
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)`, `y=(31,19,31)` possède owner
`ab` unique, deux faces aiguës et barycentriques `(47,3,55,55)/160`. Un produit
de voisinages réels conserve ces inégalités et montre qu'une masse logique
quartique peut exister avant rang. L'ancienne instanciation par cubes unitaires
u16 n'était cependant pas uniforme : seuls `2093/4096` supports passaient.

La fixture de bloc exacte mise à l'échelle utilise
`A=(20000,20000,20000)+{0,1}^3`,
`B=(30000,30000,30000)+{0,1}^3`,
`C=(19000,31000,31000)+{0,1}^3`,
`D=(31000,19000,31000)+{0,1}^3` et
`Z=(20000,20000,30000)+{0,1}^3`. Ses `4096` supports ont owner `AB`, une
barycentrique minimale `13217143/721310286>0` et les huit IDs de `Z`
strictement intérieurs. Après fixation du signe d'orientation, le déterminant
in-sphere normalisé est convexe en `z`; ses huit coins suffisent donc pour
certifier `ALL_INTERIOR`. La bonne cible est une preuve de profondeur **par
bloc avant fill**, pas une énumération plus rapide des quadruplets.

Le count M4 n'exige pas davantage de fill. Initialiser `M4_pending` à la masse
distinct-ID non décidée, calculée par Möbius sur les quinze partitions de
`A/B/C/D`. Pour une masse `m`, `ALL_Q` fait
`M4_pending-=m; M4_L+=m`, `NONE_Q` fait `M4_pending-=m` et `MIXED_Q` remplace
atomiquement le parent par ses enfants ; `M4_U=M4_L+M4_pending`.
`M4_L>B_fill` rejette seulement le fill ponctuel. `M4_U<=B_fill` reçoit sa
capacité, mais count final, offsets et publication exigent encore
`M4_pending=0`. L'identité exacte par arête/`PlaneKey` reste un microkernel
endpoint borné : à 50 000 points, un RLE global matérialiserait `1249975000`
arêtes et `62496250050000` incidences `(e,z)`. Les compteurs soustraits restent
complets ; les saturer séparément peut transformer `B+2-(B+1)=1` en zéro.
`M4_raw` pré-profondeur et `residual_output` après profondeur sont deux ledgers :
une fermeture de profondeur crédite `domain_mass_closed`, pas `M4_closed`.

La route reçue comme proposition est : proposer des bases sur l'arête, les
vérifier par `BlockJungDual64`, puis fermer seulement si `tau(E_Q)>=8` ;
`FaceAxisJungDepth8` après la porte aiguë, puis
`Corner8BallDepth/BlockBallDepth8` après la cellule apex. Un reçu de profondeur
`ALL` ferme un bloc, `MIXED` scinde et la sweep ne reçoit que le résiduel
preflighté. `2B_R` est une enveloppe extérieure sharp lorsque seule la
boule contenant les endpoints est connue ; elle réduit les cellules, pas la
masse réelle. Une grande masse logique peut tenir dans peu de blocs, mais elle
n'est utile que si le consommateur de profondeur reste lui-même factorisé.
Sur une face fixe, ce consommateur est désormais explicite : poser
`p=min(8,n_permanents)`, puis conserver les `8-p` seuils gauches les plus grands
et les `8-p` seuils droits les plus petits donne un noyau d'au plus 16 IDs qui
préserve exactement le seuil `Depth>=8`. Un scan
top-k `O(n)` et un replay des égalités remplacent la sweep et le DAG général ;
une égalité uniforme est groupée comme shell, seul un ordre indécis scinde.
Les bouts irrationnels montent vers 207 bits sous u16 et exigent i256/quatre
limbs ; le lift bloc garde sa propre preuve de largeur.

La dissection `n=1500` trouve huit témoins singleton exacts pour `89,5 %` de
200 PairId q4 ouverts tirés par masse, contre `26,5 %` de 200 rectangles hachés.
Elle révèle du potentiel pairwise, sans mesurer la perte causale : les deux
échantillons ne sont ni appariés ni pondérés dans la même unité, et le second
n'exerce aucun groupe Jung. L'option live `--ordre-proche` réduit fortement le
pending aux fenêtres 32--128, mais à finalité 256/512 le résiduel reste exactement
`E4=1071162` dans les deux ordres. C'est une compression de budget du
certificateur central, pas une réduction M4.

Réponses aux six questions, preuves, contre-fixtures et portes :
[`AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit du sampler v2, du brute-force, la réponse entière à la question
7 et les microgates `JungDual/BlockBallDepth8` sont dans
[`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).

## `JungDepth` et `BlockJungDual64` au HEAD stable : théorème fixe reçu, intégration ouverte

La forme entière duale `A/P/R` est correcte sous u16, paire propre et
`sum(weights)<=65535`, mais elle vérifie seulement les poids fournis. Son échec
reste `UNKNOWN`. `BlockJungDual64::make_base` préflighte la somme en `i128`, mais
`dual_lane` l'additionne encore en `i64` sans ce cap ; aucun des deux headers ne
porte les `PointId`, et le mutant étroit utilise un overflow signé. Le wrapper
futur authentifie donc profil, cap, IDs et disjonction. Il peut exposer une
primitive nommée `verify_dual_weights_lane`, mais ce symbole n'existe pas au
`HEAD` et ne serait jamais un décideur de couverture.

Le HEAD ajoute le bon juge indépendant pour une base fixe. Dans le plan
`s dot (b-a)=0`, il minimise exactement `||s||^2` sur les demi-plans mauvais,
en BigInt, puis compare strictement `3*r^2>D` ou `2*r^2>D`. Les formes
`K_ij`, `Delta` et `N` sont algébriquement cohérentes et les fixtures q4 de
taille un/deux passent. Les `13/13` CTests Jung ciblés sont verts. Il manque
toutefois le cas Helly réellement ternaire, les normales projetées nulles et le
preflight u16. Une fixture exacte nécessaire prend
`a=(0,100,100)`, `b=(100,100,100)`, puis
`z1=(5,90,100)`, `z2=(5,100,90)`, `z3=(0,110,110)` : les régions mauvaises
`Y>=75/2`, `Z>=75/2`, `Y+Z<=20` ont chaque intersection par paire non vide dans
le disque q4, mais leur triple est vide.

Le titre du commit « le primal retire encore un espoir » n'est pas reçu.
`--primal` ne remplace que les groupes de taille deux d'un greedy disjoint sur
`--voisins`; la taille trois emploie encore une banque finie. Il mesure donc
`p`, jamais la profondeur `d`. Aucun CTest ne lance ce mode et le claim « quatre
mesures identiques » est faux sur le binaire du même HEAD : à
`eight_clusters,n=600`, les fermetures passent `169 -> 170`, et à `n=1500`,
`189 -> 190`. Même sur sa cible réduite, le primal récupère déjà une base.

La fixture `u<p<d` du probe ne calcule pas `d` : elle imprime le littéral huit
après avoir cherché seulement singletons/paires et extrait greedily `p`.
Dupliquer géométriquement `g1` comme troisième gadget laisse cette porte publier
`u=6,p=7,d=8`, alors que le vrai minimum tombe à sept. Le prochain juge doit
calculer `d` ou son équivalent combinatoire, jamais le graver.

Cet équivalent est maintenant exact. Soit `E_q(P)` l'hypergraphe de rang trois
des bases Helly couvrantes. Les intérieurs de tout centre frappent chaque base,
et tout transversal des bases laisse, par Helly, un contre-centre sur les IDs
restants. Donc :

```text
Depth_q(P) = tau(E_q(P))
```

Le packing courant n'est que `nu(E)<=tau(E)`. Un branch-and-cut alterne un
solveur bitset de transversal et le juge primal : un transversal `R`,
`|R|<h`, donne soit un contre-centre sur `P minus R`, soit une nouvelle base
disjointe de `R`. Chaque base géométrique n'est vérifiée qu'une fois ; le replay
GPU devient combinatoire.

Le lift uniforme de chaque hyperarête est lui aussi résolu. Pour une base et
des poids fixes, poser
`A0=-W*(a dot b)+(a+b) dot Z-Q` et
`C0=W*(a cross b)-a cross Z-Z cross b`. q4 teste
`A0>0 && 2*A0^2>||C0||^2`, q3 remplace `2` par `3`. Le couple `(A0,C0)` est
affine séparément en `a` et `b`, et chaque lane est un cône de Lorentz convexe.
Les 64 couples de coins caractérisent donc exactement `ALL` sur l'enveloppe
`A×B`. `BlockJungDual64` ajoute au branch-and-cut seulement les bases qui
passent uniformément ; les autres provoquent un split. Sous u16 et
`1<=W<=65535`, les identités `A4=4*A0`, `R=4*||C0||^2` et les bornes
`|A0|<2^50`, `|C0_i|<2^49` placent ses comparaisons dans i128. Cette combinaison
`primal proposer -> dual64 verifier -> tau(E)>=8` est le premier certificat
constructif fail-open de `BlockJungDepth8` sans tableau de `PairId`. Son pire
cas peut encore visiter toutes les feuilles du produit.
Ces bornes supposent un widening avant les sommes, normes et produits, ainsi
qu'un preflight de `W` en accumulation large ou saturante.

Aucun artefact durable ne permet d'identifier un second auteur ou modèle : Git
attribue les deux flux documentaires au même auteur. Le présent contre-audit
redérive néanmoins le résultat. À base et poids communs fixés, `(A0,C0)` est
séparément affine et la lane est un cône de Lorentz strict convexe ; la double
interpolation des 64 coins est donc correcte sur l'enveloppe AABB continue. La
réserve `for all pair exists lambda(pair)` contre
`exists lambda for all pair` reste impérative. Les constantes 80/99 sont
cohérentes avec Montejano--Oliveros, théorème 3.1, puis la borne stricte de Tuza
`eta(3,h)<(h+1)^2`. Cette redérivation valide les énoncés, pas une indépendance
organisationnelle ni l'intégration live.

La réception logicielle de bloc reste ouverte. Le seul CTest BJD du commit
stable est une regex greffée au selftest Jung : il compare les deux formes sur
des couples ponctuels puis des AABB dégénérées en points. Il n'exerce aucun
produit AABB non dégénéré, aucun mutant BJD, aucun changement de poids par coin,
ni les preflights coordonnées/boîtes/IDs. `bjd_lane_box` paie en outre un
prétest intérieur puis jusqu'à 64 coins : le pire cas annoncé est 65 prédicats,
pas 64.

Son ABI confond encore invalidité et réfutation : `!base.valide` retourne
`kLaneNone`. Le callsite q4 live reste fail-open parce qu'il teste seulement
`retour>=q4`, mais un consommateur générique de lanes pourrait publier un faux
`NONE`. La réception exige `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`. Le commentaire
de `jung_dual.hpp` inverse par ailleurs le minimax ; la bonne identité est
`min_w max_z Phi_z(w)=max_lambda min_w sum_z lambda_z Phi_z(w)`. Les formules
codées suivent la bonne identité, donc cette faute documentaire ne réfute pas la
primitive.

Un wrapper de profondeur reçu devra conserver des ensembles authentifiés de
`PointId`, pas seulement `cred/ccred`. Le packing minimal exige chaque groupe disjoint des
autres groupes et de tous les singletons/spans déjà crédités dans la même vue.
La route plus complète garde les groupes comme hyperarêtes et teste
`tau(F)>=h`, ce qui traite leurs recouvrements sans double crédit. Un cap de
juge donne `PARTIEL/UNKNOWN`; il ne transforme jamais un raccord non jugé en
accord.

Le packing live est désormais sûr sous ses invariants de probe, mais il est
NO-GO comme hot path dans son ordonnance actuelle. À `n=1500`, les lectures
restent exactement identiques avec et sans BJD (`32387961` sur `uniform`,
`9366805` sur les amas), tandis que les médianes CPU utilisateur augmentent de
`5,47 %` et `8,15 %`. Le gain de masse q4 vaut respectivement `12,55 %` et
seulement `0,87 %`. Le certificat doit donc fermer avant la descente. Le
préfiltre `BJD-BilinearBounds` redérivé dans le contre-audit est sûr : 36 extrema
bilinéaires donnent le minimum exact de `A0` et une surborne de `||C0||^2` ; un
succès implique le verdict des 64 coins, un échec retombe sur ceux-ci ou
`MIXED`.

Helly avec tolérance borne en plus le payload ponctuel. Pour tout
`R`, `|R|<=h-1`, la profondeur exige l'intersection vide des mauvais convexes
hors `R`. Il existe donc un `ToleranceKernel` de taille
`eta(3,h)<(h+1)^2` : par intégralité, au plus **80 IDs pour q4** et **99 pour
q3**, sans affirmer l'optimalité de ces constantes. Ce théorème est existentiel
et pointwise : il ne borne ni la découverte, ni le nombre de proof-tiles, et
`for all pair exists kernel` ne donne aucun noyau commun à un rectangle CK.
Sur une tuile, les mêmes bases doivent être vérifiées uniformément ou provoquer
un split. Après une face aiguë fixe, la dimension un est plus forte : les top-k
seuils donnent constructivement un noyau d'au plus 16 IDs pour q4.

## SOC64 actif au HEAD : baisse locale du résiduel, aucune campagne G4 reçue

Le faible gain du classifieur scalaire `D/V/T` ne réfute que des extrema
décorrélés. Il ne réfute ni le spindle ponctuel, ni les rectangles corrélés, ni
le disque de Jung.

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q4
exige `H>0` et `3H^2>EX`. Si les 64 couples de coins de
`(C-A) times (B-C)` passent, tout le rectangle est `ALL`; le premier échec est
seulement `UNKNOWN`. La fixture axiale où toutes les différences sont
colinéaires positives ferme q4 par `SOC64` alors que la borne scalaire échoue.

Le commit `110fe76` ajoute `--soc64-actif` comme disjonctif q4 : un succès
change le fate sans double-compter ses descendants. Son message publie, à
`n=3000`, `uniform 22,842 % -> 13,602 %`,
`terrain 12,603 % -> 6,781 %` et
`eight_clusters 89,933 % -> 73,767 %`. Aucun transcript de commande ni reçu de
sortie associé dans le dépôt ne permet de transformer ces chiffres locaux en
pente ou en résultat G4.

Le branchement demeure incomplet pour la vue combinée : sous `central-NONE`,
un échec SOC au nœud courant ne descend pas vers d'éventuels descendants
SOC-`ALL`. Cela perd des fermetures mais ne crée pas de faux `ALL`. Les ledgers
baseline, SOC et futurs groupes Jung gardent des preuves d'identités et des
statuts `PENDING` séparés.

La session `soc64_actif_g4_20260814` n'a produit aucune rampe. Elle a construit
seulement `mhgp3v_wspd_wavefront_probe`, puis un regex CTest plus large a
demandé des exécutables absents ; CTest a rendu `rc=8`. Le transcript conserve
l'échec et `stop_and_verify.sh` certifie exactement
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` en
`TERMINATED`. Ce reçu qualifie l'arrêt, pas SOC64, une pente ou le SLO. Le
script du commit `783a789` est seulement corrigé partiellement et n'a pas de
reçu d'exécution : il omet encore `mhgp3v_jung_dual_judge`, sélectionné par son
regex CTest, et ce regex ignore les portes `mhgp3v_bjd_*`. Il avale le code de
l'analyseur, omet `--exige-fenetre-finale`, gate le seul `sum_E` à `1,70` au
lieu des compteurs physiques à `1,35`, et autorise `4*3000 s` par job sous des
coupe-circuits de `4800/5400 s`. Sa rampe `s=8,r0` reste CPU-only, sans aval
officiel et sans vue SOC complète sous `central-NONE`.

Prochaine porte : stabiliser le worktree, construire toutes les cibles réellement
demandées, rejouer localement les mutants et l'oracle d'union par `PointId`,
puis mesurer les mêmes tâches à `1500/3000/6000` avec `pending=0`, coût, HWM et
sorties. Aucune nouvelle rampe 50 000 avant cette porte.

`JungDiskDepth8` restreint le plan des centres au disque
`||y-d||^2<=D/2`. Une fixture à huit groupes disjoints ferme ce disque alors
que le LP global échoue même à profondeur un. Sous `smax=11`, le LP à 3280
appels correspond à `h=8`; il reste un oracle pairwise, jamais le hot path ni
le diagnostic d'une « vraie pénurie » sur le domaine Morse.

Preuves, largeurs, fixtures et cascade :
[`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Contre-audit croisé des flux documentaires

Dans ce corpus, « l'autre auditeur » désigne opérationnellement
`AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md`, explicitement relu
par `AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` section 0.1. Il
ne désigne pas une identité indépendante vérifiable. Les conclusions sont donc
reçues ou rejetées par leur contenu et leur snapshot, jamais par autorité
personnelle.

Le premier de ces documents a correctement découvert l'owner décidé sur
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
ne prouve pas l'absence d'un support source et son arbre à 3280 LP pour `h=8`
ne doit pas être présenté comme GPU/hot path.

Sa restriction « WSSD compacte ne borne pas la sortie » est correcte. Sa
conclusion « WSSD seulement broad phase » est trop étroite : avec partition
CK, owner total, certificats `[L,U]` et blocs paresseux, `OwnedCK-WST` devient
une source factorisée exacte. Elle ne devient toujours pas une liste sparse.
De même, le théorème des faces aiguës q4 est utile uniquement avant le rang ;
la fixture 64 points interdit toute cascade depuis les événements q3 retenus.

Le second contre-audit mathématique de cette passe a également corrigé mes
propres formulations : `2B_R` est sharp seulement à information de boule
contenante fixée ; un poids `lambda` commun rend `BlockJungDual` sûr mais
incomplet à cause du swap `for all pair exists lambda`; et un glouton arbitraire
de groupes sur l'axe n'est pas complet. La formulation est maintenant plus
simple : les extrema top-k donnent directement le noyau tolérant exact, sans
matching global. Ces corrections sont intégrées à la proposition et aux fixtures. Pour
`BlockBallDepth`, une profondeur moyenne élevée ne prouve pas encore que huit
mêmes groupes couvrent uniformément un bloc : cette efficacité reste une gate
falsifiable.

Le dernier contre-audit valide les signes et seuils stricts du primal Jung,
mais corrige deux nouveaux raccourcis : i256 n'est sûr qu'après la réduction
`g_i/K_ij`, et les comptes `3280/9841` appartiennent aux profondeurs `h=8/9`
sous `smax=11`. Il réfute aussi l'interprétation du diagnostic feuilles : seul
le témoin est singleton, `A/B` restent des boîtes, et `pending=0` n'implique
pas que les sous-arbres `central-NONE` ont été visités. Ces corrections sont
maintenant dans la proposition et dans l'audit M4 actif.

Le prototype historique `JungDual` code correctement la forme entière ponctuelle
`A/P/R` et ses seuils q3/q4 sous la précondition u16 et `sum w<=65535`. Son
`dual_lane` ne préflighte pas cette somme et les paramètres zéro rendent
l'ablation vacuaire. Le nouveau `BlockJungDual64::make_base` corrige le cap pour
sa propre base, sans durcir l'ancienne ABI. Le juge primal séparé reçoit désormais le collectif `k=2`, mais aucune
fixture `k=3`, aucune profondeur `tau(E)` et aucun rectangle CK uniforme. Le
raccord `--primal` reste un packing greedy et son claim de gain nul est réfuté
par les replays `169 -> 170` et `189 -> 190`. Son succès représente une paire
ponctuelle, jamais encore une proof-tile uniforme ni un chemin device.

Le contre-audit détaillé corrige aussi les anciennes confusions entre q2 et q4,
cutoff kNN, taux empirique et borne structurelle.

## Rejeux ponctuels

```text
ball_event Release ciblé : 10/10 verts
suite ciblée ball_event/WSPD/Gamma/postings/saturated : 87/87 verts
faces q4 exactes : 5/5 ; q3 côtés hors rang : 6/6
fixtures centre_cell arité 3/4 + mutant cascade : 3/3
SOC64 isolé : 16/16 ; WSPD--SOC intégré : 5/5, oracle d'union incomplet
WSPD--SOC/porteurs/two_lines/cap au HEAD : 17/17 en 33,18 s
q4_brute au HEAD : 5/5 en 2,40 s, prédicats et shell non indépendants
sous-suite SOC/WSPD/porteurs/two_lines/q4/Jung/diag/ordre : 56/56 en 25,79 s
contre-calcul BigInt ad hoc Corner8 (pas une CTest) : 4096/4096,
  marge owner=11892000,
  min bary=13217143/721310286, pire J_U=-79011820908103787995
dissection de perte live : 1/1 en 0,80 s, populations non appariées
flip direct cap 1000 : accord=OUI, juges=47, sautes=3, faux=0 (statut faux)
mutant somme cap complet : code 4, juges=168, sautes=0, faux=25
JungDual UBSan étroit : overflow signé à jung_dual.hpp:157
Jung ciblé au HEAD : 13/13 verts en 0,31 s, collectif k=2 reçu, k=3/tau(E)/OPEN_FINAL absents
BJD au HEAD : seulement point + boîtes dégénérées ; produits non dégénérés/mutants absents
worktree BJD packing : 3/3 CTests verts, nominal et deux mutants
rejeu consolidé worktree SOC/Jung/BJD : 10/10 verts en 1,21 s sur cette machine
worktree BJD cap 1 : code 0/OK avec groupes sautes=98 et fermetures sautees=10 (statut faux)
worktree BJD sans vwave : code 0/OK, essais=0, couvrants=0 (mode vacuaire)
worktree SOC actif + juge shadow : zéro verdict actif jugé ; raccord actif sans autorité intégrée
worktree judge-vwave + SOC/BJD : code 1, 149 fermetures dites « sans 10 » ; juge central incompatible avec les preuves collectives q4
rejeu local final Jung/BJD : 16/16 verts en 1,45 s, dont 13 Jung et 3 BJD live
ablation BJD n=1500 uniform : masse q4 -12,55 %, CPU user médian +5,47 %, lectures identiques
ablation BJD n=1500 amas : masse q4 -0,87 %, CPU user médian +8,15 %, lectures identiques
session G4 SOC actif : CTest rc=8, aucune rampe, cible TERMINATED
ablation primal amas n=600 : 169 -> 170 fermetures ; n=1500 : 189 -> 190
--rang=10 sans --porteurs : code 0, aucune ligne rang
two_lines n=40 avec rang : code 0, bien_centres_juges=0
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
  -> SOC64 union-disjointe + JungDiskDepth9/8 paire/microtile
  -> primal proposer -> BlockJungDual64 uniforme -> branch-and-cut tau(E)>=h
  -> CarrierBlocks dans 2B_R-lentille dès q3_open || q4_open
  -> OwnedCK-WST3 puis WST4 symbolique pré-rang
  -> FaceAxisJungDepth8 puis Corner8BallDepth/BlockBallDepth8 avant tout fill q4
  -> raffinement porteur de preuves sur les tâches encore MIXED
  -> mesurer F2/F3/C4_carrier/F4/M4_apex/W4/H4/T4, BallKeys, census, H
  -> seulement alors portage device et campagne G4 50k
```

Cet ordre bloque toute réception produit, pas la falsification architecturale.
En parallèle, une piste `counter-only` peut recevoir sur petit `n`, contre
vérité exhaustive, `CKPairTape -> carrier aigu -> BlockJungDual64/tau(F) ->
AxisKernel/BlockBallDepth`. Elle ne promeut ni 0A, ni 0B, ni le statut public ;
elle permet de mesurer les fermetures **avant** descente, les splits, `F4/M4`,
les nœuds de transversal, les octets et la HWM avant d'investir dans un portage
G4.

La proposition consolidée est
[`../PROPOSITION.md`](../PROPOSITION.md). L'index court des audits est
[`README.md`](README.md).

GCP non utilisé par le présent auditeur ; la session échouée de Claude est
certifiée `TERMINATED`.
