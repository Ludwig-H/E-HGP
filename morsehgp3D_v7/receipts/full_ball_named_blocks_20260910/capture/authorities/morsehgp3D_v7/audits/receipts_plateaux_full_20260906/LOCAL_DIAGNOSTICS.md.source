# Cas réels 50k et certificat de couverture factorisé

6 septembre 2026, complément à 30d2a4dd. `public_status=not_claimed`. Les preuves [du quotient](README.md) et [des ancres](BALL_ANCHORS.md) sont désormais reprises dans le [contrat constructeur](../../docs/PLATEAUX_FULL_ET_ANCRES.md). Ce complément répond à sa question sur un journal de contributions et contrôle les quatre coquilles extraites ; aucune compilation C++, exécution du moteur ou session GCP par l’auditeur.

## 1. Un journal redondant suffit, sans couverture globale dans le producteur

**La proposition du constructeur est correcte.** Chaque bloc fermé B à l’ordre K peut apporter une référence à sa population complète S_B=I_B∪U_B, affectée à sa composante après le lot, au niveau exact de B. À la lecture, les contributions actives sont réunies par union ensembliste dans chaque composante, avec celles héritées des parents. Un même point peut rester dans plusieurs composantes distinctes ; aucun dédoublonnage entre racines n’est permis. Une contribution à une continuation ne crée pas de nœud topologique : le token persiste jusqu’à une vraie multifusion.

On peut réduire encore la charge. Soit Q_B l’union des couvertures des composantes **strictes locales**, avant B. Définir la contribution potentielle D_B=S_B∖Q_B. Si P est l’union des couvertures des parents globaux du groupe de blocs fermé, alors Q_B⊆P pour chaque bloc de ce groupe. Par conséquent :

$$P\cup\bigcup_{B}S_B=P\cup\bigcup_{B}D_B.$$

Chaque point de Q_B est porté par une facette stricte locale ; son représentant appartient au même parent global avant le lot. Le quotient préserve cette couverture. L’égalité donne l’induction de rejeu : les anciennes couvertures sont exactes, les nouvelles facettes sont couvertes par les blocs de leur MEB, et les blocs omis sous la fenêtre utile sont inertes. La topologie et ses bons parents restent des prérequis, avec le census complet et la preuve S1. Une contribution ne certifie aucun de ces prérequis à elle seule.

Si aucune classe stricte n’existe, D_B=S_B : c’est la couverture de naissance, éventuellement plus grande que K. S’il existe une classe et K>p, tous ses représentants contiennent I, donc I⊆Q_B et **D_B⊆U**. Pour K≤p, Q_B=S_B et D_B est vide. Hors naissance, un masque de coquille suffit ainsi à décrire la contribution ; la table locale est déjà disponible. Lorsque D_B est vide, aucun enregistrement de couverture n’est requis, même si le bloc doit encore fusionner des parents ou installer une ancre.

Cette D_B n’est **pas** le delta minimal des points nouvellement couverts globalement. La redondance est autorisée explicitement ; elle évite de comparer des ensembles globaux dans le producteur. Le lecteur doit effectuer une union, jamais une somme de cardinaux. Si une API exige le delta exact, le lecteur peut le calculer au rejeu en comparant les couvertures successives ; ce service et son coût restent distincts.

Le chemin régulier garde son économie : au rang K=|S|−1, les retraits des q≥2 essentiels couvrent ensemble S, donc D_B est vide. Au rang K=|S|, S est exactement le label de naissance déjà nécessaire. Aucun payload de croissance supplémentaire n’est requis pour les connexions régulières.

### Temps, identité et volume à déclarer

Un enregistrement porte l’ordre, le niveau exact, le token du segment de composante après lot et une référence de population immuable, éventuellement un masque de U. L’archive lie cette référence à son entrée et à son census ; un pointeur vers un buffer réutilisé ne suffit pas. Les singletons K1 à zéro sont des naissances explicites. K=n conserve sa naissance terminale ; aucun ordre supérieur absent n’est inventé.

Résoudre tous les parents dans l’état strict, assembler les blocs égaux, puis publier simultanément topologie, contributions et ancres. Une coupe ouverte à t retient seulement les contributions de niveau inférieur à t ; une coupe fermée inclut tout le lot égal. Le lecteur suit les successeurs actifs **à cette coupe**, pas la racine finale. Les contributions d’un segment terminé passent à la composante résultante de sa fusion ; elles ne sont pas réaffectées rétrospectivement aux feuilles. Sinon, une contribution future pourrait apparaître dans une coupe ancienne ou dans un autre parent.

