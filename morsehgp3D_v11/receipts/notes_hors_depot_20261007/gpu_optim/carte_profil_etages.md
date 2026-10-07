# Carte du profil mesuré par étage des quatre sorties (angle « profil » du chantier GPU)

Rédigée le 6 octobre 2026, 01 h 18 à 01 h 40 UTC (`date -u`), en lecture seule, au commit `df904711a` (origin/main).
Cadre : `exploration_v11_hors_registre / cpu_reference (référence) et cuda_g4 (voie mesurée) / quantized_u21_input_only
/ not_claimed`. GCP non utilisé. Aucun commit, aucune construction. Seule exécution : sept appels du CLI déjà construit
(`build/v11-persist/b21-s10/mhgp11`), sorties dans le scratchpad, détaillés au § 1.5.

Dans ce rapport, chaque chiffre porte l'une de ces trois étiquettes :

- **G4** : mesure sur la VM `g4-standard-48` (AMD EPYC 9B45, 24 cœurs × 2 SMT, RTX PRO 6000 Blackwell `sm_120`,
  97 887 Mio), lue dans un reçu ;
- **local** : mesure sur le codespace (AMD EPYC 7763, 8 vCPU, charge 4 à 6 venant d'autres sessions) ;
- **estimation** : un raisonnement, jamais une mesure.

Sources :

| Sigle | Pièce | Régime |
| --- | --- | --- |
| FIN | `receipts/developpement_20261005/qualification_finale/claudefinmesure/sorties_g4.json` (CLI à `38b76701b`, 52 appels, `full` et `supports` L2b) | G4, CLI, ligne d'état `stages_ns` |
| QUAL | `/workspaces/.ehgp-sessions/v11.20261005.claudequalmesure/results/extracted/results/cmd/000_mesure_l2/files/sorties_g4.json` (CLI à `b319efc84`, avant L2b : bras `supports` = `build_order`, masque 7 035, **la voie de `points` et `plat`**) | G4, CLI |
| AB | `receipts/developpement_20261004/mesures_g4_ab8_diag1/sessions/claude{ab8,diag1}/ab_report.json` (sonde `mhgp11_full_bench`, sous-étages `domain_detail` et `phases`) | G4, sonde FULL |
| GPU | `receipts/developpement_20261004/gpu_g4/sessions/claudegpu3..6/gpu_k*_report.json` (voies CPU 16379, lot hôte 49147, GPU 81915) | G4, sonde FULL |
| K10 | `mesures_g4_ab8_diag1/sessions/claudediag1/k10_leaf{16,24}.json` | G4, sonde FULL |
| LOC | sept appels du CLI `b21-s10` sur `lidar_ng00`, K = 5 (§ 1.5) | local |

Trames : `lidar_ng00/01/02`, séquence 08 sans sol, grille de 1 mm, 39 885, 35 551 et 45 845 sites.

## 1. Faits mesurés

### 1.1 Les étages du CLI, K = 5 (FIN, G4, médianes des trois prises chaudes, ms)

| Trame | Sortie | W | cloud | index | domain | tree | attach | output | write | total |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | full | 1 | 3,3 | 0,4 | 5 327 | 3 765 | — | — | 2 154 | 11 256 |
| ng00 | full | 48 | 3,2 | 0,4 | 222 | 164 | — | — | 2 163 | 2 560 |
| ng00 | supports | 1 | 3,2 | 0,4 | 5 352 | 3 777 | 73 | 190 | 218 | 9 616 |
| ng00 | supports | 48 | 3,2 | 0,4 | 211 | 171 | 75 | 45 | 219 | 736 |
| ng01 | full | 48 | 2,8 | 0,4 | 180 | 139 | — | — | 1 820 | 2 152 |
| ng01 | supports | 48 | 2,9 | 0,3 | 188 | 139 | 61 | 38 | 170 | 601 |
| ng02 | full | 48 | 3,8 | 0,4 | 231 | 151 | — | — | 2 363 | 2 764 |
| ng02 | supports | 48 | 3,8 | 0,4 | 219 | 159 | 80 | 49 | 235 | 774 |

- Prises froides (premier appel de la configuration) : même ordre de grandeur. Par exemple ng00 full W48 : domain
  218, tree 186. L'écart froid/chaud reste dans la dispersion W48 : domain de 175 à 252 ms d'une prise à l'autre.
- Identité : 52 appels concordants. `tree_k_sha256` commun ; manifestes égaux entre prises et entre W.
- Fichiers : `full` 300,9 Mo (ng00), `supports` 30,8 Mo.
- Pics : domain 345 Mo, tree 363 à 408 Mo, output 244 à 303 Mo.

### 1.2 K = 10 (G4)

| Source | Sortie | Feuilles | domain | tree | attach | output | write | total |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| FIN ng00 W48 | full | 16 | 1 740 | 1 623 à 1 655 | — | — | 10 864 à 10 911 | 14 310 à 14 331 |
| FIN ng00 W48 | supports (L2b) | 16 | 1 718 à 1 735 | 1 655 à 1 665 | 241 à 247 | 189 | 746 à 749 | 4 589 à 4 637 |
| QUAL ng00 W48 | supports (`build_order`) | 16 | 1 823 à 1 874 | 921 à 935 | 251 à 253 | 193 à 200 | 746 | 3 998 à 4 030 |
| K10 sonde, trois trames, W48 | FULL | 24 | 773 à 900 | 1 155 à 1 656 | — | — | — | 1 822 à 2 506 (médianes) |
| GPU s6 sonde, trois trames, W48 | FULL | 24 | 653 à 852 (CPU) ; 629 à 739 (GPU) | 1 164 à 1 663 | — | — | — | 1 762 à 2 514 |

- Pics à K = 10 : domain 1,54 Go, tree 2,04 Go, fichier `full` 1,46 Go.
- **Fait majeur à K = 10** : les dumps FULL sont identiques avec des feuilles de 16, 24 et 32 sur les trois trames
  (même `identity` dans GPU s3, s4, s5 et s6). Or le CLI fige `leaf_size = 16` (`src/api/compute.cpp`, l. 26).
  Son étage `domain` K = 10 coûte donc 1,74 s, contre 0,83 à 0,90 s pour la sonde à feuilles de 24.

### 1.3 Sous-étages de la tour FULL, K = 5 (AB, variante `qr` de `claudediag1`, ms)

Médianes indépendantes de cinq prises à W48 (une seule prise à W1) ; les sommes ne redonnent donc pas exactement
les totaux. Sous-étages du domaine : `sp` = passe unique, `prefix`, `sort` = tri des niveaux, `level_scan`,
`assembly`, `compact`. Sous-étages des forêts : `classify`, `births`, `regular` = résolution régulière, `publish`.

| Trame | W | domain | sp | prefix | sort | level_scan | assembly | compact | forêts | classify | births | regular | publish | mur |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 1 | 5 305 | 4 673 | 85,8 | 257,9 | 151,8 | 68,2 | 36,3 | 3 744 | 37,2 | 55,7 | 3 403 | 133,1 | 9 050 |
| ng00 | 48 | 221 | 163 | 20,3 | 11,6 | 4,5 | 4,4 | 4,6 | 141 | 2,6 | 8,3 | 116 | 4,2 | 377 |
| ng01 | 48 | 195 | 144 | 17,1 | 9,6 | 3,7 | 4,1 | 4,2 | 130 | 2,2 | 6,6 | 86 | 25,8 | 324 |
| ng02 | 48 | 239 | 175 | 21,7 | 13,4 | 5,0 | 4,6 | 5,3 | 167 | 2,7 | 11,3 | 97 | 44,2 | 413 |

- `cpu_seconds` à W48 : 10,7 à 13,6 s, contre 7,0 à 9,1 s à W1. Le passage à 48 fils gonfle donc le CPU d'un facteur
  1,5 environ (SMT et attentes).
- Travail de la résolution régulière, ng00, K = 5, ordres 1 à 5 :
  - 4,80 M pas de descente ;
  - 3,62 M succès de la table de populations ;
  - 4,30 M succès du catalogue ;
  - 291 k appels du census, pour 21,1 M tests de points ;
  - 3,79 M présentations de MEB de parties.
- Registre du catalogue, ng00 (GPU s6) : 783 071 nœuds, 353 456 feuilles, 379 M tests de filtre, 120 M préfixes,
  1,31 M boules émises.

### 1.4 La voie GPU des feuilles (GPU, K = 5, feuilles de 16, W48, ms)

| Session | Trame | Voie | Régime | mur | domain | single | exécuteur |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: |
| s6 | ng00 | CPU | froid | 369 | 210 | 154 | — |
| s6 | ng00 | GPU | froid | 435 | 278 | 106 | 98 |
| s6 | ng00 | GPU | chaud (meilleure) | 398 | 246 | 112 | 58 |
| s6 | ng01 | CPU | chaud (meilleure) | 288 | 172 | 122 | — |
| s6 | ng01 | GPU | chaud (meilleure) | 325 | 210 | 95 | 54 |

L'exécuteur se décompose en ouverture du contexte, comptage, écriture, Level, rassemblement et retour. Pour s6 ng00
à froid : 32 + 38 + 18 + 6 + 2 + 2 ms. Toutes les prises sont identiques à la voie CPU, avec zéro feuille
`unresolved`.

- La voie GPU retire les feuilles de la passe unique : `single` passe de 154 à 106 ms. Il reste donc **84 à 106 ms de
  parcours** (filtre G1 des nœuds, file de travail) qui ne bougent pas.
- L'exécuteur coûte 52 à 59 ms à chaud. Les feuilles valaient 40 à 63 ms sur le CPU à 48 fils.
- Nsight :
  - 3,2 à 3,4 fils actifs sur 32 ;
  - pile locale de 3,2 Kio par fil ;
  - ALU à 24 % ;
  - queue des feuilles lourdes : 13 à 20 ms à K = 5, 106 à 117 ms à K = 10 ;
  - contexte : 78 ms ;
  - retour à 4 Go/s, plus de 10 Go/s une fois les pages pré-touchées.

### 1.5 `points` et `plat` : aucune mesure G4, seulement du local (LOC)

Le banc `bench/sorties_g4.py` ne joue que `full` et `supports` (`OUTPUTS`, l. 48). Les reçus de `points` sur G4
(`developpement_20261003/points_g4`) mesurent la chaîne Python de la v11 du 3 octobre, pas le CLI natif.

Mesure locale du 6 octobre, 01 h 22 à 01 h 24 UTC :

- binaire `build/v11-persist/b21-s10/mhgp11`, Release u21, construit le 5 octobre à 19 h 50 depuis
  `build/v11-impl-l3` (état S10) ;
- une prise par configuration, ng00, K = 5 ;
- charge du codespace de 4 à 6, venant d'autres sessions : **les temps W8 sont bruités** ;
- manifestes `full` (`aabd6491…`) et `supports` (`d7c3107b…`) **identiques** à ceux de FIN sur G4, donc mêmes objets.

| Sortie | W | domain | tree | attach | output | write | total | fichier |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| points | 1 | 9 349 | 3 667 | 102 | 554 | 486 | 14 168 | 36,8 Mo |
| points | 8 | 1 718 à 1 991 | 747 à 941 | 103 à 155 | 166 à 210 | 483 à 487 | 3 242 à 3 805 | |
| plat | 1 | 9 390 | 3 629 | 108 | 597 | 84 | 13 820 | 0,32 Mo |
| plat | 8 | 1 763 à 1 925 | 842 à 1 057 | 107 à 109 | 229 à 250 | 80 à 83 | 3 047 à 3 447 | |
| supports | 8 | 1 736 | 1 305 | 104 | 91 | 286 | 3 551 | 30,8 Mo |
| full | 8 | 1 830 | 1 253 | — | — | 1 968 | 5 080 | 300,9 Mo |

Lecture :

- `attach` ne descend pas avec W, ni en local, ni sur G4 (`supports` : 73 ms à W1 contre 75 ms à W48).
- `output` de `points` (`points::hang`) et de `plat` (tête) accélère d'un facteur 2,6 à 3,3 sur 8 fils.
- `write` de `plat` coûte 80 ms pour 0,32 Mo : c'est un coût fixe, pas de l'entrée-sortie proportionnelle.

### 1.6 `build_order`, la voie de `points` et `plat`, sur G4 (QUAL, bras `supports` avant L2b, K = 5, médianes chaudes, ms)

| Trame | tree W1 | tree W48 | accélération | FULL tree W1 | FULL tree W48 | accélération FULL |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | 1 988 | 182 | ×10,9 | 3 774 | 151 | ×25,1 |
| ng01 | 1 410 | 138 | ×10,2 | 2 776 | 117 | ×23,7 |
| ng02 | 1 584 | 184 | ×8,6 | 3 270 | 168 | ×19,5 |

`build_order` fait **deux fois moins de travail** que FULL à W1, mais il est **plus lent à W48**. C'est la voie de
`points` et de `plat` (`docs/SORTIES.md` § 1 : `build_order`, masque 7 035, sans ordres concurrents, sans verticales
parallèles ni réemploi des verticales).

## 2. Loi d'Amdahl

### 2.1 Accélération W1 → W48 et fraction série apparente par étage (FIN et QUAL, G4)

Formule : `f48 = (T48/T1 − 1/48) / (1 − 1/48)`.

Avertissement : 48 vCPU font 24 cœurs. Une accélération de 24 est donc déjà le plafond physique, et `f48` surestime
la vraie part série des étages qui sont au plafond SMT. La fraction n'est discriminante que pour les étages lents.

| Étage | Accélération (ng00 / ng01 / ng02) | f48 | Lecture |
| --- | --- | --- | --- |
| domain (catalogue) | ×24,0 / ×23,8 / ×22,2 | 2,1 à 2,5 % | au plafond des 24 cœurs : seul son **travail** compte |
| tree FULL (L2b) | ×23,0 / ×20,1 / ×21,5 | 2,3 à 3,0 % | idem, plus une queue de publication variable (4 à 51 ms) |
| tree `build_order` (points, plat) | ×10,9 / ×10,2 / ×8,6 | 7,2 à 9,7 % | **110 à 154 ms d'équivalent série** : tout son temps à W48 |
| attach | ×1,0 | ≈ 100 % | série (balayage du lemme D) : 58 à 80 ms à K = 5, 241 à 253 ms à K = 10 |
| output `supports` | ×3,9 à ×4,2 | 22 à 24 % | parallèle en partie |
| write `full` | ×1,0 | 100 % | série : 1,82 à 2,36 s à K = 5, 10,9 s à K = 10 |
| write `supports` | ×1,0 | 100 % | série : 170 à 235 ms |

Sous-étages de la tour FULL (AB, ng00, accélération W1 → W48) :

| Sous-étage | Accélération | Lecture |
| --- | ---: | --- |
| passe unique | ×28,7 | parallèle |
| `regular` | ×29,3 | parallèle |
| `level_scan` | ×34 | parallèle |
| `publish` | ×32, mais 4 à 51 ms à W48 | queue variable |
| `sort` | ×22 | parallèle |
| `assembly` | ×15 | parallélisme moyen |
| `classify` | ×14 | parallélisme moyen |
| `compact` | ×7,9 | **parallélisme faible** |
| `births` | ×6,7 | **parallélisme faible** |
| `prefix` | ×4,2 | **parallélisme faible** |

### 2.2 Où part le temps de FULL K = 5 (AB, ng00)

| Poste | W1 (ms) | part W1 | W48 (ms) | part du mur W48 |
| --- | ---: | ---: | ---: | ---: |
| passe unique du catalogue | 4 673 | 51,6 % | 163 | 43 % |
| résolution régulière | 3 403 | 37,6 % | 116 | 31 % |
| petits étages (`sort`, `level_scan`, `publish`, `prefix`, `assembly`, `births`, `classify`, `compact`) et résidus non attribués | 974 | 10,8 % | 98 | 26 % |
| mur | 9 050 | | 377 | |

**Les deux gros postes font 89 % du travail mais seulement 74 % du mur W48.** Les petits étages font 11 % du travail
mais 26 % du mur, parce qu'ils parallélisent mal. Le même calcul donne 94 ms de reste sur ng01 et 141 ms sur ng02.

### 2.3 Ce qu'un GPU peut au mieux retirer pour atteindre 100 ms

Le contrat est la tour FULL K = 5, soit `domain` + `tree` (`docs/CONCEPTION_MOTEUR.md`), `write` exclu.

- **Mesure G4** : 324 à 413 ms selon la trame (sonde AB, médianes W48). CLI FIN : 320 à 385 ms. Meilleure passe chaude de la
  sonde (GPU s5 et s6) : 275 à 374 ms.
- **Borne d'Amdahl** (calcul sur mesures, AB) : même si un GPU faisait la passe unique **et** la résolution
  régulière en un temps nul, sans transfert ni contexte, il resterait **94 à 141 ms** de petits étages CPU (§ 2.2).
  Le GPU peut retirer au mieux 230 à 279 ms (163 + 116 sur ng00, 144 + 86 sur ng01, 175 + 97 sur ng02). Ce n'est
  **pas suffisant seul**.
- **Condition nécessaire** (estimation) :
  1. ramener les petits étages et les résidus de 94 à 141 ms à environ 40 ms : `prefix` ×4,2, `births` ×6,7,
     `compact` ×7,9, queue `publish` de 4 à 51 ms, environ 22 ms de résidus non chronométrés ;
  2. **et** faire tenir passe unique + résolution régulière dans environ 50 ms sur GPU, transferts compris ;
  3. le contexte de 78 ms doit être ouvert hors de l'appel (Session persistante) : il est plus long que tout le
     budget restant.
- Rapport demandé au GPU (estimation) : les 230 à 279 ms CPU de la passe unique et de la résolution régulière à W48
  doivent tomber à environ 50 ms, soit un facteur 4,6 à 5,6 sur 48 fils SMT. La voie actuelle « un fil par feuille »
  est à parité avec le CPU sur la seule partie feuilles (§ 1.4) : il manque environ ×5 d'efficacité, et il faut
  aussi couvrir le parcours (84 à 106 ms) et la résolution régulière.
- À K = 10 : `domain` 0,63 à 0,85 s (meilleure voie), forêts 1,15 à 1,66 s. Le facteur à gagner est de 18 à 25.
  Hors de portée de toute piste de cette carte. Les forêts dominent.

### 2.4 Le mur réel du CLI, hors contrat

Pour `--sortie=full`, à W48, `write` vaut **84 %** du mur (2,16 s sur 2,56 s pour ng00 ; 10,9 s sur 14,3 s à
K = 10), et il est strictement série. L'écrivain (`src/api/write_full.cpp`, `src/io/writer.cpp`,
`src/io/sha256.cpp`) enchaîne, sur un seul fil :

