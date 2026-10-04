# Contrelecture des enveloppes M3/E4

**Favorable, aucun défaut matériel trouvé.** Source épinglée `ec55578d994fda0dbf2f91f43f2f29204703642b`, acteur observé à `66372e621dcee58daaa7d7309875ab157894acf4`. Copies Git relatives sous `source/`, avec empreintes avant/après. Le fichier demandé `src/num/geometry.cpp` est absent et déclaré comme tel dans `SOURCE.json`, pas reconstruit. Aucun moteur, natif, build, fit ou G4 exécuté ; les résultats locaux annoncés par le développeur ne sont pas requalifiés ici.

## Preuve et raccord

**M3.** Pour les longueurs opposées a,b,c d'un triangle non dégénéré, poser `D=2(a²b²+b²c²+c²a²)-a⁴-b⁴-c⁴=16*aire²>0`. Le système de Gram du centre circonscrit donne `lambda_A=a²(b²+c²-a²)/D`. Les poids de ce même centre dans le triangle des milieux opposés sont `mu_A=1-2*lambda_A=(a²-b²+c²)(a²+b²-c²)/D`, et leurs analogues B/C. Ils somment à 1. L'acuité stricte rend chacun des deux facteurs positif : le centre est strictement intérieur au triangle médian et appartient à sa boîte englobante fermée. Ce résultat vaut aussi pour un triangle oblique dans R³ et pour une enveloppe plate suivant un axe.

[leaf.cpp](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L191) applique M3 seulement après `strictly_acute`. Le retour de `q3_of` rejette la présentation q3, pas ses prolongements : la récursion reste indépendante de l'acuité et de l'enveloppe à [la ligne 345](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L345). Un préfixe obtus utile à un q4 strict reste donc vivant.

**E4.** Un centre strictement intérieur au tétraèdre est une combinaison convexe strictement positive de ses quatre sommets ; leur enveloppe contient ce centre. Une enveloppe disjointe de la boîte propriétaire ne peut donc supprimer un ancien q4 admis. Pour un centre non strict, ce raisonnement n'est pas disponible, mais le filtre historique `q4_presentation_strictly_inside` reste actif. La nouvelle enveloppe utilise bien les **quatre** sommets et ne remplace pas ce filtre : [leaf.cpp](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L203).

**Faces.** L'enveloppe fermée `[low,high]` rencontre l'intervalle propriétaire `[lo,hi)` exactement quand `high>=lo` et `low<hi`. [doubled_envelope_meets](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L171) teste ces deux conditions après doublement entier exact. Une enveloppe plate sur la face basse reste admise ; une enveloppe entièrement sur la face haute reste exclue. Chaque axe peut être testé séparément puisque l'enveloppe est une boîte sur-ensemble du triangle ou du tétraèdre.

**Compteurs et domaine.** Le nouveau `orientation` vaut `(b-a)×(c-a)·(d-a)`, identique au produit mixte `u·(v×s)` de la factory q4. Une présentation non dégénérée incrémente `q4_candidates` **avant** E4, même si le centre est non strict ou l'enveloppe disjointe. Le compteur logique reste donc celui d'avant le port, bien que les constructions effectives puissent diminuer. `judged`, le census et les émissions ne sont atteints que pour les anciens centres stricts possédés, préservés par les deux preuves. La borne locale existante `m<=1024`, puis le vidage par `checked_add`, couvre toujours cette incrémentation.

Sous T0, `0<=XYZ<2^B` et `0<=lo<hi<=2^B`. Les sommes de deux coordonnées et les comparaisons à `2*lo/hi` restent sous `2^(B+2)`, donc en i64 pour B18/21/24. L'orientation conserve sa borne `<6*2^(3B)`, donc i128 au plus 75 bits de magnitude en B24. Aucune nouvelle allocation ni workspace persistant ; l'enveloppe est un tableau local constant d'au plus 4×3 i64. Ce contrôle ne requalifie pas les factories numériques existantes ni leurs refus natifs.

## Gardes autonomes et portes

`check.py` utilise Gram/Gauss et Fraction, sans imports produit. **7 552 gardes**, 369 tuples (permutations, triangles obliques bornés, profils extrêmes) et 4 207 comparaisons de décisions locales ; normal/−O identiques. 136 anciennes admissions sont conservées et 2 261 enveloppes disjointes sont constatées dans ce modèle ; ces nombres ne décrivent pas le travail d'un catalogue natif. Centres vérifiés : triangle plat `(2,2,1)`, tétraèdre non strict `(25,25,23/2)`, tétraèdre haut strict `(15,8,611/60)`. Ce dernier est perdu par l'enveloppe fautive des seuls trois sommets de base.

Les portes ajoutées dans [center_region.cpp](source/morsehgp3D_v11/tests/catalogue/center_region.cpp#L92) ciblent causalement : contact de l'enveloppe médiane sur une face basse ; q4 non dégénéré rejeté E4 mais encore compté ; q4 strict porté en hauteur uniquement par le quatrième sommet. Le groupe `median_envelope` est bien enregistré par `tests.cmake`. Les trois mutations ajoutées ciblent respectivement ces trois obligations, avec chaînes présentes dans le code. Leur compilation et leur mort natives restent des faits de qualification distincts. La porte existante `obtuse_region` conserve le raccord q3 obtus→q4.

Commandes portables, sans site-packages ni caches :

```
python3 -B -S check.py > normal.json
python3 -B -O -S check.py > optimized.json
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

`EXPECTED.json` grave statut, nombre de gardes et comptes du modèle. `SHA256SUMS` couvre exhaustivement chaque payload et exclut seulement lui-même. Les sources sont des objets Git immuables vérifiés après les contrôles. Ni identité de dumps FULL natifs ni gain chronométrique n'est acquis par ce reçu.
