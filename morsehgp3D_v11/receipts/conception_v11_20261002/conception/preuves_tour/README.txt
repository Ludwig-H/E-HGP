Preuves de la conception de la tour v11 (CONCEPTION_TOUR.md, annexe B), 2 octobre 2026.
Controles legers executes sur le codespace (8 coeurs, charge 19 a 25), un processus a la fois. Aucune commande GCP,
aucune commande git mutante, aucun octet de nuage KITTI (les vidages lus sous /tmp ne contiennent que des indices).

noyau_v11.cpp        noyau de foret SANS LOTS (union par taille, evenements, attaches, sommet par jonction),
                     materialisation par contraction des plateaux, historique d'attache, image des jonctions ;
                     compare a une copie du Kruskal par lots de la v10 sur les entrees reelles videes par l'audit L06
                     (/tmp/v11-audit/l06_code_tour/krdump/kruskal_k{1..5}.bin, krdump10/kruskal_k10.bin, trame 00).
                     g++ -std=c++20 -O2 noyau_v11.cpp -o noyau_v11 ; ./noyau_v11 kruskal_k5.bin 7  (code 0 = egal)
noyau_v11.log        sorties : six ordres, forets et numerotations identiques, 2 444 942 images concordantes,
                     rapport de temps 2,49 a 2,90 (machine chargee : seuls les rapports sont a lire).
foret_check.py       la meme chaine en Python sur des hypergraphes aleatoires a rangs repetes, contre le Kruskal par
                     lots et contre la remontee parent par parent.   python3 -B foret_check.py <graine> <cas>
quotient_check.py    quotient local d'une coquille etendue par ensembles separables maximaux, contre la definition
                     brute (t-parties separables, liaison par union separable), coquilles de 3 a 9 sites.
quotient_check2.py   le meme sur 10 a 13 sites ; cout du quotient seul sur 24 et 30 points cospheriques et sur des
                     cercles de 12, 16 et 24 points.
controles_python.log sorties des trois scripts Python.
cover_temoins_check.py  proposition T8 : l'entree cover (niveau et ensemble des noeuds) lue sur les seuls temoins
                     p + q_min <= k <= p + m, noeud de naissance pour un temoin regulier, contre l'etage de definition
                     de la reference v11 (morsehgp3D_v11/reference, lu seulement).
                     python3 -B cover_temoins_check.py <chemin de reference/> 3 60
