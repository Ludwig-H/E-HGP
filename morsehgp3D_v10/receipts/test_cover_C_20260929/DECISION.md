# Décision du banc v10 (PREREG_V10_COVER_C_20260929)

Calculée par `decide.py` ; aucun chiffre écrit à la main.

Banc v10 préenregistré PREREG_V10_COVER_C_20260929, 960 scènes de test, comparaison appariée K = min_samples. La tour bat HDBSCAN pour K=1, K=2, K=3, K=5, K=8, K=10, sans perte significative ailleurs.
- K=1 (tw_K1 contre hdb_ms1) : la tour bat HDBSCAN ; Δ = +0,075 [+0,067 ; +0,083], p_Holm = 6e-05, 559 victoires / 326 défaites / 75 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=2 (tw_K2 contre hdb_ms2) : la tour bat HDBSCAN ; Δ = +0,073 [+0,066 ; +0,081], p_Holm = 6e-05, 595 victoires / 287 défaites / 78 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=3 (tw_K3 contre hdb_ms3) : la tour bat HDBSCAN ; Δ = +0,091 [+0,087 ; +0,096], p_Holm = 6e-05, 673 victoires / 287 défaites / 0 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=5 (tw_K5 contre hdb_ms5) : la tour bat HDBSCAN ; Δ = +0,064 [+0,060 ; +0,067], p_Holm = 6e-05, 639 victoires / 315 défaites / 6 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=8 (tw_K8 contre hdb_ms8) : la tour bat HDBSCAN ; Δ = +0,040 [+0,037 ; +0,043], p_Holm = 6e-05, 455 victoires / 501 défaites / 4 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- K=10 (tw_K10 contre hdb_ms10) : la tour bat HDBSCAN ; Δ = +0,031 [+0,028 ; +0,034], p_Holm = 6e-05, 454 victoires / 498 défaites / 8 égalités ; non-infériorité de la tour à la marge 0,02 établie.

Famille secondaire « lot B, tête C∩X du 28 septembre » (Holm dans la famille ; sans effet sur la décision principale) :
- lot B, K=5 (cap_K5 contre hdb_ms5) : pas de différence significative ; Δ = +0,011 [+0,006 ; +0,017], p_Holm = 0,18, 432 victoires / 526 défaites / 2 égalités ; non-infériorité de la tour (C∩X) à la marge 0,02 établie.
- lot B, K=8 (cap_K8 contre hdb_ms8) : HDBSCAN bat la tour (C∩X) ; Δ = -0,025 [-0,027 ; -0,022], p_Holm = 3e-05, 427 victoires / 526 défaites / 7 égalités ; non-infériorité de la tour (C∩X) non établie.
- lot B, K=10 (cap_K10 contre hdb_ms10) : HDBSCAN bat la tour (C∩X) ; Δ = -0,034 [-0,036 ; -0,031], p_Holm = 3e-05, 295 victoires / 664 défaites / 1 égalités ; non-infériorité de la tour (C∩X) non établie.

Famille secondaire « objet exact contre hiérarchie d’HDBSCAN, même entrée et même tête » (Holm dans la famille ; sans effet sur la décision principale) :
- objet, K=2 (tw_K2 contre mrb_K2) : pas de différence significative ; Δ = -0,001 [-0,004 ; +0,003], p_Holm = 1, 443 victoires / 424 défaites / 93 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- objet, K=3 (tw_K3 contre mrb_K3) : pas de différence significative ; Δ = -0,000 [-0,003 ; +0,003], p_Holm = 1, 433 victoires / 437 défaites / 90 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- objet, K=5 (tw_K5 contre mrb_K5) : pas de différence significative ; Δ = +0,001 [-0,001 ; +0,004], p_Holm = 0,95, 447 victoires / 427 défaites / 86 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- objet, K=8 (tw_K8 contre mrb_K8) : pas de différence significative ; Δ = +0,000 [-0,002 ; +0,002], p_Holm = 1, 358 victoires / 534 défaites / 68 égalités ; non-infériorité de la tour à la marge 0,02 établie.
- objet, K=10 (tw_K10 contre mrb_K10) : avantage significatif à la tour, sous la marge ou non uniforme en taille ; Δ = +0,010 [+0,006 ; +0,014], p_Holm = 0,0016, 372 victoires / 538 défaites / 50 égalités ; non-infériorité de la tour à la marge 0,02 établie.
Attribution, écrite avant le test : à K = 1, les hiérarchies de la tour et de sklearn coïncident (liaison simple) ; l’écart y vient entièrement de la tête (z, politique de bruit). À K ≥ 2, la paire principale mesure ensemble la hiérarchie, l’entrée des points et la tête ; la famille « objet » isole la hiérarchie (même entrée, même tête, même remplissage) : une parité y signifierait que l’avantage sur HDBSCAN vient de l’entrée et de la tête, applicables aussi à la hiérarchie d’HDBSCAN.

