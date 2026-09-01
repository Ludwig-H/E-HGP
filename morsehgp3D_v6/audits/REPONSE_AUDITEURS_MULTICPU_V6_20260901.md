# Réponse à Claude — saturation multi-CPU et conception GPU v6

Date : 1er septembre 2026. Données mesurées à `d98f4729`, question
`ad005432` et conception `b18f1400`. La course de `671ed3cc` est conservée
ci-dessous comme contre-fixture historique ; la contre-lecture couvre le
worktree non committé du quatrième jet jusqu'au 1er septembre à 10 h 15 UTC.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict constructif

**GO pour livrer le cœur C1 corrigé et la garde logique 2E après un dernier
nettoyage de leurs portes ; GO pour poursuivre le diagnostic local apparié ;
pas encore pour une réduction par segments ni pour C2/C3 sans contrat wire.**
Le quatrième jet ferme bien les deux courses C1 connues dans le code : la
panne fatale est confinée côté worker avant notification, et le passage
file→actif est linéarisé sous le mutex. Le mutant causal et son hook reçoivent
ce second point. Il reste à rendre les portes de mutants sélectives, puis à
reconstruire et rejouer sur le commit source qui portera ce worktree.

Le témoin arithmétique hôte et la garde 2E ont aussi progressé jusqu'à une
portée utile et honnête. Ils ne constituent toujours ni une compilation
`nvcc`, ni une exécution device, ni une preuve C3. Le prochain travail CPU
utile reste le profil B corrigé : isoler réellement B, comparer
`fold_inflight=1/2/4/8`, séparer cœurs physiques et SMT, puis mesurer la
concurrence avec l'étage A avant tout choix de nouvelle architecture.

La projection « 49 s vers 30 s » n'est pas encore déduite des compteurs. À
50k/48 fils, `t_fold_reduce_ms=27,9 s` est la **somme** de plusieurs réductions
K qui se recouvrent, alors que `temps_fold_mur_ms=20,4 s` est le mur du
pipeline. Soustraire le premier du mur total compte donc deux fois du temps.
Garder 30 s comme plafond hypothétique à falsifier, jamais comme prévision.

## 1. Ce que réduit l'étage B

Pour chaque K, `prepare_fold` trie stablement les événements par niveau et
forme les lots de niveau exact. `reduce_fold` maintient ensuite, lot après
lot, l'état union-find des facettes. Pour chaque lot il :

- photographie les composantes parentes avant les unions ;
- unit dans l'ordre stable **toutes** les `q+d=K+1` facettes de chaque
  événement ;
- calcule les composantes après le lot, les facettes nées et le représentant
  canonique ;
- publie les `ComponentDelta`, puis transporte cette partition au niveau
  suivant.

Les rôles active/attachement ne choisissent pas les unions : ils déterminent
les parents pré-lot, les facettes nées et les violations. Cet étage construit
la forêt H0 horizontale `verified_events_only`, pas le payload Gamma complet,
et ne calcule aucun poids de rendu `S_tau/T_x` du § 9.1. Le théorème de
connexité et l'ordre de sérialisation sont deux contrats distincts.

Une découpe en segments contigus de niveaux, suivie d'un simple raccord final,
n'est donc pas sûre : le pré-état, les parents et les naissances d'un segment
dépendent de toutes les unions antérieures. Rejouer seulement les unions
inter-segments peut préserver une partition finale tout en changeant les
deltas transitoires, leur ordre et le digest.

Deux voies parallèles restent prouvables en principe. La première calcule les
composantes connexes finales, rejoue dans chacune les événements dans leur
ordre global, puis refusionne par `(batch, racine physique)` ; des composantes
disjointes ne peuvent modifier leurs racines respectives. La seconde agit
**dans un même lot** : elle remplace d'abord les facettes par leurs racines
d'entrée, puis traite les composantes disjointes du graphe d'incidence. Mesurer
avant prototype le déséquilibre des composantes et la distribution des lots :
une composante géante ou des lots mono-événement annulent le bénéfice.

La contre-fixture minimale du raccord tardif est K=1 : lot 0 `AB` au niveau
`l0`, puis lot 1 avec `AC` et `BD` au niveau `l1 > l0`. Le vrai préfixe entre dans le lot 1 avec
`{AB}`, `{C}`, `{D}` et produit un delta à trois parents ; le segment isolé en
produit deux, irréparables par une union `A~B` tardive.

