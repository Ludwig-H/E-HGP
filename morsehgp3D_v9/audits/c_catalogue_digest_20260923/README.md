# Condensé du catalogue v18 : contrôle, épingles R13 et porte proposée (auditeur C)

23 septembre 2026. Cadre : `phase=exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. GCP non utilisé.

## Contexte

Le condensé FULL ne voit pas une boule redondante omise : la tour n'en
dépend pas. C'est ce que montre la [campagne d'omission](../c_omission_20260923/README.md).
C avait donc annoncé un condensé canonique du catalogue pour rendre les
comparaisons G4 GPU/moteur sensibles au catalogue entier. Le développeur
l'a intégré lui-même à S3 (sonde et protocole v18) : paquet local
`308110bc3`, publié ensuite en `ab6eb44d3` puis corrigé en `545c71799`.
Le condensé est calculé hors chrono, et le worker l'exige égal entre
jumeaux. C ne propose donc pas de second condensé. Ce dossier apporte trois
choses :

1. un contrôle indépendant de ce condensé sur les trois trames de R12 ;
2. les valeurs épinglées dont chaque cas de R13 doit hériter ;
3. une porte dédiée, absente du paquet :
   [`catalogue_digest_gate.patch`](catalogue_digest_gate.patch). Elle
   s'applique telle quelle sur `308110bc3` et sur `545c71799`.

## Épingles pour R13 (encodage v18, inchangé de `308110bc3` à `545c71799`)

Entrées : les trois trames sans sol de R12, prises dans le reçu v8
versionné (`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_0X_grid/full.u32le`).
Paramètres : `s=8`, 8 fils, build Release CPU (sans CUDA) de `308110bc3`.
Trois bras : moteur (`0/0`), lot CPU (`q34_batch_filter=1`) et
certificats S3 CPU (`q34_batch_filter=1`, `q34_batch_certificates=1`).
Nombre de boules et condensé FULL sont **identiques à R12** dans les six
cas. Le condensé du catalogue est **identique entre les trois bras** dans
chaque cas. Sorties : `results/probes/`. Sur la tête publiée `545c71799`,
la trame 08/000000 redonne les mêmes valeurs à K5 et K10, pour le moteur
comme pour les certificats S3 CPU (`results/probes_545c71799/`).

| trame | sites | K | boules | condensé FULL (= R12) | condensé du catalogue v18 | certificats S3 CPU (ms) | reprises |
| --- | ---: | ---: | ---: | --- | --- | ---: | ---: |
| 08/000000 | 39 885 | 5 | 1 306 696 | `67450c64611075b1` | `5ad1fe09354411ba` | 5 337 | 0 |
| 08/000000 | 39 885 | 10 | 5 512 670 | `ac108f7f71096c3f` | `a6e959d227f3dafa` | 11 876 | 0 |
| 08/000100 | 35 551 | 5 | 1 095 926 | `dbf799c8ed83f53f` | `a4a5149c15b4122e` | 3 800 | 0 |
| 08/000100 | 35 551 | 10 | 4 383 302 | `9ddbf7430c9086cc` | `c5cddc5b0baefcf1` | 7 554 | 0 |
| 08/000200 | 45 845 | 5 | 1 407 885 | `8240af3d4dce3d45` | `143a367b4f27ef02` | 4 249 | 0 |
| 08/000200 | 45 845 | 10 | 5 483 320 | `ba973af0c8da95bd` | `5c8cc01b1e45b461` | 12 895 | 0 |

Un cas R13 sur ces entrées, GPU ou non, doit reproduire l'avant-dernière
colonne. Un écart signale une différence de catalogue, même si le
condensé FULL et le reste de `logical_result` sont égaux. Les épingles
valent pour cet encodage, qui hache les indices de l'index de la tour et
non les PointIds. Tout changement de cet encodage, ou de
l'ordre des indices de `CloudIndex`, change ces valeurs sans changer
l'objet.

## Porte proposée : `mhgp9_catalogue_digest`

[`catalogue_digest_gate.patch`](catalogue_digest_gate.patch) ajoute
`tests/chain/catalogue_digest_gate.cpp`, ses trois entrées CMake et deux
points de mutant dans `catalogue_digest` (`#if`, absents des cibles
produit). La porte juge :

