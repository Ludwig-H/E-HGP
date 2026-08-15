# Contre-audit du commit `3d4c598` — exactitude locale, pentes et relèvement bloc

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le noyau mathématique ponctuel du cône cible est **admis** dans le profil u16 :
les trois écritures entières sont équivalentes, les huit coins donnent une
porte `ALL` exacte pour une AABB cible, et le rejet `NONE` publié est un
certificat fail-open sûr. C'est une primitive utile et nouvelle : elle atteint
les témoins proches des endpoints que les seules boules concentriques au
milieu de l'ancre manquaient.

Le producteur worktree n'est toutefois **pas reçu** comme source, comme front
industriel ou comme route 50 k. Trois blocages précèdent tout port G4 :

1. un `smax` hors largeur `int` ferme actuellement toute la masse sans un seul
   test et le juge partage la conversion fautive ;
2. le juge ne reçoit que la conjonction des trois morts, pas les décisions
   q2/q3/q4 séparées que consommerait un aval ;
3. la DFS recommencée pour chaque endpoint garde deux pentes rouges sur tous
   les compteurs de travail mesurés, même avec une banque 96.

Le statut opérationnel est donc **NO-GO du port littéral avant G4**, pas une
réfutation de la primitive ni une preuve de latence impossible sur G4. La
reprise prioritaire est le relèvement collectif
`A_endpoint × B_partner × C_witness`, avant tout `PairId`. Le contrat
`50 000/1 s`, a fortiori la cible principale `100 ms`, reste entièrement
ouvert.

## 1. Pins observés et fraîcheur

Pendant l'audit, Claude a promu le delta au `HEAD`
`3d4c5986752594b1eeb6c92d248b4d156df42b7c`, commit
`find the witnesses next to the endpoints, then measure what that
costs`. Le CMake et les deux sources spindle du worktree coïncident avec ce
commit ; les deltas restants sont documentaires et appartiennent aux
auditeurs. Les empreintes du snapshot indépendamment rejoué sont :

| objet | SHA-256 |
| --- | --- |
| `prototype/spindle_cone.hpp` | `78037fc19d0f2dae63b28745ee8741e10bd7821a8da3278032ad2dae76db0a85` |
| `prototype/spindle_cone_probe.cpp` | `bf64663298d16d2035eaf3b274ec3bd7214ce74cc701fdfe1c606636815010a3` |
| `CMakeLists.txt` | `4f4733bccf37828f735ca473b4a063947eeafaf646c6bf4c63daeea6bd4ebc44` |
| ELF Release `mhgp3v_spindle_cone_probe` | `abbc57c5a430e06c63b94631a584b37df83b1dd33280426c5199bfa3d2d5faef` |

Tout octet postérieur est un nouveau successeur et n'hérite pas
automatiquement de ce verdict. Aucun fichier d'implémentation n'a été modifié
par les auditeurs.

Le reçu brut `midball_eight_clusters_raw.txt`, SHA-256
`47a97ff78ae34cc0713057a4daaaac88093b4f75c7d0304e326e65137ffbf892`,
répare utilement la provenance des anciennes colonnes : il reproduit les
`1/2/40` prunes à `n=150/200/300`, puis les compteurs du successeur. Il publie
cependant `identite non_verifiee=no-store` et aucun digest de nuage ; c'est une
réparation de transcript, pas un reçu `SpindleConeReceipt-v1` durable.

## 2. Preuve du noyau ponctuel

Pour une ancre orientée `(a,b)` et un témoin `z`, poser
`e=z-a`, `t=b-z`, `H=t·e`, `E_2=||e||^2`, `X_2=||t||^2` et
`R=||t×e||^2`. Comme `(b-a)×(z-a)=t×e`, les spindles universels sont :

$$C_3(a,z)=\left\lbrace b:H>0\text{ et }3H^2>R\right\rbrace,\qquad C_4(a,z)=\left\lbrace b:H>0\text{ et }2H^2>R\right\rbrace.$$

L'identité de Lagrange `R=E_2X_2-H^2` donne les comparaisons moins coûteuses :

