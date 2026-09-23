# Reçu : où va le temps des survivants q3/q4 (cœur, cover, atlas, voies)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. Mesure de diagnostic, hors produit : deux
instrumentations temporaires (`consultation.patch`, `phases.patch`) appliquées
sur `f9e6a552`, construites dans un répertoire séparé, puis retirées des
sources.

## Entrée et commande

Trame sans sol 08/000000, 39 885 sites (`scene_00_grid/full.u32le` du reçu v8
`lidar_ground_20260921`, SHA-256 `0baa4de1…`), s = 8, W8 et tour statique 8,
tous les leviers de R12. Le filtre q3/q4 passe par le chemin par lots CPU
(`q34_batch_filter=1`, `q34_gpu_filter=0`). Les condensés de tour sont ceux de
R12 : `67450c64` à K5, `ac108f7f` à K10.

Hôte partagé et chargé (charge moyenne 8 à 13 sur 8 cœurs) : les temps de mur
sont bruités. Seuls les **compteurs** et les **proportions** de cycles
(`rdtsc` par arête, sommés par fil) sont utilisés.

## Consultation du cœur par `prove` (K5)

Pour chaque chargement du cœur diamétral, le patch retient le plus grand
ordinal testé par les cellules du premier niveau balayé (profondeur 2, qui
parcourent la frontière identité). Au-delà de cet ordinal, les formes ont été
calculées sans être lues.

| arêtes | nombre | sites chargés | sites consultés | arrêt précoce | cycles construction / formes / preuve |
| --- | ---: | ---: | ---: | ---: | --- |
| fermées par le cœur | 1 143 235 | 341 485 633 | 184 787 624 (54 %) | 708 208 | 29,3 / 12,8 / 19,2 G |
| laissées ouvertes | 900 377 | 18 221 642 | 18 221 642 (100 %) | 0 | 7,4 / 1,2 / 13,6 G |

Le reste du travail des arêtes ouvertes (cover, certificat sur cover, atlas,
voies q3/q4) vaut **354,7 G cycles**. Calculer les formes à la première
consultation épargnerait au mieux 46 % des 12,8 G cycles de formes des arêtes
fermées, soit **environ 1,3 %** de la phase. Piste fermée sans port.

## Ventilation de la phase des survivants

| poste | K5 (G cycles) | part | K10 (G cycles) | part |
| --- | ---: | ---: | ---: | ---: |
| cœur : construction | 41,5 | 8,9 % | 79,2 | 6,2 % |
| cœur : formes | 15,7 | 3,4 % | 30,4 | 2,4 % |
| cœur : preuve | 36,5 | 7,8 % | 80,0 | 6,2 % |
| cover : construction | 15,0 | 3,2 % | 30,2 | 2,4 % |
| cover : formes | 11,5 | 2,5 % | 29,5 | 2,3 % |
| cover : preuve | 44,5 | 9,5 % | 109,4 | 8,5 % |
| atlas | 103,2 | 22,1 % | 369,8 | 28,9 % |
| voie q3 | 103,6 | 22,2 % | 191,5 | 14,9 % |
| voie q4 | 95,5 | 20,4 % | 361,0 | 28,2 % |

Comptes associés (ledger de la sonde) :

| | K5 | K10 |
| --- | ---: | ---: |
| survivants du filtre | 2 043 612 | 4 507 278 |
| fermés par le cœur | 1 143 235 | 2 572 879 |
| covers construits | 900 377 | 1 934 399 |
| fermés par le certificat sur cover | 191 691 | 471 037 |
| tests uniformes cœur / cover | 518 M / 940 M | 1 789 M / 3 443 M |
| boules q3 / q4 émises | 691 284 / 158 496 | 2 898 219 / 1 732 548 |

## Lecture

- Le cœur et le cover avec leurs certificats font **35 %** de la phase à K5
  et **28 %** à K10. Ce travail est régulier : un parcours d'arbre sans pile,
  puis des balayages de frontière avec arrêt au seuil. Il se porte sur GPU
  avec des compteurs identiques. C'est la cible S3.
- L'atlas et les voies q3/q4 des arêtes restées vivantes font **65 %** à K5
  et **72 %** à K10. Aucun certificat de voie ne les atteint : ce sont les
  arêtes qui émettent. Leur port GPU (ou un partage entre arêtes) reste le
  verrou principal au-delà de S3.
- Les proportions viennent d'un hôte chargé et d'un seul binaire instrumenté ;
  la mesure G4 du port S3 dira le gain réel.

## Contenu

- `consultation.patch`, `phases.patch` : les deux instrumentations, contre
  `f9e6a552`.
- `out/` : sorties JSON de la sonde et lignes `EXP_*` sur stderr.
- `SHA256SUMS`.
