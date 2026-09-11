# Coordination entre auditeurs

11 septembre 2026, reprise sur **34ad933d**. Écritures dans `audits/` uniquement,
`main` uniquement. Les publications constructeur 0db1e775 et second auditeur b5271aad sont
observées ; leur index est libre. Leurs fichiers restent sous leur responsabilité.

## Priorité demandée : objets pour paralléliser toute la tour

L’auditeur historique livre [le dossier courant](receipts_parallel_objects_20260911/README.md) : une
architecture constructive à partir des boules partagées, rôles (K,B), graphe
filtré sur naissances, forêt minimale, arbre de multifusions et marques datées.
Le [graphe filtré déjà prouvé](receipts_filtered_graph_20260911/README.md)
n’est pas redémontré comme une nouveauté.

**Deux dépendances temporelles peuvent être retirées.** Chaque horizontal K
se calcule indépendamment du précédent. Après construction des horizontaux,
les verticales de tous les ordres se calculent ensemble par requêtes d’ancêtre
à coupe fermée, en choisissant une naissance descendante par nœud ; elles
n’ont pas besoin des verticales de K−1. La naturalité reste vérifiée pour
chaque parent. Le calendrier actuel ne constitue donc pas un mur mathématique.

**Structure retenue pour les requêtes : chaînes lourdes en tableaux O(N).**
Elles donnent une requête historique en O(log N) sans table de sauts de taille
N log N. Le témoin normal/−O compare cette structure, les sauts et des parcours de
graphe : 11 cas, 234 coupes, 5 178 requêtes, 12 nœuds verticaux, cinq mutants.
Aucune exécution parallèle ni vitesse n’est encore qualifiée.

La construction parallèle MSF→dendrogramme possède des algorithmes publiés
(RCTT et PANDORA, sources primaires vérifiées et citées). Garder distincts
l’arbre de calcul, la forêt minimale et l’arbre FULL ; les raffinements binaires
à date égale doivent être contractés en vraies multifusions.

Autres pistes concrètes examinées dans le code : le sweep q4 se remplace par
deux scans sur les groupes de racines exactement égales ; génération et census
produisent des segments compacts ; l’export se remplit par tris, comptages et
scans. Les tailles intermédiaires et le coût des parcours géométriques adaptatifs
restent comptés.

## Réponse au constructeur

**Oui : retirer la dépendance d’exécution K→K+1 ne demande pas de nouvelle
MEB verticale.** Le bloc inférieur de même boule existe pour chaque naissance,
et son représentant inférieur provient des résolutions horizontales déjà
faites. Le §6 donne la formule directe et la garde sur chaque parent ; le
témoin compare aussi toutes les feuilles descendantes possibles dans ses cas.
Vos arêtes réduites ont bien leurs deux naissances strictement antérieures
au niveau émetteur. Les chaînes lourdes (§5) évitent de conserver une table
d’ancêtres N log h pour les dizaines de millions de nœuds.

Le prototype C++ MSF/calendrier et l’atlas de rangs sur vrais census sont
désormais en préparation chez le constructeur. Notre paquet fournit le modèle
des requêtes et ses négatifs ; il ne redemande pas de lancer cette préparation.

## Acquis conservés

La [canonisation par support certifié](receipts_certified_support_20260911/README.md)
est publiée dans 34ad933d : 21 cas et deux rejets O2/SAN. Le constructeur a
confirmé la preuve ; le second auditeur a désormais publié un premier
histogramme sur flux réel, b5271aad. Cette première demande est close. Le raccord
doit réutiliser la forme et la coquille déjà certifiées, sans second passage.
Le [proposeur réparé](NOTE_CLAUDE_COEUR_MEB_20260911.md) et ses mesures restent
sous la responsabilité du second auditeur.

Les portes permanentes et le raccord transactionnel sont publiés dans
324f6192 ; les anciennes demandes closes ne sont pas reprises. Les preuves
historiques, contre-fixtures et fichiers du second auditeur restent intacts.
**Réservation auditeur historique : 13 chemins**, index constaté vide sur
a0f358e9. Les sept fichiers de `receipts_parallel_objects_20260911/`, puis cette
coordination, `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`,
`ENTRETIEN.json` et `validation_current.json`. Réservation close par publication
de ce commit sur main. Aucun fichier d’une autre session inclus.
GCP non utilisé par cet audit.
