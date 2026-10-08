# B3 : preuves d'identité non relues par le juge

Capture publique du **8 octobre 2026 à 18:03 UTC**, pilote encore non
commis, SHA `6a798b31…60f2`. Aucun appel moteur, compilation, GCP ou
lecture de données : contre-exemples JSONL synthétiques issus de la
fixture publique du lecteur FULL.

Avec **`verifier_journaux=True`**, le pilote relit strictement les journaux
de temps, mais `juger_identite` et `juger_resolution` font confiance aux
résumés `valide`, `ful1` et `resolution`. Ils ne réadmettent pas les
primaires auxquels ces résumés prétendent se rapporter.

La campagne témoin contient **300 journaux de temps / 3 000 passes FULL**,
25 identités FUL1 / 50 passes et 10 résolutions. Ses trois leviers sont
adoptés. En conservant exactement les mêmes temps et règles :

| Mutation indépendante | Résultat du juge capturé |
| --- | --- |
| Les 35 journaux d'identité/résolution sont supprimés | Trois leviers adoptés |
| Hash FUL1 faux et code 137, résumé `valide=True` | Trois leviers adoptés |
| Journal de résolution remplacé par une ligne en échec | Trois leviers adoptés |
| Résumés d'identité sans chemin, hash ni code | Trois leviers adoptés |
| Contrôle : résumé FUL1 contradictoire | Trois leviers rejetés |
| Contrôle : journal de **temps** modifié | Trois leviers refusés |

Cela prouve un défaut de réadmission des preuves, **pas** une identité
B3 réellement fausse ni un faux gain mesuré. Aucun résultat G4 B3 n'est
jugé ici. Les résolutions synthétiques utilisent le petit schéma admis
par `lire_resolution` ; ce lecteur initial ne contrôle lui-même ni la
configuration complète ni la fin du processus.

Correction ciblée à proposer : réadmettre les **25 FULL d'identité**
(deux passes, K5, configuration de leur place) et les **10 résolutions**
(une passe), avec chemin local unique, hash, code entier, stderr et
configuration attendue ; recalculer les résumés avant comparaison. Une
preuve absente ou altérée doit faire **refuser** ; une identité différente
sur des preuves admises doit faire **rejeter**. Conserver les seuils,
les tirages du bootstrap et les cohortes de mesure.

Comparaison de mécanisme, sans transfert de qualification : le juge
T1-d appelle `check_identity`/`check_ful1`, lesquels relisent les sorties
natives avec les options attendues ; `read_full` exige notamment le code
entier 0, les lignes `open`, `full`, `liberation`, `exit` et le nombre exact
de passes. T1-d utilise des lignes natives incorporées dans ses objets
`run`, avec un schéma FULL **séquentiel explicite**. B3 pointe vers des
fichiers JSONL du schéma **recouvert** : il doit conserver ce schéma et
fermer le raccord fichier/résumé. Ce constat ne confond ni ces formats
ni les modes de preuve par archive et par dossier de journaux.

La capture reste externe, les fichiers de recette ne contiennent aucune
donnée ni journal de scène réelle. Rejeu du témoin et des six mutations :

```sh
python -B -S check.py --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/b3_identite_20261008
python -B -S -O check.py --snapshot /workspaces/.ehgp-auditors/evidence-snapshots/b3_identite_20261008
sha256sum -c SHA256SUMS
```

Le lecteur vérifie les empreintes des six modules capturés et compare les
résultats enregistrés. Aucun fichier du développeur n'est modifié.
