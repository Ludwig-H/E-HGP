# Raccord q3/q4 par batch : trois portes avant une qualification S2

23 septembre 2026, 15 h 10 UTC. Contrelecture **WIP** du diff non commité
du développeur, `wspd_q34.cpp/.hpp` sur `5577f0f2a`, en lecture seule.
Le code peut encore changer ; ceci n'est ni un défaut d'un commit publié,
ni une mesure GPU. Le filtre CPU de référence utilise bien les mêmes
prédicats Affine sans cache que le chemin moteur, sans écart géométrique
concret trouvé dans la branche positive.

**Statut après `2059189d8`, `c265a5dae` et R12.** Le garde structurel
linéaire et son mutant causal ferment le contre-exemple local de doublon
ou de paire hors rectangle ; les cinq liens CMake et la porte G4 positive
sont aussi acquis. Les 14 cas R12 sont achevés, sept utilisent le GPU.
Les objections qui restent sont la confiance dans les **décisions de
masque** (non recertifiées par le garde structurel), l'absence de
comparaison des catalogues GPU et moteur clé par clé, la résidence
`Ω(R+P+S)` et le plafond `P≤2³¹−1`, les compteurs non comparables, et
l'absence de porte dédiée à `Kmax=2`. Les paragraphes ci-dessous
documentent l'historique WIP, pas le verdict actuel sur les points clos.

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
L'[audit A compilé](q34_batch_duplicate_gate_20260923/README.md) apporte
depuis un témoin plus fort que le tout-zéro : un doublon remplace une
paire survivante à cardinalité et ledger identiques, supprime une clé
q3 et passe `validate_completion`. Il n'impute pas cette corruption au
kernel CUDA ; il prouve que la frontière actuelle ne la détecte pas.

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
L'adaptateur CUDA mutable ajoute `pair_mask` (**P octets**), `flags`
et `positions` (**4P** chacun), puis jusqu'à **9S octets** de sorties
device, hors autres tableaux, avec refus explicite si `P>2³¹−1`.
L'hôte recrée `rank_points` (**12n octets**) et la garde brute rescane
les plages des nœuds. `filter_ms` les inclut, `device_ms` les exclut :
un prochain reçu ne doit pas annoncer un temps de chaîne à partir du
seul compteur device.

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
Le `job_ns` des workers ne chronomètre plus que la collecte du front,
alors que le chemin historique y inclut les arêtes :
`q34_occupancy.job_sum_s/max_job_ms` n'est pas comparable ON/OFF.
Conserver `front_ns`, `filter_ns`, `edges_ns` et leur somme séparés,
sans inférer un meilleur équilibrage des anciennes colonnes.

## 4. Lien CMake manquant sur les cibles qui recompilent la chaîne

Le diff de 15 h 18 fait appeler `gpu::run_filter_batch` depuis
`tower_chain.cpp` et lie `mhgp9_gpu` à la bibliothèque `mhgp9_chain`.
Cependant `mhgp9_chain_order_failure_priority_gate`, ses deux mutants
et les deux mutants Euler recompilent directement **ce même `.cpp`**
dans leurs exécutables et ne lient que `mhgp9_gen`. Ils garderont donc
une référence indéfinie à `gpu::run_filter_batch`, même lorsque CUDA est
désactivé et que l'implémentation attendue est le stub. C'est un blocage
de build statiquement visible ; ajouter le lien `mhgp9_gpu` à ces cinq
cibles, puis compiler l'ensemble des gates avec CUDA OFF et ON avant
de publier la tranche.
Au nouveau diff de 15 h 29, D a ajouté `mhgp9_gpu` aux cinq cibles.
Le défaut de lien est donc **corrigé en source WIP** ; le build complet
et son reçu restent à rejuger au commit. Les autres portes de cette
note ne sont pas fermées par ce lien.

## 5. La porte actuelle peut rester verte sans aucun GPU positif

Le nouveau `chain_batch_filter_gate.cpp` accepte, pour chacun de ses
18 cas, soit une chaîne GPU complète, soit
`chain_q34_gpu_unavailable`. Son plancher final n'impose que
`gpu_refusals+gpu_runs≥18` ; `gpu_runs=0` peut donc passer sur une VM
GPU où l'adaptateur CUDA régresse et refuse systématiquement. C'est
une bonne porte de refus explicite pour les builds sans appareil,
mais pas une qualification du raccord GPU. Prévoir un **gate G4
distinct** qui exige les 18 exécutions positives (ou un plan publié
plus étroit fixé d'avance), leurs identités et digests, et échoue si
`gpu_runs=0`. Ne pas faire passer le refus sans appareil pour un test
device exact.

La porte actuelle n'échantillonne que K3/K5/K10. Pourtant le front q3
est déjà actif à **K2** (`requested_lane_mask=6` restreint à la voie 2),
et la garde GPU mutable accepte désormais K1..10. Ajouter K2 q3-only
au différentiel CPU/GPU et K1 vide pour vérifier la base de la tour ;
sinon les deux premiers niveaux ne sont pas exercés par S2.

Ces portes concernent un **chantier mutable**. La prochaine
contrelecture doit repartir du commit publié, des tests causaux et du
reçu intégré ; ne pas extrapoler les 43–107 ms de S1 à la tour.
