# Décision du banc v10 (PREREG_V10_KMATCH_A_20260928)

Calculée par `decide.py` ; aucun chiffre écrit à la main.

Banc v10 préenregistré PREREG_V10_KMATCH_A_20260928, 960 scènes de test, comparaison appariée K = min_samples. La tour bat HDBSCAN pour K=1, K=2, K=3, sans perte significative ailleurs.
- K=1 (tw_K1 contre hdb_ms1) : la tour bat HDBSCAN ; Δ = +0,053 [+0,046 ; +0,060], p_Holm = 3e-05, 157 victoires / 36 défaites / 767 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=2 (tw_K2 contre hdb_ms2) : la tour bat HDBSCAN ; Δ = +0,092 [+0,084 ; +0,100], p_Holm = 3e-05, 674 victoires / 283 défaites / 3 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=3 (tw_K3 contre hdb_ms3) : la tour bat HDBSCAN ; Δ = +0,049 [+0,043 ; +0,056], p_Holm = 3e-05, 603 victoires / 354 défaites / 3 égalités ; non-infériorité de la tour à la marge 0,02 établie.
Attribution, écrite avant le test : à K = 1, la hiérarchie de la tour est exactement celle de sklearn à min_samples = 1 (liaison simple, à un facteur d'échelle près selon alpha) ; tout écart à K = 1 vient donc de la tête (échelle ẑ, sélection, politique de bruit), pas de la géométrie. À K ≥ 2, les deux hiérarchies diffèrent (multicouverture exacte contre atteignabilité mutuelle) et les têtes aussi ; l'écart mesure le couple hiérarchie + tête.

## ARI_s moyen pondéré par cellule

| Méthode | ARI_s | Refus |
| --- | ---: | ---: |
| tw_K1 | 0,7751 | 0 |
| hdb_ms1 | 0,7220 | 0 |
| tw_K2 | 0,7642 | 0 |
| hdb_ms2 | 0,6719 | 0 |
| tw_K3 | 0,7550 | 0 |
| hdb_ms3 | 0,7056 | 0 |
| hdb_lib | 0,5139 | 0 |
| hdb_these_K1 | 0,6885 | 0 |

## K=1 : tw_K1 contre hdb_ms1

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,053 | [+0,046 ; +0,060] | 3e-05 |
| n = 8000 | +0,047 | [+0,033 ; +0,060] | 3e-05 |
| n = 16000 | +0,064 | [+0,052 ; +0,076] | 1e-05 |
| n = 32000 | +0,048 | [+0,036 ; +0,061] | 3e-05 |
| anisotropic | +0,092 | [+0,061 ; +0,125] | 8e-05 (Holm) |
| bridge | +0,108 | [+0,083 ; +0,130] | 8e-05 (Holm) |
| filaments | +0,060 | [+0,038 ; +0,084] | 8e-05 (Holm) |
| heteroscedastic | +0,004 | [+0,001 ; +0,007] | 0,023 (Holm) |
| hierarchical | -0,111 | [-0,119 ; -0,104] | 8e-05 (Holm) |
| shells | +0,000 | [+0,000 ; +0,000] | 1 (Holm) |
| spherical | +0,271 | [+0,235 ; +0,306] | 8e-05 (Holm) |
| unbalanced | +0,001 | [+0,000 ; +0,003] | 1 (Holm) |

## K=2 : tw_K2 contre hdb_ms2

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,092 | [+0,084 ; +0,100] | 3e-05 |
| n = 8000 | +0,074 | [+0,060 ; +0,088] | 1e-05 |
| n = 16000 | +0,098 | [+0,085 ; +0,111] | 1e-05 |
| n = 32000 | +0,105 | [+0,091 ; +0,118] | 1e-05 |
| anisotropic | -0,021 | [-0,047 ; +0,003] | 0,37 (Holm) |
| bridge | -0,106 | [-0,132 ; -0,079] | 8e-05 (Holm) |
| filaments | +0,118 | [+0,091 ; +0,146] | 8e-05 (Holm) |
| heteroscedastic | +0,001 | [-0,005 ; +0,006] | 0,86 (Holm) |
| hierarchical | +0,067 | [+0,060 ; +0,074] | 8e-05 (Holm) |
| shells | +0,568 | [+0,562 ; +0,574] | 8e-05 (Holm) |
| spherical | -0,048 | [-0,077 ; -0,021] | 0,028 (Holm) |
| unbalanced | +0,159 | [+0,129 ; +0,191] | 8e-05 (Holm) |

## K=3 : tw_K3 contre hdb_ms3

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,049 | [+0,043 ; +0,056] | 3e-05 |
| n = 8000 | +0,028 | [+0,016 ; +0,040] | 0,041 |
| n = 16000 | +0,049 | [+0,039 ; +0,060] | 0,00071 |
| n = 32000 | +0,071 | [+0,057 ; +0,085] | 1e-05 |
| anisotropic | -0,068 | [-0,092 ; -0,047] | 8e-05 (Holm) |
| bridge | -0,135 | [-0,154 ; -0,115] | 8e-05 (Holm) |
| filaments | +0,053 | [+0,024 ; +0,082] | 0,028 (Holm) |
| heteroscedastic | -0,006 | [-0,010 ; -0,002] | 0,028 (Holm) |
| hierarchical | +0,088 | [+0,081 ; +0,094] | 8e-05 (Holm) |
| shells | +0,522 | [+0,516 ; +0,528] | 8e-05 (Holm) |
| spherical | -0,123 | [-0,153 ; -0,091] | 8e-05 (Holm) |
| unbalanced | +0,065 | [+0,041 ; +0,089] | 8e-05 (Holm) |

## défaut sklearn : tw_K1 contre hdb_lib (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,261 | [+0,253 ; +0,270] | 1e-05 |
| n = 8000 | +0,234 | [+0,218 ; +0,249] | 1e-05 |
| n = 16000 | +0,270 | [+0,256 ; +0,283] | 1e-05 |
| n = 32000 | +0,281 | [+0,266 ; +0,295] | 1e-05 |
| anisotropic | +0,404 | [+0,366 ; +0,441] | 8e-05 (Holm) |
| bridge | +0,479 | [+0,440 ; +0,516] | 8e-05 (Holm) |
| filaments | +0,191 | [+0,160 ; +0,223] | 8e-05 (Holm) |
| heteroscedastic | +0,136 | [+0,124 ; +0,147] | 8e-05 (Holm) |
| hierarchical | -0,113 | [-0,121 ; -0,105] | 8e-05 (Holm) |
| shells | -0,005 | [-0,005 ; -0,004] | 8e-05 (Holm) |
| spherical | +0,544 | [+0,529 ; +0,559] | 8e-05 (Holm) |
| unbalanced | +0,454 | [+0,441 ; +0,468] | 8e-05 (Holm) |

## protocole de la thèse : tw_K1 contre hdb_these_K1 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,087 | [+0,080 ; +0,093] | 1e-05 |
| n = 8000 | +0,082 | [+0,071 ; +0,094] | 1e-05 |
| n = 16000 | +0,094 | [+0,083 ; +0,104] | 1e-05 |
| n = 32000 | +0,084 | [+0,071 ; +0,097] | 1e-05 |
| anisotropic | +0,134 | [+0,109 ; +0,160] | 8e-05 (Holm) |
| bridge | +0,164 | [+0,137 ; +0,189] | 8e-05 (Holm) |
| filaments | +0,081 | [+0,060 ; +0,104] | 8e-05 (Holm) |
| heteroscedastic | +0,024 | [+0,019 ; +0,029] | 8e-05 (Holm) |
| hierarchical | -0,110 | [-0,118 ; -0,103] | 8e-05 (Holm) |
| shells | -0,003 | [-0,003 ; -0,002] | 8e-05 (Holm) |
| spherical | +0,312 | [+0,281 ; +0,342] | 8e-05 (Holm) |
| unbalanced | +0,091 | [+0,082 ; +0,100] | 8e-05 (Holm) |