## ARI_s moyen pondéré par cellule

| Méthode | ARI_s | Refus |
| --- | ---: | ---: |
| tw_K1 | 0,7952 | 0 |
| hdb_ms1 | 0,7202 | 0 |
| tw_K1_nf | 0,7308 | 0 |
| hdb_ms1_nf | 0,6968 | 0 |
| tw_K2 | 0,7932 | 0 |
| hdb_ms2 | 0,7202 | 0 |
| tw_K2_nf | 0,7380 | 0 |
| hdb_ms2_nf | 0,6968 | 0 |
| tw_K3 | 0,7966 | 0 |
| hdb_ms3 | 0,7053 | 0 |
| tw_K3_nf | 0,7365 | 0 |
| hdb_ms3_nf | 0,6876 | 0 |
| tw_K5 | 0,8001 | 0 |
| hdb_ms5 | 0,7366 | 0 |
| tw_K5_nf | 0,7521 | 0 |
| hdb_ms5_nf | 0,6747 | 0 |
| tw_K8 | 0,7960 | 0 |
| hdb_ms8 | 0,7560 | 0 |
| tw_K8_nf | 0,7602 | 0 |
| hdb_ms8_nf | 0,6606 | 0 |
| tw_K10 | 0,7957 | 0 |
| hdb_ms10 | 0,7647 | 0 |
| tw_K10_nf | 0,7647 | 0 |
| hdb_ms10_nf | 0,6593 | 0 |
| mrb_K2 | 0,7937 | 0 |
| mrb_K3 | 0,7970 | 0 |
| mrb_K5 | 0,7986 | 0 |
| mrb_K8 | 0,7958 | 0 |
| mrb_K10 | 0,7859 | 0 |
| cap_K5 | 0,7479 | 0 |
| cap_K8 | 0,7314 | 0 |
| cap_K10 | 0,7312 | 0 |

## K=1 : tw_K1 contre hdb_ms1

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,075 | [+0,067 ; +0,083] | 6e-05 |
| n = 8000 | +0,071 | [+0,056 ; +0,087] | 1e-05 |
| n = 16000 | +0,070 | [+0,057 ; +0,083] | 1e-05 |
| n = 32000 | +0,084 | [+0,073 ; +0,095] | 1e-05 |
| anisotropic | +0,149 | [+0,116 ; +0,184] | 8e-05 (Holm) |
| bridge | +0,174 | [+0,168 ; +0,190] | 8e-05 (Holm) |
| filaments | +0,105 | [+0,075 ; +0,135] | 8e-05 (Holm) |
| heteroscedastic | -0,004 | [-0,008 ; -0,000] | 0,04 (Holm) |
| hierarchical | -0,127 | [-0,128 ; -0,126] | 8e-05 (Holm) |
| shells | -0,011 | [-0,015 ; -0,006] | 8e-05 (Holm) |
| spherical | +0,302 | [+0,265 ; +0,341] | 8e-05 (Holm) |
| unbalanced | +0,012 | [+0,007 ; +0,018] | 8e-05 (Holm) |

## K=2 : tw_K2 contre hdb_ms2

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,073 | [+0,066 ; +0,081] | 6e-05 |
| n = 8000 | +0,074 | [+0,059 ; +0,090] | 1e-05 |
| n = 16000 | +0,067 | [+0,055 ; +0,079] | 1e-05 |
| n = 32000 | +0,078 | [+0,067 ; +0,089] | 1e-05 |
| anisotropic | +0,139 | [+0,105 ; +0,176] | 8e-05 (Holm) |
| bridge | +0,176 | [+0,169 ; +0,191] | 8e-05 (Holm) |
| filaments | +0,084 | [+0,059 ; +0,110] | 8e-05 (Holm) |
| heteroscedastic | -0,006 | [-0,011 ; -0,001] | 0,064 (Holm) |
| hierarchical | -0,124 | [-0,126 ; -0,122] | 8e-05 (Holm) |
| shells | -0,003 | [-0,005 ; -0,000] | 0,0055 (Holm) |
| spherical | +0,301 | [+0,264 ; +0,341] | 8e-05 (Holm) |
| unbalanced | +0,016 | [+0,007 ; +0,026] | 0,0017 (Holm) |