- le calcul exact de la sphère de naissance de chaque nœud né (897 776 à K = 5) ;
- la conversion petit-boutiste octet par octet ;
- un SHA-256 scalaire portable ;
- `fwrite`, puis `fsync`.

Les deux processeurs exposent `sha_ni`, vérifié sur le codespace par `/proc/cpuinfo`. Sur la VM, on le suppose pour
un EPYC 9B45 : **à vérifier**.

Le GPU n'y peut rien : le hachage est séquentiel, et c'est de l'entrée-sortie. Mais c'est le premier poste du mur
pour cette sortie.

## 3. Étages sans mesure G4 et protocole proposé

| Étage | Sorties | État | Protocole G4 |
| --- | --- | --- | --- |
| tree, attach, output, write de `points` | points | local seulement | `sorties_g4.py` étendu (P1) |
| tree, attach, output, write de `plat` | plat | local seulement | idem |
| sous-étages de `write` (sphères, encodage, SHA, `fwrite`, `fsync`) | toutes | jamais séparés | diagnostic jetable hors dépôt, ou compteurs de diagnostic séparés de la ligne d'état |
| phases de `build_order` (naissances, régulière, publication de l'ordre K, plateaux) | points, plat, supports avant L2b | jamais exportées par le CLI | sonde `order_tree` avec `phases` comme la sonde FULL |
| part feuilles/parcours de la passe unique à K = 10 | full | partielle (GPU s3 à s6) | `gpu_ab.py`, déjà prêt |
| `points` et `plat` à K = 10 | points, plat | aucune | P1 avec `--k10` |

**P1, mesure appariée des quatre sorties** (à écrire, sans toucher au moteur) :

1. Ajouter `--sorties=full,supports,points,plat` à `bench/sorties_g4.py` (`OUTPUTS` paramétré). Pour `points`, le
   fichier est `points.mhgp11pt` ; pour `plat`, `etiquettes.mhgp11et`.
2. Configuration : ng00, ng01, ng02, K = 5, W1 et W48, une passe froide puis trois prises chaudes par configuration,
   sorties alternées dans un ordre de Williams. K = 10 une fois sur ng00, à W48.
3. Identité exigée entre prises et entre W : fichier, manifeste et `tree_k_sha256`. Le `tree_k_sha256` doit être
   commun aux quatre sorties : il l'est déjà sur `full`, `supports` et `plat`, valeur `a4425cb5…` pour ng00, dans FIN
   et dans `build/v11-persist/s10-work`.
4. Lecture : médiane par étage. La fraction série par étage se calcule comme au § 2.1. `attach` et `write` sont
   rapportés en absolu.
5. Session : une session gardée `gcp-migration/v11_session.py` d'environ 15 min (FIN : 52 appels en 312 s, donc
   environ 10 min pour 104 appels), `--max-run-seconds 4200`, puis certifier `TERMINATED`.
