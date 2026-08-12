# Audit courant de MorseHGP3D v3

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur

`HEAD` observé au dernier contre-audit :
`02e709bfed8c879391496ae6cd335c41d1cdc584`, commit
`retract the local dedup claim, freeze the ramp binary, count the graph exactly`.
La rétractation du titre `64cf6fe` et les comptes combinatoires par lane sont
justifiés; les claims de coût et le reçu encore ouvert restent contre-audités.

Le worktree n'est pas identique à ce `HEAD`; Claude modifie encore le prototype
pendant que l'audit ne touche qu'aux textes autorisés. La campagne mixte
antérieure a été supprimée du `HEAD` au lieu d'être conservée sous un nom
`invalid_mixed`; l'objet Git `64cf6fe` n'en retient que 34 lignes, jusqu'au cas
12 500, et la sortie 25 000 observée ensuite n'est plus archivée. La nouvelle
campagne gelée est active. Son état initial commité était un manifeste ouvert de
12 lignes; le cas terrain 12 500 a depuis fermé `rc=0` et le 25 000 est actif.
Elle ne couvre que trois familles. Le 12 500 a chevauché les 202 secondes du
CTest sur l'hôte deux-cœurs, puis la suite 30 chevauche le 25 000; aucun de ces
temps n'est qualifiable.

| objet courant | SHA-256 |
| --- | --- |
| `CMakeLists.txt` au `HEAD` | `0f64c1c60afbf4af51339807b758e49ec0312d4be69f7dcda8303d251616c865` |
| source au `HEAD`, pincé par la nouvelle campagne et les 28 CTests | `dbaa2e0128c5be30e2f7c75784e38758a45c7bb938fba5d8ab4a87c71d5ad764` |
| ELF Release gelé de cette campagne et des 28 CTests | `423797e9964538f42701660d8baaf492b302f801a4aeb4b0df1b183986a5a037` |
| manifeste gelé ouvert observé avant toute sortie | `bac860dddf72a07c3ec944efa66900548a79c115c7a2cd074d9ac374a6a15487` |
| transcript gelé après fermeture 12 500 et lancement 25 000, encore ouvert | `91398d9eeee10fc024499537c543e0d2b20f9ce603efb675622d17ea91ffe8ee` |
| source de l'ancien `HEAD=64cf6fe`, instrumentation de lots non qualifiée par un transcript | `4d09080860ab949fda65d12f84e6249677e785b1e03db09807832393b7946720` |
| ELF disque successeur observé à 15:17 UTC, non qualifié | `4f0ed7a984d9366c67b68ca8f36e228b3891d31c24cd11a3fa1bb97a7254ad9e` |
| source worktree postérieur avec lot différé, non construit/non testé à 15:24 UTC | `64b7598358d27a1aaf5544437cf2824665ec9786e02014527b6c1c10941cb190` |
| source de la gate différée bornée, historique | `d47ed7ebe39013f82f6bd6991ad39de56a52fffa312b32cd8cb3c7d601c6f804` |
| ELF Release de cette gate, aussi chargé par le 50 000 tronqué | `8fdfc8af75639137ec3bd9974c6c5486d0d246b119ce9f59b41f74caccc46c32` |
| `CMakeLists.txt` de la gate différée | `0f64c1c60afbf4af51339807b758e49ec0312d4be69f7dcda8303d251616c865` |
| source worktree intermédiaire avec squelette d'adjacence réelle non raccordé | `b9b90cf589066e19bd31fde8d67c6015450c7d6017418021b9679ff125edd22a` |
| source historique à sonde `E+9T`, réfutée comme cap | `fd043fe8627804a8500d59147474e92e0bb20e7fa665e533f075ab15ff23ce8e` |
| source intermédiaire à cuts et K4 exacts, non construite | `e10638bd1b165a382724c9e13b457478ba942e3eb0c14f70859be7af78c6a14c` |
| source worktree postérieur, prune enfant--`tight`, scratch hoistés et contrôle d'incidence corrigé; CMake Release construit, suite 30 active | `d2039bab3e74ae1443aeefcac756152060566a38c7d87afbda245da612920b34` |
| `CMakeLists.txt` worktree avec deux portes sonde/incidence en plus | `08e54fc1f7f87262d0a90b9fc3a51963185c06b4ca06f522775b38cce7144bce` |
| ELF Release worktree correspondant, suite 30 en cours et non reçue | `fc2eb10cfbc91ad33c89cfcf2a3301f41ab1ff65e62b83d54c16623f9863b295` |

