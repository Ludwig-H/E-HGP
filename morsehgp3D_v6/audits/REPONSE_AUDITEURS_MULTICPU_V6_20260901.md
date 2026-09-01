# Réponse à Claude — saturation multi-CPU et conception GPU v6

Date : 1er septembre 2026. Données mesurées à `d98f4729`, question
`ad005432` et conception `b18f1400`. La course de `671ed3cc` est conservée
ci-dessous comme contre-fixture historique ; la contre-lecture couvre le
checkpoint source `4a85c13d`, puis le profil ancré à `1069bc20`. Ce dernier a
été rejoué depuis un export Git isolé : le WIP C2 postérieur n'entre donc dans
aucun verdict de profil ci-dessous. La série C a depuis été épinglée à
`cd606257`, puis sa matrice CPU directionnelle archivée à `62cd2e28` ; ces deux
objets ont des portées distinctes.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict constructif

**C1 hôte, garde logique 2E et témoin hôte partiel reçus à `4a85c13d` ; série
C C2--C5 reçue dans sa portée CPU/stub locale à `cd606257` ; aucun résultat
device ni GO G4 inconditionnel.**
Le septième jet ferme bien les deux courses C1 connues dans le code : la
panne fatale est confinée côté worker avant notification, et le passage
file→actif est linéarisé sous le mutex. Le hook et le mutant ciblent maintenant
la frontière exacte, les quatre signatures sont isolées et le flake du pic
série est réparé. Le § 5.6 reçoit ce C1 hôte au pin source `4a85c13d`.

Le témoin arithmétique hôte et la garde 2E ont aussi progressé jusqu'à une
portée utile et honnête. Ils ne constituent toujours ni une compilation
`nvcc`, ni une exécution device, ni une preuve C3. Le profil B est ancré et
fonctionnel. Le pin `cd606257`, postérieur au reçu de profil, rejette désormais
la fuite
`profil_*` sur `stderr` et exige une attribution brute non nulle ; il lui reste
seulement à recalculer indépendamment le champ imprimé `somme` depuis ses neuf
composantes avant d'utiliser la matrice comme attribution.

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

Mise à jour postérieure, désormais épinglée à `cd606257` : la seconde
contre-fixture est fermée par l'inspection explicite de `stderr`, et la porte
compilée exige
maintenant `pf.somme() > 0` sur chaque record. La première n'est fermée qu'à
moitié : `profil_gate.py` exige bien le champ imprimé `somme > 0`, mais ne le
recalcule toujours pas depuis `init`, `touch`, `pre`, `unite`,
`post_remplissage`, `materialisation_tri_copie`, `liveness`, `partition` et
`liberation`. Cette addition indépendante est le seul durcissement causal
restant ici. Les trois portes ciblées passent localement dans la campagne
24/24 rapportée au § 5.11 ; ce vert reçoit le harnais épinglé, pas encore son
usage comme attribution de performance.

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

### 5.11 Wire série C : décisions tranchées, portes locales vertes, mesure G4 encore fermée

Contre-lecture commencée sur le WIP C2 postérieur à `1069bc20`, puis fermée au
pin `cd606257`, sans GCP. Les trois verrous demandés ont une réponse courte :

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

   L'implémentation doit aussi éviter les deux débordements signés restés dans
   `-b` et `2*a`. Une construction bornée n'a besoin d'aucun signed `i128`
   intermédiaire : si `b>0`, les deux candidats rabattus valent zéro ; si
   `b=0`, ils valent 0 et 1 ; si `b<0`, calculer en `u128`
   `q=uabs128(b)/(2*u128(a))`, puis saturer `q` et `q+1` à 65535. Cela couvre
   même `b=INT128_MIN` et tout `a>0`, sans inventer un sous-profil de
   coefficients.
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

- empoisonner la construction dès le premier refus et rendre les appels
  suivants inertes. Vider physiquement les buffers partiels n'est pas une
  condition de réception si l'API documente qu'ils sont non consommables et
  si toutes les coutures testent `error` ; le demander serait inutilement
  pointilleux. Une porte valide→invalide→valide doit en revanche prouver que
  l'objet reste refusé et qu'aucun compteur publiable ne progresse ;
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

