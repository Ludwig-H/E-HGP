# Reprise indépendante v9 — q2/q3/q4 et G4

26 septembre 2026. Lecture de `a22dc9657..c529c82bb` dans un arbre isolé au
HEAD `c529c82bb`. Périmètre : générateur q2/q3/q4, intégration G4 R21/R22,
preuves de reçu et coûts de croissance. Aucun GCP utilisé pour cet audit ;
aucun moteur, reçu ou build épinglé modifié. Le statut public reste
`exploration_v9_hors_registre` / `not_claimed`.

## Verdict et portée

La v9 exécute maintenant **une tour FULL explicite** avec filtre,
certificats et voies q3/q4 sur la G4, puis catalogue, census et tour sur
CPU. R21 intègre la queue E4, le groupement haché et le pool E2 ; R22
ajoute le catalogue scellé, le census q2 pendant les appels GPU et le
bassin de mémoire hôte épinglée. Les sorties des bras appariés sont égales
dans les reçus ; les contrôles de ce rapport les recoupent depuis les JSON
bruts. La qualification reste **relative au catalogue émis** : Euler ne
porte que sur `K≤Kmax−2` et une clé géométrique jamais émise peut échapper
à ces égalités. Le [juge indépendant de supports bruts](c_raw_support_judge_20260925/README.md)
est vert sur six cas bornés, sans fermer la complétude générale.

| R22, s8/W48, grille 1 mm | sans sol K5 | sans sol K10 | brut avec sol K5 | brut avec sol K10 |
| --- | ---: | ---: | ---: | ---: |
| `chain_total`, trois trames de 08 | 0,760–0,983 s | 2,269–2,994 s | 1,810–2,027 s | 5,311–6,010 s |
| nombre de passages par trame/K | 1, sauf 000000 : 2 | 1, sauf 000000 : 2 | 1 | 1 |

La seconde est franchie pour **ces trois trames sans sol à K5**, dont
08/000200 à 0,983 s sur un seul passage, seulement 17 ms sous le seuil.
Elle ne l'est ni sur le brut à K5 ni à K10. Les six trames sont les trois
numéros 000000/000100/000200 de **la seule séquence 08**, sous deux
masques ; ce ne sont pas plusieurs séquences. Le profil exact testé est
u18 sur grille 1 mm, pas les coordonnées float32 brutes. Le reçu annonce
`contract_certified=false`, comme l'exige le [contrat v9](../README.md).
Le `FULL_executed=true` des reçus signifie que la tour a été produite,
pas que son contrat de temps ou sa complétude absolue sont acquis.

## Relecture reproductible des reçus

- `sha256sum -c SHA256SUMS` donne **534/534** fichiers OK pour
  [R21](../receipts/g4_tower_r21_20260925/README.md), **560/560** pour
  [R22](../receipts/g4_tower_r22_20260926/README.md). Les six fichiers
  d'entrée v8 nommés dans `PACKAGE.json` R22 existent dans cet arbre ;
  leurs SHA-256 et tailles `12×n` concordent, pour 35 551 à 125 526 sites.
- `host/receipt.json` et `vm/receipt.json` sont `completed` aux deux
  sessions ; la source, ses dépendances compilées et le binaire sont
  marqués stables, la garde et l'arrêt ciblé certifiés. Les
  arbres Git des commits de paquet `d054c1c5` (R21) et `43c5ad25`
  (R22) correspondent exactement aux champs `tree` de leurs
  `PACKAGE.json`. Ceux-ci disent `prepared_not_executed` et
  `GCP_used=false` :
  ce sont les manifestes **avant** session, pas une négation des reçus
  d'exécution. La relecture `TERMINATED` annoncée dans les README n'est
  pas jointe en pièce distincte, comme l'a relevé [C pour R21](CONTRE_AUDIT_C_R21_TOUR_INTEGREE_20260926.md)
  et [R22](CONTRE_AUDIT_C_R22_20260926.md).
