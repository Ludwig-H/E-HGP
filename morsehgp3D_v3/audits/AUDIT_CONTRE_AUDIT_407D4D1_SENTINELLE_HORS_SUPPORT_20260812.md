# Contre-audit `407d4d1` — parallélisme, télémétrie et sentinelle hors support

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document contre-audite le commit
`407d4d1b2745f03a7237080a75daba1c7122ea0a`, les textes non commités qui
l'accompagnent et la route proposée vers 50 000 points. Il ne modifie ni le
prototype ni les oracles. Les mesures ci-dessous sont des diagnostics locaux,
jamais des reçus du SLO.

## 1. Verdict

Le commit ajoute un parallélisme CPU par sous-arbres et corrige dans le code
l'attribution de profondeur du census différé. Il ne réduit ni le nombre de
cellules, ni les `839 582 666` occurrences du point uniforme 50 k, ni la taille
de Source S. Il n'ajoute aucun kernel CUDA, aucun producteur du payload officiel
et aucun échantillon `warm_e2e`. Le facteur `1,44` n'existe que dans le message
de commit; aucun transcript versionné ne le reçoit.

Deux corrections changent immédiatement la prochaine expérience :

1. Au `HEAD`, `--multiplicity --threads>1` publie silencieusement une télémétrie
   partielle avec le code zéro. Le delta non commité de Claude répare
   l'agrégation eager, mais pas la sémantique complète ni le mode différé; la
   section 10 en donne le nouveau pin.
2. Le top-12 global est exact mais n'est pas minimal lorsque le support `U` de
   taille `q` est déjà connu. La sentinelle minimale est le top-`(12-q)` pris
   dans `X minus U`, soit 10/9/8 retours pour q2/q3/q4.

Le chemin de travail reste donc : produire des clés compactes exactement une
fois si le front de Jung le permet, fermer le nombre de clés uniques et les
octets, puis comparer census reçu de l'enveloppe, sentinelle hors support et
census pool-relatif. Paralléliser l'ordonnance CPU exhaustive est orthogonal à
cette réduction.

## 2. Fraîcheur et tests

État observé avant rédaction :

| objet | valeur |
| --- | --- |
| `HEAD` | `407d4d1b2745f03a7237080a75daba1c7122ea0a` |
| CTests configurés | `517` |
| CTests `mhgp3v_centre_cell_` | `53` |
| CTests payload séparés | `4` |
| `CMakeLists.txt` | `3cb2d3ac4ef3e407607283e588c18682604852456029d91673f2dd928e14b87c` |
| source centre-cell | `323a08489ffa4f05d9726c2515dc528483b69386e7347e21605fe9a71f81e6f0` |
| ELF Release centre-cell | `7ed9fcfcedbbce3226388fac9d1088006873b81e14a3cc3fdd315a3af4bbb608` |
| source du juge rationnel | `b39d8d295f5c2edde75d6f88cb2bbf8bffb75440267b69ea677b5d93288d8658` |
| ELF Release du juge | `cfa11f3f4875b5be91b87beebce9eff7117f915aec8399fff829e5915fbe92da` |
| pilote sujet--juge | `3671b7ab53c73f845524aca402f2779a949fc28d1a800a9214d59cef3c4912f6` |
| pilote d'invariance workers | `86f4840a2221833558481127298aab383ad89982715737c7e31200c8cdc96fc1` |

La reconstruction Release réussit. Le filtre
`ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_centre_cell_'`
rend `53/53` en `238,06 s`. Le filtre payload rend séparément `4/4` en
`0,45 s`. Ces sorties ont été observées dans le terminal, sans archivage brut;
elles reçoivent les cas bornés raccordés, pas une latence ni une campagne 50 k.

La porte `mhgp3v_centre_cell_independant_voit_le_mutant` reste vacueuse : le
pilote direct rend le code 2 avec `REFUS : le sujet rend 2`, puis `WILL_FAIL`
convertit ce refus en vert. Elle ne prouve toujours pas un désaccord scientifique
du juge.

## 3. Portée réelle du parallélisme