Une faute locale détectée pendant la lecture a été corrigée dans le WIP :
`census_device_gate.cu` demandait `nb` octets pour la copie D2H de `d_cand`
au lieu de `nb * sizeof(u32)`. La majeure partie de `h_cand` serait restée à
zéro et la porte nominale aurait produit un faux rouge sur G4. La taille est
désormais correcte et la sentinelle la couvre ; ne pas rouvrir ce point.

Photographie WIP locale antérieure aux corrections ci-dessous : après
reconfiguration Release, 10 des 11 CTests
`wire|census_device_stub|census_stub_mutant|pilot_stub` passent en 96,67 s ;
le seul rouge est le nominal `mhgp6_wire`, volontairement bloqué par
`GRAVE_AU_PREMIER_RUN`. Les quatre mutants census et les deux mutants pilote
meurent comme attendu. C'est un bon signal fonctionnel, pas une réception :
la source n'est pas épinglée et le digest wire manque encore.

La réponse live de Claude ferme correctement les points les plus importants :
six candidats `u32` bornés construits sans négation signée dangereuse, fixture
hors `i64` et `INT128_MIN`, round-trip `BallIn`, vues hôte décodées, dataflow
`census_all` et volumes assumés, copie `cand_idx` corrigée, validateur avant
reconstruction et deux mutants de frontière. Il ne faut pas rouvrir ces
décisions. Claude a ensuite fermé dans le WIP les deux dents alors signalées :
les sept sorties sont préremplies sur le **device** à chaque lot, et le
validateur impose désormais les ensembles de statuts propres au préfiltre et
au census. La porte CUDA faisait déjà le premier travail ; la généralisation
au pilote produit était bien l'écart. Cette observation préliminaire est
supersédée par la réception au pin du paragraphe suivant ; ces points ne sont
plus manquants.

Le pin `cd606257` ferme la validation
`count/h/masse`, la reconstruction transactionnelle dans des temporaires, les
dents sélectives de statut et de `count`, la scène tardive canarisée et le cas
vide de la route stub. Il a aussi ajouté la relecture intégrale des sept
tableaux dans la porte CUDA, gravé le digest hôte, borné le lot effectif, puis
ajouté les portes `--lot=17 --min-lots=2` et le mutant
`gpu-lot-base-reset`. La dent est causale : au second lot, le mutant publie de
nouveau les indices 0--16 et le validateur attend 17--33. Les binaires produit
ne portent pas `MHGP6_TESTING` et refusent toute injection ; seules les cibles
`_test` activent le mutant.

Sur la photographie ensuite épinglée à l'identique, une reconfiguration
Release suivie de 24 CTest ciblés passe **24/24 en 188,28 s** : wire nominal et deux mutants, census
stub et ses sept mutants, pilote stub nominal et cinq mutants, parsing/parité
du wrapper `.cu`, nominal multi-lots et mutant de base, puis les trois portes
du profil. Le test bout en bout `mhgp6_pilot_stub` passe en 161,16 s. C'est une
validation locale CPU/stub utile. Un second rejeu, depuis un
`git archive cd606257` dans un build Release neuf, passe la suite complète du
label `gate` **113/113 en 232,11 s** (1 168,42 s cumulées par CTest). La série C
est donc reçue dans cette portée hôte ; aucun `nvcc`, device ou reçu de
performance n'en découle.

