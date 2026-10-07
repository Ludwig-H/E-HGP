# MES-M5 : rapport local du 7 octobre 2026

Rédigé entre 11 h 41 et 12 h 47 UTC (heures lues par `date -u`), sur le codespace (8 cœurs partagés, aucun GPU,
GCC 13.3, GCC 11.5 et Clang 18, nvcc 12.9, CMake 3.28 et 3.22.1, Python 3.12). **Aucune décision ici** : l'identité sur l'hôte, la
compilation pour l'appareil, l'essai à blanc du script et la prédiction ; le verdict appartient à la session G4.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (warp simulé) ; cuda_g4 à venir
quantification=quantized_u21_input_only (fixtures u32 synthétiques)
public_status=not_claimed
GCP non utilisé
```

## 1. Sources et constructions

| Élément | Valeur |
| --- | --- |
| v11 gelée | archive `v11_src_ac081a06f.tar.gz`, SHA-256 `6f3454ad9d9ad2f6…` vérifié par `microbancs/outils/source_v11.py` (644 fichiers) ; sources du dépôt identiques au gel (`git diff ac081a06f HEAD -- morsehgp3D_v11/src` vide) |
| `libmhgp11.a` (Release, u21, GCC 13.3) | `050532a95c322cc2…`, identique à celle des microbancs `MES-M2` et `MES-M3`/`M4` |
| outils hôte | `-Wall -Wextra -Wpedantic -Werror` sans avertissement avec GCC 13.3, Clang 18 et GCC 11.5 (paquets Ubuntu extraits dans le bloc-notes, sans installation ; la VM a GCC 11.4) ; mêmes identités et mêmes empreintes |
| banc CUDA | nvcc 12.9, sm_120, sans avertissement, hôte GCC 13.3 et GCC 11.5 ; CMake 3.22.1 (celui de la VM) configure et construit les quatre cibles |

Ressources ptxas des noyaux sans mutant (4 warps par bloc, aucun débordement de registres) :

| Noyau | Registres | Pile (o) | Partagée par bloc (o) |
| --- | ---: | ---: | ---: |
| `Select` | 72 | 48 | 13 312 |
| `Merge` | 81 | 48 | 13 312 |
| `Filter` | 76 | 48 | 9 216 |
| `Close` | 48 | 120 | 0 |
| `ScanA` / `ScanB` / `ScanC` | 56 / 40 / 40 | 0 | 0 |
| `Scatter` / `Emit` / `Iota` | 34 / 34 / 36 | 0 / 16 / 0 | 0 |

## 2. Vidages du parcours de la v11 (`MHGP12TR` v1)

Trames sans sol de `build/v11-full-data-20261002/` et nuages uniformes de la même source ; le grand livre de la
capture égale celui du `walk` gelé, et les feuilles qu'il met en file égalent les feuilles capturées, sur tous les cas
(sinon code 3). Comptes et premiers caractères du SHA-256 des fichiers (vidages hors dépôt) :

| Cas | Sites | Nœuds | Feuilles | Tests G1 | Prof. max | Sites des feuilles | Candidats (Σ listes parentes) | Mo | SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| ng00_k5_l16 | 39 885 | 783 071 | 353 456 | 379 366 675 | 36 | 4 928 075 | 30 629 245 | 123,6 | `ec8850381e37009b…` |
| ng01_k5_l16 | 35 551 | 637 505 | 284 835 | 308 648 888 | 36 | 3 968 155 | 24 886 763 | 100,3 | `051200c33ac10f0b…` |
| ng02_k5_l16 | 45 845 | 735 601 | 323 879 | 379 899 949 | 36 | 4 519 093 | 30 793 485 | 115,2 | `37df40a201e7231e…` |
| ng00_k5_l24 | 39 885 | 272 249 | 123 581 | 248 216 364 | 33 | 2 419 980 | 20 757 557 | 46,2 | `a5fc6511c7df0155…` |
| ng01_k5_l24 | 35 551 | 222 371 | 99 768 | 201 986 198 | 32 | 1 952 848 | 16 860 051 | 37,6 | `cbc27eeef54aff0c…` |
| ng02_k5_l24 | 45 845 | 262 845 | 115 657 | 258 451 105 | 33 | 2 263 204 | 21 647 021 | 44,1 | `f4f8da150f23584d…` |
| ng00_k10_l24 | 39 885 | 1 137 395 | 530 259 | 1 229 010 736 | 38 | 11 455 500 | 49 143 039 | 197,9 | `ce598217d42a61b4…` |
| ng01_k10_l24 | 35 551 | 932 305 | 430 579 | 1 006 025 554 | 36 | 9 295 715 | 40 180 139 | 161,6 | `918cad8ca13b3792…` |
| ng02_k10_l24 | 45 845 | 1 066 917 | 486 530 | 1 207 405 458 | 37 | 10 502 291 | 48 399 773 | 183,9 | `702a9b2fa59d6e36…` |
| uniform_u18_n8000_k5_l24 | 8 000 | 46 991 | 23 496 | 36 792 201 | 19 | 475 870 | 2 986 968 | 8,4 | `4e8fbff361b09131…` |
| uniform_u18_n16000_k5_l24 | 16 000 | 96 293 | 48 147 | 79 816 685 | 20 | 992 024 | 6 516 846 | 17,3 | `82a9185db6e7e0e7…` |
| uniform_u18_n32000_k5_l24 | 32 000 | 208 623 | 104 312 | 176 004 487 | 21 | 2 130 416 | 14 444 120 | 37,3 | `ea1d15f1df25add0…` |

Recoupements : feuilles 353 456 / 123 581 / 530 259 et tests 379,4 M à K5/16 sur ng00, comme le rapport C de l'audit
géant ; le chrono v11 du § 5 reconstruit le catalogue entier et retrouve 272 249 nœuds, 248 216 364 tests et
1 306 696 boules (`MESURE.md` § 3.3). Les vidages refaits par l'essai à blanc du § 7 sont identiques à l'octet.

**Forme du travail par niveau** (ng00 K5/24, statistiques du parcours) : 34 niveaux ; la racine et les six niveaux
suivants gardent des listes de 7 000 à 39 885 candidats par enfant (156 à 1 792 tâches par niveau) ; le niveau le plus
large (profondeur 21 à 23) a 27 000 à 32 000 enfants de 39 à 51 candidats et 1,41 M de candidats au total ; les dix
derniers niveaux s'amenuisent jusqu'à 2 enfants. À K10/24 : 39 niveaux, jusqu'à 134 952 enfants et 4,2 M de candidats
par niveau. Sur les nuages uniformes, 20 à 22 niveaux.

## 3. Identité sur l'hôte (warp simulé)

`mhgp12_traversal_identity --mutants all --nodes --unit` : **identité exacte sur les 12 cas et les 6 fixtures**,
statut, grand livre, ensemble des feuilles (boîtes, listes, profondeurs, chemins) **et tous les nœuds visités** (boîte
d'entrée, chemin, candidats, retenus, tests G1, genre, empreinte de la liste), en ordre préfixe. Empreinte canonique
des feuilles (FNV-1a 64, la même pour la v11 et le parcours en largeur) ; temps de l'hôte à un fil, simulé, sous
charge de 4 fils (indicatif), et `walk` séquentiel de la v11 à un fil :

| Cas | Identité | Nœuds | Niveaux | Empreinte des feuilles | Hôte simulé (s) | `walk` v11 (s) |
| --- | --- | --- | ---: | --- | ---: | ---: |
| ng00_k5_l16 | oui | oui | 37 | `3b2b15f84cf52f4f` | 9,07 | 1,45 |
| ng01_k5_l16 | oui | oui | 37 | `bb60d3e2d306e8fc` | 5,88 | 1,18 |
| ng02_k5_l16 | oui | oui | 37 | `b7ca5c5d8766ddf1` | 7,38 | 1,43 |
| ng00_k5_l24 | oui | oui | 34 | `46b66462d990a032` | 5,17 | 0,80 |
| ng01_k5_l24 | oui | oui | 33 | `97bac108d47c673c` | 4,31 | 0,66 |
| ng02_k5_l24 | oui | oui | 34 | `82501e770705c9b3` | 5,14 | 0,82 |
| ng00_k10_l24 | oui | oui | 39 | `f94c00dac13ee89a` | 12,30 | 3,63 |
| ng01_k10_l24 | oui | oui | 37 | `287740008a7ecba6` | 10,31 | 2,84 |
| ng02_k10_l24 | oui | oui | 38 | `e280a7722ed7133f` | 9,47 | 3,31 |
| uniform_u18_n8000_k5_l24 | oui | oui | 20 | `e09af4b2b6456951` | 0,41 | 0,13 |
| uniform_u18_n16000_k5_l24 | oui | oui | 21 | `39cd86e549b862f4` | 0,95 | 0,30 |
| uniform_u18_n32000_k5_l24 | oui | oui | 22 | `5cb5cc8990dbf8a9` | 2,08 | 0,82 |
| coquille24_k2_l16 (u21) | oui | oui | 61 | `5e48473bd48a4ccb` | 0,00 | — |
| coquille48_k5_l24 (u21) | oui | oui | 64 | `54e8171270928c44` | 0,01 | — |
| coquille48_k5_l8_m8 (u21) | oui (refus `wide_leaf`) | — | 61 | vide | 0,04 | — |
| coquille48_u32_k5_l24 | oui | oui | 97 | `c689132afda0789a` | 0,01 | — |
| bord_u32_k2_l5 | oui | oui | 4 | `76472b6c90f7e737` | 0,00 | — |
| uniforme_u32_k3_l8 | oui | oui | 26 | `575caf7aa8d9be5f` | 0,05 | — |

La découpe de 4 000 sites de ng00 à K5/24 (cible de Compute Sanitizer sur G4 : 25 943 nœuds, 11 602 feuilles,
29 niveaux, SHA-256 `c3f236ac226dff2c…`) est identique aussi, nœuds compris.

La simulation hôte est 2,6 à 6,5 fois plus lente que le `walk` de la v11 à un fil (32 voies jouées l'une après
l'autre, tri bitonique et fusion par rangs au lieu d'un tri par insertion) : elle sert la preuve, pas la mesure.

**Portes unitaires** : repère fermé de 33 bits pour $(0,0,0)$ et $(2^{32}-1,0,0)$ (`CST-0204`) ; clé du réservoir
$3(2^{31}-3)^{2}$ au-delà d'`i64` pour la boîte $[0,1]^{3}$ et le site $(2^{30}-1)^{3}$, repère du parent à 30 bits
donc voie large, alors que la boîte seule en demande 1 (`CST-0208`) ; borne $3B$ (`CST-0205`) : conformes.

## 4. Mutants, fixtures, oracle

Un mutant est tué si le statut, le grand livre ou l'ensemble des feuilles diffère (« feuilles » : l'ensemble des
feuilles lui-même diffère ; « grand livre » : seuls les comptes, ici les tests G1) :

| Mutant | 9 trames LiDAR | 3 nuages uniformes | 6 fixtures |
| --- | --- | --- | --- |
| `temoin_perdu` | 9 tués (feuilles) | 3 tués (feuilles) | 5 tués |
| `repere_enfant` | survit (équivalent : voie native partout en u21) | survit (idem) | **tué par les 2 fixtures u32 de la coquille et uniforme** (feuilles) |
| `compactage_instable` | 9 tués (feuilles) | 3 tués (feuilles) | 5 tués |
| `bissection_decalee` | 9 tués (feuilles) | 3 tués (feuilles) | 4 tués |
| `ex_aequo_inverses` | 9 tués (6 feuilles, 3 grand livre) | 1 tué (grand livre) | 3 tués |
| `axe_dernier_maximum` | 9 tués (feuilles) | 3 tués (feuilles) | 3 tués |

Les ex æquo de clé au seuil du réservoir existent donc sur les trames et changent l'ensemble des feuilles sur six cas
sur neuf : le départage par le rang dans la liste parente fait partie de l'objet transmis aux feuilles.

**Oracle et fixtures** (`fixtures.py`, 13 s) : l'oracle Python reproduit la v11 nœud par nœud sur les trois fixtures
u21 (profondeurs 60 et 63, refus `wide_leaf` sans préfixe publié) avant de servir de référence aux fixtures u32. La
coquille u32 atteint la profondeur **96 = 3B** (1 479 nœuds, 504 feuilles) : la borne de `CST-0205` est atteinte aux
profils 21 (63) et 32 (96). `bord_u32_k2_l5` : boîte racine $[0,2^{32})$, repère fermé de 33 bits. `uniforme_u32_k3_l8`
: 15 507 nœuds, voies large et native mêlées.

**Élagage de `Merge`** (mesuré par une construction instrumentée jetable) : en rangeant chaque paquet de 32 tâches par
sa plus petite clé et en s'arrêtant au premier dépassement du seuil courant, `Merge` ne fusionne plus que 12 659 des
37 514 sommets locaux à ng00 K5/24 (34 %), contre 87 % avec un simple filtre par paquet ; identité inchangée partout.

## 5. Chronos locaux indicatifs (aucune décision)

Aucun temps GPU local. Frontière + passe unique de la v11 sur ce codespace (`mhgp12_v11_traversal_timing`, 4 fils,
seconde passe d'un processus, machine partagée et chargée par d'autres tâches) lors des deux essais à blanc du § 7 :
l'écart entre essais (jusqu'à ×2) dit assez que ces chiffres ne prédisent pas G4, où la même étape vaut 58 ms à ng00
K5/24 sur 48 fils (`MESURE.md` § 3.2).

| Cas | Essai 2 (ms) | Essai 3 (ms) |
| --- | ---: | ---: |
| ng00 / ng01 / ng02 K5/16 | 505 / 304 / 590 | 669 / 531 / 532 |
| ng00 / ng01 / ng02 K5/24 | 217 / 184 / 347 | 450 / 312 / 301 |
| ng00 / ng01 / ng02 K10/24 | 854 / 690 / 1 191 | 1 886 / 1 089 / 1 562 |
| uniformes 8 000 / 16 000 / 32 000, K5/24 | 48 / 98 / 172 | 70 / 136 / 308 |

Un processus isolé sur ng00 K5/24 (4 fils, quatre passes) : frontière 69 à 87 ms, passe unique 256 à 271 ms. `walk`
séquentiel à un fil : 0,13 à 3,63 s selon le cas (table du § 3).

## 6. Prédiction, écrite avant G4

Modèle (statistiques par niveau du § 2 ; coûts de Session de `MES-M6` ; deux scénarios) : par niveau, neuf noyaux à
3 à 5 µs de plancher, un aller-retour des totaux de 10 à 20 µs, et le travail des noyaux à la concurrence de
188 SM × 24 warps, chaque paquet de 32 candidats coûtant une chaîne de chargements dépendants de 0,6 à 1,5 µs plus
25 instructions par test G1 ; `Merge` sur le chemin critique des enfants à plusieurs tâches ; rapatriement des
feuilles à 22 à 45 Go/s. v11 : frontière 20 à 26 ms plus 7,17 ns de CPU par test G1 répartis sur 48 fils (rapport C :
1,78 s pour 248 M tests, 37,5 ms mesurés).

| Cas | GPU total prévu | dont noyaux | dont rapatriement | v11 prévu | Rapport prévu |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 / ng01 / ng02 K5/24 (décident) | 3,0 à 6,2 ms | 1,4 à 3,1 ms | 0,4 à 0,9 ms | 50 à 65 ms | **0,06 à 0,10** |
| ng00 / ng01 / ng02 K10/24 (décident) | 5,9 à 12,9 ms | 2,9 à 6,3 ms | 1,6 à 4,0 ms | 170 à 210 ms | **0,03 à 0,06** |
| ng00 / ng01 / ng02 K5/16 (publiés) | 4,3 à 9,2 ms | 2,1 à 4,6 ms | 0,9 à 2,2 ms | 66 à 83 ms | 0,06 à 0,11 |
| uniformes 8 000 / 16 000 / 32 000, K5/24 (publiés) | 1,5 à 4,3 ms | 0,7 à 2,1 ms | 0,1 à 0,8 ms | 25 à 52 ms | 0,05 à 0,09 |

Prédiction : **verdict « adopté »**, toutes les bornes hautes sous 0,15, avec une probabilité que j'estime à 3 sur 4 ;
le temps GPU domine par les planchers de lancement et les allers-retours (34 à 39 niveaux, environ 1 à 2 ms par
parcours) plutôt que par les tests G1, et K10 coûte environ deux fois K5 quand la v11 coûte trois à quatre fois plus.
Risques du verdict, par ordre : (1) un défaut propre à l'appareil (collectives ajoutées, mémoire partagée) rendrait
« rejeté » ou un sanitizer « refusé » : le code n'a jamais tourné sur un GPU ; (2) les premiers niveaux (un warp par
enfant dans `Merge`, 156 à 1 800 tâches) et les allers-retours pèseraient plus que prévu, sans menacer le seuil
d'un quart (il faudrait plus de 14 ms à K5/24) ; (3) la v11 de la session serait plus rapide que les 58 ms publiés.

## 7. Essai à blanc du script G4 (sans GPU)

`python3 -S -O scripts/g4_traversal_bench.py --out … --work … --data … --repo … --v11-lib … --processes 1
--v11-workers 4 --v11-passes 2 --jobs 4 --no-cuda`, joué trois fois (le dernier sur les sources livrées, de 12 h 36 à
12 h 44) : configuration et construction (14 s), 12 vidages et la découpe de 4 000 sites de ng00, fixtures (13 s),
identité hôte de 13 vidages et 6 fixtures avec tous les mutants et les nœuds (143 s, 4 fils), deux tours de chronos
v11, juge. Durée totale 6 min 09 s puis 7 min 52 s (machine chargée). Résultats :

- tous les contrôles de l'hôte conformes : portes unitaires, fixtures et contrôles de l'oracle, identité sur les 12
  cas et les 6 fixtures, mutants tués comme au § 4, grand livre du chrono v11 égal à celui du vidage sur les 12 cas ;
- vidages et fixtures refaits identiques à l'octet à ceux du § 2 (déterminisme) ;
- verdict **refusé**, pour les seules raisons attendues sans GPU : Compute Sanitizer non joué, isolation du GPU non
  certifiée, `repere_enfant` non tué sur l'appareil, identité de l'appareil et durées GPU absentes ;
- `--out` : 652 Ko, aucun vidage ; `--work` : 1,1 Go ; un `--work` placé dans `--out` est refusé (code 2).

**Auto-test du juge** (`--selftest-judge`, `python3 -S -O`) : seize injections, toutes au verdict attendu — dont les
trois faux verdicts relevés par `CST-0018` sur `MES-M2` (identité hôte en défaut, résultat périmé, aucun cas qui
décide), une durée NaN ou nulle, un tour manquant, une réservation pendant le chrono, un mutant jamais tué, un
sanitizer en échec, l'isolation non certifiée, une identité absente, une borne haute au-dessus du seuil, et un cas
publié au-dessus du seuil qui ne change pas le verdict.

## 8. Ce qui reste

- **Avant l'adoption** : la session G4 (§ 7 du README), qui seule juge l'identité sur l'appareil, Compute Sanitizer et
  les temps.
- **Avant le port en T1** : voir le README, § 9.
