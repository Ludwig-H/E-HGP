# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex, avec relecture critique du brouillon de l'autre auditeur
- **Pin fonctionnel exécuté :** `10c46c87bbda13a3fda697c9dedb94fead273faa` ; les sources produit sont identiques dans `b79c001b` et `a0d13420`
- **Corrections relues statiquement, non exécutées par l'auditeur :** `6e8a6aba69b76dda936332abb7f8b1ef1b72f79f`
- **Tip de réponse relu :** `312034cec60aae85abf51dbf8eba83632ae37e28`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_adaptatif`](../receipts/campagne_g4_v5_20260827_adaptatif/RECU.txt), source `8f95df2effd07ffa7a8aa7cf7fe79be1be9c7b2c`, publié par `a0d134205b5b4364ada1e6c12995f979f59698b4`
- **Worktree observé hors verdict :** nouvelle sonde `bench/rect_probe.cpp` et son entrée CMake, postérieures à `312034ce` ; le probe racine `.codex_fold_contract_probe.cpp` appartient à un autre auditeur
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; le nouveau reçu affirme l'arrêt ciblé, mais ne conserve pas la sortie de certification correspondante

## Verdict

La v5 reste **orange et avance dans la bonne direction**. Le nouveau reçu brut
ferme empiriquement les deux anciens OOM à 50 000 points au pin `8f95df2e` :
les quatre exécutions `--gpu` terminent avec le code 0 et leurs
`digest_balls` **et** `digest_all` sont identiques aux sorties CPU appariées.
La porte lane brute présente quatre cas Q3 et quatre cas Q4 non vides sans
désaccord, et le mutant Q3 device est tué avec le code 4.

La formulation correcte est toutefois **égalité bornée observée au pin
`8f95df2e`**, pas « lane exacte » en général. La campagne est partielle : 24
runs sur 25, adaptatif `scanline_single_pass` absent, journal de session perdu,
trap non exécuté et aucun validateur final exécuté. Le reçu ne qualifie pas le
précomptage Q4, le layout SoA réduit ni le routage hôte direct, tous postérieurs
à sa source.

La factorisation `10c46c87` est utile et **a été exécutée sur une archive
propre** : 156/156 portes Release passent, dont 7 oracles, puis 9/9 portes
ciblées passent sous ASan+UBSan. Ces neuf portes, dont
`mhgp5_parallel_exception`, n'ont produit ni échec ni diagnostic de sanitizer ;
cela ne prouve pas l'absence générale de défaut de concurrence. Les sorties de
routes mixtes ont été observées dans des CTests enregistrés, mais leurs
assertions au pin exécuté ne contractualisent pas encore la non-vacuité. Le
commit `6e8a6aba` ferme une partie des trois P1, sans encore les solder.

## Revue statique de `6e8a6aba` — non exécuté par l'auditeur

Les corrections vont dans le bon sens : sites/seeds/paires sont précomptés
avant écriture, une ancre dépassant les caps repart vers le corps de production,
le lot est vidé avant l'ajout qui franchirait un seuil, les routes reçoivent des
prédicats de non-vacuité et le validateur Q4 refuse désormais plusieurs
émissions incohérentes. Les deux anciennes erreurs de fixture dues au passage
de `minimal4()` à quatre sites ont déjà été corrigées dans le worktree ; elles
ne sont donc pas retenues.

Un bloquant statique subsiste dans le commit : CMake enregistre
`mhgp5_q3_lane_device_route_mutant` avec
`--inject=route-ignore-threshold` et attend le code 4, mais
`q3_lane_device_gate.cu` ne parse pas `--inject` et n'appelle pas
`mutants_enable`. Cette porte rendra le code 2 sur argument inconnu, pas le
verdict mutant attendu. Aligner son activation sur la porte Q4.

Les verrous encore visibles sont les suivants :

- `validate_q4_batch_view` déréférence encore les tableaux structuraux sans
  rejeter leurs pointeurs nuls. `validate_q4_results_view`, appelée seule,
  suppose l'ancre du seed valide ; surtout, un seed vivant avec une complétion
  admissible mais `stages={}` et `emits={}` passe encore. Recalculer, avec
  arithmétique vérifiée, le nombre exact de `y` admissibles de chaque seed
  vivant. L'option publique `emit_eq=false` ne doit pas transformer le
  validateur d'autorité en bypass pour un mutant.
- La recherche `y in lens` est linéaire dans toute la lentille pour chaque
  émission, soit potentiellement `O(n_emits * lens_count)` dans le chemin
  chaud. Les indices de lentille sont produits croissants : verrouiller cet
  invariant une fois puis utiliser une recherche logarithmique ou un parcours
  fusionné.
- Q4 caste encore `i` en `u32` lors de la construction de `lens_idx` avant le
  rejet `nc > UINT32_MAX`. Q3 et Q4 testent encore
  `vector.size() + ajout > cap`, addition susceptible de déborder, et les caps
  publics peuvent laisser les tailles cumulées sites/lentilles/ancres/seeds
  franchir le domaine `u32` avant les casts. Comparer à `cap - courant` et
  borner chaque domaine d'index avant matérialisation.
- Les CTests nommés `ancre_trop_grande` n'exigent pas
  `anchors_oversized > 0` : `--expect-route=device` peut réussir sans exercer
  le repli. Ajouter un plancher explicite, idéalement une fixture par cap
  sites/seeds/paires.
- `--gpu-min-sites` refuse maintenant zéro et les négatifs, mais `std::atoll`
  accepte encore un suffixe (`1x`) et ne certifie ni consommation complète ni
  overflow. Employer un parseur intégral borné à `size_t` et recevoir les cas
  zéro, négatif, suffixe et dépassement avec le code 2.
- `backend_override` est un marqueur utile mais pas encore une autorité : un
  override vide peut rendre `complete_regular`, les callbacks restent
  publiables, et la sortie imprime à la fois `authority=status_terminal` et
  `backend=override_experimental (... non autoritaire)`. Définir une autorité
  machine-readable unique, orthogonale au statut, la propager aux callbacks et
  la faire vérifier par les campagnes.

Ce commit n'a pas été compilé ni exécuté par cet audit, conformément à la
consigne de ne pas lancer de tests. Il ne modifie donc pas encore le verdict du
pin exécuté.

## Résultats actuels

| Périmètre | Résultat établi | Portée exacte |
|---|---|---|
| CPU `10c46c87` | Release `gate` : **156/156**, dont 7 portes `oracle` ; ASan+UBSan ciblé : **9/9** | archive propre de `b79c001b`, dont les sources produit sont identiques à `10c46c87` et `a0d13420` |
| Corrections `6e8a6aba` | Claude déclare 47 portes CPU et 13 scénarios de selftest protocolaire | déclaration de `312034ce`, sans log/reçu versionné et non rejouée par l'auditeur ; la porte CUDA Q3 mutante reste statiquement cassée |
| Routage CPU au pin exécuté | CTests Q3/Q4 mixtes et Q4 tout-hôte : vecteurs post-RLE et compteurs explicitement comparés conformes, aucun mismatch | sorties observées de CTests enregistrés ; leurs assertions au pin ne prouvent pas encore la non-vacuité des deux routes |
| G4 source `8f95df2e` | témoin device code 0, lane Q3/Q4 sans désaccord brut, mutant code 4, quatre contrats GPU 50 k code 0 | artefacts bruts cohérents, mais campagne partielle et non validée automatiquement |
| Adaptatif source `8f95df2e` | `eight_clusters` exerce les deux routes et conserve les deux digests | `scanline_single_pass` absent ; ancien chemin hôte matérialisé |
| Documentation et registre | 210 Markdown actifs et 20 phases passent leurs vérificateurs | les vérificateurs ne détectent pas les claims GPU ni les dérives sémantiques listées plus bas |

### Temps de bout en bout sur 50 000 points

| Famille | CPU 48 fils | GPU 48 fils | Surcoût GPU | Pic RSS CPU / GPU | Digests appariés |
|---|---:|---:|---:|---:|---|
| `uniform` | 78 s | 89 s | +14 % | 19,3 / 19,0 Go | `balls` et `all` |
| `terrain` | 23 s | 44 s | +91 % | 3,68 / 5,31 Go | `balls` et `all` |
| `scanline_single_pass` | 38 s | 96 s | +153 % | 3,10 / 7,25 Go | `balls` et `all` |
| `eight_clusters` | 246 s | 718 s | +192 % | 17,6 / 17,5 Go | `balls` et `all` |

L'adaptatif `eight_clusters` à `min_sites=256` prend **713 s** et 18,2 Go.
Les ratios 70,7 % en Q3 et 31,6 % en Q4 portent sur les ancres avec seed qui
auraient été matérialisées par le chemin tout-device de `8f95df2e`, pas sur
toutes les ancres `|A| * |B|`. Parmi les seeds, 99,1 % en Q3 et 91,3 % en Q4
partent au device ; le seuil par taille de cover laisse donc la quasi-totalité
de ce travail coûteux sur le GPU.

### Pourquoi le GPU est plus lent dans ce reçu

Le pin mesuré fait encore sur CPU la descente WSPD, les covers par ancre,
l'énumération des seeds, les formes et la matérialisation des lots, puis copie
les SoA et résultats. Sur `eight_clusters`, la route tout-device traverse
18,22 milliards de seeds Q3 et 1,49 milliard de seeds Q4. Le CPU de production
peut tuer tôt ces seeds sans fabriquer les enregistrements intermédiaires ; le
chemin GPU paie cette préparation avant que le kernel ne puisse aider.

Les chiffres étayent fortement ce diagnostic : la génération passe de 189 s
CPU à 659 s GPU, tandis que `kernel_ms=111196,5` est un cumul d'événements de
48 exécuteurs, pas un mur GPU. Ils ne suffisent toutefois pas à isoler une
cause unique : les fenêtres Q4 incluent aussi transferts, synchronisations et
compaction hôte. Écrire « matérialisation et orchestration probablement
dominantes » jusqu'à disposer de murs séparés préparation/H2D/kernel/D2H.

## Requalification de `10c46c87`

Le changement ferme bien un sous-problème : il n'existe plus deux lots hôte et
device simultanément par ouvrier. Une ancre routée hôte passe immédiatement par
le même corps sémantique que la production ; seules les ancres routées device
sont matérialisées. Cette factorisation réduit la duplication et donne une
bonne base pour comparer les backends.

Quatre limites doivent être explicites :

- Le reçu adaptatif à 713 s mesure l'ancien second lot hôte de `8f95df2e`, pas
  la route directe actuelle. Fermer seulement le **tout-device matérialisé au
  pin mesuré** comme voie de gain ; mesurer `10c46c87` avant de conclure sur
  l'adaptatif courant.
- Le callback générique de `generate_q3_batched_with` ou
  `generate_q4_batched_with` ne voit désormais que les ancres device ; la route
  hôte le contourne au profit du corps de production. Documenter cette
  sémantique de backend, surtout pour les callbacks de mesure ou mutants.
- Une émission hôte immédiate peut dépasser les émissions d'un lot device déjà
  en attente. Seule la sortie post-RLE reste canonique en routage mixte ; ne pas
  promettre l'ordre brut général.
- `anchors_host` et `anchors_device` ne décrivent plus la même population. La
  route hôte compte aussi les ancres sans seed Q3 et, en Q4, celles ensuite
  tuées par W4 ou sans seed ; la route device ne compte que les ancres
  matérialisées avec seeds. Pour prouver un mix, exiger `seeds_host > 0`,
  `seeds_device > 0` et des lancements device, ou séparer compteurs « routés »
  et « traités ». `host_flushes` est maintenant un champ mort toujours nul.

### P1 preuve au pin exécuté — les portes de routage peuvent rester vertes à vide

Les sorties observées des CTests enregistrés confirment que le pin emprunte réellement les deux
branches : Q3 `uniform n=1200, min_sites=256` compte 5 204 ancres device et
117 228 hôte, avec 474 887 et 2 899 077 seeds ; Q4 compte 20 920 et 106 341
ancres, avec 4 605 159 et 1 366 207 seeds. Le probe Q4 tout-hôte donne zéro
ancre/seeds device et 34 876 ancres, 508 979 seeds hôte. Les vecteurs et tous
les compteurs comparés concordent.

Ces observations ne sont pas encore verrouillées au pin exécuté. Les portes `route_256`
n'exigent pas de travail hôte ; leur `min_flushes=1` ne prouve qu'un vidage
device, car `host_flushes` n'a plus de producteur. La porte Q4 « tout hôte »
accepte `min_flushes=0` sans imposer `anchors_device == 0` ni un travail hôte
non nul, et Q3 n'a pas de porte tout-hôte. Le commit `6e8a6aba`
ajoute les assertions de routes et le mutant demandé ; il restera à corriger
l'activation du mutant CUDA Q3 décrite plus haut, puis à recevoir ces portes.

Enfin, la route hôte et la production appellent maintenant exactement
`scan_anchor_q3` et `process_anchor_q4`. Leur égalité prouve l'orchestration,
pas indépendamment la sémantique mathématique. Les shaped gates, les oracles et
les campagnes différentielles v4/v5 restent les autorités distinctes.

## P1 — capacité device avant matérialisation

Au pin exécuté, le chemin hôte direct réduit la résidence, mais une ancre
device complète est encore copiée avant le test des seuils. Le commit
`6e8a6aba` ferme l'essentiel de ce défaut par préflight, repli
hôte et préflush. Il reste à rendre les additions non débordantes et à borner
les domaines cumulés `u32` avant tout cast, notamment `lens_idx` en Q4, comme
détaillé dans la revue statique.

Fermeture recommandée :

1. finir les gardes de représentation et d'addition avant toute écriture ;
2. contractualiser par des planchers non vacuables les trois replis
   sites/seeds/paires ;
3. appliquer le même contrat aux buffers, à la grille et aux sorties de la
   future lane par rectangle ;
4. conserver une fixture de frontière sans allocation géante lorsque les tests
   seront de nouveau autorisés.

## P1 — résultats Q4 et autorité

Au pin exécuté, `validate_q4_results_view` ne verrouille ni les émissions ni
les complétions. Le commit `6e8a6aba` ajoute la somme vérifiée,
`st.emit == n_emits`, les rejets `y=x`, `y=skip`, `y` hors lentille et plusieurs
pointeurs nuls. Il ne recalcule toutefois toujours pas le nombre exact de
complétions admissibles, n'est pas autonome face à une structure invalide et
conserve un bypass `emit_eq=false`. La revue statique ci-dessus est l'état à
fermer ; ne pas dupliquer une seconde liste de mutants ici.

À la frontière produit, les `LaneOverride` publics peuvent ne rien émettre et
laisser malgré tout le pipeline atteindre un statut terminal. Décision pour la
question de Claude : **pas de refus dur du seul fait qu'un override existe**.
Conserver `PipelineStatus::kCompleteRegular` pour l'exécution transactionnelle,
mais ajouter un axe distinct, par exemple
`ResultAuthority::kCpuReference | kExperimentalOverride`. Un override vide peut
donc terminer expérimentalement, mais jamais s'annoncer
`authority=status_terminal/reference`. Propager cette autorité à `on_forest`
ou employer un callback expérimental distinct ; le validateur de campagne
compare les sorties sans les promouvoir en référence.

## P1 preuve — rendre la prochaine campagne terminale

Le nouveau reçu compense manuellement plusieurs faiblesses : les deux digests
concordent sur les quatre couples bruts, et l'adaptatif `eight_clusters` a des
seeds des deux côtés. `6e8a6aba` améliore statiquement le protocole : les deux
digests sont comparés, l'adaptatif impose `min_sites=256` et quatre populations
de seeds non vides, et la phase GPU dépend de `gpu_lane=0` et `gpu_mutant=4`.
Les points suivants restent ouverts :

- le validateur ne lie pas le nom du run à la famille, `n=50000`, `s=8`,
  `smax=11`, la seed ni le nombre de fils annoncés dans le corps. Le faux pilote
  réutilise les mêmes digests pour toutes les familles ; un pilote ignorant
  `--family` peut donc encore produire quatre couples appariés artificiels ;
- pour un contrat `min_sites=1`, `lancements > 0` est agrégé et aucun travail
  device n'est exigé séparément en Q3 et Q4. Avec le nouveau repli oversized,
  une lane entièrement hôte peut passer. Exposer le repli et exiger des seeds
  device non nulles dans chaque lane, ou renommer précisément le régime ;
- le selftest falsifie `digest_all`, mais pas `digest_balls`. Ajouter un mutant
  dédié, ainsi que les cas backend absent, lancement nul et route device vide ;
- le faux GPU n'émet pas la ligne réelle contradictoire
  `authority=status_terminal`. Tant que l'autorité n'est pas scellée, le
  protocole ne doit pas transformer le seul préfixe backend en autorité ;
- `timeout` n'a pas de `--kill-after` : un processus ignorant `TERM` peut
  dépasser la borne par run. Les coupe-circuits VM restent le dernier recours,
  mais le contrat transactionnel annoncé doit aussi escalader vers `KILL` ;
- la campagne reçue manque toujours le second adaptatif, le journal, le verdict
  du validateur, les codes session/rapatriement et la sortie certifiant l'arrêt.

La prochaine session doit recevoir séparément : petites portes device, mutants,
deux routes réellement non vides, quatre couples CPU/GPU avec les deux digests,
et stratégie adaptative. Le tout-device dense peut rester un diagnostic de
ressource distinct ; ne pas rendre son succès obligatoire pour promouvoir une
stratégie qui ne l'utilise pas.

## Contrat conseillé pour la livraison 7 par rectangle

La direction attaque le bon poste et reste compatible avec l'invariant
d'architecture : elle ne matérialise pas la mosaïque de Delaunay d'ordre
supérieur. Mais déplacer l'énumération sur le device ne suffit pas à garantir
la mémoire ni le gain. `rect_cover_handles` fournit une antichaîne locale dont
l'union contient les covers des ancres, mais ce « cover de rectangle » est un
**sur-ensemble fail-open**, pas le cover exact de chaque ancre. Le filtre exact
et l'ordre stable en 32 bins doivent donc être reproduits après projection.

Le claim `somme covers rectangles << somme covers ancres` n'est pas mesuré. Le
reçu donne environ 9,84 paires `|A| * |B|` par rectangle vivant Q3 et 13,62 en
Q4 avant filtres, mais il ne donne pas le nombre ni la distribution des covers
effectivement calculés. Les ratios 4,7/1,6 ne comptent que les ancres avec seed
matérialisées au device et ne mesurent donc pas le partage réel. Un sur-ensemble
lâche peut coûter plus que la somme des covers exacts ; recevoir les tailles
aplaties et leurs quantiles avant d'attribuer un gain. Avant le kernel :

1. définir l'entrée comme le candidat rectangulaire issu des handles, pas comme
   un « cover du rectangle » supposé exact ; mesurer somme et maximum des sites,
   `|A| * |B|`, seeds, complétions et survivants ;
2. reproduire le filtre exact et l'ordre stable actuel des 32 seaux radiaux, ou
   redéfinir et requalifier les compteurs : les arrêts précoces rendent les
   compteurs dépendants de l'ordre de visite ;
3. employer précomptage/prefix-sum vérifié et tuilage déterministe pour ancres,
   seeds, complétions et survivants, avec reprise explicite d'overflow ;
4. ne conserver ni tous les covers aplatis ni une matrice rectangle × ancre ×
   point ; borner aussi le retour des survivants et agréger tous les compteurs ;
5. employer `cover_query` comme oracle indépendant de l'**ensemble** seulement :
   il trie complètement par distance et identifiant, alors que la production
   `anchor_cover_from_handles` préserve l'ordre d'expansion à l'intérieur de 32
   bins stables. Employer cette dernière comme référence d'**ordre**, puis
   établir l'égalité de la lane shaped brute, post-RLE et compteurs ;
6. si `Gd`, `Nd`, `bound` ou `Jlo/Jhi` passent sur device, certifier la
   conversion DI128 vers binary64 bit à bit aux frontières d'arrondi ;
7. mesurer séparément somme et maximum des handles/covers, visites, octets
   H2D/D2H, préparation, kernel et compaction aux tailles 8k à 50k.

Cette voie est prometteuse si elle partage réellement un candidat de cover
entre beaucoup d'ancres et émet peu de survivants. Ces deux rapports sont des
quantités à recevoir, pas des hypothèses à transformer en claim. Une variante
plus sobre à mesurer est un tableau global O(n) de positions/`PointId`, partagé
entre ouvriers, avec seulement les plages de handles transférées par fenêtre de
rectangles. Le dupliquer dans 48 exécuteurs recréerait un défaut de résidence.

## Revue statique de la sonde rectangle en cours — hors verdict

La nouvelle `bench/rect_probe.cpp`, observée non commise après `312034ce`,
réutilise bien les corps Q3/Q4 nominaux et vise les bonnes grandeurs. Elle ne
doit toutefois pas encore produire de reçu :

- en Q4, le cover d'une ancre est calculé puis exclu de `rect_cover_sum` si W4
  la tue, alors que ce coût a bien été payé ; à l'inverse, les ancres sans seed
  sont incluses bien que les lots ne les matérialisent pas. Le probe ignore aussi
  seuil et caps de routage. Séparer covers post-histogramme, post-W4, avec seed,
  matérialisés device et traités hôte, avec leurs dénominateurs ;
- les grandeurs dimensionnantes manquent : `seeds * cover` en Q3, lentille,
  `seeds * lens` et complétions Q4. `lo.size()` est une émission brute pré-RLE,
  pas un « survivant » comparable entre Q3 et Q4 ;
- sept `u64` sont conservés par rectangle et deux par ancre, un vecteur de
  candidats est recréé par ancre, puis chaque série est triée trois fois. À
  l'échelle de millions de rectangles et dizaines de millions d'ancres, la
  sonde peut devenir elle-même le principal coût ou OOM. Employer des
  histogrammes/estimateurs bornés, réutiliser les buffers et trier une fois ;
- sommes, produits et compteurs peuvent déborder silencieusement. Les rendre
  fail-closed ; déclarer l'estimateur de quantile, parser strictement et imprimer
  `coord`, seed, `s`, `smax`, coefficient, filtre flottant, source et worktree.
  Construire cette sonde avec `mhgp5_product_executable`, pas avec la définition
  de test `MHGP5_TESTING=1`.

## P2 — nettoyage utile

- Rafraîchir complètement [`GPU.md`](../docs/GPU.md), pas seulement son en-tête.
  Il appelle encore les lots 8f « bornés » sans mesure de leurs maxima, affirme
  d'abord « la cause n'est pas le kernel », garde `K=1..10 exact`, décrit plus
  bas les anciens OOM/caps/flush post-ancre et annonce encore Q4 « en attente de
  G4 ». Ces passages contredisent le reçu et `6e8a6aba`. Employer « égalité
  bornée observée », « les runs ont terminé sans OOM » et le diagnostic causal
  probabiliste.
- Dans la conception livraison 7, distinguer les primitives terminales déjà
  `MHGP5_HD` du travail encore hôte (`rect_cover_handles`,
  `anchor_cover_from_handles`, histogrammes de coins, formes affines et
  construction des bornes). Chaque handle couvre au plus 32 positions ; le
  nombre de handles du rectangle n'est pas borné à 32. Les 4 202 134 rectangles
  Q3 et 4 648 802 rectangles Q4 vivants imposent des fenêtres multi-rectangles
  ou une file persistante, des sorties bornées et des ordinaux stables. Les
  550 242 « lancements » reçus sont un total de kernels ; Q4 peut en lancer
  trois par lot, donc ne pas en déduire directement un ratio rectangles/lots.
- La RAII des événements et la séparation interne H2D/kernel/D2H Q3 sont de
  bonnes corrections statiques de `6e8a6aba`, mais ces deux nouveaux temps ne
  sont pas encore agrégés ni imprimés. Q4 conserve une fenêtre mêlant kernels,
  transferts, synchronisations et compaction hôte ; terminer l'instrumentation
  avant toute conclusion causale.
- Conserver le mutant Q3 reçu et ajouter, lors d'une prochaine campagne
  autorisée, un mutant propre à la lane Q4 réellement exécuté sur device.
- Mettre à jour [`MATHEMATIQUES.md`](../docs/MATHEMATIQUES.md) : relabeling,
  mutants de rendu et oracle de forêt ont désormais des portes. Cela ne livre
  pas pour autant le payload public de rendu, qui reste à distinguer.
- Aligner [`PLAN_DE_TESTS.md`](../docs/PLAN_DE_TESTS.md) sur le vrai périmètre
  de `mutants_gate` et sur la campagne G4 déjà reçue. Dans
  [`PROVENANCE.md`](../docs/PROVENANCE.md), remplacer les noms de cibles
  inexistants, unifier la classification de `device_forms` et borner le reçu
  GPU à sa source.
- Le pin différentiel `receipts/conformite_v4/familles_v4.txt` nomme un
  programme compilé hors dépôt sans source, commande, toolchain ni hash
  binaire. Le conserver comme historique, mais le régénérer de façon rejouable
  avant de lui donner davantage d'autorité.

## Ordre de fermeture conseillé à Claude

1. Réparer l'activation du mutant CUDA Q3 pour ne pas enregistrer une porte
   condamnée au mauvais code de sortie.
2. Finir le validateur Q4, les domaines `u32`, les additions vérifiées et les
   fixtures non vacuables de repli.
3. Fermer l'autorité des overrides et des callbacks.
4. Corriger les claims et contradictions de `GPU.md`, puis écrire la porte CPU
   par rectangle avec référence d'ordre et capacités explicites.
5. Durcir le protocole sur les deux digests, les routes et les préconditions de
   phase, puis seulement programmer une nouvelle session G4 gardée.

## Reproduction et limites de cet audit

Le code produit de `b79c001b` a été exporté dans une archive propre ; il est
identique à `10c46c87` et `a0d13420`. Résultats locaux :

```text
cmake -S <archive>/morsehgp3D_v5 -B <build-release> -DCMAKE_BUILD_TYPE=Release : code 0
cmake --build <build-release> --parallel 4 : code 0
ctest --test-dir <build-release> --output-on-failure --parallel 4 -L gate : 156/156, 146,45 s
cmake -S <archive>/morsehgp3D_v5 -B <build-asan> -DCMAKE_BUILD_TYPE=Debug -DMHGP5_ENABLE_SANITIZERS=ON : code 0
cmake --build <build-asan> --parallel 4 : code 0
ctest --test-dir <build-asan> --output-on-failure --parallel 4 -R '^(mhgp5_api_guard_gate|mhgp5_batch_contract|mhgp5_parallel_exception|mhgp5_q3_lane_batched_route_256|mhgp5_q4_lane_batched_route_256|mhgp5_q4_lane_batched_route_tout_hote|mhgp5_q4_lane_batched_petit_lot_paires|mhgp5_q4_lane_batched_petit_lot_sites|mhgp5_q3_lane_batched_petit_lot_sites)$' : 9/9, 229,24 s
python tools/check_docs.py : 210 Markdown actifs validés
python tools/check_implementation_status.py : 20 phases validées
```

Les `LastTest.log` correspondants résident dans un répertoire temporaire non
versionné. Ce sont des résultats locaux observés, pas des reçus pérennes ; les
durées sont donc l'élément de provenance le plus faible. Les deux vérificateurs
Python ont également été observés localement sans journal épinglé.

Le validateur épinglé du reçu G4, rejoué localement avec les codes de transport
supposés nuls, rend `campaign_status=partial_or_failed` sur le seul artefact
adaptatif `scanline_single_pass` absent. Les hashes du payload source et du
manifeste se recomposent exactement. La comparaison indépendante de toutes les
lignes `digest_balls`, `digest_all` et des forêts K=1..10 confirme les accords
CPU/GPU décrits plus haut, sans étendre leur portée au tip.

`nvcc` est absent : aucune compilation CUDA courante n'est revendiquée. GCP
n'a pas été interrogé ni muté par cet audit ; l'arrêt raconté par le reçu n'a
donc pas été recertifié indépendamment. Le probe concurrent
`.codex_fold_contract_probe.cpp` n'a été ni ouvert, ni modifié, ni inclus.
