# Contre-audit de la recette `session_axis_top8_g4.sh`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot : `HEAD=840a2e28679aa3e5e3d8ec706daa680a52ac1bde`, fichier
`gcp-migration/session_axis_top8_g4.sh`, SHA-256
`a2f67c33503fc1db8b3c3faa05a6d2b172458bd762392e51a9555dc556d1ebf0`.

GCP n'a pas été utilisé par l'auditeur. La lecture initiale est statique et la
recette ne doit pas être relancée avant réparation de la fermeture ciblée
ci-dessous.

### Addendum d'observation concurrente

Après cette lecture, un transcript non suivi est apparu dans
`receipts/axis_top8_g4_20260814/`, SHA-256
`452fc650a11e79feb7452454d403e27a82aa38e009db5acce9cebae359b01981`.
L'auditeur n'a ni lancé ni arrêté cette session. Le transcript montre un démarrage externe de la génération
`2026-08-14T13:00:56.283-07:00`, puis l'échec de certification du
`terminationTimestamp`. Le garde interne de `start_and_verify.sh` a arrêté
**cette génération exacte** et certifié `TERMINATED`. Le trap externe a ensuite
effectivement exécuté sa branche sans génération ; cette fois la cible était
déjà arrêtée, mais le risque décrit ci-dessous est donc un chemin exécuté, pas
une hypothèse. Aucun build, sweep, transfert de phase ou résultat GPU n'a été
produit par cette tentative.

## Verdict

La recette est correctement étiquetée comme campagne CPU de réfutation sans
CUDA ni SLO. Elle n'est néanmoins pas recevable en l'état : son trap peut
arrêter une génération qu'elle n'a pas démarrée, sa matrice séquentielle n'est
pas compatible avec son propre coupe-circuit de 55/75/90 minutes, et son verdict
ne couvre pas les P0 sémantiques du noyau.

## P0 sécurité — arrêt non versionné quand `GENERATION` est vide

Le trap est armé avant `start_and_verify.sh`, avec `GENERATION=""`. Si le
démarrage refuse une VM déjà active, si le handoff est absent ou illisible, ou
si son parsing échoue, la branche de secours appelle
`stop_and_verify.sh --yes` **sans** `--expected-last-start-timestamp`. Le script
d'arrêt vérifie alors le nom et le label, mais aucune génération ; il peut donc
arrêter une session préexistante ou concurrente sur la cible par défaut.

Cette branche contredit à la fois le commentaire « exactement la génération
qu'elle a démarrée » et la règle impérative de non-mutation des autres sessions.
Elle annule aussi la protection déjà présente dans `start_and_verify.sh` : ce
dernier refuse précisément un arrêt automatique non versionné lorsque la
génération démarrée reste inconnue.

Réparation requise avant exécution : état explicite `start_attempted` /
`target_generation_known`, lecture et validation du handoff dans le trap, puis
arrêt uniquement avec l'horodatage exact. Si aucune génération ciblée ne peut
être prouvée, la recette doit signaler le projet, la zone, le nom, le dernier
état connu et la commande de contrôle ; elle ne doit jamais appeler l'arrêt
non versionné. Un échec antérieur à toute tentative de démarrage ne doit appeler
aucun arrêt.

Le transcript est en outre copié avant que l'erreur finale d'arrêt non certifié
soit ajoutée au journal. Le reçu copié peut donc omettre le fait bloquant ; la
copie finale doit suivre toute décision de cleanup.

## P0 preuve négative — le verdict précède le rapatriement

Les sorties brutes `phaseA..D` restent sur la VM pendant la campagne. Le verdict
distant est exécuté sous `set -e` **avant** les quatre `scp`. Une réfutation
rend donc la commande SSH non nulle, déclenche le cleanup et empêche précisément
le rapatriement des fichiers qui l'expliquent. Le prochain `rm -rf ~/a8` les
détruit. Chaque phase doit être streamée ou copiée avant son verdict ; l'échec
est un résultat à conserver, pas une raison de sauter l'archivage.

