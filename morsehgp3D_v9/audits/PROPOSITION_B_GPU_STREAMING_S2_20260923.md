# Après S1 : filtrage GPU exact en flux borné, puis certificats avant expansion

23 septembre 2026. Proposition d'audit **non implémentée, non mesurée**.
S1 (`0d5ad2e89`) compare des masques GPU/CPU sur une population q3/q4
déjà développée. Il est utile comme porte de fidélité et de débit, mais
son tableau de masques est `O(P)` et son noyau recherche le rectangle
dans les offsets pour chaque paire (`O(P log R)`). Cette note distingue
deux jalons ; ni l'un ni l'autre ne qualifie la tour FULL.

## S2a : supprimer le surcoût de représentation, sans changer les rejets

Garder sur GPU les nœuds, les coordonnées par rang et, si l'aval l'exige,
la correspondance rang→ID du **même index certifié**. Consommer les
rectangles WSPD par lots, puis former des descripteurs de tuiles
`(ordinal_rectangle, début_A, début_B, longueur, masque)` de capacité
`B` paires. Chaque thread déduit directement ses rangs du descripteur ;
il n'effectue plus une recherche binaire dans les `R` offsets. Un
rectangle plus grand que `B` traverse autant de tuiles qu'il faut.
`B` est une fenêtre mémoire, **jamais un plafond de recherche**.

Le masque est produit/consommé sur la fenêtre courante, puis celle-ci
est vidée avant réemploi. Une compaction stable éventuelle émet les
survivants `(ID_a, ID_b, masque)` avec l'ordinal de présentation ;
l'aval peut aussi rester sur CPU au premier port. Ne jamais allouer
`pair_masks(P)` ni `cpu_pair(P)` dans la voie massive. La mémoire visée
est `O(N_index + R_lot + B + sorties_vivantes)` ; pour un arbre d'environ
`2n` nœuds, `FlatNode` prend `≈80n` octets, coordonnées ordonnées
`12n`, rang→ID `4n`, soit `≈96n` persistants (≈4,8 Go à 50 M sites),
**hors** index/WSPD CPU, files, descripteurs et catalogue. Choisir `B`
selon mémoire libre et marge mesurées, avec backpressure ; publier le
pic HBM et RSS. Ces chiffres sont un budget de représentation, pas
une promesse de résidence ou de temps G4.

La comparaison doit préserver les **présentations**, même si une paire
géométrique réapparaît : identité stable
`(ordinal_rectangle, rang_local_a, rang_local_b)`, pas seulement
`(ID_a,ID_b)`. Pour chaque voie q3 et q4, les bits ne peuvent que se
retirer ; une preuve q3 n'autorise pas à retirer q4. Les contacts et
cas incertains passent au filtre de paire exact. Un échec de ressource
ou une interruption marque la sortie **incomplète** ; la file pleine
ne supprime jamais des candidats.

## S2b : réduire le nombre de paires avant les tuiles

Sur le rectangle, garder le filtre universel strict. Pour ses voies
ouvertes, tester éventuellement une ligne `{a}×B_node`, ou un
sous-produit formé des **vrais enfants** de l'index. Une ligne dont la
voie est rejetée l'est pour chaque b de B : le certificat de témoins
compte des populations distinctes et strictement intérieures pour
**toutes** ces paires ; la [preuve et ses limites](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md)
sont déjà consignées. Si le test est indécis, revenir aux paires
exactes. Aucun ticket de parent n'est transmis sans antichaîne,
curseur et propriété de disjonction prouvés. Le DFS neuf par ligne a
déjà été défavorable en CPU local sur un cas ; son port GPU n'a donc
aucun gain présumé. Tester d'abord en observation et inclure coût du
certificat, paires, covers, formes, catalogue et FULL.

S2a enlève le surcoût `log R` et la mémoire `O(P)` sans enlever le
travail `O(P)`. Seul S2b ou un certificat plus fort peut faire baisser
structurellement la masse développée. Si `S` paires véritablement
ouvertes doivent être émises, leur coût de sortie reste `Ω(S)`.

## Porte causale et critère d'arrêt expérimental

Comparer sur **mêmes octets, même front et mêmes rectangles** :

1. S1 contre S2a, sans nouveau rejet, pour isoler tuilage, adressage,
   transferts et pic mémoire ; sur petits cas, comparer **tous** les
   masques, y compris zéro, et les ordinals au CPU indépendant.
2. S2a contre S2b, pour isoler la masse prouvée avant expansion et
   ses coûts ; comparer les survivants, puis les clés/catalogue/FULL
   canoniques une fois l'aval raccordé.

Fixtures obligatoires : `B=1` et `B=7`, tailles `B−1/B/B+1`, lot d'un/deux
rectangles, rectangle unique `>B`, ligne coupée entre tuiles, masques
q3/q4 séparés et conjoints, contacts stricts, inversion `a↔b`, produit
avec une voie seulement prouvée. Sur LiDAR, faire les coupes capteur
8k/16k/32k après masque sol figé, puis trames entières sans sol et
brutes, K5/K10, s8/10/12. Contrôler le flux sur CPU en mémoire `O(B)`
avec premier écart détaillé ; ne pas créer un autre tableau `O(P)`
dans le juge massif.

Le ledger doit vérifier, **par voie** et en classes exclusives :

`masse_front = rejet_rectangle + rejet_bloc + rejet_ligne + évalué_paire`,

`évalué_paire = rejet_paire + ouvert_paire`.

Vérifier aussi que les ordinals visités partitionnent exactement les
présentations résiduelles, sans trou ni doublon ; publier visites des
certificats, paires arrivant au cœur, covers, formes, sorties FULL,
octets H2D/D2H, queue maximale, RSS/HBM, CPU·s et mur bout-en-bout.
Le premier passage froid et les passages chauds sont distincts.

Pré-déclarer un **arrêt de la piste coûteuse**, jamais de l'algorithme :
si, sur plusieurs scènes d'intérêt, deux doublements 8k→16k→32k
donnent chacun un ratio proche de 4 (par exemple `≥3,8`) pour la masse
terminale **et** que visites/coût aval ne compensent pas, ne pas lancer
une campagne massive de cette variante ; chercher un certificat plus
fort. Un tel constat fini ne prouve pas une borne quadratique générale.
Inversement, des ratios inférieurs à 4 sur ces tailles ne prouvent pas
une borne sous-quadratique. Le vrai contrat demeure la tour entière
sur trames SemanticKITTI, d'abord 1 s, puis 100 ms sur G4.