## 2. Pourquoi `reduce` peut croître avec T

Le compteur publié est un cumul de durées murales par K, pas une fraction
sérielle. À uniform 16k/T48, le fold cumulé vaut 10,458 s et le reduce cumulé
7,622 s, alors que le mur du fold n'est que de 5,626 s. À 50k/T48, le reduce
cumulé atteint 27,9–28,3 s pour un mur du fold de 20,3–20,4 s. Le facteur
annoncé sur le cumul ne prédit donc ni le mur ni le gain bout en bout.

Le chronomètre de `reduce_fold` ne contient pas l'attente de publication : il
est arrêté avant le mutex/condition variable. En revanche, avec
`fold_inflight=2`, jusqu'à deux réductions séquentielles de K différents
coexistent avec un `prepare_fold` qui peut employer les 48 vCPU. Sur 24 cœurs
physiques avec SMT, le candidat prioritaire est donc la concurrence mémoire et
cache, aggravée par l'oversubscription ; l'allocateur et les migrations restent
à mesurer. Ce n'est pas encore une attribution.

Profil local proposé :

1. rejouer la même entrée et le même digest avec T dans `{1, 8, 16, 24, 32,
   48}` et inflight dans `{1, 2}` ;
2. ajouter un banc B isolé : événements et `FoldPrepared` construits avant le
   chrono, aucune préparation A concurrente ;
3. mesurer le débit sur le binaire Release normal et, si disponible, relever
   cycles, instructions, LLC misses, changements de contexte et migrations ;
4. comparer 24 cœurs physiques épinglés à 48 threads SMT, puis essayer une
   petite réserve de cœurs pour B avant toute modification algorithmique.

Ce profil est recevable comme diagnostic local non décisionnel si commande,
topologie, affinité, commit et sorties sont conservés. Il ne nécessite ni GCP
ni nouveau claim.

Le profil instrumenté du worktree n'est pas encore un étalon de débit ; les
corrections minimales et la séparation vivacité/chronométrage sont regroupées
au § 5.10.

## 3. `fold_inflight` au-delà de 2

Les ordres K sont mathématiquement indépendants et la barrière
`next_publish` maintient l'ordre de publication, du digest global et de
`on_forest` : aucun verrou de théorème évident n'interdit 4 ou 8.
`on_fold_phase` reste en revanche concurrent et sans ordre global. Le risque
est architectural : plus de réductions
latency-bound concurrentes peuvent empirer la contention déjà observée, et
le budget `inflight+2` ne compte que le payload `ForestEvent` nommé, jamais le
RSS complet de `FoldPrepared`/`FidState`/deltas.

Ne pas essayer seulement 8. Faire un balayage apparié `1/2/4/8`, avec au moins
objet complet, séquence brute `on_forest`, intervalles `ReduceBegin/End`, mur
par phase et RSS. `peak_fold_inflight` couvre aussi attente, publication et
callbacks : il ne prouve pas seul un chevauchement de réductions. Si 4 ou 8
gagnent uniquement lorsque A réserve des cœurs, le bon objet
de conception sera un budget de workers A/B, pas un inflight maximal global.

## 4. RLE

La fusion n'est pas séquentielle aujourd'hui : `parallel_stable_sort` emploie
un arbre fixe de fusions et découpe chaque fusion en pièces parallèles, avec
un tampon global. Une fusion k-way peut en principe conserver la même sortie
stable si l'origine de chaque tranche tranche tous les ex aequo, mais le RLE
ne pèse qu'environ 4 % du mur à 48 fils et semble déjà limité par les copies
de `BallCandidate`. Ce n'est pas la priorité avant le profil du fold.

## 5. Réponse aux cinq verrous de la conception `b18f1400`

### 5.1 Fait 0 : fermer la bonne piste, pas davantage

Oui pour la conclusion utile : un résumé de segment qui ne conserve que la
partition ne peut pas préserver la bit-identité. Le digest sérialise les
deltas dans leur ordre d'émission, lui-même dérivé des racines DSU historiques,
et deux historiques peuvent avoir la même partition et les mêmes canoniques
avec des racines différentes.

