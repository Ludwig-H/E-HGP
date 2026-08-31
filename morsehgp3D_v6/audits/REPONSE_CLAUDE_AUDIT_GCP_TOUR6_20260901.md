# RÉPONSE_CLAUDE — sixième tour GCP : la porte exécutée (registre, cible, timeouts, frontière à quatre classes, mutants causaux)

Date : 1er septembre 2026. Réponse à `AUDIT_GCP_V6_P0_20260901` (sixième
tour, autorité `e018b4c8`, complété à `5edca49d`). Le commit porteur
contient tout ; chaque suite rejouée depuis son HEAD propre. **Aucune
session lancée** — le feu vert de l'exploitant reste conditionné à votre
verdict. (Le commit intermédiaire `01558594` était une sauvegarde d'urgence
avant coupure du codespace ; son message listait honnêtement le RESTE —
le présent commit le solde.)

## Point 1 — registre : OSError, cible, reçu

- `state_snapshot()` : **seul `FileNotFoundError` vaut « absent »** ; tout
  autre `OSError`, lien symbolique ou objet non régulier vaut « illisible »
  → blocage 71 sans génération indépendante. Mutant permanent
  `registre_illisible` (chmod 000, sans handoff) : blocage 71 exigé, la
  commande de contrôle imprimée, jamais « refus avant mutation », zéro
  arrêt. Alphabets FERMÉS (`[A-Za-z0-9._:-]`) sur les champs transportés
  par mots — la même règle est appliquée au handoff.
- **Adoption à CIBLE EXACTE** : la génération d'un snapshot strict n'est
  adoptée pour l'arrêt que si `project/zone/instance` désignent la cible
  configurée ; cible discordante sans handoff → **blocage 71, aucun appel
  mutateur** (`blocage_cible_discordante`).
- **Reçu du fast-path** : `GENERATION` et `GEN_EPOCH` sont affectés depuis
  le registre certifié avant `finalize_receipt` — plus de
  `generation=inconnue` ni de `run_id` avorté sur ce chemin. L'incohérence
  post-arrêt est jugée sur un **snapshot strict**, plus par `state_field`.

## Point 2 — timeouts terminaux et marges cohérentes

- `--kill-after 30` sur TOUS les `timeout` : `run_one` du runner, SSH de
  build et de campagne, handshake boot_id, les deux SCP.
- Le dépassement `+300` est supprimé : l'enveloppe de campagne vaut
  l'échéance du runner **+60 s** (écriture du manifeste distant après le
  dernier run — le runner tronque lui-même ses runs par `past_deadline`),
  bornée par le coupe-circuit scp (`MAX−600`).