6. Ce banc ne décide rien : il est descriptif, comme FIN.

**P2, découpe de `write`** : instrumentation jetable de `write_full` (horloges autour de la sphère, de l'encodage, du
SHA et de `fsync`), avec le même fichier exigé (sha256 brut égal au dump). Une prise par trame à W1 suffit, puisque
`write` est série.

**P3, phases de `build_order`** : rejouer QUAL avec l'export des phases de `order_tree` (diagnostic). L'objectif est
d'attribuer les 110 à 154 ms d'équivalent série avant de choisir entre une voie pipeline à un ordre (S11, L5) et
`build_order_full`.

## 4. Opportunités chiffrées

Chaque gain est une **estimation**, sauf mention contraire. L'exactitude reste celle du contrat R7 : sur GPU, un
résultat exact ou `unresolved` rejoué sur CPU, avec des sorties identiques à l'octet.

1. **Passe unique du catalogue entière par sous-arbre sur GPU** (parcours G1 et feuilles J3 coopératives par warp).
   - Coût actuel : 144 à 175 ms à W48 et 4,4 à 4,7 s à W1 (AB) ; parcours de 84 à 106 ms et feuilles de 40 à 63 ms
     (GPU s6).
   - Gain : borne de 144 à 175 ms. Gain réaliste inconnu : il faudrait environ ×5 sur la voie actuelle (§ 2.3).