Avec B₀ boules retenues, L naissances, I multifusions, une référence S_B partagée et au plus un enregistrement par bloc planifié donnent un volume O(B₀+L+I+M), où M est la somme des longueurs des intervalles d’ancres, et où les populations stockées coûtent en plus leur cardinal total. Les D_B vides réduisent le nombre d’enregistrements. Aucun bloc n’est créé pour K>|S|. Dans le domaine actuel p≤9,u≤12, une population a au plus 21 identifiants et M≤Kmax·B₀. Ce sont des bornes de représentation, pas une garantie linéaire en n, ni un contrat de latence. Le nombre de contributions ne se borne pas par les seules multifusions. Le catalogue et les références peuvent être partagés entre ordres, jamais les identités de composantes.

Ce format restitue la topologie H0 et les couvertures aux coupes. Il ne restitue pas toutes les facettes et leurs dates, les poids du manuscrit ou un certificat géométrique de complétude. Les formats réguliers et les reçus réduits F gardent leur sémantique ; une nouvelle version d’archive devra déclarer `contributions` plutôt que réinterpréter `delta`.

## 2. Pourquoi un manque local ne force pas une croissance globale

Le [modèle permanent](locals_gap_model.py) complète la fixture ABCZ du premier audit. A=(1,8), B=(5,10), C=(9,8), Z=(5,0), troisième coordonnée nulle. Leur cercle a centre (5,5), rayon carré 25, intérieur vide. À K3, la seule facette stricte locale est ABC ; D_B={Z}. Dans ce nuage, Z est effectivement ajouté au niveau 25.

Ajouter X=(10,6) et Y=(9,1), tous deux extérieurs au cercle : leurs distances carrées au centre sont 26 et 32. Les mêmes I/U et D_B persistent. Mais le chemin ABC→BCX→CXY→XYZ est désormais strict avant 25. Ses trois cofaces tiennent dans les boules diamétrales AX, BY et CZ, de rayons carrés respectifs 85/4, 97/4 et 20. Le parent global couvre déjà Z : la contribution est redondante, sans naissance, fusion ni croissance effective à 25.

Deux points extérieurs sont minimaux pour **cette base** : avec un seul, la première coface introduisant Z depuis une facette qui ne le contient pas inclurait Z et deux points parmi A,B,C. Chacun de ces triples a déjà rayon carré 25 ; cette coface ne peut donc être stricte. Les [résultats normaux](locals_gap_normal.json) et [optimisés](locals_gap_optimized.json) vérifient quatre coupes exactes et réfutent `local_gap_equals_forced_global_growth`.

## 3. Ce que les quatre coquilles réelles permettent de conclure

Le [paquet constructeur](../../receipts/full_extra_shell_50000_20260906/README.md), publié par 56ace8d8, conserve l’extraction locale 50k/s8/K10 sur le digest d’entrée `3f7c6dd47bcba4222e511c94f90aaeeeb80198b0d5ac8a6721e4ff55feedab3f`. Il s’agit des quatre clés détaillées de cette capture ; les anciens refus G4 n’exportaient pas leurs clés. Leur identité individuelle avec ces anciennes boules n’est donc pas affirmée.

Chaque coquille a u=3 et contient un unique diamètre positif : q_min=2, h=2. À K=p+1, le quotient strict est connexe et couvre S. À K=p+2, les deux paires non diamétrales sont les deux classes strictes et couvrent ensemble S ; **aucun gain de points n’est possible**, même si les deux parents globaux sont distincts. À K=p+3=|S|, aucune facette stricte n’existe : c’est une naissance de couverture S.

| Indice de capture | p | Verdict global au rang ambigu | Naissance prouvée par census complet | Contribution hors naissance |
| ---: | ---: | --- | --- | --- |
| 174406 | 3 | K5, un parent : bloc inerte | K6, six points | Vide |
| 254569 | 0 | K2, deux parents distincts : fusion certaine | K3, trois points | Vide |
| 996863 | 4 | K6, deux parents distincts : fusion certaine | K7, sept points | Vide |
| 1251653 | 9 | Aucune dans K1..10 | Hors fenêtre demandée | Vide ; ancre K10 conservée |

L’absence de classe stricte locale devient ici une naissance **globale géométrique**, puisque I/U sont complets. Toute connexion au même rayon depuis une facette nouvelle aurait une boule minimale contenant cette facette, de même rayon ; l’unicité de la MEB impose la même boule. Elle ne peut rejoindre des points extérieurs à S ni un parent strict inexistant dans S. Cela n’annonce pas un nœud déjà produit par le C++.

