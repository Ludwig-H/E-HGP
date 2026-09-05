# Qualification E/q2 préparée — aucune compilation ni porte moteur exécutée

Révision 3 : fermeture de la réserve LastTest avant exécution. `revision1/` conserve le protocole initial et ses 7 selftests ; `preparation.json` reste historique. `revision2_timestamp_failure/` conserve les sources et la sortie brute des faux rejets de précision temporelle découverts ensuite. L'autorité de préparation courante est `preparation_revision3.json`, avec `manifest_revision3.json` et `SHA256SUMS`.

`qualification.py` prépare trois campagnes nouvelles : **324 portes Release complètes**, **33 portes ciblées Release**, **33 ciblées ASAN/UBSAN**. L'inventaire complet est exactement celui des 323 portes D scellées, plus `mhgp7_meb_lazy_q2_reject_shell` ; le ciblé est exactement 32 plus ce même nom. Ce sont des **inventaires attendus**, pas la déclaration d'un CMake déjà mis à jour ni des résultats E.

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Les anciens runners et inventaires restent intacts. Les juges JUnit, les commandes et les neuf targets ciblés proviennent des runners fermés `run_full_release.py` / `run_targeted.py`, et le helper d'exécution est vérifié avant exécution des mêmes octets. Les résultats historiques ne sont jamais consommés comme résultats de tests nouveaux.

| Mode | Build futur | Reçu neuf |
| --- | --- | --- |
| `full` | `build/v7_next_q2_qualification` : **incrémental**, après construction du CLI E | `full_receipts/` |
| `release` | `release/` : neuf, neuf targets | `release_receipts/` |
| `sanitized` | `sanitized/` : neuf, neuf targets | `sanitized_receipts/` |

Les chemins relatifs du tableau sont sous le présent dossier. Les dossiers de reçus et les builds ciblés sont create-only ; un échec/interruption ne se reprend pas ni ne s'efface. Toute nouvelle tentative requiert un nouveau chemin revu. L'ancien D `build/v7_meb_qualification` n'est jamais configuré ni reconstruit par ce runner.

## Liaison à E, distincte des baselines historiques

Le reçu de build attendu est `build/v7_next_q2_build_20260905/build_D.json`. Le **nom de fichier est hérité**, mais son contenu doit désigner réellement E, ses sources, son build et sa compilation Release CPU. Le lancement exige deux SHA complets explicitement revus : celui du CLI E et celui de ce reçu. La validation stricte du runner de paire (source avant/après, cache/base du même build, commande sans TESTING/PROFILE ni flags étrangers, cible réelle `build_dir/mhgp7`, permissions et octets) est réutilisée. Le runner ne déduit pas silencieusement un nouveau pin depuis un binaire quelconque.

Trois CLI historiques ont un traitement séparé : **C à 25c9bf8e dans deux chemins, D à 127c5f92 dans un troisième**. Leur table est hétérogène ; il n'existe pas de contrôle erroné « tous les pins doivent égaler C ». Ces trois binaires et leurs reçus/builds restent protégés via la liaison historique. E Release est protégé par son propre reçu. Le CLI ciblé Release doit aussi égaler E octet pour octet ; le CLI instrumenté possède son propre SHA, qui n'est pas celui de E Release.

Sources complètes de qualification, inputs du runner, build E, historiques, binaires testés et cache/base/CTest sont comparés aux frontières des étapes et après collecte Git terminale. La qualification impose l'inventaire exact et le JUnit complet : ni manquant, doublon, failure/error/skipped, ni `status!=run`. Une sortie CTest 0 ne suffit pas. Chaque commande possède un reçu d'essai avant lancement, une sortie brute et un résultat terminal ; les groupes possédés sont drainés sur timeout/interruption. Aucun cache de résultats. Le journal LastTest, y compris après échec s'il existe, est copié sans modification et n'est jamais une preuve suffisante à lui seul.

## LastTest obligatoire, frais, fermé et conservé

L'ancien log éventuel du build complet est capturé avant les commandes : SHA, taille, device/inode et timestamps, avec copie exacte `LastTest.preexisting.stdout`. Avant le CTest possédé, le runner crée exclusivement un **fence propre à ce reçu dans `Testing/Temporary` du build exact**, puis en conserve les mêmes octets et métadonnées dans le reçu. Un fence existant ne se réutilise pas.

