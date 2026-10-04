# Audit transversal de la v11 depuis ses fondations

4 octobre 2026. Produit figé **0f5e8a207** ; complément **0af635a71**
publié pendant l'audit : démos et reçu nouveaux, mêmes src/bench/tools/tests.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Quatre contrelectures coordonnées, documentation actuelle et pièges antérieurs.
Les [deux notes courantes](../../audits/README.md) portent les conclusions utiles.

**Constats nouveaux à traiter :** réveil du pipeline sur abandon permettant
une lecture de l'ordre incomplet ; export POINTS192 refusant un tétraèdre
u24 valide. Les corrections proposées et leurs témoins sont dans les
capsules ci-dessous. Aucun faux succès FULL ni race native reproduite
n'est revendiqué. Aucun nouveau défaut mathématique FULL établi.

**Aide nouvelle au développeur :** B=rencontre(H,core), stable3ε sous les
hypothèses explicites de H3 ; un LCA par point remplace le balayage des
coupes du prototype. Cette borne ne prouve pas une meilleure pertinence
statistique ni la stabilité de la partition EOM à égalité.

| Capsule | Couverture et preuves | Contrôles nouveaux normal/−O |
|---|---|---:|
| [Fondations](foundations/README.md) | core/cloud/sched, propriétaires, budgets/IDs, CMake/IO ; 373 portes natives existantes recoupées, sources identiques c40 | 431 intégrité/métadonnées |
| [Géométrie et catalogue](geometry/README.md) | num/index/catalogue, trois profils, coquilles/S*, G1/G3/J2, caches/frontier/assemblage ; tétraèdre u24 | 340 Fraction |
| [Mathématiques](mathematics/README.md) | MEB, Γ/Lk, morceaux stricts, FULL/verticales, couverture complète, frontières/points, B et tête/statistique | 12 968, dont32ordres/1 082coupes/156datesB |
| [Tower et bancs](tower_evidence/README.md) | 32 fichiers tower, pipeline/refus, export et capacités, source-parité native b872, récent lot G4/Zoltan | 72 contrôle d'abandon +37matrix/87host mocks +2 459 lecteur G4 |
| [Protocole G4](g4_protocol/README.md) | verrou, identité/génération, budgets et arrêts, fermeture des preuves ; 13 clauses | 87 intégrité/source |

**16 481 contrôles** par mode, de natures différentes : ce total additionne
calculs exacts, modèles de contrôle, mocks et intégrité, pas des milliers de
nuages natifs indépendants. Toutes les sorties normal/−O concordent.
Les **101 fichiers natifs** implémentés sont relus par module ; tests/bancs
sont inventoriés et leurs chemins difficiles ciblés, sans prétendre relire
chaque ancien harness. [Matrice globale et limites](tower_evidence/SCOPE.md).

La qualification native et les chronos antérieurs sont recoupés, **aucun
build/test natif, fit, workflow ou appel GCP nouveau par cet audit**.
Les 360 bouts du lot récent ont 107–17 593sites, dix séquences et789
observations d'objets corrélées : meilleurs blocs, sélection par annotations,
pas tête plate ni trame entière. Leur arrêt G4 et427payloads sont vérifiés.
Les dernières médianes FULL K5/u21/W48 restent412/352/381ms sur trois
trames sans sol séquence08. 100ms, GPU, massif et tête native restent ouverts.

Chaque capsule garde ses sources, hypothèses, commandes, échecs de lecteur
et ledger. Les SHA imbriqués sont **eux-mêmes** inclus dans le SHA parent ;
aucun reçu clos antérieur ni fichier d'un autre acteur n'est réécrit.
Pas d'entrée brute/coordonnée/étiquette KITTI copiée.

```sh
python3 -B check.py
python3 -B -O check.py
```

Le lecteur vérifie l'inventaire exhaustif et toutes les empreintes ; il
ne réexécute ni le moteur ni les campagnes natives. Les lecteurs exacts
et mocks se rejouent avec les commandes des README/ledgers des capsules.
