# Reçu — algèbre des coupes et des masses

26 septembre 2026. Base de travail b44a16e1e ; Python 3.12.1, hôte local.
Cadre : conception hors registre, référence Python, incidences synthétiques,
contrat des coupes, statut public non revendiqué. GCP non utilisé.

Le [script autonome](../../reference/verify_cut_algebra.py) vérifie des
identités et contre-exemples rationnels du
[contrat de conception](../../CONTRAT_COUPES_ET_MASSES_20260926.md).
Il ne lit ni le moteur ni des données LiDAR. Les matrices sont abstraites,
les poids rationnels sont fournis ; aucun calcul général exact des masses p3
ni réalisabilité géométrique n'est revendiqué.

## Exécution

Depuis la racine du dépôt, deux commandes ont terminé avec code 0 :

~~~bash
python3 Zoltan/FoundationModel/reference/verify_cut_algebra.py > Zoltan/FoundationModel/receipts/cut_algebra_20260926/normal.json
python3 -O Zoltan/FoundationModel/reference/verify_cut_algebra.py > Zoltan/FoundationModel/receipts/cut_algebra_20260926/optimized.json
~~~

[Sortie normale](normal.json) et [sortie optimisée](optimized.json) sont
identiques octet pour octet : **11 fixtures PASS, 19 variantes incorrectes
réfutées**. Ces variantes sont des opérateurs algébriques calculés dans les
fixtures ; elles ne sont pas des mutations compilées du moteur.

| artefact | SHA256 |
| --- | --- |
| script | 8ca674dc1e440085a76937acdebee97dfe6fcc6e8a8fb57781bbe168dd277ac8 |
| normal.json | 94770a1d9b57558eda78ad7fe964681af3b35a5a02f03f680cdd77c9be52573a |
| optimized.json | 94770a1d9b57558eda78ad7fe964681af3b35a5a02f03f680cdd77c9be52573a |

Le script conserve sa propre empreinte dans chaque sortie, emploie Fraction
et des contrôles explicites actifs sous −O, et termine avec code 1 si une
fixture échoue. Les sorties sont un enregistrement de ces sources, pas une
qualification automatiquement transférable à une version future.

## Ce que les fixtures établissent

| fixture | observation |
| --- | --- |
| univers et poids gelés | la composition commute ; changer les scores ou omettre un atome la rompt |
| réserve partielle | un survivant de masse 1/4 conserve une réserve de 3/4 |
| moyennes | transporter les masses donne 14/3 ; la moyenne uniforme des moyennes donne 13/2 |
| PUR et saut | constantes conservées, entrée (0,0,3) lissée en (0,1,2) ; aucune inversion générale |
| quotient | chaque fibre source doit avoir une unique image ; les fibres scindées sont rejetées |
| changement de K | une map dure ne détermine pas les poids normalisés de la branche cible |
| nouvelles naissances | une ligne résiduelle unitaire ne se scinde pas par une map dure |
| doublons | la même géométrie donne moyenne 5 par site, 5/2 par retour dans la fixture |
| routage anticipé | le choix descendant d'une branche diffère du vote sur les feuilles retenues |
| probabilités / logits | mixture (241/400,159/400), mais classe opposée avec les logits mélangés |
| multifusions | 0/1/plusieurs branches lourdes, seuil inclusif et effet erroné d'une binarisation |

Les fixtures de condensation n'ont pas de masse directement attachée à
l'événement ; ce cas reste une obligation du futur exporteur. Les fixtures
géométriques citées dans le contrat, dont growth_ABCZ, ne sont pas rejouées
par ce script. Aucun temps de production, coût d'export sur trame entière
ou résultat de réseau ne découle de ce reçu.

## Documentation

Le contrôle canonique a validé 780 fichiers Markdown ; son périmètre exclut
Zoltan. L'appel direct à son validateur sur le README de Zoltan et les
13 documents de FoundationModel passe également. Une extension aux 15
documents, archive de présentation comprise, signale seulement un espace
final préexistant à la ligne 3 du README de cette archive conservée telle
quelle. Le diff de cette tranche passe le contrôle des espaces Git.
