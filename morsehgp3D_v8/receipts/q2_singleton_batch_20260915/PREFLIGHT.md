# Préflights des petits census entrelacés

15 septembre2026. Exploration, pas qualification de tour FULL/G4.
GCP non utilisé. Les résultats ci-dessous ne sont pas des gains présumés.

Les trois configurations neuves utilisent GCC13.3 Release et Clang18.1.3
Debug ASan/UBSan ou TSan. Les seuls en-têtes Boost1.83 sont empruntés en
lecture seule par les juges indépendants ; aucune preuve v7 héritée.

Le premier gate natif compare595 appels batch/178 Coarse et13 défauts
sur13 nuages :10 958 paires/778 276 tests de sites de l'oracle,
124 145 supports vérifiés, coquille30. Il exerce177 945 tâches compactes,
9 830 entrées après crédit,4 482 en phase différée et40 054 frères dus.
Les gates de reçus normal/−O passent :24 sondes chacune,12 comparaisons
W1/W4,24 CLI invalides et3 555 mutants sur ce passage, plus12 mutations
de pins/JSON. Ces nombres ne remplacent pas une future qualification close.

Une compilation ciblée de l'agent et le build général ont brièvement
chevauché le même build neuf ; aucun test qualifiant n'a été lancé sur
ce chevauchement. Une compilation générale ultérieure s'est terminée
avant les reprises retenues. Le premier lancement de tuning a été fait
trop tôt, avant la fin de cette recompilation. L'agent l'a arrêté par
SIGTERM ; [tuning_z6h34zxc](tuning_z6h34zxc/COMPLETION.json) garde trois
records et le statut FAILED, sans réinterprétation de ses premières lignes
favorables. Les sources du harnais étaient aussi encore en finalisation.

La reprise [tuning_r9hbnll1](tuning_r9hbnll1/COMPLETION.json) est close,
avec ses huit commandes, sources et artefacts contrôlés. À quantum1, mono
sur CPU0, les temps q2 sont défavorables au format entrelacé :

| Nuage8k/K10/s8 | Coarse | L1 | L8 | L16 |
|---|---:|---:|---:|---:|
| Uniforme |4 889 ms |7 341 ms |8 615 ms |8 453 ms |
| Terrain |924 ms |1 195 ms |1 396 ms |1 342 ms |

Tous les comptes géométriques et payloads concordent. Le format courant
fait un appel de progression et repasse par un aiguillage à chaque témoin,
avec de nouveaux compteurs de gestion. Un dernier essai de quanta plus
grands doit distinguer coût des tours de lot et coût de chaque témoin.
Ces observations sont uniques ; aucune vitesse qualifiée à ce stade.

Le premier helper hors inventaire pour les quanta a échoué avant tout
appel moteur : son manifeste en mémoire utilisait des tuples alors que
son vérificateur attendait des listes JSON. [quantums_1hawsslw](quantums_1hawsslw/COMPLETION.json)
conserve cet échec de harnais à zéro record. Le helper a été corrigé,
puis gelé avant la reprise ; ne pas transformer l'échec en PASS.

La reprise [quantums_n5elpzvo](quantums_n5elpzvo/COMPLETION.json) clôt14
mesures, quanta8/64 et L1/8/16. Elle confirme la régression : uniforme
Coarse5 032 ms, L1/Q64=6 779 ms, L16/Q64=6 788 ms ; terrain Coarse955 ms,
L16/Q64=1 102 ms. La réduction du nombre de tours de lot ne suffit pas.

Avant un seul ajustement ciblé, les sources exactes sont archivées dans
`pre_local_loop/` : census b8fa8bb0d2a9efd886fb2b4f79a7c5cf20963ae63ec47e4078788bf62c470430,
header9478f2f101460fbdb99f9d9e0254a3debe73aa2dab4ceb72dd148ea0b5072c65.
Le build Release `build/v8_singleton_batch_20260915` est épinglé à ces
mesures. La version r2 utilisera un build distinct et une boucle locale
Witness, avec les mêmes transitions et comptes géométriques, en regroupant
seulement les nouveaux compteurs de gestion. Aucun gain n'en est encore
déduit ; les résultats négatifs précédents restent l'autorité de r0.

## Suite des préflights : r2 puis clôture r3

La révision r2 (boucle locale Witness, census 6fffc6fc…, build
`build/v8_singleton_batch_r2_20260915`) a été mesurée par la reprise
[quantums_5c5xaxk6](quantums_5c5xaxk6/COMPLETION.json) : 14 mesures aux quanta
8/64, 1/8/16 voies, uniforme et terrain 8k. Le surcoût baisse (lots / Coarse
×1,07 à ×1,35 contre ×1,15 à ×1,53 en r0) sans s'annuler : zéro cas sur douze
plus rapide que Coarse. [analysis_9vfx68d1](analysis_9vfx68d1/COMPLETION.json)
clôt la lecture normal/−O de ces trois captures r0/r2 ; elle reste
historique. Aucune porte ni sanitizer n'avait tourné sur r2 : les dossiers
de build `sanitize` et `tsan_clang` du 15 septembre sont restés vides.

Le 17 septembre, après changement de constructeur, la source r3 ajoute à r2
quatre retouches sans effet géométrique : la garde « lane terminée
redistribuée » à l'entrée d'une visite, le commentaire de validité du
registre sur succès seulement, le plancher `credited_rows > 0` de la porte
de reçus et l'option `--quantum` du lanceur (les campagnes d'échelle
fixaient le quantum 1, la pire configuration mesurée). Les trois sources r2
modifiées sont archivées dans [pre_closure_r2](pre_closure_r2/), hachages
identiques aux pins de quantums_5c5xaxk6. Builds neufs r3 Release, Clang
ASan/UBSan et Clang TSan ; préflight Release 75/75 avant toute capture ;
aucune compilation ni autre campagne pendant les neuf captures closes.
Leur bilan est dans le [README](README.md) : régression confirmée dans les
54 comparaisons à n ≥ 8 000, tranche close comme résultat négatif.
