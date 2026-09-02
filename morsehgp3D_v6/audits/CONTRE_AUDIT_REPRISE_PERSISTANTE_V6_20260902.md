# CONTRE-AUDIT — reprise persistante `c8f69673`

Date : 2 septembre 2026. Coupe jugée : `c8f69673` dans le `HEAD` courant.
Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict constructif

La base 0700, les deux marques du vrai garde, l'authentification en deux
étages, la génération portée partout et la reprise sans `START` sont des
fermetures réelles. Le selftest exact de `c8f69673` sort avec succès. La
session `1788312873` montre aussi que les deux marques ont été effectivement
émises avant un arrêt ciblé certifié.

La reprise n'est toutefois pas encore une porte autonome pour une nouvelle
dépense. Les coutures ci-dessous se ferment localement, sans VM et sans
rouvrir les résultats device.

## Priorités de sûreté

1. **Le verrou `reprise.pid` n'est pas atomique.** Le code teste le fichier,
   retire un verrou jugé périmé, puis l'écrit avec une redirection ordinaire.
   Deux processus peuvent franchir ensemble la fenêtre
   test/suppression/écriture. Le balayage de processus ne sauve pas le cas de
   deux reprises lancées depuis la même session POSIX, car elles s'excluent
   mutuellement du balayage par `sid`. Utiliser un verrou kernel conservé
   jusqu'à la sortie (`flock -n` sur un descripteur ouvert dans `WORK`) ; le
   PID/starttime devient alors un diagnostic, pas la primitive d'exclusion.
   Une exécution simultanée du fragment exact depuis un verrou périmé a laissé
   passer 256 acquisitions sur 256.
2. **Un faux terminal `targeted_stopped` peut conclure sans arrêt.** Le parseur
   du registre accepte `state=targeted_stopped` avec `generation=` vide. La
   boucle de concordance ignore cette valeur, puis peut prendre la génération
   d'un handoff ou d'une marque et emprunter le chemin « déjà certifié » : zéro
   `STOP`, purge et témoin terminal. Exiger une génération non vide pour tout
   état qui implique une cible démarrée, comme le fait déjà le parseur du
   cycle de vie ; sinon rendre 71 sans conclusion ni purge.
3. **Le `describe` ne lie pas assez le SCP à la génération.** Il lit bien
   `status,lastStartTimestamp`, mais la condition de rapatriement ne teste que
   le préfixe `RUNNING`. La contre-fixture
   `RUNNING<TAB>generation-concurrente` déclenche effectivement `compute scp`
   avant que le garde d'arrêt refuse la mauvaise génération. Parser le tuple
   exact et exiger l'égalité avant le SCP, rapatrier dans un staging, puis
   relire la génération après le SCP avant de promouvoir les données. Lier
   aussi `REMOTE_DIR` à `SOURCE_COMMIT` et à l'époque de la génération. Toute
   divergence rend 71 et détruit seulement le staging local.
4. **Les rejeux après arrêt incertain ne sont pas `stop-first`.** À l'entrée
   avec `targeted_stop_failed` ou `targeted_stopping`, le code peut refaire
   `describe` et SCP avant une nouvelle tentative de certification. Après un
   échec courant de `stop_and_verify`, il lance aussi le validateur : la
   contre-fixture `stop_rc=9` crée bien `validation.txt`. Router ces deux états
   directement vers un seul `STOP` sérialisé. Tant que l'arrêt n'est pas
   certifié, aucun SCP, validateur, copie récursive, hash massif ou `sync` ne
   doit retarder le retour 70 ; au besoin, ne produire qu'un témoin d'échec
   minimal et borné.
5. **Une garde externe non épinglée reste exécutable.** Lorsque `GUARDS_DIR`
   diffère du répertoire épinglé, le code affiche « selftest seulement », puis
   exécute tout de même ce `stop_and_verify.sh`. Le caractère “selftest” n'est
   établi par aucun prédicat. Le chemin production doit exiger exactement la
   copie ré-authentifiée. Le harnais peut exercer la vraie garde avec un faux
   `gcloud`, sans persister d'échappatoire dans `session.env`.
