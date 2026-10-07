# Contre-audit de JUG-EMST — 7 octobre 2026

Pin `1f7642e105aebd76632c58c63fdfd5b5c0824779`. Juge hors produit, CPU de référence,
profil quantifié u21 ; témoins numériques supplémentaires u32, `public_status=not_claimed`.
Aucun changement du juge ni du produit, aucune donnée réelle, aucun GPU/GCP, sanitizer,
million de points ou matrice complète. Une petite unité C++ Release compilée par rejeu.

## Conclusion

Le juge d'ordre un est effectivement livré et indépendant du catalogue et du moteur.
Sa preuve EMST et son traitement des plateaux sont recevables. Les **24 nuages synthétiques
de 1 à 32 sites** de cet audit donnent exactement l'EMST et l'arbre attendus par deux calculs
Python indépendants : Kruskal sur toutes les paires, et composantes du **graphe complet**
recalculées à chaque seuil exact. Les trois empreintes, les niveaux, les enfants et les
centres correspondent. Cela confirme la livraison qui manquait à `CST-0013`, sans
transformer ce reçu en qualification des neuf grands vidages historiques déclarés.

Une anomalie distincte demeure : **`CST-0232`, identifiants répétés admis avec `--ids`**.
L'arbre géométrique est correct dans le témoin ; le verdict d'identité accepte néanmoins
un vidage dont les `PointId` ne sont pas uniques.

## Preuve et numérique contre-lus

À l'ordre un, deux boules fermées de rayon √a se rencontrent exactement lorsque la distance
carrée de leurs centres est au plus 4a. Pour toute arête extérieure à un arbre couvrant
minimal, son chemin dans l'arbre a des poids au plus égaux au sien. Les composantes du
graphe complet et de l'EMST coïncident donc à **toutes** les coupes fermées, et aux coupes
ouvertes en remplaçant ≤ par <. Après contraction des composantes strictement antérieures,
les arêtes d'un plateau de l'EMST forment une forêt ; chaque composante non triviale donne
une multifusion unique. Le choix d'un EMST parmi les égalités ne modifie pas cet arbre.

Le départage lexicographique strict `(distance², a, b)` fixe en plus un EMST déterministe.
La propriété de coupe justifie les arêtes choisies par Borůvka. Le cache de voisin est sûr
tant que le voisin reste extérieur : les candidats extérieurs ne font que disparaître.
Une recherche bornée sans amélioration fournit un minorant réutilisable sous la même
monotonie. Le parcours conserve les boîtes à égalité de distance, nécessaire au départage.

Pour des coordonnées inférieures à 2³¹, trois carrés d'écarts ont une somme strictement
inférieure à 3·2⁶², donc à 2⁶⁴. Pour le domaine u32 entier, chaque carré tient en u64 et
leur somme en **66 bits**, couverte par u128 ; la conversion précède les additions. Le
maximum diagonal `3(2³²−1)²` est exercé. Les comparaisons de niveaux du FULL utilisent des
produits de naturels arbitraires, et non une conversion flottante. Des formes équivalentes
avec un facteur de **3 801 bits**, ainsi que des entiers de **64 mots** avec rembourrage,
sont réellement comparées et admises dans cet audit.

Les limites de capacité sont distinctes de ces preuves : au plus 2³¹−1 sites est annoncé,
mais aucun nuage approchant cette taille n'est alloué ici. Le coût asymptotique global et
les temps LiDAR ne se déduisent pas des petits essais.

## CST-0232 : témoin causal au vrai CLI

Entrée : sites distincts `(0,0,0)` et `(2,0,0)` ; deux naissances et une fusion binaire de
niveau 1. Le même vidage `MHGP11FUL1` de **586 octets**, `PointId=[17,17]`, produit :

| Appel | Code | Verdict |
| --- | ---: | --- |
| sans `--ids` | 3 | `PointId en double dans les sites du vidage` |
| avec un fichier `--ids` contenant `[17,17]` | **0** | **`identique`, zéro différence** |

L'empreinte du vidage est la même dans les deux appels :
`ee717fa160d209b0182226fc8b87ac701e4c35d217862c2a33695cccb19bbb60`.
Les contrôles positifs avec `[17, 0xffffffff]` passent ; changer seulement l'identifiant
de référence rend bien le code 1. Le maximum u32 est un `PointId` externe valide.