Le maître descend jusqu'à `harvest_depth`, copie chaque cellule et sa liste de
candidats dans une tâche, puis les workers explorent des sous-arbres disjoints.
Les compteurs additifs sont sommés et les high-water sont fusionnés par maximum.
Les trois portes normales comparent le stdout hors ligne `cloud=` entre
1/2/5 workers; la porte différée ne compare qu'une sélection de préfixes.

Ce vert est utile mais borné :

- les portes normales ne demandent ni `--judge` ni `--emit-identities`;
- la porte différée ne compare ni les identités complètes ni l'histogramme de
  profondeur qu'a précisément réparé le commit;
- aucun plancher n'atteste un nombre minimal de tâches ou de workers actifs;
- le reçu n'imprime ni `harvest_depth`, ni tâches récoltées, ni workers actifs,
  ni octets copiés, ni high-water des listes par worker;
- k1 parallèle, multiplicité et plusieurs profondeurs de récolte ne sont pas
  raccordés aux CTests.

L'histogramme différé n'est donc plus un défaut live du code : `BatchCell`
transporte maintenant sa profondeur et restaure `cur_depth` avant le census.
Sa réception ciblée reste ouverte.

## 4. Défaut reproductible au `HEAD` : `multiplicity` fois `threads`

Le moteur maître reçoit `track_multiplicity` et `batch_depth`. Les moteurs
workers ne reçoivent pas ces champs, et leurs tables de multiplicité ne sont
pas fusionnées vers le maître. Les supports et les lifts restent identiques,
mais l'histogramme ne couvre plus le run.

Reproduction sur les octets ci-dessus :

```bash
build/v3/mhgp3v_centre_cell --points=40 --smax=4 --family=uniform --seed=11 --multiplicity --threads=1
build/v3/mhgp3v_centre_cell --points=40 --smax=4 --family=uniform --seed=11 --multiplicity --threads=2
```

| workers demandés | `multiplicite_total_occurrences` | `lifts_built` | `ecart` |
| ---: | ---: | ---: | ---: |
| 1 | `22 535` | `22 543` | `8` |
| 2 | `7 012` | `22 543` | `15 531` |

Le petit écart séquentiel correspond aux huit lifts dégénérés déjà omis par
cet instrument historique. Le saut à `15 531` est une perte parallèle. Le code
rend pourtant zéro dans les deux cas. Une mesure `SupportKey_unique` parallèle
fondée sur cette voie serait donc fausse.

Réparation minimale proposée à Claude, sans préjuger de son choix : refuser
explicitement `multiplicity && threads>1` jusqu'à ce que chaque worker reçoive
la configuration, que les tables soient fusionnées sans collision d'identité
de lot et qu'une porte ferme les deux ledgers. Le vrai chemin produit ne doit
de toute façon pas employer ces `std::map`; il lui faut un count/radix/RLE avec
capacités et octets reçus.

## 5. Sentinelle exacte top-`(12-q)` dans `X minus U`

Soit `U` un support propre positif déjà validé, `q=|U|`, `beta` son rayon carré
et `p` le nombre de points strictement intérieurs. Poser `t=12-q`. La primitive
retourne `t` `PointId` distincts parmi `X minus U`, vrais plus proches voisins
du centre, ex aequo arbitraires, et certifie que leur distance maximale `delta`
ne dépasse aucune distance omise. Si `|X minus U|<t`, elle scanne tout
`X minus U`.

### Théorème

- Si `delta>beta`, tout point de `X minus U` à distance au plus `beta` est
  retourné. L'intérieur `I` et l'extra-shell `H=E minus U` calculés dans les
  retours sont donc globaux et complets. Le fast path publie seulement si
  `H` est vide, c'est-à-dire `E=U`.
- Si `delta<beta`, les `t` retours sont intérieurs. Alors `p>=12-q`, donc
  `p+q>=12`; le support est hors fenêtre `smax=11`.
- Si `delta=beta`, tous les intérieurs sont retournés et au moins un retour
  appartient à l'extra-shell, puisque les identifiants de `U` sont exclus.
  De plus `p<=t-1`, donc `p+q<=11`. La boule est pertinente mais non régulière;
  elle rejoint le quotient/range-report ou échoue fermée.

