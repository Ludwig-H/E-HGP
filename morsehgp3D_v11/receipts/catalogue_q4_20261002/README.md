# Niveau q4 différé : qualification et mesures G4 closes

Source publiée **`ffc2ff95f0ae7296bdc522df81df34c58c3fdf47`**, session
`v11.20261002.q4levels1`. Le catalogue matérialise le niveau q4 après positivité,
propriété, census complet, support canonique et admission. Les deux passes gardent
leur contrat. Aucun autre élagage, allocation ou réglage produit modifié.

Qualification : Release u18 **229/229** ; ASan/UBSan u24, TSan u21 et profils
21/24 **154/154 chacun** ; poison u21 **155/155** ; style 2/2, mutants 12/12.
Complément num ASan/UBSan u18 **14/14** dans une commande séparée. Clang absent.
119 mutants détectés : 78 core, 16 num, 16 cloud, 9 catalogue ; deux refus de
compilation attendus dans core, aucun signal ou délai compté comme détection.
La porte candidate joue 831 contrôles ; Fraction 528 géométries et 160 entiers,
11 838 contrôles par oracle normal/−O. Les trois nouveaux mutants num ciblent
le signe du candidat, son ancre et son niveau matérialisé.

## Temps et travail

Temps de l'appel catalogue CPU mono, deux passes, tri et sortie mémoire compris.
Hors lecture/Cloud, segmentation, sérialisation et décodage Python. Réglages
leaf16/max_leaf256, budget 8 GiB ; une répétition par entrée/profil. Les six nuages
entiers et leurs IDs sont ceux de `profiles1`, sans changement de grille 1 mm.
Le plafond de 30 s porte sur tout le processus natif ; un délai n'est pas sa durée finale.

| Entrée entière, K5 | u18 | u21 | u24 |
| --- | ---: | ---: | ---: |
| Uniforme 8k | 7,180 s | 7,743 s | 7,818 s |
| Uniforme 16k | 15,104 s | 16,345 s | 16,404 s |
| Uniforme 32k | délai 30 s | délai 30 s | délai 30 s |
| LiDAR 08/000000, 39 885 sites | 23,380 s | 24,962 s | 24,794 s |
| LiDAR 08/000100, 35 551 sites | 18,457 s | 19,777 s | 19,673 s |
| LiDAR 08/000200, 45 845 sites | 21,568 s | 23,101 s | 23,176 s |

33 tentatives : 15 réussites K5, 18 délais ; les trois K10/32k sont omis après
leur échec K5. Tous les K10 joués expirent à 30 s. Le banc clos reste **ECHEC**,
worker 1/session 3 ; il ne satisfait pas son calendrier complet ni le contrat de 100 ms.
Les trois LiDAR sans sol sont de la seule séquence 08. FULL et GPU sont absents.

Les **15 sorties égalent octet pour octet** celles de `profiles1`, à même
profil et entrée. Empreintes sémantiques, neuf compteurs géométriques, boules,
niveaux, incidences, pics Buffer et réservations finales sont aussi identiques.
Les cinq cas réussis ont les mêmes sémantiques et comptes aux trois profils.
Les rapports de durées ancien/nouveau vont de ×1,033 à ×1,053 : une nouvelle
répétition et des sessions distinctes ne prouvent pas un gain statistique stable.

| Par passe, identique aux trois profils | Centres q4 non dégénérés | Niveaux q4 matérialisés |
| --- | ---: | ---: |
| Uniforme 8k | 15 087 761 | 122 218 |
| Uniforme 16k | 31 354 208 | 253 794 |
| LiDAR 08/000000 | 52 878 237 | 158 494 |
| LiDAR 08/000100 | 41 963 267 | 121 303 |
| LiDAR 08/000200 | 48 091 032 | 143 105 |

Les deux colonnes sont réellement payées deux fois. Le nombre de niveaux égale
les boules qmin4 sérialisées : environ 99,7 % des niveaux candidats sont évités
sur LiDAR. Ce compte ne constitue ni un gain temporel équivalent, ni une borne
de croissance globale. Les refus, la complétude et les anciens travaux restent jugés.

## Pièces et lecture

`q4levels1/` conserve un seul tar original de 263 356 octets, les résumés matrix,
asan18 et profiles, le manifeste d'entrée sans données KITTI et le reçu de
fermeture compact. Arrêt ciblé certifié le 2 octobre à 15:49:35 UTC ; clés privée et
OS Login retirées, verrou/réserve libérés. Aucun natif local.

```bash
python3 -B morsehgp3D_v11/receipts/catalogue_q4_20261002/check.py
python3 -B -O morsehgp3D_v11/receipts/catalogue_q4_20261002/check.py
```

Les deux lectures passent : baseline 15/15, matrice 1014/1014, supplément conforme.
Code 0 signifie pièces cohérentes, y compris le banc échoué, pas contrat acquis.
Le lecteur LIVE requiert les reçus locaux originaux et la capture précédente ;
les fichiers canoniques ont été supprimés après mesure, leurs hashes sont
comparés aux hashes enregistrés, sans relecture ultérieure des gros payloads.
`check_selftest.json` garde une sortie commune normal/−O : 12 témoins positifs,
41 corruptions refusées. Les anciennes captures et leurs lecteurs sont inchangés.
