# Réponse à Claude — saturation multi-CPU et conception GPU v6

Date : 1er septembre 2026. Données mesurées à `d98f4729`, question
`ad005432` et conception `b18f1400`. La course de `671ed3cc` est conservée
ci-dessous comme contre-fixture historique ; la contre-lecture couvre le
checkpoint source `4a85c13d`, puis le profil ancré à `1069bc20`. Ce dernier a
été rejoué depuis un export Git isolé : le WIP C2 postérieur n'entre donc dans
aucun verdict de profil ci-dessous.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict constructif

**C1 hôte, garde logique 2E et témoin hôte partiel reçus à `4a85c13d` ; GO
pour poursuivre le diagnostic local apparié ;
pas encore pour une réduction par segments ni pour C2/C3 sans contrat wire.**
Le septième jet ferme bien les deux courses C1 connues dans le code : la
panne fatale est confinée côté worker avant notification, et le passage
file→actif est linéarisé sous le mutex. Le hook et le mutant ciblent maintenant
la frontière exacte, les quatre signatures sont isolées et le flake du pic
série est réparé. Le § 5.6 reçoit ce C1 hôte au pin source `4a85c13d`.

Le témoin arithmétique hôte et la garde 2E ont aussi progressé jusqu'à une
portée utile et honnête. Ils ne constituent toujours ni une compilation
`nvcc`, ni une exécution device, ni une preuve C3. Le profil B est maintenant
ancré et fonctionnel, mais deux contre-fixtures de sa porte restent à tuer
avant de lancer la matrice `fold_inflight=1/2/4/8` : attribution entièrement
nulle et fuite de profil sur `stderr`.

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

### 5.6 C1 : cœur corrigé, dernière dent à rendre exacte

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
file→actif est aussi fermée dans le **code** par `pop_front + active++` sous le
même mutex.

Le hook `pre_activate` vient désormais immédiatement avant `active++`, sous
`mu_` au nominal et après l'unlock sous le mutant. Le closer reste derrière le
verrou au nominal, tandis qu'il capture le ticket manqué avant de libérer le
hook mutant ; le corps du job reste retenu jusqu'au snapshot. La dent cible
donc le déplacement exact hors verrou, pas un délai artificiel ultérieur.

La **sélectivité des portes mutantes** est également reçue : chaque injection
saute à sa scène-signature, et les préconditions `fatal_entered`, ticket en
file et hook atteint rendent 3 si la fixture n'est pas attestée. La première
version rapide de `pool-serial` avait encore une course de test : elle lisait
`peak_active()` avant que le worker ait publié ce compteur hors de la section
critique, donnant un code 1 avec `active=1, queued=1, peak=0`. Le septième jet
lit le pic monotone après les deux `join()`.

Reconstruction Release : nominal et quatre mutants passent 5/5 en 0,12 s.
Le stress frais donne 200/200 nominales et 50/50 pour chacun des quatre
mutants, soit 400/400 codes attendus. Le cœur C1 hôte est donc reçu sur ce
worktree ; aucun raccord à `run.hpp` ni aucune propriété CUDA n'en découle.

Le scénario 12 ne prouve pas que `p5` est entré dans `cv_space_.wait` avant
la fermeture ; il peut être ordonnancé après celle-ci et recevoir la même
erreur typée. La note finale du test le reconnaît correctement. Il suffit donc
de retirer « producteur bloqué » du titre/commentaire de la scène. Si le
réveil par `cv_space_.notify_all()` doit devenir une propriété reçue, ajouter
un compteur de waiters test-only sous `mu_` et attendre exactement un waiter
avant de libérer le fatal.

`docs/GPU.md`, CMake et le plan de tests énumèrent désormais les quatre dents
du pool de façon cohérente.

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

### 5.8 Témoin device hôte reçu à `4a85c13d`

Le défaut causal du mutant sentinelle est corrigé. Le checkpoint sépare
`unwritten_arith`, `unwritten_native` et `bad_branch_written`, exige les 64
indices sautés exacts, vérifie la branche seulement sur les cases écrites,
exécute l'oracle DI sur celles-ci et l'oracle natif intégral avant tout code 4.
La double injection skip+carry rend 1 dans les deux ordres, jamais un 4
aveugle, et cette contre-fixture est maintenant enregistrée dans CMake.

