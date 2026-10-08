# Session K : supplément de provenance, 8 octobre 2026

**Observation locale à 02:36:54 UTC, aucune mesure nouvelle.** Le contrôleur
a été lancé à 02:31:09 ; son journal atteste le lancement du worker par code
0. À l'observation, l'état était `targeted_running`, `results/` était vide et
aucun reçu final ni arrêt certifié n'était présent. Cela n'identifie pas le
sous-bras distant actif et ne présume pas de l'état lors d'une lecture future.
Aucun appel GCP, contact/interruption du contrôleur ou test natif par l'audit.

Le paquet local de `v12.20261008.fullk` est un **worktree_snapshot/dev_snapshot**,
tête `c9ac60f20c741d9d493f4900fd8aec4590aaa5c5`, 34 entrées d'état et 2 262
fichiers. Les hashes complets du paquet, du manifeste, des plans et des quatre
sources relues figurent dans `capture.json`. Ce mode de capture ne devient
pas une qualification de commit parce que certains fichiers sont identiques.
Le plan local était égal au plan empaqueté, SHA-256 `24cdb3e5…`.

| Ordre prévu | Périmètre | Délai du plan |
|---|---|---:|
| MES-FULL | W48 ; ng00–02/K5 appareil 5 processus × 10 passes ; K10 appareil 3 × 5 ; K5 CPU 3 × 5 ; Session de 37 trames v12set, 5 processus × 2 tours, second tour compté | 2 400 s |
| MES-D6 | ng00–02/K5, profils 21/24/32, W48, 5 passes × 3 tours ; facteurs internes selon admissibilité | 1 800 s |

Budget global du contrôleur : 4 200 s. Le worker lance les commandes dans
cet ordre et consigne leurs statuts séparément ; il n'interrompt pas cette
boucle au seul premier code non nul. Aucune exécution complète de ces étapes
n'est déduite du lancement. Aucun résultat n'a été rapatrié à l'instant lu.

**FULL est bien prévu.** Il utilise son propre
`microbancs/mes_full/pilote_full.py`, SHA `a89ceec9…`, et la sonde
`bench/full_probe.cpp`, SHA `815cb33f…`. Ce pilote admet les flux dans
`parse_process`, construit les statistiques et rend le verdict via `contract` ;
il ne passe pas par D6 ni M6. Il rend code 0 même pour un verdict `refuse` ou
`non tenu` : un statut de commande réussi ne qualifie pas le contrat.

Le pilote FULL empaqueté est exactement celui désormais livré en `c9ac60f20`
et capturé auparavant dans le reçu immuable
[`mes_full_admission`](../mes_full_admission/README.md), publié `f6eaa4a66`.
Ce supplément rattache cette identité à K sans modifier le reçu antérieur,
qui décrivait correctement une version encore non commise à son instant.

**D6 utilise l'ancien lecteur**, SHA `48f40fd6…`, égal à `c9ac60f20` et
différent de `e37fd8935`. La correction des schémas G et l'admission stricte
du plan sont absentes de ce paquet. Le refus de ses lignes G est attendu
à la lecture du code ; ce n'est pas encore un refus observé dans un résultat
K. `e37fd8935` ne change ni le pilote FULL, ni sa sonde, ni sa petite porte.

## Admission des futurs bruts

Le reçu antérieur démontre des lacunes de juge, **pas la fausseté des futures
mesures brutes**. Avant d'utiliser K pour un contrat, relire les bruts avec
les contrôles stricts proposés dans ce reçu, en particulier :

- Cohorte exacte et distincte du manifeste figé, couverture des processus,
  passes et deux tours, correspondance effective des étiquettes de trames
  et des comptes de sites. Le nombre 37 seul ne certifie pas cette cohorte.
- Identité source/construction/commande et profil u21, W48, K, voie et trame ;
  phases ouverture, passe, libération, sortie ; types numériques, durées
  non négatives et inclusions des chronos ; SHA-256 valide et identité FUL1
  entre passes/processus/voies sur chaque trame/K.
- Preuve GPU disponible et vide avant **et** après : `gpu_apps=None` est une
  preuve manquante, jamais un appareil attesté isolé. Conserver le diagnostic
  de matériel/construction et l'arrêt certifié de la session.

La règle écrite du pilote porte sur les médianes et le maximum des médianes
par processus ; son maximum individuel est un diagnostic séparé. Les bruts
permettront d'examiner ces périmètres, sans inventer un temps absent. Le
verdict du pilote actuel restera provisoire tant que cette admission et la
clôture ne sont pas établies. Aucun nouveau constat ni clôture dans ce lot.

```sh
python check.py --repo /workspaces/E-HGP --session-dir /workspaces/.ehgp-sessions/v12.20261008.fullk
python -O check.py --repo /workspaces/E-HGP --session-dir /workspaces/.ehgp-sessions/v12.20261008.fullk
```

Le lecteur vérifie Git et, avec l'option, le paquet local avant/après. Il ne
lit aucune donnée de scène et n'appelle aucun contrôleur. L'état historique
`running` n'est volontairement pas rejoué ni exigé au présent. Sans l'option,
seul le rattachement des quatre sources à Git est vérifié.