$$C_3(a,z):H>0\text{ et }4H^2>E_2X_2,\qquad C_4(a,z):H>0\text{ et }3H^2>E_2X_2.$$

En écrivant `t=αu+v`, avec `u=e/||e||` et `v` orthogonal à `u`, on obtient
respectivement `||v||<sqrt(3)α` et `||v||<sqrt(2)α`, avec `α>0`. Ce sont des
cônes de Lorentz ouverts convexes, d'apex `z`. Une AABB fermée est l'enveloppe
convexe de ses huit coins ; elle est donc incluse dans `C_q(a,z)` si et
seulement si ses huit coins le sont strictement. Le centre seul, sept coins,
l'acceptation de l'égalité et l'oubli de `H>0` sont tous faux.

Le rejet `NONE` est également sûr. `H_max` est le maximum affine exact sur la
boîte. Chaque composante de `t×e` est affine en la cible ; la distance de zéro
à son intervalle donne un minorant, et leur somme quadratique donne `R_lb`.
Ainsi :

$$H_{\max}\leq0\quad\text{ou}\quad c\max(H_{\max},0)^2\leq R_{\mathrm{lb}}$$

certifie l'absence de cible dans le cône, avec `c=3` pour q3 et `c=2` pour q4.
Les corrélations peuvent seulement rendre `R_lb` trop faible et perdre un
rejet ; elles ne peuvent pas créer un faux `NONE`.

Sur u16, `H`, `E_2` et `X_2` tiennent dans 34 bits non signés après le test de
signe, mais `E_2X_2` et `4H^2` demandent respectivement jusqu'à 68 et 70 bits.
La promotion avant produit est donc obligatoire. Le mutant i64 est
effectivement tué par la fixture pleine largeur.

### 2.1 Lecture probabiliste, jamais certificat

Sous des directions indépendantes isotropes — hypothèse absente du contrat —
les fractions solides des cônes q2/q3/q4 valent respectivement
`1/2`, `1/4` et `(1-1/sqrt(3))/2≈0,211325`. À `smax=11`, les probabilités
binomiales de manquer neuf témoins q3 ou huit témoins q4 sont environ :

| banque `M` | échec q3 | échec q4 |
| ---: | ---: | ---: |
| 48 | `0,1190` | `0,1759` |
| 64 | `0,0111` | `0,0262` |
| 96 | `2,76e-5` | `2,06e-4` |

Cette seule heuristique explique pourquoi 96 améliore fortement la masse
fermée. Elle ne prouve ni indépendance des voisins, ni couverture d'une AABB,
ni borne universelle de banque, ni coût favorable : les mesures montrent
précisément que le travail reste rouge.

## 3. Rejeu des portes et réparations déjà faites par Claude

Le premier rejeu observé rendait `27/28`. Le seul rouge,
`mhgp3v_cone_mur_amas_ferme`, calculait bien `23 803` prunes et `130 246`
paires ordonnées fermées, mais sa regex `prunes=2[0-9]{4}` ne pouvait pas
matcher : le moteur de regex CMake ne reconnaît pas le quantificateur `{4}`.

Le contre-audit a également relevé que `PASS_REGULAR_EXPRESSION` ignore le
code de sortie du processus. Les anciens tests du juge imprimaient
`accord=OUI` avant d'évaluer leurs planchers ; un code 3 pouvait donc rester
vert. Claude a remplacé ces regex par des planchers de verdict et des tests à
code, puis a ajouté une garde CMake contre le motif `{n}`.

Sur les quatre empreintes de la section 1, le rejeu indépendant :

```text
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_cone_' -j2
```

rend `30/30` en `5,97 s`, avec ELF identique avant/après. Ce vert reçoit les
fixtures, les trois écritures ponctuelles, quatre juges bornés, trois
permutations, le mur amas, neuf mutants et les refus/planchers présents. Il ne
reçoit ni le domaine `smax`, ni les morts par lane, ni un vrai résiduel, ni les
pentes, ni CUDA/G4, ni le pipeline officiel.

