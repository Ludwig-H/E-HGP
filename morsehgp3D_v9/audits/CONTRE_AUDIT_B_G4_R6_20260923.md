# Contre-audit B — R6 G4, noyau diamétral ON/OFF

23 septembre 2026. Reçu [R6](../receipts/g4_tower_r6_20260923/README.md)
publié sur `main` par **`50690c12`**, exécuté sur snapshot produit
**`78ce9fd4`** : G4 SPOT, **CPU seulement**,
profil u18/grille 1 mm, trois trames SemanticKITTI **sans sol** de la
seule séquence 08 (39 885, 35 551 et 45 845 sites). Les 24 cas forment
trois scènes × K5/K10 × deux répétitions × cœur ON/OFF, avec s8, 48
workers et tour statique à 48 fils. Tous les autres leviers sont ON.
Le premier cas ON satisfait le préflight de cette session ; aucun test
GPU, float32, trame brute avec sol, s10/s12 ou autre séquence n'en découle.

## Réception et égalité effectivement prouvées

Contrelecture indépendante : les **390/390 empreintes** du reçu passent ;
paquet, trois entrées, worker et protocole correspondent au commit épinglé.
Le lecteur `validate_received` du même commit, rejoué dans un worktree
temporaire détaché, rend `completed`. Les valeurs archivées de marque et
de calendrier d'arrêt égalent exactement celles vérifiées par l'hôte ;
le contrôleur atteste l'arrêt ciblé et l'état `TERMINATED`. Le worker a
**24/24** cas `complete_relative`, `FULL_executed=true`, et les
**18/18** comparaisons de projections d'objet prévues sont égales. Aucun autre appel
GCP n'a été fait par l'auditeur.

Sur les **12 paires** ON/OFF, entrée/K/s/W/répétition et les options hors
cœur coïncident. Les champs JSON `catalogue` (statistiques, **pas** les
millions de boules), `orders` (résumés), `tower_work`, le condensé FNV-64
de la tour et les comptes de présentations q3/q4 coïncident. Dans `generator`,
`q34_cover_builds` diffère **par construction** ; les autres champs
comparés concordent. Le lecteur croisé vérifie la projection logique
catalogue/ordres/condensé, pas les listes de toutes les clés, supports,
profondeurs et IDs de coquille q3/q4 : R6 ne prouve donc pas l'égalité
octet par octet de ce flux ; FNV-64 n'est pas non plus une preuve
sans collision. Un comparateur local distinct l'a vérifiée
sur **deux coupes** de 08/000000, pas sur ces trois trames entières.
L'exactitude globale d'un catalogue sans clé omise demeure une obligation
mathématique distincte de `complete_relative`.

## Temps appariés et travail total

Temps en secondes, **OFF → ON**, répétitions 0 / 1. Les deux répétitions
ont le **même ordre ON puis OFF** : comparaison appariée utile, mais pas
un essai randomisé qui élimine toute dérive temporelle.

| Scène 08/ | K | Chaîne totale OFF → ON | q3/q4 OFF → ON |
| --- | ---: | --- | --- |
| 000000 | 5 | 6,031→6,000 / 5,996→5,765 | 3,978→3,945 / 3,915→3,728 |
| 000000 | 10 | 17,180→16,768 / 17,028→16,666 | 8,452→8,129 / 8,371→8,080 |
| 000100 | 5 | 4,212→4,151 / 4,243→4,151 | 2,692→2,634 / 2,731→2,610 |
| 000100 | 10 | 12,012→11,726 / 12,067→11,900 | 5,528→5,321 / 5,492→5,254 |
| 000200 | 5 | 7,520→6,869 / 7,419→6,917 | 5,312→4,628 / 5,202→4,657 |
| 000200 | 10 | 19,343→18,062 / 19,824→18,110 | 11,129→9,716 / 11,491→9,829 |

Le mur de chaîne diminue dans **12/12 paires**, de **0,514 à 8,663 %** ;
q3/q4 de **0,843 à 14,471 %** ; le CPU cumulé de chaîne de **8,087 à
21,175 %**. Le meilleur temps FULL observé demeure **4,150784 s** à
K1..5 et **11,726237 s** à K1..10 (08/000100). Les pics RSS alternent
hausse et baisse avec le cœur, sans économie mémoire stable.

Les identités de travail passent dans les douze paires :
`ON.core_builds=OFF.cover_builds` et
`ON.cover_builds+ON.core_closed_edges=ON.core_builds`. Le cœur ferme
**54,9–58,9 %** des arêtes qui auraient construit un cover ; les paires
développées et les rejets de témoins sont **inchangés**. Les formes
chargées **cœur + cover** baissent de **72,9–81,9 %** et les tests
uniformes des deux prouveurs de **64,1–76,7 %** ; en contrepartie, les
visites d'index des deux covers **augmentent de 21,0–45,6 %** et les
cellules des deux preuves de **51,2–55,1 %**.
Exemple 000200/K10, première paire : 4,927 M covers OFF, contre
2,027 M covers ON et 2,901 M arêtes fermées par le cœur ; formes
9,281→2,028 milliards, mais visites 1,251→1,638 milliard. Ce coût
supplémentaire explique pourquoi supprimer beaucoup de formes ne
produit qu'un gain de chaîne modeste.

Le schéma de sonde **v8** de R6 compte encore la passe de vérification
`tower_digest` dans `chain_total` et `chain_cpu_s`. Le chantier v9 qui
isole cette passe en `times_ms.digest` change la frontière des chronos :
une future campagne ne doit pas être comparée numériquement à R6 sans
réadditionner le digest ou annoncer explicitement la nouvelle portée.

Le [README R6](../receipts/g4_tower_r6_20260923/README.md) arrondit trop
ses plages : « CPU −10 à −20 % » omet **−8,09 et −21,18 %** ; « mur −2
à −8 % » omet **−0,51 et −8,66 %** ; « formes −77 à −82 % » doit commencer
à **−72,85 %** (08/000100/K10). Sa formule « générateur identique » doit
viser les **sorties**, non `q34_cover_builds`. Ce sont des corrections
de lecture du reçu, pas une invalidation de ses fichiers bruts.
L'[erratum publié ensuite](../receipts/g4_tower_r6_20260923/ERRATUM.md)
corrige le compteur de covers mais reste trop large avec « émissions »,
« catalogue » et « travail FULL » identiques : le reçu n'archive que leurs
**nombres/statistiques**, les résumés d'ordres et le condensé, pas les
listes complètes de supports/boules. Les compteurs du cache témoin
varient aussi entre ON/OFF des mêmes paires, et pas seulement entre
répétitions. L'erratum est explicitement hors du `SHA256SUMS` de R6.

## Verdict et prochain verrou

Le noyau est une optimisation réelle sur ces trois trames sans sol à
s8/W48, mais il agit **après** l'expansion `A×B`. R6 conserve jusqu'à
**32,79 millions de paires développées** et des milliards de formes ;
il ne prouve aucune croissance sous-quadratique. Le contrat de la tour
en **1 s**, puis **100 ms**, n'est pas atteint. La suite prioritaire
reste un certificat de groupes q3/q4 **avant** l'expansion, puis la
réduction de l'aval FULL ; tester 8k/16k/32k selon les coupes capteur,
trames entières de plusieurs séquences, avec et sans sol, s8/10/12,
et comparer GPU/G4 seulement lorsqu'une route GPU complète existe.
