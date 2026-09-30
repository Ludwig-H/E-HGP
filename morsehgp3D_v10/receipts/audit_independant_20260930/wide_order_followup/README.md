# Complément ciblé : ordre large, refus et coupe entière

Source figée : `408d1ffe4d90a7ee6393716c6ddc8319b2b77daa`.
Le comparateur 266/200 bits existant est utilisé à octets identiques.
Deux petits exécutables normal/UBSan, quatre jugements Python normal/−O et
un complément Fraction normal/−O passent. Les sorties natives sont identiques.
Les sept sources sont contrôlées avant/après ; compilations neuves dans `/tmp`.
Le panel antérieur de 2 444 requêtes n'est pas rejoué.

- **Refus avant tri.** Le prototype refuse correctement D=0, mais son
  `Answer.cmp` reste zéro lors d'un refus. Un futur adaptateur lisant ce
  zéro comme une égalité puis départageant par support crée un cycle réel :
  A=(niveau bas, ID2), B=(niveau haut, ID0), I=(D=0, ID1) donnent
  A<B, B<I, I<A. Les trois réponses sont vérifiées en natif ; ce comparateur
  cyclique n'est jamais passé à `std::sort`. Prévalider tous les descripteurs
  immuables ou propager le refus ; aucun refus ne vaut une égalité de niveaux.
  Ce risque concerne le raccord naïf, pas l'arithmétique protégée du prototype.
- **Collision géométrique et coupe fermée.** Dans un même nuage unitaire,
  A=(0,0,0), B=(M,0,0), C=(M,1,0), D=(M/2,0,0), M pair, les boules diamètre
  AB et AC ont pour niveaux E=M²/4 et E+1/4. Toutes deux ont p=1 et qmin=2 ;
  leurs coquilles comptent 2 et 3 sites : les deux sont admissibles à K3.
  E est aussi la vraie distance carrée AD. Pour M=2^31, les doubles des
  deux niveaux coïncident, tandis que la coupe exacte ≤E doit admettre AB
  et refuser AC. Le comparateur, un petit tri exact et `upper_bound`
  conservent cette distinction et les égalités rationnelles non réduites.
  La preuve Fraction contrôle également M=2^32−2 et la collision après
  conversion physique exacte h=0,1 mm. Aucun constructeur de catalogue
  large ni événement FULL n'est exécuté.
- **Pont KNN 66 bits.** Le seuil géométrique maximal
  e=3(2^32−1)²=55 340 232 195 358 851 075 dépasse u64.
  Avec D=2^200−1, les trois rationnels (eD−1)/D, eD/D, (eD+1)/D restent
  dans le domaine 266/200 et sont comparés à e/1 comme −1,0,+1.
  Leur numérateur est reconstruit indépendamment par Python, avec aucun
  échec de réduction transformé en vrai. Ces rationnels de bord ne sont
  pas présentés comme des rayons MEB ; e, lui, est une distance réelle u32.

Pour le palier u24, tout carré de distance est <3·2^48<2^50 ; son double
est exact, ainsi que tout niveau q2 obtenu par division par quatre.
Cette voie courte ne couvre ni les niveaux rationnels q3/q4 ni le profil
u32 complet. Conserver un rang exact commun aux niveaux et aux seuils KNN,
avec une vue double distincte, même si les exports métriques coïncident.

Le moteur actuel refuse toujours les coordonnées au-delà de u18.
Les fichiers [SOURCE_BEFORE.json](SOURCE_BEFORE.json),
[SOURCE_AFTER.json](SOURCE_AFTER.json) et [COMMANDS.json](COMMANDS.json)
identifient les sources et les commandes exactes ; [RESULT.json](RESULT.json)
donne la portée. Le [juge](judge.py) rejoue les quatre lignes natives sans
compiler ; [geometry_contract.py](geometry_contract.py) rejoue la preuve
d'incidence bornée. `record.py` écrit une capture : ne le relancer que dans
une copie de travail neuve. Pas de campagne, grosse allocation, GPU/GCP,
qualification de tri complet ou performance large.