Le chemin de reçu est fixe et n'est ni nettoyé ni versionné par génération. Un
transcript neuf peut ainsi cohabiter avec d'anciens `phase*.txt`. Il faut un
répertoire unique par génération/run, un manifeste de fichiers et leurs hashes.

## P0 faisabilité — 76 runs séquentiels sous un budget de 55 minutes

Les phases demandent 30 runs à `n=120`, 18 à `n=200`, 12 à `n=300` et 16 à
`n=200`, soit 76 runs séquentiels. Chaque run possède un timeout de 3 300 s,
alors que le calcul distant entier doit finir en 3 300 s, l'arrêt invité en
4 500 s et GCE en 5 400 s. La somme des timeouts autorisés atteint 250 800 s,
soit presque 70 h : aucune enveloppe de durée globale ne rend la matrice
terminable.

Le probe annonce lui-même une complexité exhaustive en puissance cinq. Par
rapport à `n=60`, les facteurs de travail nominaux sont 32 à `n=120`, environ
412 à `n=200` et 3 125 à `n=300`. Une seule taille maximale peut donc consommer
presque tout le budget. Il faut une rampe locale ou distante causale, un budget
global mesuré et un arrêt après le premier palier rouge ; les tailles/graines
suivantes ne sont ouvertes que si le débit observé prouve qu'elles tiennent
avant le coupe-circuit.

Le timeout individuel n'emploie pas `--kill-after` et ne constitue donc pas une
borne dure si le processus ne termine pas sur `SIGTERM`. Le deadline global
doit être monotone et inférieur à l'arrêt invité avec une marge explicite pour
le rapatriement et l'arrêt certifié.

## P1 — la campagne ne reçoit pas encore le contrat q4

`manquants=0` et `census_faux=0` ne comparent aujourd'hui que la complétude des
racines retenues et le **cardinal** intérieur. Ils ne jugent ni les listes de
vrais `PointId` de `I_B/U_B`, ni le shell `insphere_j==0`, ni `RelevantGP`, ni
le primary entre les deux `Q4Seed3`, ni la multiplicité globale des
`SupportKey`. La mort par gaps est encore posée après le sweep exhaustif dans
le probe commis. Une grande campagne ne peut pas compenser ces absences.

Les planchers par run sont tous nuls. Le verdict agrégé exige une masse shallow
globale, mais une famille ou un seuil individuel peut rester vide et être masqué
par les autres runs. Chaque famille/phase doit publier ses propres planchers de
faces, racines, événements, ties et morts, avec exception déclarée et vérifiée
pour `two_lines`.

Le parser du reçu ne compare pas l'ensemble exact des 76 tuples attendus. Un
champ absent vaut implicitement zéro dans la somme, et `len(juges)==len(codes)`
peut valider un sous-ensemble tronqué. De même, `ctest -R ... | tail -6` ne
conserve pas la sortie complète, n'exige pas `--no-tests=error` et ne vérifie
pas qu'il y a exactement 23 tests ; CTest peut rendre zéro quand la regex ne
sélectionne rien.

Enfin, le runner accepte un worktree sale, n'enregistre que son nombre de
fichiers modifiés, ne conserve pas le tar réellement envoyé et ne hash ni le
runner ni les sorties rapatriées. Le reçu doit déclarer une clé de run, le
commit, le manifeste exact du worktree, le tar/ELF/runner et un statut structuré
`completed`, `failed` ou `invalid`.

La bonne séquence est donc : recevoir localement IDs/shell, `DEAD_GAP`, overflow
fail-closed, primary/exact-once et options CLI ; ajouter au moins un CTest avec
seuil différent de sept ; mesurer une rampe bornée ; seulement alors produire
une recette G4 réaliste. Cette session restera un diagnostic CPU 48 cœurs, pas
une mesure GPU et encore moins le contrat bout-en-bout 50k sous une seconde.

## Addendum — successeur `session_q4seed_axis_topr4_g4.sh`

Snapshot statique au
`HEAD=d55bb9a7add87a54bbd500e323fe6fa5bf45c5a2`, SHA-256 du runner
`eabfdcd56503d60f1830539ee90c468f7e37863a4202139466856d70327b5d93`.
GCP non utilisé par l'auditeur.

