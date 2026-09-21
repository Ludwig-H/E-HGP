# Voies q3/q4 du front WSPD sur les scans LiDAR : fenêtre de témoins, rejet exact par paire, couvertures et seeds

Auditeur B, 21 septembre 2026. Sources du moteur épinglées à **c5308651**
(bibliothèque `libmhgp8_p0.a` sha256 `0dbfc5db4eabe42e…`, arbre de travail
détaché, 91 CTests Release passés). `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Mesure d'audit : elle compte, elle ne qualifie rien et ne
touche pas au moteur (le front est appelé tel quel, masque 6, mode
`MidpointSamples`, options par défaut).

## Les deux questions du constructeur (journal du 21 septembre)

*Question pratique* : au front multivoie, la faiblesse dominante est-elle le
choix de K témoins autour du pivot ou l'absence de crédits h_a/h_b transmis
par blocs ? *Question prioritaire* : valider un census q3 par boîtes de la
puissance exacte, et recommander des certificats de famille ou des blocs de
seeds avant la construction des clés de boule, sachant que le premier global
8k (scan 0, K5, s8, Local28) a développé 2 285 750 arêtes, 780 661 556 seeds
q3 et 361 201 093 303 tests ponctuels en 22 minutes.

## Ce qui est mesuré

Entrées : les scans préparés par A ([lidar08_20260914/](../lidar08_20260914/prepared/single_000000/METADATA.json),
quantification 2 cm, échantillons imbriqués par priorité de hachage), scans
000000/000100/000200, n ∈ {8000, 16000, 32000}, K ∈ {5, 10}, s = 8, soit
18 exécutions ; sha256 de chaque entrée vérifié contre les METADATA. Pour
chaque exécution et chaque voie q ∈ {3, 4} (seuil h_q = K + 2 − q témoins) :

1. **Plafond de boîte** : par rectangle résiduel A×B émis par le front, nombre
   U_q (plafonné à h_q) de sites hors A∪B vérifiant le prédicat de boîte du
   front (h_min > 0 et α_q·h_min² > Ξ_max, α3 = 3, α4 = 2), par descente
   saturante de l'index avec bornes conjointes exactes. U_q ≥ h_q : un
   proposeur ponctuel parfait rejetait la voie sur ce rectangle.
2. **Fenêtres L = K, 2K, 4K** de rangs autour du pivot du front (même descente
   vers le milieu, rangs de A/B sautés) : masse que la voie perdrait avec une
   fenêtre élargie. L = K rejette zéro rectangle émis (contrôle de cohérence,
   exigé par le lecteur).
3. **Juge d'échantillon exact** : 2 000 paires (a,b) par voie, tirées
   uniformément dans la masse résiduelle, et pour chacune le compte exact des
   sites du citron L_q(a,b) = {H > 0, α_q·H² > Ξ} par balayage complet ;
   c_q ≥ h_q : paire rejetable exactement (lemme du citron). Témoins classés :
   universels de boîte, non universels hors A∪B, dans A ou B.
4. **Descente saturante par paire** (boîtes singleton de a et b, nœud entier
   admis ou écarté par les mêmes bornes, feuilles jugées par le citron exact),
   en préordre puis « milieu d'abord » ; la décision doit coïncider avec le
   balayage complet (désaccords exigés nuls) ; visites de nœuds relevées.
5. **Couverture et seeds** par paire : sites de la couverture close du
   constructeur (|2z−a−b|² ≤ 4|b−a|²), seeds q3 aigus à arête ab maximale
   (égalités incluses) et, pour les paires non rejetables de la voie q3, nombre
   de seeds dont la circumboule a une profondeur < h_q (prédicat entier
   Δ|z−a|² < (z−a)·N, tout < 2^103) ; pour les 100 premières paires rejetables
   de chaque exécution, vérification exécutable du lemme (aucun seed ne doit
   passer sous le seuil).

Aucun flottant. Reçu [FRONT_LANES_CHECKS.json](FRONT_LANES_CHECKS.json)
(sorties brutes, pins, commandes), rejoué par `run_front_lanes.py read` en
`python3` et `python3 -O` (les parts publiées sont recalculées depuis les
comptes entiers). Harnais [front_lanes_probe.cpp](front_lanes_probe.cpp).

## Réponse : la fenêtre de K témoins est la faiblesse, pas les crédits par blocs

Sur les 18 exécutions (72 000 paires jugées, 0 désaccord entre les
descentes saturantes et le balayage complet) :

- **Les témoins situés dans A ou B (crédits h_a/h_b) ne pèsent rien** :
  1,2 à 2,5 % des témoins exacts en q3, 1,5 à 3,5 % en q4.
  Transmettre ces crédits par blocs ne changerait pas le front.
- **Le proposeur de boîte plafonne bien au-dessus de la fenêtre K** : un
  proposeur ponctuel parfait (mesure 1) retirerait 82 à 91 % de la masse
  résiduelle q3 et 75 à 89 % de la masse q4, quand la fenêtre 2K n'en
  retire que 36 à 56 % (q3) et 6 à 55 % (q4, seulement
  6 à 16 % à K = 5) et la fenêtre 4K 42 à 65 % (q3),
  11 à 62 % (q4). Élargir la fenêtre ne rattrape pas le plafond :
  il faut chercher les témoins par descente saturante de l'index, pas par
  rangs autour du pivot.
- **Presque toute la masse résiduelle est rejetable exactement par paire** :
  89 à 97 % des paires q3 et 88 à 97 % des paires q4 tirées
  portent au moins h_q sites dans leur citron (intervalles de Wilson à 95 %
  dans les tableaux), part qui **croît avec n**. Parmi ces paires rejetables,
  91 à 95 % (q3) et 84 à 93 % (q4) vivent dans un rectangle
  que le proposeur de boîte parfait rejetait déjà ; le reste exige un test par
  paire, car 40 à 57 % (q3) et 46 à 67 % (q4) des témoins exacts sont hors
  A∪B sans être universels pour les boîtes.

## Réponse : rejeter la paire avant la couverture divise les seeds par plusieurs centaines

- **Descente saturante par paire** : 43 à 148 visites de nœuds par paire rejetable q3
  en ordre « milieu d'abord » (159 à 898 en préordre), 63 à 222 en q4 ; les
  paires conservées coûtent 68 à 250 visites (q3) car la descente s'y épuise.
  La décision coïncide avec le balayage complet sur les 72 000 paires.
- **Couvertures** : une paire rejetable porte 2 080 à 13 444 sites dans sa couverture
  (q3), une paire conservée 47 à 792 en moyenne (maximum par exécution de 392 à 11 225) : les arêtes
  qui survivent au citron sont courtes à l'échelle locale, leurs couvertures
  sont petites, avec une queue lourde.
- **Seeds q3** : 350 à 1 950 seeds aigus par paire rejetable (le constructeur mesure 341
  par arête à 8k/K5), 8 à 125 par paire conservée. Rejeter la paire avant de
  construire sa couverture divise la masse de seeds par 87 à 1 519 selon
  l'exécution ; les seeds survivants coûtent 313 à 166 016 tests ponctuels par paire
  conservée (census naïf sur la couverture) et émettent 0,35 à 1,27 boule par
  paire conservée.
- **Lemme du citron, vérification exécutable (q3)** : 1 800 paires rejetables,
  1 712 517 seeds aigus propriétaires, 2 385 827 904 tests entiers, **0 violation** : aucun seed d'une paire
  rejetable n'a de circumboule de profondeur < h_q. C'est une confirmation
  par instances de la preuve de A, pas une preuve ; q4 n'est pas vérifié ici.

Ordre recommandé, avant covers et seeds, sans scan quadratique local :
(1) par rectangle résiduel, descente saturante de l'index avec les bornes de
boîte déjà écrites (plafond, mesure 1 : 46 077 076 à 851 337 144 visites par
exécution, 2 à 48 s mono-fil dans ce harnais pour les deux voies) ;
(2) par paire restante, descente saturante « milieu d'abord » avec les boîtes
singleton ; (3) couverture serrée |2z−a−b|² ≤ 3|b−a|² (rayon √3·D suffisant
pour toute boule q3 propriétaire ; la couverture 4|b−a|² reste nécessaire en
q4 : (1/√2 + √(3/2))·D < 2D) et census par seed, naïf ou par boîtes de la
puissance, sur les survivantes seulement.

## Tableaux

Voie q3 : masse résiduelle (part des paires du nuage), masse retirée par le
plafond de boîte et par les fenêtres 2K/4K, paires rejetables exactement
(Wilson 95 %), part des rejetables atteignables par le proposeur de boîte,
témoins universels / non universels hors A∪B / dans A ou B.

| scan | n | K | résiduel % | plafond % | 2K % | 4K % | rejetable % | atteignable % | témoins % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 000000 | 8000 | 5 | 6,7 | 86,9 | 49,2 | 55,6 | 92,8 [91,6 ; 93,9] | 93,8 | 50,5 / 47,7 / 1,8 |
| 000000 | 8000 | 10 | 14,0 | 86,6 | 56,5 | 64,5 | 91,5 [90,2 ; 92,7] | 94,4 | 55,0 / 43,2 / 1,9 |
| 000000 | 16000 | 5 | 4,0 | 86,5 | 44,9 | 52,1 | 92,8 [91,6 ; 93,9] | 92,5 | 45,1 / 52,7 / 2,1 |
| 000000 | 16000 | 10 | 9,3 | 87,9 | 52,1 | 60,2 | 93,6 [92,4 ; 94,6] | 93,4 | 50,6 / 47,7 / 1,7 |
| 000000 | 32000 | 5 | 3,1 | 88,7 | 50,0 | 56,0 | 95,3 [94,3 ; 96,1] | 92,2 | 40,3 / 57,4 / 2,3 |
| 000000 | 32000 | 10 | 6,5 | 90,2 | 56,2 | 62,6 | 96,2 [95,2 ; 96,9] | 95,1 | 47,8 / 50,5 / 1,7 |
| 000100 | 8000 | 5 | 4,9 | 81,6 | 36,4 | 44,2 | 88,9 [87,4 ; 90,2] | 91,1 | 46,5 / 50,9 / 2,5 |
| 000100 | 8000 | 10 | 12,1 | 86,0 | 51,6 | 60,4 | 91,8 [90,6 ; 93,0] | 93,7 | 56,1 / 42,1 / 1,9 |
| 000100 | 16000 | 5 | 4,2 | 86,2 | 40,3 | 46,1 | 93,3 [92,1 ; 94,3] | 92,3 | 50,5 / 47,9 / 1,6 |
| 000100 | 16000 | 10 | 7,7 | 86,5 | 52,3 | 60,2 | 92,8 [91,5 ; 93,8] | 92,7 | 52,2 / 46,2 / 1,6 |
| 000100 | 32000 | 5 | 3,3 | 88,0 | 40,3 | 45,0 | 96,0 [95,0 ; 96,7] | 92,8 | 40,8 / 57,0 / 2,2 |
| 000100 | 32000 | 10 | 5,6 | 87,4 | 51,5 | 58,5 | 94,0 [92,8 ; 94,9] | 90,8 | 46,1 / 52,1 / 1,8 |
| 000200 | 8000 | 5 | 7,9 | 85,3 | 36,5 | 41,6 | 93,3 [92,2 ; 94,4] | 91,1 | 52,6 / 45,3 / 2,2 |
| 000200 | 8000 | 10 | 14,3 | 85,4 | 45,9 | 53,9 | 92,6 [91,4 ; 93,7] | 92,4 | 53,5 / 44,1 / 2,4 |
| 000200 | 16000 | 5 | 6,9 | 87,7 | 37,9 | 43,5 | 96,5 [95,5 ; 97,2] | 92,1 | 53,7 / 44,7 / 1,5 |
| 000200 | 16000 | 10 | 11,5 | 88,0 | 49,4 | 55,7 | 94,9 [93,8 ; 95,8] | 93,7 | 53,0 / 44,8 / 2,3 |
| 000200 | 32000 | 5 | 5,3 | 89,9 | 40,3 | 45,5 | 96,9 [96,0 ; 97,5] | 91,4 | 54,5 / 43,9 / 1,6 |
| 000200 | 32000 | 10 | 10,9 | 91,4 | 46,7 | 51,6 | 96,4 [95,5 ; 97,1] | 94,5 | 59,2 / 39,6 / 1,2 |

Voie q4, mêmes colonnes :

| scan | n | K | résiduel % | plafond % | 2K % | 4K % | rejetable % | atteignable % | témoins % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 000000 | 8000 | 5 | 4,2 | 76,5 | 14,3 | 22,4 | 88,5 [87,1 ; 89,9] | 86,7 | 37,5 / 59,3 / 3,1 |
| 000000 | 8000 | 10 | 13,5 | 83,7 | 48,3 | 57,1 | 92,2 [90,9 ; 93,2] | 92,2 | 49,6 / 48,7 / 1,7 |
| 000000 | 16000 | 5 | 3,0 | 77,7 | 14,9 | 23,1 | 91,1 [89,8 ; 92,3] | 84,6 | 35,3 / 61,7 / 2,9 |
| 000000 | 16000 | 10 | 9,3 | 85,4 | 42,8 | 51,3 | 93,3 [92,1 ; 94,3] | 92,3 | 46,3 / 51,9 / 1,8 |
| 000000 | 32000 | 5 | 1,9 | 77,3 | 9,1 | 17,3 | 93,5 [92,3 ; 94,5] | 84,2 | 29,6 / 66,9 / 3,5 |
| 000000 | 32000 | 10 | 6,7 | 87,3 | 55,4 | 62,2 | 95,0 [94,0 ; 95,9] | 90,6 | 43,6 / 54,5 / 1,9 |
| 000100 | 8000 | 5 | 3,8 | 74,7 | 11,0 | 19,5 | 87,9 [86,4 ; 89,3] | 84,8 | 38,5 / 58,7 / 2,8 |
| 000100 | 8000 | 10 | 11,6 | 83,4 | 43,4 | 52,2 | 90,7 [89,3 ; 91,9] | 92,5 | 52,6 / 45,7 / 1,7 |
| 000100 | 16000 | 5 | 3,2 | 80,3 | 8,9 | 15,4 | 92,3 [91,0 ; 93,4] | 86,2 | 32,8 / 64,4 / 2,7 |
| 000100 | 16000 | 10 | 7,8 | 83,9 | 44,2 | 52,3 | 93,3 [92,2 ; 94,4] | 90,8 | 47,8 / 50,6 / 1,7 |
| 000100 | 32000 | 5 | 2,6 | 82,7 | 6,3 | 11,5 | 95,1 [94,1 ; 96,0] | 88,2 | 32,2 / 65,0 / 2,8 |
| 000100 | 32000 | 10 | 6,0 | 85,5 | 43,3 | 50,2 | 94,8 [93,7 ; 95,6] | 91,3 | 41,5 / 56,6 / 1,8 |
| 000200 | 8000 | 5 | 5,9 | 78,0 | 8,3 | 14,0 | 91,8 [90,6 ; 93,0] | 85,4 | 47,2 / 50,9 / 1,9 |
| 000200 | 8000 | 10 | 14,3 | 82,8 | 40,2 | 47,7 | 91,3 [90,0 ; 92,5] | 91,6 | 50,4 / 47,2 / 2,3 |
| 000200 | 16000 | 5 | 5,0 | 80,0 | 8,1 | 14,7 | 94,6 [93,5 ; 95,5] | 84,4 | 47,1 / 51,2 / 1,6 |
| 000200 | 16000 | 10 | 11,8 | 85,6 | 42,0 | 48,5 | 94,5 [93,4 ; 95,4] | 91,0 | 49,7 / 48,3 / 2,0 |
| 000200 | 32000 | 5 | 3,9 | 82,8 | 16,4 | 20,4 | 96,5 [95,6 ; 97,2] | 86,2 | 42,4 / 55,6 / 2,0 |
| 000200 | 32000 | 10 | 10,8 | 88,7 | 37,7 | 41,8 | 97,1 [96,3 ; 97,7] | 91,5 | 52,0 / 46,5 / 1,5 |

Coûts par paire : visites de la descente saturante (préordre / milieu
d'abord) par paire rejetable et par paire conservée, maximum milieu d'abord,
sites de couverture (4|b−a|²) par paire rejetable, par paire conservée et
maximum, seeds q3 par paire rejetable, par paire conservée et maximum, boules
émises par paire conservée, tests de census par paire conservée, facteur de
réduction de la masse de seeds (q3 seulement).

| scan | n | K | voie | visites rej. | visites cons. | max | couv. rej. | couv. cons. | max | seeds rej. | seeds cons. | max | émises | tests | facteur |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 000000 | 8000 | 5 | q3 | 192 / 57 | 68 / 68 | 1 124 | 2 386 | 47 | 392 | 389 | 8,9 | 104 | 0,55 | 388 | 570 |
| 000000 | 8000 | 5 | q4 | 222 / 103 | 85 / 85 | 1 204 | 2 202 | 83 | 1 451 | — | — | — | — | — | — |
| 000000 | 8000 | 10 | q3 | 242 / 43 | 111 / 111 | 1 102 | 2 589 | 169 | 3 022 | 377 | 28,6 | 711 | 1,26 | 12 941 | 144 |
| 000000 | 8000 | 10 | q4 | 269 / 63 | 123 / 123 | 1 441 | 2 639 | 196 | 3 139 | — | — | — | — | — | — |
| 000000 | 16000 | 5 | q3 | 306 / 61 | 98 / 98 | 2 598 | 3 806 | 120 | 5 051 | 635 | 20,4 | 954 | 0,65 | 6 743 | 403 |
| 000000 | 16000 | 5 | q4 | 386 / 96 | 101 / 101 | 3 073 | 3 884 | 104 | 1 988 | — | — | — | — | — | — |
| 000000 | 16000 | 10 | q3 | 366 / 63 | 135 / 135 | 2 310 | 4 532 | 222 | 6 230 | 716 | 37,3 | 1 491 | 1,27 | 27 362 | 282 |
| 000000 | 16000 | 10 | q4 | 442 / 94 | 97 / 97 | 2 725 | 4 776 | 115 | 1 162 | — | — | — | — | — | — |
| 000000 | 32000 | 5 | q3 | 430 / 95 | 212 / 212 | 3 571 | 8 259 | 321 | 5 042 | 1 430 | 57,2 | 835 | 0,57 | 20 197 | 508 |
| 000000 | 32000 | 5 | q4 | 304 / 197 | 116 / 116 | 6 467 | 5 694 | 218 | 8 112 | — | — | — | — | — | — |
| 000000 | 32000 | 10 | q3 | 550 / 73 | 183 / 183 | 3 649 | 8 895 | 214 | 3 688 | 1 482 | 35,5 | 612 | 0,60 | 7 121 | 1 044 |
| 000000 | 32000 | 10 | q4 | 624 / 122 | 177 / 177 | 5 096 | 9 160 | 272 | 3 692 | — | — | — | — | — | — |
| 000100 | 8000 | 5 | q3 | 159 / 69 | 97 / 97 | 1 179 | 2 080 | 184 | 2 908 | 350 | 32,8 | 655 | 0,52 | 13 445 | 87 |
| 000100 | 8000 | 5 | q4 | 215 / 124 | 107 / 107 | 1 713 | 2 348 | 189 | 2 910 | — | — | — | — | — | — |
| 000100 | 8000 | 10 | q3 | 183 / 64 | 104 / 104 | 1 119 | 2 435 | 162 | 2 764 | 355 | 26,4 | 590 | 1,05 | 7 484 | 152 |
| 000100 | 8000 | 10 | q4 | 209 / 81 | 128 / 128 | 1 156 | 2 524 | 229 | 2 945 | — | — | — | — | — | — |
| 000100 | 16000 | 5 | q3 | 322 / 104 | 70 / 70 | 2 900 | 5 155 | 47 | 403 | 880 | 8,1 | 83 | 0,46 | 313 | 1 519 |
| 000100 | 16000 | 5 | q4 | 367 / 197 | 78 / 78 | 3 202 | 5 112 | 82 | 4 827 | — | — | — | — | — | — |
| 000100 | 16000 | 10 | q3 | 290 / 74 | 145 / 145 | 1 738 | 4 437 | 378 | 5 755 | 722 | 57,5 | 1 354 | 0,81 | 54 646 | 162 |
| 000100 | 16000 | 10 | q4 | 348 / 108 | 100 / 100 | 1 921 | 4 700 | 123 | 5 147 | — | — | — | — | — | — |
| 000100 | 32000 | 5 | q3 | 529 / 148 | 89 / 89 | 4 040 | 10 413 | 152 | 6 701 | 1 863 | 31,1 | 1 671 | 0,46 | 47 243 | 1 420 |
| 000100 | 32000 | 5 | q4 | 589 / 222 | 151 / 151 | 3 367 | 10 685 | 259 | 9 716 | — | — | — | — | — | — |
| 000100 | 32000 | 10 | q3 | 571 / 104 | 130 / 130 | 5 314 | 9 211 | 297 | 10 331 | 1 598 | 45,4 | 1 534 | 1,10 | 66 669 | 548 |
| 000100 | 32000 | 10 | q4 | 680 / 165 | 151 / 151 | 4 079 | 9 994 | 349 | 9 570 | — | — | — | — | — | — |
| 000200 | 8000 | 5 | q3 | 227 / 59 | 114 / 114 | 1 247 | 2 798 | 239 | 3 241 | 401 | 39,1 | 766 | 0,53 | 14 766 | 145 |
| 000200 | 8000 | 5 | q4 | 302 / 99 | 130 / 130 | 1 928 | 2 645 | 261 | 2 810 | — | — | — | — | — | — |
| 000200 | 8000 | 10 | q3 | 219 / 56 | 139 / 139 | 1 439 | 2 757 | 305 | 3 126 | 358 | 47,1 | 645 | 0,90 | 18 118 | 96 |
| 000200 | 8000 | 10 | q4 | 251 / 66 | 155 / 155 | 1 564 | 2 866 | 325 | 3 369 | — | — | — | — | — | — |
| 000200 | 16000 | 5 | q3 | 449 / 87 | 153 / 153 | 2 868 | 6 082 | 464 | 5 791 | 922 | 86,6 | 1 179 | 0,35 | 64 147 | 290 |
| 000200 | 16000 | 5 | q4 | 513 / 169 | 164 / 164 | 2 772 | 5 589 | 467 | 6 544 | — | — | — | — | — | — |
| 000200 | 16000 | 10 | q3 | 411 / 75 | 178 / 178 | 2 334 | 5 966 | 609 | 5 815 | 795 | 111,4 | 1 480 | 0,96 | 108 347 | 134 |
| 000200 | 16000 | 10 | q4 | 475 / 104 | 180 / 180 | 2 996 | 6 125 | 503 | 5 871 | — | — | — | — | — | — |
| 000200 | 32000 | 5 | q3 | 893 / 71 | 132 / 132 | 3 379 | 11 133 | 358 | 11 003 | 1 758 | 63,4 | 2 126 | 0,40 | 82 541 | 853 |
| 000200 | 32000 | 5 | q4 | 1 258 / 126 | 307 / 307 | 4 515 | 10 707 | 758 | 11 122 | — | — | — | — | — | — |
| 000200 | 32000 | 10 | q3 | 898 / 111 | 250 / 250 | 3 931 | 13 444 | 792 | 11 225 | 1 950 | 124,8 | 2 405 | 0,65 | 166 016 | 419 |
| 000200 | 32000 | 10 | q4 | 1 046 / 185 | 319 / 319 | 4 801 | 12 476 | 1 090 | 11 024 | — | — | — | — | — | — |

## Limites

- Mesure sur trois scans et trois tailles, pas une borne ; les parts par
  paire sont des estimations d'échantillon (2 000 paires par voie).
- Le front mesuré est celui de c5308651 (masque 6, défauts) ; la tranche 31
  non commise n'est pas jugée ici, et rien n'est transféré à ses sources.
- Les seeds q4 et leur census ne sont pas mesurés ; la couverture q4 l'est.
- Aucune confrontation à une vérité terrain (demande de l'utilisateur : on
  s'occupe du calcul du clustering, pas de sa pertinence).

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/audits/front_lanes_lidar_20260921/run_front_lanes.py read
git worktree add --detach /tmp/wt-c5308651 c5308651ba31aa7178e759b08fa2a593bb7fd6b1
cmake -S /tmp/wt-c5308651/morsehgp3D_v8 -B /tmp/wt-c5308651/build/v8-audit -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/wt-c5308651/build/v8-audit --parallel
python3 -B -O morsehgp3D_v8/audits/front_lanes_lidar_20260921/run_front_lanes.py run --worktree /tmp/wt-c5308651 --binary /tmp/front_lanes_probe --output /tmp/FRONT_LANES_CHECKS.json
```