Cause : `jug_emst.cpp` vérifie seulement la longueur du fichier d'identifiants ; dans
`ComparaisonVidage::lire_sites`, le vecteur `identites` n'est rempli que **sans** `ids_`.
Avec `--ids`, égalité au fichier attendu remplace donc l'unicité. Or le domaine `Cloud`
refuse déjà `duplicate_point_id` (`src/cloud/cloud.hpp`) et le lecteur FULL refuse le
même objet sans l'option. Valider l'unicité des identifiants attendus et celle des
identifiants lus ; graver la paire d'appels. Ce constat porte sur la robustesse du juge,
pas sur l'exactitude des distances, de l'EMST ou de l'arbre du témoin.

## Capture exécutée

`check.py` ne charge aucun code Python du produit ni des tests officiels. Il compile
uniquement `juges/emst/src/jug_emst.cpp`, en C++20 `-O2 -DNDEBUG` et avertissements stricts.
Le fichier de dépendances compilées et leurs SHA-256 sont confrontés aux sources du pin.
Le binaire est temporaire : son empreinte est conservée, aucun ELF n'entre dans le reçu.

**64 vrais appels CLI par mode Python** : 24 nuages, chacun une fois en voie choisie et
une fois en u128 forcée ; puis 16 variantes de lecture FULL. Les coordonnées comprennent
des grilles avec égalités, des positions générales, des petites étendues au-dessus de
2³¹, les seuils u64/u128 et les coins u32. L'ordre d'entrée est renversé avec les
identifiants correspondants : la comparaison des sites en Morton est donc également
exercée. Voies choisies : 11 u64 et 13 u128. Le bilan des passages ordinaires comporte
48 tours de Borůvka, 976 requêtes, **12 voisins gardés**, **20 minorants réutilisés** et
14 023 distances : ces deux optimisations ne passent pas par vacuité.

Les variantes FULL détectent les modifications de niveau, parent, enfant, centre, site
et identité ; les dénominateurs nuls, zéros négatifs et troncatures sont refusés. Les
formes rationnelles équivalentes restent admises. Un préfixe ne contenant que l'ordre un
avec K annoncé égal à 2 est accepté : c'est la **limite explicitement documentée** du
juge, qui ne lit pas les ordres supérieurs ; aucun nouveau défaut n'en est déduit.
`JUG-EMST` ne remplace donc ni le lecteur FULL strict ni MES-M0, les verticales ou les
ordres k≥2. Ses SHA sont des empreintes d'ordre un, pas des empreintes FULL.

## Portée de CST-0013 et des validations historiques

La correction documentaire précédente de SUP-KRUSKAL reste recevable. La part « juge EMST
jamais implanté » de `CST-0013` est désormais satisfaite par le code, sa preuve et les
contrôles indépendants bornés ci-dessus ; conserver sa clôture dans cette portée, avec
le défaut distinct `CST-0232` suivi séparément.

Le README du développeur annonce 308 nuages d'oracle, neuf grands vidages v11 identiques,
des mutants et des campagnes sanitizer. Ces déclarations et leurs scripts ont été lus,
**pas rejoués** par cet audit. Aucun journal de qualification de cette nouvelle
implémentation EMST n'a été retrouvé dans les reçus versionnés ou les emplacements de
build locaux examinés. Les anciens journaux EMST v10 ne qualifient pas ce nouveau code.
Nous ne contre-certifions donc ni ces neuf grands vidages ni les campagnes annoncées,
et aucun coût G4 n'est acquis. Cette limite de preuve ne signifie pas que les essais
déclarés ont échoué ou n'ont pas été exécutés.

Rejeu depuis la racine :

```sh
python3 -B morsehgp3D_v12/receipts/audit_juges_emst_20261007/emst/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_juges_emst_20261007/emst/check.py
```

Les deux résultats sont identiques octet pour octet, codes 0 ; seule la capture
`normal.json` est conservée, les deux empreintes restent dans la vérification. Sources
comparées au pin puis rehachées à la fin ; script également contrôlé avant/après.
`verification.json` et `SHA256SUMS` ferment la capture ; aucun chemin privé, identité
de compte, binaire, recopie de source produit ou donnée sous licence n'y est conservé.