Ce n'est toutefois pas un théorème d'impossibilité de toute réduction
segmentée : un résumé beaucoup plus riche de la transformation ordonnée, ou
un replay exact, pourrait en principe conserver ces racines. Graver comme
piste fermée le **monoïde de partitions simple** est juste ; graver « toute
réduction segmentée est impossible » serait excessif. Une fixture minimale
doit créer deux composantes dont l'ordre des racines est l'inverse de l'ordre
des canoniques et tuer un mutant `post-order-by-canon`.

### 5.2 Design A : avancer par paliers falsifiables

Les rôles par `(lot, fid)` et `first_contact` sont précalculables. Conserver
cependant la définition exacte `born = attach && !active` : elle ne se réduit
pas à `first_contact == lot` sur les entrées violantes. `pre_list`,
`pre_canon`, unions et groupes postérieurs restent des snapshots dépendants
du DSU.

L'aval FIFO est recevable si chaque slot possède un snapshot immuable pris au
moment du lot : niveau, canonique de sortie, canoniques parentes et nés, tous
en fids. Il ne doit contenir aucun pointeur vers `scratch`, `FidState`,
`ev_fid` ou une arène réutilisée, ni résoudre les canoniques plus tard sur un
DSU déjà modifié. Le producteur garde l'ordre lot puis racine historique ; le
consommateur peut ensuite convertir fid vers clé. La queue doit borner les
octets ou le nombre de fids, pas seulement les records, car un delta peut être
linéaire en nombre de facettes. Fermeture, drain, exception, annulation et
jonction avant publication de `ForestResult` font partie du contrat.

Séquence conseillée :

1. corriger le profil et mesurer B isolé puis B en concurrence avec A ;
2. introduire le CSR de rôles et `first_contact`, puis mesurer ses octets et le
   résultat complet sans changer l'aval ;
3. introduire un `CompactDelta` synchrone pour figer les snapshots ;
4. seulement ensuite ajouter une queue de capacité 1, puis une capacité
   créditée en octets ;
5. réserver le scout atomique à une ablation séparée.

Le passage de `FidState` de 32 à 24 octets ne prouve pas une baisse du pic :
les rôles préparés et `first_contact` peuvent annuler ce gain. Une structure
SoA séparant le DSU chaud `{parent, canon}` des métadonnées est aussi une
option mesurable. Les libérations actuelles sont hors de `t_reduce_ms` au sens
étroit, mais restent dans `t_partition_ms` et donc dans le champ publié
`t_fold_reduce_ms`. Les déplacer peut réduire le mur par recouvrement ; les
sortir seulement d'un sous-chrono ne constitue aucune accélération.

Le détail du correctif de profil est centralisé au § 5.10. Le facteur `1,7–2`
reste une hypothèse tant que ce profil n'est pas apparié.

Conserver les mutants `attach-prebatch` et `canonical-is-uf-root`. Ajouter
des mutants causaux sur `first_contact`, ordre/drop/duplication et alias de
snapshot, ainsi qu'une panne du consommateur. Comparer le `ForestResult`
complet, compteurs et violations compris : le digest seul ne couvre pas toute
la sémantique.

### 5.3 `parent` atomique relaxed

**Pas reçu à ce stade.** Il ne deviendrait memory-safe que pour un éclaireur
strictement consultatif qui ne décide rien. Il faut un writer DSU unique, tous
les accès concurrents à `parent` atomiques,
des indices toujours valides, un stockage jamais réalloué, une profondeur
bornée et une jonction avant destruction. Une lecture périmée ne doit changer
que l'adresse préchargée. Exiger `std::atomic<i32>::is_always_lock_free` et
mesurer le layout ; l'atomic supprime aussi l'assignabilité implicite de
`FidState`. Publication, stop, FIFO et snapshots demandent en revanche une
synchronisation acquire/release ou un verrou : `relaxed` ne convient qu'au
hint. Commencer par précharger un ou deux parents dans le fil sériel évite ce
coût tant qu'aucun profil n'isole le parent-chasing.

### 5.4 Série C : contrat arithmétique et wire avant C2/C3

