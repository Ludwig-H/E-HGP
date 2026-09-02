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