- Les **34/34** sorties R21 sont `complete_relative` au schéma v27 ;
  **36/36** sorties R22 le sont au schéma v28. Relecture directe des
  `vm/probe_N.stdout` contre `SUMMARY.json` : statut, effectif, boules,
  Euler et `chain_total` cohérents sur les 70 cas. Les **22/22** paires
  R21 et **24/24** paires R22 de `cross_worker_comparisons` égalent, depuis
  les sorties brutes, digest de tour, catalogue, présentations et
  `tower_work`. Les 12 épingles de catalogue et de tour de
  `c_catalogue_digest_20260923/results/PINS.json` et
  `c_raw_pins_20260924/PINS_RAW.json` concordent avec chaque sortie des
  70 cas (zéro divergence de digest, effectif ou nombre de boules).
- R22 : **30** cas scellés ont exactement `ceil(boules/64)` boules
  échantillonnées ; **18** cas à census q2 précoce ont un nombre non nul
  de clés précoces ; **18** cas à bassin épinglé n'ont aucune allocation
  pendant les appels. Les 24 comparaisons incluent ablations unitaires et
  bras `gpu_r21` sur 08/000000. Les ablations unitaires n'ont qu'un
  passage et seule la répétition 1 du bras `gpu_r21` est entrelacée :
  leurs petits écarts de temps ne sont pas une estimation précise du gain.

Ces contrôles ont été recalculés par `jq` et SHA-256 dans l'arbre isolé.
Les scripts annexes de C pour R21/R22 pointent vers son ancien worktree et
un `rows.json` sous `/tmp`; ils ne sont pas, tels quels, des lecteurs
portables. Leur [contre-lecture R22](CONTRE_AUDIT_C_R22_20260926.md)
apporte en plus une revue des chemins d'échec et des mutants, que les
comparaisons de digest ci-dessus ne remplacent pas.

## Frontière des chronos et effet réel des leviers

`chain_total` exclut l'ouverture du contexte CUDA, la réservation du
bassin, le digest de contrôle et les coûts du processus. Le mur externe
est dans `SUMMARY.rows[].wall_s`. Pour **08/000000** :

| cas | chaîne R21 | chaîne R22 | mur R21 | mur R22 |
| --- | ---: | ---: | ---: | ---: |
| sans sol K5, premier passage | 0,972 s | 0,926 s | 1,722 s | 1,718 s |
| sans sol K10, premier passage | 3,165 s | 2,961 s | 5,980 s | 6,029 s |

Le gain de chaîne R22 est réel, mais le coût de processus ne diminue pas
à K5 et **augmente** à K10 sur ces passages. À 00/K10, l'ablation R22
du seul bassin donne `chain_total=3,121 s`, `wall_s=5,885 s` ; avec
bassin, 2,961/2,994 s de chaîne mais 6,029/6,036 s de mur. La session
réserve **256 Mo** à K5 en 38–41 ms et **1,28 Go** à K10 en 190–201 ms,
hors chaîne. La copie des enregistrements tombe de 27,8 à 1,9 ms à K5,
et de 144,9 à 10,5 ms à K10 sur l'ablation de 00. Ce levier attend
donc une **mesure multi-trames dans un même processus résident** avant un
crédit de coût total ; il élève aussi le RSS R22 jusqu'à environ 10 Gio
au brut K10. Ne pas soustraire `q2_census_ms` ou les sous-chronos recouverts
de `chain_total` : seul `q2_census_wait_ms` entre dans le chemin visible.

