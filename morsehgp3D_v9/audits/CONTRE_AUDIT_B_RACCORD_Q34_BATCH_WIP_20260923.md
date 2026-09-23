# Raccord q3/q4 par batch : trois portes avant une qualification S2

23 septembre 2026, 15 h 10 UTC. Contrelecture **WIP** du diff non commité
du développeur, `wspd_q34.cpp/.hpp` sur `5577f0f2a`, en lecture seule.
Le code peut encore changer ; ceci n'est ni un défaut d'un commit publié,
ni une mesure GPU. Le filtre CPU de référence utilise bien les mêmes
prédicats Affine sans cache que le chemin moteur, sans écart géométrique
concret trouvé dans la branche positive.

## 1. Une identité comptable n'est pas un certificat des paires

`run_wspd_q34_batched` accepte un `Q34BatchFilter` fourni par l'appelant.
Après l'appel, il contrôle le nombre de masques rectangles, les rangs
globaux, le masque ⊆6, `expanded_pairs` et quelques sommes de voies.
Il ne contrôle ni l'appartenance de chaque arête survivante à son
rectangle, ni l'unicité/l'ordre, ni la décision géométrique de chaque
masque. Une fonction qui rend tous les masques rectangles à zéro, zéro
paire et zéro visite satisfait ces gardes et les identités de
`validate_completion`, alors qu'elle efface les candidats. C'est un
**contre-exemple de portée du validateur**, pas une corruption observée
du filtre CUDA S1, dont six cas ont été comparés masque par masque.

Ne pas ajouter une réévaluation CPU de toutes les paires au chemin
chronométré : cela annulerait le gain. Isoler plutôt l'adaptateur GPU
concret derrière une frontière de confiance explicite et exiger une
porte différentielle hors chrono, par mêmes octets/options, comparant
chaque masque, ordre/rang et survivant puis catalogue et digest FULL.
Un mutant `all_rect_masks_zero` doit être tué par la porte, et un
mutant qui remplace une arête par une autre arête valide mais étrangère
à son rectangle doit l'être aussi. Le `Q34BatchFilter` arbitraire ne
peut être réputé exact grâce aux seules gardes de forme actuelles.

## 2. Le batch n'est pas encore le tuilage borné S2a

Le front stocke tous ses rectangles dans `by_job`, les recopie dans
`rectangles`, puis le filtre matérialise tous les survivants avant de
lancer le cœur CPU. Ce parcours a une mémoire au moins `Ω(R+S)`
(`R` rectangles, `S` paires survivantes), plus les éventuels masques
`O(P)` de l'adaptateur GPU S1 (`P` paires développées). Avec
`sizeof(WspdRectangle)=24` et `sizeof(Q34SurvivingEdge)=12` attendus
sur le build courant, les **3,13 M rectangles** de 08/000000/K5
représentent environ **150 Mo pour les deux copies simultanées** ;
les **2,04 M survivants** réels ajoutent environ **25 Mo**. Ces seuls
tableaux ne sont pas encore une panne à cette taille, mais `S` et `P`
peuvent croître beaucoup plus vite sur des scènes denses : aucun plafond
de mémoire par tuile n'est établi pour des dizaines de millions de sites.
Le `Q34FilterBatch` CPU garde en outre les vecteurs par bloc pendant
qu'il recopie les survivants dans le vecteur final.

Un raccord batch complet est utile comme **étape fonctionnelle** ; il
ne ferme pas S2a. Exiger ensuite des tuiles possédées de taille bornée,
production→filtre→consommation sans résidence de tous les rectangles
et survivants, et des offsets/tailles 64 bits vérifiés. Publier HBM,
RSS et borne de files, y compris les scènes où `S` est grand.

## 3. Le ledger masque précisément le travail qui change

Le batch ne retourne que `rectangle_visits`, `pair_visits` et des
masses. Le raccord réécrit `witness.rectangles.queries/node_visits` et
`witness.pairs.queries/node_visits`, mais les autres compteurs du
filtre (tests de bornes et de points, préparations, crédits, nœuds
admis/rejetés, etc.) restent à zéro. Les `result.workers[*]` gardent
également `input_rectangles=expanded_pairs=0` malgré le travail réel
dans le batch. Une égalité des sorties ne suffira donc pas à conclure
sur le coût ou la sous-quadraticité si ces compteurs sont comparés au
chemin historique. Renvoyer un bilan complet équivalent, ou marquer
explicitement les champs indisponibles et mesurer séparément
`R`, `P`, `S`, visites, coût de préparation/filtre/cœur et octets.

Ces trois portes concernent un **chantier mutable**. La prochaine
contrelecture doit repartir du commit publié, des tests causaux et du
reçu intégré ; ne pas extrapoler les 43–107 ms de S1 à la tour.