La preuve est le certificat du `t`-ième ordre : un point omis ne peut avoir une
distance strictement inférieure à `delta`. L'exclusion se fait par `PointId`,
jamais par coordonnées.

### Minimalité

Une sentinelle de taille `t-1` ne distingue pas trois nuages ayant les mêmes
`t-1` premiers retours : une boule régulière avec `t-1` intérieurs, la même
avec un `t`-ième intérieur, et la même avec un contact extérieur au support.
Elle ne distingue donc ni `p+q=11` de `p+q=12`, ni `E=U` d'une extra-shell.
Le top-`(12-q)` hors support est minimal parmi les sentinelles de profondeur
fixe. Le top-12 global reste sûr, mais son claim de minimalité est rétracté.

Pour une requête par `SupportKey`, employer son propre couple `(q,U)`. Si un
RLE `GeometricBallKey` précède la requête, choisir un support canonique `U_star`
d'arité minimale `q_min` et interroger `12-q_min` voisins dans
`X minus U_star`. Employer `q_max` peut rejeter une boule encore pertinente par
son support minimal; exclure l'union des supports peut masquer l'extra-shell.

Le LBVH garde la même borne AABB exacte. Les feuilles dont le `PointId`
appartient à `U` sont simplement ignorées au remplissage du petit top-k. Le
nombre de visites n'a pas de monotonie universelle et reste à mesurer par
arité.

## 6. Correction du layout compact

Les `6 331 693 908` octets annoncés pour les occurrences q2/q3/q4 sont
arithmétiquement corrects : q2, q3 et q4 emploient respectivement 4, 8 et
8 octets. Mais ce layout n'encode pas directement des `PointId` ABI 32 bits
ou 64 bits arbitraires. Il exige un `DensePointIndex:u16` et une bijection
résidente immuable `DensePointIndex in [0,n) <-> PointId`, liée à
`cloud_epoch`; à 50 000 points, les clés chaudes tiennent alors dans
`u32/u64/u64`. L'ordre canonique de sortie est défini après remap par les vrais
`PointId`, sauf preuve reçue que l'affectation dense préserve cet ordre.

La table inverse, les listes de cellules, le workspace radix et les buffers de
sortie restent hors des `6,33 Go`; le double buffer des seules clés vaut
`12,66 Go`. Les quatre/six/huit passes radix du modèle publié lisent et écrivent
`86,938208192 Go` pour ces seules occurrences. Ces nombres prouvent que ce
stream particulier tient dans les 96 Go de la G4, pas que le high-water complet
tient ni que son trafic passe sous une seconde. Une porte doit permuter les
lignes d'entrée, employer des `PointId` durables arbitraires et distincts pour
des coordonnées dupliquées, puis vérifier digest, époque et bijection.

## 7. Ordonnance unique à comparer

Les documents antérieurs mélangeaient un second RLE obligatoire et un fast
path sans RLE de sphères. Les deux variantes exactes sont :

1. **BallKey-first** : géométrie et owner après le premier RLE, puis second RLE
   par boule; choisir `(q_min,U_star)` et exécuter une seule sentinelle hors
   support par boule, sauf si le producteur livre déjà un census reçu.
2. **SupportKey-first** : utiliser d'abord le census reçu du producteur; à
   défaut, exécuter top-`(12-q)` dans `X minus U` par support. Publier
   directement uniquement lorsque `delta>beta` et `H` est vide; router tout
   `H` non vide, toute égalité et toute demande Gamma vers la side queue
   `GeometricBallKey`.

L'égalité n'est donc pas la seule cause de side queue. Avec `delta>beta`, une
extra-shell est déjà connue complètement et interdit le fast path régulier.
Une comparaison A/B doit compter requêtes, slots retournés, visites LBVH,
`BallKey` uniques, octets radix et high-water de la side queue.

### Census gratuit de l'enveloppe top-9

