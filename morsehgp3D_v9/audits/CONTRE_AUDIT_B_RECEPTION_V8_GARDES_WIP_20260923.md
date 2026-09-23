# Contre-audit B — réception v8, preuve de garde archivée

23 septembre 2026. D'abord **chantier non commité** au-dessus du produit
`a78664d4`, puis publié sous `028067a3` dans le worktree du développeur ;
aucun appel GCP. Le worker/probe
préparent le schéma v8 pour publier le travail du noyau diamétral et
resserrer les identités de catalogue, de voies et de cache demandées dans
[l'audit v6](RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md). Les selftests
de session fondés sur `snapshot.build('HEAD')` ne sont pas jugeables
pendant que les fichiers exécutés diffèrent encore du commit transporté :
un échec de hash ou de schéma à cet instant n'est pas un reçu scientifique.

## Trou reproductible de liaison de la garde

Le contrôleur lit et valide une **marque** et un **calendrier d'arrêt**
précis avant le lancement (`tower_session_v9.py:363–370`), puis reçoit
`guard_evidence.json` de l'invité. La nouvelle fonction
`validate_received(..., generation, provenance)` rejuge certains champs
de cette archive, mais ne reçoit ni la marque ni le calendrier exacts
validés par l'hôte. Elle accepte donc une preuve archivée différente
tant que celle-ci satisfait ses larges bornes de plausibilité.

Un essai hors-ligne de session factice, sans GCP et avec le schéma WIP
rendu cohérent en mémoire, donne `completed` en baseline puis encore
`completed` après chacune de ces mutations **isolées** de la preuve
archivée :

Reproduction : importer `tower_selftest_v9` depuis
`build/v9-open-worktree/gcp-migration`, faire remplacer en mémoire par
`snapshot.collect` le seul `PROBE_SOURCE` commité v7 par le fichier WIP
v8, et remplacer dans le seul `FAKE_PROBE` en mémoire le marqueur de
schéma v7 par v8. `run_scenario` dans un `TemporaryDirectory` rend alors
`(code=0,status=completed)`. Dans `host/received/output`, copier puis
modifier seulement `guard_evidence.json`, et rappeler
`session.validate_received(output, pkg['manifest'], worker_pin,
expected_cases, receipt['generation'], receipt['provenance'])` : la baseline et
les deux variantes rendent `completed`. Ces substitutions de schéma
servent uniquement à tester le lecteur WIP sans committer un faux paquet.

| Seul champ muté | Valeur acceptée | Pourquoi c'est insuffisant |
| --- | --- | --- |
| `guard_evidence.mark.date_utc` | `2050-01-01T00:00:00Z` | La réception vérifie `date≥generation`, sans borne supérieure liée à la vérification hôte. |
| `guard_evidence.schedule.USEC` | `(epoch(generation)+1800)·10⁶` | Le calendrier archivé diffère de celui que l'hôte a lu, mais demeure dans la fenêtre générique acceptée. |

Cela **ne prouve pas** que la VM a démarré sans double garde : à
l'exécution, `guard_deadline` côté hôte et `guard_values` côté invité
contrôlent réellement leur propre marque/calendrier et l'arrêt ciblé
reste distinct. Le défaut est la fidélité du **reçu réanalysable** : un
lecteur ne peut plus déduire que les valeurs archivées sont celles que
l'hôte a vérifiées. Le selftest WIP mute déjà cible/schema/date trop
ancienne et calendrier illégal, mais pas ces deux substitutions encore
plausibles.

Correction minimale : transmettre à `validate_received` la marque et le
calendrier vérifiés par l'hôte (ou leurs empreintes conservées dans l'état
de session), puis exiger l'égalité avec `guard_evidence`. Conserver aussi
la borne `mark.date_utc≤temps de vérification+5 s` déjà imposée par
`guard_deadline`; si le calendrier a changé entre les deux lectures,
refuser conservativement le reçu plutôt que réécrire l'histoire.
Ajouter les deux mutations isolées aux selftests normal/`-O` sur un
**commit atomique** protocole+sonde, puis relire le snapshot avant tout
nouveau reçu G4. Les nouvelles identités de travail q3/q4 doivent être
jugées séparément ; cette note n'en tire pas un défaut géométrique.

## Contrôle du commit `028067a3` : porte protocolaire rouge

Le commit publie des compteurs supplémentaires du cœur, des identités
plus fortes et se présente comme une réception de garde « fail-closed ».
Il ne transmet toujours **pas** la marque et le calendrier exacts de
l'hôte à `validate_received` : la reproduction ci-dessus reste applicable.
Un second rejeu **du code commité** sur la session factice nominale
retourne `completed` en baseline et après mutation isolée de la date
archivée en `2099-01-01T00:00:00Z`, puis après remplacement isolé du
calendrier archivé par `generation+600 s` (calendrier hôte observé
environ `generation+2430 s`). Il s'agit d'un écart effectif du lecteur,
pas seulement d'une lacune stylistique de tests.
En outre, le selftest complet du commit ne passe pas : exécution
indépendante `python3 -B gcp-migration/tower_selftest_v9.py`, **20 tests
passés, 1 erreur** en 46,656 s. `test_nominal_session_completed` appelle
`rewrite_guard` à sa ligne 868, mais cette fonction a été définie à la
ligne 70 **à l'intérieur de la chaîne brute `FAKE_PROBE`** (ouverte ligne
49), et n'existe donc pas dans l'espace de noms du module de tests.
L'appel lève `NameError` avant de juger les huit nouvelles mutations de
garde. Le `-L gate` CTest C++ annoncé vert ne remplace pas ce selftest
Python. Déplacer la fonction hors de `FAKE_PROBE`, tuer les mutations
exactes date future/calendrier plausible, puis repasser normal **et**
`python3 -O` sur le même commit figé avant R6.