## K=3 : tw_K3 contre hdb_ms3

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,091 | [+0,087 ; +0,096] | 6e-05 |
| n = 8000 | +0,063 | [+0,056 ; +0,071] | 1e-05 |
| n = 16000 | +0,090 | [+0,083 ; +0,097] | 1e-05 |
| n = 32000 | +0,120 | [+0,112 ; +0,129] | 1e-05 |
| anisotropic | -0,011 | [-0,021 ; -0,001] | 0,23 (Holm) |
| bridge | +0,002 | [+0,002 ; +0,003] | 0,089 (Holm) |
| filaments | +0,102 | [+0,081 ; +0,123] | 8e-05 (Holm) |
| heteroscedastic | +0,001 | [-0,004 ; +0,006] | 0,71 (Holm) |
| hierarchical | +0,006 | [+0,004 ; +0,007] | 8e-05 (Holm) |
| shells | +0,514 | [+0,507 ; +0,520] | 8e-05 (Holm) |
| spherical | +0,003 | [+0,001 ; +0,005] | 0,23 (Holm) |
| unbalanced | +0,114 | [+0,089 ; +0,139] | 8e-05 (Holm) |

## K=5 : tw_K5 contre hdb_ms5

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,064 | [+0,060 ; +0,067] | 6e-05 |
| n = 8000 | +0,041 | [+0,035 ; +0,047] | 1e-05 |
| n = 16000 | +0,065 | [+0,060 ; +0,070] | 1e-05 |
| n = 32000 | +0,085 | [+0,078 ; +0,091] | 1e-05 |
| anisotropic | -0,018 | [-0,025 ; -0,012] | 8e-05 (Holm) |
| bridge | +0,000 | [-0,000 ; +0,001] | 1 (Holm) |
| filaments | +0,025 | [+0,008 ; +0,043] | 0,15 (Holm) |
| heteroscedastic | +0,016 | [+0,011 ; +0,020] | 0,0001 (Holm) |
| hierarchical | +0,000 | [-0,001 ; +0,002] | 1 (Holm) |
| shells | +0,416 | [+0,406 ; +0,425] | 8e-05 (Holm) |
| spherical | -0,000 | [-0,001 ; +0,001] | 1 (Holm) |
| unbalanced | +0,069 | [+0,052 ; +0,086] | 8e-05 (Holm) |

## K=8 : tw_K8 contre hdb_ms8

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,040 | [+0,037 ; +0,043] | 6e-05 |
| n = 8000 | +0,020 | [+0,015 ; +0,025] | 2e-05 |
| n = 16000 | +0,042 | [+0,038 ; +0,046] | 1e-05 |
| n = 32000 | +0,058 | [+0,052 ; +0,064] | 1e-05 |
| anisotropic | -0,038 | [-0,046 ; -0,031] | 8e-05 (Holm) |
| bridge | -0,008 | [-0,009 ; -0,007] | 8e-05 (Holm) |
| filaments | -0,032 | [-0,049 ; -0,015] | 0,0023 (Holm) |
| heteroscedastic | +0,023 | [+0,020 ; +0,027] | 8e-05 (Holm) |
| hierarchical | -0,007 | [-0,008 ; -0,006] | 8e-05 (Holm) |
| shells | +0,326 | [+0,317 ; +0,336] | 8e-05 (Holm) |
| spherical | -0,008 | [-0,009 ; -0,007] | 8e-05 (Holm) |
| unbalanced | +0,063 | [+0,053 ; +0,073] | 8e-05 (Holm) |

