# Contre-audit de la campagne LiDAR v12 locale — 23 septembre 2026

**État : reçu archivé par `1f73b40d`, contrelecture close.** Le processus
local `57287` est terminé. Le [reçu v12](../receipts/lidar_scaling_local_20260923/README.md)
contient six résumés et **60 cas individuels**, tous `complete_relative`, sans
fichier `.failure.json`. Ses **70/70** lignes de `SHA256SUMS` passent ; les
66 JSON et `campaign.log` sont identiques octet pour octet à la campagne
LIVE observée. Tous les champs de `SLOPES.json` recollent aux six résumés.
Sur les 60/60 cas, schéma, statut et raison, options, ordres 1..K, digest,
résumé contre JSON individuel, SHA-256 des octets passés à la sonde, FNV-1a
interne de l'entrée et invariants/bornes des nouveaux compteurs concordent.
C'est une vérification de cohérence et de provenance, pas un rejeu HGP.

## Périmètre et identité

Campagne CPU locale issue de `build/v9-open-worktree/build/v9-scaling/`,
archivée dans `morsehgp3D_v9/receipts/lidar_scaling_local_20260923/`. Le script
`morsehgp3D_v9/bench/run_lidar_scaling.py` au commit `4530644b` exécute la
sonde `mhgp9_tower_probe_v12` de SHA-256
`e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`.
Les six résumés déclarent ce même HEAD, les mêmes arbres `src`
(`ceba3397…`) et `bench` (`c2c617e0…`), et un worktree propre pour ces deux
arbres. Il s'agit d'une lecture des objets présents, sans rejeu HGP.

Entrées : les trois trames 08/000000, 08/000100 et 08/000200 **sans sol**
du reçu v8, grille entière 1 mm, conteneur `u32le` avec domaine 18 bits ;
masque Patchwork++ géométrique approximatif. Pour chaque trame, K5 et K10,
`s=8`, W8, répétition `r0`, trois sous-nuages emboîtés 8k/16k/32k puis
sept morceaux capteur, dont la trame entière. Les emboîtés sont des sélections
par distance horizontale au centre médian ; les moitiés et quarts sont des
coupes capteur distinctes. Ces trois trames appartiennent à une seule séquence.
Les sept morceaux sont classés par signe **après la grille 1 mm**, pas par
les mots float32 bruts : la [contre-lecture des découpes](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md)
trouve 0, 0 et 1 site changé de côté sur 00, 01 et 02 après retrait du sol.
Dans 08/000200, l'ID original 61939 passe de `x<0,y<0` brut à `x≥0,y<0`
quantifié, car `x≈−0,0000909 m` devient zéro. Le reçu couvre donc les coupes
de la géométrie quantifiée explicitement déclarée ; il ne vaut pas mesure
des coupes physiques strictement définies sur les signes bruts.
Tous les JSON individuels lus indiquent `complete_relative`, dont la raison
exacte est `complete_relative_to_cross_checked_catalogue` : la complétude
des clés géométriques absentes du catalogue n'est pas ainsi démontrée.

Contrôles indépendants des entrées présentes : SHA-256 des trois fichiers
bruts, trois masques, 21 fichiers de points et 21 fichiers d'IDs des sept
morceaux = les trois manifestes v8 ; chaque partition en deux moitiés et en
quatre quarts couvre exactement les IDs de la trame pleine, sans recouvrement,
avec les mêmes coordonnées par ID. Les fichiers emboîtés K5 actuellement
présents correspondent à leurs SHA-256 de cas ; pour les trois scènes, les
SHA-256 des points et IDs emboîtés sont identiques entre K5 et K10.

Épingles communes à la lecture **LIVE** et aux résumés désormais archivés :

| Résumé | SHA-256 |
| --- | --- |
| `00/K5` | `30771b8303ae6b552b3398b3ebc8ca8d3914e6a41d0f322912faa4ce89172d6a` |
| `00/K10` | `65f638a282415ad010119539c4a56928baa5ec4b6bd240b92ad95f6e246109db` |
| `01/K5` | `b5ef2cd040ee54d566712742befc390d171ac9f4ad2046b18858d917496b8efd` |
| `01/K10` | `064267073a59c96cbbe778c1ea7e1d0b85d9443ede4b37e880f9f01babf87419` |
| `02/K5` | `710783185e0b1a15240714b349da0a9515288e40254bc183620625d8d8da6db1` |
| `02/K10` | `b98dd58c8251a6ff23cbd77852b001cae6561decb270902fbcb812822bd96feb` |