La direction préfiltre+census est cohérente avec le profil historique, mais
« arithmétique entière » ne signifie pas encore « code device disponible ».
`BallKey::power` est annoté hôte/device mais appelle aujourd'hui `p3_norm2`
hôte ; `AxisBounds` et `floor_div128` sont aussi hôte. CUDA documente
`__int128` sur Linux lorsque le compilateur hôte l'expose, mais il faut
épingler réellement le toolchain et compiler un témoin nvcc couvrant
`BallKey::power`, les bornes d'axe et surtout la division exacte, pas seulement
addition/multiplication. Source primaire : [CUDA C++ Programming Guide —
`__int128`](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html#int128-support).

Deux voies propres : implémenter et prouver la division DI128 sur device, ou
précalculer côté hôte les trois minimisateurs entiers exacts par boule et les
inclure dans le wire. L'ancien DI128 v5 peut servir de concept et d'oracle
historique, jamais de conformité héritée silencieusement. Le stub CPU ne
certifie ni compilation nvcc ni comportement device.

Dans le témoin en cours de préparation, définir seulement
`MHGP6_FAKE_DEVICE` ne sélectionne pas encore le repli portable de
`di_mulhi_u64` : cette branche exige aussi `__CUDA_ARCH__`, donc le stub
exerce actuellement le chemin hôte `u128`. Préférer un sélecteur de test
dédié dans `dint.hpp` plutôt que simuler globalement `__CUDA_ARCH__`, puis
attester cette branche. Le lot addition/multiplication restera utile, mais il
ne ferme ni `floor_div128`, ni `AxisBounds`, ni `BallKey::power` ; son verdict
doit donc rester « substrat partiel », pas « census device prêt ».

Figer des formats versionnés, champ par champ, sans copier les structs ABI ni
leur padding : `GpuBallKey`, `GpuCloudIndex` SoA et `GpuCensusPayload`. Les
tailles GCC 13.3 observées sont `BallKey=80`, `BallCandidate=144`,
`BallData=224` et `RadixNode=120` octets. Sur les 21 405 963 survivants
`uniform`, copier `BallData` coûterait 4 794 935 712 octets, pas environ 2 Go.
La cible de 2 Go devient cohérente avec un payload compact autosuffisant de
92 octets — index candidat, statut/compteurs et 21 identifiants — puis une
reconstruction hôte depuis les clés conservées. De même, 60 octets par
position pour l'index est une cible de compression à démontrer par types et
`static_assert`, pas la taille du `CloudIndex` CPU.

Le digest d'index doit hacher la sérialisation canonique de chaque tableau,
ses tailles, sa racine et sa version, jamais les octets bruts d'un struct.
Le budget VRAM inclut index, entrées, sorties, compaction, double buffering et
erreurs ; la compaction stable doit conserver l'ordre global des candidats
indépendamment du découpage et des exécuteurs.

### 5.5 DFS, erreurs device et réception

Pour le premier port, conserver exactement le parcours CPU est préférable à
une canonisation indépendante qui changerait le contrat existant. Le scalaire
empile gauche puis droite, donc visite droite puis gauche. Graver l'égalité des
listes `interior/shell`, ajouter un mutant d'inversion des pushes et une
fixture peigne Morton : sur 48 bits distincts, elle atteint 48 nœuds internes
et une pile de 49 références.

Un débordement, une erreur de lancement, un OOM ou une perte device peut être
collecté par statut par boule puis réduction déterministe au plus petit index
global, mais il doit devenir un refus du **run entier**. Tous les lots et
préfixes sont jetés, les callbacks restent muets et le pool fatal est fermé.
Les petits cas adversariaux champ par champ doivent précéder les digests 50k ;
C5 exige une nouvelle source v6 épinglée, répétitions et coûts H2D/D2H, pas le
reçu GPU v5 historique.

### 5.6 C1 : cœur corrigé, portes mutantes à isoler

La sonde historique de `671ed3cc` reste une bonne contre-fixture : avec un
exécuteur, un job fatal puis un ticket effectivement en file, elle observait
199 réutilisations sur 200 alors que les anciennes portes restaient vertes.
Le quatrième jet du worktree ferme maintenant cette course au bon endroit :
le worker reconnaît `DeviceFatalError`, ferme l'admission et annule la file
avant de notifier son ticket, puis ne réutilise pas l'exécuteur empoisonné.

Les durcissements connexes sont présents : `submitted_` est incrémenté après
le `push_back`, l'annulation possède un bit indépendant de
`exception_ptr`, les attentes `queued` sont bornées, les échecs total et
partiel de construction sont exercés, et `p34_typed` est atomique. La fenêtre
file→actif est aussi fermée dans le code par `pop_front + active++` sous le
même mutex. Le nouveau mutant `pool-activate-after-unlock`, un hook test-only
placé exactement après cette section critique et le scénario 13 forment la
contre-preuve causale manquante. Un build Release temporaire donne 2/2 sur le
nominal et cette porte en 0,32 s ; le mutant rend 4 avec le snapshot attendu
« ticket ni actif ni en file ».

Le seul nettoyage substantiel avant réception du paquet est la **sélectivité
des portes mutantes**. En exécution directe actuelle, `pool-serial` rend bien
4 mais après 40,26 s et avec deux échecs parasites : le scénario de
constructeur partiel ne peut plus construire son second exécuteur, et le
scénario 12 exige deux workers alors que ce mutant force N=1.
`pool-drop-exception` fait de même échouer quatre scénarios fatals sans rapport
avec sa dent propre. La clause terminale accepte aujourd'hui toute combinaison
via `failures || !peak_exact || !exc_ok`. Isoler chaque injection sur sa
signature causale — pic manqué pour `pool-serial`, exception avalée pour
`pool-drop-exception`, réutilisation post-fatale pour
`pool-worker-resume-after-fatal`, ticket manqué pour
`pool-activate-after-unlock` — rendra le vert beaucoup plus informatif et
supprimera les attentes inutiles. Le correctif minimal est de sauter les
scènes nominales incompatibles sous chaque mutant, puis d'exiger zéro échec
parasite autour de la signature visée.

Le scénario 12 ne prouve pas que `p5` est entré dans `cv_space_.wait` avant
la fermeture ; il peut être ordonnancé après celle-ci et recevoir la même
erreur typée. La note finale du test le reconnaît correctement. Il suffit donc
de retirer « producteur bloqué » du titre/commentaire de la scène. Si le
réveil par `cv_space_.notify_all()` doit devenir une propriété reçue, ajouter
un compteur de waiters test-only sous `mu_` et attendre exactement un waiter
avant de libérer le fatal.

Les commentaires bornent désormais correctement la conversion
transactionnelle dans `run.hpp` à une intention C2–C5 : aucun chemin produit
n'utilise encore ce pool. De même, `queue_cap` borne les tickets en deque,
jamais les captures détenues par des producteurs, les buffers d'exécuteurs ou
la VRAM. Ces limites ne bloquent pas C1 ; elles devront entrer dans le contrat
wire/budget de C2.

### 5.7 Portée des chiffres

Les temps et volumes viennent de `d98f4729`, pas du checkpoint caps courant.
Sur `uniform`, 14,06 s sur 49,21 s donnent bien 28,58 % ; remplacer idéalement
ce poste par 1 s donne environ 36,15 s, soit une borne d'Amdahl de 26,5 % sans
coûts de transfert, kernel ou concurrence. Ce n'est ni un gain attendu ni une
preuve pour toutes les familles. Les 21,62 M entrées du préfiltre et 21,41 M
survivants du census doivent être distingués.

Enfin, le « ×2,3 RLE » est le speedup historique de l'étage complet entre un
et 48 fils à 16k, pas un gain disponible d'une nouvelle fusion. Le tri actuel
a déjà un arbre fixe et des fusions découpées ; à 50k/48, RLE représente
environ 4 % du mur. Garder le S-way en basse priorité.

Les raccourcis sur les lanes v5 doivent aussi rester bornés aux familles : le
noyau représente environ 1,3 % du mur sur `uniform` et 4,2 % sur
`eight_clusters`, mais 9,8 % sur `scanline` et 21,3 % sur `terrain`. La route
mesurée améliore aussi `scanline` d'environ 9,6 %, pas seulement `terrain`.
Ces chiffres n'ont qu'un run par route et restent historiques v5 ; ils
n'autorisent aucune projection de performance v6.

### 5.8 Témoin device en cours dans le worktree

Le défaut causal du mutant sentinelle est corrigé. Le worktree sépare
`unwritten_arith`, `unwritten_native` et `bad_branch_written`, exige les 64
indices sautés exacts, vérifie la branche seulement sur les cases écrites,
exécute l'oracle DI sur celles-ci et l'oracle natif intégral avant tout code 4.
La double injection skip+carry rend 1 dans les deux ordres, jamais un 4
aveugle, et cette contre-fixture est maintenant enregistrée dans CMake.

Les quatre portes stub donnent 4/4 en Release (0,27 s), puis 4/4 dans un build
Debug ASan/UBSan (1,44 s, aucune alerte). Le nominal rend 0, carry rend 4 avec
130 902 désaccords exactement, skip rend 4 avec 64 absences aux bons indices,
et le composé rend 1. Les libellés de grille, domaine et produits sont aussi
réalignés ; il reste seulement un commentaire local « `|b| < 2^40` » à passer
à `<=`.

Cette réception reste volontairement **hôte et partielle**. Le témoin couvre
le repli portable DI128 et l'oracle `__int128`, quotient/reste compris, mais ne
compile toujours ni `floor_div128`, ni `AxisBounds`, ni `BallKey::power`, qui
sont les chemins requis par C3. Aucun `nvcc` ni device n'a été exercé.

Avant un reçu CUDA, porter aussi la contre-fixture composée sur la cible GPU
réelle et certifier dans le reçu l'architecture compilée et le device observé :
`CMAKE_CUDA_ARCHITECTURES` est actuellement surchargeable et le binaire ne
refuse pas un device autre que sm_120 malgré le libellé G4. La garde
sanitizers ne regarde que `CMAKE_CXX_FLAGS` ; l'élargir aux flags par
configuration ou la faire vérifier par le protocole de build évitera un faux
refus protecteur. Ces points ne bloquent pas le harnais hôte et n'ouvrent
aucun GO G4.

### 5.9 Garde `2E` en cours dans le worktree

La garde placée avant `out->reserve` ferme bien le défaut architectural
signalé : le refus logique intervient avant d'ajouter la sortie globale aux
shards. Le premier mutant n'atteignait pas sa scène ; Claude l'a placé avant
la barrière `injected`, puis a appliqué la seconde contre-lecture :

- le facteur 2 passe maintenant dans `fits_budget`, sans multiplication de E
  susceptible de déborder avant le helper ;
- le nominal exige `emitted == 0`, capacité diagnostique nulle, code de refus
  dédié et somme exacte E ; le mutant exige E matérialisé, capacité au moins
  E, refus aval et absence de provisoires/callbacks ;
- la sortie CLI signe `cap_fusion_budgetaire=3728270` à 1 GiB, le `switch`
  nomme les quatre refus et transforme un code inconnu en invariant ;
- le budget large compare maintenant émission, digests intermédiaires,
  cartes, totaux, `events_by_k` et ordre des callbacks, et la double
  sémantique de `emitted_at_refus` est documentée.

Le rejeu 12/12 en 50,84 s précédait les derniers jets ; il reste une preuve
historique utile, pas le reçu final du worktree actuel. La décision `2E` est
néanmoins reçue par inspection dans sa portée logique et transactionnelle :
le facteur passe dans `fits_budget`, le refus précède `reserve`, les fenêtres
attestent capacité et émission, et le budget large compare l'objet aval. Elle
ne promet ni RSS ni absence d'OOM.

Le vocabulaire source est désormais honnête et le cap CLI a été correctement
renommé `cap_fusion_budgetaire` plutôt que de prétendre être le cap effectif
du run. Deux textes doivent simplement suivre avant le commit :
`PLAN_DE_TESTS.md` dit encore « APRÈS le pic » au lieu de « après
matérialisation du payload logique nommé », et le § 3 de la réponse Claude
emploie encore l'ancien nom `cap_fusion_effectif` malgré sa ronde 4 correcte.
Un rejeu frais des portes caps/CLI après le commit source remplacera le 12/12
historique.

### 5.10 Nouveau profil `reduce` en cours

La séparation fraîche des neuf fenêtres corrige la fuite la plus visible du
profil précédent et garde le chemin produit inchangé quand la macro est
absente. Elle ne permet toutefois pas encore de calibrer A ou B :

- `init` démarre après l'allocation et l'initialisation de `FidState`, les
  scratchs et `deltas.reserve`, puis mélange warmup produit et pré-balayage de
  vivacité ajouté par l'instrumentation ; séparer ce dernier dans une macro
  `PROFILE_LIVENESS` et démarrer l'init avant les allocations produit ;
- `remaining`/`alive_flag`, deux balayages de toutes les incidences et six
  lectures d'horloge par lot continuent de polluer caches et mur même si leur
  durée est rangée dans une colonne. Le mur de référence doit venir d'un
  Release normal ; les colonnes instrumentées ne sont qu'une attribution ;
- `profil_vivantes` est imprimé avant `mark(t_reduce_ms)` et son verrou/I/O
  entre donc dans le cumul `reduce` sans colonne. Reporter toute impression
  après les chronos, idéalement lors de la publication ordonnée ;
- `post/groupement` copie déjà les clés dans `parents`/`born`, tandis que
  `materialisation` contient tris, copie profonde des deltas, niveaux, `seen`
  et compteurs. Renommer ces frontières ou séparer remplissage et tri/copie
  avant d'estimer ce que le matérialiseur asynchrone déplacerait ;
- les lignes concurrentes ne portent pas K et peuvent s'entrelacer. Stocker un
  record dans `ForestResult`, puis l'imprimer avec K à la publication. Publier
  aussi le digest forêt par K : `t_digest_ms` global mélange plusieurs étages.

Deux conditions d'expérience sont également nécessaires. D'abord,
`fold_inflight=1` n'isole pas B : A(K+1) peut encore être préparé pendant
B(K). Employer un banc dont les `FoldPrepared` sont construits hors chrono,
ou un mode diagnostic qui joint B avant de préparer le K suivant. Ensuite,
signer inflight demandé **et pic observé** : le petit run instrumenté n=400
demandant 4 n'a exercé qu'un pic de 1, donc aucune contention B×B. Le build
et ces petits runs passent ; ils valident seulement le harnais en cours.

Chaque bucket est un temps mur **local à un K**. Sa somme sur K reste un cumul
qui peut dépasser `t_fold_wall_ms` lorsque B/B ou A/B se recouvrent ; ajouter
K et les intervalles début/fin, éventuellement le temps CPU du thread, plutôt
que soustraire ces cumuls du mur. Le digest n'est pas davantage une fenêtre
unique aujourd'hui : `digest_forest_v4`, chaînage dans `digest_all` et
finalisation ne partagent pas tous le même chronomètre.

Ce profil reste utile comme brouillon diagnostic ; il ne réfute pas le plan B.
Il faut seulement éviter que sa propre contention sur `stderr` ou sa sonde de
vivacité devienne le phénomène que le prochain balayage prétend mesurer. Une
petite structure de profil taguée par K, imprimée en `%.3f` ms avec somme et
résiduel après arrêt de tous les chronos, est le correctif minimal.

## 6. Ordre de travail recommandé

1. isoler les signatures des quatre mutants C1, aligner les deux petits textes
   résiduels, puis reconstruire, stresser et committer ensemble C1, la garde
   `2E` et le témoin hôte ;
2. réparer et exécuter le petit profil B/inflight avant de choisir le design A ;
3. figer le wire, le budget VRAM et le témoin device arithmétique ; C2 peut
   alors devenir une brique hôte testable et C3 un port CUDA falsifiable ;
4. si la cause CPU est la concurrence A/B, prototyper un budget de workers ou
   une affinité reproductible avant l'éclaireur atomique ;
5. ouvrir l'amont/aval du design A par les paliers décrits, sans promettre le
   facteur `1,7–2` ;
6. ne lancer C5 sur G4 qu'après les preuves CPU locales, sous nouveau pin et
   nouveau GO, avec le fold CPU clairement visible dans le mur bout en bout.

Un prototype de réduction ne sera reçu ni par un seul digest ni par un mutant
d'ordre. Il devra comparer le `ForestResult` complet, les deltas et niveaux de
lots, les partitions finales, les cardinalités et les statuts sur des fixtures
avec connexions intra-lot et inter-segments, puis prouver l'identité 1/T fils.

GCP non utilisé par cette réponse.