## K=10 : tw_K10 contre hdb_ms10

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,031 | [+0,028 ; +0,034] | 6e-05 |
| n = 8000 | +0,012 | [+0,007 ; +0,017] | 0,0077 |
| n = 16000 | +0,032 | [+0,028 ; +0,037] | 1e-05 |
| n = 32000 | +0,049 | [+0,044 ; +0,053] | 1e-05 |
| anisotropic | -0,040 | [-0,048 ; -0,033] | 8e-05 (Holm) |
| bridge | -0,008 | [-0,008 ; -0,007] | 8e-05 (Holm) |
| filaments | -0,054 | [-0,068 ; -0,040] | 8e-05 (Holm) |
| heteroscedastic | +0,023 | [+0,020 ; +0,027] | 8e-05 (Holm) |
| hierarchical | -0,008 | [-0,010 ; -0,006] | 8e-05 (Holm) |
| shells | +0,273 | [+0,263 ; +0,284] | 8e-05 (Holm) |
| spherical | -0,009 | [-0,010 ; -0,008] | 8e-05 (Holm) |
| unbalanced | +0,070 | [+0,060 ; +0,080] | 8e-05 (Holm) |

## lot B, K=5 : cap_K5 contre hdb_ms5 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,011 | [+0,006 ; +0,017] | 0,18 |
| n = 8000 | -0,007 | [-0,018 ; +0,004] | 0,57 |
| n = 16000 | +0,012 | [+0,002 ; +0,022] | 0,39 |
| n = 32000 | +0,029 | [+0,022 ; +0,036] | 0,067 |
| anisotropic | -0,097 | [-0,119 ; -0,077] | 8e-05 (Holm) |
| bridge | -0,160 | [-0,166 ; -0,146] | 8e-05 (Holm) |
| filaments | -0,059 | [-0,083 ; -0,037] | 0,0078 (Holm) |
| heteroscedastic | +0,004 | [+0,001 ; +0,007] | 0,025 (Holm) |
| hierarchical | +0,094 | [+0,087 ; +0,101] | 8e-05 (Holm) |
| shells | +0,432 | [+0,422 ; +0,442] | 8e-05 (Holm) |
| spherical | -0,149 | [-0,171 ; -0,124] | 8e-05 (Holm) |
| unbalanced | +0,025 | [+0,009 ; +0,042] | 0,0078 (Holm) |

## lot B, K=8 : cap_K8 contre hdb_ms8 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | -0,025 | [-0,027 ; -0,022] | 3e-05 |
| n = 8000 | -0,025 | [-0,029 ; -0,021] | 1e-05 |
| n = 16000 | -0,022 | [-0,026 ; -0,018] | 1e-05 |
| n = 32000 | -0,026 | [-0,031 ; -0,022] | 1e-05 |
| anisotropic | -0,000 | [-0,003 ; +0,003] | 0,99 (Holm) |
| bridge | -0,001 | [-0,001 ; -0,000] | 0,59 (Holm) |
| filaments | -0,037 | [-0,048 ; -0,025] | 8e-05 (Holm) |
| heteroscedastic | -0,001 | [-0,002 ; -0,000] | 0,28 (Holm) |
| hierarchical | -0,001 | [-0,002 ; +0,000] | 0,69 (Holm) |
| shells | -0,166 | [-0,176 ; -0,156] | 8e-05 (Holm) |
| spherical | -0,001 | [-0,002 ; -0,001] | 0,24 (Holm) |
| unbalanced | +0,010 | [-0,002 ; +0,021] | 0,52 (Holm) |

## lot B, K=10 : cap_K10 contre hdb_ms10 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | -0,034 | [-0,036 ; -0,031] | 3e-05 |
| n = 8000 | -0,028 | [-0,032 ; -0,024] | 1e-05 |
| n = 16000 | -0,038 | [-0,043 ; -0,033] | 1e-05 |
| n = 32000 | -0,035 | [-0,039 ; -0,031] | 1e-05 |
| anisotropic | -0,015 | [-0,018 ; -0,012] | 8e-05 (Holm) |
| bridge | -0,007 | [-0,007 ; -0,006] | 8e-05 (Holm) |
| filaments | -0,055 | [-0,067 ; -0,044] | 8e-05 (Holm) |
| heteroscedastic | +0,011 | [+0,010 ; +0,012] | 8e-05 (Holm) |
| hierarchical | -0,009 | [-0,011 ; -0,007] | 8e-05 (Holm) |
| shells | -0,202 | [-0,212 ; -0,191] | 8e-05 (Holm) |
| spherical | -0,006 | [-0,007 ; -0,006] | 8e-05 (Holm) |
| unbalanced | +0,014 | [+0,001 ; +0,027] | 0,034 (Holm) |

