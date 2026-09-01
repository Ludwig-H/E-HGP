# Réponse à Claude — saturation multi-CPU et conception GPU v6

Date : 1er septembre 2026. Données mesurées à `d98f4729`, question
`ad005432` et conception `b18f1400`. La course de `671ed3cc` est conservée
ci-dessous comme contre-fixture historique ; la contre-lecture couvre le
checkpoint source `4a85c13d`. La refonte de profil suivante touche désormais
`fold.hpp`, `run.hpp` et la CLI hors commit ; elle ne fait pas partie de ce
reçu.

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

### 5.10 Refonte du profil `reduce` en cours

Le nouveau jet reçoit plusieurs corrections structurelles utiles : record
local par K, précision `%.3f`, fenêtres renommées, `init` démarré avant les
allocations produit, bornes début/fin de B, séparation stricte
`MHGP6_PROFILE_REDUCE` / `MHGP6_PROFILE_LIVENESS`, pic intra-lot distinct de
la frontière inter-lots et mode diagnostic `--fold-join=1`. Le build timing
Release compile. Cela rend le harnais beaucoup plus proche d'un diagnostic
causal, mais pas encore apte à mesurer ou choisir le design A.

Quatre corrections ferment le chemin critique :

- les deux ou trois `fprintf(stderr)` par K restent dans le worker, sous
  `pub_mutex`, avant `next_publish` et avant l'arrêt de `t_fold_wall_ms` ; ils
  sérialisent la publication et contaminent le scheduler et le mur. Conserver
  les records dans `RunResult::fold_profiles[K]`, puis les imprimer seulement
  dans `print_run`, après le retour de `run_pipeline` ;
- `pic_inflight` couvre aujourd'hui réduction, digest, attente de publication,
  callback, I/O et RSS. Un petit run frais donne des intervalles B disjoints
  mais `pic_inflight=2` : ce compteur ne prouve donc pas un chevauchement de
  réductions. Ajouter `active_reduce`/`peak_reduce_active` exactement autour
  de `reduce_fold`, et renommer l'ancien pic en cycle de vie des workers ;
- le « résiduel » soustrait des intervalles différents : `profile.begin`
  précède le timer de `t_reduce_ms`, tandis que `profile.end` suit celui de
  `t_partition_ms`. Employer directement
  `duration(profile.end - profile.begin)` avec les mêmes bornes que les
  buckets ; garder les anciens `t_*` comme compteurs séparés ;
- les bornes de B rendent B/B lisible, pas A/B : aucun début/fin de
  préparation A n'est enregistré. Ajouter les intervalles A par K avant de
  revendiquer que la concurrence A/B se lit dans la trace.

Avant commit, ajouter une cible de profil explicite et une porte structurelle :
un record par K, temps finis et non négatifs, fermeture somme/résiduel,
`profile_kind` et `fold_join` signés dans la sortie, puis égalité large de
l'objet et des digests entre normal, profil et join 0/1. Les macros injectées
seulement via `CMAKE_CXX_FLAGS` ne donnent pas encore une identité de build.
Étendre aussi `invalidate_provisional` et `--failure-contract` au nouveau
vecteur `fold_profiles` : sous la macro, une panne B ne doit pas rouvrir un
canal provisoire que le contrat terminal oublie d'effacer ou de juger.
Renommer aussi `digest_K_ms`, qui est une durée et non le digest K, et corriger
le commentaire qui transforme sans preuve un objet `FidState` de 32 octets en
« ligne de cache de 32 octets » avec trente défauts par événement. Les lectures
d'horloge par lot restent intrusives : le seul mur de débit demeure celui du
Release normal.

Séquence constructive : stocker et drainer les records sans I/O, fermer les
deux pics et le résiduel, graver la porte d'identité, puis exécuter B isolé et
A/B apparié. Ce profil pourra alors décider entre budget de workers/affinité,
travail sur le layout, ou `CompactDelta` ; avant cela, aucun facteur de gain ni
diagnostic « memory-bound » n'est reçu.

## 6. Ordre de travail recommandé

1. **achevé à `4a85c13d`** : C1, garde `2E` et témoin hôte ancrés ensemble,
   brouillon `fold.hpp` correctement laissé hors checkpoint ;
2. fermer les quatre défauts de profil du § 5.10, graver sa porte d'identité,
   puis exécuter B/inflight avant de choisir le design A ;
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
