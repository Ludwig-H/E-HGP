# census2 — FULL et census réutilisé, capture close

Source poussée `cc93360a3532790394d7a53c6ff1e11c5b25f952`, session
`v11.20261003.census2`, fermée `completed` (worker 0, DONE 0).
Arrêt ciblé, génération, retrait des clés et libération de réserve certifiés ;
aucun `error`, `warning`, flux tronqué ou groupe résiduel tué.

**3 141/3 141 portes de matrice et 266/266 ASan18 passent.** La matrice comprend
Release 582, mutants 21, ASan/UBSan, TSan, u21 et u24 chacun 507, poison 508,
style 2 ; Clang Release facultatif absent. Les 281 mutants ont leurs IDs et
causes vérifiés dans LastTest, relié à JUnit : 279 morts par code/ligne et deux
constructions attendues. `forest_cohort_nonbirth_reset` est ici tué par code.
Son refus de compilation dans [census1](../census1_failure/README.md) reste
conservé ; aucune requalification rétroactive.

Les **29 essais FULL K1..5** terminent, sans omission ni divergence : neuf
triples W48 modes 127/255/511, plus ng00/u21 en mode 511 à W1 et W8. Le mode 127
active notamment catalogue une passe et descentes régulières parallèles ; 255
ajoute les verticales parallèles ; 511 ajoute les espaces census réutilisés.
Le calendrier et les argv sont ceux des scripts figés, jamais ceux du WIP.

Les six groupes ont les mêmes empreintes sémantiques ; les octets déclarés
sont identiques dans chaque profil. Les compteurs structurels, les comptes de
lots, le travail payé à mode de descente fixe et les diagnostics mémoire/temps
sont rejugés sur chaque flux courant, y compris après réemploi d’un résumé.
La comparaison v5 entre census possédé et réutilisé conserve les compteurs
bruts et normalise seulement les tests de points : facteur deux sur la voie
réutilisée, parce que la voie possédée parcourt deux fois. **19 paires d’essais,
95 paires d’ordres, toutes positives**, vérifient cette égalité et celle des
autres travaux. Ces paires partagent des essais : elles ne sont pas des
répétitions indépendantes. Les réservations census vérifiées sont
`4 × sites × min(W,48,4096)` octets en mode 511, en plus des mémos coexistants.

## Temps observés

Temps mur **FULL natif en millisecondes**, une exécution par cellule du tableau,
W48. Le chronomètre FULL comprend index, catalogue/lookup, forêts et verticales.

| Entrée | Profil | Mode 127 | Mode 255 | Mode 511 |
|---|---:|---:|---:|---:|
| ng00, 39 885 sites | u21 | 2 929,375 | 1 731,863 | 1 588,351 |
| ng00 | u24 | 2 985,761 | 1 725,052 | 1 680,120 |
| ng01, 35 551 sites | u21 | 2 386,683 | 1 334,174 | 1 280,499 |
| ng01 | u24 | 2 372,650 | 1 381,529 | 1 282,091 |
| ng02, 45 845 sites | u21 | 3 077,030 | 1 873,443 | 1 769,845 |
| ng02 | u24 | 3 093,134 | 1 796,492 | 1 792,828 |
| uniforme 8k | u21 | 876,155 | 479,207 | 442,647 |
| uniforme 16k | u21 | 2 011,443 | 1 088,598 | 976,099 |
| uniforme 32k | u21 | 4 416,145 | 2 177,714 | 2 017,249 |

ng00/u21/mode511 : **18 342,901 ms à W1**, **2 937,780 ms à W8** et
**1 588,351 ms à W48**, avec sorties et travail discret contrôlés. W désigne
les travailleurs CPU, pas un nombre démontré de cœurs physiques.

Les [métriques dérivées](metrics.json) séparent les phases et les coûts : sur
les 29 essais, FULL totalise **71,847 s**, les processus natifs **88,229 s**,
le décodage/rehash Python **225,498 s**, la campagne **315,242 s**. Cloud,
lecture d’entrée, Pool et décodage sont hors du chronomètre FULL ; les temps
parallèles cumulés ne se soustraient pas au mur. Vingt résumés ont été réutilisés
après lecture/hash complets au moment du banc ; neuf ont été décodés.

Sur ng00/u21/mode511, domaine 837,462 ms et forêt 750,462 ms ; réservation
census 7 657 920 octets et pic Buffer 344 267 712 octets. Ces réservations ne
sont ni le RSS, ni la mémoire Python, ni une mesure de pile.

Les entrées LiDAR sont les trois sous-nuages sans sol entiers déjà préparés à
1 mm de la même séquence 08 ; elles ne remplacent pas les trames brutes ni
plusieurs séquences. u21/u24 traitent les mêmes entiers et changent la largeur
arithmétique, pas la grille. Aucun calcul GPU, projection sur les points,
clustering, ni objectif FULL 200 ms acquis. Une seule exécution par option ne
constitue pas une distribution de performances ou une preuve de gain général.

## Relecture reproductible

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v11/receipts/full_census_20261003/census2/check.py
python -B -O morsehgp3D_v11/receipts/full_census_20261003/census2/check.py
python -B morsehgp3D_v11/receipts/full_census_20261003/census2/check_selftest.py
python -B -O morsehgp3D_v11/receipts/full_census_20261003/census2/check_selftest.py
```

Le lecteur exige les objets Git, le reçu brut LIVE, l’archive originale et le
paquet source local (déplacement bit-identique par symlink accepté). Il vérifie
source, manifestes, copies, configurations, cache/flags, provenances, inventaire
et fermeture. Les sept helpers historiques et onze modules Python du banc
sont copiés et hachés à la source exécutée, avant import. Les contrôles v5 entre
voies sont préservés. Préflight : 1 600 s de commandes + 120 s de préparation
= **1 720 ≤ 1 737 s** utiles ; aucun délai déclaré n’est effacé.

Les payloads FULL ont été supprimés après lecture distante : **29 empreintes
enregistrées comparées, zéro dump réel rehaché ici**. Le lecteur rejoue les
contrôles de métadonnées, travail et temps à partir des résumés conservés ; il
ne reconstruit pas les grandes forêts. Un payload singleton synthétique est
réellement décodé dans l’auto-test. Les oracles natifs des petites fixtures
restent des portes distinctes de l’égalité des empreintes des grands essais.

[reader_proof.json](reader_proof.json) conserve les lectures normal/−O et les
mutations : **16 témoins, 107 corruptions rejetées**, aucun natif ou appel cloud.
Les erreurs de raccord initiales du lecteur sont consignées séparément. La
capsule conserve une seule archive de résultats et des fichiers légers ; aucun
paquet source, binaire natif ou payload LiDAR n’y est dupliqué. La contrelecture
de cette capture ne prétend pas réauditer exhaustivement les helpers historiques.
