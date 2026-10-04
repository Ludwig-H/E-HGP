# Feuille appareil — contrelecture géométrique du WIP figé

Snapshot du 4 octobre 2026, 14:50:18 UTC, base publiée `66372e621dcee58daaa7d7309875ab157894acf4`, acteur `81e32cd7019f666aa5bf1f768ef29d6ee6fbecb1`. Les deux nouveaux headers sont **WIP**, pas des blobs GPU publiés à ce pin. Copies minimales dans `source/`, liées à `SNAPSHOT.json` du coordinateur par `BEFORE.json` ; `AFTER.json` vérifie que ces copies et le snapshot lu n'ont pas changé. Aucun code auteur ou note d'audit modifié. Aucune compilation C++/nvcc, exécution native, GPU ou G4.

**Conclusion : aucun rejet géométrique erroné ni acceptation incorrecte n'a été établi dans le domaine certifié.** Les décisions relues correspondent à la voie CPU graphe. Cette lecture et les gardes Fraction/Gram ne qualifient ni nvcc, ni la concurrence CUDA, ni la performance, ni les sorties canoniques d'une campagne réelle.

## Domaine impératif de l'entrée interne

`leaf_device_predicates.hpp:43–52` reçoit un nuage déjà propriétaire/certifié : mêmes sites unitaires que CPU, XYZ dans `[0,2^B−1]`, SiteIdx globaux valides/uniques/croissants, liste de feuille K-certifiée (G2), `1<=m<=32`, boîte T0 `0<=lo<hi<=2^B`, `1<=kmax<=12`. `run_leaf:319` ne revalide pas tout ce contrat. Le raccord valide est contrôlé par l'auditeur transport : `check_catalogue_params`, refus poids!=1 en amont (`parallel.cpp:196–199`, `catalogue.cpp:20–24`), listes issues de la racine puis filtres conservant leur ordre, queue seulement sur pair_graph et m<=32. Les vues internes forgées n'ont pas cette garantie et ne deviennent pas une API publique certifiée grâce à cette capsule.

## Bornes et formules

Les différences sont signées avant soustraction. `dot`/`cross` restent i64 ; centre, signe et SAT sont i128. Les bornes suivantes portent sur produits **et sommes partielles**, avant annulation, avec M=2^B :

| Expression | Majorant de magnitude | u18/u21/u24 : exposant strict suffisant |
| --- | --- | --- |
| Norme/dot | 3 M² | 38 / 44 / 50 |
| Dominance sur fermeture | 12 M² | 40 / 46 / 52 |
| SAT J2 | 18 M³ | 59 / 68 / 77 |
| N q3 | 24 M⁵ | 95 / 110 / 125 |
| D q3 | 24 M⁴ | 77 / 89 / 101 |
| Coordonnée globale / owner | 48 M⁵ | 96 / 111 / 126 |
| Milieu et puissance q4 | respectivement 96 M⁵ / 72 M⁵ | 97 / 112 / 127 |
| Puissance q3 sans certificat | 216 M⁶ | 116 / **134** / **152** |

Références : `source/src/num/budgets.hpp`, `num/predicates.cpp`, `num/center_region.cpp`, copies de la voie CPU. Les bornes q4 utilisent le cross de trois points du **même carré**, pas de deux vecteurs arbitraires. Au palier u24, un majorant strict `<2^127` tient bien dans i128 signé.

La feuille reprend exactement les certificats globaux q3 : D<2^(123−2B), |N_j|<2^(124−B) pour la puissance (`leaf_device_predicates.hpp:103–123`) ; D<2^(124−3B), |N_j|<2^(124−2B) pour l'orientation (`:96–102,133–142`). Le tag reste l'**arité de présentation**, jamais qmin. Si le certificat requis manque, le prédicat ne multiplie pas ; la feuille devient entièrement unresolved. Les poids q4 portent sur N BRUT avant normalisation de det et ne sont évalués qu'après la garde cube de côté L<=2^20 (`leaf_device.hpp:154–174`). Le majorant de tous leurs produits/sommes est 117L⁶<2^127, égalité du seuil L permise ; au-delà, repli de la feuille, pas rejet de la boule.

Témoin causal : A=(0,0,0), B=(L,L,0), C=(L,0,L), z=(L,L,L), L=2^B−1. ABC est strictement aigu, son centre est dans la racine, J2 passe, z est extérieur et n'est pas un générateur. Les quatre sites appartiennent à la boîte racine ; la dominance stricte sur toute sa fermeture ne peut donc supprimer z. Au K2, son appel side est atteint. Le premier produit D|z−A|² ferait 131 bits en u21 et 149 en u24. La garde actuelle refuse avant multiplication et le flux `side faux → unresolved → arrêt récursif → kUnresolved` exige le rejeu **entier** de la feuille. Le témoin ne révèle donc pas un débordement actuel : il verrouille précisément le repli nécessaire. En u18, la voie native sûre rend extérieur.

## Complétude, contacts, canonique et ledger

