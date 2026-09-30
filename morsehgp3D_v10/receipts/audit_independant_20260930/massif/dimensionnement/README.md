# Dimensionnement massif : extraction et scénarios, 30 septembre 2026

`public_status=not_claimed`. Lecture seule des sources et reçus ; aucune VM, aucun GPU et aucun calcul massif utilisés par cet audit. [receipt.json](receipt.json) conserve les deux lignes exactes du CSV S5, les faits machine, 23 empreintes de sources, les calculs décimaux et les limites de portée. [extract.py](extract.py) permet de refaire l'extraction vers un fichier neuf.

Session S5 : 169 lignes réussies et 7 entrées non lancées sur budget temporel ; 67 entrées synthétiques et 21 secteurs issus des seules trames 08/000000, 08/000100 et 08/000200, sans sol. Backend CPU, 24 cœurs et 48 fils. Une graine et une exécution par configuration. Les ledgers conservés S5 (16 fichiers) et S7 (11 fichiers) ont été vérifiés ; ce contrôle d'intégrité ne qualifie pas leur contenu scientifique.

À 1 024 000 sites synthétiques, la tour FULL sans attaches ponctuelles prend :

| Kmax | Boules | Catalogue + tour | Mur du processus | Pic RSS |
| --- | ---: | ---: | ---: | ---: |
| 5 | 87 574 708 | 20,8461 s | 21,51 s | 23,4052 Gio / 25,1312 Go |
| 10 | 478 482 791 | 124,3633 s | 126,82 s | 135,2842 Gio / 145,2603 Go |

Le temps catalogue + tour exclut lecture, préparation, masque, quantification, attaches ponctuelles, tête, restitution aux retours et export durable. Le mur du processus inclut lecture, préparation et résumé de sortie, mais aucune segmentation, quantification ni export complet. Le compteur de boules vient d'un processus catalogue distinct ; l'ancien runner ne le compare pas au catalogue régénéré dans le processus tour. Le reçu archivé contient des résumés, pas une exportation des 606 575 494 nœuds cumulés de la ligne K10.

Faits machine capturés avant S5 : 176 Gio de RAM totale, 173 Gio disponibles ; disque racine de 97G, 79G libres ; GPU de 97 887 Mio. Il ne s'agit pas de la disponibilité actuelle de cette machine. Aucun spool massif n'est budgété ou qualifié par cette capture.

Les lignes suivantes sont exclusivement des **scénarios arithmétiques**, jamais des prédictions, bornes ou qualifications : n × ratio supposé de boules/site × 303,6–314,9 octets/boule. Ni la constance du ratio ni celle de l'empreinte n'est établie. Les ressources supplémentaires du modèle frontière, des retours, masques, poses, projections, sorties, tris externes et checkpoints restent à compter.

| Sites supposés | Boules/site supposées | Boules du scénario | RSS du scénario, Go décimaux |
| --- | ---: | ---: | ---: |
| 10 000 000 | 120 | 1 200 000 000 | 364.32–377.88 |
| 10 000 000 | 460 | 4 600 000 000 | 1396.56–1448.54 |
| 30 000 000 | 120 | 3 600 000 000 | 1092.96–1133.64 |
| 30 000 000 | 460 | 13 800 000 000 | 4189.68–4345.62 |
| 50 000 000 | 120 | 6 000 000 000 | 1821.6–1889.4 |
| 50 000 000 | 460 | 23 000 000 000 | 6982.8–7242.7 |

L'indice réservé `kNone` vaut 4 294 967 295. Les scénarios qui l'atteignent signalent seulement un problème de représentation à instruire ; d'autres index de la tour et le pic physique peuvent imposer un refus plus tôt. Le générateur courant convertit le nombre de références en u32 sans garde visible, et le budget par défaut des `Buffer` est illimité ; les grands vecteurs du catalogue ne sont pas facturés par ce budget.

Aucun SLO massif v10 n'a été trouvé dans les contrats relus. Les plafonds historiques Phase 15/v4 de 600 s à 1 M, 3 600 s à 10 000 001 et 7 200 s à 30 M sont explicitement historisés, sans migration implicite. Ils ne deviennent ni des délais v10 ni une cible massive de 100 ms. Le protocole v10 porte actuellement sur les trames LiDAR et un diagnostic cumulé de 1/2/4/8 trames.

Les réponses Claude du 30 septembre restent compatibles avec cette portée : le lecteur CUDA strict ne qualifie pas le moteur GPU complet et refuse volontairement l'ancien reçu S7 ; les copies r2 attendent l'extraction commune avec plan, empreintes et reçus. Avant toute dépense, fermer l'admission mémoire et les indices sur petites fixtures, conserver la gestion des frontières et les plateaux globaux, puis mesurer les coûts de sortie et de disque du payload choisi.

Les sources sont référencées par chemins du checkout et empreintes dans le JSON. Toute reproduction sur des sources différentes crée une autre capture. `SHA256SUMS` ferme les trois fichiers de ce sous-dossier, sans modifier les ledgers historiques.
