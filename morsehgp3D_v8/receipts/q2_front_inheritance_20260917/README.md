# Qualification des témoins hérités du front q2

17 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Flux q2 complet de supports, intérieurs stricts et coquilles complètes.
**Ni moteur q3/q4, ni HGP FULL, ni qualification G4. GCP non utilisé.**

La vingt-et-unième tranche porte dans `Front::filter` la
[transmission des témoins certifiés](../../docs/P0_TEMOINS_HERITES_Q2.md) : un
produit non rejeté passe à ses deux enfants les **rangs** de ses témoins
certifiés ; l'enfant part de ce compte et ne recompte jamais un rang reçu.
Option explicite `WspdFrontProposals::inherit_witnesses`, réservée à la voie
q2, dont le défaut reproduit le moteur de la tranche 20 compteur pour compteur.
Le dossier [cadrage](cadrage/README.md) archive les mesures jetables qui ont
choisi ce levier et écarté la reprise exacte de la descente.

## Captures propres (sources gelées le 17 septembre)

| Capture | Périmètre | État |
|---|---|---|
| [qualification_5o_33njk](qualification_5o_33njk/COMPLETION.json) | 81 CTests Release GCC 13.3 + porte explicite | PASS |
| [qualification_ya8wkiwt](qualification_ya8wkiwt/COMPLETION.json) | 81 CTests Clang 18 ASan/UBSan + porte explicite | PASS |
| [tsan_agii3c2x](tsan_agii3c2x/COMPLETION.json) | Portes héritage, dispatch et jobs sous Clang ThreadSanitizer | PASS |
| [smoke_atz8n45b](smoke_atz8n45b/COMPLETION.json) | 4 familles n32, K5, un et quatre workers | 56 mesures closes |
| [differential_8i6a6azx](differential_8i6a6azx/COMPLETION.json) | Build épinglé de la tranche 20, trois fenêtres sans héritage : 4 familles n8k/16k/32k, K10/s8 | 84 mesures closes |
| [scale_jzsscavz](scale_jzsscavz/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 1/3 | 168 mesures closes |
| [scale_l13sv57c](scale_l13sv57c/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 2/3 | 168 mesures closes |
| [scale_y44hby51](scale_y44hby51/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 3/3 | 168 mesures closes |
| [frontier_9i_xjhub](frontier_9i_xjhub/COMPLETION.json) | Uniforme n70k, K2 et K10 : rangs au-delà de 16 bits | 14 mesures closes |
| [separations_3xqbin7c](separations_3xqbin7c/COMPLETION.json) | 4 familles n8k, K5/10, s8/10/12, un worker | 140 mesures closes |
| [parallel_3pq0wb_6](parallel_3pq0wb_6/COMPLETION.json) | 4 familles n32k, K10/s8, un et quatre workers dans la même capture | 56 mesures closes |

Chaque configuration apparie la sonde parallèle Coarse du même build et six appels
de la sonde d'héritage : trois fenêtres (historique, 2K petits facteurs, 4K tous
produits), chacune sans puis avec héritage. Les 24 commandes de lecture et
d'analyse normal/−O sont closes dans [analysis_8ui2bes5](analysis_8ui2bes5/COMPLETION.json) ;
la [synthèse déterministe](analysis_8ui2bes5/SUMMARY.json) est identique dans les deux
modes : 854 mesures, 372 comparaisons appariées, 130 sources et
98 artefacts recontrôlés à la fermeture. Les captures d'échelle répètent tous
leurs compteurs à l'identique ; leurs temps sont donnés au minimum des répétitions.

La porte explicite (Release, ASan/UBSan et TSan) compare le moteur à un rejeu
indépendant du front q2 avec héritage sur 1 125 exécutions (1 010 472 produits,
compteurs et rectangles), à 12 lignes de compteurs d'un modèle Python indépendant, et
juge par force brute 711 237 paires rejetées sans perdre un support. Elle tue
10 mutants causaux de l'héritage (1 334 désaccords) ; les 4 mutants non sûrs
perdent 41 271 supports que le moteur conserve. 13 attendus exacts de trois
fixtures nommées de cinq points, 6 jeux de constantes du moteur d'avant les
options, 990 appels des cinq entrées q2 contre l'oracle force brute
(39 450 paires, 293 788 supports, coquille 24). Situations planchées :
127 175 listes pleines de Kmax − 1 rangs, 830 274 rangs reçus reproposés dont
111 173 dans l'extension, 16 809 rejets que la fenêtre seule n'aurait pas
obtenus contre 64 952 au compteur du moteur (majorant), 111 exécutions où le
nombre de produits rejetés **baisse** alors que la masse rejetée monte, et un nuage
de 320 sites où le plus grand rang reçu reproposé vaut 319.

## Le moteur sans héritage reproduit la tranche 20

La capture différentielle lance, hors du build mesuré, la sonde parallèle et la
sonde proposals du build **épinglé de la tranche 20**
(`build/v8_front_proposals_20260917`, empreintes égales à celles de ses propres
reçus), puis le nouveau moteur sans héritage, pour les trois fenêtres, sur
4 familles × n8k/16k/32k. Les 12 septuplets ont tous leurs champs discrets
identiques (front, extension, census, frère, ordre, conjoint, Pool, rappels, condensé,
plan de jobs). Seul change, pour toute option, le stockage des jobs : 544 puis
1 224 octets, la tâche du front passant de 32 à 72 octets. Temps du nouveau
moteur rapporté au build épinglé (observations uniques) : défaut ×0,974 à 1,029,
fenêtre 2K ×0,971 à 1,032, fenêtre 4K ×0,963 à 1,007.

## Temps mur du pipeline q2 complet

Front, census, collecte et callbacks ; K = 10, s = 8, Pool 64, un worker, minimum
des captures d'échelle. Chaque cellule donne le rapport à la référence sans puis
avec héritage et, entre parenthèses, le rapport de l'exécution avec héritage à
sa **jumelle** de même fenêtre : c'est lui qui isole l'héritage.

| Famille | n | Référence | Fenêtre historique | 2K petits facteurs | 4K tous produits |
|---|---:|---:|---|---|---|
| Uniforme | 8 000 | 5,18 s | ×0,97 → ×0,55 (×0,56) | ×0,45 → ×0,41 (×0,90) | ×0,44 → ×0,42 (×0,95) |
| Uniforme | 16 000 | 12,24 s | ×0,98 → ×0,53 (×0,55) | ×0,45 → ×0,40 (×0,89) | ×0,40 → ×0,38 (×0,96) |
| Uniforme | 32 000 | 29,03 s | ×0,97 → ×0,49 (×0,51) | ×0,42 → ×0,36 (×0,87) | ×0,38 → ×0,36 (×0,94) |
| Terrain | 8 000 | 0,92 s | ×0,99 → ×0,70 (×0,70) | ×0,65 → ×0,62 (×0,95) | ×0,65 → ×0,66 (×1,01) |
| Terrain | 16 000 | 1,95 s | ×0,99 → ×0,69 (×0,69) | ×0,63 → ×0,61 (×0,96) | ×0,66 → ×0,66 (×1,00) |
| Terrain | 32 000 | 4,24 s | ×0,99 → ×0,68 (×0,69) | ×0,64 → ×0,60 (×0,94) | ×0,63 → ×0,64 (×1,01) |
| Amas | 8 000 | 2,76 s | ×0,99 → ×0,68 (×0,69) | ×0,59 → ×0,56 (×0,95) | ×0,59 → ×0,58 (×0,98) |
| Amas | 16 000 | 7,71 s | ×0,98 → ×0,62 (×0,64) | ×0,54 → ×0,49 (×0,91) | ×0,50 → ×0,48 (×0,96) |
| Amas | 32 000 | 19,80 s | ×0,98 → ×0,56 (×0,58) | ×0,49 → ×0,44 (×0,89) | ×0,45 → ×0,44 (×0,97) |
| Rangées | 8 000 | 0,22 s | ×1,00 → ×0,99 (×0,99) | ×1,05 → ×1,06 (×1,01) | ×1,16 → ×1,18 (×1,02) |
| Rangées | 16 000 | 0,45 s | ×0,99 → ×0,99 (×0,99) | ×1,05 → ×1,05 (×1,00) | ×1,15 → ×1,17 (×1,02) |
| Rangées | 32 000 | 0,93 s | ×1,00 → ×1,00 (×1,00) | ×1,04 → ×1,05 (×1,00) | ×1,14 → ×1,16 (×1,01) |

K = 5, même grille :

| Famille | n | Référence | Fenêtre historique | 2K petits facteurs | 4K tous produits |
|---|---:|---:|---|---|---|
| Uniforme | 8 000 | 1,95 s | ×0,99 → ×0,61 (×0,62) | ×0,52 → ×0,47 (×0,90) | ×0,47 → ×0,45 (×0,95) |
| Uniforme | 16 000 | 4,58 s | ×0,98 → ×0,59 (×0,60) | ×0,50 → ×0,44 (×0,87) | ×0,46 → ×0,43 (×0,93) |
| Uniforme | 32 000 | 9,55 s | ×0,98 → ×0,58 (×0,60) | ×0,49 → ×0,43 (×0,88) | ×0,46 → ×0,43 (×0,93) |
| Terrain | 8 000 | 0,37 s | ×1,00 → ×0,75 (×0,75) | ×0,68 → ×0,65 (×0,95) | ×0,69 → ×0,67 (×0,98) |
| Terrain | 16 000 | 0,83 s | ×0,98 → ×0,72 (×0,74) | ×0,66 → ×0,62 (×0,94) | ×0,64 → ×0,63 (×0,98) |
| Terrain | 32 000 | 1,75 s | ×0,99 → ×0,73 (×0,73) | ×0,66 → ×0,62 (×0,94) | ×0,66 → ×0,64 (×0,97) |
| Amas | 8 000 | 1,25 s | ×0,98 → ×0,70 (×0,71) | ×0,61 → ×0,56 (×0,91) | ×0,57 → ×0,54 (×0,96) |
| Amas | 16 000 | 3,22 s | ×0,99 → ×0,65 (×0,65) | ×0,56 → ×0,51 (×0,90) | ×0,53 → ×0,49 (×0,94) |
| Amas | 32 000 | 7,30 s | ×0,99 → ×0,63 (×0,64) | ×0,54 → ×0,49 (×0,90) | ×0,52 → ×0,49 (×0,94) |
| Rangées | 8 000 | 0,12 s | ×1,01 → ×0,99 (×0,98) | ×1,03 → ×1,03 (×0,99) | ×1,10 → ×1,09 (×1,00) |
| Rangées | 16 000 | 0,25 s | ×0,99 → ×0,99 (×1,00) | ×1,02 → ×1,02 (×1,00) | ×1,08 → ×1,08 (×1,00) |
| Rangées | 32 000 | 0,51 s | ×1,01 → ×1,00 (×0,99) | ×1,04 → ×1,03 (×0,99) | ×1,09 → ×1,09 (×1,00) |

Toutes les comparaisons à 8 000 ≤ n ≤ 32 000 (échelle, séparations s8/10/12, un et
quatre workers), rapport de l'exécution avec héritage à sa jumelle, par famille :

- **Uniforme** (13 configurations) : fenêtre historique ×0,50 à 0,63 ; fenêtre 2K petits facteurs ×0,86 à 0,93 ; fenêtre 4K tous produits ×0,93 à 0,96.
- **Terrain** (13 configurations) : fenêtre historique ×0,68 à 0,75 ; fenêtre 2K petits facteurs ×0,89 à 0,96 ; fenêtre 4K tous produits ×0,93 à 1,11.
- **Amas** (13 configurations) : fenêtre historique ×0,57 à 0,72 ; fenêtre 2K petits facteurs ×0,88 à 0,96 ; fenêtre 4K tous produits ×0,94 à 0,99.
- **Rangées** (13 configurations) : fenêtre historique ×0,94 à 1,04 ; fenêtre 2K petits facteurs ×0,99 à 1,03 ; fenêtre 4K tous produits ×0,95 à 1,07.

Hors rangées, le temps q2 complet rapporté à la référence historique devient, avec
héritage : fenêtre historique ×0,49 à 0,75, 2K petits facteurs ×0,35 à 0,65, 4K
tous produits ×0,35 à 0,67. La fenêtre historique sans héritage de la sonde vaut
×0,948 à 1,051 de la référence : c'est le bruit de mesure de ces captures. Un et
quatre workers à 32k, fenêtre 2K petits facteurs, sans puis avec héritage :

| Famille | W1 sans | W1 avec | Rapport | W4 sans | W4 avec | Rapport |
|---|---:|---:|---:|---:|---:|---:|
| Uniforme | 12,15 s | 10,51 s | ×0,87 | 3,19 s | 2,74 s | ×0,86 |
| Terrain | 2,73 s | 2,55 s | ×0,94 | 0,75 s | 0,67 s | ×0,89 |
| Amas | 9,73 s | 8,68 s | ×0,89 | 2,57 s | 2,26 s | ×0,88 |
| Rangées | 0,98 s | 0,98 s | ×1,00 | 0,24 s | 0,25 s | ×1,01 |

## Ce qui change dans le travail

Candidates du census de l'exécution avec héritage, en part de sa jumelle ; les
supports, intérieurs et coquilles émis sont identiques dans toutes les variantes
(condensé canonique comparé dans chaque capture) :

- Uniforme : fenêtre historique 40,3 % à 52,9 % ; fenêtre 2K petits facteurs 75,9 % à 82,1 % ; fenêtre 4K tous produits 85,2 % à 89,0 %.
- Terrain : fenêtre historique 52,0 % à 60,5 % ; fenêtre 2K petits facteurs 83,8 % à 88,2 % ; fenêtre 4K tous produits 90,4 % à 92,7 %.
- Amas : fenêtre historique 46,0 % à 59,6 % ; fenêtre 2K petits facteurs 78,7 % à 84,5 % ; fenêtre 4K tous produits 87,4 % à 91,0 %.
- Rangées : fenêtre historique 100,0 % à 100,0 % ; fenêtre 2K petits facteurs 100,0 % à 100,0 % ; fenêtre 4K tous produits 100,0 % à 100,0 %.

Fenêtre 2K petits facteurs, autres travaux en part de la jumelle :

- Uniforme : produits visités 79,8 % à 85,5 %, tests H 61,0 % à 67,1 %, visites Z du census 77,5 % à 83,4 %.
- Terrain : produits visités 87,5 % à 91,1 %, tests H 66,9 % à 71,4 %, visites Z du census 86,6 % à 90,2 %.
- Amas : produits visités 83,0 % à 88,7 %, tests H 63,8 % à 70,3 %, visites Z du census 80,7 % à 85,9 %.
- Rangées : produits visités 100,0 % à 100,0 %, tests H 73,3 % à 80,6 %, visites Z du census 100,0 % à 100,0 %.

À uniforme 32k, K = 10, fenêtre 2K : 4 430 567 candidates deviennent
3 363 463, 13 324 576 produits visités deviennent 10 632 236, pour
1 180 529 supports ; 43 862 018 rangs reçus, dont 29 982 884
reproposés et sautés sans test. Croissance des visites Z du census, K = 10, fenêtre 2K :

| Famille | Sans héritage : ratios | Avec : 8k | 16k | 32k | Ratios |
|---|---|---:|---:|---:|---|
| Uniforme | ×2,444 / ×2,283 | 36 685 357 | 87 534 555 | 194 637 006 | ×2,386 / ×2,224 |
| Terrain | ×1,999 / ×2,280 | 7 190 411 | 14 588 960 | 32 841 999 | ×2,029 / ×2,251 |
| Amas | ×2,663 / ×2,489 | 26 161 324 | 67 496 322 | 164 665 479 | ×2,580 / ×2,440 |
| Rangées | ×2,070 / ×2,067 | 5 716 373 | 11 831 297 | 24 451 379 | ×2,070 / ×2,067 |

Le travail restant garde la croissance de la référence : l'héritage divise une
constante, il ne change pas l'exposant. **Aucune borne générale sous-quadratique,
P0 global ou tour FULL/G4 clos.**

## Rangs au-delà de 16 bits

Aucune porte CTest ne dépasse 320 sites. La campagne `frontier` (uniforme,
n = 70 000) est la seule où un rang reçu ne tient plus sur 16 bits ; le lecteur y
exige l'empreinte de supports de la jumelle sans héritage. Un moteur muté qui
stocke ses rangs sur 16 bits y perd des milliers de supports (mesure de cadrage
du constructeur, hors reçu) ; le moteur livré n'en perd aucun :

| K | Fenêtre | Supports | Candidates / jumelle | Temps / jumelle |
|---:|---|---:|---:|---:|
| 2 | historique | 538 671 | 68,7 % | ×0,80 |
| 2 | 2K petits facteurs | 538 671 | 88,2 % | ×0,94 |
| 2 | 4K tous produits | 538 671 | 92,0 % | ×0,96 |
| 10 | historique | 2 633 166 | 41,5 % | ×0,52 |
| 10 | 2K petits facteurs | 2 633 166 | 79,0 % | ×0,89 |
| 10 | 4K tous produits | 2 633 166 | 85,8 % | ×0,94 |

## Régression conservée et limites

- **Rangées** : à n = 8 000, K = 10, fenêtre 2K, 441 600 rangs sont reçus et
  94,5 % d'entre eux sont reproposés par la fenêtre de l'enfant ; les tests H
  tombent à 80,6 % de la jumelle, mais produits visités (158 894),
  produits rejetés et candidates (16 085 684) sont **identiques** : aucun produit
  supplémentaire ne tombe. Des témoins universels existent entre sites d'une même
  rangée, déjà trouvés par la fenêtre ; les candidates qui restent sont les paires
  entre rangées, sans témoin. Le compteur `inherited_rejections` y vaut
  3 382 alors que le contrefactuel vrai est nul : c'est un majorant.
- **Voie q2 seule** : l'option est refusée dès qu'une voie q3 ou q4 est active, et en
  mode `Pure` ; aucun juge indépendant des bornes Ξ n'existe encore.
- **Défaut inchangé** en compteurs ; la tâche du front pèse 72 octets au lieu de 32
  pour toute option.
- Le refus au-delà de 2^32 sites n'est jugé que sur son prédicat.
- Temps : hôte partagé, affinité libre, aucune compilation ni autre campagne
  pendant les captures ; seuls les compteurs sont déterministes. Aucun temps
  local ne qualifie un contrat G4.
- Les mesures d'audit antérieures (`audits/PROPAGATION_TEMOINS_20260914.md`, copie
  instrumentée d'un front plus ancien) ne sont pas héritées.

## Builds épinglés

`build/v8_front_inheritance_20260917`, `build/v8_front_inheritance_sanitize_20260917` et
`build/v8_front_inheritance_tsan_clang_20260917`. Ne pas les écraser. Les builds de
développement `build/v8_inherit_dev_20260917` et `build/v8_inherit_asan_dev_20260917`
ne portent aucune capture.

## Rejouer

Le [lanceur](../../bench/run_wspd_q2_inheritance_checks.py) fournit `read` pour chaque
capture et `run --campaign qualification|tsan|smoke|differential|scale|frontier|separations|parallel`
dans un build neuf (`--pinned-build` pour la campagne différentielle). `analyze.py`
lit des captures explicitement nommées ; `record_analysis.py` préserve ses lectures
normal/−O dans un dossier neuf.
