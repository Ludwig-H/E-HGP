# N1 : lecture du reçu A/B clos du 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Aucun build, test natif, accès cloud,
réseau ou calcul HGP lancé par l’auditeur ; aucun octet LiDAR ou journal brut
copié dans cette capsule.

Session `v11.20261006.claudeN1`, arrêt ciblé certifié le 6 octobre à
06:32:58 UTC. Source N1 `c72c5a5760cdd70a1246de8ce1768c564a420a85`,
base `8ee28873f2b9e59166b73a36e7b2302fa2a3a7a4`.
Archive de résultats :
`ac1b6b51dd6b98eec452fa6ef6014191f885dd07fa0a17fabd96867084945f02`.
Le paquet source et l’archive de base correspondent exactement à leurs
635 et 634 fichiers Git utiles. Les pièces publiées au pin `cf28afb04`
concordent avec la session locale ; leurs empreintes sont valides.

Le reçu reste **failed_remote**, et le banc **refus** : le plan demandait
150 tests alors que sa sélection en contient 145. Les 145 verdicts archivés
sont PASS, sans échec ; les deux mutants d’arène sont TUE par code, selon
leurs rapports individuels. Ce refus d’admission du banc reste distinct du
résultat de mesure et du rejet de l’optimisation.

Les **78 prises** ont le statut natif `ok`, le dump canonique attendu et
un registre de catalogue égal : trois trames entières ng00/ng01/ng02,
K5, u21, FULL/16379, douze paires à W48 et une paire à W1 par trame.
Chaque commande mesure une passe dans un processus neuf ; aucun régime
résident, GPU, TSan ou ASan n’est qualifié par ce plan. Les **39 prises N1
ont toutes walk_fallbacks=0**, directement lu dans leurs lignes natives.

Pour atteindre le critère prévu, il fallait notamment dix différences
appariées de `domain` négatives sur douze. Les prises en donnent
**4, 5 et 4**, donc cette condition échoue sur les trois trames. Le rejet
de N1 et son retrait en `c1675e4c9` sont une décision étayée. Cette capsule
ne présente pas l’explication SMT comme une causalité exclusive mesurée :
le reçu ne contient pas les chronos CPU par tâche et par feuille proposés
dans l’audit du plan.

`summary.json` conserve seulement les faits, sources et empreintes.
Rejeu stdlib normal et optimisé, depuis un environnement disposant des
objets Git et des archives locales :

```sh
python3 -B replay.py
python3 -B -O replay.py
```

Les deux sorties sont identiques. Les chemins sont adaptables avec
`--repo`, `--session` et `--baseline`. Le lecteur dépend des résultats et
du paquet sous `.ehgp-sessions`, ainsi que de
`build/v11-persist/n1_ab/data/base_src.tar.gz` ; cette capsule est une trace
d’audit compacte, pas une archive autonome ni un nouveau reçu natif.

La session L4 ouverte lors de cette lecture est hors de cette capsule.
