# reuse1 — réemploi des verticales régulières, capture close

Source poussée `ae817d09ec66e8024c5947f178cf2b1b3ab766e7`, session
`v11.20261003.reuse1`, fermée `completed` : worker 0, DONE 0, arrêt ciblé,
génération, retrait des clés et libération de réserve certifiés. Aucun
`error`, `warning`, flux tronqué ou groupe résiduel tué.

**3 339/3 339 portes de matrice et 299/299 ASan18 passent.** La matrice compte
Release 615, mutants 21, ASan/UBSan, TSan, u21 et u24 chacun 540, poison 541,
style 2 ; Clang Release facultatif absent. Les 292 mutants ont leurs IDs et
causes recoupés dans LastTest et JUnit : **290 morts par code/ligne et deux
constructions attendues**, sans survivant ni compilation inattendue admise.

Les **29 essais FULL K1..5** terminent, sans omission ni divergence : neuf
triples W48 modes 511/1023/2047, plus ng00/u21 en mode 2047 à W1 et W8.
Le mode 511 comprend le catalogue une passe, les descentes régulières et
verticales parallèles, le mémo et les espaces census réutilisés. Le mode 1023
ajoute le lookup dense des naissances ; 2047 ajoute le réemploi des graines
verticales régulières. Calendrier, argv, masques de comparaison 1167/1423 et
contrôles viennent exclusivement des scripts de la source exécutée.

Les six groupes ont des empreintes sémantiques égales, ainsi que les
empreintes d’octets au sein d’un profil. Les compteurs structurels, le travail
payé à mode comparable, les lots, la mémoire et les durées sont rejugés pour
chaque flux courant, même après réemploi d’un résumé. Le lecteur conserve le
comparateur v5 entre census possédé et réutilisé, mais **zéro paire entre ces
deux voies existe ici** : les trois modes utilisent le census réutilisé. Ses
contretests synthétiques ne deviennent pas de nouvelles mesures natives.

## Temps observés

Temps mur **FULL natif en millisecondes**, W48, une exécution par cellule.
FULL inclut index, catalogue/lookup, forêts et verticales.

| Entrée | Profil | Mode 511 | Mode 1023 | Mode 2047 |
|---|---:|---:|---:|---:|
| ng00, 39 885 sites | u21 | 1 622,498 | 1 435,693 | 1 463,154 |
| ng00 | u24 | 1 544,861 | 1 472,306 | 1 482,558 |
| ng01, 35 551 sites | u21 | 1 274,242 | 1 218,583 | 1 154,972 |
| ng01 | u24 | 1 220,378 | 1 190,393 | 1 154,372 |
| ng02, 45 845 sites | u21 | 1 682,725 | 1 543,061 | 1 514,543 |
| ng02 | u24 | 1 639,546 | 1 678,693 | 1 530,630 |
| uniforme 8k | u21 | 430,322 | 407,152 | 376,614 |
| uniforme 16k | u21 | 1 019,618 | 921,243 | 838,592 |
| uniforme 32k | u21 | 1 986,195 | 1 970,905 | 1 844,431 |

ng00/u21/mode2047 : **15 306,545 ms à W1**, **2 511,900 ms à W8** et
**1 463,154 ms à W48**, avec sorties et travail comparable contrôlés.
W désigne des travailleurs CPU, pas un nombre démontré de cœurs physiques.
Le mode 2047 ne réduit pas uniformément le temps FULL par rapport à 1023 :
les deux mesures ng00 sont légèrement supérieures. Une exécution par option,
dans l’ordre déclaré, ne fournit pas une distribution de performances.

Sur ng00/u21/W48/mode2047, domaine **804,679 ms**, forêt **658,042 ms**,
index **0,412 ms**. Le réemploi remplace 857 771 des 857 891 descentes
verticales ; 120 restent calculées. À k2, les 101 089 graines sont toutes
réutilisées, donc aucun lot de résolution verticale n’est lancé. Le balayage
fermé et les vérifications de tous les enfants restent effectués. Ce compte
de travail évité n’est pas un facteur d’accélération mesuré.

Les [métriques dérivées](metrics.json) séparent les coûts : sur 29 essais,
FULL totalise **53,437 s**, les processus natifs **69,972 s**, le traitement
sémantique Python **226,133 s**, la campagne **297,688 s**. Lecture d’entrée,
Cloud, Pool et décodage sont hors du chronomètre FULL. Les intervalles
parallèles cumulés ne se soustraient pas au temps mur. Vingt résumés ont été
réutilisés après lecture/hash complets pendant le banc ; neuf ont été décodés.

ng00/u21/mode2047 réserve 5 226 784 octets pour le cache vertical, 7 657 920
pour les espaces census ; lookup retenu 21 066 676 octets, pic Buffer total
344 267 712 octets. Ces valeurs ne sont ni le RSS, ni la mémoire Python,
ni une mesure de pile. Les réservations temporaires sont jugées pendant leur
coexistence, pas ajoutées arbitrairement aux pics historiques.

Les trois entrées LiDAR sont les sous-nuages sans sol entiers déjà préparés à
1 mm de la même séquence 08 ; elles ne remplacent ni les trames brutes ni
plusieurs séquences. u21/u24 traitent les mêmes entiers : largeur arithmétique
différente, grille identique. Aucun GPU, hiérarchie sur les points ou clustering
n’est qualifié par ce reçu. **Le contrat FULL 200 ms reste ouvert** : les six
mesures LiDAR W48/mode2047 sont comprises entre 1 154,372 et 1 530,630 ms.

## Relecture reproductible

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/check.py
python -B -O morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/check.py
python -B morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/check_selftest.py
python -B -O morsehgp3D_v11/receipts/full_regular_vertical_20261003/reuse1/check_selftest.py
```

Le lecteur exige le reçu brut LIVE, l’archive originale et le paquet source
local ; le déplacement bit-identique de ce dernier par symlink est accepté.
Il relit les membres du paquet épinglé, sans commande Git et sans import WIP.
Les sept helpers historiques et treize modules Python du banc sont copiés et
hachés avant import. Source, manifestes, copies, configurations, cache/flags,
provenances, inventaire et fermeture sont recoupés. Préflight : 1 600 s de
commandes + 120 s de préparation = **1 720 ≤ 1 737 s** utiles.

Les gros payloads FULL ont été supprimés après lecture distante : **29
empreintes enregistrées comparées, zéro dump réel rehaché ici**. Le lecteur
rejoue les contrôles des événements avec les résumés conservés ; il ne
reconstruit pas les grandes forêts. Un payload singleton synthétique est
réellement décodé dans l’auto-test. Les oracles natifs des petites fixtures
restent distincts de l’égalité des empreintes des grands essais.

[reader_proof.json](reader_proof.json) conserve les lectures normal/−O
identiques et les contretests : **19 témoins, 126 corruptions rejetées**,
aucun natif ou appel cloud ajouté. Les erreurs de préparation du lecteur et
les anciennes mutations devenues sans effet sont consignées séparément.
La contrelecture indépendante du lecteur est favorable et bornée à ses
sources figées, calendrier, événements et réemplois ; elle ne constitue pas
un nouvel audit exhaustif des helpers historiques.

Une seule archive de résultats est conservée. Aucun paquet source, binaire
natif ou payload LiDAR n’est dupliqué dans la capsule.