- J2 conserve les contacts avec la fermeture (`SAT left>right` seulement). Le propriétaire utilise lo inclus et hi exclu (`center_in_box:210–212`). M3/E4 emploient une enveloppe fermée contre la boîte demi-ouverte comme CPU. Les bornes de région peuvent atteindre 2^B ; elles ne sont pas des points d'entrée.
- Les q3 non aigus ne matérialisent aucune boule mais restent des préfixes q4. Fixture admissible, dans l'ordre de Morton : `(1,2,4),(2,1,7),(8,4,2),(7,8,5)`. Le premier triangle a produits aux sommets −1,12,58 ; le tétraèdre a tous ses poids Gram strictement positifs. Le code recurse malgré le retour local de q3 (`leaf_device.hpp:294–309`), comme CPU.
- Le census conserve son ordre et son arrêt au **prochain** intérieur qui dépasse theta_q ; toute coquille acceptée est complète dans la feuille G2. Les générateurs sont contacts, pas exclus de U. Les masques inside/outside contradictoires font unresolved (`:229–268`). Les intérieurs peuvent atteindre au plus le seuil et U au plus32, donc les tableaux locaux restent dans leurs bornes.
- Canonique = cardinal minimal puis tuple lexicographique des SiteIdx. Les pairs milieu, triangles stricts coplanaires au centre et tétraèdres contenant strictement le centre suivent `support.cpp`. Une coquille cube de huit sites a qmin2 même pour une présentation q4 stricte ; cette présentation est rejetée par S* et ses coefficients ne sont pas retagués. Le raccourci `count==q` est valide parce que la présentation visitée est affine indépendante et strictement positive. En cas d'orientation indécidable pendant S*, la feuille est rejouée entièrement.
- G3, lignes, angles, owner, census, S*, admission ont le même ordre de sortie anticipée que CPU. Les lignes vivantes restent des sous-ensembles des voisins ; `popc(next_logical)-popc(next)` est donc non négatif. Le rang J2 est injectif sur C(32,3)=4960 triplets ; maximum4959, 155 mots de32bits, décalages valides. Les quinze champs locaux et les q4_candidates avant E4/q4_levels après admission correspondent à la voie CPU graphe, sous succès seulement. Aucun compteur partiel unresolved ne doit être ajouté au ledger final.

**Distinction de coût nouvelle :** `seen` conserve les hits/evaluations historiques, mais ne mémorise aucune relation. Même sur hit, `center_line_meets` est recalculé (`leaf_device.hpp:89–95`). Dans le petit modèle, sept demandes donnent quatre evaluations et trois hits **logiques**, mais sept calculs physiques. Deux passes appareil ajoutent encore leur travail réel. Ce n'est pas une faute de résultat ni de ledger contractuel ; ces champs ne doivent pas être présentés comme le compte d'appels géométriques GPU ou une économie de cache mesurée. Ajouter un compteur physique séparé ou mesurer le travail total permettrait de décider si mémoriser la relation est utile.

Le Level demeure sur l'hôte : S*==generated et qmin==arité de la présentation émise garantissent la même fabrique et les mêmes num/den bruts q2/q3/q4 (`leaf.cpp:385–414`, `num/sphere.cpp`). Cette capsule ne teste pas l'assembleur ni le protocole mémoire/transfert/fill CUDA, confiés à l'auditeur transport.

## Gardes et portes utiles

Le script indépendant résout les systèmes de Gram avec Fraction, puis compare centres, contacts, signes, poids, orientations et SAT à une droite paramétrique rationnelle. **145 391 gardes**, 408 présentations, 2 370 signes natifs autorisés, 7 368 orientations, 240 tests de poids q4, 324 boîtes J2 et six cas de canonique ; permutations et trois profils, sans aucun natif. Les 48 autres présentations q4 du modèle rencontrent la garde de cube et n'exécutent pas les poids natifs. Les maxima de magnitude observés sur les intermédiaires autorisés sont i64=50 bits et i128=124 bits ; ce ne sont pas des bornes générales remplaçant les preuves.

Portes G4 prioritaires pour le port : mêmes catalogue/populations/niveaux/ledger sur CPU historique, header hôte et CUDA ; u21/u24 q3 extrême avec toute sortie partielle discarded et repli entier ; cube q4 L=2^20 puis L+1 ; préfixe obtus ci-dessus ; coquille huit sites/qmin2 ; owner sur faces hi/lo ; cache on/off avec travail physique séparé. Un accord entre hôte et appareil partageant le même header vérifie le transport mais ne remplace pas l'oracle Fraction indépendant.

Replay portable, lecture seule :

```sh
python3 -B -S check_device_geometry.py
python3 -O -B -S check_device_geometry.py
python3 -B -S verify.py
python3 -O -B -S verify.py
```

Normal/−O : résultats JSON identiques octet pour octet, stderr vide. L'échec de lecture initial de `center_line_cache.cpp` absent (code1) est consigné dans `COMMANDS.json` ; le fichier réel est `center_line_cache.hpp`, copié et relu. Aucun résultat mathématique ne dépend de cette commande manquée. `SHA256SUMS` inventorie tous les fichiers réguliers sauf son seul fichier racine.
