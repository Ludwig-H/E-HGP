# Contrelecture B — plan q2 par masse, sonde v16

23 septembre 2026. Source publiée par `f685461a` (travail du développeur
initialement à `acf58fba`). Lecture du port et des reçus R10 ; aucun nouveau
test lourd ou GCP par B. Le cadre reste CPU de référence, grille entière
1 mm, `complete_relative`, statut public `not_claimed`.

## Ce qui est acquis, et ce qui ne l'est pas

Le plan q2 prépare maintenant le plus gros produit de nœuds en premier,
puis ordonne les jobs par masse ; la chaîne passe de 16 à 64 jobs par
worker quand `q2_jobs_by_mass=ON`. `Front::expand` porte ses masques et
témoins dans la tâche, non dans l'ordre global de visite. Le mode Coarse
distribue les jobs par compteur atomique ; ses comptes de fin vérifient
la partition. La porte `wspd_q2_parallel_gate.cpp` compare les supports
triés et les compteurs de travail mono entre l'ancien plan et le plan
par masse sur au moins 400 exécutions : bonne preuve locale de neutralité
du nouvel **ordonnancement**, pas preuve de performance à l'échelle.

Le gain annoncé, `q2=1 711→1 048 ms` sur 08/000000 sans sol,
K5/s8/W8, provient d'un seul cas local. Il n'y a ni reçu brut épinglé,
ni répétitions, ni distribution de temps par job pour attribuer ce gain.
La porte rejoue le plan par masse avec 1, 4 ou 32 jobs/worker, **jamais
les 64 du chemin produit** ; elle ne compare pas directement la tour
complète ON/OFF. Le mutant du champ JSON vérifie le protocole, pas
l'égalité du résultat géométrique pour ce levier au réglage de production.

Le schéma v16 et les dix leviers sont raccordés à la sonde et au lecteur
G4 ; les anciens reçus v15 doivent rester lus avec leur lecteur épinglé.
Le runner local de croissance ne reconnaît explicitement que v12 et v16.

## Taille maximale du gain pour le contrat, d'après R10

Sur le **meilleur** cas CPU G4 R10, 08/000100 sans sol, W48/s8 :

| tour | chaîne observée | q2 observé | chaîne même si q2 devenait gratuit | q3/q4 | FULL |
| --- | ---: | ---: | ---: | ---: | ---: |
| K1..5 | 2,736 s | 0,231 s | 2,505 s | 1,653 s | 0,658 s |
| K1..10 | 8,066 s | 0,416 s | 7,651 s | 4,492 s | 2,455 s |

La colonne « q2 gratuit » est une soustraction analytique sur ce reçu,
pas un chrono prédit : changer le plan peut modifier contention, cache
et autres phases. Elle suffit à montrer que **v16 q2 seul ne peut pas
atteindre une seconde**, même sur la meilleure des trois trames R10.
Au K10, rendre q3/q4 et FULL gratuits laisserait encore 1,119 s sur
ce même chemin observé. Il faudra donc aussi optimiser le résidu,
mais le gros volume de travail réel reste q3/q4 puis FULL. La saturation
des 48 CPU sur q3/q4 en R10 écarte l'espoir d'un gain massif par un
simple nouvel ordonnancement de ces tâches.

## Porte proportionnée avant une revendication G4

Ajouter une petite fixture **64 jobs/worker** qui compare sorties,
ledger et tour ON/OFF au réglage produit. Puis, si une session G4 est
ouverte, mesurer une ablation appariée ON/OFF dans **le même binaire**,
les mêmes octets et la même configuration, en publiant `partition_ms`,
nombre de jobs, temps max/médian par worker, `q2_ms`, CPU/mur, RSS,
condensé et comptes q2. Un premier cas court peut décider si une
campagne plus large vaut son coût. Ni le gain W8 isolé ni les portes
actuelles n'établissent une pente sous-quadratique ou le contrat de tour.
