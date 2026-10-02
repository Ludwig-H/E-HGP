# MEB bornée : qualification G4 du 2 octobre 2026

Source qualifiée **`25792084eb4e672c5222d62f5b2ae87bd2ee4948`**, session
`meb3`. CPU G4 SPOT, profils18/21/24, poids géométriques unitaires du banc.
**Aucune tour FULL ni contrat de temps de tour n’est acquis.**

## Qualification

Matrice **1266/1266** : Release 270, ASan24/TSan21/profils21/24 chacun 195,
poison 196, mutants 18 et style 2. Clang absent. Complément num/index/tower
ASan18 **55/55**. Les 141 mutants sont détectés : deux refus de compilation
attendus de core, aucun signal ni délai pris pour une détection ; les dix
nouveaux mutants tower sont tués par leur juge.

MEB : 372 contrôles natifs, 350 requêtes Gram/Fraction par profil,
14 631 contrôles, 31 paires de permutations et 18 refus. Le modèle du juge
passe 43 800 contrôles, 27 corruptions et trois JSON malformés, normal/−O.
La MEB possède son résultat, n’alloue rien et accepte 1..12 sites distincts.
Son support canonique est LOCAL ; il ne vaut pas identité globale.

## Mesures

**18/18 essais**, six entrées entières × trois profils, une répétition,
48 parties choisies par essai (quatre de chaque taille 1..12). Les 864
requêtes donnent 216 census complets et 648 saturés ; six comparaisons
interprofils identiques en sémantique et travail logique. Le témoin est
un scan global avec les primitives num qualifiées ; le juge Fraction
indépendant est celui des petites fixtures, pas celui de ces grands scans.

| Trame sans sol | 48 MEB u21 | 48 MEB+census u21 | 48 MEB u24 | 48 MEB+census u24 |
| --- | ---: | ---: | ---: | ---: |
| 08/000000 | 1,166 ms | 1,777 ms | 1,184 ms | 1,747 ms |
| 08/000100 | 1,178 ms | 1,513 ms | 1,170 ms | 1,502 ms |
| 08/000200 | 1,116 ms | 1,377 ms | 1,090 ms | 1,387 ms |

Sommes des 48 intervalles mesurés ; le wrapper est chronométré séparément
et ne vaut pas la somme des deux autres mesures. Lecture, Cloud, index,
préparation des parties, scan témoin et sérialisation sont séparés.
Le premier résultat de census reste vivant pendant le wrapper : son pic
comprend les deux sorties. Ces parties choisies ne prédisent ni le nombre
ni le coût des descentes FULL. Les trois LiDAR proviennent de la seule
séquence 08 ; mêmes coordonnées 1 mm historiques dans les trois profils.

## Échecs conservés et fermeture

- `meb1`, source `ab04bc7b` : **1254/1266** et complément 53/55 ; les deux
  portes IO échouent dans chaque configuration. Le range-for C++20 du
  banc empruntait les coordonnées d’une ancre temporaire. La correction
  conserve l’ancre ; le décodeur appliquait déjà correctement le signe.
  Aucun benchmark lancé ; worker 1, résultats rapatriés, arrêt certifié.
- `meb2`, source 257 : démarrage refusé pour manque de capacité G4. Aucun
  worker ni résultat natif. Le reçu garde `shutdown_uncertified` ; la
  récupération automatique ne connaît aucune nouvelle génération. La
  vérification externe répétée constate `TERMINATED`, ancienne génération
  inchangée, opération start terminée en stockout, aucun start en cours.
  Cette pièce ne devient pas une campagne ni un arrêt attribué à meb2.
- `meb3`, source 257 : worker 0, résultats vérifiés ; arrêt ciblé certifié
  le 2 octobre à 19:18:06 UTC, génération `2026-10-02T12:11:26.693-07:00`.
  Clés supprimées, inscription OS Login retirée et verrou libéré.

## Lecture et provenance

`python3 check.py` et `python3 -O check.py` vérifient les preuves LIVE ;
code 0 signifie cohérence des pièces, y compris les échecs explicités.
`check_selftest.py` éprouve le lecteur par corruptions, sans binaire natif.
Le reçu original local reste requis ; chaque archive est conservée une
seule fois. Les sorties canoniques supprimées sur G4 ne sont pas recalculées
par le lecteur : leurs empreintes publiées sont comparées.

`inputs.json` conserve le manifeste historique sans octet KITTI. Le
répertoire temporaire antérieur ayant disparu, `restore.py` reconstruit les
mêmes douze payloads depuis les sources locales : 34 empreintes contrôlées,
manifest inchangé. `restoration.json` est la provenance de cette restauration ;
il ne prétend pas rejouer le générateur historique absent. Aucun natif local.