`campaign.log` : SHA-256
`fd773287743d6a1af55a5356a61248a7bb820766676bf2103fa64409b363a2e6`.
Les six résumés déclarent les trois SHA-256 de manifeste v8 attendus et le
même objet de masque que leurs manifestes respectifs. Aucun octet KITTI ni
binaire ELF n'a été ajouté à Git ; les empreintes de leurs entrées et de
l'ELF sont conservées dans le reçu.

## Coût de la trame entière

Temps internes de la chaîne, hors digest ; locaux, CPU et non G4. « Six DFS »
est la somme des visites de nœuds des deux recherches de témoins, des graines
q3 et des trois parcours q4 (domaine,
décomposition du cover, graines), **sans déduplication ni pondération**. Ce
n'est pas un total d'opérations.

| Trame | K | Sites | Chaîne s | q3/q4 s | Tour s | Boules M | Six DFS M |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 00 | 5 | 39 885 | 25,609 | 20,703 | 2,391 | 1,307 | 897 |
| 00 | 10 | 39 885 | 89,890 | 63,587 | 17,219 | 5,513 | 2 089 |
| 01 | 5 | 35 551 | 17,394 | 13,729 | 1,738 | 1,096 | 639 |
| 01 | 10 | 35 551 | 66,175 | 45,235 | 14,817 | 4,383 | 1 516 |
| 02 | 5 | 45 845 | 28,413 | 22,648 | 2,167 | 1,408 | 952 |
| 02 | 10 | 45 845 | 94,326 | 69,464 | 14,449 | 5,483 | 2 258 |

q3/q4 absorbe environ 68–81 % de la chaîne dans ces six cas. La tour FULL
atteint 14–17 s locaux à K10 : son coût reste matériel même si q3/q4 domine.
Les catalogues contiennent 1,1–5,5 M de boules sur ces trames.
Les balayages répétés des sites actifs q4, **hors des six DFS**, traitent
85,058 / 63,584 / 72,471 M de sites à K5 sur 00 / 01 / 02, puis
525,955 / 403,414 / 466,715 M à K10. Ils doivent rester visibles dans le
grand-livre de travail à côté des 2,089 / 1,516 / 2,258 milliards de visites
des six DFS à K10.

## Croissance locale des trois tailles emboîtées

Les triplets ci-dessous correspondent à **8k / 16k / 32k**. Les chiffres
arrondis sont en secondes ou millions de comptes. Les trois tailles ne
définissent aucune borne asymptotique et leurs densités géométriques changent.

| Trame/K | Chaîne s | Paires développées M | `core_sites` M | Six DFS M |
| --- | --- | --- | --- | --- |
| 00/K5 | 5,603 / 12,407 / 23,921 | 3,648 / 12,787 / 23,073 | 33,606 / 257,617 / 349,173 | 184 / 439 / 759 |
| 00/K10 | 18,810 / 42,028 / 75,155 | 4,953 / 15,832 / 28,874 | 105,034 / 597,535 / 876,699 | 463 / 1 032 / 1 791 |
| 01/K5 | 5,787 / 11,414 / 18,889 | 4,970 / 8,712 / 11,633 | 93,349 / 165,806 / 192,960 | 154 / 361 / 591 |
| 01/K10 | 14,968 / 30,964 / 53,850 | 6,945 / 11,915 / 17,045 | 237,504 / 423,364 / 506,895 | 381 / 838 / 1 399 |
| 02/K5 | 3,589 / 7,316 / 20,573 | 1,843 / 4,433 / 21,344 | 14,873 / 40,449 / 334,344 | 107 / 243 / 728 |
| 02/K10 | 12,643 / 27,340 / 71,355 | 3,238 / 7,140 / 30,459 | 58,493 / 141,097 / 1 023,499 | 264 / 610 / 1 737 |