L'enveloppe affine proposée pour q3/q4 peut rendre la sentinelle terminale
inutile sur sa branche certifiée. Son top-9 est pris dans `X minus {a,b}` : les
deux extrémités de l'ancre ne consomment jamais les neuf niveaux. Au cutoff,
toutes les lignes au niveau du neuvième ordre restent actives; choisir neuf
identifiants arbitraires dans un tie serait incomplet.

Au centre q3 intrinsèque ou à l'intersection q4, l'enveloppe connaît les sites
`always_inside`, toutes les lignes strictement au-dessus de zéro et toutes les
lignes égales à zéro; pour chaque ligne omise, son certificat donne une borne
strictement sous le cutoff. Elle possède donc déjà exactement `(I,E)` du
support. Le top-`(12-q)` devient alors oracle différentiel ou fallback, pas une
requête obligatoire sur quelque vingt-deux millions de supports. La porte est
l'identité `(SupportKey,I,E)` entre `census_from_envelope`, sentinelle hors
support et oracle borné, avec le ledger
`envelope_certified + knn_fallback + plateau = supports` et zéro kNN sur la
branche certifiée. La lane q2 doit réutiliser de même le census de son
producteur lorsqu'il est reçu.

### Owner génératif exact-once

Sous un front complet et une partition half-open des patches, chaque support
propre peut aussi être émis une seule fois, avant tout RLE. Pour q3, choisir
l'arête maximale canonique : parmi les arêtes de longueur maximale, l'owner est
la plus petite `PairId`. Pour q4, appliquer la même règle aux six arêtes, puis
traiter les deux carriers de l'intersection comme un ensemble non ordonné. Le
centre appartient à un unique patch half-open. Q2 possède déjà son self-join
canonique.

Ainsi, après rejet des ancres non maximales et des patches non owners, le ledger
visé est `occurrences=unique_support_keys` avant plateaux, au lieu d'un ratio
moyen proche de 39. Sans la règle canonique, le plafond structurel est trois
émissions en q3 et six en q4. Cette propriété doit être reçue contre le
catalogue borné, les permutations de `PointId`, les centres sur frontières et
deux mutants : suppression du tie-break entre arêtes maximales et choix de
l'arête minimale. Elle ne construit aucune mosaïque d'ordre supérieur.

## 8. Gates avant un nouveau run G4

Ordre proposé :

1. fermer toute la sémantique `multiplicity`, y compris dégénérés, rang et mode
   différé, ou refuser les combinaisons non reçues;
2. produire `SupportKey_unique` par arité avec fermeture exacte
   `sum multiplicites=occurrences`, puis viser l'owner génératif
   `occurrences=unique_support_keys`; recevoir octets et HWM sur
   `12 500/25 000/50 000`, au minimum `uniform` et `eight_clusters`;
3. comparer au census exhaustif borné le census de l'enveloppe et les fallbacks
   top-10/top-9/top-8 hors support sur
   `delta<beta`, `delta=beta`, `delta>beta`, ties et `n<12`;
4. tuer les mutants `t-1`, support compté dans le top-k, exclusion par
   coordonnées, égalité acceptée directement, `q_max` et union des supports;
5. comparer census producteur, sentinelle hors support, census pool-relatif et
   BallKey-first sur visites, replis, octets et side queue;
6. fermer `BallActivation`, gateways, fold, dix forêts, verticales et
   `BenchmarkOutputContract-v1`; seul ce pipeline mesure `warm_e2e`.

Le front de Jung reste une expérience parallèle : ses constantes de Poisson et
sa couverture déterministe ont été revérifiées, mais son producteur et son
extension doivent fermer séparément `W_front` et `W_extend`. Il ne remplace pas
la gate `SupportKey_unique`.

## 9. Fermeture GCP demandée pendant l'audit

Une seule VM G4 active a été trouvée :
`devpod-gpu-exploration/europe-west4-a/ehgp-blackwell-spot`, type
`g4-standard-48`, label `project=e-hgp`, génération
`2026-08-12T11:50:53.892-07:00`. À la demande explicite de l'utilisateur, elle
a été arrêtée uniquement par `./gcp-migration/stop_and_verify.sh --yes` avec
verrou de génération. Le script a certifié `TERMINATED`; son inventaire final
n'a trouvé aucune autre VM `project=e-hgp` active. Aucun benchmark GPU n'a été
lancé par cet audit.