2. **Résolution régulière sur GPU** (table de populations en mémoire du device, MEB de parties i128 certifiées).
   - Coût actuel : 86 à 116 ms à W48, 2,5 à 3,4 s à W1.
   - Gain : borne de 86 à 116 ms. Le risque est élevé (DSU, ordre canonique, compteurs du census).
3. **Petits étages de la tour sur CPU** (`prefix`, `births`, `compact`, queue `publish`, résidus).
   - Coût actuel : 94 à 141 ms à W48.
   - Gain : 40 à 90 ms. C'est une **condition nécessaire** du contrat, avant tout GPU.
4. **Feuilles de 24 à K = 10 dans le CLI**.
   - Coût actuel : `domain` à 1,74 s (FIN).
   - Gain : environ 0,85 s, **mesuré sur la sonde** (0,83 à 0,90 s), avec des dumps identiques.
5. **`points` et `plat` vers une voie qui parallélise** : `build_order_full` (comme L2b), ou pipeline à un ordre (S11).
   - Coût actuel : tree de 138 à 184 ms à W48 (QUAL), dont 110 à 154 ms d'équivalent série.
   - Gain : 0 à 25 ms par `build_order_full` (mesuré sur `supports` L2b) ; davantage avec S11 si la partie série est
     éliminée.