Le [code de chaîne](../src/chain/tower_chain.cpp) confirme que les trois
leviers sont des options séparées, inactives par défaut dans
`ChainOptions`. Le sceau vérifie arité, IDs, domaine, support régulier et
positivité au census avant la tour ; la passe 1 complète est réduite à un
résidu fixe **0 modulo 64**. C'est une décision de coût documentée, avec
le résidu de corruption en processus hors échantillon, jamais une
validation exhaustive de tout catalogue arbitraire passé à la tour.
Le census précoce réutilise l'index de la tour et déplace les clés q2
pendant les appels, mais `balls(unique)` initialise encore 224 octets par
clé et la recopie puis la libération du tableau précoce sont dans le
chrono. À 00, le gain de `census_ms` restant est seulement de l'ordre
de 1–5 ms à K5, 12–22 ms à K10 selon la base de comparaison ; le reste
vient notamment de l'index déplacé. Le [bassin](../src/gpu/record_pool.hpp)
maintient les baux jusqu'à la conversion ; le compteur zéro allocation
en appel ne prouve pas qu'un lot plus grand que la capacité réservée ne
fera jamais grandir le bassin.

## Travail q3/q4 et parallélisme restant

Sur R22 brut 08/000200/K5, la chaîne prend 2,027 s, dont q3/q4 0,970 s,
tour 0,695 s et census tardif 0,229 s. Le front q3/q4 coûte 256 ms ;
le filtre 160 ms, les certificats 202 ms et les voies 166 ms. Ces quatre
sous-chronos ne partitionnent pas strictement `q34_ms` : les préparations,
conversions et passages hôte restent dans le total. Sur le même brut à
K10, q3/q4 prend 2,197 s, tour 2,348 s et census 1,024 s, sur une
chaîne de 5,907 s. Le front coûte 478 ms, les certificats 454 ms et les
voies 565 ms. La phase A de la tour et la « glu » hôte sont encore sur
le chemin critique ; voir le [profil local corrigé de la phase A](c_phase_a_profil_20260926/README.md)
pour les proportions et ses limites de mesure.

Les 48 workers q3/q4 ne suppriment pas la tâche traînarde : sur les trois
brutes R22/K5, les fronts durent 238–288 ms alors que le plus gros job
prend 170–223 ms et que la somme CPU des jobs / 48 vaut 143–163 ms.
Ce n'est pas une simple question de transfert GPU. À R22 b02/K10,
`lanes4_max_buffered=4091` pour une capacité de 4096 ; huit voies sont
reportées au moteur, sans changement d'objet. Le corpus observé touche
donc déjà la marge q4. Le même cas compte environ **1,299 milliard**
d'incidences site–cœur et **1,082 milliard** site–cover, avec 9,056
millions d'enregistrements de voies. Les gardes de capacité et le report
fonctionnent sur ce cas ; la stabilité sur des trames plus denses et la
mémoire HBM ne sont pas mesurées dans R22.

## Croissance : trois expériences à ne pas fusionner

1. Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md)
   choisit, sur chacune de trois trames **sans sol**, des **disques
   emboîtés** de 8k⊂16k⊂32k sites, puis la trame entière. À
   08/000200, le doublement 16k→32k donne `p=log₂(W₂/W₁)` pour les
   `core_sites` de **3,05 à K5** et **2,86 à K10**, et pour les paires
   développées **2,27/2,09**. Les pentes du mur de chaîne y sont
   **1,49/1,38** : les postes internes défavorables sont masqués dans
   ces petits temps finiment mesurés. Aucune campagne 8k/16k/32k du
   **binaire R22 sur G4** n'existe ; les valeurs v12 ne se transfèrent
   pas automatiquement au chemin GPU ni au code actuel.
