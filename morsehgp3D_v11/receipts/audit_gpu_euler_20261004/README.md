# Relecture GPU/Euler — 4 octobre 2026

**Points à traiter : plafond des feuilles du lot et non-vacuité du banc.**
Les contrôles relus du juge Euler/J1, des prédicats et du repli entier sont
favorables. La réservation des payloads CUDA est corrigée dans la source
publiée. Aucun défaut de sortie FULL native n'est démontré.

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Source publiée figée :
`77db5738eb2dd5bc84ecdc4d85ade833124c58f8` (tranches Euler 462dca187,
GPU 82fff7543 et rapports Nsight 77db5738e). Actualisation des bancs et
diagnostics au pin **61da03749344a6acc4fea2b9eee875606cc857e8** ; géométrie
et transport GPU inchangés. Les premières contrelectures
partaient d'un WIP capturé à 14:50:18 UTC, base 66372e621 ; les rapprochements
avec le commit publié sont explicites. Les capsules restent immuables. L'observation initiale sur le budget est
conservée dans [batch_transport](batch_transport/README.md), **supersédée**
pour la mémoire par [batch_transport_live](batch_transport_live/README.md).
Pas de compilation, test natif, CUDA, profiler, fit, téléchargement ni GCP.

| Contrat relu | Résultat et preuve portable |
|---|---|
| Euler à K+2, supports et restriction J1 | [3 623 gardes exactes](euler/README.md) ; K+1 insuffisant dans un tétraèdre + centre |
| Géométrie des feuilles, contacts et repli | [145 391 gardes Fraction/Gram](device_geometry/README.md) ; pas de décision géométrique incorrecte établie |
| Rapprochement des prédicats au publié | [23 gardes](device_published_bindings/README.md) ; 11/12 fichiers identiques, le douzième ajoute seulement des bornes de compteurs |
| Réservations hôte/device et plafond des feuilles | [Revue des sources et modèle du parcours](batch_transport_live/README.md) ; réservation corrigée, plafond count≤n à remplacer |
| Second modèle du nombre de feuilles | [Distances exactes aux coins](centre_leaf_counterexample/README.md), indépendant de l'expression G1 du premier modèle |
| Banc froid/chaud | [73 gardes AST](gpu_bench_current/README.md), processus simulés ; « conforme » possible sans un régime mesuré |
| Réponse sur ordre/contexte/checkpoints | [237 gardes](mesure_protocol_live/README.md), trois réserves corrigées en source |
| Réponse sur lecteur pipeline et durées des tâches | [81 gardes AST/scalaires](lecteurs_published/README.md), ancien rejet e49 corrigé en source |
| Captures CPU et qualification publiée | [354 contrôles de métadonnées](mesures_bindings/README.md), lecteur développeur 176 rejoué ; 126 A/B avec identité, 48 diagnostics sans hash |

Le cube `{0,16}³` plus son centre `(8,8,8)`, K5/leaf_size8/max_leaf32,
produit **159 nœuds, 80 feuilles pour neuf sites, 648 entrées de listes**.
Les deux exécuteurs refusent ce lot sous count≤n, comme leur documentation
l'annonce. Ce plafond ne découle pas de la géométrie : les régions des
centres sont distinctes, leurs listes de sites se recouvrent. Borner les
sommes à partir du nombre réel de feuilles et vérifier séparément les
domaines d'allocation/backend conserve la sécurité sans ce refus inutile.
C'est une restriction source démontrée par modèles, aucune exécution native.

Le banc publié contrôle correctement dump et ledger **finaux**, et alterne
les deux modes à froid. Pour qualifier les deux régimes, il doit aussi
exiger des prises froides et des passes chaudes non vides, exactement 1..P,
avec succès. L'identité de la dernière passe ne certifie pas les précédentes
chronométrées ; conserver cette portée pour un diagnostic ou comparer chaque
sortie hors FULL pour qualifier chaque passe. Count/fill et les relations
J2 recomputées sur hit demandent une mesure physique séparée du ledger.

Euler/J1 ne certifient pas la complétude du catalogue : compensations,
omissions communes et erreurs numériques partagées restent possibles, limites
déjà annoncées par le développeur. Les grandes coquilles ne deviennent pas
un algorithme de production : >24 refuse dans ce juge de diagnostic.
Aucun gain GPU, contrat 100 ms ou régime massif n'est promu par cette revue.

Les nouveaux reçus CPU exécutent 54c167bb6 puis e49ea4690, pas61da :
q3/R a la qualification Release/u21 de claudeab8 (673 portes/sept mutants),
le second lot conserve son refus de style malgré ses portes numériques,
TSan et mutants favorables. K5/W48 libre :408/296/360ms ; K10/leaf24 :
2506/1822/2064ms, ces 48 diagnostics ne portant pas de hash canonique.
Limiter la phrase d’identité aux126 A/B et séparer workers/affinité avant
d’attribuer leur comparaison au SMT ou à la mémoire. Les temps sont
descriptifs et ne transfèrent aucune qualification GPU.

Les conseils sont intégrés à la [note moteur courante](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
Six Markdown actifs sont conservés dans audits/, sans nouvelle note ni journal.
Le contrôle global de documentation reste en échec sur ses 213 liens v10
préexistants ; il exclut v11. Les liens des notes et des README de ce reçu
sont contrôlés séparément, hors copies historiques des sources.

Rejeu portable, sans écriture dans les capsules :

```sh
python3 -B -S check.py
python3 -B -O -S check.py
sha256sum -c SHA256SUMS
```

Le lecteur vérifie l'inventaire exhaustif et les empreintes de chaque capsule,
puis rejoue les modèles normal/optimisé avec résultats identiques. Les comptes
sont ceux de modèles mathématiques, d'AST ou de provenance, pas des tests CUDA.
Les portes G4 CPU/header/CUDA, limites numériques, refus mémoire et FULL
canonique restent à jouer pour qualifier cette voie publiée.