## 10. Delta non commité de Claude : `UniqueKeyReceipt-v1`

Après le pin du `HEAD`, Claude a modifié le prototype pendant le contre-audit.
Ce code n'est pas attribué à l'auditeur et n'a pas été édité par lui. Snapshot
relu :

| objet | valeur |
| --- | --- |
| source `centre_cell_source.cpp` | `72e490932e5553796de0f3322f8d43d2ddfb7c5d720e04e4a0d5c81578aa862e` |
| diff contre `407d4d1` | `+205/-8` |
| ELF Release reconstruit | `772069ff1891fb0f36a2aa2d4851c42d22e41cbdd1b19b5295c0d5a269c16dc8` |
| CTests configurés | `517`, aucune nouvelle porte pour ces options |

Après reconstruction, les `53/53` CTests centre-cell passent en `271,21 s`;
les quatre workers passent isolément en `70,93 s`. Ils n'invoquent ni
`--unique-keys` ni `--multiplicity` et ne couvrent donc pas les défauts qui
suivent.

La partie utile est réelle mais étroite. Le flux `--unique-keys` contient bien
une clé par occurrence avant lift, séparée par arité; son union triée donne des
comptes identiques en eager/différé et avec 1/2/5 workers sur les petits cas
rejoués. À `terrain,n=400,smax=11`, il rend exactement `1 768 790` occurrences,
`246 263` clés uniques et un facteur `7,1825`; le juge borné reste d'accord à
`uniform,n=32,smax=7` avec 1/5 workers et en différé. Cette mesure ferme le
compte sur ces cas, pas l'identité indépendante de chaque clé ni le profil
50 k. La fermeture publiée n'existe qu'au total : sans
`occurrences_recorded_q`, une erreur compensée entre q2/q3/q4 resterait verte;
le ledger doit fermer chaque arité séparément.

### 10.1 Multiplicité encore fausse

La fusion des maps workers rend désormais l'eager invariant entre 1/2/5 sur le
cas de la section 4; le balayage `batch_depth=1..5` garde aussi lots, clés et
facteurs identiques. Elle ne rend pas l'instrument exact : les huit occurrences
dégénérées restent absentes, donc `22535` contre `22543`, avec code zéro. En
différé, `note_occurrence` n'est jamais appelé : deux workers rendent
`multiplicite_total_occurrences=0`, `ecart=11914`, des facteurs `-nan` et le code
zéro. Enfin, l'issue est notée `pertinent` avant le census; un rejet de rang
ultérieur ne la reclasse jamais.

Claude doit soit refuser `multiplicity` en différé, soit instrumenter exactement
le même flux, classer dégénéré/owner/positivité/rang/pertinent après la décision
finale et rendre tout écart non nul fatal. Une gate couvre eager/différé,
1/2/5 workers, plusieurs `batch_depth`, une fixture dégénérée et une fixture de
rang; aucun NaN n'est publiable.

### 10.2 Le cap n'est ni global ni mémoire

Le quota est divisé également entre maître et workers. Il peut donc refuser un
shard chargé alors que la somme tient. Reproduction déterministe : à
`terrain,n=100,smax=11,seed=5`, les `243833` clés u64 occupent logiquement
`1950664` octets, donc moins de 2 MiB. `--unique-keys-cap-mb=2` passe avec un
worker mais rend code 3 avec deux ou cinq workers, eager comme différé. Pire,
à `uniform,n=100,smax=4,cap=1`, dix répétitions identiques avec deux workers
alternent les codes `0/3`, car l'ordonnance atomique répartit différemment les
tâches. Le verdict de ressource dépend donc du découpage et du scheduling.
Un cas encore plus net demande 256 workers pour seulement `6622` clés
(`52976` octets) et refuse sous 1 MiB, parce que le quota est divisé par
`threads+1` plutôt que par les moteurs actifs ou géré globalement.
Le champ `flux` du reçu dépend lui aussi de l'ordonnancement : dix répétitions
de `uniform,n=40,smax=4,seed=3,threads=5` donnent neuf fois `18` et une fois
`15`, malgré des comptes de clés identiques. Il doit sortir de l'identité
canonique ou être remplacé par un sharding déterministe.

