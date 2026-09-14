# Partage du nuage et de Z — gain réel, coût des facteurs encore ouvert

14 septembre 2026. Mono CPU, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. **Un rectangle partitionné**,
pas une WSPD, une tour HGP FULL ou une exécution G4. GCP non utilisé.

## Ce qui est acquis

Le [raccord](../../docs/P0_NUAGE_ET_INDEX_PARTAGES.md) retire les copies,
validations et constructions d'index global répétées pour chaque rectangle.
Un propriétaire immuable et un index Z peuvent servir plusieurs rectangles
et seuils. Les boîtes de plages coûtent O(log n), sans parcourir les facteurs.
L'ancienne factory reste un adaptateur qui prépare un nuage neuf par appel.

Les 66 invocations comparent `fresh` (nuage/Z préparés par rectangle) à
`shared` (une seule préparation). Dans chaque paire, les sorties, le
résidu et tous les compteurs locaux publiés sont identiques. Les compteurs
de préparation globale sont divisés exactement par R, hors maxima de
profondeur. Les compteurs ne sont pas un décompte de toutes les instructions ;
projections, regroupements et certaines initialisations sont payés dans
le temps englobant sans avoir chacun un compteur distinct.

## Temps à 32k, R=32, Kmax10, s8

Une mesure par ordre dans `main_matrix`, sans mélanger les deux ordres :

| Famille | Ordre | Préparations répétées (ms) | Partage global (ms) |
| --- | --- | ---: | ---: |
| Grille | fresh puis shared | 919,32 | 457,05 |
| Grille | shared puis fresh | 731,72 | 465,36 |
| Nappe complète | fresh puis shared | 16177,38 | 15845,03 |
| Nappe complète | shared puis fresh | 16473,49 | 15931,07 |
| Déséquilibré | fresh puis shared | 423,80 | 95,32 |
| Déséquilibré | shared puis fresh | 408,57 | 86,57 |

Le partage économise effectivement les préparations répétées. Il ne règle
pas le résidu des nappes : **35 639 168 candidates** à 32k, puis
1 583 673 252 visites de comptage dans chacun des deux bras. La subdivision
de A affaiblit les minorants locaux ; ce résidu est beaucoup plus grand
que celui du rectangle non subdivisé de la tranche précédente. Ce n'est
pas un gain sur le moteur complet, ni une comparaison chronométrique
entre révisions. Les certificats encore valables après subdivision devront
être transmis par une API prouvée, pas abandonnés puis remplacés par des
crédits enfants plus faibles.

Les temps sont exploratoires sur machine partagée. Tous les s donnent
le même travail ici, mais les temps varient nettement : sur grille32k,
le bras partagé va de 372,7 à 1288,7 ms dans la matrice. Ce bruit interdit
d'attribuer ces différences à s ou de promettre un facteur d'accélération
stable. Sur nappe32k/s10/shared-first, le partage est même légèrement
plus lent (16284,9 contre 16213,2 ms). Aucune mesure n'est écartée.

## Est-on sous-quadratique ? Pas encore dans tous les régimes

La **préparation globale seule** passe d'un coût répété par rectangle
à O(n log n + R log n), hors propositions de cœur : unicité globale,
index global et requêtes de boîtes. Cela ne borne ni le nombre de
rectangles, ni le travail local, ni le volume de sortie.

À R=32 fixé, doublements 8k→16k→32k du bras partagé, tous s et ordres :

| Famille | Temps total × | Visites de comptage × | Candidates × |
| --- | ---: | ---: | ---: |
| Grille | 1,37–5,62 | 1,80–1,90 | 1,24–2,05 |
| Nappe complète | 1,29–2,79 | 2,35–2,83 | 2,24–2,62 |
| Déséquilibré | 1,98–2,60 | 2,10–2,50 | 2,18–2,25 |

Les compteurs de census croissent moins que ×4 sur ces séries finies.
Les chronomètres ne le font pas tous ; aucune borne globale ne résulte
de ces observations. Pour ne pas masquer le terme répété des facteurs,
trois campagnes supplémentaires font aussi doubler R, à s8/Kmax10,
sur grille et déséquilibré. Le seul compteur des copies de restrictions
est exactement |A|+R|B| :

