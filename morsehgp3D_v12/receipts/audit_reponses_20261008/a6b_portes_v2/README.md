# A6b v2 : portes d'attente des aides et de préfixe

8 octobre 2026. Complément distinct de [a6b_produit](../a6b_produit/README.md), sans réécriture de sa capture.
**Le commit `8b9eab40a` ne livre pas A6b** : il corrige seulement le marqueur CTest du juge T1-d. Les sources
examinées ici viennent du prototype développeur **w2**, capturé par double lecture stable à 15:42:52 UTC ;
quatorze empreintes dans `capture.json`. Aucun moteur compilé ou exécuté ; aucune invocation GPU/GCP ni lecture de coordonnées de jeux de données.

## Changement réel

`run_hint` passe de `pipeline_steps.cpp` à `pipeline_run.cpp`, dans la même unité que la fermeture du noyau,
pour observer ces deux fonctions dans la cible instrumentée. Quatre crochets supplémentaires sont vides lorsque
`MHGP12_REGION_HOOKS` est absent : début de fermeture, boucle d'attente, fin d'attente, aide ayant franchi sa garde.
Le pont acquire/release, N, les calculs d'admission et R1 restent identiques à la capture précédente.
L'en-tête de `pipeline.hpp` décrit désormais le bon ordre de priorité. Le commentaire général du noyau disant
« feuilles relâchées » et le message « toutes après G » restent à borner comme dans l'avis précédent : toutes les
tranches **réclamées**, pas tous les calculs achevés. La proposition [a6_admission_n](../a6_admission_n/README.md)
reste applicable mais non implantée ; aucune qualification mémoire supplémentaire n'est acquise par ces portes.

## Attente et garde de fermeture : témoin ciblé favorable

`region.fermeture` prépare une vraie Session K1 de 800 sites synthétiques. Le noyau K est arrêté au début de sa
fermeture. Une aide H appelle `run_hint`, annonce `active=1`, puis est arrêtée **après** avoir lu `closed=false`.
Le premier événement observé quand K reprend doit être l'attente : noyau non terminé, union-find vivant.
Le harnais relâche H, attend son retour, puis laisse K atteindre le point précédant `close_kernel`.
Un autre appel tardif doit alors rendre zéro, avant que K libère les tampons et termine la Session.

Les deux mutants proposés ont des discriminants causaux : supprimer l'attente change le premier événement en
« fin sans attente » ; supprimer la garde rend un compte tardif positif. Le harnais garde les buffers vivants
jusqu'à ces observations et joint les fils avant les assertions finales : il n'utilise pas un accident d'accès
à mémoire libérée comme oracle attendu. La forêt finale est comparée à la Session de référence.

**Portée exacte** : H et l'appel tardif invoquent directement `run_hint`, sans la réclamation CAS de `hint_slice`.
Leur tranche est déjà consommée quand K ferme ; le tardif arrive après `closed`, mais avant `reset`. La porte
exerce l'attente et la garde de durée de vie, pas une réclamation tardive réelle ni les lectures d'une feuille
pendant sa consommation par le noyau. Les sémaphores ajoutent des relations happens-before : cette porte ne tue
pas à elle seule une mutation acquire/release→relaxed. La preuve du pont mémoire reste distincte.

## Préfixe : oracle fonctionnel, pas contre-exécution native

`levers.tranches` reprend la fixture de [a6_prefixe_tranches](../a6_prefixe_tranches/README.md) : cinq naissances,
513 cellules, 517 représentants, tranches de 256/256/1 cellules. Le code déclare explicitement qu'il s'agit d'une
entrée T **structurelle**, pas d'une trame HGP réalisable. L'oracle par ensembles est indépendant du union-find.
Les 83 programmes d'indices sont exécutés séquentiellement autour des avances du noyau et comparés aux événements,
attaches et sommets de référence. Cela exerce plusieurs dates valides, sans explorer tous les entrelacements C++.

Les deux témoins fautifs injectent délibérément un indice calculé sur l'état final dans un préfixe antérieur :
une feuille modifiée laisse la clôture réussir mais rend l'historique faux ; deux feuilles modifiées entraînent
`tower_invariant`. Ils rendent l'obligation de préfixe observable. Ils ne démontrent aucune exécution du produit
corrigé qui fabriquerait cet indice futur.

## Traces locales déjà disponibles

Le journal CTest du développeur, encore **partiel** à 15:45:45, contient des blocs terminés et `Test Passed.` :

- `region_fermeture` : 15 contrôles, zéro échec, premier événement « attente », 512/512 représentants indicés,
  noyau non terminé à l'arrêt et appel tardif nul.
- `levers_tranches` : 511 contrôles, zéro échec, 83 programmes et 42 143 feuilles indicées ; témoins fautifs détectés.
- Carte de lien normale et `-O` : marqueur annonçant le corps instrumenté, membre d'archive absent, archive sans crochet.

Le cache CMake déclare Release/u21 et l'arbre w2. Ces métadonnées et les blocs lus sont épinglés ; elles ne constituent
pas une chaîne de compilation indépendante ni une vérification du binaire après toute modification ultérieure.
Au relevé : 663 tests passés dans la sélection de 754, conducteur sans code CTest final, deux nouveaux mutants
de fermeture non encore clos. Aucun succès des **752 tests ou 12 mutants v1** n'est transféré à v2 ; aucune
qualification exhaustive, GPU, FULL 100 ms ou clôture automatique du registre n'en découle.

Contrelecture mathématique indépendante favorable au témoin d'attente/garde et à ces limites. Rejeu de métadonnées
seulement, contre le snapshot externe contenant `src/`, `tests/` et `traces/gate_blocks.json` :

```sh
python -B check.py SNAPSHOT
python -B -O check.py SNAPSHOT
```

Sorties identiques à `results.json`. Aucune source entière ni trace avec chemin de session n'est recopiée ici.