- **fixture gravée de coquilles étendues**, jugée en premier. Huit coins
  d'un cube (arité 2, coquille 8). Un triangle aigu et un point hors plan
  sur sa sphère (arité 3, coquille 4). Cinq points entiers de
  $x^2+y^2+z^2=9$, sans paire antipodale ni triplet coplanaire avec le
  centre (arité 4, coquille 5). Échelle 100, loin d'un fond de 400 points.
  Les trois boules doivent être au catalogue ;
- **recalcul local** de l'encodage documenté, sur le catalogue gardé ;
- **égalités** : moteur 1 fil, moteur 4 fils, lot CPU et certificats S3
  CPU. Condensés égaux et catalogues canoniques égaux, sur la fixture et
  sur trois familles à K5 et K10. Option coupée : condensé et temps nuls,
  tour FULL identique. Liste renversée : même condensé ;
- **sensibilité** : six altérations (boule retirée, identifiant, site
  passé de l'intérieur à la coquille, niveau, arité, clé) changent le
  condensé ;
- **zone aveugle, par arité** : vingt boules q2 à $p=K_{\max}-1$ et vingt
  q3 à $p=K_{\max}-2$, à pas régulier, retirées une à une. Plancher : au
  moins une par arité laisse la tour FULL inchangée, et le condensé les
  voit toutes ;
- **deux mutants recompilés** : dernière boule omise ; coquille étendue
  hachée sur ses seuls `arité` premiers sites. Tous deux sont tués sur la
  fixture (code 1, `cause=catalogue_digest.recompute cospheric K5`).

Résultats sur `308110bc3` avec le patch, build Release CPU local
(`results/`), confirmés sur `545c71799` (`results/published_545c71799.txt`) :

- porte `--n=1500`, code 0 : `runs=34` et 1 122 536 boules. Coquilles
  étendues de la fixture : 8, 4 et 2 aux arités 2, 3 et 4. Les six
  altérations sont vues. Zone aveugle : 18 retraits q2 sur 20 et 13 q3 sur
  20 laissent la tour FULL inchangée, et le condensé les voit tous ;
- mutants : code 1, `cause=catalogue_digest.recompute cospheric K5` ;
  argument invalide : code 2 ;
- suite `ctest -L gate` : 152/152, un test désactivé en amont.

La porte a été contre-lue par trois lentilles adverses (chaîne, porte,
contrat sonde/worker) dans une version antérieure écrite pour une
variante de C. Deux défauts confirmés sont corrigés ici. D'abord, la zone
aveugle ne retirait que des q2, dont les clés précèdent les clés q3 ; le
tirage se fait maintenant par arité. Ensuite, les coquilles étendues
n'étaient couvertes que par hasard (5 boules sur 1,1 million à 1 500
sites, 0 à 300), d'où la fixture et le second mutant. La lentille
« contrat » a dressé une liste d'adoption pour le worker. `308110bc3` la
couvre déjà : clés, champ de temps, 16 hexadécimaux, mur externe incluant
le condensé, `logical_result`.

## Remarques au développeur

- **Coût hors chrono.** Le condensé à un fil coûte localement 0,39 à
  0,44 s à K5 et 1,5 à 2,6 s à K10, selon la trame et le bras, sur une
  machine chargée (`results/probes/`). Il allonge le mur de chaque cas G4, pas le contrat.
  Une variante de C à deux étages (une ligne FNV par boule en parallèle,
  puis un pli en ordre de clé), mesurée puis non publiée, descend à
  0,41 s à K10 sur 8 fils locaux. Elle reste disponible si ce coût gêne
  en session.
- **Portée.** L'égalité de deux condensés de 64 bits établit l'égalité
  du catalogue entier à une collision FNV près : chaque clé, niveau,
  arité et identifiant y entre. Elle **ne localise pas** une différence
  et ne remplace pas le différentiel littéral des petites portes (B,
  17 h 00). Elle ne juge pas non plus la vérité du catalogue : deux
  chemins également faux auraient le même condensé. C'est le rôle des
  juges q2/q3 et d'Euler. « Clé par clé » dans `docs/PROVENANCE.md`
  gagnerait à devenir « égalité du catalogue entier (condensé) ».
- **Localiser.** En cas d'écart, garder les deux catalogues
  (`keep_catalogue`) sur la même entrée et les comparer après tri
  canonique. [`batch_diff`](../c_omission_20260923/batch_diff.cpp) le fait
  pour les bras moteur et lot CPU ; pour un autre bras, il suffit de
  changer les leviers de la seconde chaîne.
