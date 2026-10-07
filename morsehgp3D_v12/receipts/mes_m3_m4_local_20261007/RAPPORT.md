# Rapport : microbancs de la tour v12 (MES-M3, MES-M4) et vidage de la v11

7 octobre 2026, passage local sur le codespace (heure de fin de rédaction lue par `date -u` : dernière ligne). Travail
d'un agent développeur de la v12, dans `v12_tour/` (scratchpad) ; le développeur principal intègre et committe.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
quantification=quantized_u21_input_only (construction v11 liée : MHGP11_COORD_BITS=21, moteur ac081a06f,
                libmhgp11.a sha256 050532a95c322cc2…)
public_status=not_claimed
GCP non utilisé (aucune commande gcloud ni session G4)
```

Rien n'a été écrit dans le dépôt (ni `build/`) ; aucune commande git mutante. Les vidages (de 0,17 à 2,0 Go par cas,
7,8 Go au total) dérivent de SemanticKITTI : ils restent dans `v12_tour/out/`, jamais dans le dépôt.

## 0. L'essentiel

1. **Vidage de la v11 gelée** (`mhgp12_vidage`, lié à `libmhgp11.a`) sur ng00, ng01, ng02 à K5 et à K10, et sur les
   uniformes 8 000, 16 000 et 32 000 à K5 (neuf cas). Contrôles, tous verts :
   - le vidage `MHGP11FUL1` réécrit par l'outil est **identique à l'octet** aux neuf empreintes de `MESURE.md` § 4
     (le domaine et les forêts vidés sont donc ceux de la référence) ;
   - les **graines rejouées** (même suite d'appels que `PopulationLookup::descend_each_step`) sont **identiques au
     journal de graines de la v11** (`ForestBuilder::seed_log`) sur tous les ordres des neuf cas, trace par trace ;
   - la forêt de la voie série de la v11 est identique à la forêt publiée (voie des ordres concurrents) ;
   - les naissances de la forêt sont exactement les boules de genre « naissance » de la classification v11.
2. **MES-M3** (plus petite boule proposée puis certifiée) : sur **29 878 990 parties de descente** (toutes celles où
   la v11 calcule une plus petite boule, neuf cas), **identité** de la sphère (centre et niveau exacts), du support local
   canonique et de l'identification au catalogue avec `bounded_meb`, sans exception ; **aucun repli exact** ;
   `LEM-T1` corrigé certifie sans arithmétique **toutes** les parties qui y sont éligibles : 75,2 % à 80,0 % des parties
   sur les trames LiDAR (K5 et K10), 85,7 % sur les uniformes ; le reste est certifié en exact puis recensé, comme la
   v11 le fait. Temps de la plus petite boule, tous ordres : ×0,77 à ×0,89 à K5, **×0,26 à ×0,28 à K10**.
3. **Constat `CST-0101` (`LEM-T1` exige S ⊆ F) intégré dès l'écriture** : le test précède la table, un échec renvoie au
   repli exact ; témoin `WIT-T1-CARRE` gravé ; mutant « sans S ⊆ F » **tué par la porte** et, avec une proposition
   « mère » (S* de la boule du pas précédent), **tué sur données réelles** : sur ng00 K5, 1 024 098 parties recevraient
   une boule fausse sans ce test. Avec la proposition de Welzl, tirée de F, le test n'est jamais décisif (§ 4.1).
   Nouveaux chiffres « avec le test complet » : 75,19 % (ng00 K5) à 79,97 % (ng02 K5) ; ils ne diffèrent du chiffre
   « sphère au catalogue » (le 76 % de la v10, calculé sans le test) que de 3 à 39 parties par cas.
4. **Règle d'adoption de MES-M3** (CPU de résolution à K10 réduit d'au moins 40 % à un fil, résultats identiques) :
   la réplique de la descente v11 avec la plus petite boule certifiée coûte **×0,483 (ng00), ×0,512 (ng01), ×0,567
   (ng02)** de la même réplique avec la plus petite boule exhaustive, soit **−52 %, −49 %, −43 %**, et ×0,40 à ×0,45 du
   code v11 lui-même ; graines identiques trace par trace sur les trois bras. À K5 : ×0,83 à ×0,92. **Local, à
   confirmer sur G4** (règle tenue localement sur les trois trames, de peu sur ng02).
5. **MES-M4** (forêt sans lots) : forêts **identiques** à celles de la v11 sur tous les ordres des neuf cas (nœuds,
   parents, rangs, clés de naissance, enfants et décalages, racine) ; naissances canoniques et graines identiques ;
   **`LEM-T6` sans écart** sur 17 497 207 naissances ; porte (témoins de plateau, borne de `CST-0212`, 6 000
   hypergraphes à rangs répétés contre un Kruskal par lots) verte ; mutant « sans contraction » tué par la porte et sur
   données réelles. Temps locaux à un fil (dernier passage, charge 5 à 9) : noyau 9,7 à 13,1 ms à l'ordre 5 (K5),
   25,8 à 34,0 ms à l'ordre 10 (K10) ; contraction séquentielle du même ordre que le noyau ; contraction parallèle à
   3 fils identique à l'octet, ×0,7 à ×0,85 de la séquentielle aux ordres hauts. Règles (noyau ≤ 10 ms à K5, ≤ 35 ms à
   K10, contraction ≤ 3 ms) : **à juger sur G4**.
6. **Constat `CST-0212`** (opérande sur 32 bits dont le bit haut porte le genre) : domaine exact écrit et gardé, au plus
   2^31 − 1 naissances par ordre (donc au plus 2^31 − 2 événements et 2^32 − 3 nœuds), refus explicite avant toute
   allocation, porte de refus à la borne ; codage inchangé, sans coût (§ 4.2).

## 1. Livrables (`v12_tour/`)

| Fichier | Rôle |
| --- | --- |
| `README.md` | conception, formats binaires (version 1), usage, ce qui se joue sur G4 |
| `RAPPORT.md` | ce rapport |
| `CMakeLists.txt` | construction (CMake ≥ 3.22, C++20 sans extensions, `-Wall -Wextra -Wpedantic -Werror`) |
| `common/format.hpp` | écriture et lecture des vidages ; comparaison exacte de centres (produits croisés sur 256 bits) |
| `vidage/vidage_v11.cpp` | `mhgp12_vidage` ; mesure de résolution à trois bras |
| `mes_m3/welzl_proposal.hpp` | port de `DWelzl` (v10, `tower.cpp` l. 250-440) |
| `mes_m3/meb_cert.hpp` | cœur de `LEV-MEB-CERT` : `LEM-T1` corrigé, certificat exact, canonisation, repli, juge |
| `mes_m3/mes_m3.cpp` | `mhgp12_mes_m3` (porte et banc) |
| `mes_m4/mes_m4.cpp` | `mhgp12_mes_m4` (porte et banc ; noyau, contraction séquentielle et parallèle, `LEM-T6`) |
| `pilote.py` | pilote en bibliothèque standard (construire, portes, vider, m3, m3var, m4, rapport) |
| `out/` | vidages, journaux `*.jsonl`, `rapport_mes_m3_m4.json`, `tableaux.md` (jamais versés) |

À intégrer : les sources ci-dessus, pas `out/` ni `build/`. Compilation vérifiée avec GCC 13.3 (le
compilateur du codespace ; GCC 11.4 de la VM n'est pas disponible ici) et, en vérification syntaxique stricte, Clang 18 :
aucun avertissement. Le code n'emploie aucune liaison structurée capturée par une lambda (non garanti par GCC 11). Le
pilote a été joué de bout en bout (`tout`, deux processus) sous `python3 -S`.

## 2. Formats (résumé ; contrat complet : `README.md` § 4)

Un fichier = en-tête de 64 octets (`MHGP12DP`, version 1, genre, bits de coordonnées, K, k, nombre de sections, n,
trame), puis des sections étiquetées (8 octets d'étiquette, taille d'élément, nombre d'éléments, données complétées à 8
octets). Le lecteur C++ (`common/format.hpp`) et le lecteur Python strict (`pilote.py`, `lire_vidage`) refusent une
magie ou une version inconnue, une section absente, inattendue, en double, de taille fausse, tronquée, ou des octets en
trop.

- `cat.bin` : sites (SiteIdx, ordre de Morton de la v11) ; Cat_K canonique (rang de niveau, p, m, q, S*), populations
  I puis U en CSR ; nombre de niveaux. Le niveau exact se recalcule depuis S* et les coordonnées.
- `ordre_<k>.bin` : naissances (clé, rang, nœud v11, coquille étendue) et leurs **centres exacts** (`i128` x, y, z, d) ;
  cellules régulières et étendues (boule, rang, p, m, q) ; traces strictes (masque de A dans U, ordre de la v11) ;
  **graine v11** de chaque trace (clé de naissance, nœud, fin de descente, nombre de pas) ; **parties de descente** où
  la v11 calcule une plus petite boule (k SiteIdx), avec la **route** de la v11 (table de populations par la fin de
  descente ; catalogue, census saturé, census complet par partie), l'action du pas (saut intérieur, trace stricte,
  terminal), B(F) dans Cat_K (exact) et S*(B(F)) ⊆ F.
- `foret_<k>.bin` : forêt publiée par la v11 (rang, parent, clé de naissance, enfants en CSR, verticales).

## 3. Résultats

Machine : codespace AMD EPYC 7763, 8 cœurs, **charge 5 à 14 pendant les mesures** (autres agents) ; vidage à 3 fils,
microbancs et mesure de résolution à 1 fil, un processus par mesure. **Les temps sont indicatifs** (seule G4 juge les
temps) ; compteurs, routes et identités sont déterministes et définitifs.

### 3.1 Vidages et contrôles

| Cas | `MHGP11FUL1` | Graines = journal v11 | Forêt série = publiée | Durée (3 fils) | Octets |
| --- | --- | --- | --- | ---: | ---: |
| ng00 K5 | identique (`3a2bfb4f…`) | 5 ordres sur 5 | oui | 56 s | 370 427 240 |
| ng01 K5 | identique (`5212a2ce…`) | 5 sur 5 | oui | 46 s | 309 147 336 |
| ng02 K5 | identique (`78feb765…`) | 5 sur 5 | oui | 54 s | 393 692 176 |
| u8000 K5 | identique (`f87dbb19…`) | 5 sur 5 | oui | 23 s | 168 731 096 |
| u16000 K5 | identique (`141bc7d6…`) | 5 sur 5 | oui | 44 s | 347 961 352 |
| u32000 K5 | identique (`a7563907…`) | 5 sur 5 | oui | 114 s | 716 025 688 |
| ng00 K10 | identique (`61a4245b…`) | 10 sur 10 | oui | 293 s | 2 008 028 288 |
| ng01 K10 | identique (`838a447e…`) | 10 sur 10 | oui | 232 s | 1 574 922 296 |
| ng02 K10 | identique (`81f89995…`) | 10 sur 10 | oui | 209 s | 1 925 350 960 |

Les durées comprennent la mesure de résolution à un fil (trois bras ; 3 répétitions à K5, 1 à K10).

### 3.2 Profil des descentes de la v11 (part de `MES-M7` côté v11)

Extraits (tableau complet par ordre : `out/tableaux.md`) :

| Cas | k | Traces | Pas | Parties (MEB) | Table sur la trace | Catalogue | Census saturé | Census complet | Plus longue chaîne |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 5 | 1 350 288 | 1 919 207 | 568 919 | 963 244 | 351 594 | 150 419 | 66 906 | 11 |
| ng01 K5 | 5 | 1 102 505 | 1 541 599 | 439 094 | 798 081 | 278 098 | 112 877 | 48 119 | 12 |
| ng02 K5 | 5 | 1 393 952 | 1 893 266 | 499 328 | 1 025 951 | 342 600 | 105 580 | 51 148 | 10 |
| ng00 K10 | 5 | 1 350 288 | 1 919 207 | 568 919 | 963 244 | 545 714 | 23 202 | 3 | 11 |
| ng00 K10 | 10 | 3 831 491 | 6 183 682 | 2 352 191 | 2 518 056 | 1 193 824 | 724 379 | 433 988 | 21 |
| ng01 K10 | 10 | 2 899 382 | 4 604 361 | 1 704 979 | 1 942 098 | 867 932 | 536 809 | 300 238 | 21 |
| ng02 K10 | 10 | 3 513 603 | 5 325 820 | 1 812 217 | 2 406 840 | 1 004 860 | 493 602 | 313 755 | 18 |

Toutes les descentes finissent sur la table de populations, sauf 14 (ng02 K5), 24 (ng01 K10) et 20 (ng02 K10) :
naissances étendues dont la population compte plus de k sites, terminées par un pas. Les pas recoupent le registre de
la v11 à 114 près au plus par ordre (descentes verticales, non rejouées). Un même ensemble de parties revient souvent :
59 % à 68 % seulement des parties sont distinctes. À K10, les ordres k ≤ 5 ont beaucoup plus de pas « catalogue »
qu'à K5 (Cat_10 contient les boules de p + q jusqu'à 11).

### 3.3 MES-M3

**Identité** : 0 écart de sphère, de support, d'identification et de cohérence avec le vidage, sur tous les ordres des
neuf cas. **Routes** : aucun repli, aucune proposition échouée ; `cert_table` = 5 parties en tout (§ 5).

| Cas | Parties | Part `t1` | Part « sphère au catalogue » | Part éligible (test complet) | MEB réf. (s) | MEB v12 (s) | Rapport |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 1 175 034 | 0,7519 | 0,7519 | 0,7519 | 0,798 | 0,663 | 0,830 |
| ng01 K5 | 911 687 | 0,7615 | 0,7615 | 0,7615 | 0,634 | 0,565 | 0,891 |
| ng02 K5 | 1 039 136 | 0,7997 | 0,7997 | 0,7997 | 0,699 | 0,593 | 0,847 |
| u8000 K5 | 565 549 | 0,8585 | 0,8585 | 0,8585 | 0,433 | 0,345 | 0,798 |
| u16000 K5 | 1 178 683 | 0,8573 | 0,8573 | 0,8573 | 1,053 | 0,830 | 0,788 |
| u32000 K5 | 2 446 499 | 0,8571 | 0,8571 | 0,8571 | 2,436 | 1,886 | 0,774 |
| ng00 K10 | 8 856 134 | 0,7609 | 0,7609 | 0,7609 | 35,214 | 9,612 | 0,273 |
| ng01 K10 | 6 557 526 | 0,7614 | 0,7614 | 0,7614 | 21,217 | 5,940 | 0,280 |
| ng02 K10 | 7 148 742 | 0,7953 | 0,7953 | 0,7953 | 22,346 | 5,726 | 0,256 |

(Les trois parts ne diffèrent qu'au-delà de la quatrième décimale : § 4.1.) Les parties non certifiées par `LEM-T1`
sont des sphères **hors de la table** (route `cert_census` : certificat exact du support proposé, niveau matérialisé,
census, comme la v11) : boules hors de Cat_K (p + q > K + 1, ou p ≥ K) pour l'écrasante majorité. Par ordre, à K10 sur
ng00 (ns par partie, un fil) :

| k | Parties | `t1` | `cert_census` | Part `t1` | ns réf. | ns v12 | Rapport |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 64 721 | 63 916 | 805 | 0,988 | 436 | 487 | 1,116 |
| 3 | 187 877 | 184 586 | 3 291 | 0,982 | 566 | 578 | 1,021 |
| 4 | 353 517 | 344 201 | 9 316 | 0,974 | 771 | 670 | 0,869 |
| 5 | 568 919 | 545 714 | 23 205 | 0,959 | 1 045 | 769 | 0,736 |
| 6 | 824 192 | 771 469 | 52 723 | 0,936 | 1 534 | 929 | 0,605 |
| 7 | 1 132 112 | 1 018 614 | 113 498 | 0,900 | 2 339 | 998 | 0,427 |
| 8 | 1 482 422 | 1 241 299 | 241 121 | 0,837 | 2 555 | 801 | 0,313 |
| 9 | 1 890 183 | 1 374 752 | 515 431 | 0,727 | 4 461 | 1 088 | 0,244 |
| 10 | 2 352 191 | 1 193 824 | 1 158 367 | 0,508 | 7 686 | 1 555 | 0,202 |

À l'ordre 8, deux parties passent par `cert_table`. À l'ordre 2, la voie nouvelle est un peu plus lente que la
référence (×1,06 à ×1,17 selon le cas), et parfois à l'ordre 3 (×0,87 à ×1,06) : `bounded_meb` n'y présente qu'un à
deux supports, et la proposition flottante plus l'inclusion F ⊆ P_b coûtent autant. Le gain croît avec k comme le
nombre de présentations de la référence (1,0 à l'ordre 2, 4,6 à l'ordre 5, 72 à 74 à l'ordre 10 par plus petite
boule). À l'ordre 10, par route : `t1` 5 927 à 9 159 ns → 888 à 1 367 ns (×0,15 environ), `cert_census` 4 108 à 6 224
ns → 881 à 1 278 ns (×0,21 à ×0,26). Variante `--repere absolu` (port littéral, coordonnées absolues) sur ng00 K5 :
mêmes routes, aucun repli ; le repère local reste le défaut (conditionnement en u24 et u32).

### 3.4 Résolution à un fil (règle d'adoption de MES-M3)

Trois bras sur les mêmes traces, dans le même processus, un fil ; graines identiques exigées trace par trace (vérifié
sur tous les ordres des neuf cas) : `v11` (code v11, `descend_each_step`), `réplique v11` (même descente réécrite pas
à pas, plus petite boule `bounded_meb`), `réplique v12` (même réplique, plus petite boule proposée et certifiée).

| Cas | v11 (s) | Réplique v11 (s) | Réplique v12 (s) | v12 / réplique v11 | v12 / v11 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 5,317 | 3,602 | 3,314 | 0,920 | 0,623 |
| ng01 K5 | 4,134 | 2,839 | 2,363 | 0,832 | 0,572 |
| ng02 K5 | 5,078 | 3,120 | 2,849 | 0,913 | 0,561 |
| u8000 K5 | 1,991 | 1,356 | 1,205 | 0,889 | 0,605 |
| u16000 K5 | 4,496 | 2,913 | 2,670 | 0,917 | 0,594 |
| u32000 K5 | 10,652 | 8,419 | 7,144 | 0,849 | 0,671 |
| **ng00 K10** | 69,515 | 60,121 | 29,026 | **0,483** | 0,418 |
| **ng01 K10** | 48,991 | 38,319 | 19,605 | **0,512** | 0,400 |
| **ng02 K10** | 43,313 | 34,626 | 19,616 | **0,567** | 0,453 |

Par ordre (v12 / réplique v11), ordres 2 à 10 : ng00 0,74 ; 0,99 ; 0,98 ; 0,89 ; 0,86 ; 0,59 ; 0,57 ; 0,42 ; 0,39 —
ng01 0,90 ; 1,05 ; 1,11 ; 0,66 ; 0,77 ; 0,65 ; 0,59 ; 0,46 ; 0,40 — ng02 0,83 ; 0,90 ; 0,81 ; 0,79 ; 0,80 ; 0,67 ;
0,54 ; 0,50 ; 0,51. Lecture : la comparaison propre est `réplique v12 / réplique v11` (seule la plus petite boule
diffère) : **−52 %, −49 %, −43 % à K10**, au-delà du seuil de 40 % sur les trois trames, de peu sur ng02 ; à K5 le gain
n'est que de 8 à 17 %, car le census (38 % des parties à l'ordre 5) et la table dominent. La réplique est elle-même 1,2
à 1,6 fois plus rapide que le code v11 (pas de registres transactionnels par pas) : c'est un second gain, distinct, que
`v12 / v11` cumule. Avec un seul processus par cas et une machine chargée, ces rapports portent une incertitude de
quelques pour cent : **l'adoption se décide sur G4**.

### 3.5 MES-M4

Forêts identiques sur tous les ordres des neuf cas ; naissances canoniques = `v11_node` ; graines = nœuds v11 ; dates
(rang de graine < rang de cellule) toutes valides ; `LEM-T6` : 0 écart. Temps à un fil (dernier passage, charge 5 à 9,
minimum de 5 passes) ; contraction parallèle à 3 fils, sortie identique exigée et obtenue partout :

| Cas | k | Naissances | Représentants | Fusions | Naissances (ms) | Noyau (ms) | Contraction (ms) | Contraction, 3 fils (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 | 5 | 341 081 | 1 350 288 | 235 290 | 1,03 | 12,05 | 11,11 | 9,19 |
| ng01 K5 | 5 | 283 207 | 1 102 505 | 195 058 | 0,78 | 9,67 | 8,97 | 7,59 |
| ng02 K5 | 5 | 361 326 | 1 393 952 | 248 050 | 2,02 | 13,05 | 12,29 | 10,36 |
| u32000 K5 | 5 | 700 184 | 3 165 977 | 463 572 | 1,21 | 26,86 | 23,13 | 15,78 |
| ng00 K10 | 10 | 979 350 | 3 831 491 | 659 223 | 2,51 | 33,98 | 34,73 | 25,72 |
| ng01 K10 | 10 | 758 514 | 2 899 382 | 506 551 | 2,05 | 25,76 | 26,00 | 20,29 |
| ng02 K10 | 10 | 937 412 | 3 513 603 | 618 368 | 3,51 | 33,15 | 34,69 | 24,35 |

D'un passage à l'autre, la charge de la machine a fait varier ces temps jusqu'à ×1,4 (premier passage : noyau de 16 ms
à l'ordre 5 de ng00 et 46 ms à l'ordre 10). La contraction séquentielle par groupes de rang coûte autant que le noyau
(le prototype de la conception coûtait le double) ; la parallèle ne gagne qu'un quart à 3 fils sur cette machine, et
perd aux petits ordres (environ 10 ms de réveil des fils) : rien d'utile pour la cible de 3 ms avant G4.

### 3.6 Portes et mutants

| Porte | Code | Attendu | Verdict |
| --- | ---: | ---: | --- |
| `mhgp12_mes_m3 --porte` (10 témoins, dont `WIT-T1-CARRE`) | 0 | 0 | conforme |
| mutant `sans_s_dans_f`, porte | 1 | 1 | tué (par `WIT-T1-CARRE`) |
| mutant `sans_s_dans_f`, ng00 K5, proposition `mere_puis_welzl` | 1 | 1 | tué (1 024 098 parties fausses) |
| binaire normal, ng00 K5, proposition `mere_puis_welzl` | 0 | 0 | identité ; mère écartée 1 024 098 fois |
| `mhgp12_mes_m4 --porte 6000` (borne `CST-0212`, 3 témoins de plateau, 6 000 hypergraphes) | 0 | 0 | conforme (149 415 nœuds, 15 419 fusions d'au moins trois enfants) |
| mutant `sans_contraction`, porte | 1 | 1 | tué (2 témoins, 5 646 cas aléatoires) |
| mutant `sans_contraction`, ng00 K5 et ng01 K10 | 1 | 1 | tué (tous les ordres) |

## 4. Constats de l'auditeur intégrés

### 4.1 `CST-0101` : `LEM-T1` exige S ⊆ F

Le constat est arrivé avant l'écriture du microbanc : le code n'a jamais existé sans le test. Énoncé appliqué : si
S ⊆ F (inclusion de multiensembles d'indices de sites : S strictement croissant, chaque site de S trouvé dans F), si
S = S*(b) pour une boule b du catalogue et si F ⊆ P_b, alors B(F) = b. Dans `certify()`, le test S ⊆ F précède la
recherche dans la table ; son échec rend la raison `s_hors_de_f` et le **repli exact** (`bounded_meb`), jamais un
succès ; le certificat exact suppose lui aussi un support tiré de F et n'est donc pas tenté.

- **Témoin gravé** `WIT-T1-CARRE` (porte de MES-M3) : A = (0,0,0), B = (2,0,0), C = (2,2,0), D = (0,2,0) ; b = boule
  de diamètre AC, coquille ABCD (q = 2, coquille étendue) ; F = {A, B} ⊆ P_b ; S = S*(b) ⊄ F. Attendu : route
  `repli_table`, raison `s_hors_de_f`, rayon carré 1 (boule de diamètre AB, présente au mini-catalogue). Le mutant rend
  `t1` et b, de rayon carré 2 : écart de sphère, de boule et de support, code 1.
- **Part des pas certifiés avec le test complet** : la part éligible (S*(B(F)) ⊆ F ⊆ P_b, mesurée sur la référence) et
  la part réellement certifiée par `t1` sont **égales** dans tous les cas, et ne diffèrent de la part « sphère au
  catalogue » (le chiffre de 76 % de la v10, calculé sans le test) que de 3 parties (ng00 K5, ng01 K5), 13 (ng02 K5,
  ng01 K10), 18 (ng00 K10) et 39 (ng02 K10) : B(F) au catalogue mais S*(b) ⊄ F, toutes repérées par le census complet
  de la v11. **Nouveaux chiffres, sur les descentes de la v11 : 75,19 % (ng00 K5), 76,15 % (ng01 K5), 79,97 % (ng02 K5),
  76,09 % (ng00 K10), 76,14 % (ng01 K10), 79,53 % (ng02 K10), 85,7 % (uniformes)** des plus petites boules certifiées
  sans arithmétique. La part décroît avec k : 96 % à l'ordre 2, 62 % à 69 % à l'ordre 5 de K5, 51 % à 55 % à l'ordre 10
  de K10 ; à l'ordre maximal, près de la moitié des sphères sont hors du catalogue.
- **Pourquoi le test ne coûte rien et n'est jamais décisif avec Welzl** : la proposition de DWelzl est un ensemble
  d'indices de F ; S ⊆ F tient par construction, et le mutant survit sur les données réelles avec cette proposition
  (vérifié sur ng00 K5). Le test devient décisif avec **toute proposition venue d'ailleurs** : marche depuis la sphère
  mère (la seconde candidate de D-G3), cache de supports, proposition calculée sur le GPU. La variante
  `mere_puis_welzl` le montre sur données réelles : la mère contient toujours la partie (F ⊆ P_mère), son S* n'y est
  jamais (descente stricte) ; sans le test, chaque pas dont la mère est au catalogue serait certifié faux.

### 4.2 `CST-0212` : domaine du codage des opérandes du noyau

Le noyau code un opérande sur 32 bits, bit 31 = genre (naissance ou événement) : 31 bits utiles, et la naissance 2^31
se confondrait avec l'événement 0. **Choix retenu : écrire le domaine exact et le garder**, plutôt qu'un genre séparé
ou des indices sur 64 bits — l'événement garde ses 20 octets, aucun coût mesurable (un seul test par ordre, hors de
la boucle). Domaine : au plus **2^31 − 1 naissances par ordre**, donc au plus 2^31 − 2 événements (une union de
moins que de naissances) et au plus 2^32 − 3 nœuds, sous la sentinelle 2^32 − 1 (deux `static_assert`). Au-delà :
refus explicite décidé sur le seul nombre de naissances, **avant toute allocation** (`kernel()` rend un refus, le banc
sort en code 2 avec `domaine_operandes_31_bits`). **Porte à la borne** (`mhgp12_mes_m4 --porte`) : 2^31 − 1 dans le
domaine, 2^31 et 2^32 − 1 refusés par le noyau sans qu'aucune cellule ni aucun événement ne soit alloué ; verte. La v11
refusait déjà 2^31 naissances et plus (`forest_capacities`). Données : au plus 979 350 naissances par ordre. Les
résultats de MES-M4 publiés ci-dessus sont ceux du binaire corrigé (portes et banc rejoués après le correctif).
Domaine de chaque espace du microbanc (la note de l'auditeur demande aussi des décalages en `u64` jusque dans l'entrée
du noyau : c'est le cas, contrairement au prototype `noyau_v11.cpp`) :

| Espace | Type | Domaine |
| --- | --- | --- |
| naissance (feuille) | `u32`, bit 31 libre | 0 à nb − 1, nb ≤ 2^31 − 1 (refus au-delà, avant allocation) |
| événement brut | `u32`, codé avec le bit 31 | 0 à nb − 2 (une union de moins que de naissances) |
| opérande | `u32` | feuille (bit 31 nul) ou événement (bit 31 mis) ; jamais la sentinelle dans le domaine |
| nœud final | `u32` | 0 à 2 nb − 2 ≤ 2^32 − 4 (au plus 2^32 − 3 nœuds) ; `0xFFFFFFFF` = absence (parent de la racine, clé d'une fusion) |
| rang, boule (clé de naissance) | `u32` | rangs et `BallIdx` de la v11, < `0xFFFFFFFF` (domaine de la v11) |
| décalages (représentants par jonction, enfants, traces, parties) | `u64` | sans borne pratique |


## 5. Cas limites rencontrés (tous traités, comptés, aucun en silence)

| Cas | Où | Compte | Traitement |
| --- | --- | ---: | --- |
| sphère hors de Cat_K (p + q > K + 1, ou p ≥ K) | toutes les trames, surtout à l'ordre maximal | 38 % des parties à l'ordre 5 de K5 (ng00), 45 % à 49 % à l'ordre 10 de K10 | route `cert_census` : certificat exact du support proposé, niveau matérialisé, census (comme la v11) |
| sphère au catalogue mais S*(b) ⊄ F | census complet de la v11 | 3 à 39 par cas | `LEM-T1` inéligible ; `cert_census` ; identification par census et S* global, identique à la v11 |
| coquille étendue (m > q) au catalogue | LiDAR seulement | 292 (ng01 K5) à 4 764 (ng02 K10) | `t1` sauf 5 parties |
| proposition valide mais non canonique (coquille étendue) | ng00 K10 (ordre 8), ng01 K10 (ordre 6) | 2 + 3 | canonisation parmi F ∩ U, route `cert_table` |
| sites de F cosphériques en plus du support | LiDAR | 67 à 1 860 par cas | canonisation, ou `t1` quand la proposition est S* |
| naissance étendue (population > k) terminée par un pas | ng02 K5, ng01 K10, ng02 K10 | 14, 24, 20 | pas terminal de `descent_step` ; graine identique au journal |
| plus petites boules des traces (`strict_trace`, coquilles étendues) | LiDAR | 323 à 4 225 par cas | hors du périmètre de MES-M3 ; gardées en `bounded_meb` dans les deux répliques |
| proposition flottante en échec, barycentre nul, site extérieur, support dégénéré | aucun sur les données ; témoins de porte | 0 | repli exact, raison publiée |

## 6. Ce qui reste pour le passage sur G4

1. **Session gardée** (le développeur principal la lance) : copier `v12_tour/` (sources), construire la v11 Release u21
   (`cmake -S morsehgp3D_v11 -B <b> -DCMAKE_BUILD_TYPE=Release -DMHGP11_COORD_BITS=21`, cible `mhgp11`), puis deux lots
   de moins de 30 minutes (estimés depuis les durées locales) :
   `python3 pilote.py --v11-build <b> --donnees <données> --sortie <out> --fils 48 --fils-construction 48
   --fils-contraction 48 --processus 5 --cas ng00:5,ng01:5,ng02:5 tout`, puis
   `... --processus 3 --cas ng00:10,ng01:10,ng02:10 vider m3 m4 rapport`. Rapatrier `rapport_mes_m3_m4.json`,
   `tableaux.md` et les `*.jsonl`, pas les vidages.
2. **Juger MES-M3** : `réplique v12 / réplique v11` sur la somme des ordres à K10, trois trames, plusieurs processus
   (règle : −40 % au moins ; localement −43 % à −52 %) ; non-régression à l'ordre 2 (×1,06 à ×1,17 localement).
3. **Juger MES-M4** : noyau ≤ 10 ms (ordre 5, K5) et ≤ 35 ms (ordre 10, K10) à un fil — localement 9,7 à 13,1 ms et
   25,8 à 34,0 ms — ; contraction ≤ 3 ms avec `--fils-contraction 48`.
4. **Trames de plusieurs séquences** (décision D7) : le pilote accepte tout cas `X:K` désignant `<données>/X.u32le` ;
   sans empreinte de référence, l'identité `MHGP11FUL1` est publiée « sans référence », les autres contrôles restent
   exigés.
5. **Hors de ce lot, à faire avant le port de l'étage G** : `LEM-T3` (arrêt sur la première cellule de fenêtre) et D-G4
   (census borné aux k plus proches) : avec la plus petite boule certifiée, le census devient le coût dominant de la
   résolution aux ordres hauts (45 % à 49 % des parties de l'ordre 10 sont hors catalogue) ; plus petites boules des
   traces strictes par les prédicats de `OBJ-T2` au lieu de `bounded_meb` ; un mémo des parties (32 % à 41 % de parties
   répétées). Côté forêt : historique d'attache et verticales des fusions (`LEM-T5`), recouvrement noyau-résolution
   (O1, O2). Côté mesures : la moitié v10 de MES-M7, MES-M2, MES-M5, MES-M6.

## 7. Limites de ce passage

- Temps locaux sur une machine partagée à charge 5 à 14 pour 8 cœurs, un seul processus par mesure : ils ne prédisent
  pas G4 et ne permettent aucune adoption. Les rapports entre bras d'un même processus sont plus fiables que les valeurs
  absolues, sans plus.
- La réplique de résolution réécrit la descente de la v11 hors de son code (fidélité vérifiée par l'identité des
  graines sur les trois bras, pas par un second juge) ; elle ne tient pas les registres de travail de la v11.
- La proposition est le port de DWelzl ; la marche depuis la sphère mère (seconde candidate de D-G3) n'a pas été
  implantée comme proposition, seulement comme témoin du test S ⊆ F (où elle est écartée à chaque pas).
- Les uniformes ont été joués à K5 seulement ; aucune trame d'autre séquence n'a été préparée ici.
- GCC 11.4 (VM) n'a pas pu être essayé localement (GCC 13.3 et Clang 18 seulement).

Fin de rédaction : 2026-10-07 10:52:27 UTC (`date -u`).