Un second rejeu final des mêmes 30 portes et du même ELF rend également
`30/30`, en `9,41 s` sous une charge différente. Ces durées ne sont pas des
mesures de performance ; seul l'accord fonctionnel est retenu.
L'inventaire live total est `603` CTests. La suite globale n'a pas
été rejouée.

Le mutant `cone-ignore-inherited` existe mais n'a pas de CTest dans ce
snapshot. Son exécution manuelle sur `uniform,n=300,bank=48,--verify` rend bien
le code 4 et neuf désaccords affichés ; cette sensibilité doit devenir une
porte permanente, car l'héritage sans recrédit est l'invariant central du
parcours.

## 4. Blocages d'exactitude et de contrat

### 4.1 Conversion `smax` partagée avec le juge

La CLI stocke `smax` en `long long`, ne vérifie qu'une borne basse, puis le
convertit en `int` dans le producteur et dans le juge. Au pin de la section 1 :

```text
./build/v3/mhgp3v_spindle_cone_probe \
  --points=20 --family=uniform --seed=3 --bank=4 --verify \
  --smax=9223372036854775807
```

rend le code zéro, `witness_node_tests=0`, `fermee=380/380`, puis
`juge accord=380/380`. Les seuils convertis sont non positifs ; sujet et juge
ferment donc ensemble sans témoin. C'est une contradiction exacte, pas une
question de performance.

La réparation minimale est de valider la largeur avant tout cast. Le profil
produit peut simplement exiger `smax=11`; un probe générique peut accepter une
borne documentée dans `int`. `smax>n` n'a pas besoin de fabriquer un rejet :
il peut rester un cas fail-open où aucun seuil n'est atteignable. Une banque
plus petite qu'un seuil est de même sûre : la lane correspondante est
infermable par cette banque et rejoint l'aval sans parcours inutile.

`parse_ll` doit en outre remettre `errno` à zéro et refuser
`ERANGE`. L'autorité d'enveloppe hôte permet actuellement
`4<=smax<=34`, tandis que la qualification device annonce
`4<=smax<=24`. Claude doit choisir un domaine commun, le valider avant
le cast et graver `LLONG_MAX`, `INT_MAX+1`, la borne
supérieure plus un, `3` et les suffixes.

### 4.2 La cardinalité demandée n'est pas la cardinalité jugée

La commande suivante retourne zéro :

```text
./build/v3/mhgp3v_spindle_cone_probe \
  --points=100 --coord=2 --verify --permute
```

Le reçu publie pourtant `cloud ... n=8`. Toutes les identités, le juge
et la permutation sont donc verts sur un autre univers que celui demandé.
Immédiatement après `make_family_cloud`, le probe doit exiger
`pts.size()==opt.n` et refuser en code 2 avant le LBVH. La porte grave
cardinalité demandée, cardinalité produite et hash du nuage.

### 4.3 Le juge ne reçoit pas les lanes séparées

Le sujet comptabilise `mass_closed_q2/q3/q4`, mais son bitset scientifique est
rempli seulement lorsque les trois bits de mort sont présents. Le juge marque
de même une paire vraie seulement lorsque q2, q3 et q4 sont toutes mortes. Il
reçoit donc le prune total actuel, pas une fermeture q3 ou q4 isolée qui
autoriserait l'aval à sauter une lane.

La porte bornée requise possède trois bitsets sujet et trois vérités `(g,Q)` :

```text
closed_q2_subject subset dead_q2_judge
closed_q3_subject subset dead_q3_judge
closed_q4_subject subset dead_q4_judge
```

Elle compare les identités de `PointId`, pas seulement les masses, et tue un
mutant propre à chaque lane. Les identités
`mass_closed_q + mass_alive_q=n(n-1)` sont nécessaires mais purement
comptables. Elles ne remplacent aucune des trois inclusions.

Ce juge de front ne remplace toujours pas le juge aval demandé
`(BallKey,SupportKey,I_B,U_B,ownerPair)`. Il certifie seulement qu'une paire ne
peut produire un support pertinent dans la lane considérée.

