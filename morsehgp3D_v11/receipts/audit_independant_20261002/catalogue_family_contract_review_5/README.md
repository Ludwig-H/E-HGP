# Certificats de familles de supports — propositions exactes

Les documents demandés comme WIP avaient déjà été publiés lors de leur
capture à `665dff6f336d7360da2004dbee062cf3b87c253b`. Les copies de trois
documents et trois fichiers catalogue, leurs empreintes et le delta depuis
f391 sont conservés ici. Ce delta adopte P4 à K fixé et la qualification
catalogue ; **il n'adopte pas encore les certificats de familles ci-dessous**.
La clôture consigne aussi l'observation du checkout développeur après
lecture, séparément des copies qui font autorité. Aucun produit modifié,
compilé ou exécuté, aucun appel cloud et aucun reçu antérieur rouvert.

## Feuille entièrement cosphérique

Soit L la liste K-certifiée d'une feuille Q, tous ses sites exactement sur
la sphère positive b=(c,R). Chaque quadruplet indépendant de L a l'unique
circumsphère b ; les quadruplets dépendants ne donnent aucun q4 strict.

- Si un support positif de b de cardinal 2 ou 3 a été trouvé, supprimer
  **toute l'énumération q4 de cette feuille** est sûr. Garder les q2/q3,
  leurs autres centres, et l'émission canonique de b lorsqu'elle est admise
  et appartient à Q. Une coquille seulement partielle ne certifie pas L.
- Plus généralement, tous les q4 de L forment **une seule famille de
  boule**. Si c est hors de Q, aucun n'est propriétaire et on peut tous
  les écarter. Sinon décider une fois le support canonique de b et son
  admission, émettre une seule fois, puis conserver tous les q2/q3 de
  centres différents. b peut aussi ne pas être critique : tous les sites
  d'une sphère situés dans un même hémisphère ouvert en donnent un exemple.
- Pour 1<=K<=n, si c appartient à la fermeture K-certifiée de Q,
  alors **I(b)=∅ et U(b)=L globalement**. Si p>=K,
  la certification impose K intérieurs dans L, contradiction. Si p<K,
  elle impose tous les intérieurs et toute la coquille dans L : aucun
  intérieur n'étant présent, p=0 et la coquille globale est exactement L.
  La certification cosphérique peut donc fournir directement I/U pour b,
  sans nouveau census, et l'admission est `q_min <= K+1`. L'émission exige
  en plus l'appartenance à Q demi-ouverte. Cette déduction globale est
  interdite hors de la fermeture certifiée. Pour K>n, le filtre effectif
  ne peut retirer aucun site : L=X, et la même conclusion est directe ;
  la convention des plus proches est alors min(K,n), avec tous les ex æquo.

Un détecteur possible cherche une base affine, de taille au plus quatre,
puis, en dimension trois, construit sa circumsphère et teste exactement
tous les m sites de L. La base n'a pas besoin d'être un support positif.
Ce passage O(m) ne s'applique qu'après réussite de chaque test, sans
limiter les coquilles ou modifier les critères de propriété.

## Coquille qmin4 : ancre du support canonique

Si b possède **dans L** un support q4 positif et aucun q2/q3, alors c est
intérieur à conv(L). Dans la branche propriétaire certifiée ci-dessus,
L est la coquille globale et ce cardinal est bien qmin4. Pour tout a dans L,
la demi-droite depuis a passant par c rencontre la frontière opposée en
y. Une face contenant y fournit une représentation par au plus trois
sites de L. La face ne contient pas a. Avec un ou deux sites, le segment
entre a et y donnerait un support de cardinal au plus trois, impossible.
On obtient donc trois sites formant avec a un tétraèdre indépendant,
dont les quatre poids de c sont strictement positifs. Tout a peut ainsi
appartenir à un support q4 de b ; son support canonique commence au plus
petit SiteIdx de la coquille complète.

Après exclusion exacte des q2/q3 de b, chercher seulement les triplets
complétant cette ancre décide donc existence et support canonique q4.
Avec le passage q2/q3, le coût reste O(m³) tests exacts, au lieu d'une
énumération de toutes les présentations q4 O(m⁴). Le
[tetra_support actuel](source/morsehgp3D_v11/src/catalogue/support.cpp#L62)
retourne déjà au premier tétraèdre strict et, sous ces hypothèses,
trouve celui d'indice initial zéro : **sa recherche canonique bénéficie
déjà de cette borne**. Le gain proposé concerne le DFS de toutes les
présentations q4, les canonicalisations et collectes répétées.

Ce lemme d'ancre ne s'étend pas à qmin3 en dimension trois. Sur la sphère
de centre (5,5,5), rayon5, placer en premier (5,5,10), puis (10,5,5),
(2,9,5), (2,1,5). La seule présentation positive de b est le triangle
des trois derniers sites ; le premier n'appartient à aucun tel triangle.

## Coupures de préfixes et d'extensions

Le rang affine donne un certificat supplémentaire : une feuille de
dimension au plus deux n'a aucun support q4 strict ; un préfixe de trois
sites collinéaires ne peut être prolongé en quadruplet indépendant.
Cela permet de couper la récursion avant les préfixes q4. Un triplet
seulement obtus reste disponible, comme dans le
[DFS actuel](source/morsehgp3D_v11/src/catalogue/leaf.cpp#L125).

Pour un triplet non collinéaire T, soit (c_T,r_T) sa circumsphère dans
son plan, même si T est obtus. Pour un quatrième site s hors du plan,
noter h sa hauteur signée et P=||s-c_T||²-r_T². Le centre de la
circumsphère du tétraèdre vaut `c_T + [P/(2h)] n`, avec n normal unitaire.
Son poids barycentrique associé à s vaut donc **P/(2h²)**. Un q4 strict
exige P>0. Les sites dans la boule fermée de T peuvent être exclus des
extensions avant construction du q4 ; si tous les sites restants le
sont, couper la famille entière. P>0 reste seulement nécessaire : les
autres poids doivent encore être vérifiés. Ce certificat est géométrique,
sans census au centre c_T ni exigence que c_T appartienne à Q.

## Vérifications et portée

[check.py](check.py) n'importe ni produit ni oracle développeur.
**110 gardes** passent en normal et -O, avec sorties identiques en moins
d'une seconde au total. Un témoin entier à cinq sites sur b=((3,3,3),3)
a qmin4 et deux présentations positives :
`(4,5,5),(4,1,1),(2,5,1),(2,1,5),(5,4,5)`.
Cinq permutations cycliques vérifient l'ancre et comparent le catalogue
complet, I/U inclus, aux supports exhaustifs pour K1..6. Les autres gardes
contrôlent le contre-cas qmin3, une famille non critique, les puissances
nulle/négative/positive, le préfixe obtus strict et la nécessité de Q.

Ce sont des preuves et un prototype rationnel borné, **pas un port natif
ni un gain mesuré**. La détection, les budgets numériques, la mémoire,
les refus et le coût des q2/q3 restent à qualifier. Ces certificats ne
suppriment pas à eux seuls le refus wide_leaf actuel. Ils préservent les
enregistrements de CatK et leurs incidences ; la qualification de FULL,
core et des projections exige encore leurs propres contrats et portes.

[RUN.json](RUN.json) garde les commandes et premières sorties.
`SOURCE_BEFORE.json`, `SOURCE_AFTER.json` et `SHA256SUMS` ferment les
copies et observations ; le lecteur vérifie le manifeste lorsqu'il existe.