Deux durcissements P2 restent raisonnables sans rouvrir cette réception.
D'abord,
`decode_index_wire` déduit le nombre de nœuds de `node_left`, puis lit
`node_right`, `node_first` et `node_last` sans vérifier leurs longueurs ; la
porte ne contrôle pas non plus explicitement ces trois tailles. Le seul
producteur actuel est le builder interne, donc ce n'est ni un P0 ni un motif
pour rouvrir les kernels, mais une validation commune des sept formes rendrait
le refus wire réellement total. Ensuite, la route device retourne sur
`nb_total==0` sans publier les sorties vides, contrairement à la route stub
désormais corrigée. Aligner ce cas rend le callback directement substituable.
Les constantes inutilisées `kWirePrefilterOutBytes=12` et
`kWireCensusOutBytes=92` doivent enfin devenir 9/91 ou être supprimées : elles
contredisent autrement le wire SoA réellement consommé.

La dernière fermeture **avant un reçu G4**, et non avant la parité locale,
porte sur la mesure. Les copies actuelles valent 112 octets d'entrée + 100
octets de sentinelles H2D par candidat, soit 4 583 965 760 octets à 21 622 480
candidats, plus l'index ; le D2H vaut 2 162 248 000 octets. Le pilote utilise
un seul jeu de tampons de 212 octets par boule, réutilisé sur des lots
séquentiels ; 424 octets décrivent seulement un futur double tampon. De plus,
`h2d_ms` englobe actuellement l'initialisation du contexte et les allocations
initiales. Séparer `setup_alloc_ms` des seuls `cudaMemcpy`, ou renommer
honnêtement la métrique, est requis avant de lui attribuer un temps H2D. Le
RAII complet, le plafond extrême de grille et un budget VRAM agrégé restent de
bonnes défenses P2, mais ne doivent pas retarder seuls le pin fonctionnel.

Aligner `GPU.md` sur ces volumes, borner son plafond d'Amdahl à `uniform`,
ajouter les cinq champs du cadre et supprimer la note Claude C2 transitoire
suffit pour la documentation ; nul besoin d'un nouvel audit.

La matrice locale `matrice_fold_locale_20260901`, archivée par `62cd2e28`, est
mécaniquement terminée et hashée, mais ne doit pas décider le design A. Ses
deux premières cellules, qui ne changent que `fold_inflight`, font déjà varier des étages antérieurs au fold
d'environ 33--37 %. De plus, notre passe CTest ciblée antérieure a chevauché la
cellule `ref_uniform16000_t8_i2_j0` : cette cellule est explicitement
contaminée par une charge CPU concurrente. Le reçu porte l'intitulé honnête
`diagnostic_non_decisionnel`, mais le dossier ne conserve pas le harnais
exécuté et n'effectue ni répétitions ni contrebalancement. Sa lecture
`CompactDelta` est une bonne priorité de sonde, pas une décision acquise. Une
future mesure décisionnelle devra utiliser des copies
privées des binaires, au moins trois répétitions avec un ordre global
préenregistré et une règle d'invalidation sur les étages amont.

Le chemin local C2→C5 est désormais acquis. Le prochain travail utile ne porte
plus sur les kernels mais sur un protocole de mesure falsifiable ; la réponse
à la demande de GO `9c5517c9` est donnée ci-dessous.

### 5.12 Demande G4 série C `9c5517c9` : GO conditionnel, aucun démarrage au pin courant

La demande est légitime et l'autorisation exploitant/SPOT est acquise. Le
profil proposé ne peut toutefois pas être lancé au pin courant :
`g4_serie_c_v1.env` déclare lui-même que ses clés `GPUV6_*`, `MATRICE_*` et
`ATTRIB_*` ne sont consommées ni par le runner ni par le validateur. Une
session maintenant produirait donc soit zéro run de ces phases, soit des
sorties hors manifeste impossibles à recevoir. Ce **NO START** est borné au
protocole actuel ; il ne remet pas en cause le GO scientifique de la série C.

Réponse aux trois questions de Claude :

1. **Profil dédié `g4_serie_c_v1`, dans le cycle de vie v6 existant.** Étendre
   le runner, le validateur, le pin de protocole et leurs selftests, mais
   réutiliser impérativement `session_campagne_v6_g4.sh`,
   `v6_session_lifecycle.sh`, `start_and_verify.sh` et
   `stop_and_verify.sh`. Ne pas créer un lanceur brut. La v6 CPU du même pin
   est la référence appariée du pilote ; les phases différentielles v5 de ce
   profil dédié deviennent `aucun`, la v5 restant historique.
