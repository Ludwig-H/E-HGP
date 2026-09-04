# Première session G4 v7 qualifiée

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [reçu public](../receipts/gcp_requalified_20260904/published/receipt.json)
est une projection sélectionnée des reçus privés vérifiés par le contrôleur.
Il conserve les statuts, hashes, compteurs, coûts et objets ; il ne remplace
pas le reçu privé ni ne constitue un certificat de performance.

## 50k : quatre paires complètes et identiques

Même entrée synthétique par famille et seed=3, n=50000, séparation s=8,
48 threads, CSR, digest, deux folds en vol. Tour entière K1..10 puis
réexécution réelle K1..5, puisque la seconde est dépassée. L'objet mesuré
est **`verified_events_only`**, pas Gamma complet. Le défaut du générateur
donne ici `coord=368` pour l'uniforme et `coord=1118` pour le terrain ;
aucun SHA littéral du tableau généré n'est revendiqué.

| Famille | Tour entière | v6, secondes | v7, secondes | v7 RSS max, Kio |
| --- | --- | ---: | ---: | ---: |
| Uniforme | 1..10 | 54,624 | 50,120 | 19 801 492 |
| Terrain | 1..10 | 18,357 | 18,283 | 4 280 308 |
| Uniforme | 1..5 | 9,953 | 10,117 | 3 997 828 |
| Terrain | 1..5 | 5,178 | 5,432 | 1 247 012 |

Les huit processus sont achevés, les quatre paires ont les mêmes digests
et cardinalités. Temps processus externe, génération et digest compris,
sans archive disque. Un seul couple par configuration, ordre v6 puis v7,
sans répétitions contractuelles : aucun gain statistique, p95 ou SLO.
Les temps K5 de la v7 sont légèrement supérieurs dans ces observations ;
le gain mono local ne se transpose pas automatiquement au multi-CPU.

Pour la v7 uniforme K10, génération 12,479 s, RLE 3,069 s,
préfiltre 2,839 s, census 2,731 s, puis fold en temps mur 23,760 s.
Le fold 36,233 s et le digest 18,223 s affichés ailleurs sont des cumuls
par étage en recouvrement : ne pas les additionner au mur de 50,023 s.
Les 134 766 584 facettes sont cumulées sur les dix ordres, pas toutes
simultanément résidentes. Le terrain K10 reste dominé par la génération,
12,427 s. Les compteurs complets permettent d'attribuer les futurs deltas.

Ces mesures ne satisfont donc ni la seconde sur 50k, ni le repli 1..5.
La cible 100 ms reste postérieure à la qualification de la seconde.
La comparaison s=8/10/12 exécutée en mono demeure une
[campagne locale distincte](RESULTATS_MONO_20260904.md), pas trois mesures G4.

## Complétion candidate : succès et refus distincts

À 50k avec les coordonnées par défaut, la route normalisée refuse une
extra-shell pertinente non prise en charge. C'est un refus de domaine,
pas une sortie exacte rapide. Aucun objet Gabriel n'est substitué au résultat.

À n=8000, uniforme, coord=65536, seed=3, 48 threads et s=8, la tour
normalisée 1..10 termine : 85,396 s processus, 85,348 s pipeline,
RSS max 3 492 804 Kio. Elle produit 4 384 229 événements et
26 434 998 facettes cumulées. Son statut reste
`normalized_horizontal_h0_candidate`, sans certificat global ni verticale.

La complétion compte 802 125 328 supports MEB examinés et 581 904 257
visites spatiales cumulées sur K2..K10 ; la chaîne maximale atteint 18.
Ces mesures identifient un levier mono concret : proposer les supports MEB
plus efficacement, puis conserver la validation entière positive/contenance
et un repli exhaustif borné. Cette optimisation est **proposée**, pas livrée
ni mesurée ici. Modifier l'ordre de recherche doit préserver les supports
canoniques, les refus de dégénérescence et la comptabilité des budgets.

## Primitives GPU et rattachement matériel

Les 12 portes CTest GPU passent : témoins device, census device et leurs
mutants. Elles ne constituent ni une tour GPU complète, ni une campagne
10M/50M, ni une performance GPU de bout en bout. Le temps CTest de 10,86 s
est une durée de qualification, pas une latence HGP.

[Matériel sélectionné](../receipts/gcp_requalified_20260904/environment_selected.json) :
AMD EPYC 9B45, 48 CPU logiques sur 24 cœurs, SMT2 ; RTX PRO 6000 Blackwell
Server Edition. Compilateur CPU GCC 11.4.0 ; CUDA 12.9.41. Le CPU utilise
CMake système 3.22.1, le GPU la distribution privée CMake 3.31.6 épinglée.
Le CLI G4 v7 a le SHA-256
`f7ec55a4e427c768910b1f299b153acdab2826ebec0abfebbd57255553eecf1f` :
ce n'est pas le binaire GCC13 de la qualification locale C, même si ses
sources moteur sont les mêmes. Les
[pins de la copie consommée](../receipts/gcp_requalified_20260904/published/source_pins.json)
identifient le worktree réel, pas une hypothèse de commit propre.

## Fermeture de la ressource

Session du 4 septembre 2026, 22:45:21 à 22:55:26 UTC. Cible
`devpod-gpu-exploration / us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35`,
id `4526841274685623561`, génération `2026-09-04T15:45:50.919-07:00`.
G4 SPOT, STOP GCE 3600 s et arrêt invité 45 minutes vérifiés avant calcul.
Le contrôleur certifie `TERMINATED` pour cette génération ; une lecture
indépendante root le confirme ensuite. L'inventaire en lecture seule ne
trouve aucune autre VM E-HGP active. Aucune autre VM n'a été arrêtée.
Le disque conservé reste facturable ; l'arrêt n'est pas une suppression.
