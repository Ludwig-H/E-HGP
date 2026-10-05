# Delta mathématique S9 locale 451301787 — 5 octobre 2026

Aucun nouveau défaut mathématique important trouvé dans les changements examinés, sur le chemin de comparaisons acceptées. Le défaut du tri en cas de refus reste présent dans ce commit local ; sa correction WIP et sa qualification sont suivies dans la capsule de l'auditeur natif. Ce rapport ne clôt pas ce défaut ni S9 globalement.

Pin local `451301787c7e02b8f5e1a4c08064275b6af6de1e`, sur `53c027fe8`, lu dans `build/v11-impl-l3` ; distinct de main et des quatre fichiers de correction WIP. Cadre `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. 135 fichiers inventoriés et hachés depuis Git ; cette capture n'est pas une revue ligne par ligne des 135 fichiers. Rapports `impl_s9.md` et `verif_s9.md` copiés stables et hachés. Quatre fichiers WIP de correction sont inventoriés avec SHA avant/après identiques ; ils ne sont pas l'entrée du rejeu mathématique.

## Changements réellement examinés

Comparaison avec la capture précédente `v11-audit-math-s9-20261005` (reprise dans le reçu publié à l'époque de `d448b3d03`) : incidences, qualification, recherche du plancher et settlement, ancestor_index.cpp et construction/balayage de l'arbre de points sont identiques. Les changements de hang.cpp et ancestor_index.hpp sont des commentaires. Les deux nouveautés exécutables de `src/points/exact.cpp` renvoient l'égalité lorsque deux paires de rangs coïncident à permutation près, ou lorsque les deux triplets de date sont identiques. Ce sont des égalités exactes sur le domaine de rangs valides ; elles économisent l'arithmétique sans modifier un ordre ni une date. Le modèle S9 inchangé n'est pas rejoué.

Les nouvelles portes fixent treize faits (dont K1 sans override m), cinq mutations causales (qualification, marge carrée, suppression de marge, coupe ouverte, m(1)=2), et un plancher de dix décisions natives prises par le repli exact. La ligne gravée de l'oracle en déclare douze. La routine indépendante de définition garde ses coupes fermées, son comparateur propre de deux racines, ses signatures de propriétaires et ses partitions exactes ; son changement ajoute essentiellement le compteur de repli et le mode sans `--m` pour K1. Les constats natifs des rapports ne sont pas réexécutés ici.

## Deux seuls témoins supplémentaires

`check_witnesses.py` interroge directement `Definition` et `reference_radius`, sans moteur ni relecture du modèle précédent :

- F2, deux triangles, K2/m1 : les groupes exacts à sqrt(1) sont `{0,1} | {4,5}`.
- F6, quatre sites, K2/m1 : la date du site 0 est `(2,18,8)`, soit `sqrt(2)+sqrt(18)−sqrt(8)=sqrt(8)` ; le propriétaire de la coupe fermée naît exactement au niveau 8 et couvre `{0,1,3}`. Son parent est strictement après cette date. Cet attendu est sensible à une coupe ouverte lors de l'égalité.

Ces deux attendus nouvellement gravés concordent avec l'oracle exact. Ce contrôle ne prétend pas prouver que le C++ les produit : cette comparaison appartient aux portes natives. Il ne rejoue ni les 7 000 comparaisons arithmétiques, ni les 1 086 ordres, ni les anciens six témoins de la traduction S9.

Quatre invocations closes au code 0, sans essai en échec : normal, `-O`, replay Git normal, replay Git `-O`. Sorties identiques octet pour octet, SHA256 `62d1f881ce47530c83978f70f094ca20b77ce0561034dc778f94ea5bff96dc27`. Les commandes, stderr et empreintes des scripts sont dans `executions.json`. La source initiale du harnais est conservée, identique à la finale.

## Reprise compacte

```sh
python3 replay.py /workspaces/E-HGP
python3 -O replay.py /workspaces/E-HGP
```

Le replay vérifie les sources par `git show` du pin et SHA256 puis exécute les deux témoins dans un dossier temporaire. Ne pas recopier `snapshot/` ni `wip_fix/` : reprendre seulement ce rapport, `source_manifest.json`, `check_witnesses.py`, `replay.py`, les quatre JSON et quatre stderr, `executions.json`, `SHA256.json`, `developer_reports/` et `attempts/initial_check_witnesses.py`. Aucune source produit modifiée ; aucun build, exécutable natif ou GCP utilisé.
