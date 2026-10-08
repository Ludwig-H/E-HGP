# Admission T2-d-A : contre-exemples sur le pilote en préparation

**Capture du 8 octobre 2026, 04:49:09 UTC**, A/w8da non commis, base déclarée `8da450ab7`.
Pilote `cb6bbfd02e45…`, sonde `50d628000026…` et bornes produit épinglés dans [pins.json](pins.json).
`phase=exploration_v12_hors_registre`, `public_status=not_claimed`. Aucun moteur, build, GCP ni donnée LiDAR.
Les sources complètes sont gelées hors Git ; seuls ce lecteur de preuve et des résultats petits sont déposés ici.
La refonte annoncée ensuite (base `902041f66`, option `--recouvert`, objet `fenetres_ns`) reste à contre-lire :
ces verdicts concernent exclusivement le pin ci-dessus.

[check.py](check.py) génère des **JSON synthétiques**, selon les lignes publiées par les sondes avant/après
(open, full, liberation, exit ; champs CPU/RSS, appareil, empreinte optionnelle et recouvrement). Ce ne sont ni des
chronos ni des résultats géométriques. Il appelle les véritables `lire_prise` puis `juger(..., verifier=True)` :
les journaux sont relus, leurs hashes contrôlés et les résumés recalculés. Le témoin complet est admis ; il comporte
les identités K5/K10 des trois trames, les trois tailles uniformes, 37 noms synthétiques et ng00 à un fil, puis
5 paires de processus × 10 passes pour chacune des trois trames décisives.

Les [résultats normal et −O identiques](results.json) montrent :

| Mutation isolée | Résultat du lecteur/juge actuel |
| --- | --- |
| Mur de chaque passe « après » remplacé par 1 ns, P+C+G+TMVR conservé à 55 000 ns | **adopte**, aucun refus |
| Cohorte d'identité réduite à ng00 K5 ; autres trames, K10, uniformes et 37 trames supprimés | **adopte**, aucun refus |
| `queue_ns=0`, alors que `fin_ns−fin_g_ns=15000` et TMVR=15000 | admis |
| `threads=true` pour une commande à un fil | admis |
| `liberation.pass=false` pour la passe 0 | admis |
| Suppression de `cpu_ns`, que cette sonde publie toujours (valeur ou null) | admis |

Le mur impossible résulte de l'absence de garde de partition dans `lire_prise`. L'identité tronquée résulte du
parcours des seules clés présentes par `juger_identite`, sans cohorte attendue fermée. Le re-hachage des bruts
n'élimine aucun de ces deux défauts. Une auto-épreuve statistique utilisant `verifier=False` ne les couvre pas.
Ils doivent bloquer l'adoption du pilote avant la campagne, sans révoquer aucune mesure passée.

Correction attendue, à adapter explicitement au prochain schéma : valider les égalités directement émises
G=`fin_g_ns`, TMVR=`queue_ns`=`fin_ns−fin_g_ns`, les bornes internes et P+C+G+TMVR≤wall ; garder `tour_ns` comme
mur extérieur de l'appel. **Ne pas exiger T+M+V+R≤wall ou ≤TMVR** : ce sont des fenêtres de tâches recouvertes
(le témoin positif utilise justement T=80000 et wall=70000 à W48). Typer les métadonnées et indices avec des entiers
stricts, distinguer null de champ absent, fermer la cohorte d'identité et les configurations à partir du protocole,
puis recalculer les résumés. Aucun ordre entre tâches indépendantes n'est supposé ici.

Rejeu léger, avec la source épinglée du pilote :

```sh
python check.py --pilot /chemin/vers/pilote_t2d_a.py
python -O check.py --pilot /chemin/vers/pilote_t2d_a.py
```

Le script refuse une autre empreinte et échoue si ces verdicts changent. Il ne lance aucun sous-processus moteur.
La géométrie, les valeurs FUL1 et la performance réelle ne sont pas jugées par ces JSON de contrôle.