6. **Le témoin terminal précède une purge dont les échecs sont masqués.** Le
   bloc écrit `recu_publie`, tente `rm`/`shred`, puis finit par `true`. Une
   panne ou un arrêt du processus après le témoin interdit toute reprise alors
   que les credentials peuvent subsister. Sur l'état déjà
   `targeted_stopped`, purger et vérifier d'abord, puis publier atomiquement le
   témoin et synchroniser son répertoire parent. Une injection faisant échouer
   `rm` et `shred` reproduit actuellement `rc=0` avec `recu_publie`, clé privée
   et configuration GCloud encore présentes. Le cycle de vie nominal porte le
   même ordre et doit recevoir la même correction. En cas d'échec, laisser une
   reprise locale de purge rejouable sans nouvel appel GCP.

Le contrôle de vivacité doit aussi porter l'identité de la session créée par
`setsid`, pas seulement le PID principal et les argv contenant `WORK`. Graver
SID/PGID et refuser tout membre vivant ferme le fils orphelin dont le chemin
n'apparaît plus dans sa ligne de commande. Enfin, les marques ne sont pas
encore parsées comme des objets stricts : type régulier, keyset et unicité,
champ `mark=<nom>` et génération non vide doivent être exigés avant de croire
`double_guard_verified`.

Le commentaire « une seconde tentative seulement » n'est pas implémenté : la
variable `allowance` vaut toujours 1 et n'est jamais lue. Deuxième et troisième
échecs forcés ont chacun ajouté un `STOP`, puis une quatrième reprise a encore
été admise. Retirer la variable et documenter soit une borne persistante, soit
des rejeux **manuels**, idempotents, sérialisés, liés à la génération et
toujours `stop-first` ; il ne faut pas transformer cela en boucle automatique
illimitée.

## Signalement adjacent sur le clamp invité WIP

Ce point ne vise pas `c8f69673`, mais le worktree non commité de
`start_and_verify.sh`. Deux chemins fail-open doivent être fermés avant de
recevoir ce lot :

- le succès final dépend seulement de la présence de
  `__EHGP_GUEST_GUARD_VERIFIED__` dans la sortie capturée. Le code ne conserve
  pas le fait que `gcloud_ssh_guard` a rendu zéro. Un SSH en échec qui réimprime
  sa commande — laquelle contient littéralement ce marqueur — peut donc finir
  par être certifié. Publier un booléen uniquement dans la branche `rc=0`,
  après une ligne terminale exacte, puis décider exclusivement sur ce booléen ;
- le clamp évalue directement `$(date +%s)` dans une condition arithmétique.
  Dans ce contexte, un `date` en échec ou non numérique n'est pas une preuve
  de dépassement et la boucle peut continuer. Capturer l'instant, vérifier le
  code et la grammaire, puis comparer ; toute horloge illisible déclenche
  l'arrêt ciblé.

Ajouter les mutants « SSH imprime le marqueur puis rend 255 », `date` non nul
et date non numérique. Le nouveau test d'intégration du clamp attend par
construction environ 200 secondes réelles, sans timeout de `run_script`, et
n'établit qu'une borne large. Une horloge et un `sleep` factices doivent rendre
ce scénario instantané, avec nombre et ordre exacts des appels puis arrêt de la
génération simulée.

## Dents minimales

- deux reprises synchronisées dans le même `sid` : une seule franchit le
  verrou et exactement un appel au faux garde est observé ;
- `targeted_stopped,generation=` vide + handoff valide : code 71, zéro témoin,
  purge ou appel externe ;
- `describe=RUNNING,<autre génération>` avant ou après le SCP : code 71, zéro
  promotion du staging et zéro STOP de cette autre génération ;
- entrée `targeted_stop_failed` ou `targeted_stopping` : premier appel externe
  égal au `STOP`, sans SCP ni validation ;
- faux STOP non nul + faux validateur lent ou gros `out/` : code 70 borné,
  validateur et copie massive jamais appelés ;
- `GUARDS_DIR` extérieur au pin : refus avant appel ;
- marque `double_guard_verified` dont `mark=guest_guard_pending` : code 71,
  sans SCP ni STOP ;
- fils orphelin du SID/PGID sans `WORK` dans son argv : reprise refusée ;
- échec de purge après `targeted_stopped` dans la reprise et le cycle nominal :
  aucun témoin terminal, puis purge locale sans `describe`, SCP ou STOP ;
- troisième rejeu après `targeted_stop_failed` : comportement conforme à la
  politique explicitement retenue, sans variable morte.