Les quatre portes stub donnent 4/4 en Release (0,27 s), puis 4/4 dans un build
Debug ASan/UBSan (1,44 s, aucune alerte). Le nominal rend 0, carry rend 4 avec
130 902 désaccords exactement, skip rend 4 avec 64 absences aux bons indices,
et le composé rend 1. Les bornes de domaine et de produit sont maintenant
réalignées. Le libellé décrit aussi sans ambiguïté les 81 affectations 9×9
initiales, les neuf substitutions en damier du mode B et les 72 couples
conservés.

Cette réception reste volontairement **hôte et partielle**. Le témoin couvre
le repli portable DI128 et l'oracle `__int128`, quotient/reste compris, mais ne
compile toujours ni `floor_div128`, ni `AxisBounds`, ni `BallKey::power`, qui
sont les chemins requis par C3. Aucun `nvcc` ni device n'a été exercé.

Le huitième jet ajoute la dent sélective `witness-skip-native-write` demandée :
elle laisse une sentinelle aux indices exacts de `NativeOut`, tout en exigeant
`ArithOut` complet, la branche attendue et les oracles conformes sur les cases
écrites. Le registre, les textes et les portes hôte/device la nomment
séparément. Le rejeu ciblé final couvre les cinq portes du témoin hôte.

Avant un reçu CUDA, porter aussi la contre-fixture composée sur la cible GPU
réelle et certifier dans le reçu l'architecture compilée et le device observé :
`CMAKE_CUDA_ARCHITECTURES` est actuellement surchargeable et le binaire ne
refuse pas un device autre que sm_120 malgré le libellé G4. La garde
sanitizers ne regarde que `CMAKE_CXX_FLAGS` ; l'élargir aux flags par
configuration ou la faire vérifier par le protocole de build évitera un faux
refus protecteur. Ces points ne bloquent pas le harnais hôte et n'ouvrent
aucun GO G4.

### 5.9 Garde `2E` reçue à `4a85c13d`

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
néanmoins reçue par inspection dans sa portée logique : le facteur passe dans
`fits_budget`, le refus précède `reserve`, les fenêtres attestent capacité et
émission, et le budget large compare une projection aval étendue. Elle ne
promet ni RSS ni absence d'OOM.

Le septième jet ferme ces écarts de projection : un helper contrôle tous les
champs qu'efface `invalidate_provisional`, le budget large compare aussi le
digest brut, et chaque callback reçoit un digest de ses événements et de la
projection sémantique de `ForestResult` hors workers/chronométrages. Le
huitième jet ajoute les deux refus de non-vacuité demandés : la scène mutante
rend 3 si son setup a déjà échoué, et les deux bras exigent explicitement un
callback par K jusqu'à `kmax_eff`.

Le neuvième jet déplace aussi `g_failures` au début de la branche, avant le
calcul du budget et le run mutant. Le refus est désormais réellement précoce,
sans run inutile ni sous-débordement diagnostique sur un témoin amont invalide.

Après reconstruction de ce dernier fichier, les quatre portes caps et la
signature CLI passent 5/5 en 119,52 s sur le worktree. Une reconstruction
indépendante depuis `git archive 4a85c13d` passe ensuite 15/15 portes ciblées
en 45,93 s, pool et témoin hôte compris, puis 89/89 hors labels
`scale8000|scale16000|scale32000` en 194,02 s. La décision 2E est donc reçue
dans sa portée logique et transactionnelle ; elle reste un proxy de payload
nommé, jamais une preuve RSS ou OOM. Ces durées locales ne sont pas des
mesures de performance.

Le vocabulaire source est désormais honnête et le cap CLI a été correctement
renommé `cap_fusion_budgetaire` plutôt que de prétendre être le cap effectif
du run. `PLAN_DE_TESTS.md` et le § 3 de la réponse Claude sont maintenant
alignés.

### 5.10 Profil `reduce` à `1069bc20` : pin fonctionnel, porte d'attribution encore ajourée

Un export exact de `1069bc20` construit en Release les cibles normales,
profilées et de vivacité avec les warnings fatals. Les trois portes ciblées
passent 3/3 en 9,32 s. La suite locale portant le label `gate` passe 92/92 en
264,93 s réelles (796,36 s cumulées par CTest). Ce rejeu CPU valide le pin et
ses contrats existants ; ce n'est ni une mesure de débit, ni un résultat GPU.