## objet, K=2 : tw_K2 contre mrb_K2 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | -0,001 | [-0,004 ; +0,003] | 1 |
| n = 8000 | +0,000 | [-0,007 ; +0,007] | 0,94 |
| n = 16000 | -0,004 | [-0,009 ; +0,003] | 0,3 |
| n = 32000 | +0,002 | [-0,005 ; +0,009] | 0,65 |
| anisotropic | -0,007 | [-0,024 ; +0,011] | 1 (Holm) |
| bridge | +0,007 | [+0,000 ; +0,021] | 0,4 (Holm) |
| filaments | +0,003 | [-0,018 ; +0,025] | 1 (Holm) |
| heteroscedastic | -0,001 | [-0,006 ; +0,004] | 1 (Holm) |
| hierarchical | -0,001 | [-0,005 ; +0,002] | 1 (Holm) |
| shells | +0,002 | [+0,001 ; +0,003] | 0,043 (Holm) |
| spherical | +0,000 | [-0,002 ; +0,002] | 1 (Holm) |
| unbalanced | -0,007 | [-0,013 ; -0,000] | 0,33 (Holm) |

## objet, K=3 : tw_K3 contre mrb_K3 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | -0,000 | [-0,003 ; +0,003] | 1 |
| n = 8000 | -0,000 | [-0,006 ; +0,006] | 0,99 |
| n = 16000 | -0,002 | [-0,007 ; +0,003] | 0,41 |
| n = 32000 | +0,001 | [-0,004 ; +0,006] | 0,76 |
| anisotropic | +0,001 | [-0,011 ; +0,013] | 1 (Holm) |
| bridge | -0,000 | [-0,001 ; +0,000] | 1 (Holm) |
| filaments | +0,000 | [-0,017 ; +0,018] | 1 (Holm) |
| heteroscedastic | +0,000 | [-0,005 ; +0,006] | 1 (Holm) |
| hierarchical | +0,000 | [-0,001 ; +0,001] | 1 (Holm) |
| shells | +0,002 | [+0,001 ; +0,004] | 0,0014 (Holm) |
| spherical | +0,001 | [-0,000 ; +0,002] | 0,71 (Holm) |
| unbalanced | -0,007 | [-0,016 ; +0,002] | 0,71 (Holm) |

## objet, K=5 : tw_K5 contre mrb_K5 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,001 | [-0,001 ; +0,004] | 0,95 |
| n = 8000 | +0,004 | [-0,001 ; +0,009] | 0,096 |
| n = 16000 | +0,001 | [-0,003 ; +0,004] | 0,69 |
| n = 32000 | -0,000 | [-0,005 ; +0,004] | 0,89 |
| anisotropic | +0,003 | [-0,003 ; +0,010] | 1 (Holm) |
| bridge | +0,001 | [+0,000 ; +0,002] | 0,1 (Holm) |
| filaments | +0,008 | [-0,007 ; +0,022] | 1 (Holm) |
| heteroscedastic | -0,000 | [-0,006 ; +0,005] | 1 (Holm) |
| hierarchical | -0,001 | [-0,002 ; +0,000] | 1 (Holm) |
| shells | +0,006 | [+0,003 ; +0,010] | 0,007 (Holm) |
| spherical | +0,000 | [-0,001 ; +0,001] | 1 (Holm) |
| unbalanced | -0,007 | [-0,017 ; +0,003] | 1 (Holm) |

## objet, K=8 : tw_K8 contre mrb_K8 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,000 | [-0,002 ; +0,002] | 1 |
| n = 8000 | +0,004 | [+0,001 ; +0,008] | 0,029 |
| n = 16000 | -0,002 | [-0,006 ; +0,001] | 0,2 |
| n = 32000 | -0,002 | [-0,004 ; +0,001] | 0,19 |
| anisotropic | -0,001 | [-0,007 ; +0,005] | 1 (Holm) |
| bridge | -0,003 | [-0,004 ; -0,002] | 8e-05 (Holm) |
| filaments | +0,004 | [-0,006 ; +0,013] | 1 (Holm) |
| heteroscedastic | -0,003 | [-0,007 ; +0,001] | 0,59 (Holm) |
| hierarchical | -0,002 | [-0,003 ; -0,001] | 8e-05 (Holm) |
| shells | +0,012 | [+0,006 ; +0,018] | 0,0012 (Holm) |
| spherical | -0,002 | [-0,003 ; -0,001] | 0,0019 (Holm) |
| unbalanced | -0,003 | [-0,010 ; +0,004] | 1 (Holm) |