Jusqu'à ces dents, conserver le résultat déjà acquis et l'arrêt certifié du
reçu `1788312873`, mais ne pas prendre son succès nominal comme preuve des
chemins de reprise. Aucun GO GCP n'est ouvert par ce document.

## État du WIP après le contre-audit

Photographie : worktree non commité au-dessus de `38281dc7`. Claude a fermé
une part substantielle du rapport : `flock` remplace le verrou non atomique,
la garde STOP extérieure au pin est refusée, une génération vide est rejetée,
les marques sont parsées strictement, SID/PGID sont persistés, les états
incertains sont routés stop-first, le staging est lié à `REMOTE_DIR` puis
recontrôlé après SCP, le validateur est interdit après STOP non certifié, la
purge précède le témoin et la politique de rejeu manuel est explicite. Le code
WIP du clamp invité lie aussi le marqueur à un rc SSH nul et ferme l'horloge
hôte illisible.

Quatre coutures nouvelles ou encore incomplètes empêchent de recevoir ce WIP :

1. **Promotion après collecte non prouvée.** Après le SCP, le cas
   `describe_indisponible` ne devient pas une erreur et le staging peut être
   promu. Un `SCP_RC` non nul n'interdit pas non plus explicitement la
   promotion d'un `out/` partiel. Exiger succès SCP et tuple post-SCP strict,
   lisible et de la génération attendue avant tout `mv`.
2. **Le chemin d'échec n'est pas encore borné.** Le reçu dit minimal copie
   récursivement `marques/`, puis le hache ; `session.log` et `reprise.log` ne
   sont pas bornés. Après un STOP non certifié, n'écrire qu'un témoin minimal
   de taille plafonnée, sans copie ni hash récursifs.
3. **L'échec du témoin terminal peut être masqué.** Dans la reprise, une purge
   réussie suivie d'un `publish_witness` en échec peut encore rendre zéro. Le
   cycle nominal masque de même l'échec du Python de publication par son
   `true` final. Le succès doit dépendre de la publication atomique et du
   `sync` du parent.
4. **Le terminal est testé avant le verrou.** `recu_publie` est lu avant
   l'acquisition de `flock`, sans seconde lecture sous verrou. Une reprise
   suspendue avant le verrou puis réveillée après la conclusion d'une autre
   peut conclure une seconde fois. Refaire tous les choix terminaux sous le
   verrou acquis.

`selftest_cycle_vie_v6.sh` n'ajoute encore aucun mutant causal pour ces
fermetures et ces quatre coutures ; son changement observé se limite à
l'inventaire du nouveau profil. Le test « marqueur puis rc 255 » oublie par
ailleurs de borner son environnement et peut durer plusieurs dizaines de
secondes ; ajouter `clamp_environment()` et un timeout de harnais.

Le verdict reste donc : progrès net, **porte autonome non reçue**, aucune
nouvelle dépense GCP. Les six priorités de la coupe `c8f69673` restent
historiquement exactes ; cette section les reclasse pour le WIP courant.

## Réception critique du pin `fa9b2633`

Le lot § 5.21 est désormais commité par `fa9b2633` et inchangé dans le `HEAD`
`1cb60655`. Un rejeu complet encadré par les SHA-256 des scripts jugés donne :

- `bash gcp-migration/selftest_revalidate_v6.sh` : 15 scènes vertes ;
- `bash gcp-migration/selftest_cycle_vie_v6.sh` : code 0, 35 scénarios,
  `D9`--`D11bis` verts ;
- contrôle extérieur : `AUDIT_RC=0 SNAPSHOT_STABLE=oui`.

La priorité 68 est donc bien appliquée sur les deux sorties terminales et la
promotion conditionnelle du staging est reçue dans les scènes présentes. Ce
progrès ferme les quatre défauts qui motivaient le § 5.21, mais **pas** le
contrat complet de cycle de vie ni celui du revalidateur.

### Contrats encore ouverts

1. **Un échec local peut encore court-circuiter le STOP.** La reprise est sous
   `set -e` sans trap d'arrêt. Après connaissance de la génération et avant la
   garde épinglée, un échec de `date`, `rlog`, `rm` ou `mv` termine donc le
   processus. La collision de `out.partiel_$(date +%s)` rend ce chemin
   matériel. Router toute sortie vers un funnel d'arrêt inconditionnel, ou
   arrêter avant toute sauvegarde/promotion locale ; ajouter un `mv` non nul
   qui exige exactement un STOP de la génération.
