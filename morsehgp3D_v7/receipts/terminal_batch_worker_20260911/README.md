# Worker terminal par lots — préparation portable

Ce paquet ferme uniquement les sources du worker invité et ses tests Python
purs. Il ne contient aucun résultat d'exécution GPU ou GCP. La session réelle,
son arrêt ciblé et ses observations appartiennent à un reçu séparé.

Le snapshot contient 59 fichiers : gate compacte `98e426f2`, probe privé
`21d0a5dd`, leurs inclusions exactes, support `da967163`, adaptateur strict
`994d9e69` et probe historique requis mais jamais sélectionné. Les sources
communes sont liées à la capture SAN hôte de la gate ; ses résultats ne sont
pas transférés à un device. Le manifeste source est `803f1fcf…850f76`, le tar
`15f0abbd…90d2c4b` et le worker `043197e4…d2c0fb3`.

Les deux captures Python sont conservées intégralement : r1 puis r2, dont le
seul changement ajoute le plancher explicite 29 contrôles/231 rejets. Les modes
normal et `-O` passent dans chaque capture. Ce sont des messages synthétiques
pour les parsers/configurations, pas des géométries, benchmarks ou kernels.

Le contrôleur `177b25a0` et les gardes start `73d76c67` / stop `ddcad77a`
sont copiés sans modification pour provenance. Le worker ne les invoque pas
lui-même ; il utilise le support de processus et vérifie les deux échéances.
Il impose une fenêtre économique de travail de 900 s, sans la présenter comme
un plafond GCE de 15 min. Aucun garde, pilote, paquet ou quota algorithmique
n'est changé.

Le plan est : qualification device bornée, refus CLI et deux fautes causales,
paire CPU/GPU n200, premier 50k K1..10 GPU, repli K1..5 si nécessaire, puis
comparaison CPU48/GPU et s8/10/12 selon le temps restant. Le census et le
calendrier restent CPU ; les mesures n'acquièrent jamais automatiquement
`contract_qualified=true`. Voir le README du worker parmi les fichiers logiques.

Lecture sans cloud ni compilation :

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7-terminal-worker-fresh
```

L'extraction refuse un répertoire existant et produit `command_plan.json`,
avec les deux commandes de replay Python pur sur une arborescence autonome.
Elle ne lance aucune commande. Le tar source est lui aussi vérifié entrée par
entrée ; aucune clé ou donnée de session n'est fournie.

`logical_manifest.json` conserve les noms/hashes logiques ; `storage_map.json`
déduplique les octets sous `objects/*.source`, y compris les Markdown historiques
qui restent des sources et non des documents actifs. `manifest.json` ferme les
fichiers physiques. Aucun ELF, vendor ou résultat de vitesse n'est distribué.
