# Contre-audit B — correction du protocole v4

23 septembre 2026. Lecture initiale du worktree développeur, ensuite
publié **sans changement de ces trois octets** au commit `e54f727c` :
worker SHA-256 `a6715ac8…`, session `c7f5f32f…`, nouvelle porte réelle
`probe_worker_contract.py` `d3b48060…`. Aucun nouveau GCP lancé par
l'auditeur. Le paquet R2 antérieur, `0b29b6c3`, reste définitivement
`worker_failed/probe_failed` malgré ses treize sorties brutes.

Le correctif va dans le sens requis : plan v2 et sonde v4 épinglent
explicitement `saturate_deep` et `q3_leaf` ; le worker distingue la chaîne
MEB et son tableau des compteurs entiers ; il arrête les cas suivants
après le premier `probe_failed` ; les sous-chronos ne peuvent plus
dépasser le total, qui est lui-même borné par le mur externe du cas.
Cette dernière borne comporte toutefois **une marge absolue de 1 s**
(`validate_external_wall`) : elle ne permet pas de certifier à elle seule
un contrat de 1 s, encore moins de 100 ms. Le temps hôte et le temps
interne doivent être rapprochés avec une tolérance de mesure motivée et
publiés tous les deux, sans soustraire arbitrairement cette marge.
Une porte CTest lance une **vraie** sonde native et compare le résultat
on/off et onze mutations du protocole. Ces améliorations sont versionnées,
mais leurs portes complètes et leur raccord pré-G4 restent à vérifier sur
le commit ; aucune qualification G4 v4 n'en découle.

## Trois mutations encore acceptées

À cet état, `_tower_work` accepte des noms arbitraires d'entiers et ne
requiert que les deux champs spéciaux MEB. Sur une sortie factice v4
valide, `validate_probe` rend encore `complete_relative` après chacune
des mutations suivantes :

1. `meb_supports_by_size=[0]`, alors que la sonde native publie
   exactement quatre positions ;
2. ajout d'un champ inconnu `tower_work.extra=1` ;
3. suppression du compteur attendu `tower_work.records`.

Ces acceptations ne prouvent pas une erreur géométrique, mais empêchent
la réception autonome d'attester le schéma précis du travail FULL.
Définir le jeu exact des clés `tower_work`, la longueur exacte et le
domaine du tableau MEB, puis ajouter ces trois mutants à la porte réelle
et aux selftests. Les mutations déjà testées `saturate_deep` inversé et
`chain_total=0` sont correctement refusées.

La contrelecture après publication trouve quatre autres sorties
factices acceptées comme `complete_relative` : supprimer
`generator.q34_expanded_pairs`, supprimer `ledger.q3_seeds`, remplacer
`catalogue.by_qmin` ou `catalogue.by_shell` par `[]`. La sonde native v4
émet pourtant respectivement **7** clés `generator`, **38** clés
`ledger`, puis des histogrammes de **3** et **17** entiers. Le schéma
de réception doit fixer ces formes, refuser champs manquants/inconnus
et tester ces quatre mutations ; une simple vérification « tous les
champs présents sont entiers » n'atteste pas le travail mesuré. Ces
mutations n'impliquent pas que la sonde native ait elle-même omis un
compteur : c'est le **lecteur** qui les tolère.
Les sept mutants de cette section, appliqués à la valeur factice de
`tower_selftest_v9.probe_value`, ont été rejugés directement contre le
worker publié : les sept rendent `complete_relative` avec code de
sonde 0. Cette fixture factice ne publie elle-même que quatre champs
`ledger` et cinq cases `by_shell` ; il faudra la synchroniser avec le
schéma natif au moment de durcir le validateur, sans en faire l'autorité
du schéma.

## Préflight et coût G4

La nouvelle porte réelle est inscrite à CTest, mais ni le constructeur de
paquet, ni le contrôleur de session, ni le worker invité ne l'exécutent
obligatoirement avant les cas LiDAR. Une session payante pourrait donc
encore être lancée sans avoir passé précisément la porte qui aurait évité
R2. Exiger un reçu local sur le **snapshot commité** avant
`guarded_start`, ou exécuter ce petit cas dès le build invité avant le
premier cas coûteux ; l'échec doit clore la session avec capture et arrêt
ciblé. Les anciennes fausses sorties des selftests ne valent pas ce reçu.

Le script réel autorise 600 s à `subprocess.run` par cas, alors que son
CTest est plafonné à 300 s. Si CTest tue son parent, l'enfant natif peut
survivre. Fixer un délai interne inférieur au délai CTest et une fermeture
du groupe des processus ; un petit nuage de 360 sites ne justifie pas
une attente de plusieurs minutes. Cette précaution est distincte des
plafonds de recherche géométrique, qu'il ne faut pas réintroduire.

