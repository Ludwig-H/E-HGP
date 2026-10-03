# FULL forest3 — capture close du 3 octobre 2026

Source `3dbfd1c328561845584d39a204e34de6a054052e`, session
`/workspaces/.ehgp-sessions/v11.20261003.forest3`. Cette capsule conserve la
campagne **failed_remote**, worker1, DONE3 : 18 succès sur19 demandes et
uniforme32k/u21/mode15/W48 **non lancé pour budget restant insuffisant**.
Le rapport est clos (`complete=true`), mais le calendrier et la campagne
ne sont pas conformes. Aucun échec géométrique n'est déduit de cette omission.

Le reçu LIVE original, son empreinte et le DONE local restent obligatoires.
Arrêt ciblé certifié, génération contrôlée, clé privée supprimée, clé OSLogin
retirée et réserve libérée ; erreurs/avertissements vides. L'archive originale
unique, 623341 octets, a pour SHA256
`85bbdc0919d500dc311c1450d41b83c98fcbbf44154c1b5f953de3a3c9499724`.
Son manifeste est relu intégralement ; aucun membre tronqué ou omis.
Les trois copies compactes sont comparées aux octets archivés. Aucun binaire,
paquet source ni donnée KITTI n'est copié ici.

Le lecteur réimporte exclusivement les neuf helpers `source_3db/`, avec
empreintes et octets recoupés par `git show` à la source. Le plan, les deux
matrices et les sept manifestes de mutants sont épinglés de la même façon.
Les anciens lecteurs de transport/provenance sont importés sans modification.
Les options et argv réellement exécutés sont rapprochés du plan figé.
Le manifeste d'entrée est une copie identique à celui d'assembly1, admise
seulement après égalité SHA256/taille avec le fichier annoncé dans le reçu LIVE.

Les portes observées et rejugées sont 2673/2673 pour la matrice et 201/201 pour
ASan18. Les 254 mutants sont 252 rejets par code/ligne et deux refus de
construction explicitement attendus ; aucun mort par signal/délai.
Le lecteur relie chaque ligne causale à son test JUnit passé et à sa section
complète LastTest. Cela ne transforme pas les refus de construction en
exécutions natives. La configuration Clang est absente, conservée comme telle.

Les six paires LiDAR 7/15 passent, ainsi que les contrôles W1/W8/W48 à mode15,
la paire sans mémo3/11 sur ng00/u21, et les uniformes8k/16k. Les cinq groupes
avec résultats sont égaux ; le groupe32k reste incomplet. Au sein d'un profil,
RAW est identique ; semantic est identique entre profils/modes/workers.
Le travail invariant, le travail payé à mode constant et la couverture des
cellules régulières/étendues sont recoupés séparément.

| Scène/profil | FULL mode7 (s) | FULL mode15 (s) | Ratio7/15 |
|---|---:|---:|---:|
| ng00/u21 |14,790|6,163|2,400|
| ng00/u24 |14,820|6,119|2,422|
| ng01/u21 |10,209|4,408|2,316|
| ng01/u24 |10,113|4,358|2,320|
| ng02/u21 |12,074|5,548|2,176|
| ng02/u24 |11,855|5,537|2,141|

Un seul processus neuf par unité, sans répétition statistique. Mode 7 = cacheJ2,
tri indirect, mémo sériel ; mode 15 ajoute les lanes privées de descentes
régulières. Le catalogue FULL de cette source ne reçoit que les options 3.
Les compteurs de travail payé peuvent donc changer entre 7 et 15 ; ils restent
égaux entre W1/W8/W48 pour 15. Sur ng00/u21, ces trois W donnent respectivement
35,569/7,808/6,163 s FULL ; W agit aussi sur le catalogue, donc ce rapport
n'est pas une accélération isolée de la forêt. La paire sans mémo3/11 donne
18,425/6,394 s FULL et15,084/2,989 s forêt.

Ng00/u21/mode 15 : domaine 3,385942 s, forêt 2,776397 s. Les intervalles par ordre
additionnés donnent classification 68,997 ms, naissances 280,464 ms,
plateaux 814,476 ms et verticales 1599,695 ms. Ces phases sont disjointes mais
n'épuisent pas nécessairement le mur API. Leurs détails et les réservations
Buffer figurent dans `metrics.json`, recalculé par le lecteur ; ni RSS ni pic
par ordre ne sont déduits. La somme des durées des tâches n'est pas un mur.

Coût de campagne : 370,451 s, dont 188,778 s dans les processus et 180,953 s de
lecture/validation sémantique, distincts du chrono FULL. Dix résumés ont été
réutilisés après relecture/hash intégral des octets dans le banc source 3db ;
le lecteur vérifie contexte, source antérieure réussie, RAW/taille/résumé et
les diagnostics de chaque tentative. **Les 18 gros payloads ont été supprimés
sur la VM : leurs hashes sont enregistrés, pas recalculés par cette capsule.**
Une petite fixture synthétique archivée en mémoire est réellement décodée par
l'auto-test ; ce n'est pas une nouvelle lecture des sorties LiDAR.

Le lecteur LIVE et son auto-test passent en Python normal et−O ; résultats
identiques, 17 témoins et 94 corruptions causales. `check_selftest.json` conserve
une seule copie des sorties communes et les quatre commandes/empreintes.
Les corruptions couvrent fermeture/copies, calendrier/omission, provenance,
compteurs/temps de lanes, travail comparable, cache sémantique, faux succès,
mutants et décodage réel de la petite fixture.

Commandes depuis ce dossier : `python check.py`, `python -O check.py`,
`python check_selftest.py`, `python -O check_selftest.py`. Un code0 du lecteur
signifie **preuves cohérentes**, avec `conforming=false` pour cette campagne.
Aucun nouveau calcul natif, aucune mutation cloud, aucun contrat 200 ms acquis.
Les optimisations q3 contrôlé, catalogue une passe et futures verticales
parallèles n'héritent pas de cette qualification.