## objet, K=10 : tw_K10 contre mrb_K10 (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,010 | [+0,006 ; +0,014] | 0,0016 |
| n = 8000 | +0,007 | [+0,003 ; +0,012] | 0,002 |
| n = 16000 | +0,009 | [+0,002 ; +0,017] | 0,046 |
| n = 32000 | +0,013 | [+0,004 ; +0,021] | 0,066 |
| anisotropic | -0,008 | [-0,015 ; -0,002] | 0,042 (Holm) |
| bridge | -0,003 | [-0,004 ; -0,002] | 8e-05 (Holm) |
| filaments | -0,004 | [-0,016 ; +0,007] | 1 (Holm) |
| heteroscedastic | +0,001 | [-0,004 ; +0,005] | 1 (Holm) |
| hierarchical | -0,002 | [-0,002 ; -0,001] | 0,00012 (Holm) |
| shells | +0,103 | [+0,075 ; +0,131] | 8e-05 (Holm) |
| spherical | -0,003 | [-0,004 ; -0,002] | 0,00012 (Holm) |
| unbalanced | -0,005 | [-0,010 ; +0,000] | 0,25 (Holm) |

## K=1 sans remplissage : tw_K1_nf contre hdb_ms1_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,034 | [+0,028 ; +0,041] | 1e-05 |
| n = 8000 | +0,039 | [+0,027 ; +0,052] | 0,0001 |
| n = 16000 | +0,029 | [+0,018 ; +0,040] | 0,0024 |
| n = 32000 | +0,034 | [+0,024 ; +0,044] | 0,0011 |
| anisotropic | +0,069 | [+0,043 ; +0,096] | 8e-05 (Holm) |
| bridge | +0,082 | [+0,065 ; +0,100] | 8e-05 (Holm) |
| filaments | +0,047 | [+0,031 ; +0,064] | 8e-05 (Holm) |
| heteroscedastic | +0,001 | [+0,000 ; +0,003] | 0,091 (Holm) |
| hierarchical | -0,136 | [-0,144 ; -0,127] | 8e-05 (Holm) |
| shells | +0,001 | [+0,000 ; +0,004] | 1 (Holm) |
| spherical | +0,209 | [+0,175 ; +0,244] | 8e-05 (Holm) |
| unbalanced | -0,002 | [-0,004 ; +0,000] | 0,37 (Holm) |

## K=2 sans remplissage : tw_K2_nf contre hdb_ms2_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,041 | [+0,035 ; +0,047] | 1e-05 |
| n = 8000 | +0,044 | [+0,032 ; +0,057] | 6e-05 |
| n = 16000 | +0,036 | [+0,027 ; +0,047] | 0,00065 |
| n = 32000 | +0,043 | [+0,034 ; +0,053] | 0,00019 |
| anisotropic | +0,088 | [+0,062 ; +0,115] | 8e-05 (Holm) |
| bridge | +0,113 | [+0,105 ; +0,125] | 8e-05 (Holm) |
| filaments | +0,060 | [+0,040 ; +0,081] | 8e-05 (Holm) |
| heteroscedastic | +0,005 | [+0,003 ; +0,008] | 8e-05 (Holm) |
| hierarchical | -0,162 | [-0,165 ; -0,159] | 8e-05 (Holm) |
| shells | -0,000 | [-0,002 ; +0,002] | 1 (Holm) |
| spherical | +0,227 | [+0,194 ; +0,262] | 8e-05 (Holm) |
| unbalanced | -0,001 | [-0,006 ; +0,005] | 1 (Holm) |

