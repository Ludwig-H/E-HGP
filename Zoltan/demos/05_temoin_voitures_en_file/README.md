# Témoin : voitures garées en file, un même niveau HDBSCAN les sépare

Trame SemanticKITTI `08/002554` (sol retiré par Patchwork++, paramètres de la v8) : 72 426 points dans la hiérarchie, trame entière. Empreinte sha256 du `.bin` : `cfab4cbbca898c4e0899088d5c706f96d22a01de075ef49f53f2eeb5cef4792d`.

Témoin, pour ne pas surinterpréter les démos précédentes : sur trame sans sol, la hiérarchie HDBSCAN contient les voitures (aucun échec sur les 2 188 voitures d'au moins 50 points du criblage). Ici, un même niveau apparie les trois voitures à la fois.

Le niveau est choisi avec la vérité terrain : la sélection automatique (EOM) n'est pas évaluée. La vidéo ALPINE montre qu'à son seuil voiture (t = 1,8 m), sans sémantique ni découpage par boîte, les voitures A et B sont déjà fusionnées.

## Objets suivis

| objet | classe | vérité terrain | points |
| --- | --- | --- | ---: |
| A | voiture | sem 10, instance 169 | 1419 |
| B | voiture | sem 10, instance 170 | 1199 |
| C | voiture | sem 10, instance 171 | 896 |

## Résultats

Meilleur IoU d’un nœud de l’arbre (≤ 0,5 : aucune extraction ne rend l’objet), premier niveau apparié, premier niveau fusionné, en mètres.

| méthode | A : IoU / apparié / fusionné | B : IoU / apparié / fusionné | C : IoU / apparié / fusionné | coupe commune |
| --- | --- | --- | --- | --- |
| HDBSCAN, K = 5 | 0,86 / 0,070 / 0,241 | 0,98 / 0,085 / 0,893 | 0,99 / 0,087 / 0,658 | 3 sur 3 (r de 0,087 à 0,241) |
| ALPINE sans sémantique | 0,88 / 0,021 / 0,228 | 0,99 / 0,024 / 0,985 | 0,97 / 0,032 / 0,413 | 3 sur 3 (t de 0,032 à 0,228) |

Meilleur IoU pour chaque K = min_samples de HDBSCAN, et pour ALPINE sans sémantique :

| objet | K = 1 | K = 2 | K = 3 | K = 4 | K = 5 | K = 6 | K = 7 | K = 8 | K = 9 | K = 10 | ALPINE |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 0,87 | 0,87 | 0,87 | 0,87 | 0,86 | 0,86 | 0,86 | 0,86 | 0,83 | 0,83 | 0,88 |
| B | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,98 | 0,99 |
| C | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,99 | 0,97 |

## Vidéos

- HDBSCAN, K = 5 : vidéo [sombre](05_temoin_voitures_en_file_hdbscan_K5_sombre.mp4) · [clair](05_temoin_voitures_en_file_hdbscan_K5_clair.mp4) ; instant clé [sombre](05_temoin_voitures_en_file_hdbscan_K5_sombre_instant_cle.png) · [clair](05_temoin_voitures_en_file_hdbscan_K5_clair_instant_cle.png) ; bilan [sombre](05_temoin_voitures_en_file_hdbscan_K5_sombre_bilan.png) · [clair](05_temoin_voitures_en_file_hdbscan_K5_clair_bilan.png)
- ALPINE sans sémantique : vidéo [sombre](05_temoin_voitures_en_file_alpine_bev_sombre.mp4) · [clair](05_temoin_voitures_en_file_alpine_bev_clair.mp4) ; instant clé [sombre](05_temoin_voitures_en_file_alpine_bev_sombre_instant_cle.png) · [clair](05_temoin_voitures_en_file_alpine_bev_clair_instant_cle.png) ; bilan [sombre](05_temoin_voitures_en_file_alpine_bev_sombre_bilan.png) · [clair](05_temoin_voitures_en_file_alpine_bev_clair_bilan.png)

Fusions de branches suivies (HDBSCAN, K = 5) : B+C à r = 0,658 m.

Chaque vidéo existe en thème sombre (fond marine) et clair (fond blanc), comme Percolia.com : prendre celui du fond des diapositives.

Régénérer : `python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/05_temoin_voitures_en_file`, puis `node Zoltan/demos/tools/render_video.cjs Zoltan/demos/05_temoin_voitures_en_file <étiquette>` (les deux thèmes ; `--theme clair` ou `--theme sombre` pour un seul).
