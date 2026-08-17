# Audit ciblé après `e265f6` — une rupture SSH déclenche encore l’arrêt avant le rapatriement

Date : 17 août 2026.  
HEAD audité : `e265f6d55fdb5e29525c76ba8c64ac84d167bb60`.  
Pins de code concernés : `63d364a` (axial borné), `3792d56` (campagne transactionnelle), `79c5182` (RSS enfant).

## Verdict

Je reçois les développements mathématiques récents :

- l’axial borné conserve correctement les petites racines sur `B>0`, les grandes sur `B<0`, les égalités de frontière et le minimum canonique du groupe ;
- la porte appariée est substantielle et a effectivement détecté l’erreur initiale de direction ;
- le verdict CPU négatif est honnête, donc le maintien en opt-in est la bonne décision ;
- le format `juge=off desaccords=NA` et la séparation latence isolée / débit contendu sont corrects ;
- la correction `79c5182` mesure désormais le vrai enfant du wrapper dans le repli RSS.

Je ne vois pas de verrou de correction restant dans l’axial borné.

L’audit parallèle `AUDIT_CIBLE_3792D56_PIN_SOURCE_ET_RSS_G4_20260817.md` couvre déjà le pin exact de la source ; je ne le répète pas. Il reste toutefois un défaut transactionnel indépendant, avant la session payante :

> si la commande SSH de campagne rend un code non nul, le shell local quitte immédiatement et le trap arrête la VM **avant toute tentative de `scp`**.

Les statuts par run sont donc robustes lorsque la connexion tient, mais ils peuvent encore disparaître avec la connexion qui devait justement les rapporter.

---

## 1. Le chemin actuel n’atteint pas toujours l’étape annoncée « rapatriement TOUJOURS »

Le script local est sous :

```bash
set -euo pipefail
```

et lance la campagne par :

```bash
"${SSH[@]}" '... campagne distante ...' 2>&1 | tee -a "${LOG}"

# seulement ensuite :
gcloud compute scp ...
```

Avec `pipefail`, un code non nul de `gcloud compute ssh` fait échouer le pipeline. `errexit` déclenche alors immédiatement le trap `cleanup`, qui appelle `stop_and_verify`.

Cela arrive notamment si :

- la connexion SSH tombe après plusieurs runs terminés ;
- la VM préemptible perd sa session distante ;
- le shell distant est interrompu par le coupe-circuit invité ;
- une erreur d’infrastructure survient après l’écriture de plusieurs `.status`.

Dans ces cas :

```text
résultats distants potentiellement utiles
→ retour SSH non nul
→ trap EXIT
→ arrêt de la VM
→ aucun scp
```

Le statut final est certes non nul, mais les preuves partielles sont perdues. Le contrat écrit dans le script, « rapatriement TOUJOURS, puis validation », n’est donc pas encore réalisé.

---

## 2. Correction minimale : capturer le code distant, puis tenter le `scp` quoi qu’il arrive

Le bloc de campagne doit désarmer localement `errexit`, comme `run_one` le fait déjà pour chaque probe :

```bash
REMOTE_CAMPAIGN_RC=0
set +e
"${SSH[@]}" '... campagne distante ...' 2>&1 | tee -a "${LOG}"
REMOTE_CAMPAIGN_RC=${PIPESTATUS[0]}
set -e
printf 'remote_campaign_rc=%d\n' "${REMOTE_CAMPAIGN_RC}" | tee -a "${LOG}"
```

Puis tenter explicitement le rapatriement, avec quelques reprises bornées :

```bash
SCP_RC=1
for attempt in 1 2 3; do
  if gcloud compute scp --recurse ... 2>&1 | tee -a "${LOG}"; then
    SCP_RC=0
    break
  fi
  sleep $((5 * attempt))
done
```

La validation locale reçoit ensuite aussi :

```text
remote_campaign_rc,
scp_rc.
```

Le statut `complete` exige naturellement les deux à zéro. Sinon :

```text
campaign_status=partial_or_failed
```

mais les fichiers déjà produits restent disponibles pour diagnostiquer le dernier point atteint.

Le trap ne doit arrêter la VM qu’après ces tentatives de rapatriement. Une déconnexion permanente peut encore empêcher le transfert, phénomène contre lequel aucun script n’a de théorème magique ; elle doit alors être journalisée comme `scp_rc != 0`, pas transformer silencieusement une campagne partielle en répertoire local vide.

---

## 3. La phase de couverture ne doit pas utiliser un pilote de mémoire échoué

`run_one` retourne volontairement zéro même si le probe rend `124`, `137`, etc., afin que les statuts soient tous matérialisés. C’est correct pour une vague de couverture.

Mais les quatre runs isolés ont un rôle supplémentaire : leurs pics RSS pilotent la concurrence. Après eux, `conc_for` lit seulement :

```text
peak_rss_kb
```

sans vérifier :

```text
code=0,
finished=1,
timing_scope=isolated_latency.
```

Un pilote interrompu peut donc laisser un RSS numérique partiel, puis autoriser la phase 2 avec une capacité dérivée d’un run qui n’a jamais atteint son vrai pic.

Avant les vagues, ajouter une porte :

```bash
require_successful_pilot out/lat_uniform_n8000_smax11_rep1.status
require_successful_pilot out/lat_uniform_n16000_smax11.status
require_successful_pilot out/lat_uniform_n32000_smax11.status
```

qui exige au minimum :

```text
code=0,
finished=1,
peak_rss_kb numérique et > 0,
timing_scope=isolated_latency.
```

Si un pilote échoue, il faut sauter la couverture, rapatrier les statuts, puis rendre `partial_or_failed`. Lancer vingt-quatre runs sur une estimation de mémoire issue d’un processus avorté serait une utilisation assez littérale du terme « pilote ».

---

## 4. Porte shell suffisante

Un selftest court peut remplacer `SSH` et `scp` par des scripts factices :

1. la fausse commande distante écrit deux résultats, puis rend `255` ;
2. le faux `scp` copie ces fichiers localement ;
3. le test exige que le `scp` ait bien été appelé malgré `remote_campaign_rc=255` ;
4. le validateur rend `partial_or_failed`, avec les deux résultats accessibles ;
5. un pilote factice `code=124` doit empêcher le lancement de la phase de couverture.

Ce test protège le comportement causal recherché sans payer une minute de G4.

---

## Ordre utile

1. Exécuter le pin de source demandé par l’audit parallèle.
2. Capturer le retour SSH de campagne sans déclencher immédiatement le trap.
3. Toujours tenter le `scp`, puis valider localement les codes distants et le transfert.
4. Refuser la phase de couverture si un pilote RSS n’a pas réussi.
5. Lancer ensuite la campagne G4.

## Conclusion

Le protocole sait désormais distinguer une campagne complète d’une collection de fichiers heureux. Il lui manque encore la propriété plus primitive : **rapporter les fichiers avant d’éteindre la machine lorsque la session distante échoue**.

La correction est locale et ne remet en cause ni les mesures déjà publiées, ni l’axial borné, ni le pilotage RSS corrigé. Elle évite seulement que l’échec le plus intéressant soit aussi celui qui efface ses propres traces.