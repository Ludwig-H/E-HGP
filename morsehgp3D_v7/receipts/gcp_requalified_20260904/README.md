# Projection publique de la session GCP

L'export réel a été exécuté après revue du script et arrêt ciblé certifié.
Le [reçu publié](published/receipt.json), ses [sources](published/source_pins.json)
et son [manifeste](published/manifest.json) sont conservés sans modification.
Publication create-only réussie, synchronisation du parent confirmée.
Le [complément matériel sélectionné](environment_selected.json) contient
uniquement des champs non privés, reliés aux hashes des sorties originales.

`export_public.py` est inerte sans `--execute`. Aucun reçu de session réelle
n'est créé par la préparation ni par les tests ; la publication réelle
ci-dessus a nécessité le GO explicite de root après arrêt ciblé certifié.

```bash
python3 -B morsehgp3D_v7/receipts/gcp_requalified_20260904/selftest.py
python3 -B -O morsehgp3D_v7/receipts/gcp_requalified_20260904/selftest.py
```

L'export vérifie l'état terminal du contrôleur épinglé, la génération, la
cible exacte (projet/zone/instance/id), les champs sélectionnés du contrôle
`TERMINATED` et le seul marqueur `double_guard_verified`. Les durées STOP3600s
et invité45min restent explicites. Ce script ne contacte jamais GCP/SSH et
ne réarrête jamais une ressource.

La liste blanche lit `session.json`, `prepared.json`, le manifeste des
sources, les trois fichiers structurés `control/{stop.json,post_stop.json,
post_stop.stdout}`, le marqueur sélectionné, puis les JSON scientifiques et
d'outillage présents dans le manifeste de résultats certifié par le
contrôleur. Pour conserver les coûts, seuls les stdout CPU/candidat complets
et celui du CTest GPU sont lus, vérifiés par SHA et projetés par expressions
régulières fermées en chiffres/noms de tests connus. Aucune copie brute de
stdout, stderr, argv, journaux start/SSH, worktree_status, clé, compte, email
ou réseau n'est publiée. Les raisons libres restent identifiées par SHA.

Les digests et cardinalités sont conservés seulement lorsque le contrôleur
a achevé sa revalidation sémantique. Un échec/censure reste un échec/censure,
une paire incomplète n'acquiert pas `equal`, et un résultat fatal partiel est
marqué `raw_manifest_and_terminal_only`. Les temps processus (génération et
digest inclus), compteurs, RSS et timers par étage sont distincts ; fold et
digest cumulés ne sont pas additionnés au temps mur. Le candidat d'incidences
garde sa sémantique `normalized_horizontal_h0_candidate`, sans promotion
d'exactitude ; ses compteurs MEB/query/chaînes sont conservés par ordre.

Après revue et GO root uniquement, avec le chemin privé déjà connu de root :

```bash
python3 -B morsehgp3D_v7/receipts/gcp_requalified_20260904/export_public.py \
  --execute --session /CHEMIN/PRIVE/DE/LA/SESSION
```

La destination par défaut est `published/` sous ce dossier. Elle doit être
absente. Les trois fichiers (`receipt.json`, `source_pins.json`,
`manifest.json`) sont préparés dans un provisoire privé puis publiés par
rename atomique **create-only** ; rien n'est écrasé. Un échec avant commit
peut laisser un provisoire privé `.public-export-*`, jamais un faux reçu
publié. Un échec de fsync du parent après commit est rapporté explicitement.
Le reçu privé vérifié par le contrôleur reste l'autorité ; cette projection
explicite n'en est pas un remplacement ni un certificat de SLO.
