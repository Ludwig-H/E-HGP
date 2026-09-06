# Preuves du filtre MEB privé — 6 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Aucun moteur FULL intégré, gain de tour ou
résultat G4 revendiqué. GCP non utilisé.

## Verdict et autorité

La campagne R2 ferme 41 commandes : quatre binaires nominaux par build
O2 et ASan/UBSan, sans macro de test, trois copies mutantes compilées,
portes nominales, rejets d'arguments et mutants causaux. Les rejugements
Python normal et `-O` consomment les mêmes sorties, sans nouveau moteur.
Leurs stdout sont identiques : SHA
`d178cf376b5888c10f808bda7e8043dbf3d7ffb9176d91fdc725a5928e095888`.

Par build : 9 344 appels F/Trace/NoObserver, 59 frontières ciblées,
3 430 appels contrôlés par l'oracle rationnel et 1 507 ordinaux.
La porte de trajectoire sépare 180 appels natifs et leurs rejeux, puis
8 appels locaux/6 natifs/1 rejeu pour le complément admissible d'ordre.
Voir les [résultats et limites](../../docs/RESULTATS_MEB_FILTREE_20260906.md).

Le helper filtré `484a89bc` demeure inchangé ; il est repris explicitement
de la préparation du 5 septembre. Ses commentaires et README initiaux
décrivent cette préparation, pas le verdict présent. Le repli F et les
formes exactes sont épinglés dans le même snapshot. Les modules rationnels
de l'auditeur sont des copies inertes : leurs anciens runners ne sont pas
invoqués et aucun fichier de son dossier vivant n'est modifié.

R1 a passé ses propres portes et reste conservé dans le build privé.
R2 le complète après la correction mathématique de l'auditeur : base
positive unique dans un pivot strict ; l'ordre change les essais et P,
pas le support à budget non limitant. Seul ce dernier lot complet est
publié ici, sans dupliquer les captures positives intermédiaires.

## Organisation et pins

- `runs/v7_meb_filter_qualification_20260906_r2/` : sources originales et
  copies mutantes, stdin/stdout/stderr, intentions et terminaux, dépendances
  effectives et hashes des binaires non distribués.
- `checks/checks_r2/` : recorder, contrôleur consommé, deux commandes
  Python, bruts et reçu de clôture.
- `protocol/publish.py` : copies contrôlées, sans moteur ni rejugement.
- `publication.json` : provenance et exclusions explicites ; copier une
  capture ne lui confère aucune nouvelle qualification.

Run : `981f3b3e67f3f8e731aceca964c9faaae32b16b372f58704b205585374f83e87`.
Carte des 63 sources :
`7e881f998a2f6bcac8e709a3ee3fa0c176973a7670e2a22960a1d46641627172`.
Contrôleur : `0f5f0f6c9cb5b86117cb92334d764e16baee060089f177e1e688a42eae6a874c`.
Reçu des deux rejugements :
`44d590bb318d7e21cca5fe4055422a2661d18e77694001f044080b49fc5b97ac`.

Les commandes enregistrées gardent leurs chemins historiques. Pour rejuger
les copies depuis la racine du dépôt, sans compiler ni lancer de C++ :

```bash
python3 -B morsehgp3D_v7/receipts/meb_filtered_20260906/runs/v7_meb_filter_qualification_20260906_r2/snapshot/build/v7_meb_filter_qualification_20260906/capture.py --judge morsehgp3D_v7/receipts/meb_filtered_20260906/runs/v7_meb_filter_qualification_20260906_r2 --source-sha 7e881f998a2f6bcac8e709a3ee3fa0c176973a7670e2a22960a1d46641627172 --run-sha 981f3b3e67f3f8e731aceca964c9faaae32b16b372f58704b205585374f83e87
```

Ajouter `-O` au Python donne le second mode. Pour reconstruire les gates,
reprendre les flags de `commands/*_compile.json`, en pointant source et
include vers le snapshot publié et sortie/dépendances vers un build neuf.
Les includes relatifs sont conservés. Ces reconstructions seraient des
exécutions nouvelles, pas celles des présents reçus.

Les binaires `bin/` et éventuels caches Python sont les seules exclusions
autorisées par le publisher. Aucun ELF n'est distribué. `SHA256SUMS`
couvre toutes les copies et métadonnées, avec chemins relatifs au paquet.
Tous les moteurs C++ sont clos à 08:14:41 UTC ; les durées capturées
servent à borner les commandes, pas à mesurer une accélération MEB.
