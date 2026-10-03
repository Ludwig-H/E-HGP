# Adaptive3 — échec de qualification conservé

Source exécutée : `f718f53aa9f76ee19f6be0dc50c1897801f32c1f`. Session du 3 octobre 2026 :
`/workspaces/.ehgp-sessions/v11.20261003.adaptive3`.

La matrice compte **2 474 portes passées sur 2 475**. Les configurations fonctionnelles présentes passent ;
Clang est absent, conformément à son caractère facultatif. Les mutants atteignent **20/21 portes** :
la porte `mhgp11_mutants_tower` commence, puis la fenêtre de la configuration expire après **700,494 s**.
La construction et la liste des tests ont réussi. Le CTest interrompu n'a pas produit de JUnit final ni de
verdict tower ; cet épisode ne prouve ni un défaut géométrique ni la réussite de cette porte.
La configuration dispose de **12 fils**, fixés pour son exécution ; aucune redistribution ultérieure
des ressources n'est attestée par cette capture.

Le supplément **ASan/UBSan 18 bits passe 178/178**. Le banc adaptatif refuse ensuite la qualification
incomplète, code 2 : **zéro tentative**, aucune intention de mesure et aucun `adaptive.json.gz`.
Aucun temps adaptatif, gain moteur ou réutilisation de décodage n'est acquis ici. Les 36 positions du
calendrier restent non jouées ; ce ne sont pas 36 omissions expérimentales produites par le banc.
La sortie précise `unstarted_declared_units=36` et `unpersisted=0` : aucun résultat lancé n'est perdu.

La fermeture est certifiée sur la même génération de l'instance
`ehgp-v7-3b1d496aed430749ea7e049f`, `us-central1-c`, projet `devpod-gpu-exploration` : état `TERMINATED`,
clé OS Login retirée, clé privée supprimée et réservation libérée, sans erreur ni avertissement.
Le reçu original reste intact et obligatoire pour la lecture LIVE.

`check.py` contrôle les empreintes des copies compactes, l'archive complète et son manifeste,
les commandes et profils exécutés, les inventaires CTest, l'interruption de la dernière porte,
la fermeture et l'absence de benchmark. Son code 0 signifie **preuves cohérentes** ; le champ
`conforming` reste `false`. Les sorties détaillées des campagnes mutantes sont incomplètes :
le lecteur rapporte les portes CTest passées, sans certifier à nouveau les morts individuelles.
Il émet donc `causal_mutants=null` et `individual_mutant_verdicts=not_certified`.

Les helpers de banc et de transport sous `frozen/` sont des copies explicites du commit, vérifiées
avant tout import. Ils totalisent environ 165 Ko. La préparation du lecteur de banc est conservée :
gzip borné à 128 Mio décompressés, 36 positions exactes, diagnostics par tâche, comparaisons entre
profils et contrôles de réutilisation par contexte complet, taille, SHA brut et origine antérieure validée.
Ces branches passent uniquement des contretests synthétiques dans cette capsule. La limite d'archive
du plan reste 64 Mio. Aucun payload canonique ou KITTI, paquet source ou nouveau résultat natif n'est copié.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 check.py
PYTHONDONTWRITEBYTECODE=1 python3 -O check.py
PYTHONDONTWRITEBYTECODE=1 python3 selftest.py
PYTHONDONTWRITEBYTECODE=1 python3 -O selftest.py
```

Les quatre sorties sont conservées dans `checks.json`. La première erreur de préparation du lecteur
(ordre des imports figés) est conservée séparément dans `reader_initial_failures.json`.