2. **Pas de point 200k dans la première session.** Il ne conditionne ni la
   parité device, ni le verdict 50k, et multiplie une résidence hôte non encore
   budgétée. Le rouvrir dans la même session seulement comme point optionnel à
   une répétition, après scellement des quatre familles 50k et si le deadline
   laisse la marge d'arrêt/rapatriement, demanderait un plan et un budget
   mémoire explicites ; le chemin sobre est de le différer.
3. **Le reçu doit ajouter causalité de mesure et fermeture temporelle.** Les
   signatures arch/device, la relecture d'index et la parité sont nécessaires
   mais insuffisantes ; les exigences ci-dessous sont la fermeture minimale.

Avant le nouveau pin, trois corrections courtes suffisent :

- séparer `setup_alloc_ms` du temps des seuls `cudaMemcpy` H2D, puis imprimer
  `nb_total`, `lot_effectif`, octets index, boules et sentinelles séparément ;
  le validateur recalcule `H2D = index + 212 * nb_total`, puis
  `D2H = 100 * nb_total`, pour l'implémentation actuelle ;
- ne plus mesurer systématiquement CPU puis device. Ajouter un ordre explicite
  `cpu-device` / `device-cpu`, exécuter un échauffement non retenu, puis quatre
  répétitions mesurées contrebalancées ABBA. Le validateur exige les indices de
  répétition, `parite=OUI` à chacune et refuse toute répétition absente,
  dupliquée ou non finie ;
- recalculer dans `profil_gate.py` la `somme` depuis les neuf composantes avant
  toute attribution de la matrice CPU. Joindre les petites cohérences
  documentaires du § 5.11 au même commit, sans refactorer les kernels.

Le profil `e8289d9a` doit aussi corriger deux faits avant d'entrer au manifeste.
Le CMake courant enregistre exactement **16** portes `gpu` — quatre témoins,
huit census et quatre pilotes — et non 17 ; le validateur doit exiger leurs
noms exacts, jamais seulement un plancher. Ajouter une cinquième porte témoin
n'est pas requis pour cette session. Ensuite, le point CPU 50k promet
`avec/sans --digest` mais n'annonce actuellement que `avec` : le plan effectif
doit porter les deux bras et une expansion de séquence non ambiguë. La même
porte grave l'affinité réelle des 24 cœurs physiques et des 48 fils SMT ; le
seul argument `--threads` ne la prouve pas.

Le profil de campagne peut être nettement plus court que le produit cartésien
proposé. Préenregistrer des contrastes explicites suffit : T dans
`{16,24,32,48}` à `inflight=2, join=0, digest=off` ; inflight dans `{1,2,4}`
à T=24 et 48 ; join 0/1 à T=16 et 48, inflight=2 ; digest off/on à T=48,
inflight=2 pour les deux joins ; enfin le point 50k T48/inflight2/join0/1,
digest off/on. Trois répétitions par point, ordonnées par une séquence globale
préenregistrée aller/retour/cyclique, plus les quatre attributions déjà
proposées, répondent à l'arbre de décision sans payer deux fois tous les axes.
Cet ordre de matrice n'est pas nommé ABBA ; ABBA désigne seulement les quatre
paires CPU/device du pilote.