Ce quota ne borne que `size()*8`. Il ignore les `capacity()` des vecteurs et
leurs reallocations, le vecteur `runs` de huit octets par clé unique, les tas,
les positions, l'allocateur et le reste du moteur. `cap_mb=0` désactive toute
borne. Le tri crée jusqu'à trois threads par moteur, soit 771 au maximum CLI;
les échecs `bad_alloc` ou de création de thread ne sont pas normalisés. Une
création de thread échouant après des créations réussies peut même dérouler des
objets encore joinables et terminer le processus. Le cap passé sans
`--unique-keys` est silencieusement ignoré; zéro doit être refusé ou annoncé
explicitement comme `unbounded`.

Ce n'est pas davantage un preflight transactionnel. À `n=160,smax=4,cap=1`, le
moteur calcule les `141621` occurrences et imprime déjà l'en-tête et les
compteurs `CentreCellReceipt-v3`, puis refuse avant la sous-section unique. Le
garde `n<=65535` arrive après génération/dédoublonnage du nuage. Le reçu doit
donc annoncer un HWM global réellement mesuré et un refus déterministe, ou se
nommer explicitement diagnostic sans claim de capacité.

### 10.3 Layout et ordre différents de la proposition

Toutes les arités sont stockées dans `unsigned long long`. Q2 paie donc huit
octets, pas quatre : sur `n=40`, ses `4400` occurrences annoncent
`octets_flux=35200` au lieu de `17600`. Extrapoler ce collecteur aux masses
gelées donne `6 716 661 328` octets, pas le layout `u32/u64/u64` de
`6 331 693 908` octets. Le chiffre documentaire reste un modèle de layout
proposé, jamais une mesure de ce code.

En outre, `v << (16*i)` place le plus petit identifiant dans les bits faibles.
Le tri entier ordonne donc q4 par `(d,c,b,a)`, pas lexicographiquement par
`(a,b,c,d)`; les apex d'un préfixe de face `(a,b,c)` ne sont pas contigus. Cela
n'empêche pas le simple comptage d'égalité, mais contredit la factorisation q4
annoncée. Il faut inverser l'empaquetage ou recevoir un comparateur/radix adapté,
avec une porte de contiguïté du préfixe.

Enfin, les indices sont les positions implicites du nuage généré. Le reçu ne
lie ni bijection `DensePointIndex`--`PointId`, ni digest, ni `cloud_epoch`; il ne
qualifie donc pas encore le layout ABI décrit en section 6.

La statistique `p95` doit également annoncer sa convention : l'index actuel
`floor(0,95 N)` diffère du nearest-rank lorsque `N` est multiple de vingt.

### 10.4 Portée industrielle

`std::sort` comparatif sur les centaines de millions de clés puis fusion hôte
est un instrument ponctuel pour fermer `SupportKey_unique`. Ce n'est ni le
radix CUDA, ni l'owner génératif exact-once, ni un chemin vers `warm_e2e<1 s`.
La prochaine livraison doit séparer dans son reçu temps de génération, collecte,
tri et merge, ajouter les CTests CLI/cap/identités, puis mesurer le profil 50 k
avec source et ELF immuables. Le front exact-once et le census d'enveloppe des
sections 7--8 restent la réduction structurelle prioritaire.

Les gates minimales du collecteur sont : oracle map indépendant et digest par
arité; eager/différé fois 1/2/5/256 workers et plusieurs profondeurs; pack aux
indices 0/65534, permutations, padding et mutant 15 bits; seuils globaux
cap-1/cap/cap+1 avec HWM RSS; mutant drop/double-push sans aucun préfixe de reçu;
vingt répétitions byte-for-byte hors télémétrie physique déclarée; permutation
du stockage avec mêmes `PointId`, mapping et `cloud_epoch`.