6. **`write` de `full`** : sphères en parallèle, encodage par blocs, SHA-NI.
   - Coût actuel : 1,82 à 2,36 s à K = 5 et 10,9 s à K = 10, série.
   - Gain : 1,5 à 2 s à K = 5. Hors contrat, mais c'est le premier poste du mur CLI.
7. **`attach` en parallèle** (balayage du lemme D par sous-arbres).
   - Coût actuel : 58 à 80 ms à K = 5, 241 à 253 ms à K = 10, série.
   - Gain : 40 à 70 ms à K = 5, pour `supports`, `points` et `plat`.

## 5. Pistes écartées

- **Feuilles seules sur GPU à K = 5** : mesurées perdantes de 30 à 50 ms (GPU s3 à s6). Le parcours reste sur le CPU,
  et l'exécuteur coûte autant que les feuilles retirées.
- **Lot hôte (masque 49147)** : toujours plus lent que la voie CPU, de 100 à 170 ms à K = 5 (GPU s4 et s5).
- **GPU pour `write`** : SHA-256 séquentiel, `fsync`. La voie est CPU (SHA-NI, encodage parallèle).
- **GPU pour `cloud` et `index`** : 3,6 ms en tout.
- **GPU pour `attach`, `output` de `supports` et tête de `plat`** : quelques dizaines de ms, avec une structure de
  balayage ordonné. La parallélisation CPU passe avant, et `points` comme `plat` sont à mesurer d'abord (P1).
- **Tri des niveaux sur GPU** : 11,6 à 13,4 ms à W48, déjà ×22.
- **Changer d'ISA** (`-march=x86-64-v3/v4`) : aucun gain mesuré le 3 octobre.

## 6. Questions ouvertes

1. Le contrat de 100 ms porte-t-il sur `domain + tree` d'un appel isolé, contexte CUDA compris (78 ms), ou sur une
   Session persistante ?
2. Que coûtent les résidus non chronométrés de la tour (environ 22 ms à W48 sur ng00) ?
3. Pourquoi la queue `publish` varie-t-elle de 4 à 51 ms à W48 d'une prise à l'autre ?
4. Qu'est-ce qui est série dans `build_order` à W48 : la publication de l'ordre K, les naissances, autre chose (P3) ?
5. Changer de feuille à K = 10 change-t-il le registre du catalogue publié, ou seulement des compteurs internes ? La
   règle 6 d'`ARCHITECTURE.md` interdit une option de moteur : faut-il qualifier un paramètre fixe par K ?
6. La VM expose-t-elle `sha_ni`, et quel est le coût de `fsync` sur son disque ?
7. Que valent `points::hang` et la tête plate sur G4, à W1 et à W48 (P1) ?
