# Audit ponctuel du Pool R2

30 septembre 2026. Capture indépendante des sources et des preuves déjà
terminées dans `/tmp/mhgp10-r2/pool`, sans modifier le moteur, les harnais
du développeur ni ses campagnes. Aucun test relancé, aucun processus arrêté,
aucun GCP ou GPU utilisé. `public_status=not_claimed`.

Le correctif examiné remplace la réclamation `fetch_add` par un CAS saturant
et calcule chaque fin de tranche avec `begin + min(grain, n - begin)`,
également en série. Le compteur ne dépasse pas n et reste arrêté à n après
annulation. Les deux harnais interceptent désormais les formes de new/delete
alignées. L'élection atomique d'un unique écrivain de l'exception est conservée.

## Preuves conservées

- [ASan/UBSan](preuves/asan/codes.txt) : onze commandes, code 0 chacune,
  aucun rapport sanitizer annoncé. Marqueur de fin daté 03:26:50 UTC.
- [TSan](preuves/tsan/codes.txt) : dix commandes, code 0 chacune,
  aucun rapport sanitizer annoncé. Marqueur de fin daté 03:27:45 UTC.
- [Sondes indépendantes](sondes/r2/codes.txt) : vingt-sept commandes,
  neuf par régime Release/ASan/TSan, toutes code 0. Fin 03:29:55 UTC.
- [MA1](mutants/ma1_fetch_add/unit_pool_claim_wrap.stdout) réintroduit
  fetch_add ; [MA3](mutants/ma3_serie_b_plus_grain/unit_pool_claim_wrap.stdout)
  réintroduit le débordement série. La porte des tranches échoue avec code 1
  et décrit les anomalies géométriques des intervalles.
- [M5 sans élection](mutants/m5_sans_election_tsan/codes.txt) :
  six essais TSan code 66. Les traces localisent les courses sur
  `job.error` / `exception_ptr::swap`, pas sur une simple configuration du runtime.
- [MB7 sans delete alignés](mutants/mb7_delete_aligne_non_remplace_asan/codes.txt) :
  ASan code 1 avec `alloc-dealloc-mismatch`. Son code 0 en Release est
  conservé et explicitement distingué, pas présenté comme un échec.
- [MA4](mutants/ma4_borne_large/codes.txt) remplace `begin < n` par
  `begin <= n`. À n, le CAS n→n reprend les tranches vides sans fin.
  Neuf codes 124 : six délais de 120 s puis trois de 300 s, soit 1 620 s
  de délais programmés sur le même défaut. Le [runner](mutants/run_pool_mutant.sh)
  documente ces durées. Ce constat informe le développeur ; aucun arrêt
  de sa campagne n'a été effectué par l'auditeur.

Ces commandes appartiennent au développeur ; l'auditeur a lu puis figé les
fichiers. Elles ne sont pas quarante-huit tests géométriques indépendants :
certaines portes globales rejouent des groupes déjà testés séparément.

## Portée et traçabilité

[receipt.json](receipt.json) décrit les contrôles et leurs limites ;
[source_inventory.json](source_inventory.json) conserve, pour chaque copie,
le chemin d'origine exact, la taille, la date UTC, les nanosecondes encodées
en chaîne et le SHA-256. Les 200 contenus copiés concordent avec leurs
origines avant/après ; les dates originales sont préservées.
[observed_binaries.json](observed_binaries.json) contient les empreintes
observées des binaires, qui ne sont pas copiés.

Les sources unitaires ont reçu un reformatage de commentaires après les
premières portes : la capture des sources actuelles n'est donc pas une
clôture de toutes les dépendances avant compilation. Aucun transfert
automatique de qualification vers une future extraction commune.
L'erreur initiale du collecteur, qui comparait des nanosecondes arrondies
après passage par JavaScript, est conservée dans
[capture_checks/first_recheck_nanosecond_precision.json](capture_checks/first_recheck_nanosecond_precision.json).
Le contrôle corrigé garde les nanosecondes exactes en chaînes ;
aucun hash de copie ne diverge.

Pendant la copie, les campagnes générales de mutants et de différentiels
étaient encore ouvertes. Au contrôle de 03:59 UTC, le marqueur général
des mutants est daté 03:58:43 et leur PID n'apparaît plus, mais les résultats
des autres mutants ne sont pas examinés ou qualifiés par ce paquet.
Le différentiel reste actif (PID 96654), sans marqueur final.
Aucun log de différentiel en cours n'est inclus.

Réserve du harnais CLI : son balayage aligné s'arrête à huit indices sans
exiger un essai sans injection. Les reçus étudiés observent une allocation
alignée puis N=1 sans injection ; la couverture de cette fixture est donc
réelle. Pour un futur chemin ayant plus de huit allocations alignées,
ajouter une condition explicite de fin de balayage.

Ce paquet ne qualifie ni tous les nouveaux correctifs ensemble, ni une
tour FULL, ni le contrat de 100 ms sur G4.