- Marges alignées : `RAPATRIEMENT_MARGE_S=2700` (échéance des runs à
  MAX−2700), SCP à 900 s par tentative avec **garde d'échéance avant chaque
  tentative** (aucune tentative ne peut déborder sur l'arrêt), arrêt invité
  à 470 min = MAX−600, les 600 dernières secondes pour validation + arrêt.
- Budget renommé **estimation nominale** ; l'**enveloppe de terminaison**
  (somme des plafonds par run) est calculée et publiée séparément, avec la
  phrase honnête : ce sont les gardes d'échéance qui bornent la session,
  jamais cette somme. Les builds GPU sont crédités À LEURS PLAFONDS
  (`GPU_BUILD_TIMEOUT=900`, mesures réelles v5 : 12–90 s) — crédit ==
  plafond, plus de substitution. G4 : nominal 23 794 s, fenêtre 25 200 s.

## Point 3 — frontière : quatre classes fermées, RLIMIT attesté

Classes admises et RIEN d'autre :

1. `0` : contrat pipeline v6 complet + **motifs interdits scannés** ;
2. `124` : **attesté par le superviseur** (« Exit status: 124 » dans la
   sortie GNU time) — décrit prudemment comme non distingué d'un exit 124
   du binaire ;
3. `2` : uniquement avec le message exact du pipeline
   `REFUS resource_exhausted` (vérifié sur `mhgp6.cpp` :
   `status_exit_code` rend 2 et le message commence par
   `resource_exhausted`) ;
4. `134` : diagnostic exact `std::bad_alloc` **ET RLIMIT_AS attesté**.

Codes 3, 127, 139, non décimaux et signaux non attribués invalident la
phase. `limit_kind`/`limit_kb` sont **gravés dans chaque statut** et en
tête du résumé ; le plafond du plan est **lié à la commande exécutée**
(jetons `ulimit` + valeur exigés dans `commande=`). Le motif fatal
(INVARIANT, DIVERGENCE, PLANCHER, sanitizer, Killed, command not found,
segfault) est scanné sur TOUTES les classes et n'est jamais masqué par un
motif de capacité. Durée décimale exigée. Le résumé rappelle : frontière
**sous plafond virtuel RLIMIT_AS**, pas le mur RAM natif.

Les huit probes de votre table meurent désormais chacun sur sa cause :
code=abc → « code non décimal » ; code=3 + bad_alloc → « HORS des quatre
classes » ; INVARIANT sur 9/0/124 → « motif FATAL » ; duree_s=abc →
« durée non décimale » ; argument décoré → « sans le jeton exact » ;
commande sans ulimit → « sans liaison ulimit au plafond du plan » ; plus
« 124 sans attestation » et « 134 sans std::bad_alloc ».

## Point 4 — mutants causaux

- Deux helpers distincts : `falsify_transport` (corruption SANS rehash,
  cause = le contrôle de transport) et `falsify_semantique` (mutation PUIS
  **recalcul complet de `MANIFESTE_DISTANT.txt`**, diagnostic **exact**
  exigé par motif). Tous les mutants sémantiques (code, durée, motifs
  fatals, argv, digests GPU, compteurs, plans, vacuité fils) empruntent le
  second chemin — la béquille du hash de transport est retirée.
- **Publication interrompue RÉELLE** : nouveau test d'intégration — un
  `sitecustomize.py` fait lever `EIO` au `os.link` du VRAI publisher sur le
  registre ; le garde refuse AVANT toute mutation (aucun `instances start`
  au faux gcloud), **ni fichier final ni `.partial` orphelin** (le
  publisher nettoie désormais le temporaire sur TOUT échec du lien), zéro
  arrêt, reçu `refus_avant_mutation`. Le mutant fabriqué du selftest est
  conservé comme témoin du chemin absent.
- Ordre des mesures : **GPU déplacé APRÈS le bench** — ordre final :
  accord différentiel → fils → queue → bench (toutes les mesures CPU) →
  GPU historique → frontière. Le build nvcc `-j8` ne peut plus contaminer
  un mur CPU.
- Wording de `g4_mesure_v1` purgé : « invariance du grand-livre » (jamais
  « bit-identité »), « observations GPU historiques non autoritaires »
  (jamais « gain GPU »), « exploratoire tronquable ».

## Rejeu depuis le HEAD propre du commit livré

- `selftest_campagne_v6.sh` : **56 vérifications**, code 0 (profil G4
  exact 81 runs, huit mutants de frontière, mutants coordonnés) ;
- `selftest_cycle_vie_v6.sh` : **22 scénarios + 11 refus de pin**, code 0 ;
- `tests.gcp` : **83/83** (81 sûreté + 2 intégration dont l'EIO réel) ;
- `git diff --check` propre ; `check_docs` 241 fichiers.

## Limites maintenues (pas de sur-claim)

Le reçu est « atomique et cohérent après exécution » + `sync` final —
nous ne revendiquons pas de durabilité fsync-par-fichier. La frontière
mesure un plafond RLIMIT_AS, pas le mur RAM natif. Les murs fils/GPU sont
des observations, jamais des claims de speedup ou de gain. La liaison
bootstrap/pin n'est pas cryptographique.

Demande : **verdict frais sur le commit porteur**. La session
`CAMPAIGN_PROFILE=g4_mesure_v1` ne partira que sur votre GO — condition de
l'exploitant.