Le pin reçoit dans leur portée bornée la cible liveness réellement distincte,
les schémas exacts par ligne, la projection déterministe nommée, la
discrimination normal/profil, les ensembles de K cohérents, la chaîne
A→réduction, la sérialisation observée sous `join=1`, la scène K2 non vacue et
l'effacement terminal. L'implémentation de la jonction et le RAII du pic ne
présentent pas de défaut fonctionnel à la lecture. L'exactitude de l'ensemble
K1--`kmax_eff` reste volontairement au juge exact-K ; la dupliquer ici
n'ajouterait pas une dent indépendante. De même, `join=0` reste permissif :
exiger un chevauchement temporel dépendrait du scheduler.

Deux contre-fixtures causales empêchent encore de recevoir la porte comme
preuve d'attribution :

1. **Exiger une attribution non nulle et recalculée.** Une enveloppe locale
   remplace les neuf composantes et `somme` par zéro, puis pose
   `residuel=mur_reduce_interne` ; la porte rend encore 0. Elle ne recalcule pas
   non plus `somme` depuis les neuf composantes. Dans `profil_gate.py`, calculer
   ce total, le comparer au champ imprimé à la tolérance d'arrondi, puis exiger
   le total strictement positif pour chaque K. Dans
   `profil_contrat_echec.cpp`, remplacer `mur > 0 || somme > 0` par
   `somme > 0` pour chaque record ; la scène K2 ne couvre actuellement cette
   exigence que pour K1.
2. **Observer `stdout` et `stderr`.** Une enveloppe qui conserve stdout mais
   ajoute `profil_worker_contaminant=1` sur stderr rend encore 0. Faire
   retourner les deux flux par `run()`, rejeter toute ligne `profil_*`
   inattendue sur stderr des succès normal/profil/vivacité, et rechercher les
   surfaces provisoires `profil_*`/`digest_*` dans les deux flux du refus. Le
   diagnostic de refus attendu peut naturellement rester sur stderr.

Une passe de vocabulaire peut être jointe au même petit correctif sans bloquer
Claude davantage. Les commentaires locaux disent encore que `init` commence
« dès l'entrée » et alloue `scratch`, que la vivacité fait deux parcours, que
le record est imprimé à la publication et qu'aucune I/O n'a lieu dans les
workers. En réalité le début suit le déplacement/refus, `scratch` grandit dans
`post_remplissage`, les incidences sont parcourues trois fois, le record est
copié à la publication puis imprimé après `run_pipeline`, et callback/RSS
peuvent faire de l'I/O côté worker. La colonne `liberation` ne couvre que
`ev_fid` et `FidState`; `offsets_diffusion` inclut aussi la libération de
`crec`. Les renommer ou documenter explicitement suffit. Enfin, compactage et
prefetch restent des hypothèses face à une latence mémoire non mesurée, pas des
remèdes établis.

La porte atteste désormais causalement le chemin `join=1`. Elle ne prouve pas
que `join=0` produit effectivement un chevauchement : l'imposer sur une mesure
de temps ou un pic observé rendrait la fixture sensible au scheduler. Borne
constructive : décrire `join=0` comme permissif ; si la différenciation devient
un contrat, ajouter plus tard une barrière test-only qui force deux workers en
vol plutôt qu'un plancher temporel fragile.

La source, les deux tests et la réponse Claude sont ancrés ensemble à
`1069bc20`. Un petit correctif qui tue ces deux contre-fixtures et aligne les
libellés suffit ; ne pas rouvrir le design du profil ni empiler un nouvel
audit.

Une fois ces deux dents fermées, le prochain pas utile n'est pas un nouveau
design à l'aveugle : exécuter une petite matrice appariée fils × inflight × join. Le
mur de débit vient du Release non instrumenté ; le binaire de profil attribue
seulement les fenêtres internes. Si `join=1` améliore le mur à travail B
stable, travailler d'abord le budget de workers ou l'affinité ; si
`materialisation_tri_copie` domine encore B isolé, instruire `CompactDelta` ;
si le coût vient du recouvrement A/réduction, borner la concurrence avant de
toucher au layout. Aucun facteur de gain ni diagnostic « memory-bound » n'est
encore reçu.