### 4.4 Caps diagnostiques et absence de reçu résiduel

Aucune porte ne fait mordre `--max-depth` ou `--max-visits`. Les compteurs
`PairId_before_terminal` et `bank_restarts` restent structurellement nuls dans
les scénarios exercés. Avec `--points=30 --bank=4 --max-visits=0`, le binaire
accepte pourtant la configuration, rend le code zéro et publie :

```text
masse_ordonnee entree=870 fermee=0 survivante=870 candidate_pairs=0
budget unknown_to_residual=30 residual_block_mass=870
```

`candidate_pairs` omet donc toute la masse résiduelle. Les deux scalaires ne
sont pas un reçu : aucune plage cible, aucun endpoint, epoch LBVH, digest de
banque, masque hérité, bit de lane ou budget restant n'est conservé. En outre,
le test `visits>cap` intervient après la classification du nœud et les frames
déjà empilées seront encore lues ; ce n'est pas un cap absolu de travail.

Pour le mode borné de diagnostic, l'identité correcte est, par lane :

```text
mass_closed + terminal_alive_ordered + residual_alive_ordered = n*(n-1)
```

À petit `n`, trois bitsets disjoints `closed/terminal/residual` doivent former
la partition complète. Chaque record résiduel porte au moins
`cloud_epoch`, le digest LBVH, l'endpoint, `NodeId` et sa plage, la version et
les slots de banque, les états crédités/réfutés, les lanes mortes et l'état de
reprise. Le replay de tous les records doit reproduire l'ensemble du run non
capé.

La spécification industrielle est plus stricte : elle interdit tout budget
configuré, même non atteint, dans une mesure SLO. Les caps actuels classent ce
binaire comme diagnostic borné. Le chemin produit doit continuer sa frontière
jusqu'à résolution exacte ou échouer sur une ressource réelle ; il ne peut pas
qualifier `warm_e2e` avec un résiduel.

### 4.5 k-NN et domaine d'entrée

La banque est fail-open : ne pas prendre les vrais `M` plus proches ne peut pas
inventer un témoin. En revanche, le reçu affirme une requête k-NN exacte sans
porte comparant ses identités à un tri brut borné, y compris les égalités de
distance et permutations. Cette porte reste nécessaire pour recevoir les
compteurs de travail et la politique canonique.

Le profil amont suppose des positions distinctes ou une agrégation explicite
des doublons. Le commentaire local qui saute `d2=0` ne constitue pas ce
préflight. Le probe doit refuser ou annoncer la politique avant tout verdict
scientifique.

### 4.6 Les planchers anchor restent contournables

La réparation de Claude retire correctement
`PASS_REGULAR_EXPRESSION` des portes cône à planchers. Le même défaut
reste présent dans plusieurs groupes `anchor`. CTest considère le test
réussi lorsque le motif apparaît, même si le processus retourne ensuite un
code non nul. Or `anchor_source` imprime
`verify ... accord=OUI` avant les planchers supports, ancres et prunes.
Une reproduction locale retourne code 3 tout en imprimant le motif vert :

```text
mhgp3v_anchor_source --points=90 --seed=1 --verify \
  --min-supports=100000000
verify exhaustif=14769 produit=14769 accord=OUI
REFUS : plancher de supports 100000000 > 14769
```

Les groupes uniform, pipeline, `eight_clusters`, `smax`, budget
précoce et garde de densité emploient encore ce schéma. La ligne theta est
également imprimée avant `min-theta-active`, et la parité des moteurs
avant les derniers planchers. Retirer ces propriétés et faire foi du code, ou
n'imprimer qu'un marqueur terminal après toutes les validations. Un
expected-code test distinct ne rend pas opérant le plancher positif de la
porte principale.

### 4.7 Le chemin CUDA anchor est statiquement non compilable

`run_anchor_point` exige désormais les deux booléens
`theta_audit` et `density_guard`. Les appels de
`anchor_source_kernel.cu` et
`anchor_source_device_qualification.cpp` omettent le second ;
`run_device` et `anchor_kernel` ne le transportent pas non
plus. nvcc est absent du poste d'audit, mais la discordance de signature est
directe.

