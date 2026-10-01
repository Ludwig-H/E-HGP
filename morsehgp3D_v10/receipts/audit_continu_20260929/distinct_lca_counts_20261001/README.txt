Comptages distincts par corrections LCA — témoin clos du 1er octobre 2026
=======================================================================

Statut : exploration_v10_hors_registre / cpu_reference / not_claimed.
Ce paquet juge une proposition exacte sur 20 profils persistants abstraits
(n=6), pas un export FULL géométriquement réalisé ni un moteur natif.
GCP non utilisé ; aucun chrono LiDAR/GPU/100ms ou sous-quadratique global.

Résultat conservé
-----------------
180 cardinalités naissance/avant-mort, vraie AST StructureER 99e8,
oracle par ensembles, Fenwick et corrections LCA concordants.
31 corrections : 27 sur naissance B, quatre sur avant-mort F.
Les cinq contrôles arithmétiques exposent respectivement 4/12/10/17/5
cas erronés : toutes corrections en B, toutes en F, somme des enfants,
plafonnement mcs2 pris pour égalité, clamp des corrections signées.
Ce sont des variantes RAM, pas cinq campagnes de mutants compilés.

Ces profils ne sont PAS élagués en antichaînes : les corrections F
restent nécessaires. Le cas même propriétaire/entrée précoce et tardive
est positivement exercé. La simplification après antichaîne, ses 630
admissibilités et 126 sigma appartiennent à une autre sonde OPEN ;
leur qualification n'est pas héritée de ce reçu. Vies positives,
graines COMPLÈTES et propriétaires normalisés restent des hypothèses.

Capture et contre-lectures
-------------------------
Préparation R2 entièrement relue avant fermeture et unique record.
Commandes normales/−O, checkpoints, stdout/stderr bruts, issues terminales,
UTC, Python et hashes avant/après conservés dans capture/.
Deux codes0 ; aucun signal, timeout, erreur ou stderr. Les sorties
normale et−O ont exactement les mêmes octets :
682094cb79749d9f479cf2f91e42879ab88d5abceaef955fcef0c4d759ff400e.
La gestion des interruptions est auditée dans le code, NON exercée
causalement par cette capture sans signal. SIGKILL/panne noyau/I/O
ne sont pas promis comme récupérables.

Lecteur statique : pins externes avant parsing, inventaires réels exacts,
types JSON stricts, origine/commandes concordantes, oracle indépendant.
Recoupes par l'auditeur : lecteurs normal/−O, copie déplacée, replays
directs normal/−O reproduisant le stdout archivé ; faux pins code2.
Cinq corruptions RAM des comptes, cibles LCA, nombre de cas, types et
contrôles causaux sont refusées par la comparaison sémantique du lecteur.
Contre-lecture indépendante du même paquet : aucun écart.

Autorités externes :
source/SHA256SUMS
  0cf4a63865b5139db8fc82e973a4481612fcd7c454ff7c1a35308217f40c92a5
capture/SHA256SUMS
  5bfab7d05b8cd7bc1b482a318fc350af958a35e2a531bfec901bbee9bd505e46

Depuis la racine de ce paquet :
  python3 -B source/read.py --source-manifest-sha 0cf4a63865b5139db8fc82e973a4481612fcd7c454ff7c1a35308217f40c92a5 --capture CHEMIN_ABSOLU_CAPTURE --capture-manifest-sha 5bfab7d05b8cd7bc1b482a318fc350af958a35e2a531bfec901bbee9bd505e46
Ajouter−O avant source/read.py pour la seconde lecture. Le lecteur
n'exécute pas les sources archivées et n'exige pas les anciens chemins
/tmp ni le Python archivé. Les chemins de provenance ne sont pas
des dépendances LIVE de cette lecture statique.

source/README.txt décrit la préparation AVANT le record ; son état
historique n'a pas été réécrit. Le premier candidat OPEN est resté
inchangé et n'a jamais été enregistré comme une capture qualifiée.
Ne pas relancer record.py dans ces dossiers clos. Un futur essai
requiert un chemin neuf, sans remplacement des premières tentatives.

Limites de coût
---------------
LCA originale par remontées parentales ici : pas de préparation linéaire
ni de requêtes O(1) qualifiées. Les petits sets/Fenwick/arbres augmentés
sont des oracles indépendants, pas la représentation de production.
Le port conseillé reste déduplication/antichaîne, LCA et préfixes signés
sur le vrai FULL ; fabriquer les graines, les normaliser et payer T0,
tris et comparaisons exactes demeure obligatoire. Aucun gain natif
nouveau ni changement statistique ER/MMtA n'est acquis par ce paquet.