| n | R | Grille : copies | Déséquilibré : copies |
| ---: | ---: | ---: | ---: |
| 8000 | 32 | 132 000 | 23 500 |
| 16000 | 64 | 520 000 | 79 000 |
| 32000 | 128 | 2 064 000 | 286 000 |

Sur grille, ce travail est multiplié par **3,94 puis 3,97**, malgré le
partage du nuage. Pour R proportionnel à n et B proportionnel à n, le
terme est mathématiquement quadratique. Il reste dans Pool/Axis et doit
être traité avant un passage massif général. Les nappes ne sont pas
rejouées dans cette seconde série : le verrou est déjà démontré par le
code et les deux familles, sans payer un census supplémentaire peu utile.

Les capacités conservées du nuage valent 240 000 / 480 000 / 960 000 octets
aux trois tailles ; celles de Z, séparées, valent 981 504 / 1 963 008 /
3 926 016 octets. Elles doublent dans ces builds et ne sont pas multipliées
par R en mémoire vive : un seul contexte est retenu dans chaque bras.
**Plans, buffers, métadonnées et pic RSS sont exclus** de ces capacités.
Ce ne sont pas des mesures de mémoire totale ou de VRAM.

## Protocole, tests et filiation

Quatre campagnes closes, 66 lignes, deux bras par ligne :

- `main_matrix` : 54, trois familles × trois tailles × s8/10/12 × deux
  ordres, Kmax10, R32.
- `growing_8k`, `growing_16k`, `growing_32k` : quatre lignes chacune,
  grille/déséquilibré × deux ordres, s8/Kmax10, R32/64/128 respectivement.

Les processus constructeur sont séquentiels, sans échauffement caché,
sans compilation ni autre grosse campagne constructeur simultanée.
La machine reste partagée ; la provenance déclarée ne certifie pas son
exclusivité. La génération, les copies, les index, Pool, Additive, le
census individuel, la collecte, le digest et les destructions sont payés.
Comparaison des bras, parsing et impression JSON ne font pas partie du
total de composant. Kmax10 désigne un seuil, pas dix hiérarchies calculées.
s vérifie la séparation des bandes fixes, pas une génération WSPD.

Le binaire Release GCC 13.3 est épinglé à
`a6ab14ad9d3272a36ece061aa4a4111447508ee3f0b39827f76764a39d68c25f`.
Les manifestes conservent sources, CMake, compilateur, machine, commandes,
bruts/base64 et hashes avant/après/fermeture. Le HEAD de contexte ne
remplace jamais les pins des contenus réellement consommés.
[SUMMARY.json](SUMMARY.json) conserve la projection complète par tuple et
ordre ; [VALIDATION.json](VALIDATION.json) conserve les deux lecteurs.

```bash
python3 -B morsehgp3D_v8/bench/run_cloud_reuse_matrix.py check morsehgp3D_v8/receipts/cloud_reuse_20260914 --summary
python3 -B -O morsehgp3D_v8/bench/run_cloud_reuse_matrix.py check morsehgp3D_v8/receipts/cloud_reuse_20260914
```

La lecture future exige ces sources épinglées ; ne pas attribuer aux
captures une implémentation ultérieure. Le lecteur contrôle la cohérence
des reçus, pas la géométrie ni une reconstruction du binaire.

[QUALIFICATION.json](QUALIFICATION.json) conserve 55 pins de sources/juges
et les XML de **37 CTests par build**, en deux groupes disjoints 35+2,
Release et Clang ASan/UBSan. Le nouveau gate passe 7 780 contrôles,
19 nuages, 2 404 requêtes de boîtes, 24 census confrontés au juge,
89 entrées invalides et 17 contre-modèles. Les gates de reçus passent
normal/−O : 12 petites captures, 31 mutants, deux échecs réels conservés.
Le premier essai sanitizer en sandbox échoue à cause de ptrace/LSan :
son XML est conservé, puis la suite passe hors sandbox avec autorisation,
**sans désactiver LeakSanitizer**. Les échecs de compilation de développement
sont signalés séparément, pas présentés comme une passe de qualification.

Les builds `v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont désormais
épinglés. Aucun artefact historique n'est remplacé. La prochaine tranche
doit viser front réel, petits facteurs, certificats et préparations partagés,
avant workers CPU/GPU. P0, q3/q4, FULL, tours 50k et massif G4 restent ouverts.
