# Journal des tentatives — campagne scale_threads G4, 18 août 2026

Reçu HONNÊTE : **six** tentatives de lancement, ZÉRO campagne de
mesure, zéro reçu de mesure. Coût VM total ≈ 12 minutes de
g4-standard-48 SPOT (GPU au repos accepté ≤ 1 h par directive
explicite de l'utilisateur). Chaque échec a produit un correctif
poussé sur `main` — le protocole en sort plus dur qu'il n'est entré.

Statut d'arrêt, à la lettre de ce qui est conservé (correction du
18 août, audit `dd0d4a6` § 4 — la rédaction initiale annonçait « cinq
tentatives » alors que la table en porte six, et affirmait que
**chaque** VM avait été certifiée `TERMINATED`) :

- tentatives 2, 3 et 5 : arrêt d'urgence exécuté ET lecture d'état
  certifiée dans leur log ;
- tentatives 1 et 4 : refus AVANT toute création de VM — il n'y a rien
  à certifier ;
- tentative 6 : arrêt déclenché par le trap et **double coupe-circuit
  certifié** dans son log (`terminationTimestamp=13:43:52Z`, invité
  55 min), mais **aucune lecture finale explicite n'est conservée**
  dans ce reçu. La garantie y est celle du coupe-circuit, pas celle
  d'une observation — c'est pourquoi la PREMIÈRE action de la prochaine
  session GCP reste un inventaire en lecture seule.

| # | Heure UTC | Poste | Cause d'échec | Étape atteinte | Correctif poussé |
|---|---|---|---|---|---|
| 1 | ~11h50 | Codespace (Louis) | Garde 5 de `start_and_verify` non modélisée au préflight (invité 57 min + 300 s > 3600 s) | refus AVANT VM | six gardes au préflight, TTL défaut 427 (`ab8fde5`) puis TTL DÉRIVÉ + constantes lues à la source + `--check-envelope` (`b4dcdb4`, session Codespace) |
| 2 | ~12h05 | Codespace (Louis) | `terminationTimestamp` lu au premier niveau (n'existe pas) → certification post-démarrage impossible | VM RUNNING ~4 min, urgence certifiée | lecture `scheduling.terminationTimestamp` (`113b25c`) |
| 3 | ~12h07 | Codespace (Louis) | idem champ (toujours vide sous `scheduling.`) + PRÉEMPTION Spot immédiate (~15 s) en `europe-west4-a` | VM RUNNING ~1 min, urgence : déjà TERMINATED | lecture en cascade jusqu'à `resourceStatus.scheduling.terminationTimestamp` (`5c326c1`) |
| 4 | ~12h30 | Session principale CCR (jeton utilisateur transitoire) | `ssh-keygen` absent du conteneur (rc=127) | refus AVANT VM (seul `maxRunDuration` reposé) | `openssh-client` installé dans le conteneur |
| 5 | ~12h40 | Session principale CCR | armement de la garde invité impossible : le bac à sable **bloque le port 22 sortant** (vérifié : `github.com:22` bloqué, 443 ouvert) — `terminationTimestamp` LU ET CERTIFIÉ cette fois (`13:26:58Z`, zone `-ai1a`, capacité OK ~6 min sans préemption) | VM RUNNING ~6 min, urgence certifiée | conclusion structurelle : la campagne s'exécute depuis un poste avec SSH ; l'API seule ne suffit pas |
| 6 | ~12h47 | Codespace (Louis) | `sshd` de la VM `-ai1a` pas encore prêt (premier boot du disque), fenêtre de retry en cours quand l'utilisateur a suspendu la campagne | VM RUNNING, coupe-circuits certifiés (`terminationTimestamp=13:43:52Z`) ; arrêt par trap/coupe-circuit | — (suspension utilisateur : « on va arrêter tout cela ») |

Vérification finale de séance (12h44 UTC, lecture seule) : les quatre
instances du projet (`ehgp-blackwell-spot`, `frangi-blackwell-spot`,
`ehgp-blackwell-spot-ai1a`, `cracksam-frangigraph-g4-spot-ew8c`)
étaient `TERMINATED`. La tentative 6 s'est terminée APRÈS cette
vérification : son extinction est garantie par le double coupe-circuit
certifié dans son log (GCE 3600 s → 13h44 UTC au plus tard, invité
55 min), et **par rien d'autre dans ce reçu** ; la PREMIÈRE action de
la prochaine session GCP reste un inventaire lecture seule.

Ce qui est PROUVÉ au terme de la journée : préflight six gardes
(refus code 2 avant toute action), pin/inventaire/`set-scheduling`/
clé OS Login/démarrage gardé/certification `terminationTimestamp` via
`resourceStatus.scheduling`/arrêt d'urgence — chaque maillon a
fonctionné au moins une fois sur le terrain. Le seul maillon jamais
franchi : l'armement SSH de la garde invité (deux causes distinctes :
port 22 du bac à sable ; lenteur du premier boot `-ai1a`).

La ligne prête et les leçons sont consolidées dans `../../PASSATION.md`
§ 6.
