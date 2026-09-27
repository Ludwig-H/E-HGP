# Listes B partagées en ordre original — capture locale close

27 septembre 2026, base `4badf8b7d`, autorité `r1/capture.json`.
25 commandes closes : Release et Clang ASan/UBSan/LSan, deux mutants par
build, puis 15 mesures. Lecteurs normal et `python3 -O` PASS.
[Code, preuve et limites](../../audits/b_q34_ordered_rows_20260927/README.md).
Aucun moteur modifié, aucun GCP, aucun nouveau résultat FULL.

## Correction et ordre

Les deux gates sont identiques : 17 488 vrais rectangles de 96 fronts,
26 496 comparaisons de paires, 1 556 rejets implicites et 360 cas dont les
classes A permutent l'ordre original. L'émission par lignes partagées
retrouve néanmoins directement l'ordre original. Quatre refus et les cas
vides/K1/saturés passent. Les mutants retournent 1 avec motif causal :
`ordered_original_B` pour B inversé, `ordered_mass` pour mask6 abusif.

Les quinze grandes mesures gardent exactement P/E/E3/E4/F des captures Pool
publiées. Elles vérifient intégralement les listes B et leurs masques, sans
expanser les E paires. L'expansion complète est réservée aux petits gates.
Le filtre ponctuel S2, les lanes et FULL ne sont pas exécutés ici.

## Travail réellement ajouté

W compte les comparaisons de crédits lors des scans de B par classe A
occupée ; T compte les entrées B conservées une seule fois par classe.
E est le résidu total, petits rectangles non préparés compris. W et T
portent seulement sur les rectangles préparés. Les scans de facteurs et
les 100 cases d'histogramme par plan sont publiés en plus.

| entrée | W | T | E, inchangé | ajout des lignes, ms CPU |
| --- | ---: | ---: | ---: | ---: |
| 08/000000 sans sol K5/s8 | 4 079 626 | 1 968 618 | 9 122 704 | 71,985 |
| 08/000100 sans sol K5/s8 | 2 552 919 | 1 376 615 | 5 661 491 | 60,448 |
| 08/000200 sans sol K5/s8 | 4 388 202 | 2 388 949 | 10 498 526 | 95,410 |
| 08/000000 sans sol K10/s8 | 7 615 855 | 4 734 110 | 18 231 486 | 87,823 |
| 08/000000 sans sol K5/s10 | 3 590 928 | 1 793 221 | 7 732 782 | 70,896 |
| 08/000000 sans sol K5/s12 | 3 228 210 | 1 687 413 | 6 880 859 | 78,723 |

Une seule réalisation par entrée, CPU local partagé, avec d'autres
compilations/travaux concurrents. Ces ajouts sont chronométrés après la
construction de l'ancien plan ; ils excluent destruction et vérification,
payées dans le temps complet. **Ne pas comparer ces temps à une autre
capture de bandes pour annoncer un gain relatif.** Le constructeur ancien
reste payé : par exemple 1 516,428 ms sur 00/K5/s8, dans une sonde totale
de 19 048,769 ms. Ce n'est pas un chrono de préparation directe ni de tour.

Trames entières sans sol : 39 885 / 35 551 / 45 845 sites, grille 1 mm,
trois trames de la seule séquence 08. Les mêmes masques/IDs/préparations
que les captures publiées sont contrôlés ; aucun octet KITTI n'est ajouté.
Les coupes capteur ne sont pas remesurées par cette sonde.

| synthétique K5/s8 | W | T | E |
| --- | ---: | ---: | ---: |
| uniforme 8k | 676 | 670 | 435 709 |
| uniforme 16k | 1 658 | 1 642 | 908 050 |
| uniforme 32k | 3 088 | 3 061 | 1 876 820 |
| terrain 8k | 965 | 951 | 140 079 |
| terrain 16k | 1 451 | 1 434 | 285 967 |
| terrain 32k | 1 882 | 1 860 | 591 278 |
| amas 8k | 337 873 | 51 945 | 2 091 410 |
| amas 16k | 707 310 | 107 913 | 7 787 691 |
| amas 32k | 1 437 893 | 223 269 | 30 699 080 |

Sur amas, W fait ×2,093 puis ×2,033 ; T fait ×2,077 puis ×2,069.
Ce travail de représentation mesuré est sous le quadruplement, **mais E
garde ×3,724 puis ×3,942**. Une représentation de croissance modérée ne
transforme pas le résidu du générateur en algorithme sous-quadratique.
Sur uniforme/terrain, beaucoup de petits rectangles ne passent pas par
ces plans : ne pas généraliser leurs très petits W à tout le filtre.

## Mémoire : sommes de capacités, non pics RAM/GPU

| entrée sans sol | sidecars, capacité Mo | contenu logique + objets, Mo | ancien plan encore conservé, Mo |
| --- | ---: | ---: | ---: |
| 00 K5/s8 | 33,648 | 28,865 | 125,200 |
| 01 K5/s8 | 26,903 | 23,546 | 105,362 |
| 02 K5/s8 | 42,002 | 36,232 | 160,840 |
| 00 K10/s8 | 47,592 | 36,242 | 178,802 |
| 00 K5/s10 | 30,641 | 26,239 | 116,060 |
| 00 K5/s12 | 29,191 | 25,031 | 112,333 |

Les colonnes ne sont pas des mémoires de remplacement : la sonde garde
les deux représentations pendant chaque comparaison, puis les détruit.
Le contenu logique comprend l'objet C++ par rectangle et les offsets
des listes ; il ne compte pas les capacités vides comme du contenu utile.
Sur 00/K5/s8, les 1 968 618 entrées B occupent seules 9,843 Mo logiques
de rangs/masques, avant les classes et les 884 642 slots A. Une arène
collective, ses préfixes par ancre et ses temporaires restent à implémenter.

## Décision

Conserver cette alternative : elle évite structurellement le tri final des
survivants tout en partageant les listes. La comparer à l'arène de bandes
sur le coût **complet** (préparation, décodage, S2 et transport), avant tout
choix de défaut. Ne pas copier ces listes pour chaque ancre. Ni T≤E ni la
borne T≤90 F_B ne garantissent seules une faible mémoire sur toute entrée.

Builds épinglés : `build/v9-q34-ordered-rows-20260927-r1_release` et
`build/v9-q34-ordered-rows-20260927-r1_sanitize`. Le lecteur LIVE lie les
sources, bibliothèques, entrées, commandes/intentions, flux et l'inventaire
complet des huit empreintes de build. Il ne relance pas les exécutables.

```bash
python3 -B morsehgp3D_v9/audits/b_q34_ordered_rows_20260927/run.py check morsehgp3D_v9/receipts/q34_ordered_rows_20260927/r1
python3 -B -O morsehgp3D_v9/audits/b_q34_ordered_rows_20260927/run.py check morsehgp3D_v9/receipts/q34_ordered_rows_20260927/r1
```

Ne pas reconstruire ces builds ou écraser r1 pour poursuivre.
