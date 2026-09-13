# Payer le census après le filtre axial

13 septembre 2026. Snapshot produit `8e406f9b`, distinct de l'addition
et de l'intersection actuellement en construction. Cadre :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`.

## Expérience et résultat utile

Le [prototype C++ d'audit](q2_indexed_consumer_probe.cpp) consomme
physiquement toutes les candidates du véritable AxisQ2Plan publié.
Un index indépendant partitionne **tous les sites** du propriétaire,
y compris ceux hors des deux facteurs. Chaque paire interroge cet index
depuis zéro et s'arrête à Kmax intérieurs stricts. Aucun crédit du
préfiltre n'est réintroduit. La sortie compte des paires, sans collecter
les intérieurs, les coquilles ou des boules canoniques.

Sur les nappes complètes 50×80, 100×80 et 125×128, s12, voici les travaux
effectivement exécutés. Kmax est le seuil du census, sans cœur sur ces
six entrées. La colonne « sous le seuil » contient les paires dont le
compte exact d'intérieurs est inférieur à Kmax.

| n | Kmax | Candidates axiales consommées | Paires sous le seuil | Visites de nœuds du census |
| --- | --- | --- | --- | --- |
| 8 000 | 5 | 442 000 | 81 160 | 16 932 198 |
| 8 000 | 10 | 1 475 800 | 111 600 | 61 781 194 |
| 16 000 | 5 | 909 500 | 164 060 | 36 842 376 |
| 16 000 | 10 | 3 124 300 | 225 900 | 137 010 276 |
| 32 000 | 5 | 1 853 410 | 330 454 | 78 739 804 |
| 32 000 | 10 | 6 483 670 | 455 418 | 295 540 004 |

À n32k, construire l'index coûte 990 464 visites de points, 63 999 nœuds
et une profondeur maximale de 15. Les 295,5 millions de visites aval à
Kmax10 sont donc bien payées ; elles ne sont pas un coût de construction
reporté sous un autre nom. Le census élimine encore 6 028 252 candidates,
soit environ 93 % de ce résidu axial. Les compteurs précis dépendent de
ce découpage et de l'ordre du parcours ; ils ne sont pas une borne
inférieure pour tout index.

Le relevé unique GCC13/O2 donne à n32k/Kmax10 environ 57,8 ms de filtre
axial, 2,8 s de consommation et 2,87 s depuis la préparation. Ces durées
brutes sont conservées dans le [reçu](Q2_INDEXED_CONSUMER_CHECKS.json),
mais **ne sont pas une campagne de performance qualifiée** : hôte partagé,
pas d'appariement ni répétitions, compteurs actifs. Elles ne comparent
pas le code additif en cours et ne mesurent ni WSPD ni tour FULL. Aux
petites tailles, la durée de consommation inclut même le juge ponctuel.
La génération des coordonnées et la destruction des objets sont hors
chronomètre ; `total_ms` ne signifie donc pas bout en bout produit.

Cette expérience fournit une référence concrète pour le prochain
consommateur : indexer tous les sites évite un balayage de n points par
paire, mais le nombre de requêtes reste coûteux. Comparer les filtres
sur **construction + candidates + visites de census**, puis comparer
ce parcours indépendant au partage des requêtes par blocs proposé par
l'[autre auditeur](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches).
L'[intersection compacte](P0_INTERSECTION_RESIDUS.md) est une autre façon
de réduire les requêtes en amont. Aucune de ces pistes ne gagne par
principe ; les états, copies et reprises d'un parcours partagé sont aussi
à compter.

## Exactitude et coût de l'index proposé

Pour une paire a,b, stocker c₂=a+b et r₄=|a−b|². Pour une boîte Z,
calculer en i64 les distances carrées minimale et maximale à c₂ depuis
la boîte doublée 2Z. Les deux décisions sont :

- distance minimale ≥r₄ : aucun intérieur strict, terminer le nœud ;
- distance maximale <r₄ : tous les IDs sont intérieurs, ajouter leur
  population saturée au seuil ; sinon descendre vers les enfants.

La [fixture de centre demi-entier et de coquille](P0_CENSUS_Q2_ET_COQUILLE.md)
explique les inégalités et la portée de cette sortie. Le calcul utilise
des coordonnées promues avant les produits : chaque somme de distances
est au plus 12·65535², donc tient en i64. Les extrémités ne peuvent être
créditées puisque leur puissance vaut zéro. Une partition d'IDs disjoints
et le remplacement d'un nœud par ses deux enfants empêchent les doublons.

L'arbre coupe la plus grande étendue à son milieu entier. Sur des sites
u16 distincts, chaque coupe réduit au moins de moitié le nombre de
valeurs possibles d'une coordonnée dans le chemin : profondeur ≤48,
2n−1 nœuds, construction O(48n). Cette borne repose sur le profil u16
et le rejet des coordonnées dupliquées par le propriétaire. Chaque
requête peut encore visiter O(n) nœuds ; un arrêt au seuil ne promet pas
O(log n). Aucun tableau A×B ni mosaïque de Delaunay n'est construit.

## Portes et limites reproductibles

Le [runner](q2_indexed_consumer_checks.py) exporte sept sources exactes de
`8e406f9b` dans un répertoire neuf et compile le juge en C++20 strict,
GCC/O2 puis UBSan. Les deux exécutions concordent sur 16 cas : frontière,
site extérieur aux facteurs, cœur à ne pas précharger, coins de boîte
tous extérieurs, deux diamètres d'une même boule demi-entière, extrêmes
u16, neuf petites nappes. Le census indépendant évalue directement H,
sans réutiliser les distances aux boîtes : 3 076 448 tests ponctuels par
exécution. Il vérifie aussi toutes les paires des petits produits,
y compris celles rejetées en amont. Trois vraies mutations C++ sont
réfutées avec code 1 : boule fermée, cœur préchargé, sites hors A∪B oubliés.

Les six grandes expériences développent et consomment leurs candidates,
mais ne lancent pas de census exhaustif indépendant sur A×B. Elles ne
constituent donc pas une nouvelle qualification géométrique exhaustive
à n32k. Le reçu sépare ces relevés des portes bornées et épingle les
hashes des sources, du runner et du binaire chronométré.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/q2_indexed_consumer_checks.py --selftest --large
python3 -B -O audits/morsehgp3D_v8_complementaire/q2_indexed_consumer_checks.py --selftest
```

L'arrêt de chaque commande est borné ; une interruption des grandes
expériences conserve leurs sorties partielles. Les six lignes publiées
ici ont toutes terminé. Aucun fichier moteur modifié ; P0 et FULL restent
ouverts. GCP non utilisé.
