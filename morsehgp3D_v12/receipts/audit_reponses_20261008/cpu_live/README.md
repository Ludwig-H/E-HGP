# Deux propositions CPU indépendantes pour les feuilles

Propositions non intégrées, non compilées. Aucun moteur, CUDA, GCP, nuage réel ni
nouveau chrono exécuté. Base `8da450ab7`, quatre sources égales aux pins main
avant/après de [capture.json](capture.json). Le [précédent audit](../../audit_performance_20261007/feuilles/README.md#reproduction-et-limites)
signalait déjà la symétrie ; ce reçu fournit deux patches séparés sans table
auxiliaire. Il ne revendique aucune accélération mesurée ni qualification native.

**1. [live symétrique](01_live_symetrique.patch).** `pair_rows` construit un graphe
`nbr` symétrique, sans diagonale, sur `0..m−1`. Pour chaque arête `{x,y}`,
`w=|dom[x]∪dom[y]|` est symétrique. `nbr[x]&above(x)` la visite exactement une
fois. Écrire les deux bits si `w≤K−1−t` donne donc exactement l'ancienne ligne
`live[t][x]`, pour chacun des trois t. Toutes les `3×N` lignes sont vidées AVANT
ces écritures, y compris les rangs morts ; un effacement par ligne entrelacé avec
les miroirs détruirait les bits déjà déposés. Les seuils restent signés, notamment
pour K=1,2. `m≤N` et les masques issus de `pair_rows` bornent chaque bit.

Le corps sériel utilise le `LeafShared` propre au worker (`leaves.cpp:139–147`) ;
aucune écriture entre workers n'est ajoutée. La synchronisation finale demeure.
`dom`, `nbr`, arithmétique géométrique et stockage de la feuille restent identiques.
Toutes les lectures aval, dont Q3/Q4, voient donc les mêmes masques.

**2. [G3 Q2 redondant sur l'hôte](02_q2_g3_hote.patch).** Dans `run_leaf`,
`next1[i]⊆live[0][i]∩above(i)`. `phase<2>→next_item<2>→queue→round<2>`
ne fait que compter, empaqueter et transmettre ces paires. Chaque item Q2 satisfait
donc déjà `|dom[i]∪dom[j]|≤K−1`. Le garde retiré ne modifie aucun compteur et ne
peut rejeter cet item ; `children` et `q2_in_box` restent exécutés à l'identique.
Cette preuve porte sur les appels du générateur, pas sur un appel artificiel à
`judge_item` avec un item arbitraire. La recherche dans `src/` et `tests/` ne trouve
que sa définition et l'appel de `round`. Les gardes Q3/Q4 sont conservés.

Les deux patches s'appliquent séparément ou dans les deux ordres. La projection
textuelle `MHGP12_SIMT_WARP=1` est égale avant/après : le warp CUDA produit garde
son corps. Le mode appareil d'essai `MHGP12_SIMT_SERIAL` choisit aussi la branche
sérielle et changerait comme l'hôte ; aucune qualification de ce mode n'est héritée.
Une projection textuelle ne remplace pas une compilation CUDA.

**Preuve bornée indépendante.** [check.py](check.py) construit un oracle d'ensembles
à partir des trois relations possibles par paire (voisins, dominance dans un sens
ou l'autre). Il énumère les `3^6=729` relations à m=4, K=1..12 et N=32/256 :
17 496 cas, plus 132 cas m=1,2,31,32,33,64,65,128,129,255,256. Cela comprend des
relations non réalisables géométriquement : la propriété combinatoire est plus
générale. Les lignes sont initialement empoisonnées ; les masques, les rangs élevés,
les seuils négatifs, l'ordre des items et leur codage 5/8 bits sont contrôlés.
La file est relue par tranches de 512. Dix mutations de MODÈLE sont refusées ;
`double_visit` viole uniquement le nombre de calculs, les neuf autres changent les
masques ou les items. Ce ne sont pas dix mutants C++ compilés. Les tests utilisent
des exceptions explicites et produisent le même JSON en normal et sous `-O`.

**Coût et mesure à prévoir.** Si E est le nombre d'arêtes voisines et Q le nombre
d'items Q2 jugés, le premier levier remplace 2E unions/popcounts par E dans
`prepare`, le second supprime Q unions/popcounts dans le garde Q2. Aucun autre
popcount n'est concerné. Les quinze compteurs logiques et émissions restent
identiques ; ces comptes physiques sont distincts. Pas de nouveau tableau ni
allocation ; nombre constant de masques temporaires de largeur N. L'ordre de
complexité est inchangé. Les écritures directes dans `live` et la vectorisation
peuvent modifier le bilan : ni moitié du temps CPU ni gain FULL n'est déduit.

Avant adoption : compiler et comparer la référence, chacun des deux leviers puis
leur composition sur feuilles N32/N256, politiques Narrow/Exact et stockage
réutilisé ; contrôler émissions, ordre, quinze compteurs et refus. Mesurer ensuite
le coût complet des feuilles/catalogue CPU, repli CPU du catalogue GPU compris,
puis FULL sur les mêmes scènes et cohortes. Déclarer chaque bras à l'avance,
séparer diagnostics physiques et temps non instrumentés. Les patches n'ajoutent
aucun compteur produit pour rendre artificiellement cette mesure favorable.

```sh
python morsehgp3D_v12/receipts/audit_reponses_20261008/cpu_live/check.py
python -O morsehgp3D_v12/receipts/audit_reponses_20261008/cpu_live/check.py
```

`--repo CHEMIN` vise un checkout portant les quatre sources épinglées. Le lecteur
vérifie leurs blobs Git et octets avant/après ; `git apply --check` et les applications
se font uniquement dans des copies temporaires supprimées ensuite. Aucun fichier
produit ni index Git n'est modifié. Contrelecture statique indépendante favorable
par l'auditeur E, sans rejeu natif de sa part.
