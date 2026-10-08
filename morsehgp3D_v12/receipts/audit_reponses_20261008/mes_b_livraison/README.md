# MES-B livré : corrections acquises et limite du mur nul

8 octobre 2026. Produit épinglé à `8da450ab751017a77928cb5c33e284b0fd0371e8`.
Complément de la [prélecture immuable](../mes_b_prelecture/README.md), sans modification de celle-ci.
Les six sources sont hachées dans `capture.json`. Le pilote final `84777f19…` est identique
à la contrelecture préliminaire conservée avant la coupure (`root_mes_b_live_review.json`, SHA dans la capture).

Les deux portes officielles Python, normale et `-O`, rendent code 0 et la même sortie :
23 contrôles de lecture, 7 d'issues, 8 de verdicts, 3 d'empreintes, 12 étiquettes et 2 de pilotage,
selon leurs compteurs affichés. Le runner Python tue ses **20 mutants** (code 0).
Lecture préalable effectuée : seules des sondes Python fictives sont lancées ; le relevé
d'environnement appelle `cmake --version`, jamais une construction. Aucun moteur, GPU, GCP
ni donnée de scène réelle n'a été utilisé par cet audit.

## Corrections et règle déclarée

- Les quatre corruptions de la prélecture sont refusées : clé répétée, booléen comme rang de
  libération, ouverture `ok/device_fault`, entier hors u64. Les étiquettes longues répétées terminent
  et restent uniques. Le schéma contrôle aussi les métadonnées, les clés des blocs, leurs types,
  les phases et inclusions des durées ; ceci ne prouve pas une admission exhaustive de toute corruption.
- Tout refus K10 lancé rend B4 non tenu. Pour K5, le code et la documentation déclarent désormais
  explicitement une exception : refus **à partir de 10 millions de sites** toléré par B1, avec mention
  dans le détail. Sous ce seuil, le refus échoue ; un échec échoue à toute taille. Le témoin K5/12M
  de la prélecture n'est donc plus un refus oublié sous la règle livrée. Ce choix écrit avant campagne
  ne constitue pas une réussite de calcul sur la scène refusée.
- `full_probe.cpp` publie `null` si CPU/RSS est indisponible et protège la soustraction CPU si le
  compteur recule. Le lecteur classe ce `null` comme contrôle manquant (`illisible`), sans inventer
  zéro ni sous-flux u64. RSS reste le maximum depuis le lancement du processus, entrées et passes
  antérieures comprises ; CPU encadre `run_wall`, avec les coûts des appels de mesure aux frontières.

## Résidu reproduit, sans défaut de mesure réelle déduit

Un JSON avec mur nul et blocs de durées nuls passe `parse_output` comme `ok`. Placé comme dernière
passe d'un membre d'une série B3 complète, il conduit à `log(0)` dans `slope` : `ValueError` remonte
avant l'écriture du rapport. Le témoin emploie uniquement deux cas inventés. Aucun mur nul natif
n'a été observé. Correction minimale proposée : exiger `wall_ns > 0` à l'admission avant le calcul
des statistiques, et graver ce contre-exemple. Aucun patch produit ni nouveau constat créé ici.

```sh
python3 -B check.py --repo /chemin/depot_epingle
python3 -B -O check.py --repo /chemin/depot_epingle
python3 -B check.py --repo /chemin/depot_epingle --run-gates
```

Le lecteur vérifie les SHA du reçu, compare les six sources au pin Git, rejoue les JSON ciblés,
puis revérifie les sources. `--run-gates` rejoue les trois commandes Python ci-dessus ; il ne
lance aucune sonde native. Ce reçu ne qualifie ni une campagne MES-B réelle, ni les nouvelles
optimisations T2-d, ni l'identité des scènes sans empreinte au-delà du seuil déclaré du pilote.
