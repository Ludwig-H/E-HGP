# Vélos contre une façade : HDBSCAN n'isole jamais A ni B

Trame SemanticKITTI `08/000882` (sol retiré par Patchwork++, paramètres de la v8) : 76 011 points dans la hiérarchie, trame entière. Empreinte sha256 du `.bin` : `67792442be545437694ef0fddbd7ad4fa21bf64430a2c6660f0192c8ba8d8148`.

Trois vélos rangés contre un mur, peu de points par vélo. HDBSCAN n'isole les vélos A et B pour aucun K de 1 à 10 (au plus 0,36 et 0,24). Le vélo C, lui, est isolé (IoU 0,60 à K = 5, 0,52 à K = 10) : ce qu'il absorbe est surtout du mobilier « autre objet », que l'évaluation panoptique ignore.

ALPINE sans sémantique n'isole pas B (0,24) et n'isole A et C que de justesse (0,52). Ce qu'il faut regarder : A et B sont encore des fragments quand leurs branches absorbent la façade et se rejoignent.

## Objets suivis

| objet | classe | vérité terrain | points |
| --- | --- | --- | ---: |
| A | vélo | sem 11, instance 38 | 52 |
| B | vélo | sem 11, instance 58 | 98 |
| C | vélo | sem 11, instance 59 | 147 |

## Résultats

Meilleur IoU d’un nœud de l’arbre (≤ 0,5 : aucune extraction ne rend l’objet), premier niveau apparié, premier niveau fusionné, en mètres.

| méthode | A : IoU / apparié / fusionné | B : IoU / apparié / fusionné | C : IoU / apparié / fusionné | coupe commune |
| --- | --- | --- | --- | --- |
| HDBSCAN, K = 5 | 0,31 / — / 0,145 | 0,18 / — / 0,135 | 0,60 / 0,131 / 0,133 | 1 sur 3 (r de 0,131 à 0,133) |
| HDBSCAN, K = 10 | 0,15 / — / 0,174 | 0,15 / — / 0,168 | 0,52 / 0,154 / 0,161 | 1 sur 3 (r de 0,154 à 0,161) |
| ALPINE sans sémantique | 0,52 / 0,060 / 0,066 | 0,24 / — / 0,067 | 0,52 / 0,069 / 0,075 | 1 sur 3 (t de 0,060 à 0,064) |

Meilleur IoU pour chaque K = min_samples de HDBSCAN, et pour ALPINE sans sémantique :

| objet | K = 1 | K = 2 | K = 3 | K = 4 | K = 5 | K = 6 | K = 7 | K = 8 | K = 9 | K = 10 | ALPINE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 0,36 | 0,36 | 0,33 | 0,30 | 0,31 | 0,28 | 0,26 | 0,23 | 0,16 | 0,15 | 0,52 |
| B | 0,24 | 0,24 | 0,22 | 0,20 | 0,18 | 0,13 | 0,19 | 0,19 | 0,16 | 0,15 | 0,24 |
| C | 0,71 | 0,71 | 0,67 | 0,62 | 0,60 | 0,56 | 0,37 | 0,58 | 0,36 | 0,52 | 0,52 |

## Vidéos

- HDBSCAN, K = 5 : vidéo [sombre](02_velos_contre_facade_hdbscan_K5_sombre.mp4) · [clair](02_velos_contre_facade_hdbscan_K5_clair.mp4) ; instant clé [sombre](02_velos_contre_facade_hdbscan_K5_sombre_instant_cle.png) · [clair](02_velos_contre_facade_hdbscan_K5_clair_instant_cle.png) ; bilan [sombre](02_velos_contre_facade_hdbscan_K5_sombre_bilan.png) · [clair](02_velos_contre_facade_hdbscan_K5_clair_bilan.png)
- HDBSCAN, K = 10 : vidéo [sombre](02_velos_contre_facade_hdbscan_K10_sombre.mp4) · [clair](02_velos_contre_facade_hdbscan_K10_clair.mp4) ; instant clé [sombre](02_velos_contre_facade_hdbscan_K10_sombre_instant_cle.png) · [clair](02_velos_contre_facade_hdbscan_K10_clair_instant_cle.png) ; bilan [sombre](02_velos_contre_facade_hdbscan_K10_sombre_bilan.png) · [clair](02_velos_contre_facade_hdbscan_K10_clair_bilan.png)
- ALPINE sans sémantique : vidéo [sombre](02_velos_contre_facade_alpine_bev_sombre.mp4) · [clair](02_velos_contre_facade_alpine_bev_clair.mp4) ; instant clé [sombre](02_velos_contre_facade_alpine_bev_sombre_instant_cle.png) · [clair](02_velos_contre_facade_alpine_bev_clair_instant_cle.png) ; bilan [sombre](02_velos_contre_facade_alpine_bev_sombre_bilan.png) · [clair](02_velos_contre_facade_alpine_bev_clair_bilan.png)

Fusions de branches suivies (HDBSCAN, K = 5) : A+B à r = 0,145 m ; A+B+C à r = 0,197 m.

Chaque vidéo existe en thème sombre (fond marine) et clair (fond blanc), comme Percolia.com : prendre celui du fond des diapositives.

Régénérer : `python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/02_velos_contre_facade`, puis `node Zoltan/demos/tools/render_video.cjs Zoltan/demos/02_velos_contre_facade <étiquette>` (les deux thèmes ; `--theme clair` ou `--theme sombre` pour un seul).