Ordre des phases sur la VM : matrice CPU et attribution d'abord, avant tout
build `nvcc`, conformément à la doctrine du runner ; chaque phase reste
fail-fast. Construire ensuite CUDA, puis exécuter `ctest -V -L gpu` sur
l'inventaire exact des 16 noms afin que les signatures des tests verts soient
archivées ; au premier rouge, aucun pilote. Les quatre familles 50k suivent
avec un warm-up exclu et quatre répétitions mesurées ABBA. Le contrôle
multi-lots utilise le lot produit par défaut avec `--min-lots=2` : à environ
21,6 millions de candidats il exerce déjà plusieurs lots. **Ne jamais employer
`--lot=17` à 50k**, qui provoquerait environ 1,27 million de lancements ; cette
valeur reste réservée à la petite porte n=400. Chaque run conserve commande,
code, stdout/stderr, GNU time/RSS, hashes binaires, charge/affinité/NUMA,
versions driver/nvcc, UUID et compute capability, température/horloges avant
et après, ainsi que les chronos bruts et leur fermeture contre le mur d'étage.
Le validateur doit posséder des contre-fixtures pour plan manquant, mauvaise
parité, répétition manquante, mauvais ordre, compte d'octets falsifié, chrono
non fini et troncature deadline.

Une fois ce protocole commité sur `main`, ses selftests verts et le pin local
rejoué, le **GO G4 devient exécutable sans nouvelle discussion de design**,
mais seulement après un accusé bref qui grave le SHA d'exécution exact et ces
résultats. Ce GO ne se transfère à aucun `HEAD` ultérieur : une seule VM
`g4-standard-48` SPOT, `instanceTerminationAction=STOP`. Le nouvel estimateur
doit prouver que le plan obligatoire et sa marge tiennent avant le démarrage.
Préférer 5 h GCE (`maxRunDuration=18000`) et 270 min invité si le profil réduit
tient ; sinon réduire encore les tâches optionnelles. Le plafond exceptionnel
de cette session est 7 h GCE (`25200`) et 415 min invité, jamais le 8 h par
défaut ni un dépassement silencieux. La deadline de campagne garde au moins
30 min pour rapatriement/arrêt. Le démarrage passe exclusivement par
le script gardé et l'arrêt du projet/zone/instance/génération exacts est
certifié `TERMINATED`, succès ou échec. Les autres VM sont seulement signalées.
Aucun résultat de cette session ne changera `public_status=not_claimed` sans
audit du reçu.

### 5.13 Checkpoint `d5ed0fb3` : pilote mieux mesurable, pin de session encore fermé

Le commit reçoit utilement les trois corrections locales demandées, sans
prétendre être le pin de session. `setup_alloc_ms` isole désormais les
allocations initiales des copies H2D ; index, boules, sentinelles, D2H,
`nb_total` et lot effectif sont publiés séparément. Le warm-up porte
`retenue=NON`, puis les quatre répétitions exécutent réellement ABBA dans les
deux sens possibles. Un export Git propre construit le profil et le vrai
fichier `.cu` en stub C++, puis passe les cinq portes touchées **5/5 en
9,00 s**. Claude rapporte aussi 113/113 `gate` sur son build. Aucun `nvcc` ou
device n'a été exercé.

Il n'est pas utile de rouvrir les kernels. Quatre coutures courtes doivent en
revanche précéder le futur accusé de pin :

1. La ligne du pilote ne publie que `parite=OUI` et le `digest_all` CPU, alors
   que le code compare aussi les deux `digest_balls`, `digest_postprefilter`,
   vecteurs de digests forêt, cartes, événements et émissions. Publier les
   deux signatures d'une projection canonique couvrant exactement ce tuple,
   ou les champs CPU/device séparés, permet au validateur de recalculer
   l'égalité ; le booléen imprimé reste une redondance, jamais l'autorité.
   Ajouter au juge stub les cinq records exacts, la séquence ABBA/BAAB, les
   formules d'octets, les temps finis et les lots rend ces nouveaux champs
   causaux avant G4.
2. Le recalcul des neuf composantes est présent, mais ses seuils `0.009` et
   `0.014` sont trop larges. La contre-fixture exécutée avec neuf composantes
   à zéro, `somme=0.008`, `residuel=0.012` et `mur_reduce_interne=0.020` rend
   encore `COUNTERFIXTURE_ACCEPTED`. Avec les impressions `%.3f`, des seuils
   proches de `0.0051` pour la somme et `0.006` pour la fermeture couvrent les
   arrondis. Une petite contre-fixture Python autonome doit tuer cette scène.
