# A6b : correctif documentaire proposé sur le retrait publié

Base **8a0716e74**, capturée le 8 octobre 2026 à 17:54:51 UTC. Les deux
fichiers étaient alors exactement égaux à ce commit. Le retrait est
désormais publié ; les scopes tour, tests tour et manifeste des mutants
sont identiques à **47feedc96**. Ce reçu ne reprend donc pas l'état local
non commis observé plus tôt.

`proposition.patch` corrige uniquement `docs/PLAN.md` et le README du
reçu développeur `g4_a6b_20261008` :

- Les exemples nommés ne sont plus présentés comme médiane et maximum
  de la cohorte. Les agrégats admis des 21 grandes trames sont indiqués :
  médiane des médianes **152,754103 → 132,303583 ms**, pire médiane
  **289,254605 → 241,637358 ms**, maximum brut **291,917600 → 242,388635 ms**.
  Le rapport géométrique apparié **0,849071** reste distinct du quotient
  des médianes globales ; les 21 trames restent au-dessus de 100 ms.
- Le rejet de ng00 vient de la **borne haute 1,011640**, alors que son
  rapport central **1,008600** est inférieur à 1,01. La borne haute de
  ng01 vaut **1,019481**. La règle ne change pas.
- La priorité porte sur les tranches G **encore réclamables**. Toutes
  peuvent être réclamées alors que certaines restent actives ; le
  compteur ne démontre pas que les aides attendent la fin de G.
- Le retrait est épinglé à **8a0716e74** ; la mention antérieure d'une
  A6b encore à concevoir renvoie maintenant à son évaluation.

La proposition n'est **pas appliquée** au travail du développeur. Les
empreintes avant/après sont dans `capture.json`, avec l'empreinte des
[mesures indépendamment admises](../session_a6b_admission/README.md).
Le reçu d'admission des mesures n'a pas été modifié. Le contexte du patch
est volontairement limité : il ne recopie aucune identité de compte ni
cible distante présente ailleurs dans le document développeur.

Le lecteur récupère les deux bases depuis Git, applique le patch dans un
répertoire temporaire, puis vérifie les postimages et les chiffres. Avec
`--live`, il refuse si un fichier courant diffère de la base prévue :

```sh
python -B -S check.py --repo /workspaces/E-HGP --live
python -B -S -O check.py --repo /workspaces/E-HGP --live
sha256sum -c SHA256SUMS
```

Pour une application ultérieure, vérifier à nouveau les **deux** empreintes
avant. Si le développeur a modifié l'un des documents, adapter explicitement
la proposition ; aucune application approximative n'est qualifiée ici.
Sans `--live`, le lecteur reste rejouable contre les bases Git historiques
et indique séparément l'état des fichiers courants.