Le successeur corrige deux points : aucune branche n'appelle plus un arrêt sans
génération, et la cible par défaut est la paire IA autorisée
`europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, où le garde peut certifier
l'échéance calculée si `terminationTimestamp` est entièrement absent. La
première tentative en zone standard a donc échoué fermé à cause du choix de
zone ; elle ne prouve aucune régression GCE.

Le nouveau runner ne doit pourtant pas être lancé avant les réparations
suivantes.

### P0 — verdict impossible et champs périmés

Le parser ne collecte comme juges que les lignes commençant par
`q4seed_axis_topr4`, mais ajoute aux `codes` les neuf runs `exact_once`. Si ces
runs terminent, `len(juges)==len(codes)` est faux par construction : le verdict
ne peut jamais être `ACCORD`.

Il cherche en outre les anciens champs `aigues_owner`, `bornes_cassees` et
`census_faux`. Le probe courant publie `seeds`, `bornes`,
`identites_fausses` et `gaps_faux`. Les métriques absentes valent silencieusement
zéro, la détection de famille vide cherche elle aussi l'ancien libellé, et
aucune grammaire ne compare les tuples attendus. Le verdict doit parser deux
types de records explicitement, exiger chaque champ, chaque clé
`(palier,famille,n,seed,smax)` et les neuf clés exact-once, puis rejeter doublon,
absence et ligne inconnue.

### P0 — une réfutation reste bloquée sur la VM

`rampe.txt` et `exact_once.txt` sont encore rapatriés **après** le verdict sous
`set -e`. Un verdict rouge saute donc les `scp` et détruit la preuve au prochain
`rm -rf ~/q4`. Les deux fichiers doivent être streamés ou rapatriés avant toute
décision. Le chemin de reçu reste fixe, sans clé de génération ni manifeste de
hashes.

### P0 — la rampe n'a pas encore de deadline global

`reste` est calculé une fois au début d'un palier puis réutilisé comme timeout
pour chacun de ses 18 runs séquentiels. Ce n'est pas une borne du palier. Les
neuf exact-once de 900 s chacun sont ensuite hors du budget de rampe. Un palier
rouge n'arrête pas immédiatement les suivants et `timeout` n'emploie toujours
pas `--kill-after`.

Il faut un deadline monotone absolu : recalcul du reste avant chaque run,
réserve fixe pour copie/cleanup, arrêt au premier code non nul, puis exact-once
ouvert seulement si sa borne totale tient encore. `RUN_TIMEOUT` doit être parsé
comme entier borné avant interpolation dans la commande distante ; aujourd'hui
une valeur d'environnement arbitraire devient du shell distant.

### P1 — provenance et cleanup

Le commentaire et la commande parlent encore de 23 tests alors que le pin en
possède 36. `ctest -R ... | tail -6` n'emploie pas `--no-tests=error`, ne vérifie
pas le cardinal et jette la sortie complète. Le tar envoyé n'est pas conservé,
le runner et les sorties ne sont pas hashés et un worktree sale reste admis.

En cleanup, le transcript est bien copié après l'appel d'arrêt, mais toujours
**avant** l'ajout de `[ARRET NON CERTIFIE]` lorsque `stop_rc!=0`. La copie doit
être la toute dernière opération après calcul et journalisation du statut
final.

Enfin, l'en-tête « ne modifie aucune garde » reste faux : la recette reconfigure
`maxRunDuration` et ajoute une clé OS Login expirante. Ces mutations peuvent
être autorisées et sûres, mais doivent être nommées exactement.

Indépendamment du runner, `33766f6` répare le P0 de capacité du census au
`3507b5e` et `a369452` ferme les replays `MORT_GAP`/deep. La session reste
interdite : sélection et replay ne valident toujours pas
l'injectivité/disjonction des `PointId`, tandis que les défauts propres au
runner ci-dessus sont inchangés. La suite fraîche passe `39/39`, ce qui reçoit
le probe borné et non une route 50k.
