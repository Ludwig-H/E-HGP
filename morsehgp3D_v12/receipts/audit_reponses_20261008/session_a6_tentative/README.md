# A6 : fermeture de la tentative, résultats non rapatriés

Observation du 8 octobre 2026, vers 12:28 UTC. Source du paquet **30a69104a**,
archive avant déclarée **bdfca8fb1**. Lecture locale de métadonnées seulement ;
aucun appel distant, moteur, compilation ou lecture de données.

La Session lancée à **11:54:03 UTC** est arrêtée : le reçu certifie
`RUNNING → TERMINATED`, une tentative d'arrêt, code 0, garde intacte et réserve
libérée. Son état final est `failed_remote`, le worker a terminé avec **code 1**,
et `DONE` vaut **3**. Ces trois champs désignent des niveaux différents.

Le rapatriement a été refusé pour espace local insuffisant : **572 317 696 octets
libres**, pour une archive annoncée de **575 450 octets**. La garde du contrôleur
au pin 30a exige deux fois la taille compressée plus **1 Gio** (source
`gcp-migration/v12_session.py:143,1936`), soit ici **1 074 892 724 octets libres**.
Ce message ne signifie donc pas que l'archive dépasse seule l'espace libre.
Le hash annoncé à distance est
`1ee399cdb3ef9e0ed401a3cae75b68ba4dc8de5c57450e59e55ed7b0a3e5eecb`.
**Ce hash n'a pas été vérifié sur une archive locale**, absente à cette observation ;
`results_verified=false`. Le journal worker local est vide et aucune table de
commandes n'a été rapatriée.

Le plan annonce `socle_ctest` (400 s), `t2da6_pilote` (1 500 s), `lidar_ctest`
(900 s) et `mutants_tour` (600 s), après une construction plafonnée à 400 s.
**Leurs codes et durées effectifs sont inconnus ici.** Le code worker 1 ne permet
pas de choisir entre échec de construction, porte, pilote ou moteur ; le refus
local de récupération est une seconde limite, distincte. Aucun défaut
géométrique, blocage concurrent ou nouveau temps FULL n'est déduit de cet état.

Le cadre d'admission A6 reste préparatoire : catalogue GPU, cache 8 Gio dans
les deux bras ; les temps éventuels devront être relus avec la cohorte et les
identités fermées par la [proposition du juge](../a6_pilote_admission/README.md).
Cette tentative ne qualifie ni l'adoption A6, ni le contrat de 100 ms, ni CST-0242.
Une récupération ultérieure doit faire l'objet d'une preuve distincte ; elle ne
réécrit pas cette observation datée.

`capture.json` ne conserve que les champs publics sélectionnés et les empreintes
des primaires locaux, sans identité de compte, cible distante ou commande privée.
Le lecteur vérifie ces octets et la fermeture sans exécuter le contrôleur :

```sh
python check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.t2da6
python -O check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.t2da6
sha256sum -c SHA256SUMS
```

La présence éventuelle d'une archive acquise **après** l'observation n'invalide
pas le rejeu de ces primaires clos ; elle n'est pas évaluée par ce lecteur.
