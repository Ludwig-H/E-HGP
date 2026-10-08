# T2-d-B : preuve de la proposition entière, et portée du support canonique

8 octobre 2026. Prototype B/git8da4 reporté sur `902041f66`, proposition `9a2260a5`,
résolveur et portes dans [pins.json](pins.json). Revue mathématique/source et oracle rationnel
Python seulement : aucune compilation, moteur, donnée LiDAR ni nouvelle mesure.

**La voie entière est correcte pour 2≤n≤12 sites distincts d’un même Cloud, en repère commun.** Soit (a,b) une paire
réalisant le diamètre D de la partie F. Pour m=(a+b)/2,

`(x−a)·(x−b) = ||x−m||² − D²/4`.

Si tous ces produits sont ≤0, la boule de diamètre [a,b] contient F. Toute boule contenant
a et b a rayon ≥D/2, donc celle-ci est la plus petite boule. En cas de diamètres ex æquo,
si une boule diamétrale contient F, chaque autre paire de diamètre D est antipodale dans
cette même boule : choisir la première paire par positions reste sûr. Si n=3 et le test
échoue, l'angle opposé au plus grand côté est aigu ; les deux autres le sont aussi.
Le triangle est non dégénéré et son cercle circonscrit a son centre strictement intérieur.
Pour n≥4, l'échec du test ne permet pas de choisir un triangle : le code passe au proposant
flottant, puis conserve la certification exacte existante.

**Largeur arithmétique.** Les écarts locaux et leurs différences représentent des différences
de coordonnées du nuage : valeur absolue ≤2^B−1, malgré une origine commune non nulle.
Une somme de trois carrés, ou la valeur absolue d'un produit scalaire, est bornée par
`3(2^B−1)²`. Elle tient dans i64 pour B≤30 ; B=31 peut dépasser 2^63−1.
Pour B=32 elle vaut au plus 55 340 232 195 358 851 075 (<2^66), donc i128 suffit.
Les soustractions intermédiaires sont en i64 et les produits sont élargis avant multiplication.
Le choix `ExactSquare = i64` jusqu'à 30, `i128` ensuite est donc adapté. Cela ne qualifie
pas les autres prédicats q3/q4 ni le moteur complet u32.

**Précision de contrat utile : le départage est local à F.** Sur les quatre sommets
(0,0,0), (0,2,0), (2,0,0), (2,2,0), le support global minimal par positions est la première
diagonale. Pour F réduit à l'autre diagonale, le support proposé est nécessairement différent,
alors que la boule est identique. Le commentaire « celle de S* » doit préciser cette portée :
[patch de commentaire](precision_support.patch), sans changement d'algorithme.

Le raccord lu préserve cette distinction : `propose` trie les SiteIdx ; `locate` essaie LEM-T1,
puis `certify_part`. Si la clé locale n'est pas dans la table globale, le census récupère toute
la coquille et appelle `canonical_support` avant la recherche catalogue. Aucun retrait de ce
census ni assimilation du support local à une identité globale n'est justifié par le levier.
Le contre-exemple porte sur le commentaire, pas sur un résultat moteur faux démontré.

[check.py](check.py) ne copie pas l'oracle C++ : il énumère les supports de cardinal ≤4,
résout leurs systèmes de Gram en Fraction, exige des poids barycentriques strictement
positifs puis la couverture de tous les points, et choisit le rayon minimal/canonique.
Sur 191 parties (grille, permutations d'un rectangle, 3D de 4 à 12 sites, extrêmes B21/24/30/31/32),
151 propositions de paire et 13 de triangle concordent ; 27 parties passent au flottant,
avec support exact de cardinal au moins trois. Le proposant DWelzl n'est pas rejoué ici.
Contrelecture indépendante de la preuve et de l’oracle favorable, sans nouveau rejeu natif.
Le carré ci-dessus confirme séparément la différence local/global.

Résultats normal/−O identiques : [results.json](results.json). Modèle et preuve complètent les
portes natives du développeur ; ils ne les remplacent pas et n'établissent aucun gain de G.

```sh
python3 -B check.py
python3 -B -O check.py
```
