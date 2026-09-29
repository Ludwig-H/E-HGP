# Reçu : des ordres K plus grands aident-ils le clustering ? (dev, 29 septembre 2026)

`public_status=not_claimed`, `mode=benchmark_only`, graines **dev** seulement. Ce n'est pas un test préenregistré.

## Question

Le moteur exact de la tour est borné à K = 10 (`kMaxOrder`). Le bruit de l'estimateur K-NN, d'écart-type relatif
d'environ 1/√K, efface les cols peu profonds : aucune composante n'est retenue au-delà d'un col à 70 % du pic
(`audits/tete_multik_20260929`), et l'écart restant à la partition de Morse se concentre sur `anisotropic`
(`receipts/bench_dev_alloc_20260929`). Faut-il étendre le moteur à K > 10 ?

On mesure d'abord l'effet de K sur le témoin MR₂-bord. À même entrée et même tête, il égale la tour (lot C, famille
« objet » : écart d'au plus 0,010).

## Exécution

- Script `bench/synthetic/bigk_dev.py`, lecteur `bench/synthetic/analyse_bigk.py`.
- 384 scènes dev : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ; 32 000} × 2 répliques.
- Méthodes, avec mcs = √n :
  - MR₂-bord + tête v10 (EOM, z = 3 et 6), à K ∈ {10, 16, 24, 32, 48} ;
  - sklearn HDBSCAN (feuilles, α = 2, `min_samples` = K), aux mêmes K ;
  - la tour à K = 10 (entrée cover, EOM z = 6), comme ancre.
- Remplissages communs : `none`, `b2`, `b2.5`, `asc20_b2`, `ascK_b2`. Les tableaux ci-dessous utilisent
  `asc20_b2`, le meilleur remplissage de la tour ; la lecture `none` est dans
  [`analyse_bigk_none.txt`](analyse_bigk_none.txt).
- **G4**, CPU seul, 46 processus, deux sessions gardées, arrêt certifié TERMINATED sur la cible exacte après chacune,
  clé retirée :
  - `s6a`, commit `97a0b50ac`, 128 scènes à 32 000 points en 283 s (génération `12:08:57.758-07:00`, arrêt
    `12:21:32.310-07:00`) ;
  - `s6b`, commit `6206d1d11`, même code C++ et même script, 256 scènes à 8 000 et 16 000 points en 154 s
    (génération `12:25:29.003-07:00`, arrêt `12:36:37.679-07:00`).

  Le statut `failed_remote` des deux sessions ne vient que de pip. Aucun refus. Fichiers de session dans
  `sessions/`, adresse du compte masquée.

## Résultats (ARI_s, `asc20_b2`, [`analyse_bigk_asc20_b2.txt`](analyse_bigk_asc20_b2.txt))

Écart apparié à K = 10, avec l'IC bootstrap 95 % sur les scènes :

| Méthode | K = 10 | K = 16 | K = 24 | K = 32 | K = 48 |
| --- | ---: | ---: | ---: | ---: | ---: |
| MR₂-bord, z = 6 | 0,7906 | −0,084 [−0,108 ; −0,060] | −0,090 | −0,088 | −0,091 |
| MR₂-bord, z = 3 | 0,7081 | −0,015 [−0,032 ; +0,001] | −0,011 | −0,009 | −0,012 |
| sklearn, feuilles | 0,7404 | **+0,008 [+0,004 ; +0,013]** | −0,002 | −0,014 | −0,024 |

La tour à K = 10 obtient 0,8003 (8 000 : 0,803 ; 16 000 : 0,793 ; 32 000 : 0,805).

**Par famille** :

- **`shells` s'effondre dès K = 16** : −0,67 à −0,71 pour MR₂-bord à z = 6, −0,20 à z = 3, jusqu'à −0,19 pour
  sklearn.
- **Ailleurs, les gains sont faibles** :
  - `anisotropic` : +0,002 à +0,032 selon z et K ;
  - `spherical` : +0,002 à +0,033 ;
  - `unbalanced` : jusqu'à +0,013 ;
  - `bridge` : +0,04 à z = 3 seulement ;
  - `heteroscedastic` recule légèrement.

## Lecture

- **Un K plus grand n'aide pas le clustering sur ce banc.**
  - Il réduit bien le bruit de l'estimateur : de petits gains apparaissent sur les familles gaussiennes.
  - Mais il lisse la densité à travers les vides étroits, et les coquilles voisines fusionnent. C'est le biais
    d'estimation du K-NN, qui croît avec K, et la loi de la demi-lacune : un vide de largeur g est franchi vers g/2.
  - Le meilleur réglage reste la tour à K = 10 avec z = 6.
- **Conséquence** : étendre le moteur exact au-delà de K = 10 ne se justifie pas pour le clustering. Le levier
  restant est la sélection par scène ou par famille (`receipts/bench_dev_shrink_20260929`).
- L'essai d'une seule scène `anisotropic` difficile (K = 32 : 0,880 contre 0,802) n'était pas représentatif. En
  moyenne sur `anisotropic`, le gain reste de +0,002 à +0,032.
