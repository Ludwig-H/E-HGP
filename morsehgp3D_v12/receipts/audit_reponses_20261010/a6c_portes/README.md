# A6c : inventaire des portes et pénurie — 10 octobre 2026

Prélecture statique du candidat `aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66`, comparé à R1
`8a0716e7470197c95953b38d79f26b8d8f2379fc`. Sources seulement : aucun moteur, compilateur, CTest,
GPU, GCP ni donnée LiDAR. Ce reçu ne qualifie ni les sorties natives, ni les temps, ni les mutants exécutés.

## Résultat

Le manifeste tower contient **72 mutants**, contre 55 dans R1 : 17 ajouts. Les **76 substitutions**
s'appliquent chacune une seule fois, dans l'ordre déclaré ; les portes des 17 ajouts figurent dans
`tests/tower/tests.cmake`. Le lecteur du développeur a aussi rendu
`manifeste_ok module=tower mutants=72 plancher=72` avec `--check` sur le snapshot exact du candidat.
Cela confirme l'applicabilité du manifeste, pas la compilation ou la mise à mort des mutants.

La porte `bascule` force les deux chemins sur neuf couples de Sessions K5, 1 000 / 2 000 / 3 000 sites,
1 / 3 / 8 fils. Elle compare FUL1, registre, attaches, événements par survivant, rangs et nœuds des
événements, compteurs de travail. Elle exige les compteurs de chemin appropriés et au moins une aide
sur l'ensemble des Sessions engagées. Les deux chemins y utilisent un budget illimité.

**Couverture manquante : la chaîne engagée échappe désormais aux portes de budget et de pénurie.**
Le seuil par défaut vaut 43 900 sites ; aucun `chain_sites` forcé n'apparaît dans les deux fichiers :

| Porte | Entrée définie par le source | Chemin A6c |
|---|---|---|
| `pipeline_unit.cpp:admission` | 30 à 229 sites, K2..5 | chaîne non engagée |
| `pipeline_unit.cpp:refus` | 60 à 159 sites, K4 | chaîne non engagée |
| `pipeline_fault.cpp:allocation` | 70 sites, K4 | chaîne non engagée |
| `pipeline_fault.cpp:penurie` | 60 sites, K4 | chaîne non engagée |

Les nouvelles allocations par morceaux de N et les erreurs de la nouvelle organisation de H ne sont
donc pas exercées sous pénurie par ces portes. C'est une limite de qualification, pas un défaut
natif de mémoire démontré. Les petites fixtures évitent justement de devoir construire 43 900 sites.

La porte `region:fermeture` injecte manuellement `run_hint` pendant l'arrêt du noyau avant clôture,
puis après `hint_closed`. Ce test ciblé de la garde reste utile même si sa Session de 800 sites est
au chemin non engagé : le noyau ne poursuit pas ses accès ordinaires pendant l'aide injectée.
Il ne remplace pas une pénurie dans une Session engagée complète.

## Proposition native ciblée au développeur

1. Paramétrer les aides de test `measure` et `under_limit` par le seuil ; construire le `SessionRun`
   du test, fixer `chain_sites` à **0** ou au maximum **avant l'ouverture/admission**, et jouer les trois
   temps. Conserver la frontière `guarded` pour convertir les allocations qui lèvent. Aucun seuil
   public ni option du produit supplémentaire n'est nécessaire.
2. Rejouer admission et limite exacte / limite moins un dans les deux modes sur les petites fixtures,
   en vérifiant que le témoin ON a effectivement `chain_orders != 0`, N/H exécutés et le budget rendu.
   Garder des cohortes de taille supérieure à un à K5 (grilles déjà disponibles), pour allouer le
   tampon de sphères de `number_births_range`, ainsi qu'un témoin K1 sans ce tampon.
3. Conserver la porte publique `build_tower` des **trois allocations qui lèvent**. Ajouter dans la même
   cible de test un appel interne forcé ON et OFF pour la pénurie sans exception : compter d'abord
   les allocations de ce chemin, refuser chacune à un fil puis un échantillon à trois fils. Attendre
   `memory_budget`, aucun reste de budget ni attente indéfinie ; rejouer ensuite sans injection et
   comparer FUL1 au témoin. Ne pas reprendre un ancien nombre d'allocations.
4. Archiver le manifeste, la construction du témoin, le résultat détaillé des mutants, les codes,
   stdout/stderr et empreintes avant/après des exécutables réellement joués. La simple ligne de
   succès du CTest de campagne ne prouve pas individuellement les 72 verdicts.

## Relecture

Depuis un dépôt possédant les deux commits épinglés :

```sh
python3 -B morsehgp3D_v12/receipts/audit_reponses_20261010/a6c_portes/check.py /chemin/du/depot
python3 -O -B morsehgp3D_v12/receipts/audit_reponses_20261010/a6c_portes/check.py /chemin/du/depot
```

Le lecteur vérifie les substitutions séquentielles, l'enregistrement des portes ajoutées et les
ancres des fixtures, puis recalcule les empreintes des sources et compare `results.json`. Les
conclusions de couverture reposent sur la lecture des corps épinglés ; ce n'est pas une analyse
sémantique générale de C++. Les lignes de la proposition sont des travaux à faire sur G4.