La vraie porte compare actuellement saturation/feuille **et** W1 contre
W2/FULL statique0 contre statique2. Elle vérifie utilement l'identité de
l'objet sur plusieurs voies, mais ne mesure pas causalement le seul
effet de q3 feuille. Pour une ablation de vitesse, faire des paires
identiques en entrée, W, s, K et politique FULL, en ne variant qu'un
levier à la fois ; inclure les coûts de coquille q3 et de balayage q4.
La porte actuelle exige cinq ordres et 1 000 boules, mais pas
`q3_leaf_censuses>0` ni une cellule saturée : son égalité on/off peut
être verte sans exercer les deux nouveautés visées. Ajouter des fixtures
non vacantes de chaque branche, dont la coquille q3 à quatre contacts
décrite dans le contre-audit B de la feuille.

Enfin, trois faiblesses plus anciennes demeurent hors de ce correctif :
le lecteur hôte accepte encore un `partial` avec zéro cas complet et code
de sortie 0, ne recalcule pas tous les temps/RSS résumés à partir des
fichiers bruts, et ne lie pas la provenance déclarée du reçu aux objets
Git lors de la réception. Aucune n'explique l'échec R2, mais elles
interdisent de lire tout `partial` comme preuve autonome de contrat.
En particulier, il conserve les `probe_i.summary.json` sans les relire :
les `elapsed_seconds`, `chain_total_ms` et `gnu_time_max_rss_kb` du reçu
ne sont pas recoupés avec stdout et GNU time à cette étape.
Un scénario local à six refus explicites sur le code commis reproduit
concrètement `status=partial`, `worker_status=partial`, **zéro cas complet**
et code de sortie hôte **0**, malgré un arrêt GCP factice correct. Ce
statut doit être distingué d'un succès de tour par les scripts d'appel et
les tableaux de résultats ; idéalement, un contrat sans aucun cas complet
doit sortir en erreur de qualification.

## Relecture après le commit

Le commit publié est `e54f727c` (mêmes hashes des trois fichiers cités).
Deux selftests ciblés, validation de sonde/temps et arrêt après défaut,
passent. Sur la suite Python de 18 tests, 17 ont passé et
`test_snapshot_from_commit` a échoué parce que `HEAD` a changé pendant
son exécution ; rejoué seul sur un `HEAD` stable, il passe. Cette course
de test n'est pas une réfutation du protocole, mais elle impose de
publier les reçus avec un commit stable plutôt qu'un résultat global
« 18/18 » déduit d'un rejeu favorable. Les trois mutants acceptés et
l'absence de préflight obligatoire demeurent dans les octets commis.
Un rejeu ultérieur complet dans le worktree détaché à `bb2c40dc`, HEAD
stable, a cette fois passé **18/18** tests en 50,706 s ; il ne tue pas les
sept mutants de schéma acceptés ci-dessus, absents de cette suite.

Le workflow CI v9 ne surveille, parmi les fichiers de protocole, que
`tower_worker_v9.py` : une modification de `tower_session_v9.py`,
`tower_snapshot_v9.py` ou `tower_selftest_v9.py` seule ne déclenche pas
ce workflow. Il exécute les CTests CPU, pas la suite Python de cycle de
vie factice. Ajouter ces chemins et ce test avant d'utiliser le statut
CI comme porte protocolaire complète.

## Nouvelle lecture du protocole v5 non commité

Le worktree développeur `build/v9-open-worktree`, base `bb2c40dc`, ajoute
un certificat de voies mortes et passe à `plan_v3`/`probe_v5`. La chaîne
plan → commande → CLI → option moteur → `options.q34_dead_lanes` de la
réponse est bien raccordée. Lecture du worker SHA-256 `4b08d1f6…` et
de la porte réelle `4bd185a0…` ; ce sont des **octets WIP**, pas un
nouveau reçu G4. Le journal local de CTest affiche bien les deux portes
normale/`-O` vertes et `mutants_killed=12/12`. Le nouveau mutant inverse
le drapeau de voie morte, mais les sept mutations de schéma ci-dessus
ne figurent toujours pas dans la porte.

Contre-épreuve sur le JSON de la **vraie sonde native v5** du petit nuage
de cette porte : la base et chacune des sept mutations restent acceptées
`complete_relative`, y compris `meb_supports_by_size=[0]`, un compteur
`tower_work` inconnu, l'absence de `tower_work.records` ou de compteurs
`generator`/`ledger`, et les deux histogrammes catalogue vides. Supprimer
**tous les nouveaux champs `ledger.dead_*`** est également accepté.
Ce défaut de réception n'est donc pas seulement celui de la fausse
fixture du selftest, qui garde elle-même quatre champs `ledger`.
Le nuage de la porte active réellement les nouvelles branches (des
cellules et des voies mortes non nulles), mais elle ne fixe pas un seuil
de couverture et son on/off change aussi W1/statique0 en W2/statique2 :
une égalité d'objet n'est pas une ablation temporelle de ce seul levier.