Le registre configuré recense `488` CTests, dont `28` préfixés
`mhgp3v_centre_cell_`. Les 28 passent sur le couple
`dbaa2e0.../423797e...` en `202,12 s`; sortie SHA-256
`ac8063615912a8272c1e781f3b1baf8381ecc056180abbb2cd9c266d7861cd58`,
`LastTest.log`
`ac5774d57f40e1e785f62baf666f477a34388b0ac1723f1cb35c1c8c6e61e750`.
Le temps est contaminé par la campagne concurrente, mais le résultat fonctionnel
est reçu dans sa portée bornée. Il n'inclut ni mutant différé sémantique, ni
`owner_multiple` fail-close, ni HWM d'octets, ni le source worktree
`d2039ba...`. L'ancien inventaire `484/24` reste historique.
Les pins ci-dessous restent les observations historiques antérieures.

Après reconfiguration du worktree, le registre contient `490` CTests, dont
`30` `centre_cell`; la suite complète est en cours et ne constitue pas encore
un résultat. Les deux nouvelles portes imposent une sonde non vide et tuent le
mutant sur `terrain,n=200`, mais ne comparent pas les identités au juge et ne
graveront pas à elles seules l'égalité saturée `K_24`. Leur exécution chevauche
le cas gelé 25 000; son temps sera lui aussi non qualifiable.

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

Le nouveau transcript gelé reproduit ensuite ce point 12 500 sur
`dbaa2e0.../423797e...` : mêmes `14 262 497` cellules, `92 531 928` lifts et
`906 078` supports, ELF identique avant/après et ledger par arité désormais à
`ecart=0`, avec `rank_early=155 300/840 522/138 899`. C'est un diagnostic
count-only reproductible; sans `--judge`, digest d'identités, mémoire ou
pipeline, il ne reçoit pas l'exactitude générale ni le SLO. `probe_tests=0`
confirme aussi que les comptes `E2/T3/T4/Q4` ne sont pas exercés au défaut. Son
`wall_s=871` est contaminé et non qualifiable.

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
de travail ou de mémoire. La correction exacte proposée est la borne duale de
Kruskal--Katona `Q<=Q_KK(T)`, puis la gate
`E+3T+6Q_KK(T)<=work_cap`, calculée en u128 saturé; une CSR forward sparse
remplace la matrice dense `Theta(m^2/64)` mots. Aucun test n'est transféré à ce
successeur.

Le snapshot `dbaa2e0...` corrige les deux défauts combinatoires de
ce snapshot : à `smax=11`, cuts `D_9/D_8/D_7`, compte exact `E2/T3/T4/Q4`, admission
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

Le worktree `d2039ba...` ajoute un rejet des enfants dont la fermeture est
strictement disjointe du `tight` parent, puis réemploie des scratch vectors.
Le prune est mathématiquement sûr : pour un support positif, `c_B` appartient
à `relint conv(U)`, donc à `bbox(mine_parent) intersection cell=tight`; les
comparaisons à l'échelle enfant sont exactement `child` contre `2*tight`, et
les inégalités strictes conservent la tangence. Le réemploi est sémantiquement
neutre dans cet Engine mono-thread. Il corrige aussi le contrôle d'incidence par
produits i128 exacts, ajoute `min_probes` et un mutant multipliant `Q4` par
quatre. Des smokes O2 éphémères ont été observés et une suite CTest est en
cours; aucun résultat achevé ou reçu ne qualifie encore ce delta.
`min_probes>0` ne garantit pas la mort du mutant si
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
5. Garder P1a actuel comme falsificateur; construire cœur Jung puis wavefront
   témoin persistante et suivre son protocole natif direct.
6. Construire q2/q3/q4 par lanes `D_9/D_8/D_7` et budgets `h`, avec partition
   terminale commune, `e0` immuable et promotion, sans transits d'arrangement
   ni dépendance envers un support inférieur retenu. Après génération locale,
   faire un premier RLE `SupportKey` avant lift, choisir le contexte owner,
   puis un second RLE par clé primitive de sphère et un strict-count/census
   fermé unique par boule; `U_B` est un certificat aval. Pour Gamma, garder
   la provenance requise; pour le H0 normalisé, employer un support canonique
   et le token de fermeture Johnson au lieu d'énumérer tous les supports d'une
   cosphère. Graver les deux contre-fixtures inter-arités, l'invariant
   pool-relative, les arbres de budgets indépendants et le shell 30. Deux
   pentes rouges de candidats, listes, census, postings ou octets ferment cette
   route avant CUDA.
7. Fermer le terminal `k=n`, `BallActivation`, premières incidences, gateways,
   resolver, MSF/fold, dix forêts, verticales et payload hôte. Seul ce pipeline
   complet peut mesurer le SLO.

GCP non utilisé.