3. Le profil dédié garde huit `CONF_SPECS` v5/v6 et une séquence
   `aller retour aller`. Conformément au § 5.12, mettre la phase
   différentielle à `aucun` et nommer la troisième permutation cyclique
   exacte, par exemple une rotation fixe des 16 points. La note Claude reste
   aussi à marquer supersédée ou à corriger dans sa seconde moitié, où 17
   portes, trois répétitions, 200k et le produit cartésien sont encore décrits.
4. Le runner doit appliquer et attester l'affinité demandée — un `taskset` de
   la commande dérivé de la topologie physique/SMT, pas l'affinité inchangée
   du shell — et passer explicitement `--repeat=4`, `--ordre=cpu-device` et
   `--min-lots=2`. Les axes de durée du profil doivent piloter les vrais
   `MAX_RUN_SECONDS`/`GUEST_SHUTDOWN_MINUTES` avant tout garde-fou ; profil,
   manifeste, estimateur, transport, validateur et deux selftests restent à
   fermer ensemble.

Le statut demeure donc **NO START à `d5ed0fb3`**. Cette réponse est une aide
directe au prochain commit : une fois ces coutures intégrées et falsifiées,
seul l'accusé bref du SHA exact restera avant le démarrage gardé.

### 5.14 Contre-lecture coopérative du WIP postérieur : noyau local corrigé, intégration encore NO START

Cette photographie porte sur un worktree **non épinglé** postérieur à
`d5ed0fb3` ; elle aide à finir le commit, mais ne reçoit donc encore aucun
nouveau pin. Les corrections locales vont dans le bon sens et ne sont pas à
rouvrir : signatures CPU/device séparées sur exactement le tuple comparé,
juge commun des cinq records et de l'ordre ABBA/BAAB, seuils de profil
`0.0051`/`0.006` avec contre-fixture, `CONF_SPECS=aucun`, troisième passage
`rotation8`, durées de session lues avant les garde-fous, et `taskset` appliqué
aux murs matrice comme aux attributions. Les contre-fixtures pilote et profil
passent sous `python3` et `python3 -O`; la syntaxe Bash/Python et
`git diff --check` sont vertes sur cette coupe.

Quatre coutures matérielles restent, avec une fermeture courte pour chacune.

1. **Le profil et son juge ne sont pas encore dans le pin.**
   `g4_serie_c_v1.env` manque des deux listes `PROTOCOL_FILES`; il n'est donc
   ni matérialisé ni revalidé et `CAMPAIGN_PROFILE=g4_serie_c_v1` refuse avant
   GCP. De même, le validateur cherche
   `pinned/morsehgp3D_v6/tests/pilote_juge.py`, que le pin ne copie pas.
   Ajouter ces deux fichiers aux inventaires normatifs dans le même ordre,
   créer leur répertoire parent pendant la matérialisation, les couvrir par le
   selftest du manifeste et refuser une modification isolée de chacun. Le
   lifecycle commun construit encore et teste systématiquement la v5 alors
   que l'utilisateur a borné ce travail à la **v6** et que conf/bench/GPU-v5
   valent `aucun`; rendre ce préflight conditionnel et explicite évite de
   repayer une lignée seulement historique. Ne pas seulement masquer son
   plancher.
2. **Le profil 5 h se refuse lui-même.** Le recalcul exact du modèle courant
   donne matrice `3 084 s`, attribution `488 s`, build+portes CUDA `5 400 s`,
   pilotes `3 000 s` et overhead `580 s`, soit `ESTIMATE_S=12 552 s`. Avec
   `18 000 s`, arrêt invité à 270 min, réserves `4 805 s` et build général
   `900 s`, `WINDOW_S=12 295 s` : le preflight refuse de **257 s avant toute
   mutation**. Le coefficient est déjà optimiste, car chaque répétition
   crédite la route device à seulement `0,5` route CPU alors que son gain est
   précisément inconnu. Si les 16 points, quatre attributions et quatre
   familles restent obligatoires, la solution honnête est l'enveloppe
   exceptionnelle déjà autorisée de 7 h/415 min, avec un budget device au
   moins égal à une route CPU avant mesure; sinon retirer une tâche réellement
   optionnelle. Ne pas faire passer 5 h en abaissant arbitrairement le modèle.
   Graver le calcul du profil canonique dans un selftest.
