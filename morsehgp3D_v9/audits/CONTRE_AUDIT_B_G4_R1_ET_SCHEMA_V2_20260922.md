# Contre-audit B — reçu G4 R1 et rupture du schéma v2

22 septembre 2026. Lecture indépendante hors ligne du commit `ad2d0ebb`,
puis de `5ab4326c332ec2a2d951fd7c3bbdeffa22918873`, dans le worktree détaché
`build/v9-open-worktree`. Aucun moteur, compilation, selftest complet ou
appel GCP exécuté par cette contrelecture ; seuls des lecteurs et des
contre-fixtures JSON en mémoire ont été appelés. Aucun fichier du moteur,
du protocole ou du reçu historique n'a été modifié.

Cadre conservé : `exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, `not_claimed`.

## Verdict

Le [reçu G4 R1](../receipts/g4_tower_r1_20260922/README.md) est cohérent
avec **huit tours CPU `complete_relative`**, exécutées depuis le paquet
**`e28296bb`**, puis avec l'arrêt ciblé certifié de sa génération. Il ne
mesure pas le nouveau noyau MEB introduit par `ad2d0ebb`, ni une voie GPU.

**Nouvelle session bloquée sur `5ab4326c` : le lecteur v2 refuse la sortie
v2 réelle du programme.** Le selftest utilise encore une forme différente
de `tower_work` et ne détecte pas cette incompatibilité. Un second défaut
de réception concerne les cas censurés : `group_closed=false` n'empêche pas
l'acceptation d'un reçu `partial`. Aucun de ces deux défauts n'invalide les
huit résultats historiques R1 relus ici.

## Chaîne d'artefacts vérifiée

- Les **175 entrées** de `g4_tower_r1_20260922/SHA256SUMS` passent depuis le
  répertoire du reçu. Les fichiers publics contrôlés sont ceux du commit
  audité, pas une nouvelle exécution.
- Les **111 empreintes** de `vm/sources_before.json` sont identiques à
  celles de `sources_after.json`. Le hash exact de ce manifeste est
  `afae898072f1ed54b8c514a3a3ab4818933216809713651dd0421921ad32f906`,
  identique au paquet et aux reçus hôte/invité.
- **110 liens** ont été vérifiés contre les objets Git d'`e28296bb` ou la
  sérialisation canonique de la provenance : 101 fichiers v9, quatre
  fichiers de protocole, le helper épinglé, trois entrées complètes et la
  provenance. L'arbre Git annoncé est correct. Le dernier élément est le
  plan personnalisé : ses huit cas sont disponibles et identiques dans
  `PACKAGE.json` et le reçu invité, mais ses octets originaux ne sont pas
  publiés. La reconstruction *octet pour octet* de l'archive
  `e766319e…` n'a donc pas été certifiée par cette contrelecture.
- Le worker réellement annoncé et son exemplaire committé ont le SHA
  `2098e77652a8f5a36d010278dafd0d44acedfb3c93626252861d5280668d5310` ;
  même contrôle pour le contrôleur
  `bcf19e3cd1f47174b539d35cba8f8551483c65bf2256962f055cf6e58163401e`.
  Chargés depuis les octets Git historiques, leurs fonctions de lecture
  rejugent le répertoire public `vm/` avec **`validate_received=completed`**.
- Les **31 commandes** invitées, leurs sorties et leurs SHA sont relus par
  ce lecteur. Configuration Release, GCC 11.4, cible unique
  `mhgp9_tower_probe`, construction CMake stricte : aucun ELF local
  transporté ni compilation CUDA. Les 423 dépendances compilées incluent
  88 sources du paquet dont les empreintes correspondent au manifeste.
  L'ELF déclaré est
  `9f010b05296902d6d762a4c4f80ecb41c51e61c5c7403d71a2a440d34986715f`,
  déclaré stable avant/après par le worker. Le binaire et les dépendances
  système ne sont pas joints : leur re-hachage indépendant après arrêt
  n'est pas une propriété du reçu public.

`PACKAGE.json` reste correctement `prepared_not_executed`, avec
`GCP_used=false` : il décrit la préparation. Les reçus hôte et invité,
distincts, décrivent l'exécution. Ne pas utiliser le statut du paquet comme
statut de la session, ni réécrire son état historique.

## Machine et fermeture

La preuve invitée identifie le projet `devpod-gpu-exploration`, la zone
`us-central1-b`, l'instance `ehgp-v7-4fa0e0789a7d5bb06b787d35` et le type
`g4-standard-48`. `lscpu` indique **AMD EPYC 9B45, 48 vCPU, 24 cœurs et
deux fils par cœur** ; les 48 CPU disponibles sont enregistrés. Le worker
contrôle cette identité via le service de métadonnées et recoupe le boot
avec la génération. Ce n'est pas une preuve d'exécution sur GPU :
`GPU_executed=false` et `backend=reference_cpu` sont cohérents.

La génération `2026-09-22T16:27:08.673-07:00` est identique dans le handoff,
la marque de double garde et les reçus. La marque hôte est identique à sa
copie dans `vm/guard_evidence.json` : garde GCE 3600 s, arrêt invité 40 min,
marge de fermeture 300 s. Les lectures `describe` avant transfert,
exécution et récupération sont enregistrées ; leurs JSON bruts ne sont
pas publiés, seulement les hashes/commandes. L'audit public ne rejoue donc
pas indépendamment tous les champs SPOT/STOP du service GCE.

L'arrêt utilise `--expected-last-start-timestamp` pour cette génération,
rend 0, et sa sortie annonce `TERMINATED` ainsi que l'absence d'autre VM
active portant le label. Le fichier `guarded_stop.redacted.stdout` est
**octet pour octet égal au hash stdout de la commande**, malgré son nom
« redacted ». `targeted_shutdown_certified=true` concorde. Les commandes
archivées vont de **23:26:33 UTC à 23:38:25 UTC** ; l'en-tête README
23:26–23:44 ne constitue pas une preuve d'activité facturable jusqu'à
23:44. Aucun état GCP présent n'est déduit de ce reçu historique.

## Résultats et statuts

Les huit sorties portent les identités d'entrée attendues, `run_tower=true`,
K effectif correct, tous les ordres 1..5 ou 1..10 et le code 0. Les deux
comparaisons entre configurations (0/6 et 3/7) sont égales selon la
projection du protocole. Les **six comparaisons G4/local correspondantes**
ont aussi été recoupées champ par champ : `generator`, `catalogue`,
`tower_work`, `orders` et `tower_digest` sont identiques, au-delà du seul
condensé. Il s'agit des **résumés JSON**, non d'une comparaison BallKey par
BallKey (la sonde ne publie pas cet inventaire). Cette égalité concerne ces
cas, pas tous les nuages possibles.

Les valeurs du `SUMMARY.json` concordent avec les bruts : temps de chaîne
et de phases arrondis, RSS et cardinalités ; `cpu_s` est la somme
utilisateur+système de GNU time arrondie à 0,1 s, pas `chain_cpu_s`.
La colonne temps total est `chain_total`, distincte de la durée externe
de commande et de la préparation/segmentation hors ligne. Aucun contrat
1 s/100 ms, trame brute float32 ou GPU n'est acquis par ces trois trames
sans sol sur grille 1 mm, une seule répétition par cas.

`completed` est recalculé seulement lorsque tous les cas sont
`complete_relative` ; `partial` conserve explicitement refus, cas censurés
ou sautés. **Le code 0 du worker/contrôleur accepte aussi `partial` et ne
garantit même pas une tour achevée**. Lire le statut, les cas achevés, les
comparaisons et la fermeture ; ne pas transformer le simple code shell en
succès scientifique. R1 satisfait effectivement la condition des huit cas
achevés et n'est pas un `partial` promu.

## Blocage certain : sonde et lecteur désynchronisés

1. À `ad2d0ebb`, `bench/tower_probe.cpp:169–178` ajoute `ledger`, tout en
   conservant `mhgp9_tower_probe_v1`. `tower_worker_v9.py:91–94,291` exige
   exactement l'ancien ensemble de champs. Une sortie valide dotée de
   `ledger` est rejetée **`ValueError: probe JSON fields`**.
2. À `5ab4326c`, le passage commun à v2 règle ce premier décalage, mais
   `tower_probe.cpp:181–192` publie désormais dans `tower_work` la chaîne
   `meb_accounting` et le tableau de quatre compteurs
   `meb_supports_by_size`. `tower_worker_v9.py:309–311` exige encore que
   **toutes** les valeurs de `tower_work` satisfassent `_count`, donc soient
   des entiers scalaires. La contre-fixture JSON donne exactement
   **`ValueError: probe counters tower_work`**, avant acceptation du cas.
   Le worker produirait `probe_failed`, pas une nouvelle mesure acceptée.
3. Le faux programme de `tower_selftest_v9.py:76–79` conserve l'ancien
   `tower_work` sans ces deux champs. ROOT a rejoué le selftest complet
   de `5ab4326c` : **17/17 passent en 45,34 s**, puis sa contre-fixture avec
   les deux types réels échoue. Ce rejeu est un contrôle de ROOT, pas un
   nouveau selftest exécuté par le présent auditeur. Le défaut explique
   précisément pourquoi ce vert ne qualifie pas le raccord réel.

Octets de `5ab4326c` contre-lus :

- worker : `a1e36e49592bca8be54a7b335862a091e44674413d04f56f864c8e00101254c8` ;
- selftest : `f42dd3538fba31f4d2d8958939433c259c23dc195e07fedf76bef401bd551080` ;
- sonde : `0f1345feeabd0bef6099a34dc1915e18887869cb6efe8202135df546a6c0867f`.

Avant GCP : définir les champs v2 requis et leurs types (compteurs u64,
identifiant d'accounting exact, tableau de longueur quatre), mettre le faux
producteur au même contrat, et ajouter une **porte avec la sortie d'une
vraie petite exécution de la sonde**. Cette porte doit lire le JSON sans le
réécrire, exercer succès et refus, et garder l'identité d'entrée de test
isolée sans assouplir les pins des trames de production. Ajouter les
contre-cas : v1 inattendu, champ absent, mauvais accounting, tableau mal
dimensionné, booléen/négatif/débordement au lieu d'un compteur. Relire aussi
le reçu complet synthétique par `validate_received`, normal et `-O`.

## Fermeture des cas censurés : refus manquant

La branche `tower_session_v9.py:192–199` accepte un cas
`killed_case_cap`/`killed_budget` avec les flags deadline et kill, **sans
exiger `group_closed=true`**. Le helper épinglé
`full_probe_worker_v7.py:234–243` peut laisser ce champ à `false` après
10 secondes de drainage. La `SessionDeadline` poursuit alors sa remontée
et contourne le contrôle de fermeture normale situé ligne 249.

Contre-fixture exécutée entièrement en mémoire à partir du reçu R1 :
dernier cas converti en `killed_case_cap`, code −9, durée 600 s,
`session_deadline_reached=true`,
`residual_or_interrupted_group_killed=true`, **`group_closed=false`** ;
liste des commandes et état final ajustés de manière cohérente.
`validate_received` rend **`partial`**. Ce n'est pas la preuve qu'un
processus actif a subsisté dans R1 : ses 31 commandes sont toutes fermées.
C'est un reçu de fermeture négatif accepté par le protocole.

Avant nouvelle session, exiger aussi la fermeture du groupe sur chaque
cas censuré, et tester causalement les deux issues `true`/`false`.
Conserver la censure et ses logs, mais classer la fermeture non prouvée
comme échec de protocole, sans poursuivre des mesures supposées isolées.
L'arrêt GCE ciblé doit rester inconditionnel et indépendant de ce verdict.

## Provenance déclarée, mais pas recertifiée par le lecteur

Contre-test indépendant sur `5ab4326c`, **hors GCP** : le constructeur
officiel `tower_snapshot_v9.collect` lit bien les blobs Git du commit et
vérifie leur identité avant de fabriquer son paquet (`:106–136`). En
revanche, le worker ne contrôle que la forme hexadécimale de `commit` et
`tree` dans `validate_provenance` (`tower_worker_v9.py:219–229`), puis les
SHA des fichiers **contre le manifeste transporté** (`:239–247`) ; il ne
relit aucun objet Git. Le contrôleur `require_committed_protocol`
(`tower_session_v9.py:425–435`) exige seulement la chaîne
`protocol_source="commit"`. L'auditeur a muté une source du paquet
selftest, recalculé son manifeste cohérent, indiqué les identifiants
inexistants `commit=000…` et `tree=111…` : `validate_snapshot` accepte
encore les huit cas et `require_committed_protocol` accepte le paquet.
La revendication « ce paquet provient de ce commit » n'est donc pas
validée à la frontière d'une session recevant un paquet externe ou
altéré. Cela **n'invalide pas R1** : ses liens aux blobs Git ont été
rejoués indépendamment plus haut.

La réception finale a une deuxième lacune de liaison :
`validate_received(output, manifest, worker_pin, expected_cases)` ne
reçoit pas la provenance attendue. Remplacer uniquement
`vm/receipt.json.provenance.commit` par `000…`, en laissant le reçu hôte
inchangé, laisse encore le verdict `completed` dans le contre-test.
Avant une nouvelle session facturée, reconstruire les blobs attendus
depuis le commit annoncé (ou les comparer par une autre vérification Git
équivalente) **et** exiger l'égalité de la provenance du reçu invité avec
la provenance de paquet déjà vérifiée. Ajouter ces deux mutants au
selftest sans toucher à la logique de l'arrêt ciblé.

## Cohérence des chronos : garde à préparer pour le contrat

`validate_probe` exige seulement que les sous-temps soient des nombres
non négatifs (`tower_worker_v9.py:306–308`) ; `validate_gnu_time`
(`:330–335`) cherche la ligne de durée externe sans en comparer la valeur.
Une fausse sortie `complete_relative` avec `chain_total=0 ms` et
`tower=100 000 ms` passe encore le lecteur. Les huit temps historiques R1
ne sont pas soupçonnés par ce test : les durées externes de leurs commandes
dépassent `chain_total` de **0,059 à 0,248 s**. Mais une future
qualification à <1 s doit confronter chronos internes, temps externe
parsable et frontière mesurée, avec une tolérance publiée ; un digest ou
une publication hors sous-chrono reste inclus dans le mur.
