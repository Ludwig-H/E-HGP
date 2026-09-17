# Qualification de la fenêtre de propositions élargie du front q2

17 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Flux q2 complet de supports, intérieurs stricts et coquilles complètes.
**Ni moteur q3/q4, ni HGP FULL, ni qualification G4. GCP non utilisé.**

La vingtième tranche porte dans `Front::filter` la
[surproposition de témoins](../../docs/P0_SURPROPOSITION_TEMOINS_Q2.md) : le seuil
de rejet reste Kmax, seul le nombre de rangs proposés change. Option explicite
`WspdFrontProposals` (facteur de fenêtre 1, 2 ou 4 ; limite « petits facteurs »),
réservée à la voie q2, dont le défaut reproduit le front historique à l'unité.

## Captures propres (sources gelées le 17 septembre)

| Capture | Périmètre | État |
|---|---|---|
| [qualification_6a82u7pz](qualification_6a82u7pz/COMPLETION.json) | 78 CTests Release GCC 13.3 + porte explicite | PASS |
| [qualification_fhtxvo26](qualification_fhtxvo26/COMPLETION.json) | 78 CTests Clang 18 ASan/UBSan + porte explicite | PASS |
| [tsan_jgzwyjuf](tsan_jgzwyjuf/COMPLETION.json) | Portes proposals et dispatch sous Clang ThreadSanitizer | PASS |
| [smoke_nn_y26pi](smoke_nn_y26pi/COMPLETION.json) | 4 familles n32, K5, un et quatre workers | 48 mesures closes |
| [differential_dul4w6yj](differential_dul4w6yj/COMPLETION.json) | Build épinglé de la tranche 19, référence et facteur 1 : 4 familles n8k/16k/32k, K10/s8 | 36 mesures closes |
| [scale_ijxv9clc](scale_ijxv9clc/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 1/3 | 144 mesures closes |
| [scale__pc3s6bx](scale__pc3s6bx/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 2/3 | 144 mesures closes |
| [scale_wgblrm2_](scale_wgblrm2_/COMPLETION.json) | 4 familles n8k/16k/32k, K5 et K10, s8, Pool64, un worker, capture 3/3 | 144 mesures closes |
| [separations_xcy1bk7b](separations_xcy1bk7b/COMPLETION.json) | 4 familles n8k, K5/10, s8/10/12, un worker | 120 mesures closes |
| [parallel_iwuhx1k1](parallel_iwuhx1k1/COMPLETION.json) | 4 familles n32k, K10/s8, un et quatre workers dans la même capture | 48 mesures closes |

Chaque configuration apparie la sonde parallèle Coarse du même build (proposals par
défaut) et cinq appels de la sonde proposals : facteur 1, puis 2K et 4K avec la
limite 16 ou sans limite. Les 22 commandes de lecture et d'analyse normal/−O sont closes
dans [analysis_33ezki5_](analysis_33ezki5_/COMPLETION.json) ; la
[synthèse déterministe](analysis_33ezki5_/SUMMARY.json) est identique dans les deux
modes : 684 mesures, 300 comparaisons appariées, 125 sources et
93 artefacts recontrôlés à la fermeture. Les trois captures d'échelle répètent
tous leurs compteurs à l'identique ; leurs temps sont donnés au minimum des trois.

La porte explicite (Release, ASan/UBSan et TSan) juge 35 280 fenêtres par un oracle
arithmétique, compare le moteur à un rejeu indépendant du front q2 sur 1 575
exécutions (1 469 453 produits, compteurs et rectangles), tue 10 mutants causaux
de l'extension (5 345 désaccords), vérifie 8 attendus exacts de deux fixtures
minimales nommées et 6 jeux de constantes du moteur d'avant la tranche, puis compare
990 appels des cinq entrées q2 à l'oracle force brute (38 487 paires,
2 711 256 tests de sites, 282 656 supports, coquille 24). Situations de bord
exercées et planchées : 120 338 tangences H = 0 sur produits singleton dans
l'extension, 503 727 rangs de A ou B sautés, 89 842 et 48 352 fenêtres
butées à gauche et à droite, 1 048 fenêtres 2K tronquées par n, 617 949
extensions épuisées sans K succès, 69 327 K-ièmes témoins trouvés par la seule
extension sur produit singleton.

## Le défaut reproduit le moteur d'avant la tranche

La capture différentielle lance la sonde parallèle du build **épinglé de la tranche 19**
(`build/v8_singleton_batch_r3_20260917`, extérieur au build mesuré), la sonde parallèle
du nouveau build et la sonde proposals au facteur 1, sur 4 familles × n8k/16k/32k :
les 12 triplets ont tous leurs champs discrets identiques (front, census, frère, ordre,
Pool, condensé). Temps du nouveau défaut rapporté au build épinglé : ×0,978 à 1,019 pour la
sonde parallèle, ×0,992 à 1,031 pour la sonde proposals au facteur 1 (observations
uniques). La porte du front grave en plus des constantes à trois voies du moteur
d'avant la tranche, et la porte proposals des constantes q2.

## Temps mur du pipeline q2 complet

Front, census, collecte et callbacks ; K = 10, s = 8, Pool 64, un worker, minimum de
trois captures. Colonnes : temps de référence (fenêtre historique), puis rapport
variante / référence ; les trois dernières colonnes donnent, pour 2K petits
facteurs, les candidats du census, les visites Z et les propositions du front en
part de la référence.

| Famille | n | Référence | 2K / 16 | 2K / tous | 4K / 16 | 4K / tous | Candidats | Visites Z | Propositions |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Uniforme | 8 000 | 5,00 s | ×0,46 | ×0,46 | ×0,46 | ×0,45 | 26,5 % | 26,2 % | 89,5 % |
| Uniforme | 16 000 | 11,83 s | ×0,46 | ×0,46 | ×0,42 | ×0,41 | 27,3 % | 26,6 % | 89,3 % |
| Uniforme | 32 000 | 27,95 s | ×0,43 | ×0,43 | ×0,40 | ×0,39 | 25,6 % | 25,1 % | 86,8 % |
| Terrain | 8 000 | 0,91 s | ×0,67 | ×0,66 | ×0,67 | ×0,67 | 36,6 % | 40,6 % | 118,9 % |
| Terrain | 16 000 | 1,90 s | ×0,64 | ×0,63 | ×0,69 | ×0,68 | 34,2 % | 38,8 % | 114,2 % |
| Terrain | 32 000 | 4,14 s | ×0,64 | ×0,64 | ×0,65 | ×0,65 | 35,1 % | 39,8 % | 116,2 % |
| Amas | 8 000 | 2,69 s | ×0,61 | ×0,61 | ×0,61 | ×0,61 | 36,3 % | 38,0 % | 108,5 % |
| Amas | 16 000 | 7,47 s | ×0,55 | ×0,55 | ×0,51 | ×0,51 | 33,3 % | 34,2 % | 101,7 % |
| Amas | 32 000 | 19,15 s | ×0,51 | ×0,50 | ×0,48 | ×0,47 | 30,5 % | 31,5 % | 96,1 % |
| Rangées | 8 000 | 0,21 s | ×1,06 | ×1,06 | ×1,17 | ×1,17 | 100,0 % | 99,5 % | 194,3 % |
| Rangées | 16 000 | 0,44 s | ×1,07 | ×1,07 | ×1,18 | ×1,18 | 100,0 % | 99,5 % | 194,3 % |
| Rangées | 32 000 | 0,92 s | ×1,07 | ×1,06 | ×1,17 | ×1,17 | 100,0 % | 99,5 % | 194,2 % |

K = 5, même grille :

| Famille | n | Référence | 2K / 16 | 2K / tous | 4K / 16 | 4K / tous | Candidats | Visites Z | Propositions |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Uniforme | 8 000 | 1,92 s | ×0,53 | ×0,53 | ×0,48 | ×0,48 | 32,8 % | 33,5 % | 96,6 % |
| Uniforme | 16 000 | 4,48 s | ×0,52 | ×0,51 | ×0,47 | ×0,46 | 31,5 % | 32,2 % | 93,8 % |
| Uniforme | 32 000 | 9,31 s | ×0,50 | ×0,49 | ×0,48 | ×0,48 | 29,6 % | 30,5 % | 91,4 % |
| Terrain | 8 000 | 0,37 s | ×0,69 | ×0,68 | ×0,71 | ×0,70 | 39,9 % | 47,0 % | 116,4 % |
| Terrain | 16 000 | 0,81 s | ×0,68 | ×0,67 | ×0,67 | ×0,66 | 39,7 % | 46,8 % | 115,9 % |
| Terrain | 32 000 | 1,72 s | ×0,67 | ×0,67 | ×0,68 | ×0,67 | 38,6 % | 46,1 % | 113,8 % |
| Amas | 8 000 | 1,23 s | ×0,62 | ×0,61 | ×0,58 | ×0,58 | 39,1 % | 41,4 % | 108,5 % |
| Amas | 16 000 | 3,14 s | ×0,58 | ×0,57 | ×0,53 | ×0,53 | 36,0 % | 38,2 % | 102,2 % |
| Amas | 32 000 | 7,20 s | ×0,55 | ×0,54 | ×0,53 | ×0,52 | 33,7 % | 36,4 % | 97,5 % |
| Rangées | 8 000 | 0,12 s | ×1,04 | ×1,04 | ×1,11 | ×1,11 | 100,0 % | 100,0 % | 184,6 % |
| Rangées | 16 000 | 0,25 s | ×1,04 | ×1,04 | ×1,10 | ×1,10 | 100,0 % | 100,0 % | 184,6 % |
| Rangées | 32 000 | 0,51 s | ×1,04 | ×1,04 | ×1,10 | ×1,10 | 100,0 % | 100,0 % | 184,6 % |

Toutes les comparaisons à n ≥ 8 000 (échelle, séparations s8/10/12, un et quatre
workers), rapport variante / référence par famille :

- **Uniforme** (13 configurations à n ≥ 8 000) : 2K, petits facteurs ×0,42 à 0,54 ; 2K, tous produits ×0,42 à 0,53 ; 4K, petits facteurs ×0,39 à 0,50 ; 4K, tous produits ×0,38 à 0,49.
- **Terrain** (13 configurations à n ≥ 8 000) : 2K, petits facteurs ×0,64 à 0,71 ; 2K, tous produits ×0,63 à 0,68 ; 4K, petits facteurs ×0,65 à 0,71 ; 4K, tous produits ×0,64 à 0,70.
- **Amas** (13 configurations à n ≥ 8 000) : 2K, petits facteurs ×0,51 à 0,62 ; 2K, tous produits ×0,50 à 0,63 ; 4K, petits facteurs ×0,47 à 0,61 ; 4K, tous produits ×0,47 à 0,61.
- **Rangées** (13 configurations à n ≥ 8 000) : 2K, petits facteurs ×1,01 à 1,18 ; 2K, tous produits ×1,01 à 1,09 ; 4K, petits facteurs ×1,09 à 1,21 ; 4K, tous produits ×1,09 à 1,20.

Le facteur 1 de la sonde proposals vaut ×0,957 à 1,024 de la référence : c'est le bruit de
mesure de ces captures. Un et quatre workers à 32k, fenêtre 2K petits facteurs :

| Famille | Référence W1 | 2K / 16 W1 | Référence W4 | 2K / 16 W4 |
|---|---:|---:|---:|---:|
| Uniforme | 28,10 s | ×0,43 | 7,60 s | ×0,42 |
| Terrain | 4,15 s | ×0,65 | 1,09 s | ×0,64 |
| Amas | 19,20 s | ×0,51 | 5,03 s | ×0,52 |
| Rangées | 0,93 s | ×1,06 | 0,25 s | ×1,18 |

## Ce qui change dans le travail

Les candidats du census baissent parce que des produits sont rejetés plus haut dans
le front ; les supports, intérieurs et coquilles émis sont identiques dans toutes
les variantes (condensé canonique comparé dans chaque capture) :

- Uniforme : fenêtre 2K 25,6 % à 33,4 % des candidats de référence, fenêtre 4K 16,6 % à 22,8 %.
- Terrain : fenêtre 2K 34,2 % à 39,9 % des candidats de référence, fenêtre 4K 25,2 % à 32,3 %.
- Amas : fenêtre 2K 30,5 % à 39,4 % des candidats de référence, fenêtre 4K 20,5 % à 27,8 %.
- Rangées : fenêtre 2K 100,0 % à 100,0 % des candidats de référence, fenêtre 4K 100,0 % à 100,0 %.

Les politiques « petits facteurs » (limite 16) et « tous produits » diffèrent de
moins de 0,11 point sur les candidats. Croissance des visites Z du census, K = 10 :

| Famille | Référence : ratios | 2K / 16 : 8k | 16k | 32k | Ratios |
|---|---|---:|---:|---:|---|
| Uniforme | ×2,406 / ×2,421 | 45 020 340 | 110 046 283 | 251 280 566 | ×2,444 / ×2,283 |
| Terrain | ×2,091 / ×2,223 | 8 305 673 | 16 606 802 | 37 869 372 | ×1,999 / ×2,280 |
| Amas | ×2,958 / ×2,701 | 30 782 332 | 81 968 710 | 203 988 224 | ×2,663 / ×2,489 |
| Rangées | ×2,070 / ×2,067 | 5 716 373 | 11 831 297 | 24 451 379 | ×2,070 / ×2,067 |

Le travail restant garde la même croissance que la référence : la fenêtre élargie
divise une constante, elle ne change pas l'exposant. **Aucune borne générale
sous-quadratique, P0 global ou tour FULL/G4 clos.**

## Régression conservée et limites

- **Rangées** : deux rangées parallèles n'ont presque aucun témoin universel ; la
  fenêtre élargie y propose sans rejeter et coûte le temps indiqué ci-dessus. C'est
  le régime de surcoût pur annoncé par la note ; il est publié, pas masqué.
- **Voie q2 seule** : un facteur différent de 1 est refusé dès qu'une voie q3 ou q4
  est active ; aucun juge ne couvre une fenêtre élargie pour ces voies.
- **Défaut inchangé** : `window_factor = 1`. Les sondes et reçus antérieurs restent
  comparables ; aucune entrée ne choisit 2K à la place de l'appelant.
- Temps : hôte partagé, affinité libre, aucune compilation ni autre campagne
  pendant les captures ; seuls les compteurs sont déterministes. Aucun temps
  local ne qualifie un contrat G4.
- Les mesures d'audit antérieures (`audits/surproposition_20260915/`, copie patchée)
  ne sont pas héritées ; elles concordent avec ces captures sur les compteurs.

## Builds épinglés

`build/v8_front_proposals_20260917`, `build/v8_front_proposals_sanitize_20260917` et
`build/v8_front_proposals_tsan_clang_20260917`. Ne pas les écraser. Le build de
développement `build/v8_proposals_dev_20260917` ne porte aucune capture.

## Rejouer

Le [lanceur](../../bench/run_wspd_q2_proposals_checks.py) fournit `read` pour chaque
capture et `run --campaign qualification|tsan|smoke|differential|scale|separations|parallel`
dans un build neuf (`--pinned-build` pour la campagne différentielle). `analyze.py`
lit des captures explicitement nommées ; `record_analysis.py` préserve ses lectures
normal/−O dans un dossier neuf.