2. **La provenance de `out/` n'est pas établie.** Après l'arrêt, le validateur
   se contente d'un `WORK/out` non vide. Si le SCP courant n'a rien promu, un
   `out/` hérité d'une tentative antérieure peut être validé. Graver un état de
   promotion atomique lié à la génération, au commit et au succès SCP, puis
   rendre cet état obligatoire avant validation. La dent `D10` doit partir
   avec un ancien `out/` non vide.
3. **Purge nominale : le vert `D8` est un faux vert.** Le plan de tests exige
   explicitement qu'un échec de purge rende 67 dans la reprise et le cycle
   nominal. Or le lifecycle ne possède que `WITNESS_RC` ; sa branche
   `purge_incomplete` écrit le marqueur puis rend vrai, et les deux sorties ne
   propagent que 68. `D8` masque le code avec `wait ... || true` et ne
   l'asserte pas. Reprendre le patron déjà correct de `recover_v6_session.sh` :
   porter `PURGE_RC=67`, définir sa priorité terminale avec 68, faire exiger 67
   à `D8`, puis conserver `D8bis=0` avec zéro appel GCP.
4. **Le revalidateur accepte un juge muet.** La commande
   `revalidate_v6_receipt.sh <reçu-c8f69673> /dev/null` rend 0 et affiche
   « recu intact » sans aucun résumé. Le second argument n'est pas authentifié
   et la boucle ne compare un résumé que si les deux côtés existent. Hors mode
   selftest explicite, exiger le validateur canonique et son hash ; dans tous
   les modes, exiger chaque résumé attendu. Ajouter aussi un fichier régulier
   qui rend 0 sans écrire de résumé : tester seulement `-f` ne suffit pas.
5. **Un `SHA256SUMS` imbriqué reste invisible.** Les générateurs de reçus, le
   revalidateur et leurs selftests emploient `! -name SHA256SUMS` ou
   `! -name 'SHA256SUMS*'`. Ils excluent donc aussi `out/SHA256SUMS` et
   `marques/SHA256SUMS`. Seul `./SHA256SUMS` racine doit être omis ; ajouter
   les deux contre-fixtures, à la construction puis pendant la revalidation.
6. **L'inventaire texte n'est pas injectif.** Un nom contenant un saut de
   ligne peut se sérialiser comme plusieurs entrées autorisées ; par exemple
   un seul répertoire `out\nmarques` se confond avec deux noms. Comparer des
   séquences NUL ou leurs digests, sans substitution de commande ni boucle
   `for` sur du texte.

`D11` mérite enfin une dent causale plus étroite : son attente du handshake est
tolérée avec `|| true`, puis elle ne compte ni le STOP ni l'issue du reçu.
Rendre le rendez-vous fatal, exiger exactement un STOP au total, le message du
fast-path et `issue=arret_certifie_par_le_garde`. Le code observé est bon ;
c'est la preuve permanente qui est encore trop permissive.

Deux durcissements P2 restent utiles : nettoyer le
`.recu_publie.*.partial` si `os.replace` échoue, et borner le reçu minimal à
la liste des marques connues sans développer tous les `*.partial.*`. Le mot
« intact » doit aussi être borné aux noms, types et octets, ou lier
explicitement modes et métadonnées.

La livraison utile suivante reste locale : funnel STOP, provenance de
promotion, `D8=67`, dent causale `D11`, puis les trois dents du revalidateur.
Aucun nouveau design, aucun nouveau résultat G4 et aucune VM ne sont
nécessaires ; aucun GO GCP n'est ouvert.

## Réception critique du pin `4ef96717`

Le pin § 5.22 apporte des fermetures utiles et vérifiées : D8 propage 67,
D11 exige son rendez-vous, un STOP et l'issue exacte, le validateur canonique
est authentifié, un juge muet est refusé, un `SHA256SUMS` imbriqué redevient un
fichier ordinaire et les noms contenant un saut de ligne sont rejetés. Le
selftest de revalidation rend 20/20 ; les rejeux ciblés D8, D11 et D12
ordinaires sont verts. Ces progrès sont reçus.

