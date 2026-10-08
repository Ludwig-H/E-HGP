# MES-C petits : fermer la cohorte avant le verdict

Prélecture du prototype `microbancs/mes_c_petits/pilote_c.py`, encore non suivi
à la capture du 8 octobre 2026 vers 05:58 UTC, SHA **66ef2d8b…**. Aucun résultat
réel MES-C n'est invalidé : ce reçu précède la campagne. Aucun natif ni GCP.

Le pilote annonce C3 sur **tous les nuages difficiles, K5, CPU et appareil,
48 fils**. Pourtant `verdicts` retire les lignes `non_joue`, puis accepte dès
qu'il reste au moins un succès sans échec. Trois témoins synthétiques donnent
donc C1=C2=C3 tenus : dernier cas non joué, CPU seul, ou tous les cas à un fil.
Une ligne absente ou dupliquée échappe aussi au contrôle. Les boucles du pilote
enregistrent les cas non joués sans ajouter de contrôle manquant ; le verdict
global peut alors devenir `tenu` sur un sous-ensemble. Une expiration effectivement
jouée reste correctement négative : elle ne doit pas être confondue avec l'absence.

Le [patch proposé](cohorte_proposed.patch), **non appliqué**, transmet les noms
attendus depuis le manifeste et exige une occurrence exacte de chaque paire
(nuage, CPU/appareil), à K5/W48, sans cas non joué. Une cohorte incomplète devient
`non evalue` ; un critère non évalué rend le verdict global `refuse`. Les refus ou
expirations d'une cohorte complète restent `non tenu`. K10 reste informatif pour C3.

[check.py](check.py) applique/inverse le patch sur copie, contrôle les hashes,
extrait seulement la fonction par AST et juge douze scénarios, plus une liste
attendue vide. Normal et −O donnent les mêmes résultats ; C1/C2 sont inchangés.
[Contrelecture indépendante](contrelecture.json) : sept sélections globales vérifiées
par AST, avec refus observé, contrôle absent, critère non évalué et mode essai.
Aucun rejeu n’exécute le pilote complet ni ses commandes. Les cas sont inventés et les
noms ne sont pas ceux d'un jeu de données. Le correctif ne prétend pas qualifier
les archives, toutes les validations de manifeste ou les autres critères.

Point de méthode distinct : C2 utilise une **pente ajustée** sur le groupe réel,
comparée à 241,3 ms / 64 740 sites. Ce choix explicite ne prouve pas à lui seul
que chaque nuage respecte un plafond de coût par site. Garder les points et
résidus dans l'analyse ; déclarer cette interprétation avant la campagne.

```sh
python3 -B check.py --source COPIE_EPINGLEE/pilote_c.py
python3 -B -O check.py --source COPIE_EPINGLEE/pilote_c.py
```

Sources/postimage : [pins.json](pins.json). Cas rejoués : [results.json](results.json).
