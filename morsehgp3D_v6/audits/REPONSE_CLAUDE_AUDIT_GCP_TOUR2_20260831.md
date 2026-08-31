# Réponse Claude au deuxième tour de l'audit GCP — les deux P0 résiduels et les six P1 exécutés

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé — le NO-GO est respecté, aucune session lancée ; votre
porte de réouverture (audit statique frais concluant GO) reste le préalable.
Vous aviez raison sur la surqualification de ma réponse précédente : le
« 10+7 vert » n'était pas rejouable depuis un HEAD propre — c'est corrigé et
prouvé ci-dessous.

## P0-2 — plus aucun pin du worktree avant authentification

`session_campagne_v6_g4.sh` est refondu en DEUX ÉTAGES (votre recette) :
l'étage 1 capture UNE fois `SOURCE_COMMIT`, matérialise bootstrap ET pin par
`git show`, compare la copie en cours au bootstrap matérialisé (refus
sinon), puis `exec` l'étage 2 (la copie du commit). L'étage 2 exécute le pin
MATÉRIALISÉ avec le commit IMPOSÉ (`v6_campaign_pin.sh WORK COMMIT`, plus
aucun `rev-parse` dans le pin), puis vérifie pin exécuté == pin du
manifeste, puis `exec` le cycle de vie épinglé. Le point d'entrée de
confiance maximal est documenté en tête du bootstrap (matérialisation
directe depuis le commit, `MHGP6_BOOTSTRAP_COMMIT`).

Le manifeste est désormais CANONIQUE : `schema` + `commit` + une ligne
`sha256<TAB>taille<TAB>chemin` par fichier dans l'ordre normatif —
`protocol_manifest_sha256` est le SHA-256 de CE manifeste (les frontières de
fichiers sont liées) ; le cycle de vie le RECALCULE depuis les copies
matérialisées et refuse toute divergence avant d'exécuter quoi que ce soit.

Vos deux contre-scénarios sont gravés dans `selftest_cycle_vie_v6.sh` :
**pin altéré qui neutralise son propre contrôle** (contrôle `git diff`
commenté dans le worktree du clone + garde altérée → le bootstrap
matérialise le pin DU COMMIT, qui refuse — le pin altéré n'est JAMAIS
exécuté) et **bootstrap altéré** (refus d'identité de l'étage 1).

## P0-1 — enregistrement d'état partagé, terminal `targeted_stopped`

`--mutation-witness-file` est remplacé par `--lifecycle-state-file` dans
`start_and_verify.sh` : un enregistrement ATOMIQUE au patron du handoff
(chemin absolu non symbolique, temporaire dans le même répertoire, fsync du
fichier, rename, **fsync du parent**, jamais d'écrasement à la création),
publié juste avant la demande de start avec la sémantique conservative que
vous avez nommée (`start_may_have_been_requested`), mis à jour avec la
GÉNÉRATION dès qu'elle est capturée, puis par le trap interne :
`targeted_stopping` / `targeted_stopped` / `targeted_stop_failed`, et
`targeted_running` au succès. Les 81 tests de sûreté restent verts.

La table du cleanup extérieur reconnaît le TERMINAL PARTAGÉ : état absent ⟹
propagation sans arrêt ni blocage ; `targeted_stopped` (cible exacte
vérifiée) ⟹ ni second arrêt ni faux blocage, message « arrêt déjà certifié
par le garde » — votre scénario du faux blocage 71 est mort, et il est gravé
(scénario 9). Un handoff corrompu ne bloque plus quand l'état porte la
génération (scénario 3 : UN arrêt ciblé). `targeted_stop_failed` ⟹ UNE
retentative délibérée et documentée. Génération illisible après mutation
attestée ⟹ blocage 71, inchangé.

## Selftest rejouable depuis un HEAD propre

Le `git commit` du clone est conditionnel (`git diff --cached --quiet ||`),
défaut du premier tour corrigé. Rejeu au commit livré `9b21544c`, worktree
propre sur les chemins du protocole : `selftest_cycle_vie_v6.sh` → **12
scénarios + 10 refus de pin, 0 échec** ; `selftest_campagne_v6.sh` → 24
verts ; `test_gcp_safety` → 81/81.

## P1 — tous pris

- **Handshake non mutant** : SSH n°1 ne lit QUE `boot_id` (validé
  `[0-9a-f-]{36}`), encadré par deux `describe` de génération ; le build et
  la campagne REVÉRIFIENT ce boot_id en tête de leur propre commande
  distante ; l'envoi du bundle et chaque rapatriement SCP sont encadrés
  avant ET après (un rapatriement dont la génération a changé après coup
  n'est plus attribuable et le dit).
- **Profils canoniques** : `gcp-migration/profils/{decision_v1,smoke_v1}.env`
  versionnés, matérialisés par le pin, hashés au manifeste ; TOUTE
  surcharge d'un axe dégrade le profil effectif en `custom` — un validateur
  ne peut plus appeler `decision` une matrice réduite concordante.
- **Conformité aux tailles mesurées** : `decision_v1` porte les QUATRE
  familles partagées à TOUTES les tailles (32000/50000/100000/200000) ; le
  bench passe aux paires `BENCH_SPECS` (plans v2) — à 100000/200000 il ne
  mesure que des paires dont la conformité est couverte.
- **TTL OS Login** : `(MAX_RUN_SECONDS + 600)/60`, garde arithmétique
  locale contre la fenêtre `MAX_RUN_SECONDS + 660`, et fixture sur les
  valeurs par défaut (scénario 10 : le TTL demandé au faux gcloud est lu et
  vérifié dans la fenêtre).
- **Reçu durable OBLIGATOIRE** : `DURABLE_RECEIPT_DIR` requis (le bootstrap
  le fixe sous `receipts/session_g4_<date>_<pin>/`) ; l'échec de
  publication n'est plus avalé — l'arrêt passe d'abord, puis rc 66 explicite
  si le reçu manque ; le profil entre dans `SHA256SUMS`, les résumés et le
  manifeste revalidé sont archivés.
- **Isolation gcloud** : `CLOUDSDK_CONFIG` privé à la session (copie de la
  configuration existante, credentials compris) — `config set project` ne
  mute plus l'état partagé ; `--project`/`--zone` restent explicites
  partout.

## Ce qui reste à vous

Les corrections des deux P0 vivent dans le même pin que leurs tests de
rupture (`9b21544c`) ; les selftests sont verts depuis un HEAD propre ; les
défauts passent réellement le préflight (budget estimé 21 160 s pour une
fenêtre de 26 400 s à 8 h) ; `decision` et `smoke/custom` sont distingués
par des profils canoniques hashés. Reste votre point 5 : l'audit statique
frais concluant GO — je ne lance rien avant.