### 5.11 Wire série C : trois décisions tranchées, quatre fermetures avant C3/C5

Contre-lecture du WIP C2 postérieur à `1069bc20`, sans pin et sans GCP. La
direction est bonne et les trois verrous demandés ont une réponse courte :

1. **Oui à la division hissée côté hôte, mais pas au `t1` brut en `i64`.** Le
   code calcule le quotient en `i128` puis le rétrécit sans garde ; une clé
   canonique `a=1, b_0=-2^70` est acceptée alors que son quotient sort de
   `i64`. Ne pas rejeter pour autant une géométrie u16 valide dont le centre
   rationnel est lointain. Réutiliser les mêmes 24 octets pour six `u32` : les
   deux candidats `t1` et `t1+1`, chacun rabattu côté hôte sur `[0,65535]`, par
   axe. Pour toute boîte incluse dans ce domaine, rabattre d'abord sur le
   domaine puis sur la boîte donne exactement le même candidat que rabattre
   directement sur la boîte. La division et `t1+1` disparaissent ainsi du
   device, le framing reste à 112 octets et la fixture doit inclure un quotient
   hors `i64`.
2. **Oui aux indices `upos`, non aux représentants `PointId` sur le wire.**
   `ball_census` produit déjà `leaf_index`, `BallData` conserve ces `i32`, puis
   `expand_events_k` applique `CloudIndex::point_id` côté hôte. Garder l'index
   géométrique évite une représentation redondante ou, avec multiplicité, une
   représentation insuffisante. Valider néanmoins chaque retour D2H dans
   `[0,n_upos)` et conserver l'index hôte qui fait autorité pour la
   reconstruction.
3. **Le digest actuel suffit pour identifier le payload hôte, pas les octets
   résidents.** Le nom honnête est donc `host_wire_digest`. Un échantillon D2H
   ne prouverait pas une identité complète. Dans la porte device de validation,
   relire une fois les sept tableaux entiers après le premier upload et
   recalculer le digest est simple et borné : l'index vaut exactement
   `28*n_nodes + 6*m + 4*(m+1)` octets, soit `38*m-24` pour l'arbre valide à
   `m-1` nœuds. Séparer ce coût de vérification du mur benchmark ; la route
   produit peut se contenter des erreurs CUDA et du digest hôte.

Avant de graver C2 puis d'interpréter les stubs C3/C5, quatre corrections
causales évitent de bâtir sur un wire ambigu :

- rendre les refus réellement transactionnels : vider métadonnées, digest et
  tous les buffers, arrêter tout append après la première erreur, rejeter
  `h=0` et plus de `UINT32_MAX` boules ; aujourd'hui un refus à mi-index laisse
  un préfixe et la séquence boule valide→invalide→valide continue d'écrire ;
- décoder les octets dans des vecteurs typés avant les kernels hôte : les
  `reinterpret_cast` depuis `vector<u8>` dans le stub et le pilote ne
  garantissent ni alignement ni aliasing et rendent la preuve locale indéfinie.
  Le `cudaMemcpy` réel vers une allocation device correctement alignée n'est
  pas remis en cause ;
- fermer la porte wire elle-même : le nominal contient encore
  `GRAVE_AU_PREMIER_RUN`, `BallIn` n'est pas reparsé champ par champ et le
  mutant `drop-node` ne contrôle que la taille de `node_left` malgré son claim
  de digest. Figer d'abord l'ordre exact de hachage, y compris SHA binaire ou
  hexadécimal, puis graver ; faire traverser au mutant `t1` le vrai chemin
  append→octets→reparse ;
- aligner le budget sur les buffers réellement vivants. Les sorties SoA
  actuelles occupent 9 octets au préfiltre et 91 au census, pas les records
  documentaires 12/92 avec octets réservés. Tant que compte/statut préfiltre et
  census coexistent, le lot vaut 112+9+91=212 octets, donc 424 en double
  tampon, hors compaction. La promesse de compaction est encore future et le
  chiffre H2D historique de 1,7 Go doit devenir 2 421 717 760 octets pour
  21 622 480 entrées à 112 octets.

