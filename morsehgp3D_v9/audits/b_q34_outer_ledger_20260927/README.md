# q34 hors S2 : ce que les chronos mesurent vraiment

27 septembre 2026, base `a7e80d7f9`, grille 1 mm/u18, `not_claimed`.
Audit de sources et relecture de deux premiers passages historiques G4,
pas de nouveau benchmark ni appel GCP. Une trame sans sol 08/000000,
39 885 sites, K5/s8. Ni plusieurs scènes ni une mesure des passages chauds.

## Décision

La couture rectangles GPU → Pool → vagues reste une expérience utile,
mais **S2 n'est qu'environ 101 ms sur les 487–491 ms de q34** dans cette
capture. Même un S2 gratuit ne ferme pas le contrat FULL 100 ms. Il faut
aussi agir sur les certificats S3, les supports S4 et les scans hôte.
Les derniers prototypes de vagues ne remplacent pas encore ces étapes.

Le lecteur [analyze.py](analyze.py) rejoue l'autorité historique du
[chemin critique](../b_critical_path_20260927/README.md), avec ses sources
et son archive épinglées, avant de calculer les intervalles suivants.

| intervalle disjoint, ms | passage 0 | passage 3 |
| --- | ---: | ---: |
| front et collecte des rectangles | 98,959 | 101,482 |
| S2, appel complet du filtre | 101,017 | 100,707 |
| S3, appel des certificats | 118,615 | 118,711 |
| travail arêtes CPU | 1,554 | 1,552 |
| attente restante de S4 | 79,643 | 79,912 |
| fin différée et livraison des supports | 4,331 | 4,201 |
| **somme de ces intervalles** | **404,119** | **406,565** |
| **reste non attribué dans q34** | **83,145** | **84,296** |
| **q34 complet** | **487,264** | **490,861** |

S4 dure environ 81 ms mais recouvre le travail arêtes CPU : ajouter son
temps entier à l'attente serait un double comptage. La préparation GPU,
q2 et son census anticipé se recouvrent aussi avec cette chaîne. Les
sous-temps kernels/transferts ne sont pas des lignes supplémentaires.
S3 ne coûte pas seulement son kernel d'environ 90 ms.

**Les 83–84 ms ne sont pas attribuées à la validation par cette capture.**
Ni une initialisation CUDA ni une destruction ne peuvent être déduites
du seul écart. Les étapes ci-dessous expliquent où instrumenter ; elles
ne sont pas des gains mesurés ni des postes à supprimer aveuglément.

## Lecture des frontières de chronométrage

Source : [wspd_q34.cpp](../../src/gen/pipeline/wspd_q34.cpp),
`run_wspd_q34_batched`, et ses contrôles structurels `check_certificate_batch` /
`check_lanes_batch`. L'appel est enveloppé par
[tower_chain.cpp](../../src/chain/tower_chain.cpp).

- Après le front : lancement du travail q2 via callback, hors sous-timers.
- Après S2 : contrôle des R masques et masses, parcours ordonné des S
  survivantes, recomposition des compteurs, libération des rectangles.
- Après S3 : contrôle des décisions, compteurs et préparation des demandes
  S4. Le chrono certificat s'arrête avant ces contrôles.
- Après la jointure S4 : contrôle des tranches de supports et de leurs
  payloads, collecte des arêtes différées et réduction des compteurs.
- Après le tail : fusion des états, `validate_completion`, affectation des
  statistiques et destructions des objets encore vivants.

Ici R=3 133 819, S=2 043 612, 849 780 enregistrements S4. Ce sont de vrais
volumes mémoire, même sans arête différée. Le contrôle des certificats est
O(S). Celui des supports parcourt S et les enregistrements, avec des
contrôles d'IDs et d'intérieurs bornés par K ; il ne recalcule pas toute la
géométrie. Le contrôle S2 est O(R+S), pas une nouvelle énumération P.

Ces contrôles vérifient forme, domaines et bilans, pas la complétude d'une
décision géométrique arbitraire. Une suppression fautive peut respecter
les bilans : conserver les oracles différentiels et une couture possédée
entre producteurs qualifiés. Le [contrat du raccord filtré](../b_q34_filtered_contract_review_20260927/README.md)
précise cette frontière pour S2.
Ils ne sont pas les appels `judge_*` différentiels qui recalculent la
géométrie ; ces derniers ne sont pas actifs dans les deux passages lus.

## Prochaine instrumentation et transformations testables

1. Mesurer séparément chacun des scans/contrôles ci-dessus, les allocations
   et les destructions ; conserver un reste explicite. Les chronos doivent
   couper des intervalles disjoints, pas additionner les fils concurrents.
2. Pour chaque scan, séparer les tests indépendants par élément des
   réductions de masse/préfixes. Une réduction parallèle peut conserver le
   premier ordinal fautif ; elle doit payer ses temporaires et ses joins.
   L'ordre stable et les limites de chaque tranche restent contrôlés.
3. Garder les décisions S3 et supports S4 dans des objets possédés liés au
   même nuage/K. Étudier la résidence device et la fusion des scans une
   fois ce contrat établi, sans ressaisir des masques « fiables » externes.
4. S4 expose 2 009 427 tâches et une tâche maximale de 5 810 étapes dans
   ces deux passages. Il reste à mesurer la distribution et les rounds,
   pas seulement le maximum ; aucune conclusion de déséquilibre dominant
   ne découle de ces deux compteurs seuls.

Le front, la géométrie des certificats, le census et la construction FULL
restent des chantiers distincts. Aucun de ces constats ne prouve une borne
sous-quadratique globale ou le respect de 100 ms. Le but est de consacrer
le travail aux coûts complets, sans confondre un kernel rapide avec une
tour explicite terminée.

## Reproduction

```sh
python3 -B morsehgp3D_v9/audits/b_q34_outer_ledger_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
python3 -B -O morsehgp3D_v9/audits/b_q34_outer_ledger_20260927/analyze.py /workspaces/E-HGP/build/v9-g4-core-snapshot-20260927/snapshot.tar.gz
```

Le lecteur exige l'archive privée de la capture close ; il n'est pas une
preuve autonome reconstruite à partir du seul Markdown.
Les deux lectures ont terminé avec code 0 et le même résultat `passed`.