Les [certificats globaux ultérieurs](GLOBAL_PARENTS.md) décident ces trois cas par un pont extérieur et deux preuves d’isolement ; ils ne déduisent pas ces verdicts du seul quotient local et ne reconstruisent pas les lots égaux complets. La garde FULL actuelle ne peut pas être supprimée après le seul classement local. Ces cas réels n’exercent ni croissance ni naissance de couverture supérieure à K ; les fixtures synthétiques restent nécessaires pour qualifier le domaine non régulier complet.

## 4. Vérifications et reproduction

[shell_diagnostic.py](shell_diagnostic.py) lit strictement le format JSONL fourni : clés et entiers bornés, BallKey primitive, rayon rationnel sur trois limbs y compris non réduit, populations disjointes et pouvoirs exacts. Il recalcule les supports positifs par Gram rationnel, puis les composantes sur les masques de coquille. Il n’énumère aucun sous-ensemble des intérieurs. Sa sortie reste explicitement locale : elle ne certifie pas la complétude d’un census simplement reçu.

Le [juge synthétique](shell_diagnostic_test.py) confronte 12 boules et 64 ordres à un graphe strict calculé indépendamment sur toutes les petites facettes, avec bijection des représentants et comparaison des couvertures. Il exerce aussi p=9,u=12 sans construire les sous-ensembles des 21 points : 781 supports au plus et 4 096 masques. Les permutations, PointId maximal, troisième limb non nul, rayon non réduit, 22 rejets de format/géométrie et clés JSON dupliquées sont vérifiés. Un enregistrement cohérent mais incomplet reste non certifié ; un dernier enregistrement invalide ne laisse aucun préfixe publié. Les CLI donnent 0/2/2. Les [sorties normales](shell_diagnostic_normal.json) et [optimisées](shell_diagnostic_optimized.json) sont identiques.

```bash
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/shell_diagnostic_test.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/shell_diagnostic_test.py
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/locals_gap_model.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/locals_gap_model.py
```

Le [census indépendant](real_shell_census.py) régénère les 50 000 points : état scalaire canonique de MT19937, cœur CPython et extraction des 16 bits hauts. Il reproduit le digest complet, puis les rangs Morton par entrelacement d’octets et tri. Les quatre scans calculent les distances au centre en coordonnées entières multipliées par 2A, sans importer le générateur, l’index ou le juge du constructeur. **200 000 comparaisons exactes** confirment les quatre populations complètes et les 28 lignes I/U. Cinq mutants — intérieur omis, point de coquille omis, coordonnée, rang Morton, digest — sont rejetés. Les [résultats normaux](real_shell_census_normal.json) et [optimisés](real_shell_census_optimized.json) sont identiques. Le rapport local imbriqué conserve sa portée ; le contrôle du census complet est porté séparément par ce lecteur de campagne.

```bash
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_shell_census.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/real_shell_census.py
```

Le [modèle du journal](coverage_contribution_model.py) construit sa topologie sans couverture globale ni membres de facettes dans son état producteur. Après production, il sérialise les références S_B et D_B, puis consulte ces **journaux finaux** aux anciennes coupes : le lecteur doit réellement filtrer les contributions futures. Sur dix runs, **45 ordres, 718 coupes, 2 588 facettes et 779 couvertures de composantes** concordent avec Gamma. Les 300 blocs donnent 300 références complètes contre 163 contributions réduites. La topologie garde 160 naissances et 69 multifusions, aucun nœud artificiel de croissance. Ces comptes décrivent uniquement ce corpus borné.

Les mutants « supprimer une contribution de continuation » et « ignorer sa date d’activation » sont réfutés sur ABCZ. ABCZXY conserve la contribution {Z} sans inventer de croissance. Doubler les contributions est idempotent à chaque coupe, séparément dans chaque composante. Les [sorties normales](coverage_contribution_normal.json) et [optimisées](coverage_contribution_optimized.json) sont identiques, codes 0. Le désérialiseur est celui du modèle nominal, pas un validateur industriel de formats arbitraires ; ses références utilisent le census exact du modèle, sans export géométrique autonome.

```bash
taskset -c 1 python3 -B morsehgp3D_v7/audits/receipts_plateaux_full_20260906/coverage_contribution_model.py
taskset -c 1 python3 -B -O morsehgp3D_v7/audits/receipts_plateaux_full_20260906/coverage_contribution_model.py
```

Les [sources, commandes et portées](shell_diagnostic_review.json) sont épinglées. Aucun résultat C++ ou de performance n’est transféré à ces vérifications Python.