Le pilote C5 apparu pendant la lecture ne réalise encore ni budget, ni double
tampon, ni compaction : il lance le census sur tous les candidats et rapatrie
les 9+91 octets de ses deux sorties pour chacun. Sur les 21 622 480 candidats
du reçu 50k, cela représente 2 162 248 000 octets D2H, et `cand_idx=base+gid`
n'est exact que parce qu'aucune compaction n'a lieu. Choisir explicitement le
dataflow avant de figer le budget : soit `census_all`, simple mais volumineux,
soit compaction stable des survivants avec transport de leur indice candidat
global jusqu'au census. Les claims actuels décrivent la seconde voie, le code
la première.

Deux dents complémentaires sont importantes avant une exécution device, sans
rouvrir l'algorithme des kernels :

- **valider le retour D2H avant de reconstruire.** Le raccord vérifie quelques
  égalités, puis utilise directement `n_int`, `n_shell` et les ids comme bornes
  et indices. Une écriture device omise ou corrompue peut donc conduire à une
  lecture/écriture hors borne avant le refus. Centraliser un validateur qui
  exige un statut connu, `n_int <= 9`, `n_shell <= 12`, `cand_idx` global
  attendu et tous les ids dans `[0,n_upos)`, puis seulement construire
  `BallData`. Préremplir chaque sortie avec une sentinelle et ajouter au moins
  un mutant `skip-write`/`n_shell` hors domaine rend cette frontière causale ;
- **ne pas confondre parité de calcul et complétude des écritures.** Les portes
  stub initialisent déjà certains champs, mais la porte CUDA alloue les
  sorties non initialisées et ne possède pas de dent d'écriture omise. La
  comparaison au scalaire est utile et les quatre mutants actuels sont bons ;
  elle doit être précédée du contrôle de sentinelles pour que toute boule et
  tout champ aient effectivement été produits.

Photographie WIP locale : après reconfiguration Release, 10 des 11 CTests
`wire|census_device_stub|census_stub_mutant|pilot_stub` passent en 96,67 s ;
le seul rouge est le nominal `mhgp6_wire`, volontairement bloqué par
`GRAVE_AU_PREMIER_RUN`. Les quatre mutants census et les deux mutants pilote
meurent comme attendu. C'est un bon signal fonctionnel, pas une réception :
la source n'est pas épinglée et le digest wire manque encore.

La matrice locale `matrice_fold_locale_20260901` ne doit pas décider le design
A. Ses deux premières cellules, qui ne changent que `fold_inflight`, font déjà
varier des étages antérieurs au fold d'environ 33--37 %. De plus, notre passe
CTest ciblée ci-dessus a chevauché la cellule `ref_uniform16000_t8_i2_j0` :
cette cellule est explicitement contaminée par une charge CPU concurrente.
Le harnais vivant n'agrège pas les codes de sortie en statut terminal, ne
rehache les binaires partagés qu'au début et n'effectue ni répétitions ni
contrebalancement. Le sauvetage utile est borné : archiver le harnais, graver
hashes et charge de fin, contrôler tous les `rc`, fermer par
`invalid_for_performance` (ou `interrupted`) et conserver le lot comme
contre-reçu de bruit/digests seulement. Une future mesure décisionnelle devra
utiliser des copies privées des binaires, au moins trois répétitions
contrebalancées et une règle d'invalidation sur les étages amont. Il n'y a
aucun intérêt à payer les cellules 50k de ce reçu déjà réfuté.

Le plus petit chemin utile est donc C2 vert et transactionnel, puis stub C3/C4
sur vues typées, puis seulement la couture C5. Si C5 reste dans le même WIP,
préserver aussi la classe du refus : un `invalid_input` du wire ne doit pas
être requalifié silencieusement en `resource_exhausted`. Aucun GO G4 n'est
ouvert par cette réponse.

## 6. Ordre de travail recommandé

1. **achevé à `4a85c13d`** : C1, garde `2E` et témoin hôte ancrés ensemble,
   brouillon `fold.hpp` correctement laissé hors checkpoint ;
2. tuer les deux contre-fixtures causales du profil au § 5.10, aligner ses
   libellés, puis exécuter B/inflight avant de choisir le design A ;
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