2. Les [coupes spatiales et densités sans sol](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
   sont un autre axe : 13/36 liens spatiaux à densité pleine et 10/84
   liens de densité franchissent `p_core≥2`, alors qu'aucun des 84 liens
   de densité ne franchit 2 en CPU de chaîne ou paires développées.
   Les moitiés/quarts conservent leurs coordonnées globales, mais
   changent géométrie, arêtes et frontières ; leurs tours ne se somment
   pas pour reconstruire la tour entière.
3. Sur **08/000000 brute avec sol**, les [trois densités de scène entière K5](lidar_raw_physical_scaling_20260923/README.md)
   contiennent 30 847→61 694→123 389 sites : le dernier lien atteint
   `p=2,136` pour `dead_core_form_sites`, les formes de cœur
   matérialisées hors deux extrémités par charge, contre
   `p=1,297` pour le CPU de chaîne. Le [panneau K10](lidar_raw_k10_density_20260923/README.md)
   sur les mêmes entrées donne `p_formes=1,936`, `p_CPU=1,239`, mais
   `dead_core_form_sites=1 238 630 455` (hors extrémités),
   `core_sites=1 254 254 109` (extrémités comprises),
   **11 387 391** boules et 8,219 Gio de RSS local au plein. La coupe en secteurs physiques
   trouve encore 7/18 liens spatiaux et 2/14 liens de densité à
   `p_formes≥2` à K5, puis 3/18 et 1/14 à K10. Ces chiffres ne
   prouvent ni une borne quadratique universelle ni une borne
   sous-quadratique globale ; ils isolent le coût de matérialisation
   par cœur et l'effet de l'étendue spatiale.

Le [test s8/s10/s12 sur un quart](b_s8_s10_s12_k5_quarter_20260924/README.md)
de 1 288 sites donne 46 218→40 728→37 843 paires développées, mais
37 459→42 686→47 158 rectangles ; les 27 099 survivantes, le cœur,
les digests catalogue et tour restent identiques. Ses murs locaux sont
instables. J'ai revérifié les trois SHA de sorties du manifeste et les
champs de travail/digest des JSON ; le lecteur `sweep.py verify`, normal
et `-O`, échoue **après** ces contrôles sur `command argv`, car il
reconstruit le chemin absolu courant de l'entrée au lieu du chemin
absolu archivé. C'est une limite de **portabilité du lecteur**, pas une
divergence géométrique constatée. La [session R8](../receipts/g4_tower_r8_20260923/README.md)
comparait s8/s10/s12 sur une seule trame entière sans sol, en CPU G4
(3,67/3,77/4,19 s à 000100/K5). R21/R22 exécutent seulement `s=8` :
aucune élection actuelle de s sur les trois trames brutes et sans sol
en GPU G4 n'est acquise.

## Suite utile au développeur

1. Garder distincts `chain_total`, mur externe d'un processus, et mur
   d'un processus résident multi-trames. Tester le bassin sur ce dernier
   avant d'en créditer le débit ; publier simultanément RSS et HBM.
2. Sur le même code que R22, apparier `s=8/10/12`, K5/K10,
   sans sol/brut, W1/W48, les disques 8k/16k/32k **et** les secteurs
   physiques/entiers, avec `R,P,S`, formes cœur/cover effectivement
   payées, reports/capacités q4, catalogue, tour, CPU, murs et mémoire.
   Ces mesures doivent rester des diagnostics distincts du contrat
   multi-séquences.
3. Prioriser la réduction des formes **avant** `dead_.load`, la
   redistribution du front traînard et la phase A de **tous** les
   ordres. Les certificats par nœuds essayés après chargement avaient
   réduit les formes tout en régressant en CPU : exiger le bilan complet
   de bornes, replis, sorties et FULL pour chaque nouvelle variante.
4. Compléter les portes R22 à faible coût signalées par C : échantillon
   scellé vérifié au moins une fois, refus exact et absence de repli
   silencieux du census q2, croissance réelle du bassin, panne de
   lancement des aides. Ne pas transformer un digest égal ou Euler
   `holds` en preuve de clés absentes.

Sources principales : [provenance](../docs/PROVENANCE.md),
[R21](../receipts/g4_tower_r21_20260925/README.md),
[R22](../receipts/g4_tower_r22_20260926/README.md),
[contre-lecture C de R22](CONTRE_AUDIT_C_R22_20260926.md),
[croissance q3/q4 après S2](CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md).
