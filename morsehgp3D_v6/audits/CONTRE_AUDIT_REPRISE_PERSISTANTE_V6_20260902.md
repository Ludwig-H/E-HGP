# CONTRE-AUDIT — reprise persistante `c8f69673`

Date : 2 septembre 2026. Coupe jugée : `c8f69673` dans le `HEAD` courant.
Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Verdict constructif

La base 0700, les deux marques du vrai garde, l'authentification en deux
étages, la génération portée partout et la reprise sans `START` sont des
fermetures réelles. Le selftest de `c8f69673` repasse ses 74 contrôles. La
session `1788312873` montre aussi que les deux marques ont été effectivement
émises avant un arrêt ciblé certifié.

La reprise n'est toutefois pas encore une porte autonome pour une nouvelle
dépense. Les cinq coutures ci-dessous se ferment localement, sans VM et sans
rouvrir les résultats device.

## Coutures à fermer

1. **Le verrou `reprise.pid` n'est pas atomique.** Le code teste le fichier,
   retire un verrou jugé périmé, puis l'écrit avec une redirection ordinaire.
   Deux processus peuvent franchir ensemble la fenêtre
   test/suppression/écriture. Le balayage de processus ne sauve pas le cas de
   deux reprises lancées depuis la même session POSIX, car elles s'excluent
   mutuellement du balayage par `sid`. Utiliser un verrou kernel conservé
   jusqu'à la sortie (`flock -n` sur un descripteur ouvert dans `WORK`) ; le
   PID/starttime devient alors un diagnostic, pas la primitive d'exclusion.
2. **Le `describe` ne lie pas la génération avant le SCP.** Il lit bien
   `status,lastStartTimestamp`, mais la condition de rapatriement ne teste que
   le préfixe `RUNNING`. Si la même instance a été redémarrée par une autre
   session, la reprise peut lire les sorties de cette nouvelle génération ;
   le garde d'arrêt refusera ensuite de l'arrêter, trop tard pour le SCP.
   Exiger `lastStartTimestamp == GENERATION` avant tout SCP. Une divergence
   doit rendre 71, sans SCP et sans mutation.
3. **Un arrêt échoué ne court-circuite pas le validateur.** Après
   `stop_and_verify` non nul et l'état `targeted_stop_failed`, le script lance
   encore le validateur borné avant de rendre 70. La cible peut rester
   `RUNNING` pendant ce travail local. Finaliser immédiatement le reçu
   d'échec et rendre 70 ; aucun validateur ne précède une nouvelle tentative
   d'arrêt. La validation partielle n'a de sens qu'après
   `targeted_stopped` certifié.
4. **Une garde externe non épinglée reste exécutable.** Lorsque `GUARDS_DIR`
   diffère du répertoire épinglé, le code affiche « selftest seulement », puis
   exécute tout de même ce `stop_and_verify.sh`. Le caractère “selftest” n'est
   établi par aucun prédicat. Le chemin production doit exiger exactement la
   copie ré-authentifiée ; le harnais peut recevoir une entrée de test
   explicitement fermée et impossible dans un `session.env` production.
5. **Le témoin terminal précède une purge dont les échecs sont masqués.** Le
   bloc écrit `recu_publie`, tente `rm`/`shred`, puis finit par `true`. Une
   panne ou un arrêt du processus après le témoin interdit toute reprise alors
   que les credentials peuvent subsister. Sur l'état déjà
   `targeted_stopped`, purger et vérifier d'abord, puis publier atomiquement le
   témoin. En cas d'échec, laisser la reprise rejouable sans aucun nouvel appel
   GCP.

Le commentaire « une seconde tentative seulement » n'est pas implémenté : la
variable `allowance` vaut toujours 1 et n'est jamais lue. Pour la sûreté, le
choix le plus utile est de documenter une reprise répétable jusqu'à
certification de l'arrêt, chaque tentative restant sérialisée et liée à la
génération, plutôt que d'introduire une limite arbitraire silencieuse.

## Dents minimales

- deux reprises synchronisées dans le même `sid` : une seule franchit le
  verrou et exactement un appel au faux garde est observé ;
- `describe=RUNNING,<autre génération>` : code 71, zéro SCP, zéro STOP ;
- faux STOP non nul + faux validateur lent : code 70 immédiat et validateur
  jamais appelé ;
- `GUARDS_DIR` extérieur au pin : refus avant appel ;
- échec de purge après `targeted_stopped` : aucun témoin terminal, puis reprise
  locale de la purge sans `describe`, SCP ou STOP ;
- troisième rejeu après `targeted_stop_failed` : comportement conforme à la
  politique explicitement retenue, sans variable morte.

Jusqu'à ces dents, conserver le résultat déjà acquis et l'arrêt certifié du
reçu `1788312873`, mais ne pas prendre son succès nominal comme preuve des
chemins de reprise. Aucun GO GCP n'est ouvert par ce document.