3. **Le juge doit agir avant la famille suivante et fermer les chronos.** Le
   runner ne regarde encore que `code=0`; un stdout falsifié mais un code nul
   consommerait les autres pilotes, puis serait refusé seulement après l'arrêt.
   Ajouter un mode fichier au même `pilote_juge.py` et l'appeler après chaque
   pilote. Le juge doit aussi exiger, avec la tolérance d'impression `%.1f`,
   `route_device_etage_ms + 0.4 >= wire + setup_alloc + h2d + kernels + d2h + rebuild`,
   puis que les murs CPU/device enveloppent leurs sous-étages. Une
   contre-fixture où six composantes totalisent 1 000 mais l'étage vaut 1 doit
   mourir. Stabiliser aussi signature, `nb_total`, octets et lot effectif entre
   répétitions d'une même entrée.
4. **Le reçu doit juger les surfaces selon leur nature.** Le contrôle exact
   des 16 noms GPU doit précéder `ctest -V`, pas seulement découvrir un 17e
   test après son exécution. Dans le validateur, ne pas appliquer le filtre
   produit générique `FORBIDDEN` au transcript CTest agrégé : la porte
   négative attendue `mhgp6_pilote_refus_n` écrit légitimement `REFUS` tout en
   étant `Passed` — fait reproduit localement. Accepter aussi les deux résumés
   CTest déjà admis par le lifecycle. Forcer `nounits` sur les snapshots
   `nvidia-smi`, pinner le juge importé, exiger la finitude et l'ensemble exact
   des K de l'attribution, puis conserver les hashes des binaires exécutés et
   copier `matrice_resume.txt`/`gpuv6_resume.txt` dans le reçu durable. Enfin,
   dériver l'affinité par `(socket, core)` dans le cpuset autorisé, et faire
   recalculer par le validateur le même masque pour matrice et attribution.

Ordre de fermeture conseillé : pin profil+juges et portée v6-only ; budget
canonique ; juge fail-fast/fermeture/inventaire pré-exécution ; validateur et
deux selftests ; enfin export propre du SHA candidat. À ce moment seulement,
un accusé bref pourra rendre le GO conditionnel exécutable. **Aucune session
GCP ne doit partir de cette photographie WIP.**

## 6. Ordre de travail recommandé

1. **achevé à `4a85c13d`** : C1, garde `2E` et témoin hôte ancrés ensemble,
   brouillon `fold.hpp` correctement laissé hors checkpoint ;
2. **partiel à `d5ed0fb3`** : recalculer la `somme` depuis ses neuf
   composantes ; resserrer maintenant sa tolérance et graver la contre-fixture
   du § 5.13, puis réserver la matrice locale au diagnostic de bruit ;
3. **achevé à `cd606257`** : recevoir le wire et la couture C2--C5 dans leur
   portée CPU/stub ; joindre les durcissements P2 au prochain petit correctif ;
4. si la cause CPU est la concurrence A/B, prototyper un budget de workers ou
   une affinité reproductible avant l'éclaireur atomique ;
5. ouvrir l'amont/aval du design A par les paliers décrits, sans promettre le
   facteur `1,7–2` ;
6. rendre exécutable et falsifiable le profil dédié du § 5.12, puis appliquer
   son GO conditionnel sous nouveau pin, avec arrêt GCP ciblé certifié.

Un prototype de réduction ne sera reçu ni par un seul digest ni par un mutant
d'ordre. Il devra comparer le `ForestResult` complet, les deltas et niveaux de
lots, les partitions finales, les cardinalités et les statuts sur des fixtures
avec connexions intra-lot et inter-segments, puis prouver l'identité 1/T fils.

GCP non utilisé par cette réponse.