Le log nouveau doit être sur ce même filesystem, avec mtime/ctime au moins égaux à ceux du fence. La comparaison est non stricte pour la granularité de l'horloge fichiers ; l'UTC `time.time_ns()` demeure diagnostique et n'est plus comparée directement au mtime. Ce changement suit une contre-fixture réelle : un fichier réellement écrit après `time.time_ns()` peut avoir un timestamp kernel légèrement antérieur. Aucune durée positive n'est exigée des tests. Un log identique octet pour octet à l'ancien reste refusé, même retouché.

Avant tout verdict `passed`, la copie brute `LastTest.stdout` est réalisée et vérifiée. Le juge exige header, **tous les blocs de tests complets et passés**, noms et indices sans doublon, aucun nom manquant/étranger, et footer terminal ; l'ordre parallèle est permis. Absence, troncature, ancien log, footer seul ou copie altérée interdisent le succès. Les octets existants sont conservés même en cas de refus. Après jugement, le runner relit encore source, archive et fence et vérifie SHA/stat/identité : une modification pendant copie ou jugement interdit la qualification. Aucun succès synthétique sans log n'est désormais possible.

## Exécution réservée à un GO ultérieur

Prévisualisations inertes :

```bash
python3 -B build/v7_next_q2_tests_20260905/qualification.py --mode full
python3 -B build/v7_next_q2_tests_20260905/qualification.py --mode release
python3 -B build/v7_next_q2_tests_20260905/qualification.py --mode sanitized
```

Après revue, intégration produit et build E achevé seulement, ajouter `--expected-e-sha256 SHA_E --build-receipt-sha256 SHA_RECU --execute`. Les placeholders doivent être remplacés par les hashes effectivement vérifiés. Aucun lancement automatique par cette préparation. Ne pas chevaucher ces charges lourdes avec les paires de mesure mono.

Pour le mode instrumenté, le processus de runner doit recevoir exactement `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` et aucun `LSAN_OPTIONS`. Aucun `LD_*` non vide n'est accepté ; ses valeurs ne sont jamais imprimées. **Ne pas désactiver LeakSanitizer** : si le sandbox/ptrace empêche LSAN, conserver l'échec et demander l'escalade appropriée pour une nouvelle tentative ciblée. Cette préparation ne contourne ni les permissions ni le contrôle de fuite.

Bornes par étape : configure 120 s, build complet 3600 s ou ciblé 1200 s, inventaire 60 s, CTest complet 7200 s ou ciblé 1200 s ; parallélisme 2. Pas de plafond RLIMIT_AS=26 GiB hérité du banc : la mémoire virtuelle d'ASAN exige son espace de shadow. Ce sont des coûts de qualification sur hôte partagé, pas des chronos de pipeline.

## Selftests de préparation

Les neuf méthodes passent en Python normal et `-O` : trois inventaires/JUnit positifs et 24 rejets JUnit, trois previews, protection des options de sanitizers, treize contre-conditions terminales, SHA explicites obligatoires, contrôle strict des commandes/cache/build des neuf targets, et **33 scénarios synthétiques de lifecycle**, dont log absent, ancien et tronqué. Le juge de log relit aussi deux journaux historiques épinglés (sans réutiliser leurs qualifications), accepte l'ordre parallèle et réfute neuf corruptions structurelles. Huit scénarios d'archivage couvrent succès, absence, ancien, troncature, copie altérée, modification après jugement de la source ou de l'archive et fence étranger ; la réutilisation d'un fence est refusée. Aucune commande CMake/CTest ni aucun moteur n'est lancé par ces tests. Les fixtures de commandes et journaux sont des données, pas des mesures E. Les appels courants et leurs sorties sont dans `preparation_revision3.json` ; les selftests sont épinglés au CPU0 pour éviter le CPU6 des paires mono concurrentes.

Cette génération de protocole ne qualifie ni E ni un produit exact, ne démontre aucun SLO 50k/1 s/100 ms et ne réalise aucun passage massif/GPU. **GCP non utilisé.**
