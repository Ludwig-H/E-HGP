# FULL M — protocole préparé, aucune mesure admise ici

Observation du 8 octobre 2026 à 09:19 UTC. Session `v12.20261008.fullm`, source
`957e9784fb19ccd2d6348e779ed8b7affadc1f3d` : **356 fichiers moteur/sondes/tests/CMake
du paquet identiques au commit**, plus les pilotes et lecteurs ciblés. Plan
`15a37875…e04e`, paquet `b257b211…4ac5` (9 277 035 octets) ; empreintes complètes
dans `capture.json`. Aucun résultat ni arrêt certifié à cet instant ; ce reçu
fige le cadre, sans préjuger l'issue. Aucun moteur ni appel distant exécuté par l'audit.

## Campagnes effectivement demandées

Profil u21, Release, W48. `pilote_full.py:283–329` n'est ni en `--essai` ni en
`--sequentiel` : FULL recouvert par défaut, GPU pour le catalogue des branches appareil.

| MES-FULL | Processus | Passes/processus | Chaudes attendues |
| --- | ---: | ---: | ---: |
| ng00–02, GPU K5 | 3×5 | 10 | 135 |
| ng00–02, GPU K10 | 3×3 | 5 | 36 |
| ng00–02, CPU K5 | 3×3 | 5 | 36 |
| Session des 37 trames, GPU K5 | 5 | 74 | 185 |

Soit **38 processus, 610 passes, 392 chaudes**, si toutes les prises aboutissent.
Chaque passe demande FUL1, calculé hors mur. Pour chaque trame, la médiane publiée
réunit les passes chaudes ; le maximum contractuel porte sur les médianes des
processus, le maximum brut restant diagnostic (`frame_stats:210–225`). Pour la
Session37, seul le second passage de chaque trame compte, ordre tournant par processus.
L'identité CPU/GPU concerne K5 sur les trois ng, pas une qualification CPU K10.

Le pilote apparié joue **un seul binaire** avec quatre options : `ref=` et `aa=`
(recouvert sans cache), `cache=--cache=8589934592` (cache hôte 8 Gio),
`seq=--sequentiel`. Trois ng, GPU K5/W48, 10 tours×10 passes par bras ; identité
préalable de deux passes avec FUL1, puis chronos sans digest. Les deux tours
Session37 sont informatifs, hors verdict d'adoption. Attendus : 12 processus
d'identité/24 passes, 120 processus décisifs/1 200 passes dont 1 080 chaudes,
puis 8 Sessions/592 passes dont 296 secondes visites.

La règle appariée utilise les médianes chaudes par processus, ratios appariés,
moyenne géométrique et bootstrap 10 000 (graine 20261008) : borne supérieure à 95 %
strictement sous 1 sur chaque ng ; A/A dans [0,985 ; 1,015]. Ce test compare les
options d'un même produit courant, pas une ablation historique A seule.
D6 demande séparément les trois ng, profils21/24/32, K5/W48, trois tours et cinq
passes, seulement aux dilatations admissibles : catalogue puis G, **pas FULL élargi**.

## Frontières et provenance à fermer au retour

`full_probe.cpp:271–315,408–460` : entrées déjà quantifiées en mémoire ; le mur
inclut P, catalogue CPU/GPU puis `build_tower` avec verticales et registre.
Pool, entrées et contexte appareil persistent entre passes ; catalogue et tour
sont reconstruits par passe. Validation externe, digest et libération sont
chronométrés à part. Segmentation et chargement ne sont pas le mur FULL.
Les fenêtres cumulées T/M/V/R recouvertes ne s'additionnent pas au mur ; G désigne
une échéance avec forêt concurrente, puis TMVR la queue après G.

Le cache réutilise des blocs d'allocation, pas des résultats géométriques.
`pic_octets` compte les blocs vivants et réservations du budget partagé, sans
les blocs inactifs retenus par le cache. Il ne représente donc pas à lui seul
son coût physique total. Relire séparément les capacités appareil/épinglées et
`rss_max_octets` des JSONL (maximum cumulatif du processus, hors mémoire GPU),
que le résumé apparié ne conserve pas ; aucune économie mémoire n'est acquise.

Les constructions MES-FULL (`full/b21cuda`) et appariée (`apparie`) sont
distinctes, comme le répertoire D6. Les commandes s'exécutent successivement ;
aucune reconstruction d'un même chemin ELF entre ces deux pilotes n'est prévue.
Dans l'appariement, tous les bras utilisent le même chemin sans reconstruction
(`pilote_apparie.py:425–459,482–496`). Cela ne prouve pas encore l'identité des ELF
avant/après campagne ni entre les deux constructions : leurs hashes et caches
devront être relus. Le plan vise les mêmes entrées ng et la même archive37,
contrôlé par égalité des références sans lecture de leurs contenus.

Les cinq commandes ont pour limites 900/2400/1500/600/1800 s (socle, FULL,
apparié, mutants tour, D6), contre une limite globale annoncée de 4 200 s et
un garde invité de 55 minutes. Ce ne sont pas cinq réservations indépendantes ;
l'exécution tardive et la complétude doivent être constatées, pas présumées.
MES-M0 LiDAR n'est **pas** une commande de ce plan ; son précédent succès B reste
une preuve distincte, aucun nouveau résultat M0 annoncé ici.

Le source957 conserve l'ancien retrait de compteur de [CST-0241](../a_terminaison/README.md).
Le correctif aperçu ensuite dans le worktree ne fait pas partie du paquet.
Il existe un scénario abstrait de non-terminaison, sans blocage observé déduit
pour cette session. Les propositions d'[admission FULL](../mes_full_admission/README.md)
et de [composition appariée](../apparie_livraison/README.md), publiées après le pin
de campagne, ne sont pas attribuées à ses lecteurs. Une relecture indépendante
des bruts pourra les appliquer hors produit, sans changer les seuils préannoncés.

## Rejeu de cette préparation

`python3 check.py --repo DEPOT --session DOSSIER_FULLM`, puis la même commande
avec `python3 -O`. Le lecteur réutilise le [contrôle de sources](../session_mes_c_provenance/source_check.py)
épinglé, compare les options et les sept fichiers ciblés au paquet et au Git.
Il ne lit aucun payload de scène, ne lance aucun pilote et ne juge aucun temps.