Claude doit soit retirer la garde de densité du chemin produit conformément à
l'ablation, soit la propager explicitement dans toute l'ABI hôte/device. Une
cible CUDA opt-in doit compiler avant toute session G4. Ce correctif ne
justifie pas le port du probe endpoint, dont l'ordonnance reste refusée.

Même compilée, la baseline anchor lance un thread scalaire par endpoint et
réserve `222 208` octets de scratch par slot : au défaut de
`16 384` slots, cela représente `3 640 655 872` octets,
soit environ `3,39 Gio`, avant LBVH, sorties et workspaces. Elle
alloue et copie autour de chaque appel et ne chronomètre que le kernel. Le
front collectif devra employer des tâches coopératives, des queues et arènes
persistantes, un layout SoA et un count→scan→fill device ; cette baseline
reste un différentiel, jamais le layout `warm_e2e`.

### 4.8 UB, isométries et mutants manquants

Un build UBSan séparé signale l'overflow signé du LCG dans
`spindle_cone_probe.cpp` et celui du mutant
`narrow-i64`. Le selftest Release imprime néanmoins
`accord=OUI`. Employer un état `uint64_t` pour le générateur ;
le mutant doit définir explicitement son wrap ou attendre une détection
d'overflow, jamais dépendre d'un comportement indéfini.

Le selftest revendique les réflexions mais n'exerce que six permutations
d'axes et des translations. Ajouter les huit choix de signes compatibles avec
le domaine, ou réduire le claim. Enfin, `cone-ignore-inherited` doit
devenir un CTest permanent et la banque doit être comparée à un top-M
exhaustif, égalités comprises. Le diagnostic de refus annonce encore
`[1,64]` alors que `kMaxBank=256`.

## 5. Pentes indépendantes du chemin ponctuel

Une rampe mono-ELF a exécuté `n=500/1 000/2 000`, `seed=3`, `leaf=8`, banques
48 et 96, sur `uniform` et `eight_clusters`. L'ELF Release
`ed1a93f3fee56285af6c0ac8d446aaf8d0d749445c6f6eed1bc34447c34f77a5`
est resté identique avant/après. Les temps ont volontairement été écartés : la
machine à deux vCPU portait des campagnes concurrentes. Les compteurs et leurs
deux pentes log2 successives sont :

| famille / banque | nœuds cibles | tests témoin--nœud | coins | masse survivante | candidats |
| --- | --- | --- | --- | --- | --- |
| `uniform/48` | `1,955 / 1,840` | `1,821 / 1,676` | `1,803 / 1,622` | `1,882 / 1,836` | `1,869 / 1,846` |
| `eight_clusters/48` | `1,875 / 1,881` | `1,837 / 1,666` | `1,871 / 1,586` | `1,888 / 1,790` | `1,871 / 1,783` |
| `uniform/96` | `1,809 / 1,599` | `1,677 / 1,470` | `1,698 / 1,452` | `1,621 / 1,514` | `1,609 / 1,521` |
| `eight_clusters/96` | `1,643 / 1,631` | `1,453 / 1,465` | `1,492 / 1,438` | `1,714 / 1,560` | `1,684 / 1,539` |

Aucune ligne ne ferme deux pentes `<=1,35`. À `n=2 000`, banque 96,
`uniform` dépense `39 207 462` tests témoin--nœud et `83 992 823` tests de
coins ; `eight_clusters` en dépense `25 585 403` et `50 088 926`. Une
extrapolation des seules dernières pentes — diagnostic, jamais théorème —
donne environ neuf et cinq milliards de tests de coins à 50 k. Une G4
mesurerait donc aujourd'hui une ordonnance déjà rouge, sans payload complet.

La banque 256 que Claude mesure après ce pin doit publier les mêmes compteurs.
Une baisse de `candidate_pairs` seule ne suffit pas : augmenter `M` peut réduire
le résiduel tout en augmentant les tests, les octets de frames et les produits
70 bits.

### 5.1 Rampe finale jusqu'à 4 000 sur l'ELF live

