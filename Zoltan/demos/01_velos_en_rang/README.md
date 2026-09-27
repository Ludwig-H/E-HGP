# Vélos garés en rang : HDBSCAN n'isole pas le vélo B

Trame SemanticKITTI `08/001176` (sol retiré par Patchwork++, paramètres de la v8) : 67 114 points dans la hiérarchie, trame entière. Empreinte sha256 du `.bin` : `2843601f1a174ca647ead5172b6be4a7f93a865c37185b54187c9cc75a03de00`.

Quatre vélos garés en deux paires sur un trottoir, devant un bâtiment : A et B côte à côte (leurs boîtes de vérité terrain se recouvrent en vue de dessus), C derrière, collé à un quatrième vélo non suivi (instance 55, qui échoue lui aussi).

Ce qu'il faut regarder : la branche de A absorbe B alors que B n'a jamais été isolé ; A reste apparié (la PQ le compte juste) mais B est perdu, et ce pour tout K de 1 à 10. L'arbre ALPINE en vue de dessus, lui, sépare les trois vélos suivis pour t ∈ [0,096 ; 0,102[ m, six fois sous son seuil vélo : pas de vidéo ALPINE pour cette scène.

## Objets suivis

| objet | classe | vérité terrain | points |
| --- | --- | --- | ---: |
| A | vélo | sem 11, instance 43 | 265 |
| B | vélo | sem 11, instance 57 | 194 |
| C | vélo | sem 11, instance 56 | 207 |

## Résultats

Meilleur IoU d’un nœud de l’arbre (≤ 0,5 : aucune extraction ne rend l’objet), premier niveau apparié, premier niveau fusionné, en mètres.

| méthode | A : IoU / apparié / fusionné | B : IoU / apparié / fusionné | C : IoU / apparié / fusionné | coupe commune |
| --- | --- | --- | --- | --- |
| HDBSCAN, K = 5 | 0,67 / 0,120 / 0,202 | 0,41 / — / 0,171 | 0,75 / 0,145 / 0,202 | 2 sur 3 (r de 0,145 à 0,202) |
| HDBSCAN, K = 10 | 0,62 / 0,186 / 0,270 | 0,40 / — / 0,196 | 0,64 / 0,177 / 0,263 | 2 sur 3 (r de 0,186 à 0,263) |
| ALPINE sans sémantique | 0,91 / 0,076 / 0,128 | 0,82 / 0,096 / 0,102 | 0,89 / 0,063 / 0,126 | 3 sur 3 (t de 0,096 à 0,102) |

Meilleur IoU pour chaque K = min_samples de HDBSCAN, et pour ALPINE sans sémantique :

| objet | K = 1 | K = 2 | K = 3 | K = 4 | K = 5 | K = 6 | K = 7 | K = 8 | K = 9 | K = 10 | ALPINE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 0,66 | 0,66 | 0,65 | 0,64 | 0,67 | 0,66 | 0,64 | 0,58 | 0,58 | 0,62 | 0,91 |
| B | 0,41 | 0,41 | 0,41 | 0,42 | 0,41 | 0,40 | 0,40 | 0,40 | 0,40 | 0,40 | 0,82 |
| C | 0,65 | 0,65 | 0,64 | 0,63 | 0,75 | 0,75 | 0,72 | 0,63 | 0,63 | 0,64 | 0,89 |

## Vidéos

- HDBSCAN, K = 5 : vidéo [sombre](01_velos_en_rang_hdbscan_K5_sombre.mp4) · [clair](01_velos_en_rang_hdbscan_K5_clair.mp4) ; instant clé [sombre](01_velos_en_rang_hdbscan_K5_sombre_instant_cle.png) · [clair](01_velos_en_rang_hdbscan_K5_clair_instant_cle.png) ; bilan [sombre](01_velos_en_rang_hdbscan_K5_sombre_bilan.png) · [clair](01_velos_en_rang_hdbscan_K5_clair_bilan.png)
- HDBSCAN, K = 10 : vidéo [sombre](01_velos_en_rang_hdbscan_K10_sombre.mp4) · [clair](01_velos_en_rang_hdbscan_K10_clair.mp4) ; instant clé [sombre](01_velos_en_rang_hdbscan_K10_sombre_instant_cle.png) · [clair](01_velos_en_rang_hdbscan_K10_clair_instant_cle.png) ; bilan [sombre](01_velos_en_rang_hdbscan_K10_sombre_bilan.png) · [clair](01_velos_en_rang_hdbscan_K10_clair_bilan.png)
- ALPINE sans sémantique : pas de vidéo (arbre calculé, chiffres ci-dessus seulement).

Fusions de branches suivies (HDBSCAN, K = 5) : A+B à r = 0,171 m ; A+B+C à r = 0,202 m.

Chaque vidéo existe en thème sombre (fond marine) et clair (fond blanc), comme Percolia.com : prendre celui du fond des diapositives.

Régénérer : `python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/01_velos_en_rang`, puis `node Zoltan/demos/tools/render_video.cjs Zoltan/demos/01_velos_en_rang <étiquette>` (les deux thèmes ; `--theme clair` ou `--theme sombre` pour un seul).
