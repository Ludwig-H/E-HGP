# Faux positif du validateur sur un historique altéré

Lecture du prototype TMVR `repo5`, le 8 octobre 2026 ; code non livré à cette capture.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

`forest.hpp:184–188` et `CONTRAT_TOUR.md` §9.5 annoncent notamment
`événements = naissances − 1` et une CSR cohérente pour `validate_forests`.
Le corps épinglé ne lit pourtant aucun des champs `attach_rank`, `survivor_events`,
`event_rank`, `event_node`, `event_cell`. Il contrôle la forme de l'arbre, les
profondeurs d'attache, les branches et les verticales. Modifier uniquement les
champs omis conserve donc tous ses prédicats, ainsi que son admission mémoire.
Les SHA-256 avant/après sont dans `capture.json` ; le lecteur refuse leur dérive.

Le petit modèle emploie l'arbre K1 de deux sites `(0,0,0)`, `(2,0,0)` : naissances
0 et 1 de rang 0, fusion 2 de rang 1, parent `[2,2,aucun]`, attache de 1 vers 0.
La CSR des enfants est `off=[0,0,0,2], val=[0,1]`, celle des branches ouvertes
`off=[0,2], val=[0,1]`. Initialement l'unique événement pointe vers 2 ; la CSR de
survivant est `off=[0,1,1], val=[0]`.

Vider les trois tableaux d'événements et remplacer cette dernière CSR par
`off=[0,0,0], val=[]` donne une CSR bien formée mais zéro événement au lieu d'un.
Le validateur conserve son verdict de succès par non-lecture, avec un budget
suffisant. L'équation de `component_at(0,1)` rend alors 0, tandis que la coupe
fermée de l'arbre parent donne 2. Tous les indices consultés restent valides.
Une deuxième altération, `event_node[0]=0`, montre que des tailles correctes ne
suffisent pas. Le modèle exécute aussi le cas sain, qui rend bien 2.

La conclusion est un **faux positif de validation sur un objet volontairement
altéré**, pas une sortie FULL incorrecte démontrée. Aucun indice ne montre que
`build_forests` produit cet historique. Le domaine documenté de `component_at`
impose la naissance avant la requête ; cela n'autorise pas à lui passer n'importe
quel objet malformé. La faille concerne la portée annoncée du validateur public,
qui ne documente pas ici une précondition rendant superflus ces contrôles.

`gardes_minimales.patch` propose, après `check_shape`, les tailles de l'historique,
le nombre d'événements, la CSR des survivants et les domaines des références ;
un événement doit pointer vers une fusion du même rang et une cellule existante.
Le patch n'alloue rien, coûte O(naissances + événements) par ordre, hors chemin
chronométré, et conserve le refus `tower_invariant`. Son application textuelle
est vérifiée sur une extraction temporaire du seul fichier concerné.

Cette proposition tue les deux altérations du modèle mais **ne certifie pas toute
la sémantique de l'historique** : unicité des événements, appartenance au bon
survivant, monotonie et exactitude des rangs d'attache restent à vérifier. Le
contre-cas résiduel `attach_rank[1]=2` passe ces gardes, puis rend 1 au lieu de 2
à la requête `(1,1)`. Une porte native ciblée et son mutant sont à ajouter avant
de qualifier une correction complète ; le patch n'a été ni compilé ni exécuté.

Relecture autonome, depuis ce dossier :

```sh
python check.py --prototype DOSSIER_TMVR
python -O check.py --prototype DOSSIER_TMVR
```

Les deux sorties doivent être identiques au champ `result` de `capture.json`.
La preuve combine un contrôle statique de non-lecture et un modèle Python des
deux coupes ; elle n'exécute pas le validateur C++. Aucun moteur, GPU, GCP,
payload LiDAR ou nouveau chrono n'est utilisé.