Une seconde rampe propre porte exactement l'ELF
`abbc57c5a430e06c63b94631a584b37df83b1dd33280426c5199bfa3d2d5faef`,
`leaf=8`, `seed=3` et aucun bloc résiduel. Les cellules brutes
suivent l'ordre `500 / 1 000 / 2 000 / 4 000` :

| série | visites cible | tests témoin--nœud | candidats | wall CPU |
| --- | --- | --- | --- | --- |
| `uniform/48` | `84 842 / 328 844 / 1 177 056 / 4 341 114` | `2 580 724 / 9 118 007 / 29 143 814 / 99 552 271` | `104 289 / 380 939 / 1 369 645 / 4 920 845` | `0,164 / 0,551 / 2,116 / 7,501 s` |
| `uniform/96` | `79 100 / 277 202 / 840 004 / 2 530 816` | `4 427 268 / 14 156 152 / 39 207 462 / 112 781 793` | `84 120 / 256 619 / 736 638 / 1 969 116` | `0,248 / 1,122 / 2,596 / 7,619 s` |
| `eight_clusters/48` | `76 420 / 280 268 / 1 032 290 / 3 618 768` | `1 820 761 / 6 505 165 / 20 641 781 / 69 819 004` | `85 047 / 311 008 / 1 069 943 / 3 900 545` | `0,301 / 0,909 / 2,196 / 4,389 s` |
| `eight_clusters/96` | `72 752 / 227 280 / 703 982 / 2 134 060` | `3 385 983 / 9 269 896 / 25 585 403 / 78 920 963` | `75 944 / 244 083 / 709 416 / 2 153 592` | `0,179 / 0,519 / 1,504 / 4,879 s` |

Les trois pentes log2 successives sont :

| série | visites cible | tests témoin--nœud | candidats |
| --- | --- | --- | --- |
| `uniform/48` | `1,955 / 1,840 / 1,883` | `1,821 / 1,676 / 1,772` | `1,869 / 1,846 / 1,845` |
| `uniform/96` | `1,809 / 1,599 / 1,591` | `1,677 / 1,470 / 1,524` | `1,609 / 1,521 / 1,419` |
| `eight_clusters/48` | `1,875 / 1,881 / 1,810` | `1,837 / 1,666 / 1,758` | `1,871 / 1,783 / 1,866` |
| `eight_clusters/96` | `1,643 / 1,631 / 1,600` | `1,453 / 1,465 / 1,625` | `1,684 / 1,539 / 1,602` |

Le troisième doublement confirme le NO-GO : aucun compteur dominant ne passe
sous `1,35`. Le wall exclut toujours la génération, le build LBVH et
le tableau de rangs ; il ne qualifie aucun protocole e2e.

### 5.2 Coûts encore invisibles

`corner_evals` ne compte pas les appels `none_mask_of_box`, alors que chacun
calcule `H_max`, trois intervalles de produit vectoriel et plusieurs carrés
larges. Il faut `none_classifier_calls` et ses opérations, séparés des succès
`none_q3/q4`.

Au pin, un `Frame` contient six masques de quatre `u64` et environ 208 octets.
Chaque endpoint réserve 128 frames, soit 26 624 octets de capacité, puis copie
l'état complet aux splits. Il s'agit de 50 000 réservations/libérations et de
copies non comptées à 50 k, pas d'un minorant de trafic DRAM. La banque alloue
en plus un vecteur trié par endpoint. Publier `stack_hwm`, allocations, octets
effectivement copiés et high-water complet est obligatoire avant toute
projection device.

Le chronomètre du probe commence après la génération du nuage et
`tree.build`. Il n'est ni `warm_e2e`, ni même le temps complet du front.

### 5.3 La permutation ne reçoit pas le flux candidat

La porte de permutation compare les fermetures ordonnées. En revanche,
`candidate_pairs` est émis seulement pour `id>a` : la banque
choisie pour une paire non ordonnée est celle de l'endpoint au plus petit
PointId. Une renumérotation peut inverser cette orientation et modifier le
flux réellement remis à l'aval sans modifier l'ensemble ordonné jugé.

