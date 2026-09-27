# Constructeur événementiel A — prototype C++ sériel qualifié

27 septembre 2026. `exploration_v9_hors_registre`, `cpu_reference`,
grille 1 mm/u18 pour les catalogues géométriques, `not_claimed`.
La qualification locale **R1 est close** : 17 commandes Release et Clang
ASan/UBSan/LSan, sources et 1 141 dépendances compilées fermées.
Le [préflight](PREFLIGHT.md) conserve l'échec initial et les corrections ;
le premier essai de qualification fraîche, lui, a réussi sans échec.
Pas de moteur modifié, de GCP ni de nouveau temps FULL.

## Ce que ce code change

Le constructeur natif parcourt les contacts successifs dans chaque K.
Ce prototype en remplace la partie A par un graphe immuable, puis des
requêtes d'historique et des écritures de tableaux. Il garde les mêmes
cibles géométriques natives ; il ne réimplémente pas leur résolution.

L'entrée est le [manifeste natif](../b_full_a_manifest_20260927/README.md),
avec tous les blocs actifs, y compris ceux qui ne produisent aucune ligne
visible. La sortie est le draft avant remappage des populations, toutes
les ancres, les successeurs, les naissances, les rangs et les racines
brutes de **chaque occurrence** de représentant. Une égalité de partition
finale ne suffit donc pas à faire passer le test.

Port explicite du modèle
[`event_model.py`](../b_full_phase_a_work_20260926/event_model.py),
SHA-256 `b59d908368c92a4e0305aa0ea55936014ea0aade2e2273683135c272bdf47085`.
Le code C++ ajoute les domaines K1, BallId et rangs globaux natifs,
les masques/intérieurs des contributions, les niveaux rationnels représentés,
les tableaux de sortie et les comptes de travail. La qualification du
modèle Python ne se transfère pas automatiquement à ce port.

## Les étapes et leur raison

1. Un sommet par bloc actif, plus les points initiaux de K1. Une arête
   par représentant rejoint son bloc cible, avec comme poids le rang du
   bloc source. Les cibles sont strictement antérieures.
2. Une forêt couvrante minimale conserve les composantes à chaque seuil.
   Ici Kruskal est **séquentiel**. Les arêtes arrivent déjà en poids
   croissants : aucun tri ni tableau supplémentaire de taille E n'est
   nécessaire pour cette version témoin. Les égalités sont départagées
   par leur ordinal, sans créer de nouveaux niveaux géométriques.
3. La forêt est stockée en CSR et enracinée. Les tables ancêtre/maximum
   du chemin répondent aux deux questions distinctes : composante après
   le contact fermé, et parent juste avant le contact ouvert. Une arête
   de faible poids après une arête de poids fort ne peut pas être franchie
   en ignorant le maximum du chemin.
4. Les blocs de même rang et même composante fermée forment un événement.
   Le dernier événement d'un parent avant ce rang fournit sa vraie
   identité historique. Les événements silencieux sont conservés.
5. Un événement avec zéro ou plusieurs parents crée un nœud. Avec un
   parent, il renvoie à une identité antérieure ; des doublements
   résolvent ces renvois. Ses contributions tardives restent datées.
6. Des préfixes et écritures de tableaux rendent les IDs dans l'ordre
   natif, les parents triés, puis les contributions dans l'ordre des
   blocs. Le niveau représenté vient du premier bloc du plateau entier,
   même si ce premier bloc est silencieux.

Pour K1, les sites initiaux sont des sommets de rang zéro, ordonnés selon
le domaine original. Leur référence de population n'est pas ball-tagged.
Les rangs globaux peuvent avoir des trous. Deux enracinements opposés
doivent rendre exactement les mêmes objets.

## Coût et limites

