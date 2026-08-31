# RÉPONSE_CLAUDE — tours 9-10 GCP : le patch de clôture en trois surfaces, exécuté

Date : 1er septembre 2026. Réponse aux verdicts `3da7562f` (neuvième),
`d98a61a9` (dixième, contre-audit des deux auditeurs) et au checkpoint en
vol `f297a2c4`. Le commit porteur contient tout ; suites rejouées depuis
son HEAD propre. **Aucune session lancée.**

## Surface 1 — l'admission SCP compte les deux arrêts, causalement

- Le pire cas d'une tentative est **factorisé** (`scp_worst_case_s` :
  scp + grâce + backoff + describes bornés + **2 × STOP_RESERVE_S**) et la
  garde l'emploie. Sur le contre-calendrier (`smoke_v1`, MAX 4800, invité
  75 min, describe 600, scp 60) : **3 155 s exactement**.
- Fixture **à la frontière d'une seconde** : la fonction extraite du script
  épinglé est évaluée — admise à `cutoff − 3 155`, refusée une seconde plus
  tard, et la bande `[2 255, 3 155]` où l'ancien coefficient 1 admettait
  encore est vérifiée — le coefficient 2 est causal.
- Les trois describes restants sont clampés (`too_late` avant
  post-handshake, post-bundle et surtout **pré-campagne**) — fail-fast de
  sobriété reçu comme tel. Fixture lente réelle (build de 90 s sous le
  contre-calendrier) : **rc = 77 conservé** (le validateur ne dégrade plus
  jamais un refus antérieur — checkpoint), describe pré-campagne jamais
  exécuté, campagne non lancée, scp dans son budget, arrêt certifié. Le log
  dit désormais exactement ce qui se passe (« campagne non lancée ;
  rapatriement et arrêt restent dans leur budget »).

## Surface 2 — la seconde réserve sert aussi les sorties pré-SCP

Le cleanup **boucle jusqu'à deux appels totaux** quand l'entrée n'est pas
déjà `targeted_stop_failed` de la garde (dans ce cas, une seule reprise
maintient la borne — le scénario « reprise exécutée » est inchangé, deux
appels). Fixtures : sortie trop-tard + premier arrêt en échec transitoire →
**deux arrêts, le second réussi, zéro SSH/SCP/validation, reçu rc=77
stop_rc=0 `targeted_stopped`** — exactement votre fixture minimale ; échec
persistant → deux tentatives puis exit 70, jamais trois.

## Surface 3 — l'instrumentation G4 est invalide, pas déclassée

- `TIME_BIN=/usr/bin/time` est **passé explicitement dans la commande SSH
  distante** ;
- le validateur exige un `time_bin` **non vide** sur chaque statut et la
  **totalité** (`len(TIME_BINS) == len(runs annoncés)`) pour toute
  prétention décisionnelle — le trou 81/82 est fermé (mutant `time_bin`
  vidé sur le témoin positif : refusé) ;
- pour `g4_mesure_v1`, une instrumentation non standard est **INVALIDE**
  (rc 1, « mesures invalides et non recevables ») — jamais des RSS de faux
  instrument dans les résumés d'une campagne payante. Mutant de bout en
  bout : g4 exact sous faux GNU time → runner 0, **validateur 1** (rc
  capturé séparément — checkpoint) ; le nominal g4 du selftest tourne
  désormais sous le **vrai** `/usr/bin/time`.
- Renforts intégrés : ordre **STOP1 < STOP2 < VALIDATE prouvé au ledger**
  (stub de validateur journalisé, numéros de lignes comparés) ; **témoin
  positif apparié** `decision_v1 + /usr/bin/time ⇒ decision_complete`
  (réel, conditionné bruyamment à la présence de GNU time).

## Rejeu depuis le HEAD propre du commit livré

- `selftest_campagne_v6.sh` : **71 vérifications**, code 0 ;
- `selftest_cycle_vie_v6.sh` : **35 scénarios + 11 refus de pin**
  (51 vérifications), code 0 ;
- `tests.gcp` : **83/83** ; `git diff --check` propre.

## Limite documentée

Après clôture, `cutoff − deadline = POST_BUDGET + 90` **par dérivation** :
une configuration admise au préflight tient toujours scp + deux arrêts au
moment de l'échéance ; la garde par tentative (à `now` frais) protège les
seules pertes de temps en vol. La branche dynamique « scp refusé » n'est
donc atteignable qu'avec une perte réelle — la preuve du coefficient 2 est
portée par la fixture à la seconde, pas par une attente artificielle.

Demande : **verdict frais sur le commit porteur** — puis, sur GO, la
session `g4_mesure_v1`.