L'union des deux directions réduit les candidats de 12 à 40 % :

| série | n=500 : orientation courante → union | n=1 000 : orientation courante → union |
| --- | ---: | ---: |
| `uniform/48` | `104 289 → 91 449` | `380 939 → 314 206` |
| `uniform/96` | `84 120 → 70 651` | `256 619 → 197 996` |
| `eight_clusters/48` | `85 047 → 59 816` | `311 008 → 213 579` |
| `eight_clusters/96` | `75 944 → 52 943` | `244 083 → 147 452` |

`--symmetric` alloue aujourd'hui un bitset quadratique et n'est pas le
chemin produit. La gate correcte compare sous permutation les PairId
**effectivement survivants**. Le self-join collectif doit employer une
orientation géométrique canonique et l'union dédupliquée des deux banques
seulement comme priorité de recherche.

### 5.4 Réfutations NONE repayées

Dans `run_endpoint`, les masques `nq3/nq4` sont consultés selon
`want` avant de calculer `floor`. Si une lane inférieure a déjà
atteint son seuil global, `floor` peut valoir q3 ou q4 alors que
`want` vaut encore q2 ; une réfutation héritée de la lane réellement
utile est ignorée puis repayée dans les descendants. Calculer
`floor` d'abord, puis tester `nq4` si
`floor>=q4` et `nq3` si `floor>=q3`.

Les compteurs `witness_none_q3/q4` sont incrémentés après
`mask_set` sans vérifier que le bit était nouveau. Leurs grands
planchers comptent donc des événements, pas nécessairement des réfutations
distinctes. Séparer transitions nouvelles, hits hérités et tests évités.

## 6. Relèvement recommandé `A × B × C`

La primitive ponctuelle doit rester oracle borné et classifieur terminal. La
route industrielle partage le travail entre des blocs d'endpoints `A`, des
blocs partenaires `B` et une antichaîne de blocs témoins `C`, sans mosaïque de
Delaunay, cellule globale, coface, incidence ou tableau de paires.

Pour `a∈A`, `b∈B`, `z∈C`, garder `H=(b-z)·(z-a)` et
`R=||(b-a)×(z-a)||^2`. Par coordonnée, `H` est affine en `a,b`, concave en
`z` et séparable ; son minimum exact demande huit combinaisons d'extrémités par
axe, pas `8^3` triples 3D. `R` est séparément convexe en `a,b,z`, donc son
maximum est atteint sur un des 512 triples de coins. Une borne d'intervalles
rapide donne d'abord `R_ub`; les 512 triples restent le fallback exact rare et
compté.

Les certificats suffisants sont :

$$H_{\min}>0\text{ et }3H_{\min}^2>R_{\max}\Longrightarrow A\mathbin{\times}B\mathbin{\times}C\subset W_3,$$

$$H_{\min}>0\text{ et }2H_{\min}^2>R_{\max}\Longrightarrow A\mathbin{\times}B\mathbin{\times}C\subset W_4.$$

Un fallback plus serré donne même un `ALL` exact. À `a,z`
fixes, la fibre admissible en `b` est le cône cible convexe ; à
`b,z` fixes, la symétrie des endpoints donne le même cône en
`a` ; à `a,b` fixes, la fibre en `z` est le spindle
convexe. Le prédicat est donc convexe séparément dans les trois variables.
Si les 512 triples de coins sont strictement admis, interpoler successivement
`C`, puis `B`, puis `A` prouve que tout le produit est
admis. La réciproque est immédiate. Tester directement les 512 prédicats de
coins est ainsi une équivalence `ALL` ; l'échec signifie seulement
« pas ALL », jamais `NONE`. Un contrôle brut complémentaire sur
8 000 produits de petites boîtes, dont 15 cas `ALL`, ne trouve aucun
contre-exemple ; la preuve reste la convexité séparée, pas ce tirage.

La comparaison `Hmin/Rmax` demeure la broad phase suffisante et moins
chère. Ses extrema peuvent venir de triples différents : son échec reste
`UNKNOWN`, puis le test direct des coins ou un split prend le relais.