Le préflight natif reste non obligatoire avant le départ G4, et le
timeout enfant de 600 s reste supérieur au CTest 300 s. Aucun de ces
constats n'annule les douze refus effectivement vérifiés ; ils montrent
simplement que `12/12` n'est pas encore une porte complète pour les
compteurs de coût ou pour la prochaine session G4. Exiger le schéma
exact sur **sortie native réelle**, un reçu préflight du snapshot
commité, puis seulement des cas G4 payants.

## Actualisation WIP stricte à 01:42 UTC

Le constructeur a modifié le protocole **après** les octets ci-dessus.
Worker SHA-256 `255aeeed…`, session `8fb0e14b…`, selftest `8168577e…`,
sonde source `b800a092…` ; toujours **non commités** à cette lecture.
Le worker impose maintenant exactement sept clés `generator`, 48 clés
`ledger` (dont les 12 masses `dead_*`), 14 clés de catalogue avec
histogrammes de tailles 3/17, et 18 clés `tower_work` dont histogramme
MEB de taille 4 ; les entiers sont bornés à u64. La fixture factice a
été synchronisée. Sur le binaire natif **reconstruit** à 01:42,
`validate_probe` accepte la base et **refuse les sept mutants historiques**
ainsi que la suppression de tous les nouveaux `dead_*`. L'ancien refus
obtenu avec le binaire de 01:25 était un décalage source/binaire, pas un
défaut du schéma courant. Ces objections v4/v5 initiales sont donc
**corrigées dans la révision WIP**, sous réserve d'un snapshot commité
et d'un reçu de porte fermé.

Un préflight de 1 500 sites u18 est maintenant **obligatoire chez l'invité
après le build et avant `probe_0`**. L'hôte recalcule ses octets, commande,
sortie, durée et digest avant de recevoir la session ; c'est une bonne
protection contre la répétition des treize calculs R2 refusés. Elle a
lieu **après** `guarded_start` G4, donc n'évite pas le coût VM/build, et
sa commande n'a pas encore de plafond propre court : un blocage de cette
petite sonde peut consommer l'essentiel du budget invité avant échec.
La tolérance du rapprochement chaîne/mur externe passe de `1 s` à
`0,05 s` ; publier les deux temps reste nécessaire pour le jalon 100 ms.

Le CTest actuel affiche **106 tests exécutés PASS, un Disabled** : ne
pas écrire « 107 PASS ». Les deux portes de protocole normal/`-O` sont
parmi les tests exécutés. La porte a ensuite été renforcée (script
SHA-256 `14025f0f…`) et rejouée ciblée normal/`-O` à 01:43 :
**19/19 mutants tués** sur la vraie sonde reconstruite, dont les sept
historiques, et non-vacuité exigée des branches q3 feuille/atlas/voies
mortes. Ce ciblage est postérieur à la suite complète de 106 ; il ne la
remplace pas. Le statut de la suite Python de cycle de
vie entière et de l'exécution G4 sur ce snapshot restent séparés.
Rejeu direct de la suite Python actuelle : **20 tests lancés, 14 erreurs
en 4,111 s**, toutes à `snapshot.build('HEAD')` lors de la validation
du token `probe_v5` contre le `HEAD` produit encore v4 du worktree
détaché `bb2c40dc`. Le mode de selftest n'inclut pas la source produit
WIP dans son archive : c'est un refus attendu du paquet non commité,
pas un défaut géométrique ni un `20/20` réussi. Relancer sur un commit
produit cohérent avant tout départ G4.
La lecture des octets courants de `tower_session_v9.py` trouve aussi
deux corrections indépendantes : `validate_received` refuse maintenant
une campagne avec **zéro tour complète** et recalcule chaque résumé de
cas à partir de la sonde, de GNU time et de la ligne de commande ;
`require_committed_protocol` reconstruit le paquet depuis les objets Git
du commit déclaré avant `guarded_start`. Elles sont présentes dans le
WIP, mais leur suite de cycle de vie n'est pas encore jugeable sur un
snapshot cohérent. Un statut `partial` avec **au moins une** tour complète
reste possible et rend code 0 ; les tableaux de contrat doivent le
distinguer explicitement de `completed`.

Deux petites portes restent faibles dans ces octets : le mutant de mur
externe de `probe_worker_contract.py` soustrait **5 s** au temps chaîne
de sa toute petite sonde, fournissant une durée négative déjà refusée
par `_number`. Son succès ne tuerait pas un retour accidentel de la
tolérance à `+1 s` ; utiliser un cas synthétique à durée positive, par
exemple chaîne `1,5 s`, mur `1,4 s`, qui sépare `+0,05` et `+1`.
À la réception, l'hôte relit la sortie et le mur du préflight, mais pas
son stderr GNU time/RSS, pourtant jugé par le worker avant LiDAR.
Enfin, le workflow CI v9 ne surveille toujours pas `tower_session_v9.py`,
`tower_snapshot_v9.py` et `tower_selftest_v9.py` et n'exécute pas la
suite de cycle de vie. Ces écarts ne rouvrent pas les sept défauts de
schéma corrigés ; ils bornent la portée de la preuve protocolaire.