## K=3 sans remplissage : tw_K3_nf contre hdb_ms3_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,049 | [+0,043 ; +0,055] | 1e-05 |
| n = 8000 | +0,054 | [+0,043 ; +0,066] | 1e-05 |
| n = 16000 | +0,047 | [+0,037 ; +0,058] | 1e-05 |
| n = 32000 | +0,045 | [+0,035 ; +0,055] | 6e-05 |
| anisotropic | +0,098 | [+0,074 ; +0,124] | 8e-05 (Holm) |
| bridge | +0,097 | [+0,079 ; +0,116] | 8e-05 (Holm) |
| filaments | +0,052 | [+0,034 ; +0,072] | 8e-05 (Holm) |
| heteroscedastic | +0,016 | [+0,013 ; +0,019] | 8e-05 (Holm) |
| hierarchical | -0,145 | [-0,151 ; -0,138] | 8e-05 (Holm) |
| shells | -0,004 | [-0,005 ; -0,001] | 8e-05 (Holm) |
| spherical | +0,249 | [+0,217 ; +0,283] | 8e-05 (Holm) |
| unbalanced | +0,028 | [+0,022 ; +0,033] | 8e-05 (Holm) |

## K=5 sans remplissage : tw_K5_nf contre hdb_ms5_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,077 | [+0,072 ; +0,082] | 1e-05 |
| n = 8000 | +0,081 | [+0,073 ; +0,089] | 1e-05 |
| n = 16000 | +0,075 | [+0,066 ; +0,084] | 1e-05 |
| n = 32000 | +0,076 | [+0,067 ; +0,085] | 1e-05 |
| anisotropic | +0,139 | [+0,116 ; +0,162] | 8e-05 (Holm) |
| bridge | +0,162 | [+0,159 ; +0,165] | 8e-05 (Holm) |
| filaments | +0,102 | [+0,081 ; +0,124] | 8e-05 (Holm) |
| heteroscedastic | +0,029 | [+0,027 ; +0,031] | 8e-05 (Holm) |
| hierarchical | -0,157 | [-0,159 ; -0,155] | 8e-05 (Holm) |
| shells | -0,009 | [-0,010 ; -0,008] | 8e-05 (Holm) |
| spherical | +0,286 | [+0,263 ; +0,311] | 8e-05 (Holm) |
| unbalanced | +0,068 | [+0,062 ; +0,073] | 8e-05 (Holm) |

## K=8 sans remplissage : tw_K8_nf contre hdb_ms8_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,100 | [+0,094 ; +0,105] | 1e-05 |
| n = 8000 | +0,102 | [+0,094 ; +0,111] | 1e-05 |
| n = 16000 | +0,097 | [+0,089 ; +0,107] | 1e-05 |
| n = 32000 | +0,099 | [+0,089 ; +0,109] | 1e-05 |
| anisotropic | +0,174 | [+0,150 ; +0,198] | 8e-05 (Holm) |
| bridge | +0,188 | [+0,181 ; +0,200] | 8e-05 (Holm) |
| filaments | +0,114 | [+0,094 ; +0,135] | 8e-05 (Holm) |
| heteroscedastic | +0,037 | [+0,035 ; +0,039] | 8e-05 (Holm) |
| hierarchical | -0,152 | [-0,153 ; -0,150] | 8e-05 (Holm) |
| shells | -0,009 | [-0,013 ; -0,003] | 8e-05 (Holm) |
| spherical | +0,351 | [+0,327 ; +0,378] | 8e-05 (Holm) |
| unbalanced | +0,092 | [+0,085 ; +0,098] | 8e-05 (Holm) |

## K=10 sans remplissage : tw_K10_nf contre hdb_ms10_nf (descriptif)

| Strate | Δ | IC 95 % | p |
| --- | ---: | --- | ---: |
| toutes | +0,105 | [+0,100 ; +0,111] | 1e-05 |
| n = 8000 | +0,111 | [+0,102 ; +0,121] | 1e-05 |
| n = 16000 | +0,104 | [+0,095 ; +0,114] | 1e-05 |
| n = 32000 | +0,101 | [+0,092 ; +0,110] | 1e-05 |
| anisotropic | +0,184 | [+0,161 ; +0,209] | 8e-05 (Holm) |
| bridge | +0,200 | [+0,189 ; +0,215] | 8e-05 (Holm) |
| filaments | +0,129 | [+0,107 ; +0,152] | 8e-05 (Holm) |
| heteroscedastic | +0,040 | [+0,038 ; +0,041] | 8e-05 (Holm) |
| hierarchical | -0,149 | [-0,150 ; -0,148] | 8e-05 (Holm) |
| shells | -0,000 | [-0,012 ; +0,015] | 0,99 (Holm) |
| spherical | +0,344 | [+0,327 ; +0,362] | 8e-05 (Holm) |
| unbalanced | +0,096 | [+0,089 ; +0,102] | 8e-05 (Holm) |