Notons V les sommets du graphe, E les occurrences de représentants,
B la taille du catalogue et N la taille du domaine de points. Le code
paie une validation en N, un tableau de B ancres comme le natif, des tables
ancêtre/maximum de taille V·ceil(log2(V+1)), des occurrences et parents
en O(E), et les tris des sommets/groupes/parents locaux.
Sa borne de travail est O(N+B+(V+E)·log(V+E+1)), mémoire O(B+E+V·log(V+1)),
hors entrée possédée par l'appelant et avec K fixé. Cette borne **dans la
taille du manifeste** ne prouve rien de sous-quadratique en nombre de
points LiDAR tant que la croissance du générateur et du manifeste n'est
pas mesurée. Le résidu q3/q4 ne change pas.

Les incidences de parents sont publiées dans trois domaines distincts :
parents des événements (y compris un parent pour un événement muet),
occurrences de parents dans le draft (continuations comprises), et liens
de la forêt finale de nœuds (sans les continuations). Ce ne sont pas trois
noms pour le même P.

Kruskal, l'enracinement et plusieurs scans sont encore séquentiels.
Ce premier port rend l'objet testable avant leur remplacement parallèle ;
il ne constitue pas une accélération CPU/GPU. Une version massive devra
aussi payer la forêt parallèle, son enracinement et la mémoire des tables,
pas seulement le tri final des événements.

Les champs mémoire donnent les maxima **observés de capacités simultanées**
des vecteurs temporaires et temporaires+sorties, puis la capacité finale
de sortie. Entrée, piles de tri, surcharge de l'allocateur et coexistence
interne à une réallocation sont exclus. Ce ne sont pas des pics RSS.
Le temps interne inclut les libérations explicites des temporaires ;
une mesure de performance devra aussi envelopper réellement l'appel,
la préparation et la destruction des résultats.

## Porte qualifiée et reproduction

Les tableaux sont comparés au constructeur natif instrumenté **et** au
juge chronologique indépendant. Petits catalogues réguliers et non réguliers,
permutations et IDs non identitaires, contributions tardives, événements
silencieux, niveaux rationnels représentés différemment, K1 et multifusion
32 parents. La [capture fraîche R1](receipts/r1/summary.json) ferme
376 comparaisons issues de 44 captures, avec les deux enracinements,
Release/sanitizers identiques. Lecteurs LIVE normal/−O et 25 corruptions
ciblées du lecteur passent dans les deux modes. Pas de TSan ni de GPU.

Le catalogue et les cibles restent ceux du producteur natif ; leur
exactitude géométrique n'est pas indépendamment reprouvée par ce test.
B, C, banque, encodage FULL et GPU ne sont pas remplacés dans ce lot.

Le gate ajoute séparément K1 avec un seul site non identitaire et zéro
boule, puis 32 programmes combinatoires de 8 à 257 blocs, rejugés dans
les deux enracinements. Ces programmes ne sont pas des catalogues LiDAR.
Leur fabrique bornée peut balayer les préfixes ; elle ne fait pas partie
du candidat ni d'une mesure de croissance.

Trois branches mutantes du même binaire sont exercées : coupe parent
fermée au lieu d'ouverte, omission des historiques de groupes sans
contribution (fusions comprises), inversion des groupes d'un même plateau.
Le test exige une divergence effective et un motif connu. Ce ne sont
pas trois builds mutants compilés indépendamment.

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_events_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
python3 -B -O morsehgp3D_v9/audits/b_full_a_events_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
python3 -B morsehgp3D_v9/audits/b_full_a_events_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_events_20260927/receipts/r1
```

Builds désormais figés :
`/workspaces/E-HGP/build/v9-a-events-20260927-r1_release` et
`/workspaces/E-HGP/build/v9-a-events-20260927-r1_sanitize`.
Le lecteur dépend de ces builds et de leurs sources locales ; il ne
recompile pas et ne rejoue pas les expériences. La capacité combinée
maximale observée de 13 004 octets est propre à ce petit corpus et à son
ABI/libstdc++, pas une borne universelle ni un pic RSS. Aucun chrono du
gate n'est un benchmark de l'algorithme sur LiDAR.