Les deux lignes **02/K5 et 02/K10, 16k→32k** sont particulièrement
défavorables. À K5, paires développées ×4,82 et `core_sites` ×8,27 ; à K10,
respectivement ×4,27 et ×7,25, contre ×2,61 pour le chrono de chaîne K10.
`dead_core_uniform_tests` augmente aussi de 397,861 à 1 965,318 M (×4,94) ;
les visites de témoins sur paires, de 235,358 à 884,570 M (×3,76).
Les rectangles q3/q4 font seulement 1,791→3,459 M (×1,93), tandis que
leur masse de paires d'entrée fait 37,762→113,260 M (×3,00) : la fraction
de cette masse développée passe de 18,9 à 26,9 %. C'est un recul de la
sélectivité du filtre rectangle sur ces deux sous-nuages, distinct du nombre
de rectangles. À 00/K5, `core_sites` fait également ×7,66 entre 8k et 16k ;
à 00/K10, ×5,69. Les temps doux ne suffisent pas à qualifier une croissance
du **travail total** sous-quadratique.

## Runner historique, correctif et limites restantes

Le reçu `1f73b40d` utilise le runner v2 : il ne relisait pas les IDs des
morceaux, ne comparait pas le FNV annoncé par la sonde aux octets d'entrée,
et pouvait écrire un succès avant qu'un champ de temps ou digest mal formé
ne lève une exception. Pour ces **60 cas conservés**, nos contrôles
indépendants des fichiers d'IDs, des SHA-256 et FNV et des champs de sortie
n'ont trouvé aucun écart. Le `grid=unspecified` de leurs JSON est le défaut
historique de la sonde ; le manifeste d'entrée décrit la grille 1 mm.

Le correctif `06f71037` traite ces trois défauts pour les prochaines
exécutions : contrôle points **et IDs** de chaque morceau ; FNV recalculé,
format, grille, fils statiques, digest et temps contrôlés avant succès ;
nouveaux appels avec `--grid=1mm --static=W`. Son
[addendum](../receipts/lidar_scaling_local_20260923_revalidation/README.md)
rejuge les sorties historiques selon leurs options d'origine, sans nouveau
calcul HGP : `REVALIDATION.json` rapporte **60 cas / 6 campagnes / 0 échec**
et ses 2/2 SHA passent. J'ai réexécuté la porte du lecteur en Python normal
et `-O` : **25/25 mutations refusées dans les deux modes**. Ce correctif ne
réétiquette pas rétroactivement les JSON historiques `grid=unspecified`.

Le revalidateur parcourt les résumés qu'il trouve et réussit dès qu'au moins
un cas est accepté sans échec. Il ne fixe pas lui-même les six campagnes
attendues ni ne compare K/s/W du résumé avec ceux tirés de `record.argv`.
Notre lecture indépendante a vérifié les six tags 00/01/02 × K5/K10,
`s=8`, W8, `r0` et les 60 cas du reçu précis ; cette limite porte sur
l'usage général du revalidateur, pas sur une divergence observée ici.

Le README du reçu initial contient encore plusieurs erreurs de prose : « 66
cas » compte en réalité **60 cas + 6 résumés** ; « onze JSON de sonde » par
campagne inclut le résumé du runner ; « boules sous-linéaires » est contredit
par `p=1,028` sur 01/K10, 16k→32k ; « seul signal superquadratique » omet
les paires développées (`p=2,267`), les visites de témoins sur paires
(`p=2,115`) et les tests uniformes du cœur (`p=2,406`) sur 02/K5, 16k→32k.
« Les parcours cachés v12 restent entre p=0,68 et 1,32 » est également
faux pour le parcours de témoins **par paire** (`p=2,115` dans ce même cas) ;
la somme non pondérée des six DFS fait 243→728 M visites (`p≈1,58`).
La fenêtre annoncée s'arrête à 08 h 03, tandis que le dernier résumé et le
journal LIVE sont datés **08 h 10 min 25 s UTC**. L'addendum corrige le
nombre de cas ; les sorties chiffrées archivées restent intactes.

Enfin, le lanceur shell LIVE aurait continué après un échec de groupe et
écrit `CAMPAIGN_DONE` sans condition ; ici six résumés et 60 cas ont été
contrôlés séparément. Le SHA de l'ELF et les arbres `src`/`bench` fixent les
objets observés, mais aucune fermeture de compilation liant formellement
l'ELF à ces arbres n'est archivée ; le binaire demeure dans `build/` ignoré.

Les compteurs v12 couvrent les six DFS jusque-là cachés et plusieurs autres
postes, mais leurs unités et recouvrements ne permettent pas de sommer
`WORK_KEYS` en un coût d'opérations unique. Ces reçus locaux ne sont ni une
exécution G4/GPU, ni la trame brute entière, ni des répétitions, ni plusieurs
séquences. Aucune qualification de la tour sous la seconde n'en découle.
