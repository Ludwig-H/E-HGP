# Compression exacte des seeds de couverture — prototype structurel

30 septembre 2026. Paquet autonome d'audit, créé uniquement sous `/tmp`.
Le prototype ne lance ni moteur, ni générateur, ni compilateur, ni GCP.
Il ne juge pas la géométrie native et ne qualifie aucun contrat FULL/G4.

## Objet

Chaque seed `(point, propriétaire, niveau exact)` représente une incidence
de couverture qui persiste en remontant les parents. La forêt est monotone,
les coupes sont fermées. Les plateaux parent/enfant égaux sont quotientés ;
une activation à la mort du nœud est normalisée au propriétaire après plateau.
Une activation antérieure à la naissance est refusée.

Le prototype conserve le niveau minimal de chaque `(point,node)`, trie les
nœuds d'un point par `tin`, puis supprime un nœud si le prochain sélectionné
est dans `[tin,tout)`. Les dates des seeds restantes ne sont jamais remplacées
par les dates de naissance. Les nœuds internes sont admis au même titre que
les feuilles. La réduction préserve des ensembles de composantes couvrantes,
pas des nombres de votes ou des poids de K-parties.

`prototype.py` confronte cette réduction à un oracle indépendant par lignées
de parents, sans Euler ni déduplication. Il vérifie chaque date critique et
un rationnel strictement intermédiaire entre deux dates, ainsi que l'avant
et l'après. Cas déterministes, 500 arbres aléatoires figés, permutations d'IDs,
plateaux, doublons, seeds internes tardives, activation=mort, niveaux distincts
coalescents en double, chaîne de 20 000 nœuds, forêt à deux racines. Une forêt
reste une forêt ; le profil à racine unique la refuse, sans racine inventée.

Six petits exports natifs EXISTANTS et clos sont copiés dans `fixtures/`.
Ils fournissent 88 incidences fortes du snapshot d'origine, dont les cas
internes K3 et K5. Aucun nouvel export natif n'est créé. Les seeds sont prises
sur toutes les incidences I/U de `population≥K` et `p+q_min≤K`, centre résolu
et vivant à son propre niveau. Les fusions FULL restent dans l'arbre.
`input_sources.json` et `upstream_SHA256SUMS` épinglent leur provenance.

Six mutations logiques causales sont rejugées : cinq changent effectivement
la relation de couverture ; le quotient par doubles détruit une précondition
de propriétaire vivant avant naissance et est compté séparément. Ce ne sont
pas des mutants compilés du moteur, ni des résultats de clustering/EOM.

## Lecture et preuve

`record.py` capture deux appels Python normal/−O avec UTC réelles, codes,
streams, hashes des sources/inputs avant et après, provenance figée.
Le paquet est ensuite fermé une seule fois par `SHA256SUMS`.

La lecture exige le SHA externe complet de ce manifeste :

```text
python3 -B verify.py SHA_MANIFEST_EXTERNE
python3 -B -O verify.py SHA_MANIFEST_EXTERNE
```

Le lecteur vérifie d'abord le SHA externe, tous les 17 fichiers attendus,
l'inventaire réel sans extra, la provenance, les commandes et leurs captures.
Puis il rejoue uniquement le prototype, compare les bytes capturés et revérifie
tous les fichiers. Il n'écrit rien dans l'archive. Ne pas relancer `record.py`
sur un paquet clos ; il refuse cet écrasement.

## Coût et limites

Pour H nœuds et D seeds fournies : quotient et Euler O(H), déduplication
O(D) attendue via dictionnaire Python, puis O(D log D) pour le tri comparatif,
balayage O(D), stockage O(H+D).
La normalisation via `Tree.owner` parcourt cependant les parents des seeds
stales et peut coûter O(DH) avant réduction ; ce prototype de vérification ne
revendique PAS une normalisation industrielle linéaire. Des owners déjà vivants
et une table de plateau préparée suppriment ce coût. Le tri radix à mots fixes
peut enlever le logarithme, mais n'est pas implémenté ou chronométré ici.
L'oracle cache ses lignées et utilise une recherche binaire seulement pour
éviter le rejeu quadratique de la chaîne ; son coût n'est pas celui du reducer.

Aucune borne globale de D, du catalogue, des coquilles étendues, des unités
aval ou de la génération FULL. Pas de mesure statistique, GPU ou G4.