Chaque `PointId` d'un bloc `C` disjoint de `A∪B` crédite alors simultanément
toutes les paires du bloc `A×B`. Les crédits sont saturés aux seuils
`smax-q+1`; un crédit q4 implique q3, sans double compte. Les plages `C`
créditées forment une antichaîne canonique par lane. Scinder `A` ou `B`
partitionne la masse de paires ; scinder `C` partitionne seulement la recherche
de témoins et ne recrédite jamais la masse.

L'état minimal d'un bloc porte les plages `A/B`, les crédits saturés, les
plages témoins déjà acquises et une frontière `C` persistante. Une boîte
témoin qui chevauche `A` ou `B` est divisée jusqu'à exclure les endpoints. Une
ambiguïté conserve le bloc pour la source exacte aval. Dans le profil produit
sans budget, elle ne devient ni un scan global de `PairId`, ni un résultat
partiel.

À `smax=11`, huit témoins ferment q4, neuf ferment q3 et dix ferment
q2. Le front q2 Yao/affine/dual reste séparé : la source q3/q4 peut consommer
ses morts par lane sans attendre le prune simultané des trois lanes. Les blocs
encore vivants passent ensuite à la cutting signée et au census par conflits ;
ils ne retournent ni à `C(n_lens,2)`, ni à
`supports × kept`.

Pour le seul oracle ponctuel, les six masques peuvent déjà être remplacés par
deux états monotones de deux bits par slot — plus haute lane créditée et plus
haute lane encore possible — avec comptes saturés, ou au minimum dimensionnés
à `ceil(M/64)`. Une DFS mutable avec journal de deltas évite de copier six
bitsets à chaque split. Ce compactage ne répare cependant pas les pentes : il
vient après le relèvement bloc, pas à sa place.

### 6.1 Prédicat GPU sans entier général 128 bits

Les produits utiles n'occupent que 70 bits. Après `H>0`, un couple
`(hi:u64,lo:u64)` obtenu par multiplication 64×64 et mot haut suffit pour
`H^2`, `E_2X_2` et les petits coefficients 3/4. Le port device doit utiliser
un lowering deux limbs explicite, comparer bit à bit au CPU sur les frontières
et publier les ambiguïtés ; l'annotation `MHGP_HD` et un `__int128` hôte ne
constituent ni compilation CUDA, ni preuve de coût, ni parité G4.

## 7. Ordre de reprise remis à Claude

1. refuser avant calcul tout `smax` non représentable, toute
   cardinalité générée différente et toute entrée dupliquée non agrégée ;
2. ajouter les trois vérités indépendantes par lane, UBSan, le top-M brut, les
   réflexions et le CTest permanent `cone-ignore-inherited` ;
3. retirer les faux verts `PASS_REGULAR_EXPRESSION` des portes
   `anchor` à planchers et rétablir la compilation CUDA opt-in ;
4. pour le diagnostic borné, recevoir la partition et le replay du résiduel,
   les caps mordants et les octets ; ne jamais employer ce mode pour le SLO ;
5. geler la rampe mono-ELF banque 48/96/256 avec tous les compteurs, y compris
   `NONE`, frames, construction LBVH et flux candidat canonique ;
6. conserver le pointwise comme oracle et implémenter le self-join collectif
   `A×B×C`, avec masse de paires séparée de la frontière témoin ;
7. exiger deux pentes `<=1,35`, des caps absolus de travail et d'octets
   sur `eight_clusters` puis `uniform`, sous le profil produit
   sans budget ;
8. raccorder la cutting, le census par conflits, le fold et le squelette
   `BenchmarkOutputContract-v1` sans matérialiser Delaunay d'ordre
   supérieur, cellules, cofaces ou incidences globales ;
9. porter seulement ce front collectif sur CUDA, recevoir le prédicat deux
   limbs et la parité native, puis appliquer le smoke
   `12 500/25 000/50 000` avant toute campagne G4 officielle.

GCP non utilisé pour ce contre-audit.
