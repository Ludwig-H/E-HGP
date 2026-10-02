# Ablation catalogue3 : feuilles 16 contre 32

Source réellement exécutée : `e6fe34cb082f19d0041c829dfb38ea249319ab19`. Capture catalogue3 fermée localement, publication encore non versionnée à la lecture. Les cinq pièces sont copiées puis hachées avant/après ; seules dix petites sources Git e6 utiles aux nouveautés sont ajoutées. Comparaison catalogue2 depuis notre reçu déjà clos, jamais modifié. [Provenance initiale](sources_before.json), [stabilité finale](capture_after.json), [faits recoupés](observations.json).

Cadre `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, audit de preuves et d'ablation, `not_claimed`. **GCP non utilisé par cet audit ; aucun build, CTest produit ou binaire natif lancé.** Le [lecteur autonome](review.py) relit seulement les copies, en normal/−O ; aucun chantier catalogue4 ou source LIVE ultérieure n'est importé.

Qualification : **960/960 portes**, dont Release 220, ASan+UBSan/TSan/profils 21/24 à 145 chacun, poison 146, style 2 et mutants 12 ; Clang facultatif absent. Les **neuf mutants catalogue sont TUE/code**, sans signal, délai ou compilation : le nouveau `support_tetra_strict_ignore` est effectivement détecté.

La fixture `extended_q4` ajoute cinq points cosphériques, qmin 4, deux supports stricts et choix canonique lexicographique (`model_test.py:38–43`). Les logs normal/−O conservent 378 requêtes natives, 358 succès attendus, 20 refus, 132 505 contrôles et 127 comparaisons métamorphiques aux trois profils. Les 42 faits et 17 corruptions de réponse qualifient le juge, distinctement des neuf mutants produit. La nouvelle fixture de projection conserve des descendants core croisés entre K1/K2 **après les attaches**, avec permutations/translation/échelle (`test_projection_contracts.py:122–166`). Les deux étages Python indépendants sont joués en Release normal/−O : `projection_contracts_ok faits=5`. Cela ne qualifie aucune projection ou tour FULL native.

Le calendrier du banc est clos avec **13 tentatives, 7 succès, 6 timeouts et 23 omissions**, couvrant les 36 unités demandées. K5/8k a trois répétitions achevées ; les autres succès dépassent 10 s et n'ont qu'une observation. K10 expire sur 8k/16k et les trois trames  ; 32k/K5 expire et K10 est omis. Un timeout 30 s est un délai de processus, pas un chrono catalogue complet. La commande banc rend 1 : session globale `failed_remote`, matrice conforme séparément.

| Appel catalogue API achevé, leaf16 | Durée exacte en secondes | Boules | Incidences | Pic Buffer, octets |
| --- | ---: | ---: | ---: | ---: |
| Uniforme 8k/K5,r0 | 8,239569411 | 597998 | 2895136 | 133416208 |
| Uniforme 8k/K5,r1 | 8,240274530 | 597998 | 2895136 | 133416208 |
| Uniforme 8k/K5,r2 | 8,210329939 | 597998 | 2895136 | 133416208 |
| Uniforme 16k/K5,r0 | 17,309648807 | 1233046 | 5979160 | 275155856 |
| 08/000100 sans sol, 35 551 sites/K5,r0 | 20,740759141 | 1095926 | 5085683 | 235905260 |
| 08/000000 sans sol, 39 885 sites/K5,r0 | 26,017549456 | 1306696 | 6097121 | 279721668 |
| 08/000200 sans sol, 45 845 sites/K5,r0 | 24,092861068 | 1407885 | 6514697 | 297653548 |

Comparaison leaf32→16 à 8k/K5 : **même manifeste d'entrées, même hash d'exécutable Release**, aucune source produit modifiée entre f391 et e6, seul argument API `leaf_size=32→16`. Les 597 998 boules, 597 987 niveaux, 2 895 136 incidences et pic 133 416 208 octets sont inchangés. Les trois sorties leaf16 déclarent le même SHA canonique `2671f84acd61597300af06e7f164b0c8cd552b726bf7519711c8772d3febf74a` et 109 592 066 octets que l'unique sortie leaf32. Fichiers supprimés sur VM, hash déclaré non recalculé ici. La mesure leaf32 unique 15,478187473 s divisée par la médiane leaf16 de 8,239569411 s donne **1,878518974** : comparaison observée non appariée, pas preuve d'un gain général ou comparaison statistique robuste avec une seule baseline.

La durée API comprend deux passes, tri et sorties mémoire ; processus, lecture, Cloud et sérialisation sont distincts. Masque de sol/préparation horsligne exclus, pics Buffer incluant Cloud différents du RSS. Les trois LiDAR sont des trames entières d'une même séquence, sans transfert vers plusieurs séquences, FULL, GPU ou 100 ms. Leaf16 permet ici trois catalogues LiDAR K5 achevés, encore à 20–26 s ; leurs K10 ne sont pas achevés.

Clôture recoupée : paquet identique à un `git archive` indépendant d'e6, archive et 106 entrées de manifeste valides, hash du binaire mesuré égal au manifeste Release, textes flags/cache hachés. Génération démarrée/arrêtée `2026-10-02T06:09:30.381-07:00`, état `TERMINATED` à 06:19:47.288−07:00, garde-fous conservés, clé/OS Login retirés et verrou libéré. Les groupes worker sont fermés avant/après banc ; la limite de quiescence interne de la matrice demeure. Aucun contrôle GCP actuel n'est effectué.

Le premier lecteur audit comptait aussi les lignes `Command:` contenant le verdict de projection ; il refusait à tort les deux sorties directes. Cet échec de notre collecteur est conservé puis corrigé par comparaison de lignes exactes, sans changement du produit ni des pièces G4. Résultats finaux : [normal/−O](review_runs.json). Les reçus antérieurs restent intacts ; ce dossier est clos par `SHA256SUMS`.
