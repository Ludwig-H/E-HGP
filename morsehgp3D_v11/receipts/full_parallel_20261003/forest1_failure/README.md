# forest1 — premier échec conservé

Source exécutée : `e5f6a5683237005117a6b9f2c23bdc7eda6e3caa`.
Campagne **non conforme** : matrice **2 660/2 667** portes,
supplément ASan18 **199/200**. La compilation réussit dans chaque
configuration présente. Le benchmark FULL refuse la qualification avant
toute mesure : **zéro essai, zéro chrono FULL parallèle**.

Le reçu brut LIVE obligatoire est
`/workspaces/.ehgp-sessions/v11.20261003.forest1/receipt.json`.
Clôture `stopped`, `DONE=3`, `failed_remote`, worker1 ; génération
`2026-10-02T18:16:50.814-07:00`. Arrêt ciblé, résultats vérifiés, retrait
OS Login, suppression de la clé privée et libération du verrou sont tous
certifiés. Erreurs et avertissements du contrôleur sont vides ; cela ne
rend pas conforme le résultat des tests.

Archive worker originale unique : **573 406 octets**, SHA256
`6acd9221e7e21d3e774d7aadac6b552203865a1e5c65afc6688be13c5f734177`.
Les journaux complets, JUnit, inventaires et preuves de compilation y
restent intacts. Aucun paquet source, binaire exécutable ou payload KITTI
supplémentaire. `source_contract.json` fixe les sources Git réellement
exécutées, dont les lignes du test, le produit, le harnais et le plan.
Le lecteur dépend aussi de ces objets Git ; il n'importe aucun banc WIP.

## Cause précise

`tests/tower/forest_parallel_test.cpp:74` attend
`times.orders[1].extended_cells == 1u`, et reçoit0. Cette même assertion
échoue pour les trois capacités Q1/Q2/Q4 :49 contrôles,3 échecs,
plancher45. C'est un **faux attendu du test** sur la ligne de sites0,2,4.
À K2, la boule extrême a centre2, niveau4, intérieur `{2}`, coque `{0,4}` :
**m=qmin=2**, donc une cellule régulière et zéro cellule étendue.
Le nombre de points total3 ne doit pas être pris pour la taille de coque.
La forêt K2 attendue garde deux naissances au niveau1 puis une fusion au
niveau4. Ce diagnostic ne démontre pas un défaut géométrique du produit.

| Configuration | Portes passées / sélectionnées | Échec |
|---|---:|---|
| Release | 502 / 503 | même test plateau |
| Mutants | 20 / 21 | témoin tower rouge |
| ASan/UBSan24 | 427 / 428 | même test plateau |
| TSan21 | 427 / 428 | même test plateau |
| u21 | 427 / 428 | même test plateau |
| u24 | 427 / 428 | même test plateau |
| Poison21 | 428 / 429 | même test plateau |
| Style | 2 / 2 | aucun |
| ASan18 num/index/tower | 199 / 200 | même test plateau |

Clang est absent, optionnel. Les sept lancements du test natif plateau
(six dans la matrice, un dans le supplément) ont réellement tourné :
ce ne sont ni des erreurs de compilation ni des lancements impossibles.
Leur code1 est reçu par CTest comme échec8.

`mhgp11_mutants_tower` annonce que son témoin sans mutation échoue sur
cette porte, puis **aucun mutant tower jugé**. Les six autres campagnes
de mutations sont closes : les logs complets recoupés avec JUnit attestent
174 morts par code/ligne et2 refus de construction attendus. Aucun total
de réussite des mutants tower n'est transféré depuis une ancienne capture.

L'injection mémoire `forest_parallel_fault_starvation` passe : les JUnit
de la matrice enregistrent55 positions injectées, refusées et restaurées
pour W1 et W4 ;2–3 injections W4 ont eu lieu hors pilote selon la
configuration. C'est une observation de cette capture : la porte imprime
`off_pilot` mais n'en exige pas la positivité à chaque rejeu.

Les trois commandes worker sortent1/1/2. La dernière conserve uniquement
`full_parallel_refused: ValueError`, aucun rapport de benchmark, aucune
tentative ni réemploi sémantique. Les autres portes passées constituent
des résultats partiels conservés, pas une qualification complète de ce lot.
Ni gain de performance, ni contrat200ms, GPU ou clustering n'en découle.

## Lecture

Depuis ce dossier :

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B check_selftest.py
python3 -B -O check_selftest.py
```

Code0 signifie **preuves cohérentes d'un échec** ; la sortie garde
`conforming=false`. Le lecteur vérifie le brut, les hashes, le manifeste
tar sans extraction, les copies compactes, les JUnit/inventaires, les
options et empreintes de compilation, la cause exacte et le refus du banc.
Les contrôles purs comptent6 témoins et47 corruptions refusées, sans
appel natif, build ou GCP. Leurs sorties et empreintes sont fixées dans
`check_selftest.json`. Le reçu original n'est jamais réécrit par ce lecteur.