Quatre mutants plus précis restent acceptés au pin et empêchent seulement de
qualifier le durcissement complet ou de rouvrir une session facturable :

1. **STOP encore évitable dans la garde.** Un faux `tee` qui échoue uniquement
   sur le journal « arret cible : » donne code 1, zéro STOP et laisse le registre
   `targeted_stopping`. `STOP_ATTEMPTED=1` et `trap - ERR` précèdent encore le
   journal et l'appel réel. Passer toute la primitive en non-fatal, rendre
   publications et journaux best effort, puis exécuter la garde quoi qu'il
   arrive ; son échec doit rendre 70 et dominer l'erreur locale.
2. **Marqueur de promotion rejouable.** Un ancien `out/` et un
   `out.promotion` plausible portant la même génération, le même commit et
   `scp_rc=0`, suivis d'une SCP courante en échec, autorisent encore le
   validateur sur l'ancien contenu. Le wrapper rend 0 et le reçu montre
   pourtant `scp_rc=1`. Lier le marqueur à un nonce de la tentative courante
   et précharger D13 avec l'ancien marqueur, pas seulement avec `out/`.
3. **Allowlist de répertoires réinterprétée par le shell.** Un répertoire
   racine vide nommé `out marques` est découpé en deux noms autorisés ; la
   revalidation canonique rend 0 et « recu intact ». Lire et comparer les chemins
   NUL directement, puis garder ce nom exact comme contre-fixture.
4. **Résumé différent non fatal.** Un faux validateur qui reproduit tous les
   résumés puis change un octet de l'un d'eux peut encore afficher
   « DIFFERENT » puis rendre 0. Accumuler la divergence et rendre 3 après le
   contrôle d'intégrité.

Le WIP postérieur au pin suit déjà ces quatre corrections : garde non fatale,
identifiant de tentative dans `out.promotion`, allowlist NUL/Python et drapeau
de résumé divergent. Il reste à épingler ce contenu et à faire porter aux
selftests les mutants exacts ci-dessus. Cette passe locale ne remet pas en
cause `fa9b2633`, les reçus G4 déjà arrêtés ni le prototype sémantique KeyCSR,
qui peut continuer en parallèle. Aucun GO GCP n'est ouvert.

## Réception critique du pin `c2d2ac69`

Le lot correctif est reçu dans sa portée principale. Le selftest du
revalidateur passe ses 22 scènes avec sources stables ; le répertoire
`out marques` est refusé, un résumé reproduit différent rend 3 et les noms
NUL restent comparés sans réinterprétation du shell. Le lifecycle complet rend
0 et ses dents D12--D15 passent. En particulier, l'ancien marqueur de promotion
ne permet plus de valider un `out/` hérité lorsque la SCP de la tentative
courante échoue. Les quatre défauts précis du pin `4ef96717` sont donc fermés.

Il reste une seule fenêtre de sûreté, plus étroite mais causale. La génération
est résolue avant le calcul de `GEN_EPOCH`, tandis que le trap du funnel n'est
armé qu'après ce calcul. Un faux `python3` qui laisse passer les parseurs de
marques puis rend 42 uniquement sur la conversion de la génération produit :
code 42, zéro STOP, registre `targeted_running` et aucun reçu. Les deux
coupe-circuits bornent encore la session, mais l'arrêt immédiat promis est
sauté. Déplacer cette conversion juste après l'armement, rendre une époque vide
fatale sous le trap et conserver le mutant donne la fermeture minimale : STOP
réussi attendu à 74, STOP échoué attendu à 70, exactement une tentative dans
les deux cas.

Un écart secondaire ne doit pas masquer ce progrès. Si `tee` échoue seulement
sur le journal « arret cible », la garde effectue bien un STOP unique ; un
échec d'arrêt rend 70, mais un arrêt réussi est rapporté 0 au lieu de 74. Claude
peut soit mémoriser séparément l'erreur locale du journal, y compris
`PIPESTATUS[1]`, puis passer par le funnel sans second STOP, soit déclarer cette
télémétrie best effort. C'est un choix de contrat P2, pas une nouvelle faille
d'arrêt ni un motif pour suspendre KeyCSR.

Aucune VM n'est nécessaire pour fermer ces deux points : les faux outils du
selftest local suffisent. Aucun GO GCP n'est ouvert.
