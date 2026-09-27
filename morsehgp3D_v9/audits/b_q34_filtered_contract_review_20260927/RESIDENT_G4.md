# Contrelecture du premier reçu résident G4

27 septembre 2026. Nouvelle note après la qualification portable ;
[QUALIFICATION.md](QUALIFICATION.md) et les sources closes restent inchangées.
Aucun appel GCP, compilation ou nouvelle mesure effectué par ce contre-audit.

## Reçu réel rejugé

[Reçu public r1](../../receipts/q34_resident_g4_20260927/r1/README.md),
source expérimentale `af369c44efa75236fa98e25e8f1bc4708b128fc4`.
J'ai exécuté les lectures LIVE normale et `-O` avec
[`readback.py`](../b_q34_resident_g4_readback_20260927/readback.py) :
deux sorties concordantes, code 0. Le snapshot et les originaux privés
ont été rejugés, pas seulement le résumé public.

Le reçu garde `completed`, `qualified_S2`, `FULL_executed=false`,
`contract_certified=false`, `GPU_baseline_comparison=not_acquired`.
Arrêt de la même génération `2026-09-27T01:40:23.482-07:00`, à
`2026-09-27T01:43:35.383-07:00`, soit 191,901 s d'allocation observée.
Ce n'est pas un montant facturé.

Le gate CUDA a effectivement exécuté 85 cas, 255 passages portables et
194 passages CUDA du corpus. Les 126 656 requêtes et 49 841 survivantes
sont des cumuls des deux backends, pas un compte device seul. L'appel
valide supplémentaire de la porte d'identité reste hors de ces compteurs.
Le lecteur/export a également été rejugé hors ligne : tests normal/−O,
17 positifs/21 refus, plus fixtures 5/44, sans subprocess réel dans les
selftests.

## Les temps payés, sans soustraction de démarrage

Une mesure par largeur, W4 puis W48, dans deux processus distincts.
W désigne l'arène CPU ; le front reste mono et l'oracle CPU reste W4.

| Phase, ms | W4 | W48 |
| --- | ---: | ---: |
| Adaptateur S2 complet, froid | 627,503 | 504,328 |
| Ouverture complète | 325,035 | 319,930 |
| Dont initialisation CUDA | 161,392 | 155,936 |
| Dont lancement/synchronisation des rectangles | 82,299 | 81,369 |
| Construction de l'arène | 202,425 | 83,113 |
| Consommation des paires et sortie | 97,494 | 98,509 |
| Dont vagues et petits compteurs D2H | 34,303 | 34,541 |
| Dont D2H survivantes et croissance hôte | 20,402 | 21,029 |
| Dont tri/conversion | 36,217 | 36,354 |
| Fermeture des propriétaires | 2,544 | 2,772 |
| Front mono + adaptateur | 2 738,875 | 2 616,350 |

Les lignes « dont » sont incluses dans les phases extérieures. L'adaptateur
conserve seulement sa sortie S et les R masques ; sa décision, Prepared,
Session et leurs buffers propres ont été détruits. L'amont reste possédé
par le harnais. Le contexte CUDA global n'est pas détruit. Le total du
harnais, qui paie aussi l'oracle, n'est pas le coût du candidat.

La baisse observée de l'arène avec W48 mérite d'être conservée comme
diagnostic, pas comme gain stable établi par répétitions. Ces durées ne
constituent ni un temps chaud obtenu par soustraction, ni un contrat FULL.
Le front mono du harnais n'est pas le front parallèle du moteur historique.

## Comparaison de travail avec la capture moteur historique

Les deux largeurs résidentes ont exactement les mêmes masses, digest et
visites. Les champs historiques ci-dessous proviennent de
[`g4_core_warm/probe_0.stdout`](../../receipts/g4_core_warm_20260927/vm/probe_0.stdout),
pas d'une nouvelle mesure moteur.

| Quantité | Moteur historique | Résident |
| --- | ---: | ---: |
| R rectangles | 3 133 819 | 3 133 819 |
| Masse brute avant filtre rectangle | 103 861 099 | 103 861 099 |
| Rectangles fermés | 2 005 653 | 2 005 653 |
| Visites rectangles | 229 928 699 | 229 928 699 |
| P logique | 23 686 751 | 23 686 751 |
| Requêtes ponctuelles exécutées | 23 686 751 | 9 122 704 |
| Visites ponctuelles | 1 110 657 775 | 537 798 656 |
| S final | 2 043 612 | 2 043 612 |

Les compteurs historiques de visites sont
`ledger.witness_rect_node_visits` et `ledger.witness_pair_node_visits`.
La réduction de travail ponctuel est réelle ; aucun rapport de ces comptes
ne prouve à lui seul un rapport de temps. L'égalité du travail rectangle
agrégé ne prouve pas l'égalité de l'ordre des requêtes entre fronts mono
et parallèle.

## Pourquoi les 81 ms rectangles ne sont pas une limite démontrée

Le moteur historique publiait `filter_kernel_ms=64.678`, mais
[`tower_chain.cpp`](../../src/chain/tower_chain.cpp) le définit comme
`rect_ms + scan_ms + pair_ms + select_ms`. Ce n'est pas un noyau unique.
Ses durées proviennent d'événements CUDA ; la nouvelle durée rectangle
est un chrono hôte autour du lancement, contrôle d'erreur et
`cudaDeviceSynchronize`, additionné sur douze vagues.

La comparaison de sources met en évidence plusieurs différences à isoler,
**sans leur attribuer une fraction des 81 ms** :

- ancien rectangle : entrées SoA et réduction des visites par warp avant
  une atomic ; nouveau rectangle : AoS et une atomic par thread actif ;
- ancien : une grille couvrant R ; nouveau : douze lancements Qr=262144,
  susceptibles de changer regroupement, queues de travail et localité ;
- ordre produit par des fronts de calendriers différents ;
- coût du premier lancement potentiellement présent dans le chrono hôte.
  Le chargement différé des kernels peut intervenir au premier usage,
  séparément de la création du contexte ; son mode et sa part ne sont
  pas mesurés ici. Voir le [guide NVIDIA sur le chargement différé](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/lazy-loading.html).

Les deux noyaux rectangles utilisent 128 threads ; aucun des deux ne
porte `__launch_bounds__`. Les annotations du fichier moteur concernent
notamment les noyaux certificate/lanes, pas ce rectangle.

Conclusion pratique : reprendre exactement la même population **et son
ordre**, mesurer les phases natives et candidates dans le même processus,
avec froid et répétitions réellement exécutées, avant de choisir une
optimisation. Les variantes de compteur/layout/vagues sont testables
séparément ; une campagne entière de changements simultanés ne résoudrait
pas cette question causale. Aucun gain sur le GPU natif n'est encore acquis.

Identités de cette relecture :

```text
readback.py : 67d635fc638a6f98c41735a0516028407c2b7ee973c3d6917a98fff1178471dc
SUMMARY.json : 4f09e76c214320ddcc9e0048b9b2452aacbe28e13257f166b78609be96230f44
```
