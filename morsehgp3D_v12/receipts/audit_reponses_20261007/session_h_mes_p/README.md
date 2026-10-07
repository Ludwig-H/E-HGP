# Session H — petits nuages et portée du coût fixe

**Les tableaux MES-P du reçu `136e07628` sont reproduits sur les agrégats publiés.** 840 prises pour 140 nuages,
838 succès et deux expirations K10 à un fil (uniforme et huit amas, 10 000 sites). Chaque succès conserve quatre
durées ; la médiane des trois dernières correspond à `chaud` dans la tolérance de l'arrondi publié (0,5 µs).
Les échecs n'ont pas de durée chaude. Les 123 nuages réels sont communs aux trois nombres de fils, à K5 et K10.
Les dix exclusions réseau/quasi-sphère restent explicites. La commande globale est coupée à son échéance :
`status=deadline_cut`, code 124 ; ce lot partiel n'est pas une campagne achevée sur les 149 nuages annoncés.

Cadre : exploration v12 hors registre ; référence v11 `ac081a06f`, CPU, u21 ; `public_status=not_claimed`.
Lecture d'agrégats, sans accès aux points, sans nouvelle mesure moteur ni opération GCP.

| K5, sites | prises par nombre de fils | médiane à 1 fil, ms | 4 fils | 48 fils |
| --- | ---: | ---: | ---: | ---: |
| 100–299 | 32 | 11,862 | 5,649 | 6,293 |
| 300–999 | 35 | 52,369 | 18,611 | 10,366 |
| 1 000–2 999 | 26 | 211,900 | 64,312 | 19,824 |
| 3 000–10 000 | 30 | 749,177 | 215,025 | 44,392 |

Les résultats K10, les médianes par site et les ajustements sont dans `result.json`. Les 32 petites scènes ont
des durées proches à 4 et 48 fils ; aucune identité des performances ni seuil v12 n'en découle. La droite ajustée
à K5 donne des intercepts −28,36 / −2,86 / 6,95 ms à 1 / 4 / 48 fils. Les valeurs négatives confirment qu'il
s'agit d'une description des scènes et tailles observées, pas d'une décomposition physique en coûts positifs.

## Précision sur le pool

Le reçu H conclut que le coût fixe « n'est donc pas celui du pool » puisque le mono est plus lent dès 100 sites.
Cette implication n'est pas valable pour le coût d'ordonnancement du pool pendant une passe. Un contre-modèle
en **microsecondes inventées**, pas un ajustement aux mesures, suffit : `T1(n)=100n`, `T48(n)=6000+4n`.
Le mono est plus lent pour tout `n≥100` alors que les 6 ms fixes du régime parallèle peuvent entièrement provenir
de son ordonnancement. Plus généralement, la baisse du coût variable peut masquer un surcoût fixe parallèle.
Le contrôle exécutable vérifie cinq tailles ; l'inégalité générale est `96n>6000`, soit `n>62,5`.

La **création** du pool est bien hors chrono, mais pour une autre raison vérifiable : la sonde v11 gelée prépare
le pool dans `run`, puis lance le chronomètre dans `full_pass`. La préparation du nuage est aussi hors chrono,
y compris entre les passes ; index, domaine et forêt sont dans `wall_ns`. Source relue à `ac081a06f` :
`bench/full_probe.cpp`, SHA256 `9d70956efddcef97be0a9153da5feb2417702402b46394ac778c6d78e2c49c2c`.
Une attribution à l'ordonnancement, aux barrières ou aux allocations demanderait une instrumentation ou une
ablation appariée du même travail. Aucun seuil CPU/GPU v12 n'est choisi à partir de ces temps v11.

## Vérifications et limites

`check.py` vérifie les identités de prises, le nombre de passes enregistrées, les médianes arrondies et les cohortes,
puis appelle le lecteur officiel courant en normal et `-O` : sorties identiques. Le contrôle lui-même a également
été joué dans ces deux modes, avec `result.json` identique. Pins des trois fichiers lus inclus dans le résultat.
La première invocation du script d'audit avait une racine relative erronée et n'a lu aucune mesure ; elle a été
corrigée avant les deux exécutions rapportées.

Les JSONL bruts de chaque petite scène ne sont pas présents dans ce reçu H : on ne reconstitue donc ni leurs
métadonnées, ni les sorties géométriques, ni leur provenance binaire depuis le seul résumé. Les agrégats ne sont
pas des preuves nouvelles de conformité. Le défaut générique d'un pilote permissif serait distinct : aucun
succès tronqué n'a été trouvé parmi les 838 enregistrements publiés. CST-0238 reste clos dans sa portée
séparation/cohorte ; cette précision ne rouvre pas ce correctif.

Rejeu depuis ce dossier : `python3 -B -S check.py`, puis `python3 -B -S -O check.py` ; comparer les deux sorties
à `result.json`. `sha256sum -c SHA256SUMS` contrôle le reçu.
